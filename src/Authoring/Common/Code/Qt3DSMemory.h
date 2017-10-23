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
#ifndef __QT3DS_MEMORY_H__
#define __QT3DS_MEMORY_H__

#include "Qt3DSMacros.h"

namespace Q3DStudio {
#define QT3DS_MEM_HEADERSIG 0xF00D

//==============================================================================
/**
 *	@class CMemory
 *	@brief Small wrapper around memory allocation.
 *	@see CSharedMemory
 *	@see SSharedHeader
 */
class CMemory
{
public:
    static void *Allocate(long inSize);
    static void *Reallocate(void *inMemory, long inSize);
    static void Free(void *inMemory);
    static void Error();
};

//==============================================================================
/**
 *	@struct SSharedHeader
 *	@brief Block of code at the beginning of memory enabling refcounting.
 *	@see CMemory
 *	@see CSharedMemory
 */
struct SSharedHeader
{
#ifdef DEBUG
    long m_Signature;
#endif
    long m_RefCount;
    long m_Size;
};

//==============================================================================
/**
 *	@class CSharedMemory
 *	@brief Lowlevel memory class that hides extra header info enabling refcounting.
 *	@see CMemory
 *	@see SSharedHeader
 */
class CSharedMemory
{
protected:
#ifdef DEBUG
    static void Validate(void *inMemory)
    {
        if ((reinterpret_cast<SSharedHeader *>(inMemory) - 1)->m_Size < 0
            || QT3DS_MEM_HEADERSIG != (reinterpret_cast<SSharedHeader *>(inMemory) - 1)->m_Signature)
            CMemory::Error();
    }
    static SSharedHeader *Header(void *inMemory)
    {
        Validate(inMemory);
        return reinterpret_cast<SSharedHeader *>(inMemory) - 1;
    }
#else
    // inline static void Validate( void* /*inMemory*/ )			{}
    inline static SSharedHeader *Header(void *inMemory)
    {
        return reinterpret_cast<SSharedHeader *>(inMemory) - 1;
    }
#endif

public:
    inline static long *RefCountAddress(void *inMemory) { return &(Header(inMemory)->m_RefCount); }
    inline static long GetSize(void *inMemory) { return Header(inMemory)->m_Size; }

    static long AddRef(void *inMemory);
    static long SubRef(void *inMemory);

    static long Quantize(long inSpaceNeeded);

    static void *Allocate(long inSize);
    static void *Reallocate(void *inMemory, long inSize);
    static void Free(void *inMemory);
};
} // namespace Q3DStudio
#endif // __QT3DS_MEMORY_H__
