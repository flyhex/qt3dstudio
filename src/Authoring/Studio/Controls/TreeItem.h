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
#ifndef INCLUDED_TREE_ITEM_H
#define INCLUDED_TREE_ITEM_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "FlowLayout.h"
#include "ToggleButton.h"
#include "TextEditInPlace.h"

#include <QPixmap>
//==============================================================================
//	Forwards
//==============================================================================
class CTreeControl;
class CRenderer;
class CSIcon;
class CTreeItem;
class CTest_TreeControl;

//=============================================================================
/**
 * Class encapsulating a single row in a tree control.  Manages the toggle, the
 * icon, and the name of the tree item.  When you create a tree item you must
 * associate it with a tree control.  Then you can specify it's parent when you
 * call AddItem( ).
 */
class CTreeItem : public CFlowLayout
{
public:
    typedef std::vector<CTreeItem *> TItemList;

public:
    CTreeItem(CTreeControl *inTreeControl);
    virtual ~CTreeItem();

    DEFINE_OBJECT_COUNTER(CTreeItem)

    void Draw(CRenderer *inRenderer) override;

    void SetBackgroundColor(const ::CColor &inColor);
    ::CColor GetBackgroundColor();
    void SetTextColor(const ::CColor &inColor);
    void SetTextBGFocusColor(const ::CColor &inColor);

    void SetParentItem(CTreeItem *inParent);
    CTreeItem *GetParentItem();
    bool GetEditMode();

    virtual void SetIcon(const QString &inIcon,
                         const QString &inSelectedIcon);
    virtual void SetIcon(const QPixmap &inNormalIcon, const QPixmap &inSelectedIcon);
    void SetIsNameEditable(bool inIsEditable);
    virtual void SetText(const Q3DStudio::CString &inText);
    virtual Q3DStudio::CString GetText();
    virtual void OnNameChanged(CControl *inControl);
    void SetIndent(long inAmount, bool inIsRecursive);

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    bool IsExpanded();
    virtual void ToggleExpansion();
    virtual void Expand();
    virtual void Collapse();

    virtual void AddItem(CTreeItem *inItem, CTreeItem *inInsertBefore = nullptr);
    long GetNumChildItems();
    CTreeItem *GetChildItem(long inIndex);
    long GetChildTreeItemIndex(const CTreeItem *inChildItem) const;
    void GetItemIterator(TItemList::iterator &outListBegin, TItemList::iterator &outListEnd);
    void GetReverseItemIterator(TItemList::reverse_iterator &outListBegin,
                                TItemList::reverse_iterator &outListEnd);
    virtual bool IsContainer();
    bool IsRootItem();
    virtual void RemoveItem();
    void UpdateToggle();
    virtual void RemoveChildren();
    void RemoveFromList(CTreeItem *inItem);
    virtual void Detach();
    virtual CTreeItem *FindPrevSortSibling(CTreeItem *inChild);
    virtual CTreeItem *FindNextSortSibling(CTreeItem *inChild);

    bool IsSelected();
    virtual void SetSelected(bool inIsSelected);
    bool DeselectChildren();

    void SetVisibleSelection(bool inValue);
    bool GetVisibleSelection() const;

    boost::signal1<void, CControl *> SigToggle;

protected:
    // Member Vars
    CTreeControl *m_TreeControl;
    CTreeItem *m_Parent;
    CToggleButton *m_Toggle;
    CTextEditInPlace *m_Name;
    CCommitDataListener *m_NameChangeListener;
    CSIcon *m_Icon;
    QPixmap m_NormalIcon;
    QPixmap m_SelectedIcon;
    TItemList m_ItemList;
    long m_ControlGap;
    long m_IndentLevel;
    long m_IndentSize;
    bool m_IsExpanded;
    bool m_IsSelected;
    bool m_ShowVisibleSelection;
    ::CColor m_Color;
    ::CColor m_TextColor;

    // Member Functions
    void CreateToggle();
    void OnToggleExpansion(CToggleButton *inButton, CButtonControl::EButtonState inState);
    void HideChildItems();
    void ShowChildItems();

private:
    /// Private because no one outside this class should be calling it; use AddItem( ) instead.
    void AddChild(CControl *inControl, CControl *inInsertBefore = nullptr) override;
};
#endif // INCLUDED_TREE_ITEM_H
