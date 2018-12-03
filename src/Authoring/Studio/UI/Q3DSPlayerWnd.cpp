/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "Q3DSPlayerWnd.h"
#include "IDragable.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"
#include "Core.h"
#include "HotKeys.h"
#include "Dispatch.h"
#include "SceneDropTarget.h"
#include "Q3DStudioRenderer.h"

#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglcontext.h>
#include <QtWidgets/qmessagebox.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>

namespace Q3DStudio
{

static bool compareSurfaceFormatVersion(QSurfaceFormat a, QSurfaceFormat b)
{
    if (a.renderableType() != b.renderableType())
        return false;
    if (a.majorVersion() != b.majorVersion())
        return false;
    if (a.minorVersion() > b.minorVersion())
        return false;
    return true;
}

static QSurfaceFormat selectSurfaceFormat(QOpenGLWidget *window)
{
    struct ContextVersion {
        int major;
        int minor;
        bool gles;
    };

    ContextVersion versions[] = {
        {4, 5, false},
        {4, 4, false},
        {4, 3, false},
        {4, 2, false},
        {4, 1, false},
        {3, 3, false},
        {2, 1, false},
        {3, 2, true},
        {3, 1, true},
        {3, 0, true},
        {2, 1, true},
        {2, 0, true},
    };

    QSurfaceFormat result = window->format();
    bool valid = false;

    for (const auto &ver : versions) {
        // make an offscreen surface + context to query version
        QScopedPointer<QOffscreenSurface> offscreenSurface(new QOffscreenSurface);

        QSurfaceFormat format = window->format();
        if (ver.gles) {
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
        if (queryContext->create() && compareSurfaceFormatVersion(format, queryContext->format())) {
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

Q3DSPlayerWnd::Q3DSPlayerWnd(QWidget *parent)
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

Q3DSPlayerWnd::~Q3DSPlayerWnd()
{

}

void Q3DSPlayerWnd::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);
    update();
}

void Q3DSPlayerWnd::mouseMoveEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        sr.engine()->handleMouseMoveEvent(event);
    }

    if (m_mouseDown) {
        long theModifierKeys = 0;
        if (event->buttons() & Qt::LeftButton
                || (!g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance().Valid()
                && !m_containerWnd->IsDeploymentView())) {
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

void Q3DSPlayerWnd::mousePressEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        sr.engine()->handleMouseMoveEvent(event);
    }

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

void Q3DSPlayerWnd::mouseReleaseEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        sr.engine()->handleMouseMoveEvent(event);
    }

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

void Q3DSPlayerWnd::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        sr.engine()->handleMouseMoveEvent(event);
    }

    g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDblClick(
                SceneDragSenderType::SceneWindow, event->pos());
}

bool Q3DSPlayerWnd::OnDragWithin(CDropSource &inSource)
{
    CSceneViewDropTarget theTarget;
    return theTarget.Accept(inSource);
}

bool Q3DSPlayerWnd::OnDragReceive(CDropSource &inSource)
{
    CSceneViewDropTarget theTarget;
    Q_EMIT dropReceived();
    return theTarget.Drop(inSource);
}

void Q3DSPlayerWnd::setContainerWnd(CPlayerContainerWnd *inContainerWnd)
{
    m_containerWnd = inContainerWnd;
    updateGeometry();
}

QSize Q3DSPlayerWnd::sizeHint() const
{
    if (m_containerWnd)
        return m_containerWnd->GetEffectivePresentationSize();
    else
        return QOpenGLWidget::sizeHint();
}

void Q3DSPlayerWnd::initializeGL()
{
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
    if (!theRenderer.IsInitialized()) {
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

void Q3DSPlayerWnd::paintGL()
{
    g_StudioApp.getRenderer().RenderNow();

    Q_EMIT newFrame();
}

qreal Q3DSPlayerWnd::fixedDevicePixelRatio() const
{
    // Fix a problem on X11: https://bugreports.qt.io/browse/QTBUG-65570
    qreal ratio = devicePixelRatio();
    if (QWindow *w = window()->windowHandle()) {
        if (QScreen *s = w->screen())
            ratio = s->devicePixelRatio();
    }
    return ratio;
}

void Q3DSPlayerWnd::resizeGL(int width, int height)
{
    QSize size(width, height);
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
    theRenderer.SetViewRect(QRect(0, 0, int(width * fixedDevicePixelRatio()),
                                        int(height * fixedDevicePixelRatio())), size);
}

}
