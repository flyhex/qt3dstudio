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

#ifndef __INCLUDED_QT3DS_THREAD_H
#define __INCLUDED_QT3DS_THREAD_H

#include <jni.h>
#include <pthread.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** @file nv_thread.h
  The NVThread library makes it easy to create native threads that can acess
  JNI objects.  By default, pthreads created in the Android NDK are NOT connected
  to the JVM and JNI calls will fail.  This library wraps thread creation in
  such a way that pthreads created using it will connect to and disconnect from
  the JVM as appropriate.  Applications creating all of their threads with these
  interfaces can use the provided NVThreadGetCurrentJNIEnv() function to
  get the current thread's JNI context at any time.

  Note that native-created threads still have JNI limitations over threads
  that are calls down to native from Java.  The JNI function FindClass will
  NOT find application-specific classes when called from native threads.
  Native code that needs to call FindClass and record the indices of Java
  class members for later access must call FindClass and Get*FieldID/Get*MethodID
  in threads calling from Java, such as JNI_OnLoad
 */

/**
  Initializes the nv_thread system by connecting it to the JVM.  This
  function must be called as early as possible in the native code's
  JNI_OnLoad function, so that the thread system is prepared for any
  JNI-dependent library initialization calls.
  @param vm The VM pointer - should be the JavaVM pointer sent to JNI_OnLoad.
  */
void NVThreadInit(JavaVM *vm);

/**
  Retrieves the JNIEnv object associated with the current thread, allowing
  any thread that was creating with NVThreadSpawnJNIThread() to access the
  JNI at will.  This JNIEnv is NOT usable across multiple calls or threads
  The function should be called in each function that requires a JNIEnv
  @return The current thread's JNIEnv, or NULL if the thread was not created
  by NVThreadSpawnJNIThread
  @see NVThreadSpawnJNIThread
  */
JNIEnv *NVThreadGetCurrentJNIEnv();

/**
  Spwans a new native thread that is registered for use with JNI.  Threads
  created with this function will have access to JNI data via the JNIEnv
  available from NVThreadGetCurrentJNIEnv().
  @param thread is the same as in pthread_create
  @param attr is the same as in pthread_create
  @param start_routine is the same as in pthread_create
  @param arg is the same as in pthread_create
  @return 0 on success, -1 on failure
  @see NVThreadGetCurrentJNIEnv
*/
int NVThreadSpawnJNIThread(pthread_t *thread, pthread_attr_t const *attr,
                           void *(*start_routine)(void *), void *arg);

/**
  Sleeps the current thread for the specified number of milliseconds
  @param millisec Sleep time in ms
  @return 0 on success, -1 on failure
*/
int NVThreadSleep(unsigned long millisec);

#if defined(__cplusplus)
}
#endif

#endif
