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

#ifndef __kd_QT3DS_multitouch_h_
#define __kd_QT3DS_multitouch_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* KD_IOGROUP_MULTITOUCH: I/O group for Multitouch input devices. */
#define KD_IOGROUP_MULTITOUCH_QT3DS 0x40004000
#define KD_STATE_MULTITOUCH_AVAILABILITY_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 0)
#define KD_INPUT_MULTITOUCH_FINGERS_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 1)
#define KD_INPUT_MULTITOUCH_WIDTH_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 2)
#define KD_INPUT_MULTITOUCH_X_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 3)
#define KD_INPUT_MULTITOUCH_Y_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 4)
#define KD_INPUT_MULTITOUCH_X2_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 5)
#define KD_INPUT_MULTITOUCH_Y2_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 6)
#define KD_INPUT_MULTITOUCH_PRESSURE_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 7)
#define KD_INPUT_MULTITOUCH_GESTURES_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 8)
#define KD_INPUT_MULTITOUCH_RELX_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 9)
#define KD_INPUT_MULTITOUCH_RELY_QT3DS (KD_IOGROUP_MULTITOUCH_QT3DS + 10)

/* KD_EVENT_INPUT_MULTITOUCH_QT3DS: Multitouch event. */
#define KD_EVENT_INPUT_MULTITOUCH_QT3DS 1001
typedef struct KDEventInputMultitouchDataNV
{
    KDint32 index;
    KDint8 fingers;
    KDint8 width;
    KDint16 x;
    KDint16 y;
    KDint16 x2;
    KDint16 y2;
    KDint16 pressure;
} KDEventInputMultitouchDataNV;

/* KD_EVENT_INPUT_MULTITOUCH_GESTURE_QT3DS: Multitouch gesture event. */
#define KD_EVENT_INPUT_MULTITOUCH_GESTURE_QT3DS 1002

/* kdGetEventInputMultitouchDataNV: Get auxiliary event data for multitouch input. */
KD_API KDint KD_APIENTRY kdGetEventInputMultitouchDataNV(const KDEvent *event,
                                                         KDEventInputMultitouchDataNV *data);

/* kdSetEventInputMultitouchActiveNV: Activate Multitouch input events */
KD_API KDint KD_APIENTRY kdSetEventInputMultitouchActiveNV(KDboolean activate);

/* kdEnableEventInputMultitouchMergeNV: Activate merging of Multitouch input events */
KD_API void KD_APIENTRY kdEnableEventInputMultitouchMergeNV(KDboolean enable);

#ifdef __cplusplus
}
#endif

#endif /* __kd_QT3DS_multitouch_h_ */
