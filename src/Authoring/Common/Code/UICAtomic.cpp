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

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#include "UICSynch.h"

#if defined(__GNUC__) && (defined(__i386__) || defined(__I386__))
#define GCC_i386
#elif defined(_MSC_VER) && (defined(_M_IX86) && !defined(_AMD64_))
#define MSC_i386
#elif defined(_MSC_VER) && defined(_AMD64_)
#define MSC_i686
#elif defined(__BORLANDC__) && defined(_M_IX86)
#define BCC_i386
#endif

#if defined(MSC_i386) || defined(BCC_i386)

//
// atomic operations for Microsoft C or Borland C on Windows
//

#if defined(_MSC_VER)
#pragma warning(disable : 4035)
#elif defined(__BORLANDC__)
#pragma warn - rvl
#endif

// !!! NOTE
// the following functions implement atomic exchange/inc/dec on
// windows. they are dangerous in that they rely on the calling
// conventions of MSVC and BCC. the first one passes the first
// two arguments in ECX and EDX, and the second one - in EAX and
// EDX.

long __PFASTCALL pincrement(long *)
{
    __asm
    {
#ifdef BCC_i386
.486
        mov         ecx,eax
#endif
        mov         eax,1;
        lock xadd   [ecx],eax;
        inc         eax
    }
}

long __PFASTCALL pdecrement(long *)
{
    __asm
    {
#ifdef BCC_i386
.486
        mov         ecx,eax
#endif
        mov         eax,-1;
        lock xadd   [ecx],eax;
        dec         eax
    }
}

long __PFASTCALL pexchange(long *, long)
{
    __asm
    {
#ifdef BCC_i386
.486
        xchg        eax,edx;
        lock xchg   eax,[edx];
#else
        mov         eax,edx;
        lock xchg   eax,[ecx];
#endif
    }
}

void *__PFASTCALL pexchange(void **, void *)
{
    __asm
    {
#ifdef BCC_i386
.486
        xchg        eax,edx;
        lock xchg   eax,[edx];
#else
        mov         eax,edx;
        lock xchg   eax,[ecx];
#endif
    }
}

#elif defined(GCC_i386)

//
// GNU C compiler on any i386 platform (actually 486+ for xadd)
//

long pexchange(long *target, long value)
{
    __asm__ __volatile("lock ; xchgl (%1),%0" : "+r"(value) : "r"(target));
    return value;
}

void *pexchange(void **target, void *value)
{
    __asm__ __volatile("lock ; xchgl (%1),%0" : "+r"(value) : "r"(target));
    return value;
}

long pincrement(long *target)
{
    int temp = 1;
    __asm__ __volatile("lock ; xaddl %0,(%1)" : "+r"(temp) : "r"(target));
    return temp + 1;
}

long pdecrement(long *target)
{
    long temp = -1;
    __asm__ __volatile("lock ; xaddl %0,(%1)" : "+r"(temp) : "r"(target));
    return temp - 1;
}

#elif defined(MSC_i686)

long __PFASTCALL pincrement(long *target)
{
    mutex mtx;
    mtx.enter();
    long r = ++(*target);
    mtx.leave();
    return r;
}

long __PFASTCALL pdecrement(long *target)
{
    mutex mtx;
    mtx.enter();
    long r = --(*target);
    mtx.leave();
    return r;
}

long pexchange(long *target, long value)
{
    mutex mtx;
    mtx.enter();
    long r = *target;
    *target = value;
    mtx.leave();
    return r;
}

void *pexchange(void **target, void *value)
{
    mutex mtx;
    mtx.enter();
    void *r = *target;
    *target = value;
    mtx.leave();
    return r;
}

#else

//
// other platforms: mutex locking
//

// we use a prime number for hashing
const long MUTEX_HASH_SIZE = 17;

#define MTX_INIT PTHREAD_MUTEX_INITIALIZER
#define mtx_t pthread_mutex_t
#define mtx_lock pthread_mutex_lock
#define mtx_unlock pthread_mutex_unlock

static mtx_t mutextbl[MUTEX_HASH_SIZE] = { MTX_INIT, MTX_INIT, MTX_INIT, MTX_INIT, MTX_INIT,
                                           MTX_INIT, MTX_INIT, MTX_INIT, MTX_INIT, MTX_INIT,
                                           MTX_INIT, MTX_INIT, MTX_INIT, MTX_INIT, MTX_INIT,
                                           MTX_INIT, MTX_INIT };

inline mtx_t *pgetmemlock(void *addr)
{
    return mutextbl + quint64(addr) % MUTEX_HASH_SIZE;
}

long pexchange(long *target, long value)
{
    mtx_t *m = pgetmemlock(target);
    mtx_lock(m);
    long r = *target;
    *target = value;
    mtx_unlock(m);
    return r;
}

void *pexchange(void **target, void *value)
{
    mtx_t *m = pgetmemlock(target);
    mtx_lock(m);
    void *r = *target;
    *target = value;
    mtx_unlock(m);
    return r;
}

long pincrement(long *target)
{
    mtx_t *m = pgetmemlock(target);
    mtx_lock(m);
    long r = ++(*target);
    mtx_unlock(m);
    return r;
}

long pdecrement(long *target)
{
    mtx_t *m = pgetmemlock(target);
    mtx_lock(m);
    long r = --(*target);
    mtx_unlock(m);
    return r;
}

#endif
