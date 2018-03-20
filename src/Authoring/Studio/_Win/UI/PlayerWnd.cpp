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

#include <QtWidgets/qmessagebox.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>

CPlayerWnd::CPlayerWnd(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_containerWnd(nullptr)
    , m_mouseDown(false)
{
    setAcceptDrops(true);
    RegisterForDnd(this);
    AddMainFlavor(QT3DS_FLAVOR_FILE);
    AddMainFlavor(QT3DS_FLAVOR_ASSET_UICFILE);
    AddMainFlavor(QT3DS_FLAVOR_ASSET_LIB);
    AddMainFlavor(QT3DS_FLAVOR_BASIC_OBJECTS);

    setFormat(CWGLRenderContext::selectSurfaceFormat(this));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CPlayerWnd::~CPlayerWnd()
{
}

void CPlayerWnd::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);
    update();
}

void CPlayerWnd::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mouseDown) {
        long theModifierKeys = 0;
        if (event->buttons() & Qt::LeftButton
                || (!g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance().Valid())
                && !m_containerWnd->IsDeploymentView()) {
            // When in edit camera view and nothing is selected, all buttons are mapped
            // as left button. That is how camera control tools work, they are all
            // assuming left button.
            theModifierKeys = CHotKeys::MOUSE_LBUTTON | CHotKeys::GetCurrentKeyModifiers();
        } else if (event->buttons() & Qt::RightButton) {
            theModifierKeys = CHotKeys::MOUSE_RBUTTON | CHotKeys::GetCurrentKeyModifiers();
        } else if (event->buttons() & Qt::MiddleButton) {
            theModifierKeys = CHotKeys::MOUSE_MBUTTON | CHotKeys::GetCurrentKeyModifiers();
        }
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDrag(
                    SceneDragSenderType::Matte, event->pos(), g_StudioApp.GetToolMode(),
                    theModifierKeys);
    } else {
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseMove(
                    SceneDragSenderType::SceneWindow, event->pos());
    }
}

void CPlayerWnd::mousePressEvent(QMouseEvent *event)
{
    long toolMode = g_StudioApp.GetToolMode();
    const Qt::MouseButton btn = event->button();

    if (!g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance().Valid()
            && !m_containerWnd->IsDeploymentView() && (event->modifiers() & Qt::AltModifier)
            && g_StudioApp.GetRenderer().DoesEditCameraSupportRotation(
                g_StudioApp.GetRenderer().GetEditCamera())) {
        // We are in edit camera view and nothing is selected, so we are in Alt-click camera tool
        // controlling mode
        m_mouseDown = true;
        if (btn == Qt::MiddleButton) {
            // Alt + Wheel Click
            toolMode = STUDIO_TOOLMODE_CAMERA_PAN;
        } else if (btn == Qt::LeftButton) {
            // Alt + Left Click
            toolMode = STUDIO_TOOLMODE_CAMERA_ROTATE;
        } else if (btn == Qt::RightButton) {
            // Alt + Right Click
            toolMode = STUDIO_TOOLMODE_CAMERA_ZOOM;
        }
        g_StudioApp.SetToolMode(toolMode);
        Q_EMIT m_containerWnd->toolChanged();
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(SceneDragSenderType::Matte,
                                                                 event->pos(), toolMode);
    } else {
        if (btn == Qt::LeftButton || btn == Qt::RightButton) {
            // Pause playback for the duration of the mouse click
            if (g_StudioApp.IsPlaying()) {
                g_StudioApp.PlaybackStopNoRestore();
                m_resumePlayOnMouseRelease = true;
            } else {
                m_resumePlayOnMouseRelease = false;
            }

            toolMode = g_StudioApp.GetToolMode();
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(
                        SceneDragSenderType::SceneWindow, event->pos(), toolMode);
            m_mouseDown = true;
        } else if (btn == Qt::MiddleButton) {
            event->ignore();
        }
    }
}

void CPlayerWnd::mouseReleaseEvent(QMouseEvent *event)
{
    const Qt::MouseButton btn = event->button();
    if (!m_containerWnd->IsDeploymentView()
            && !g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance().Valid()) {
        // We are in edit camera view and nothing is selected
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(SceneDragSenderType::Matte);
        g_StudioApp.GetCore()->CommitCurrentCommand();
        m_mouseDown = false;
        // Restore normal tool mode
        if (event->modifiers() & Qt::AltModifier)
            g_StudioApp.SetToolMode(STUDIO_TOOLMODE_SCALE);
        else if (event->modifiers() & Qt::ControlModifier)
            g_StudioApp.SetToolMode(STUDIO_TOOLMODE_ROTATE);
        else
            g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
        Q_EMIT m_containerWnd->toolChanged();
    } else {
        if (btn == Qt::LeftButton || btn == Qt::RightButton) {
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(
                        SceneDragSenderType::SceneWindow);
            g_StudioApp.GetCore()->CommitCurrentCommand();
            m_mouseDown = false;
            if (m_resumePlayOnMouseRelease) {
                m_resumePlayOnMouseRelease = false;
                g_StudioApp.PlaybackPlay();
            }
        } else if (btn == Qt::MiddleButton) {
            event->ignore();
        }
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

void CPlayerWnd::setContainerWnd(CPlayerContainerWnd *inContainerWnd)
{
    m_containerWnd = inContainerWnd;
    updateGeometry();
}

QSize CPlayerWnd::sizeHint() const
{
    if (m_containerWnd)
        return m_containerWnd->GetEffectivePresentationSize();
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
                                  tr("Unable to initialize OpenGL.\nThis may be because your "
                                     "graphic device is not sufficient, or simply because your "
                                     "driver is too old.\n\nPlease try upgrading your graphics "
                                     "driver and try again."));
            exit(1);
        }
    }
}

void CPlayerWnd::paintGL()
{
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.GetRenderer());
    // Don't use request render here, this has to be synchronous inside paintGL
    theRenderer.RenderNow();
}

qreal CPlayerWnd::fixedDevicePixelRatio() const
{
    // Fix a problem on X11: https://bugreports.qt.io/browse/QTBUG-65570
    qreal ratio = devicePixelRatio();
    if (QWindow *w = window()->windowHandle()) {
        if (QScreen *s = w->screen())
            ratio = s->devicePixelRatio();
    }
    return ratio;
}

void CPlayerWnd::resizeGL(int width, int height)
{
    // This also passes the new FBO to the OpenGLContext
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.GetRenderer());
    theRenderer.SetViewRect(QRect(0, 0, width * fixedDevicePixelRatio(),
                                  height * fixedDevicePixelRatio()));
}
