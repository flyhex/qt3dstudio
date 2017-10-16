/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#include "q3dswidget_p.h"
#include "Qt3DSAudioPlayerImpl.h"
#include "viewerqmlstreamproxy_p.h"
#include "q3dsviewersettings_p.h"
#include "q3dspresentation_p.h"
#include "studioutils_p.h"

#include <QtGui/qevent.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/QWindow>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdebug.h>

using namespace UICViewer;

QT_BEGIN_NAMESPACE

typedef void (QWidget::*QWidgetVoidSlot)();

Q3DSWidget::Q3DSWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new Q3DSWidgetPrivate(this))
{
    // Get keyboard handling
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

Q3DSWidget::~Q3DSWidget()
{
    delete d_ptr;
}

bool Q3DSWidget::initialize()
{
    return d_ptr->initialize();
}

int Q3DSWidget::updateInterval() const
{
    return d_ptr->m_updateInterval;
}

bool Q3DSWidget::isRunning() const
{
    return d_ptr->m_viewerApp != nullptr;
}

Q3DSViewerSettings *Q3DSWidget::settings() const
{
    return d_ptr->settings();
}

Q3DSPresentation *Q3DSWidget::presentation() const
{
    return d_ptr->presentation();
}

void Q3DSWidget::initializeGL()
{
    // Find the native window to determine pixel ratio
    QWidget *widget = this;
    QWindow *window = windowHandle();
    while (widget && window == nullptr) {
        widget = qobject_cast<QWidget *>(widget->parent());
        window = widget->windowHandle();
    }
    if (window)
        d_ptr->m_pixelRatio = window->devicePixelRatio();
}

void Q3DSWidget::resizeGL(int w, int h)
{
    if (d_ptr->m_viewerApp)
        d_ptr->m_viewerApp->Resize(int(w * d_ptr->m_pixelRatio), int(h * d_ptr->m_pixelRatio));
}

void Q3DSWidget::paintGL()
{
    if (d_ptr->m_viewerApp && d_ptr->m_viewerApp->IsInitialised())
        d_ptr->m_viewerApp->Render();
}

void Q3DSWidget::setUpdateInterval(int interval)
{
    d_ptr->setUpdateInterval(interval);
}

void Q3DSWidget::shutdown()
{
    d_ptr->shutdown();
}

void Q3DSWidget::reset()
{
    d_ptr->reset();
}

void Q3DSWidget::mousePressEvent(QMouseEvent *event)
{
    if (d_ptr->m_pixelRatio != 1.0) {
        QMouseEvent scaledEvent(event->type(), event->pos() * d_ptr->m_pixelRatio,
                                event->button(), event->buttons(), event->modifiers());
        d_ptr->m_presentation->mousePressEvent(&scaledEvent);
    } else {
        d_ptr->m_presentation->mousePressEvent(event);
    }
}

void Q3DSWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (d_ptr->m_pixelRatio != 1.0) {
        QMouseEvent scaledEvent(event->type(), event->pos() * d_ptr->m_pixelRatio,
                                event->button(), event->buttons(), event->modifiers());
        d_ptr->m_presentation->mouseReleaseEvent(&scaledEvent);
    } else {
        d_ptr->m_presentation->mouseReleaseEvent(event);
    }
}

void Q3DSWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (d_ptr->m_pixelRatio != 1.0) {
        QMouseEvent scaledEvent(event->type(), event->pos() * d_ptr->m_pixelRatio,
                                event->button(), event->buttons(), event->modifiers());
        d_ptr->m_presentation->mouseMoveEvent(&scaledEvent);
    } else {
        d_ptr->m_presentation->mouseMoveEvent(event);
    }
}

void Q3DSWidget::wheelEvent(QWheelEvent *event)
{
    d_ptr->m_presentation->wheelEvent(event);
}

void Q3DSWidget::keyPressEvent(QKeyEvent *event)
{
    d_ptr->m_presentation->keyPressEvent(event);
}

void Q3DSWidget::keyReleaseEvent(QKeyEvent *event)
{
    d_ptr->m_presentation->keyReleaseEvent(event);
}

Q3DSWidgetPrivate::Q3DSWidgetPrivate(Q3DSWidget *q)
    : QObject(q)
  , q_ptr(q)
  , m_viewerApp(nullptr)
  , m_timer(nullptr)
  , m_updateInterval(-1)
  , m_pixelRatio(1.0)
  , m_settings(new Q3DSViewerSettings(this))
  , m_presentation(new Q3DSPresentation(this))
{
    connect(m_presentation, &Q3DSPresentation::sourceChanged, this, &Q3DSWidgetPrivate::reset);
}

Q3DSWidgetPrivate::~Q3DSWidgetPrivate()
{
    releaseRuntime();

    delete m_timer;
}

void Q3DSWidgetPrivate::reset()
{
    if (m_viewerApp) {
        releaseRuntime();
        initializeRuntime();
    }
}

void Q3DSWidgetPrivate::setUpdateInterval(int interval)
{
    if (m_updateInterval != interval) {
        m_updateInterval = interval;
        resetUpdateTimer();
        Q_EMIT q_ptr->updateIntervalChanged(m_updateInterval);
    }
}

bool Q3DSWidgetPrivate::initialize()
{
    Q_ASSERT(q_ptr->context());

    if (m_presentation->source().isEmpty()) {
        qWarning("Failed to initialize Q3DSWidget,"
                 " presentation source must be set before calling initialize()");
        return false;
    }

    QFileInfo info(Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source()));
    if (!info.exists()) {
        qWarning() << "Failed to initialize Q3DSWidget, the presentation doesn't exist:"
                   << m_presentation->source().toString();
        return false;
    }

    shutdown();

    connect(q_ptr->context(), &QOpenGLContext::aboutToBeDestroyed,
            this, &Q3DSWidgetPrivate::shutdown);

    bool success = initializeRuntime();

    if (success)
        Q_EMIT q_ptr->runningChanged(true);

    return success;
}

Q3DSViewerSettings *Q3DSWidgetPrivate::settings()
{
    return m_settings;
}

Q3DSPresentation *Q3DSWidgetPrivate::presentation()
{
    return m_presentation;
}

void Q3DSWidgetPrivate::shutdown()
{
    bool oldInitialized = (m_viewerApp != nullptr);

    if (q_ptr->context()) {
        disconnect(q_ptr->context(), &QOpenGLContext::aboutToBeDestroyed,
                   this, &Q3DSWidgetPrivate::shutdown);
    }

    releaseRuntime();

    if (oldInitialized)
        Q_EMIT q_ptr->runningChanged(false);
}

bool Q3DSWidgetPrivate::initializeRuntime()
{
    Q_ASSERT(!m_viewerApp);

    q_ptr->makeCurrent();

    m_viewerApp = &UICViewerApp::Create(q_ptr->context(), new CUICAudioPlayerImpl());

    Q_ASSERT(m_viewerApp);

    const QString localSource = Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source());

    if (!m_viewerApp->InitializeApp(int(q_ptr->width() * m_pixelRatio),
                                    int(q_ptr->height() * m_pixelRatio),
                                    q_ptr->context()->format(),
                                    q_ptr->defaultFramebufferObject(), localSource,
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

void Q3DSWidgetPrivate::releaseRuntime()
{
    m_settings->d_ptr->setViewerApp(nullptr);
    m_presentation->d_ptr->setViewerApp(nullptr);

    if (m_viewerApp) {
        q_ptr->makeCurrent();
        m_viewerApp->Release();
        m_viewerApp = nullptr;
    }

    resetUpdateTimer();
}

void Q3DSWidgetPrivate::resetUpdateTimer()
{
    if (m_viewerApp && m_updateInterval >= 0) {
        if (!m_timer) {
            m_timer = new QTimer();
            connect(m_timer, &QTimer::timeout,
                    q_ptr, static_cast<QWidgetVoidSlot>(&QWidget::update));
        }
        m_timer->start(m_updateInterval);
    } else if (m_timer) {
        m_timer->stop();
    }
}

QT_END_NAMESPACE
