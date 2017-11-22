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

#include "ToggleControl.h"
#include "Renderer.h"
#include "StateRow.h"
#include "BlankControl.h"
#include "HotKeys.h"
#include "Bindings/ITimelineItemBinding.h"

CToggleControl::CToggleControl(CStateRow *inStateRow, ITimelineItemBinding *inTimelineItemBinding)
    : CBlankToggleControl(inStateRow)
    , m_TimelineItemBinding(inTimelineItemBinding)
{
    m_StateRow = inStateRow;
    long theLeftOffset = 4;

    m_Shy = new CToggleButton();
    m_Shy->SetName("ShyToggle");
    m_Shy->SetUpImage("Toggle-Empty.png");
    m_Shy->SetDownImage("Toggle-Shy.png");
    m_Shy->SetPosition(CPt(theLeftOffset, 0));
    CPt theShySize = m_Shy->GetSize();

    m_Visible = new CToggleButton();
    m_Visible->SetName("VisibilityToggle");
    m_Visible->SetUpImage("Toggle-Empty.png");
    m_Visible->SetDownImage("Toggle-HideShow.png");
    m_Visible->SetUpDisabledImage("Toggle-Empty.png"); // show empty if disabled
    m_Visible->SetDisabledImage("Toggle-HideShow-disabled.png");
    m_Visible->SetPosition(CPt(theShySize.x + 7, 0));
    CPt theVisibleSize = m_Visible->GetSize();

    m_Locked = new CToggleButton();
    m_Locked->SetName("LockToggle");
    m_Locked->SetUpImage("Toggle-Empty.png");
    m_Locked->SetDownImage("Toggle-Lock.png");
    m_Locked->SetPosition(CPt(theVisibleSize.x + theShySize.x + 10, 0));
    CPt theLockedSize = m_Locked->GetSize();

    AddChild(m_Shy);
    AddChild(m_Visible);
    AddChild(m_Locked);

    SetAbsoluteSize(CPt(theShySize.x + theVisibleSize.x + theLockedSize.x + 1, theShySize.y));

    // Button down listeners
    QObject::connect(m_Shy,&CToggleButton::SigToggle,
                     std::bind(&CToggleControl::OnShyClicked, this,
                               std::placeholders::_1, std::placeholders::_2));
    QObject::connect(m_Visible,&CToggleButton::SigToggle,
                     std::bind(&CToggleControl::OnVisibleClicked, this,
                               std::placeholders::_1, std::placeholders::_2));
    QObject::connect(m_Locked,&CToggleButton::SigToggle,
                     std::bind(&CToggleControl::OnLockClicked, this,
                               std::placeholders::_1, std::placeholders::_2));



    ITimelineItem *theTimelineItem = m_TimelineItemBinding->GetTimelineItem();
    // Initial toggle state of the eye visibility button
    // Note GetViewToggleOff==FALSE means visible
    m_Visible->SetToggleState(theTimelineItem->IsVisible());

    // Initial toggle state of the button
    m_Locked->SetToggleState(theTimelineItem->IsLocked());

    // Initial toggle state of the button
    m_Shy->SetToggleState(theTimelineItem->IsShy());

    // sk - if anything, this should be checked in the data model level, not here.
    // The tri-state (on, off, dim) of visibility depends on two bools:
    // GetViewToggleOff and IsEnabled. Ensure that any bogus 4th permutation
    // is detected ASAP
    //	ASSERT( !inStateRow->GetState( )->GetViewToggleOff( ) ||
    //		    !inStateRow->GetState( )->IsEnabled( ) );
}

CToggleControl::~CToggleControl()
{
    delete m_Shy;
    delete m_Visible;
    delete m_Locked;
}

//==============================================================================
/**
 * Handles clicking on the Shy button.  Toggles the shy state on the asset.  Shy
 * objects can be filtered out of the timeline.
 */
void CToggleControl::OnShyClicked(CToggleButton *, CToggleButton::EButtonState inButtonState)
{
    m_TimelineItemBinding->GetTimelineItem()->SetShy(inButtonState
                                                     == CToggleButton::EBUTTONSTATE_DOWN);

    m_StateRow->Filter(*m_StateRow->GetFilter());
    CTimelineRow *theParentRow = m_StateRow->GetParentRow();
    if (theParentRow != nullptr)
        theParentRow->OnChildVisibilityChanged();
}

//==============================================================================
/**
 * Handles clicking on the Visible button.  Toggles the Visible state on the asset.
 */
void CToggleControl::OnVisibleClicked(CToggleButton *inButton,
                                      CToggleButton::EButtonState inButtonState)
{
    Q_UNUSED(inButton);

    m_TimelineItemBinding->GetTimelineItem()->SetVisible(inButtonState
                                                         == CToggleButton::EBUTTONSTATE_DOWN);
    m_StateRow->Filter(*m_StateRow->GetFilter());
}

//==============================================================================
/**
 * Handles clicking on the Lock button.  Toggles the Lock state on the asset.
 */
void CToggleControl::OnLockClicked(CToggleButton *inButton,
                                   CToggleButton::EButtonState inButtonState)
{
    Q_UNUSED(inButton);

    m_TimelineItemBinding->GetTimelineItem()->SetLocked(inButtonState
                                                        == CToggleButton::EBUTTONSTATE_DOWN);
    m_StateRow->Filter(*m_StateRow->GetFilter());
}

//==============================================================================
/**
 * Refreshes the state of this control accd to the asset state
 */
void CToggleControl::Refresh()
{
    CBlankToggleControl::Refresh();

    ITimelineItem *theTimelineItem = m_TimelineItemBinding->GetTimelineItem();
    m_Shy->SetToggleState(theTimelineItem->IsShy());
    m_Locked->SetToggleState(theTimelineItem->IsLocked());

    bool theEnabled = m_TimelineItemBinding->IsVisibleEnabled();
    m_Visible->SetEnabled(theEnabled);
    if (theEnabled) // only valid to update this if the enabled flag is true.
        m_Visible->SetToggleState(theTimelineItem->IsVisible());

    this->Invalidate();
}
