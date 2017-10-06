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

#include "BaseTimebarlessRow.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "BaseStateRow.h"

CBaseTimebarlessRow::CBaseTimebarlessRow()
    : m_Selected(false)
    , m_DirtyFlag(true)
    , m_TimeRatio(0.0f)
{
}

CBaseTimebarlessRow::~CBaseTimebarlessRow()
{
}

void CBaseTimebarlessRow::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());

    // Fill in the background
    if (!m_Selected)
        inRenderer->FillSolidRect(theRect, m_BackgroundColor);
    else
        inRenderer->FillSolidRect(theRect, CStudioPreferences::GetTimelineSelectColor());

    // Draw the line at the bottom of this control and the one on the side
    inRenderer->PushPen(CStudioPreferences::GetPropertyFloorColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));
    inRenderer->PopPen();
}

//=============================================================================
/**
 * Set this control to being highlighted or not.
 * @param inIsHighlighted true if this is to be highlighted.
 */
void CBaseTimebarlessRow::SetBackgroundColor(::CColor inBackgroundColor)
{
    if (m_BackgroundColor != inBackgroundColor) {
        m_BackgroundColor = inBackgroundColor;
        Invalidate();
    }
}

void CBaseTimebarlessRow::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
}

//=============================================================================
/**
 * Notification that the object that this row is representing has been selected.
 */
void CBaseTimebarlessRow::OnSelect()
{
    m_Selected = true;

    Invalidate();
}

//=============================================================================
/**
 * Notification that the object that this row is representing has been deselected.
 */
void CBaseTimebarlessRow::OnDeselect()
{
    m_Selected = false;

    Invalidate();
}

//=============================================================================
/**
 * called when meta data for this row is changed... should be overridden by the
 * timebar row
 */
void CBaseTimebarlessRow::RefreshRowMetaData()
{
}

//=============================================================================
/**
 * called when a child changes and the keyframes need to be refreshed
 * @param inDirtyFlag true if this object is now dirty
 */
void CBaseTimebarlessRow::SetDirty(bool inDirtyFlag)
{
    if (m_DirtyFlag == inDirtyFlag)
        return;

    m_DirtyFlag = inDirtyFlag;
    Invalidate();
}

void CBaseTimebarlessRow::UpdateTime(long inStartTime, long inEndTime)
{
    Q_UNUSED(inStartTime);
    Q_UNUSED(inEndTime);
}

//=============================================================================
/**
 * OnMouseOver event, handles the highlighting of the row.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
void CBaseTimebarlessRow::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    GetBaseStateRow()->OnMouseOver();
}

//=============================================================================
/**
 * OnMouseOut event, handles the de-highlighting of this row.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
void CBaseTimebarlessRow::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    GetBaseStateRow()->OnMouseOut();
}

//=============================================================================
/**
 * OnMouseDown event, handles the selecting of this object.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
bool CBaseTimebarlessRow::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    return CControl::OnMouseDown(inPoint, inFlags);
#if 0
	// this addition is causing 4085: Cannot do rubber band selection from sections of timeline that don't contain timebars anymore
	bool theReturn = CControl::OnMouseDown( inPoint, inFlags );
	if ( !theReturn )
	{
		// Tests if the user has pressed the modifier key, where the intention is to multi-select keyframes.
		if ( !(inFlags & CHotKeys::MODIFIER_CONTROL ) )
		{	
			// SK - I changed this to select the row when this is clicked, because I think its a nice feature. ie don't always have to click on the timebar (esp for those e.g. scene without one)
			// when the modifier key is pressed.
			GetBaseStateRow( )->Select( false );

			theReturn = true;
		}
	}
	return theReturn;
#endif
}
