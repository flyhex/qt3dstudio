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

#include "BlankToggleControl.h"
#include "Renderer.h"
#include "BaseStateRowUI.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "BaseStateRow.h"

CBlankToggleControl::CBlankToggleControl(CBaseStateRowUI *inBaseStateRow)
    : m_StateRowUI(inBaseStateRow)
    , m_Selected(false)
{
    m_StateRow = static_cast<CBaseStateRow *>(m_StateRowUI->GetTimelineRow());
    m_BackgroundColor = m_StateRow->GetTimebarBackgroundColor(m_StateRow->GetObjectType());
}

CBlankToggleControl::~CBlankToggleControl()
{
}

//==============================================================================
/**
 * Handles the drawing fo rthe toggle control
 */
void CBlankToggleControl::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());

    // Fill in the background
    if (!m_Selected)
        inRenderer->FillSolidRect(theRect, m_BackgroundColor);
    else
        inRenderer->FillSolidRect(theRect, CStudioPreferences::GetTimelineSelectColor());

    // Draw the line at the bottom of this control
    inRenderer->PushPen(CStudioPreferences::GetTreeFloorColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));
    inRenderer->PopPen();

    // Draw the line on the left side of this control
    inRenderer->PushPen(CStudioPreferences::GetPropertyFloorColor());
    inRenderer->MoveTo(CPt(0, 0));
    inRenderer->LineTo(CPt(0, theRect.size.y - 1));
    inRenderer->PopPen();

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

//=============================================================================
/**
 * Notification that the object that this row is representing has been selected.
 */
void CBlankToggleControl::OnSelect()
{
    m_Selected = true;

    Invalidate();
}

//=============================================================================
/**
 * Notification that the object that this row is representing has been deselected.
 */
void CBlankToggleControl::OnDeselect()
{
    m_Selected = false;

    Invalidate();
}

//==============================================================================
/**
 * Handler for the OnMouseDown event
 *
 * @param inPoint the point where this event takes place
 * @param inFlags the state when this event takes place.
 */
bool CBlankToggleControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_StateRow->Select(Qt::NoModifier);
    }
    return true;
}

//==============================================================================
/**
 * Sets the background color of this toggle control
 */
void CBlankToggleControl::SetBackgroundColor(::CColor inBackgroundColor)
{
    if (m_BackgroundColor == inBackgroundColor)
        return;

    m_BackgroundColor = inBackgroundColor;

    Invalidate();
}

//==============================================================================
/**
 * Handler for the OnMouseOver event
 *
 * @param inPoint the point where this event takes place
 * @param inFlags the state when this event takes place.
 */
void CBlankToggleControl::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    m_StateRowUI->OnMouseOver();
}

//==============================================================================
/**
 * Handler for the OnMouseOut event
 *
 * @param inPoint the point where this event takes place
 * @param inFlags the state when this event takes place.
 */
void CBlankToggleControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    m_StateRowUI->OnMouseOut();
}

void CBlankToggleControl::Refresh()
{
}

CBaseStateRow *CBlankToggleControl::baseStateRow() const
{
    return m_StateRow;
}
