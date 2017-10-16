/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "q3dssurfaceviewer_p.h"
#include "Qt3DSAudioPlayerImpl.h"
#include "viewerqmlstreamproxy_p.h"
#include "q3dsviewersettings_p.h"
#include "q3dspresentation_p.h"
#include "studioutils_p.h"

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qwindow.h>
#include <QtGui/QPlatformSurfaceEvent>

#include <QtCore/QFileInfo>

using namespace UICViewer;

QT_BEGIN_NAMESPACE

Q3DSSurfaceViewer::Q3DSSurfaceViewer(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSSurfaceViewerPrivate(this))
{
}

Q3DSSurfaceViewer::~Q3DSSurfaceViewer()
{
    delete d_ptr;
}

bool Q3DSSurfaceViewer::initialize(QSurface *surface, QOpenGLContext *context, GLuint fboId)
{
    return d_ptr->initialize(surface, context, fboId);
}

void Q3DSSurfaceViewer::shutdown()
{
    d_ptr->shutdown();
}

void Q3DSSurfaceViewer::reset()
{
    d_ptr->reset();
}

void Q3DSSurfaceViewer::update()
{
    d_ptr->update();
}

QImage Q3DSSurfaceViewer::grab(const QRect &rect)
{
    return d_ptr->grab(rect);
}

QSize Q3DSSurfaceViewer::size() const
{
    return d_ptr->m_size;
}

void Q3DSSurfaceViewer::setSize(const QSize &size)
{
    d_ptr->setSize(size);
}

bool Q3DSSurfaceViewer::autoSize() const
{
    return d_ptr->m_autoSize;
}

void Q3DSSurfaceViewer::setAutoSize(bool autoSize)
{
    if (d_ptr->m_autoSize != autoSize) {
        d_ptr->m_autoSize = autoSize;
        Q_EMIT autoSizeChanged(autoSize);
    }
}

int Q3DSSurfaceViewer::updateInterval() const
{
    return d_ptr->m_updateInterval;
}

void Q3DSSurfaceViewer::setUpdateInterval(int interval)
{
    d_ptr->setUpdateInterval(interval);
}

bool Q3DSSurfaceViewer::isRunning() const
{
    return d_ptr->m_viewerApp != nullptr;
}

int Q3DSSurfaceViewer::fboId() const
{
    return d_ptr->m_fboId;
}

QSurface *Q3DSSurfaceViewer::surface() const
{
    return d_ptr->m_surface;
}

QOpenGLContext *Q3DSSurfaceViewer::context() const
{
    return d_ptr->m_context;
}

Q3DSViewerSettings *Q3DSSurfaceViewer::settings() const
{
    return d_ptr->m_settings;
}

Q3DSPresentation *Q3DSSurfaceViewer::presentation() const
{
    return d_ptr->m_presentation;
}

Q3DSSurfaceViewerPrivate::Q3DSSurfaceViewerPrivate(Q3DSSurfaceViewer *q)
    : QObject(q)
    , q_ptr(q)
    , m_viewerApp(nullptr)
    , m_timer(nullptr)
    , m_updateInterval(-1)
    , m_pixelRatio(1.0)
    , m_fboId(0)
    , m_surface(nullptr)
    , m_context(nullptr)
    , m_autoSize(true)
    , m_settings(new Q3DSViewerSettings(this))
    , m_presentation(new Q3DSPresentation(this))
{
    connect(m_presentation, &Q3DSPresentation::sourceChanged,
            this, &Q3DSSurfaceViewerPrivate::reset);
}

Q3DSSurfaceViewerPrivate::~Q3DSSurfaceViewerPrivate()
{
    releaseRuntime();

    delete m_timer;
}

void Q3DSSurfaceViewerPrivate::reset()
{
    if (m_viewerApp) {
        releaseRuntime();
        initializeRuntime();
    }
}

void Q3DSSurfaceViewerPrivate::setSize(const QSize &size)
{
    if (m_size != size) {
        m_size = size;

        if (m_viewerApp) {
            m_context->makeCurrent(m_surface);
            m_viewerApp->Resize(int(m_size.width() * m_pixelRatio),
                                int(m_size.height() * m_pixelRatio));
        }

        Q_EMIT q_ptr->sizeChanged(m_size);
    }
}

void Q3DSSurfaceViewerPrivate::setUpdateInterval(int interval)
{
    if (m_updateInterval != interval) {
        m_updateInterval = interval;
        resetUpdateTimer();
        Q_EMIT q_ptr->updateIntervalChanged(m_updateInterval);
    }
}

bool Q3DSSurfaceViewerPrivate::initialize(QSurface *surface, QOpenGLContext *context, GLuint fboId)
{
    Q_ASSERT(context);
    Q_ASSERT(surface);

    if (m_presentation->source().isEmpty()) {
        qWarning("Failed to initialize Q3DSSurfaceViewer,"
                 " presentation source must be set before calling initialize()");
        return false;
    }

    QFileInfo info(Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source()));
    if (!info.exists()) {
        qWarning() << "Failed to initialize Q3DSSurfaceViewer, the presentation doesn't exist:"
                   << m_presentation->source().toString();
        return false;
    }

    shutdown();

    m_surface = surface;
    m_context = context;
    m_fboId = fboId;
    if (m_surface->surfaceClass() == QSurface::Window && fboId == 0)
        m_pixelRatio = static_cast<QWindow *>(m_surface)->devicePixelRatio();

    surfaceObject()->installEventFilter(this);

    connect(context, &QOpenGLContext::aboutToBeDestroyed, this, &Q3DSSurfaceViewerPrivate::shutdown);

    bool success = initializeRuntime();

    if (success)
        Q_EMIT q_ptr->runningChanged(true);

    return success;
}


void Q3DSSurfaceViewerPrivate::shutdown()
{
    bool oldInitialized = (m_viewerApp != nullptr);

    if (m_context) {
        disconnect(m_context, &QOpenGLContext::aboutToBeDestroyed,
                   this, &Q3DSSurfaceViewerPrivate::shutdown);
    }

    if (m_surface)
        surfaceObject()->removeEventFilter(this);

    releaseRuntime();

    m_surface = nullptr;
    m_context = nullptr;
    m_fboId = 0;

    if (oldInitialized)
        Q_EMIT q_ptr->runningChanged(false);
}


void Q3DSSurfaceViewerPrivate::update()
{
    if (m_viewerApp && m_viewerApp->IsInitialised()) {
        if (m_surface->surfaceClass() != QSurface::Window
                || static_cast<QWindow *>(m_surface)->isExposed()) {
            m_context->makeCurrent(m_surface);
            if (m_autoSize)
                setSize(m_surface->size());
            m_viewerApp->Render();

            if (m_fboId == 0)
                m_context->swapBuffers(m_surface);
        }
    }
}

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format,
                                                  bool include_alpha);

QImage Q3DSSurfaceViewerPrivate::grab(const QRect &rect)
{
    QRect captureRect;
    QSize fullSize = m_size * m_pixelRatio;
    if (rect.isValid()) {
        captureRect = QRect(rect.x() * m_pixelRatio, rect.y() * m_pixelRatio,
                            rect.width() * m_pixelRatio, rect.height() * m_pixelRatio);
    } else {
        captureRect = QRect(0, 0, fullSize.width(), fullSize.height());
    }
    QImage image(captureRect.size(), QImage::Format_ARGB32);

    if (m_surface && m_context && m_viewerApp && m_viewerApp->IsInitialised()
            && (m_surface->surfaceClass() != QSurface::Window
                || static_cast<QWindow *>(m_surface)->isExposed())) {
        m_context->makeCurrent(m_surface);

        // Render now to ensure the image is up to date and will actually exist in case
        // the surface has a non-preserved swap buffer
        if (m_autoSize)
            setSize(m_surface->size());
        m_viewerApp->Render();

        m_context->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        QImage fullGrab = qt_gl_read_framebuffer(fullSize, false, false);

        // Also update the screen to match the grab, since we just rendered
        if (m_fboId == 0)
            m_context->swapBuffers(m_surface);

        if (captureRect.size() == fullSize)
            image = fullGrab;
        else
            image = fullGrab.copy(captureRect);
    }

    return image;
}

bool Q3DSSurfaceViewerPrivate::eventFilter(QObject *obj, QEvent *e)
{
    if (m_surface && e->type() == QEvent::PlatformSurface) {
        if (surfaceObject() == obj) {
            QPlatformSurfaceEvent *ev = static_cast<QPlatformSurfaceEvent *>(e);
            if (ev->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
                shutdown();
        }
    }
    return QObject::eventFilter(obj, e);
}

bool Q3DSSurfaceViewerPrivate::initializeRuntime()
{
    Q_ASSERT(!m_viewerApp);

    m_context->makeCurrent(m_surface);

    m_viewerApp = &UICViewerApp::Create(m_context, new CUICAudioPlayerImpl());

    Q_ASSERT(m_viewerApp);

    const QString localSource = Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source());

    if (m_autoSize)
        m_size = m_surface->size();

    if (!m_viewerApp->InitializeApp(int(m_size.width() * m_pixelRatio),
                                    int(m_size.height() * m_pixelRatio),
                                    m_context->format(), m_fboId, localSource,
                                    m_presentation->d_ptr->streamProxy())) {
        releaseRuntime();
        qWarning("Failed to initialize runtime");
        return false;
    }

    m_settings->d_ptr->setViewerApp(m_viewerApp);
    m_presentation->d_ptr->setViewerApp(m_viewerApp);

    resetUpdateTimer();

    return true;
}

void Q3DSSurfaceViewerPrivate::releaseRuntime()
{
    m_settings->d_ptr->setViewerApp(nullptr);
    m_presentation->d_ptr->setViewerApp(nullptr);

    if (m_context && m_surface)
        m_context->makeCurrent(m_surface);

    if (m_viewerApp) {
        m_viewerApp->Release();
        m_viewerApp = nullptr;
    }

    resetUpdateTimer();
}

void Q3DSSurfaceViewerPrivate::resetUpdateTimer()
{
    if (m_viewerApp && m_updateInterval >= 0) {
        if (!m_timer) {
            m_timer = new QTimer();
            connect(m_timer, &QTimer::timeout, this, &Q3DSSurfaceViewerPrivate::update);
        }
        m_timer->start(m_updateInterval);
    } else if (m_timer) {
        m_timer->stop();
    }
}

QObject *Q3DSSurfaceViewerPrivate::surfaceObject()
{
    if (m_surface) {
        if (m_surface->surfaceClass() == QSurface::Window)
            return static_cast<QWindow *>(m_surface);
        else
            return static_cast<QOffscreenSurface *>(m_surface);
    }
    return nullptr;
}

QT_END_NAMESPACE
