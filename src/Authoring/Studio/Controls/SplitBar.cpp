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

//=============================================================================
// Prefix
//=============================================================================
#include "stdafx.h"

//=============================================================================
// Includes
//=============================================================================
#include "SplitBar.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "MouseCursor.h"
#include "StudioPreferences.h"
#include "HotKeys.h"

#include <QApplication>
//=============================================================================
// Static Variables
//=============================================================================
const long CSplitBar::DEFAULT_WIDTH = 5;

//=============================================================================
/**
 * Constructor
 */
CSplitBar::CSplitBar(CSplitterBase *inSplitter, const long inWidth /*=DEFAULT_WIDTH*/)
    : m_Splitter(inSplitter)
    , m_Width(inWidth)
    , m_MouseDown(false)
{

}

//=============================================================================
/**
 * Destructor
 */
CSplitBar::~CSplitBar()
{
}

//=============================================================================
/**
 * Handles mouse down events, starts the dragging.
 * @param inPoint where the mouse was clicked.
 * @param inFlags the state of the mouse.
 */
bool CSplitBar::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseDown(inPoint, inFlags);

    m_MouseDown = true;
    m_MouseDownPoint = inPoint;

    Invalidate();

    return true;
}

//=============================================================================
/**
 * Handles mouse up events, stops the dragging.
 * @param inPoint where the mouse was let up.
 * @param inFlags the state of the mouse.
 */
void CSplitBar::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);

    m_MouseDown = false;

    Invalidate();
}

//=============================================================================
/**
 * Do the drawing of this splitter.
 * @param inRenderer the renderer to draw to.
 */
void CSplitBar::Draw(CRenderer *inRenderer)
{
    CRct theRect(CPt(0, 0), GetSize());
    CRct theHighlightRect;
    CRct theShadowRect;

    if (m_Splitter->GetSplitDirection() == CSplitterBase::SPLIT_HORIZONTAL) {
        theHighlightRect.position = CPt(0, 0);
        theHighlightRect.size = CPt(GetSize().x, 1);
        theShadowRect.position = CPt(0, theRect.size.y - 1);
        theShadowRect.size = CPt(theRect.size.x, 1);
    } else {
        theHighlightRect.position = CPt(0, 0);
        theHighlightRect.size = CPt(1, GetSize().y);
        theShadowRect.position = CPt(theRect.size.x - 1, 0);
        theShadowRect.size = CPt(1, theRect.size.y);
    }

    inRenderer->FillSolidRect(theRect, CStudioPreferences::GetBaseColor());
    inRenderer->FillSolidRect(theHighlightRect, CStudioPreferences::GetButtonHighlightColor());
    inRenderer->FillSolidRect(theShadowRect, CStudioPreferences::GetButtonShadowColor());
}

//=============================================================================
/**
 * Get the width of this splitter bar.
 */
long CSplitBar::GetWidth()
{
    return m_Width;
}

//=============================================================================
/**
 * Set the width of this splitter bar.
 */
void CSplitBar::SetWidth(long inWidth)
{
    m_Width = inWidth;
}

//=============================================================================
/**
 * Processes the dragging of the split bar.
 * @param inPoint the position of the mouse.
 * @param inFlags the state of the mouse buttons.
 */
void CSplitBar::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    const Qt::MouseButtons buttons = QApplication::mouseButtons();

    // Don't show the cursor if the mouse is down from someone else.
    if (!(buttons & Qt::LeftButton) && !(buttons & Qt::RightButton)) {
        // If the buttons are not down and the mouse is over this control
        if (HitTest(inPoint + GetPosition())) {
            // Show the appropriate resize cursor
            setCursorIfNotSet(m_Splitter->GetSplitDirection() == CSplitterBase::SPLIT_HORIZONTAL
                                ? CMouseCursor::CURSOR_RESIZE_UPDOWN
                                : CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);
        } else {
            resetCursor();
        }
    }

    // Only care if the mouse is down
    if (m_MouseDown) {
        // When calculating the offsets remember that this object is moving with the mouse
        // and the inPoint is relative to this position, so inPoint's relative position is
        // changing on every drag.
        long theSplitPos = m_Splitter->GetSplitLocation();

        if (m_Splitter->GetSplitDirection() == CSplitterBase::SPLIT_HORIZONTAL) {
            theSplitPos += inPoint.y - m_MouseDownPoint.y;
        } else {
            theSplitPos += inPoint.x - m_MouseDownPoint.x;
        }

        m_Splitter->SetSplitLocation(theSplitPos);
    }
}

//=============================================================================
/**
 * Processes the mouse out event.  Changes the cursor if necessary back to normal.
 * @param inPoint the position of the mouse.
 * @param inFlags the state of the mouse buttons.
 */
void CSplitBar::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    const Qt::MouseButtons buttons = QApplication::mouseButtons();
    // Don't change the cursor if the mouse is down (from someone else or from ourselves)
    if (!m_MouseDown && !(buttons & Qt::LeftButton) && !(buttons & Qt::RightButton))
        resetCursor();
    CControl::OnMouseOut(inPoint, inFlags);
}
