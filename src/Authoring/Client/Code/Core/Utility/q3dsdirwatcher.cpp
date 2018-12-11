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

#include "q3dsdirwatcher.h"
#include <QtCore/qdiriterator.h>

using namespace Q3DStudio;

Q3DSDirWatcher::Q3DSDirWatcher() : QObject(nullptr)
{
    createWatcher();
}

Q3DSDirWatcher::Q3DSDirWatcher(TCallbackCaller caller)
    : QObject(nullptr), m_callbackCaller(caller)
{
    Q_ASSERT(m_callbackCaller);
    createWatcher();
}

Q3DSDirWatcher::Q3DSDirWatcher(TCallbackCaller caller, const QString &path)
    : Q3DSDirWatcher(caller)
{
    init(path);
}

Q3DSDirWatcher::Q3DSDirWatcher(const Q3DSDirWatcher &other)
{
    createWatcher();
    assign(other);
}

Q3DSDirWatcher &Q3DSDirWatcher::operator=(const Q3DSDirWatcher &other)
{
    createWatcher();
    assign(other);
    return *this;
}

void Q3DSDirWatcher::assign(const Q3DSDirWatcher &other)
{
    init(other.m_root);
    m_callbackCaller = other.m_callbackCaller;
    m_callbacks = other.m_callbacks;
    m_recordCache = other.m_recordCache;
    m_records = other.m_records;
}

void Q3DSDirWatcher::init(const QString &path)
{
    addPathRecursively(path);
    m_root = QDir(path).path();
    m_files = m_watcher->files();
    m_directories = m_watcher->directories();
}

void Q3DSDirWatcher::addCallback(TFileModCallbackType &callback)
{
    TFileModificationList records;
    for (const QString &path : qAsConst(m_directories)) {
        if (path != m_root)
            records.push_back(getRecordFromCache(path, FileModificationType::Created));
    }
    for (const QString &path : qAsConst(m_files))
        records.push_back(getRecordFromCache(path, FileModificationType::Created));

    if (!m_callbacks.contains(&callback))
        m_callbacks.insert(&callback, callback);

    callback(records);
}

void Q3DSDirWatcher::removeCallback(TFileModCallbackType &callback)
{
    for (auto it = m_callbacks.begin(); it != m_callbacks.end();) {
        if (it.key() == &callback)
            it = m_callbacks.erase(it);
        else
            ++it;
    }
    //If no callbacks left, stop watching directories by restarting the watcher
    if (!hasCallbacks())
        createWatcher();
}

bool Q3DSDirWatcher::hasCallbacks() const
{
    return !m_callbacks.empty();
}

void Q3DSDirWatcher::directoryChanged(const QString &path)
{
    addPathRecursively(path);

    handleChanges();
    m_callbackCaller(std::bind(&Q3DSDirWatcher::sendRecords, this));
}

void Q3DSDirWatcher::fileChanged(const QString &path)
{
    addRecord(path, FileModificationType::Modified);

    handleChanges();
    m_callbackCaller(std::bind(&Q3DSDirWatcher::sendRecords, this));
}

void Q3DSDirWatcher::createWatcher()
{
    m_watcher = std::unique_ptr<QFileSystemWatcher>(new QFileSystemWatcher());
    QObject::connect(m_watcher.get(), &QFileSystemWatcher::directoryChanged,
                     this, &Q3DSDirWatcher::directoryChanged);
    QObject::connect(m_watcher.get(), &QFileSystemWatcher::fileChanged,
                     this, &Q3DSDirWatcher::fileChanged);
}

void Q3DSDirWatcher::addPathRecursively(const QString &path)
{
    QDir dir(path);
    m_watcher->addPath(dir.path());
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString nextPath = it.next();
        if (!nextPath.endsWith("."))
            m_watcher->addPath(nextPath);
    }
}

void Q3DSDirWatcher::addChanges(QStringList &to, const QStringList &from)
{
    const QStringList destroyed = QStringList::fromSet(to.toSet().subtract(from.toSet()));
    const QStringList created = QStringList::fromSet(from.toSet().subtract(to.toSet()));

    for (const QString &path : destroyed)
        addRecord(path, FileModificationType::Destroyed);

    for (const QString &path : created)
        addRecord(path, FileModificationType::Created);

    to = from;
}

template <typename T>
QStringList removeDeletedPaths(QFileSystemWatcher &watcher, const QStringList &paths)
{
    QStringList removed;
    for (const QString &str : paths) {
        T path(str);
        if (!path.exists())
            removed.push_back(str);
    }
    if (!removed.empty())
        return watcher.removePaths(removed);
    return QStringList();
}

void Q3DSDirWatcher::handleChanges()
{
    removeDeletedPaths<QFile>(*m_watcher, m_watcher->files());
    QStringList failures = removeDeletedPaths<QDir>(*m_watcher, m_watcher->directories());

    //It seems that QFileSystemWatcher doesn't want to remove renamed
    //directories from the watchlist if the directory contains files.
    //If this happens restart the whole watcher and repopulate the watchlist
    if (!failures.empty()) {
        createWatcher();
        addPathRecursively(m_root);
    }

    addChanges(m_directories, m_watcher->directories());
    addChanges(m_files, m_watcher->files());
}

void Q3DSDirWatcher::sendRecords()
{
    if (!m_records.empty()) {
        //A callback can cause the destruction of this object
        //so member variables shouldn't be accessed after a callback
        TFileModificationList records = m_records;
        m_records.clear();
        auto callbacks = m_callbacks;
        for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
            auto& callback = it.value();
            callback(records);
        }
    }
}

void Q3DSDirWatcher::addRecord(const QString &path, FileModificationType::Enum type)
{
    auto iter = std::find_if(m_records.begin(), m_records.end(),
                             [&](SFileModificationRecord &m) -> bool
                             { return m.m_File == path && m.m_ModificationType == type; });

    if (iter == m_records.end()) // add only unique entries
        m_records.push_back(getRecordFromCache(path, type));
}

SFileModificationRecord Q3DSDirWatcher::getRecordFromCache(const QString &path,
                                                           FileModificationType::Enum type)
{
    if (m_recordCache.contains(path)) {
        m_recordCache[path].m_ModificationType = type;
        return m_recordCache[path];
    }

    CFilePath filePath = CFilePath(CString::fromQString(path));
    SFileInfoFlags flags = filePath.GetFileFlags();
    SFileData data = filePath.GetFileData();
    m_recordCache.insert(path, SFileModificationRecord(filePath, flags, data, type));
    return m_recordCache[path];
}
