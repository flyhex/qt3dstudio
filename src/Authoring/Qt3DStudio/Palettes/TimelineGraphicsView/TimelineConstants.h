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
    const int ROW_GRAPH_H       = 120;   // property row height when graph is shown
    const int ROW_GRAPH_H_MAX   = 300;   // property row height when graph is shown maximized
    const int ROW_SPACING       = 2;
    const int ROW_DEPTH_STEP    = 15;    // x-distance between 2 consecutive depths
    const double RULER_SEC_W    = 60;    // width of 1 second section (at scale 1)
    const double RULER_MILLI_W  = RULER_SEC_W / 1000.0; // width of 1 millisecond section at scale 1
    const int RULER_SEC_DIV     = 10;    // second divisions
    const int RULER_DIV_H1      = 5;     // height of main divisions
    const int RULER_DIV_H2      = 3;     // height of secondary divisions
    const int RULER_DIV_H3      = 1;     // height of minor divisions
    const int RULER_BASE_Y      = 18;    // baseline Y
    const int RULER_TICK_SCALE1 = 2;
    const int RULER_TICK_SCALE2 = 3;
    const int RULER_TICK_SCALE3 = 6;
    const int RULER_TICK_SCALE4 = 21;
    const int TOOLBAR_MARGIN    = 10;    // margin between the timeline and the toolbar
    const int ROW_TEXT_OFFSET_Y = 3;     // offset Y of comment text on row

    const double RULER_EDGE_OFFSET = 15;
    const double TREE_MIN_W     = 160;
    const double TREE_MAX_W     = 600;
    const double TREE_DEFAULT_W = 250;
    const double TREE_BOUND_W   = 10000; // real width of the row (> max possible visible tree area)
    const double TREE_ICONS_W   = 53;
    const int SPLITTER_W        = 4;
    const int PLAYHEAD_W        = 14;
    const int DURATION_HANDLE_W = 14;    // width of duration end handles in a timeline row
    const int NAVIGATION_BAR_H  = 30;    // height of navigation/breadcrumb bar
    const int TIMEBAR_TOOLTIP_OFFSET_V = 10;

    // Other
    const int EXPAND_ANIMATION_DURATION = 200;
    const int AUTO_SCROLL_PERIOD        = 50;  // time steps (millis) while auto scrolling
    const int AUTO_SCROLL_DELTA         = 8;   // increment in scroll at each time step
    const int AUTO_SCROLL_TRIGGER       = 500; // time after which auto scroll starts (millis)
    const int AUTO_EXPAND_TIME          = 500; // auto expand a hovered row (while DnD-ing)
    const long MAX_SLIDE_TIME           = 3599000; // milliseconds

    const int TIMELINE_SCROLL_MAX_DELTA = 25; // Maximum amount of pixels to scroll per frame
    const int TIMELINE_SCROLL_DIVISOR = 6;    // Divisor for timeline autoscroll distance
}

#endif // TIMELINECONSTANTS_H
