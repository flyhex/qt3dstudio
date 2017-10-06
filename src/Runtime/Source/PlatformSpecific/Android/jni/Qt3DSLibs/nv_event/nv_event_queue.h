/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
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

#ifndef QT3DS_EVENT_QUEUE
#define QT3DS_EVENT_QUEUE

#include "nv_event.h"
#include <pthread.h>

class NVEventSync
{
public:
    pthread_cond_t m_cond;
    bool m_block;
};

class NVEventQueue
{
public:
    void Init();
    void Shutdown();
    void Flush();
    void UnblockConsumer();
    void UnblockProducer();
    // Events are copied, so caller can reuse ev immediately
    void Insert(const NVEvent *ev);
    // Waits until the event is consumed.  Returns whether the
    // consumer indicates handling the event or ignoring it
    bool InsertBlocking(const NVEvent *ev);

    // Returned event is valid only until the next call to
    // RemoveOldest or until a call to DoneWithEvent
    // Calling RemoveOldest again without calling DoneWithEvent
    // indicates that the last event returned was NOT handled, and
    // thus InsertNewestAndWait for that even would return false
    const NVEvent *RemoveOldest(int waitMSecs);

    // Indicates that all processing of the last event returned
    // from RemoveOldest is complete.  Also allows the app to indicate
    // whether it handled the event or not.
    // Do not dereference the last event pointer after calling this function
    void DoneWithEvent(bool ret);

protected:
    bool insert(const NVEvent *ev);

    enum { QUEUE_ELEMS = 256 };
    enum { QUEUE_MASK = 0x000000ff };

    int32_t m_nextInsertIndex;
    int32_t m_headIndex;

    pthread_mutex_t m_accessLock;

    NVEventSync m_blockerSync;
    NVEventSync m_consumerSync;

    NVEvent m_events[QUEUE_ELEMS];
    const NVEvent *m_blocker;
    enum BlockerState { NO_BLOCKER, PENDING_BLOCKER, PROCESSING_BLOCKER, RETURNED_BLOCKER };
    BlockerState m_blockerState;
    bool m_blockerReturnVal;
};

#endif // #ifndef QT3DS_EVENT_QUEUE
