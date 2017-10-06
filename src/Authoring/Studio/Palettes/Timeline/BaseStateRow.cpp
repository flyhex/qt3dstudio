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

#include "BaseStateRow.h"
#include "PropertyRow.h"
#include "BaseTimelineTreeControl.h"
#include "ToggleControl.h"
#include "BaseTimebarlessRow.h"
#include "ColorControl.h"
#include "StateRowFactory.h"
#include "TimelineTimelineLayout.h"
#include "ComponentContextMenu.h"
#include "ITimelineControl.h"
#include "ResourceCache.h"
#include "StudioUtils.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"

const long CBaseStateRow::DEFAULT_TOGGLE_LENGTH = 57;

CBaseStateRow::CBaseStateRow()
    : m_TimeRatio(0.0f)
    , m_TreeList(true)// true to align the children in the timeline.
    , m_TreeControl(nullptr)
    , m_ColorControl(nullptr)
    , m_ToggleControl(nullptr)
    , m_TimebarControl(nullptr)
    , m_Loaded(false)
    , m_IsExpanded(false)
    , m_Highlighted(false)
    , m_Dirty(false)
    , m_Selected(false)
    , m_TimelineItemBinding(nullptr)
    , m_ActiveStart(0)
    , m_ActiveEnd(0)
{
}

CBaseStateRow::~CBaseStateRow()
{
    delete m_TreeControl;
    delete m_ColorControl;
    delete m_ToggleControl;
    delete m_TimebarControl;

    // Go through all the state rows and delete them, this control owns all child controls.
    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos)
        (*thePos)->Dispose();

    // Go through all the properties and delete them, this control owns all child controls.
    TPropertyRowList::iterator thePropertyPos = m_PropertyRows.begin();
    for (; thePropertyPos != m_PropertyRows.end(); ++thePropertyPos) {
        CPropertyRow *theRow = (*thePropertyPos);
        delete theRow;
    }
}

void CBaseStateRow::Initialize(ITimelineItemBinding *inTimelineItemBinding)
{
    m_Dirty = true;
    ASSERT(inTimelineItemBinding);
    m_TimelineItemBinding = inTimelineItemBinding;

    m_TreeControl = new CBaseTimelineTreeControl(this, GetTimelineItem()->IsMaster());
    m_ColorControl = new CColorControl(this);
    m_ToggleControl = CreateToggleControl();
    m_TimebarControl = CreateTimebarRow();

    long theTimebarHeight = CStudioPreferences::GetRowSize();
    m_TreeControl->SetSize(CPt(500, theTimebarHeight));
    m_ColorControl->SetAbsoluteSize(CPt(theTimebarHeight, theTimebarHeight));
    m_ToggleControl->SetAbsoluteSize(CPt(DEFAULT_TOGGLE_LENGTH, theTimebarHeight));
    m_TimebarControl->SetSize(CPt(800, theTimebarHeight));

    ::CColor theColor = GetTimebarBackgroundColor(GetObjectType());
    m_TreeControl->SetBackgroundColor(theColor);
    m_ToggleControl->SetBackgroundColor(theColor);
    m_TimebarControl->SetBackgroundColor(theColor);

    m_ColorList.AddChild(m_ColorControl);
    m_TreeList.AddChild(m_TreeControl);
    m_ToggleList.AddChild(m_ToggleControl);
    m_TimebarList.AddChild(m_TimebarControl);

    // sk - I think setting controls' names is only useful for debugging.
    /*Q3DStudio::CString theAssetName( m_Asset->GetName( ) );
    m_TreeControl->SetName( theAssetName + "TreeControl" );
    m_TreeList.SetName( theAssetName + "TreeList" );
    m_ColorControl->SetName( theAssetName + "ColorControl" );
    m_ColorList.SetName( theAssetName + "ColorList" );
    m_ToggleControl->SetName( theAssetName + "ToggleControl" );
    m_ToggleList.SetName( theAssetName + "ToggleList" );
    m_TimebarControl->SetName( theAssetName + "TimebarControl" );
    m_TimebarList.SetName( theAssetName + "TimebarList" );*/

    // Bind after all the UI is setup.
    m_TimelineItemBinding->Bind(this); // see Dispose where it properly unbinds.

    ClearDirty();
}

//=============================================================================
/**
 * Expand this node of the tree control.
 * This will display all children the fit the filter.
 */
void CBaseStateRow::Expand(bool inExpandAll /*= false*/, bool inExpandUp)
{
    if (!m_IsExpanded) {
        m_Filter.SetExpanded(true);

        // Expand/Collapse is done by adding and removing the children, add all the
        // properties first so they are at the top of the list.
        TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
        for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow)
                thePropRow->Filter(m_Filter, false);
        }
        // Add all the State rows after the properties.
        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos) {
            CStateRow *theRow = (*thePos);
            theRow->Filter(m_Filter, false);
        }

        m_TreeControl->SetExpanded(true);
        m_IsExpanded = true;
        m_ColorControl->UpdateIconStatus();
    }

    if (inExpandAll) {
        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos)
            (*thePos)->Expand(inExpandAll);
    }

    if (inExpandUp && m_ParentRow)
        m_ParentRow->Expand(false, inExpandUp);
}

//=============================================================================
/**
 * Collapse this node of the tree control.
 * This will hide all children of this control.
 */
void CBaseStateRow::Collapse(bool inCollapseAll /* = false */)
{
    if (m_IsExpanded) {
        CFilter theFilter = m_Filter;
        theFilter.SetExpanded(false);

        TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
        for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow)
                thePropRow->Filter(theFilter);
        }

        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos) {
            CStateRow *theRow = (*thePos);
            theRow->Filter(theFilter);
        }

        m_TimelineItemBinding->OnCollapsed();

        m_TreeControl->SetExpanded(false);
        m_IsExpanded = false;
        m_ColorControl->UpdateIconStatus();
    }

    if (inCollapseAll) {
        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos) {
            (*thePos)->Collapse(inCollapseAll);
        }
    }
}

//=============================================================================
/**
 * Toggle the expansion state of this control.
 * This will expand the control if it is closed, or collapse it if it is
 * open.
 */
void CBaseStateRow::ToggleExpansion(CToggleButton *, CButtonControl::EButtonState inButtonState)
{
    if (inButtonState == CButtonControl::EBUTTONSTATE_UP)
        Collapse();
    else
        Expand();
}

//=============================================================================
/**
 * Shows or hides rows for all children, based on the filter.
 * @param inFilter Object specifying the filters currently applied to the timeline.
 * @param inFilterChildren true if the filter should go recursively to children.
 */
void CBaseStateRow::Filter(const CFilter &inFilter, bool inFilterChildren /*= true*/)
{
    m_Filter = inFilter;

    // For each child object
    if (inFilterChildren) {
        CFilter theChildFilter = inFilter;
        theChildFilter.SetExpanded(m_IsExpanded);

        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos) {
            // Apply the filter
            CStateRow *theRow = (*thePos);
            theRow->Filter(theChildFilter);
        }
    }

    // This flag determines whether or not the controls on this row should be shown, based on the
    // filter
    bool theVisibleFlag = PerformFilter(m_Filter);

    m_IsViewable = theVisibleFlag;

    theVisibleFlag &= inFilter.IsExpanded();

    // Show or hide the controls on this row before we iterate through the properties
    m_ColorList.SetVisible(theVisibleFlag);
    m_TreeList.SetVisible(theVisibleFlag);
    m_ToggleList.SetVisible(theVisibleFlag);
    m_TimebarList.SetVisible(theVisibleFlag);

    if (inFilterChildren) {
        CFilter theChildFilter = inFilter;
        theChildFilter.SetExpanded(m_IsExpanded);

        // For each property on this object
        TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
        for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
            // Apply the filter
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow)
                thePropRow->Filter(theChildFilter);
        }
    }

    m_TreeControl->SetToggleVisible(this->HasVisibleChildren());
}

//=============================================================================
/**
 * Get the color control for this row.
 * @return the color control for this row.
 */
CControl *CBaseStateRow::GetColorControl()
{
    return &m_ColorList;
}

//=============================================================================
/**
 * Get the tree control for this row.
 * @return the tree control for this row.
 */
CControl *CBaseStateRow::GetTreeControl()
{
    return &m_TreeList;
}

//=============================================================================
/**
 * Get the toggle control for this row.
 * @return the toggle control for this row.
 */
CControl *CBaseStateRow::GetToggleControl()
{
    return &m_ToggleList;
}

//=============================================================================
/**
 * Get the timebar control for this row.
 * @return the timebar control for this row.
 */
CControl *CBaseStateRow::GetTimebarControl()
{
    return &m_TimebarList;
}

//=============================================================================
/**
 * Remove a row from this control.
 * @param inState the state of the row to be removed.
 */
void CBaseStateRow::RemoveRow(CStateRow *inRow)
{
    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos) {
        CStateRow *theRow = (*thePos);
        if (theRow == inRow) {
            DeleteRow(theRow);
            m_StateRows.erase(thePos);
            break;
        }
    }
    m_TreeControl->SetToggleVisible(this->HasVisibleChildren());
}

//=============================================================================
/**
 * Helper function to remove all controls of this property row and dispose of it.
 */
void CBaseStateRow::DeletePropertyRow(CPropertyRow *inPropertyRow)
{
    if (!inPropertyRow)
        return;

    m_ColorList.RemoveChild(inPropertyRow->GetColorControl());
    m_TreeList.RemoveChild(inPropertyRow->GetTreeControl());
    m_ToggleList.RemoveChild(inPropertyRow->GetToggleControl());
    m_TimebarList.RemoveChild(inPropertyRow->GetTimebarControl());
    delete inPropertyRow;
}

//=============================================================================
/**
 * By default, we don't show shy/eye/lock toggles
 */
CBlankToggleControl *CBaseStateRow::CreateToggleControl()
{
    return new CBlankToggleControl(this);
}

//=============================================================================
/**
 * Get the StateRow that is representing this child timeline item.
 * @param inTimelineItem child timeline item
 * @return the StateRow for inState.
 */
CStateRow *CBaseStateRow::GetRow(ITimelineItem *inTimelineItem)
{
    if (inTimelineItem) {
        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos) {
            if ((*thePos)->GetTimelineItem() == inTimelineItem)
                return (*thePos);
        }
    }
    return nullptr;
}

//=============================================================================
/**
 * Called when a row is to be completely removed from the UI
 */
void CBaseStateRow::DeleteRow(CStateRow *inRow)
{
    m_ColorList.RemoveChild(inRow->GetColorControl());
    m_TreeList.RemoveChild(inRow->GetTreeControl());
    m_ToggleList.RemoveChild(inRow->GetToggleControl());
    m_TimebarList.RemoveChild(inRow->GetTimebarControl());

    inRow->Dispose();
}

//=============================================================================
/**
 * Call from the child controls that the mouse is over one of the children.
 * This is used to highlight the entire row on mouse over.
 */
void CBaseStateRow::OnMouseOver()
{
    if (!m_Highlighted) {
        try {
            // TODO: Added the try/catch block to prevent crashing when the instance handle is not
            // found
            // this will happen sometimes when delete the object from the timeline
            // need to really fix this at the root.
            ::CColor theColor = GetTimebarHighlightBackgroundColor(GetObjectType());
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
void CBaseStateRow::OnMouseOut()
{
    if (m_Highlighted) {
        try {
            // TODO: Added the try/catch block to prevent crashing when the instance handle is not
            // found
            // this will happen sometimes when delete the object from the timeline
            // need to really fix this at the root.
            ::CColor theColor = GetTimebarBackgroundColor(GetObjectType());
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
 * Tells this that the Asset data has changed and it needs to be updated.
 * Someone should call ClearDirty afterwards.
 */
void CBaseStateRow::OnDirty()
{
    m_Dirty = true;
}

void CBaseStateRow::ClearDirty()
{
    if (m_Dirty) {
        m_TreeControl->Refresh(m_TimelineItemBinding->GetTimelineItem());
        m_ToggleControl->Refresh();
        m_ColorControl->Invalidate();
        m_TimebarControl->RefreshRowMetaData();
        m_Dirty = false;
    }
}

//=============================================================================
/**
 * Recursively load the children of this control, used by derived classes
 * This will load all the properties and states, and create controls for them.
 */
void CBaseStateRow::LoadChildren()
{
    if (!m_Loaded) {
        m_Loaded = true;

        LoadProperties();

        CTimelineItemOrderedIterator theChildIter(m_TimelineItemBinding);
        // Go through all the children and load them too.
        for (; !theChildIter.IsDone(); ++theChildIter)
            CreateChildRow(*theChildIter, nullptr);

        GetTopControl()->OnLayoutChanged();
    }
}

//=============================================================================
/**
 * Add a row that represents this child timeline item
 * @param inNextItem indicates row to follow behind the row for inTimeLineItem, nullptr to append inRow
 * to the end of the current list.
 */
void CBaseStateRow::AddChildRow(ITimelineItemBinding *inTimeLineItem,
                                ITimelineItemBinding *inNextItem)
{
    if (!inTimeLineItem)
        return;

    // only add if loaded, else it will get added twice.
    if (m_Loaded) {
        CStateRow *theStateRow = CreateChildRow(
            inTimeLineItem, inNextItem ? GetRow(inNextItem->GetTimelineItem()) : nullptr);
        if (theStateRow)
            theStateRow->LoadChildren();
    }
    Expand(false, true);

    CBaseStateRow *theRow = GetRow(inTimeLineItem->GetTimelineItem());
    if (theRow) {
        CControl *theTreeControl = theRow->GetTreeControl();
        if (theTreeControl)
            theTreeControl->EnsureVisible();
    }
}

void CBaseStateRow::RemoveChildRow(ITimelineItemBinding *inTimelineItem)
{
    CStateRow *theChildRow = GetRow(inTimelineItem->GetTimelineItem());
    inTimelineItem->SetParent(nullptr);
    if (theChildRow) {
        RemoveRow(theChildRow);
        // preserving legacy behavior.
        GetTopControl()->HideTimelineMoveableTooltip();
    }
}

//=============================================================================
/**
 * Removes all child rows from this row. Called prior to a load. The load call is responsible for
 * updating the UI.
 */
void CBaseStateRow::RemoveAllChildren()
{
    RemoveAllProperties();

    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos)
        DeleteRow(*thePos);

    m_StateRows.clear();
}

//=============================================================================
/**
 * Remove all the properties from this object. Called prior to a load. The load call is responsible
 * for updating the UI.
 */
void CBaseStateRow::RemoveAllProperties()
{
    TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
    for (; thePropPos != m_PropertyRows.end(); ++thePropPos)
        DeletePropertyRow(*thePropPos);

    m_PropertyRows.clear();
}

//=============================================================================
/**
 * Set this row to selected
 */
void CBaseStateRow::Select(SBaseStateRowSelectionKeyState inState,
                           bool inCheckKeySelection /*= true */)
{
    bool alreadySelected = m_Selected;
    m_TimelineItemBinding->SetSelected(inState.IsControlDown());
    if (inCheckKeySelection) {
        if (inState.IsShiftDown())
            m_TimebarControl->SelectAllKeys();
        else if (!alreadySelected)
            m_TimelineItemBinding->ClearKeySelection();
    }
}

//=============================================================================
/**
 * Change the selection state of the row.
 */
void CBaseStateRow::OnSelected(bool inSelection)
{
    if (inSelection == m_Selected)
        return;

    m_Selected = inSelection;
    if (inSelection) {
        if (m_ParentRow)
            m_ParentRow->Expand(false, true);

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

//=============================================================================
/**
 * Call to add a property row as a child of this control.
 * @param inRow the row to be added.
 */
void CBaseStateRow::AddPropertyRow(CPropertyRow *inRow, CTimelineRow *inNextRow /*= nullptr */)
{
    m_PropertyRows.push_back(inRow);
    InitializePropertyRow(inRow, inNextRow);
    // For snapping timebars/keyframes
    inRow->SetSnappingListProvider(GetSnappingListProvider());

    m_TimebarControl->SetDirty(true);
}

//=============================================================================
/**
 * Remove the property row.
 */
void CBaseStateRow::RemovePropertyRow(const CPropertyRow *inRow)
{
    if (!inRow)
        return;

    TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
    for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
        CPropertyRow *theRow = *thePropPos;
        if (theRow == inRow) {
            DeletePropertyRow(theRow);
            m_PropertyRows.erase(thePropPos);

            // Update flippy
            OnChildVisibilityChanged();
            break;
        }
    }
}

//=============================================================================
/**
 * Helper function to initialize a new property row
 * @param inRow				the row to be added.
 * @param inNextRow			if specified, row that should be after inRow after
 * insertion.
 */
void CBaseStateRow::InitializePropertyRow(CPropertyRow *inRow, CTimelineRow *inNextRow /*= nullptr */)
{
    CFilter theFilter = m_Filter;
    theFilter.SetExpanded(m_IsExpanded);

    if (!inNextRow) { // not provided, this property row would be inserted before the first
                      // non-property row.
        CTimelineItemOrderedIterator theIterator(m_TimelineItemBinding);
        if (!theIterator.IsDone())
            inNextRow = GetRow(theIterator.GetCurrent()->GetTimelineItem());
    }
    AddRowToUILists(inRow, inNextRow, theFilter);
}

void CBaseStateRow::AddRowToUILists(CTimelineRow *inRow, CTimelineRow *inNextRow, CFilter &inFilter)
{
    // Default the insert locations to the end of the list.
    CControl *theNextColorControl = nullptr;
    CControl *theNextTreeControl = nullptr;
    CControl *theNextToggleControl = nullptr;
    CControl *theNextTimebarControl = nullptr;
    if (inNextRow) {
        theNextColorControl = inNextRow->GetColorControl();
        theNextTreeControl = inNextRow->GetTreeControl();
        theNextToggleControl = inNextRow->GetToggleControl();
        theNextTimebarControl = inNextRow->GetTimebarControl();
    }
    inRow->SetIndent(m_Indent + CTimelineRow::TREE_INDENT);
    inRow->SetParent(this);
    inRow->Filter(inFilter);
    inRow->SetTimeRatio(m_TimeRatio);

    CControl *theColorControl = inRow->GetColorControl();
    CControl *theTreeControl = inRow->GetTreeControl();
    CControl *theToggleControl = inRow->GetToggleControl();
    CControl *theTimebarControl = inRow->GetTimebarControl();

    // If not expanded then hide the controls.
    if (!m_IsExpanded) {
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

    m_TreeControl->SetToggleVisible(this->HasVisibleChildren());
}

CStateRow *CBaseStateRow::CreateChildRow(ITimelineItemBinding *inChildBinding, CStateRow *inNextRow)
{
    CStateRow *theRow =
        CStateRowFactory::CreateStateRow(inChildBinding, this, GetSnappingListProvider());
    if (theRow) { // add by appending to the list
        AddStateRow(theRow, inNextRow);
    }
    inChildBinding->SetParent(m_TimelineItemBinding);
    return theRow;
}

long CBaseStateRow::GetNumNonPropertyRows() const
{
    return static_cast<long>(m_StateRows.size());
}

CBaseStateRow *CBaseStateRow::GetNonPropertyRow(long inIndex) const
{
    return m_StateRows.at(inIndex);
}

long CBaseStateRow::GetNumPropertyRows() const
{
    return static_cast<long>(m_PropertyRows.size());
}
CPropertyRow *CBaseStateRow::GetPropertyRow(long inIndex) const
{
    return m_PropertyRows.at(inIndex);
}

//=============================================================================
/**
 * Call to add a state row as a child of this control.
 * @param inRow the row to be added.
 * @param inNextRow row to follow behind the row that would be added, nullptr to append inRow to the
 * end of the current list.
 */
void CBaseStateRow::AddStateRow(CStateRow *inRow, CStateRow *inNextRow)
{
    if (inNextRow != nullptr) {
        TStateRowList::iterator thePos = m_StateRows.begin();
        while (thePos != m_StateRows.end()) {
            if ((*thePos) == inNextRow) {
                m_StateRows.insert(thePos, inRow);
                thePos = m_StateRows.end();
            } else
                ++thePos;
        }
    } else {
        m_StateRows.push_back(inRow);
    }

    AddRowToUILists(inRow, inNextRow, m_Filter);
}

//=============================================================================
/**
 * Checks to see if there are any visible children of this row.
 * This is used for figuring out whether the expand button should be displayed
 * or not.
 */
bool CBaseStateRow::HasVisibleChildren()
{
    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos) {
        // Apply the filter
        if ((*thePos)->IsViewable())
            return true;
    }

    // For each property on this object
    TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
    for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
        // Apply the filter
        CPropertyRow *thePropRow = (*thePropPos);
        if (thePropRow && thePropRow->IsViewable())
            return true;
    }
    return false;
}

//=============================================================================
/**
 * Set the amount of time that is represented by a pixel.
 * This modifies the length of this control.
 * @param inTimePerPixel the time per pixel.
 */
void CBaseStateRow::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
    m_TimebarControl->SetTimeRatio(inTimeRatio);

    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos) {
        (*thePos)->SetTimeRatio(inTimeRatio);
    }

    TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
    for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
        CPropertyRow *thePropRow = (*thePropPos);
        if (thePropRow)
            thePropRow->SetTimeRatio(inTimeRatio);
    }
}

//=============================================================================
/**
 * Called when a child becomes visible/invisible.
 */
void CBaseStateRow::OnChildVisibilityChanged()
{
    m_TreeControl->SetToggleVisible(this->HasVisibleChildren());
}

//=============================================================================
/**
 * Called when the mouse is double clicked.
 * @param inPoint location of the mouse at time of event
 * @param inFlags modifier key states at time of event
 */
void CBaseStateRow::OnMouseDoubleClick(CPt, Qt::KeyboardModifiers inFlags)
{
    // Do nothing by default. Let subclasses define what to do.
    Q_UNUSED(inFlags);
}

//=============================================================================
/**
 * Show context menu for this row
 */
void CBaseStateRow::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inFlags);

    Select(SBaseStateRowSelectionKeyState()); // ensure this is selected, but doesn't affect any key
                                              // selections, because this can be triggered from a
                                              // key being selected
    CComponentContextMenu theMenu(m_TreeControl, m_TimelineItemBinding);
    m_TreeControl->DoPopup(&theMenu, inPoint);
}

//=============================================================================
/**
 * Selects keys in a given rect
 * @param inRect the rect to use for selection
 */
void CBaseStateRow::SelectKeysInRect(CRct inRect, bool inModifierKeyDown,
                                     bool inGlobalCommitSelectionFlag)
{
    CRct theOffsetRect = inRect;
    theOffsetRect.Offset(-m_TimebarList.GetPosition());

    // Commits the keyframe selection by setting the keyframes' previous state to its current state,
    // when the user releases the mouse button.
    // This will help the keyframes to retain their original states even though they are
    // not in the mouse select region.
    if (inGlobalCommitSelectionFlag) {
        m_TimebarControl->CommitSelections();

        // iterates through every property row and commits the selection states of properties
        // keyframes
        TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
        for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow && thePropRow->IsViewable())
                thePropRow->CommitSelections();
        }
    }

    if (m_IsExpanded) {
        // Iterates each property row and select the keys that are in the rectangle
        TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
        for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
            CPropertyRow *thePropRow = (*thePropPos);
            if (thePropRow && thePropRow->IsViewable())
                thePropRow->SelectKeysInRect(theOffsetRect, inModifierKeyDown);
        }

        // Recurse the each state row (or master row) and selects the property keyframes in them
        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos)
            (*thePos)->SelectKeysInRect(theOffsetRect, inModifierKeyDown,
                                        inGlobalCommitSelectionFlag);

    } else {
        // Selects all the master key frames  in the rect
        m_TimebarControl->SelectKeysInRect(theOffsetRect, inModifierKeyDown);
    }
}

//=============================================================================
/**
 * Deletes all the keys for the asset that was chosen by the user
 * @param inBatch the batch used to batch all the deletes together
 */
void CBaseStateRow::DeleteAllKeys()
{
    // Iterate through all the property rows and delete all their keys
    TPropertyRowList::iterator thePropPos = m_PropertyRows.begin();
    for (; thePropPos != m_PropertyRows.end(); ++thePropPos) {
        CPropertyRow *thePropRow = (*thePropPos);
        if (thePropRow)
            thePropRow->DeleteAllKeys();
    }
}

//=============================================================================
/**
 * Add snapping points to inSnappingList.
 * This will add the snapping points for any visible objects to inSnappingList.
 * @param inSnappingList the list to add the snapping points to.
 */
void CBaseStateRow::PopulateSnappingList(CSnapper *inSnappingList)
{
    inSnappingList->PushOffset(-m_TimebarList.GetPosition().y);
    m_TimebarControl->PopulateSnappingList(inSnappingList);

    if (IsExpanded()) {
        TStateRowList::iterator thePos = m_StateRows.begin();
        for (; thePos != m_StateRows.end(); ++thePos) {
            (*thePos)->PopulateSnappingList(inSnappingList);
        }
    }
}

//=============================================================================
/**
 * Sets all the child control enable states
 * @param inEnabled the state to set the controls to
 */
void CBaseStateRow::SetEnabled(bool inEnabled)
{
    m_TreeControl->SetEnabled(inEnabled);
    m_ToggleControl->SetEnabled(inEnabled);
    m_ColorControl->SetEnabled(inEnabled);
    m_TimebarControl->SetEnabled(inEnabled);
}

//=============================================================================
/**
 * Begin dragging.
 * sk - potential spot for refactoring the Drag&Drop implementation.
 * Right now, each IDragable is implicitly assumed to be a asset implementation. See
 * *DropSource.cpp: each IDragable is dynamically cast to its implementation.
 */
void CBaseStateRow::DoStartDrag(CControlWindowListener *inWndListener)
{
    m_TimelineItemBinding->DoStartDrag(inWndListener);
}

void CBaseStateRow::AcceptDropAfter(bool inAccept)
{
    m_TreeControl->AcceptDropAfter(inAccept);
}

void CBaseStateRow::AcceptDropBefore(bool inAccept)
{
    m_TreeControl->AcceptDropBefore(inAccept);
}

//=============================================================================
/**
 *	Pass through to the binding to set up the target aset for a drag&drop action on this
 *control.
 */
void CBaseStateRow::SetDropTarget(CDropTarget *inDropTarget)
{
    m_TimelineItemBinding->SetDropTarget(inDropTarget);
}

void CBaseStateRow::SetTimelineLatestTime(long inTime)
{
    long theLength = ::TimeToPos(inTime, m_TimeRatio) + CTimelineTimelineLayout::END_BUFFER_SIZE;
    m_TimebarControl->SetAbsoluteSize(CPt(theLength, m_TimebarControl->GetSize().y));

    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos)
        (*thePos)->SetTimelineLatestTime(inTime);
}

//=============================================================================
/**
 * Determines whether or not a row is expanded.  A row can be expanded even if
 * it is not visible.
 * @return true if the row is currently expanded, otherwise false
 */
bool CBaseStateRow::IsExpanded()
{
    return m_IsExpanded;
}

//=============================================================================
/**
 * Determines whether or not a row is loaded. The rows are delayed loaded, i.e.
 * it will be loaded when it's visible for the first time. Before it's loaded, any
 * updates to the structure, say, adding dynamic properties does not need to update the
 * timeline.
 * @return true if the row is currently loaded, otherwise false
 */
bool CBaseStateRow::IsLoaded()
{
    return m_Loaded;
}

long CBaseStateRow::GetStartTime()
{
    ITimelineTimebar *theTimebar = m_TimelineItemBinding->GetTimelineItem()->GetTimebar();
    if (theTimebar)
        return theTimebar->GetStartTime();
    return 0;
}

long CBaseStateRow::GetEndTime()
{
    ITimelineTimebar *theTimebar = m_TimelineItemBinding->GetTimelineItem()->GetTimebar();
    if (theTimebar)
        return theTimebar->GetEndTime();
    return 0;
}

long CBaseStateRow::GetActiveStart()
{
    return m_ActiveStart;
}
long CBaseStateRow::GetActiveEnd()
{
    return m_ActiveEnd;
}

//=============================================================================
/**
 * Get the start time of this row, which is accumulative of all its descendants.
 */
long CBaseStateRow::GetEarliestStartTime()
{
    long theEarliestStartTime = 0;
    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos) {
        CStateRow *theRow = (*thePos);
        long theStartTime = theRow->GetEarliestStartTime();
        if (theStartTime < theEarliestStartTime)
            theEarliestStartTime = theStartTime;
    }
    return theEarliestStartTime;
}

//=============================================================================
/**
 * Get the end time of this row, which is accumulative of all its descendants.
 */
long CBaseStateRow::GetLatestEndTime()
{
    long theLatestTime = 0;
    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos) {
        CStateRow *theRow = (*thePos);
        long theEndTime = theRow->GetLatestEndTime();
        if (theEndTime > theLatestTime)
            theLatestTime = theEndTime;
    }
    return theLatestTime;
}

//=============================================================================
/**
 * Lame switch to get the normal state object specific icon.
 * @return the icon to be used in the 'normal' state.
 */
QPixmap CBaseStateRow::GetIcon()
{
    return CResourceCache::GetInstance()->GetBitmap(
        CStudioObjectTypes::GetNormalIconName(GetObjectType()));
}

//=============================================================================
/**
 * Lame switch to get the disabled state object specific icon.
 * @return the icon to be used in the disabled state.
 */
QPixmap CBaseStateRow::GetDisabledIcon()
{
    return CResourceCache::GetInstance()->GetBitmap(
        CStudioObjectTypes::GetDisabledIconName(GetObjectType()));
}

//=============================================================================
/**
 * @return the studio type of the object represented by this row
 */
EStudioObjectType CBaseStateRow::GetObjectType() const
{
    return GetTimelineItem()->GetObjectType();
}

ITimelineItemBinding *CBaseStateRow::GetTimelineItemBinding() const
{
    return m_TimelineItemBinding;
}

ITimelineItem *CBaseStateRow::GetTimelineItem() const
{
    return m_TimelineItemBinding->GetTimelineItem();
}

//=============================================================================
/**
 * When this row is no longer useful, clean up.
 */
void CBaseStateRow::Dispose()
{
    // Disconnection point
    if (m_TimelineItemBinding)
        m_TimelineItemBinding->Release();

    CTimelineRow::Dispose();
}

void CBaseStateRow::UpdateActionStatus()
{
    m_ColorControl->UpdateIconStatus();
}

//=============================================================================
/**
 * Restores the focus state of this row.
 */
void CBaseStateRow::SetFocus()
{
    CControl *theParent = m_TreeControl->GetParent();
    if (theParent)
        theParent->GrabFocus(m_TreeControl);
}

CBaseTimebarlessRow *CBaseStateRow::GetTimebar() const
{
    return m_TimebarControl;
}

void CBaseStateRow::SetNameReadOnly(bool inReadOnly)
{
    m_TreeControl->SetNameReadOnly(inReadOnly);
}
