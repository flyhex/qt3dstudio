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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "CColor.h"
#include "SystemPreferences.h"

#include <QApplication>
#include <QPalette>

//=============================================================================
/**
 * Destructor
 */
CSystemPreferences::~CSystemPreferences()
{
}

//=============================================================================
/**
 * Retrieves the system preferred background color.  Example use would be for
 * the fill color of a button.
 */
QColor CSystemPreferences::GetSystemBackgroundColor()
{
    const auto palette = QApplication::palette();
    return palette.color(QPalette::Active, QPalette::Button);
}

//=============================================================================
/**
 * Returns the color used by the system to indicate that an item is selected.
 */
QColor CSystemPreferences::GetSelectedItemColor()
{
    const auto palette = QApplication::palette();
    return palette.color(QPalette::Active, QPalette::Highlight);
}

//=============================================================================
/**
 * Returns the color of selected text to be displayed on top of GetSelectedItemColor().
 */
QColor CSystemPreferences::GetSelectedTextColor()
{
    const auto palette = QApplication::palette();
    return palette.color(QPalette::Active, QPalette::HighlightedText);
}

//=============================================================================
/**
 * Supposed to get whether or not anti aliasing should be enabled for a specified
 * font size, but the Theme does not seem to include any data on it. If the
 * theme starts working in a later release of the OS then this should work.
 */
bool CSystemPreferences::IsFontAntiAliasing(float inFontSize)
{
    Q_UNUSED(inFontSize);
    return false;
}

//=============================================================================
/**
 * Scroll bars contain two arrows (up/down or left/right).  These can be located
 * on either end of the scroll bar, or located next to each other at one end.  On
 * Mac OS X, this is a system preference.
 * @return true if the scroll bar arrows should be next to each other, at one end of the scroll bar
 */
bool CSystemPreferences::AreScrollArrowsAdjacent()
{
    return false;
}
