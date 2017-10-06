#############################################################################
##
## Copyright (C) 2009-2010 NVIDIA Corporation.
## Copyright (C) 2017 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of Qt 3D Studio.
##
## $QT_BEGIN_LICENSE:GPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 or (at your option) any later version
## approved by the KDE Free Qt Foundation. The licenses are as published by
## the Free Software Foundation and appearing in the file LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

MY_LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_math
LOCAL_MODULE := nvmath
LOCAL_SRC_FILES := nv_math.cpp nv_matrix.cpp nv_quat.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_shader
LOCAL_MODULE := nvshader
LOCAL_SRC_FILES := nv_shader.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_util
LOCAL_MODULE := nvutil
LOCAL_SRC_FILES := nv_util.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_time
LOCAL_MODULE := nvtime
LOCAL_SRC_FILES := nv_time.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_event
LOCAL_MODULE := nvevent
LOCAL_SRC_FILES := nv_event.cpp nv_event_queue.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_file
LOCAL_MODULE := nvfile
LOCAL_SRC_FILES := nv_file.c
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_thread
LOCAL_MODULE := nvthread
LOCAL_SRC_FILES := nv_thread.c
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_hhdds
LOCAL_MODULE := nvhhdds
LOCAL_SRC_FILES := nv_hhdds.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_glesutil
LOCAL_MODULE := nvglesutil
LOCAL_SRC_FILES := nv_images.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_sound
LOCAL_MODULE := nvsound
LOCAL_SRC_FILES := nv_sound.cpp
LOCAL_ARM_MODE   := arm

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_apk_file_am
LOCAL_MODULE := nv_apk_file_am
LOCAL_SRC_FILES := nv_apk_file_am.c
LOCAL_ARM_MODE   := arm
LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/KD
LOCAL_MODULE := MinKD
LOCAL_SRC_FILES := MinKD.c
LOCAL_ARM_MODE   := arm
LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_gesture
LOCAL_MODULE := nvgesture
LOCAL_SRC_FILES := nv_gesture.c
LOCAL_ARM_MODE   := arm
LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_LOCAL_PATH)/nv_bitfont
LOCAL_MODULE := nvbitfont
LOCAL_SRC_FILES := nv_bitfont.cpp
LOCAL_ARM_MODE   := arm
LOCAL_CFLAGS := -DOPENKODE -DANDROID
LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
