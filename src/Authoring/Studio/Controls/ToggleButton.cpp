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
#include "ToggleButton.h"

IMPLEMENT_OBJECT_COUNTER(CToggleButton)

//=============================================================================
/**
 *	Constructor
 */
CToggleButton::CToggleButton()
    : m_IsToggleDown(false)
{
    ADDTO_OBJECT_COUNTER(CToggleButton)
}

//=============================================================================
/**
 *	Destructor
 */
CToggleButton::~CToggleButton()
{
    REMOVEFROM_OBJECT_COUNTER(CToggleButton)
}

//=============================================================================
/**
 * Handles mouse click events (full mouse down and mouse up combo).  Causes the
 * button to change states.
 * @param inListener toggle listener to be removed
 */
void CToggleButton::OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CButtonControl::OnMouseClick(inPoint, inFlags);
    Toggle();
}

//=============================================================================
/**
 * Handles mouse down on the button.  Allows a button down event to be fired
 * by the base class, but then returns the button to the proper state, meaning
 * that toggle buttons should not actually change states on mouse down messages
 * even though that's what the base class does.
 * @param inPoint location of the mouse when this event occurred
 * @param inFlags state of modifier keys at the time that this event occurred
 */
bool CToggleButton::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theReturn = CButtonControl::OnMouseDown(inPoint, inFlags);

    if (m_IsToggleDown)
        SetButtonState(EBUTTONSTATE_DOWN);
    else
        SetButtonState(EBUTTONSTATE_UP);
    Invalidate();

    return theReturn;
}

//=============================================================================
/**
 * Handles mouse up on the button.  Fires a mouse up event, but not a toggle
 * event, as this is handled by OnMouseClick.  Button state is maintained for
 * the toggle button instead of changing immediately like on the base class.
 * @param inPoint location of the mouse when this event occurred
 * @param inFlags state of modifier keys at the time that this event occurred
 */
void CToggleButton::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CButtonControl::OnMouseUp(inPoint, inFlags);

    if (m_IsToggleDown)
        SetButtonState(EBUTTONSTATE_DOWN);
    else
        SetButtonState(EBUTTONSTATE_UP);

    Invalidate();
}

//=============================================================================
/**
 * Changes the state of the button.  If the button is currently down, it will
 * be changed to up and vice versa.  An event is fired to notify interested
 * listeners that the button was toggled.
 */
void CToggleButton::Toggle()
{
    m_IsToggleDown = !m_IsToggleDown;
    if (m_IsToggleDown)
        SetButtonState(EBUTTONSTATE_DOWN);
    else
        SetButtonState(EBUTTONSTATE_UP);

    SigToggle(this, GetButtonState());

    Invalidate();
}

void CToggleButton::SetToggleState(bool inIsDown)
{
    if (inIsDown != m_IsToggleDown) {
        m_IsToggleDown = inIsDown;
        SetButtonState(inIsDown ? EBUTTONSTATE_DOWN : EBUTTONSTATE_UP);
    }
}
