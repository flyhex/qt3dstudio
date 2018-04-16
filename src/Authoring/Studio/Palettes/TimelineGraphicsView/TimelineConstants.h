/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef TIMELINECONSTANTS_H
#define TIMELINECONSTANTS_H

namespace TimelineConstants
{
    // Dimensions
    const int ROW_H             = 20;
    const int ROW_SPACING       = 2;
    const int RULER_SEC_W       = 30;    // width of 1 second section (at scale 1)
    const int RULER_SEC_DIV     = 10;    // second divisions
    const int RULER_DIV_H1      = 5;     // height of main divisions
    const int RULER_DIV_H2      = 3;     // height of secondary divisions
    const int RULER_DIV_H3      = 1;     // height of minor divisions
    const int RULER_BASE_Y      = 18;    // baseline Y
    const int RULER_LABEL_W     = 60;
    const int RULER_LABEL_H     = 10;
    const int RULER_TICK_SCALE1 = 2;
    const int RULER_TICK_SCALE2 = 3;
    const int RULER_TICK_SCALE3 = 6;
    const int RULER_TICK_SCALE4 = 21;

    const double RULER_EDGE_OFFSET = 15;
    const double TREE_MIN_W     = 160;
    const double TREE_MAX_W     = 600;
    const double TREE_DEFAULT_W = 250;
    const double TREE_BOUND_W   = 10000; // real width of the row (> max possible visible tree area)
    const double TREE_ICONS_W   = 53;
    const int SPLITTER_W        = 8;
    const int PLAYHEAD_W        = 14;
    const int DURATION_HANDLE_W = 14;    // width of duration end handles in a timeline row

    // Colors
    const char ROW_COLOR_NORMAL[]              = "#404040";
    const char ROW_COLOR_NORMAL_PROP[]         = "#373737";
    const char ROW_COLOR_OVER[]                = "#4d4d4d";
    const char ROW_COLOR_SELECTED[]            = "#336699";
    const char ROW_COLOR_DURATION[]            = "#66CCFF";
    const char LAYER_COLOR_DURATION[]          = "#9999FF";
    const char ROW_COLOR_DURATION_OFF1[]       = "#3388B3"; // duration off ancestors' bounds color1
    const char ROW_COLOR_DURATION_OFF2[]       = "#222222"; // duration off ancestors' bounds color2
    const char ROW_COLOR_DURATION_EDGE[]       = "#000000"; // duration left and right edge lines
    const char ROW_COLOR_DURATION_SELECTED[]   = "#4D99CC";
    const char LAYER_COLOR_DURATION_SELECTED[] = "#7D7DD1";
    const char ROW_COLOR_MOVE_SRC[]            = "#464600";
    const char ROW_TEXT_COLOR[]                = "#bbbbbb";
    const char ROW_TEXT_COLOR_DISABLED[]       = "#888888";
    const char PLAYHEAD_COLOR[]                = "#ff0066";
    const char RULER_COLOR[]                   = "#888888";
    const char RULER_COLOR_DISABLED[]          = "#444444";
    const char ROW_MOVER_COLOR[]               = "#ffff00";
    const char WIDGET_BG_COLOR[]               = "#222222";
    const char PLAYHEAD_LINE_COLOR[]           = "#b20808";
    const char FILTER_BUTTON_SELECTED_COLOR[]  = "#000000";

    // Other
    const int EXPAND_ANIMATION_DURATION       = 200;

    // TODO: move the colors (and maybe dimensions) to StudioPreferences.
}

#endif // TIMELINECONSTANTS_H
