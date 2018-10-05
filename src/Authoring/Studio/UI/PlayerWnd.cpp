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

#include "Qt3DSCommonPrecompile.h"
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
#include <QtGui/qopenglcontext.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qoffscreensurface.h>
#include <QtWidgets/qopenglwidget.h>


static bool compareContextVersion(QSurfaceFormat a, QSurfaceFormat b)
{
    if (a.renderableType() != b.renderableType())
        return false;
    if (a.majorVersion() != b.majorVersion())
        return false;
    if (a.minorVersion() > b.minorVersion())
        return false;
    return true;
}

static QSurfaceFormat selectSurfaceFormat(QOpenGLWidget* window)
{
    struct ContextVersion {
        int major;
        int minor;
        qt3ds::render::NVRenderContextType contextType;
    };

    ContextVersion versions[] = {
        {4, 5, qt3ds::render::NVRenderContextValues::GL4},
        {4, 4, qt3ds::render::NVRenderContextValues::GL4},
        {4, 3, qt3ds::render::NVRenderContextValues::GL4},
        {4, 2, qt3ds::render::NVRenderContextValues::GL4},
        {4, 1, qt3ds::render::NVRenderContextValues::GL4},
        {3, 3, qt3ds::render::NVRenderContextValues::GL3},
        {2, 1, qt3ds::render::NVRenderContextValues::GL2},
        {2, 0, qt3ds::render::NVRenderContextValues::GLES2},
        {0, 0, qt3ds::render::NVRenderContextValues::NullContext}
    };

    QSurfaceFormat result = window->format();
    bool valid = false;

    for (const auto &ver : versions) {
        if (ver.contextType == qt3ds::render::NVRenderContextValues::NullContext)
            break;

        // make an offscreen surface + context to query version
        QScopedPointer<QOffscreenSurface> offscreenSurface(new QOffscreenSurface);

        QSurfaceFormat format = window->format();
        if (ver.contextType == qt3ds::render::NVRenderContextValues::GLES2) {
            format.setRenderableType(QSurfaceFormat::OpenGLES);
        } else {
            format.setRenderableType(QSurfaceFormat::OpenGL);
            if (ver.major >= 2)
                format.setProfile(QSurfaceFormat::CoreProfile);
        }
        format.setMajorVersion(ver.major);
        format.setMinorVersion(ver.minor);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);

        offscreenSurface->setFormat(format);
        offscreenSurface->create();
        Q_ASSERT(offscreenSurface->isValid());

        QScopedPointer<QOpenGLContext> queryContext(new QOpenGLContext);
        queryContext->setFormat(format);
        if (queryContext->create() && compareContextVersion(format, queryContext->format())) {
            valid = true;
            result = format;
            break;
        }
    } // of version test iteration

    if (!valid)
        qFatal("Unable to select suitable OpenGL context");

    qDebug() << Q_FUNC_INFO << "selected surface format:" << result;
    QSurfaceFormat::setDefaultFormat(result);
    return result;
}

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

    setFormat(selectSurfaceFormat(this));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_previousToolMode = g_StudioApp.GetToolMode();
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
        if (((event->buttons() & Qt::LeftButton)
                || (!g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance().Valid()))
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
    g_StudioApp.setLastActiveView(this);

    long toolMode = g_StudioApp.GetToolMode();
    const Qt::MouseButton btn = event->button();
    bool toolChanged = false;

    if (!m_containerWnd->IsDeploymentView() && (event->modifiers() & Qt::AltModifier)) {
        // We are in edit camera view, so we are in Alt-click camera tool
        // controlling mode
        m_mouseDown = true;
        if (btn == Qt::MiddleButton) {
            // Alt + Wheel Click
            toolMode = STUDIO_TOOLMODE_CAMERA_PAN;
            toolChanged = true;
        } else if (btn == Qt::LeftButton) {
            // Alt + Left Click
            if (g_StudioApp.getRenderer().DoesEditCameraSupportRotation(
                        g_StudioApp.getRenderer().GetEditCamera())) {
                toolMode = STUDIO_TOOLMODE_CAMERA_ROTATE;
                toolChanged = true;
            }
        } else if (btn == Qt::RightButton) {
            // Alt + Right Click
            toolMode = STUDIO_TOOLMODE_CAMERA_ZOOM;
            toolChanged = true;
        }

        if (toolChanged) {
            g_StudioApp.SetToolMode(toolMode);
            Q_EMIT m_containerWnd->toolChanged();
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(SceneDragSenderType::Matte,
                                                                     event->pos(), toolMode);
        }
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

    if (!m_containerWnd->IsDeploymentView()) {
        // We are in edit camera view
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(SceneDragSenderType::Matte);
        g_StudioApp.GetCore()->CommitCurrentCommand();
        m_mouseDown = false;
        // Restore normal tool mode
        g_StudioApp.SetToolMode(m_previousToolMode);
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
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
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
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
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
    QSize size(width, height);
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
    theRenderer.SetViewRect(QRect(0, 0, width * fixedDevicePixelRatio(),
                                  height * fixedDevicePixelRatio()), size);
}
