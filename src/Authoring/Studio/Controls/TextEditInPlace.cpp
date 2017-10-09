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

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

//==============================================================================
//	Includes
//==============================================================================
#include "TextEditInPlace.h"
#include "Renderer.h"

#include <QTimer>

//==============================================================================
//	Static class constants
//==============================================================================
const long CTextEditInPlace::s_RightBuffer =
    6; ///< Extra space allotted on the right side of the text

//==============================================================================
/**
 * Constructor
 */
CTextEditInPlace::CTextEditInPlace()
    : CStringEdit()
    , m_IsInEditMode(false)
    , m_IsEditable(true)
    , m_AllowSingleClickEdit(true)
{
    SetAlignment(LEFT);
    SetReadOnly(true);
    SetFillBackground(false);
}

//==============================================================================
/**
 * Destructor
 */
CTextEditInPlace::~CTextEditInPlace()
{
}

//==============================================================================
/**
 * Handles the mouse double-click event.  Enters the text into edit mode so that
 * the user can change its value.
 * @param inPoint location of the mouse
 * @param inFlags modifier flags for the mouse
 * @return true if this message should not be passed to any other children, otherwise false
 */
bool CTextEditInPlace::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theMessageWasHandled = false;
    if (m_IsEditable) {
        SetEditMode(true);
        SetSelection(0, GetString().Length());
        m_Caret.show = true;
        theMessageWasHandled = true;
    } else
        theMessageWasHandled = CStringEdit::OnMouseDoubleClick(inPoint, inFlags);
    return theMessageWasHandled;
}

void CTextEditInPlace::OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CStringEdit::OnMouseClick(inPoint, inFlags);
    if (IsInFocus() && !(inFlags & CHotKeys::MOUSE_RBUTTON) && !m_IsInEditMode && m_IsEditable
        && m_AllowSingleClickEdit) {
        SetEditMode(true);
        SelectAllText();
    }
}

//==============================================================================
/**
 * Called when this control loses focus.  Turns off edit mode and redraws the
 * control.
 */
void CTextEditInPlace::OnLoseFocus()
{
    SetEditMode(false);
    CStringEdit::OnLoseFocus();
    Invalidate();
}

//==============================================================================
/**
 * Handles most of the drawing, with some help from the parent class.
 * @param inRenderer Renderer to draw to
 */
void CTextEditInPlace::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize()); // CRct( CPt( ::dtol( CalculateCharWidths( inRenderer ) + s_RightBuffer
                             // ), GetSize( ).y ) );

    inRenderer->PushClippingRect(theRect);
    CStringEdit::Draw(inRenderer);
    inRenderer->PopClippingRect();
}

//==============================================================================
/**
 * Enables or disables this control from being able to enter "Edit Mode" when
 * SetEditMode() is called.
 * @param inIsEditable true if you want the control to be editable when double-clicked, otherwise
 * false
 */
void CTextEditInPlace::SetEditable(bool inIsEditable)
{
    m_IsEditable = inIsEditable;

    if (!m_IsEditable && m_IsInEditMode)
        SetEditMode(false);
}

//==============================================================================
/**
 * Starts or stops "Edit Mode".  While in Edit Mode, the user can change the
 * text, move the caret, and highlight the text.  An edit box is also drawn
 * around the text.
 * @param inEditMode true to turn on Edit Mode, false to turn off Edit Mode
 */
void CTextEditInPlace::SetEditMode(bool inEditMode)
{
    if (m_IsEditable && (inEditMode != m_IsInEditMode)) {
        m_IsInEditMode = inEditMode;
        SetReadOnly(!m_IsInEditMode);

        // If we are in edit mode
        if (m_IsInEditMode) {
            m_PreviousValue = GetString();
        }
        // If we are not in edit mode
        else {
            // Restore the text color and commit the changes
            FireCommitEvent();
        }
    }
}

//==============================================================================
/**
 * Overriden from the parent to cause this control to lose focus when the Enter
 * button is pressed on the keyboard.
 * @param inHighlight true to highlight all of the text, false to just change the string
 */
void CTextEditInPlace::EnterText(bool inHighlight)
{
    CStringEdit::EnterText(inHighlight);
    QTimer::singleShot(0, [this] { OnLoseFocus(); });
}

//==============================================================================
/**
 * Overriden to also invalidate/validate the parent.
 * @param inInvalidate true to invalidate this control and the parent control
 */
void CTextEditInPlace::Invalidate(bool inInvalidate)
{
    if (inInvalidate && GetParent())
        GetParent()->Invalidate(inInvalidate);

    CStringEdit::Invalidate(inInvalidate);
}

//==============================================================================
/**
 * Override to avoid selecting all the text unless in edit mode.
 */
void CTextEditInPlace::OnGainFocus()
{
    if (m_IsInEditMode && m_IsEditable)
        CStringEdit::OnGainFocus();
}

bool CTextEditInPlace::CanGainFocus()
{
    return true;
}

//==============================================================================
/**
 * Returns the edit mode.
 * @return true if the control is currently in edit mode meaning that the user
 * can enter text into it, otherwise false
 */
bool CTextEditInPlace::GetEditMode()
{
    return m_IsInEditMode;
}

long CTextEditInPlace::GetRightBuffer()
{
    return s_RightBuffer;
}

//==============================================================================
/**
 * Handles the Right mouse button down command.  Only shows the cut/copy/paste
 * context menu if the control is currently being edited.  You can't change a
 * string that's not currently being edited
 * @param inPoint The mouse position at the time of the event
 * @param inFlags The state of the modifier keys at the time of the event
 * @return true if this function handled the character, otherwise false
 */

bool CTextEditInPlace::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theMessageWasHandled = false;

    // The base class shows the cut/copy/paste menu, so only do it if we are in edit mode
    if (m_IsInEditMode)
        theMessageWasHandled = CStringEdit::OnMouseRDown(inPoint, inFlags);

    return theMessageWasHandled;
}

//=============================================================================
/**
 * Handles character input from the keyboard.
 *
 * @param inChar Character that was pressed
 * @return true if the character was handled, false if this control does not
 * care about the character that was pressed
 */
bool CTextEditInPlace::OnChar(const QString &inChar, Qt::KeyboardModifiers inModifiers)
{
    bool theMessageWasHandled = false;
    if (GetEditMode())
        theMessageWasHandled = CStringEdit::OnChar(inChar, inModifiers);
    return theMessageWasHandled;
}

//==============================================================================
/**
 * By default, clicking on a CTextEditInPlace control that already has focus, will
 * cause the control to enter edit mode (if SetEditable( true ) has been called).
 * This is referred to as single-click editing.  This function allows you to
 * enable or disable this functionality.  Even if single-click editing is
 * disabled, the user can still edit the text control by double-clicking on it.
 * @param inAllow true to enable single-click editing, false to prevent it
 */
void CTextEditInPlace::SetSingleClickEdit(bool inAllow)
{
    m_AllowSingleClickEdit = inAllow;
}

//==============================================================================
/**
 * Reverts the displayed text to the previous text.
 */
void CTextEditInPlace::RevertText()
{
    if (m_IsInEditMode)
        CStringEdit::RevertText();
}

//==============================================================================
/**
 * Override the base class's version
 */
void CTextEditInPlace::DoFillBackground(CRenderer *inRenderer)
{
    if (m_FillBackground && !m_IsInEditMode)
        inRenderer->FillSolidRect(QRect(0, 0, GetSize().x, GetSize().y), m_BackgroundColorNoFocus);
}

//==============================================================================
/**
 * Handles any non-character keys that were pressed and need to be handled.
 *
 * @param inChar The key that was pressed
 * @param inFlags Indicates which modifier keys were down when event occurred
 * @return true if this function handled the character, otherwise false
 */
bool CTextEditInPlace::HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    bool theMessageWasHandled = false;

    switch (inChar) {
    // We're overwriting the Escape hotkey sequence from StringEdit because we need to lose focus
    // in TextEditInPlace, StringEdit basically does the same thing for Escape and Ctrl + Z.
    case Qt::Key_Escape:
        RevertText();
        OnLoseFocus();
        theMessageWasHandled = true;
        break;

    default:
        theMessageWasHandled = CStringEdit::HandleSpecialChar(inChar, inFlags);
    }

    return theMessageWasHandled;
}
