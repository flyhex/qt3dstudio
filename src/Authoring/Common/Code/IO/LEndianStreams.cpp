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
#include "stdafx.h" //Where the defines for big or little endian must lie
#include "LEndianStreams.h"
#include "IOLibraryException.h"
#include "UICEndian.h"

CLEndianInStream::CLEndianInStream()
{
}

CLEndianInStream::CLEndianInStream(CInputStream *inStream)
    : CInStream(inStream)
{
}

CLEndianInStream::CLEndianInStream(const CInStream &inStream)
    : CInStream(inStream)
{
}

CLEndianInStream::~CLEndianInStream()
{
}

CInStream *CLEndianInStream::Copy(CInputStream *inBase)
{
    return new CLEndianInStream(inBase);
}

CInStream *CLEndianInStream::Copy()
{
    return new CLEndianInStream();
}

CInStream &CLEndianInStream::operator>>(double &)
{
    throw CIOException();
}

CInStream &CLEndianInStream::operator>>(float &outValue)
{
    long theData;
    ReadData(&theData, sizeof(theData));
    long *theValue = reinterpret_cast<long *>(&outValue);
    LTOH4(theData, *theValue);
    return *this;
}

CInStream &CLEndianInStream::operator>>(long &outValue)
{
    long theData;
    ReadData(&theData, sizeof(theData));
    long *theValue = reinterpret_cast<long *>(&outValue);
    LTOH4(theData, *theValue);
    return *this;
}

CInStream &CLEndianInStream::operator>>(unsigned long &outValue)
{
    unsigned long theData;
    ReadData(&theData, sizeof(theData));
    unsigned long *theValue = reinterpret_cast<unsigned long *>(&outValue);
    LTOH4(theData, *theValue);
    return *this;
}

CInStream &CLEndianInStream::operator>>(short &outValue)
{
    short theTemp;
    ReadData(&theTemp, sizeof(theTemp));
    LTOH2(theTemp, outValue);
    return *this;
}

CInStream &CLEndianInStream::operator>>(wchar_t &outValue)
{
    wchar_t theTemp;
    ReadData(&theTemp, sizeof(theTemp));
    LTOH2(theTemp, outValue);
    return *this;
}

CLEndianOutStream::CLEndianOutStream()
{
}

CLEndianOutStream::CLEndianOutStream(COutputStream *inStream)
    : COutStream(inStream)
{
}

CLEndianOutStream::~CLEndianOutStream()
{
}

COutStream *CLEndianOutStream::Copy(COutputStream *inBase)
{
    return new CLEndianOutStream(inBase);
}

COutStream *CLEndianOutStream::Copy()
{
    return new CLEndianOutStream();
}

COutStream &CLEndianOutStream::operator<<(double)
{
    throw CIOException();
}

COutStream &CLEndianOutStream::operator<<(float inValue)
{
    long *theTemp = reinterpret_cast<long *>(&inValue);
    long theWritten;
    HTOL4(*theTemp, theWritten);
    WriteData(&theWritten, sizeof(theWritten));
    return *this;
}
COutStream &CLEndianOutStream::operator<<(long inValue)
{
    long *theTemp = reinterpret_cast<long *>(&inValue);
    long theWritten;
    HTOL4(*theTemp, theWritten);
    WriteData(&theWritten, sizeof(theWritten));
    return *this;
}
COutStream &CLEndianOutStream::operator<<(unsigned long inValue)
{
    long *theTemp = reinterpret_cast<long *>(&inValue);
    long theWritten;
    HTOL4(*theTemp, theWritten);
    WriteData(&theWritten, sizeof(theWritten));
    return *this;
}
COutStream &CLEndianOutStream::operator<<(short inValue)
{
    short *theTemp = reinterpret_cast<short *>(&inValue);
    short theWritten;
    HTOL2(*theTemp, theWritten);
    WriteData(&theWritten, sizeof(theWritten));
    return *this;
}

COutStream &CLEndianOutStream::operator<<(wchar_t inValue)
{
    short *theTemp = reinterpret_cast<short *>(&inValue);
    short theWritten;
    HTOL2(*theTemp, theWritten);
    WriteData(&theWritten, sizeof(theWritten));
    return *this;
}
