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
#include "stdafx.h"
#include "Thread.h"
#include "ThreadException.h"

#ifndef WIN32
#include <unistd.h>
#endif

class FunctionThread : public QThread
{
public:
    FunctionThread(unsigned long (*function)(void* param), void* param)
        : m_function(function),
          m_param(param)
    {
    }

protected:
    void run() override
    {
        m_function(m_param);
    }

private:
    unsigned long (*m_function)(void* param);
    void* m_param;
};

//=============================================================================
/**
 * Windows thread entry.
 */
unsigned long ThreadRun(void *args)
{
    try {
        CThread *theThread = (CThread *)args;
        theThread->Run(NULL);
        if (theThread->IsAutoDelete())
            delete theThread;
        return 0;
    } catch (std::exception &) {
        return 0xFFFFFFFF;
    }
}

//=============================================================================
/**
 * Creates a new thread.
 * @param inRunnable the class to be run.
 * @param inArgs the arguments for the class to be run.
 * @param inStartImmediately true if this is supposed to be started immediately.
 */
CThread::CThread(CRunnable *inRunnable, const Q3DStudio::CString &inThreadName, void *inArgs,
                 bool inStartImmediately)
    : m_Thread(nullptr)
    , m_Runnable(inRunnable)
    , m_IsActive(false)
    , m_DeleteOnDeath(false)
    , m_Args(inArgs)
    , m_ThreadID(0)
    , m_ThreadName(inThreadName)
{
    if (inStartImmediately)
        Start();
}

//=============================================================================
/**
 * Destructor, nothing special.
 */
CThread::~CThread()
{
    if (m_Thread != nullptr) {
        delete m_Thread;
        m_Thread = nullptr;
    }
    m_ThreadID = 0;
}

//=============================================================================
/**
 * If this is not started immediately (see constructor) then this will start the thread.
 */
unsigned long CThread::Start()
{
    // SDJ 2/15/2005 User Story
    // Convert this to beginthreadex to prevent memory leaks from the c runtimes

    m_Thread = new FunctionThread(&ThreadRun, this);
    m_Thread->setObjectName(m_ThreadName.toQString());
    m_Thread->start();

    return m_ThreadID;
}

//=============================================================================
/**
 * Entry location for the executing thread.
 * Calling this directly will result in synchronous execution.
 */
void CThread::Run(void *inArgs)
{
    Q_UNUSED(inArgs);

    m_IsActive = true;
    m_Runnable->Run(m_Args);
    m_IsActive = false;
}

//=============================================================================
/**
 * Blocking call until this thread exits.
 */
void CThread::Join()
{
    // Only do this if run was called!!!
    if (m_Thread != nullptr) {
        bool nret = m_Thread->wait();
        if (nret == false)
            throw CThreadException("Thread.Join: join failed");
    }
}

//=============================================================================
/**
 * Check to see if this thread is currently executing.
 */
bool CThread::IsActive()
{
    return m_IsActive;
}

//=============================================================================
/**
 * Get the platform specific thread descriptor.
 */
CThread::TThread CThread::GetThread()
{
    return m_Thread;
}

//=============================================================================
/**
 * Get the unique ID of the specified thread.
 */
unsigned long CThread::GetThreadID()
{
    return m_ThreadID;
}

//=============================================================================
// Static Methods
//=============================================================================

//=============================================================================
/**
 * Cause the currently executing thread to pause for inTime.
 * @param inTime the amount to pause for, in milliseconds.
 */
void CThread::Sleep(unsigned long inTime)
{
    QThread::msleep(inTime);
}

//=============================================================================
/**
 * Return the thread handle for the currently executing thread.
 */
CThread::TThread CThread::GetCurrentThread()
{
    return QThread::currentThread();
}

//=============================================================================
/**
 * Gets the unique ID of the current thread.
 */
unsigned long CThread::GetCurrentThreadID()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    return QThread::currentThreadId();
#endif
    return {};
}

//=============================================================================
/**
 *	Gets the number of milliseconds spent in the current thread.
 *	This does not include sleeping or being interrupted by other threads.
 *	@note Only the Win32 portion is implemented and is only accurate to 10ms.
 *	@return CPU time spent in the thread in ms
 */
long CThread::GetUserTime()
{
    long theTime = 0;

#ifdef KDAB_TEMPORARILY_REMOVED
    unsigned __int64 theBigUser;
    unsigned __int64 theBigKernel;
    FILETIME theStart;
    FILETIME theStop;
    FILETIME theKernel;
    FILETIME theUser;
    BOOL theOK;

    // Win32 call to get the usertime
    theOK = ::GetThreadTimes(::GetCurrentThread(), &theStart, &theStop, &theKernel, &theUser);

    // Convert from FILETIME to 64bit integer
    theBigUser = theUser.dwHighDateTime;
    theBigUser <<= 32;
    theBigUser |= theUser.dwLowDateTime;

    theBigKernel = theKernel.dwHighDateTime;
    theBigKernel <<= 32;
    theBigKernel |= theKernel.dwLowDateTime;

    // Convert from 100ns units to 1ms units
    theTime = static_cast<long>((theBigUser + theBigKernel) / 10000);
#endif

    return theTime;
}
