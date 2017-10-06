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

//==============================================================================
//	Includes
//==============================================================================

#include "stdafx.h"
#include "StackOps.h"
#include <stdio.h>

#ifdef WIN32

#include <comdef.h>

typedef std::vector<size_t> ADDRVECTOR; ///< The address typedef.

CrashProc CStackOps::g_CrashProc = NULL;
void *CStackOps::g_UserData = NULL;

//==============================================================================
/**
 * Registers the crash handler for all unhandled exceptions.
 */
//==============================================================================
void CStackOps::RegisterCrashHandler()
{
    ::SetUnhandledExceptionFilter(&UnhandledCrashHandler);
}

//==============================================================================
/**
 *	RunStackTrace: performs a stack trace
 *
 *	@param ioBuffer buffer to put the stack trace in
 *	@param inBufferLength size of the buffer
 */
//==============================================================================
void CStackOps::RunStackTrace(TCHAR *ioBuffer, DWORD inBufferLength)
{
    // TODO
}

//==============================================================================
/**
 * Generates a stack trace from a given CONTEXT.
 * This is useful for getting stack traces within catch statements for where
 * an exception was thrown.
 *
 * @param ioBuffer buffer to put the stack trace in.
 * @param inBufferLength the size of the buffer.
 * @param inContext the state of the thread for the exception, from GetExceptionInformation(
 * )->ContextRecord.
 */
//==============================================================================
void CStackOps::GetExceptionStackTrace(TCHAR *, DWORD, CONTEXT *, DWORD)
{
    // TODO
}

void CStackOps::SetCrashProc(CrashProc inCrashProc)
{
    g_CrashProc = inCrashProc;
}
//==============================================================================
/**
 * This gets called any time an unhandled exception is thrown anywhere in the
 * application.
 *
 * @param pExPtrs a description of the exception.
 * @return 0 if not handled, 1 if it was handled.
 */
//==============================================================================
LONG __stdcall CStackOps::UnhandledCrashHandler(EXCEPTION_POINTERS *pExPtrs)
{
    if (g_CrashProc)
        g_CrashProc(pExPtrs);
    return EXCEPTION_EXECUTE_HANDLER;
}

//==============================================================================
/**
 * Get a somewhat useful short description of the exception.
 *
 * @param pExPtrs description of the exception, from GetExceptionInformation.
 */
//==============================================================================
Q3DStudio::CString CStackOps::GetExceptionDescription(EXCEPTION_POINTERS *pExPtrs)
{
    Q3DStudio::CString theException;

    switch (pExPtrs->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        theException = "an Access Violation";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        theException = "an Array Bounds Exceeded";
        break;
    case EXCEPTION_BREAKPOINT:
        theException = "a Breakpoint";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        theException = "a DataType Misalignment";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        theException = "a Float Denormal Operand";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        theException = "a Float Divide by Zero";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        theException = "a Float Inexact Result";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        theException = "a Float Invalid Operation";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        theException = "a Float Overflow";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        theException = "a Float Stack Check";
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        theException = "a Float Underflow";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        theException = "an Illegal Instruction";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        theException = "an In Page Error";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        theException = "an Integer Divide by Zero";
        break;
    case EXCEPTION_INT_OVERFLOW:
        theException = "an Integer Overflow";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        theException = "an Invalid Disposition";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        theException = "a Non-Continuable Exception";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        theException = "a Private Instruction";
        break;
    case EXCEPTION_SINGLE_STEP:
        theException = "a Single Step";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        theException = "a Stack Overflow";
        break;
    default:
        theException.Format(_UIC("an Error:0x%X"), pExPtrs->ExceptionRecord->ExceptionCode);
        break;
    }
    return GetExceptionDescription(theException, pExPtrs);
}

//==============================================================================
/**
 * Get a somewhat useful short description of the exception.
 *
 * @param Q3DStudio::CString description of the exception
 */
//==============================================================================
Q3DStudio::CString CStackOps::GetExceptionDescription(Q3DStudio::CString theException,
                                                      EXCEPTION_POINTERS *pExPtrs)
{
    Q3DStudio::CString theDescription;
    char theBuffer[512];

    theBuffer[::GetModuleFileNameA(NULL, theBuffer, sizeof(theBuffer))] = 0;

    theDescription += theBuffer;
    theDescription += " Caused ";
    theDescription += theException;

    theException.Format(_UIC(" at 0x%X"), pExPtrs->ExceptionRecord->ExceptionAddress);

    theDescription += theException;

    return theDescription;
}

#pragma warning(disable : 4748)
#pragma warning(disable : 4740)

#endif

#if _MSC_VER >= 1300
#include <rtcapi.h>
#endif
#ifndef _AddressOfReturnAddress
// Taken from: http://msdn.microsoft.com/en-us/library/s975zw7k(VS.71).aspx
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
// _ReturnAddress and _AddressOfReturnAddress should be prototyped before use
EXTERNC void *_AddressOfReturnAddress(void);
EXTERNC void *_ReturnAddress(void);
#endif
// The following function retrieves exception info
// The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)
#ifdef KDAB_TEMPORARILY_REMOVED
void CStackOps::GetExceptionPointers(DWORD dwExceptionCode,
                                     EXCEPTION_POINTERS **ppExceptionPointers)
{
    // The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)

    EXCEPTION_RECORD ExceptionRecord;
    CONTEXT ContextRecord;
    memset(&ContextRecord, 0, sizeof(CONTEXT));

#ifdef _X86_
    __asm {
      mov dword ptr [ContextRecord.Eax], eax
      mov dword ptr [ContextRecord.Ecx], ecx
      mov dword ptr [ContextRecord.Edx], edx
      mov dword ptr [ContextRecord.Ebx], ebx
      mov dword ptr [ContextRecord.Esi], esi
      mov dword ptr [ContextRecord.Edi], edi
      mov word ptr [ContextRecord.SegSs], ss
      mov word ptr [ContextRecord.SegCs], cs
      mov word ptr [ContextRecord.SegDs], ds
      mov word ptr [ContextRecord.SegEs], es
      mov word ptr [ContextRecord.SegFs], fs
      mov word ptr [ContextRecord.SegGs], gs
      pushfd
      pop [ContextRecord.EFlags]
    }
    ContextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable : 4311)
    ContextRecord.Eip = (ULONG)_ReturnAddress();
    ContextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
    ContextRecord.Ebp = *((ULONG *)_AddressOfReturnAddress() - 1);
#elif defined(_IA64_) || defined(_AMD64_)
    /* Need to fill up the Context in IA64 and AMD64. */
    RtlCaptureContext(&ContextRecord);
#else /* defined (_IA64_) || defined (_AMD64_) */
    ZeroMemory(&ContextRecord, sizeof(ContextRecord));
#endif /* defined (_IA64_) || defined (_AMD64_) */
    ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));
    ExceptionRecord.ExceptionCode = dwExceptionCode;
    ExceptionRecord.ExceptionAddress = _ReturnAddress();

    EXCEPTION_RECORD *pExceptionRecord = new EXCEPTION_RECORD;
    memcpy(pExceptionRecord, &ExceptionRecord, sizeof(EXCEPTION_RECORD));
    CONTEXT *pContextRecord = new CONTEXT;
    memcpy(pContextRecord, &ContextRecord, sizeof(CONTEXT));
    *ppExceptionPointers = new EXCEPTION_POINTERS;
    (*ppExceptionPointers)->ExceptionRecord = pExceptionRecord;
    (*ppExceptionPointers)->ContextRecord = pContextRecord;
}

//==============================================================================
/**
 *	EnableCrashingOnCrashes, allow exceptions to be thrown regardless due to the
 *  fact that on 64bit windows, it was not possible to unwind stack across kernel boundary
 *
 *	hhttps://randomascii.wordpress.com/2012/07/05/when-even-crashing-doesnt-work/
 *  http://support.microsoft.com/kb/976038
 *
 */
//==============================================================================

void CStackOps::EnableCrashingOnCrashes()
{
    typedef BOOL(WINAPI * tGetPolicy)(LPDWORD lpFlags);
    typedef BOOL(WINAPI * tSetPolicy)(DWORD dwFlags);
    const DWORD EXCEPTION_SWALLOWING = 0x1;

    HMODULE kernel32 = LoadLibraryA("kernel32.dll");
    tGetPolicy pGetPolicy =
        (tGetPolicy)GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy");
    tSetPolicy pSetPolicy =
        (tSetPolicy)GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy");
    if (pGetPolicy && pSetPolicy) {
        DWORD dwFlags;
        if (pGetPolicy(&dwFlags)) {
            // Turn off the filter
            pSetPolicy(dwFlags & ~EXCEPTION_SWALLOWING);
        }
    }
}
#endif
