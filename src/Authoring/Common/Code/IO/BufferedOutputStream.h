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

#ifndef INCLUDED_BUFFERED_OUTPUT_STREAM_H
#define INCLUDED_BUFFERED_OUTPUT_STREAM_H

#include "OutputStream.h"

//==============================================================================
/**
 *	@class	CBufferedOutputStream
 *	@brief	Brief description goes here.
 *
 *	This is where a complete desciption of the class belongs.
 */
class CBufferedOutputStream : public COutputStream
{
protected:
    COutputStream *m_OutputStream;
    unsigned char *m_Buffer;
    long m_BufferLength;
    long m_BufferUsed;

public:
    CBufferedOutputStream(COutputStream *inStream);
    virtual ~CBufferedOutputStream();

    // IBufferedOutputStream
public:
    long Write(const void *inBuffer, long inBufferLength) override;
    virtual bool Write(std::wostringstream &inStringStream);
    void Flush() override;
    void Close() override;
    bool IsValid() const override;
};

class CBufferedSeekOutputStream : public CSeekOutputStream
{
protected:
    CSeekOutputStream *m_OutputStream;
    unsigned char *m_Buffer;
    const long m_BufferLength;
    long m_BufferUsed;
    long m_BufferPosition;

    unsigned long m_RunningCounter;

public:
    CBufferedSeekOutputStream(CSeekOutputStream *inStream, long inBufferSize = 1048576 /*1 meg*/);
    virtual ~CBufferedSeekOutputStream();

    // IBufferedOutputStream
public:
    long Write(const void *inBuffer, long inBufferLength) override;

    virtual void FlushBuffer();

    void Flush() override;

    void Close() override;

    bool IsValid() const override;

    // CSeekOutputStream
    long Seek(ESeekPosition inPosition, long inOffset) override;
    long GetCurrentPosition() override;
};

#endif //__BUFFEREDOUTPUTSTREAM_H_
