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

#ifndef Q3DSDIRWATCHER_H
#define Q3DSDIRWATCHER_H

#include <QtCore/qfilesystemwatcher.h>
#include "IDirectoryWatchingSystem.h"
#include "UICFileTools.h"

class Q3DSDirWatcher : public QObject
{
    Q_OBJECT

public:
    Q3DSDirWatcher();
    Q3DSDirWatcher(Q3DStudio::TCallbackCaller caller);
    Q3DSDirWatcher(Q3DStudio::TCallbackCaller caller, const QString &path);
    Q3DSDirWatcher(const Q3DSDirWatcher &other);
    Q3DSDirWatcher &operator=(const Q3DSDirWatcher &other);

    void addCallback(Q3DStudio::TFileModCallbackType &callback);
    void removeCallback(Q3DStudio::TFileModCallbackType &callback);
    bool hasCallbacks() const;

private Q_SLOTS:
    void directoryChanged(const QString &path);
    void fileChanged(const QString &path);

private:
    void init(const QString &path);
    void assign(const Q3DSDirWatcher &other);
    void createWatcher();
    void addPathRecursively(const QString &path);
    void addChanges(QStringList &to, const QStringList &from);
    void handleChanges();
    void sendRecords();
    void addRecord(const QString &path, Q3DStudio::FileModificationType::Enum type);
    Q3DStudio::SFileModificationRecord
    getRecordFromCache(const QString &path, Q3DStudio::FileModificationType::Enum type);

    std::unique_ptr<QFileSystemWatcher> m_watcher;
    QString m_root;
    QStringList m_directories;
    QStringList m_files;

    //TFileModCallbackType can't be compared so the pointer is needed for comparisons.
    //A copy is also needed so that calls can be made after the pointed object has been
    //destroyed. Pointed object is owned by Q3DSDirCallback.
    QMap<Q3DStudio::TFileModCallbackType *, Q3DStudio::TFileModCallbackType> m_callbacks;
    Q3DStudio::TCallbackCaller m_callbackCaller;
    QMap<QString, Q3DStudio::SFileModificationRecord> m_recordCache;
    Q3DStudio::TFileModificationList m_records;
};

#endif // Q3DSDIRWATCHER_H
