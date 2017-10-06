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
#ifndef INCLUDED_TREE_CONTROL_H
#define INCLUDED_TREE_CONTROL_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "LazyFlow.h"
#include "TreeItem.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;
class CTest_TreeControl;

//==============================================================================
//	Functor
//==============================================================================
///	This means that any class that wants be informed when TreeItems are selected
///	needs to implement the CTreeItemSelectionListener interface, and the
///	void OnTreeItemSelected( CTreeItem* ) method
/// The CTreeItem is the item that was selected
GENERIC_FUNCTOR_1(CTreeItemSelectionListener, OnTreeItemSelected, CTreeItem *);

//=============================================================================
/**
 * Custom tree control class.  You need to call one of the AddItem*( ) functions
 * to add tree items to this control.  Multiple selection can be enabled using
 * the provided functions.  The tree control maintains a list of all the root
 * level items.  It is also responsible for managing the separate "Control
 * hierarchy."
 */
class CTreeControl : public Q3DStudio::Control::SVerticalLazyFlow
{
public:
    CTreeControl();
    virtual ~CTreeControl();
    void SetBackgroundColor(const ::CColor &inColor);
    void Draw(CRenderer *inRenderer) override;
    virtual void AddItem(CTreeItem *inParent, CTreeItem *inChild);
    virtual void AddItemBefore(CTreeItem *inParent, CTreeItem *inChild, CTreeItem *inBefore);
    virtual void AddItemAfter(CTreeItem *inParent, CTreeItem *inChild, CTreeItem *inAfter);
    virtual void RemoveItem(CTreeItem *inItem);
    virtual void RemoveSelectedItems();
    virtual void RemoveAllItems();
    virtual void Detach(CTreeItem *inItem);
    void SetAllowMultipleSelection(bool inAllow);
    bool GetAllowMultipleSelection();
    void GetSelectionIterators(CTreeItem::TItemList::iterator &outListBegin,
                               CTreeItem::TItemList::iterator &outListEnd);
    long GetNumSelectedItems();
    CTreeItem *GetFirstSelectedItem();
    virtual void Select(CTreeItem *inItem, long inKeyModifiers = 0);
    virtual void Deselect(CTreeItem *inItem);
    virtual void DeselectAll();
    void SetAllowRefresh(bool inAllowRefresh);
    bool GetAllowRefresh();
    long GetNumRootItems();
    CTreeItem *GetRootItem(long inIndex);
    virtual void ExpandTo(CTreeItem *inItem);
    virtual void ExpandFrom(CTreeItem *inItem);
    virtual void CollapseAll();
    virtual void SetSorting(bool inSort) { m_SortFlag = inSort; }
    virtual bool IsSorting() { return m_SortFlag; }

    virtual bool IsItemGreater(CTreeItem *, CTreeItem *) { return true; }
    virtual bool IsItemLess(CTreeItem *, CTreeItem *) { return false; }

    // SelectionListener methods
    void AddSelectionListener(CTreeItemSelectionListener *inListener);
    void RemoveSelectionListener(CTreeItemSelectionListener *inListener);
    void FireItemSelected(CTreeItem *inItem);

    // Override to hear select
    virtual void OnTreeItemSelected(CTreeItem *) {}
    virtual void OnItemExpanding(CTreeItem *) {}
    virtual void OnItemExpanded(CTreeItem *) { NotifyParentNeedsLayout(); }
    virtual void OnItemCollapsing(CTreeItem *) {}
    virtual void OnItemCollapsed(CTreeItem *) { NotifyParentNeedsLayout(); }

protected:
    // Member Vars
    ::CColor m_BGColor; ///< Specifies the background color of this control
    bool m_AllowMultipleSelection; ///< If true, multiple selection is enabled for this control
    CTreeItem::TItemList m_ItemList; ///< List of all root level tree items
    CTreeItem::TItemList m_SelectedItemList; ///< List of all selected tree items, regardless of
                                             ///whether or not they are root level
    bool m_Refresh;
    bool m_SortFlag;

    // Member Functions
    CTreeItem *GetSiblingAfter(CTreeItem *inItem);
    CTreeItem *RecurseGetControlAfter(CTreeItem *inItem);
    CTreeItem *FindPrevSortSibling(CTreeItem *inParent, CTreeItem *inChild);
    CTreeItem *FindNextSortSibling(CTreeItem *inParent, CTreeItem *inChild);
    void AddToRootItemList(CTreeItem *inItem, CTreeItem *inInsertBefore);
    void RemoveFromRootItemList(CTreeItem *inItem);
    void AddToHierarchy(CTreeItem *inItem, CTreeItem *inBefore, bool inIsRecursive);
    void RemoveFromHierarchy(CTreeItem *inItem, bool inIsRecursive);

private:
    /// Private because you can only add CTreeItems to this control with AddItem( ).
    void AddChild(CControl *inControl, CControl *inInsertBefore = nullptr) override;

    CMulticaster<CTreeItemSelectionListener *>
        m_SelectionListeners; ///< List of listeners who want to know when a TreeItem is selected
};
#endif // INCLUDED_TREE_CONTROL_H
