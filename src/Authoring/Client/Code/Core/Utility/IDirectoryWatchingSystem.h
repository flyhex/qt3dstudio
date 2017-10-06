/****************************************************************************
**
** Copyright (C) 2000 NVIDIA Corporation.
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

#pragma once
#ifndef IDIRECTORYWATCHINGSYSTEMH
#define IDIRECTORYWATCHINGSYSTEMH
#include <vector>
#include <QtCore/qstring.h>

namespace UICDM {
class ISignalConnection;
}

namespace Q3DStudio {
struct SFileModificationRecord;

typedef std::function<void(const std::vector<SFileModificationRecord> &)> TFileModCallbackType;
// A function that takes no arguments and returns nothing.
typedef std::function<void(void)> TCallbackFunc;
typedef std::function<void(TCallbackFunc)> TCallbackCaller;

// Wrap the file path system for finding directory differences with a nice wrapper that takes care
// of
// using an event to cut down on the number of times we query the file path API.
class IDirectoryWatchingSystem
{
protected:
    virtual ~IDirectoryWatchingSystem() {}
public:
    // Add this directory to the system.  Clients will get an *immediate* callback with all of the
    // files underneath this directory listed as created.  They will then get callbacks as files
    // change.
    // All of the callbacks, for now, are in the UI thread so you don't need to bounce off the
    // message router.
    // This will remain constant even if we move the query into another thread (which really isn't
    // likely).
    virtual std::shared_ptr<UICDM::ISignalConnection>
    AddDirectory(const QString &inDirectory, TFileModCallbackType inCallback) = 0;

    friend class std::shared_ptr<IDirectoryWatchingSystem>;
    /**
     *	Create a directory watcher that signals to listeners via the callback caller that things
     *have changed.
     *	The callback caller is expected to execute the provided callback func in the UI thread.
     */
    static std::shared_ptr<IDirectoryWatchingSystem>
    CreateThreadedDirectoryWatchingSystem(TCallbackCaller inCaller);
};
}
#endif
