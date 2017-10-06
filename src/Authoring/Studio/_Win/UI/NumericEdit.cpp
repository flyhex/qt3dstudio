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

#include "NumericEdit.h"

#include <minmax.h>

/////////////////////////////////////////////////////////////////////////////
// CNumericEdit class

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================
CNumericEdit::CNumericEdit()
{
    m_RangeLow = -1;
    m_RangeHigh = -1;

    m_WrapFlag = FALSE;
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CNumericEdit::~CNumericEdit()
{
}

//=============================================================================
/**
 *  Message Map
 */
//=============================================================================
BEGIN_MESSAGE_MAP(CNumericEdit, CEdit)
//{{AFX_MSG_MAP(CNumericEdit)
ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillFocus)
ON_CONTROL_REFLECT(EN_UPDATE, OnUpdate)
ON_WM_CHAR()
ON_WM_MOUSEWHEEL()
ON_WM_KEYDOWN()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//=============================================================================
/**
 *	SetRange: Sets the value range for this control.
 *
 *	@param	inRangeLow	Low range.
 *	@param	inRangeHigh	High range
 */
//=============================================================================
void CNumericEdit::SetRange(long inRangeLow, long inRangeHigh)
{
    // Ensure they are in low/high order
    m_RangeLow = min(inRangeLow, inRangeHigh);
    m_RangeHigh = max(inRangeLow, inRangeHigh);
}

//=============================================================================
/**
 *	SetValue: Sets the numeric value for this control.
 *
 *	@param	inValue		The new value for this control.
 */
//=============================================================================
void CNumericEdit::SetValue(long inValue)
{
    CString theStrValue;

    theStrValue.Format(L"%lu", inValue);
    this->SetWindowText(theStrValue);
}

//=============================================================================
/**
 *	GetValue: Returns the numeric value for this control.
 *
 *	@param	None
 *
 *	@return	long	The numeric value for this control.
 */
//=============================================================================
long CNumericEdit::GetValue()
{
    CString theStrValue;

    // Get the current control value.
    this->GetWindowText(theStrValue);

    // Convert to long and return.
    return atol((char *)LPCTSTR(theStrValue));
}

/////////////////////////////////////////////////////////////////////////////
// CNumericEdit message handlers

//=============================================================================
/**
 *	OnKillFocus: Handle the EN_KILLFOCUS and validate data.
 *
 *	@param	None
 */
//=============================================================================
void CNumericEdit::OnKillFocus()
{
    this->ValidateData();
}

//=============================================================================
/**
 *	OnUpdate: Handle the EN_UPDATE and handle validation before the control is updated.
 *
 *	@param	None
 */
//=============================================================================
void CNumericEdit::OnUpdate()
{
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CEdit::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_UPDATE flag ORed into the lParam mask.

    // TODO: Add your control notification handler code here

    this->ValidateData();
}

//=============================================================================
/**
 *	OnChar: Handle the WM_CHAR and validate the key that was pressed.
 *
 *	@param	inChar
 *	@param	inRepCnt
 *	@param	inFlags
 */
//=============================================================================
void CNumericEdit::OnChar(UINT inChar, UINT inRepCnt, UINT inFlags)
{
    if (IsNumeric(inChar) || IsEditKey(inChar)) {
        CEdit::OnChar(inChar, inRepCnt, inFlags);
    }
}

//=============================================================================
/**
 *	OnKeyDown: Handle the WM_KEYDOWN and validate the key that was pressed.
 *
 *	@param	inChar
 *	@param	inRepCnt
 *	@param	inFlags
 */
//=============================================================================
void CNumericEdit::OnKeyDown(UINT inChar, UINT inRepCnt, UINT inFlags)
{
    if (inChar == VK_UP) {
        this->IncrementValue(+1);
        this->SetSel(0, -1);
    } else if (inChar == VK_DOWN) {
        this->IncrementValue(-1);
        this->SetSel(0, -1);
    } else {
        CEdit::OnKeyDown(inChar, inRepCnt, inFlags);
    }
}

//=============================================================================
/**
 *	IsEditKey: Determine if the pressed key was an edit key.
 *
 *	@param	inChar		Key character value.
 *
 *	@return	TRUE if an editing key
 */
//=============================================================================
BOOL CNumericEdit::IsEditKey(UINT inChar)
{
    BOOL theEditKeyFlag = FALSE;

    if (inChar == VK_DELETE || inChar == VK_BACK || inChar == VK_LEFT || inChar == VK_RIGHT
        || inChar == VK_HOME || inChar == VK_END || inChar == VK_UP || inChar == VK_DOWN)
        theEditKeyFlag = TRUE;

    return theEditKeyFlag;
}

//=============================================================================
/**
 *	IsNumeric: Determine if the pressed key was a numeric key.
 *
 *	@param	inChar		Key character value.
 *
 *	@return	TRUE if a numeric key
 */
//=============================================================================
BOOL CNumericEdit::IsNumeric(UINT inChar)
{
    CString theNumericCheck = L"0123456789";
    BOOL theNumericFlag = FALSE;

    // Determine if the character is a numeric value.
    if (theNumericCheck.Find((TCHAR)inChar) != -1) {
        theNumericFlag = TRUE;
    }

    return theNumericFlag;
}

//=============================================================================
/**
 *	ValidateData: Validates the value in the control to keep within range.
 *
 *	@param	None
 */
//=============================================================================
void CNumericEdit::ValidateData()
{
    CString theStrValue;
    long theValue = 0;
    BOOL theChangeFlag = FALSE;

    // Get the current text in the control.
    this->GetWindowText(theStrValue);
    if (theStrValue.GetLength() == 0) {
        theValue = 0;
        theChangeFlag = TRUE;
    } else
        // Check to see that a range has been set
        if (m_RangeLow != -1 && m_RangeHigh != -1) {
        theValue = this->GetValue();

        // Check against the low and high range
        if (theValue < m_RangeLow) {
            theValue = m_RangeLow;
            theChangeFlag = TRUE;
        } else if (theValue > m_RangeHigh) {
            theValue = m_RangeHigh;
            theChangeFlag = TRUE;
        }
    }

    // Has the value changed?
    if (theChangeFlag) {
        // Update the control's value.
        this->SetValue(theValue);
        this->SetSel(0, -1);
    }
}

//=============================================================================
/**
 *	IncrementValue: Increments the current value by inAmount
 *
 *	@param	inAmount	Amount to change the current control value.
 */
//=============================================================================
void CNumericEdit::IncrementValue(long inAmount)
{
    long theValue;

    // Get the current value
    theValue = this->GetValue();

    // Change theValue
    theValue += inAmount;

    // Has a range been set?
    if (m_RangeLow != -1 && m_RangeHigh != -1) {
        if (theValue < m_RangeLow) {
            theValue = m_RangeLow;

            if (m_WrapFlag)
                theValue = m_RangeHigh;
        } else if (theValue > m_RangeHigh) {
            theValue = m_RangeHigh;

            if (m_WrapFlag)
                theValue = m_RangeLow;
        }
    }

    this->SetValue(theValue);
}

//=============================================================================
/**
 *	OnMouseWheel: Handle the WM_MOUSEWHEEL message.
 *
 *	@param	inFlags
 *	@param	inzDelta
 *	@param	inPoint
 *
 *	@return	TRUE if processed.
 */
//=============================================================================
BOOL CNumericEdit::OnMouseWheel(UINT inFlags, short inzDelta, CPoint inPoint)
{
    long theWheelDelta = 0;

    if (inzDelta != 0)
        theWheelDelta = inzDelta / abs(inzDelta);

    this->IncrementValue(theWheelDelta);

    this->SetSel(0, -1);

    return CEdit::OnMouseWheel(inFlags, inzDelta, inPoint);
}
