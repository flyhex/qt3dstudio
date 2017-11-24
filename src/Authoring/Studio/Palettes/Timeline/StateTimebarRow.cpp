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

#include "StateTimebarRow.h"
#include "Renderer.h"
#include "StateRow.h"
#include "StateRowUI.h"
#include "StudioPreferences.h"
#include "TimebarControl.h"
#include "MasterP.h"
#include "Snapper.h"
#include "Bindings/ITimelineItemBinding.h"

//=============================================================================
/**
 * Creates a new CStateTimebarRow for the StateRow.
 * @param inStateRow the State Row that this is on.
 * @param inCreateTimebar true if the constructor is responsible for creating a timebar, otherwise
 * the derived class will take care of the construction.
 *
 */
CStateTimebarRow::CStateTimebarRow(CStateRowUI *inStateRow, bool inCreateTimebar /*=true*/)
    : CStateTimebarlessRow(inStateRow)
    , m_Timebar(nullptr)
{
    if (inCreateTimebar) {
        m_Timebar = new CTimebarControl(this, inStateRow->GetTimelineRow()->GetTimelineItemBinding());
        m_Timebar->SetMinimumSize(CPt(0, CStudioPreferences::GetRowSize()));

        AddChild(m_Timebar);
    }
}

CStateTimebarRow::~CStateTimebarRow()
{
    delete m_Timebar;
}

//=============================================================================
/**
 * OnMouseRDown event, handles context menus for this object.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
bool CStateTimebarRow::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CStateTimebarlessRow::OnMouseRDown(inPoint, inFlags)) {
        CPt theTimebarPoint = inPoint - m_Timebar->GetPosition();
        m_Timebar->ShowContextMenu(theTimebarPoint, false);
    }
    return true;
}

//=============================================================================
/**
 * Set the amount of time that is being represented per pixel.
 * @param inTimerPerPixel the amound of time being represented per pixel.
 */
void CStateTimebarRow::SetTimeRatio(double inTimeRatio)
{
    CStateTimebarlessRow::SetTimeRatio(inTimeRatio);

    m_Timebar->SetTimeRatio(inTimeRatio);
}

//=============================================================================
/**
 * Notification that the object that this row is representing has been selected.
 */
void CStateTimebarRow::OnSelect()
{
    CStateTimebarlessRow::OnSelect();

    m_Timebar->SetSelected(true);
}

//=============================================================================
/**
 * Notification that the object that this row is representing has been deselected.
 */
void CStateTimebarRow::OnDeselect()
{
    CStateTimebarlessRow::OnDeselect();

    m_Timebar->SetSelected(false);
}

//=============================================================================
/**
 * Notification from the Asset that it's time has changed.
 * @param inStartTime the new start time.
 * @param inEndTime the new end time.
 */
void CStateTimebarRow::UpdateTime(long inStartTime, long inEndTime)
{
    m_Timebar->Refresh(inStartTime, inEndTime);
    Invalidate();
}

void CStateTimebarRow::PopulateSnappingList(CSnapper *inSnappingList)
{
    if (inSnappingList->GetSource() != m_Timebar) {
        inSnappingList->AddTime(m_Timebar->GetStartTime());
        inSnappingList->AddTime(m_Timebar->GetEndTime());
    }
    m_Timebar->PopulateSnappingList(inSnappingList);
    CStateTimebarlessRow::PopulateSnappingList(inSnappingList);
}

//=============================================================================
/**
 * called when meta data for this row is changed... should be overridden by the
 * timebar row
 */
void CStateTimebarRow::RefreshRowMetaData()
{
    Invalidate();
    m_Timebar->RefreshMetaData();
}

void CStateTimebarRow::SetSnappingListProvider(ISnappingListProvider *inProvider)
{
    m_Timebar->SetSnappingListProvider(inProvider);
}

ISnappingListProvider &CStateTimebarRow::GetSnappingListProvider() const
{
    return m_Timebar->GetSnappingListProvider();
}
