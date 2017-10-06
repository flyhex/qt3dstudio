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

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//==============================================================================
//	Includes
//==============================================================================

#include "BaseLink.h"
#include "Resource.h"
#include "StudioColors.h"

// The IDC_HAND should be defined in WinUser.h if the WINVER is 0x5000 or greater
#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseLink class

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================
CBaseLink::CBaseLink()
{
    m_bCapture = FALSE;
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CBaseLink::~CBaseLink()
{
}

//==============================================================================
//	Message Map
//==============================================================================

BEGIN_MESSAGE_MAP(CBaseLink, CWnd)
//{{AFX_MSG_MAP(CBaseLink)
ON_WM_PAINT()
ON_WM_SETCURSOR()
ON_WM_MOUSEMOVE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
/**
 *	ShowTextLink: Shows the text link.
 *
 *	@param	None
 */
//==============================================================================
void CBaseLink::ShowTextLink(COLORREF inTextColor, COLORREF inBackColor, UINT inTextFormat,
                             bool inUnderlineText)
{
    if (IsWindow(m_hWnd)) {
        CRect theRect;
        CFont *theFont;
        CFont *theOldFont = nullptr;
        CDC *theDC;
        CDC theMemDC;
        CString theTextBuffer;

        // Set up the link font.
        theFont = GetFont();
        if (theFont != nullptr) {
            LOGFONT theLogFont;

            theFont->GetLogFont(&theLogFont);

            // Turn on the underline style...
            if (inUnderlineText)
                theLogFont.lfUnderline = TRUE;
            else
                theLogFont.lfUnderline = FALSE;

            theFont = new CFont();

            if (theFont != nullptr) {
                theFont->CreateFontIndirect(&theLogFont);
            }
        }

        GetClientRect(&theRect);
        GetWindowText(theTextBuffer);

        theDC = GetDC();

        theDC->SetTextColor(inTextColor);
        theDC->SetBkColor(inBackColor);

        // Fill the background.
        theDC->FillSolidRect(&theRect, inBackColor);

        // Set the font and draw the text.
        if (theFont != nullptr) {
            theOldFont = (CFont *)theDC->SelectObject(theFont);
        }
        theDC->DrawText(theTextBuffer, &theRect, inTextFormat);

        if (theOldFont != nullptr) {
            theDC->SelectObject(theOldFont);
        }
        ReleaseDC(theDC);

        if (theFont != nullptr) {
            theFont->DeleteObject();
            delete theFont;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CBaseLink message handlers

//==============================================================================
/**
 *	OnPaint - Handler for the WM_PAINT message
 *
 *	Draw the control.
 *
 *	@param	None
 */
//==============================================================================
void CBaseLink::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    if (IsWindowEnabled())
        this->ShowLink();
}

//==============================================================================
/**
 *	OnSetCursor
 *
 *	Handle the WM_SETCURSOR message.
 *
 *	@param	inWnd		Pointer to the parent CWnd object.
 *	@param	inHitTest	Specifies the hit-test area code. The hit test determines the
 *cursor�s location.
 *	@param	inMessage	Specifies the mouse message number.
 */
//==============================================================================
BOOL CBaseLink::OnSetCursor(CWnd *inWnd, UINT inHitTest, UINT inMessage)
{
    UNREFERENCED_PARAMETER(inWnd);
    UNREFERENCED_PARAMETER(inHitTest);
    UNREFERENCED_PARAMETER(inMessage);

    SetCursor(LoadCursor(nullptr, IDC_HAND));

    return TRUE;
}

//==============================================================================
/**
 *	OnMouseMove
 *
 *	Windows callback handler for the WM_MOUSEMOVE message
 *
 *	@param	inFlags		Flags for the left button up message
 *	@param	inPoint		Point where the left button was last moved (client coordinates)
 */
//==============================================================================
void CBaseLink::OnMouseMove(UINT inFlags, CPoint inPoint)
{
    UNREFERENCED_PARAMETER(inFlags);
    UNREFERENCED_PARAMETER(inPoint);

    if (IsWindowEnabled()) {

        if (!m_bCapture) {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            SetCapture();

            m_bCapture = TRUE;
            this->ShowLink();
        } else {
            // Already captured...
            if (!this->IsMouseOverLink()) {
                m_bCapture = FALSE;
                ReleaseCapture();

                this->ShowLink();
            }
        }
    }
}

//==============================================================================
/**
 *	IsMouseOverLink: Determine if the mouse is over the control.
 *
 *	@param	None
 *
 *	@return	true if the mouse is over the control.
 */
//==============================================================================
bool CBaseLink::IsMouseOverLink()
{
    POINT theMousePos;
    RECT theWndRect;

    ::GetCursorPos(&theMousePos);
    GetWindowRect(&theWndRect);

    return PtInRect(&theWndRect, theMousePos) ? true : false;
}
