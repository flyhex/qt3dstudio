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
#include "ScrollerThumb.h"
#include "ScrollerBar.h"
#include "Renderer.h"
#include "StudioPreferences.h"

//==============================================================================
//	Static variables
//==============================================================================
const long CScrollerThumb::MIN_LENGTH = 32;

//=============================================================================
/**
 * Constructor
 * @param inScrollerBar the scroller bar this is operating on.
 */
CScrollerThumb::CScrollerThumb(IScrollerBar *inScrollerBar)
    : m_MouseDown(false)
{
    m_ScrollerBar = inScrollerBar;
}

//=============================================================================
/**
 * Destructor
 */
CScrollerThumb::~CScrollerThumb()
{
}

//=============================================================================
/**
 * Draw this scroller bar.
 * @param inRenderer the renderer this is to draw to.
 */
void CScrollerThumb::Draw(CRenderer *inRenderer)
{
    if (IsEnabled()) {
        CPt theSize = GetSize();
        CRct theRect(theSize);

        // Draw the thumb
        inRenderer->FillRoundedRect(theRect, CStudioPreferences::GetScrollThumbBGColor(),
                                    m_ScrollerBar->GetOrientation() == CScrollerBar::VERTICAL);
    }
}

//=============================================================================
/**
 * Listener for the OnMouseDown to allow dragging.
 * Begins dragging of the control.
 */
bool CScrollerThumb::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_MouseDown = true;
        m_MouseDownPoint = inPoint;

        Invalidate();
    }

    return true;
}

//=============================================================================
/**
 * Ends dragging of the control.
 */
void CScrollerThumb::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
    m_MouseDown = false;

    Invalidate();
}

//=============================================================================
/**
 * Used for dragging the control.
 */
void CScrollerThumb::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    // Only care if the mouse is down.
    if (m_MouseDown) {
        long theBarPos = m_ScrollerBar->GetBarPosition();

        // Adjust the position based on which way we are being dragged.
        if (m_ScrollerBar->GetOrientation() == CScrollerBar::VERTICAL) {
            theBarPos += inPoint.y - m_MouseDownPoint.y;
        } else {
            theBarPos += inPoint.x - m_MouseDownPoint.x;
        }

        // Update the position.
        m_ScrollerBar->SetBarPosition(theBarPos);
    }
}

//=============================================================================
/**
 * Get the minimum size that this scroller thumb is allowed to be.
 * @return the minimum size that this scroller thumb is allowed to be.
 */
CPt CScrollerThumb::GetMinimumSize()
{
    if (m_ScrollerBar->GetOrientation() == CScrollerBar::HORIZONTAL)
        return CPt(MIN_LENGTH, 0);
    else
        return CPt(0, MIN_LENGTH);
}
