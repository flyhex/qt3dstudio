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

#include "nv_util.h"
#include <jni.h>
#include "../nv_thread/nv_thread.h"
#include <string.h>

static jmethodID g_hasAppLocalValue;
static jmethodID g_getAppLocalValue;
static jmethodID g_setAppLocalValue;
static jmethodID g_getParameter;
static jobject g_globalThiz;

void NvUtilInit()
{
    JNIEnv *jniEnv = NVThreadGetCurrentJNIEnv();
    jclass k = jniEnv->FindClass("com/nvidia/devtech/NvUtil");
    g_hasAppLocalValue = jniEnv->GetMethodID(k, "hasAppLocalValue", "(Ljava/lang/String;)Z");
    g_getAppLocalValue =
        jniEnv->GetMethodID(k, "getAppLocalValue", "(Ljava/lang/String;)Ljava/lang/String;");
    g_setAppLocalValue =
        jniEnv->GetMethodID(k, "setAppLocalValue", "(Ljava/lang/String;Ljava/lang/String;)V");
    g_getParameter =
        jniEnv->GetMethodID(k, "getParameter", "(Ljava/lang/String;)Ljava/lang/String;");

    jmethodID getInstance =
        jniEnv->GetStaticMethodID(k, "getInstance", "()Lcom/nvidia/devtech/NvUtil;");
    g_globalThiz = jniEnv->CallStaticObjectMethod(k, getInstance);
}

static void NvUtilGetStringValue(jmethodID method, char *buffer, int bufferLength, const char *name)
{
    JNIEnv *jniEnv = NVThreadGetCurrentJNIEnv();
    jstring nameJava = jniEnv->NewStringUTF(name);
    jstring valueJava = (jstring)jniEnv->CallObjectMethod(g_globalThiz, method, nameJava);

    int len = 0;
    if (valueJava) {
        const char *string = jniEnv->GetStringUTFChars(valueJava, NULL);

        len = strlen(string);
        if (len > bufferLength - 1)
            len = bufferLength - 1;
        strncpy(buffer, string, len);
        jniEnv->ReleaseStringUTFChars(valueJava, string);
    }
    buffer[len] = '\0';
}

void NvUtilGetLocalAppValue(char *buffer, int bufferLength, const char *name)
{
    NvUtilGetStringValue(g_getAppLocalValue, buffer, bufferLength, name);
}

bool NvUtilHasLocalAppValue(const char *name)
{
    JNIEnv *jniEnv = NVThreadGetCurrentJNIEnv();
    jstring nameJava = jniEnv->NewStringUTF(name);
    jboolean hasIt = jniEnv->CallBooleanMethod(g_globalThiz, g_hasAppLocalValue, nameJava);

    return (hasIt == JNI_TRUE) ? true : false;
}

void NvUtilSetLocalAppValue(const char *name, const char *value)
{
    JNIEnv *jniEnv = NVThreadGetCurrentJNIEnv();
    jstring nameJava = jniEnv->NewStringUTF(name);
    jstring valueJava = jniEnv->NewStringUTF(value);
    jniEnv->CallVoidMethod(g_globalThiz, g_setAppLocalValue, nameJava, valueJava);
}

void NvUtilGetParameter(char *buffer, int bufferLength, const char *parameter)
{
    NvUtilGetStringValue(g_getParameter, buffer, bufferLength, parameter);
}
