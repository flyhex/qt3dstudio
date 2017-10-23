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

#ifndef REMOTEDEPLOYMENTSENDER_H
#define REMOTEDEPLOYMENTSENDER_H

#include <QtCore/qobject.h>
#include <QtCore/qdatetime.h>
#include <QtWidgets/qwidget.h>
#include <QtNetwork/qtcpsocket.h>

class QMessageBox;

class RemoteDeploymentSender : public QObject
{
    Q_OBJECT
public:
    explicit RemoteDeploymentSender(QWidget *parent);
    ~RemoteDeploymentSender();

    QPair<QString, int> initConnection();
    void connect(const QPair<QString, int> &info);
    void disconnect();
    bool isConnected() const;

    void streamProject(const QString &);

public Q_SLOTS:
    void checkConnection();
    void connectionError();

Q_SIGNALS:
    void connectionChanged(bool) const;

private:
    QTcpSocket *m_tcpSocket;
    QWidget *m_mainWindow;
    QMessageBox *m_connectionError;
    QDateTime m_lastUpdate;
    QString m_projectFile;
};

#endif // REMOTEDEPLOYMENTSENDER_H
