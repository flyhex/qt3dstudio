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

#ifndef INCLUDED_CONDITIONAL_H
#define INCLUDED_CONDITIONAL_H

#pragma once

#include "ThreadException.h"

#include <QMutex>
#include <QWaitCondition>

class CConditional
{
public:
    CConditional();
    ~CConditional();

    void Wait();
    bool Wait(unsigned long inTimeout);
    void Notify();

    static const unsigned long WAITINFINITE = ULONG_MAX;

private:
    CConditional(const CConditional &inConditional);
    CConditional &operator=(const CConditional &inConditional);

    QWaitCondition* m_Event;
};

//=============================================================================
/**
 * Create a new conditional.
 * This will wait for the Mutex to become available then lock it when
 * it becomes avaialable.
 */
inline CConditional::CConditional()
{
    m_Event = new QWaitCondition;
}

inline CConditional::~CConditional()
{
    delete m_Event;
}

//=============================================================================
/**
 * Wait for this conditional event to occur.
 * The mutex should be locked entering this function and will be locked on
 * exit. The thread is blocked until Notify is called by another thread. This
 * is most commonly used for queues that need to block if no data is available.
 * Intended useage:
 * <code>
 * mutex.Lock( );
 * while( [condition] )
 *   conditional.Wait( mutex );
 * [process condition]
 * </code>
 */
inline void CConditional::Wait()
{
    QMutex m;
    m.lock();
    m_Event->wait(&m, WAITINFINITE);
    m.unlock();
}

//=============================================================================
/**
 * Put the thread to sleep for inTimeout or until notified.
 * This is used to provide interuptable sleeps.
 * @param inTimeout the timeout for the wait, in milliseconds.
 * @return true if the wait succeeded, false if timeout.
 */
inline bool CConditional::Wait(unsigned long inTimeout)
{
    QMutex m;
    m.lock();
    const bool result =  m_Event->wait(&m, inTimeout);
    m.unlock();
    return result;
}

//=============================================================================
/**
 * Notify any threads waiting for this condition that it has occurred.
 */
inline void CConditional::Notify()
{
    m_Event->wakeAll();
}
#endif // INCLUDED_CONDITIONAL_H
