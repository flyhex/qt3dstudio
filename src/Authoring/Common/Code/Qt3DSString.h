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

#pragma once
#ifndef __QT3DS_STRING_H__
#define __QT3DS_STRING_H__

#ifdef _WIN32
#pragma warning(push)
#endif

#include "AutoPtr.h"
#include "Qt3DSMacros.h"
#include "Qt3DSObjectCounter.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <EASTL/string.h>

#include <QString>
#include <QDebug>

namespace Q3DStudio {
#define _LSTR(string) L##string

#if !defined(_MSC_VER) || _MSC_VER >= 1400 // VS2005 = 1400
typedef wchar_t Qt3DSChar;
#else
typedef unsigned short Qt3DSChar;
#endif

extern Qt3DSChar *s_EmptyString;

template <class T>
long StrLen(const T *inChars);

template <class T, class U>
void StrToStr(const T *inChars, long inLength, U *outChars);

// Stack based conversion from any array to Qt3DSChar*.  Treat like a cast operator.
// Qt3DSChar* memory valid for the duration of scope.
class Qt3DSStr
{
    Qt3DSChar *m_Buffer;
    enum {
        ENDOFSTRING = -2, /// End of String - used in extract and insert. Also returned as position
                          /// when the wanted string was not found
    };

public:
    operator Qt3DSChar *() { return m_Buffer; }
    ~Qt3DSStr() { delete[] m_Buffer; }
    template <class T>
    Qt3DSStr(const T *inString, long inLength = ENDOFSTRING)
        : m_Buffer(NULL)
    {
        if (ENDOFSTRING == inLength)
            inLength = StrLen(inString);

        m_Buffer = new Qt3DSChar[inLength + 1];
        m_Buffer[inLength] = 0;
        StrToStr(inString, inLength, m_Buffer);
    }
};

// The venerable string class
class CString
{
public:
    long Length() const;
    long size() const { return Length(); }

    enum {
        ENDOFSTRING = -2, /// End of String - used in extract and insert. Also returned as position
                          /// when the wanted string was not found
        npos = ENDOFSTRING, /// To make converting algorithms using std::wstring to CString easier.
    };

protected:
    // Fields
    Qt3DSChar *m_Data;
    mutable CAutoArrayPtr<wchar_t> m_WideData; ///< Mutable to allow const buffer access operators
    mutable CAutoArrayPtr<char> m_CharData;
#ifdef WIN32
    /// Multibyte used on Windows only
    mutable CAutoArrayPtr<char> m_MultiData;
#endif

    // Memory management
    void Allocate(long inLength);
    void Reallocate(long inLength);
    void Free();

    // Char buffer synchronization
    void SyncCharBuffer() const;
    void SyncWideBuffer() const;
    void DirtyBuffers();

    // Implementation of operators and templates
    void StrInsert(long inPosition, const Qt3DSChar *inChars, long inLength = ENDOFSTRING);
    void StrConcat(const Qt3DSChar *inChars, long inLength = ENDOFSTRING);
    void StrAssign(const Qt3DSChar *inChars, long inLength = ENDOFSTRING);
    bool StrCompare(const Qt3DSChar *inChars, long inLength = ENDOFSTRING,
                    bool inCaseSensitive = true) const;

public:
    typedef const wchar_t *const_iterator;
    typedef long size_type;
    const_iterator begin() const
    {
        if (Length())
            return c_str();
        return NULL;
    }
    const_iterator end() const { return begin() + Length(); }

    // Tools
    template <class T>
    void Insert(long inPosition, const T *inChars, long inLength = ENDOFSTRING)
    {
        StrInsert(inPosition, Qt3DSStr(inChars, inLength), inLength);
    }
    void Insert(long inPosition, const CString &inString)
    {
        StrInsert(inPosition, inString.m_Data, inString.Length());
    }

    template <class T>
    void Concat(const T *inChars, long inLength = ENDOFSTRING)
    {
        StrConcat(Qt3DSStr(inChars, inLength), inLength);
    }
    void Concat(const CString &inString);
    void Concat(char inChar);
    void Concat(Qt3DSChar inChar);
    void append(const CString &inString) { Concat(inString); }
    void append(char inChar) { Concat(inChar); }
    void append(Qt3DSChar inChar) { Concat(inChar); }
    void append(const Qt3DSChar *inChars, long inLength) { Concat<Qt3DSChar>(inChars, inLength); }
    void append(const CString &inString, long inOffset, long inLength)
    {
        append(inString.Extract(inOffset, inLength));
    }

    template <class T>
    void Assign(const T *inChars, long inLength = ENDOFSTRING);
    void assign(const wchar_t *inChars, long inLength = ENDOFSTRING) { Assign(inChars, inLength); }
    void Assign(const CString &inString);
    void assign(const CString &inString) { Assign(inString); }

    template <class T>
    bool Compare(const T *inChars, long inLength = ENDOFSTRING, bool inCaseSensitive = true) const
    {
        return StrCompare(Qt3DSStr(inChars, inLength), inLength, inCaseSensitive);
    }
    bool Compare(const CString &inString, bool inCaseSensitive = true) const
    {
        return StrCompare(inString.m_Data, inString.Length(), inCaseSensitive);
    }

private:
    template <class T>
    T *CreateBuffer() const;

public:
    DEFINE_OBJECT_COUNTER(CString)

    // Construction
    CString(char inChar);
    CString(const CString &inString);
    CString(Qt3DSChar inChar);
    CString(void)
        : m_Data(s_EmptyString)
    {
        ADDTO_OBJECT_COUNTER(CString);
    }
    template <class T>
    CString(const T *inChars, long inLength = ENDOFSTRING)
        : m_Data(s_EmptyString)
    {
        ADDTO_OBJECT_COUNTER(CString);
        Assign(inChars, inLength);
    }

    ~CString()
    {
        REMOVEFROM_OBJECT_COUNTER(CString);
        Clear();
    }

protected:
    CString(const Qt3DSChar *inString1, long inLength1, const Qt3DSChar *inString2, long inLength2);

public:
    // Operators
    CString &operator=(const CString &inString)
    {
        Assign(inString);
        return *this;
    }
    CString &operator=(char inChar)
    {
        Assign(inChar);
        return *this;
    }
    CString &operator=(const Qt3DSChar inChar)
    {
        Assign(inChar);
        return *this;
    }

    CString &operator+=(const CString &inString)
    {
        Concat(inString);
        return *this;
    }
    CString &operator+=(char inChar)
    {
        Concat(inChar);
        return *this;
    }

    CString operator+(const CString &inString) const
    {
        return CString(m_Data, Length(), inString.m_Data, inString.Length());
    }
    CString operator+(char inChar) const
    {
        Qt3DSChar theChar = inChar;
        return CString(m_Data, Length(), &theChar, 1);
    }
    template <class T>
    CString operator+(const T *inChars) const
    {
        return CString(m_Data, Length(), Qt3DSStr(inChars), StrLen(inChars));
    }

    bool operator==(const CString &inString) const { return Compare(inString); }
    bool operator==(char inChar) const { return (1 == Length()) && (m_Data[0] == inChar); }
    template <class T>
    bool operator==(const T *inChars) const
    {
        return StrCompare(Qt3DSStr(inChars));
    }

    bool operator!=(const CString &inString) const { return !(*this == inString); }
    bool operator!=(char inChar) const { return !(*this == inChar); }
    template <class T>
    bool operator!=(const T *inChars) const
    {
        return !(*this == inChars);
    }

    bool operator<(const CString &inString) const;

    // this also covers wchar_t on 2 byte wchar_t systems (win)
    operator const Qt3DSChar *() const { return m_Data; }
//			operator long( ) const;

#ifdef CHECK_BOUNDS
    Qt3DSChar &operator[](long inIndex);
#else
    inline Qt3DSChar &operator[](long inIndex) { return Unique()[inIndex]; }
#endif
    inline Qt3DSChar &operator[](int inIndex) { return this->operator[]((long)inIndex); }

    /// IMPORTANT NOTE! the char* cast operator is intentionally unimplemented.
    /// this has been done so that the caller must explicitly call GetCharStar( ) (which flattens
    /// unicode)
    /// or GetMulti( ), which returns a multibyte character buffer windows can understand.  using
    /// wchar_ts
    /// is preferred when possible to either of these options.  questions? Ask Andy Skalet

    // operator const char*( )	const

    // replaces char* cast operator, which can be implicit (bad)
    // should only be used if you know for sure 1 byte per character is ok for your need
    char *GetCharStar() const
    {
        SyncCharBuffer();
        return m_CharData;
    }

public:
    // Utililty
    void Clear();
    Qt3DSChar *Unique();
    void SetLength(long inLength);
    long RefCount() const;
    bool IsEmpty() const { return 0 == Length(); }
    bool CompareNoCase(const CString &inString) const { return Compare(inString, false); }

    // Explicit manipulation
    CString Extract(long inStart, long inLength = ENDOFSTRING) const;
    void Delete(long inStart, long inLength = ENDOFSTRING);
    CString &ToUpper();
    CString &ToLower();
    void TrimRight();
    void TrimLeft();
    CString Mid(int nFirst) const;
    CString Mid(int nFirst, int nCount) const;
    CString Left(int nCount) const;
    CString Right(int nCount) const;

    // Searching
    Qt3DSChar GetAt(long inIndex) const;
    Qt3DSChar at(long inIndex) const { return GetAt(inIndex); }
    long Find(const CString &inString) const;
    long Find(const CString &inString, long inStart) const;
    long Find(char inChar) const;
    long Find(char inChar, long inStart) const;
    long find(const CString &inString) const { return Find(inString); }
    long find(const CString &inString, long inStart) const { return Find(inString, inStart); }
    long find(char inChar) const { return Find(inChar); }
    long find(char inChar, long inStart) const { return Find(inChar, inStart); }
    long find_first_of(const CString &inString, long inStart = 0) const;

    long ReverseFind(const CString &inString) const;
    long ReverseFind(char inChar) const;
    long rfind(const CString &str) const { return ReverseFind(str); }
    long rfind(char inChar) const { return ReverseFind(inChar); }
    long rfind(wchar_t inChar, long offset = ENDOFSTRING) const;
    CString erase(long offset, long length) const
    {
        CString retval(*this);
        retval.Delete(offset, length);
        return retval;
    }
    CString substr(long offset, long length = ENDOFSTRING) const { return Extract(offset, length); }
    void reserve(long numChars);

    long Replace(const CString &inString, const CString &inReplacement);

    const Qt3DSChar *c_str() const { return (const wchar_t *)*this; }

    // Format and Scan
    void Format(const wchar_t *inFormat, ...);
    long Scan(const char *inFormat, ...) const;

    // Static utilities
    static void AssertChar(const Qt3DSChar *inString); ///< Throw if any Qt3DSChar is greater than 255
    static Qt3DSChar ToLower(Qt3DSChar inChar)
    {
        return inChar >= 'A' && inChar <= 'Z' ? static_cast<Qt3DSChar>(inChar + 'a' - 'A') : inChar;
    }

    /// Everything below this line (within the same class) is handlers for os or library specific
    /// types
    /// If we want to run on Windows 9X, we need to use multibyte instead of unicode in system
    /// calls.
    /// These methods deal with multibyte

    static CString fromQString(const QString& q);
    QString toQString() const;

public:
#ifdef __AFX_H__
    /// Methods and operators for MFC CStrings
public:
    CString(::CString inString)
        : m_Data(s_EmptyString)
    {
        ADDTO_OBJECT_COUNTER(CString);
        Assign(inString.GetBuffer(0));
        inString.ReleaseBuffer();
    }

    CString &operator=(::CString inString)
    {
        Assign(inString.GetBuffer(0));
        inString.ReleaseBuffer();
        return *this;
    }

    operator const ::CString() const
    {
        return ::CString(this->c_str());
    } // compile error without "this->"
#endif
};

/////// HELPER TEMPLATES ////////

/// Get the length of any zero terminated array
template <class T>
long StrLen(const T *inChars)
{
    const T *theScanner = inChars;

    if (theScanner)
        while (*theScanner)
            ++theScanner;

    return static_cast<long>(theScanner - inChars);
}

// Convert from and to any array, statically converting every element
template <class T, class U>
void StrToStr(const T *inChars, long inLength, U *outChars)
{
    for (long theChar = 0; theChar < inLength; ++theChar)
        outChars[theChar] = static_cast<U>(inChars[theChar]);
    outChars[inLength] = 0;
}

// Compare memory pointed to by two null terminated pointers, up to inCount elements
template <class T>
long StrNCmp(const T *inChars1, const T *inChars2, long inCount)
{
    if (inCount <= 0)
        return 0;

    while (--inCount && *inChars1 && *inChars1 == *inChars2) {
        ++inChars1;
        ++inChars2;
    }

    return *inChars1 - *inChars2;
}

/////// CString TEMPLATE CODE ////////

// Generic addition starting with any array type
template <class T>
CString operator+(const T *inLHS, const CString inRHS)
{
    return CString(inLHS) + inRHS;
}

// Generic assignment of any array
template <class T>
void CString::Assign(const T *inChars, long inLength)
{
    if (ENDOFSTRING == inLength)
        inLength = StrLen(inChars);

    if (reinterpret_cast<const Qt3DSChar *>(inChars) != m_Data) {
        SetLength(inLength);
        StrToStr(inChars, inLength, m_Data);
        m_Data[inLength] = 0;
    }

    DirtyBuffers();
}

// Generic creation of any array type trying to represent the buffer.
// Caller assumes responibility for deleting array using delete [] operator.
template <class T>
T *CString::CreateBuffer() const
{
    long theLength = Length();
    T *outBuffer = new T[theLength + 1];

    outBuffer[theLength] = 0;
    StrToStr(m_Data, theLength, outBuffer);

    return outBuffer;
}

};

QDebug operator<<(QDebug stream, const Q3DStudio::CString &s);

void AppendString(eastl::string &str, const char *delim, const char *string);

#ifdef _WIN32
#pragma warning(pop)
#endif
#endif // __QT3DS_STRING_H__
