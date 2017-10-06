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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "StringTokenizer.h"

using namespace Q3DStudio;

//==============================================================================
/**
 *	formats inString so it is a correct delimited string using inToken
 *	@param inToken the token that delimets the string
 *	@param inString theString containing the delimited items yo
 */
CStringTokenizer::CStringTokenizer(Q3DStudio::CString inString, Q3DStudio::CString inToken)
{
    // If the first of the string is a token extract it off until there is no token at the first of
    // the string
    Q3DStudio::CString theTempString = inString.Extract(0, inToken.Length());
    while (inToken.Length() && theTempString == inToken) {
        inString = inString.Extract(inToken.Length());
        theTempString = inString.Extract(0, inToken.Length());
    }

    // Do the same thing for the end part of the string
    theTempString = inString.Extract(inString.Length() - 1 - inToken.Length());
    while (theTempString == inToken) {
        inString = inString.Extract(0, inString.Length() - inToken.Length());
        theTempString = inString.Extract(inString.Length() - 1 - inToken.Length());
    }

    m_OriginalString = inString;
    m_Token = inToken;
    m_Index = 0;
}

//==============================================================================
/**
 *	DTOR
 */
CStringTokenizer::~CStringTokenizer()
{
}

void CStringTokenizer::operator++()
{
    if (m_Index != Q3DStudio::CString::ENDOFSTRING) {
        long theCurrentTokenIndex = m_OriginalString.Find(m_Token, m_Index);

        // Case last partition after last token
        if (theCurrentTokenIndex == Q3DStudio::CString::ENDOFSTRING) {
            m_Index = Q3DStudio::CString::ENDOFSTRING;
        }
        // Somewhere in the middle or beginnign for the string
        else {
            m_Index = theCurrentTokenIndex + m_Token.Length();
            if (m_Index > m_OriginalString.Length())
                m_Index = Q3DStudio::CString::ENDOFSTRING;
        }
    }
}

//==============================================================================
/**
 *	Gets the next delimeted partition
 */
Q3DStudio::CString CStringTokenizer::GetCurrentPartition()
{
    Q3DStudio::CString theReturnString = "";
    if (m_Index != Q3DStudio::CString::ENDOFSTRING) {
        long theCurrentTokenIndex = m_OriginalString.Find(m_Token, m_Index);

        // Case last partition after last token
        if (theCurrentTokenIndex == Q3DStudio::CString::ENDOFSTRING) {
            theReturnString = m_OriginalString.Extract(m_Index);
        }
        // Somewhere in the middle or beginnign for the string
        else {
            theReturnString = m_OriginalString.Extract(m_Index, theCurrentTokenIndex - m_Index);
        }
    }
    return theReturnString;
}

//==============================================================================
/**
 *	return true if there is more data to be extracted
 */
bool CStringTokenizer::HasNextPartition()
{
    return (m_Index != Q3DStudio::CString::ENDOFSTRING);
}