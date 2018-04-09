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
#include "TreeItem.h"
#include "TreeControl.h"
#include "Renderer.h"
#include "SIcon.h"
#include "ResourceCache.h"
#include "HotKeys.h"
#include "MasterP.h"
#include "StudioPreferences.h"

IMPLEMENT_OBJECT_COUNTER(CTreeItem)

//=============================================================================
/**
 * Constructor
 * @param inTreeControl the tree control that this item will belong to
 */
CTreeItem::CTreeItem(CTreeControl *inTreeControl)
    : m_TreeControl(inTreeControl)
    , m_Parent(NULL)
    , m_Toggle(NULL)
    , m_Name(NULL)
    , m_Icon(NULL)
    , m_ControlGap(2)
    , m_IndentLevel(0)
    , m_IndentSize(0)
    , m_IsExpanded(false)
    , m_IsSelected(false)
    , m_ShowVisibleSelection(true)
    , m_Color(CStudioPreferences::GetBaseColor())
    , m_TextColor(CStudioPreferences::GetNormalColor())
{
    ADDTO_OBJECT_COUNTER(CTreeItem)

    // Set up some of our flow layout properties
    SetFlowDirection(FLOW_HORIZONTAL);
    SetAlignment(ALIGN_VERT_NEITHER, ALIGN_LEFT);
    SetGapBetweenChildren(m_ControlGap);
    SetMaximumSize(CPt(LONG_MAX, CStudioPreferences::GetDefaultTextEditSize()));
    SetMinimumSize(CPt(0, CStudioPreferences::GetDefaultTextEditSize()));

    // Create the toggle
    CreateToggle();

    // Icon control
    m_Icon = new CSIcon();

    // Name control
    m_Name = new CTextEditInPlace();
    m_Name->SetAlignment(CTextEditInPlace::LEFT);
    m_Name->SetTextColor(m_TextColor);
    m_Name->SetBGColorNoFocus(CStudioPreferences::GetSelectColor());
    m_Name->SetFillBackground(false);
    m_Name->AllowAutoSize(true);

    // Name change listener
    m_NameChangeListener =
        new CSpecificCCommitDataListener<CTreeItem>(this, &CTreeItem::OnNameChanged);
    m_Name->AddCommitListener(m_NameChangeListener);

    // Toggle listener
    QObject::connect(m_Toggle,&CToggleButton::SigToggle,
                     std::bind(&CTreeItem::OnToggleExpansion, this,
                               std::placeholders::_1, std::placeholders::_2));
    // Add all the controls to the flow
    AddChild(m_Toggle);
    AddChild(m_Icon);
    AddChild(m_Name);
}

//=============================================================================
/**
 * Destructor
 */
CTreeItem::~CTreeItem()
{
    m_Name->RemoveCommitListener(m_NameChangeListener);
    delete m_NameChangeListener;

    // Clear out our list of subitems, but don't delete anything (whoever called new should do the
    // delete)
    m_ItemList.clear();
    delete m_Toggle;
    delete m_Icon;
    delete m_Name;

    REMOVEFROM_OBJECT_COUNTER(CTreeItem)
}

//=============================================================================
/**
 * Fills in the background color of this control.
 * @param inRenderer renderer to draw to
 */
void CTreeItem::Draw(CRenderer *inRenderer)
{
    inRenderer->FillSolidRect(QRect({}, GetSize()), m_Color);
}

//=============================================================================
/**
 * Allows you to set the background color of this control.
 * @param inColor new background color for this control
 */
void CTreeItem::SetBackgroundColor(const CColor &inColor)
{
    m_Color = inColor;
    Invalidate();
}

void CTreeItem::SetTextColor(const CColor &inColor)
{
    m_Name->SetTextColor(inColor);
    m_TextColor = inColor;
    Invalidate();
}

void CTreeItem::SetTextBGFocusColor(const ::CColor &inColor)
{
    m_Name->SetBGColorNoFocus(inColor);
    Invalidate();
}
//=============================================================================
/**
 * @return the current background color of this control
 */
CColor CTreeItem::GetBackgroundColor()
{
    return m_Color;
}

//=============================================================================
/**
 * Sets the parent of this tree item.  The parent is the tree item that can
 * be toggled to hide or show this item.  You can set this value to nullptr to
 * specify that this item is a root item, thus making it always be visible.
 * @param inParent parent of this item
 */
void CTreeItem::SetParentItem(CTreeItem *inParent)
{
    m_Parent = inParent;
}

//=============================================================================
/**
 * Allows you to access the parent of this tree item.  Note that this parent is
 * not the same as parenting in the Control hierarchy.  The parent is the tree
 * item that can be toggled to hide or show this item.
 * @return the parent of this tree item, or nullptr if this is a root level item.
 */
CTreeItem *CTreeItem::GetParentItem()
{
    return m_Parent;
}

//=============================================================================
/**
 * Allows you to change the icon associated with this tree item.
 * @param inIcon name of the icon in normal state
 * @param inSelectedIcon name of the icon to be displayed when this item is selected
 */
void CTreeItem::SetIcon(const QString &inIcon, const QString &inSelectedIcon)
{
    SetIcon(CResourceCache::GetInstance()->GetBitmap(inIcon),
            CResourceCache::GetInstance()->GetBitmap(inSelectedIcon));
}

//=============================================================================
/**
 * Allows you to change the icon associated with this tree item.
 */
void CTreeItem::SetIcon(const QPixmap &inNormalIcon, const QPixmap &inSelectedIcon)
{
    m_NormalIcon = inNormalIcon;
    m_SelectedIcon = inSelectedIcon;

    m_Icon->SetImage(m_NormalIcon);
}

//=============================================================================
/**
 * Allows you to specifiy whether or not the name of this tree item can be edited.
 * If set to true, the user can edit an item's name by double-clicking on it.
 * @param inIsEditable true to specify that the user is allowed to edit this
 * item, otherwise false to prevent double-click editing.
 */
void CTreeItem::SetIsNameEditable(bool inIsEditable)
{
    m_Name->SetEditable(inIsEditable);
}

//=============================================================================
/**
 * Sets the text displayed by this tree item.  This does not trigger a name
 * change event.
 * @param inText text to be displayed
 */
void CTreeItem::SetText(const Q3DStudio::CString &inText)
{
    m_Name->SetData(inText);
}

//=============================================================================
/**
 * Gets the text displayed by this tree item.
 * @return the text that is displayed.
 */
Q3DStudio::CString CTreeItem::GetText()
{
    return m_Name->GetString();
}

//=============================================================================
/**
 * Called when the name of the tree item is changed.  Notifies any interested
 * listeners.  This is only for when the name control is edited directly.  If
 * you call SetText directly or change the text by other means, no listeners
 * are notified.
 * @param inControl control that generated the name change message (not used)
 */
void CTreeItem::OnNameChanged(CControl *inControl)
{
    Q_UNUSED(inControl);

    // Maintain the color scheme based upon whether or not this item is selected
    if (m_ShowVisibleSelection) {
        if (m_IsSelected) {
            m_Name->SetFillBackground(true);
            m_Name->SetTextColor(CColor(255, 255, 255));
        } else {
            m_Name->SetFillBackground(false);
            m_Name->SetTextColor(m_TextColor);
        }
    }

    // TODO: notify listeners
}

//=============================================================================
/**
 * Allows you to specify the indent level of this tree item.  Generally, this
 * is called automatically when adding a child item.  An item at level 0 is a
 * root item, an item at level 1 is a child of a root item, and so on.  The
 * indent level is multiplied by a fixed amount to cause the control to appear
 * indented.
 * @param inAmount number indicating how many children deep this item is
 * @param inIsRecursive set to true to recursively set the indent level of all
 * children in addition to this item.  Each child is set at one more indent
 * level than its parent.
 */
void CTreeItem::SetIndent(long inAmount, bool inIsRecursive)
{
    m_IndentLevel = inAmount;

    SetLeftMargin((m_IndentLevel * m_IndentSize) + m_ControlGap);

    if (inIsRecursive) {
        TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            CTreeItem *theChild = *thePos;
            theChild->SetIndent(m_IndentLevel + 1, inIsRecursive);
        }
    }
}

//=============================================================================
/**
 * Handles clicking on this item and selects or deselects it appropriately.
 * @param inPoint location of the mouse
 * @param inFlags key modifier states at time of mouse event
 */
bool CTreeItem::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CFlowLayout::OnMouseDown(inPoint, inFlags)) {
        // If the name is being edited, but was not clicked on, force it to lose focus
        if (m_Name->GetEditMode() && !m_Name->HitTest(inPoint))
            m_Name->OnLoseFocus();

        // The item needs to see if the control key is down, because that might mean that it needs
        // to deselect itself
        bool theControlKeyIsDown =
            (inFlags & CHotKeys::MODIFIER_CONTROL) == CHotKeys::MODIFIER_CONTROL;

        // If the control key is down and we are currently selected, deselect ourselves
        if (theControlKeyIsDown && IsSelected())
            m_TreeControl->Deselect(this);
        // Otherwise, go ahead and perform a normal select command
        else
            m_TreeControl->Select(this, inFlags);
    }

    return true;
}

//=============================================================================
/**
 * If this item is a container, double-click will expand it or collapse it.
 * @param inPoint location of mouse at time of event
 * @param inFlags modifier flags at time of event
 */
bool CTreeItem::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // Let the children have the chance to handle the message
    bool theMessageWasHandled = CFlowLayout::OnMouseDoubleClick(inPoint, inFlags);

    // If no one else handled it, we'll just go ahead and expand
    if (!theMessageWasHandled) {
        // If this item is a container, toggle whether or not it's expanded
        if (IsContainer())
            ToggleExpansion();
    }

    return theMessageWasHandled;
}

//=============================================================================
/**
 * Handles right clicking on this item and selects or deselects it appropriately.
 * @param inPoint location of the mouse
 * @param inFlags key modifier states at time of mouse event
 * @return true if the event was consumed, false if it should continue to propagate
 */
bool CTreeItem::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theMessageWasHandled = CFlowLayout::OnMouseRDown(inPoint, inFlags);
    if (!theMessageWasHandled) {
        // If the name is being edited, but was not clicked on, force it to lose focus
        if (m_Name->GetEditMode() && !m_Name->HitTest(inPoint))
            m_Name->OnLoseFocus();

        // If this item is not already selected
        if (!IsSelected()) {
            // Select this tree item
            m_TreeControl->Select(this, inFlags);
        }
    }

    return theMessageWasHandled;
}

//=============================================================================
/**
 * @return true if this row is currently expanded (showing it's children).
 */
bool CTreeItem::IsExpanded()
{
    return m_IsExpanded;
}

//=============================================================================
/**
 * If the row is currently expanded, this call will collapse it, or vice-versa.
 */
void CTreeItem::ToggleExpansion()
{
    if (IsExpanded())
        Collapse();
    else
        Expand();
}

//=============================================================================
/**
 * Expands this item to reveal all of its children.
 */
void CTreeItem::Expand()
{
    QT3DS_PROFILE(Expand);

    if (!m_IsExpanded) {
        m_TreeControl->OnItemExpanding(this);

        bool theAllowRefresh = m_TreeControl->GetAllowRefresh();
        m_TreeControl->SetAllowRefresh(false);

        TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            CTreeItem *theItem = *thePos;
            theItem->ShowChildItems();
        }

        m_IsExpanded = true;

        m_TreeControl->SetAllowRefresh(theAllowRefresh);

        // Expand/collapse might come from somewhere other than the button itself (see
        // CTreeItem::OnMouseDoubleClick),
        // so make sure we toggle it just in case
        m_Toggle->SetToggleState(true);

        m_TreeControl->OnItemExpanded(this);
    }
}

//=============================================================================
/**
 * Collapses this row, hiding all of its children
 */
void CTreeItem::Collapse()
{
    if (m_IsExpanded) {
        m_TreeControl->OnItemCollapsing(this);

        bool theAllowRefresh = m_TreeControl->GetAllowRefresh();
        m_TreeControl->SetAllowRefresh(false);

        TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            CTreeItem *theItem = *thePos;
            theItem->HideChildItems();
        }

        m_IsExpanded = false;

        // Deselect all children if they are being hidden and see if any children were in fact
        // selected
        // If there were children that were selected, we need to add this item to the selection
        // list, without
        // interfering with the selection state of other items
        if (DeselectChildren()) {
            long theFlags = 0;

            // If multiple selection is enabled, we need to make sure that we don't disrupt any
            // other selection,
            // so we simulate a control click on this item (selects this item in addition to
            // whatever is already
            // selected).
            if (m_TreeControl->GetAllowMultipleSelection())
                theFlags = CHotKeys::MODIFIER_CONTROL;

            m_TreeControl->Select(this, theFlags);
        }

        m_TreeControl->SetAllowRefresh(theAllowRefresh);

        // Expand/collapse might come from somewhere other than the button itself (see
        // CTreeItem::OnMouseDoubleClick),
        // so make sure we toggle it just in case
        m_Toggle->SetToggleState(false);

        m_TreeControl->OnItemCollapsed(this);
    }
}

//=============================================================================
/**
 * Adds a tree item as a child of this one.
 * @param inItem the new item to be added.
 * @param inInsertBefore the item to insert the new one before, or nullptr to
 * insert at the end of the list
 */
void CTreeItem::AddItem(CTreeItem *inItem, CTreeItem *inInsertBefore /*=nullptr*/)
{
    inItem->SetIndent(m_IndentLevel + 1, true);

    inItem->SetVisible(IsExpanded());

    if (!inInsertBefore) {
        m_ItemList.push_back(inItem);
    } else {
        TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            CTreeItem *theItem = *thePos;
            if (theItem == inInsertBefore) {
                m_ItemList.insert(thePos, inItem);
                break;
            }
        }
    }

    inItem->SetParentItem(this);

    UpdateToggle();

    Invalidate();
}

//=============================================================================
/**
 * The number of children this item has.  Note that this is different than
 * the GetChildCount( ) function on the base class CControl.  The "children"
 * counted by this function are the ones that have been added to this control
 * to form a tree by calls to AddItem.
 * @return the number of items added a "child tree items" to this one.
 */
long CTreeItem::GetNumChildItems()
{
    return (long)m_ItemList.size();
}

//=============================================================================
/**
 *  Retrieve the child tree item based on it's position within m_ItemList
 *  @param inIndex position of the child tree item
 *  @return CTreeItem* ptr to child tree item or nullptr if inIndex is invalid.
 */
CTreeItem *CTreeItem::GetChildItem(long inIndex)
{
    CTreeItem::TItemList::iterator thePos = m_ItemList.begin();
    std::advance(thePos, inIndex);

    if (thePos != m_ItemList.end())
        return (*thePos);

    return nullptr;
}

//=============================================================================
/**
 *	Retrieve the index of a child tree item based on it's position within
 *	m_ItemList.
 *	@param inChildItem ptr to the child tree item
 *	@return long index to the child tree item or -1 if inChildItem is invalid.
 */
long CTreeItem::GetChildTreeItemIndex(const CTreeItem *inChildItem) const
{
    TItemList::const_iterator thePos = m_ItemList.begin();
    TItemList::const_iterator theEnd = m_ItemList.end();

    long theItemIndex(-1);
    bool theIsFound(false);
    for (; thePos != theEnd && !theIsFound; ++thePos) {
        ++theItemIndex;

        if ((*thePos) == inChildItem)
            theIsFound = true;
    }

    if (theIsFound)
        return theItemIndex;
    else
        return -1;
}

//=============================================================================
/**
 * Retrieves the necessary iterators for searching through this item's list
 * of children.  You should not need to access this directly.  This is provided
 * for use by the CTreeControl class.
 * @param outListBegin returns an iterator pointing to the first element in the list
 * @param outListEnd returns an iterator pointing to the last element
 */
void CTreeItem::GetItemIterator(TItemList::iterator &outListBegin, TItemList::iterator &outListEnd)
{
    outListBegin = m_ItemList.begin();
    outListEnd = m_ItemList.end();
}

//=============================================================================
/**
 * Retrieves the necessary iterators for searching through this item's list
 * of children (in reverse order).  You should not need to access this directly.
 * This is provided for use by the CTreeControl class.
 * @param outListBegin returns a reverse iterator pointing to the last element in the list
 * @param outListEnd returns a reverse iterator pointing to the first element
 */
void CTreeItem::GetReverseItemIterator(TItemList::reverse_iterator &outListBegin,
                                       TItemList::reverse_iterator &outListEnd)
{
    outListBegin = m_ItemList.rbegin();
    outListEnd = m_ItemList.rend();
}

//=============================================================================
/**
 * An item that is a container has a visible toggle that can be clicked to
 * hide or show its children.
 * @return true if this item is a parent and has children, otherwise false
 */
bool CTreeItem::IsContainer()
{
    return m_ItemList.size() > 0;
}

//=============================================================================
/**
 * @return true if this item is a root level item, false if it has a parent
 */
bool CTreeItem::IsRootItem()
{
    return (!m_Parent);
}

//=============================================================================
/**
 * Removes this tree item and all its children from the tree control.  Recursively
 * goes through all children and asks them to detach themselves as children of
 * the tree control.  No items are actually deleted, as this is deferred to
 * the class that created the items.
 */
void CTreeItem::RemoveItem()
{
    // Enable or disable the toggle on the parent accordingly
    if (m_Parent)
        m_Parent->UpdateToggle();

    // Recursively remove all children
    RemoveChildren();

    // If this item has a parent, ask the parent to remove us from it's list of children
    if (m_Parent)
        m_Parent->RemoveFromList(this);

    // Remove this item from the control hierarchy
    m_TreeControl->RemoveChild(this);
}

//=============================================================================
/**
 * Updates the visibility of the expand toggle.  If the the item is a container,
 * the toggle is shown, otherwise it is hidden.
 */
void CTreeItem::UpdateToggle()
{
    QT3DS_PROFILE(UpdateToggle);

    bool theEnableToggle = IsContainer();
    bool theToggleIsEnabled = m_Toggle->IsEnabled();

    if (theEnableToggle != theToggleIsEnabled) {
        m_Toggle->SetEnabled(theEnableToggle);
        Invalidate();
    }
}

//=============================================================================
/**
 * Recursively removes all children from the child list and from the control
 * hierarchy maintained by the tree control.
 */
void CTreeItem::RemoveChildren()
{
    // If this item has children
    if (IsContainer()) {
        // Iterate through each child
        TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            // And recursively remove the child's children
            CTreeItem *theItem = *thePos;
            theItem->RemoveChildren();

            // Ask the tree control to remove this child from the control hierarchy
            m_TreeControl->RemoveChild(theItem);
        }

        // Clear the child list since we deleted all the children
        m_ItemList.clear();
    }
}

//=============================================================================
/**
 * Helper function for item removal.  Goes through the list of child items that
 * belongs to this control and removes the specified item from the list.
 * @param inItem item to be removed from list
 */
void CTreeItem::RemoveFromList(CTreeItem *inItem)
{
    TItemList::iterator thePos = m_ItemList.begin();
    for (; thePos != m_ItemList.end(); ++thePos) {
        CTreeItem *theItem = *thePos;
        if (theItem == inItem) {
            m_ItemList.erase(thePos);
            break;
        }
    }
}

//=============================================================================
/**
 * Detaches this item from its parent.
 */
void CTreeItem::Detach()
{
    if (m_Parent)
        m_Parent->RemoveFromList(this);
}

//=============================================================================
/**
 * @return true if this item is currently selected, otherwise false.
 */
bool CTreeItem::IsSelected()
{
    return m_IsSelected;
}

//=============================================================================
/**
 * Selects or deselects this row.  Color of the selected item changes, as well
 * as the item's icon.
 * @param inIsSelected true to select this item, false to deselect it
 */
void CTreeItem::SetSelected(bool inIsSelected)
{
    // If this command is actually resulting in a change of state
    if (m_IsSelected != inIsSelected) {
        m_IsSelected = inIsSelected;

        if (m_ShowVisibleSelection) {
            // If the item is to be selected
            if (m_IsSelected) {
                // Change to the selection color
                m_Name->SetFillBackground(true);

                if (!m_SelectedIcon.isNull())
                    m_Icon->SetImage(m_SelectedIcon);
            }
            // Otherwise, the item is to be deselected
            else {
                // Change to the normal color scheme
                m_Name->SetFillBackground(false);
                m_Name->SetTextColor(m_TextColor);

                if (!m_NormalIcon.isNull())
                    m_Icon->SetImage(m_NormalIcon);
            }
        }
    }
}

//=============================================================================
/**
 * Deselects all children, grandchildren, and so on who are currently selected.
 * @return true if there were any selected children who were deselected
 */
bool CTreeItem::DeselectChildren()
{
    bool theChildSelection = false;

    TItemList::iterator thePos = m_ItemList.begin();
    for (; thePos != m_ItemList.end(); ++thePos) {
        CTreeItem *theChild = *thePos;

        if (theChild->IsContainer()) {
            bool theRecursiveCheck = theChild->DeselectChildren();
            if (!theChildSelection)
                theChildSelection = theRecursiveCheck;
        }

        if (theChild->IsSelected()) {
            m_TreeControl->Deselect(theChild);
            theChildSelection = true;
        }
    }

    return theChildSelection;
}

//=============================================================================
/**
 * Creates the toggle control used by this tree item.
 */
void CTreeItem::CreateToggle()
{
    m_Toggle = new CToggleButton();

    // Make sure these sizes are accurate
    QSize theImageSize = CResourceCache::GetInstance()->GetBitmap("arrow.png").size();
    m_Toggle->SetUpImage("arrow.png");

    m_Toggle->SetDownImage("arrow_down.png");
    m_Toggle->SetDisabledImage("empty-pixel.png");
    m_Toggle->SetAbsoluteSize({theImageSize.width(), theImageSize.height()});
    m_Toggle->SetEnabled(false);

    // This is to get around a problem in SetIndent when the parent control is 0
    m_IndentSize = theImageSize.width();
}

//=============================================================================
/**
 * Called when the toggle button for this item is clicked on.  Expands the item
 * if is currently collapsed, otherwise, collapses the row.
 * @param inButton button that generated this event
 * @param inState state of the button after the toggle event
 */
void CTreeItem::OnToggleExpansion(CToggleButton *inButton, CButtonControl::EButtonState inState)
{
    Q_UNUSED(inState);

    if (inButton == m_Toggle)
        ToggleExpansion();
}

//=============================================================================
/**
 * Recursive function to help change the visiblity of all children of this item.
 */
void CTreeItem::HideChildItems()
{
    TItemList::iterator thePos = m_ItemList.begin();
    for (; thePos != m_ItemList.end(); ++thePos) {
        CTreeItem *theItem = *thePos;
        theItem->HideChildItems();
    }

    SetVisible(false);
}

//=============================================================================
/**
 * Recursive function to help change the visiblity of all children of this item.
 * If a child is expanded, this function is called recursively to reveal all of
 * the grandchildren, and so on.
 */
void CTreeItem::ShowChildItems()
{
    if (m_Parent)
        SetVisible(m_Parent->IsVisible());
    else
        SetVisible(true);

    TItemList::iterator thePos = m_ItemList.begin();
    for (; thePos != m_ItemList.end(); ++thePos) {
        CTreeItem *theItem = *thePos;
        if (IsExpanded())
            theItem->ShowChildItems();
    }
}

//=============================================================================
/**
 * Private function for adding children to this control.  The only children
 * should be a toggle, an icon, and a text control for the name.  If you
 * need to add another tree item as a "child" of this one, you should use
 * AddItem( ).
 * @param inControl Control to be added as a child to this one
 * @param inInsertBefore Control to insert before in the hierarchy or nullptr to
 * insert at the end of the list.
 */
void CTreeItem::AddChild(CControl *inControl, CControl *inInsertBefore /*= nullptr*/)
{
    CFlowLayout::AddChild(inControl, inInsertBefore);
}

//=============================================================================
/**
 * Find the item that this item should be added after to maintain sort order.
 */
CTreeItem *CTreeItem::FindPrevSortSibling(CTreeItem *inChild)
{
    CTreeItem *theSortItem = nullptr;

    // No items in the list - return nullptr (because we don't care)
    if (m_ItemList.size()) {
        CTreeItem::TItemList::iterator thePos = m_ItemList.end();
        for (; thePos != m_ItemList.begin(); --thePos) {
            // Item in the list
            theSortItem = *thePos;
            if (m_TreeControl->IsItemLess(inChild, theSortItem))
                break;
        }

        // Last item in the list
        if (thePos == m_ItemList.begin())
            theSortItem = nullptr;
    }

    return theSortItem;
}

//=============================================================================
/**
 * Find the item that this item should be added before to maintain sort order.
 */
CTreeItem *CTreeItem::FindNextSortSibling(CTreeItem *inChild)
{
    CTreeItem *theSortItem = nullptr;

    // No items in the list - return nullptr (because we don't care)
    if (m_ItemList.size()) {
        CTreeItem::TItemList::iterator thePos = m_ItemList.begin();
        for (; thePos != m_ItemList.end(); ++thePos) {
            // Item in the list
            theSortItem = *thePos;
            if (m_TreeControl->IsItemLess(inChild, theSortItem))
                break;
        }

        // Last item in the list
        if (thePos == m_ItemList.end())
            theSortItem = nullptr;
    }

    return theSortItem;
}

/**
 * Retrieve the edit state of the text edit control associated with this tree item
 */
bool CTreeItem::GetEditMode()
{
    if (m_Name) {
        return m_Name->GetEditMode();
    }

    return false;
}

//=============================================================================
/**
 * Sets whether item is 'visibly' selected
 */
void CTreeItem::SetVisibleSelection(bool inValue)
{
    m_ShowVisibleSelection = inValue;
}

//=============================================================================
/**
 * Gets whether item is 'visibly' selected
 */
bool CTreeItem::GetVisibleSelection() const
{
    return m_ShowVisibleSelection;
}
