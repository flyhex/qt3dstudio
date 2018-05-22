/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once
// User Windows messages
#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define WM_STUDIO_INITIALIZE_PALETTES (WM_APP + 1)
#define WM_STUDIO_DUPLICATE_SERIAL (WM_APP + 2)
#define WM_STUDIO_SHOWCOMPRESSION (WM_APP + 3)
#define WM_STUDIO_LOADPROGRESS (WM_APP + 4)
#define WM_STUDIO_NOTIFYLOADCOMPLETE (WM_APP + 5)
#define WM_STUDIO_TIMER (WM_APP + 6)
#define WM_STUDIO_MESSAGE_ROUTER (WM_APP + 7)
#define WM_STUDIO_OPEN_RECENT_MIN (WM_APP + 9)
#define WM_STUDIO_OPEN_RECENT_MAX (WM_APP + 19)
#define WM_STUDIO_QUICKSTART_MIN (WM_APP + 31)
#define WM_STUDIO_QUICKSTART_MAX (WM_APP + 44)
#define WM_CUSTOMIZE_TOOLBAR (WM_APP + 45)

enum EPreviewApplication { INTERNET_EXPLORER = 0, QT3DS_MEDIA_PLAYER, WINDOWS_MEDIA_PLAYER };

// property pages
const long PAGE_STUDIOAPPPREFERENCES = 0;
const long PAGE_STUDIOPROJECTSETTINGS = 1;

// Playhead moving with keyboard:
const long SHIFT_PLAYHEAD_SMALL_TICK = 0;
const long SHIFT_PLAYHEAD_LARGE_TICK = 1;

// Used to reset default preferences
const long PREFS_SETTINGS_RESTART = 997;
const long PREFS_RESET_LAYOUT = 998;
const long PREFS_RESET_DEFAULTS = 999;

// Message values
#define STUDIO_PLAYHEADUPDATEONLY -1
#define STUDIO_PLAYHEADSYNCPRESENTATION -2

const unsigned long PARAMINDEX_RED = 0;
const unsigned long PARAMINDEX_GREEN = 1;
const unsigned long PARAMINDEX_BLUE = 2;
const unsigned long PARAMINDEX_ALPHA = 3;

const unsigned long PARAMINDEX_X = 0;
const unsigned long PARAMINDEX_Y = 1;
const unsigned long PARAMINDEX_Z = 2;

// These are the new operation update masks
namespace FILTERMASK {
const long FILTER_MODELS = 0x02; // Mask for filtering out Models
const long FILTER_IMAGES = 0x20; // Mask for filtering out Images
const long FILTER_ALL = 0xff; // Used for adding all files back to display mode
}

// Used for timebar display - should we display Comments or Time Start/End/Duration information?
const long TIMEBAR_COMMENTS = 1;
const long TIMEBAR_TIMEINFO = 2;

// Used to set the number of decimal places in the float dialog boxes.
const long FLOAT_NUM_DECIMAL_PLACES = 3;

// Default palette positions
const long DEFAULT_MAINFRM_STARTWIDTH = 25;
const long DEFAULT_MAINFRM_WIDTH = 50;
const long DEFAULT_MAINFRM_STARTHEIGHT = 0;
const long DEFAULT_MAINFRM_HEIGHT = 50;

const long DEFAULT_INSPECTOR_STARTHEIGHT = 50;
const long DEFAULT_INSPECTOR_HEIGHT = 50;
const long DEFAULT_INSPECTOR_STARTWIDTH = 0;
const long DEFAULT_INSPECTOR_WIDTH = 25;

const long DEFAULT_LIBRARY_STARTHEIGHT = 0;
const long DEFAULT_LIBRARY_HEIGHT = 50;
const long DEFAULT_LIBRARY_STARTWIDTH = 0;
const long DEFAULT_LIBRARY_WIDTH = 25;

const long DEFAULT_TIMELINE_STARTHEIGHT = 50;
const long DEFAULT_TIMELINE_HEIGHT = 50;
const long DEFAULT_TIMELINE_STARTWIDTH = 25;
const long DEFAULT_TIMELINE_WIDTH = 75;

const long DEFAULT_HISTORY_STARTHEIGHT = 0;
const long DEFAULT_HISTORY_HEIGHT = 50;
const long DEFAULT_HISTORY_STARTWIDTH = 75;
const long DEFAULT_HISTORY_WIDTH = 25;

const long DEFAULT_STORAGE_STARTHEIGHT = 0;
const long DEFAULT_STORAGE_HEIGHT = 50;
const long DEFAULT_STORAGE_STARTWIDTH = 75;
const long DEFAULT_STORAGE_WIDTH = 25;

const long DEFAULT_HELP_STARTHEIGHT = 25;
const long DEFAULT_HELP_HEIGHT = 50;
const long DEFAULT_HELP_STARTWIDTH = 25;
const long DEFAULT_HELP_WIDTH = 50;

// Port number for serial number verification on the LAN
const long SERIAL_NUMBER_PORT = 3133;
const long SERIAL_NUMBER_BASE = 10;
const long NUMBER_SERIAL_DIGITS_CHECK = 12;

// Constants for AKTRACE statements
// These are bitmasks - must be sequential and not overlap.
const long TRACE_STUDIO_OBJECT = 0x0001;
const long TRACE_TIMELINE = 0x0002;
const long TRACE_KEYFRAMEDATA = 0x0004;
const long TRACE_ANIMTRACKS = 0x0008;
const long TRACE_UNDOREDO = 0x0010;
const long TRACE_LIBRARY = 0x0020;
const long TRACE_MSGHANDLER = 0x0040;
const long TRACE_SNAPPING = 0x0080;
const long TRACE_INSPECTOR = 0x0100;
const long TRACE_CLIENT = 0x0200;
const long TRACE_STORAGE = 0x0400;
const long TRACE_TESTING = 0x0800;
const long TRACE_HELPVIEW = 0x1000;
const long TRACE_STUDIODOC = 0x2000;
const long TRACE_MAINFRAME = 0x4000;
const long TRACE_ALL = 0xffff;

const char CLIENT_CORE_DLL_NAME[] = "AKCore.dll";

// Timeline icon dimensions
const long TREEICON_WIDTH = 14;
const long TREEICON_HEIGHT = 14;
const long TIMER_DRAGHOVEREXPAND = 2;
const long TIMER_DRAGDROPSCROLL = 1;
const long DRAGSCROLL_DIRECTION_UP = 1;
const long DRAGSCROLL_DIRECTION_DOWN = 2;
const long TIMER_DRAGDROPSCROLL_DELAY = 20;
const long TIMER_DRAGHOVEREXPAND_DELAY = 1500; // 2000;	// 3-second delay
