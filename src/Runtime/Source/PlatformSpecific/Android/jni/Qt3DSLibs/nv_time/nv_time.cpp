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

#include "nv_time.h"
#include "../nv_thread/nv_thread.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <time.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>

#ifndef EGL_NV_system_time
#define EGL_NV_system_time 1
typedef khronos_uint64_t EGLuint64NV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeFrequencyNV(void);
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeNV(void);
#endif
typedef EGLuint64NV(EGLAPIENTRYP PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)(void);
typedef EGLuint64NV(EGLAPIENTRYP PFNEGLGETSYSTEMTIMENVPROC)(void);
#endif

void nvAcquireTimeExtensionJNI(JNIEnv *, jobject)
{
    nvAcquireTimeExtension();
}

jlong nvGetSystemTimeJNI(JNIEnv *, jobject)
{
    return (jlong)nvGetSystemTime();
}

static PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC eglGetSystemTimeFrequencyNVProc = NULL;
static PFNEGLGETSYSTEMTIMENVPROC eglGetSystemTimeNVProc = NULL;
static EGLuint64NV eglSystemTimeFrequency = 0;
static bool timeExtensionQueried = false;

void nvAcquireTimeExtension()
{
    if (timeExtensionQueried)
        return;
    timeExtensionQueried = true;

    eglGetSystemTimeFrequencyNVProc =
        (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)eglGetProcAddress("eglGetSystemTimeFrequencyNV");
    eglGetSystemTimeNVProc = (PFNEGLGETSYSTEMTIMENVPROC)eglGetProcAddress("eglGetSystemTimeNV");

    // now, we'll proceed through a series of sanity checking.
    // if they all succeed, we'll return.
    // if any fail, we fall out of conditional tests to end of function, null pointers, and return.
    if (eglGetSystemTimeFrequencyNVProc && eglGetSystemTimeNVProc) {
        eglSystemTimeFrequency = eglGetSystemTimeFrequencyNVProc();
    }

    // fall back if we've not returned already.
    eglGetSystemTimeFrequencyNVProc = (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)NULL;
    eglGetSystemTimeNVProc = (PFNEGLGETSYSTEMTIMENVPROC)NULL;
}

bool nvValidTimeExtension()
{
    if (NULL == eglGetSystemTimeFrequencyNVProc || NULL == eglGetSystemTimeNVProc)
        return false;
    else
        return true;
}

long nvGetSystemTime()
{
    static struct timeval start_time, end_time;
    static int isinit = 0;
    jlong curr_time = 0;

    if (eglGetSystemTimeNVProc) {
        EGLuint64NV egltime;
        EGLuint64NV egltimequot;
        EGLuint64NV egltimerem;

        egltime = eglGetSystemTimeNVProc();

        egltimequot = egltime / eglSystemTimeFrequency;
        egltimerem = egltime - (eglSystemTimeFrequency * egltimequot);
        egltimequot *= 1000;
        egltimerem *= 1000;
        egltimerem /= eglSystemTimeFrequency;
        egltimequot += egltimerem;
        return (jlong)egltimequot;
    }

    if (!isinit) {
        gettimeofday(&start_time, 0);
        isinit = 1;
    }
    gettimeofday(&end_time, 0);
    curr_time = (end_time.tv_sec - start_time.tv_sec) * 1000;
    curr_time += (end_time.tv_usec - start_time.tv_usec) / 1000;

    return curr_time;
}

void NVTimeInit()
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    JNINativeMethod methods_time[] = {
        { "nvAcquireTimeExtension", "()V", (void *)nvAcquireTimeExtension },
        { "nvGetSystemTime", "()J", (void *)nvGetSystemTime },
    };
    jclass k_time;
    k_time = (env)->FindClass("com/nvidia/devtech/NvActivity");
    (env)->RegisterNatives(k_time, methods_time, 2);
}
