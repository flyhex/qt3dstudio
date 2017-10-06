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

#ifndef __INCLUDED_NV_UTIL_H
#define __INCLUDED_NV_UTIL_H

/** @file nv_util.h
 * Used to set and retrieve Java parameter and shader values from
 * within native code.  Both command-line arguments and predefined
 * shared values, such as the location of the external storage are
 * available
 */

/**
  Initializes the library.  This function MUST be called from the application's
  JNI_OnLoad, from a function known to be called by JNI_OnLoad, or from a function
  in a Java-called thread.  nv_thread-created native threads cannot call this
  initialization function.
  @see nv_thread.h
  */
void NvUtilInit();

/**
 * Used to retrieve Java-set string values in native code
 * Pre-set values include:
 * <ul>
 *  <li> STORAGE_ROOT: The full path to the external storage device (SD card)
 * </ul>
 * Gets the specified key value from the app local value list
 * @param buffer The buffer to receive the value's string value
 * @param bufferLength the size of buffer in bytes
 * @param The name of the value to retrieve
 */
void NvUtilGetLocalAppValue(char *buffer, int bufferLength, const char *name);

/**
 * Used to verify Java-set string values in native code
 * @param name The name of the value to verify
 * @return true if the value exists, false if not
 */
bool NvUtilHasLocalAppValue(const char *name);

/**
 * Used to share string values between native and Java code
 * Sets the specified key value in the app local value list
 * @param name The key to set the value of
 * @param value The value
 */
void NvUtilSetLocalAppValue(const char *name, const char *value);

/**
 * A JNI-accessible version of NvUtil::getParameter.  This function is used to get the parameters
 * used to start the Activity via, for example:
 * <pre>
 * adb shell am start -a android.intent.action.MAIN -n
 * com.nvidia.devtech.water/com.nvidia.devtech.water.Water -e param1 1 -e param2 2
 * </pre>
 * Where "param1" and "param2" are the parameter names and "1" and "2" are the parameter values.
 *
 * @param buffer The buffer to receive the parameter's string value
 * @param bufferLength the size of buffer in bytes
 * @param The name of the parameter to retrieve
 */
void NvUtilGetParameter(char *buffer, int bufferLength, const char *parameter);

#endif
