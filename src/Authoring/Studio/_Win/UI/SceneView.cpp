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

CSceneView::CSceneView(CStudioApp *inStudioApp, QWidget *parent)
    : QWidget(parent)
{
    Q_UNUSED(inStudioApp)
    m_PreviousSelectMode = g_StudioApp.GetSelectMode();

    m_PlayerContainerWnd = new CPlayerContainerWnd(this);
    connect(m_PlayerContainerWnd, &CPlayerContainerWnd::toolChanged,
            this, [=](){ SetViewCursor(); Q_EMIT toolChanged(); });

    m_PlayerWnd = new CPlayerWnd(m_PlayerContainerWnd);
    connect(m_PlayerWnd, &CPlayerWnd::dropReceived, this, &CSceneView::onDropReceived);

    m_PlayerContainerWnd->SetPlayerWnd(m_PlayerWnd);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CSceneView::CSceneView()
{
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

CSceneView::~CSceneView()
{
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    // Stop listening for selection change events
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
    SetViewCursor();
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
    SetViewCursor();
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
    SetViewCursor();
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
 *	Resets its scroll ranges and recenters client in the window. This is called when
 *  an outside source needs to tell the scene view that size ranges have changed such
 *  as the preferences telling the sceneview that the size changed.
 */
void CSceneView::RecheckSizingMode()
{
    if (m_PlayerContainerWnd)
        m_PlayerContainerWnd->SetScrollRanges();
}

//==============================================================================
/**
 *	Redirect to PlayerContainerWnd to check whether we are in deployment view mode.
 *	@return true if is in deployment view mode, else false
 */
bool CSceneView::IsDeploymentView()
{
    // Default mode is SCENE_VIEW so if playercontainerwnd does not exist (should only happen on
    // startup), it is deployment view
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
    if (m_PlayerContainerWnd)
        m_PlayerContainerWnd->SetViewMode(inViewMode);
}

void CSceneView::SetToolMode(long inMode)
{
    if (m_PlayerContainerWnd)
        m_PlayerContainerWnd->setToolMode(inMode);
    SetViewCursor();
}

//==============================================================================
/**
 *	When the active camera is changed, the display string needs to be changed. Hence
 *	find which entry is the one which is modified and update with the new string
 *	@param inCamera	the camera that has been modified
 */
void CSceneView::OnEditCameraChanged()
{
    // Reset any scrolling and recalculate the window position.
    if (m_PlayerContainerWnd) {
        m_PlayerContainerWnd->SetScrollRanges();
        RecalcMatte();
        SetPlayerWndPosition();
    }

    // Update the view mode accordingly
    SetViewMode(g_StudioApp.GetRenderer().GetEditCamera() >= 0 ? CPlayerContainerWnd::VIEW_EDIT
                                                               : CPlayerContainerWnd::VIEW_SCENE);
    m_PlayerWnd->update();
}

void CSceneView::OnAuthorZoomChanged()
{
    OnEditCameraChanged();
}

void CSceneView::OnRulerGuideToggled()
{
    m_PlayerContainerWnd->OnRulerGuideToggled();
}
