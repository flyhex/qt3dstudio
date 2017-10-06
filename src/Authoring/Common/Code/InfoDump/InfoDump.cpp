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
#include "InfoDump.h"

//==============================================================================
//	Includes
//==============================================================================
#include "InfoSink.h"

//==============================================================================
//	Ctors and Dtor
//==============================================================================
//==============================================================================
/**
*		Ctor
*/
CInfoDump::CInfoDump()
    : m_DebugON(true)
{
    m_Buffer[0] = '\0';
    m_SinkList.clear();
}

//==============================================================================
/**
*		Dtor
*/
CInfoDump::~CInfoDump()
{
    // for ( TSinkListItr theItr = m_SinkList.begin( ); theItr != m_SinkList.end( ); ++theItr )
    //{
    //	delete *theItr;
    //}
    m_SinkList.clear();
}

//==============================================================================
//	Interface
//==============================================================================
//==============================================================================
/**
*	Dumps out the string
*	Loops through the vec of CInfoSink, pushes out the string
*/
void CInfoDump::DumpStr(const wchar_t *inStr, const long &inPriority, bool inDumpCondition)
{
    if (m_DebugON ? inDumpCondition : true) {
        for (TSinkListItr theItr = m_SinkList.begin(); theItr != m_SinkList.end(); ++theItr) {
            if ((*theItr)->MatchPriority(inPriority))
                (*theItr)->Sink(inStr);
        }
    }
}

void CInfoDump::DumpStrIndent(const wchar_t *inStr, const long inIndentCount,
                              const long &inPriority, bool inDumpCondition)
{
    std::wstring theCompoStr = L"";

    for (long theIndex = 0; theIndex < inIndentCount; ++theIndex) {
        theCompoStr += L" ";
    }

    theCompoStr += inStr;

    DumpStr(theCompoStr.c_str(), inPriority, inDumpCondition);
}

#include <stdarg.h>
#include "StrUtilities.h"
void CInfoDump::DumpCStr(const char *inFormatString, ...) // printf style
{
    /*
    code snippet adopted from:
    http://www.newtek.com/products/lightwave/developer/75lwsdk/docs/commands.html
    */

    // pb - 5.14.2004 - removed POS super slow code.
    if (inFormatString)

    {

#ifdef _DEBUG
        // only for debug...?
        static char theArg[1024];
        va_list ap;
        va_start(ap, inFormatString);
        vsprintf(theArg, inFormatString, ap);
        va_end(ap);

        std::wstring theWString;
        theWString << theArg;
        DumpStr(theWString.c_str());
#endif
    }
}

////==============================================================================
///**
//*	Sets the output destination(s) of the this class
//*	@param inSink	a new output destination
//*/
// void CInfoDump::SetSink( CInfoSink& inSink )
//{
//	m_SinkList.push_back( &inSink );
//}

//==============================================================================
/**
*	Sets the output destination(s) of the this class
*	@param inSink	a new output destination
*/
void CInfoDump::SetSink(CInfoSink *inSink)
{
    // no duplicates
    TSinkListItr theItr = m_SinkList.begin();
    for (; theItr != m_SinkList.end(); ++theItr) {
        if (*theItr == inSink)
            break;
    }
    if (theItr == m_SinkList.end()) {
        m_SinkList.push_back(inSink);
    }
}

//==============================================================================
/**
*	Sets the output destination(s) of the this class
*	@param inSink	a new output destination
*/
void CInfoDump::SetSink(const CInfoDump::TSinkList &inSinkList)
{
    m_SinkList = inSinkList;
}

//==============================================================================
/**
*	Retrieve the Sinks ref that's set
*/
const CInfoDump::TSinkList &CInfoDump::GetSinks()
{
    return m_SinkList;
}

//==============================================================================
/**
*	Sets the output destination(s) of the this class
*	@param inSink	a new output destination
*/
void CInfoDump::RemoveSink(CInfoSink *inSink)
{
    for (TSinkListItr theItr = m_SinkList.begin(); theItr != m_SinkList.end(); ++theItr) {
        if (*theItr == inSink) {
            m_SinkList.erase(theItr);
            break;
        }
    }
}

//==============================================================================
/**
*	Sets the mode of the InfoDump to Debug
*
*/
void CInfoDump::SetAsDebug()
{
    m_DebugON = true;
}

//==============================================================================
/**
*	Sets the mode of the InfoDump to Debug
*
*/
void CInfoDump::SetAsUnitTest()
{
    m_DebugON = false;
}

//==============================================================================
/**
*	Checks the debug mode
*
*/
bool CInfoDump::GetDebugMode()
{
    return m_DebugON;
}
