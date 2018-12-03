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

CSceneView::CSceneView(QWidget *parent)
    : QWidget(parent)
    , m_playerContainerWnd(new CPlayerContainerWnd(this))
    , m_playerWnd(new Q3DStudio::Q3DSPlayerWnd(m_playerContainerWnd.data()))
{
    m_previousSelectMode = g_StudioApp.GetSelectMode();

    connect(m_playerContainerWnd.data(), &CPlayerContainerWnd::toolChanged,
            this, [=](){ setViewCursor(); Q_EMIT toolChanged(); });

    connect(m_playerWnd.data(), &Q3DStudio::Q3DSPlayerWnd::dropReceived,
            this, &CSceneView::onDropReceived);

    m_playerContainerWnd->SetPlayerWnd(m_playerWnd.data());

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CSceneView::CSceneView()
{
    m_previousSelectMode = g_StudioApp.GetSelectMode();
}

void CSceneView::onDropReceived()
{
    setFocus();
}

Q3DStudio::Q3DSPlayerWnd *CSceneView::getPlayerWnd() const
{
    return m_playerWnd.data();
}

CSceneView::~CSceneView()
{
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    // Stop listening for selection change events
    theDispatch->RemoveEditCameraChangeListener(this);
}

QSize CSceneView::sizeHint() const
{
    return CStudioPreferences::GetDefaultClientSize();
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

    m_playerContainerWnd->RecenterClient();

    // Set the scroll information.
    m_playerContainerWnd->SetScrollRanges();

    // Create the cursors
    m_arrowCursor = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ARROW);
    m_cursorGroupMove = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_GROUP_MOVE);
    m_cursorGroupRotate =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_GROUP_ROTATE);
    m_cursorGroupScale = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_GROUP_SCALE);
    m_cursorItemMove = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ITEM_MOVE);
    m_cursorItemRotate = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ITEM_ROTATE);
    m_cursorItemScale = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_ITEM_SCALE);
    m_cursorEditCameraPan =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_PAN);
    m_cursorEditCameraRotate =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_ROTATE);
    m_cursorEditCameraZoom =
            CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_ZOOM);

    g_StudioApp.GetCore()->GetDispatch()->AddEditCameraChangeListener(this);

    // Set the default cursor
    setViewCursor();
}

//==============================================================================
/**
 * OnToolGroupSelection: Called when the Group Selection button is pressed.
 * Sets the current tool mode and changes the cursor.
 */
void CSceneView::onToolGroupSelection()
{
    m_previousSelectMode = g_StudioApp.GetSelectMode();
    g_StudioApp.SetSelectMode(STUDIO_SELECTMODE_GROUP);
    setViewCursor();
    Q_EMIT toolChanged();
}

//==============================================================================
/**
 *	OnToolItemSelection: Called when the Item Selection button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CSceneView::onToolItemSelection()
{
    m_previousSelectMode = g_StudioApp.GetSelectMode();
    g_StudioApp.SetSelectMode(STUDIO_SELECTMODE_ENTITY);
    setViewCursor();
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
void CSceneView::setViewCursor()
{
    long theCurrentToolSettings = g_StudioApp.GetToolMode();
    long theCurrentSelectSettings = g_StudioApp.GetSelectMode();

    // See what tool mode we are in
    switch (theCurrentToolSettings) {
    case STUDIO_TOOLMODE_MOVE:
        switch (theCurrentSelectSettings) {
        case STUDIO_SELECTMODE_ENTITY:
            m_playerWnd->setCursor(m_cursorItemMove);
            break;
        case STUDIO_SELECTMODE_GROUP:
            m_playerWnd->setCursor(m_cursorGroupMove);
            break;
            // Default - shouldn't happen
        default:
            m_playerWnd->setCursor(m_cursorItemMove);
            break;
        }
        break;

    case STUDIO_TOOLMODE_ROTATE:
        switch (theCurrentSelectSettings) {
        case STUDIO_SELECTMODE_ENTITY:
            m_playerWnd->setCursor(m_cursorItemRotate);
            break;
        case STUDIO_SELECTMODE_GROUP:
            m_playerWnd->setCursor(m_cursorGroupRotate);
            break;
            // Default - shouldn't happen
        default:
            m_playerWnd->setCursor(m_cursorItemRotate);
            break;
        }
        break;

    case STUDIO_TOOLMODE_SCALE:
        switch (theCurrentSelectSettings) {
        case STUDIO_SELECTMODE_ENTITY:
            m_playerWnd->setCursor(m_cursorItemScale);
            break;
        case STUDIO_SELECTMODE_GROUP:
            m_playerWnd->setCursor(m_cursorGroupScale);
            break;
            // Default - shouldn't happen
        default:
            m_playerWnd->setCursor(m_cursorItemScale);
            break;
        }
        break;

    case STUDIO_TOOLMODE_CAMERA_PAN:
        m_playerWnd->setCursor(m_cursorEditCameraPan);
        break;

    case STUDIO_TOOLMODE_CAMERA_ZOOM:
        m_playerWnd->setCursor(m_cursorEditCameraZoom);
        break;

    case STUDIO_TOOLMODE_CAMERA_ROTATE:
        m_playerWnd->setCursor(m_cursorEditCameraRotate);
        break;

        // Default - shouldn't happen
    default:
        m_playerWnd->setCursor(m_cursorItemMove);
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
void CSceneView::recalcMatte()
{
    long theXOffset = 0;
    long theYOffset = 0;
    QRect theClientRect = rect();

    // Adjust the client area based if rulers are visible
    if (m_playerContainerWnd) {
        m_playerContainerWnd->setGeometry(theXOffset, theYOffset,
                                          theClientRect.width() - theXOffset,
                                          theClientRect.height() - theYOffset);

        // Recenter the Client rect
        m_playerContainerWnd->RecenterClient();
    }
}

void CSceneView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_playerContainerWnd) {
        recalcMatte();
        setPlayerWndPosition();
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
void CSceneView::setPlayerWndPosition()
{
    // Move the child player window to coincide with the scrollbars
    if (m_playerContainerWnd) {
        m_playerContainerWnd->SetPlayerWndPosition();
        m_playerContainerWnd->update();
    }
}

//==============================================================================
/**
 *	Resets its scroll ranges and recenters client in the window. This is called when
 *  an outside source needs to tell the scene view that size ranges have changed such
 *  as the preferences telling the sceneview that the size changed.
 */
void CSceneView::recheckSizingMode()
{
    if (m_playerContainerWnd)
        m_playerContainerWnd->SetScrollRanges();
}

//==============================================================================
/**
 *	Redirect to PlayerContainerWnd to check whether we are in deployment view mode.
 *	@return true if is in deployment view mode, else false
 */
bool CSceneView::isDeploymentView()
{
    // Default mode is SCENE_VIEW so if playercontainerwnd does not exist (should only happen on
    // startup), it is deployment view
    bool theStatus = true;
    if (m_playerContainerWnd)
        theStatus = m_playerContainerWnd->IsDeploymentView();

    return theStatus;
}

//==============================================================================
/**
 *	Redirect to PlayerContainerWnd to set the view mode of the current scene view,
 *	whether we are in editing mode or deployment mode. For editing mode, we want to
 *	use the full scene area without any matte area.
 *	@param inViewMode	the view mode of this scene
 */
void CSceneView::setViewMode(CPlayerContainerWnd::EViewMode inViewMode)
{
    if (m_playerContainerWnd)
        m_playerContainerWnd->SetViewMode(inViewMode);
}

void CSceneView::setToolMode(long inMode)
{
    if (m_playerContainerWnd)
        m_playerContainerWnd->setToolMode(inMode);
    setViewCursor();
}

//==============================================================================
/**
 *	When the active camera is changed, the display string needs to be changed. Hence
 *	find which entry is the one which is modified and update with the new string
 *	@param inCamera	the camera that has been modified
 */
void CSceneView::onEditCameraChanged()
{
    // Reset any scrolling and recalculate the window position.
    if (m_playerContainerWnd) {
        m_playerContainerWnd->SetScrollRanges();
        recalcMatte();
        setPlayerWndPosition();
    }

    // Update the view mode accordingly
    setViewMode(g_StudioApp.getRenderer().GetEditCamera() >= 0 ? CPlayerContainerWnd::VIEW_EDIT
                                                               : CPlayerContainerWnd::VIEW_SCENE);
    m_playerWnd->update();
}

void CSceneView::onAuthorZoomChanged()
{
    onEditCameraChanged();
}

void CSceneView::onRulerGuideToggled()
{
    m_playerContainerWnd->OnRulerGuideToggled();
}
