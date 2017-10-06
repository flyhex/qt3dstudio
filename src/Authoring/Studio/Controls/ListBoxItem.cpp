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
#include "ListBoxItem.h"
#include "Renderer.h"
#include "HotKeys.h"
#include "SystemPreferences.h"
#include "StudioPreferences.h"

IMPLEMENT_OBJECT_COUNTER(CListBoxItem)

//==============================================================================
/**
 * Constructor
 */
CListBoxItem::CListBoxItem()
    : m_Selected(false)
    , m_BorderColor(CStudioPreferences::GetControlRectSideLineColor())
    , m_BackgroundColorSelected(CSystemPreferences::GetSelectedItemColor())
    , m_BackgroundColorUnselected(CColor(255, 255, 255))
{
    ADDTO_OBJECT_COUNTER(CListBoxItem)

    // this reserves the minimum height for the ListBoxItem in the ListBox first
    SetAbsoluteSize(CPt(0, CStudioPreferences::GetDefaultTextEditSize()));
}

//==============================================================================
/**
 * Destructor
 */
CListBoxItem::~CListBoxItem()
{
    REMOVEFROM_OBJECT_COUNTER(CListBoxItem)
}

//==============================================================================
/**
 * Draws the bounding rectangle and white edit box for the List Box Item class.
 *
 * @param inRenderer The renderer that is responsible for drawing this Item.
 */
void CListBoxItem::Draw(CRenderer *inRenderer)
{
    const auto rect = QRect(QPoint(0, 0), GetSize());
    inRenderer->PushClippingRect(rect);

    // Fill the interior of the ListBoxItem
    CColor theFillColor = m_BackgroundColorUnselected;

    // If this ListBox is selected
    if (m_Selected)
        theFillColor = m_BackgroundColorSelected;

    inRenderer->FillSolidRect(rect, theFillColor);

    /*
    CRct theRect = GetSize();
    CPt theLowerRight( theRect.position.x + theRect.size.x - 1, theRect.position.y + theRect.size.y
    - 1 );
    CPt theLowerLeft( theRect.position.x, theLowerRight.y );

    // draw the bottom line for this item;
    // the top will be drawn by the prev item, otherwise it's the bounding box for the ListBox
    inRenderer->PushPen( m_BorderColor, 1 );
    inRenderer->MoveTo( theLowerLeft );
    inRenderer->LineTo( theLowerRight );
    inRenderer->PopPen();
    */

    inRenderer->PopClippingRect();
}

//=============================================================================
/**
 * Handles Mouse down events on this CListBoxItem.  When a Mouse Click occurs
 * on this ListBoxItem, fire off a SelectionEvent, to let the parent
 * CListBoxControl know that this is selected, and to perform neccesary selection
 * actions, e.g. Single Selection, or Multiple Selection.
 *
 * @param inPoint where the mouse was clicked, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 */
bool CListBoxItem::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theIsHandled = CControl::OnMouseDown(inPoint, inFlags);
    m_SelectionListeners.FireEvent(&CListBoxSelectionListener::OnSelectItem, this, inFlags);
    return theIsHandled;
}

//==============================================================================
/**
 * Overridden to help recalc the minimum size of the item
 */
void CListBoxItem::OnParentChanged(CControl *inParent)
{
    CControl::OnParentChanged(inParent);
    RecalcMinimumSize();
}

//==============================================================================
/**
 * Overridden to help recalc the minimum size of the item
 */
void CListBoxItem::SetMinimumSize(CPt inSize)
{
    CControl::SetMinimumSize(inSize);
    RecalcMinimumSize();
}

//==============================================================================
/**
 * Set the minimum width of the item based on the size of the parent
 */
void CListBoxItem::RecalcMinimumSize()
{
    if (GetParent()) {
        CPt theSize = CControl::GetMinimumSize();
        CPt theParentSize = GetParent()->GetSize();

        // If the minimum width of the item is less than the
        // current width  of the parent, set the size to be the
        // width of the parent
        if (theParentSize.x > (theSize.x - GetPosition().x))
            theSize.x = theParentSize.x - GetPosition().x;

        CControl::SetMinimumSize(theSize);
    }
}

//==============================================================================
/**
 * Sets this ListBoxItem to be selected or unselected.
 *
 * @param inSelected True if this CListBoxItem is selected.
 */
void CListBoxItem::SetSelectedState(bool inSelected)
{
    m_Selected = inSelected;
    Invalidate();
}

//==============================================================================
/**
 * Returns true if this ListBoxItem is selected.
 *
 * @return true if this CListBoxItem is selected.
 */
bool CListBoxItem::IsSelected() const
{
    return m_Selected;
}

//==============================================================================
/**
 * Sets the color of this control's border.  Defines the color of the bounding
 * rectangle.
 *
 * @param inColor the Color of this control's border.
 */
void CListBoxItem::SetBorderColor(const CColor &inColor)
{
    m_BorderColor = inColor;
}

//==============================================================================
/**
 * Returns the color of this control's border.
 *
 * @return the color of the border of this control.
 */
CColor CListBoxItem::GetBorderColor() const
{
    return m_BorderColor;
}

//==============================================================================
/**
 * Sets the background color for the ListBoxItem when it is selected.
 *
 * @param inColor the background color for the ListBoxItem when it is selected.
 */
void CListBoxItem::SetBGColorSelected(const CColor &inColor)
{
    m_BackgroundColorSelected = inColor;
}

//==============================================================================
/**
 * Returns the background color for the ListBoxItem when it is selected.
 *
 * @return the background color for the ListBoxItem when it is selected.
 */
CColor CListBoxItem::GetBGColorSelected() const
{
    return m_BackgroundColorSelected;
}

//==============================================================================
/**
 * Sets the background color for the ListBoxItem when it is not selected.
 *
 * @param inColor the background color for the ListBoxItem when it is not selected.
 */
void CListBoxItem::SetBGColorUnselected(const CColor &inColor)
{
    m_BackgroundColorUnselected = inColor;
}

//==============================================================================
/**
 * Returns the background color for the ListBoxItem when it is not selected.
 *
 * @return the background color for the ListBoxItem when it is not selected.
 */
CColor CListBoxItem::GetBGColorUnselected() const
{
    return m_BackgroundColorUnselected;
}

//==============================================================================
/**
 * Add a SelectionListener to this control's list of SelectionListeners.
 *
 * @param inListener The SelectionListener to add to the list of SelectionListeners
 */
void CListBoxItem::AddSelectionListener(CListBoxSelectionListener *inListener)
{
    m_SelectionListeners.AddListener(inListener);
}

//==============================================================================
/**
 * Remove a SelectionListener from this control's list of SelectionListeners.
 *
 * @param inListener The SelectionListener to remove from the list of SelectionListeners
 */
void CListBoxItem::RemoveSelectionListener(CListBoxSelectionListener *inListener)
{
    m_SelectionListeners.RemoveListener(inListener);
}
