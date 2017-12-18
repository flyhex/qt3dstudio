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
#include "FilterToolbar.h"
#include "ButtonControl.h"
#include "SystemPreferences.h"
#include "Renderer.h"
#include "TimelineTreeLayout.h"
#include "StudioPreferences.h"

//=============================================================================
/**
 * Constructor
 */
CFilterToolbar::CFilterToolbar(CTimelineTreeLayout *inTreeLayout)
    : CFlowLayout(nullptr, false)
{
    m_TreeLayout = inTreeLayout;

    SetFlowDirection(FLOW_HORIZONTAL);
    SetAlignment(ALIGN_TOP, ALIGN_LEFT);
    SetLeftMargin(1);

    // Create the buttons
    m_FltrBehaviorsBtn = new CProceduralButton<CToggleButton>();
    m_FltrPropertiesBtn = new CProceduralButton<CToggleButton>();
    m_FltrMaterialsBtn = new CProceduralButton<CToggleButton>();
    m_FltrShyBtn = new CProceduralButton<CToggleButton>();
    m_FltrVisibleBtn = new CProceduralButton<CToggleButton>();

    // Load the bitmaps
    m_FltrBehaviorsBtn->SetUpImage("obsolete_placeholder.png");
    m_FltrBehaviorsBtn->SetDownImage("obsolete_placeholder.png");

    m_FltrPropertiesBtn->SetUpImage("obsolete_placeholder.png");
    m_FltrPropertiesBtn->SetDownImage("obsolete_placeholder.png");

    m_FltrMaterialsBtn->SetUpImage("obsolete_placeholder.png");
    m_FltrMaterialsBtn->SetDownImage("obsolete_placeholder.png");

    m_FltrShyBtn->SetUpImage("Toggle-Shy.png");
    m_FltrShyBtn->SetDownImage("Toggle-Shy.png");

    m_FltrVisibleBtn->SetUpImage("Toggle-HideShow.png");
    m_FltrVisibleBtn->SetDownImage("Toggle-HideShow.png");

    // Turn off the left border of each button since they are all next to each other, otherwise,
    // you'll get a double line effect
    CProceduralButton<CToggleButton>::SBorderOptions theBorderOptions(false, true, true, true);
    m_FltrBehaviorsBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrPropertiesBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrMaterialsBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrShyBtn->SetBorderVisibilityAll(theBorderOptions);
    m_FltrVisibleBtn->SetBorderVisibilityAll(theBorderOptions);

    // Set the max sizes for the buttons
    m_FltrBehaviorsBtn->SetAbsoluteSize(m_FltrBehaviorsBtn->GetSize());
    m_FltrPropertiesBtn->SetAbsoluteSize(m_FltrPropertiesBtn->GetSize());
    m_FltrMaterialsBtn->SetAbsoluteSize(m_FltrMaterialsBtn->GetSize());
    m_FltrShyBtn->SetAbsoluteSize(m_FltrShyBtn->GetSize());
    m_FltrVisibleBtn->SetAbsoluteSize(m_FltrShyBtn->GetSize());

    // Tooltips
    m_FltrBehaviorsBtn->SetTooltipText(QObject::tr("Hide behaviors"));
    m_FltrPropertiesBtn->SetTooltipText(QObject::tr("Hide properties"));
    m_FltrMaterialsBtn->SetTooltipText(QObject::tr("Hide materials"));
    m_FltrShyBtn->SetTooltipText(QObject::tr("Hide shy objects"));
    m_FltrVisibleBtn->SetTooltipText(QObject::tr("Hide inactive objects"));

    // Callback for one of the Filter buttons being clicked on
    QObject::connect(m_FltrBehaviorsBtn,&CToggleButton::SigToggle,
                     std::bind(&CFilterToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    QObject::connect(m_FltrPropertiesBtn,&CToggleButton::SigToggle,
                     std::bind(&CFilterToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    QObject::connect(m_FltrMaterialsBtn,&CToggleButton::SigToggle,
                     std::bind(&CFilterToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    QObject::connect(m_FltrShyBtn,&CToggleButton::SigToggle,
                     std::bind(&CFilterToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    QObject::connect(m_FltrVisibleBtn,&CToggleButton::SigToggle,
                     std::bind(&CFilterToolbar::OnButtonToggled, this,
                               std::placeholders::_1, std::placeholders::_2));
    // Add the buttons to this layout
    AddChild(m_FltrMaterialsBtn);
    AddChild(m_FltrPropertiesBtn);
    AddChild(m_FltrBehaviorsBtn);
    AddChild(m_FltrShyBtn);
    AddChild(m_FltrVisibleBtn);

    m_FltrBehaviorsBtn->SetToggleState(false);
    m_FltrPropertiesBtn->SetToggleState(false);
    m_FltrMaterialsBtn->SetToggleState(false);
    m_FltrShyBtn->SetToggleState(false);
    m_FltrVisibleBtn->SetToggleState(false);
}

//=============================================================================
/**
 * Destructor
 */
CFilterToolbar::~CFilterToolbar()
{
    delete m_FltrBehaviorsBtn;
    delete m_FltrPropertiesBtn;
    delete m_FltrMaterialsBtn;
    delete m_FltrShyBtn;
    delete m_FltrVisibleBtn;
}

//=============================================================================
/**
 * Overriden to draw some highlighting.
 */
void CFilterToolbar::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());
    // Draw the highlight at the bottom
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));
    inRenderer->PopPen();

    // Draw the line on the left side
    inRenderer->PushPen(CStudioPreferences::GetButtonShadowColor());
    inRenderer->MoveTo(0, 0);
    inRenderer->LineTo(0, theRect.size.y - 1);
    inRenderer->PopPen();
}

//=============================================================================
/**
 * Turns filtering on and off for behavior objects in the timeline.
 * @param inFilter true to filter behaviors out of the timeline, false to show
 * behaviors in the timeline.
 */
void CFilterToolbar::FilterBehaviors(bool inFilter)
{
    if (inFilter)
        m_FltrBehaviorsBtn->SetTooltipText(QObject::tr("Show behaviors"));
    else
        m_FltrBehaviorsBtn->SetTooltipText(QObject::tr("Hide behaviors"));

    m_TreeLayout->GetFilter()->SetBehaviors(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Turns filtering on and off for properties on objects in the timeline.
 * @param inFilter true to filter properties out of the timeline, false to show
 * properties in the timeline.
 */
void CFilterToolbar::FilterProperties(bool inFilter)
{
    if (inFilter)
        m_FltrPropertiesBtn->SetTooltipText(QObject::tr("Show properties"));
    else
        m_FltrPropertiesBtn->SetTooltipText(QObject::tr("Hide properties"));

    m_TreeLayout->GetFilter()->SetProperties(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Turns filtering on and off for material objects.
 * @param inFilter true to filter material objects out of the timeline, false to show
 * material objects in the timeline.
 */
void CFilterToolbar::FilterMaterials(bool inFilter)
{
    if (inFilter)
        m_FltrMaterialsBtn->SetTooltipText(QObject::tr("Show materials"));
    else
        m_FltrMaterialsBtn->SetTooltipText(QObject::tr("Hide materials"));

    m_TreeLayout->GetFilter()->SetMaterials(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Turns filtering on and off for shy objects.
 * @param inFilter true to filter shy objects out of the timeline, false to show
 * shy objects in the timeline.
 */
void CFilterToolbar::FilterShy(bool inFilter)
{
    if (inFilter)
        m_FltrShyBtn->SetTooltipText(QObject::tr("Show shy objects"));
    else
        m_FltrShyBtn->SetTooltipText(QObject::tr("Hide shy objects"));

    m_TreeLayout->GetFilter()->SetShy(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Turns filtering on and off for visible objects.
 * @param inFilter true to filter visible objects out of the timeline, false to show
 * shy objects in the timeline.
 */
void CFilterToolbar::FilterVisible(bool inFilter)
{
    if (inFilter)
        m_FltrVisibleBtn->SetTooltipText(QObject::tr("Show inactive objects"));
    else
        m_FltrVisibleBtn->SetTooltipText(QObject::tr("Hide inactive objects"));

    m_TreeLayout->GetFilter()->SetVisible(inFilter);
    m_TreeLayout->Filter();
}

//=============================================================================
/**
 * Handles turning a filter on or off in response to a button being pressed.
 * @param inButton button that generated the event
 * @param inState new state of the button after being toggled
 */
void CFilterToolbar::OnButtonToggled(CToggleButton *inButton, CButtonControl::EButtonState inState)
{
    bool theFilterNeedsApplied = (inState == CButtonControl::EBUTTONSTATE_UP);

    if (inButton == m_FltrBehaviorsBtn)
        FilterBehaviors(theFilterNeedsApplied);
    else if (inButton == m_FltrPropertiesBtn)
        FilterProperties(theFilterNeedsApplied);
    else if (inButton == m_FltrMaterialsBtn)
        FilterMaterials(theFilterNeedsApplied);
    else if (inButton == m_FltrShyBtn)
        FilterShy(theFilterNeedsApplied);
    else if (inButton == m_FltrVisibleBtn)
        FilterVisible(theFilterNeedsApplied);
}
