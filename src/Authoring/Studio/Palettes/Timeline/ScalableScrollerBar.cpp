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

#include "ScrollerBackground.h"
#include "ScalableScrollerBar.h"
#include "ScalableScroller.h"
#include "Renderer.h"
#include "MouseCursor.h"
#include "ResourceCache.h"

#include <QApplication>

//=============================================================================
/**
 * Creates a new ThumbTab (the scalable ends of the thumb).
 * @param inThumb the thumb this belongs to.
 * @param inIsRight true if this is the right side, false for left.
 * @param inBar the ScalableBar this belongs to.
 */
CScalableThumbTab::CScalableThumbTab(CScalableThumb *inThumb, bool inIsRightTab,
                                     CScalableBar *inBar)
    : m_Bar(inBar)
    , m_IsMouseDown(false)
    , m_IsRightTab(inIsRightTab)
{
    m_Thumb = inThumb;
}

CScalableThumbTab::~CScalableThumbTab()
{
}

//=============================================================================
/**
 * MouseOver handler, modifies the cursor.
 * @param inPoint the mouse location.
 * @param inFlags the mouse state.
 */
void CScalableThumbTab::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    setCursorIfNotSet(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);
    Invalidate();
}

//=============================================================================
/**
 * Mouse out handler, invalidates the control to clear the mouse over drawing.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CScalableThumbTab::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    if (!m_IsMouseDown)
        resetCursor();

    Invalidate();
}

//=============================================================================
/**
 * Mouse move handlers, if this was clicked on then drags the control.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CScalableThumbTab::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    // Only drag if this was clicked on
    if (m_IsMouseDown) {
        long theDiff = inPoint.x - m_MouseDownLoc.x;
        // Fire the scaling event for the delta size.
        if (m_IsRightTab)
            m_Bar->OnScalingRight(theDiff);
        else
            m_Bar->OnScalingLeft(theDiff);
    }

    CRct theRect(GetSize());
    if (theRect.IsInRect(inPoint))
        setCursorIfNotSet(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);
}

//=============================================================================
/**
 * Mouse click handler, starts resizing the control.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
bool CScalableThumbTab::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseDown(inPoint, inFlags);

    setCursorIfNotSet(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);
    m_IsMouseDown = true;
    m_MouseDownLoc = inPoint;

    return true;
}

//=============================================================================
/**
 * Mouse up handler, ends resizing this control.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CScalableThumbTab::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);

    resetCursor();

    m_IsMouseDown = false;
}

//=============================================================================
/**
 * Draw this control.
 * @param inRenderer the renderer to draw to.
 */
void CScalableThumbTab::Draw(CRenderer *inRenderer)
{
    CPt theSize = GetSize();
    CRct theRect(theSize);

    inRenderer->FillSolidRect(theRect, CStudioPreferences::GetScrollThumbHighlightColor());
}

//=============================================================================
/**
 * Create a scalable thumb, this is the dragging component of the bar.
 * @param inScrollerBar the bar this belongs to.
 */
CScalableThumb::CScalableThumb(CScalableBar *inScrollerBar)
    : CScrollerThumb(inScrollerBar)
    , m_LeftTab(this, false, inScrollerBar)
    , m_RightTab(this, true, inScrollerBar)
{
    m_ScrollerBar = inScrollerBar;

    m_LeftTab.SetPosition(CPt(0, 0));

    AddChild(&m_LeftTab);
    AddChild(&m_RightTab);
}

CScalableThumb::~CScalableThumb()
{
}

//=============================================================================
/**
 * Set the size of this component, overrides to update the location of the tabs.
 * @param inSize the new size of this control.
 */
void CScalableThumb::SetSize(CPt inSize)
{
    CScrollerThumb::SetSize(inSize);
    m_LeftTab.SetSize(CPt(7, inSize.y));
    m_RightTab.SetSize(CPt(7, inSize.y));
    m_RightTab.SetPosition(CPt(inSize.x - m_RightTab.GetSize().x, 0));
}

//=============================================================================
/**
 * On double click this sends off a reset scaling notification.
 * @param inPoint the mouse location.
 * @param inFlags the state of the mouse.
 */
bool CScalableThumb::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDoubleClick(inPoint, inFlags))
        m_ScrollerBar->OnScalingReset();

    return true;
}

//=============================================================================
/**
 * Creates a new scalable scroller bar.
 * This object can only be used for horizontal scrolling.
 * @param inScroller the scalable scroller this is operating on.
 */
CScalableBar::CScalableBar(CScalableScroller *inScroller)
    : CScrollerBar(inScroller, false)
    , m_ScalableScroller(inScroller)
    , m_Listener(nullptr)
    , m_Thumb(nullptr)
{
    Initialize();
}

CScalableBar::~CScalableBar()
{
}

//=============================================================================
/**
 * Create a new thumb control.
 * This method is here so the ScalableBar can override it and return the Scalable
 * Thumb instead of a normal thumb.
 */
CControl *CScalableBar::CreateThumb()
{
    if (m_Thumb == nullptr)
        m_Thumb = new CScalableThumb(this);
    return m_Thumb;
}

//=============================================================================
/**
 * This control is not allowed to become disabled (it should always allow scaling).
 * @param inIsEnabled ignored.
 */
void CScalableBar::SetEnabled(bool inIsEnabled)
{
    Q_UNUSED(inIsEnabled);
    CControl::SetEnabled(true);
}

//=============================================================================
/**
 * Set the single scaling listener that is to carry out the actual scaling work.
 * @param inListener the scaling listener, there is only one.
 */
void CScalableBar::SetScalingListener(CScalingListener *inListener)
{
    m_Listener = inListener;
}

//=============================================================================
/**
 * Event from the left ThumbTab that it is scaling.
 * @param inAmount the amount that the left ThumbTab is being scaled by.
 */
void CScalableBar::OnScalingLeft(long inAmount)
{
    CPt theLoc = m_Thumb->GetPosition();
    CPt theSize = m_Thumb->GetSize();

    // Don't let the loc go before the end of the control.
    if (theLoc.x + inAmount < 0)
        inAmount = -theLoc.x;

    // Anchors the scroller position when its size reaches the minimum size
    // The algorithm does not modify the inAmount as the scale can be further reduced,
    // when the scroller reaches the minimum size.

    CPt thePreviousPosition;
    bool theAnchor = false;
    if (theSize.x - inAmount < m_Thumb->GetMinimumSize().x) {
        thePreviousPosition = m_Thumb->GetPosition();
        theAnchor = true;
    }

    // Tell the listener of the scaling, it's the listener that will do the actual scaling work.
    if (m_Listener != nullptr)
        m_Listener->OnScalingLeft(theSize.x - inAmount, m_Background->GetSize().x,
                                  m_Thumb->GetPosition().x + inAmount);

    // When the Anchor flag is true (i.e. when the scroller has reach its minimum size), stop the
    // scroller
    // from moving by restoring its previous position.
    if (theAnchor) {
        m_Thumb->SetPosition(thePreviousPosition);
    }

    Invalidate();
}

//=============================================================================
/**
 * Event from the right ThumbTab that it is scaling.
 * @param inAmount the amount that the left ThumbTab is being scaled by.
 */
void CScalableBar::OnScalingRight(long inAmount)
{
    CPt theLoc = m_Thumb->GetPosition();
    CPt theSize = m_Thumb->GetSize();
    // Don't let the loc go after the end of the control.
    if (theLoc.x + theSize.x + inAmount > m_Background->GetSize().x)
        inAmount = m_Background->GetSize().x - (theLoc.x + theSize.x);

    // Anchors the scroller position when its size reaches the minimum size
    // The algorithm does not modify the inAmount as the scale can be further reduced,
    // when the scroller reaches the minimum size.

    CPt thePreviousPosition;
    bool theAnchor = false;
    if (theSize.x + inAmount < m_Thumb->GetMinimumSize().x) {
        thePreviousPosition = m_Thumb->GetPosition();
        theAnchor = true;
    }

    // Tell the listener of the scaling, it's the listener that will do the actual scaling work.
    if (m_Listener != nullptr)
        m_Listener->OnScalingRight(theSize.x + inAmount, m_Background->GetSize().x,
                                   m_Thumb->GetPosition().x);

    // When the Anchor flag is true (i.e. when the scroller has reach its minimum size), stop the
    // scroller
    // from moving by restoring its previous position.
    if (theAnchor) {
        m_Thumb->SetPosition(thePreviousPosition);
    }
    Invalidate();
}

//=============================================================================
/**
 * Handles scaling reset messages commands, just routes them to the listener.
 */
void CScalableBar::OnScalingReset()
{
    m_Listener->OnScalingReset();
}
