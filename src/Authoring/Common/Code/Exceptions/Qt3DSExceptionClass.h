/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
//	Prefix
//==============================================================================
#ifndef __QT3DS_EXCEPTION_CLASS_H_
#define __QT3DS_EXCEPTION_CLASS_H_

//==============================================================================
//	Includes
//==============================================================================

#include "StudioException.h"
#include "Qt3DSExceptionConstants.h"

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
//	Typedefs
//==============================================================================

//==============================================================================
//	Constants
//==============================================================================
const short MAX_ENTRY_LENGTH = 2048;
const short MAX_STACKTRACE_LENGTH = 1024;

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

//==============================================================================
//	Class
//==============================================================================

//==============================================================================
/**
 *	@class	CExceptionClass
 *	@brief
 */
class Qt3DSExceptionClass : public CStudioException
{
    //==============================================================================
    //	Fields
    //==============================================================================

protected:
    wchar_t m_FileName[_MAX_PATH]; ///< The file name where the exception occurred
    wchar_t m_Location[_MAX_PATH]; ///< The location description
    wchar_t m_Description[MAX_ENTRY_LENGTH]; ///< A description of the exception
    wchar_t m_StackTrace[MAX_STACKTRACE_LENGTH]; ///< The stack when the exception was created.
    long m_ErrorCode; ///< The error code (long)
    long m_long; ///< The long that FAILED
    va_list m_ParameterList; ///< The optional parameter list passed to the constructor ( sprintf
                             ///type functionality ).
    unsigned long m_LineNumber; ///< The line number of the exception
    unsigned long m_Type; ///< The type of exception

    //==============================================================================
    //	Methods
    //==============================================================================

    // Construction

protected:
    Qt3DSExceptionClass(const wchar_t *inFileName, const unsigned long inLineNumber,
                       const long inErrorCode, const long inlong, va_list inParameters);
    virtual ~Qt3DSExceptionClass() {}

    // Access

public:
    wchar_t *GetFileName() { return m_FileName; }
    wchar_t *GetLocation() { return m_Location; }
    wchar_t *GetStackTrace() { return m_StackTrace; };
    const wchar_t *GetDescription() const override { return m_Description; }
    long GetErrorCode() { return m_ErrorCode; }
    long Getlong() { return m_long; }
    unsigned long GetLineNumber() { return m_LineNumber; }
    unsigned long GetType() { return m_Type; }

    // Implementation

protected:
    // void	GetStringFromID( unsigned long inID, wchar_t* outString );
    bool FormatDescription();
    void HandleException();
    void BreakInDebugger();
    void DisplayMessageBox();
    // void	Log2File();
    void Output2Debugger();

    //==============================================================================
    //	Static Methods
    //==============================================================================

public:
    static void Throw(const wchar_t *inFileName, const unsigned long inLineNumber,
                      const long inErrorCode, ...);
    static void ThrowFail(const long inlong, const wchar_t *inFileName,
                          const unsigned long inLineNumber, const long inErrorCode, ...);
    static void ThrowFalse(bool inCondition, const wchar_t *inFileName,
                           const unsigned long inLineNumber, const long inErrorCode, ...);

    static bool s_HandlingTypes;

protected:
    static void Initialize();
};

#endif //__QT3DS_EXCEPTION_CLASS_H_
