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
#ifndef INFODUMP_H
#define INFODUMP_H
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include <vector>
#include "StrUtilities.h"
#include "InfoSink.h"
#include <string>

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CInfoDump
 *
 *	This concrete class that formats output strings, manages output sources
 *	derived from CInfoSink, and "dump" outputs to the Sinks
 *	Supports multiple output destination.
 *
 *	This class is responsible to hold/release the pointer of the InfoSink classes
 */
class CInfoDump
{
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef std::vector<CInfoSink *> TSinkList;
    typedef TSinkList::iterator TSinkListItr;

protected:
    TSinkList m_SinkList;
    char m_Buffer[1024];
    bool m_DebugON;

    //==============================================================================
    //	Ctor & Dtor
    //==============================================================================
public:
    CInfoDump();
    virtual ~CInfoDump();

    //==============================================================================
    //	Interface
    //==============================================================================
public:
    template <typename T>
    void DumpVal(const std::wstring &inPrefix, const T &inVal,
                 const long &inPriority = CInfoSink::ELOG, bool inDumpCondition = true)
    {
        std::wstring theStr = inPrefix;
        theStr << inVal;

        DumpStr(theStr.c_str(), inPriority, inDumpCondition);
    }

    virtual void DumpStr(const wchar_t *inStr, const long &inPriority = CInfoSink::ELOG,
                         bool inDumpCondition = true);
    virtual void DumpStrIndent(const wchar_t *inStr, const long inIndentCount,
                               const long &inPriority = CInfoSink::ELOG,
                               bool inDumpCondition = true);

    virtual void DumpCStr(const char *inFormatString, ...); // printf style

    // void SetSink( CInfoSink& inSink );
    virtual void SetSink(CInfoSink *inSink);
    virtual void SetSink(const TSinkList &inSinkList);
    virtual const TSinkList &GetSinks();
    virtual void RemoveSink(CInfoSink *inSink);

    virtual void SetAsDebug();
    virtual void SetAsUnitTest();
    virtual bool GetDebugMode();
};

#endif