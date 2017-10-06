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

#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include "../nv_thread/nv_thread.h"

#define MODULE "soundlib"
#define THE_ACTIVITY "com/nvidia/devtech/AudioHelper"

#include "nv_sound.h"

static jobject s_globalThiz;
static jclass NvAudioHelper_class;

static jmethodID s_load;
static jmethodID s_loadasset;
static jmethodID s_resume;
static jmethodID s_stop;
static jmethodID s_play;
static jmethodID s_volume;
static jmethodID s_unload;
static jmethodID s_source;
static jmethodID s_startmusic;
static jmethodID s_musicvolume;
static jmethodID s_stopmusic;
static jmethodID s_maxvolume;

void NvSoundInit()
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    NvAudioHelper_class = env->FindClass(THE_ACTIVITY);

    jmethodID getInstance = env->GetStaticMethodID(NvAudioHelper_class, "getInstance",
                                                   "()Lcom/nvidia/devtech/AudioHelper;");

    __android_log_print(ANDROID_LOG_DEBUG, "apk", "inst = %d\n", getInstance);

    s_load = env->GetMethodID(NvAudioHelper_class, "LoadSound", "(Ljava/lang/String;I)I");
    s_loadasset = env->GetMethodID(NvAudioHelper_class, "LoadSoundAsset", "(Ljava/lang/String;I)I");
    s_resume = env->GetMethodID(NvAudioHelper_class, "ResumeSound", "(I)V");
    s_stop = env->GetMethodID(NvAudioHelper_class, "StopSound", "(I)V");
    s_play = env->GetMethodID(NvAudioHelper_class, "PlaySound", "(IFFIIF)I");
    s_volume = env->GetMethodID(NvAudioHelper_class, "SetVolume", "(IFF)V");
    s_unload = env->GetMethodID(NvAudioHelper_class, "UnloadSample", "(I)Z");
    s_source = env->GetMethodID(NvAudioHelper_class, "MusicSetDataSource", "(Ljava/lang/String;)V");
    s_startmusic = env->GetMethodID(NvAudioHelper_class, "MusicStart", "()V");
    s_musicvolume = env->GetMethodID(NvAudioHelper_class, "MusicVolume", "(FF)V");
    s_stopmusic = env->GetMethodID(NvAudioHelper_class, "MusicStop", "()V");
    s_maxvolume = env->GetMethodID(NvAudioHelper_class, "SetMaxVolume", "()V");

    jobject thiz = env->CallStaticObjectMethod(NvAudioHelper_class, getInstance);

    if (thiz == NULL) {
        __android_log_print(ANDROID_LOG_DEBUG, MODULE, "no this");
    }

    s_globalThiz = env->NewGlobalRef(thiz);
}

void NVSoundShutdown()
{
    s_globalThiz = NULL;
}

int SoundPoolLoadSFX(const char *FileName, int Priority)
{
    int SoundID;

    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    jstring s = env->NewStringUTF(FileName);
    SoundID = env->CallIntMethod(s_globalThiz, s_load, s, Priority);
    env->DeleteLocalRef((jobject)s);

    return SoundID;
}

int SoundPoolLoadSFXAsset(const char *FileName, int Priority)
{
    int SoundID;

    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    jstring s = env->NewStringUTF(FileName);
    SoundID = env->CallIntMethod(s_globalThiz, s_loadasset, s, Priority);
    env->DeleteLocalRef((jobject)s);

    return SoundID;
}

void SoundPoolResume(int StreamID)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->CallVoidMethod(s_globalThiz, s_resume, StreamID);
}

void SoundPoolStop(int StreamID)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    jint i = StreamID;

    env->CallVoidMethod(s_globalThiz, s_stop, i);
}

int SoundPoolPlaySound(int SoundID, float LeftVolume, float RightVolume, int Priority, int Loop,
                       float Rate)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    return env->CallIntMethod(s_globalThiz, s_play, SoundID, LeftVolume, RightVolume, Priority,
                              Loop, Rate);
}

void SoundPoolSetVolume(int StreamID, float LeftVolume, float RightVolume)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->CallVoidMethod(s_globalThiz, s_volume, StreamID, LeftVolume, RightVolume);
}

bool SoundPoolUnloadSample(int SoundID)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    return env->CallBooleanMethod(s_globalThiz, s_unload, SoundID);
}

void MediaPlayerSetDataSource(const char *FileName)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    jstring s = env->NewStringUTF(FileName);
    env->CallVoidMethod(s_globalThiz, s_source, s);
    env->DeleteLocalRef((jobject)s);
}

void MediaPlayerStart()
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->CallVoidMethod(s_globalThiz, s_startmusic);
}

void MediaPlayerSetVolume(float LeftVolume, float RightVolume)
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->CallVoidMethod(s_globalThiz, s_musicvolume, LeftVolume, RightVolume);
}

void MediaPlayerStop()
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->CallVoidMethod(s_globalThiz, s_stopmusic);
}

void MediaSetMaxVolume()
{
    JNIEnv *env = NVThreadGetCurrentJNIEnv();

    env->CallVoidMethod(s_globalThiz, s_maxvolume);
}
