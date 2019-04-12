/****************************************************************************
**
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

/* TAGRELEASE: public */

#ifndef _QT3DSLAUNCHERDEFS_H
#define _QT3DSLAUNCHERDEFS_H

/* Applications should call kdSetWindowPropertyiv with
 * KD_WINDOWPROPERTY_APP_ID_QT3DS as the pname
 * and one of the ID's below as the param
 * Or else define their own in the range starting with
 * KD_APP_ID_USER_BASE_QT3DS
 * call must be made _prior_ to window realize
 */

/* Need to define an NV internal KD property range - defining to a fixed value for now */
/* But this window property is unused in the current KD spec */
#define KD_WINDOWPROPERTY_APP_ID_QT3DS 0x1000

#define KD_APP_ID_UNKNOWN_QT3DS 0
#define KD_APP_ID_KEYBOARD_QT3DS 1 /* QT3DS RESERVED */
#define KD_APP_ID_CONTACTS_QT3DS 2 /* QT3DS RESERVED */
#define KD_APP_ID_MESSAGING_QT3DS 3 /* QT3DS RESERVED */
#define KD_APP_ID_EMAIL_QT3DS 4 /* QT3DS RESERVED */
#define KD_APP_ID_IM_QT3DS 5 /* QT3DS RESERVED */
#define KD_APP_ID_PHONEDIALER_QT3DS 6 /* QT3DS RESERVED */
#define KD_APP_ID_GAME_QT3DS 7
#define KD_APP_ID_ADDRESSBOOK_QT3DS 8 /* Assigned to NV addrbook demo */
#define KD_APP_ID_MUSICPLAYER_QT3DS 9 /* Assigned to NV mediaplayer demo */
#define KD_APP_ID_SMS_QT3DS 10 /* QT3DS RESERVED */
#define KD_APP_ID_IMAGEBROWSER_QT3DS 11 /* Assigned to NV photoviewer demo */
#define KD_APP_ID_MAP_QT3DS 12 /* QT3DS RESERVED */
#define KD_APP_ID_HOME_QT3DS 13 /* Assigned to NV launcher_gui home screen */
#define KD_APP_ID_CALENDAR_QT3DS 14 /* QT3DS RESERVED */
#define KD_APP_ID_VIDEO_QT3DS 15 /* Assigned to NV videoplayer demo */
#define KD_APP_ID_WIDGETS_QT3DS 16 /* QT3DS RESERVED */
#define KD_APP_ID_CAMERA_QT3DS 19 /* QT3DS RESERVED */
#define KD_APP_ID_MEDIAWALL_QT3DS 20 /* QT3DS RESERVED */
#define KD_APP_ID_OPERA_QT3DS 21 /* QT3DS RESERVED */
#define KD_APP_ID_YOUTUBE_QT3DS 22 /* QT3DS RESERVED */
#define KD_APP_ID_VISION_QT3DS 23 /* QT3DS RESERVED */
#define KD_APP_ID_USER_BASE_QT3DS 1024 /* Base of app-reserved */

/* starting orientation support */
#define KD_WINDOWPROPERTY_ORIENTATION_QT3DS 0x1001

#define KD_WINDOWPROPERTY_STATUSBAR_QT3DS 0x1002
#define KD_STATUSBAR_HIDEALL_QT3DS 0x0000
#define KD_STATUSBAR_SHOWBAR_QT3DS 0x0001
#define KD_STATUSBAR_SHOWBG_QT3DS 0x0002 /* not set as state, just for mask */
#define KD_STATUSBAR_SHOWALL_QT3DS 0x0003 /* this is showbar+showbg in bits...*/

#define KD_WINDOWPROPERTY_PROCESS_ID_QT3DS 0x1003

/* App-level KD state - if this exists and is nonzero, then the app is */
/* Running in the launcher */

#define KD_STATE_LAUNCHER_ACTIVE_QT3DS 0x11001

/* Part of GL_NV_log_textures extension */
/* Not currently supported in external release */
#define GL_LOG_TEXTURES_QT3DS 0x85FF
#define GL_LOG_TEXTURES_ZONE_QT3DS 0x85FE

#endif
