/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets/qapplication.h>
#include <QtCore/qvector.h>

QT_FORWARD_DECLARE_CLASS(QSharedMemory)

namespace SharedTools {

class QtLocalPeer;

class QtSingleApplication : public QApplication
{
    Q_OBJECT

public:
    QtSingleApplication(const QString &id, int &argc, char **argv);
    ~QtSingleApplication();

    bool isRunning(qint64 pid = -1);

#if (defined Q_OS_MACOS)
    bool event(QEvent *event);
#endif

    QString applicationId() const;
    void setBlock(bool value);

    bool sendMessage(const QString &message, bool activateOnAccept = true,
                     int timeout = 5000, qint64 pid = -1);

    QVector<qint64> runningInstances() const;

Q_SIGNALS:
    void messageReceived(const QString &message, QObject *socket);
#if (defined Q_OS_MACOS)
    void fileOpenRequest(const QString &file);
#endif

private:
    QString instancesFileName(const QString &m_appId);

    qint64 m_firstPeer;
    QSharedMemory *m_instances;
    QtLocalPeer *m_pidPeer;
    QString m_appId;
    bool m_block;
    QVector<qint64> m_peers;
};

} // namespace SharedTools
