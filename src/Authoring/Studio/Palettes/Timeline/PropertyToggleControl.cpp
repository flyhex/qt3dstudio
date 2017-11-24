/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "stdafx.h"

#include "PropertyToggleControl.h"
#include "TimelineRow.h"
#include "PropertyRow.h"
#include "PropertyRowUI.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "HotKeys.h"

CPropertyToggleControl::CPropertyToggleControl(CPropertyRowUI *inPropertyRow)
    : m_PropertyRowUI(inPropertyRow)
{
    auto propertyRow = static_cast<CPropertyRow *>(m_PropertyRowUI->GetTimelineRow());
    m_BackgroundColor = propertyRow->GetTimebarBackgroundColor();
}

CPropertyToggleControl::~CPropertyToggleControl()
{
}

void CPropertyToggleControl::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());
    inRenderer->FillSolidRect(QRect(0, 0, theRect.size.x + 1, theRect.size.y), m_BackgroundColor);

    // Draw the line at the bottom of this control
    inRenderer->PushPen(CStudioPreferences::GetPropertyFloorColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));

    // Draw the line on the left side of this control
    inRenderer->MoveTo(CPt(0, 0));
    inRenderer->LineTo(CPt(0, theRect.size.y - 1));

    // Draw the highlight
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    inRenderer->MoveTo(CPt(1, 0));
    inRenderer->LineTo(CPt(1, theRect.size.y - 1));
    inRenderer->PopPen();

    // Draw the line on the right side of this control
    inRenderer->PushPen(CStudioPreferences::GetButtonShadowColor());
    inRenderer->MoveTo(CPt(theRect.size.x - 1, 0));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y - 1));
    inRenderer->PopPen();
}

void CPropertyToggleControl::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    m_PropertyRowUI->OnMouseOver();
}

void CPropertyToggleControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    m_PropertyRowUI->OnMouseOut();
}

void CPropertyToggleControl::SetHighlighted(bool inIsHighlighted)
{
    auto propertyRow = static_cast<CPropertyRow *>(m_PropertyRowUI->GetTimelineRow());
    if (inIsHighlighted)
        m_BackgroundColor = propertyRow->GetTimebarHighlightBackgroundColor();
    else
        m_BackgroundColor = propertyRow->GetTimebarBackgroundColor();

    Invalidate();
}

//==============================================================================
/**
 *	Handles the OnMouseDownEvent
 */
bool CPropertyToggleControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        auto propertyRow = static_cast<CPropertyRow *>(m_PropertyRowUI->GetTimelineRow());
        propertyRow->Select((CHotKeys::MODIFIER_SHIFT & inFlags) == CHotKeys::MODIFIER_SHIFT);
    }

    return true;
}
