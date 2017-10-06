/****************************************************************************
**
** Copyright (C) 2007 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/* Sample KD/kdplatform.h for OpenKODE Core 1.0.3  */
#ifndef __kdplatform_h_
#define __kdplatform_h_

#define KD_API
#define KD_APIENTRY

typedef int KDint32;
typedef unsigned int KDuint32;
typedef long long KDint64;
typedef unsigned long long KDuint64;
typedef short KDint16;
typedef unsigned short KDuint16;
typedef unsigned long KDuintptr;
typedef unsigned long KDsize;
typedef long KDssize;
#define KDINT_MIN (-0x7fffffff-1)
#define KDINT_MAX 0x7fffffff
#define KDUINT_MAX 0xffffffffU
#define KDINT64_MIN (-0x7fffffffffffffffLL-1)
#define KDINT64_MAX 0x7fffffffffffffffLL
#define KDUINT64_MAX 0xffffffffffffffffULL
#define KDSSIZE_MIN (-0x7fffffff-1)
#define KDSSIZE_MAX 0x7fffffff
#define KDSIZE_MAX 0xffffffffU
#define KDUINTPTR_MAX 0xffffffffU
#define KD_NORETURN
#define KD_WINDOW_SUPPORTED
#ifdef KD_NDEBUG
#define kdAssert(c)
#else
#define kdAssert(c) ((void)( (c) ? 0 : (kdHandleAssertion(#c, __FILE__, __LINE__), 0)))
#endif

#define KD_INFINITY_BITS 0x7f800000u
#define KD_INFINITY (kdBitsToFloatNV(KD_INFINITY_BITS))

KD_API float KD_APIENTRY kdBitsToFloatNV(KDuint32 x);


/** KHR_formatted extension */
#include <stdarg.h>
typedef va_list KDVaListKHR;

// If there are problems with including stdarg.h here under any of the Windows
// operating systems, we might want to change it back to using char* instead of
// va_list
//typedef char *KDVaListKHR;


#define KD_VA_START_KHR(ap, parmN)  \
        do {ap = (KDVaListKHR)&parmN + sizeof(parmN);} while(0)

#define KD_VA_ARG_CHAR_KHR(ap)      (KDchar)va_arg(ap, KDint)
#define KD_VA_ARG_CHARPTR_KHR(ap)   (KDchar*)(KD_VA_ARG_PTR_KHR(ap))
#define KD_VA_ARG_INT16_KHR(ap)     (KDint16)va_arg(ap, KDint)
#define KD_VA_ARG_INT32_KHR(ap)     va_arg(ap, KDint32)
#define KD_VA_ARG_INT_KHR(ap)       va_arg(ap, KDint)
#define KD_VA_ARG_INT64_KHR(ap)     va_arg(ap, KDint64)
#define KD_VA_ARG_INTPTR_KHR(ap)    (KDint*)(KD_VA_ARG_PTR_KHR(ap))
#define KD_VA_ARG_INT16PTR_KHR(ap)  (KDint16*)(KD_VA_ARG_PTR_KHR(ap))
#define KD_VA_ARG_FLOAT32_KHR(ap)   (KDfloat32)(va_arg(ap, double))
#define KD_VA_ARG_PTR_KHR(ap)       (void *)va_arg(ap, char *)

#define KD_VA_END_KHR(ap)           \
        do {ap = (KDVaListKHR)0;} while(0)

#endif /* __kdplatform_h_ */

