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
#include "Strings.h"
#include "StringLoader.h"

//==============================================================================
//	Includes
//==============================================================================

#include "ToggleToolbar.h"
#include "SIcon.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "TimelineTreeLayout.h"

//=============================================================================
/**
 * Constructor
 */
CToggleToolbar::CToggleToolbar(CTimelineTreeLayout *inTreeLayout)
    : CFlowLayout(new CBlankControl(CStudioPreferences::GetTopRowColor()))
{
    m_TreeLayout = inTreeLayout;

    m_Color = CStudioPreferences::GetBaseColor();

    SetFlowDirection(FLOW_HORIZONTAL);
    SetAlignment(ALIGN_TOP, ALIGN_LEFT);
    SetLeftMargin(1);

    CProceduralButton<CToggleButton>::SBorderOptions theBorderOptions(false, true, true, true);

    m_FltrShyBtn = new CProceduralButton<CToggleButton>();
    m_FltrShyBtn->SetUpImage("Toggle-Shy.png");
    m_FltrShyBtn->SetDownImage("Toggle-Shy.png");
    m_FltrShyBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrShyBtn->SetAbsoluteSize(m_FltrShyBtn->GetSize());
    m_FltrShyBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_SHY2));
    QObject::connect(m_FltrShyBtn,&CToggleButton::SigToggle,
                     std::bind(&CToggleToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    AddChild(m_FltrShyBtn);
    m_FltrShyBtn->SetToggleState(false);

    m_FltrVisibleBtn = new CProceduralButton<CToggleButton>();
    m_FltrVisibleBtn->SetUpImage("filter-toggle-eye-up.png");
    m_FltrVisibleBtn->SetDownImage("filter-toggle-eye-down.png");
    m_FltrVisibleBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrVisibleBtn->SetAbsoluteSize(m_FltrVisibleBtn->GetSize());
    m_FltrVisibleBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_VISIBLE2));
    QObject::connect(m_FltrVisibleBtn,&CToggleButton::SigToggle,
                     std::bind(&CToggleToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    AddChild(m_FltrVisibleBtn);
    m_FltrVisibleBtn->SetToggleState(false);

    m_FltrLockBtn = new CProceduralButton<CToggleButton>();
    m_FltrLockBtn->SetUpImage("Toggle-Lock.png");
    m_FltrLockBtn->SetDownImage("Toggle-Lock.png");
    m_FltrLockBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrLockBtn->SetAbsoluteSize(m_FltrLockBtn->GetSize());
    m_FltrLockBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_LOCK2));
    QObject::connect(m_FltrLockBtn,&CToggleButton::SigToggle,
                     std::bind(&CToggleToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    AddChild(m_FltrLockBtn);
    m_FltrLockBtn->SetToggleState(false);
}

//=============================================================================
/**
 * Destructor
 */
CToggleToolbar::~CToggleToolbar()
{
    delete m_FltrShyBtn;
    delete m_FltrVisibleBtn;
    delete m_FltrLockBtn;
}

//=============================================================================
/**
 * Fills in the background color and highlighting for this control.
 */
void CToggleToolbar::Draw(CRenderer *inRenderer)
{
    CRct theRect(0, 0, GetSize().x, GetSize().y - 1);
    inRenderer->FillSolidRect(theRect, m_Color);
    CFlowLayout::Draw(inRenderer);
    inRenderer->Draw3dRect(theRect, CStudioPreferences::GetButtonShadowColor(),
                           CStudioPreferences::GetButtonShadowColor());

    // Draw the highlight at the bottom
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y));
    inRenderer->PopPen();

    // Draw the one pixel on the end of the highlight that has to be dark
    // Apparently there's some problems trying to draw one pixel, so clip the excess
    inRenderer->PushClippingRect(QRect(0, 0, GetSize().x, GetSize().y));
    inRenderer->PushPen(CStudioPreferences::GetButtonShadowColor());
    inRenderer->MoveTo(CPt(theRect.size.x, theRect.size.y));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y));
    inRenderer->PopPen();
    inRenderer->PopClippingRect();
}

//=============================================================================
/**
 * Handles turning a filter on or off in response to a button being pressed.
 * @param inButton button that generated the event
 * @param inState new state of the button after being toggled
 */
void CToggleToolbar::OnButtonToggled(CToggleButton *inButton, CButtonControl::EButtonState inState)
{
    bool theFilterNeedsApplied = (inState == CButtonControl::EBUTTONSTATE_UP);

    if (inButton == m_FltrShyBtn)
        FilterShy(theFilterNeedsApplied);
    else if (inButton == m_FltrVisibleBtn)
        FilterVisible(theFilterNeedsApplied);
    else if (inButton == m_FltrLockBtn)
        FilterLocked(theFilterNeedsApplied);
}

//=============================================================================
/**
 * Turns filtering on and off for shy objects.
 * @param inFilter true to filter shy objects out of the timeline, false to show
 * shy objects in the timeline.
 */
void CToggleToolbar::FilterShy(bool inFilter)
{
    if (inFilter)
        m_FltrShyBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_SHY2));
    else
        m_FltrShyBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_SHY1));

    m_TreeLayout->GetFilter()->SetShy(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Turns filtering on and off for visible objects.
 * @param inFilter true to filter visible objects out of the timeline, false to show
 * visible objects in the timeline.
 */
void CToggleToolbar::FilterVisible(bool inFilter)
{
    if (inFilter)
        m_FltrVisibleBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_VISIBLE2));
    else
        m_FltrVisibleBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_VISIBLE1));

    m_TreeLayout->GetFilter()->SetVisible(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Turns filtering on and off for locked objects.
 * @param inFilter true to filter locked objects out of the timeline, false to show
 * locked objects in the timeline.
 */
void CToggleToolbar::FilterLocked(bool inFilter)
{
    if (inFilter)
        m_FltrLockBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_LOCK2));
    else
        m_FltrLockBtn->SetTooltipText(::LoadResourceString(IDS_FLTR_TOOLTIP_LOCK1));

    m_TreeLayout->GetFilter()->SetLocked(inFilter);
    m_TreeLayout->Filter();
}
