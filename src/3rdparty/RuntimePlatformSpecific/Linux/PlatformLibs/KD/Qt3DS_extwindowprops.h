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

#ifndef __kd_QT3DS_extwindowprops_h_
#define __kd_QT3DS_extwindowprops_h_
#include <KD/kd.h>
#include <EGL/eglext.h>

#ifdef __cplusplus
extern "C" {
#endif

/* KD_WINDOWPROPERTY_FULLSCREEN: Control over resizing a window to fill the complete screen */

// KDboolean - set the window to fullscreen mode
#define KD_WINDOWPROPERTY_FULLSCREEN_QT3DS 9999

// KDint - set which KD_DISPLAY_* display that the window should be opened on
#define KD_WINDOWPROPERTY_DISPLAY_QT3DS 9998

// KDboolean - sets whether overlay should be used to create window
#define KD_WINDOWPROPERTY_OVERLAY_QT3DS 9997

#define KD_DISPLAY_PRIMARY_QT3DS 0
#define KD_DISPLAY_INTERNAL0_QT3DS 0
#define KD_DISPLAY_INTERNAL1_QT3DS 1
#define KD_DISPLAY_INTERNAL2_QT3DS 2
#define KD_DISPLAY_INTERNAL3_QT3DS 3
#define KD_DISPLAY_EXTERNAL0_QT3DS 1000
#define KD_DISPLAY_EXTERNAL1_QT3DS 1001
#define KD_DISPLAY_EXTERNAL2_QT3DS 1002
#define KD_DISPLAY_EXTERNAL3_QT3DS 1003

#ifdef __cplusplus
}
#endif

#endif /* __kd_QT3DS_extwindowprops_h_ */
