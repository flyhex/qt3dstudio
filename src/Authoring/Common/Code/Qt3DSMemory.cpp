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

#include "stdafx.h"

//#include <stdlib.h>
#include "Qt3DSMemory.h"
//#include "Qt3DSSynch.h"

namespace Q3DStudio {

long s_DebugMemoryTracker = 0;

//====================================================================
/**
 * General error method that needs more work.
 */
void CMemory::Error()
{
    throw;
}

//====================================================================
/**
 * Allocation wrapper that performs some error checking.
 * @param inSize is the requested amount of memory in bytes
 * @return a pointer to allocated heap memory
 */
void *CMemory::Allocate(long inSize)
{
    if (0 == inSize) {
        return 0;
    } else {
        void *theMemory = ::malloc(inSize);
        if (0 == theMemory) {
            Error();
        }
        return theMemory;
    }
}

//====================================================================
/**
 * Reallocates memory previously allocated using CMemory.
 * @param inMemory is the old allocated memory
 * @param inSize is the new requested size in bytes
 * @return a pointer to reallocated heap memory
 */
void *CMemory::Reallocate(void *inMemory, long inSize)
{
    if (0 == inSize) {
        Free(inMemory);
        return 0;
    } else if (0 == inMemory) {
        return Allocate(inSize);
    } else {
        inMemory = ::realloc(inMemory, inSize);
        if (0 == inMemory) {
            Error();
        }
        return inMemory;
    }
}

//====================================================================
/**
 * Frees memory previously allocated using CMemory.
 * @param inMemory is the old allocated memory
 */
void CMemory::Free(void *inMemory)
{
    if (0 != inMemory)
        ::free(inMemory);
}

//====================================================================
/**
 * Prevents framentation and optimizes memory allocation.
 * Method rounds up the value given to optimize memory usage.
 * These are the ranges and return values: 0-15 = 16, 16-31 = 32
 * 32 - 4095 = next 64 factor, 4096+ = next 4096 factor.
 * @param inSpaceNeeded is the requested amount of memory in bytes
 * @return optimal memory allocation amount in bytes
 */
long CSharedMemory::Quantize(long inSpaceNeeded)
{
    static long theBigQuant = 4096;
    static long theBigMask = ~4095;
    static long theSmallQuant = 64;
    static long theSmallMask = ~63;

    if (inSpaceNeeded < 16)
        return 16;
    if (inSpaceNeeded < 32)
        return 32;
    if (inSpaceNeeded < 2048)
        return (inSpaceNeeded + theSmallQuant) & theSmallMask;
    else
        return (inSpaceNeeded + theBigQuant) & theBigMask;
}

//====================================================================
/**
 * Allocate memory with reference count and length.
 * CSharedMemory allocates room for a header that allows tracking of
 * reference count and size of the buffer.  Memory allocated using
 * this method MUST be deallocated using the corresponding Free call.
 * The void pointer returned can be used directly as if returned
 * by CMemory::Allocate or new char[inSize].
 * @param inSize is the requested amount of memory in bytes
 * @return a pointer to allocated heap memory
 */
void *CSharedMemory::Allocate(long inSize)
{
    if (inSize <= 0)
        CMemory::Error();

    long theQuantizedSize = Quantize(inSize);
    SSharedHeader *theSharedMemory =
        static_cast<SSharedHeader *>(CMemory::Allocate(theQuantizedSize + sizeof(SSharedHeader)));

    theSharedMemory->m_Size = inSize;
    theSharedMemory->m_RefCount = 1;
#ifdef DEBUG
    theSharedMemory->m_Signature = QT3DS_MEM_HEADERSIG;
#endif

    // NULL terminate manually.  Quantization ensures there is space for it.
    reinterpret_cast<char *>(theSharedMemory + 1)[inSize] = 0;

#ifdef DEBUG
    s_DebugMemoryTracker += theQuantizedSize;
#endif

    return reinterpret_cast<void *>(theSharedMemory + 1);
}

//====================================================================
/**
 * Reallocates memory previously allocated using CSharedMemory.
 * CMemory and CSharedMemory pointers or NOT interchangable and
 * great care must be taken to call the correct Reallocate method.
 * @param inMemory is the old allocated memory
 * @param inSize is the new requested size in bytes
 * @return a pointer to reallocated heap memory
 */
void *CSharedMemory::Reallocate(void *inMemory, long inSize)
{
    SSharedHeader *theSharedMemory = Header(inMemory);

    if (inSize <= 0)
        CMemory::Error();

    long theNewQuantizedSize = Quantize(inSize);
    long theOldQuantizedSize = Quantize(theSharedMemory->m_Size);
    if (theNewQuantizedSize != theOldQuantizedSize) {
        theSharedMemory = reinterpret_cast<SSharedHeader *>(
            CMemory::Reallocate(theSharedMemory, theNewQuantizedSize + sizeof(SSharedHeader)));
    }
    theSharedMemory->m_Size = inSize;

    // NULL terminate manually.  Quantization ensures there is space for it.
    reinterpret_cast<char *>(theSharedMemory + 1)[inSize] = 0;

#ifdef DEBUG
    s_DebugMemoryTracker += theNewQuantizedSize - theOldQuantizedSize;
#endif

    return reinterpret_cast<void *>(theSharedMemory + 1);
}

//====================================================================
/**
 * Frees memory previously allocated using CSharedMemory.
 * @param inMemory is the old allocated memory
 */
void CSharedMemory::Free(void *inMemory)
{
    SSharedHeader *theSharedMemory = Header(inMemory);

#ifdef DEBUG
    s_DebugMemoryTracker -= Quantize(theSharedMemory->m_Size);
#endif

    CMemory::Free(theSharedMemory);
}

long CSharedMemory::AddRef(void *inMemory)
{
    return ++(*(RefCountAddress(inMemory)));
} //						{ return pincrement( RefCountAddress( inMemory ) );
  //}
long CSharedMemory::SubRef(void *inMemory)
{
    return --(*(RefCountAddress(inMemory)));
} //						{ return pdecrement( RefCountAddress( inMemory ) );
  //}

} // namespace Q3DStudio
