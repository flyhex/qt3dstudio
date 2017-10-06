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

#ifndef _UICEXCEPTIONS_H__
#define _UICEXCEPTIONS_H__

//==============================================================================
//	Includes
//==============================================================================

#include "UICExceptionClass.h"

//==============================================================================
//	Constants
//==============================================================================

const unsigned long LEVEL_SUCCESS = 0x00000000;
const unsigned long LEVEL_INFORMATIONAL = 0x00000001;
const unsigned long LEVEL_WARNING = 0x00000002;
const unsigned long LEVEL_ERROR = 0x00000003;

//==============================================================================
//	Macros
//==============================================================================

#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define UIC_DEFINE_THISFILE static wchar_t UIC_THIS_FILE[] = __WFILE__;

// The class
#define UIC_EXCEPTION CUICExceptionClass

// An exception with no extra parameters.
#define UIC_THROW(inErrorCode) UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode);

// An exception with one extra paramter.
#define UIC_THROW1(inErrorCode, inParam1)                                                          \
    UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode, inParam1);

// An exception with two extra paramters.
#define UIC_THROW2(inErrorCode, inParam1, inParam2)                                                \
    UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode, inParam1, inParam2);

// An exception with no extra parameters, only if inHResult doesn't SUCCEED().
#define UIC_THROWFAIL(inHResult, inErrorCode)                                                      \
    if (FAILED(inHResult))                                                                         \
        UIC_EXCEPTION::ThrowFail(inHResult, __WFILE__, __LINE__, inErrorCode);

// An exception with one extra paramter, is thrown only if inHResult doesn't SUCCEED().
#define UIC_THROWFAIL1(inHResult, inErrorCode, inParam1)                                           \
    if (FAILED(inHResult))                                                                         \
        UIC_EXCEPTION::ThrowFail(inHResult, __WFILE__, __LINE__, inErrorCode, inParam1);

// An exception with two extra paramters, is thrown only if inHResult doesn't SUCCEED().
#define UIC_THROWFAIL2(inHResult, inErrorCode, inParam1, inParam2)                                 \
    if (FAILED(inHResult))                                                                         \
        UIC_EXCEPTION::ThrowFail(inHResult, __WFILE__, __LINE__, inErrorCode, inParam1, inParam2);

// An exception with no extra parameters, only if inCondition is false.
#define UIC_THROWFALSE(inCondition, inErrorCode)                                                   \
    if (!(inCondition))                                                                            \
        UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode);

// An exception with one extra paramter, is thrown only if inCondition is false.
#define UIC_THROWFALSE1(inCondition, inErrorCode, inParam1)                                        \
    if (!(inCondition))                                                                            \
        UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode, inParam1);

// An exception with two extra paramters, is thrown only if inCondition is false.
#define UIC_THROWFALSE2(inCondition, inErrorCode, inParam1, inParam2)                              \
    if (!(inCondition))                                                                            \
        UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode, inParam1, inParam2);

// An exception with no extra parameters, only if inPointer is NULL.
#define UIC_THROWNULL(inPointer, inErrorCode)                                                      \
    if ((inPointer) == NULL)                                                                       \
        UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode);

// An exception with one extra parameters, only if inPointer is NULL.
#define UIC_THROWNULL1(inPointer, inErrorCode, inParam1)                                           \
    if ((inPointer) == NULL)                                                                       \
        UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode, inParam1);

// An exception with two extra parameters, only if inPointer is NULL.
#define UIC_THROWNULL2(inPointer, inErrorCode, inParam1, inParam2)                                 \
    if ((inPointer) == NULL)                                                                       \
        UIC_EXCEPTION::Throw(__WFILE__, __LINE__, inErrorCode, inParam1, inParam2);

// Standard catch that transfers the HRESULT from the exception
#define UIC_CATCH(outResult)                                                                       \
    catch (UIC_EXCEPTION & inException) { outResult = inException.GetErrorCode(); }                \
    catch (...) { outResult = E_FAIL; }

// Catch that transfers the HRESULT and sets the resource string from the exception
// DEPRECATED: DO not use!!!!!!!
#define UIC_CATCHERRORINFO(outResult)                                                              \
    catch (UIC_EXCEPTION & inException) { outResult = inException.GetErrorCode(); }                \
    catch (...) { outResult = E_FAIL; }

// Wrapper around DOM calls to catch and translate exceptions resource strings to ErrorInfo
#define UIC_DOMTRYCATCH(inStatement)                                                               \
    {                                                                                              \
        HRESULT theResult;                                                                         \
                                                                                                   \
        try {                                                                                      \
            theResult = inStatement;                                                               \
        }                                                                                          \
        UIC_CATCHERRORINFO(theResult)                                                              \
                                                                                                   \
        return theResult;                                                                          \
    }

#ifndef _WIN32
#define E_FAIL 0x80004005L
#endif

#endif // #ifndef _UICEXCEPTIONS_H__
