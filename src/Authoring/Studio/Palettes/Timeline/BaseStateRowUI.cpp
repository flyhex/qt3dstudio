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
#include "BaseStateRowUI.h"

#include "BaseStateRow.h"
#include "BaseTimelineTreeControl.h"
#include "BaseTimebarlessRow.h"
#include "ColorControl.h"
#include "PropertyRow.h"
#include "StateRow.h"
#include "ToggleControl.h"
#include "TimelineRow.h"
#include "TimelineUIFactory.h"
#include "ITimelineControl.h"
#include "StudioPreferences.h"
#include "Snapper.h"
#include "PropertyRowUI.h"
#include "ComponentContextMenu.h"

#include "Bindings/ITimelineItemBinding.h"

const long CBaseStateRowUI::DEFAULT_TOGGLE_LENGTH = 57;

CBaseStateRowUI::CBaseStateRowUI(CBaseStateRow *baseStateRow,
                                 CAbstractTimelineRowUI *parentUiRow)
    : CAbstractTimelineRowUI(baseStateRow, parentUiRow)
    , m_TreeList(true)// true to align the children in the timeline.
    , m_TreeControl(nullptr)
    , m_ColorControl(nullptr)
    , m_ToggleControl(nullptr)
    , m_TimebarControl(nullptr)
    , m_baseStateRow(baseStateRow)
    , m_Highlighted(false)
{
    connectBackend();
}

CBaseStateRowUI::~CBaseStateRowUI()
{
    delete m_TreeControl;
    delete m_ColorControl;
    delete m_ToggleControl;
    delete m_TimebarControl;
}

void CBaseStateRowUI::Initialize()
{
    m_TreeControl = new CBaseTimelineTreeControl(this, m_baseStateRow->GetTimelineItem()->IsMaster());
    m_ColorControl = new CColorControl(m_baseStateRow);
    m_ToggleControl = CreateToggleControl();
    m_TimebarControl = CreateTimebarRow();

    m_timelineRow->setDirty(true);

    long theTimebarHeight = CStudioPreferences::GetRowSize();
    m_TreeControl->SetSize(CPt(500, theTimebarHeight));
    m_ColorControl->SetAbsoluteSize(CPt(theTimebarHeight, theTimebarHeight));
    m_ToggleControl->SetAbsoluteSize(CPt(DEFAULT_TOGGLE_LENGTH, theTimebarHeight));
    m_TimebarControl->SetSize(CPt(800, theTimebarHeight));

    ::CColor theColor = m_timelineRow->GetTimebarBackgroundColor(m_timelineRow->GetObjectType());
    m_TreeControl->SetBackgroundColor(theColor);
    m_ToggleControl->SetBackgroundColor(theColor);
    m_TimebarControl->SetBackgroundColor(theColor);

    m_ColorList.AddChild(m_ColorControl);
    m_TreeList.AddChild(m_TreeControl);
    m_ToggleList.AddChild(m_ToggleControl);
    m_TimebarList.AddChild(m_TimebarControl);

    m_timelineRow->setDirty(false);
}

void CBaseStateRowUI::connectBackend()
{
    connect(m_baseStateRow, &CBaseStateRow::initialized,
            this, &CBaseStateRowUI::Initialize);
    connect(m_baseStateRow, &CBaseStateRow::dirtyChanged,
            this, &CBaseStateRowUI::handleDirtyChange);
    connect(m_baseStateRow, &CBaseStateRow::propertyRowAdded,
            this, &CBaseStateRowUI::handlePropertyRowAdded);
    connect(m_baseStateRow, &CBaseStateRow::rowAdded,
            this, &CBaseStateRowUI::handleRowAdded);
    connect(m_baseStateRow, &CBaseStateRow::childrenLoaded,
            this, &CBaseStateRowUI::handleChildrenLoaded);
    connect(m_baseStateRow, &CBaseStateRow::timeRatioChanged,
            this, &CBaseStateRowUI::setTimeRatio);
    connect(m_baseStateRow, &CBaseStateRow::expanded,
            this, &CBaseStateRowUI::handleExpanded);
    connect(m_baseStateRow, &CBaseStateRow::visibleChanged,
            this, &CBaseStateRowUI::handleVisibleChanged);
    connect(m_baseStateRow, &CBaseStateRow::hasChildrenChanged,
            this, &CBaseStateRowUI::handleHasChildrenChanged);
    connect(m_baseStateRow, &CBaseStateRow::rowAboutToBeRemoved,
            this, &CBaseStateRowUI::handleRowAboutToBeRemoved);
    connect(m_baseStateRow, &CBaseStateRow::selectAllKeys,
            this, &CBaseStateRowUI::handleSelectAllKeys);
    connect(m_baseStateRow, &CBaseStateRow::selectedChanged,
            this, &CBaseStateRowUI::handleSelectedChanged);
    connect(m_baseStateRow, &CBaseStateRow::addRowToUILists,
            this, &CBaseStateRowUI::handleAddRowToUILists);
    connect(m_baseStateRow, &CBaseStateRow::refreshRowMetaData,
            this, &CBaseStateRowUI::handleRefreshRowMetaDataRequest);
    connect(m_baseStateRow, &CBaseStateRow::setNameReadOnly,
            this, [this] { SetNameReadOnly(true);});
    connect(m_baseStateRow, &CBaseStateRow::updateActionStatus,
            this, &CBaseStateRowUI::UpdateActionStatus);
}

//=============================================================================
/**
 * Get the color control for this row.
 * @return the color control for this row.
 */
CControl *CBaseStateRowUI::GetColorControl()
{
    return &m_ColorList;
}

//=============================================================================
/**
 * Get the tree control for this row.
 * @return the tree control for this row.
 */
CControl *CBaseStateRowUI::GetTreeControl()
{
    return &m_TreeList;
}

//=============================================================================
/**
 * Get the toggle control for this row.
 * @return the toggle control for this row.
 */
CControl *CBaseStateRowUI::GetToggleControl()
{
    return &m_ToggleList;
}

//=============================================================================
/**
 * Get the timebar control for this row.
 * @return the timebar control for this row.
 */
CControl *CBaseStateRowUI::GetTimebarControl()
{
    return &m_TimebarList;
}

CBaseTimebarlessRow *CBaseStateRowUI::GetTimebar() const
{
    return m_TimebarControl;
}

//=============================================================================
/**
 * Restores the focus state of this row.
 */
void CBaseStateRowUI::SetFocus()
{
    if (!initialized())
        return;

    CControl *theParent = m_TreeControl->GetParent();
    if (theParent)
        theParent->GrabFocus(m_TreeControl);
}

void CBaseStateRowUI::SetNameReadOnly(bool inReadOnly)
{
    if (!initialized())
        return;

    m_TreeControl->SetNameReadOnly(inReadOnly);
}

//=============================================================================
/**
 * Call from the child controls that the mouse is over one of the children.
 * This is used to highlight the entire row on mouse over.
 */
void CBaseStateRowUI::OnMouseOver()
{
    if (!initialized())
        return;

    if (!m_Highlighted) {
        try {
            // TODO: Added the try/catch block to prevent crashing when the instance handle is not
            // found
            // this will happen sometimes when delete the object from the timeline
            // need to really fix this at the root.
            ::CColor theColor = m_baseStateRow->GetTimebarHighlightBackgroundColor(
                        m_baseStateRow->GetObjectType());
            m_TreeControl->SetBackgroundColor(theColor);
            m_ToggleControl->SetBackgroundColor(theColor);
            m_TimebarControl->SetBackgroundColor(theColor);

            m_Highlighted = true;
        } catch (...) {
        }
    }
}


//=============================================================================
/**
 * Call from the child controls that the mouse is no longer over one of the children.
 * This is used to highlight the entire row on mouse over.
 */
void CBaseStateRowUI::OnMouseOut()
{
    if (!initialized())
        return;

    if (m_Highlighted) {
        try {
            // TODO: Added the try/catch block to prevent crashing when the instance handle is not
            // found
            // this will happen sometimes when delete the object from the timeline
            // need to really fix this at the root.
            ::CColor theColor = m_baseStateRow->GetTimebarBackgroundColor(
                        m_baseStateRow->GetObjectType());
            m_TreeControl->SetBackgroundColor(theColor);
            m_ToggleControl->SetBackgroundColor(theColor);
            m_TimebarControl->SetBackgroundColor(theColor);

            m_Highlighted = false;
        } catch (...) {
        }
    }
}

//=============================================================================
/**
 * Called when the mouse is double clicked.
 * @param inPoint location of the mouse at time of event
 * @param inFlags modifier key states at time of event
 */
void CBaseStateRowUI::OnMouseDoubleClick(CPt, Qt::KeyboardModifiers inFlags)
{
    // Do nothing by default. Let subclasses define what to do.
    Q_UNUSED(inFlags);
}

//=============================================================================
/**
 * Show context menu for this row
 */
void CBaseStateRowUI::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!initialized())
        return;

    Q_UNUSED(inFlags);

    m_baseStateRow->Select(SBaseStateRowSelectionKeyState()); // ensure this is selected, but doesn't affect any key
                                              // selections, because this can be triggered from a
                                              // key being selected
    CComponentContextMenu theMenu(m_TreeControl, m_baseStateRow->GetTimelineItemBinding());
    m_TreeControl->DoPopup(&theMenu, inPoint);
}

void CBaseStateRowUI::OnChildVisibilityChanged()
{
    if (!initialized())
        return;

    m_TreeControl->SetToggleVisible(m_timelineRow->HasVisibleChildren());
}

//=============================================================================
/**
 * Selects keys in a given rect
 * @param inRect the rect to use for selection
 */void CBaseStateRowUI::SelectKeysInRect(CRct inRect, bool inModifierKeyDown, bool inGlobalCommitSelectionFlag)
{
    if (!initialized())
        return;

    CRct theOffsetRect = inRect;
    theOffsetRect.Offset(-m_TimebarList.GetPosition());

    // KDAB_TODO avoid deep copy
    auto propertyRows = m_baseStateRow->GetPropertyRows();

    // Commits the keyframe selection by setting the keyframes' previous state to its current state,
    // when the user releases the mouse button.
    // This will help the keyframes to retain their original states even though they are
    // not in the mouse select region.
    if (inGlobalCommitSelectionFlag) {
        m_TimebarControl->CommitSelections();

        // iterates through every property row and commits the selection states of properties
        // keyframes
        auto thePropPos = propertyRows.begin();
        for (; thePropPos != propertyRows.end(); ++thePropPos) {
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow && thePropRow->IsViewable()) {
                auto uiRow = static_cast<CPropertyRowUI *>(TimelineUIFactory::instance()->uiForRow(thePropRow));
                uiRow->CommitSelections();
            }
        }
    }

    if (m_timelineRow->isExpanded()) {
        // Iterates each property row and select the keys that are in the rectangle
        auto thePropPos = propertyRows.begin();
        for (; thePropPos != propertyRows.end(); ++thePropPos) {
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow && thePropRow->IsViewable()) {
                auto uiRow = TimelineUIFactory::instance()->uiForRow(thePropRow);
                uiRow->SelectKeysInRect(theOffsetRect, inModifierKeyDown, false);
            }
        }

        // KDAB_TODO avoid deep copy
        auto stateRows = m_baseStateRow->GetStateRows();
        // Recurse the each state row (or master row) and selects the property keyframes in them
        auto thePos = stateRows.begin();
        for (; thePos != stateRows.end(); ++thePos) {
            auto uiRow = TimelineUIFactory::instance()->uiForRow(*thePos);
            uiRow->SelectKeysInRect(theOffsetRect, inModifierKeyDown,
                                        inGlobalCommitSelectionFlag);
        }

    } else {
        // Selects all the master key frames  in the rect
        m_TimebarControl->SelectKeysInRect(theOffsetRect, inModifierKeyDown);
    }
}

//=============================================================================
/**
 * Add snapping points to inSnappingList.
 * This will add the snapping points for any visible objects to inSnappingList.
 * @param inSnappingList the list to add the snapping points to.
 */
void CBaseStateRowUI::PopulateSnappingList(CSnapper *inSnappingList)
{
    if (!initialized())
        return;

    inSnappingList->PushOffset(-m_TimebarList.GetPosition().y);
    m_TimebarControl->PopulateSnappingList(inSnappingList);

    if (m_timelineRow->isExpanded()) {
        auto baseStateRow = static_cast<CBaseStateRow *>(m_timelineRow);
        // KDAB_TODO avoid deep copy
        auto stateRows = baseStateRow->GetStateRows();

        auto thePos = stateRows.begin();
        for (; thePos != stateRows.end(); ++thePos) {
            auto uiRow = TimelineUIFactory::instance()->uiForRow(*thePos);
            uiRow->PopulateSnappingList(inSnappingList);
        }
    }
}

//=============================================================================
/**
 * Begin dragging.
 * sk - potential spot for refactoring the Drag&Drop implementation.
 * Right now, each IDragable is implicitly assumed to be a asset implementation. See
 * *DropSource.cpp: each IDragable is dynamically cast to its implementation.
 */
void CBaseStateRowUI::DoStartDrag(CControlWindowListener *inWndListener)
{
    m_timelineRow->GetTimelineItemBinding()->DoStartDrag(inWndListener);
}

void CBaseStateRowUI::AcceptDropAfter(bool inAccept)
{
    if (!initialized())
        return;
    m_TreeControl->AcceptDropAfter(inAccept);
}

void CBaseStateRowUI::AcceptDropBefore(bool inAccept)
{
    if (!initialized())
        return;
    m_TreeControl->AcceptDropBefore(inAccept);
}

//=============================================================================
/**
 *  Pass through to the binding to set up the target aset for a drag&drop action on this
 *control.
 */
void CBaseStateRowUI::SetDropTarget(CDropTarget *inDropTarget)
{
    m_timelineRow->GetTimelineItemBinding()->SetDropTarget(inDropTarget);
}

//=============================================================================
/**
 * Sets all the child control enable states
 * @param inEnabled the state to set the controls to
 */
void CBaseStateRowUI::SetEnabled(bool inEnabled)
{
    if (!initialized())
        return;
    m_TreeControl->SetEnabled(inEnabled);
    m_ToggleControl->SetEnabled(inEnabled);
    m_ColorControl->SetEnabled(inEnabled);
    m_TimebarControl->SetEnabled(inEnabled);
}

void CBaseStateRowUI::UpdateActionStatus()
{
    if (!initialized())
        return;
    m_ColorControl->UpdateIconStatus();
}

void CBaseStateRowUI::setTimeRatio(double inTimeRatio)
{
    if (!initialized())
        return;
    m_TimebarControl->SetTimeRatio(inTimeRatio);
}

void CBaseStateRowUI::handleDirtyChange(bool dirty)
{
    if (!initialized())
        return;

    if (dirty) {
        m_TimebarControl->SetDirty(true);
        m_TreeControl->Refresh(m_baseStateRow->GetTimelineItemBinding()->GetTimelineItem());
        m_ToggleControl->Refresh();
        m_ColorControl->Invalidate();
        m_TimebarControl->RefreshRowMetaData();
    }
}

void CBaseStateRowUI::handleRowAdded(CBaseStateRow *row)
{
    auto uiRow = TimelineUIFactory::instance()->uiForRow(row);
    CControl *theTreeControl = uiRow->GetTreeControl();
    if (theTreeControl)
        theTreeControl->EnsureVisible();
}

void CBaseStateRowUI::handlePropertyRowAdded(CPropertyRow *row)
{
    if (!initialized())
        return;

    // For snapping timebars/keyframes
    auto uiRow = TimelineUIFactory::instance()->uiForRow(row);
    uiRow->SetSnappingListProvider(GetSnappingListProvider());

    m_TimebarControl->SetDirty(true);
}

void CBaseStateRowUI::handleChildrenLoaded()
{
    GetTopControl()->OnLayoutChanged();
}

void CBaseStateRowUI::handleVisibleChanged(bool visible)
{
    m_ColorList.SetVisible(visible);
    m_TreeList.SetVisible(visible);
    m_ToggleList.SetVisible(visible);
    m_TimebarList.SetVisible(visible);
}

void CBaseStateRowUI::handleHasChildrenChanged(bool hasChildren)
{
    m_TreeControl->SetToggleVisible(hasChildren);
}

void CBaseStateRowUI::handleRowAboutToBeRemoved(CTimelineRow *row)
{
    auto uiRow = TimelineUIFactory::instance()->uiForRow(row);

    m_ColorList.RemoveChild(uiRow->GetColorControl());
    m_TreeList.RemoveChild(uiRow->GetTreeControl());
    m_ToggleList.RemoveChild(uiRow->GetToggleControl());
    m_TimebarList.RemoveChild(uiRow->GetTimebarControl());
}

void CBaseStateRowUI::handleSelectAllKeys()
{
    if (!initialized())
        return;

    m_TimebarControl->SelectAllKeys();
}

void CBaseStateRowUI::handleSelectedChanged(bool selected)
{
    if (!initialized())
        return;

    if (selected) {
        m_TreeControl->EnsureVisible();

        m_TreeControl->OnSelect();
        m_ToggleControl->OnSelect();
        m_ColorControl->OnSelect();
        m_TimebarControl->OnSelect();
    } else {
        m_TreeControl->OnDeselect();
        m_ToggleControl->OnDeselect();
        m_ColorControl->OnDeselect();
        m_TimebarControl->OnDeselect();
    }
}

void CBaseStateRowUI::handleAddRowToUILists(CTimelineRow *inRow, CTimelineRow *inNextRow, CFilter &inFilter)
{
    // Default the insert locations to the end of the list.
    CControl *theNextColorControl = nullptr;
    CControl *theNextTreeControl = nullptr;
    CControl *theNextToggleControl = nullptr;
    CControl *theNextTimebarControl = nullptr;
    if (inNextRow) {
        auto uiNextRow = TimelineUIFactory::instance()->uiForRow(inNextRow);

        theNextColorControl = uiNextRow->GetColorControl();
        theNextTreeControl = uiNextRow->GetTreeControl();
        theNextToggleControl = uiNextRow->GetToggleControl();
        theNextTimebarControl = uiNextRow->GetTimebarControl();
    }

    auto inRowUI = TimelineUIFactory::instance()->uiForRow(inRow);
    inRowUI->SetIndent(m_Indent + CTimelineRow::TREE_INDENT);
    inRow->SetParent(m_timelineRow);
    inRow->Filter(inFilter);
    inRow->SetTimeRatio(m_timelineRow->GetTimeRatio());

    CControl *theColorControl = inRowUI->GetColorControl();
    CControl *theTreeControl = inRowUI->GetTreeControl();
    CControl *theToggleControl = inRowUI->GetToggleControl();
    CControl *theTimebarControl = inRowUI->GetTimebarControl();

    // If not expanded then hide the controls.
    if (!m_timelineRow->isExpanded()) {
        theColorControl->SetVisible(false);
        theTreeControl->SetVisible(false);
        theToggleControl->SetVisible(false);
        theTimebarControl->SetVisible(false);
    }

    // Add the controls to the lists in the prioritized order
    m_ColorList.AddChild(theColorControl, theNextColorControl);
    m_TreeList.AddChild(theTreeControl, theNextTreeControl);
    m_ToggleList.AddChild(theToggleControl, theNextToggleControl);
    m_TimebarList.AddChild(theTimebarControl, theNextTimebarControl);

    m_TreeControl->SetToggleVisible(m_timelineRow->HasVisibleChildren());
}

void CBaseStateRowUI::handleRefreshRowMetaDataRequest()
{
    if (initialized())
        m_TimebarControl->RefreshRowMetaData();
}

void CBaseStateRowUI::handleExpanded(bool expanded)
{
    if (!initialized())
        return;

    m_TreeControl->SetExpanded(expanded);
    m_ColorControl->UpdateIconStatus();
}


//=============================================================================
/**
 * By default, we don't show shy/eye/lock toggles
 */
CBlankToggleControl *CBaseStateRowUI::CreateToggleControl()
{
    return new CBlankToggleControl(this);
}

bool CBaseStateRowUI::initialized() const
{
    return m_TreeControl && m_ToggleControl && m_ColorControl && m_TimebarControl;
}
