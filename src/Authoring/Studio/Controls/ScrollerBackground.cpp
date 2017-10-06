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
#include "ScrollerBackground.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "Scroller.h"

//=============================================================================
/**
 * Constructor
 */
CScrollerBackground::CScrollerBackground(CColor inColor)
    : CBlankControl(inColor)
{
    m_Orientation = CScrollerBar::HORIZONTAL;
    m_ScrollerBar = nullptr;
    m_Control = this;
}

//=============================================================================
/**
 * Destructor
 */
CScrollerBackground::~CScrollerBackground()
{
}

//=============================================================================
/**
 * Overrides CBlankControl::Draw to handle scroller-specific drawing.
 */
void CScrollerBackground::Draw(CRenderer *inRenderer)
{
    CBlankControl::Draw(inRenderer);

    if (CScrollerBar::HORIZONTAL == m_Orientation) {
        // Horizontal lines
        inRenderer->PushPen(CStudioPreferences::GetScrollBGOutlineColor(), 1);
        inRenderer->MoveTo(0, 0);
        inRenderer->LineTo(GetSize().x, 0);
        inRenderer->PopPen();
        inRenderer->PushPen(CStudioPreferences::GetScrollBGOutlineColor(), 1);
        inRenderer->MoveTo(0, GetSize().y - 1);
        inRenderer->LineTo(GetSize().x, GetSize().y - 1);
        inRenderer->PopPen();
    } else {
        // Vertical lines
        inRenderer->PushPen(CStudioPreferences::GetScrollBGOutlineColor(), 1);
        inRenderer->MoveTo(0, 0);
        inRenderer->LineTo(0, GetSize().y - 1);
        inRenderer->MoveTo(GetSize().x - 1, 0);
        inRenderer->LineTo(GetSize().x - 1, GetSize().y - 1);
        inRenderer->PopPen();
    }
}

//=============================================================================
/**
 * Handles mouse down events.  Scrolls the thumb to the clicked position.
 * @param inPoint location of the mouse click
 * @param inFlags mouse event flags
 */
bool CScrollerBackground::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theHandledFlag = CBlankControl::OnMouseDown(inPoint, inFlags);

    if (!theHandledFlag && m_ScrollerBar) {
        CPt theThumbPosition = m_ScrollerBar->GetThumb()->GetPosition();
        bool theScrollForwardFlag = false;

        if (CScrollerBar::HORIZONTAL == m_Orientation) {
            if (inPoint.x > theThumbPosition.x)
                theScrollForwardFlag = true;
        } else {
            if (inPoint.y > theThumbPosition.y)
                theScrollForwardFlag = true;
        }

        m_MousePos = inPoint;

        if (theScrollForwardFlag)
            OnScrollForward(this);
        else
            OnScrollBackward(this);

        theHandledFlag = true;
    }

    return theHandledFlag;
}

//=============================================================================
/**
 * Handles mouse up events
 */
void CScrollerBackground::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CBlankControl::OnMouseUp(inPoint, inFlags);

    OnCancelScrolling();
}

//=============================================================================
/**
 * Returns the amount to scroll
 */
long CScrollerBackground::DetermineScrollAmount()
{
    long theDistance = 0;
    CPt theThumbPosition = m_ScrollerBar->GetThumb()->GetPosition();
    CPt theThumbSize = m_ScrollerBar->GetThumb()->GetSize();

    if (CScrollerBar::VERTICAL == m_Orientation) {
        if ((m_ScrollingForward && (m_MousePos.y > theThumbPosition.y + theThumbSize.y))
            || (m_ScrollingBackward && (m_MousePos.y < theThumbPosition.y))) {
            theDistance = theThumbSize.y;
        }
    } else // HORIZONTAL
    {
        if ((m_ScrollingForward && (m_MousePos.x > theThumbPosition.x + theThumbSize.x))
            || (m_ScrollingBackward && (m_MousePos.x < theThumbPosition.x))) {
            theDistance = theThumbSize.x;
        }
    }

    return theDistance;
}

//=============================================================================
/**
 * Overwrite the MouseMove function.  We want to take care of the scenario where
 * it is in a scrolling state, then determine whether to scroll forwards or backwards
 * depending on the mouse position relative to the scroller thumb.
 */
void CScrollerBackground::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // Call the default mouseMove function
    CControl::OnMouseMove(inPoint, inFlags);

    // We only want to do stuff if it is currently scrolling
    if (m_ScrollingForward || m_ScrollingBackward) {
        // save the mouse position
        m_MousePos = inPoint;

        CPt theThumbPosition = m_ScrollerBar->GetThumb()->GetPosition();
        CPt theThumbSize = m_ScrollerBar->GetThumb()->GetSize();

        if (CScrollerBar::VERTICAL == m_Orientation) {
            if (m_MousePos.y > (theThumbPosition.y + theThumbSize.y)) {
                m_ScrollingForward = true;
                m_ScrollingBackward = false;
            } else {
                m_ScrollingForward = false;
                m_ScrollingBackward = true;
            }
        } else // HORIZONTAL
        {
            if (m_MousePos.x > (theThumbPosition.x + theThumbSize.x)) {
                m_ScrollingForward = true;
                m_ScrollingBackward = false;
            } else {
                m_ScrollingForward = false;
                m_ScrollingBackward = true;
            }
        }
    }
}
