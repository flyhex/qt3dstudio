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
#include "IOStreams.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "IOLibraryException.h"

#include <wctype.h>
#include <ctype.h>
#include <wchar.h>
#include <limits>
#include <stdlib.h>

/** Create an input stream wrapper with no stream.
 */
CInStream::CInStream()
    : m_Stream(NULL)
{
}

/** Create an input stream wrapper for inStream.
 * @param inStream the stream to be wrapped.
 */
CInStream::CInStream(CInputStream *inStream)
{
    m_Stream = inStream;
}

/** Create an input stream wrapper duplicating the old stream
 * @param inOldStream the stream to be copied upon construction
 */
CInStream::CInStream(const CInStream &inStream)
{
    m_Stream = inStream.m_Stream;
    m_URLString = inStream.m_URLString;
}

/**
  * Destroy an input stream
  */
CInStream::~CInStream()
{
}

/** Set the input stream this is wrapping.
 * @param inStream theStream to be wrapped.
 */
void CInStream::SetInputStream(CInputStream *inStream)
{
    m_Stream = inStream;
}

/** Get the input stream this is wrapping.
 * @param outStream theStream to being wrapped.
 */
CInputStream *CInStream::GetInputStream()
{
    return m_Stream;
}

void CInStream::SetURL(const wchar_t *inURLString)
{
    m_URLString = inURLString;
}

void CInStream::GetURL(wchar_t *ioURLString, const long &inSize)
{
    long theEndCount = m_URLString.Length();
    wchar_t *theString;
    theString = const_cast<wchar_t *>(static_cast<const wchar_t *>(m_URLString));

    if (inSize >= m_URLString.Length()) {
        // The swprintf just refused to work
        // =================================
        // POSSIBLE PROBLEM: the swprintf reads unsigned short (2 bytes) instead
        // of wchar_t (4 bytes)
        // as alphanumerics seldom exceeds 1 byte, the 3rd & 4th bytes of a wchar_t was usu
        // a '\0'. swprintf ASSUMES EOS upon encountering '\0' and hence FAILURE
        //
        // A SLOPPY SOLUTION: blind copying theString to ioURLString, very amateurish, but it did
        // the job...

        // the stuff that dint work:
        // theCharCount = _aswprintf( ioURLString, inSize, _LSTR( "%s" ), theString );
        // ... that's even when you call the swprintf directly...
        // theCharCount = swprintf(ioURLString, inSize, _LSTR( "%s" ), (unsigned short *)theString );

        // the sloppy solution
        for (long theCharCount = 0; theCharCount < theEndCount; ++theCharCount) {
            ioURLString[theCharCount] = theString[theCharCount];
        }

    } else
        throw CIOException();
}

CInStream *CInStream::Copy(CInputStream *inBase)
{
    return new CInStream(inBase);
}

CInStream *CInStream::Copy()
{
    return new CInStream();
}
//==============================================================================
/**
 * Read: Get the specified number of bytes from the stream.
 * @param outBuffer void*. This buffer better be at least inByteCount in length.
 * @param inByteCount long.  This is the length of the buffer.
 * @return The number of bytes read.
 *		In most cases the return value will be the same as the parameter inByteCount.
 *However
 *		when the end of file comes before the specified number of bytes is read, the return
 *value will
 *		will only contain the read.
 * @see Write
 */
//==============================================================================
long CInStream::Read(void *outBuffer, long inByteCount)
{
    return m_Stream->Read(outBuffer, inByteCount);
}

void CInStream::ReadData(void *inBuffer, long inBufferLength)
{
    long theLength = m_Stream->Read(inBuffer, inBufferLength);
    if (theLength != inBufferLength) {
        throw CIOException();
    }
}

void CInStream::Close()
{
    m_Stream->Close();
}

bool CInStream::IsValid()
{
    return m_Stream->IsValid();
}

COutStream::COutStream()
    : m_Stream(NULL)
{
}

COutStream::COutStream(COutputStream *inStream)
{
    m_Stream = inStream;
}

void COutStream::SetOutputStream(COutputStream *inStream)
{
    m_Stream = inStream;
}

void COutStream::SetURL(const wchar_t *inURLString)
{
    m_URLString = inURLString;
}

void COutStream::GetURL(wchar_t *ioURLString, const long &inSize)
{
    if (inSize >= m_URLString.Length())
#ifdef WIN32
        _aswprintf(ioURLString, inSize, _LSTR("%ls"), static_cast<const wchar_t *>(m_URLString));
#else
        swprintf(ioURLString, inSize, _LSTR("%ls"), static_cast<const wchar_t *>(m_URLString));
#endif
    else
        throw CIOException();
}

COutStream *COutStream::Copy(COutputStream *inBase)
{
    return new COutStream(inBase);
}

COutStream *COutStream::Copy()
{
    return new COutStream();
}

//==============================================================================
/**
 *		Write: Puts the specified number of bytes out to the stream.
 *
 *		@param		inBuffer		const void*.
 *						This is a pointer the chunk of data that will be written out
 *the stream.
 *						It must be at least inByteCount in length.  If the pointer is
 *NULL, nothing will happen.
 *						If the pointer is dangling, the worst possible error will
 *occur.
 *		@param		inByteCount		long.	This is the number of bytes to be
 *written.
 *
 *		@return 	The number of bytes written.
 *		@see		Read
 */
//==============================================================================
long COutStream::Write(const void *inBuffer, long inByteCount)
{
    return m_Stream->Write(inBuffer, inByteCount);
}

void COutStream::WriteData(const void *inBuffer, long inByteCount)
{
    long theLength = m_Stream->Write(inBuffer, inByteCount);
    if (theLength < inByteCount) {
        throw CIOException();
    }
}

void COutStream::Flush()
{
    m_Stream->Flush();
}

void COutStream::Close()
{
    m_Stream->Close();
}

bool COutStream::IsValid()
{
    return m_Stream->IsValid();
}

//==============================================================================
/**
 *		operator << double: Writes a double to the stream.
 *					This operator will write either a textual representation of the
 *inValue, or just
 *					X number of bytes to the stream (X is the number of bytes used to
 *hold the type).
 *		@param		inValue			double
 *
 *		@return 	The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(double inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 *		operator << float: Writes a float to the stream.
 *					This operator will write either a textual representation of the
 *inValue, or just
 *					X number of bytes to the stream (X is the number of bytes used to
 *hold the type).
 *		@param		inValue			double
 *
 *		@return 	The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(float inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 *		operator << long: Writes a long to the stream.
 *					This operator will write either a textual representation of the
 *inValue, or just
 *					X number of bytes to the stream (X is the number of bytes used to
 *hold the type).
 *
 *		@param		inValue			long
 *
 *		@return 	The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(long inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 * operator << unsigned long: Writes a long to the stream.
 * This operator will write either a textual representation of the inValue, or just
 * X number of bytes to the stream (X is the number of bytes used to hold the type).
 * @param inValue long
 * @return The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(unsigned long inValue)
{
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 * operator << short: Writes a short to the stream.
 * This operator will write either a textual representation of the inValue, or just
 * X number of bytes to the stream (X is the number of bytes used to hold the type).
 * @param inValue short
 * @return The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(short inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 * operator << char: Writes a char to the stream.
 * This operator will write either a textual representation of the inValue, or just
 * X number of bytes to the stream (X is the number of bytes used to hold the type).
 * @param inValue char
 * @return The reference to this object
 */
//==============================================================================

COutStream &COutStream::operator<<(char inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 *		operator << unsigned char: Writes a char to the stream.
 *					This operator will write either a textual representation of the
 *inValue, or just
 *					X number of bytes to the stream (X is the number of bytes used to
 *hold the type).
 *
 *		@param		inValue			char
 *
 *		@return 	The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(unsigned char inValue)
{
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 * operator << wchar_t: Writes a character to the stream.
 * This operator will write either a textual representation of the inValue, or just
 * X number of bytes to the stream (X is the number of bytes used to hold the type).
 * @param inValue wchar_t
 * @return The reference to this object
 */
//==============================================================================
COutStream &COutStream::operator<<(wchar_t inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}

//==============================================================================
/**
 * operator << unsigned short: Writes a character to the stream.
 * This operator will write either a textual representation of the inValue, or just
 * X number of bytes to the stream (X is the number of bytes used to hold the type).
 * @param inValue wchar_t
 * @return The reference to this object
 */
//==============================================================================

#if _MSC_VER >= 1400 // VS2005
COutStream &COutStream::operator<<(unsigned short inValue)
{
    // in binary mode we just copy the number value byte for byte
    WriteData((unsigned char *)&inValue, sizeof(inValue));
    return *this;
}
#endif

//==============================================================================
/**
 *		operator >> double& : Read a double from the stream.
 *
 *		@param		outValue			double&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(double &outValue)
{
    // read the number of bytes and copy it directly to the outvalue
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 * operator >> float& : Read a float from the stream.
 * @param outValue float&
 * @return The reference to this object
 * @exception An exception it thrown if there is a mismatch between the
 *		the type read and the destination type.  If the _USE_EXCEPTIONS flag
 *		not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(float &outValue)
{
    // read the number of bytes and copy it directly to the outvalue
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 *		operator >> long& : Read a long from the stream.
 *
 *		@param		outValue			long&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(long &outValue)
{
    // determine how many digits necessary to hold the requested number
    // read the number of bytes and copy it directly to the outvalue
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 *		operator >> unsigned long& : Read an unsigned long from the stream.
 *
 *		@param		outValue			unsigned long&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(unsigned long &outValue)
{
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 * operator >> short& : Read a short from the stream.
 * @param outValue short&
 * @return The reference to this object
 * @exception An exception it thrown if there is a mismatch between the
 *		the type read and the destination type.  If the _USE_EXCEPTIONS flag
 *		not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(short &outValue)
{
    // read the number of bytes and copy it directly to the outvalue
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 *		operator >> char& : Read an unsigned char from the stream.
 *
 *		@param		outValue			unsigned char&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(unsigned char &outValue)
{
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 *		operator >> char& : Read a char from the stream.
 *
 *		@param		outValue			char&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(char &outValue)
{
    // get one bytes worth of data
    ReadData((unsigned char *)&outValue, sizeof(outValue));
    return *this;
}

//==============================================================================
/**
 *		operator >> wchar_t& : Read a wchar_t from the stream.
 *
 *		@param		outValue			wchar_t&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================
CInStream &CInStream::operator>>(wchar_t &outValue)
{
    // do a byte for byte read
    ReadData((unsigned char *)&outValue, sizeof(wchar_t));
    return *this;
}

//==============================================================================
/**
 *		operator >> unsigned short& : Read a unsigned short from the stream.
 *
 *		@param		outValue			wchar_t&
 *
 *		@return 	The reference to this object
 *		@exception  An exception it thrown if there is a mismatch between the
 *					the type read and the destination type.  If the _USE_EXCEPTIONS
 *flag
 *					not defined, the outValue will contain the quiet NaN value.
 */
//==============================================================================

#if _MSC_VER >= 1400 // VS2005
CInStream &CInStream::operator>>(unsigned short &outValue)
{
    // do a byte for byte read
    ReadData((unsigned char *)&outValue, sizeof(unsigned short));
    return *this;
}
#endif
