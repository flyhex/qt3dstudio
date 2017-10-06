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

#pragma once

#include "UICThreadSafeScratchpad.h"
#include "UICThreadSafeQueue.h"
#include "UICArray.h"

namespace Q3DStudio {
//==============================================================================
//	Fields specific to the thread manager
//==============================================================================

// NOTE:	The order that these are defined matters (specifically, the relationship
//			between the struct forward declare, struct definition, and function pointer.

// TODO - ah
// The interaction between these is not that clear... comment stuff!!

struct SThreadResultItem;
struct SThreadWorkItem;

typedef CThreadSafeQueue<SThreadWorkItem> TThreadWorkQueue;
typedef CThreadSafeQueue<SThreadResultItem> TThreadResultQueue;

// The function to call when a work item is 'finished'
typedef void (*TThreadFinishFunction)(void *inUserData, CThreadSafeScratchPad &inScratchPad);

// This encapsulates a 'result'.
struct SThreadResultItem
{
    void *m_UserData;
    TThreadFinishFunction m_Function;
};
typedef SThreadResultItem (*TThreadWorkFunction)(void *inUserData,
                                                 CThreadSafeScratchPad &inScratchPad);

struct SThreadWorkItem
{
    UINT32 m_WorkItemNameHash; ///< Used to identify a class of work items we can remove them from
                               ///time to time.
    void *m_UserData;
    TThreadWorkFunction m_Function;
};

typedef void *TThreadID;
typedef void *(*TThreadSpinFunction)(void *inUserData);

//==============================================================================
/**
*	The thread manager is the interface to create work on other threads and
*	process the completed results. This is done abstractly through work and
*	result items.
*/
class CThreadManager
{

    //==============================================================================
    //	Fields
    //==============================================================================
private:
    CThreadSafeScratchPad m_ScratchPad;
    TThreadWorkQueue m_WorkQueue;
    TThreadResultQueue m_ResultsQueue;
    CArray<TThreadID> m_Threads;
    INT32 m_WorkItems; // running count
    volatile BOOL m_Running;
    BOOL m_Unused[3];

    //==============================================================================
    //	Methods
    //==============================================================================
public:
    CThreadManager(INT32 inThreads);
    ~CThreadManager();

public: // Interface
    INT32 Update();
    void Push(const SThreadWorkItem &inItem);
    // Remove all work items of a certain class.  This returns
    // an array containing the removed items.
    void Remove(UINT32 inItemNameHash, CArray<SThreadWorkItem> &ioRemovedItems);

    CThreadSafeScratchPad &GetScratchPad();

private:
    bool Pop(SThreadResultItem &outItem);
    static void *ThreadSpinFunction(void *inUserData);
    void DoWork();
    void CheckForCompletedWork();
};

} // namespace Q3DStudio

//===================================================================
// The application needs to provide these thread primitive functions
//===================================================================

Q3DStudio::TThreadID AppCreateThread(Q3DStudio::TThreadSpinFunction inFunc, void *inUserData);
void AppDestroyThread(Q3DStudio::TThreadID inThreadID);
void AppSleepThread();
