/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "q3dsdirsystem.h"
#include "q3dsdirwatcher.h"
#include "Qt3DSDMSignals.h"

using namespace qt3dsdm;
using namespace Q3DStudio;

class Q3DSDirCallback : public ISignalConnection
{
public:
    Q3DSDirCallback(QMap<QString, Q3DSDirWatcher> &watchers, const QString& path,
                    TFileModCallbackType callback)
        : m_watchers(watchers), m_path(path), m_callback(callback)
    {
        if (m_watchers.contains(m_path))
            m_watchers[m_path].addCallback(m_callback);
    }

    ~Q3DSDirCallback()
    {
        if (m_watchers.contains(m_path))
            m_watchers[m_path].removeCallback(m_callback);
    }

private:
    QMap<QString, Q3DSDirWatcher> &m_watchers;
    QString m_path;
    TFileModCallbackType m_callback;
};

Q3DSDirSystem::Q3DSDirSystem(TCallbackCaller caller) : m_callbackCaller(caller) {}

std::shared_ptr<ISignalConnection> Q3DSDirSystem::AddDirectory(const QString &directory,
                                                                 TFileModCallbackType callback)
{
    QString path = directory;

    if (path.endsWith("\\") || path.endsWith("/"))
        path = path.left(path.size() - 1);

    for (auto it = m_watchers.begin(); it != m_watchers.end();) {
        if (!it->hasCallbacks())
            it = m_watchers.erase(it);
        else
            ++it;
    }

    if (!m_watchers.contains(path))
        m_watchers.insert(path, Q3DSDirWatcher(m_callbackCaller, path));

    return std::make_shared<Q3DSDirCallback>(m_watchers, path, callback);
}

std::shared_ptr<IDirectoryWatchingSystem>
IDirectoryWatchingSystem::CreateThreadedDirectoryWatchingSystem(TCallbackCaller caller)
{
    return std::make_shared<Q3DSDirSystem>(caller);
}
