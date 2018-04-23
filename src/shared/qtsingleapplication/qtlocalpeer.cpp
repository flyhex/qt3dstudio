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

#include "qtlocalpeer.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qdatetime.h>

#if defined(Q_OS_WIN)
#include <QtCore/qlibrary.h>
#include <QtCore/qt_windows.h>
typedef BOOL(WINAPI *PProcessIdToSessionId)(DWORD, DWORD *);
static PProcessIdToSessionId pProcessIdToSessionId = nullptr;
#endif

#if defined(Q_OS_UNIX)
#include <time.h>
#include <unistd.h>
#endif

namespace SharedTools {

// All reply strings must be of same length
static const QByteArray acceptReplyStr = QByteArrayLiteral("@A@");
static const QByteArray denyReplyStr = QByteArrayLiteral("@D@");

QString QtLocalPeer::appSessionId(const QString &appId)
{
    QByteArray idc = appId.toUtf8();
    quint16 idNum = qChecksum(idc.constData(), idc.size());

    QString res = QLatin1String("qtsingleapplication-")
                 + QString::number(idNum, 16);
#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId) {
        QLibrary lib(QLatin1String("kernel32"));
        pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId) {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
        res += QLatin1Char('-') + QString::number(sessionId, 16);
    }
#else
    res += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif
    return res;
}

QByteArray QtLocalPeer::acceptReply()
{
    return acceptReplyStr;
}

QByteArray QtLocalPeer::denyReply()
{
    return denyReplyStr;
}

QtLocalPeer::QtLocalPeer(QObject *parent, const QString &appId)
    : QObject(parent), m_id(appId)
{
    if (m_id.isEmpty())
        m_id = QCoreApplication::applicationFilePath();

    m_socketName = appSessionId(m_id);
    m_server = new QLocalServer(this);
    QString lockName = QDir(QDir::tempPath()).absolutePath()
                       + QLatin1Char('/') + m_socketName
                       + QLatin1String("-lockfile");
    m_lockFile.setFileName(lockName);
    m_lockFile.open(QIODevice::ReadWrite);
}

bool QtLocalPeer::isClient()
{
    if (m_lockFile.isLocked())
        return false;

    if (!m_lockFile.lock(QtLockedFile::WriteLock, false))
        return true;

    if (!QLocalServer::removeServer(m_socketName))
        qWarning("QtSingleCoreApplication: could not cleanup socket");
    bool res = m_server->listen(m_socketName);
    if (!res) {
        qWarning("QtSingleCoreApplication: listen on local socket failed, %s",
                 qPrintable(m_server->errorString()));
    }
    QObject::connect(m_server, &QLocalServer::newConnection,
                     this, &QtLocalPeer::receiveConnection);
    return false;
}

bool QtLocalPeer::sendMessage(const QString &message, int timeout, bool block)
{
    if (!isClient())
        return false;

    QLocalSocket socket;
    bool connOk = false;
    for (int i = 0; i < 2; i++) {
        // Try twice, in case the other instance is just starting up
        socket.connectToServer(m_socketName);
        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
            break;
        int ms = 250;
#if defined(Q_OS_WIN)
        Sleep(DWORD(ms));
#else
        struct timespec ts = {ms / 1000, (ms % 1000) * 1000 * 1000};
        nanosleep(&ts, NULL);
#endif
    }
    if (!connOk)
        return false;

    QByteArray uMsg(message.toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());
    bool res = socket.waitForBytesWritten(timeout);
    res &= socket.waitForReadyRead(timeout); // wait for reply
    QByteArray reply = socket.read(acceptReplyStr.size());
    res &= reply == acceptReplyStr;
    if (block) // block until peer disconnects
        socket.waitForDisconnected(-1);
    return res;
}

void QtLocalPeer::receiveConnection()
{
    QLocalSocket* socket = m_server->nextPendingConnection();
    if (!socket)
        return;

    // Why doesn't Qt have a blocking stream that takes care of this?
    while (socket->bytesAvailable() < static_cast<int>(sizeof(quint32))) {
        if (!socket->isValid()) // stale request
            return;
        socket->waitForReadyRead(1000);
    }
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));
    if (got < 0) {
        qWarning() << "QtLocalPeer: Message reception failed" << socket->errorString();
        delete socket;
        return;
    }
    QString message = QString::fromUtf8(uMsg.constData(), uMsg.size());
    // messageReceived handler must write the reply string
    emit messageReceived(message, socket); // might take a long time to return
}

} // namespace SharedTools
