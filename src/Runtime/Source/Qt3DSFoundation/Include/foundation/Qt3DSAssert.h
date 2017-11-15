/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#pragma once
#ifndef QT3DS_ASSERT_H
#define QT3DS_ASSERT_H
#include "EABase/eabase.h"
#include "EABase/config/eacompiler.h"
#include "EABase/config/eacompilertraits.h"
#include "EABase/config/eaplatform.h"
#include "foundation/Qt3DSPreprocessor.h"

#ifdef QT3DS_WINDOWS

#include <stdio.h>

#ifdef QT3DS_VC
#include <crtdbg.h>

extern void __debugbreak();
#ifndef QT3DS_DOXYGEN

namespace qt3ds {
#endif
QT3DS_INLINE void NVAssert(const char *exp, const char *file, int line, bool *ignore)
{
// printf("Assertion failed: %s, file %s, line %d\n", exp, file, line);
#ifdef _DEBUG
    if (ignore != NULL) {
        int reportType = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_REPORT_MODE);
        // PH: _CrtDbgReport returns -1 on error, 0 on 'ignore', 1 on 'retry'. Hitting 'abort' will
        // terminate the process immediately.
        // If the mode is not 'Window', we just always break.
        *ignore = *ignore || ((reportType == _CRTDBG_MODE_WNDW)
                              && (_CrtDbgReport(_CRT_ASSERT, file, line, NULL, "%s", exp) == 0));
    }

    if (ignore == NULL || !*ignore)
#else
    QT3DS_FORCE_PARAMETER_REFERENCE(exp);
    QT3DS_FORCE_PARAMETER_REFERENCE(file);
    QT3DS_FORCE_PARAMETER_REFERENCE(line);
    QT3DS_FORCE_PARAMETER_REFERENCE(ignore);
#endif
        __debugbreak();
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

#ifdef __CUDACC__
#define QT3DS_ASSERT(exp) ((void)0)
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#else
#ifndef NDEBUG
#define QT3DS_ASSERT(exp)                                                                             \
    {                                                                                              \
        static bool ignore = false;                                                                \
        (void)((!!(exp)) || (qt3ds::NVAssert(#exp, __FILE__, __LINE__, &ignore), false));             \
    }
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp)                                                              \
    {                                                                                              \
        static bool ignore = false;                                                                \
        (void)((qt3ds::NVAssert(#exp, __FILE__, __LINE__, &ignore), false));                          \
    }
#else
#define QT3DS_ASSERT(exp) ((void)0)
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#endif
#endif
#else

#include <assert.h>

namespace qt3ds {

QT3DS_INLINE void NVAssert(const char *exp, const char *file, int line, bool *ignore)
{
    QT3DS_FORCE_PARAMETER_REFERENCE(exp);
    QT3DS_FORCE_PARAMETER_REFERENCE(file);
    QT3DS_FORCE_PARAMETER_REFERENCE(line);
    QT3DS_FORCE_PARAMETER_REFERENCE(ignore);
    assert(false);
}

} // qt3ds

#ifndef NDEBUG
#define QT3DS_ASSERT(exp)                                                                   \
    {                                                                                       \
        static bool ignore = false;                                                         \
        (void)((!!(exp)) || (qt3ds::NVAssert(#exp, __FILE__, __LINE__, &ignore), false));   \
    }
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp)                                                    \
    {                                                                                       \
        static bool ignore = false;                                                         \
        (void)((qt3ds::NVAssert(#exp, __FILE__, __LINE__, &ignore), false));                \
    }
#else
#define QT3DS_ASSERT(exp) ((void)0)
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#endif

#endif

#elif defined(QT3DS_PS3)
#include "foundation/ps3/NVPS3Assert.h"

#else // Force the user to define the NVAssert function
namespace qt3ds {
void NVAssert(const char *exp, const char *file, int line, bool *ignore);
}
#ifdef __CUDACC__
#define QT3DS_ASSERT(exp) ((void)0)
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#else
#ifndef NDEBUG
#define QT3DS_ASSERT(exp)                                                                             \
    {                                                                                              \
        static bool ignore = false;                                                                \
        (void)((!!(exp)) || (qt3ds::NVAssert(#exp, __FILE__, __LINE__, &ignore), false));             \
    }
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp)                                                              \
    {                                                                                              \
        static bool ignore = false;                                                                \
        (void)((qt3ds::NVAssert(#exp, __FILE__, __LINE__, &ignore), false));                          \
    }
#else
#define QT3DS_ASSERT(exp) ((void)0)
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#endif
#endif
#endif

#define QT3DS_ALWAYS_ASSERT() QT3DS_ASSERT(0)

#endif
