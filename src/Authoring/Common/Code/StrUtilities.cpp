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
//	Prefixes
//==============================================================================
#include "stdafx.h"
#include "StrUtilities.h"
#include <wchar.h>
#include <stdio.h>
//==============================================================================
//	Internal Helper
//==============================================================================
//==============================================================================
/**
 *	@class	CConvertStr
 *
 *	Converts string from I type to O type
 */
template <typename I, typename O>
class CConvertStr
{
private:
    std::basic_string<O> m_OutStr;

public:
    CConvertStr() {}

    ~CConvertStr() {}

public:
    //==============================================================================
    /**
    *	The function. Takes in either const I* pointer or std::basic_string< I >
    *	@param inStr == the string to be converted
    *	@param inCount == the string length, in case, we do not hav a properly terminated
    *					  I* pointer passed in
    *	@return the converted string
    */
    const O *operator()(const std::basic_string<I> &inStr, long inCount = 0)
    {
        m_OutStr.clear();
        if (inCount == 0) {
            for (typename std::basic_string<I>::const_iterator theIterator = inStr.begin();
                 theIterator != inStr.end(); ++theIterator) {
                m_OutStr.push_back(static_cast<O>(*theIterator));
            }
        } else {
            for (long theIndex = 0; theIndex < inCount; ++theIndex) {
                m_OutStr.push_back(static_cast<O>(inStr[theIndex]));
            }

            m_OutStr.push_back(0);
        }

        return m_OutStr.c_str();
    }
};

//==============================================================================
//	String Conversion Functions
//==============================================================================

std::wstring &operator<<(std::wstring &inStr, bool inBool)
{
    inStr += inBool ? L"true" : L"false";

    return inStr;
}

std::wstring &operator<<(std::wstring &inStr, long inLong)
{
    // wchar_t theStr[ 64 ];

    //#ifdef WIN32
    //	::swprintf( theStr, L"%ld", inLong );
    //#else
    //	::swprintf( theStr, 32, L"%ld", inLong );
    //#endif
    // this should work across platforms
    //::swprintf( theStr, 64, L"%ld", inLong );

    char theStr[64];
    sprintf(theStr, "%ld", inLong);

    inStr << theStr;
    return inStr;
}

std::wstring &operator<<(std::wstring &inStr, double inDouble)
{
    // wchar_t theStr[ 64 ];

    //#ifdef _WIN32
    //	::swprintf( theStr, L"%g", inDouble );
    //#else
    //	::swprintf( theStr, 32, L"%g", inDouble ) );
    //#endif
    // this should work across platforms
    //::swprintf( theStr, 64, L"%g", inDouble );

    char theStr[64];
    sprintf(theStr, "%g", inDouble);

    inStr << theStr;
    return inStr;
}

std::wstring &operator<<(std::wstring &inStr, const char *inStr02)
{
    if (inStr02) {
        CConvertStr<char, wchar_t> theConvert;
        inStr += theConvert(inStr02);
    }
    return inStr;
}

std::wstring &operator<<(std::wstring &inStr, const wchar_t *inStr02)
{
    if (inStr02) {
        inStr += inStr02;
    }
    return inStr;
}

std::wstring &operator<<(std::wstring &inStr, const std::string &inStr02)
{
    CConvertStr<char, wchar_t> theConvert;
    inStr += theConvert(inStr02);
    return inStr;
}

std::wstring &operator<<(std::wstring &inStr, const std::wstring &inStr02)
{
    inStr += inStr02;
    return inStr;
}

std::string &operator<<(std::string &inStr, bool inBool)
{
    char theStr[32];
    ::sprintf(theStr, "%s", inBool ? "true" : "false");
    inStr += theStr;
    return inStr;
}

std::string &operator<<(std::string &inStr, long inLong)
{
    char theStr[32];
    ::sprintf(theStr, "%ld", inLong);
    inStr += theStr;
    return inStr;
}

std::string &operator<<(std::string &inStr, double inDouble)
{
    char theStr[32];
    ::sprintf(theStr, "%g", inDouble);
    inStr += theStr;
    return inStr;
}

std::string &operator<<(std::string &inStr, const char *inStr02)
{
    if (inStr02) {
        inStr += inStr02;
    }
    return inStr;
}

std::string &operator<<(std::string &inStr, const wchar_t *inStr02)
{
    if (inStr02) {
        CConvertStr<wchar_t, char> theConvert;
        inStr += theConvert(inStr02);
    }
    return inStr;
}

std::string &operator<<(std::string &inStr, const std::string &inStr02)
{
    inStr += inStr02;
    return inStr;
}

std::string &operator<<(std::string &inStr, const std::wstring &inStr02)
{
    CConvertStr<wchar_t, char> theConvert;
    inStr += theConvert(inStr02);
    return inStr;
}
