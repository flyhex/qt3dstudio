/****************************************************************************
**
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
#include "StateRowUI.h"

#include "ColorControl.h"
#include "StateTimebarRow.h"
#include "StateRow.h"
#include "ToggleControl.h"
#include "BaseTimelineTreeControl.h"
#include "TimelineUIFactory.h"
#include "StateRow.h"
#include "ITimelineControl.h"
#include "PropertyRow.h"

#include "Bindings/ITimelineItemBinding.h"

CStateRowUI::CStateRowUI(CStateRow *stateRow, CAbstractTimelineRowUI *parentUiRow)
    : CBaseStateRowUI(stateRow, parentUiRow)
{
    connect(stateRow, &CStateRow::timeChanged,
            this, &CStateRowUI::handleTimeChanged);
    connect(stateRow, &CStateRow::layoutRecalcRequested,
            this, &CStateRowUI::handleRecalcLayoutRequested);

}

void CStateRowUI::SetSnappingListProvider(ISnappingListProvider *inProvider)
{
    CStateTimebarRow *theTimebarControl = dynamic_cast<CStateTimebarRow *>(m_TimebarControl);
    if (theTimebarControl)
        theTimebarControl->SetSnappingListProvider(inProvider);
}

ISnappingListProvider *CStateRowUI::GetSnappingListProvider() const
{
    CStateTimebarRow *theTimebarControl = dynamic_cast<CStateTimebarRow *>(m_TimebarControl);
    return (theTimebarControl) ? &theTimebarControl->GetSnappingListProvider() : nullptr;

}

//=============================================================================
/**
 * Set the indent of this control.
 * This controls how far to the right the toggle and text display on this
 * control. The indent should be increased for every level of sub-controls.
 * @param inIndent how much this control should be indented.
 */
void CStateRowUI::SetIndent(long inIndent)
{
    if (!initialized())
        return;

    CAbstractTimelineRowUI::SetIndent(inIndent);

    m_TreeControl->SetIndent(inIndent);

    auto baseStateRow = static_cast<CStateRow *>(m_timelineRow);

    // KDAB_TODO avoid deep copy
    auto stateRows = baseStateRow->GetStateRows();

    auto thePos = stateRows.begin();
    for (; thePos != stateRows.end(); ++thePos) {
        auto uiRow = TimelineUIFactory::instance()->uiForRow(*thePos);
        uiRow->SetIndent(inIndent + CTimelineRow::TREE_INDENT);
    }

    // For each property on this object
    // KDAB_TODO avoid deep copy
    auto propertyRows = baseStateRow->GetPropertyRows();
    auto thePropPos = propertyRows.begin();
    for (; thePropPos != propertyRows.end(); ++thePropPos) {
        CPropertyRow *thePropRow = (*thePropPos);
        if (thePropRow) {
           auto uiRow = TimelineUIFactory::instance()->uiForRow(thePropRow);
           uiRow->SetIndent(inIndent + CTimelineRow::TREE_INDENT);
        }
    }
}
//=============================================================================
/**
 * Trigger any external applications where applicable.
 */
void CStateRowUI::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);

    auto timelineItemBinding = m_timelineRow->GetTimelineItemBinding();
    if (!timelineItemBinding->OpenAssociatedEditor()) // if not handled, fall backon the base class
        CBaseStateRowUI::OnMouseDoubleClick(inPoint, inFlags);
}

void CStateRowUI::handleTimeChanged()
{
    if (!initialized())
        return;

    m_TimebarControl->UpdateTime(m_baseStateRow->GetStartTime(),
                                 m_baseStateRow->GetEndTime());

    GetTopControl()->OnLayoutChanged();
}

void CStateRowUI::handleRecalcLayoutRequested()
{
    GetTopControl()->OnLayoutChanged();
}

CBlankToggleControl *CStateRowUI::CreateToggleControl()
{
    auto timelineItemBinding = m_timelineRow->GetTimelineItemBinding();
    return (timelineItemBinding->ShowToggleControls())
        ? new CToggleControl(this, timelineItemBinding)
        : CBaseStateRowUI::CreateToggleControl();
}

//=============================================================================
/**
 * Create a new CStateTimebarRow.
 * This is virtual and used for objects to return their type specific
 * timebar rows if they want to.
 * @return the created timebar row.
 */
CBaseTimebarlessRow *CStateRowUI::CreateTimebarRow()
{
    auto stateTimebarRow = new CStateTimebarRow(this);
    stateTimebarRow->SetTimeRatio(m_baseStateRow->GetTimeRatio());
    return stateTimebarRow;
}
