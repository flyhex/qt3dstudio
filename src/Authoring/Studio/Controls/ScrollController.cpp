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
#include "ScrollController.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "Scroller.h"
#include "Control.h"

using namespace Q3DStudio;

const long CScrollController::DEFAULT_SCROLL_AMOUNT = 20;

//=============================================================================
/**
 * Constructor
 */
CScrollController::CScrollController()
    :  m_Orientation(IScrollerBar::VERTICAL)
    , m_ScrollerBar(NULL)
    , m_Control(NULL)
    , m_ScrollingForward(false)
    , m_ScrollingBackward(false)
{
}

//=============================================================================
/**
 * Destructor
 */
CScrollController::~CScrollController()
{
}

//=============================================================================
/**
 * Sets the control.  Used to get the check if the mouse is over the control.
 */
void CScrollController::SetControl(CControl *inControl)
{
    m_Control = inControl;
}

//=============================================================================
/**
 * Sets the scroller bar for this control.  Used to get the thumb position in
 * OnMouseDown.
 */
void CScrollController::SetScrollerBar(IScrollerBar *inScrollerBar)
{
    m_ScrollerBar = inScrollerBar;
}

//=============================================================================
/**
 * Tells this control which way to draw itself.
 * @param inOrientation Direction of the scroller that is using this control
 */
void CScrollController::SetOrientation(IScrollerBar::EOrientation inOrientation)
{
    m_Orientation = inOrientation;
}

//=============================================================================
/**
 * Returns the amount to scroll
 */
long CScrollController::DetermineScrollAmount()
{
    return DEFAULT_SCROLL_AMOUNT;
}

//=============================================================================
/**
 * Call to scroll forwards.
 * Forwards is right/down depending on orientation.
 */
void CScrollController::OnScrollForward(CControl *inButton)
{
    if (!m_ScrollingForward && inButton) {
        m_ScrollingForward = true;

        Q3DStudio::CString theName = "CScrollController::OnScrollForward";
        if (m_Control) {
            theName += "::%s";
            theName.Format(theName, static_cast<const wchar_t *>(m_Control->GetName()));
        }

        m_TimerConnection = ITickTock::GetInstance().AddTimer(
            100, true, std::bind(&CScrollController::OnTimer, this), theName);
    }

    Q_UNUSED(inButton);
    CPt theScrollDistance(0, 0);
    long theScrollAmount = DetermineScrollAmount();

    if (m_ScrollerBar->GetOrientation() == IScrollerBar::VERTICAL) {
        theScrollDistance.y = theScrollAmount;
    } else {
        theScrollDistance.x = theScrollAmount;
    }

    m_ScrollerBar->GetScroller()->SetVisiblePosition(
        m_ScrollerBar->GetScroller()->GetVisiblePosition() + theScrollDistance);
    m_ScrollerBar->RepositionThumb();
}

//=============================================================================
/**
 * Call to scroll backwards.
 * Backwards is left/up depending on orientation.
 */
void CScrollController::OnScrollBackward(CControl *inButton)
{
    if (!m_ScrollingBackward && inButton) {
        m_ScrollingBackward = true;

        Q3DStudio::CString theName = "CScrollController::OnScrollBackward";
        if (m_Control) {
            theName += "::%s";
            theName.Format(theName, static_cast<const wchar_t *>(m_Control->GetName()));
        }

        m_TimerConnection = ITickTock::GetInstance().AddTimer(
            100, true, std::bind(&CScrollController::OnTimer, this), theName);
    }

    Q_UNUSED(inButton);
    CPt theScrollDistance(0, 0);
    long theScrollAmount = DetermineScrollAmount();

    if (m_ScrollerBar->GetOrientation() == IScrollerBar::VERTICAL) {
        theScrollDistance.y = theScrollAmount;
    } else {
        theScrollDistance.x = theScrollAmount;
    }

    m_ScrollerBar->GetScroller()->SetVisiblePosition(
        m_ScrollerBar->GetScroller()->GetVisiblePosition() - theScrollDistance);
    m_ScrollerBar->RepositionThumb();
}

//=============================================================================
/**
 * Cancels the scrolling.  Removes the timer.
 */
void CScrollController::OnCancelScrolling(CControl *inButton)
{
    Q_UNUSED(inButton);

    m_ScrollingForward = false;
    m_ScrollingBackward = false;

    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();
}

//=============================================================================
/**
 * Overwritten TickTockProc thats called everytime the timer is activated.
 */
void CScrollController::OnTimer()
{
    if (m_ScrollingForward) {
        // We want it to scroll only if the mouse is over the control
        if (m_Control->IsMouseOver()) {
            OnScrollForward(nullptr);
        }
    } else if (m_ScrollingBackward) {
        // We want it to scroll only if the mouse is over the control
        if (m_Control->IsMouseOver()) {
            OnScrollBackward(nullptr);
        }
    } else // The timer should not be called if we are not scrolling forward nor backwards
    {
        ASSERT(false);
    }
}
