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

#ifndef AMXMEMSINKH
#define AMXMEMSINKH
#include "OutputStream.h"

class CMemOutputStream : public CSeekOutputStream
{
public:
    CMemOutputStream();
    virtual ~CMemOutputStream();
    virtual void Write(const char *inData, unsigned long inLength, unsigned long &outWritten);
    long Write(const void *inBuffer, long inBufferLength) override;
    void Close() override {}
    bool IsValid() const override { return m_Position == m_Length; }
    void Flush() override;
    long Seek(ESeekPosition inPosition, long inOffset) override;
    long GetCurrentPosition() override;
    virtual void SetMem(unsigned char *inMem, unsigned long inLength) = 0;
    virtual void GetMem(unsigned char *&outMem, unsigned long &inLength);
    virtual void Detach(unsigned char *&outMem, unsigned long &outLength) = 0;
    virtual void Enlarge(unsigned long inNewSize) = 0;

protected:
    unsigned char *m_BasePtr;
    unsigned long m_Length;
    unsigned long m_Position;
};

template <class T>
class CFullMemOutputStream : public CMemOutputStream
{
public:
    CFullMemOutputStream(){}
    virtual ~CFullMemOutputStream(){}
    virtual void Write(const char *inData, unsigned long inLength, unsigned long &outWritten)
    {
        CMemOutputStream::Write(inData, inLength, outWritten);
        m_Buffer.SetDirty((long)m_Position);
    }
    virtual long Write(const void *inBuffer, long inBufferLength)
    {
        return CMemOutputStream::Write(inBuffer, inBufferLength);
    }
    virtual void SetMem(unsigned char *inMem, unsigned long inLength)
    {
        m_Buffer.SetMem(inMem, (unsigned long)inLength);
        m_Position = 0;
        ResetBasePtr();
    }
    virtual void Enlarge(unsigned long inNewSize)
    {
        m_Buffer.SetDirty((unsigned long)m_Position);
        m_Buffer.SetSize((unsigned long)inNewSize);
        ResetBasePtr();
    }
    virtual void Detach(unsigned char *&outMem, unsigned long &outLength)
    {
        outLength = m_Position;
        outMem = m_Buffer.Detach();
        m_Position = 0;
        ResetBasePtr();
    }
    virtual void ResetBasePtr()
    {
        m_BasePtr = m_Buffer.GetBuffer();
        m_Length = m_Buffer.GetSize();
    }

    T m_Buffer;
};

#endif
