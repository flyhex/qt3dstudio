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

#include <QtGlobal>

#include "Qt3DSId.h"

/// Update masks. Each view will check their mask against the HINT passed in via UpdateAllViews (in
/// their OnUpdate). If
/// their bit is set, they will requery the document for the entire presentation's data. Otherwise,
/// they'll ignore the
/// update.
// These are the new operation update masks
namespace UPDATEMASK {
const long NONE = 0x00000000;
const long UPDATECLIENT = 0x00000010; // Update the Client Scene
const long NEWRESOURCE = 0x00000100; // We've added a new item to the Library
const long NEWRESOURCE_LOADING =
    0x00000200; // This is when we're loading items from the Library - not everything exists yet
const long PROPERTYCHANGE = 0x00000400; // Property on the selected object changed
const long ASSOCIATIONCHANGE = 0x00000800; // An association changed (added or removed)
const long SELECTIONCHANGE = 0x00001000; // The selected object changed
const long RESOURCECHANGE = 0x00002000; // A resource changed (usually a refresh from file)
const long TIMECHANGE = 0x00004000; // The time of the active time context changed
const long NEWPRESENTATION = 0x00008000; // New presentation created - start from scratch
const long CONFIGCHANGE = 0x00020000; // Configuration/Preferences were changed
const long NEWCOMMAND = 0x00040000; // A new CStudioCommand object was committed
const long COMMANDCHANGE = 0x00080000; // A command property has changed (been undone/redone)
const long REACQUIRE = 0x00100000; // Dump stored data and get everything new from Client - resync
const long KEYFRAMEADD = 0x00200000; // A keyframe was inserted
const long KEYFRAMEDELETE = 0x00400000; // A keyframe was removed
const long KEYFRAMESELECT = 0x00800000; // A keyframe was selected
const long KEYFRAMEDESELECT = 0x01000000; // A keyframe was deselected
const long KEYFRAMEMOVE = 0x02000000; // A keyframe was moved (time was changed)
const long TIMEBARCHANGE =
    0x04000000; // Position of a timebar has changed (primarily due to undo/redo)
const long INITSTORAGEPALETTE =
    0x08000000; // Clear storage palette tabs and create the default tabs
const long SHOWHELPPALETTE = 0x10000000; // Show the Help Palette if it is not already showing
const long RESETDATA = 0x20000000; // Reset the data in the view by clearing the view's contents
const long PLAYMODECHANGE = 0x40000000; // The Studio play mode has changed
}

// Used for play mode notifications inside the Document.
enum EPlayMode {
    PLAYMODE_STOP = 1,
    PLAYMODE_PLAY = 2,
};

/// Define used to reference a GUID used internally in Client.
static const GUID GUID_SCENE = {
    0xFFFFFFFF, 0xFFFF, 0xFFFF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

// Used to determine if a keyframe needs to be set on F6.
// This is determined by FLOAT_NUM_DECIMAL_PLACES and should not be larger than
// .1^FLOAT_NUM_DECIMAL_PLACES.
const float FLOAT_DIFF_TOLERANCES = .001f;

// Length of the animation tracks. This needs to be at least 1 ms longer
// than the limit of the timeline.
// This is currently 10 min, 18 sec. The limit is therefore 10 min, 20 sec, 1 ms.
// set to long max.
const long ANIMATION_TRACK_LIMIT = LONG_MAX - 1;

// Snapping grid settings
enum ESnapGridResolution {
    SNAPGRID_SECONDS = 0,
    SNAPGRID_HALFSECONDS = 1,
    SNAPGRID_TICKMARKS = 2,
};

// Client Tool Mode Constants
const long STUDIO_TOOLMODE_SCALE = 0x01;
const long STUDIO_TOOLMODE_MOVE = 0x02;
const long STUDIO_TOOLMODE_ROTATE = 0x04;
const long STUDIO_TOOLMODE_CAMERA_PAN = 0x10;
const long STUDIO_TOOLMODE_CAMERA_ZOOM = 0x20;
const long STUDIO_TOOLMODE_CAMERA_ROTATE = 0x40;

const long STUDIO_CAMERATOOL_MASK = 0xF0;

/// Defines the base duration of the presentation on creation (on File | New).
const long DEFAULT_DURATION = 10000;

// Client Select Mode Constants
const long STUDIO_SELECTMODE_ENTITY = 0x01;
const long STUDIO_SELECTMODE_GROUP = 0x02;

struct SceneDragSenderType
{
    enum Enum {
        Unknown = 0,
        Matte,
        SceneWindow,
    };
};
