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
#include "FloatEdit.h"
#include "MouseCursor.h"
#include "ResourceCache.h"
#include "StudioClipboard.h"
#include "ControlData.h"
#include "Qt3DSMath.h"

#include <QtWidgets/qapplication.h>
// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

//==============================================================================
/**
 * Constructor
 */
CFloatEdit::CFloatEdit()
    : m_NumDecimalPlaces(3)
    , m_FixedPlaces(-1)
    , m_IsMouseDown(false)
    , m_Min(-FLT_MAX)
    , m_Max(FLT_MAX)
    , m_EditMode(false)
    , m_RevertListener(NULL)
{
    m_ControlData->SetMouseWheelEnabled(true);
    // Set the default string
    SetData(0.0f, false);

    SetReadOnly(true);
    SetName("FloatEdit");

    // TODO: Used only for timeline palette timebar. It is not editable, we do not need
    // to register any events. Registering minus here will prevent it from working for zooming
    // out in the timeline.
    // TODO: This whole class will most likely be removed when timeline is converted to Qt.
    //    m_CommandHandler.RegisterKeyEvent(
    //        new CDynHotKeyConsumer<CFloatEdit>(this, &CFloatEdit::AddCharNegative), 0, '-');
    //    m_CommandHandler.RegisterKeyEvent(
    //        new CDynHotKeyConsumer<CFloatEdit>(this, &CFloatEdit::AddCharPeriod), 0, '.');
}

//==============================================================================
/**
 * Destructor
 */
CFloatEdit::~CFloatEdit()
{
}

void CFloatEdit::AddCharNegative()
{
    unsigned int theMinus = '-';

    InsertChar(theMinus);
}

void CFloatEdit::AddCharPeriod()
{
    unsigned int thePeriod = '.';

    InsertChar(thePeriod);
}

void CFloatEdit::FormatString()
{
    Q3DStudio::CString theFormatString;
    if (m_FixedPlaces == -1)
        theFormatString.Format(_LSTR("%%.%df"), m_NumDecimalPlaces);
    else
        theFormatString.Format(_LSTR("%%0%d.%df"), m_FixedPlaces, m_NumDecimalPlaces);

    m_ValueString.Format(theFormatString, m_Value);
}

//==============================================================================
/**
 * Converts the internal (float) data of this control into a string that can be
 * displayed.  The parent class (CTextEdit) calls this function when it needs
 * to redraw the control and the text within it.
 *
 * @return the string to be displayed.
 */
Q3DStudio::CString CFloatEdit::GetString()
{
    return m_ValueString;
}

//==============================================================================
/**
 * Sets the internal data of this control the specified value and marks the
 * control as dirty so that it gets redrawn during the next free cycle.
 *
 * @param inValue the new value for this control
 * @param inFireEvent true to fire a property change event
 */
void CFloatEdit::SetData(float inValue, bool inFireEvent /* = true */)
{
    m_Value = inValue;

    FormatString();
    SetDisplayString(GetString(), inFireEvent);
    if (inFireEvent)
        SetDirty(true);
}

//==============================================================================
/**
 * Attempts to set the internal data of this control to the specified string.
 *
 * @param inData a string containing the new data
 */
void CFloatEdit::SetData(const Q3DStudio::CString &inData, bool inFireEvent /*= true*/)
{
    m_Value = static_cast<float>(::atof(inData.GetCharStar()));

    FormatString();
    // The string needs to be set precisely here, otherwise
    // we end up truncating - and . from the floats
    SetDisplayString(inData, inFireEvent);
    SetDirty(true);
}

//==============================================================================
/**
 * Determines if the specified character can be inserted into the string.  This
 * control only accepts numbers, decimals, and minus signs.  Further, a minus
 * sign may only be inserted at the front of the string if there is not already
 * one in the string and a decimal may only be inserted if there is not one in
 * the string already.
 *
 * @param inCheckString the string that is to be checked (in case a lenght requirement is exceeded)
 * @param inChar the character that was pressed
 * @param inPosition character index where we are attempting to insert inChar into inCheckString
 * @return true if this control can accept the character into the string, otherwise false
 */
bool CFloatEdit::CanAcceptChar(const Q3DStudio::CString &inCheckString, unsigned int inChar,
                               unsigned int inPosition)
{
    bool theRetVal = false;

    unsigned int thePeriod = '.';
    unsigned int theMinus = '-';

    if ((CTextEdit::CanAcceptChar(inCheckString, inChar, inPosition) && inCheckString.Length() < 10)
            || inChar == thePeriod || inChar == theMinus) {
        if (inChar >= '0' && inChar <= '9')
            theRetVal = true;
        if (!theRetVal) {
            if (thePeriod == inChar) {
                if (inCheckString.Find(char(thePeriod)) == Q3DStudio::CString::ENDOFSTRING)
                    theRetVal = true;
                else
                    theRetVal = false;
            } else if (theMinus == inChar) {
                if ((inCheckString.Find(char(theMinus)) == Q3DStudio::CString::ENDOFSTRING)
                        && (inPosition == 0)) {
                    theRetVal = true;
                }
            }
        }
    }
    return theRetVal;
}

bool CFloatEdit::HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    bool isLargeStep = inFlags & CHotKeys::MODIFIER_SHIFT ? true : false;
    bool isSmallStep = inFlags & CHotKeys::MODIFIER_CONTROL ? true : false;

    bool wasHandled = false;
    switch (inChar) {
    case Qt::Key_Up:
        if (isLargeStep)
            SetFloatValue(m_Value + 10.0f);
        else if (isSmallStep)
            SetFloatValue(static_cast<float>(m_Value + 0.1f));
        else
            SetFloatValue(static_cast<float>(m_Value + 1.0f));
        wasHandled = true;
        break;

    case Qt::Key_Down:
        if (isLargeStep)
            SetFloatValue(static_cast<float>(m_Value - 10.0f));
        else if (isSmallStep)
            SetFloatValue(static_cast<float>(m_Value - 0.1f));
        else
            SetFloatValue(m_Value - 1.0f);
        wasHandled = true;
        break;

    case Qt::Key_Enter:
        ExitEditMode();
        wasHandled = true;
        CTextEdit::OnLoseFocus();
        break;

    case Qt::Key_Escape:
        if (m_RevertListener)
            m_RevertListener->OnDiscardChanges(this);
        ExitEditMode();
        wasHandled = true;
        CTextEdit::OnLoseFocus();
        break;

        /*
        case CHotKeys::KEY_SUBTRACT:
        case CHotKeys::KEY_SUBTRACT_OEM:
        case CHotKeys::KEY_PERIOD_OEM:
                wasHandled = true;
        break;
        */
    }

    if (!wasHandled)
        wasHandled = CTextEdit::HandleSpecialChar(inChar, inFlags);

    return wasHandled;
}

float CFloatEdit::GetData()
{
    return m_Value;
}

float CFloatEdit::GetDisplayData()
{
    float theFloat = static_cast<float>(::atof(GetDisplayString().GetCharStar()));
    return theFloat;
}

void CFloatEdit::OnLoseFocus()
{
    ExitEditMode();
    CTextEdit::OnLoseFocus();
}

void CFloatEdit::OnGainFocus()
{
    if (!m_IsMouseDown && IsEnabled())
        SetReadOnly(false);
    CTextEdit::OnGainFocus();
}

bool CFloatEdit::CanGainFocus()
{
    return true;
}

void CFloatEdit::ExitEditMode()
{
    m_EditMode = false;
    SetReadOnly(true);
}

void CFloatEdit::EnterEditMode()
{
    m_EditMode = true;
    SetReadOnly(false);
}

//==============================================================================
/**
 * Handles the dynamic dragging of float values.
 */
void CFloatEdit::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_IsMouseDown && !m_EditMode) {
        // If we haven't started handling selecting or dragging figure out which one to do
        if (!m_Trapping) {
            // If moved outside of the safety zone start dragging the value.
            if (::labs(inPoint.y - m_MouseStartPos.y) > 2) {
                m_Trapping = true;
                // In Mac there's no infinite mouse, so don't hide it.
                setCursorIfNotSet(CMouseCursor::CURSOR_BLANK);
            }
        }
        // If in trapping mode then do the value drag processing.
        if (m_Trapping) {
            float theDiff = static_cast<float>(m_MouseStartPos.y - inPoint.y);

            if (inFlags & CHotKeys::MODIFIER_CONTROL)
                theDiff *= 0.1f;
            else if (inFlags & CHotKeys::MODIFIER_SHIFT)
                theDiff *= 10.0f;

            SetFloatValue(theDiff + m_StartDragVal);

            CPt theMouseLoc = CControl::ClientToScreen(m_MouseStartPos);
            getCursor().setPos(theMouseLoc.x, theMouseLoc.y);

            m_StartDragVal = GetData();
        }
    } else {
        CTextEdit::OnMouseMove(inPoint, inFlags);
    }
}

void CFloatEdit::SetFloatValue(float inValue)
{
    // If the min and max are both zero, special case, don't bother capping the value to the
    // min/max.
    if (m_Min != 0 || m_Max != 0) {
        if (inValue > m_Max)
            inValue = m_Max;
        else if (inValue < m_Min)
            inValue = m_Min;
    }

    SetData(inValue);
    ReloadData();
    Q3DStudio::CString theNewValue = GetString();
    SetData(theNewValue);
}

//==============================================================================
/**
 * Handler to start mouse dragging actions on this.
 */
bool CFloatEdit::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (IsEnabled()) {
        if (!CControl::OnMouseDown(inPoint, inFlags)) {
            if (m_EditMode)
                CTextEdit::OnMouseDown(inPoint, inFlags);

            m_IsMouseDown = true;
            m_Trapping = false;
            m_StartDragVal = GetData();

            m_MouseStartPos = inPoint;
        }
    }

    return true;
}

//==============================================================================
/**
 * Handler to finish the mouse dragging actions.
 */
void CFloatEdit::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_IsMouseDown && !m_Trapping && m_IsReadOnly) {
        SetReadOnly(false);
        SelectAllText();
        EnterEditMode();
    }

    CTextEdit::OnMouseUp(inPoint, inFlags);

    resetCursor();
    m_IsMouseDown = false;

    if (m_Trapping)
        FireFocusEvent(true);
}

//==============================================================================
/**
 * Set the minimum value allowed in this edit field.
 */
void CFloatEdit::SetMin(float inMin)
{
    m_Min = inMin;
}

//==============================================================================
/**
 * Set the maximum value allowed in this field.
 */
void CFloatEdit::SetMax(float inMax)
{
    m_Max = inMax;
}

//==============================================================================
/**
 * Handle mouse wheel messages to allow scrolling of the values.
 */
bool CFloatEdit::OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    bool theRet = false;
    if (GetParent() && GetParent()->HasFocus(this)) {
        float theDiff = 1.0f;
        if (inFlags & CHotKeys::MODIFIER_CONTROL)
            theDiff = 0.1f;
        else if (inFlags & CHotKeys::MODIFIER_SHIFT)
            theDiff = 10.0f;

        if (inAmount < 0)
            theDiff *= -1.0f;

        SetFloatValue(GetData() + theDiff);

        theRet = true;
    }
    return theRet;
}

//==============================================================================
/**
 * Set the number of decimal places to be displayed in this.
 */
void CFloatEdit::SetNumDecimalPlaces(short inNumDecimalPlaces)
{
    m_NumDecimalPlaces = inNumDecimalPlaces;
    FormatString();
}

//==============================================================================
/**
 * Set the number of fill places before the start of the number to fill if the number
 * is too short. This defaults to -1 which means none.
 */
void CFloatEdit::SetFixedPlaces(short inFixedPlaces)
{
    m_FixedPlaces = inFixedPlaces;
}

//==============================================================================
/**
 * Ensure that the text from clipboard can be pasted into this control
 */
bool CFloatEdit::CanPaste()
{
    bool theValid = false;

    const auto text = Q3DStudio::CString::fromQString(CStudioClipboard::GetTextFromClipboard());
    if (m_Caret.show) {
        Q3DStudio::CString theNewString(m_DisplayString);
        long theCaret = m_Caret.position;

        if (HasSelectedText()) {
            theNewString.Delete(GetSelectionLeft(), GetSelectionRight() - GetSelectionLeft());
            theCaret = Q3DStudio::MIN(GetSelectionLeft(), GetSelectionRight());
        }
        theNewString.Insert(theCaret, text);
        theValid = Validate(theNewString);
    } else {
        theValid = Validate(text);
    }

    return theValid;
}

//==============================================================================
/**
 * Returns true if the given string can be pasted into the control
 */
bool CFloatEdit::Validate(const Q3DStudio::CString &inString)
{
    Q3DStudio::CString theCheckString("");
    Q3DStudio::CString theTemp;
    bool theRetVal = true;
    long theIter;

    for (theIter = 0; theIter < inString.Length(); ++theIter) {
        theTemp = inString.Extract(theIter, 1);

        if (!CanAcceptChar(theCheckString, theTemp[0], theIter)) {
            theRetVal = false;
            break;
        } else {
            theCheckString += inString.Extract(theIter, 1);
        }
    }

    return theRetVal;
}

//==============================================================================
/**
 * Refreshes the display string from the internal data
 */
void CFloatEdit::RefreshDisplayFromData()
{
    SetDisplayString(GetString());
    Invalidate();
}
