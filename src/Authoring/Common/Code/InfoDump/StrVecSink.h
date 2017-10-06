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
#ifndef _STRVECSINK_H_
#define _STRVECSINK_H_
#pragma once

//==============================================================================
//	Include
//==============================================================================
#include <set>
#include <vector>
#include <string>
#include "InfoSink.h"

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CStrVecSink
 *
 *	This base class that collects all the info passed from:
 *	1 ) the InfoDump
 *	2 ) an XML file
 *	Stores them as multisets of strings, which can be processed by various
 *	classes.
 */
class CStrVecSink : public CInfoSink
{
public:
    typedef std::multiset<std::wstring> TStrVec;

protected:
    TStrVec m_StrVec;
    //==============================================================================
    //	Cotr & Dtor
    //==============================================================================
public:
    CStrVecSink(const long &inPriority);
    CStrVecSink(const CStrVecSink &inStrVecSink);
    virtual ~CStrVecSink();

    CStrVecSink &operator=(const CStrVecSink &inStrVecSink);

    //==============================================================================
    //	Interface
    //==============================================================================
public:
    void Sink(const wchar_t *inStr) override;
    void LoadXML(const std::wstring &inXMLFile);
    long GetTotalLines();
    bool RetrieveLine(const long inIndex, std::wstring &outStr);
    long FindThisLine(const std::wstring &inStr);
    bool FindAndRemoveLine(const std::wstring &inStr);
    std::vector<std::wstring>::iterator RetrieveDifferenceSet(CStrVecSink &inCompareSink,
                                                              std::vector<std::wstring> &outSet);

    void ToFile(const std::wstring &inFilename);
};

#endif
