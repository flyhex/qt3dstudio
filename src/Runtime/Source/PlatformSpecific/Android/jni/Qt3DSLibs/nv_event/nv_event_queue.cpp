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

#define MODULE "NVEventQueue"
#define DBG_DETAILED 0
#include "../nv_debug.h"

#include "nv_event_queue.h"
#include <assert.h>
#include <stdlib.h>
#include <jni.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <android/log.h>

#define NVNextWrapped(index) (((index) + 1) & QUEUE_MASK)
#define NVPrevWrapped(index) (((index)-1) & QUEUE_MASK)

/* you must be inside a m_accessLock lock to invoke this! */
static void unlockAll(NVEventSync *sem)
{
    sem->m_block = false;
    pthread_cond_broadcast(&(sem->m_cond));
}

static int32_t pthread_cond_timeout(pthread_cond_t *cond, pthread_mutex_t *mutex, unsigned waitMS)
{
    struct timespec ts;
    int32_t status;

    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec += (waitMS / 1000);
    ts.tv_nsec += ((waitMS % 1000) * 1000000);

    status = pthread_cond_timedwait(cond, mutex, &ts);

    if (status == ETIMEDOUT)
        return ETIMEDOUT;

    return 0;
}

/* you must be inside mutex lock to invoke this! */
static int32_t wait(NVEventSync *sem, pthread_mutex_t *mutex, int waitMS)
{
    // TBD - spec is dodgy; do we definitely release the mutex even if
    // wait fails?
    if (sem->m_block) {
        if (waitMS < 0) {
            return pthread_cond_wait(&sem->m_cond, mutex);
        } else {
            return pthread_cond_timeout(&sem->m_cond, mutex, (unsigned)waitMS);
        }
    } else {
        // must release this, as failure assumes no lock
        pthread_mutex_unlock(mutex);
        return 1; // do not return 0 - we do not own the lock!
    }
}

static void signal(NVEventSync *sem)
{
    pthread_cond_signal(&sem->m_cond);
}

static void broadcast(NVEventSync *sem)
{
    pthread_cond_broadcast(&sem->m_cond);
}

static void syncInit(NVEventSync *sync)
{
    pthread_cond_init(&(sync->m_cond), NULL);
    sync->m_block = true;
}

static void syncDestroy(NVEventSync *sync)
{
    pthread_cond_destroy(&sync->m_cond);
}

/* you must be inside a m_accessLock lock to invoke this! */
bool NVEventQueue::insert(const NVEvent *ev)
{
    // Is the queue full?
    int32_t nextNext = NVNextWrapped(m_nextInsertIndex);
    if (nextNext == m_headIndex) {
        // TBD - what to do when we cannot insert (full queue)
        return false;
    }

    NVEvent *evDest = m_events + m_nextInsertIndex;
    memcpy(evDest, ev, sizeof(NVEvent));

    m_nextInsertIndex = nextNext;
    return true;
}

void NVEventQueue::Init()
{
    m_nextInsertIndex = 0;
    m_headIndex = 0;
    pthread_mutex_init(&(m_accessLock), NULL);
    syncInit(&m_consumerSync);
    syncInit(&m_blockerSync);

    m_blocker = NULL;
    m_blockerState = NO_BLOCKER;
    m_blockerReturnVal = false;
}

void NVEventQueue::Shutdown()
{
    pthread_mutex_destroy(&(m_accessLock));

    // free everyone...
    unlockAll(&m_consumerSync);
    unlockAll(&m_blockerSync);
    syncDestroy(&(m_consumerSync));
    syncDestroy(&(m_blockerSync));
}

void NVEventQueue::Flush()
{
    // TBD: Lock the mutex????
    m_headIndex = m_nextInsertIndex;
}

void NVEventQueue::UnblockConsumer()
{
    unlockAll(&(m_consumerSync));
}

void NVEventQueue::UnblockProducer()
{
    unlockAll(&(m_blockerSync));
}

void NVEventQueue::Insert(const NVEvent *ev)
{
    pthread_mutex_lock(&(m_accessLock));

    // insert the event and unblock a waiter
    insert(ev);
    signal(&m_consumerSync);
    pthread_mutex_unlock(&(m_accessLock));
}

bool NVEventQueue::InsertBlocking(const NVEvent *ev)
{
    // TBD - how to handle the destruction of these mutexes

    pthread_mutex_lock(&(m_accessLock));
    while (m_blocker) {
        if (wait(&(m_blockerSync), &(m_accessLock), QT3DS_EVENT_WAIT_FOREVER))
            return false;
    }

    assert(!m_blocker);
    assert(m_blockerState == NO_BLOCKER);

    // we have the mutex _and_ the blocking event is NULL
    // So now we can push a new one
    m_blocker = ev;
    m_blockerState = PENDING_BLOCKER;

    // Release the consumer, as we just posted a new event
    signal(&(m_consumerSync));

    // Loop on the condition variable until we find out that
    // there is a return value waiting for us.  Since only we
    // will null the blocker pointer, we will not let anyone
    // else start to post a blocking event
    while (m_blockerState != RETURNED_BLOCKER) {
        if (wait(&(m_blockerSync), &(m_accessLock), QT3DS_EVENT_WAIT_FOREVER))
            return false;
    }

    bool handled = m_blockerReturnVal;
    m_blocker = NULL;
    m_blockerState = NO_BLOCKER;

    DEBUG("producer unblocking from consumer handling blocking event (%s)",
          handled ? "true" : "false");

    // We've handled the event, so the producer can release the
    // next thread to potentially post a blocking event
    signal(&(m_blockerSync));
    pthread_mutex_unlock(&(m_accessLock));

    return handled;
}

const NVEvent *NVEventQueue::RemoveOldest(int waitMSecs)
{
    pthread_mutex_lock(&(m_accessLock));

    // Hmm - the last event we got from RemoveOldest was a
    // blocker, and DoneWithEvent not called.
    // Default to "false" as a return value
    if (m_blockerState == PROCESSING_BLOCKER) {
        m_blockerReturnVal = false;
        m_blockerState = RETURNED_BLOCKER;
        broadcast(&(m_blockerSync));
    }

    // Blocker is waiting - return it
    // And push the blocker pipeline forward
    if (m_blockerState == PENDING_BLOCKER) {
        m_blockerState = PROCESSING_BLOCKER;
        const NVEvent *ev = m_blocker;
        pthread_mutex_unlock(&(m_accessLock));

        return ev;
    } else if (m_nextInsertIndex == m_headIndex) {
        // We're empty - so what do we do?
        if (waitMSecs == 0) {
            goto no_event;
        } else {
            // wait for the specified time
            wait(&(m_consumerSync), &(m_accessLock), (unsigned)waitMSecs);
        }

        // check again after exiting cond waits, either we had a timeout
        if (m_blockerState == PENDING_BLOCKER) {
            m_blockerState = PROCESSING_BLOCKER;
            const NVEvent *ev = m_blocker;
            pthread_mutex_unlock(&(m_accessLock));

            return ev;
        } else if (m_nextInsertIndex == m_headIndex) {
            goto no_event;
        }
    }

    {
        // One way or another, we have an event...
        const NVEvent *ev = m_events + m_headIndex;
        m_headIndex = NVNextWrapped(m_headIndex);

        pthread_mutex_unlock(&(m_accessLock));
        return ev;
    }

no_event:
    pthread_mutex_unlock(&(m_accessLock));
    return NULL;
}

void NVEventQueue::DoneWithEvent(bool ret)
{
    // We only care about blockers for now.
    // All other events just NOP
    pthread_mutex_lock(&(m_accessLock));
    if (m_blockerState == PROCESSING_BLOCKER) {
        m_blockerReturnVal = ret;
        m_blockerState = RETURNED_BLOCKER;
        broadcast(&(m_blockerSync));
    }
    pthread_mutex_unlock(&(m_accessLock));
}
