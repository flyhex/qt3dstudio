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
#include "Qt3DSCommonPrecompile.h"
#include "DumpFileSink.h"
#include <fstream>
#include "StrUtilities.h"

/**
*		Ctor
*/
CDumpFileSink::CDumpFileSink(const wchar_t *inFilename, const long &inPriority)
    : CInfoSink(inPriority)
    , m_Filename(inFilename)
{
    std::string theWFilename = "";
    theWFilename << inFilename;

    std::wofstream theStream;
    theStream.open(theWFilename.c_str(),
                   std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if (theStream.is_open())
        theStream.close();
    else
        throw std::logic_error("CDumpFileSink::CDumpFileSink - cannot open file");
}

//==============================================================================
/**
*		Dtor
*/
CDumpFileSink::~CDumpFileSink()
{
}

//==============================================================================
//	Interface
//==============================================================================
//==============================================================================
/**
*		Writes to a file, throws on error
*		@param inStr == the string to write, whcar_t type
*/
void CDumpFileSink::Sink(const wchar_t *inStr)
{
    // CConvertStr< wchar_t, char > theConvert;
    std::string theFilename = "";
    theFilename << m_Filename;

    std::wofstream theStream;

    theStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    theStream.open(theFilename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::app
                       | std::ios_base::binary);

    if (theStream.is_open()) {
        theStream.seekp(std::ios_base::end);
        theStream << inStr << L"\n";
        theStream.flush();
        theStream.close();

    } else {
        throw std::logic_error("CDumpFileSink::Sink - cannot open file");
    }
}
