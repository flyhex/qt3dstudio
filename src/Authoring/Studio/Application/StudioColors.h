/****************************************************************************
**
** Copyright (C) 2002 Anark Corporation.
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

#ifndef INCLUDED_STUDIOCOLORS_H
#define INCLUDED_STUDIOCOLORS_H

#pragma once

//==============================================================================
//	Studio colors
//==============================================================================

const COLORREF STUDIO_MATTE_COLOR = RGB(119, 122, 125); // GetSysColor( COLOR_APPWORKSPACE );
const COLORREF MENU_IMAGELIST_BACKCOLOR = RGB(255, 0, 255); // RGB( 255, 128, 0 );

//==============================================================================
//	TimeBar colors
//==============================================================================

// TimeBar pen colors

const COLORREF TIMEBAR_PEN_COLOR_INNER = RGB(254, 254, 254);
const COLORREF TIMEBAR_PEN_COLOR_INACTIVE = RGB(132, 130, 132);
const COLORREF TIMEBAR_PEN_COLOR_ACTIVE = RGB(0, 0, 0);

// TimeBar shadow colors

const COLORREF TIMEBAR_INACTIVE_SHADOW_COLOR = RGB(0xC4, 0xC4, 0xC4);
const COLORREF TIMEBAR_ACTIVE_SHADOW_COLOR = RGB(0x84, 0x84, 0x84);

// TimeBar comment colors

const COLORREF TIMEBAR_COMMENT_ACTIVE_TC = RGB(0x00, 0x00, 0x00);
const COLORREF TIMEBAR_COMMENT_INACTIVE_TC = RGB(0x72, 0x72, 0x72);

const COLORREF COLOR_TIMEBAR_DISABLED = RGB(207, 207, 207);
const COLORREF COLOR_TIMEBAR_BEHAVIOR =
    RGB(148, 189, 150); // RGB( 92, 164, 131 );//RGB( 243, 243, 243 );
const COLORREF COLOR_TIMEBAR_MATERIAL =
    RGB(194, 155, 181); // RGB( 190, 184, 152 );	//RGB( 239, 230, 210 );
const COLORREF COLOR_TIMEBAR_IMAGE =
    RGB(194, 155, 181); // RGB( 190, 184, 152 );	//RGB( 198, 225, 199 );
const COLORREF COLOR_TIMEBAR_LOCKED = RGB(228, 228, 228);

const COLORREF TIMEBAR_COLOR_TRANSPARENT = RGB(255, 0, 255);

// A constant to represent color for inner white rectangle of center bar
const COLORREF COLOR_INNER_BAR_RECTANGLE = RGB(253, 253, 254);
// Default timebar fill color
const COLORREF COLOR_TIMEBAR_DEFAULT_FILL = RGB(
    163, 178, 227); // RGB( 174, 183, 210 );	//RGB( 148, 166, 189 );	//RGB( 211, 226, 240 );

// Selected time bar colors

const COLORREF COLOR_TIMEBAR_SELECTED = RGB(75, 75, 75); // RGB( 14, 49, 98 );
const COLORREF COLOR_TIMEBAR_SELECTED_TEXT = RGB(255, 255, 255);
const COLORREF COLOR_TIMEBAR_SELECTED_BEHAVIOR = RGB(174, 216, 176);
const COLORREF COLOR_TIMEBAR_SELECTED_HIGHLIGHT = RGB(27, 73, 136);

// const	COLORREF	COLOR_SELECTED_TIMEBAR_3DOBJECT		=	RGB( 190, 191, 202
// );

const COLORREF COLOR_TIMEBAR_HIGHLIGHT = RGB(190, 191, 202);
const COLORREF COLOR_TIMEBAR_HIGHLIGHT_GROUP = RGB(174, 216, 176);

const COLORREF COLOR_PLAYHEADSCRUBTIME = RGB(0, 85, 0);

//==============================================================================
//	Timeline colors
//==============================================================================

const BOOL INVERT_TIMELINE_TEXT_COLOR = TRUE;

const COLORREF COLOR_SELECTEDTEXT = RGB(255, 255, 255);
const COLORREF COLOR_SELECTEDTEXTBKGRND = RGB(14, 49, 98);
const COLORREF COLOR_DRAGDROPTARGETTEXT = RGB(255, 255, 255);
const COLORREF COLOR_DRAGDROPTARGETBKGRND = RGB(24, 98, 198);

const COLORREF COLOR_SHADOW = RGB(165, 162, 161); // RGB( 132, 130, 132 );

const COLORREF COLOR_PLAYHEADLINE = RGB(0xDD, 0x04, 0x05);
const COLORREF COLOR_BACKGRND =
    GetSysColor(COLOR_3DFACE); // RGB( 206, 203, 198 );		//RGB( 0xE7, 0xE7, 0xE7 );
const COLORREF COLOR_SELECTED = RGB(255, 255, 255);
const COLORREF COLOR_DEFAULTOBJECTS = RGB(206, 203, 198); // RGB( 0xE7, 0xE7, 0xE7 );
const COLORREF COLOR_3DOBJECTS = RGB(206, 203, 198); // RGB( 0xC7, 0xD4, 0xE1 );
const COLORREF COLOR_DEFAULTOBJECTHIGHLIGHT = RGB(222, 223, 222); // RGB( 255, 255, 255 );
const COLORREF COLOR_DRAGHIGHLIGHT = RGB(255, 255, 255);
const COLORREF COLOR_INSERTMARK = RGB(0x00, 0x00, 0x00);
const COLORREF COLOR_TIMELINEINSERTMARK = RGB(0x00, 0x00, 0xFF);
const COLORREF COLOR_TEXTCOLOR = RGB(0x00, 0x00, 0x00);
const COLORREF COLOR_CONTAINERTEXTCOLOR = RGB(80, 80, 140);
const COLORREF COLOR_LOCKEDTEXTCOLOR = RGB(0x80, 0x80, 0x80);
const COLORREF COLOR_INDEPENDENTOBJECT = RGB(0x80, 0x80, 0x80);
const COLORREF COLOR_SPLITLINE = RGB(0x00, 0x00, 0x7F);
const COLORREF COLOR_SEPARATORLINE = RGB(0x00, 0x00, 0x20);
const COLORREF COLOR_LIGHTMODEL = RGB(206, 203, 198); // RGB( 198, 211, 224 );
const COLORREF COLOR_GROUP =
    RGB(206, 203, 198); // RGB( 198, 211, 224 );		//RGB( 199, 220, 193 );
const COLORREF COLOR_PROPERTY = RGB(228, 225, 219); // RGB( 241, 241, 241 );
const COLORREF COLOR_CHANNEL = RGB(228, 225, 219); // RGB( 241, 241, 241 );
const COLORREF COLOR_CONTAINER =
    RGB(228, 225, 219); // RGB( 206, 203, 198 );		//RGB( 231, 231, 231 );
const COLORREF COLOR_MATERIAL = RGB(210, 210, 210); // RGB( 170, 224, 170 );
const COLORREF COLOR_CONTAINERHIGHLIGHT = RGB(193, 180, 180);
const COLORREF COLOR_BEHAVIOR = RGB(199, 220, 193);
const COLORREF COLOR_BEHAVIORHIGHLIGHT = RGB(255, 255, 255);
const COLORREF COLOR_IMGLISTTRANSPARENT = RGB(0xFF, 0x00, 0xFF);
const COLORREF COLOR_TIMEMEASURE = GetSysColor(COLOR_3DFACE); // RGB( 170, 166, 160 );

#endif
