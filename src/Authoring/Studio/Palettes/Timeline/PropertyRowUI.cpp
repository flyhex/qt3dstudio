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
#include "PropertyRowUI.h"

#include "PropertyRow.h"
#include "PropertyTreeControl.h"
#include "TimelineControl.h"
#include "PropertyColorControl.h"
#include "PropertyToggleControl.h"
#include "PropertyTreeControl.h"
#include "PropertyTimebarRow.h"
#include "BlankControl.h"

CPropertyRowUI::CPropertyRowUI(CPropertyRow *propertyRow, CAbstractTimelineRowUI *parentUiRow)
    : CAbstractTimelineRowUI(propertyRow, parentUiRow)
    , m_propertyRow(propertyRow)
{
    connect(propertyRow, &CPropertyRow::dirtyChanged,
            this, &CPropertyRowUI::handleDirtyChanged);
    connect(propertyRow, &CPropertyRow::visibleChanged,
            this, &CPropertyRowUI::handleVisibleChanged);
    connect(propertyRow, &CPropertyRow::selectAllKeys,
            this, &CPropertyRowUI::handleSelectAllKeys);
    connect(propertyRow, &CPropertyRow::deleteAllKeys,
            this, &CPropertyRowUI::handleDeleteAllKeys);
    connect(propertyRow, &CPropertyRow::refreshRequested,
            this, &CPropertyRowUI::handleRefreshRequested);
    connect(propertyRow, &CPropertyRow::selectKeysByTime,
            this, &CPropertyRowUI::handleSelectKeysByTime);
    connect(propertyRow, &CPropertyRow::timeRatioChanged,
            this, &CPropertyRowUI::setTimeRatio);

    m_TreeControl = new CPropertyTreeControl(this);
    m_ToggleControl = new CPropertyToggleControl(this);
    m_TimebarRow = new CPropertyTimebarRow(this);
    m_TimebarRow->SetTimeRatio(propertyRow->GetTimeRatio());

    m_PropertyColorControl = new CPropertyColorControl(m_propertyRow);
    long theTimebarHeight = CStudioPreferences::GetRowSize();

    // do not set absolute size because it messes up the timeline
    m_TreeControl->SetSize(CPt(500, theTimebarHeight));
    m_PropertyColorControl->SetAbsoluteSize(CPt(theTimebarHeight, theTimebarHeight));
    m_ToggleControl->SetAbsoluteSize(CPt(57, theTimebarHeight));
    m_TimebarRow->SetSize(CPt(0, theTimebarHeight));

    // sk: setting controls names' seem to be only useful for debugging.
    // Q3DStudio::CString thePropName( inProperty->GetName( ) );
    // m_TreeControl->SetName( thePropName );
    // m_TimebarRow->SetName( thePropName + "TimebarRow" );
}

CPropertyRowUI::~CPropertyRowUI()
{
    delete m_TreeControl;
    delete m_ToggleControl;
    delete m_PropertyColorControl;
    delete m_TimebarRow;
}


//=============================================================================
/**
 * Get the left hand color control for this row.
 * @return the color control for this row.
 */
CControl *CPropertyRowUI::GetColorControl()
{
    return m_PropertyColorControl;
}

//=============================================================================
/**
 * Get the tree control object for this row.
 * @return the tree control object for this row.
 */
CControl *CPropertyRowUI::GetTreeControl()
{
    return m_TreeControl;
}

//=============================================================================
/**
 * Get the toggle control object for this row.
 * @return the toggle control object for this row.
 */
CControl *CPropertyRowUI::GetToggleControl()
{
    return m_ToggleControl;
}

//=============================================================================
/**
 * Get the timebar control for this row.
 * @return the timebar control for this row.
 */
CControl *CPropertyRowUI::GetTimebarControl()
{
    return m_TimebarRow;
}

//=============================================================================
/**
 * Set the amount that this row is indented.
 * This handles the tree indenting.
 * @param inIndent the amount of indent for this row.
 */
void CPropertyRowUI::SetIndent(long inIndent)
{
    m_TreeControl->SetIndent(inIndent);
}

long CPropertyRowUI::GetIndent() const
{
    return m_TreeControl->GetIndent();
}

void CPropertyRowUI::SetSnappingListProvider(ISnappingListProvider *inProvider)
{
    m_TimebarRow->SetSnappingListProvider(inProvider);
}

ISnappingListProvider *CPropertyRowUI::GetSnappingListProvider() const
{
    return &m_TimebarRow->GetSnappingListProvider();
}

//=============================================================================
/**
 * Notification from one of the child controls that the mouse just entered.
 * This is used to handle the highlighting of the entire row.
 */
void CPropertyRowUI::OnMouseOver()
{
    // Only change if change is needed
    if (!m_Highlighted) {
        m_TreeControl->SetHighlighted(true);
        m_ToggleControl->SetHighlighted(true);
        m_TimebarRow->SetHighlighted(true);

        m_Highlighted = true;
    }
}

//=============================================================================
/**
 * Notification from one of the child controls that the mouse just left.
 * This is used to handle the highlighting of the entire row.
 */
void CPropertyRowUI::OnMouseOut()
{
    // Only change is change is needed.
    if (m_Highlighted) {
        m_TreeControl->SetHighlighted(false);
        m_ToggleControl->SetHighlighted(false);
        m_TimebarRow->SetHighlighted(false);

        m_Highlighted = false;
    }
}

//=============================================================================
/**
 * Double click handler for the property row
 */
void CPropertyRowUI::OnMouseDoubleClick()
{
    SetDetailedView(!m_DetailedView);
}

//=============================================================================
/**
 * Selects all keyframes of this property that are inside inRect
 *
 * @param inRect theRect to check
 */
void CPropertyRowUI::SelectKeysInRect(CRct inRect, bool inModifierKeyDown,
                                      bool inGlobalCommitSelectionFlag)
{
    Q_UNUSED(inGlobalCommitSelectionFlag);
    CRct theOffsetRect = inRect;

    theOffsetRect.Offset(-m_TimebarRow->GetPosition());
    m_TimebarRow->SelectKeysInRect(theOffsetRect, inModifierKeyDown);
}

//=============================================================================
/**
 * Sets all the child control enable states
 * @param inEnabled the state to set the controls to
 */
void CPropertyRowUI::SetEnabled(bool inEnabled)
{
    m_TreeControl->SetEnabled(inEnabled);
    m_ToggleControl->SetEnabled(inEnabled);
    m_PropertyColorControl->SetEnabled(inEnabled);
    m_TimebarRow->SetEnabled(inEnabled);
}


//=============================================================================
/**
 * CommitSelections: commits all the property keyframe selections by setting their
 *                   previous selection state to the current selection state.
 *                   This will prevent the keyframes in the current selection
 *                   from
 *                   switching states as we select other keyframes.
 *
 * @param NONE
 * @return NONE
 */
void CPropertyRowUI::CommitSelections()
{
    m_TimebarRow->CommitSelections();
}

void CPropertyRowUI::handleDirtyChanged(bool dirty)
{
    m_TimebarRow->SetDirty(true);
}

void CPropertyRowUI::setTimeRatio(double inTimeRatio)
{
    m_TimebarRow->SetTimeRatio(inTimeRatio);
}

void CPropertyRowUI::handleVisibleChanged(bool visible)
{
    m_TreeControl->SetVisible(visible);
    m_TimebarRow->SetVisible(visible);
    m_ToggleControl->SetVisible(visible);
    m_PropertyColorControl->SetVisible(visible);
}

void CPropertyRowUI::handleSelectAllKeys()
{
    m_TimebarRow->SelectAllKeys();
}

void CPropertyRowUI::handleDeleteAllKeys()
{
    m_TimebarRow->Invalidate();
}

void CPropertyRowUI::handleRefreshRequested()
{
    if (m_TimebarRow->IsVisible())
        m_TimebarRow->SetDirty(true);
}

void CPropertyRowUI::handleSelectKeysByTime(long inTime, bool inSelected)
{
    m_TimebarRow->SelectKeysByTime(inTime, inSelected);
}

//=============================================================================
/**
 * Expands the row out so a more detailed view of the animation tracks can be seen
 *
 *	@param inIsInDetailedView true if we are switching to a detailed view
 */
void CPropertyRowUI::SetDetailedView(bool inIsDetailedView)
{
    m_DetailedView = inIsDetailedView;

    if (m_DetailedView) {
        long theHeight = 100;
        m_PropertyColorControl->SetAbsoluteSize(
            CPt(m_PropertyColorControl->GetSize().x, theHeight));
        m_TreeControl->SetSize(CPt(m_TreeControl->GetSize().x, theHeight));
        m_ToggleControl->SetAbsoluteSize(CPt(m_ToggleControl->GetSize().x, theHeight));
        m_TimebarRow->SetAbsoluteSize(CPt(m_TimebarRow->GetSize().x, theHeight));

        m_TimebarRow->SetDetailedView(true);
    } else {
        long theDefaultHeight = CStudioPreferences::GetRowSize();

        m_PropertyColorControl->SetAbsoluteSize(
            CPt(m_PropertyColorControl->GetSize().x, theDefaultHeight));
        m_TreeControl->SetSize(CPt(m_TreeControl->GetSize().x, theDefaultHeight));
        m_ToggleControl->SetAbsoluteSize(CPt(m_ToggleControl->GetSize().x, theDefaultHeight));
        m_TimebarRow->SetAbsoluteSize(CPt(m_TimebarRow->GetSize().x, theDefaultHeight));

        m_TimebarRow->SetDetailedView(false);
    }

    GetTopControl()->OnLayoutChanged();
}
