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
//	Include
//==============================================================================
#include "StringEdit.h"
#include "OffscreenRenderer.h"
#include "CoreUtils.h"

//==============================================================================
/**
 * Constructor
 */
CStringEdit::CStringEdit()
    : m_AutoSize(false)
{
    // StringEdit is going to have to handle its own RevertText.
    // This can't be done in TextEdit because FloatEdit ( VectorEdit, etc ) are using the command
    // stack for undo/redo
    // StringEdit doesn't have command stack set until you hit enter or lose focus.
    // We can't make CTextEdit::RegisterCommands a virtual function since its being called in the
    // constructor,
    // therefore this is a pretty lame way to register for a hotkey.
    m_CommandHandler->RegisterKeyEvent(
        new CDynHotKeyConsumer<CStringEdit>(this, &CStringEdit::RevertText),
        Qt::ControlModifier, Qt::Key_Z);
}

//==============================================================================
/**
 * Destructor
 */
CStringEdit::~CStringEdit()
{
}

//==============================================================================
/**
 * Returns the string value of the control.
 */
Q3DStudio::CString CStringEdit::GetString()
{
    return m_Value;
}

//==============================================================================
/**
 * Sets the string value of the control.
 */
void CStringEdit::SetData(const Q3DStudio::CString &inValue, bool inFireEvent /*= true*/)
{
    if (m_Value != inValue) {
        m_Value = inValue;
        SetDisplayString(inValue, inFireEvent);

        if (inFireEvent)
            SetDirty(true);
    }
}

//==============================================================================
/**
 * Commits changes to the value of this control when the control loses focus.
 */
void CStringEdit::OnLoseFocus()
{
    CTextEdit::OnLoseFocus();
    FireCommitEvent();
}

//==============================================================================
/**
 * Commits changes to the value of this control when the Enter button is pressed.
 * @param inHighlight true to highlight the text after committing it
 */
void CStringEdit::EnterText(bool inHighlight)
{
    CTextEdit::EnterText(inHighlight);
    FireCommitEvent();
}

void CStringEdit::RefreshDisplayFromData()
{
    SetDisplayString(GetString());
    Invalidate();
}

bool CStringEdit::CanPaste()
{
    return true;
}

//==============================================================================
/**
 * Enables or disables auto-sizing of this control.  If auto-sizing is enabled
 * the size of this control will be automatically set to fit the text that it
 * contains.
 * @param inAllow true to enable auto-sizing, false to disable auto-sizing
 */
void CStringEdit::AllowAutoSize(bool inAllow)
{
    m_AutoSize = inAllow;
}

//==============================================================================
/**
 * @return true if auto-resizing is enabled, otherwise false
 */
bool CStringEdit::GetAllowAutoSize()
{
    return m_AutoSize;
}

//==============================================================================
/**
 * If auto-resizing is enabled, this function will resize this control so that
 * it is the same size as the text that it is displaying.  Text size is calculated
 * with an offscreen buffer, so this can be done outside of a draw operation.
 */
void CStringEdit::ResetSize()
{
    // If auto-resizing of the text field is enabled
    if (m_AutoSize) {
        // Resize the control to fit the text, plus the buffer gap
        COffscreenRenderer theRenderer(CRct(0, 0, 100, 16));
        CPt theSize;
        const auto textSize = theRenderer.GetTextSize(GetDisplayString().toQString());
        theSize.x = textSize.width() + GetBufferLength() * 3;
        theSize.y = textSize.height() + 1;
        SetMinimumSize(theSize);
        SetPreferredSize(theSize);
        SetMaximumSize(theSize);
    }
}

//==============================================================================
/**
 *	Primarily delegates up to the parent class, but responds by recalculating
 *	size of the text box for auto-sized strings.
 *
 *	@param inDirty true to mark this control as dirty, which causes the string to be redrawn
 *	false to mark the control as not needing to reevaluate its text during next draw cycle
 */
void CStringEdit::SetDirty(bool inDirty)
{
    // Allow the parent to handle this situation
    CTextEdit::SetDirty(inDirty);

    ResetSize();
}

//==============================================================================
/**
 * Reverts the displayed text to the previous text.
 */
void CStringEdit::RevertText()
{
    SetData(m_PreviousValue);
    SetDisplayString(m_PreviousValue);

    FireCommitEvent();
    SelectAllText();
}

//==============================================================================
/**
 * Handles any non-character keys that were pressed and need to be handled.
 *
 * @param inChar The key that was pressed
 * @param inFlags Indicates which modifier keys were down when event occurred
 * @return true if this function handled the character, otherwise false
 */
bool CStringEdit::HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    bool theMessageWasHandled = false;

    switch (inChar) {
    // Escape and Ctrl+Z both do basically the same thing for StringEdits,
    // They are special cased in TextEditInPlace though.
    case Qt::Key_Escape:
        RevertText();
        theMessageWasHandled = true;
        break;

    default:
        theMessageWasHandled = CTextEdit::HandleSpecialChar(inChar, inFlags);
    }

    return theMessageWasHandled;
}

//==============================================================================
/**
 * Called when this control gains focus.  Shows the caret, clears the current
 * selection and invalidates the control so that it will get redrawn.
 */
void CStringEdit::OnGainFocus()
{
    CTextEdit::OnGainFocus();
    m_PreviousValue = GetString();
}

//=============================================================================
/**
 * Handles character input from the keyboard.
 *
 * @param inChar Character that was pressed
 * @return true if the character was handled, false if this control does not
 * care about the character that was pressed
 */
bool CStringEdit::OnChar(const QString &inChar, Qt::KeyboardModifiers inFlags)
{
    return CTextEdit::OnChar(inChar, inFlags);
}
