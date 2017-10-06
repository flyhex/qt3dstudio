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
#ifndef INCLUDED_TOGGLE_BUTTON_H
#define INCLUDED_TOGGLE_BUTTON_H 1
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "ButtonControl.h"

//==============================================================================
//	Functors
//==============================================================================

//=============================================================================
/**
 * Class for making toggle buttons.  A toggle button changes state each time a
 * full mouse click is received.  When you click the mouse on a toggle button,
 * it will enter the "down" state.  When you click it again, the toggle button
 * will enter the "up" state.  This differs from a regular button, which only
 * enters the "down" state while the mouse is being held down, then immediately
 * returns to the "up" state when the mouse is released.
 */
class CToggleButton : public CButtonControl
{
public:
    CToggleButton();
    virtual ~CToggleButton();

    DEFINE_OBJECT_COUNTER(CToggleButton)

    void OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    virtual void Toggle();

    virtual void SetToggleState(bool inIsDown);
    bool GetToggleState() const { return m_IsToggleDown; }

    //=============================================================================
    /**
    * Functor for handling button toggle messages.  The message will be fired to
    * interested listeners when a full button click occurs on this button.  If you
    * want to be a listener for button toggle events, you must define an
    * OnButtonToggle function with the following parameters.
    * @param CToggleButton* pointer to the button that generated the event
    * @param EButtonState the state of the button as a result of this event
    */
    boost::signal2<void, CToggleButton *, CButtonControl::EButtonState> SigToggle;

protected:
    // protected to make fools use SetToggleState instead.
    void SetButtonState(EButtonState inState) override { CButtonControl::SetButtonState(inState); }

    bool m_IsToggleDown;
};

#endif // INCLUDED_TOGGLE_BUTTON_H
