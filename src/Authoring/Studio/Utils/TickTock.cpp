/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#include "Qt3DSCommonPrecompile.h"
#include "ITickTock.h"
#include "Qt3DSDMSignals.h"
#include "Thread.h"
#include "Mutex.h"
#include "Conditional.h"
#include "StandardExtensions.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtWidgets/qwidget.h>

using namespace Q3DStudio;
using Q3DStudio::CString;
using namespace qt3dsdm;
using namespace std;

namespace {

struct TickTockImpl;

struct TickTockItem : public ISignalConnection
{
    TTickTockProc m_Callback;
    bool m_IsPeriodic;
    unsigned long m_Time;
    quint64 m_NextTime;
    CString m_Name;
    TickTockImpl *m_Impl;

    TickTockItem(TTickTockProc inCallback, bool inPeriodic, unsigned long inTime,
                 quint64 inNextTime, const CString &inName, TickTockImpl &inImpl)
        : m_Callback(inCallback)
        , m_IsPeriodic(inPeriodic)
        , m_Time(inTime)
        , m_NextTime(inNextTime)
        , m_Name(inName)
        , m_Impl(&inImpl)
    {
    }
    // Implemented below to access TickTockImpl API
    virtual ~TickTockItem();

    // Called when our tick tock impl is through with us.
    // We cannot access the tick tock impl after this as
    // it could cause a crash
    void Release() { m_Impl = nullptr; }

    void Signal()
    {
        if (m_Impl)
            m_Callback();
    }
    bool operator<(const TickTockItem &inOther) const { return m_NextTime < inOther.m_NextTime; }
};

struct TickTockDerefCompare
{
    bool operator()(const TickTockItem *lhs, const TickTockItem *rhs) { return *lhs < *rhs; }
};

typedef vector<TickTockItem *> TTockList;

/**
 *	Struct that implements the ITickTock interface via a thread that runs
 *	and schedules information and a message that is sent back in order to
 *	process that schedule information and send out signals on the UI thread.
 */
struct TickTockImpl : public ITickTock, public CRunnable
{
    // Mutex to protect our internal datastructures
    // Mainly m_Tockers and m_SignalledTockers
    CMutex m_Mutex;
    // Our thread
    CThread m_Thread;
    // Event to wake up schedule thread; used to keep thread from
    // spinning till next event.
    CConditional m_Event;

    // The sorted list (by m_NextTime) of tick tock signals
    TTockList m_Tockers;
    // The list of signals that are past their time and need to be sent.
    TTockList m_SignalledTockers;
    // True if the thread is running and we want it to continue.
    volatile bool m_IsRunning;
    // Message to fire into the window in order to get our notifications
    // processed on the UI thread
    long m_MessageID;
    // Target window that we use in order to fire events and get processing
    // done back on the UI thread from the schedule thread.
    QWidget *m_Target;

    TickTockImpl(long inMessageID, QWidget *inTarget)
        : m_Thread(this, "ITickTock", NULL, false)
        , m_IsRunning(false)
        , m_MessageID(inMessageID)
        , m_Target(inTarget)
    {
    }

    virtual void Initialize()
    {
        m_IsRunning = true;
        m_Thread.Start();
    }

    virtual ~TickTockImpl()
    {
        if (m_Instance == this)
            m_Instance = nullptr;
        {
            CMutex::Scope __mutexScope(&m_Mutex);
            UnsafeReleaseTockers(m_Tockers);
            UnsafeReleaseTockers(m_SignalledTockers);
        }

        if (m_IsRunning) {
            m_IsRunning = false;
            // Wake up the thread to notify it to exit
            m_Event.Notify();
            // Wait for the thread to exit.
            m_Thread.Join();
        }
    }

    TSignalConnectionPtr AddTimer(unsigned long inTime, bool inIsPeriodic,
                                  TTickTockProc inTickTockProc,
                                  const QString &inName) override
    {
        // Lock down so we don't conflict with the timer thread.
        CMutex::Scope __mutexScope(&m_Mutex);

        std::shared_ptr<TickTockItem> retval =
                std::make_shared<TickTockItem>(inTickTockProc, inIsPeriodic, inTime,
                                               QDateTime::currentMSecsSinceEpoch() + inTime,
                                               Q3DStudio::CString::fromQString(inName),
                                               std::ref(*this));
        if (inTime > 0) {
            UnsafeInsertTimer(*retval);
        } else {
            assert(inIsPeriodic == false);
            m_SignalledTockers.push_back(retval.get());
        }
        m_Event.Notify();

        return retval;
    }

    void RemoveTimer(TickTockItem &inItem)
    {
        CMutex::Scope __mutexScope(&m_Mutex);
        UnsafeRemoveTimer(inItem, m_Tockers);
        UnsafeRemoveTimer(inItem, m_SignalledTockers);
    }

    //=============================================================================
    /**
     * Call from the main processing thread to process any existing messages.
     * This should be called when a message of the type specified in the constructor
     * is recieved. This will call all the functors on the timers that have
     * triggered.
     */
    void ProcessMessages() override
    {
        quint64 theCurrentTime = QDateTime::currentMSecsSinceEpoch();

        CMutex::Scope __mutexScope(&m_Mutex);
        // Go through all the timers looking for expired ones
        for (TTockList::iterator theTocks = m_SignalledTockers.begin(),
             end = m_SignalledTockers.end();
             theTocks != end; ++theTocks) {
            TickTockItem *theTock = (*theTocks);
            // If this item hasn't been released in another thread.
            if (theTock->m_Impl) {
                theTock->Signal();

                // If it is periodic, re-add it.
                if (theTock->m_IsPeriodic == true) {
                    theTock->m_NextTime = theCurrentTime + theTock->m_Time;
                    UnsafeInsertTimer(*theTock);
                }
                // If it isn't, then we forget about it.  The client's have a shared-ptr
                // to the object so it will get deleted eventually, it just isn't any of
                // our business any more
                else
                    theTock->Release();
            }
        }

        m_SignalledTockers.clear();
        m_Event.Notify();
    }

    void Run(void *) override
    {
        unsigned long theNextTime = (unsigned long)-1;
        // Stay in this loop until the running flag is turned off on the constructor.
        while (m_IsRunning) {
            // Wait for either the next time or the change notification
            m_Event.Wait(theNextTime);

            CMutex::Scope __mutexScope(&m_Mutex);

            // Get the amount of time this should sleep for.
            theNextTime = (unsigned long)-1;
            quint64 theCurrentTime = QDateTime::currentMSecsSinceEpoch();
            // We know that m_Tocks is sorted by m_NextTime.
            // So we run through it linearly, transferring singalled items
            // into the signalled vector.
            size_t idx = 0, end = m_Tockers.size();
            for (; idx < end; ++idx) {
                TickTockItem *item(m_Tockers[idx]);
                if (item->m_NextTime <= theCurrentTime && item->m_Impl != nullptr) {
                    m_SignalledTockers.push_back(item);
                } else
                    break;
            }
            // We then remove all signalled items.
            if (m_Tockers.empty() == false)
                m_Tockers.erase(m_Tockers.begin(), m_Tockers.begin() + idx);

            // And we reset next time to the difference between the current time
            // and the first tock's next time.
            if (m_Tockers.empty() == false) {
                assert(m_Tockers.front()->m_NextTime > theCurrentTime);
                theNextTime = m_Tockers.front()->m_NextTime - theCurrentTime;
            }

            if (m_SignalledTockers.empty() == false) {
                // Send the async notification that timers have expired.
                qApp->postEvent(m_Target, new QTimerEvent(m_MessageID));
            }
        }
    }

private:
    ////////////////////////////////////////////////////////////////
    // Unsafe functions are functions that require the mutex locked
    // in order to function correctly
    ////////////////////////////////////////////////////////////////
    void UnsafeInsertTimer(TickTockItem &inItem)
    {
        binary_sort_insert_unique(m_Tockers, &inItem, TickTockDerefCompare());
    }

    void UnsafeReleaseTockers(TTockList &inTockers)
    {
        for (size_t idx = 0, end = inTockers.size(); idx < end; ++idx)
            inTockers[idx]->Release();
        inTockers.clear();
    }

    // Unsafe means we don't have the mutex
    void UnsafeRemoveTimer(TickTockItem &inItem, TTockList &ioList)
    {
        TTockList::iterator theTimer = std::find(ioList.begin(), ioList.end(), &inItem);
        if (theTimer != ioList.end())
            ioList.erase(theTimer);
    }
};

TickTockItem::~TickTockItem()
{
    if (m_Impl != NULL)
        m_Impl->RemoveTimer(*this);
    m_Impl = NULL;
}
}

ITickTock *ITickTock::m_Instance(NULL);

std::shared_ptr<ITickTock> ITickTock::CreateTickTock(long inMessageID, QWidget *inWnd)
{
    std::shared_ptr<TickTockImpl> theTickTock(
                std::make_shared<TickTockImpl>(inMessageID, std::ref(inWnd)));
    theTickTock->Initialize();
    if (m_Instance == NULL)
        m_Instance = theTickTock.get();
    return theTickTock;
}

ITickTock &ITickTock::GetInstance()
{
    return *m_Instance;
}
