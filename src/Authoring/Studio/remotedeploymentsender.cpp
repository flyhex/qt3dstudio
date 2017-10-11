/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "remotedeploymentsender.h"

#include <QtCore/qpair.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qdiriterator.h>
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qdialogbuttonbox.h>

class ConnectionDialog : public QDialog
{
public:
    static QPair<QString, int> getInfo(QWidget *parent);

private:
    ConnectionDialog(QWidget *parent);
    QLineEdit *m_hostLineEdit = nullptr;
    QLineEdit *m_portLineEdit = nullptr;
};

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    m_hostLineEdit = new QLineEdit(this);
    QLabel *hostLabel = new QLabel(tr("Address:"));
    hostLabel->setBuddy(m_hostLineEdit);

    m_portLineEdit = new QLineEdit(this);
    m_portLineEdit->setText("36000");
    QLabel *portLabel = new QLabel(tr("Port:"));
    portLabel->setBuddy(m_portLineEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(m_hostLineEdit, 0, 1);
    mainLayout->addWidget(portLabel, 1, 0);
    mainLayout->addWidget(m_portLineEdit, 1, 1);
    mainLayout->addWidget(buttonBox, 3, 0, 1, 2);

    setWindowTitle(tr("Connect to Device"));
    m_hostLineEdit->setFocus();
}

QPair<QString, int> ConnectionDialog::getInfo(QWidget *parent)
{
    ConnectionDialog dialog(parent);
    if (!dialog.exec())
        return QPair<QString, int>();

    return qMakePair(dialog.m_hostLineEdit->text(),
                     dialog.m_portLineEdit->text().toInt());
}

RemoteDeploymentSender::RemoteDeploymentSender(QWidget *parent)
    : QObject(parent)
    , m_tcpSocket(0)
    , m_mainWindow(parent)
{
}

void RemoteDeploymentSender::connect()
{
    if (isConnected())
        return;

    delete m_tcpSocket;
    m_tcpSocket = new QTcpSocket(this);

    QObject::connect(m_tcpSocket, &QTcpSocket::connected, this,
                     &RemoteDeploymentSender::checkConnection);
    QObject::connect(m_tcpSocket, &QTcpSocket::disconnected, this,
                     &RemoteDeploymentSender::checkConnection);
    QObject::connect(m_tcpSocket,
                     static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>
                     (&QAbstractSocket::error),
                     this, &RemoteDeploymentSender::connectionError);

    QPair<QString, int> info = ConnectionDialog::getInfo(m_mainWindow);

    m_tcpSocket->connectToHost(info.first, info.second);
    if (!m_tcpSocket->waitForConnected(2000)) {
        m_tcpSocket->abort();
        connectionError();
    }
}

void RemoteDeploymentSender::disconnect()
{
    Q_ASSERT(m_tcpSocket);
    m_tcpSocket->disconnectFromHost();
}

bool RemoteDeploymentSender::isConnected() const
{
    return m_tcpSocket && m_tcpSocket->state()
            == QAbstractSocket::ConnectedState;
}

void RemoteDeploymentSender::checkConnection()
{
    Q_EMIT connectionChanged(isConnected());
}

void RemoteDeploymentSender::connectionError()
{
    if (m_tcpSocket) {
        QMessageBox::warning(m_mainWindow, tr("Connect to Device"),
                             tr("Device connection error: ") + m_tcpSocket->errorString());
    }
    Q_EMIT connectionChanged(isConnected());
}

void RemoteDeploymentSender::streamProject(const QString &projectFile)
{
    Q_ASSERT(isConnected());
    if (!isConnected())
        return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_8);

    const QFileInfo fileInfo(projectFile);
    if (!fileInfo.exists()) {
        qWarning() << "failed to find file " << projectFile;
        return;
    }

    const QDir projectDirectory(fileInfo.absolutePath());

    // The file to be loaded
    const QString relativePath
            = projectDirectory.relativeFilePath(fileInfo.filePath());

    int fileCount = 0;
    QDirIterator it(fileInfo.absolutePath(), QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "could not open file " << filePath;
            return;
        }

        fileCount++;
        const QString relativePath = projectDirectory.relativeFilePath(filePath);
        const QByteArray payload = file.readAll();
        out << relativePath;
        out << payload;
    }

    QByteArray metaBlock;
    QDataStream metaOut(&metaBlock, QIODevice::WriteOnly);
    metaOut.setVersion(QDataStream::Qt_5_8);
    metaOut << block.size();
    metaOut << fileCount;
    metaOut << relativePath;

    m_tcpSocket->write(metaBlock);
    m_tcpSocket->write(block);
}
