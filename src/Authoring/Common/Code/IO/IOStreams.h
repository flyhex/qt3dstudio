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

#ifndef INCLUDED_IO_STREAMS_H
#define INCLUDED_IO_STREAMS_H 1

#include "UICString.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "IOLibraryException.h"

// The base formatting streams simply write out data to the stream

class CInStream : public CInputStream
{
public:
    CInStream();
    CInStream(CInputStream *inStream);
    CInStream(const CInStream &inInStream);
    virtual ~CInStream();

    virtual void SetInputStream(CInputStream *inStream);
    virtual CInputStream *GetInputStream();

    virtual void SetURL(const wchar_t *inURLString);
    virtual void GetURL(wchar_t *ioURLString, const long &inSize);

    virtual CInStream *Copy(CInputStream *inBase);
    virtual CInStream *Copy();

    virtual CInStream &operator>>(double &outValue);
    virtual CInStream &operator>>(float &outValue);
    virtual CInStream &operator>>(long &outValue);
    virtual CInStream &operator>>(unsigned long &outValue);
    virtual CInStream &operator>>(short &outValue);
    virtual CInStream &operator>>(char &outValue);
    virtual CInStream &operator>>(unsigned char &outValue);
    virtual CInStream &operator>>(wchar_t &outValue);

#if _MSC_VER >= 1400 // VS2005
    virtual CInStream &operator>>(unsigned short &outValue);
#endif

    // CInputStream functions
    long Read(void *outBuffer, long inByteCount) override;
    void Close() override;
    bool IsValid() override;
    Q3DStudio::CString GetMimeType() override { return L""; }
    Q3DStudio::CString GetSource() override { return L""; }

    // Ease of use function
    virtual void ReadData(void *inBuffer, long inBufferLength);

protected:
    CInputStream *m_Stream;
    Q3DStudio::CString m_URLString;
};

class COutStream : public COutputStream
{
public:
    COutStream();
    COutStream(COutputStream *inStream);
    virtual ~COutStream() {}

    virtual void SetOutputStream(COutputStream *inStream);

    virtual void SetURL(const wchar_t *inURLString);
    virtual void GetURL(wchar_t *ioURLString, const long &inSize);

    virtual COutStream *Copy(COutputStream *inBase);
    virtual COutStream *Copy();

    virtual COutStream &operator<<(double inValue);
    virtual COutStream &operator<<(float inValue);
    virtual COutStream &operator<<(long inValue);
    virtual COutStream &operator<<(unsigned long inValue);
    virtual COutStream &operator<<(short inValue);
    virtual COutStream &operator<<(char inValue);
    virtual COutStream &operator<<(unsigned char inValue);
    virtual COutStream &operator<<(wchar_t inValue);

#if _MSC_VER >= 1400 // VS2005
    virtual COutStream &operator<<(unsigned short inValue);
#endif

    // Output Stream functions
    long Write(const void *inBuffer, long inBufferLength) override;
    void Flush() override;
    void Close() override;
    bool IsValid() override;

    // Ease of use function
    virtual void WriteData(const void *inBuffer, long inBufferLength);

protected:
    COutputStream *m_Stream;
    Q3DStudio::CString m_URLString;
};

#endif // _IO_STREAMS_H
