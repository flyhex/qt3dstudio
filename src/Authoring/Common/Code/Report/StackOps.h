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

#ifndef INCLUDED_STACK_OPS_H
#define INCLUDED_STACK_OPS_H

#pragma once

#include "UICString.h"

#ifdef WIN32
typedef void (*CrashProc)(EXCEPTION_POINTERS *pExPtrs);
#endif

class CStackOps
{
public:
#ifdef WIN32
    static void RunStackTrace(TCHAR *inBuffer, DWORD inBufferLength);
    static void GetExceptionStackTrace(TCHAR *inBuffer, DWORD inBufferLength, CONTEXT *stCtx,
                                       DWORD dwNumSkip = 0);

    static LONG __stdcall UnhandledCrashHandler(EXCEPTION_POINTERS *pExPtrs);
    static void RegisterCrashHandler();
    static void SetCrashProc(CrashProc inCrashProc);

    static Q3DStudio::CString GetExceptionDescription(EXCEPTION_POINTERS *pExPtrs);
    static Q3DStudio::CString GetExceptionDescription(Q3DStudio::CString theException,
                                                      EXCEPTION_POINTERS *pExPtrs);
    static void GetExceptionPointers(DWORD dwExceptionCode,
                                     EXCEPTION_POINTERS **ppExceptionPointers);
    static void EnableCrashingOnCrashes();

    // this should only be called from the place where the object is setting the call back
    // otherwise we could end up with a nasty runtime error
    static void SetUserData(void *inUserData) { g_UserData = inUserData; }
    static void *GetUserData() { return g_UserData; }
#else
#endif

protected:
#ifdef WIN32
    static CrashProc g_CrashProc;
    static void *g_UserData;
#endif
};

#endif // INCLUDED_STACK_OPS_H

#ifdef WIN32
#define TRY_REPORT __try
#define CATCH_REPORT                                                                               \
    __except (CStackOps::UnhandledCrashHandler(GetExceptionInformation())) {}
#else
#define TRY_REPORT try
#define CATCH_REPORT                                                                               \
    catch (...) { /** TODO: implement stacktrace*/}
#endif

//#ifdef WIN32
//#define REPORT_CRASH CStackObject::StudioUnhandledCrashHandler( ::GetExceptionInformation( ) )
//#else
//#define REPORT_CRASH
//#endif

/*
Sample code to activate the exception handler from a try/except
__try
{
}
__except( CStackOps::StudioUnhandledCrashHandler( GetExceptionInformation( ) ) )
{
}
*/