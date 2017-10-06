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

#include "WebLink.h"
#include "Resource.h"
#include "BCMenu.h"
#include "StudioColors.h"

/////////////////////////////////////////////////////////////////////////////
// CWebLink class

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================
CWebLink::CWebLink()
    : CBaseLink()
{
    // Set default link colors
    m_ColorLink = CStudioPreferences::GetMasterColor(); // green
    m_ColorHighlight = RGB(0xFF, 0x00, 0x00); // red
    m_ColorVisited = RGB(0x7F, 0x00, 0x7F); // purple
    m_ColorBackground = CStudioPreferences::GetDarkBaseColor();

    m_bVisited = FALSE;

    m_URL.Empty();

    m_Bitmap = nullptr;

    m_bPopup = FALSE;
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CWebLink::~CWebLink()
{
}

//==============================================================================
//	Message Map
//==============================================================================

BEGIN_MESSAGE_MAP(CWebLink, CWnd)
//{{AFX_MSG_MAP(CWebLink)
ON_WM_PAINT()
ON_WM_SETCURSOR()
ON_WM_LBUTTONDOWN()
ON_WM_MOUSEMOVE()
ON_WM_CONTEXTMENU()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
/**
 *	SetURL: Sets the URL for the control.
 *
 *	@param	inURL	URL to point the default browser to when the control is clicked.
 */
//==============================================================================
void CWebLink::SetURL(CString inURL)
{
    m_URL = inURL;
}

//==============================================================================
/**
 *	SetImage: Sets an bitmap image for the control.
 *
 *	@param	inResBmp		BITMAP resource for the image to load.
 *	@param	inAutoSizeFlag	TRUE if the control should be resized to the image size.
 */
//==============================================================================
void CWebLink::SetImage(UINT inResBmp, bool inAutoSizeFlag)
{
    HBITMAP theBitmap = (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inResBmp),
                                           IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    if (theBitmap != nullptr) {
        this->SetImage(theBitmap, inAutoSizeFlag);
    }
}

//==============================================================================
/**
 *	SetImage: Sets an bitmap image for the control.
 *
 *	@param	inBitmap		HBITMAP handle of the loaded bitmap.
 *	@param	inAutoSizeFlag	TRUE if the control should be resized to the image size.
 */
//==============================================================================
void CWebLink::SetImage(HBITMAP inBitmap, bool inAutoSizeFlag)
{
    m_Bitmap = inBitmap;

    if (inAutoSizeFlag && m_Bitmap != nullptr) {
        // Resize this control based on the size of the bitmap.
        BITMAP theBitmapStruct;
        int theWidth, theHeight;

        GetObject(m_Bitmap, sizeof(BITMAP), &theBitmapStruct);
        theWidth = theBitmapStruct.bmWidth;
        theHeight = theBitmapStruct.bmHeight;

        SetWindowPos(nullptr, 0, 0, theWidth, theHeight, SWP_NOZORDER | SWP_NOMOVE);
    }

    this->ShowLink();
}

//==============================================================================
/**
 *	SetText: Sets the text for the control.
 *
 *	@param	inText	Text for the control.
 */
//==============================================================================
void CWebLink::SetText(CString inText)
{
    SetWindowText(inText);
    m_Bitmap = nullptr;
    this->ShowLink();
}

COLORREF CWebLink::GetColor()
{
    if (m_bCapture) {
        return m_ColorHighlight;
    } else {
        if (m_bVisited)
            return m_ColorVisited;
        else
            return m_ColorLink;
    }
}

//==============================================================================
/**
 *	ShowLink: Shows the link.
 *
 *	@param	None
 */
//==============================================================================
void CWebLink::ShowLink()
{
    if (IsWindow(m_hWnd)) {
        // Is the link represented by an image?
        if (m_Bitmap != nullptr) {
            ShowBitmapLink();
        } else {
            ShowTextLink(GetColor(), m_ColorBackground, DT_WORDBREAK | SS_LEFT, true);
        }
    }
}

//==============================================================================
/**
 *	ShowLink: Shows the bitmap link.
 *
 *	@param	None
 */
//==============================================================================
void CWebLink::ShowBitmapLink()
{
    if (IsWindow(m_hWnd)) {
        CRect theRect;
        CDC *theDC;
        CDC theMemDC;

        BITMAP theBitmapStruct;
        long theWidth, theHeight;
        CBrush theBrush;

        theDC = GetDC();
        theMemDC.CreateCompatibleDC(theDC);
        CBitmap *theBitmap = CBitmap::FromHandle(m_Bitmap);

        CBitmap *theOldBmp = (CBitmap *)theMemDC.SelectObject(theBitmap);

        GetObject(m_Bitmap, sizeof(BITMAP), &theBitmapStruct);
        theWidth = theBitmapStruct.bmWidth;
        theHeight = theBitmapStruct.bmHeight;

        theDC->BitBlt(0, 0, theWidth, theHeight, &theMemDC, 0, 0, SRCCOPY);

        COLORREF theColor = GetColor();
        if (theColor == m_ColorHighlight || theColor == m_ColorVisited) {
            // Draw a highlight frame
            GetClientRect(&theRect);

            theBrush.CreateSolidBrush(theColor);
            theDC->FrameRect(&theRect, &theBrush);
            theBrush.DeleteObject();
        }

        theMemDC.SelectObject(theOldBmp);

        theMemDC.DeleteDC();
        ReleaseDC(theDC);
    }
}

//==============================================================================
/**
 *	JumpLink: Jumps to the link URL.
 *
 *	@param	None
 */
//==============================================================================
void CWebLink::JumpLink()
{
    CString theWebLink;

    GetWindowText(theWebLink);

    if (!m_URL.IsEmpty())
        theWebLink = m_URL;

    if (!theWebLink.IsEmpty()) {
        this->OpenURL(theWebLink);
    }
}

//==============================================================================
/**
 *	OpenURL: Opens the specified URL with the default browser.
 *
 *	@param	inURL	URL to open with the browser.
 */
//==============================================================================
void CWebLink::OpenURL(CString inURL)
{
    HCURSOR thePrevCursor;

    thePrevCursor = SetCursor(LoadCursor(nullptr, IDC_APPSTARTING));
    ShellExecute(GetSafeHwnd(), L"open", inURL, nullptr, nullptr, SW_SHOWNORMAL);
    SetCursor(thePrevCursor);
}

/////////////////////////////////////////////////////////////////////////////
// CWebLink message handlers

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
BOOL CWebLink::OnSetCursor(CWnd *inWnd, UINT inHitTest, UINT inMessage)
{
    UNREFERENCED_PARAMETER(inWnd);
    UNREFERENCED_PARAMETER(inHitTest);
    UNREFERENCED_PARAMETER(inMessage);

    if (m_bPopup) {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    } else {
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    }

    return TRUE;
}

//==============================================================================
/**
 *	OnLButtonDown
 *
 *	Windows callback handler for the WM_LBUTTONDOWN message
 *
 *	@param	inFlags		Flags for the left button down message
 *	@param	inPoint		Point where the mouse was left clicked (client coordinates)
 */
//==============================================================================
void CWebLink::OnLButtonDown(UINT inFlags, CPoint inPoint)
{
    UNREFERENCED_PARAMETER(inFlags);
    UNREFERENCED_PARAMETER(inPoint);

    if (IsWindowEnabled()) {
        m_bCapture = FALSE;
        ReleaseCapture();

        m_bVisited = TRUE;
        this->ShowLink();

        this->JumpLink();
    }
}

//==============================================================================
/**
 *	OnContextMenu: Handle for the WM_CONTEXTMENU message.
 *
 *	@param	inWnd		Window for the context menu.
 *	@param	inPoint		Mouse position when the right button was clicked.
 */
//==============================================================================
void CWebLink::OnContextMenu(CWnd *inWnd, CPoint inPoint)
{
    UNREFERENCED_PARAMETER(inWnd);

    BCMenu theContextMenu;
    BCMenu *thePopupMenu = nullptr;
    long theMenuCmd;

    ReleaseCapture();

    // Load the context menu
    theContextMenu.LoadMenu(IDR_WEBLINK_CONTEXT_MENU);
#ifdef _USECONTENTMENUIMAGES_
    theContextMenu.SetBitmapBackground(MENU_IMAGELIST_BACKCOLOR);
    theContextMenu.LoadToolbar(IDR_WEBLINKCONTEXTTOOLBAR);
#endif

    thePopupMenu = (BCMenu *)theContextMenu.GetSubMenu(0);

    // Set the default menu item to "Open"
    SetMenuDefaultItem(thePopupMenu->m_hMenu, IDM_OPENLINK, FALSE);

    m_bPopup = TRUE;

    // Show the context menu
    theMenuCmd = (long)thePopupMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
                                                    inPoint.x, inPoint.y, this);

    m_bPopup = FALSE;

    m_bCapture = FALSE;

    theContextMenu.DestroyMenu();

    this->ShowLink();

    // Handle the popup menu command

    switch (theMenuCmd) {
    case IDM_OPENLINK:
        this->JumpLink();
        break;

    case IDM_COPYSHORTCUT2:
        this->CopyShortcut();
        break;

    default:
        break;
    }

    theContextMenu.DestroyMenu();
}

//==============================================================================
/**
 *	CopyShortcut: Copy the link URL text to the clipboard.
 *
 *	@param	None
 */
//==============================================================================
void CWebLink::CopyShortcut()
{
    // Open the clipboard so we can copy data to it.
    if (OpenClipboard()) {
        DWORD theBufferSize;
        HGLOBAL theGlobalBuffer;
        LPSTR theClipboardText;

        // Determine the buffer size, based on the length of the URL
        theBufferSize = m_URL.GetLength();

        // Clear the contents of the clipboard
        ::EmptyClipboard();

        // Allocate a global buffer and copy the text to it
        theGlobalBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, theBufferSize + 1);
        theClipboardText = (LPSTR)GlobalLock(theGlobalBuffer);

        CopyMemory(theClipboardText, m_URL, theBufferSize + 1);

        // Unlock the buffer
        GlobalUnlock(theGlobalBuffer);

        // Pass the data to the clipboard
        SetClipboardData(CF_TEXT, theGlobalBuffer);

        // Close the clipboard, but do not GlobalFree() the global buffer
        CloseClipboard();
    }
}
