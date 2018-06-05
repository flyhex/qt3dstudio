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

#include "qtsingleapplication.h"
#include "qtlocalpeer.h"

#include "qtlockedfile.h"

#include <QtCore/qdir.h>
#include <QtCore/qsharedmemory.h>
#include <QtWidgets/qwidget.h>
#include <QtGui/qevent.h>

#if defined(Q_OS_WIN)
#include <QtCore/qlibrary.h>
#include <QtCore/qt_windows.h>
typedef BOOL(WINAPI *allowSetForegroundWindow)(DWORD);
static allowSetForegroundWindow pAllowSetForegroundWindow = nullptr;
#endif

namespace SharedTools {

static const int instancesSize = 1024;

static QString instancesLockFilename(const QString &appSessionId)
{
    const QChar slash(QLatin1Char('/'));
    QString res = QDir::tempPath();
    if (!res.endsWith(slash))
        res += slash;
    return res + appSessionId + QLatin1String("-instances");
}

QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
    : QApplication(argc, argv),
      m_firstPeer(-1),
      m_pidPeer(0)
{
    this->m_appId = appId;

    const QString appSessionId = QtLocalPeer::appSessionId(appId);

    // This shared memory holds a zero-terminated array of active (or crashed) instances
    m_instances = new QSharedMemory(appSessionId, this);
    m_block = false;

    // First instance creates the shared memory, later instances attach to it
    const bool created = m_instances->create(instancesSize);
    if (!created) {
        if (!m_instances->attach()) {
            qWarning() << "Failed to initialize instances shared memory: "
                       << m_instances->errorString();
            delete m_instances;
            m_instances = 0;
            return;
        }
    }

    // QtLockedFile is used to workaround QTBUG-10364
    QtLockedFile lockfile(instancesLockFilename(appSessionId));

    lockfile.open(QtLockedFile::ReadWrite);
    lockfile.lock(QtLockedFile::WriteLock);
    qint64 *pids = static_cast<qint64 *>(m_instances->data());
    if (!created) {
        // Find the first instance that it still running
        // The whole list needs to be iterated in order to append to it
        for (; *pids; ++pids) {
            if (isRunning(*pids)) {
                m_peers.append(*pids);
                if (m_firstPeer == -1)
                    m_firstPeer = *pids;
            }
        }
    }
    // Add current pid to list and terminate it
    *pids++ = QCoreApplication::applicationPid();
    *pids = 0;
    m_pidPeer = new QtLocalPeer(this, appId + QLatin1Char('-') +
                                QString::number(QCoreApplication::applicationPid()));
    connect(m_pidPeer, &QtLocalPeer::messageReceived,
            this, &QtSingleApplication::messageReceived);
    m_pidPeer->isClient();
    lockfile.unlock();

#if defined(Q_OS_WIN)
    if (!pAllowSetForegroundWindow) {
        QLibrary lib(QLatin1String("user32"));
        pAllowSetForegroundWindow =
                (allowSetForegroundWindow)lib.resolve("AllowSetForegroundWindow");
    }
#endif
}

QtSingleApplication::~QtSingleApplication()
{
    if (!m_instances)
        return;
    const qint64 appPid = QCoreApplication::applicationPid();
    QtLockedFile lockfile(instancesLockFilename(QtLocalPeer::appSessionId(m_appId)));
    lockfile.open(QtLockedFile::ReadWrite);
    lockfile.lock(QtLockedFile::WriteLock);
    // Rewrite array, removing current pid and previously crashed ones
    qint64 *pids = static_cast<qint64 *>(m_instances->data());
    qint64 *newpids = pids;
    for (; *pids; ++pids) {
        if (*pids != appPid && isRunning(*pids))
            *newpids++ = *pids;
    }
    *newpids = 0;
    lockfile.unlock();
}

#if (defined Q_OS_MACOS)
bool QtSingleApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *foe = static_cast<QFileOpenEvent *>(event);
        emit fileOpenRequest(foe->file());
        return true;
    }
    return QApplication::event(event);
}
#endif

bool QtSingleApplication::isRunning(qint64 pid)
{
    if (pid == -1) {
        pid = m_firstPeer;
        if (pid == -1)
            return false;
    }

    QtLocalPeer peer(this, m_appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.isClient();
}

bool QtSingleApplication::sendMessage(const QString &message, bool activateOnAccept,
                                      int timeout, qint64 pid)
{
    if (pid == -1) {
        pid = m_firstPeer;
        if (pid == -1)
            return false;
    }

#if defined(Q_OS_WIN)
    if (activateOnAccept && pAllowSetForegroundWindow) {
        // In windows it is necessary to specifically allow bringing another application to
        // foreground. Otherwise it will just blink in the task bar.
        pAllowSetForegroundWindow(pid);
    }
#else
    Q_UNUSED(activateOnAccept)
#endif

    QtLocalPeer peer(this, m_appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.sendMessage(message, timeout, m_block);
}

QString QtSingleApplication::applicationId() const
{
    return m_appId;
}

void QtSingleApplication::setBlock(bool value)
{
    m_block = value;
}

QVector<qint64> QtSingleApplication::runningInstances() const
{
    // Note that this is a list of running instances at the time of application startup,
    // so it should only be used at application startup.
    return m_peers;
}

} // namespace SharedTools
