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
#include <string.h>
#include <sstream>
#include "BufferedOutputStream.h"

CBufferedOutputStream::CBufferedOutputStream(COutputStream *inOutputStream)
    : m_OutputStream(inOutputStream)
{
    const long MAXBUFFERLENGTH = 33554432; // 32MB
    m_BufferLength = MAXBUFFERLENGTH;
    m_Buffer = new unsigned char[m_BufferLength];
    m_BufferUsed = 0;
}

CBufferedOutputStream::~CBufferedOutputStream()
{
    Flush();
    delete[] m_Buffer;
}

long CBufferedOutputStream::Write(const void *inBuffer, long inBufferLength)
{
    const char *theBuffer = reinterpret_cast<const char *>(inBuffer);

    long theBufferLength = inBufferLength;
    long theWroteCount = 0;

    while (theBufferLength > m_BufferLength - m_BufferUsed) {
        ::memcpy(&m_Buffer[m_BufferUsed], theBuffer, m_BufferLength - m_BufferUsed);

        theWroteCount += m_BufferLength - m_BufferUsed;

        theBufferLength -= (m_BufferLength - m_BufferUsed);
        theBuffer += (m_BufferLength - m_BufferUsed);
        m_BufferUsed = m_BufferLength;

        Flush();
    }

    ::memcpy(&m_Buffer[m_BufferUsed], theBuffer, theBufferLength);
    theWroteCount = inBufferLength;
    m_BufferUsed += theBufferLength;

    return theWroteCount;
}

bool CBufferedOutputStream::Write(std::wostringstream &inStringStream)
{
    // Writing out to file
    long theDataLength = static_cast<long>(inStringStream.str().length()) * sizeof(wchar_t);

#ifdef KDAB_TEMPORARILY_REMOVED
    std::string theMultiByteBuffer;
    int theSize = ::WideCharToMultiByte(CP_UTF8, 0, inStringStream.str().c_str(), theDataLength / 2,
                                        0, 0, 0, 0);
    if (theSize > 0) {
        theMultiByteBuffer.resize(theSize);
        ::WideCharToMultiByte(CP_UTF8, 0, inStringStream.str().c_str(), theDataLength / 2,
                              const_cast<char *>(theMultiByteBuffer.c_str()), theSize, 0, 0);
        Write(theMultiByteBuffer.c_str(), theSize);
        return true;
    }
#endif

    return false;
}

void CBufferedOutputStream::Close()
{
    this->Flush();
    m_OutputStream->Close();
}

bool CBufferedOutputStream::IsValid() const
{
    if (m_OutputStream != NULL)
        return m_OutputStream->IsValid();
    return false;
}

void CBufferedOutputStream::Flush()
{
    long theWroteCount = m_OutputStream->Write(m_Buffer, m_BufferUsed);

    if (theWroteCount != m_BufferUsed) {
        // Short write, move the unwritten data to the beginning of the buffer.
        ::memmove(m_Buffer, &m_Buffer[theWroteCount], m_BufferUsed - theWroteCount);
        m_BufferUsed -= theWroteCount;
    } else {
        m_BufferUsed = 0;
    }
}

CBufferedSeekOutputStream::CBufferedSeekOutputStream(CSeekOutputStream *inStream,
                                                     long inBufferSize /*=1048576*/)
    : m_OutputStream(inStream)
    , m_BufferLength(inBufferSize)
    , m_BufferUsed(0)
    , m_BufferPosition(0)
    , m_RunningCounter(0)
{
    m_Buffer = new unsigned char[inBufferSize];
}

CBufferedSeekOutputStream::~CBufferedSeekOutputStream()
{
    Close();
    if (m_Buffer) {
        delete[] m_Buffer;
    }
}

long CBufferedSeekOutputStream::Write(const void *inBuffer, const long inBufferLength)
{
    long theDataRead = 0;
    long theDataLeft = inBufferLength;
    long theBufferSizeWritable = m_BufferLength - m_BufferPosition;
    const char *theBuffer = static_cast<const char *>(inBuffer);

    /* Early flushing?
    if ( inBufferLength > m_BufferLength )
    {
            FlushBuffer( );
            theDataRead = m_OutputStream->Write( theBuffer, inBufferLength );
            return inBufferLength;
    }
    */

    while (theDataLeft > theBufferSizeWritable) {
        ::memcpy(&m_Buffer[m_BufferPosition], &theBuffer[theDataRead], theBufferSizeWritable);
        m_BufferUsed += theBufferSizeWritable;
        FlushBuffer();
        theDataLeft -= theBufferSizeWritable;
        theDataRead += theBufferSizeWritable;
        theBufferSizeWritable = m_BufferLength;
    }

    ::memcpy(&m_Buffer[m_BufferPosition], &theBuffer[theDataRead], theDataLeft);
    m_BufferPosition += theDataLeft;
    m_BufferUsed = Q3DStudio::MAX(m_BufferUsed, m_BufferPosition);

    return inBufferLength;
}

void CBufferedSeekOutputStream::FlushBuffer()
{
    long theFlushPoint = 0;
    long theWroteCount = 0;
    while (m_BufferUsed != 0) {
        theWroteCount = m_OutputStream->Write(&m_Buffer[theFlushPoint], m_BufferUsed);
        m_BufferUsed -= theWroteCount;
        theFlushPoint += theWroteCount;
    }

    m_BufferPosition = 0;
}

void CBufferedSeekOutputStream::Flush()
{
    FlushBuffer();
    m_OutputStream->Flush();
}

void CBufferedSeekOutputStream::Close()
{
    Flush();
    m_OutputStream->Close();
}

bool CBufferedSeekOutputStream::IsValid() const
{
    return m_OutputStream->IsValid();
}

// CSeekOutputStream
long CBufferedSeekOutputStream::Seek(ESeekPosition inPosition, long inOffset)
{
    long theUnflushedPosition = m_OutputStream->GetCurrentPosition();

    long theAbsolutePosition = -1;
    switch (inPosition) {
    case Q3DStudio::ISeekable::EBEGIN:
        theAbsolutePosition = inOffset;
        break;
    case Q3DStudio::ISeekable::ECURRENT:
        theAbsolutePosition = theUnflushedPosition + m_BufferPosition + inOffset;
        break;
        // case Q3DStudio::ISeekable::EEND: // This case need not be optimized
        // break;
    default:
        break;
    }

    if (theAbsolutePosition >= theUnflushedPosition
        && theAbsolutePosition <= theUnflushedPosition + m_BufferUsed) {
        m_BufferPosition = theAbsolutePosition - theUnflushedPosition;
        return m_BufferPosition;
    } else {
        FlushBuffer();
        return m_OutputStream->Seek(inPosition, inOffset);
    }
}

long CBufferedSeekOutputStream::GetCurrentPosition()
{
    return m_OutputStream->GetCurrentPosition() + m_BufferPosition;
}
