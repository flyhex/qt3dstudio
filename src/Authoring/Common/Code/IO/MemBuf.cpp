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
#include "stdafx.h"

#include <string.h>
#include "MemBuf.h"
#include "IOLibraryException.h"

using namespace Q3DStudio;

/**
   @brief Sets member variables to the uninitialized state.
*/
CMemBuf::CMemBuf()
{
    m_Buffer = NULL;
    m_Dirty = 0;
    m_BufSize = 0;
}

/**
   @brief Does nothing.
*/
CMemBuf::~CMemBuf()
{
}

/**
   @brief Return the current buffer size.
   @return The current buffer size.
*/
long CMemBuf::GetSize()
{
    return m_BufSize;
}
/**
   @brief returns the memory buffer without releasing it from the object.
   @return The actual memory buffer.
*/
unsigned char *CMemBuf::GetBuffer()
{
    return m_Buffer;
}
/**
   @brief returns the memory buffer, releasing it from the object.
*/
unsigned char *CMemBuf::Detach()
{
    unsigned char *theTemp = m_Buffer;
    m_Buffer = NULL;
    m_Dirty = 0;
    m_BufSize = 0;
    return theTemp;
}

/**
   @brief Set the amount of dirty memory in the buffer.
   The buffer will copy over this amount of memory to a new buffer
   on resize, so setting this is only important if you care for
   data persistence.
   @param dirty The amount of dirty memory.
*/
void CMemBuf::SetDirty(long theDirty)
{
    m_Dirty = theDirty;
}

/**
   @brief Return the amount of dirty memory in buffer.
   @return The amount of data that is dirty.
*/
long CMemBuf::GetDirty()
{
    return m_Dirty;
}

/**
   @brief Does nothing.
*/
CDynamicMemBuf::CDynamicMemBuf()
    : m_DisableMinFlag(false)
{
}

/**
   @brief Deletes buffer if allocated.
*/
CDynamicMemBuf::~CDynamicMemBuf()
{
    if (m_Buffer != NULL) {
        delete[] m_Buffer;
    }
}

/**
   @brief Sets the flag to prevent SetSize from allocating a MIN buffer size
*/
void CDynamicMemBuf::DisableMinimumBuffer(bool inFlag)
{
    m_DisableMinFlag = inFlag;
}

/**
   @brief Set the size of the buffer, causing resize if larger.
*/
void CDynamicMemBuf::SetSize(long theSize)
{
    if (theSize > m_BufSize) {
        unsigned char *theTemp;

        // adjust size to allocate accordingly, keeping to at least MIN_SIZE (to prevent excessive
        // fragmentation)
        if (theSize < MIN_SIZE && !m_DisableMinFlag)
            theSize = MIN_SIZE;
        else if (theSize > MIN_SIZE)
            theSize += MIN_SIZE - (theSize % MIN_SIZE);

        // chia: exception would be thrown here if new fails
        try {
            theTemp = new unsigned char[theSize];
        } catch (...) {
            throw CIOException();
        }

        if (m_Buffer != NULL) {
            if (m_Dirty != 0) {
                // copy old to new
                memcpy(theTemp, m_Buffer, m_Dirty);
            }
            delete[] m_Buffer;
        }

        m_Buffer = theTemp;

        if (m_Buffer == NULL) {
            throw CIOException();
        }

        m_BufSize = theSize;
        m_Dirty = 0;
    }
}

/**
   @brief Set the memory of the object, deleting old memory
   and not copying over old memory no matter if dirty or not.
*/
void CDynamicMemBuf::SetMem(unsigned char *theMem, long theSize)
{
    if (m_Buffer != NULL) {
        delete[] m_Buffer;
    }
    m_BufSize = theSize;
    m_Buffer = theMem;
    m_Dirty = 0;
}

/**
   @brief Does nothing.
*/
CStaticMemBuf::CStaticMemBuf()
{
}

/**
   @brief Does nothing.
*/
CStaticMemBuf::~CStaticMemBuf()
{
}

/**
   @brief Sets the memory buffers, does not delete old buffer.
   @param mem The memory to set.
   @param size The size of that memory.
*/
void CStaticMemBuf::SetMem(unsigned char *theMem, long theSize)
{
    m_BufSize = theSize;
    m_Buffer = theMem;
    m_Dirty = 0;
}

/**
   @brief Set the size of the buffer.
   @param size The size of the buffer.
*/
void CStaticMemBuf::SetSize(long)
{
    throw CIOException();
}
