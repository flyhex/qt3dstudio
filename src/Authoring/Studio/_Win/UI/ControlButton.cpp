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

#include "resource.h"
#include "ControlButton.h"
#include "MemoryDC.h"
#include "WinUtils.h"

//==============================================================================
/*
 *		Constructor: Initializes the object.
 */
CControlButton::CControlButton()
    : m_ImageUp(nullptr)
    , m_ImageDown(nullptr)
    , m_ImageDisabled(nullptr)
    , m_ImageActive(nullptr)
    , m_ImageMouseTrack(nullptr)
    , m_ImageLimbo(nullptr)
    , m_ActiveFlag(FALSE)
    , m_HotTrackFlag(FALSE)
    , m_MouseOverFlag(FALSE)
    , m_ActiveDepressedFlag(FALSE)
    , m_LimboEnabled(FALSE)
    , m_LimboFlag(FALSE)
    , m_SimpleFrame(TRUE)
    , m_DrawFrame(TRUE)
    , m_DepressFlag(FALSE)
    , m_3DColorsFlag(FALSE)
{
    m_ImageOffset = CPoint(0, 0);
    m_FillColor = GetSysColor(COLOR_WINDOW);
    m_TransparentColor = RGB(255, 0, 255);
    m_FillColor = GetSysColor(COLOR_3DFACE);
}

//==============================================================================
/**
 *		Destructor: Releases the object.
 */
CControlButton::~CControlButton()
{
}

BEGIN_MESSAGE_MAP(CControlButton, CButton)
//{{AFX_MSG_MAP(CControlButton)
ON_WM_MOUSEMOVE()
ON_WM_ERASEBKGND()
//}}AFX_MSG_MAP
ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

//==============================================================================
/**
 *	Method to create the button
 *
 *	@param	inStyle		Window style for the button
 *	@param	inRect		Bounding rectangle for the button
 *	@param	inParentWnd	Parent window for the button
 *	@param	inID		Child window ID for the button
 *	@param	inDepressFlag
 *	@param	inLimboEnabled Set to TRUE if this is a tri-state button (defaults to false).
 *The third state is referred to as "limbo".
 *			Then use SetLimbo() to enter and leave the limbo state.
 *
 *	@return	Returns TRUE if the button was created
 */
BOOL CControlButton::Create(DWORD inStyle, const RECT &inRect, CWnd *inParentWnd, short inID,
                            BOOL inDepressFlag, BOOL inLimboEnabled)
{
    m_DepressFlag = inDepressFlag;
    m_LimboEnabled = inLimboEnabled;

    return CButton::Create(L"", inStyle /*| WS_BORDER*/ | BS_OWNERDRAW, inRect, inParentWnd, inID);
}

//==============================================================================
/**
 *	Adds images to the button for the different button states.
 *	@param	inTransparentColor	RGB value of the transparency color used within the
 *bitmaps.  If there is no transparency color,
 *								set this to a value that does not appear in any of
 *the images.
 *	@param	inImageUp			Image for the up state
 *	@param	inImageDown			Image for the down state
 *	@param	inImageDisabled		Image for the disabled state
 *	@param	inImageActive		Image for the active state
 *	@param	inImageMouseTrack	Image for hot mouse tracking
 *	@param	inImageLimbo		Image for "limbo" state (used for tri-state buttons)
 */
void CControlButton::AddImages(COLORREF inTransparentColor, short inImageUp, short inImageDown,
                               short inImageDisabled, short inImageActive, short inImageMouseTrack,
                               short inImageLimbo)
{
    if (inImageUp != 0)
        m_ImageUp = (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inImageUp),
                                       IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    // You always need to have at least one valid image for a button
    ASSERT(m_ImageUp != nullptr);

    if (inImageDown != 0)
        m_ImageDown = (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inImageDown),
                                         IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    if (inImageDisabled != 0)
        m_ImageDisabled =
            (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inImageDisabled),
                               IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    if (inImageActive != 0)
        m_ImageActive = (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inImageActive),
                                           IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    if (inImageMouseTrack != 0)
        m_ImageMouseTrack =
            (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inImageMouseTrack),
                               IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    if (inImageLimbo != 0)
        m_ImageLimbo = (HBITMAP)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(inImageLimbo),
                                          IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    m_TransparentColor = inTransparentColor;

    if (IsWindow(m_hWnd))
        this->Redraw();
}

//==============================================================================
/**
 *	SetImageOffset
 *
 *	Sets an image drawing offset
 *
 *	@param	inOffset	Offset for image from (0,0)
 */
void CControlButton::SetImageOffset(CPoint inOffset)
{
    m_ImageOffset = inOffset;
}

//==============================================================================
/**
 *	SetActiveDepressed
 *
 *	Sets the active depressed flag.  If TRUE and this control is set to active,
 *	then the image will be displayed in a depressed state (for toggle items).
 *
 *	@param	inActiveDepressedFlag	TRUE if the active state should be depressed
 */
void CControlButton::SetActiveDepressed(BOOL inActiveDepressedFlag)
{
    m_ActiveDepressedFlag = inActiveDepressedFlag;
}

//==============================================================================
/**
 *	SetActive
 *
 *	Sets the active state for the button
 *
 *	@param	inActiveState	Active state for the button
 */
void CControlButton::SetActive(BOOL inActiveFlag)
{
    m_ActiveFlag = inActiveFlag;

    if (IsWindow(m_hWnd))
        this->Redraw();
}

//==============================================================================
/**
 *	GetActive
 *
 *	Gets the active state for the button
 *
 *	@param	None
 *
 *	@return	Active state for the button
 */
BOOL CControlButton::GetActive()
{
    return m_ActiveFlag;
}

//==============================================================================
/**
 *	DrawItem
 *
 *	Handle the owner drawn button
 *
 *	@param	inDrawItemStruct	DRAWITEMSTRUCT structure
 */
void CControlButton::DrawItem(LPDRAWITEMSTRUCT inDrawItemStruct)
{
    CDC *theDC;
    CMemoryDC theMemDC;
    BOOL theEnabledFlag;
    BOOL theDownStateFlag;
    HBITMAP theBitmap = m_ImageUp;
    CRect theRect;

    theDC = GetDC();

    // Get the bounding client rectangle
    GetClientRect(theRect);

    theMemDC.Create(theDC, theRect);

    theDownStateFlag = (inDrawItemStruct->itemState & ODS_SELECTED);

    // Is this window enabled?
    theEnabledFlag = IsWindowEnabled();

    if (m_ActiveFlag)
        theBitmap = m_ImageActive;

    if (m_MouseOverFlag)
        if (m_ImageMouseTrack != 0)
            theBitmap = m_ImageMouseTrack;

    if (!theEnabledFlag) {
        // Disabled
        if (m_ImageDisabled != 0)
            theBitmap = m_ImageDisabled;
    } else {
        // Window is enabled
        if (theDownStateFlag)
            if (m_ImageDown != 0)
                theBitmap = m_ImageDown;
    }

    // If the button has limbo enabled, and it in limbo state, it overrides any previous button
    // settings
    if (m_LimboEnabled && m_LimboFlag)
        if (m_ImageLimbo != 0)
            theBitmap = m_ImageLimbo;

    // Fill the background color
    theMemDC.FillSolidRect(theRect, m_FillColor);

    // Draw the bitmap if it exists
    if (theBitmap)
        CWinUtils::DrawTransparentBitmap(
            &theMemDC, m_ImageOffset.x + theDownStateFlag * m_DepressFlag,
            m_ImageOffset.y + theDownStateFlag * m_DepressFlag, theBitmap, m_TransparentColor);

    // Hot tracking the mouse over the button?
    if (m_HotTrackFlag) {
        if (m_DepressFlag) {
            if (theDownStateFlag)
                DrawButtonFrame((CDC *)&theMemDC, &theRect, TRUE);
            else if (m_MouseOverFlag)
                DrawButtonFrame((CDC *)&theMemDC, &theRect, FALSE);
        } else {
            if (m_MouseOverFlag)
                DrawButtonFrame((CDC *)&theMemDC, &theRect, FALSE);
        }
    } else {
        // Draw the frame around the button
        if (!m_ActiveFlag)
            DrawButtonFrame((CDC *)&theMemDC, &theRect, theDownStateFlag * m_DepressFlag);
    }

    theMemDC.Release();

    ReleaseDC(theDC);
}

//==============================================================================
/**
 *	DrawButtonFrame
 *
 *	Draw the button frame
 *
 *	@param	inDC		Device context for drawing
 *	@param	inRect		Bounding rectangle for the button frame
 *	@param	inDownFlag	If TRUE, the button is down
 */
void CControlButton::DrawButtonFrame(CDC *inDC, CRect *inRect, BOOL inDownFlag)
{
    if (m_DrawFrame) {
        COLORREF theShadowColor = GetSysColor(COLOR_3DSHADOW);
        COLORREF theHiliteColor = GetSysColor(COLOR_3DHIGHLIGHT);

        if (inDownFlag) {
            theShadowColor = GetSysColor(COLOR_3DHIGHLIGHT);
            theHiliteColor = GetSysColor(COLOR_3DSHADOW);
        }

        CRect theFrameRect;

        theFrameRect.CopyRect(inRect);

        if (m_SimpleFrame) {
            inDC->Draw3dRect(&theFrameRect, theHiliteColor, theShadowColor);
        } else {
            if (inDownFlag) {
                theFrameRect.right--;
                theFrameRect.bottom--;

                inDC->Draw3dRect(&theFrameRect, theHiliteColor, theShadowColor);
            } else {
                // the button is up
                theFrameRect.right--;
                theFrameRect.bottom--;

                inDC->Draw3dRect(&theFrameRect, theHiliteColor, theShadowColor);

                inDC->MoveTo(theFrameRect.right, theFrameRect.top);
                inDC->LineTo(theFrameRect.right, theFrameRect.bottom);
                inDC->LineTo(theFrameRect.left, theFrameRect.bottom);
            }
        }
    }
}

//==============================================================================
/**
 *	SetHotTrack
 *
 *	Set the hot track (mouse over) flag
 *
 *	@param	inHotTrackFlag	Hot tracking flag
 */
void CControlButton::SetHotTrack(BOOL inHotTrackFlag)
{
    m_HotTrackFlag = inHotTrackFlag;

    if (IsWindow(m_hWnd))
        this->Redraw();
}

//==============================================================================
/**
 *	OnMouseMove
 *
 *	Handle the WM_MOUSEMOVE message
 *
 *	@param	inFlags		Flags for WM_MOUSEMOVE
 *	@param	inPoint		Cursor position in client coordinates
 */
void CControlButton::OnMouseMove(UINT inFlags, CPoint inPoint)
{
    CButton::OnMouseMove(inFlags, inPoint);

    if (!m_MouseOverFlag) {
        this->TrackMouse();
        m_MouseOverFlag = TRUE;

        this->Redraw();
    }
}

//==============================================================================
/**
 *	OnMouseMove
 *
 *	Handle the WM_MOUSELEAVE message
 *
 *	@param	inwParam	WPARAM for the WM_MOUSELEAVE message
 *	@param	inlParam	LPARAM for the WM_MOUSELEAVE message
 *
 *	@param	Return code - returns TRUE if this message is processed.
 */
LRESULT CControlButton::OnMouseLeave(WPARAM inwParam, LPARAM inlParam)
{
    Q_UNUSED(inwParam);
    Q_UNUSED(inlParam);

    m_MouseOverFlag = FALSE;

    this->TrackMouse(FALSE);

    this->Redraw();

    return TRUE;
}

//==============================================================================
/**
 *	TrackMouse
 *
 *	Register for the WM_MOUSELEAVE message to determine when
 *	the mouse leaves this button.
 *
 *	@param	inTrackEnableFlag - TRUE/FALSE depending on whether we want to track the mouse
 *event state.
 */
void CControlButton::TrackMouse(BOOL inTrackEnableFlag)
{
    TRACKMOUSEEVENT theTrackMouseEvent;

    memset(&theTrackMouseEvent, 0, sizeof(theTrackMouseEvent));

    theTrackMouseEvent.cbSize = sizeof(theTrackMouseEvent);
    theTrackMouseEvent.dwFlags = TME_LEAVE;
    if (!inTrackEnableFlag)
        theTrackMouseEvent.dwFlags |= TME_CANCEL;
    theTrackMouseEvent.hwndTrack = m_hWnd;

    _TrackMouseEvent(&theTrackMouseEvent);
}

//==============================================================================
/**
 *	Redraw
 *
 *	Invalidate and update the button
 *
 *	@param	None
 */
void CControlButton::Redraw()
{
    // Invalidate and update the window
    InvalidateRect(nullptr, TRUE);
    UpdateWindow();
}

//==============================================================================
/**
 *	Handles the WM_ERASEBKGND message.  This function is overridden so that we
 *	can prevent the background from being erased.  When we draw the button, the
 *	background is filled in with the appropriate color.  Actually erasing the
 *	background before a draw was producing flickering.
 *
 *	@param	pDC Not used.
 */
BOOL CControlButton::OnEraseBkgnd(CDC *pDC)
{
    Q_UNUSED(pDC);
    return 1;
}
