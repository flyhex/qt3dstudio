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
#include "FileInputStream.h"
#include "IOLibraryException.h"

CFileInputStream::CFileInputStream(const QString &inFilename)
    : m_FileName(inFilename)
    , m_ReadBytesFromCurrentBuffer(0)
    , m_AvailableBytesInBuffer(0)
    , m_File(inFilename)
{
    m_InternalBuffer[0] = '\0';
    if (!m_File.open(QFile::ReadOnly))
        throw CIOException();
}

CFileInputStream::~CFileInputStream()
{
    Close();
}

/**
 *	Seek to inOffset from inBegin.  If the result is past either end of the
 *	file then the next byte read from the stream will either not exist or come
 *	from the beginning, ie the file position pointer will not be moved past
 *	the end or in front of the beginning of the file.
 *	@param inOffset The offset to seek to.
 *	@param inBegin where to seek from, i.e. current position, begin or end.
 *	@return success if this is a valid file handle.
 */
long CFileInputStream::Seek(Q3DStudio::ISeekable::ESeekPosition inBegin, long inOffset)
{
    if (!IsValid())
        throw CIOException();
    bool result = false;
    switch (inBegin) {
    case Q3DStudio::ISeekable::EBEGIN: {
        result = m_File.seek(inOffset);
        break;
    }
    case Q3DStudio::ISeekable::ECURRENT: {
        inOffset -= m_AvailableBytesInBuffer;
        result = m_File.seek(m_File.pos() + inOffset);
        break;
    }
    case Q3DStudio::ISeekable::EEND: {
        result = m_File.seek(m_File.size() + inOffset);
        break;
    }
    default: {
        throw CIOException();
        break;
    }
    }

    m_ReadBytesFromCurrentBuffer = 0;
    m_AvailableBytesInBuffer = 0;

    if (result == false)
        throw CIOException();
    return 0; // okay because we implement seeking
}

/**
 *	Read a chunk of the file into the inBuffer.
 * This method will read as much as possible into the buffer. If not enough
 * data exists in the file the outReadCount will reflect the amount of data
 * successfully read. If the end of file has been reached the read count is
 * 0. An error will set the read count to -1. This function will only fail if
 * no data could be read (EOF or error).
 * @param inBuffer destination for data being read in.
 * @param inBufferLength the amount of data to be read.
 * @param outReadCount the amount of data read.
 * @return <HRESULT> S_OK if successful or if partial read succeeded.
 */
long CFileInputStream::Read(void *inBuffer, long inBufferLength)
{
    long theReadCount = 0;
    char *theBufferPointer = static_cast<char *>(inBuffer);

    // If reading more than the internal buffer, directly use the buffer supplied.
    if (inBufferLength > INTERNAL_BUFFER_SIZE) {
        long theActualReadCount = inBufferLength;
        if (m_ReadBytesFromCurrentBuffer > 0) { // dump whatever that was previously read in first
            ::memcpy(theBufferPointer, &m_InternalBuffer[m_ReadBytesFromCurrentBuffer],
                     m_AvailableBytesInBuffer);
            theActualReadCount -= m_AvailableBytesInBuffer;
            theBufferPointer += m_AvailableBytesInBuffer;
        }
        if ((theReadCount = m_File.read(theBufferPointer, theActualReadCount)) == -1)
            throw CIOException();

        // update and reset the counters
        theReadCount += m_AvailableBytesInBuffer;
        m_ReadBytesFromCurrentBuffer = m_AvailableBytesInBuffer = 0;
    } else {
        long theRequiredLength = inBufferLength;
        while (true) {
            BOOL theSuccess = TRUE;
            if (m_ReadBytesFromCurrentBuffer
                <= 0) { // If internal buffer is empty, read a chunk first
                long theActualReadCount = m_File.read(m_InternalBuffer, INTERNAL_BUFFER_SIZE);
                theSuccess = theActualReadCount > 0;
                m_AvailableBytesInBuffer = theActualReadCount;
            }
            if (!theSuccess)
                throw CIOException();
            // End of file
            if (m_AvailableBytesInBuffer <= 0)
                break;

            long theAvailableBytes = (m_AvailableBytesInBuffer < theRequiredLength)
                ? m_AvailableBytesInBuffer
                : theRequiredLength;
            // copy the relevant portion over
            ::memcpy(theBufferPointer, &m_InternalBuffer[m_ReadBytesFromCurrentBuffer],
                     theAvailableBytes);
            // update the number of bytes 'read' so far
            theReadCount += theAvailableBytes;
            // if the required data is sufficient, we are done
            if (theAvailableBytes >= theRequiredLength) {
                m_ReadBytesFromCurrentBuffer +=
                    theAvailableBytes; // so the next read will start from here.
                if (m_ReadBytesFromCurrentBuffer >= INTERNAL_BUFFER_SIZE)
                    m_ReadBytesFromCurrentBuffer = 0; // reset
                m_AvailableBytesInBuffer -= theAvailableBytes;
                break;
            } else // loop back and read more data
            {
                m_ReadBytesFromCurrentBuffer = 0;
                theBufferPointer += theAvailableBytes; // move the pointer too
                theRequiredLength -= theAvailableBytes; // this much is still required
            }
        }
    }
    return theReadCount;
}

/**
 *	Close the file handle.
 */
void CFileInputStream::Close()
{
    m_File.close();
    m_ReadBytesFromCurrentBuffer = m_AvailableBytesInBuffer = 0;
}

/**
 *	Returns true if the file is open and can be read.
 *	Does not check for EOF.
 *	@param outResult true if file is open.
 */
bool CFileInputStream::IsValid() const
{
    return m_File.isOpen();
}

QString CFileInputStream::GetSource() const
{
    return m_FileName;
}
