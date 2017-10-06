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
#include "PopupWnd.h"
#include "AppFonts.h"

#include "StudioPreferences.h"

//==============================================================================
/**
 *	Constructor:	Initialize the object.
 *
 *	Creates default fonts, colors, etc.
 */
CPopupWnd::CPopupWnd()
    : m_CenterPt(0, 0)
    , m_IsStationary(false)
{
    // get the proper font from appfonts
    m_Font = CAppFonts::GetInstance()->GetNormalFont();

    // Set the default colors (use default system tooltip colors)
    m_TextColor = ::GetSysColor(COLOR_INFOTEXT);
    m_BkColor = ::GetSysColor(COLOR_INFOBK);
    ::CColor theBGColor(CStudioPreferences::GetTooltipBackgroundColor());
    m_BkColor = theBGColor.GetRGBColor();
    m_BkBrush = new CBrush();
    m_BkBrush->CreateSolidBrush(m_BkColor);
}

//==============================================================================
/**
 *	Destructor:		Destroys the object and releases associated memory.
 */
CPopupWnd::~CPopupWnd()
{
    // Clean up GDI objects

    //	m_Font.DeleteObject();

    if (m_BkBrush) {
        DeleteObject(m_BkBrush);
        delete m_BkBrush;
    }
}

//==============================================================================
//	Message Map
//==============================================================================
BEGIN_MESSAGE_MAP(CPopupWnd, CWnd)
//{{AFX_MSG_MAP(CPopupWnd)
ON_WM_PAINT()
ON_WM_ERASEBKGND()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
/**
 *	OnPaint:	Handles the WM_PAINT windows message.
 *
 *	Calls GetWindowText() and draws the text to the screen.
 */
void CPopupWnd::OnPaint()
{
    CPaintDC theDC(this); // device context for painting
    CString theText;

    GetWindowText(theText);
    if (theText.GetLength() > 0) {

        // Change the font
        CFont *theOldFont = theDC.SelectObject(m_Font);
        theDC.SetTextColor(m_TextColor);
        theDC.SetBkColor(m_BkColor);

        // Paint the text
        CRect theClientRect;
        this->GetClientRect(theClientRect);
        theDC.DrawText(theText, theClientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Restore original font
        theDC.SelectObject(theOldFont);
    }
}

//==============================================================================
/**
 *	SetWindowText:	Sets the text for this window.
 *
 *	Overridden from parent so that we can calculate how big we need to make the
 *	pop-up window so that the text fits.  Automatically adjusts the window
 *	size/position to fit the new text string.
 */
void CPopupWnd::SetWindowText(LPCTSTR lpszString)
{
    CString theNewText;
    theNewText.Format(L" %s ", lpszString);

    // Set the window text
    CWnd::SetWindowText(theNewText);

    // Calculate the new window rect with the specified text
    CDC *theDC = GetDC();
    CRect theRect(0, 0, 1, 1);
    CFont *theOldFont = theDC->SelectObject(m_Font);

    theDC->DrawText(theNewText, theRect, DT_CALCRECT);
    theDC->SelectObject(theOldFont);
    ::AdjustWindowRect(theRect, this->GetStyle(), FALSE);
    CPoint theTopLeft = theRect.TopLeft();
    CPoint theBottomRight = theRect.BottomRight();
    ::ClientToScreen(this->GetParent()->GetSafeHwnd(), &theTopLeft);
    ::ClientToScreen(this->GetParent()->GetSafeHwnd(), &theBottomRight);
    CRect theAdjustedRect(theTopLeft, theBottomRight);
    int theX = abs((int)(theAdjustedRect.Width() / 2) - m_CenterPt.x);
    int theY = abs((int)(theAdjustedRect.Height() / 2) - m_CenterPt.y);

    ReleaseDC(theDC);

    // Move the window to the new position, with the new size, still centered at m_CenterPt
    this->SetWindowPos(nullptr, theX, theY, theAdjustedRect.Width(), theAdjustedRect.Height(),
                       SWP_NOZORDER | SWP_NOACTIVATE);
}

//==============================================================================
/**
 *	SetCenterPoint:	Changes the center of this window.
 *
 *	Use this function to specify where you want the center of the pop-up window
 *	to appear.  The window is automatically moved to the new center position.
 */
void CPopupWnd::SetCenterPoint(CPoint inPoint)
{
    if ((m_IsStationary && !IsWindowVisible()) || !m_IsStationary) {
        // Store the center point
        m_CenterPt = inPoint;

        // Calculate the new window position so that it is centered at the new point
        CRect theWindowRect;
        this->GetWindowRect(theWindowRect);
        CPoint theTopLeft = theWindowRect.TopLeft();
        CPoint theBottomRight = theWindowRect.BottomRight();
        ::ClientToScreen(this->GetParent()->GetSafeHwnd(), &theTopLeft);
        ::ClientToScreen(this->GetParent()->GetSafeHwnd(), &theBottomRight);
        CRect theAdjustedRect(theTopLeft, theBottomRight);
        int theX = abs((int)(theAdjustedRect.Width() / 2) - m_CenterPt.x);
        int theY = abs((int)(theAdjustedRect.Height() / 2) - m_CenterPt.y);

        // Move the window
        SetWindowPos(nullptr, theX, theY, theAdjustedRect.Width(), theAdjustedRect.Height(),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

//==============================================================================
/**
 *	SetBkColor:	Changes the background color of this window.
 *
 *	Defaults to COLOR_INFOBK (tooltip background color).  Creates a brush used
 *	in OnEraseBkgnd() of the specified color.
 *
 *	@param	inColor	The new background color.
 */
void CPopupWnd::SetBkColor(COLORREF inColor)
{
    m_BkColor = inColor;

    // If we already have a background brush
    if (m_BkBrush) {
        // Kill it
        DeleteObject(m_BkBrush);
        delete m_BkBrush;
    }

    // Make a brush of the specified color
    m_BkBrush = new CBrush();
    m_BkBrush->CreateSolidBrush(m_BkColor);
}

//==============================================================================
/**
 *	OnEraseBkgnd:	Handles the WM_ERASEBKGND windows message.
 *	Redraws the client rect using the current background brush.
 *
 *	@param pDC	Pointer to a display context for drawing.
 *	@return TRUE.
 */
BOOL CPopupWnd::OnEraseBkgnd(CDC *pDC)
{
    // Erase the client rect using the background brush so it's the right color
    CRect theClientRect;
    this->GetClientRect(theClientRect);
    pDC->FillRect(theClientRect, m_BkBrush);
    return TRUE;
}

//==============================================================================
/**
 *	UpdateToolTip:	Allows you to change multiple properties on this window at once.
 *
 *	@param	inCenter		New center point of the popup window
 *	@param	inText			New window text for the window
 *	@param	inShowWindow	true to show the window, false to hide the window
 */
void CPopupWnd::UpdateToolTip(CPoint inCenter, CString inText, bool inShowWindow)
{
    int theShowFlag = (inShowWindow) ? SW_SHOWNA : SW_HIDE;

    this->SetCenterPoint(inCenter);
    this->SetWindowText(inText);
    this->ShowWindow(theShowFlag);
    this->Invalidate(FALSE);
    this->UpdateWindow();
}
