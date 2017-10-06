/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
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

#ifndef SCOPED_PROFILER_H
#define SCOPED_PROFILER_H

#define PERF_STMTS 0
#if PERF_STMTS == 1
#include "../nv_time/nv_time.h"
#include "stdio.h"
#include "stdlib.h"

static char s_bigString[4096];
static int s_bigStringSize;

static char s_tmpBuf[1024];

class ScopedProfiler
{
public:
    ScopedProfiler(const char *text)
    {
        _text = text;
        _startTime = nvGetSystemTime();
        __last = this;
    }
    ~ScopedProfiler() { stop(); }
    inline void stop()
    {
        if (_text) {
            int size = snprintf(s_tmpBuf, dimof(s_tmpBuf) - 1, "%d ms spent in %s",
                                (int)(nvGetSystemTime() - _startTime), _text);
            strcat(s_bigString + s_bigStringSize, s_tmpBuf);
            s_bigStringSize += size;
            _text = 0;
        }
    }
    static void stopLast()
    {
        if (__last)
            __last->stop();
        __last = 0;
    }
    const char *_text;
    long _startTime;
    static ScopedProfiler *__last;
};
ScopedProfiler *ScopedProfiler::__last = 0;

#define STRINGIFIER(s) #s
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define PERFBLURB(s)                                                                               \
    static const char CONCAT(___str, __LINE__)[] = s "\n";                                         \
    ScopedProfiler CONCAT(SCOPED_PROFILER, __LINE__)(CONCAT(___str, __LINE__));
#define RESET_PROFILING()                                                                          \
    {                                                                                              \
        DEBUG_D("%s", s_bigString);                                                                \
        s_bigString[0] = 0;                                                                        \
        s_bigStringSize = 0;                                                                       \
    }
#else
#define PERFBLURB(s)
#define RESET_PROFILING()
#endif

#endif // SCOPED_PROFILER_H
