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
#include "BufferedInputStream.h"
#include "IOLibraryException.h"
#include <string.h>
#include <sstream>

CBufferedInputStream::CBufferedInputStream(CInputStream *inInputStream, long inBufferLength)
    : m_InputStream(inInputStream)
{
    const long MAXBUFFERLENGTH = 33554432; // 32MB
    if (inBufferLength > MAXBUFFERLENGTH)
        m_BufferLength = MAXBUFFERLENGTH;
    else
        m_BufferLength = inBufferLength;

    m_Buffer = new unsigned char[m_BufferLength];
    m_BufferPosition = 0;
    m_BufferUsedCount = 0;
}

CBufferedInputStream::~CBufferedInputStream()
{
    this->Close();
    delete[] m_Buffer;
}

long CBufferedInputStream::Read(void *inBuffer, long inBufferLength)
{
    long nread = 1;
    long theReadCount = 0;

    // Loop around until the entire inBuffer has been filled (or an error occurs)
    // This must be able to handle requests larger than this buffer.
    // while the number of bytes read is less than the requested length...
    while (theReadCount < inBufferLength) {
        // If the amount of data left to read is greater than the amount currently in the buffer...
        if ((inBufferLength - theReadCount) > (m_BufferUsedCount - m_BufferPosition)) {
            char *theBuffer = reinterpret_cast<char *>(inBuffer);
            // Copy what is in this buffer into the outgoing buffer
            ::memcpy(&theBuffer[theReadCount], &m_Buffer[m_BufferPosition],
                     m_BufferUsedCount - m_BufferPosition);

            // Adjust the number of bytes read
            theReadCount += m_BufferUsedCount - m_BufferPosition;

            // Fill up the buffer to continue reading
            nread = FillBuffer();
            if (nread == 0 || nread == (unsigned long)-1) {
                break;
            }
        }
        // The amount of data requested is less than the amount in the buffer,
        // so the request comes completely from the buffer.
        else {
            char *theBuffer = reinterpret_cast<char *>(inBuffer);
            ::memcpy(&theBuffer[theReadCount], &m_Buffer[m_BufferPosition],
                     inBufferLength - theReadCount);
            m_BufferPosition += inBufferLength - theReadCount;
            theReadCount += inBufferLength - theReadCount;
        }
    }
    return theReadCount;
}

void CBufferedInputStream::Close()
{
    if (m_InputStream)
        m_InputStream->Close();
}

bool CBufferedInputStream::IsValid() const
{
    if (m_InputStream != NULL)
        return m_InputStream->IsValid();
    return false;
}

/** Fill up the internal buffer from the sub-inputstream.
 * This just calls read on the stream and returns the amount that was read.
 * @return the amount of data read.
 */
long CBufferedInputStream::FillBuffer()
{
    long theReadCount = m_InputStream->Read(m_Buffer, m_BufferLength);

    if (theReadCount != (unsigned long)-1) {
        m_BufferUsedCount = theReadCount;
        m_BufferPosition = 0;
    } else {
        m_BufferUsedCount = 0;
        m_BufferPosition = 0;
    }

    return theReadCount;
}

/** Insert the data into the buffer.
 * This will put the data at the end of the data currently in the buffer, and is
 * meant primarily for initializing the buffer with data. The amount of data
 * cannot be larger than the amount of space left in the buffer.
 * @param inData the data to be inserted.
 * @param inDataLength the amount of data to be inserted.
 * @return true if successful, false if not enough space.
 */
void CBufferedInputStream::InsertIntoBuffer(void *inData, long inDataLength)
{
    // Not enough space in the buffer for the data
    if (inDataLength > m_BufferLength - m_BufferUsedCount)
        throw CIOException();

    // Copy the data into the buffer and adjust the buffer used count
    ::memcpy(&m_Buffer[m_BufferUsedCount], inData, inDataLength);
    m_BufferUsedCount += inDataLength;
}
