/****************************************************************************
**
** Copyright (C) 1999-2004 NVIDIA Corporation.
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
#include "StackTokenizer.h"

using namespace Q3DStudio;

//==============================================================================
/**
 *	CTOR
 *		@param inString	the string to tokenize
 *		@param inToken single char token
 *		@param	inEscapeChar single char escape char
 */
CStackTokenizer::CStackTokenizer(const Q3DStudio::CString &inString, Q3DStudio::UICChar inDelimiter,
                                 Q3DStudio::UICChar inEscapeChar)
    : m_String(inString)
    , m_Delimiter(inDelimiter)
    , m_EscapeChar(inEscapeChar)
    , m_Index(0)
    , m_LastIndex(0)
    , m_StringLength(inString.Length())
{
}

//==============================================================================
/**
 *	DTOR
 */
CStackTokenizer::~CStackTokenizer()
{
}

//==============================================================================
/**
 *	Returns true if string contains another token that can be read.
 *		@return bool true of there is another token to be read
 */
bool CStackTokenizer::HasNextPartition()
{
    if (m_Index >= m_StringLength)
        return false;
    else
        return true;
}

//==============================================================================
/**
 *	Retrieve the current token within the string.
 *		@return Q3DStudio::CString the string value of the token
 */

Q3DStudio::CString CStackTokenizer::GetCurrentPartition()

{
    std::deque<Q3DStudio::UICChar> theStack;
    bool theIgnoreEscapeChar = false;
    bool theFoundToken = false;
    long theCurrentIndex = m_Index;
    Q3DStudio::CString theResult;

    if (!m_String.IsEmpty()) {
        while (!theFoundToken) {
            // get the char at this index
            Q3DStudio::UICChar theChar = m_String.GetAt(theCurrentIndex++);

            if (theChar == m_Delimiter) {
                if (theStack.size() != 0) {
                    if (theStack.front() == m_EscapeChar && !theIgnoreEscapeChar) {
                        theStack.pop_front(); // escape this delimiter
                        theStack.push_front(theChar);
                    } else
                        theFoundToken = true; // found a delimiter, time to stop
                } else
                    theFoundToken = true; // this results in an empty token
            } else if (theChar == m_EscapeChar) {
                if (theStack.size() != 0) {
                    if (theStack.front() == m_EscapeChar && !theIgnoreEscapeChar) {
                        theIgnoreEscapeChar = true; //'escape' the escape char
                    } else {
                        theStack.push_front(theChar); // treat escape char as a normal
                        theIgnoreEscapeChar = false; // char.
                    }
                } else
                    theStack.push_front(theChar); // wait for next character to determine
                // what this should do.
            } else
                theStack.push_front(theChar); // any other characters

            if (theCurrentIndex == m_StringLength) // case when we reach the end of the string
                theFoundToken = true; // without encountering a delimiter.
        }

        m_LastIndex = theCurrentIndex; // update the last 'tokenized' position

        while (theStack.size() != 0) {
            theResult.Concat(theStack.back()); // put everything back into a string
            theStack.pop_back();
        }
    }
    return theResult;
}

//==============================================================================
/**
 *	Advances to the next token within the string. Use HasNextPartition to
 *	determine when to stop advancing.
 */
void CStackTokenizer::operator++()
{
    if (m_LastIndex > m_Index)
        m_Index = m_LastIndex;
    else {
        GetCurrentPartition();
        this->operator++();
    }
}
