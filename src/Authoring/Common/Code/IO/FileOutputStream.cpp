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

//==============================================================================
//	Includes
//==============================================================================
#include "FileOutputStream.h"
#include "IOLibraryException.h"

#include <QByteArray>
#include <QString>

/* STL implementation
CFileOutputStream::CFileOutputStream( const Q3DStudio::CString& inFilename, bool inAppend, long
inBuffersize )
{
        m_Buffer = new char[inBuffersize];
        m_FileStream = new std::ofstream( inFilename.GetMulti( ), inAppend ? ( std::ios_base::app |
std::ios_base::binary ) : ( std::ios_base::out | std::ios_base::trunc | std::ios_base::binary ) );

        m_FileStream->rdbuf( )->pubsetbuf( m_Buffer, inBuffersize );
        m_FileStream->sync_with_stdio( false );

        m_FileStream->unsetf( std::ios_base::unitbuf );
        m_FileStream->tie( NULL );
}

CFileOutputStream::~CFileOutputStream( )
{
        Close( );
        delete m_FileStream;
        delete[] m_Buffer;
}

long CFileOutputStream::Write( const void* inBuffer, long inBufferLength )
{
        m_FileStream->write( static_cast<const char*>( inBuffer ), inBufferLength );
        return inBufferLength;
}

void CFileOutputStream::Flush( )
{
        m_FileStream->flush( );
}

void CFileOutputStream::Close( )
{
        m_FileStream->close( );
}

bool CFileOutputStream::IsValid( )
{
        return m_FileStream->good( );
}

//CSeekOutputStream
long CFileOutputStream::Seek( Q3DStudio::ISeekable::ESeekPosition inBegin, long inOffset )
{
        switch ( inBegin )
        {
                case Q3DStudio::ISeekable::EBEGIN:
                        m_FileStream->seekp( inOffset, std::ios_base::beg );
                break;
                case Q3DStudio::ISeekable::EEND:
                        m_FileStream->seekp( inOffset, std::ios_base::end );
                break;
                case Q3DStudio::ISeekable::ECURRENT:
                        m_FileStream->seekp( inOffset, std::ios_base::cur );
                break;
        }

        return 0;
}

long CFileOutputStream::GetCurrentPosition( )
{
        return m_FileStream->tellp( );
}
*/

CFileOutputStream::CFileOutputStream(const Q3DStudio::CString &inFilename, bool inAppend /* = false */)
    : m_Position(0)
    , m_Length(0)
{
#ifdef WIN32
    m_File = ::CreateFile(inFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          (inAppend) ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (m_File == INVALID_HANDLE_VALUE)
        throw CIOException();
    // In this case, set the file pointer to the end of the file
    if (inAppend)
        ::SetFilePointer(m_File, 0, NULL, FILE_END);
#else
    QByteArray theUTFBuffer = inFilename.toQString().toUtf8();
    m_File = ::fopen(theUTFBuffer.constData(), (inAppend) ? "a" : "w");
    if (m_File == NULL)
        throw CIOException();
#endif
}

CFileOutputStream::~CFileOutputStream()
{
    this->Close();
}

/** Write the specified buffer to this file.
 * @param inBuffer the bytes to write to this file.
 * @param inBufferLength the number of bytes to write.
 * @param outWroteCount the number of bytes actually written.
 */
long CFileOutputStream::Write(const void *inBuffer, long inBufferLength)
{
    long theWroteCount = 0;
#ifdef WIN32
    ::WriteFile(m_File, inBuffer, inBufferLength, (DWORD *)&theWroteCount, NULL);
    if (theWroteCount <= 0 && inBufferLength > 0)
        throw CIOException();
#else
    theWroteCount = ::fwrite(inBuffer, 1, inBufferLength, m_File);
    if (theWroteCount != inBufferLength)
        throw CIOException();
#endif
    m_Position += theWroteCount;
    if (m_Position > m_Length)
        m_Length = m_Position;

    return theWroteCount;
}

void CFileOutputStream::Flush()
{
#ifdef WIN32
#else
    ::fflush(m_File);
#endif
}

/** Closes the file.
 * This method should be called to eliminate handle leaks.
 */
void CFileOutputStream::Close()
{
#ifdef WIN32
    if (m_File != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_File);
        m_File = INVALID_HANDLE_VALUE;
    }
#else
    if (m_File != NULL) {
        ::fflush(m_File);
        ::fclose(m_File);
        m_File = NULL;
    }
#endif
}

/** Check to see if thus stream is valid (opened successfully and still open).
 */
bool CFileOutputStream::IsValid()
{
#ifdef WIN32
    return (m_File != INVALID_HANDLE_VALUE);
#else
    return (m_File != NULL);
#endif
}

/**	Seek to inOffset from inBegin.  If the result is past either end of the
  *	file then the next byte read from the stream will either not exist or come
  *	from the beginning, ie the file position pointer will not be moved past
  *	the end or in front of the beginning of the file.
  *	@param inOffset The offset to seek to.
  *	@param inBegin where to seek from, i.e. current position, begin or end.
  *	@return success if this is a valid file handle.
  */
long CFileOutputStream::Seek(Q3DStudio::ISeekable::ESeekPosition inBegin, long inOffset)
{
    if (!IsValid())
        throw CIOException();
#ifdef KDAB_TEMPORARILY_REMOVED
    long theSeek = INVALID_SET_FILE_POINTER;
    switch (inBegin) {
    case Q3DStudio::ISeekable::EBEGIN: {
        theSeek = ::SetFilePointer(m_File, inOffset, NULL, FILE_BEGIN);
        m_Position = inOffset;
        break;
    }
    case Q3DStudio::ISeekable::ECURRENT: {
        theSeek = ::SetFilePointer(m_File, inOffset, NULL, FILE_CURRENT);
        m_Position += inOffset;
        break;
    }
    case Q3DStudio::ISeekable::EEND: {
        theSeek = ::SetFilePointer(m_File, inOffset, NULL, FILE_END);
        m_Position = m_Length + inOffset;
        break;
    }
    default: {
        throw CIOException();
        break;
    }
    }
    if (theSeek == INVALID_SET_FILE_POINTER)
        throw CIOException();
#endif
    return 0; // okay because we implement seeking
}

long CFileOutputStream::GetCurrentPosition()
{
    return m_Position;
}
