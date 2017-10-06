/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#include "SystemPrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "UICThreadManager.h"

#ifdef _TEGRAPLATFORM
#include <Winbase.h.>

// This is to try to throttle background threads to not hog all
// the cpu while doing work.
// http://msdn.microsoft.com/en-us/library/aa450618.aspx
static const INT32 s_ThreadPriority = 252;
#endif

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
CThreadManager::CThreadManager(INT32 inThreads)
    : m_WorkItems(0)
{
    Q3DStudio_ASSERT(inThreads > 0);
    m_Running = true;
    for (INT32 theIter = 0; theIter < inThreads; theIter++) {
        m_Threads.Push(AppCreateThread(ThreadSpinFunction, this));
    }
}

//==============================================================================
/**
 *	Destructor
 */
CThreadManager::~CThreadManager()
{
    m_Running = false;
    for (INT32 theIter = 0; theIter < m_Threads.GetCount(); theIter++) {
        AppDestroyThread(m_Threads[theIter]);
    }
    m_Threads.Clear();
}

//==============================================================================
/**
*	Check for completed work and return the number of work items left in the queue
*/
INT32 CThreadManager::Update()
{
    CheckForCompletedWork();
    return m_WorkQueue.GetCount();
}

//==============================================================================
/**
 *	Push work onto the work queue
 */
void CThreadManager::Push(const SThreadWorkItem &inItem)
{
    Q3DStudio_ASSERT(inItem.m_UserData && inItem.m_Function);
    m_WorkQueue.Push(inItem);
}

struct SItemNameHashPredicate
{
    UINT32 m_ItemNameHash;
    bool operator()(const SThreadWorkItem &inItem)
    {
        return inItem.m_WorkItemNameHash == m_ItemNameHash;
    }
};
//==============================================================================
/**
 *	Remove a list of items currently in the work queue.
 */
void CThreadManager::Remove(UINT32 inItemNameHash, CArray<SThreadWorkItem> &ioRemovedItems)
{
    SItemNameHashPredicate thePredicate = { inItemNameHash };
    m_WorkQueue.Remove(thePredicate, ioRemovedItems);
}

//==============================================================================
/**
 *	Return a reference to the scratch pad
 */
CThreadSafeScratchPad &CThreadManager::GetScratchPad()
{
    return m_ScratchPad;
}

//==============================================================================
/**
*	Pop completed work off the result queue
*/
bool CThreadManager::Pop(SThreadResultItem &outItem)
{
    return m_ResultsQueue.Pop(outItem);
}

//==============================================================================
/**
 *	The function that created threads call (worker threads)
 */
void *CThreadManager::ThreadSpinFunction(void *inUserData)
{
#ifdef _TEGRAPLATFORM
// CeSetThreadPriority( GetCurrentThread(), s_ThreadPriority );
#endif

    CThreadManager *theThreadManager = reinterpret_cast<CThreadManager *>(inUserData);
    theThreadManager->DoWork();

    return NULL;
}

//==============================================================================
/**
*	The function that created threads spin on (worker threads)
*/
void CThreadManager::DoWork()
{
    volatile BOOL theRunning = m_Running;
    while (theRunning) {
        SThreadWorkItem theWorkItem = { 0 };
        while (m_WorkQueue.Pop(theWorkItem)) {
            SThreadResultItem theResultItem =
                theWorkItem.m_Function(theWorkItem.m_UserData, m_ScratchPad);

            // For now, always ensure we have a valid result item
            Q3DStudio_ASSERT(theResultItem.m_Function && theResultItem.m_UserData);

            if (theResultItem.m_Function && theResultItem.m_UserData)
                m_ResultsQueue.Push(theResultItem);
        }
        // Sleep after every task..?
        AppSleepThread();

        theRunning = m_Running;
    }
}

//==============================================================================
/**
*	The function that created threads spin on (main thread)
*/
void CThreadManager::CheckForCompletedWork()
{
    SThreadResultItem theResultItem = { 0 };
    while (m_ResultsQueue.Pop(theResultItem)) {
        theResultItem.m_Function(theResultItem.m_UserData, m_ScratchPad);
    }
}
}
