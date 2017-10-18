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
#include "TreeControl.h"
#include "Renderer.h"
#include "HotKeys.h"
#include "MasterP.h"
#include "StudioPreferences.h"
#include "StudioErrorIDs.h"

//=============================================================================
/**
 * Constructor
 */
CTreeControl::CTreeControl()
    : m_AllowMultipleSelection(false)
    , m_Refresh(true)
    , m_SortFlag(false)
{

    m_BGColor = CStudioPreferences::GetBaseColor();
}

//=============================================================================
/**
 * Destructor
 */
CTreeControl::~CTreeControl()
{
    // Just clear out our lists, deletion is handled by whoever created the items.
    m_ItemList.clear();
    m_SelectedItemList.clear();
}

//=============================================================================
/**
 * Sets the background color of this control.
 * @param inColor new background color
 */
void CTreeControl::SetBackgroundColor(const CColor &inColor)
{
    m_BGColor = inColor;
    Invalidate();
}

//=============================================================================
/**
 * Draws the control.
 */
void CTreeControl::Draw(CRenderer *inRenderer)
{
    inRenderer->FillSolidRect(QRect(QPoint(0, 0), GetSize()), m_BGColor);
}

//=============================================================================
/**
 * Find the item that this item should be added after to maintian sort order.
 */
CTreeItem *CTreeControl::FindPrevSortSibling(CTreeItem *, CTreeItem *)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    QT3DS_THROW(E_FAIL); // this is ass
#endif
    return NULL;
}

//=============================================================================
/**
 * Find the item that this item should be before after to maintian sort order.
 */
CTreeItem *CTreeControl::FindNextSortSibling(CTreeItem *inParent, CTreeItem *inChild)
{
    CTreeItem *theSortItem = nullptr;

    if (inParent) {
        theSortItem = inParent->FindNextSortSibling(inChild);
    } else {
        // No items in the list - return nullptr (because we don't care)
        if (m_ItemList.size()) {
            CTreeItem::TItemList::iterator thePos = m_ItemList.begin();
            for (; thePos != m_ItemList.end(); ++thePos) {
                // Item in the list
                theSortItem = *thePos;
                if (IsItemLess(inChild, theSortItem))
                    break;
            }

            // Last item in the list
            if (thePos == m_ItemList.end())
                theSortItem = nullptr;
        }
    }

    return theSortItem;
}

//=============================================================================
/**
 * Adds a tree item to the tree control.  The child is automatically added to
 * the end of the list under the specified parent.  Once you add a tree item,
 * to this tree control.  You must call the remove functions to take it out of
 * the tree control, after which point, it is safe for you to delete it.
 * @param inParent parent of the tree item or nullptr if the item is to be a root
 * @param inChild the child item to be added
 */
void CTreeControl::AddItem(CTreeItem *inParent, CTreeItem *inChild)
{
    CTreeItem *theBeforeItem = nullptr;

    if (IsSorting())
        theBeforeItem = FindNextSortSibling(inParent, inChild);

    AddItemBefore(inParent, inChild, theBeforeItem);
}

//=============================================================================
/**
 * Same as AddItem( ), except that you can specify where to put the child in relation
 * to its siblings.
 * @param inParent parent of the new item, or nullptr if adding a root level item
 * @param inChild new child to be added
 * @param inBefore should be a child of inParent, in which case, inChild is added before it;
 * can be nullptr to indicate that inChild needs to go at the end of the inParent's children.
 */
void CTreeControl::AddItemBefore(CTreeItem *inParent, CTreeItem *inChild, CTreeItem *inBefore)
{
    if (inParent)
        inParent->AddItem(inChild, inBefore);
    else
        AddToRootItemList(inChild, inBefore);

    CTreeItem *theControlBefore = inBefore;
    if (inParent && !inBefore)
        theControlBefore = RecurseGetControlAfter(inParent);

    AddToHierarchy(inChild, theControlBefore, true);
}

//=============================================================================
/**
 * Same as AddItem( ), except that you can specify where to put the child in relation
 * to its siblings.
 * @param inParent parent of the new item, or nullptr if adding a root level item
 * @param inChild new child to be added
 * @param inAfter should be a child of inParent, in which case, inChild is added after it
 */
void CTreeControl::AddItemAfter(CTreeItem *inParent, CTreeItem *inChild, CTreeItem *inAfter)
{
    CTreeItem *theItemBefore = nullptr;
    CTreeItem *theControlBefore = nullptr;

    theItemBefore = GetSiblingAfter(inAfter);
    theControlBefore = RecurseGetControlAfter(inAfter);

    if (inParent)
        inParent->AddItem(inChild, theItemBefore);
    else
        AddToRootItemList(inChild, theItemBefore);

    AddToHierarchy(inChild, theControlBefore, true);
}

//=============================================================================
/**
 * Simply detaches inItem from the tree.  Provided so that subclasses can
 * override this function to actually perform item deletion if necessary.
 * @param inItem Item to be removed
 */
void CTreeControl::RemoveItem(CTreeItem *inItem)
{
    Detach(inItem);
}

//=============================================================================
/**
 * Removes all currently selected items.
 * @param inItem Item to be removed
 */
void CTreeControl::RemoveSelectedItems()
{
    // Iterate through the selected items, removing each one
    CTreeItem::TItemList::iterator thePos = m_SelectedItemList.begin();
    for (; thePos != m_SelectedItemList.end(); ++thePos) {
        CTreeItem *theItem = *thePos;
        RemoveItem(theItem);
    }
    m_SelectedItemList.clear();
}

//=============================================================================
/**
 * Removes all currently selected items.
 * @param inItem Item to be removed
 */
void CTreeControl::RemoveAllItems()
{
    // Iterate through the selected items, removing each one
    CTreeItem::TItemList::iterator thePos = m_ItemList.begin();

    while (thePos != m_ItemList.end()) {
        CTreeItem *theItem = *thePos;
        RemoveItem(theItem);
        thePos = m_ItemList.begin();
    }
    ASSERT(m_ItemList.empty() == true);
}

//=============================================================================
/**
 * Creates a break in the tree.  The item being detached is disconnected from
 * it's parent (or from the tree control itself if it has no parent).  The
 * releations between branches below inItem are maintained internally by
 * each child item.  However, inItem and all of it's children are removed
 * from the control hierarchy (they won't be drawn).  This is useful if you
 * want to move sections of the tree around without deleting anything.
 * @param inItem item to detach
 */
void CTreeControl::Detach(CTreeItem *inItem)
{
    inItem->Detach();

    if (inItem->IsRootItem())
        RemoveFromRootItemList(inItem);

    RemoveFromHierarchy(inItem, true);
}

//=============================================================================
/**
 * Allows you to enable or disable multiple selection for this tree control.
 * @param inAllow true to allow multiple selection, false to only allow single selection
 */
void CTreeControl::SetAllowMultipleSelection(bool inAllow)
{
    m_AllowMultipleSelection = inAllow;
}

//=============================================================================
/**
 * @return true if multiple selection is enabled, false if only single
 * selection is enabled.
 */
bool CTreeControl::GetAllowMultipleSelection()
{
    return m_AllowMultipleSelection;
}

//=============================================================================
/**
 * @return the number of items at the root level of this tree control
 */
long CTreeControl::GetNumRootItems()
{
    return (long)m_ItemList.size();
}

//=============================================================================
/**
 *	Get the root item at the specified index
 */
CTreeItem *CTreeControl::GetRootItem(long inIndex)
{
    CTreeItem::TItemList::iterator thePos = m_ItemList.begin();
    std::advance(thePos, inIndex);

    if (thePos != m_ItemList.end())
        return (*thePos);

    return nullptr;
}

//=============================================================================
/**
 * Gets iterators for traversing the list of selected tree items.
 * @param outListBegin returns an iterator to the beginning of the selected item list
 * @param outListEnd returns an iterator to the end of the selected item list
 */
void CTreeControl::GetSelectionIterators(CTreeItem::TItemList::iterator &outListBegin,
                                         CTreeItem::TItemList::iterator &outListEnd)
{
    outListBegin = m_SelectedItemList.begin();
    outListEnd = m_SelectedItemList.end();
}

//=============================================================================
/**
 * @return number of selected items in this tree control
 */
long CTreeControl::GetNumSelectedItems()
{
    return (long)m_SelectedItemList.size();
}

//=============================================================================
/**
 * @return the first selected item in the tree control
 */
CTreeItem *CTreeControl::GetFirstSelectedItem()
{
    CTreeItem::TItemList::iterator theBegin = m_SelectedItemList.begin();
    if (theBegin != m_SelectedItemList.end())
        return (*theBegin);
    return nullptr;
}

//=============================================================================
/**
 * Selects the specified tree item and adds the item to the selection list.
 * Multiple selection is handled via inKeyModifiers and setting of the multiple
 * selection flag on this class.
 * @param inItem item to be selected
 * @param inKeyModifiers optionally indicates the state of shift, ctrl, etc. for multiple selection
 */
void CTreeControl::Select(CTreeItem *inItem, long inKeyModifiers /*=0*/)
{
    ASSERT(inItem != nullptr);

    bool theCtrlKeyIsDown =
        (inKeyModifiers & CHotKeys::MODIFIER_CONTROL) == CHotKeys::MODIFIER_CONTROL;
    bool theShiftKeyIsDown =
        (inKeyModifiers & CHotKeys::MODIFIER_SHIFT) == CHotKeys::MODIFIER_SHIFT;

    // clears all existing selection if multiple selection is not allowed
    // If multiple selection is not enabled or neither control nor shift is down
    if (!m_AllowMultipleSelection || (!theCtrlKeyIsDown && !theShiftKeyIsDown)) {
        CTreeItem::TItemList::iterator thePos = m_SelectedItemList.begin();
        // Iterate through all the selected items and deselect them
        for (; thePos != m_SelectedItemList.end();) {
            CTreeItem *theItem = *thePos;
            if (theItem->IsSelected() && inItem != theItem) {
                theItem->SetSelected(false);
                thePos = m_SelectedItemList.erase(thePos); // remove unselected item from list
            } else
                ++thePos;
        }
    }

    // Don't bother selecting the item if it is already selected
    if (!inItem->IsSelected()) {
        inItem->SetSelected(true);
        m_SelectedItemList.push_back(inItem);

        // Tell all TreeItemSelectionListeners that the item was selected
        FireItemSelected(inItem);
    }

    if (theShiftKeyIsDown) {
        // TODO: advanced selection
    }
}

//=============================================================================
/**
 * Deselects the specified item, if it is selected.
 * @param inItem item to be deselected
 */
void CTreeControl::Deselect(CTreeItem *inItem)
{
    // Run through the list and look for the specified item
    bool theItemWasFound = false;
    CTreeItem::TItemList::iterator thePos = m_SelectedItemList.begin();
    CTreeItem::TItemList::iterator theEnd = m_SelectedItemList.end();
    for (; thePos != theEnd; ++thePos) {
        CTreeItem *theItem = *thePos;
        if (theItem == inItem) {
            theItemWasFound = true;
            break;
        }
    }

    // If the item was found, remove it from the selected item list
    if (theItemWasFound) {
        m_SelectedItemList.erase(thePos);
        // Tell the item it is no longer selected
        inItem->SetSelected(false);
    }
}

//=============================================================================
/**
 * Deselects all currently selected items.
 */
void CTreeControl::DeselectAll()
{
    CTreeItem::TItemList::iterator thePos = m_SelectedItemList.begin();
    CTreeItem::TItemList::iterator theEnd = m_SelectedItemList.end();
    for (; thePos != theEnd; ++thePos) {
        CTreeItem *theItem = *thePos;
        theItem->SetSelected(false);
    }
    m_SelectedItemList.clear();
}

//=============================================================================
/**
 * Expand all nodes to the specified item.
 */
void CTreeControl::ExpandTo(CTreeItem *inItem)
{
    // Turn off tree refreshing
    bool theAllowRefresh = GetAllowRefresh();
    SetAllowRefresh(false);

    // Expand the specified item
    inItem->Expand();

    // Expand the parents
    CTreeItem *theParent = inItem->GetParentItem();
    if (theParent)
        ExpandTo(theParent);

    // Restore tree refreshing
    SetAllowRefresh(theAllowRefresh);
}

//=============================================================================
/**
 * Expand all nodes from this item onwards.
 */
void CTreeControl::ExpandFrom(CTreeItem *inItem)
{
    // Turn off tree refreshing
    bool theAllowRefresh = GetAllowRefresh();
    SetAllowRefresh(false);

    // Expand the item's immediate children
    inItem->Expand();

    CTreeItem::TItemList::iterator theBegin;
    CTreeItem::TItemList::iterator theEnd;

    inItem->GetItemIterator(theBegin, theEnd);

    // Expand all children
    std::for_each(theBegin, theEnd, std::bind1st(std::mem_fun(&CTreeControl::ExpandFrom), this));

    // Restore refresh state
    SetAllowRefresh(theAllowRefresh);
}

//=============================================================================
/**
 * Collapse all items in the tree.
 */
void CTreeControl::CollapseAll()
{
    // Not implemented
    QT3DS_THROW(STUDIO_E_FAIL);
}

//=============================================================================
/**
 * Allows or disallows calls to RecalcLayout.  This is useful if you are adding
 * a bulk amount of items to the tree control at once.  You can simply call
 * this function with false, add your items, the call this function again with
 * true.  In this way, you don't incur a call to RecalcLayout for each item
 * added, just one call at the end when all the items are added.
 * @param inAllowRefresh true to allow RecalcLayout calls, false to prevent them
 * from doing anything.
 */
void CTreeControl::SetAllowRefresh(bool inAllowRefresh)
{
    m_Refresh = inAllowRefresh;
}

//=============================================================================
/**
 * Tells you whether or not a call to RecalcLayout will actually result in
 * resizing and repositioning the items in the tree.
 * @return true if the tree control is currently allowing recalc layout calls
 * to process, otherwise false.
 */
bool CTreeControl::GetAllowRefresh()
{
    return m_Refresh;
}

//=============================================================================
/**
 * Helper function for finding the tree item that is the next sibling after
 * the specified item.  If inItem has a parent, the parent's list is searched,
 * otherwise, we just search the root item list.  If there is no sibling, this
 * function recurses on the parent (if there is one) all the way up the tree
 * until we reach the top.  If there is not a control after the specified one,
 * this function returns nullptr.
 * @param inItem Item whose sibling we are searching for
 * @return the sibling that occurs after inItem in the tree hierarchy
 */
CTreeItem *CTreeControl::GetSiblingAfter(CTreeItem *inItem)
{
    UICPROFILE(GetSiblingAfter);

    ASSERT(inItem);

    CTreeItem *theSibling = nullptr;
    CTreeItem *theParent = inItem->GetParentItem();

    // Declare the iterators, then use them on either list
    CTreeItem::TItemList::reverse_iterator thePos;
    CTreeItem::TItemList::reverse_iterator theListEnd;

    // If we have a parent, get the iterators from the parent
    if (theParent) {
        theParent->GetReverseItemIterator(thePos, theListEnd);
    }
    // If we don't have a parent, we must be searching for a root-level item, so search the tree
    // control's list
    else {
        thePos = m_ItemList.rbegin();
        theListEnd = m_ItemList.rend();
    }

    // Iterate through the list
    bool theItemWasFound = false;
    CTreeItem *thePreviousItem = nullptr;
    for (; thePos != theListEnd; ++thePos) {
        CTreeItem *theCurrentItem = *thePos;

        // If we found the item we are looking for, break out of the loop
        if (theCurrentItem == inItem) {
            theItemWasFound = true;
            break;
        }

        thePreviousItem = theCurrentItem;
    }

    // If the item we were looking for was found, the sibling is probably contained in
    // thePreviousItem
    if (theItemWasFound)
        theSibling = thePreviousItem;

    return theSibling;
}

//=============================================================================
/**
 * Helper function for fetching the control after inItem.  If inItem has siblings,
 * those are searched to determine if there is a sibling after inItem.  If no
 * sibling is found, this function recurses, this time searching for a sibling
 * of inItem->GetParentItem( ); assuming that inItem has a parent.  In this
 * way, we determine the control that occurs after inItem in the Control
 * hierarchy.
 * @param inItem we are searching for the item after this one
 * @return the item after inItem in the control hierarchy or nullptr if there is no item after inItem.
 */
CTreeItem *CTreeControl::RecurseGetControlAfter(CTreeItem *inItem)
{
    UICPROFILE(RecurseGetControlAfter);

    ASSERT(inItem);

    CTreeItem *theControlAfter = GetSiblingAfter(inItem);
    CTreeItem *theParent = inItem->GetParentItem();

    // If we still haven't found a sibling, and we have a valid parent, recursively search for a
    // sibling of the parent
    if (!theControlAfter && theParent)
        theControlAfter = RecurseGetControlAfter(theParent);

    return theControlAfter;
}

//=============================================================================
/**
 * Protected function for adding items to the root level of this control.
 * @param inItem new item to be added
 * @param inInsertBefore item to insert inItem before in the list, or
 * nullptr to insert the item at the end of the list
 */
void CTreeControl::AddToRootItemList(CTreeItem *inItem, CTreeItem *inInsertBefore)
{
    // If an item to insert before is specified
    if (inInsertBefore) {
        // Go through the list and insert inItem before inInsertBefore
        CTreeItem::TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            CTreeItem *theCurrentItem = *thePos;
            if (theCurrentItem == inInsertBefore) {
                m_ItemList.insert(thePos, inItem);
                break;
            }
        }
    }
    // If no item is specified for inserting before, add inItem to the end of the list
    else {
        m_ItemList.push_back(inItem);
    }
}

//=============================================================================
/**
 * Protected function for removing items from the root level of this control.
 * The item is not deleted and nothing is done to the children of the item.
 * @param inItem item to be removed from the list
 */
void CTreeControl::RemoveFromRootItemList(CTreeItem *inItem)
{
    bool theItemWasFound = false;
    CTreeItem::TItemList::iterator thePos = m_ItemList.begin();
    for (; thePos != m_ItemList.end(); ++thePos) {
        CTreeItem *theItem = *thePos;
        if (theItem == inItem) {
            theItemWasFound = true;
            break;
        }
    }

    if (theItemWasFound) {
        m_ItemList.erase(thePos);
    }
}

//=============================================================================
/**
 * Protected function for adding items to the root level of this control.  No
 * lists are updated, this is merely for updating the control hierarchy.
 * @param inItem item to be added to the control hierarchy
 * @param inBefore item to insert inItem before in the control hierarchy or nullptr
 * to insert at the end of the control hierarchy.
 * @param inIsRecursive true to recursively add all children of inItem to hierarchy
 */
void CTreeControl::AddToHierarchy(CTreeItem *inItem, CTreeItem *inBefore, bool inIsRecursive)
{
    AddChild(inItem, inBefore);

    if (inIsRecursive) {
        bool theAllowRefresh = GetAllowRefresh();
        SetAllowRefresh(false);

        if (inItem->IsContainer()) {
            CTreeItem::TItemList::iterator thePos;
            CTreeItem::TItemList::iterator theEnd;
            inItem->GetItemIterator(thePos, theEnd);
            for (; thePos != theEnd; ++thePos) {
                CTreeItem *theChild = *thePos;
                AddToHierarchy(theChild, inBefore, inIsRecursive);
            }
        }

        SetAllowRefresh(theAllowRefresh);
    }
}

//=============================================================================
/**
 * Protected function for removing an item from the control hierarchy.  The
 * item is not deleted, but it will no longer be drawn.
 * @param inItem Item to remove
 * @param inIsRecursive true to recursively call this function on all of the item's children
 */
void CTreeControl::RemoveFromHierarchy(CTreeItem *inItem, bool inIsRecursive)
{
    if (inIsRecursive) {
        bool theAllowRefresh = GetAllowRefresh();
        SetAllowRefresh(false);

        if (inItem->IsContainer()) {
            CTreeItem::TItemList::iterator thePos;
            CTreeItem::TItemList::iterator theEnd;
            inItem->GetItemIterator(thePos, theEnd);
            for (; thePos != theEnd; ++thePos) {
                CTreeItem *theChild = *thePos;
                RemoveFromHierarchy(theChild, inIsRecursive);
            }
        }

        SetAllowRefresh(theAllowRefresh);
    }

    Deselect(inItem);
    RemoveChild(inItem);
}

//=============================================================================
/**
 * Private function for adding children to the Control hierarchy.  Only this
 * class should call this function.  Everyone else needs to go through AddItem( ).
 * @param inControl new control to be added
 * @param inInsertBefore control to insert the new one before in the hierarchy
 */
void CTreeControl::AddChild(CControl *inControl, CControl *inInsertBefore)
{
    Q3DStudio::Control::SVerticalLazyFlow::AddChild(inControl, inInsertBefore);
}

//==============================================================================
/**
 *	Add a CTreeItemSelectionListener to this control's list of
 *	CTreeItemSelectionListeners.
 *
 *	@param inListener CTreeItemSelectionListener to add to m_SelectionListeners
 */
void CTreeControl::AddSelectionListener(CTreeItemSelectionListener *inListener)
{
    m_SelectionListeners.AddListener(inListener);
}

//==============================================================================
/**
 *	Remove a CTreeItemSelectionListener from this control's list of
 *	CTreeItemSelectionListeners.
 *
 *	@param inListener CTreeItemSelectionListener to remove from m_SelectionListeners
 */
void CTreeControl::RemoveSelectionListener(CTreeItemSelectionListener *inListener)
{
    m_SelectionListeners.RemoveListener(inListener);
}

void CTreeControl::FireItemSelected(CTreeItem *inItem)
{
    OnTreeItemSelected(inItem);
    m_SelectionListeners.FireEvent(&CTreeItemSelectionListener::OnTreeItemSelected, inItem);
}
