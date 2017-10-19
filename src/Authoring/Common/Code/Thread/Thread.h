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
#ifndef INCLUDED_THREAD_H
#define INCLUDED_THREAD_H
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Runnable.h"
#include "Qt3DSString.h"

//==============================================================================
//	Platform dependent includes
//==============================================================================
#include <QThread>

//==============================================================================
/**
 *	@class CThread
 *	@brief Cross platform abstraction and implementation of threads.
 */
class CThread : public CRunnable
{
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef QThread* TThread; // Windows uses HANDLE

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    TThread m_Thread; ///< Platform dependent representation of thread
    CRunnable *m_Runnable;
    bool m_IsActive;
    bool m_DeleteOnDeath;
    void *m_Args;
    unsigned long m_ThreadID;
    Q3DStudio::CString m_ThreadName;

    //==============================================================================
    //	Methods
    //==============================================================================
protected: // Protected constructors
    CThread(TThread &inThreadDesc);
    CThread();
    CThread(const CThread &inThread);

public: //  Construction
    CThread(CRunnable *inRunnable, const Q3DStudio::CString &inThreadName, void *inArgs = NULL,
            bool inStartImmediately = true);
    virtual ~CThread();

    void Run(void *inArgs = NULL) override;

    unsigned long Start();
    void Join();

    bool IsActive();
    bool IsAutoDelete() { return m_DeleteOnDeath; }
    void SetAutoDelete(bool inAutoDelete) { m_DeleteOnDeath = inAutoDelete; }

    TThread GetThread();
    unsigned long GetThreadID();

    //==============================================================================
    //	Statics
    //==============================================================================
public:
    static void Sleep(unsigned long inTime);
    static TThread GetCurrentThread();
    static unsigned long GetCurrentThreadID();
    static long GetUserTime();
};
#endif // __THREAD_H_
