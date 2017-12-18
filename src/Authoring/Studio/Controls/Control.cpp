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

#include "stdafx.h"

#include "Control.h"
#include "Renderer.h"
#include "MasterP.h"
#include "StudioUtils.h"
#include "HotKeys.h"
#include "ControlGraph.h"
#include "ControlData.h"
#include "ResourceCache.h"
#include "MouseCursor.h"

#include <QtWidgets/qapplication.h>

using namespace Q3DStudio::Control;

IMPLEMENT_OBJECT_COUNTER(CControl)

//=============================================================================
/**
 * Constructor, creates a control with all default values.
 */
CControl::CControl()
    : m_cursorSet(-1)
{
    m_ControlData = CControlData::CreateControlData(*this);
    ADDTO_OBJECT_COUNTER(CControl)
}

//=============================================================================
/**
 * Destructor
 */
CControl::~CControl()
{
    REMOVEFROM_OBJECT_COUNTER(CControl)
    m_ControlData->ReleaseControl();
}

//=============================================================================
/**
 * Draw this control and all children below it.
 * This can be overriden by controls to draw themselves but should still be
 * called if sub controls are to be drawn.
 * This will call Draw if this control is actually supposed to be drawn. It will
 * be drawn if it is invalidated or if anything above it in it's heirarchy has
 * been invalidated.
 * @param inRenderer the renderer to draw this control out to.
 */
void CControl::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect,
                      bool inIgnoreValidation /* = false */)
{
    EnsureLayout();
    bool isInvalidated = IsInvalidated();

    //	inRenderer->PushClippingRect( GetSize( ) );

    CRct theBoundingBox = inRenderer->GetClippingRect();

    if (isInvalidated) {
        CRct theRect(GetSize());
        theRect.And(theBoundingBox);
        theRect.Offset(inRenderer->GetTranslation());
        inDirtyRect.Or(theRect);
    }

    if (isInvalidated || inIgnoreValidation)
        Draw(inRenderer);

    // Notify the children in the reverse order that they are drawn.
    ControlGraph::SReverseIterator theRPos = ControlGraph::GetRChildren(*this);
    for (; !theRPos.IsDone(); ++theRPos) {
        (*theRPos)->EnsureLayout();
        (*theRPos)->BeginDrawChildren(inRenderer);
    }

    // Go through all the children and draw them in the correct order. By keeping
    // this order there is a semblance of depth.
    ControlGraph::SIterator thePos = ControlGraph::GetChildren(*this);
    for (; !thePos.IsDone(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        if (theChild->IsVisible()) {
            CPt thePosition = theChild->GetPosition();
            inRenderer->PushTranslation(thePosition);

            // Clipping of non-visible objects.
            if (theChild->IsInRect(theBoundingBox))
                theChild->OnDraw(inRenderer, inDirtyRect, inIgnoreValidation || isInvalidated);
            else
                theChild->NotifyNotInClipRect();
            inRenderer->PopTranslation();
        }
    }

    //	inRenderer->PopClippingRect( );

    // Set this as not being invalidated.
    Invalidate(false);
}

//=============================================================================
/**
 * Performs the drawing for this control.
 * Does nothing by default.
 * @param inRenderer the renderer to draw to.
 */
void CControl::Draw(CRenderer *inRenderer)
{
    Q_UNUSED(inRenderer);
}

//=============================================================================
/**
 * Notification that this control is not in the clipping rect and hence will
 * not be drawn.
 * By default, tell inform its children
 */
void CControl::NotifyNotInClipRect()
{
    ControlGraph::SIterator thePos = ControlGraph::GetChildren(*this);
    for (; !thePos.IsDone(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        theChild->NotifyNotInClipRect();
    }
}

//=============================================================================
/**
 * Get the position of this control.
 * The position of this control is relative to it's parent's 0,0 coordinate.
 * @return the position relative to it's parent's position.
 */
CPt CControl::GetPosition() const
{
    return m_ControlData->m_Position;
}

//=============================================================================
/**
 * Set the position of this control.
 * The position of this control is relative to it's parent's 0,0 coordinate.
 * @return the position relative to it's parent's position.
 */
void CControl::SetPosition(CPt inPosition)
{
    if (inPosition != m_ControlData->m_Position) {
        MarkChildrenNeedLayout();
        m_ControlData->m_Position = inPosition;
        Invalidate();
    }
}

//=============================================================================
/**
 * Overload of SetPosition to allow it to take 2 parameters.
 * This will call SetPosition( CPt ) so there is no need to extend this
 * method.
 * @param inX the X position to set.
 * @param inY the Y position to set.
 */
void CControl::SetPosition(long inX, long inY)
{
    SetPosition(CPt(inX, inY));
}

//=============================================================================
/**
 * Get the size of the control.
 * The size is the width and height of the control from it's position.
 * @return the size of this control.
 */
CPt CControl::GetSize() const
{
    return m_ControlData->m_Size;
}

//=============================================================================
/**
 * Set the size of this control.
 * The size is the width and height of the control from it's position.
 * @param inSize the new size of the control.
 */
void CControl::SetSize(CPt inSize)
{
    if (GetSize() != inSize) {
        m_ControlData->m_Size = inSize;
        if (GetParent() != nullptr)
            GetParent()->OnChildSizeChanged(this); // this callback needs to die
        OnSizeChanged(inSize);
    }
}

void CControl::OnSizeChanged(CPt /*inSize*/)
{
    MarkChildrenNeedLayout();
    Invalidate();
}

//=============================================================================
/**
 * Overload of SetSize to allow it to take 2 parameters.
 * This will call SetSize( CPt ) so there is no need to extend this method.
 * @param inWidth the new width of this control.
 * @param inHeight the new height of this control.
 */
void CControl::SetSize(long inX, long inY)
{
    SetSize(CPt(inX, inY));
}

//=============================================================================
/**
 * Get the minimum allowable size of this control.
 * This is used by layout managers to get the minimum size that a control is
 * allowed to be when it is resizing the control. This defaults to 0,0.
 * @return the minimum size that this control is allowed to be.
 */
CPt CControl::GetMinimumSize()
{
    return m_ControlData->m_MinSize;
}

//=============================================================================
/**
 * Set the minimum allowable size of this control.
 * This is used by layout managers to get the minimum size that a control is
 * allowed to be when it is resizing the control. This defaults to 0,0.
 * param inSize the minimum size that this control is allowed to be.
 */
void CControl::SetMinimumSize(CPt inSize)
{
    if (inSize != m_ControlData->m_MinSize) {
        NotifyParentNeedsLayout();
        m_ControlData->m_MinSize = inSize;
        if (inSize.x > m_ControlData->m_MaxSize.x)
            m_ControlData->m_MaxSize.x = inSize.x;
        if (inSize.y > m_ControlData->m_MaxSize.y)
            m_ControlData->m_MaxSize.y = inSize.y;
    }
}

//=============================================================================
/**
 * Get the maximum allowable size of this control.
 * This is used by layout managers to get the maximum size that a control is
 * allowed to be when it is resizing the control. This defaults to LONG_MAX,LONG_MAX.
 * @return the maximum size that this control is allowed to be.
 */
CPt CControl::GetMaximumSize()
{
    return m_ControlData->m_MaxSize;
}

//=============================================================================
/**
 * Set the maximum allowable size of this control.
 * This is used by layout managers to get the maximum size that a control is
 * allowed to be when it is resizing the control. This defaults to LONG_MAX,LONG_MAX.
 * @param inSize the maximum size that this control is allowed to be.
 */
void CControl::SetMaximumSize(CPt inSize)
{
    if (inSize != m_ControlData->m_MaxSize) {
        NotifyParentNeedsLayout();
        m_ControlData->m_MaxSize = inSize;
    }
}

//=============================================================================
/**
 * Get the preferred size of this control.
 * This is used by layout managers to get the preferred size of the control.
 * The preferred size is often used for relative scaling of sibling controls.
 * @return the preferred size of this control.
 */
CPt CControl::GetPreferredSize()
{
    return m_ControlData->m_PrefSize;
}

//=============================================================================
/**
 * Set the preferred size of this control.
 * This is used by layout managers to get the preferred size of the control.
 * The preferred size is often used for relative scaling of sibling controls.
 * @param the preferred size of this control.
 */
void CControl::SetPreferredSize(CPt inSize)
{
    if (inSize != m_ControlData->m_PrefSize) {
        NotifyParentNeedsLayout();
        Invalidate();
        m_ControlData->m_PrefSize = inSize;
    }
}

void CControl::SetAbsoluteSize(CPt inSize)
{
    SetMinimumSize(inSize);
    SetMaximumSize(inSize);
    SetPreferredSize(inSize);
    SetSize(inSize);
}

//=============================================================================
/**
 * Event called when the mouse is moving.
 * This event will be called when the mouse is over this control or when this
 * control has the focus. This can be extended by controls but should be called
 * if the event is to be propagated down to child controls.
 * @param inPoint the location of the mouse relative to this control.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 */
void CControl::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theHitFlag = false;
    // Go through all the children notifying them of mouse moves.
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If a child has not already gotten the move (higher level child covering a lower level
        // one)
        // then check the hit test.
        if (!theHitFlag && theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            // This is the first child to be hit, do not allow the message to go onto another
            // sibling under this one.
            theHitFlag = true;

            // Do not fire events to non-enabled children.
            if (theChild->IsEnabled()) {
                // If this is the first time the mouse is over this then fire a mouse over
                if (!theChild->IsMouseOver())
                    theChild->OnMouseOver(theChildPoint, inFlags);
                // Fire a mouse move as well, this will also propagate the mouse over to
                // grand-children etc.
                theChild->OnMouseMove(theChildPoint, inFlags);
            }
        }
        // Check all the children and if they think the mouse is over them then notify them of a
        // mouse out.
        else if (theChild->IsMouseOver()) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            theChild->OnMouseOut(theChildPoint, inFlags);
        }
    }

    // If there is a child with focus and the mouse is not over it then still notify it of the mouse
    // move
    // If the mouse is over it then it would have gotten the move from the above loop.
    if (m_ControlData->m_MouseFocus && !m_ControlData->m_MouseFocus->IsMouseOver()) {
        CPt theChildPoint = inPoint - m_ControlData->m_MouseFocus->GetPosition();
        m_ControlData->m_MouseFocus->OnMouseMove(theChildPoint, inFlags);
    }
}

//=============================================================================
/**
 * Notification that the mouse is over this control.
 * This is only called once when the mouse enters the control, and does not get
 * called until the mouse leaves then re-enters the control.
 * @param inPoint where the mouse is, relative to this control.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 */
void CControl::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);
    m_ControlData->m_IsMouseOver = true;
}

//=============================================================================
/**
 * Notification that the mouse left this control.
 * This is only called once when the mouse leaves the control and does not get
 * called until the mouse enters then re-leaves the control.
 * This also notifies all children that the mouse is over that it is no longer
 * over.
 * @param inPoint where the mouse is, relative to this control.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 */
void CControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_ControlData->m_IsMouseOver = false;

    GetWindowListener()->HideTooltips();
    // Go through all the children looking for ones that think the mouse is over.
    // If it is then notify it of a mouse out.
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        if (theChild->IsMouseOver()) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            theChild->OnMouseOut(inPoint, inFlags);
        }
    }
}

//=============================================================================
/**
 * Notification that the left mouse button was clicked on this control.
 * This handles the mouse hits and sets the focus to the control that was
 * clicked on.
 * @param inPoint where the mouse was clicked, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 * @return true if the mouse event is processed.
 */
bool CControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;
    bool theChildGotHit = false;

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If the child was hit then notify it of the mouse down, and set the focus to
        // be it.
        if (theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            theChildGotHit = true;
            // Only send the event if the child is enabled.
            if (theChild->IsEnabled()) {
                theRetVal = theChild->OnMouseDown(theChildPoint, inFlags);

                if (m_ControlData->m_Focus != theChild) {
                    if (m_ControlData->m_Focus)
                        m_ControlData->m_Focus->OnLoseFocus();
                    if (theChild->CanGainFocus()) {
                        m_ControlData->m_Focus = theChild;
                        m_ControlData->m_Focus->OnGainFocus();
                    } else {
                        m_ControlData->m_Focus = std::shared_ptr<CControlData>();
                    }
                }
                m_ControlData->m_MouseFocus = theChild;
            } else {
                m_ControlData->m_Focus = std::shared_ptr<CControlData>();
            }

            // only want OnMouseDown to be called on the first child under the point.
            break;
        }
    }
    if (!theChildGotHit && m_ControlData->m_Focus) {
        m_ControlData->m_Focus->OnLoseFocus();
        m_ControlData->m_Focus = std::shared_ptr<CControlData>();
    }
    m_ControlData->SetMouseDown(!theRetVal);

    return theRetVal;
}

//=============================================================================
/**
 * Notification that the right mouse button was clicked on this control.
 * This handles the mouse hits and sets the focus to the control that was
 * clicked on.
 * @param inPoint where the mouse was clicked, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 * @return true if the mouse event is processed.
 */
bool CControl::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If the child was hit then notify it of the mouse down, and set the focus to
        // be it.
        if (theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();

            // Only send the event if the child is enabled.
            if (theChild->IsEnabled()) {
                if (m_ControlData->m_Focus != theChild) {
                    if (m_ControlData->m_Focus)
                        m_ControlData->m_Focus->OnLoseFocus();
                    if (theChild->CanGainFocus()) {
                        m_ControlData->m_Focus = theChild;
                        m_ControlData->m_Focus->OnGainFocus();
                    } else {
                        m_ControlData->m_Focus = std::shared_ptr<CControlData>();
                    }
                }
                m_ControlData->m_MouseFocus = theChild;
                theRetVal = theChild->OnMouseRDown(theChildPoint, inFlags);
            } else {
                m_ControlData->m_Focus = std::shared_ptr<CControlData>();
            }

            // only want OnMouseDown to be called on the first child under the point.
            break;
        }
    }

    return theRetVal;
}

//=============================================================================
/**
 * Notification thatthe mouse was double clicked on this control.
 * This handled the mouse hits and passes it down to all the children.
 * @param inPoint where the mouse was clicked, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 * @return true if the mouse event is processed.
 */
bool CControl::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If the child was hit then notify it of the mouse down, and set the focus to
        // be it.
        if (theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            // Only send the event if the child is enabled.
            if (theChild->IsEnabled())
                theRetVal = theChild->OnMouseDoubleClick(theChildPoint, inFlags);

            // only want OnMouseDown to be called on the first child under the point.
            break;
        }
    }

    return theRetVal;
}

bool CControl::OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;

    // try letting the focus getting the wheel first
    if (m_ControlData->m_Focus) {
        CPt theChildPoint = inPoint - m_ControlData->m_Focus->GetPosition();
        theRetVal = m_ControlData->m_Focus->OnMouseWheel(theChildPoint, inAmount, inFlags);
    }

    // if the focus does not want the wheel then let the mouse pos do it.
    if (!theRetVal) {
        // Go through all the children looking for the first one that was clicked on
        ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);
            if (theChild->GetMouseWheelEventState() == ControlEventState::Listening
                    && theChild->IsEnabled() && theChild->HitTest(inPoint)) {
                CPt theChildPoint = inPoint - theChild->GetPosition();
                theRetVal = theChild->OnMouseWheel(theChildPoint, inAmount, inFlags);
                break;
            }
        }
    }

    return theRetVal;
}

//=============================================================================
/**
 * Notification that the mouse is hovering over this control.
 * This handles the mouse hover and passes it down to all the children.
 * @param inPoint where the mouse is located, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 * @return true if the mouse event is processed.
 */
bool CControl::OnMouseHover(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If the child was hit then notify it of the mouse down, and set the focus to
        // be it.
        if (theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            // Only send the event if the child is enabled.
            if (theChild->IsEnabled())
                theRetVal = theChild->OnMouseHover(theChildPoint, inFlags);

            // only want OnMouseHover to be called on the first child under the point.
            break;
        }
    }

    // If the mouseover was not handled
    if (!theRetVal) {
        // If the tooltip text is not empty
        QString theTooltipText = GetTooltipText();
        if (!theTooltipText.isEmpty()) {
            // Show the tooltip, and return true so that parents don't cover this tooltip with their
            // own
            // Note that we are offsetting the point so that it appears below the mouse cursor
            GetWindowListener()->ShowTooltips(GetGlobalPosition(CPt(inPoint.x, inPoint.y + 32)),
                                              GetTooltipText());
            theRetVal = true;
        }
    }

    return theRetVal;
}

//=============================================================================
/**
 * Notification that the left mouse button was released.
 * This is only called on the control that has focus, not on the control under
 * the mouse.
 * @param inPoint where the mouse was released, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 */
void CControl::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_ControlData->m_MouseFocus) {
        CPt theChildPoint = inPoint - m_ControlData->m_MouseFocus->GetPosition();
        m_ControlData->m_MouseFocus->OnMouseUp(theChildPoint, inFlags);
    }

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If the child was hit then notify it of the mouse down, and set the focus to
        // be it.
        if (theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            // Only send the event if the child is enabled.
            if (theChild->IsEnabled() && theChild != m_ControlData->m_MouseFocus
                    && theChild != m_ControlData->m_Focus) {
                theChild->OnMouseUp(theChildPoint, inFlags);
            }

            // only want OnMouseUp to be called on the first child under the point.
            break;
        }
    }
    m_ControlData->m_MouseFocus = std::shared_ptr<CControlData>();

    if (m_ControlData->m_IsMouseDown) {
        OnMouseClick(inPoint, inFlags);
        m_ControlData->SetMouseDown(false);
    }
}

//=============================================================================
/**
 * Notification that the right mouse button was released.
 * This is only called on the control that has focus, not on the control under
 * the mouse.
 * @param inPoint where the mouse was released, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * combination of the following: MODIFIER_CONTROL | MODIFIER_SHIFT |
 * MODIFIER_ALT | MOUSE_LBUTTON | MOUSE_RBUTTON | MOUSE_MBUTTON
 */
void CControl::OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_ControlData->m_MouseFocus) {
        CPt theChildPoint = inPoint - m_ControlData->m_MouseFocus->GetPosition();
        m_ControlData->m_MouseFocus->OnMouseRUp(theChildPoint, inFlags);
    }

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        // If the child was hit then notify it of the mouse down, and set the focus to
        // be it.
        if (theChild->HitTest(inPoint)) {
            CPt theChildPoint = inPoint - theChild->GetPosition();
            // Only send the event if the child is enabled.
            if (theChild->IsEnabled() && theChild != m_ControlData->m_MouseFocus
                    && theChild != m_ControlData->m_Focus) {
                theChild->OnMouseRUp(theChildPoint, inFlags);
            }

            // only want OnMouseUp to be called on the first child under the point.
            break;
        }
    }
    m_ControlData->m_MouseFocus = std::shared_ptr<CControlData>();
}

//=============================================================================
/**
 * Handles character input from the keyboard.
 *
 * @param inChar Character that was pressed
 * @return true if the character was handled, false if this control does not
 * care about the character that was pressed
 */
bool CControl::OnChar(const QString &inChar, Qt::KeyboardModifiers inModifiers)
{
    if (m_ControlData->m_Focus)
        return m_ControlData->m_Focus->OnChar(inChar, inModifiers);
    else
        return false;
}

//=============================================================================
/**
 * Handles a key down message from the keyboard.
 *
 * @param inChar Character that was pressed
 * @return true if the character was handled, false if this control does not
 * care about the character that was pressed
 */
bool CControl::OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inModifiers)
{
    bool theRetVal = false;

    if (m_ControlData->m_Focus)
        theRetVal = m_ControlData->m_Focus->OnKeyDown(inChar, inModifiers);

    if (!theRetVal) {
        if (inChar == Qt::Key_Tab) {
            if (inModifiers & Qt::ShiftModifier)
                OnReverseTab();
            else
                OnTab();
            theRetVal = true;
        }
    }

    return theRetVal;
}

//=============================================================================
/**
 * Handles a key up from the keyboard.
 *
 * @param inChar Character that was pressed
 * @return true if the character was handled, false if this control does not
 * care about the character that was pressed
 */
bool CControl::OnKeyUp(unsigned int inChar, Qt::KeyboardModifiers)
{
    Q_UNUSED(inChar);
    return false;
}

//=============================================================================
/**
 *	Find the first child (descendant) control that has a valid drop target.
 */
CDropTarget *CControl::FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags)
{
    CDropTarget *theDropTarget = NULL;

    // Go through all the children looking for the first one that was clicked on
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        if (theChild->IsMouseOver() && theChild->IsEnabled()) {
            // Put the point into this childs coords.
            CPt theChildPoint = inMousePoint - theChild->GetPosition();

            // Allow the child the opportunity to respond
            theDropTarget = theChild->FindDropCandidate(theChildPoint, inFlags);

            if (theDropTarget)
                break;
        }
    }
    return theDropTarget;
}

//=============================================================================
/**
 * Add a child control to this control.
 * This will make the child behave as a child of this, get set up for drawing
 * and events. The inInsertBefore control is used to determine Z-depth, or
 * manually insert a control into a specific location.
 * The child cannot already be a child of another control.
 * @param inControl the control to be added.
 * @param inInsertBefore the control to be inserted before, or std::shared_ptr<CControlData>() to
 * be at the back.\
 */
void CControl::AddChild(CControl *inControl,
                        CControl *inInsertBefore /*=std::shared_ptr<CControlData>()*/)
{
    NotifyParentNeedsLayout();
    ControlGraph::AddChild(*this, *inControl, inInsertBefore);
}

//=============================================================================
/**
 * Remove a child control from this control.
 * This will remove it from drawing and getting any events.
 * @param inControl the control to be removed.
 */
void CControl::RemoveChild(CControl *inControl)
{
    if (inControl) {
        ControlGraph::RemoveChild(*this, *inControl);

        if (m_ControlData->m_Focus == inControl->m_ControlData)
            m_ControlData->m_Focus = std::shared_ptr<CControlData>();
        if (m_ControlData->m_MouseFocus == inControl->m_ControlData)
            m_ControlData->m_MouseFocus = std::shared_ptr<CControlData>();
    }
}

//=============================================================================
/**
 *	Remove all child controls from this control.
 *	This will remove it from drawing and getting any events.
 *
 *	This is not recursive
 */
void CControl::RemoveAllChildren()
{
    NotifyParentNeedsLayout();
    ControlGraph::RemoveAllChildren(*this);

    m_ControlData->m_Focus = std::shared_ptr<CControlData>();
    m_ControlData->m_MouseFocus = std::shared_ptr<CControlData>();
}

//=============================================================================
/**
 *	Retrieve the index of a child control.  The index will return the zero based position.
 *	@param inChildControl the control that is a direct child of this control
 *	@return the zero-based index of this control we will return -1 if we don't find the control
 */
long CControl::GetChildIndex(CControl *inChildControl)
{
    return ControlGraph::GetChildIndex(*this, *inChildControl);
}

static inline CControl *ToControl(std::shared_ptr<CControlData> inPtr)
{
    if (inPtr)
        return inPtr->GetControl();
    return nullptr;
}

//=============================================================================
/**
 * Finds a child control by its name
 * @return the child if found, std::shared_ptr<CControlData>() otherwise
 */
CControl *CControl::FindChildByName(const Q3DStudio::CString &inName)
{
    std::shared_ptr<CControlData> theResult = std::shared_ptr<CControlData>();

    ControlGraph::SIterator theChildIter = GetChildren();
    for (; !theChildIter.IsDone(); ++theChildIter) {
        if (theChildIter.GetCurrent()->GetName() == inName) {
            theResult = theChildIter.GetCurrent();
            break;
        }
    }

    return ToControl(theResult);
}

CControl *CControl::FocusedChild()
{
    CControl *theResult = nullptr;

    ControlGraph::SIterator theChildIter = GetChildren();
    for (; !theChildIter.IsDone(); ++theChildIter) {
        auto current = ToControl(theChildIter.GetCurrent());
        auto hasFocus = HasFocus(current);
        if (hasFocus) {
            if (current->GetFirstChild())
                theResult = current->FocusedChild();
            else
                theResult = current;
            break;
        }
    }

    return theResult;
}

//=============================================================================
/**
 * Check to see if the mouse is over this control or not.
 * @return true if the mouse is over this control.
 */
bool CControl::IsMouseOver() const
{
    return m_ControlData->m_IsMouseOver;
}

//=============================================================================
/**
 * Check to see if inPoint is over this control or not.
 * This is used for mouse hits and can be extended for non-standard control
 * shapes. Non-visible controls always return false.
 * @param inPoint the location of the mouse in local coordinates.
 */
bool CControl::HitTest(const CPt &inPoint) const
{
    CPt thePoint = inPoint - GetPosition();
    CPt theSize = GetSize();
    // Basic check to see if it's in the size.
    if (IsVisible() && thePoint.x >= 0 && thePoint.y >= 0 && thePoint.x < theSize.x
            && thePoint.y < theSize.y) {
        return true;
    }
    return false;
}

//=============================================================================
/**
 * Checks to see if any part of this control is in the rect.
 * This is used for drawing and ignoring objects that do not need to be
 * redrawn or need to be drawn.
 * @param inRect the rect to check to see if this is in.
 * @return true if this is in the rect.
 */
bool CControl::IsInRect(const CRct &inRect) const
{
    CRct myRect(GetPosition(), GetSize());

    if (myRect.position <= inRect.size + inRect.position) {
        if (myRect.size + myRect.position >= inRect.position)
            return true;
    }
    return false;
}

//=============================================================================
/**
 * Invalidate this control and cause it to be redrawn.
 * @param inInvalidate true if this is to be invalidated.
 */
void CControl::Invalidate(bool inInvalidate /*= true*/)
{
    m_ControlData->m_IsInvalidated = inInvalidate;
    if (inInvalidate && GetParent() != nullptr)
        GetParent()->OnChildInvalidated();
    if (!inInvalidate)
        m_ControlData->m_IsChildInvalidated = false;
}

//=============================================================================
/**
 * Invalidate this object and all children within inRect.
 * @param inRect the rect in which to invalidate all children.
 */
void CControl::InvalidateRect(const CRct &inRect)
{
    Invalidate();

    ControlGraph::SIterator thePos = GetChildren();
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        if (theChild->IsInRect(inRect)) {
            CRct theChildRect = inRect;
            theChildRect.Offset(theChild->GetPosition());

            theChild->InvalidateRect(inRect);
        }
    }
}

//=============================================================================
/**
 * Check to see if this control is invalidated or not.
 * @return true if this control is invalidated.
 */
bool CControl::IsInvalidated() const
{
    return m_ControlData->IsInvalidated();
}

//=============================================================================
/**
 * Notifies this control that a child of it has been invalidated.
 */
void CControl::OnChildInvalidated()
{
    // Only do it if we haven't already, avoid multiple traversals up the tree.
    if (!m_ControlData->m_IsChildInvalidated) {
        if (GetParent() != nullptr)
            GetParent()->OnChildInvalidated();
        else if (m_ControlData->m_WindowListener != nullptr)
            m_ControlData->m_WindowListener->OnControlInvalidated();

        m_ControlData->m_IsChildInvalidated = true;
    }
}

//=============================================================================
/**
 * Checks to see if a child of this control is invalidated.
 */
bool CControl::IsChildInvalidated() const
{
    return m_ControlData->m_IsChildInvalidated || m_ControlData->m_IsInvalidated;
}

void CControl::SetWindowListener(CControlWindowListener *inListener)
{
    m_ControlData->m_WindowListener = inListener;
}

//=============================================================================
/**
 * Retrieves the topmost window of this control.  The window listener is the
 * bridge between the OS and the cross-platform custom control code below.
 * This function was added specifically to support drag-and-drop by providing
 * a way of setting the drag state on the outermost window.
 * @return pointer to the control window listener or std::shared_ptr<CControlData>() if there is
 * not one
 */
CControlWindowListener *CControl::GetWindowListener()
{
    CControlWindowListener *theWindowListener = nullptr;
    if (GetParent() != nullptr)
        theWindowListener = GetParent()->GetWindowListener();
    else
        theWindowListener = m_ControlData->m_WindowListener;
    return theWindowListener;
}

//=============================================================================
/**
 * Set this control as being visible or not.
 * If the control is not visible then it will not be drawn and will not
 * get mouse clicks.
 * @param inIsVisible true if this control is to be visible.
 */
void CControl::SetVisible(bool inIsVisible)
{
    if (inIsVisible != m_ControlData->m_IsVisible) {
        m_ControlData->m_IsVisible = inIsVisible;
        NotifyParentNeedsLayout();

        if (GetParent() != nullptr)
            GetParent()->OnChildSizeChanged(this);

        OnVisibleStateChange(inIsVisible);
        OnParentVisibleStateChanged(inIsVisible);

        this->Invalidate();
    }
}

//=============================================================================
/**
 * Notification that the visible state of a control has changed
 */
void CControl::OnVisibleStateChange(bool inIsVisible)
{
    Q_UNUSED(inIsVisible);
}

void CControl::OnParentVisibleStateChanged(bool inIsVisible)
{
    NotifyParentNeedsLayout();
    ControlGraph::SIterator thePos = GetChildren();
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        theChild->OnParentVisibleStateChanged(inIsVisible);
    }
}

//=============================================================================
/**
 * Checks to see if this control is visible or not.
 * If the control is not visible then it will not be drawn and will not
 * get mouse clicks.
 * @return true if this control is visible.
 */
bool CControl::IsVisible() const
{
    return m_ControlData->m_IsVisible;
}

//=============================================================================
/**
 * Sets whether or not this control is enabled.
 * If the control is not enabled then it is still drawn and still intercepts
 * mouse clicks, but it will not actually process them.
 * @param inIsEnabled true if this control is to be enabled.
 */
void CControl::SetEnabled(bool inIsEnabled)
{
    if (inIsEnabled != m_ControlData->m_IsEnabled) {
        NotifyParentNeedsLayout();
        m_ControlData->m_IsEnabled = inIsEnabled;

        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);
            theChild->SetParentEnabled(inIsEnabled);
        }
        Invalidate();
    }
}

void CControl::SetParentEnabled(bool inParentEnabled)
{
    NotifyParentNeedsLayout();
    m_ControlData->m_IsParentEnabled = inParentEnabled;
    ControlGraph::SIterator thePos = GetChildren();
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        theChild->SetParentEnabled(inParentEnabled);
    }
    Invalidate();
}

//=============================================================================
/**
 * Gets whether or not this control is enabled.
 * If the control is not enabled then it is still drawn and still intercepts
 * mouse clicks, but it will not actually process them.
 * @param inIsEnabled true if this control is to be enabled.
 */
bool CControl::IsEnabled() const
{
    return m_ControlData->IsEnabled();
}

//=============================================================================
/**
 * Gets teh value of the enabled flag without its parent's flag
 */
bool CControl::GetEnabledFlag()
{
    return m_ControlData->GetEnabledFlag();
}

//=============================================================================
/**
 * Sets the enabled flag...this is used when a control wants to override the
 * SetEnabled function and does not necessarily want to pass enabled messages
 * to its children no matter what
 */
void CControl::SetEnabledFlag(bool inIsEnabled)
{
    m_ControlData->SetEnabledFlag(inIsEnabled);
    Invalidate();
}

//=============================================================================
/**
 * Notification that the size of a child has changed.
 * @param inControl the control that has changed size.
 */
void CControl::OnChildSizeChanged(CControl *inControl)
{
    Q_UNUSED(inControl);
}

void CControl::SetName(const QString &inName)
{
    m_ControlData->SetName(Q3DStudio::CString::fromQString(inName));
}

QString CControl::GetName()
{
    return m_ControlData->GetName().toQString();
}

void CControl::BeginDrawChildren(CRenderer *inRenderer)
{
    Q_UNUSED(inRenderer);
}

long CControl::DoPopup(QMenu *inMenu, CPt inLocation)
{
    inLocation.Offset(GetPosition());
    CControl *theParent(GetParent());
    if (theParent)
        return theParent->DoPopup(inMenu, inLocation);
    else
        return m_ControlData->m_WindowListener->DoPopup(inMenu, inLocation);
}

//=============================================================================
/**
 * Called when a control acquires focus
 */
void CControl::OnGainFocus()
{
}

//=============================================================================
/**
 * Causes focus to be lost.
 */
void CControl::OnLoseFocus()
{
    if (m_ControlData->m_Focus)
        m_ControlData->m_Focus->OnLoseFocus();
    FireFocusEvent(false);
    m_ControlData->m_Focus = std::shared_ptr<CControlData>();
}

//=============================================================================
/**
 * @return the parent control of this class
 */
CControl *CControl::GetParent()
{
    return m_ControlData->GetParent();
}

const CControl *CControl::GetParent() const
{
    return m_ControlData->GetParent();
}

//=============================================================================
/**
 * Removes a control from the top level control
 */
void CControl::RemoveUberControl(CControl *inControl)
{
    if (GetParent() != nullptr)
        GetParent()->RemoveUberControl(inControl);
    else
        RemoveChild(inControl);
    Invalidate();
}

long CControl::GetChildCount()
{
    return ControlGraph::GetNumChildren(*this);
}

Q3DStudio::Control::ControlGraph::SIterator CControl::GetChildren()
{
    return ControlGraph::GetChildren(*this);
}

Q3DStudio::Control::ControlGraph::SReverseIterator CControl::GetReverseChildren()
{
    return ControlGraph::GetRChildren(*this);
}

//=============================================================================
/**
 * Gets the global position of the point in regards to the top level control
 */
CPt CControl::GetGlobalPosition(CPt inChildPoint) const
{
    CPt thePosition(GetPosition());
    CPt thePoint = CPt(inChildPoint.x + thePosition.x, inChildPoint.y + thePosition.y);
    if (GetParent())
        return GetParent()->GetGlobalPosition(thePoint);
    else
        return thePoint;
}

//=============================================================================
/**
 * Query the platform specific render device (window)
 */
Qt3DSRenderDevice CControl::GetPlatformDevice()
{
    if (GetParent())
        return GetParent()->GetPlatformDevice();
    return nullptr;
}

//=============================================================================
/**
 * Does self or child use this render device?
 * @see CWndControl::OnKillFocus
 */
bool CControl::IsChildPlatformDevice(Qt3DSRenderDevice inDevice)
{
    if (GetPlatformDevice() == inDevice)
        return true;

    ControlGraph::SIterator thePos = GetChildren();
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        if (theChild->IsChildPlatformDevice(inDevice))
            return true;
    }

    return false;
}

//=============================================================================
/**
 * Shows the window's moveable window with text
 *
 * @param inLocation the postion of hte center point of the window
 * @param inText the text the window will display
 */
void CControl::ShowMoveableWindow(CPt inLocation, const Q3DStudio::CString &inText,
                                  CRct inBoundingRct)
{
    CPt thePosition(GetPosition());
    CPt thePoint = CPt(inLocation.x + thePosition.x, inLocation.y + thePosition.y);

    if (GetParent())
        GetParent()->ShowMoveableWindow(thePoint, inText, inBoundingRct);
    else if (m_ControlData->m_WindowListener)
        m_ControlData->m_WindowListener->ShowMoveableWindow(thePoint, inText, inBoundingRct);
}

//=============================================================================
/**
 * Hides the window's moveable window
 */
void CControl::HideMoveableWindow()
{
    if (GetParent() != nullptr)
        GetParent()->HideMoveableWindow();
    else if (m_ControlData->m_WindowListener)
        m_ControlData->m_WindowListener->HideMoveableWindow();
}

//=============================================================================
/**
 * Offsets the position of this control... this is useful if you don't want to calculate
 * the global position every time
 */
void CControl::OffsetPosition(CPt inOffset)
{
    CPt thePosition(GetPosition());
    thePosition.Offset(inOffset);
    SetPosition(thePosition);
}

//=============================================================================
/**
 * Gets the first child of this control
 */
CControl *CControl::GetFirstChild()
{
    std::shared_ptr<CControlData> theChild = std::shared_ptr<CControlData>();
    ControlGraph::SIterator thePos = ControlGraph::GetChildren(*this);
    if (!thePos.IsDone())
        theChild = (*thePos);
    return ToControl(theChild);
}

//=============================================================================
/**
 * @return true if this control has focus
 */
bool CControl::HasFocus(CControl *inControl)
{
    return (ToControl(m_ControlData->m_Focus) == inControl);
}

//=============================================================================
/**
 * default is true for controls... override if the control cannot have focus
 */
bool CControl::CanGainFocus()
{
    if (IsVisible()) {
        for (ControlGraph::SIterator thePos = ControlGraph::GetChildren(*this); !thePos.IsDone();
             ++thePos) {
            if ((*thePos)->CanGainFocus())
                return true;
        }
    }
    return false;
}

//=============================================================================
/**
 *	Returns true if this CControl is in focus.
 *	@return True if this CControl is in focus.
 */
bool CControl::IsInFocus()
{
    if (GetParent())
        return GetParent()->HasFocus(this);

    return false;
}

//=============================================================================
/**
 * Handles the tab button in controls
 */
void CControl::OnTab()
{
    // Go through the children... if there is a focus, then get the next control
    ControlGraph::SIterator thePos = ControlGraph::GetChildren(*this);
    bool theFoundFlag = false;
    if (m_ControlData->m_Focus) {
        while (!thePos.IsDone() && !theFoundFlag) {
            std::shared_ptr<CControlData> theCurrentControl = (*thePos);
            if (theCurrentControl == m_ControlData->m_Focus) {
                ++thePos;
                while (!thePos.IsDone() && !theFoundFlag) {
                    std::shared_ptr<CControlData> theNextFocusCanidate = (*thePos);
                    if (theNextFocusCanidate->CanGainFocus()) {
                        m_ControlData->m_Focus->OnLoseFocus();
                        theFoundFlag = true;
                        m_ControlData->m_Focus = theNextFocusCanidate;
                        m_ControlData->m_Focus->SetFocusToFirstAvailable();
                    } else {
                        ++thePos;
                    }
                }
            } else {
                ++thePos;
            }
        }
        // If we didn't find it and we have a parent, then allow the parent to decide
        if (!theFoundFlag) {
            m_ControlData->m_Focus->OnLoseFocus();
            m_ControlData->m_Focus = std::shared_ptr<CControlData>();
            if (GetParent())
                GetParent()->OnTab();
            else
                SetFocusToFirstAvailable();
        }
    } else {
        // If no focus, then go to first available control
        SetFocusToFirstAvailable();
    }
}

//=============================================================================
/**
 * Handles the shift tab button in controls
 */
void CControl::OnReverseTab()
{
    // Go through the children in reverse order... if there is a focus, then get the next control
    ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
    bool theFoundFlag = false;
    if (m_ControlData->m_Focus) {
        while (!thePos.IsDone() && !theFoundFlag) {
            std::shared_ptr<CControlData> theCurrentControl = (*thePos);
            if (theCurrentControl == m_ControlData->m_Focus) {
                ++thePos;
                while (!thePos.IsDone() && !theFoundFlag) {
                    std::shared_ptr<CControlData> theNextFocusCanidate = (*thePos);
                    if (theNextFocusCanidate->CanGainFocus()) {
                        m_ControlData->m_Focus->OnLoseFocus();
                        theFoundFlag = true;
                        m_ControlData->m_Focus = theNextFocusCanidate;
                        m_ControlData->m_Focus->SetFocusToLastAvailable();
                    } else {
                        ++thePos;
                    }
                }
            } else {
                ++thePos;
            }
        }
        // If we didn't find it and we have a parent, then allow the parent to decide
        if (!theFoundFlag) {
            m_ControlData->m_Focus->OnLoseFocus();
            m_ControlData->m_Focus = std::shared_ptr<CControlData>();
            if (GetParent())
                GetParent()->OnReverseTab();
            else
                SetFocusToLastAvailable();
        }
    } else {
        // If no focus, then go to last available control
        SetFocusToLastAvailable();
    }
}

//=============================================================================
/**
 * Sets the focus to the first available child control
 */
void CControl::SetFocusToFirstAvailable()
{
    bool theFlag = false;
    OnGainFocus();

    // if there are any child controls, then go through them
    for (ControlGraph::SIterator thePos = ControlGraph::GetChildren(*this);
         !thePos.IsDone() && !theFlag; ++thePos) {
        std::shared_ptr<CControlData> theControl = (*thePos);
        if (theControl->CanGainFocus()) {
            m_ControlData->m_Focus = theControl;
            m_ControlData->m_Focus->SetFocusToFirstAvailable();
            theFlag = true;
        }
    }
}

//=============================================================================
/**
 * Sets the focus to the last available child control
 */
void CControl::SetFocusToLastAvailable()
{
    bool theFlag = false;
    OnGainFocus();

    // if there are any child controls, then go through them
    for (ControlGraph::SReverseIterator thePos = ControlGraph::GetRChildren(*this);
         !thePos.IsDone() && !theFlag; ++thePos) {
        std::shared_ptr<CControlData> theControl = (*thePos);
        if (theControl->CanGainFocus()) {
            m_ControlData->m_Focus = theControl;
            m_ControlData->m_Focus->SetFocusToLastAvailable();
            theFlag = true;
        }
    }
}

//===============================================================================
/**
* Coverts the given point from local coordinates into global coordinates.
* @param inPoint the point in local coordinates to be converted
* @return The point translated to global coordinates
*/
CPt CControl::ClientToScreen(CPt inPoint)
{
    CPt theFinalPt = inPoint + m_ControlData->m_Position;

    if (GetParent())
        theFinalPt = GetParent()->ClientToScreen(theFinalPt);
    else if (m_ControlData->m_WindowListener)
        theFinalPt = m_ControlData->m_WindowListener->ClientToScreen(theFinalPt);

    return theFinalPt;
}

//===============================================================================
/**
* Coverts the given point from screen coordinates into local space.
* @param inPoint the point in screen coordinates to be converted
* @return The point translated to local, client coordinates
*/
CPt CControl::ScreenToClient(CPt inPoint)
{
    CPt theFinalPt = inPoint - m_ControlData->m_Position;

    if (GetParent())
        theFinalPt = GetParent()->ScreenToClient(theFinalPt);
    else if (m_ControlData->m_WindowListener)
        theFinalPt = m_ControlData->m_WindowListener->ScreenToClient(theFinalPt);

    return theFinalPt;
}

//===============================================================================
/**
* Adds a focus listener to this control
*/
void CControl::AddFocusListener(CChildFocusListener *inListener)
{
    m_ControlData->m_FocusListeners.AddListener(inListener);
}

//===============================================================================
/**
* Removes a focus listener to this control
*/
void CControl::RemoveFocusListener(CChildFocusListener *inListener)
{
    m_ControlData->m_FocusListeners.RemoveListener(inListener);
}

//===============================================================================
/**
* tells anyone listeneing that the focus of this control has changed
*/
void CControl::FireFocusEvent(bool inStatus)
{
    m_ControlData->m_FocusListeners.FireEvent(&CChildFocusListener::OnChildFocusChange, inStatus);
}

//===============================================================================
/**
 * Get the platform specific view that this is embedded into.
 * Used for when platform dependent controls have to be embedded or used.
 */
TPlatformView CControl::GetPlatformView()
{
    if (GetParent())
        return GetParent()->GetPlatformView();
    else if (m_ControlData->m_WindowListener)
        return m_ControlData->m_WindowListener->GetPlatformView();
    return nullptr;
}

bool CControl::IsMouseDown()
{
    return m_ControlData->m_IsMouseDown;
}

void CControl::OnMouseClick(CPt, Qt::KeyboardModifiers)
{
}

//===============================================================================
/**
 * Sets the text of the tooltip for this control.  If the string is empty, no
 * tooltip will be shown.
 * @param inText text of the tooltip
 */
void CControl::SetTooltipText(const QString &inText)
{
    m_ControlData->SetTooltipText(inText);
}

//===============================================================================
/**
 * @return the current tooltip text for this control
 */
QString CControl::GetTooltipText()
{
    return m_ControlData->GetTooltipText();
}

void CControl::GrabFocus(CControl *inControl)
{
    if (GetParent())
        GetParent()->GrabFocus(this);

    std::shared_ptr<CControlData> theNewFocus;
    if (inControl)
        theNewFocus = inControl->m_ControlData;

    if (m_ControlData->m_Focus != theNewFocus) {
        if (m_ControlData->m_Focus)
            m_ControlData->m_Focus->OnLoseFocus();
        m_ControlData->m_Focus = theNewFocus;
        if (m_ControlData->m_Focus)
            m_ControlData->m_Focus->OnGainFocus();
    }
}

//===============================================================================
/**
 * Used to notify scrolling views that something should be visible.
 */
void CControl::EnsureVisible(CRct inRect)
{
    if (GetParent()) {
        inRect.Offset(GetPosition());
        GetParent()->EnsureVisible(inRect);
    }
}

//===============================================================================
/**
 * Call to make this control visible.
 */
void CControl::EnsureVisible()
{
    CRct theRect(CPt(0, 0), GetSize());
    EnsureVisible(theRect);
}

void CControl::OnParentChanged(CControl * /*inNewParent*/)
{
}

void CControl::MarkChildrenNeedLayout()
{
    if (m_ControlData->m_ChildrenNeedLayout == false) {
        for (ControlGraph::SIterator theIter = GetChildren(); theIter.IsDone() == false; ++theIter)
            (*theIter)->MarkNeedsLayout();
        m_ControlData->m_ChildrenNeedLayout = true;
    }
}

void CControl::NotifyParentNeedsLayout()
{
    CControl *theParent(GetParent());
    if (theParent)
        theParent->MarkChildrenNeedLayout();
}

void CControl::MarkNeedsLayout()
{
    m_ControlData->m_NeedsLayout = true;
}
// Tell this control that one of its children need to be layed out.
void CControl::SetLayout(CPt inSize, CPt inPosition)
{
    SetSize(inSize);
    SetPosition(inPosition);
    m_ControlData->m_NeedsLayout = false;
}

void CControl::LayoutChildren()
{
    for (ControlGraph::SIterator theIter = GetChildren(); theIter.IsDone() == false; ++theIter) {
        std::shared_ptr<CControlData> theChild(*theIter);
        // By default the children get the exact same layout that I do.
        theChild->SetLayout(theChild->m_Size, theChild->m_Position);
    }
    m_ControlData->m_ChildrenNeedLayout = false;
}

void CControl::EnsureLayout()
{
    if (m_ControlData->m_NeedsLayout) {
        if (IsVisible()) {
            CControl *parent = GetParent();
            if (parent)
                parent->LayoutChildren();
        }
        m_ControlData->m_NeedsLayout = false;
    }
    if (m_ControlData->m_ChildrenNeedLayout) {
        LayoutChildren();
        m_ControlData->m_ChildrenNeedLayout = false;
    }
}

void CControl::ChildrenChanged()
{
    MarkChildrenNeedLayout();
    Invalidate();
    m_ControlData->OnHierarchyChanged();
}

void CControl::setCursorIfNotSet(long cursor)
{
    if (cursor != m_cursorSet) {
        if (m_cursorSet != -1)
            qApp->changeOverrideCursor(CResourceCache::GetInstance()->GetCursor(cursor));
        else
            qApp->setOverrideCursor(CResourceCache::GetInstance()->GetCursor(cursor));
        m_cursorSet = cursor;
    }
}

void CControl::resetCursor()
{
    if (m_cursorSet != -1) {
        // Restoring back to no-override state seems to not change the cursor automatically
        // to the default cursor, so let's do that manually before restoring the cursor
        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();
        m_cursorSet = -1;
    }
}

QCursor CControl::getCursor() const
{
    if (m_cursorSet != -1)
        return CResourceCache::GetInstance()->GetCursor(m_cursorSet);
    return QCursor();
}
