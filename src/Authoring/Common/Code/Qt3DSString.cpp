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
#include "Qt3DSString.h"
#include "Qt3DSMath.h"
#include "Qt3DSMemory.h"
#include <stdarg.h>

#include <QtCore/qstring.h>

namespace Q3DStudio {

#ifdef DEBUG
SSharedHeader s_EmptyStringBuffer[2] = { { QT3DS_MEM_HEADERSIG, 0, sizeof(Qt3DSChar) }, { 0, 0, 0 } };
#else
SSharedHeader s_EmptyStringBuffer[2] = { { 0, sizeof(Qt3DSChar) }, { 0, 0 } };
#endif

Qt3DSChar *s_EmptyString = reinterpret_cast<Qt3DSChar *>(s_EmptyStringBuffer + 1);

IMPLEMENT_OBJECT_COUNTER(CString)

//====================================================================
/**
 * Memory allocator.
 * @param inSize is the requested capacity of the string
 */
void CString::Allocate(long inLength)
{
    m_Data =
        reinterpret_cast<Qt3DSChar *>(CSharedMemory::Allocate((inLength + 1) * sizeof(Qt3DSChar)));

    // NULL terminate string and dirty
    m_Data[inLength] = 0;
    DirtyBuffers();
}

//====================================================================
/**
 * Memory re-allocator.
 * @param inSize is the new requested capacity of the string
 */
void CString::Reallocate(long inLength)
{
    m_Data = reinterpret_cast<Qt3DSChar *>(
        CSharedMemory::Reallocate(m_Data, (inLength + 1) * sizeof(Qt3DSChar)));

    // NULL terminate string and dirty
    m_Data[inLength] = 0;
    DirtyBuffers();
}

//====================================================================
/**
 * Memory releaser that resets the data pointer to the empty string.
 */
void CString::Free()
{
    if (m_Data != s_EmptyString) {
        CSharedMemory::Free(m_Data);
        m_Data = s_EmptyString;
        DirtyBuffers();
    }
}

//====================================================================
/**
 * SyncCharBuffer.
 */
void CString::SyncCharBuffer() const
{
    // NULL buffers means it was never created or was dirtied
    if (m_CharData == NULL) {
        // Create new buffer equally as long but char based
        m_CharData = CreateBuffer<char>();
    }
}

//====================================================================
/**
 * SyncWideBuffer.
 */
void CString::SyncWideBuffer() const
{
    // NULL buffers means it was never created or was dirtied
    if (m_WideData == NULL) {
        // Create new buffer equally as long but char based
        m_WideData = CreateBuffer<wchar_t>();
    }
}

//====================================================================
/**
 * DirtyBuffers.
 */
void CString::DirtyBuffers()
{
    m_CharData = NULL;
    m_WideData = NULL;
#ifdef WIN32
    m_MultiData = NULL;
#endif
}

//====================================================================
/**
  * Construct a composite from two plain char strings.
  * This is equivalent to make inString1 + inString2.
  * @param inString1 is the first part of the string
  * @param inLength1 is the length of the first string
  * @param inString2 is the second part of the string
  * @param inLength2 is the length of the second string
  */
CString::CString(const Qt3DSChar *inString1, long inLength1, const Qt3DSChar *inString2, long inLength2)
    : m_Data(s_EmptyString)
{
    ADDTO_OBJECT_COUNTER(CString)

    if (inLength1 <= 0) {
        Assign(inString2, inLength2);
    } else if (inLength2 <= 0) {
        Assign(inString1, inLength1);
    } else {
        Allocate(inLength1 + inLength2);
        ::memcpy(m_Data, inString1, inLength1 * sizeof(Qt3DSChar));
        ::memcpy(m_Data + inLength1, inString2, inLength2 * sizeof(Qt3DSChar));
    }
    DirtyBuffers();
}

//====================================================================
/**
* Insert an Qt3DSChar string into this string.
* @param inPosition is the offset into this string where the char string should be inserted
* @param inString is the string to be inserted
* @param inLength is the length of the desired string
*/
void CString::StrInsert(long inPosition, const Qt3DSChar *inCharString, long inLength)
{
    if (ENDOFSTRING == inLength)
        inLength = StrLen(inCharString);

    long theCurrentLength = Length();
    if (inLength > 0 && inPosition >= 0 && inPosition <= theCurrentLength) {
        if (0 == theCurrentLength) {
            Assign(inCharString, inLength);
        } else {
            SetLength(theCurrentLength + inLength);

            long t = Length() - inPosition - inLength;
            Qt3DSChar *p = m_Data + inPosition;

            if (t > 0)
                ::memmove(p + inLength, p, t * sizeof(Qt3DSChar));
            ::memmove(p, inCharString, inLength * sizeof(Qt3DSChar));
        }
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Add the given Qt3DSChar string at the end of this string.
 * @param inCharString is the pointer to the string to be added
 * @param inLength is the length of the string to be added
 */
void CString::StrConcat(const Qt3DSChar *inCharString, long inLength)
{
    if (ENDOFSTRING == inLength)
        inLength = StrLen(inCharString);

    if (IsEmpty()) {
        Assign(inCharString, inLength);
    } else if (inLength > 0) {
        long theOldLength = Length();

        // We must check this before calling SetLength(), since
        // the buffer pointer may be changed during reallocation
        if (m_Data == inCharString) {
            SetLength(theOldLength + inLength);
            ::memmove(m_Data + theOldLength, m_Data, inLength * sizeof(Qt3DSChar));
        } else {
            SetLength(theOldLength + inLength);
            ::memmove(m_Data + theOldLength, inCharString, inLength * sizeof(Qt3DSChar));
        }
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Add the given string at the end of this string.
 * @param inString is the string to be added
 */
void CString::Concat(const CString &inString)
{
    if (IsEmpty()) {
        Assign(inString);
    } else if (inString.Length() > 0) {
        StrConcat(inString.m_Data, inString.Length());
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Add the given char at the end of this string.
 * @param inChar is the char to be added
 */
void CString::Concat(char inChar) // char = bad
{
    if (IsEmpty()) {
        Assign(&inChar, 1);
    } else {
        SetLength(Length() + 1);
        m_Data[Length() - 1] = inChar;
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Add the given char at the end of this string.
 * @param inChar is the char to be added
 */
void CString::Concat(Qt3DSChar inChar) // char = bad
{
    if (IsEmpty()) {
        Assign(&inChar, 1);
    } else {
        SetLength(Length() + 1);
        m_Data[Length() - 1] = inChar;
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Make string a copy the given char string and release old buffer
 * if needed.  This looks similar to the Initialize line of methods
 * but Assign takes care of detaching and releasing any previously
 * used buffers.
 * @see Initialize
 * @param inCharString is the pointer to the string
 * @param inLength is the length of the string
 */
void CString::StrAssign(const Qt3DSChar *inCharString, long inLength)
{
    if (ENDOFSTRING == inLength)
        inLength = StrLen(inCharString);

    if (!IsEmpty() && inLength > 0) {
        if (CSharedMemory::RefCountAddress(m_Data)[0] > 1) {
            Qt3DSChar *theOldData = m_Data;
            Allocate(inLength);
            ::memcpy(m_Data, inCharString, inLength * sizeof(Qt3DSChar));
            --(CSharedMemory::RefCountAddress(theOldData)[0]);
        } else {
            Reallocate(inLength);
            ::memmove(m_Data, inCharString, inLength * sizeof(Qt3DSChar));
        }
    } else {
        Clear();
        if (inLength != 0) {
            Allocate(inLength);
            ::memmove(m_Data, inCharString, inLength * sizeof(Qt3DSChar));
        }
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Make string a copy the given string and release old buffer if needed.
 * @param inString is the string to be copied
 */
void CString::Assign(const CString &inString)
{
    if (m_Data != inString.m_Data) {
        Clear();
        if (inString.m_Data != NULL) {
            m_Data = inString.m_Data;
            CSharedMemory::AddRef(m_Data);
            DirtyBuffers();
        }
    }
}

//====================================================================
/**
 * Make string a copy the given char string and release old buffer
 * if needed.  This looks similar to the Initialize line of methods
 * but Assign takes care of detaching and releasing any previously
 * used buffers.
 * @see Initialize
 * @param inCharString is the pointer to the string
 * @param inLength is the length of the string
 */
bool CString::StrCompare(const Qt3DSChar *inCharString, long inLength, bool inCaseSensitive) const
{
    if (ENDOFSTRING == inLength)
        inLength = StrLen(inCharString);

    long theMinLength = MIN(inLength, Length());
    long theIndex = 0;

    // Two differently size strings are not equal no matter what
    if (inLength != Length())
        return false;

    // Consider two zero length strings equal
    if (0 == theMinLength)
        return true;

    if (inCaseSensitive) {
        while (theIndex < theMinLength && inCharString[theIndex] == m_Data[theIndex])
            ++theIndex;
    } else {
        while (theIndex < theMinLength
               && ToLower(inCharString[theIndex]) == ToLower(m_Data[theIndex]))
            ++theIndex;
    }

    // Equal if we scanned the whole string
    return theIndex == theMinLength;
}

//====================================================================
/**
 * Copy constructor
 * @param inString is the length of the string
 */
CString::CString(const CString &inString)
    : m_Data(s_EmptyString)
{
    ADDTO_OBJECT_COUNTER(CString)

    Assign(inString);
}

//====================================================================
/**
 * Copy constructor
 * @param inString is the length of the string
 */
CString::CString(char inChar)
    : m_Data(s_EmptyString)
{
    ADDTO_OBJECT_COUNTER(CString)

    Assign(&inChar, 1);
}

//====================================================================
/**
 * Copy constructor
 * @param inString is the length of the string
 */
CString::CString(Qt3DSChar inChar)
    : m_Data(s_EmptyString)
{
    ADDTO_OBJECT_COUNTER(CString)

    Assign(&inChar, 1);
}

//====================================================================
/**
 * Compare the given char string to the buffer.
 * MF: Needs more work to make consistent comparison of equal substrings.
 */
bool CString::operator<(const CString &inString) const
{
    long theMinLength = MIN(inString.Length(), Length());
    return (::memcmp(m_Data, inString.m_Data, (theMinLength + 1) * sizeof(Qt3DSChar)) < 0);
}

#ifdef CHECK_BOUNDS
//====================================================================
/**
 * Get the char at the indicated location.
 * This will cause the buffer to split which may be an expensive operation.
 * @param inIndex is the location of the desired char
 * @return a char at the indecated index.
 */
Qt3DSChar &CString::operator[](long inIndex)
{
    Unique();
    return m_Data[inIndex];
}
#endif

//====================================================================
/**
 * Detach from the buffer and release it if this string is the
 * last reference.
 */
void CString::Clear()
{
    if (!IsEmpty() && 0 == CSharedMemory::SubRef(m_Data))
        Free();

    m_Data = s_EmptyString;
    DirtyBuffers();
}

//====================================================================
/**
 * Make a unique copy of the buffer and detach from the old buffer.
 * Single-threaded version.
 * @return a pointer to the new buffer
 */
Qt3DSChar *CString::Unique()
{
    if (!IsEmpty() && CSharedMemory::RefCountAddress(m_Data)[0] > 1) {
        Qt3DSChar *theOldData = m_Data;
        Allocate(Length());
        ::memcpy(m_Data, theOldData, Length() * sizeof(Qt3DSChar));
        --(CSharedMemory::RefCountAddress(theOldData)[0]);
    }

    DirtyBuffers();
    return m_Data;
}

//====================================================================
/**
 * Set the length of the sting.  The content is undefined past the
 * length of the old string length if expanding.
 * @param inNewLength is the new length of the string
 */
void CString::SetLength(long inNewLength)
{
    long theCurrentLength = Length();

    if (inNewLength < 0)
        return;

    // If becoming empty
    if (0 == inNewLength)
        Clear();

    // If otherwise was empty before
    else if (0 == theCurrentLength)
        Allocate(inNewLength);

    // If length is not changing, return a unique string
    else if (inNewLength == theCurrentLength)
        Unique();

    else {
        if (CSharedMemory::RefCountAddress(m_Data)[0] > 1) {
            Qt3DSChar *theOldData = m_Data;
            Allocate(inNewLength);
            ::memcpy(m_Data, theOldData, MIN(theCurrentLength, inNewLength) * sizeof(Qt3DSChar));
            --(CSharedMemory::RefCountAddress(theOldData)[0]);
        } else {
            Reallocate(inNewLength);
        }
    }
    DirtyBuffers();
}

//====================================================================
/**
 * Return the length of the string.
 * @return the length of the string
 */
long CString::Length() const
{
    return CSharedMemory::GetSize(m_Data) / sizeof(Qt3DSChar) - 1;
}

//====================================================================
/**
 * Examine how many strings share the buffer of this string.
 * @return how many strings share the buffer of this string
 */
long CString::RefCount() const
{
    return CSharedMemory::RefCountAddress(m_Data)[0];
}

//====================================================================
/**
 * Extract a section of this string.
 * @param inStart is the offset from the beginning of this string
 * @param inLength is the length of the desired string
 * @return a new string
 */
CString CString::Extract(long inStart, long inLength) const
{
    CString theReturn;

    if (Length() > 0 && inStart >= 0 && inStart < Length()) {
        long theMinLength = MIN(inLength, Length() - inStart);

        if (inStart == 0 && theMinLength == Length()) {
            theReturn = *this;
        } else {
            if (theMinLength == ENDOFSTRING)
                theMinLength = Length() - inStart;

            // Added to prevent memory leak - this class sucks
            theReturn.Clear();
            theReturn.SetLength(theMinLength);
            // theReturn.Allocate( theMinLength );
            ::memmove(theReturn.m_Data, m_Data + inStart, theMinLength * sizeof(Qt3DSChar));
            // theReturn.m_Data[ theMinLength ] = 0;
        }
    }
    return theReturn;
    ;
}

CString CString::Mid(int nFirst) const
{
    return Extract(nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
{
    return Extract(nFirst, nCount);
}

CString CString::Left(int nCount) const
{
    return Extract(0, nCount);
}

CString CString::Right(int nCount) const
{
    return Extract(Length() - nCount);
}

//====================================================================
/**
 * Delete a section of this string.
 * @param inStart is the offset from the beginning of this string
 * @param inLength is the number of charachter to be deleted
 */
void CString::Delete(long inStart, long inLength)
{
    long theCurrentLength = Length();
    if (inLength == ENDOFSTRING)
        inLength = theCurrentLength - inStart;

    if (inStart >= 0 && inStart < theCurrentLength && inLength > 0) {
        if (inStart + inLength >= theCurrentLength)
            inLength = theCurrentLength - inStart;

        Unique();
        ::memmove(m_Data + inStart, m_Data + inStart + inLength,
                  (theCurrentLength - inStart - inLength) * sizeof(Qt3DSChar));

        SetLength(theCurrentLength - inLength);
    }
    DirtyBuffers();
}

//====================================================================
/**
 * ToUpper.
 */
CString &CString::ToUpper()
{
    Unique();

    long theLength = Length();
    for (long theScanner = 0; theScanner < theLength; ++theScanner)
        if (m_Data[theScanner] >= 'a' && m_Data[theScanner] <= 'z')
            m_Data[theScanner] = static_cast<Qt3DSChar>(m_Data[theScanner] + 'A' - 'a');
    DirtyBuffers();
    return *this;
}

//====================================================================
/**
 * ToLower.
 */
CString &CString::ToLower()
{
    Unique();

    long theLength = Length();
    for (long theScanner = 0; theScanner < theLength; ++theScanner)
        if (m_Data[theScanner] >= 'A' && m_Data[theScanner] <= 'Z')
            m_Data[theScanner] = static_cast<Qt3DSChar>(m_Data[theScanner] + 'a' - 'A');
    DirtyBuffers();
    return *this;
}

//====================================================================
/**
 * Trims all white-space characters from the right side of the string.
 * White-space characters include newlines, tabs, spaces, and line
 * feeds.
 */
void CString::TrimRight()
{
    const wchar_t *theString = *this;
    long theLength = Length();
    long theIndex = theLength - 1;

    while ((theIndex > 0) && ((theString[theIndex] == '\t') || (theString[theIndex] == '\r')
                              || (theString[theIndex] == '\n') || (theString[theIndex] == ' '))) {
        --theIndex;
    }

    if (theIndex > 0 && theIndex != ENDOFSTRING)
        Delete(theIndex + 1, ENDOFSTRING);
}

//====================================================================
/**
 * Trims all white-space characters from the left side of the string.
 * White-space characters include newlines, tabs, spaces, and line
 * feeds.
 */
void CString::TrimLeft()
{
    const wchar_t *theString = *this;
    long theIndex = 0;
    long theLength = Length();

    while ((theIndex < theLength)
           && ((theString[theIndex] == '\t') || (theString[theIndex] == '\r')
               || (theString[theIndex] == '\n') || (theString[theIndex] == ' '))) {
        ++theIndex;
    }

    if (theIndex > 0 && theIndex != ENDOFSTRING)
        Delete(0, theIndex);
}

//====================================================================
/**
 * Fetches the character located at the specified index into the
 * string.  Note that you must pass in a value greater than zero and
 * less than the string's length or you will get an exception.
 * @param inIndex index of the character to get
 * @return the character located at inIndex
 */
Qt3DSChar CString::GetAt(long inIndex) const
{
    // Index must be positive
    if (inIndex < 0)
        throw;

    // Index must not be longer than the number of chars in the string
    if (inIndex > Length())
        throw;

    const wchar_t *theString = *this;
    return theString[inIndex];
}

//====================================================================
/**
 * Find the first occurrance of the given string.
 * @param inString is string we're looking for
 * @return the offset in glyphs from the beginning of this string or ENDOFSTRING if not found.
 */
long CString::Find(const CString &inString) const
{
    Qt3DSChar *theSource = m_Data;
    long theSubLength = inString.Length() - 1;

    // Finding an empty string is easy
    if (0 != *inString.m_Data) {
        // Spin until the whole string matches
        do {
            // But first spin until first char matches
            Qt3DSChar theSourceChar;
            do {
                theSourceChar = *theSource++;
                if (0 == theSourceChar)
                    return ENDOFSTRING;

            } while (theSourceChar != *inString.m_Data);
        } while (0 != StrNCmp(theSource, inString.m_Data + 1, theSubLength));
        --theSource;
    }

    return static_cast<long>(theSource - m_Data);
}

//====================================================================
/**
 * Find the first occurrance of the given string after the start position.
 * @param inString is string we're looking for
 * @param inStart index to start the search from
 * @return the offset in glyphs from the beginning of this string or ENDOFSTRING if not found.
 */
long CString::Find(const CString &inString, long inStart) const
{
    long theIndex = ENDOFSTRING;

    // If the starting point is valid, extract the substring and just search it
    if (inStart < Length() && inStart >= 0)
        theIndex = Extract(inStart, ENDOFSTRING).Find(inString);

    // If the value was found in the substring, calculate the correct index value
    if (theIndex != ENDOFSTRING)
        theIndex += inStart;

    return theIndex;
}

//====================================================================
/**
 * Find the first occurrance of the given character.
 * @param inChar is char we're looking for
 * @return the offset in glyphs from the beginning of this string or NOTFOUND if not found.
 */
long CString::Find(char inChar) const // char = bad?
{
    long theLength = Length();
    long theIndex;

    for (theIndex = 0; theIndex < theLength && m_Data[theIndex] != inChar; ++theIndex) {
        // Empty
    }

    if (theIndex == theLength)
        theIndex = ENDOFSTRING;

    return theIndex;
}

long CString::find_first_of(const CString &inString, long inStart) const
{
    for (long idx = inStart, len = Length(); idx < len; ++idx) {
        for (long strIdx = 0, strLen = inString.Length(); strIdx < strLen; ++strIdx) {
            Qt3DSChar mine(m_Data[idx]);
            Qt3DSChar theirs(inString.m_Data[strIdx]);
            if (mine == theirs)
                return idx;
        }
    }
    return ENDOFSTRING;
}

//====================================================================
/**
 * Find the first occurrence of the given character after the start position.
 * @param inChar is char we're looking for
 * @param inStart
 * @return the offset in glyphs from the beginning of this string or NOTFOUND if not found.
 */
long CString::Find(char inChar, long inStart) const
{
    long theIndex = ENDOFSTRING;

    // If the starting point is valid, extract the substring and just search it
    if (inStart < Length() && inStart >= 0)
        theIndex = Extract(inStart, ENDOFSTRING).Find(inChar);

    // If the value was found in the substring, calculate the correct index value
    if (theIndex != ENDOFSTRING)
        theIndex += inStart;

    return theIndex;
}

//====================================================================
/**
 * Find the last occurrance of the given string.
 * @param inString is string we're looking for
 * @return the offset in glyphs from the beginning of this string or ENDOFSTRING if not found.
 */
long CString::ReverseFind(const CString &inString) const
{
    // This uses "lesser" calls, but does it recursively... I was hoping this could be a better
    // approach...
    long inLength = inString.Length();
    long currStart = Length() - inLength;

    // Extracts the last inString.Length() of strings...
    CString currStr = Extract(currStart, inLength);

    // If the inString is empty, no match
    // If we have a 0 length string, the function fails. Return ENDOFSTRING
    if (inString.Length() == 0 || Length() == 0)
        return ENDOFSTRING;

    // If we have found a match, we quit the search and return the values
    if (currStr == inString)
        return currStart;

    // chop off the present string and try to find the inString in the chopped version
    currStr = Extract(0, Length() - 1);
    return currStr.ReverseFind(inString);
}

long CString::rfind(wchar_t inChar, long offset) const
{
    long length = offset == ENDOFSTRING ? Length() : MIN(Length(), offset + 1);
    for (long idx = 0; idx < length; ++idx) {
        long curIdx = length - idx - 1;
        if (at(curIdx) == inChar)
            return curIdx;
    }
    return ENDOFSTRING;
}
void CString::reserve(long numChars)
{
    long len = Length();
    if (numChars > len) {
        SetLength(numChars);
        SetLength(len);
    }
}

//====================================================================
/**
 * Find the last occurrance of the given character.
 * @param inChar is char we're looking for
 * @return the offset in glyphs from the beginning of this string or NOTFOUND if not found.
 */
long CString::ReverseFind(char inChar) const
{
    long theIndex;
    for (theIndex = Length() - 1; theIndex >= 0 && m_Data[theIndex] != inChar; --theIndex) {
        // Empty
    }

    if (theIndex < 0)
        theIndex = ENDOFSTRING;

    return theIndex;
}

//=============================================================================
/**
 * Replaces all instances of inString with inReplacement.
 * @return the number of instances that were replaced.
 */
long CString::Replace(const CString &inString, const CString &inReplacement)
{
    long theIndex = Find(inString);
    long theFoundCount = 0;
    while (theIndex != ENDOFSTRING) {
        Delete(theIndex, inString.Length());
        Insert(theIndex, inReplacement);

        ++theFoundCount;
        theIndex += inReplacement.Length();

        theIndex = Find(inString, theIndex);
    }

    return theFoundCount;
}

//====================================================================
/**
 * Sets this string to a string specified via inFormat and parameters
 * in sprintf style
 * DO NOT PASS AN OBJECT INTO THE ...!!  these old sprintf methods work
 * via stack magic and the ... will suck up anything without casting it
 *
 * @param inFormat the format string, e.g. "Qt rocks the %ls"
 * @param ... the parameters to sprint into the format string
 */
void CString::Format(const wchar_t *inFormat, ...)
{
    va_list theArgs;
    va_start(theArgs, inFormat);

    long theBufferSize = 512;

    long theWrittenCount = -1;

    while (-1 == theWrittenCount) {
        // If using existing buffer failed, retry using progressively larger temp buffers
        CAutoPtr<CArrayDeleteHandler<wchar_t>, wchar_t> theBuffer;
        theBuffer = new wchar_t[theBufferSize];

#ifdef WIN32
        theWrittenCount = ::_vsnwprintf(theBuffer, theBufferSize, inFormat, theArgs);
#else
        theWrittenCount = vswprintf(theBuffer, theBufferSize, inFormat, theArgs);
#endif

        if (-1 == theWrittenCount) {
            // Quadruple the buffer size every time we fail
            theBufferSize *= 4;
        } else {
            // Copy the temp buffer to the main string buffer on success
            Assign(static_cast<wchar_t *>(theBuffer), theWrittenCount);
        }
    }
}

//====================================================================
/**
 * Scan.
 * @param inFormat is
 */
long CString::Scan(const char *inFormat, ...) const
{
    va_list theArgs;
    va_start(theArgs, inFormat);

    AssertChar(m_Data); // ok?
    SyncCharBuffer();
    return 0; // Bastard Microsoft doesn't implement an sscanf with va_arg...  will have to copy
              // some code over.
}

//====================================================================
/**
 * Make sure that no characters are unicode.
 */
void CString::AssertChar(const Qt3DSChar *inString)
{
    while (*inString) {
        if (*inString > 255)
            throw;
        ++inString;
    }
}

void AppendString(eastl::string &str, const char *delim, const char *string)
{
    if (string && *string) {
        str.append(delim);
        str.append(string);
    }
}

//====================================================================

CString CString::fromQString(const QString& q)
{
    const int count = q.size();
    wchar_t* tempBuf = reinterpret_cast<wchar_t*>(alloca(count * sizeof(wchar_t)));
    const int actualLength = q.toWCharArray(tempBuf);
    CString cs;
    cs.Assign(tempBuf, actualLength);
    return cs;
}

QString CString::toQString() const
{
    // ensure wide data is in sync
    return QString::fromWCharArray(c_str());
}

QDebug operator<<(QDebug stream, const CString &s)
{
    stream << s.c_str();
    return stream;
}

} // namespace Q3DStudio

