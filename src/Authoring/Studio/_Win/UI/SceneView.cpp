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
//	Includes
//==============================================================================
#include "SceneView.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "StudioApp.h"
#include "Doc.h"
#include "Dispatch.h"
#include "MouseCursor.h"
#include "ResourceCache.h"
#include "Core.h"
#include "IStudioRenderer.h"

#include <QtGui/qevent.h>

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================

CSceneView::CSceneView(CStudioApp *inStudioApp, QWidget *parent)
    : QWidget(parent)
{
    Q_UNUSED(inStudioApp)
    m_PreviousToolMode = g_StudioApp.GetToolMode();
    m_PreviousSelectMode = g_StudioApp.GetSelectMode();

    m_PlayerContainerWnd = new CPlayerContainerWnd(this);
    connect(m_PlayerContainerWnd, &CPlayerContainerWnd::toolChanged, this,
            &CSceneView::toolChanged);

    m_PlayerWnd = new CPlayerWnd(m_PlayerContainerWnd);
    connect(m_PlayerWnd, &CPlayerWnd::dropReceived, this, &CSceneView::onDropReceived);

    m_PlayerContainerWnd->SetPlayerWnd(m_PlayerWnd);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CSceneView::CSceneView()
{
    m_PreviousToolMode = g_StudioApp.GetToolMode();
    m_PreviousSelectMode = g_StudioApp.GetSelectMode();
}

void CSceneView::onDropReceived()
{
    setFocus();
}

CPlayerWnd *CSceneView::GetPlayerWnd() const
{
    return m_PlayerWnd;
}

//==============================================================================
/**
 * Destructor: Releases the object.
 */
CSceneView::~CSceneView()
{
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    // Stop listening for selection change events
    theDispatch->RemoveSelectedNodePropChangeListener(this);
    theDispatch->RemoveClientPlayChangeListener(this);
    theDispatch->RemoveEditCameraChangeListener(this);
}

QSize CSceneView::sizeHint() const
{
    CPt theSize = CStudioPreferences::GetDefaultClientSize();
    return QSize(theSize.x, theSize.y);
}

//==============================================================================
/**
 * Called by the framework after the view is first attached
 * to the document, but before the view is initially displayed.
 * Notifies the Main Frame that the palettes need to be shown, and destroys the
 * splash screen.
 */
void CSceneView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    m_PlayerContainerWnd->RecenterClient();

    // Set the scroll information.
    m_PlayerContainerWnd->SetScrollRanges();

    // Create the cursors
    m_ArrowCursor = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ARROW);
    m_CursorGroupMove = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_GROUP_MOVE);
    m_CursorGroupRotate =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_GROUP_ROTATE);
    m_CursorGroupScale = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_GROUP_SCALE);
    m_CursorItemMove = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ITEM_MOVE);
    m_CursorItemRotate = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ITEM_ROTATE);
    m_CursorItemScale = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ITEM_SCALE);
    m_CursorEditCameraPan =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_PAN);
    m_CursorEditCameraRotate =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_ROTATE);
    m_CursorEditCameraZoom =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_ZOOM);

    g_StudioApp.GetCore()->GetDispatch()->AddEditCameraChangeListener(this);

    // Set the default cursor
    OnSetCursor();
}

//==============================================================================
/**
 * OnToolGroupSelection: Called when the Group Selection button is pressed.
 * Sets the current tool mode and changes the cursor.
 */
void CSceneView::OnToolGroupSelection()
{
    m_PreviousSelectMode = g_StudioApp.GetSelectMode();
    g_StudioApp.SetSelectMode(STUDIO_SELECTMODE_GROUP);
    OnSetCursor();
    Q_EMIT toolChanged();
}

//==============================================================================
/**
 *	OnToolItemSelection: Called when the Item Selection button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CSceneView::OnToolItemSelection()
{
    m_PreviousSelectMode = g_StudioApp.GetSelectMode();
    g_StudioApp.SetSelectMode(STUDIO_SELECTMODE_ENTITY);
    OnSetCursor();
    Q_EMIT toolChanged();
}

//==============================================================================
/**
 *	SetViewCursor: Sets the cursor for the view according to the current Client Tool.
 *
 *	Changes the cursor depending on the current tool mode.  Each time the Tool mode
 *	changes, you should call this function.  If you add extra tool modes, you
 *	will need to adjust this function.
 */
void CSceneView::SetViewCursor()
{
    long theCurrentToolSettings = g_StudioApp.GetToolMode();
    long theCurrentSelectSettings = g_StudioApp.GetSelectMode();

    // See what tool mode we are in
    switch (theCurrentToolSettings) {
    case STUDIO_TOOLMODE_MOVE:
        switch (theCurrentSelectSettings) {
        case STUDIO_SELECTMODE_ENTITY:
            m_PlayerWnd->setCursor(m_CursorItemMove);
            break;
        case STUDIO_SELECTMODE_GROUP:
            m_PlayerWnd->setCursor(m_CursorGroupMove);
            break;
            // Default - shouldn't happen
        default:
            m_PlayerWnd->setCursor(m_CursorItemMove);
            break;
        }
        break;

    case STUDIO_TOOLMODE_ROTATE:
        switch (theCurrentSelectSettings) {
        case STUDIO_SELECTMODE_ENTITY:
            m_PlayerWnd->setCursor(m_CursorItemRotate);
            break;
        case STUDIO_SELECTMODE_GROUP:
            m_PlayerWnd->setCursor(m_CursorGroupRotate);
            break;
            // Default - shouldn't happen
        default:
            m_PlayerWnd->setCursor(m_CursorItemRotate);
            break;
        }
        break;

    case STUDIO_TOOLMODE_SCALE:
        switch (theCurrentSelectSettings) {
        case STUDIO_SELECTMODE_ENTITY:
            m_PlayerWnd->setCursor(m_CursorItemScale);
            break;
        case STUDIO_SELECTMODE_GROUP:
            m_PlayerWnd->setCursor(m_CursorGroupScale);
            break;
            // Default - shouldn't happen
        default:
            m_PlayerWnd->setCursor(m_CursorItemScale);
            break;
        }
        break;

    case STUDIO_TOOLMODE_CAMERA_PAN:
        m_PlayerWnd->setCursor(m_CursorEditCameraPan);
        break;

    case STUDIO_TOOLMODE_CAMERA_ZOOM:
        m_PlayerWnd->setCursor(m_CursorEditCameraZoom);
        break;

    case STUDIO_TOOLMODE_CAMERA_ROTATE:
        m_PlayerWnd->setCursor(m_CursorEditCameraRotate);
        break;

        // Default - shouldn't happen
    default:
        m_PlayerWnd->setCursor(m_CursorItemMove);
        break;
    }
}

//==============================================================================
/**
 *	RecalcMatte: Recalculates the matte around the Client based on the settings.
 *
 *	This will recalculate the matting around the Client based on the Client's
 *	current size. If the Client is a "Fit To Window" mode, then the matte region
 *	is cleared.
 */
//==============================================================================
void CSceneView::RecalcMatte()
{
    long theXOffset = 0;
    long theYOffset = 0;
    QRect theClientRect = rect();

    // Adjust the client area based if rulers are visible
    if (m_PlayerContainerWnd) {
        m_PlayerContainerWnd->setGeometry(theXOffset, theYOffset,
                                          theClientRect.width() - theXOffset,
                                          theClientRect.height() - theYOffset);

        // Recenter the Client rect
        m_PlayerContainerWnd->RecenterClient();
    }
}

//==============================================================================
/**
 *	PtInClientRect: Returns true if the point lies in the client rect, false otherwise.
 *
 *	@param		inPoint	The point to check.
 */
//==============================================================================
bool CSceneView::PtInClientRect(const QPoint& inPoint)
{
    return m_PlayerContainerWnd && m_PlayerContainerWnd->geometry().contains(inPoint);
}

//==============================================================================
/**
 *	HandleModifierDown: Called when a modifier key (ctrl or alt) is pressed and held.
 *
 *	Changes tool modes and saves the previous mode.
 *
 *	@param		inChar		Contains the character code value of the key.
 *  @param      inRepCnt    Contains the repeat count, the number of times the keystroke
 *                          is repeated when user holds down the key.
 *  @param      modifiers   Contains the scan code, key-transition code, previous
 *                          key state, and context code.
 */
//==============================================================================
bool CSceneView::HandleModifierDown(int inChar, int inRepCnt, Qt::KeyboardModifiers modifiers)
{
    bool theHandledFlag = false;

    UNREFERENCED_PARAMETER(inRepCnt);

    // Get the position of the mouse and the rectangle containing the scene view
    QPoint theCursorPosition = mapFromGlobal(QCursor::pos());
    QRect theWindowRect = rect();

    // If we are currently dragging an object or the cursor is over the scene
    if (((theWindowRect.contains(theCursorPosition))
         || (m_PlayerContainerWnd && m_PlayerContainerWnd->IsMouseDown()))) {
        bool theCtrlKeyIsDown = modifiers & Qt::ControlModifier;
        bool theAltKeyIsDown = modifiers & Qt::AltModifier;

        // If the control key is being pressed and the Alt key is not down
        if ((inChar == Qt::Key_Control) && (!theAltKeyIsDown)) {
            // If this is the first press, toggle tool modes
            m_RegisteredKeyDown = true;

            if (m_PlayerContainerWnd->IsMiddleMouseDown() && !IsDeploymentView()) {
                // Do nothing, do not let it switch to other tool when middle mouse is down
            } else {
                // See what tool mode we are in and change modes accordingly
                SetToolOnCtrl();
                theHandledFlag = true;
            }
        }
        // If the alt key is being pressed and the control key is not down
        else if ((inChar == Qt::Key_Alt) && (!theCtrlKeyIsDown)) {
            m_RegisteredKeyDown = true;

            if (m_PlayerContainerWnd->IsMiddleMouseDown() && !IsDeploymentView()) {
                // press Alt-Scroll Wheel Click
                // Do Camera Rotate if we are in 3D Camera
                if (g_StudioApp.GetRenderer().DoesEditCameraSupportRotation(
                            g_StudioApp.GetRenderer().GetEditCamera())) {
                    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
                    theHandledFlag = true;
                }
            } else {
                // See what tool mode we are in and change modes accordingly
                SetToolOnAlt();
                theHandledFlag = true;
            }
        }
        OnSetCursor();
        Q_EMIT toolChanged();
    }

    return theHandledFlag;
}

//==============================================================================
/**
 *	HandleModifierUp: Handles the release of a modifier key (ctrl or alt).
 *
 *	Changes tool modes back to the original tool mode.
 *
 *	@param	inChar		Contains the character code value of the key.
 *  @param  inRepCnt    Contains the repeat count, the number of times the keystroke is
 *                      repeated when user holds down the key.
 *  @param  modifiers   Contains the scan code, key-transition code, previous key
 *state, and context code.
 */
//==============================================================================
bool CSceneView::HandleModifierUp(int inChar, int inRepCnt, Qt::KeyboardModifiers modifiers)
{
    UNREFERENCED_PARAMETER(inRepCnt);

    bool theHandledFlag = false;

    // Get the position of the mouse and the rectangle containing the scene view
    QPoint theCursorPosition = mapFromGlobal(QCursor::pos());
    QRect theWindowRect = rect();

    // If we are currently dragging an object or the cursor is over the scene
    if (((theWindowRect.contains(theCursorPosition))
         || (m_PlayerContainerWnd && m_PlayerContainerWnd->IsMouseDown()))) {
        bool theCtrlKeyIsDown = modifiers & Qt::ControlModifier;
        bool theAltKeyIsDown = modifiers & Qt::AltModifier;

        // If the control key or alt key is released (and the opposite key is not down) revert back
        // to the original tool mode
        if (((inChar == Qt::Key_Control) && (!theAltKeyIsDown))
                || ((inChar == Qt::Key_Alt) && (!theCtrlKeyIsDown))) {
            if (m_PlayerContainerWnd->IsMiddleMouseDown())
                g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
            else
                RestorePreviousTool();

            OnSetCursor();
            Q_EMIT toolChanged();

            theHandledFlag = true;
        }
    }

    return theHandledFlag;
}

//==============================================================================
/**
 * resizeEvent: Handles the WM_SIZE message
 *
 * Recenters the Client and recaluclates the matte when a resize message is
 * generated.
 *
 * @param  nType    Specifies the type of resizing requested.
 * @param  cx       Specifies the new width of the client area.
 * @param  cy       Specifies the new height of the client area.
 */
//==============================================================================
void CSceneView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_PlayerContainerWnd) {
        RecalcMatte();
        SetPlayerWndPosition();
        g_StudioApp.GetCore()->GetDoc( )->GetSceneGraph()->RequestRender();
    }
}

//==============================================================================
/**
 *	SetPlayerWndPosition: Sets the position of the child player window
 *
 *  Called when the view is scrolled to position the child player window
 *
 */
//==============================================================================
void CSceneView::SetPlayerWndPosition()
{
    // Move the child player window to coincide with the scrollbars
    if (m_PlayerContainerWnd) {
        long theLeft, theTop;
        // Retrieve the left and top edge of the presentation currently in view
        m_PlayerContainerWnd->SetPlayerWndPosition(theLeft, theTop);
        m_PlayerContainerWnd->update();
    }
}

//==============================================================================
/**
 *	SetRegisteredKey: Sets the staus to true or false
 *
 *	If we've already processesed an alt or tab, then set the flag
 */
//==============================================================================
void CSceneView::SetRegisteredKey(bool inStatus)
{
    m_RegisteredKeyDown = inStatus;
}

//==============================================================================
/**
 *	GetKeyStatus: get the staus
 *
 *	check to see if the flag is true or false
 */
//==============================================================================
bool CSceneView::GetKeyStatus()
{
    return m_RegisteredKeyDown;
}

//=============================================================================
/**
 * Register all the events for hotkeys that are active for the entire application.
 * Hotkeys for the entire application are ones that are not view specific in
 * scope.
 *
 * @param inShortcutHandler the global shortcut handler.
 */
//=============================================================================
void CSceneView::RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler)
{
    inShortcutHandler->RegisterKeyDownEvent(
                new CDynHotKeyConsumer<CSceneView>(this, &CSceneView::HandleModifierDown),
                Qt::ControlModifier, Qt::Key_Control);
    inShortcutHandler->RegisterKeyDownEvent(
                new CDynHotKeyConsumer<CSceneView>(this, &CSceneView::HandleModifierDown),
                Qt::AltModifier, Qt::Key_Alt);
    inShortcutHandler->RegisterKeyUpEvent(
                new CDynHotKeyConsumer<CSceneView>(this, &CSceneView::HandleModifierUp), 0,
                Qt::Key_Control);
    inShortcutHandler->RegisterKeyUpEvent(
                new CDynHotKeyConsumer<CSceneView>(this, &CSceneView::HandleModifierUp), 0,
                Qt::Key_Alt);
}

//==============================================================================
/**
 * OnSetCursor: Handles the WM_SETCURSOR Windows message.
 *
 * Changes the cursor depending on the current tool mode.
 *
 * @param inWnd     Specifies a pointer to the window that contains the cursor.
 * @param inHitTest Specifies the hit-test area code. The hit test determines the cursor's
 *location.
 * @param inMessage Specifies the mouse message number.
 *
 * @return true if the cursor was set
 */
void CSceneView::OnSetCursor()
{
    SetViewCursor();
}

//==============================================================================
/**
 *	Called when the tool mode changes, scene view maintains its own mode so it can
 *	return to that mode when modifiers are pressed
 *  @param inMode the mode to which to change
 */
void CSceneView::SetToolMode(long inMode)
{
    m_PreviousToolMode = inMode;
    SetViewCursor();
}

//==============================================================================
/**
 *	Resets its scroll ranges and recenters client in the window. This is called when
 *  an outside source needs to tell the scene view that size ranges have changed such
 *  as the preferences telling the sceneview that the size changed.
 */
void CSceneView::RecheckSizingMode()
{
    if (m_PlayerContainerWnd) {
        m_PlayerContainerWnd->SetScrollRanges();
    }
}

//=============================================================================
/**
 *	Called a selected node property has changed.
 *	@param inPropertyName	name of the property changed
 */
void CSceneView::OnPropValueChanged(const Q3DStudio::CString &inPropertyName)
{
}

//=============================================================================
/**
 *	Called a selected node property has its property changed via a mouse drag
 *	@param inConstrainXAxis  true if dragging is constrained along the X-Axis
 *	@param inConstrainYAxis  true if dragging is constrained along the Y-Axis
 */
void CSceneView::OnMouseDragged(bool inConstrainXAxis, bool inConstrainYAxis)
{
    // If not constraining to Y-Axis, snap horizontally
    // If not constraining to X-Axis, snap vertically
    // true to update tranform, as this is called from a mouse drag and the global transform may be
    // outdated.
    // false to not update the scene, since this is a result of a mouse drag which is already doing
    // the updating
}

//==========================================================================
/**
 *	Called when the presentation time changes.
 *	When animating an object, the markers must be updated with the selected object
 */
void CSceneView::OnTimeChanged(long inTime)
{
    UNREFERENCED_PARAMETER(inTime);
}

//==============================================================================
/**
 *	Restore to the previous tool mode. This is called when the modifier is released
 */
//==============================================================================
void CSceneView::RestorePreviousTool()
{
    // What was the original tool mode?
    switch (m_PreviousToolMode) {
    case STUDIO_TOOLMODE_MOVE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
        break;

    case STUDIO_TOOLMODE_ROTATE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_ROTATE);
        break;

    case STUDIO_TOOLMODE_SCALE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_SCALE);
        break;

    case STUDIO_TOOLMODE_CAMERA_PAN:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
        break;

    case STUDIO_TOOLMODE_CAMERA_ROTATE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
        break;

    case STUDIO_TOOLMODE_CAMERA_ZOOM:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ZOOM);
        break;
    }
}

//==============================================================================
/**
 *	Change the tool when the Ctrl Key is pressed
 */
//==============================================================================
void CSceneView::SetToolOnCtrl()
{
    switch (m_PreviousToolMode) {
    // If we are in move mode, switch to rotate
    case STUDIO_TOOLMODE_MOVE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_ROTATE);
        break;

    // If we are in rotate mode, switch to move
    case STUDIO_TOOLMODE_ROTATE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
        break;

    // If we are in scale mode, switch to move
    case STUDIO_TOOLMODE_SCALE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
        break;

    // If we are in camera pan mode, switch to camera orbit
    case STUDIO_TOOLMODE_CAMERA_PAN:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
        break;

    // If we are in camera orbit mode, switch to camera pan
    case STUDIO_TOOLMODE_CAMERA_ROTATE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
        break;

    // If we are in camera zoom mode, switch to camera pan
    case STUDIO_TOOLMODE_CAMERA_ZOOM:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
        break;
    }
}

//==============================================================================
/**
 *	Change the tool when the Alt Key is pressed
 */
//==============================================================================
void CSceneView::SetToolOnAlt()
{
    switch (m_PreviousToolMode) {
    // If we are in move mode, switch to rotate
    case STUDIO_TOOLMODE_MOVE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_SCALE);
        break;

    // If we are in rotate mode, switch to move
    case STUDIO_TOOLMODE_ROTATE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_SCALE);
        break;

    // If we are in scale mode, switch to move
    case STUDIO_TOOLMODE_SCALE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_ROTATE);
        break;

    // If we are in camera pan mode, switch to camera zoom
    case STUDIO_TOOLMODE_CAMERA_PAN:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ZOOM);
        break;

    // If we are in camera orbit mode, switch to camera zoom
    case STUDIO_TOOLMODE_CAMERA_ROTATE:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ZOOM);
        break;

    // If we are in camera zoom mode, switch to camera orbit
    case STUDIO_TOOLMODE_CAMERA_ZOOM:
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
        break;
    }
}

//==============================================================================
/**
 *	Redirect to PlayerContainerWnd to check whether we are in deployment view mode.
 *	@return true if is in deployment view mode, else false
 */
bool CSceneView::IsDeploymentView()
{
    // default mode is SCENE_VIEW so if playercontainerwnd does not exist ( should only happen on
    // startup ), it is deployment view
    bool theStatus = true;
    if (m_PlayerContainerWnd)
        theStatus = m_PlayerContainerWnd->IsDeploymentView();

    return theStatus;
}

//==============================================================================
/**
 *	Redirect to PlayerContainerWnd to set the view mode of the current scene view,
 *	whether we are in editing mode or deployment mode. For editing mode, we want to
 *	use the full scene area without any matte area.
 *	@param inViewMode	the view mode of this scene
 */
void CSceneView::SetViewMode(CPlayerContainerWnd::EViewMode inViewMode)
{
    if (m_PlayerContainerWnd) {
        m_PlayerContainerWnd->SetViewMode(inViewMode);
    }
}

//==============================================================================
/**
 *	When the active camera is changed, the display string needs to be changed. Hence
 *	find which entry is the one which is modified and update with the new string
 *	@param inCamera	the camera that has been modified
 */
void CSceneView::OnEditCameraChanged()
{
    // reset any scrolling and recalculate the window position.
    if (m_PlayerContainerWnd) {
        m_PlayerContainerWnd->SetScrollRanges();
        RecalcMatte();
        SetPlayerWndPosition();
    }

    // update the view mode accordingly
    SetViewMode(g_StudioApp.GetRenderer().GetEditCamera() >= 0 ? CPlayerContainerWnd::VIEW_EDIT
                                                               : CPlayerContainerWnd::VIEW_SCENE);
    m_PlayerWnd->update();
}

//==============================================================================
/**
 *	When the active camera is changed, the display string needs to be changed. Hence
 *	find which entry is the one which is modified and update with the new string
 *	@param inCamera	the camera that has been modified
 */
void CSceneView::OnEditCamerasTransformed()
{
}

void CSceneView::OnAuthorZoomChanged()
{
    OnEditCameraChanged();
}

void CSceneView::OnRulerGuideToggled()
{
    m_PlayerContainerWnd->OnRulerGuideToggled();
}
