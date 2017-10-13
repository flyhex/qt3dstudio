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

#include "TextEdit.h"
#include "Renderer.h"
#include "SystemPreferences.h"
#include "HotKeys.h"
#include "TextEditContextMenu.h"
#include "StudioClipboard.h"
#include "CoreUtils.h"
#include "UICMath.h"
#include "UICDMSignals.h"
#include "MouseCursor.h"
#include "StudioApp.h"
#include "Core.h"

#include <QApplication>

using namespace Q3DStudio;

//==============================================================================
//	Constants
//==============================================================================
const long BLINKSPEED =
    700; ///< Speed of the blinking cursor while editing, measured in milliseconds

IMPLEMENT_OBJECT_COUNTER(CTextEdit)

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

//==============================================================================
/**
 * Constructor
 */
CTextEdit::CTextEdit()
    : m_IsStringDirty(true)
    , m_NeedsCommit(false)
    , m_Alignment(RIGHT)
    , m_TotalCharWidth(0)
    , m_BufferLength(2)
    , m_MaxLength(QT3DS_MAX_I32)
    , m_IsReadOnly(false)
    , m_MouseIsDown(false)
    , m_FillBackground(true)
    , m_TextColor(0, 0, 0)
    , m_UseBGColorReadOnly(false)
    , m_BackgroundColorNoFocus(CStudioPreferences::GetTextBoxBGColorNoFocus())
    , m_BackgroundColorFocus(CStudioPreferences::GetTextBoxBGColorWithFocus())
    , m_BoldText(false)
    , m_ScrollAmount(0, 0)
    , m_CommandHandler(new CHotKeys())
{
    ADDTO_OBJECT_COUNTER(CTextEdit)

    m_Caret.color = ::CColor(0, 0, 0);
    m_Caret.position = 0;
    m_Caret.show = false;

    m_StartDragPt = CPt(0, 0);
    m_SelectionStart = 0;
    m_SelectionEnd = 0;

    m_TextColor = CStudioPreferences::GetRulerTickColor();

    RegisterCommands();
}

//==============================================================================
/**
 * Destructor
 */
CTextEdit::~CTextEdit()
{
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();

    REMOVEFROM_OBJECT_COUNTER(CTextEdit)

    // Added to help debug freeing memory allocated from inside a DLL
    m_DisplayString.Clear();
    delete m_CommandHandler;
}

//==============================================================================
/**
 * Sets text justification to left, right, or center.
 *
 * @param inAlignment Enumeration describing the t3xt justification
 */
void CTextEdit::SetAlignment(EAlignment inAlignment)
{
    m_Alignment = inAlignment;
}

//==============================================================================
/**
 * Draws the bounding rectangle and white edit box for the text edit class.  Also
 * draws the actual text in the box by calling GetString(), which should be
 * implemented by any class that extends CTextEdit.
 */
void CTextEdit::Draw(CRenderer *inRenderer)
{
    inRenderer->PushClippingRect(QRect(0, 0, GetSize().x, GetSize().y));

    // If the string has changed since we last drew it, cache the character widths
    if (m_IsStringDirty) {
        CalculateCharWidths(inRenderer);
        SetDirty(false);
    }

    DoFillBackground(inRenderer);

    // Draw the actual text
    DrawText(inRenderer);

    // Draw the caret last
    DrawCaret(inRenderer);

    inRenderer->PopClippingRect();
}

//==============================================================================
/**
 * Determines the bounding rectangle for the text based on the text justification
 * setting.
 *
 * @return the bounding rectangle for the text being displayed.
 */
CRct CTextEdit::GetTextRect()
{
    CRct theTextRect;

    switch (m_Alignment) {
    case LEFT:
        theTextRect = CRct(CPt(m_BufferLength, 0), CPt(::dtol(m_TotalCharWidth), GetSize().y));
        break;

    case CENTER:
        theTextRect = CRct(CPt(GetSize().x / 2 - ::dtol(m_TotalCharWidth / 2), 0),
                           CPt(::dtol(m_TotalCharWidth), GetSize().y));
        break;

    case RIGHT:
    // NO BREAK
    default: {
        theTextRect = CRct(CPt(GetSize().x - ::dtol(m_TotalCharWidth + m_BufferLength), 0),
                           CPt(::dtol(m_TotalCharWidth), GetSize().y));
    } break;
    }

    return theTextRect;
}

//==============================================================================
/**
 * Draws the text out to the screen.  If any selection is occurring, the
 * appropriate text is rendered out to the screen using the system preferences
 * for selection color.
 *
 * @param inRenderer Renderer to draw the text out to
 */
void CTextEdit::DrawText(CRenderer *inRenderer)
{
    Q3DStudio::CString theCompleteString = GetDisplayString();
    Q3DStudio::CString theFirstPart = theCompleteString;
    Q3DStudio::CString theHighlightedString;
    Q3DStudio::CString theLastPart;

    float theTextSizeX;
    float theTextSizeY;

    if (HasSelectedText()) {
        theFirstPart = theCompleteString.Extract(0, GetSelectionLeft());
        theHighlightedString =
            theCompleteString.Extract(GetSelectionLeft(), GetSelectionRight() - GetSelectionLeft());
        theLastPart = theCompleteString.Extract(GetSelectionRight(),
                                                theCompleteString.Length() - GetSelectionRight());
    }

    // Calculate where the upper left corner where text begins (we'll move this point for each part
    // of the text)
    float theUpperLeftX = static_cast<float>(GetTextRect().position.x + m_ScrollAmount.x);
    float theUpperLeftY = static_cast<float>(GetTextRect().position.y + m_ScrollAmount.y);

    const QRect sizeRect(0, 0, GetSize().x, GetSize().y);
    CRct theSelectionRect(0, 0, 0, 0);

    if (theFirstPart.Length() > 0) {
        if (!m_BoldText) {
            inRenderer->DrawText(theUpperLeftX, theUpperLeftY, theFirstPart.toQString(),
                                 CRct(GetSize()), m_TextColor);
        } else {
            inRenderer->DrawBoldText(theUpperLeftX, theUpperLeftY, theFirstPart.toQString(),
                                     CRct(GetSize()), m_TextColor);
        }

        // Move the upper left corner
        const auto textSize = inRenderer->GetTextSize(theFirstPart.toQString());
        theTextSizeX = textSize.width();
        theTextSizeY = textSize.height();
        theUpperLeftX += theTextSizeX;
    }

    if (theHighlightedString.Length() > 0) {
        // Draw the selection rectangle
        const auto textSize = inRenderer->GetTextSize(theHighlightedString.toQString());
        theTextSizeX = textSize.width();
        theTextSizeY = textSize.height();
        theSelectionRect.position.x = ::dtol(theUpperLeftX);
        theSelectionRect.size = CPt(::dtol(theTextSizeX), ::dtol(theTextSizeY) + 2);

        // If the caret is at the far right of the selection, shrink the highlight rect by one
        // pixel.  This is so that you can actually see the blinking cursor on the right edge.
        if (static_cast<long>(m_Caret.position) == GetSelectionRight())
            --theSelectionRect.size.x;

        inRenderer->FillSolidRect(theSelectionRect, CSystemPreferences::GetSelectedItemColor());

        // Draw the actual text in the highlight color
        if (!m_BoldText) {
            inRenderer->DrawText(theUpperLeftX, theUpperLeftY, theHighlightedString.toQString(),
                                 sizeRect, CSystemPreferences::GetSelectedTextColor());
        } else {
            inRenderer->DrawBoldText(theUpperLeftX, theUpperLeftY, theHighlightedString.toQString(),
                                     sizeRect, CSystemPreferences::GetSelectedTextColor());
        }

        // Move the upper left corner
        theUpperLeftX += theTextSizeX;
    }

    if (theLastPart.Length() > 0) {
        if (!m_BoldText) {
            inRenderer->DrawText(theUpperLeftX, theUpperLeftY, theLastPart.toQString(), sizeRect,
                                 m_TextColor);
        } else {
            inRenderer->DrawBoldText(theUpperLeftX, theUpperLeftY, theLastPart.toQString(), sizeRect,
                                     m_TextColor);
        }
    }
}

//==============================================================================
/**
 * Draws the caret out to the screen.
 *
 * @param inRenderer Renderer to draw the text out to
 */
void CTextEdit::DrawCaret(CRenderer *inRenderer)
{
    if (m_Caret.show && m_Caret.visible) {
        CRct theTextRect = GetTextRect();
        CPt theCaretPosition(theTextRect.position.x, 0);

        float theSizeX;
        float theSizeY;

        const auto textSize = inRenderer->GetTextSize(GetDisplayString().
                                                      Extract(0, m_Caret.position).toQString());
        theSizeX = textSize.width();
        theSizeY = textSize.height();

        theCaretPosition.x = ::dtol(theSizeX) + theTextRect.position.x;

        // Adjust the caret postion so that it draws just before the current character, instead of
        // on the current character
        theCaretPosition.x--;

        theCaretPosition += m_ScrollAmount;

        inRenderer->PushPen(::CColor(m_Caret.color));
        theCaretPosition.y = theCaretPosition.y;
        inRenderer->MoveTo(theCaretPosition);
        inRenderer->LineTo(theCaretPosition.x, GetSize().y - 1);
        inRenderer->PopPen();
    }
}

//=============================================================================
/**
 *	Inserts the specified character into the control
 */
bool CTextEdit::InsertChar(unsigned int inChar)
{
    bool theReturnValue = false;
    Q3DStudio::CString theTempString = m_DisplayString;

    if (!m_Caret.show)
        SelectAllText();

    long theLeftSelection = GetSelectionLeft();
    long theRightSelection = GetSelectionRight();
    long thePosition = m_Caret.position;

    if (theLeftSelection != theRightSelection) {
        Q3DStudio::CString theDisplayString = m_DisplayString;

        theTempString = "";
        theTempString = theDisplayString.Extract(0, theLeftSelection);
        theTempString += theDisplayString.Extract(theRightSelection,
                                                  theDisplayString.Length() - theRightSelection);
        thePosition = theLeftSelection;
    }

    if (CanAcceptChar(theTempString, inChar, thePosition)
        && theTempString.Length() < GetMaxLength()) {
        DeleteCurrentSelection(false);

        Q3DStudio::CString theChar = static_cast<wchar_t>(inChar);
        theTempString.Insert(m_Caret.position, theChar);
        SetData(theTempString);

        // Advance the caret past the character that we just typed
        MoveCaretTo(m_Caret.position + 1);

        ResetBlinkingCursor();
        theReturnValue = true;
    }

    return theReturnValue;
}

//=============================================================================
/**
 * Handles character input from the keyboard.
 *
 * @param inChar Character that was pressed
 * @return true if the character was handled, false if this control does not
 * care about the character that was pressed
 */
bool CTextEdit::OnChar(const QString &inChar, Qt::KeyboardModifiers inFlags)
{
    // 0x7F is a character generated by the delete key
    if (inChar.size() == 0 || inChar[0].unicode() == 0x7F)
        return false;

    bool theReturnValue = false;

    // Do not process the character if control is down, this will allow
    // app's hotkey to process application hotkeys.
    if (!(inFlags &  Qt::ControlModifier)) {
        // Always return true whenever it captures focus and tries insert character.
        // Though InsertChar fails, this is still getting the focus and have processed the
        // event, return true so that no other controls will handle it
        // Refer to Bug 897
        InsertChar(inChar[0].unicode());
        theReturnValue = true;
    }

    return theReturnValue;
}

//=============================================================================
/**
 * Virtual function that determines if a key recieved via OnChar can actually be
 * processed.  Sub-classes should override this implementation and provide their
 * own function for determining what characters they accept.
 *
 * @param inCheckString the string that is to be checked (in case a lenght requirement is exceeded)
 * @param inChar the character that was pressed
 * @param inPosition character index where we are attempting to insert inChar into inCheckString
 * @return true if this control can accept the character into the string, otherwise false
 */
bool CTextEdit::CanAcceptChar(const Q3DStudio::CString &inCheckString, unsigned int inChar,
                              unsigned int inPosition)
{
    Q_UNUSED(inCheckString);
    Q_UNUSED(inPosition);

    bool theAcceptFlag = false;

    if (inChar >= 32 && inChar <= 255 && !m_IsReadOnly)
        theAcceptFlag = true;

    return theAcceptFlag;
}

//=============================================================================
/**
 * Handles key presses.  OnKeyDown events pass in non-printable characters
 * such as the arrow keys or Enter.  Some of these keys will be handled by this
 * control.
 *
 * @param inChar the character that was pressed
 * @return true if the character was consumed by this control, otherwise false
 */
bool CTextEdit::OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    bool theHandledFlag = false;
    if (!m_IsReadOnly) {
        if (!m_CommandHandler->OnChar(inChar, 1, inFlags)) {
            theHandledFlag = HandleSpecialChar(inChar, inFlags);

            // Enter key commits the last change
            if (inChar == Qt::Key_Enter)
                FireCommitEvent();

            ResetBlinkingCursor();
            Invalidate();
        } else {
            theHandledFlag = true;
        }
    }
    return theHandledFlag;
}

//==============================================================================
/**
 * Handles any non-character keys that were pressed and need to be handled.
 *
 * @param inChar The key that was pressed
 * @param inFlags Indicates which modifier keys were down when event occurred
 * @return true if this function handled the character, otherwise false
 */
bool CTextEdit::HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    bool theHandledFlag = true;
    bool isShiftDown = inFlags & CHotKeys::MODIFIER_SHIFT ? true : false;
    // bool isControlDown = inFlags & CHotKeys::MODIFIER_CONTROL ? true : false;

    switch (inChar) {
    case Qt::Key_Left:
        // If we are cancelling selection, then we need to change how the caret moves
        if (HasSelectedText() && !isShiftDown)
            MoveCaretTo(GetSelectionLeft(), isShiftDown);
        // Otherwise, just move the caret left by one character
        else
            MoveCaretTo(m_Caret.position - 1, isShiftDown);
        break;

    case Qt::Key_Right:
        // If we are cancelling selection, then we need to change how the caret moves
        if (HasSelectedText() && !isShiftDown)
            MoveCaretTo(GetSelectionRight(), isShiftDown);
        // Otherwise, just move the caret right by one character
        else
            MoveCaretTo(m_Caret.position + 1, isShiftDown);
        break;

    case Qt::Key_Home:
        MoveCaretTo(0, isShiftDown);
        break;

    case Qt::Key_End:
        MoveCaretTo((long)m_CharWidths.size(), isShiftDown);
        break;

    case Qt::Key_Backspace:
        if (HasSelectedText())
            DeleteCurrentSelection();
        else
            DeleteCharacter(false);
        break;

    case Qt::Key_Delete: {
        if (HasSelectedText())
            DeleteCurrentSelection();
        else
            DeleteCharacter(true);
    } break;
    case Qt::Key_Return:
        EnterText(true);
        break;

    default:
        theHandledFlag = false;
        break;
    }

    return theHandledFlag;
}

//==============================================================================
/**
 * Handles mouse down events.  Places the caret in the appropriate place based
 * upon the mouse position.
 *
 * @param inLocation Location of the mouse at the time the click occurred
 * @param inFlags Flags indicating various key states (not used)
 * @return true (this control always handles mouse down events)
 */
void CTextEdit::OnMouseOver(CPt inLocation, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inLocation, inFlags);

    if (!m_IsReadOnly)
        setCursorIfNotSet(CMouseCursor::CURSOR_IBEAM);
}

//==============================================================================
/**
 * Handles mouse down events.  Places the caret in the appropriate place based
 * upon the mouse position.
 *
 * @param inLocation Location of the mouse at the time the click occurred
 * @param inFlags Flags indicating various key states (not used)
 * @return true (this control always handles mouse down events)
 */
bool CTextEdit::OnMouseDown(CPt inLocation, Qt::KeyboardModifiers inFlags)
{
    bool theReturnValue = CControl::OnMouseDown(inLocation, inFlags);

    if (!m_IsReadOnly) {
        // If the text is scrolled, we'll need to adjust for that
        inLocation -= m_ScrollAmount;

        m_StartDragPt = inLocation;
        m_MouseIsDown = true;

        ClearSelection();

        MoveCaretTo(ConvertPositionToIndex(inLocation), false, false);

        // Turn the caret on
        m_Caret.show = true;
        m_Caret.visible = true;

        ResetBlinkingCursor();

        m_TimerConnection =
            ITickTock::GetInstance().AddTimer(1000, true, std::bind(&CTextEdit::OnTimer, this),
                                              "CTextEdit::OnMouseDown::" + GetDisplayString());

        theReturnValue = true;
    }

    return theReturnValue;
}

//==============================================================================
/**
 * Called when the mouse button is released.  Disables any dragging that may
 * have been occurring.
 *
 * @param inPoint location of the mouse when the event occurred
 * @param inFlags various flags about other key states
 */
void CTextEdit::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_MouseIsDown = false;
    m_StartDragPt = CPt(0, 0);
    CControl::OnMouseUp(inPoint, inFlags);
}

//==============================================================================
/**
 * Handles mouse movement.  If the left mouse button is down and the user is
 * dragging the mouse over the text in the edit box, the appropriate text is
 * selected.
 *
 * @param inPoint location of the mouse.
 * @param inFlags not used
 */
void CTextEdit::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inFlags);

    if (!m_IsReadOnly)
        setCursorIfNotSet(CMouseCursor::CURSOR_IBEAM);

    if (m_MouseIsDown && !m_IsReadOnly) {
        // The point was scrolled in OnMouseDown, so we have to scroll it when we drag as well so
        // that it lines up
        inPoint -= m_ScrollAmount;

        // Figure out what character index inPoint.x translates to
        long theCharOffset = ConvertPositionToIndex(inPoint);

        MoveCaretTo(theCharOffset, true);
    }
}

//==============================================================================
/**
 * Converts a given pixel position on the screen into an index into the display
 * string.
 *
 * @param inPosition the point to be converted
 * @return the character index derived from inPostion
 */
long CTextEdit::ConvertPositionToIndex(CPt inPosition)
{
    CRct theTextRect = GetTextRect();
    long theIndex = 0;
    long theTotalPixels = theTextRect.position.x;

    // NOTE: We technically only care about the x-position of the point, so just ignore
    // inPosition.y if it is not within the text area.  This may need to be changed in
    // the future (perhaps if vertical alignment is implemented).
    CPt thePositionNoY = CPt(inPosition.x, theTextRect.position.y);

    // If the point is within the text area
    if (theTextRect.IsInRect(thePositionNoY)) {
        // Figure out which characters it falls in between
        for (TCharLengths::iterator thePos = m_CharWidths.begin(); thePos != m_CharWidths.end();
             ++thePos) {
            theTotalPixels += ::dtol(*thePos);
            if (inPosition.x < theTotalPixels)
                break;
            theIndex++;
        }
    }
    // If the point is to the left of the text, default the index to the beginning of the text
    else if (theTextRect.position.x > thePositionNoY.x) {
        theIndex = 0;
    }
    // Otherwise, the point must be to the right of the text, so move the index to the end of the
    // text
    else {
        theIndex = (long)m_CharWidths.size();
    }

    return theIndex;
}

//==============================================================================
/**
 * Converts a given character index into a pixel postion on the screen.  This
 * function is the inverse of ConvertPositionToIndex.
 * @param inIndex the character index to be converted
 * @return the x-position (in pixels) of character at inIndex
 */
long CTextEdit::ConvertIndexToPosition(long inIndex)
{
    long theXPixelOffset = GetTextRect().position.x;
    long theCurrentIndex = 0;

    for (TCharLengths::iterator thePos = m_CharWidths.begin(); thePos != m_CharWidths.end();
         ++thePos) {
        theCurrentIndex++;

        if (theCurrentIndex > inIndex) {
            break;
        }

        theXPixelOffset += ::dtol(*thePos);
    }

    return theXPixelOffset;
}

//==============================================================================
/**
 * Handles double clicks on the control.  Highlights all the text in the control.
 *
 * @param inPoint Not used
 * @param inFlags Not used
 * @return true (this control always handles double clicks)
 */
bool CTextEdit::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theHandledFlag = false;
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);
    if (!m_IsReadOnly) {
        SelectAllText();

        if (GetParent())
            GetParent()->GrabFocus(this);

        theHandledFlag = true;
    }

    return theHandledFlag;
}

//==============================================================================
/**
 * Selects all the text in the display string
 */
void CTextEdit::SelectAllText()
{
    if (!m_IsReadOnly) {
        MoveCaretTo(GetDisplayString().Length());
        SetSelection(0, GetDisplayString().Length());
        // Turn the caret on
        m_Caret.show = true;
    }
}

//==============================================================================
/**
 * Marks the string contained by this control as dirty so that it is reevaluated
 * the next time that the control draws itself.  Also clears out any selection
 * that may have been present, resetting the selection to the caret position.
 *
 * @param inDirty true to mark this control as dirty, which causes the string to be redrawn
 * false to mark the control as not needing to reevaluate its text during next draw cycle
 */
void CTextEdit::SetDirty(bool inDirty)
{
    m_IsStringDirty = inDirty;
    if (m_IsStringDirty) {
        m_NeedsCommit = true;
        ClearSelection();
        Invalidate();
    }
}

//==============================================================================
/**
 * @return true if the string is currently dirty and needs to be reevalutated.
 */
bool CTextEdit::IsDirty() const
{
    return m_IsStringDirty;
}

//==============================================================================
/**
 * Determines the width in pixels of each letter in the display string.
 *
 * @param inRenderer The current renderer being drawn to (since only the renderer
 * knows how to calculate the pixel width of the text in the current font).
 * @return The total width in pixels of the entire display string
 */
float CTextEdit::CalculateCharWidths(CRenderer *inRenderer)
{
    Q3DStudio::CString theString = GetDisplayString();

    m_CharWidths.clear();
    m_TotalCharWidth = 0;

    for (long theCharIndex = 0; theCharIndex < theString.Length(); ++theCharIndex) {
        const auto textSize = inRenderer->GetTextSize(theString.Extract(theCharIndex, 1).toQString());
        m_CharWidths.push_back(textSize.width());
        m_TotalCharWidth += textSize.width();
    }

    return m_TotalCharWidth;
}

//==============================================================================
/**
 * Sets the number of pixels worth of buffer space to place at the left and
 * right ends of the edit box.
 */
void CTextEdit::SetBufferLength(long inLength)
{
    m_BufferLength = inLength;
}

//==============================================================================
/**
 * Sets the maximum length of text
  */
void CTextEdit::SetMaxLength(long inLength)
{
    m_MaxLength = inLength;
}

//==============================================================================
/**
 * Allows you to turn the background fill on and off.
 * @param inFillBackground true to fill the background color when drawing, false to turn it off
 */
void CTextEdit::SetFillBackground(bool inFillBackground)
{
    if (m_FillBackground == inFillBackground)
        return;
    m_FillBackground = inFillBackground;
    Invalidate();
}

//==============================================================================
/**
 * Called when this control loses focus.  Hides the caret, clears the current
 * selection and invalidates the control so that it will get redrawn.
 */
void CTextEdit::OnLoseFocus()
{
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();
    m_Caret.show = false;
    m_ScrollAmount.x = 0;
    ClearSelection();
    m_IsStringDirty = true;
    FireCommitEvent();
    resetCursor();
    CControl::OnLoseFocus();
}

//==============================================================================
/**
 * Called when this control gains focus.  Shows the caret, clears the current
 * selection and invalidates the control so that it will get redrawn.
 */
void CTextEdit::OnGainFocus()
{
    if (!m_MouseIsDown) {
        m_Caret.position = 0;
        m_Caret.show = !m_IsReadOnly;
        if (!m_IsReadOnly)
            SelectAllText();

        if (m_Caret.show) {
            ResetBlinkingCursor();
            m_TimerConnection = ITickTock::GetInstance().AddTimer(
                1000, true, std::bind(&CTextEdit::OnTimer, this),
                "CTextEdit::OnGainFocus::" + GetDisplayString());
        }
    }

    Invalidate();
}

//==============================================================================
/**
 * Override to set whether or not this can gain focus.
 */
bool CTextEdit::CanGainFocus()
{
    return !m_IsReadOnly;
}

//==============================================================================
/**
 * If this control has focus, it's display may have a different string then the
 * actual underlying string.  This enables us to revert back to the original
 * string if the user presses the Escape while editing the text.  So this function
 * should be used to obtain the proper string to work with.  If this control has
 * focus, the internally stored string is returned, otherwise the CTextEdit asks
 * its child class to reevaluate the string.
 *
 * @return the string being displayed in the edit box
 */
Q3DStudio::CString CTextEdit::GetDisplayString()
{
    // If this control does not have focus, we should just reevaluate the string
    if (GetParent()) {
        if (!GetParent()->HasFocus(this))
            SetDisplayString(GetString(), false);
    }

    // If the edit box is read only, then add the pre and postfix,
    // otherwise, just the display string is drawn while editing
    Q3DStudio::CString theCompleteString = m_DisplayString;
    if (m_IsReadOnly) {
        theCompleteString = m_Prefix + m_DisplayString + m_Postfix;
    }

    return theCompleteString;
}

//==============================================================================
/**
 * Invokied in a Draw call. Derived class can override to do their own background fill
 */
void CTextEdit::DoFillBackground(CRenderer *inRenderer)
{
    if ((m_FillBackground && m_UseBGColorReadOnly) || !m_IsReadOnly) {
        // Fill the interior of the edit box
        ::CColor theFillColor = m_BackgroundColorNoFocus;
        // If this control has focus
        if (GetParent()) {
            // Lighten the fill color
            if (GetParent()->HasFocus(this))
                theFillColor = m_BackgroundColorFocus;
        }
        inRenderer->FillSolidRect(QRect(0, 0, GetSize().x, GetSize().y), theFillColor);
    }
}

//==============================================================================
/**
 * Selects the specified text within the edit box.  If inFirstIndex == inLastIndex
 * no text is selected.  If the indices specified do not fall within the size of
 * the actual text, no selection occurs.
 *
 * @param inFirstIndex Index of the first letter to be selected
 * @param inLastIndex Index of the last letter to be selected
 */
void CTextEdit::SetSelection(long inFirstIndex, long inLastIndex)
{
    if (inFirstIndex < 0 || inLastIndex > GetDisplayString().Length()) {
        ClearSelection();
    } else {
        if (m_SelectionStart != inFirstIndex || m_SelectionEnd != inLastIndex)
            Invalidate();

        m_SelectionStart = inFirstIndex;
        m_SelectionEnd = inLastIndex;
    }

    m_Caret.position = m_SelectionEnd;
}

//==============================================================================
/**
 * Sets this CTextEdit as either read-only or read/write.  When a CTextEdit
 * control is set to read-only, users cannot input data via the keyboard,
 * and the text cannot be selected.
 *
 * @param inReadOnly true to specify static, non-editable, non-selectable text
 */
void CTextEdit::SetReadOnly(bool inReadOnly)
{
    if (inReadOnly != m_IsReadOnly)
        Invalidate();

    m_IsReadOnly = inReadOnly;
}

//==============================================================================
/**
 * @return true if this control should not allow the string to be edited
 */
bool CTextEdit::IsReadOnly()
{
    return m_IsReadOnly;
}

//==============================================================================
/**
 * Sets the color of the text dispalyed by this control.
 * @param inColor Color of the text to be displayed
 */
void CTextEdit::SetTextColor(::CColor inColor)
{
    if (m_TextColor == inColor)
        return;

    m_TextColor = inColor;
    m_Caret.color = inColor;
    Invalidate();
}

//==============================================================================
/**
 * @return The current text color
 */
::CColor CTextEdit::GetTextColor()
{
    return m_TextColor;
}

//==============================================================================
/**
 * Sets the flag that says whether or not to use the m_BackgroundColorFocus
 * and m_BackgroundColorNoFocus user colors to draw the background color
 * of the text box.
 */
void CTextEdit::SetUseBGColorReadOnly(bool inUseBGColorReadOnly)
{
    m_UseBGColorReadOnly = inUseBGColorReadOnly;
}

//==============================================================================
/**
 * Sets the color of the background of this control when it does not have focus.
 * Note that this color only has an effect when SetFillBackground has been
 * called with true.
 * @param inColor Color of the background when the control does not have focus
 * @return The previous background color
 */
::CColor CTextEdit::SetBGColorNoFocus(::CColor inColor)
{
    ::CColor theOldColor = m_BackgroundColorNoFocus;
    m_BackgroundColorNoFocus = inColor;
    return theOldColor;
}

//==============================================================================
/**
 * Sets the color of the background of this control when it has focus.
 * Note that this color only has an effect when SetFillBackground has been
 * called with true.
 * @param inColor Color of the background when the control has focus
 * @return The previous background color
 */
::CColor CTextEdit::SetBGColorFocus(::CColor inColor)
{
    ::CColor theOldColor = m_BackgroundColorFocus;
    m_BackgroundColorFocus = inColor;
    return theOldColor;
}

void CTextEdit::AddCommitListener(CCommitDataListener *inListener)
{
    m_CommitListeners.AddListener(inListener);
}

void CTextEdit::RemoveCommitListener(CCommitDataListener *inListener)
{
    m_CommitListeners.RemoveListener(inListener);
}

//==============================================================================
/**
 *	Fires commit events on Listeners.  Only fires a commit event if it has not
 *	been fired previously.
 */
void CTextEdit::FireCommitEvent()
{
    // If we are not currently processing a command to fire commit events to listeners
    if (m_NeedsCommit) {
        // Fire the event and undirty the text
        m_CommitListeners.FireEvent(&CCommitDataListener::OnSetData, this);
        m_NeedsCommit = false;
    }
}

void CTextEdit::AddChangeListener(IChangeDataListener *inListener)
{
    m_ChangeListeners.AddListener(inListener);
}

void CTextEdit::RemoveChangeListener(IChangeDataListener *inListener)
{
    m_ChangeListeners.RemoveListener(inListener);
}

//==============================================================================
/**
 *	Fires change events on Listeners.
 */
void CTextEdit::FireChangeEvent(const Q3DStudio::CString &inOldString,
                                const Q3DStudio::CString &inNewString)
{
    m_ChangeListeners.FireEvent(&IChangeDataListener::OnChangeData, inOldString, inNewString);
}

void CTextEdit::AddEnterListener(ITextEditEnterListener *inListener)
{
    m_EnterListeners.AddListener(inListener);
}

void CTextEdit::RemoveEnterListener(ITextEditEnterListener *inListener)
{
    m_EnterListeners.RemoveListener(inListener);
}

//==============================================================================
/**
 *	Fires enter events on listeners.
 */
void CTextEdit::FireEnterEvent()
{
    m_EnterListeners.FireEvent(&ITextEditEnterListener::OnEnter, this);
}

//==============================================================================
/**
 * Delete any text that is currently selected.
 * If there is no text selected this will do nothing.
 */
void CTextEdit::DeleteCurrentSelection(bool inFireEvent /*=true*/)
{
    if (HasSelectedText()) {
        Q3DStudio::CString theTempString = m_DisplayString;
        theTempString.Delete(GetSelectionLeft(), GetSelectionRight() - GetSelectionLeft());

        m_Caret.position = GetSelectionLeft();

        ClearSelection();

        // @see DeleteCharacter( )
        SetData(theTempString, inFireEvent);
        SetDisplayString(theTempString, inFireEvent);
        SetDirty(true);
    }
}

//==============================================================================
/**
 * Delete a character to the left or right of the cursor.
 * This will delete a single character on the specified side of the cursor.
 * @param inOnRight true if the character to be deleted is on the right of the cursor.
 */
void CTextEdit::DeleteCharacter(bool inOnRight)
{
    if (!inOnRight) {
        if (m_Caret.position == 0)
            return;
        else
            m_Caret.position--;
    }

    Q3DStudio::CString theTempString = m_DisplayString;
    theTempString.Delete(m_Caret.position, 1);
    // Since SetData is overloaded by other types of text edit controls,
    // it has to be called first so that subclasses have a chance to set their internal values
    // before calling SetDisplayString, which fires a DataChanged event.
    // @see DeleteCurrentSelection( ). EnterText got it right.
    SetData(theTempString);
    SetDisplayString(theTempString);
    SetDirty(true);
}

//==============================================================================
/**
 * Move the caret to the specified position in the string.
 * If inSelect then this will select the text from the selection start position
 * to the new position. If there is no current selection start then this will
 * select from the old position to the new position. If select is false and there
 * is selected text the selection will be cleared.
 * @param inPosition the position to move to in the string.
 * @param inSelect true if the selection should be changed when moving.
 * @param inScroll true if moving the caret should allow the text to scroll within the box
 */
void CTextEdit::MoveCaretTo(long inPosition, bool inSelect /*=false*/, bool inScroll /*=true*/)
{
    if (inPosition < 0)
        inPosition = 0;
    if (inPosition > GetDisplayString().Length())
        inPosition = GetDisplayString().Length();

    // Only perform the operation if the caret is actually going to move, or the selection is going
    // to change
    if ((static_cast<unsigned long>(inPosition) != m_Caret.position)
        || (HasSelectedText() && !inSelect)) {
        if (inSelect) {
            if (!HasSelectedText())
                m_SelectionStart = m_Caret.position;
            m_SelectionEnd = inPosition;
        } else
            ClearSelection();

        m_Caret.position = inPosition;

        // Scroll if necessary
        if (inScroll) {
            long theCaretPixelPos = ConvertIndexToPosition(m_Caret.position);
            if (theCaretPixelPos + m_ScrollAmount.x <= 0)
                Scroll(m_ScrollAmount.x - (theCaretPixelPos + m_ScrollAmount.x) + m_BufferLength);
            else if (theCaretPixelPos + m_ScrollAmount.x > GetSize().x)
                Scroll(-(theCaretPixelPos - GetSize().x + m_BufferLength));
        }

        Invalidate();
    }
}

//==============================================================================
/**
 * Effectively scrolls the text in this edit box.
 * Enables you to set the number of pixels to offset the text and caret by when
 * they are drawn to the screen.  If inPixel = 0, no scrolling takes place.  If
 * inPixel < 0, text is scrolled to the left.  If inPixel > 0, text is scrolled
 * to the right.
 * @param inPixel specifies the horizontal scroll in pixels
 */
void CTextEdit::Scroll(long inPixel)
{
    m_ScrollAmount.x = inPixel;
    Invalidate();
}

//==============================================================================
/**
 * Pushes the display string back down to the control and optionally highlights
 * the entire string.  Called when the user clicks the enter button in an edit
 * box.
 * @param inHighlight true to highlight the string
 */
void CTextEdit::EnterText(bool inHighlight)
{
    SetData(m_DisplayString);
    SetDisplayString(GetString());

    if (inHighlight)
        SetSelection(0, m_DisplayString.Length());

    Invalidate();

    FireEnterEvent();
}

//==============================================================================
/**
 * Check to see if there is any text selected.
 * @return true if there is any text selected.
 */
bool CTextEdit::HasSelectedText()
{
    return m_SelectionStart != m_SelectionEnd;
}

//==============================================================================
/**
 * Get the index of the leftmost selected character.
 * @return the index, invalid if no text is selected.
 */
long CTextEdit::GetSelectionLeft()
{
    return m_SelectionStart < m_SelectionEnd ? m_SelectionStart : m_SelectionEnd;
}

//==============================================================================
/**
 * Get the index of the rightmost selected character.
 * @return the index, invalid if no text is selected.
 */
long CTextEdit::GetSelectionRight()
{
    return m_SelectionStart > m_SelectionEnd ? m_SelectionStart : m_SelectionEnd;
}

//==============================================================================
/**
 * Clear any text from being selected, this will not delete the text, only deselect it.
 */
void CTextEdit::ClearSelection()
{
    m_SelectionStart = 0;
    m_SelectionEnd = 0;

    Invalidate();
}

//==============================================================================
/**
 *	Re-get the display string.
 */
void CTextEdit::ReloadData()
{
    SetDisplayString(GetString());
}

//==============================================================================
/**
 *	Supply a prefix to be displayed when the string is not being edited (e.g. "#").
 */
void CTextEdit::SetPrefix(const Q3DStudio::CString &inPrefix)
{
    m_Prefix = inPrefix;
}

//==============================================================================
/**
 *	Supply a postfix to be displayed when the string is not being edited (e.g. " inches").
 */
void CTextEdit::SetPostfix(const Q3DStudio::CString &inPostfix)
{
    m_Postfix = inPostfix;
}

//==============================================================================
/**
 * Handles the hotkey commands
 */

void CTextEdit::RegisterCommands()
{
    m_CommandHandler->RegisterKeyEvent(new CDynHotKeyConsumer<CTextEdit>(this, &CTextEdit::CopyText),
                                       Qt::ControlModifier, Qt::Key_C);
    m_CommandHandler->RegisterKeyEvent(
        new CDynHotKeyConsumer<CTextEdit>(this, &CTextEdit::PasteText),  Qt::ControlModifier,
        Qt::Key_V);
    m_CommandHandler->RegisterKeyEvent(new CDynHotKeyConsumer<CTextEdit>(this, &CTextEdit::CutText),
                                       Qt::ControlModifier, Qt::Key_X);
    m_CommandHandler->RegisterKeyEvent(
        new CDynHotKeyConsumer<CTextEdit>(this, &CTextEdit::SelectAllText),
         Qt::ControlModifier, Qt::Key_A);
}

//==============================================================================
/**
 *	Copies selected text to clipboard.  If nothing is selected, everything is copied.
 *	@return true if text was copied
 */
bool CTextEdit::CopyText()
{
    if (!m_IsReadOnly) {
        if (HasSelectedText())
            CStudioClipboard::CopyTextToClipboard(m_DisplayString.Extract(
                GetSelectionLeft(), GetSelectionRight() - GetSelectionLeft()).toQString());
        else
            CStudioClipboard::CopyTextToClipboard(m_DisplayString.toQString());
    }
    return !m_IsReadOnly;
}

//==============================================================================
/**
 * Pastes text to clipboard
 */

bool CTextEdit::PasteText()
{
    Q3DStudio::CString theText = Q3DStudio::CString
            ::fromQString(CStudioClipboard::GetTextFromClipboard());
    bool theHandledFlag = !m_IsReadOnly && CanPaste() && !theText.IsEmpty();

    theHandledFlag &= theText.Length() <= GetMaxLength();
    if (HasSelectedText()) {
        theHandledFlag &=
            m_DisplayString.Length() - (GetSelectionRight() - GetSelectionLeft()) + theText.Length()
            <= GetMaxLength();
    } else {
        theHandledFlag &= m_DisplayString.Length() + theText.Length() <= GetMaxLength();
    }

    if (theHandledFlag) {
        if (!m_Caret.show) {
            SetData(theText);
        } else {
            Q3DStudio::CString theNewString(m_DisplayString);

            if (HasSelectedText()) {
                theNewString.Delete(GetSelectionLeft(), GetSelectionRight() - GetSelectionLeft());
                m_Caret.position = Q3DStudio::MIN(GetSelectionLeft(), GetSelectionRight());
            }
            theNewString.Insert(m_Caret.position, theText);

            SetData(theNewString);
            m_Caret.position += theText.Length();
        }
        SetDirty(true);
    }
    return theHandledFlag;
}

//==============================================================================
/**
 * Cuts text to clipboard
 */

bool CTextEdit::CutText()
{
    bool theReturn = false;

    if (CopyText()) {
        DeleteCurrentSelection();
        theReturn = true;
    }

    return theReturn;
}

//==============================================================================
/**
 * Reloads the display string with the given string
 * @param inString The string to be set
 */
void CTextEdit::SetDisplayString(const Q3DStudio::CString &inString, bool inFireEvent /*= true*/)
{
    if (m_DisplayString != inString) {
        Q3DStudio::CString theOldString = m_DisplayString;
        m_DisplayString = inString;

        if (inFireEvent)
            FireChangeEvent(theOldString, m_DisplayString);

        m_IsStringDirty = true;
    }
}

//==============================================================================
/**
 * The buffer length is the number of pixels between where the control starts
 * and where the text begins drawing (in the case of left alignment).  In the
 * case of right alignment it's the number of pixels on the right side of the
 * control where the text ends before the actual edge of the control.  Provided
 * for sub classes.
 */
long CTextEdit::GetBufferLength()
{
    return m_BufferLength;
}

//==============================================================================
/**
 * The maximum allowed length of text
 */
long CTextEdit::GetMaxLength()
{
    return m_MaxLength;
}

//==============================================================================
/**
 * Displays the Copy/Cut/Paste context menu on right mouse down
 */
void CTextEdit::DisplayContextMenu(CPt inPt)
{
    CTextEditContextMenu theMenu(this);
    DoPopup(&theMenu, inPt);
}

//==============================================================================
/**
 * Handles the Right mouse down event.  Displays a cut/copy/paste context menu.
 * @param inPoint location of the mouse at time of the event (context menu's upper left corner will
 * be here)
 * @param inFlags state of the modifier keys at the time of the event
 * @return true - this control always consumes the event
 */
bool CTextEdit::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // Give children the chance to handle the event, otherwise display the context menu
    if (!CControl::OnMouseRDown(inPoint, inFlags)) {
        if (!IsReadOnly()) {
            DisplayContextMenu(CPt(inPoint.x, GetSize().y));
            return true;
        }
    }
    return false;
}

//==============================================================================
/**
 * Periodic callback to make the caret blink.
 */
void CTextEdit::OnTimer()
{
    m_Caret.visible = !m_Caret.visible;
    Invalidate();
}

//==============================================================================
/**
 * Cancels the current timer and adds a new timer so that the cursor continues
 * to blink.  If the cursor is not currently visible, it is made visible.
 */
void CTextEdit::ResetBlinkingCursor()
{
    // Cancel the current timer
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();

    // If the cursor is not visible, make it visible
    if (!m_Caret.visible) {
        m_Caret.visible = true;
        Invalidate();
    }

    // Add the timer so that the cursor will blink back off after BLINKSPEED amount of time
    m_TimerConnection =
        ITickTock::GetInstance().AddTimer(BLINKSPEED, true, std::bind(&CTextEdit::OnTimer, this),
                                          "CTextEdit::ResetBlinkingCursor::" + m_DisplayString);
}

//==============================================================================
/**
 * Set to true to indicate text should be drawn bold.
 * @param inBold true to set text to bold
 */
void CTextEdit::SetBoldText(bool inBold)
{
    m_BoldText = inBold;
}
