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
#include "StateRowFactory.h"
#include "ResourceCache.h"
#include "StudioUtils.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"


CBaseStateRow::CBaseStateRow(CTimelineRow *parent, bool loaded)
    : CTimelineRow(parent)
    , m_Loaded(loaded)
    , m_Selected(false)
{
}

CBaseStateRow::~CBaseStateRow()
{
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
    // Bind after all the UI is setup.
    Q_ASSERT(inTimelineItemBinding);
    m_TimelineItemBinding = inTimelineItemBinding;
    m_TimelineItemBinding->Bind(this); // see Dispose where it properly unbinds.

    emit initialized();
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

        m_IsExpanded = true;

        emit expanded(true);
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

        m_IsExpanded = false;

        emit expanded(false);
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
    emit visibleChanged(theVisibleFlag);

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

    emit hasChildrenChanged(HasVisibleChildren());
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
    emit hasChildrenChanged(HasVisibleChildren());
}

//=============================================================================
/**
 * Helper function to remove all controls of this property row and dispose of it.
 */
void CBaseStateRow::DeletePropertyRow(CPropertyRow *inPropertyRow)
{
    if (!inPropertyRow)
        return;

    emit rowAboutToBeRemoved(inPropertyRow);
    delete inPropertyRow;
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
    emit rowAboutToBeRemoved(inRow);
    inRow->Dispose();
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

        emit childrenLoaded();
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
        emit rowAdded(theRow);
    }
}

void CBaseStateRow::RemoveChildRow(ITimelineItemBinding *inTimelineItem)
{
    CStateRow *theChildRow = GetRow(inTimelineItem->GetTimelineItem());
    inTimelineItem->SetParent(nullptr);
    if (theChildRow) {
        RemoveRow(theChildRow);
        // KDAB_TODO check if needed
        // preserving legacy behavior.
//        GetTopControl()->HideTimelineMoveableTooltip();
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
void CBaseStateRow::Select(Qt::KeyboardModifiers modifier,
                           bool inCheckKeySelection /*= true */)
{
    bool alreadySelected = m_Selected;
    m_TimelineItemBinding->SetSelected(modifier.testFlag(Qt::ControlModifier));
    if (inCheckKeySelection) {
        if (modifier.testFlag(Qt::ShiftModifier))
            emit selectAllKeys();
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
    }

    emit selectedChanged(inSelection);
}

void CBaseStateRow::RequestRefreshRowMetaData()
{
    emit refreshRowMetaData();
}

void CBaseStateRow::ForceEmitChildrenChanged()
{
    emit hasChildrenChanged(HasVisibleChildren());
}

void CBaseStateRow::requestSetNameReadOnly()
{
    emit setNameReadOnly();
}

void CBaseStateRow::requestUpdateActionStatus()
{
    emit updateActionStatus();
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

    emit propertyRowAdded(inRow);
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
            emit hasChildrenChanged(HasVisibleChildren());
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
    emit addRowToUILists(inRow, inNextRow, theFilter);
}

CStateRow *CBaseStateRow::CreateChildRow(ITimelineItemBinding *inChildBinding, CStateRow *inNextRow)
{
    CStateRow *theRow =
        CStateRowFactory::CreateStateRow(inChildBinding, this);
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

    emit addRowToUILists(inRow, inNextRow, m_Filter);
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
    if (m_TimeRatio != inTimeRatio) {
        m_TimeRatio = inTimeRatio;

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
        emit timeRatioChanged(inTimeRatio);
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

// KDAB_TODO unused?
/*
void CBaseStateRow::SetTimelineLatestTime(long inTime)
{
    long theLength = ::TimeToPos(inTime, m_TimeRatio) + CTimelineTimelineLayout::END_BUFFER_SIZE;
    m_TimebarControl->SetAbsoluteSize(CPt(theLength, m_TimebarControl->GetSize().y));

    TStateRowList::iterator thePos = m_StateRows.begin();
    for (; thePos != m_StateRows.end(); ++thePos)
        (*thePos)->SetTimelineLatestTime(inTime);
}
*/

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
 * When this row is no longer useful, clean up.
 */
void CBaseStateRow::Dispose()
{
    // Disconnection point
    if (m_TimelineItemBinding)
        m_TimelineItemBinding->Release();

    CTimelineRow::Dispose();
}


CBaseStateRow::TPropertyRowList CBaseStateRow::GetPropertyRows() const
{
    return m_PropertyRows;
}

CBaseStateRow::TStateRowList CBaseStateRow::GetStateRows() const
{
    return m_StateRows;
}

