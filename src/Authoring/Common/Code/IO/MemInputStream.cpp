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
#include "MemInputStream.h"
#include "IOLibraryException.h"

//==========================================================================
/**
 */
CMemInputStream::CMemInputStream()
    : m_BasePtr(NULL)
    , m_Length(0)
    , m_Position(0)
{
}

//==========================================================================
/**
  *
        */
CMemInputStream::~CMemInputStream()
{
}

long CMemInputStream::Read(void *inBuffer, long inBufferLength)
{
    size_t theRead;
    size_t theNeeded = (size_t)inBufferLength;
    Read((char *)inBuffer, theNeeded, theRead);
    return (long)theRead;
}
void CMemInputStream::Close()
{
}

bool CMemInputStream::IsValid()
{
    return true;
}

//==========================================================================
/**
  *
        */
void CMemInputStream::Read(char *ioBuffer, size_t inLength, size_t &outRead)
{
    if (m_Position > m_Length) {
        throw CIOException();
    }

    size_t theResult = m_Position + inLength;
    // Make sure we can't become overdrawn
    if (theResult >= m_Length) {
        // Set length to the amount we will grant
        inLength = inLength - (theResult - m_Length);
    }
    if (inLength != 0) {
        memcpy(ioBuffer, m_BasePtr + m_Position, inLength);
        m_Position += inLength;
    }
    outRead = inLength;
}

//==========================================================================
/**
  *
        */
long CMemInputStream::Seek(Q3DStudio::ISeekable::ESeekPosition inPosition, long inOffset)
{
    switch (inPosition) {
    case Q3DStudio::ISeekable::EBEGIN: {
        m_Position = 0;
        break;
    }
    case Q3DStudio::ISeekable::ECURRENT: {
        break;
    }
    case Q3DStudio::ISeekable::EEND: {
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
        size_t theResult = m_Position + inOffset;
        if (theResult > m_Length) {
            m_Position = m_Length;
        } else {
            m_Position = theResult;
        }
    }
    return 0; // okay because we do implement seek
}

//==========================================================================
/**
 *
 */
long CMemInputStream::GetCurrentPosition()
{
    return (long)m_Position;
}

long CMemInputStream::GetLength()
{
    return (long)m_Length;
}
