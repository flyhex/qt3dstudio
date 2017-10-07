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

#include "PlayerWnd.h"
#include "MainFrm.h"
#include "SceneView.h"
#include "Dispatch.h"
#include "MasterP.h"
#include "HotKeys.h"
#include "StudioApp.h"
#include "Doc.h"
#include "Dispatch.h"
#include "HotKeys.h"
#include "MouseCursor.h"
#include "ResourceCache.h"
#include "SceneDropTarget.h"
#include "Core.h"
#include "IDragable.h"
#include "WGLRenderContext.h"
#include "IStudioRenderer.h"

#include <QMessageBox>
#include <QMouseEvent>

//==============================================================================
//	Class CPlayerWnd
//==============================================================================

CPlayerWnd::CPlayerWnd(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_ContainerWnd(nullptr)
    , m_IsMouseDown(false)
    , m_PreviousToolMode(0)
    , m_FitClientToWindow(false)
{
    m_LastKnownMousePosition = QPoint(-1, -1);

    setAcceptDrops(true);
    RegiserForDnd(this);
    AddMainFlavor(EUIC_FLAVOR_FILE);
    AddMainFlavor(EUIC_FLAVOR_ASSET_UICFILE);
    AddMainFlavor(EUIC_FLAVOR_ASSET_LIB);
    AddMainFlavor(EUIC_FLAVOR_BASIC_OBJECTS);

    setFormat(CWGLRenderContext::selectSurfaceFormat(this));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CPlayerWnd::~CPlayerWnd()
{
}

//==============================================================================
/**
 *	OnMouseMove: Handle the WM_MOUSEMOVE message
 *
 *	@param inFlags	flags passed in with th mouse move message
 *	@param inPoint	where the mouse is
 */
//==============================================================================
void CPlayerWnd::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::MiddleButton) {
        // Middle button events are handled by the parent CPlayerContainerWnd
        event->ignore();
    } else {
        if (m_IsMouseDown) {
            long theModifierKeys = 0;
            if (event->buttons() & Qt::LeftButton)
                theModifierKeys = CHotKeys::MOUSE_LBUTTON | CHotKeys::GetCurrentKeyModifiers();
            else if (event->buttons() & Qt::RightButton)
                theModifierKeys = CHotKeys::MOUSE_RBUTTON | CHotKeys::GetCurrentKeyModifiers();

            long theToolMode = g_StudioApp.GetToolMode();
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDrag(
                SceneDragSenderType::SceneWindow, event->pos(), theToolMode, theModifierKeys);
        } else {
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseMove(
                        SceneDragSenderType::SceneWindow, event->pos());
        }
    }
}

//==============================================================================
/**
 *	OnSize: Handle the WM_SIZE message
 *
 *	@param nType Passed to the base class
 *	@param cx change in x window size
 *	@param cy change in y window size
 */
//==============================================================================
void CPlayerWnd::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);

    update();
}

//==============================================================================
/**
 *	OnLButtonDown: Handle the WM_LBUTTONDOWN message
 *
 *	@param inFlags Flags passed in from the message
 *	@param inPoint The point wher the button was clicked
 */
//==============================================================================
void CPlayerWnd::mousePressEvent(QMouseEvent *event)
{
    const Qt::MouseButton btn = event->button();
    if ((btn == Qt::LeftButton) || (btn == Qt::RightButton)) {
        long theToolMode = g_StudioApp.GetToolMode();
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(SceneDragSenderType::SceneWindow,
                                                                 event->pos(), theToolMode);
        m_IsMouseDown = true;
    } else if (btn == Qt::MiddleButton) {
        event->ignore();
    }
}

//==============================================================================
/**
 *	OnLButtonUp: Called whenever the user releases the left mouse button.
 *
 *	This processes the WM_LBUTTONUP message. This message is generated whenever
 *	the left mouse button. We release the mouse capture to stop dragging.
 *
 *	@param inFlags the flags passed in from the message call
 *	@param the point where the lbutton up takes place
 */
//==============================================================================
void CPlayerWnd::mouseReleaseEvent(QMouseEvent *event)
{
    const Qt::MouseButton btn = event->button();
    if ((btn == Qt::LeftButton) || (btn == Qt::RightButton)) {
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(SceneDragSenderType::SceneWindow);
        g_StudioApp.GetCore()->CommitCurrentCommand();
        m_IsMouseDown = false;
    } else if (btn == Qt::MiddleButton) {
        event->ignore();
    }
}

void CPlayerWnd::mouseDoubleClickEvent(QMouseEvent *event)
{
    g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDblClick(
        SceneDragSenderType::SceneWindow, event->pos());
}


bool CPlayerWnd::OnDragWithin(CDropSource &inSource)
{
    CSceneViewDropTarget theTarget;
    return theTarget.Accept(inSource);
}
bool CPlayerWnd::OnDragReceive(CDropSource &inSource)
{
    CSceneViewDropTarget theTarget;
    Q_EMIT dropReceived();
    return theTarget.Drop(inSource);
}
void CPlayerWnd::OnDragLeave()
{
}

//==============================================================================
/**
 *	SetContainerWnd: keep track of a pointer to the containing window
 *
 *	@param inContainerWnd pointer to what the containing window will be set to
 */
//==============================================================================
void CPlayerWnd::SetContainerWnd(CPlayerContainerWnd *inContainerWnd)
{
    m_ContainerWnd = inContainerWnd;
    updateGeometry();
}

QSize CPlayerWnd::sizeHint() const
{
    if (m_ContainerWnd)
        return m_ContainerWnd->GetEffectivePresentationSize();
    else
        return QOpenGLWidget::sizeHint();
}

void CPlayerWnd::initializeGL()
{
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.GetRenderer());
    if (theRenderer.IsInitialized() == false) {
        try {
            theRenderer.Initialize(this);
        } catch (...) {

            QMessageBox::critical(this, tr("Fatal Error"),
                tr("Unable to initialize OpenGL.\nThis may be because your graphic device is "
                   "not sufficient, or simply because your driver is too old.\n\nPlease try "
                   "upgrading your graphics driver and try again."));
            exit(1);
        }
    }
}

void CPlayerWnd::paintGL()
{
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.GetRenderer());
    // don't use request render here, this has to be
    // synchronous inside paintGL
    theRenderer.RenderNow();
}

void CPlayerWnd::resizeGL(int width, int height)
{
    // this also passes the new FBO to the CWGLContext
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.GetRenderer());
    theRenderer.SetViewRect(QRect(0, 0, width * devicePixelRatio(),
                                  height * devicePixelRatio()));
}
