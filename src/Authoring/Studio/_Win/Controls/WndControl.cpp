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

#include "stdafx.h"

#include "WndControl.h"
#include "WinRenderer.h"
#include "Control.h"
#include "MasterP.h"
#include "AppFonts.h"
#include "ContextMenu.h"
#include "HotKeys.h"
#include "StudioPreferences.h"
#include "WinDnd.h"
#include "Doc.h"
#include "DropSource.h"
#include "IDragable.h"
#include "OffscreenRenderer.h"

BEGIN_MESSAGE_MAP(CWndControl, CWnd)
//{{AFX_MSG_MAP(CWndControl)
ON_WM_KILLFOCUS()
ON_WM_SETFOCUS()
ON_WM_PAINT()
ON_WM_LBUTTONUP()
ON_WM_LBUTTONDOWN()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_WM_LBUTTONDBLCLK()
ON_WM_SIZING()
ON_WM_ERASEBKGND()
ON_WM_SIZE()
ON_WM_MOUSEMOVE()
ON_WM_CREATE()
ON_WM_MOUSEACTIVATE()
ON_WM_MOUSEWHEEL()
ON_WM_CTLCOLOR()
ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
//}}AFX_MSG_MAP
ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
END_MESSAGE_MAP()

//=============================================================================
/**
 * Creates the pass thru class for the wnd control.
 */
CWndControlControlListener::CWndControlControlListener(CWndControl *inParent)
{
    m_Parent = inParent;
}

//=============================================================================
/**
 * Notification from the control that the window was invalidated.
 */
void CWndControlControlListener::OnControlInvalidated()
{
    m_Parent->OnControlInvalidated();
}

//=============================================================================
/**
 * Notification from the control to do a popup at the specified location.
 */
long CWndControlControlListener::DoPopup(CContextMenu *inContextMenu, CPt inPoint)
{
    return m_Parent->DoPopup(inContextMenu, inPoint);
}

//=============================================================================
/**
 * Get the location of the point in Screen coordinates.
 */
CPt CWndControlControlListener::ClientToScreen(CPt inPoint)
{
    CPoint thePoint(inPoint.x, inPoint.y);
    m_Parent->ClientToScreen(&thePoint);

    return CPt(thePoint.x, thePoint.y);
}

//=============================================================================
/**
 * Get the location of the point into client coordinates.
 */
CPt CWndControlControlListener::ScreenToClient(CPt inPoint)
{
    CPoint thePoint(inPoint.x, inPoint.y);
    m_Parent->ScreenToClient(&thePoint);

    return CPt(thePoint.x, thePoint.y);
}

//=============================================================================
/**
 * Get the platform dependent view that this is embedding.
 * Used when platform dependent controls need to be embedded into the Controls.
 */
TPlatformView CWndControlControlListener::GetPlatformView()
{
    return m_Parent->m_hWnd;
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's SetIsDragging function.
 * @param inIsDragging true to specify that a drag is occurring or false to cancel a drag
 */
void CWndControlControlListener::SetIsDragging(bool inIsDragging)
{
    m_Parent->SetIsDragging(inIsDragging);
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's ShowTooltips function.
 * @param inLocation mid-point of the tooltip in global coordinates
 * @param inText text to display as a tooltip
 */
void CWndControlControlListener::ShowTooltips(CPt inLocation, Q3DStudio::CString inText)
{
    m_Parent->ShowTooltips(inLocation, inText);
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's HideTooltips function.
 */
void CWndControlControlListener::HideTooltips()
{
    m_Parent->HideTooltips();
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's ShowMoveableTooltips function.
 * @param inLocation mid-point of the tooltip in global coordinates
 * @param inText text to display as a tooltip
 */
void CWndControlControlListener::ShowMoveableWindow(CPt inLocation, Q3DStudio::CString inText,
                                                    CRct inBoundingRct)
{
    m_Parent->ShowMoveableWindow(inLocation, inText, inBoundingRct);
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's HideMoveableTooltips function.
 */
void CWndControlControlListener::HideMoveableWindow()
{
    m_Parent->HideMoveableWindow();
}

void CWndControlControlListener::DoStartDrag(IDragable *inDragable)
{
    m_Parent->DoStartDrag(inDragable);
}

//===============================================================================
/**
* performs a drag operation on a file
*/
void CWndControlControlListener::DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileNameList)
{
    m_Parent->DoStartDrag(inDragFileNameList);
}

IMPLEMENT_OBJECT_COUNTER(CWndControl);

//==============================================================================
/**
 *	Constructor
 */
CWndControl::CWndControl(CControl *inControl)
    : m_MouseOver(false)
    , m_MemDC(nullptr)
    , m_IsDragging(false)
    , m_ControlListener(this)
    , m_IsMouseDown(false)
{
    ADDTO_OBJECT_COUNTER(CWndControl);

    m_Control = inControl;

    if (m_Control)
        m_Control->SetWindowListener(&m_ControlListener);

    RegisterWindowClass();

    // Create the tooltip for this window (there's only one per view)
    m_Tooltip.CreateEx(WS_EX_TOOLWINDOW, AfxRegisterWndClass(CS_SAVEBITS), L"CPopupWndTooltip",
                       WS_POPUP | WS_BORDER, CRect(0, 0, 20, 10), this, 0);
    m_Tooltip.SetStationary(true);

    // Create the tooltip for this window (there's only one per view)
    m_MoveableTooltip.CreateEx(WS_EX_TOOLWINDOW, AfxRegisterWndClass(CS_SAVEBITS),
                               L"CPopupWndTooltip", WS_POPUP | WS_BORDER, CRect(0, 0, 20, 10), this,
                               0);
    m_MoveableTooltip.SetStationary(false);

    m_Brush.CreateSolidBrush(CStudioPreferences::GetBaseColor());

#ifdef _DEBUG
    m_IsPainting = false;
#endif
}

//==============================================================================
/**
 *	Destructor
 */
CWndControl::~CWndControl()
{
    DeleteBuffers();

    REMOVEFROM_OBJECT_COUNTER(CWndControl);
}

// Register the window class if it has not already been registered.
BOOL CWndControl::RegisterWindowClass()
{
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!(::GetClassInfo(hInst, WNDCONTROL_CLASSNAME, &wndcls))) {
        // otherwise we need to register a new class
        wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc = ::DefWindowProc;
        wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
        wndcls.hInstance = hInst;
        wndcls.hIcon = nullptr;
        wndcls.hCursor = nullptr;
        wndcls.hbrBackground = m_Brush;
        wndcls.lpszMenuName = nullptr;
        wndcls.lpszClassName = WNDCONTROL_CLASSNAME;

        if (!AfxRegisterClass(&wndcls)) {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}

//==============================================================================
/**
 *	Create: Handles the WM_CREATE message.
 *
 *	Creates the view, sets the font, and loads the toolbar.
 *
 *	@param	lpszClassName		Names the Windows class. The class name can be any name
 *registered with the AfxRegisterWndClass
 *	@param	lpszWindowName		Represents the window name. Used as text for the title bar.
 *	@param	dwStyle				Specifies the window style attributes.
 *	@param	rect				Specifies the size and position of the window.
 *	@param	pParentWnd			Pointer to the parent of the view.
 *	@param	nID					The ID for the view.
 *	@param	pContext			Create context for the view.
 *
 *	@return TRUE if creation was successful, otherwise FALSE.
 */
//==============================================================================
int CWndControl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    return CWnd::OnCreate(lpCreateStruct);
}

//=============================================================================
/**
 * MFC notification that this window needs to be redrawn.
 * This buffers the draw DC, makes a Renderer and has the Control draw to the
 * buffered DC.
 */
void CWndControl::OnPaint()
{
#ifdef _DEBUG
    m_IsPainting = true;
#endif

    UICPROFILE(OnPaint);

    CPaintDC dc(this);

    HDC theDeviceContext = dc.GetSafeHdc();

    RECT theSize;
    this->GetClientRect(&theSize);

    bool theRedrawAllFlag = false;

    if (m_MemDC == nullptr) {
        m_MemDC = new CDC();
        m_MemDC->CreateCompatibleDC(&dc);
        m_MemDC->SelectObject(CAppFonts::GetInstance()->GetNormalFont());

        m_MemBitmap = ::CreateCompatibleBitmap(theDeviceContext, theSize.right, theSize.bottom);

        m_OldBitmap = (HBITMAP)m_MemDC->SelectObject(m_MemBitmap);

        theRedrawAllFlag = true;
    }

    CRct theDirtyRect;
    {
        UICPROFILE(OnPaint_Render);
        CWinRenderer theRenderer(m_MemDC, CRct(0, 0, theSize.right, theSize.bottom));
        if (m_Control)
            m_Control->OnDraw(&theRenderer, theDirtyRect, theRedrawAllFlag);
    }

    // Copy (BitBlt) bitmap from bitmap memory DC to memory DC
    //	::BitBlt( theDeviceContext, theDirtyRect.position.x, theDirtyRect.position.y,
    //theDirtyRect.size.x, theDirtyRect.size.y, m_MemDC->GetSafeHdc( ), theDirtyRect.position.x,
    //theDirtyRect.position.y, SRCCOPY );
    ::BitBlt(theDeviceContext, 0, 0, theSize.right, theSize.bottom, m_MemDC->GetSafeHdc(), 0, 0,
             SRCCOPY);

#ifdef _DEBUG
    m_IsPainting = false;
#endif
}

void CWndControl::DeleteBuffers()
{
    if (m_MemDC != nullptr) {
        m_MemDC->SelectObject(m_OldBitmap);
        ::DeleteObject(m_MemBitmap);
        m_MemDC->DeleteDC();
        delete m_MemDC;
        m_MemDC = nullptr;
    }
}

//=============================================================================
/**
 * Notification to this window that it has lost focus.
 * @param pNewWnd the window that has gained focus.
 */
void CWndControl::OnKillFocus(CWnd *pNewWnd)
{
    CWnd::OnKillFocus(pNewWnd);

    // This is used for special cases where we lose control of the mouse and lose focus as well.
    // It makes sure we are not still receiving mouse messages.
    if (m_IsMouseDown) {
        // Tell the control that it got a mouse up
        if (m_Control)
            m_Control->OnMouseUp(CPt(-1, -1), CHotKeys::GetCurrentKeyModifiers());
        ReleaseCapture();
    }

    if (m_Control) {
        // For the case where pNewWnd is an "embedded" control in m_Control (eg.
        // CPlatformEditControl)
        // m_Control should not lose focus.
        // Note that since most of the Studio controls uses a common PlatformDevice (the Studio
        // window),
        // this (OnKillFocus) only occurs when 1) switching to another app, 2) popping up a dialog
        // box or 3) an embedded control
        // takes focus.
        // This takes care of the case when an embedded control takes focus but unwittingly causes
        // its parent to lose focus.
        // This can have dire consequences if the parent control were to do something on losing
        // focus, such as
        // destroying all its children. For example, Bug3421.
        if (!m_Control->IsChildPlatformDevice(pNewWnd->GetSafeHwnd()))
            m_Control->OnLoseFocus();
    }
}

void CWndControl::OnSetFocus(CWnd *pPrevWnd)
{
    CWnd::OnSetFocus(pPrevWnd);
    if (m_Control)
        m_Control->OnGainFocus();
}

//=============================================================================
/**
 * Notification of mouse up.
 */
void CWndControl::OnLButtonUp(UINT nFlags, CPoint inPoint)
{
    Q_UNUSED(nFlags);
    m_IsMouseDown = false;
    // Tell the control that it got a mouse up
    if (m_Control)
        m_Control->OnMouseUp(CPt(inPoint.x, inPoint.y), CHotKeys::GetCurrentKeyModifiers());

    ReleaseCapture();

    CWnd::OnLButtonUp(nFlags, inPoint);
}

//=============================================================================
/**
 * Notification of a mouse click.
 */
void CWndControl::OnLButtonDown(UINT nFlags, CPoint inPoint)
{
    Q_UNUSED(nFlags);
    m_IsMouseDown = true;
    SetCapture();
    // Tell the control that it got a mouse down.
    if (m_Control)
        m_Control->OnMouseDown(CPt(inPoint.x, inPoint.y), CHotKeys::GetCurrentKeyModifiers());

    CWnd::OnLButtonDown(nFlags, inPoint);
}

//=============================================================================
/**
 * Notification of a mouse click.
 */
void CWndControl::OnRButtonDown(UINT nFlags, CPoint inPoint)
{
    SetCapture();

    if (m_IsMouseDown) {
        // Tell the control that it got a mouse up
        if (m_Control)
            m_Control->OnMouseUp(CPt(inPoint.x, inPoint.y), CHotKeys::GetCurrentKeyModifiers());
        m_IsMouseDown = false;
    }

    // Tell the control that it got a mouse down.
    if (m_Control)
        m_Control->OnMouseRDown(CPt(inPoint.x, inPoint.y), CHotKeys::GetCurrentKeyModifiers());

    CWnd::OnRButtonDown(nFlags, inPoint);
}

//=============================================================================
/**
 * Notification of mouse up.
 */
void CWndControl::OnRButtonUp(UINT nFlags, CPoint inPoint)
{
    Q_UNUSED(nFlags);

    // Tell the control that it got a mouse up
    if (m_Control)
        m_Control->OnMouseRUp(CPt(inPoint.x, inPoint.y), CHotKeys::GetCurrentKeyModifiers());

    ReleaseCapture();
}

long CWndControl::DoPopup(CContextMenu *inMenu, CPt inPoint)
{
    CPoint thePoint(inPoint.x, inPoint.y);
    ClientToScreen(&thePoint);

    long theSelectedItem = inMenu->DoPopup(CPt(thePoint.x, thePoint.y), m_hWnd);
    if (m_IsMouseDown) {
        m_IsMouseDown = false;
        if (m_Control)
            m_Control->OnMouseUp(CPt(-1, -1), 0);
    }
    ReleaseCapture();

    return theSelectedItem;
}

//=============================================================================
/**
 * Notification of a mouse double click.
 */
void CWndControl::OnLButtonDblClk(UINT nFlags, CPoint inPoint)
{
    Q_UNUSED(nFlags);

    // Tell the control that it got a mouse down.
    if (m_Control)
        m_Control->OnMouseDoubleClick(CPt(inPoint.x, inPoint.y), nFlags);
}

//=============================================================================
/**
 * Notification of a key down message.
 */
bool CWndControl::OnKeyDown(UINT inChar, UINT, UINT)
{
    return m_Control->OnKeyDown(inChar, CHotKeys::GetCurrentKeyModifiers());
}

//=============================================================================
/**
 * Notification of a key up messages.  May be hooked up later.
 */
bool CWndControl::OnKeyUp(UINT inChar, UINT, UINT)
{
    return m_Control->OnKeyUp(inChar, CHotKeys::GetCurrentKeyModifiers());
}

//=============================================================================
/**
 * Notification of a character press message.
 *
 * @return true if the character was processed by the control, otherwise false
 */
bool CWndControl::OnChar(UINT inChar, UINT, UINT)
{
    return m_Control->OnChar(inChar, CHotKeys::GetCurrentKeyModifiers());
}

//=============================================================================
/**
 * Displays a tooltip at the desired location, and with the desired text.
 * @param inLocation mid-point of the tooltext message box to be displayed
 * @param inText text to be displayed as the tooltip
 */
void CWndControl::ShowTooltips(CPt inLocation, Q3DStudio::CString inText)
{
    // If the global preference for tooltips is enabled
    if (CStudioPreferences::ShouldShowTooltips()) {
        // If the tooltip text is not empty, show the tooltip
        if (!inText.IsEmpty()) {
            // So that tooltip text renders within the window
            // this would be nicely taken care of if this were a MFC control...
            COffscreenRenderer theOffscreenRenderer(CRct(0, 0, 1, 1));
            float theWidth(0);
            float theHeight(0);
            theOffscreenRenderer.GetTextSize(inText, theWidth, theHeight);

            long theRightMargin = m_Control->GetPosition().x + m_Control->GetSize().x;
            if (inLocation.x + ::dtol(theWidth) > theRightMargin) // re-centers this
                inLocation.x = theRightMargin - ::dtol(theWidth) / 2;

            // Convert the point to screen coordinates
            ::CPoint theScreenCoords = ::CPoint(inLocation.x, inLocation.y);
            ClientToScreen(&theScreenCoords);
            m_Tooltip.UpdateToolTip(theScreenCoords, ::CString(inText.GetMulti()), true);
        }
        // If the tooltip text is empty, hide the tooltip
        else
            m_Tooltip.ShowWindow(SW_HIDE);
    }
}

//=============================================================================
/**
 * Hides any tooltip currently being displayed.
 */
void CWndControl::HideTooltips()
{
    m_Tooltip.ShowWindow(SW_HIDE);
}

//=============================================================================
/**
 *	Starts the Dragging process.
 *	On the given IDragable.
 *	@param inAsset the Asset to be drug.
 */
void CWndControl::DoStartDrag(IDragable *inDragable)
{
    // Tell the window that we are beginning a drag
    if (inDragable && !m_IsDragging) {
        // This should prevent the Container from dragging while it is already dragging.
        SetIsDragging(true);

        // This is a Wrapper for the OLEDROP
        CWinDragManager theDragManager;

        // Theoretically: there is no limit to the amount of data you can add.
        // For now only one object will work, until i figure out how to drag multiple objects in
        // windows.
        Q3DStudio::CAutoMemPtr<CDropSource> theDropSource =
            CDropSourceFactory::Create(inDragable->GetFlavor(), inDragable);

        // Add the UIC_GESTURE_FLAVOR.  This will allow us to drag to StudioControls.
        theDragManager.AddData(inDragable->GetFlavor(), &theDropSource, sizeof(CDropSource *));
        // This is a blocking call when we get done with this call we should have no more data in
        // our Drag list.
        theDragManager.StartDrag();

        // Tell the window that we are done dragging
        SetIsDragging(false);
    }
}

//===============================================================================
/**
* performs a drag operation on a file
*/
void CWndControl::DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileNameList)
{
    // Tell the window that we are beginning a drag
    if (!m_IsDragging) {
        SetIsDragging(true);

        try {
            CWinDragManager theDragManager;

            std::vector<Q3DStudio::CString>::iterator thePos = inDragFileNameList.begin();
            std::vector<Q3DStudio::CString>::iterator theEndPos = inDragFileNameList.end();

            Q3DStudio::CAutoMemPtr<CDropSource> theDropSource;
            Q3DStudio::CAutoMemPtr<CUICFile> theDragFile;
            for (; thePos != theEndPos; ++thePos) {
                Q3DStudio::CString theDragFileName = *thePos;
                if (theDragFileName.Length() > 0) {
                    theDragFile = new CUICFile(theDragFileName);
                    theDropSource = CDropSourceFactory::Create(
                        EUIC_FLAVOR_ASSET_UICFILE, (void *)theDragFile, sizeof(theDragFile));
                    // Add the UIC_GESTURE_FLAVOR.  This will allow us to drag to StudioControls.
                    theDragManager.AddData(EUIC_FLAVOR_ASSET_UICFILE, &theDropSource,
                                           sizeof(CDropSource *));
                    break;
                }
            }
            theDragManager.StartDrag();
        } catch (...) { // if there are any errors that throws an exception, there
            // there will be no more drag and drop, since the flag will not be reset.
        }
        // Tell the window that we are done dragging
        SetIsDragging(false);
    }
}

//=============================================================================
/**
 * Notification of a mouse wheel movement.
 */
BOOL CWndControl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    ScreenToClient(&pt);
    if (m_Control)
        m_Control->OnMouseWheel(CPt(pt.x, pt.y), zDelta, CHotKeys::GetCurrentKeyModifiers());

    //Q_UNUSED( nFlags );
    //Q_UNUSED( zDelta );
    //Q_UNUSED( pt );
    return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

LRESULT CWndControl::OnRegisteredMouseWheel(WPARAM, LPARAM)
{
    return 0;
}

//=============================================================================
/**
 * Notification that this window is being resized.
 */
void CWndControl::OnSizing(UINT nSide, LPRECT lpRect)
{
    Q_UNUSED(nSide);
    Q_UNUSED(lpRect);
}

//=============================================================================
/**
 * Override of erase to prevent flashing of the background.
 */
BOOL CWndControl::OnEraseBkgnd(CDC *pDC)
{
    Q_UNUSED(pDC);
    return FALSE;
}

//=============================================================================
/**
 * Notification that this window has been resized.
 */
void CWndControl::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    MoveWindow(0, 0, cx, cy);

    CRect theSize;
    GetClientRect(&theSize);

    if (m_Control)
        m_Control->SetSize(CPt(theSize.right, theSize.bottom));

    // Gotta recreate the buffered objects, otherwise they will be the wrong dimensions
    DeleteBuffers();
}

//=============================================================================
/**
 * Event notification that the mouse has moved.
 */
void CWndControl::OnMouseMove(UINT, CPoint point)
{
#ifdef _DEBUG
    m_IsPainting = true;
#endif

    UICPROFILE(OnMouseMove);

    // Track the mouse while it's in here, this will give us the hover msg
    m_MouseOver = true;

    TRACKMOUSEEVENT theEventStruct;
    theEventStruct.cbSize = sizeof(TRACKMOUSEEVENT);
    theEventStruct.dwFlags = TME_HOVER | TME_LEAVE;
    theEventStruct.hwndTrack = GetSafeHwnd();
    theEventStruct.dwHoverTime = HOVER_DEFAULT;

    ::TrackMouseEvent(&theEventStruct);

    // Notify the control that the mouse moved
    if (m_Control) {
        m_Control->OnMouseMove(CPt(point.x, point.y), CHotKeys::GetCurrentKeyModifiers());

        // If the control invalidated because of a mouse event then we want to do an immediate
        // redraw.
        // this ensures consistent visible feedback.
        if (m_Control->IsChildInvalidated()) {
            UICPROFILE(RedrawWindow);
            RedrawWindow(nullptr, nullptr, RDW_UPDATENOW);
        }
    }

#ifdef _DEBUG
    m_IsPainting = false;
#endif
}

//=============================================================================
/**
 * Notification that the mouse has left this window.
 */
LRESULT CWndControl::OnMouseLeave(WPARAM inwParam, LPARAM inlParam)
{
#ifdef _DEBUG
    if (!m_IsPainting) {
#endif _DEBUG

        Q_UNUSED(inwParam);
        Q_UNUSED(inlParam);

        CPoint thePoint;

        ::GetCursorPos(&thePoint);

        // This will tell me what window is under this point.
        HWND theWindow = ::WindowFromPoint(thePoint);

        if (theWindow != m_hWnd) {
            m_MouseOver = false;
            if (m_Control)
                m_Control->OnMouseMove(CPt(-1, -1), 0);
        }

#ifdef _DEBUG
    }
#endif

    return 0;
}

//=============================================================================
/**
 * Override to provide the hover message to the control.
 */
LRESULT CWndControl::OnMouseHover(WPARAM inwParam, LPARAM inlParam)
{

    long theX = inlParam & 0xFFFF;
    long theY = (inlParam >> 16) & 0xFFFF;

    if (m_Control)
        m_Control->OnMouseHover(CPt(theX, theY), (UINT)inwParam);

    TRACKMOUSEEVENT theEventStruct;
    theEventStruct.cbSize = sizeof(TRACKMOUSEEVENT);
    theEventStruct.dwFlags = TME_HOVER | TME_LEAVE;
    theEventStruct.hwndTrack = GetSafeHwnd();
    theEventStruct.dwHoverTime = HOVER_DEFAULT;

    ::TrackMouseEvent(&theEventStruct);

    return 0;
}

//=============================================================================
/**
 * Callback from the control that it has been invalidated.
 * This will post a message for a redraw.
 */
void CWndControl::OnControlInvalidated()
{
    if (::IsWindow(m_hWnd)) {
        Invalidate();
        ::PostMessage(m_hWnd, WM_PAINT, 0, 0);
    }
}

//=============================================================================
/**
 * Override of PreTranslateMessage for dragging implementation.
 */
BOOL CWndControl::PreTranslateMessage(MSG *inMessage)
{
    BOOL theReturn = FALSE;

    switch (inMessage->message) {
    case WM_KEYDOWN: {
        theReturn =
            OnKeyDown((UINT)inMessage->wParam, inMessage->lParam & 0xFFFF, (UINT)inMessage->lParam);

        BYTE theKeyboardState[256];
        ::GetKeyboardState(theKeyboardState);

        theKeyboardState[VK_CONTROL] = 0;

        WORD theChar = 0;
        // Get the ascii character for the VK key.
        ::ToAscii((UINT)inMessage->wParam, (inMessage->lParam >> 16) & 0xFF, theKeyboardState,
                  &theChar, 0);
        unsigned int theCharacterCode = (char)theChar;

        if (theCharacterCode != 0) {
            theReturn = theReturn
                || OnChar(theCharacterCode, inMessage->lParam & 0xFFFF, (UINT)inMessage->lParam);
        }
    } break;

    case WM_KEYUP:
        // Do the OnKeyUp
        theReturn =
            OnKeyUp((UINT)inMessage->wParam, inMessage->lParam & 0xFFFF, (UINT)inMessage->lParam);
        break;

    case WM_CHAR:
        theReturn = OnChar((UINT)inMessage->wParam, 0, 0);
        break;

    default:
        break;
    }

    return theReturn;
}

//=============================================================================
/**
 * Call this function to indicate that a drag operation is currently taking
 * place or has just finished.  If the drag operation just finished, a mouse up
 * event will be sent to all children so that the one who initiated the drag
 * can clean things up.
 * @param inIsDragging true to specify that a drag is occurring or false if a drag ended
 */
void CWndControl::SetIsDragging(bool inIsDragging)
{
    m_IsDragging = inIsDragging;

    // If we are no longer dragging, notify the children that a mouse up occurred since OLE won't
    // tell us that
    if (!m_IsDragging)
        m_Control->OnMouseUp(CPt(-1, -1), 0);
}

//=============================================================================
/**
 * Override of MouseActivate to set the focus on this.
 */
int CWndControl::OnMouseActivate(CWnd *pDesktopWnd, UINT nHitTest, UINT message)
{
    CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
    SetFocus();
    m_Control->OnGainFocus();
    return MA_ACTIVATE;
}

bool CWndControl::OnDragWithin(CDropSource &inSource)
{
    bool theReturn = false;
    CPt thePoint = inSource.GetCurrentPoint();
    long theFlags = inSource.GetCurrentFlags();
    CDropTarget *theDropTarget = m_Control->FindDropCandidate(thePoint, theFlags);

    if (theDropTarget) {
        theReturn = theDropTarget->Accept(inSource);
        delete theDropTarget;
    }
    return theReturn;
}

bool CWndControl::OnDragReceive(CDropSource &inSource)
{
    bool theReturn = false;
    CPt thePoint = inSource.GetCurrentPoint();
    long theFlags = inSource.GetCurrentFlags();

    CDropTarget *theDropTarget = m_Control->FindDropCandidate(thePoint, theFlags);

    if (theDropTarget) {
        theReturn = theDropTarget->Drop(inSource);
        delete theDropTarget;
    }
    return theReturn;
}

void CWndControl::OnDragLeave()
{
    m_MouseOver = false;
    m_Control->OnMouseMove(CPt(-1, -1), 0);
}

void CWndControl::OnReflectMouse(CPt &inPoint, long inFlags)
{
    // Notify the control that the mouse moved
    m_Control->OnMouseMove(inPoint, inFlags /*CHotKeys::GetCurrentKeyModifiers( )*/);

    // If the control invalidated because of a mouse event then we want to do an immediate redraw.
    // this ensures consistent visible feedback.
    if (m_Control->IsChildInvalidated()) {
        UICPROFILE(RedrawWindow);
        RedrawWindow(nullptr, nullptr, RDW_UPDATENOW);
    }
}

void CWndControl::ShowMoveableWindow(CPt inLocation, Q3DStudio::CString inText, CRct inBoundingRct)
{
    // If the global preference for tooltips is enabled
    if (CStudioPreferences::ShouldShowTooltips()) {
        // This flag is set when the control is created, it tells itself that things should
        // not move around outside its boundaries.
        if (inLocation.x < inBoundingRct.position.x)
            inLocation.x = inBoundingRct.position.x;
        if (inLocation.y < inBoundingRct.position.y)
            inLocation.y = inBoundingRct.position.y;
        if (inLocation.x > (inBoundingRct.position.x + inBoundingRct.size.x))
            inLocation.x = inBoundingRct.position.x + inBoundingRct.size.x;
        if (inLocation.y > (inBoundingRct.position.y + inBoundingRct.size.y))
            inLocation.y = inBoundingRct.position.y + inBoundingRct.size.y;

        // Convert the point to screen coordinates
        ::CPoint theScreenCoords = ::CPoint(inLocation.x, inLocation.y);
        ClientToScreen(&theScreenCoords);
        // If the tooltip text is not empty, show the tooltip
        if (!inText.IsEmpty())
            m_MoveableTooltip.UpdateToolTip(theScreenCoords, ::CString(inText.GetMulti()), true);
        // If the tooltip text is empty, hide the tooltip
        else
            m_MoveableTooltip.ShowWindow(SW_HIDE);
    }
}

void CWndControl::HideMoveableWindow()
{
    m_MoveableTooltip.ShowWindow(SW_HIDE);
}

//=============================================================================
/**
 * Override of OnCtlColor( ) to provide a callback when the dialog is closed.
 */
HBRUSH CWndControl::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
    // Call the base class implementation first! Otherwise, it may undo what we are trying to
    // accomplish here.
    HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);

    // Set the text color to red.
    // pDC->SetTextColor(RGB(255, 0, 0));

    // pDC->SetBkColor( CStudioPreferences::GetBaseColor( ) );

    // Return handle to our CBrush object.
    // hbr = m_Brush;

    return hbr;
}

//=============================================================================
/**
 * Override of EN_CHANGE
 */
void CWndControl::OnEnChange()
{
}
