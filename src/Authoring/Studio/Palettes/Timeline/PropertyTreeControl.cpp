/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "PropertyTreeControl.h"
#include "PropertyRow.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"
#include "HotKeys.h"
#include "ResourceCache.h"
#include "BaseTimelineTreeControl.h"
#include "Bindings/ITimelineItemProperty.h"
#include "CoreUtils.h"

CPropertyTreeControl::CPropertyTreeControl(CPropertyRow *inPropRow)
    : m_Icon(CResourceCache::GetInstance()->GetBitmap("Objects-Property-Normal.png"),
             CResourceCache::GetInstance()->GetBitmap("Objects-Property-Disabled.png"))
{
    m_PropRow = inPropRow;

    CBaseStateRow *theParentRow = m_PropRow->GetParentRow();
    if (theParentRow) // property row typically should never exists on its own, but to be safe.
        m_BackgroundColor = m_PropRow->GetTimebarBackgroundColor();
    else
        m_BackgroundColor = CStudioPreferences::GetPropertyBackgroundColor();

    AddChild(&m_Icon);

    m_Text.SetData(inPropRow->GetProperty()->GetName());
    m_Text.SetAlignment(CTextEdit::LEFT);
    m_Text.SetReadOnly(true);

    AddChild(&m_Text);

    SetIndent(0);
}

CPropertyTreeControl::~CPropertyTreeControl()
{
}

void CPropertyTreeControl::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());

    inRenderer->FillSolidRect(theRect, m_BackgroundColor);

    // Draw the line at the bottom of this control
    CColor theShadowColor = CStudioPreferences::GetPropertyFloorColor();
    inRenderer->PushPen(theShadowColor);
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));
    inRenderer->PopPen();

    ::CColor theTextColor = CStudioPreferences::GetNormalColor();
    if (m_PropRow->GetProperty()->IsMaster())
        theTextColor = CStudioPreferences::GetMasterColor();
    m_Text.SetTextColor(theTextColor);
}

void CPropertyTreeControl::SetIndent(long inIndent)
{
    m_Indent = inIndent;

    // place it it's size x2 to the right since we don't have a toggle.
    m_Icon.SetPosition(CPt(m_Indent + m_Icon.GetSize().x, 0));

    m_Text.SetPosition(CPt(m_Icon.GetPosition().x + m_Icon.GetSize().x + 5, 0));
    m_Text.SetSize(CPt(200, m_Icon.GetSize().y - 1));
}

long CPropertyTreeControl::GetIndent()
{
    return m_Indent;
}

void CPropertyTreeControl::SetHighlighted(bool inIsHighlighted)
{
    CBaseStateRow *theParentRow = m_PropRow->GetParentRow();
    if (theParentRow) // property row typically should never exists on its own, but to be safe.
        m_BackgroundColor = (inIsHighlighted) ? m_PropRow->GetTimebarHighlightBackgroundColor()
                                              : m_PropRow->GetTimebarBackgroundColor();
    else
        m_BackgroundColor = (inIsHighlighted) ? CStudioPreferences::GetPropertyMouseOverColor()
                                              : CStudioPreferences::GetPropertyBackgroundColor();

    Invalidate();
}

//==============================================================================
/**
 * HACK: Trying to scroll the timeline during a drag and drop operation.
 *
 * The Property Tree Control will never have a drop candidate, so this function
 * returns nullptr.  However, we still need to scroll the timeline in case we are
 * at the top or bottom of the window.  This is hackish for two reasons.  1) It
 * points out that CPropertyTreeControl and CStateTreeControl should have a
 * common base class that handles similar code.  2) The TreeControls should not
 * have to scroll the timeline automatically.  It should be handled through a
 * timer on the scroller when the mouse is being dragged and hovering near the
 * border of the scroller.  But all of that will be saved for another day.
 * @param inMousePoint location of the mouse
 * @param inFlags not used (modifier key flags)
 * @return nullptr (no drop candidate is ever found)
 */
CDropTarget *CPropertyTreeControl::FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inFlags);

    CPt theSize(GetSize());

    // If the mouse is towards the top of this row, make sure that this row and the
    // one above it are visible (scroll upwards)
    if (inMousePoint.y <= ::dtol(theSize.y / 2.0f))
        EnsureVisible(CRct(CPt(0, -theSize.y), theSize));
    // Otherwise, the mouse is towards the bottom of this row, so make sure this row
    // and the one below it are visible (scroll downwards)
    else
        EnsureVisible(CRct(CPt(theSize.x, theSize.y * 2)));

    return nullptr;
}

void CPropertyTreeControl::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    m_PropRow->OnMouseOver();
}

void CPropertyTreeControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    m_PropRow->OnMouseOut();
}

//==============================================================================
/**
 *	Handles the OnMouseDownEvent
 */
bool CPropertyTreeControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags))
        m_PropRow->Select((CHotKeys::MODIFIER_SHIFT & inFlags) == CHotKeys::MODIFIER_SHIFT);

    return true;
}

bool CPropertyTreeControl::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDoubleClick(inPoint, inFlags)) {
        m_PropRow->OnMouseDoubleClick();
    }
    return true;
}
