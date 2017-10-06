/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "stdafx.h"
#include "FormattedInputStream.h"
#include "UICEndian.h"
#include "FileOutputStream.h"
//#include "BufferedOutputStream.h"

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

CFormattedInputStream::CFormattedInputStream(CInputStream *inInputStream)
    : m_InputStream(inInputStream)
    , m_IsBigEndian(false)
    , m_Count(0)
{
}

CFormattedInputStream::~CFormattedInputStream()
{
}

long CFormattedInputStream::Read(void *inData, long inLength)
{
    m_Count += inLength;

    return m_InputStream->Read(inData, inLength);
}

void CFormattedInputStream::Close()
{
    m_InputStream->Close();
}

bool CFormattedInputStream::IsValid()
{
    return m_InputStream->IsValid();
}

CFormattedInputStream &CFormattedInputStream::operator>>(long &inLong)
{
    m_Count += sizeof(long);

    m_InputStream->Read(&inLong, sizeof(long));
    BTOH4(inLong, inLong);
    SWAP4BYTES(inLong, inLong);
    return *this;
}

CFormattedInputStream &CFormattedInputStream::operator>>(unsigned long &inLong)
{
    m_Count += sizeof(unsigned long);

    m_InputStream->Read(&inLong, sizeof(unsigned long));
    BTOH4(inLong, inLong);
    SWAP4BYTES(inLong, inLong);
    return *this;
}

CFormattedInputStream &CFormattedInputStream::operator>>(float &inFloat)
{
    m_Count += sizeof(float);

    m_InputStream->Read(&inFloat, sizeof(float));
    //	BTOH4( inFloat, inFloat );

    return *this;
}

CFormattedInputStream &CFormattedInputStream::operator>>(short &inShort)
{
    m_Count += sizeof(short);

    m_InputStream->Read(&inShort, sizeof(short));
    BTOH2(inShort, inShort);
    SWAP2BYTES(inShort, inShort);
    return *this;
}

CFormattedInputStream &CFormattedInputStream::operator>>(Q3DStudio::UICChar &inUICChar)
{
    m_Count += sizeof(Q3DStudio::UICChar);

    m_InputStream->Read(&inUICChar, sizeof(Q3DStudio::UICChar));
    BTOH2(inUICChar, inUICChar);
    SWAP2BYTES(inUICChar, inUICChar);
    return *this;
}

CFormattedInputStream &CFormattedInputStream::operator>>(bool &inBool)
{
    m_Count += sizeof(bool);

    m_InputStream->Read(&inBool, sizeof(bool));

    return *this;
}

unsigned char CFormattedInputStream::ReadUnsignedChar()
{
    m_Count += sizeof(unsigned char);

    unsigned char theChar;
    m_InputStream->Read(&theChar, sizeof(unsigned char));
    return theChar;
}

unsigned short CFormattedInputStream::ReadUnsignedShort()
{
    m_Count += sizeof(unsigned short);

    unsigned short theShort;
    m_InputStream->Read(&theShort, sizeof(unsigned short));
    BTOH2(theShort, theShort);
    SWAP2BYTES(theShort, theShort);
    return theShort;
}

void CFormattedInputStream::CopyToFile(const CUICFile &inFile, long inLength,
                                       bool inCloseStream /*= true */)
{
    Q3DStudio::CString thePath = inFile.GetAbsolutePosixPath();

    CFileOutputStream theFileStream(thePath);

    unsigned long theLength = inLength;
    unsigned long theBufferSize = 1024;
    while (theBufferSize < theLength / 10 && theBufferSize < 1e6)
        theBufferSize *= 2; // double until we have a buffer that is about %10 of file size
    char *theBuffer = new char[theBufferSize];

    unsigned long theReadCount = theBufferSize;
    unsigned long theTotalReadCount = 0;

    // Loop around reading from the archive and writing to the file.
    while (theTotalReadCount < theLength) {
        if (theLength - theTotalReadCount < theReadCount)
            theReadCount = theLength - theTotalReadCount;

        Read(theBuffer, theReadCount);
        theFileStream.Write(theBuffer, theReadCount);

        theTotalReadCount += theReadCount;
    }

    if (inCloseStream)
        theFileStream.Close();
    delete[] theBuffer;
}