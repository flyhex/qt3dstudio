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

#include <string.h>

#include "MemOutputStream.h"
#include "IOLibraryException.h"

//==========================================================================
/**
 *
 */
CMemOutputStream::CMemOutputStream()
    : m_BasePtr(NULL)
    , m_Length(0)
    , m_Position(0)
{
}

//==========================================================================
/**
  *
        */
CMemOutputStream::~CMemOutputStream()
{
}

//==========================================================================
/**
  *
        */
void CMemOutputStream::Write(const char *inData, unsigned long inLength, unsigned long &outWritten)
{
    unsigned long theResult = inLength + m_Position;
    if (theResult > m_Length) {
        // lets not pre-allocate such a large size
        Enlarge(static_cast<unsigned long>(1.5 * theResult));
    }
    memcpy(m_BasePtr + m_Position, inData, inLength);

    m_Position += inLength;

    outWritten = inLength;
}

long CMemOutputStream::Write(const void *inBuffer, long inBufferLength)
{
    unsigned long theRet;
    Write((char *)inBuffer, inBufferLength, theRet);
    return (long)theRet;
}

//==========================================================================
/**
  *
        */
void CMemOutputStream::Flush()
{
}

//==========================================================================
/**
  *
        */
long CMemOutputStream::Seek(ISeekable::ESeekPosition inPosition, long inOffset)
{
    switch (inPosition) {
    case ISeekable::EBEGIN: {
        m_Position = 0;
        break;
    }
    case ISeekable::ECURRENT: {
        break;
    }
    case ISeekable::EEND: {
        m_Position = m_Length;
        break;
    }
    default: {
        throw CIOException();
        break;
    }
    }
    if (inOffset < 0) {
        inOffset = -inOffset;
        if (inOffset > (long)m_Position) {
            m_Position = 0;
        } else {
            m_Position = m_Position - inOffset;
        }
    } else {
        unsigned long theResult = m_Position + inOffset;
        if (theResult > m_Length) {
            m_Position = m_Length;
        } else {
            m_Position = theResult;
        }
    }
    return 0; // okay because we implement seeking
}

//==========================================================================
/**
 *
 */
long CMemOutputStream::GetCurrentPosition()
{
    return (long)m_Position;
}

void CMemOutputStream::GetMem(unsigned char *&outMem, unsigned long &outLength)
{
    outMem = m_BasePtr;
    outLength = m_Position;
}
