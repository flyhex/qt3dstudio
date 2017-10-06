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
#include "StrVecSink.h"

#include <fstream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include "StrUtilities.h"
#include "DumpFileSink.h"

//==============================================================================
//	Local Funks
//==============================================================================
namespace StrVecSink {
typedef std::wstring::const_iterator TWSItr;
typedef std::wstring::reverse_iterator TRWSItr;

//==============================================================================
/**
*	Trims white spaces before and after the "valid string" which begins wif
*	a non whitespace
*	@param inStr == the string to trim
*	@param outStr == the resultant string
*/
void TrimWhiteSpace(const std::wstring &inStr, std::wstring &outStr)
{
    const std::wstring WHITECHARS = L" \t\n";

    // Trim the front
    // makes sure theTempStr have some valid values
    std::wstring theTempStr = inStr;
    TWSItr theStrItr;
    for (inStr.begin(); theStrItr != inStr.end(); ++theStrItr) {
        bool theFoundWhiteSpace = false;
        for (TWSItr theWSItr = WHITECHARS.begin(); theWSItr != WHITECHARS.end(); ++theWSItr) {
            if (*theStrItr == *theWSItr) {
                // found whitespace for the front, break
                theFoundWhiteSpace = true;
                break;
            }
        }

        if (!theFoundWhiteSpace) {
            // indicates we have encountered the first NON whitespace,
            // reassign theTempStr and break
            theTempStr = inStr.substr(std::distance(inStr.begin(), theStrItr));
            break;
        }
    }

    if (theStrItr == inStr.end()) {
        // this means there's no valid char in this string, all are white spaces
        // assign a blank string to theTempStr
        theTempStr = L"";
    }

    // now we have the head portion trimmed, trim the tail portion
    // first make sure outStr has some valid values
    outStr = theTempStr;
    for (TRWSItr theRStrItr = theTempStr.rbegin(); theRStrItr != theTempStr.rend(); ++theRStrItr) {
        bool theFoundWhiteSpace = false;
        for (TWSItr theWSItr = WHITECHARS.begin(); theWSItr != WHITECHARS.end(); ++theWSItr) {
            if (*theRStrItr == *theWSItr) {
                // found whitespace for the back, break
                theFoundWhiteSpace = true;
                break;
            }
        }

        if (!theFoundWhiteSpace) {
            // indicates we have encountered the first NON whitespace,
            // reassign outStr and break
            std::wstring::iterator theStrItr = theRStrItr.base();
            outStr = theTempStr.substr(0, std::distance(theTempStr.begin(), theStrItr));
            break;
        }
    }
}
}

//==============================================================================
//	Cotr & Dtor
//==============================================================================
//==============================================================================
/**
*	Ctor
*/
CStrVecSink::CStrVecSink(const long &inPriorityBits)
    : CInfoSink(inPriorityBits)
{
}

//==============================================================================
/**
*	Copy Ctor
*/
CStrVecSink::CStrVecSink(const CStrVecSink &inStrVecSink)
    : CInfoSink(inStrVecSink.m_PriorityBits)
{
    m_StrVec = inStrVecSink.m_StrVec;
}

//==============================================================================
/**
*	Dtor
*/
CStrVecSink::~CStrVecSink()
{
}

//==============================================================================
/**
*	assign operator
*/
CStrVecSink &CStrVecSink::operator=(const CStrVecSink &inStrVecSink)
{
    if (this != &inStrVecSink) {
        m_StrVec = inStrVecSink.m_StrVec;
    }

    return *this;
}

//==============================================================================
//	Interface
//==============================================================================
//==============================================================================
/**
*	Stores incoming info into a multiset
*	@param inStr == the info to consume
*/
void CStrVecSink::Sink(const wchar_t *inStr)
{
    if (inStr != NULL) {
        std::wstring theStr = inStr;
        std::wstring theTrimmed;
        // Trims white spaces
        StrVecSink::TrimWhiteSpace(theStr, theTrimmed);
        m_StrVec.insert(theTrimmed);
    }
}

//==============================================================================
/**
*	Loads XML file using incoming filename, this is one of the 2 ways to use this
*	class.
*	@param inXMLFile == the incoming filename
*/
void CStrVecSink::LoadXML(const std::wstring &inXMLFile)
{
    std::string theFilename = "";
    theFilename << inXMLFile;

    std::wifstream theStream;
    theStream.open(theFilename.c_str(), std::ios_base::in | std::ios_base::binary);

    if (theStream.is_open()) {
        std::wstring theBuffer = L"";
        wchar_t theChar[8];
        // size_t theRead = 0;
        bool theDone = false;
        for (; theDone == false;) {
            theStream.read(theChar, 1);
            if (theStream.gcount() != 1) {
                if (theStream.eof() == false) {
                    throw std::logic_error("CStrVecSink::LoadXML - error");
                } else {
                    theDone = true;
                    break;
                }
            }

            if (theChar[0] == '\n') {
                Sink(theBuffer.c_str());
                theBuffer = L"";
            } else {
                theChar[1] = 0;
                theBuffer << theChar;
            }
        }

        theStream.close();
    }
}

//==============================================================================
/**
*	The total lines collected
*	@return the number of lines collected
*/
long CStrVecSink::GetTotalLines()
{
    return static_cast<long>(m_StrVec.size());
}

//==============================================================================
/**
*	Extracts a line from the str vec
*	@param inIndex == the line index
*	@param outStr == the holding string
*	@return true if success, false otherwise
*/
bool CStrVecSink::RetrieveLine(const long inIndex, std::wstring &outStr)
{
    bool theReturn = true;
    if (inIndex >= static_cast<long>(m_StrVec.size()) || inIndex < 0)
        theReturn = false;

    TStrVec::iterator theItr = m_StrVec.begin();
    std::advance(theItr, inIndex);
    outStr = *theItr;

    return theReturn;
}

//==============================================================================
/**
*	Finds the line equivalent to the inStr
*	@param inStr == line to look for
*	@return -1 for failure, success otherwise
*/
long CStrVecSink::FindThisLine(const std::wstring &inStr)
{
    long theReturn = -1;

    TStrVec::iterator theItr = m_StrVec.find(inStr);
    if (theItr != m_StrVec.end()) {
        // found the first one, it doesnt matter which
        theReturn = static_cast<long>(std::distance(m_StrVec.begin(), theItr));
    }

    return theReturn;
}

//==============================================================================
/**
*	Finds the line equivalent to the inStr and remove it
*	@param inStr == line to look for
*	@return true for success, false otherwise
*/
bool CStrVecSink::FindAndRemoveLine(const std::wstring &inStr)
{
    bool theReturn = true;
    long theLineIndex = -1;

    if ((theLineIndex = FindThisLine(inStr)) == -1)
        theReturn = false;

    if (theReturn) {
        bool theIsDelingLastLine = (theLineIndex == (long)m_StrVec.size() - 1);

        TStrVec::iterator theItr = m_StrVec.begin();
        std::advance(theItr, theLineIndex);

        m_StrVec.erase(theItr);

        if (!theIsDelingLastLine && !m_StrVec.empty())
            theReturn = false;
    }

    return theReturn;
}

//==============================================================================
/**
*	Match differences between inCompareSink and this Sink, puts the result in outSet
*	@param inComapreSink == the sink to compare to
*	@param outSet == the set of differences found
*/
std::vector<std::wstring>::iterator
CStrVecSink::RetrieveDifferenceSet(CStrVecSink &inCompareSink, std::vector<std::wstring> &outSet)
{
    outSet.resize(m_StrVec.size() > inCompareSink.m_StrVec.size() ? m_StrVec.size()
                                                                  : inCompareSink.m_StrVec.size());
    return std::set_difference(m_StrVec.begin(), m_StrVec.end(), inCompareSink.m_StrVec.begin(),
                               inCompareSink.m_StrVec.end(), outSet.begin());
}

//==============================================================================
/**
*	Enable the stored data to be dumped to file
*/
void CStrVecSink::ToFile(const std::wstring &inFilename)
{
    CDumpFileSink theFileSink(inFilename.c_str(), CInfoSink::EDONT_CARE);
    std::wstring theBuffer;

    for (std::multiset<std::wstring>::iterator theItr = m_StrVec.begin(); theItr != m_StrVec.end();
         ++theItr) {
        theBuffer << *theItr << "\n";
    }
    theFileSink.Sink(theBuffer.c_str());
}