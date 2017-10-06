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

#ifndef AMXMEMSOURCEH
#define AMXMEMSOURCEH
#ifdef WIN32
#pragma warning(disable : 4505)
#endif
#include "InputStream.h"
#include <stddef.h>

class CMemInputStream : public CInputStream
{
public:
    CMemInputStream();
    virtual ~CMemInputStream() = 0;
    long Read(void *inBuffer, long inBufferLength) override;
    void Close() override;
    bool IsValid() override;
    Q3DStudio::CString GetMimeType() override { return L""; }
    Q3DStudio::CString GetSource() override { return L""; }
    //========================================================================
    /**
            *	Returns amount read into buffer, which could be zero.
            *	@param ioBuffer The buffer to read into.
            *	@param inLength The length to attempt to read.
            *	@param outRead the amount actually read.
            */
    virtual void Read(char *ioBuffer, size_t inLength, size_t &outRead);
    //==========================================================================
    /**
            *	Seeks to offset from position.
            *	@param inPosition The position to seek from.
            *	@param inOffset The offset to seek to.
            */
    long Seek(Q3DStudio::ISeekable::ESeekPosition inPosition, long inOffset) override;
    //==========================================================================
    /**
            *	Returns the current offset of the input pointer from the beginning of the
            *	sink.
            */
    long GetCurrentPosition() override;

    virtual long GetLength();

    virtual void SetMem(unsigned char *inMem, size_t inLength) = 0;

    virtual void Detach(unsigned char *&outMem, size_t &outLength) = 0;

protected:
    unsigned char *m_BasePtr;
    size_t m_Length;
    size_t m_Position;
};

template <class T>
class CFullMemInputStream : public CMemInputStream
{
public:
    CFullMemInputStream()
        : CMemInputStream(){}
    virtual ~CFullMemInputStream(){}

    virtual void SetMem(unsigned char *inMem, size_t inLength)
    {
        m_Buffer.SetMem(inMem, (unsigned long)inLength);
        m_Position = 0;
        ResetBasePtr((long)inLength);
    }
    virtual void Detach(unsigned char *&outMem, size_t &outLength)
    {
        outLength = (unsigned long)m_Length;
        outMem = m_Buffer.Detach();
        ResetBasePtr(0);
    }
    virtual void ResetBasePtr(long inLength)
    {
        m_BasePtr = m_Buffer.GetBuffer();
        m_Length = inLength;
    }
    T m_Buffer;
};

#endif
