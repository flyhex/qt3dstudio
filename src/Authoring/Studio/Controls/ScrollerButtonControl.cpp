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
#include "ScrollerButtonControl.h"
#include "Scroller.h"

//=============================================================================
/**
 * Constructor
 *
 * @param inToggleButton true if this is suppose to be a toggle button.  Toggle
 * buttons stay pressed when clicked on and have to be clicked again in order
 * to release.  Normal buttons return to the up state when the button is released.
 */
CScrollerButtonControl::CScrollerButtonControl(IScrollerBar *inScrollerBar, EDirection inDirection)
    : m_Direction(inDirection)
{
    SetAbsoluteSize(CPt(CScrollerBar::DEFAULT_WIDTH, CScrollerBar::DEFAULT_WIDTH));
    SetScrollerBar(inScrollerBar);
    m_Control = this;

    Initialize();
}

//=============================================================================
/**
 * Destructor
 */
CScrollerButtonControl::~CScrollerButtonControl()
{
}

void CScrollerButtonControl::Initialize() {
    // Add the button down listeners
    if (m_Direction == FORWARD) {
        QObject::connect(this, &CButtonControl::SigButtonDown,
                         std::bind(&CScrollerButtonControl::OnScrollForward,
                                   static_cast<CScrollController *>(this), std::placeholders::_1));
    } else {
        QObject::connect(this, &CButtonControl::SigButtonDown,
                         std::bind(&CScrollerButtonControl::OnScrollBackward,
                                   static_cast<CScrollController *>(this), std::placeholders::_1));
    }
    QObject::connect(this, &CButtonControl::SigClicked,
                     std::bind(&CScrollerButtonControl::OnCancelScrolling,
                                   static_cast<CScrollController *>(this), std::placeholders::_1));
}

//=============================================================================
/**
 * Handles mouse up on the button.
 */
void CScrollerButtonControl::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CButtonControl::OnMouseUp(inPoint, inFlags);

    // If the mouse is not over this button, the base class will not fire an up event.
    // However, we want to fire this event anyway so that the timer stops (otherwise,
    // the next time you move your mouse over the button, it will start scrolling again).
    if (!IsMouseOver())
        SigButtonUp(this);
}
