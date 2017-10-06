/****************************************************************************
**
** Copyright (C) 2005 NVIDIA Corporation.
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
#ifndef INCLUDED_LIST_BOX_ITEM_H
#define INCLUDED_LIST_BOX_ITEM_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "FlowLayout.h"
#include "Multicaster.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;
class CListBoxItem;

//==============================================================================
//	Functor
//==============================================================================
GENERIC_FUNCTOR_2(CListBoxSelectionListener, OnSelectItem, CListBoxItem *, long);

//==============================================================================
/**
 * abstract CListBoxItem base class that all other ListBoxItem classes
 * must inherit from. e.g. CListBoxStringItem, CActionListBoxItem
 */
class CListBoxItem : public CControl
{
public:
    CListBoxItem();
    virtual ~CListBoxItem();

    DEFINE_OBJECT_COUNTER(CListBoxItem)

    virtual Q3DStudio::CString GetString() = 0; ///< returns the string representation of this item

    void Draw(CRenderer *inRenderer) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnParentChanged(CControl *inParent) override;
    void SetMinimumSize(CPt inSize) override;

    virtual void SetSelectedState(bool inSelected);
    virtual void RecalcMinimumSize();
    bool IsSelected() const;

    // Color Accessors / Mutators
    void SetBorderColor(const CColor &inColor);
    CColor GetBorderColor() const;
    void SetBGColorSelected(const CColor &inColor);
    CColor GetBGColorSelected() const;
    void SetBGColorUnselected(const CColor &inColor);
    CColor GetBGColorUnselected() const;

    // SelectionListener methods
    void AddSelectionListener(CListBoxSelectionListener *inListener);
    void RemoveSelectionListener(CListBoxSelectionListener *inListener);

protected:
    bool m_Selected;
    CColor m_BorderColor;
    QColor m_BackgroundColorSelected;
    CColor m_BackgroundColorUnselected;
    CMulticaster<CListBoxSelectionListener *>
        m_SelectionListeners; ///<	Usually only just the ListBoxControl owning this item
};

#endif // INCLUDED_LIST_BOX_ITEM_H
