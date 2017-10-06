/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#include "Qt3DSView.h"
#include "Qt3DSRenderer.h"
#include "q3dspresentationitem.h"

#include <QtStudio3D/private/q3dsviewersettings_p.h>
#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtStudio3D/private/q3dssceneelement_p.h>
#include <QtStudio3D/private/viewerqmlstreamproxy_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfileinfo.h>
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

Q3DSView::Q3DSView()
    : m_viewerSettings(nullptr)
    , m_presentation(nullptr)
    , m_emitRunningChange(false)
    , m_isRunning(false)
    , m_ignoreMouseEvents(false)
    , m_ignoreWheelEvents(false)
    , m_ignoreKeyboardEvents(false)
    , m_pixelRatio(1.0)
{
    setMirrorVertically(true);
    connect(this, &Q3DSView::windowChanged, this, &Q3DSView::handleWindowChanged);
    connect(this, &Q3DSView::visibleChanged, this, &Q3DSView::handleVisibleChanged);

    setIgnoreEvents(false, false, false);
}

Q3DSView::~Q3DSView()
{
}

Q3DSPresentationItem *Q3DSView::presentation() const
{
    return m_presentation;
}

Q3DSViewerSettings *Q3DSView::viewerSettings() const
{
    return m_viewerSettings;
}

void Q3DSView::setIgnoreEvents(bool mouse, bool wheel, bool keyboard)
{
    // TODO: It might be beneficial to have these as properties so they could be acceessed from QML
    m_ignoreMouseEvents = mouse;
    m_ignoreWheelEvents = wheel;
    m_ignoreKeyboardEvents = keyboard;

    if (mouse)
        setAcceptedMouseButtons(Qt::NoButton);
    else
        setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setAcceptHoverEvents(!mouse);
}

void Q3DSView::componentComplete()
{
    const auto childObjs = children();
    for (QObject *child : childObjs) {
        auto settings = qobject_cast<Q3DSViewerSettings *>(child);
        if (settings) {
            if (m_viewerSettings)
                qWarning("Duplicate ViewerSettings defined for Studio3D.");
            else
                m_viewerSettings = settings;
        }
        auto presentation = qobject_cast<Q3DSPresentationItem *>(child);
        if (presentation) {
            if (m_presentation)
                qWarning("Duplicate Presentation defined for Studio3D.");
            else
                m_presentation = presentation;
        }
    }

    if (!m_viewerSettings)
        m_viewerSettings = new Q3DSViewerSettings(this);
    if (!m_presentation)
        m_presentation = new Q3DSPresentationItem(this);

    m_viewerSettings->d_ptr->setCommandQueue(&m_pendingCommands);
    m_presentation->d_ptr->setCommandQueue(&m_pendingCommands);

    QQuickFramebufferObject::componentComplete();
}

void Q3DSView::handleWindowChanged(QQuickWindow *window)
{
    if (!window)
        return;

    window->setClearBeforeRendering(false);
    m_pixelRatio = window->devicePixelRatio();

    // Call tick every frame of the GUI thread to notify QML about new frame via frameUpdate signal
    connect(window, &QQuickWindow::afterAnimating, this, &Q3DSView::tick);
    // Call update after the frame is handled to queue another frame
    connect(window, &QQuickWindow::afterSynchronizing, this, &Q3DSView::update);
}

// Queue up a command to inform the renderer of a newly-changed visible/hidden status.
void Q3DSView::handleVisibleChanged()
{
    m_pendingCommands.m_visibleChanged = true;
    m_pendingCommands.m_visible = isVisible();
}

void Q3DSView::reset()
{
    // Fake a source change to trigger a reloading of the presentation
    m_pendingCommands.m_sourceChanged = true;
    m_pendingCommands.m_source = m_presentation->source();
}

void Q3DSView::requestResponseHandler(const QString &elementPath, CommandType commandType,
                                      void *requestData)
{
    switch (commandType) {
    case CommandType_RequestSlideInfo: {
        Q3DSSceneElement *handler = qobject_cast<Q3DSSceneElement *>(
                    m_presentation->registeredElement(elementPath));
        if (handler)
            handler->d_ptr->requestResponseHandler(commandType, requestData);
        else
            qWarning() << __FUNCTION__ << "RequestSlideInfo response got for unregistered scene.";
        break;
    }
    default:
        qWarning() << __FUNCTION__ << "Unknown command type.";
        break;
    }
}

// Create the Q3DSRenderer. Invoked automatically by the QML scene graph.
QQuickFramebufferObject::Renderer *Q3DSView::createRenderer() const
{
    // It is "illegal" to create a connection between the renderer
    // and the plugin, and vice-versa. The only valid time the two
    // may communicate is during Q3DSRenderer::synchronize().
    Q3DSRenderer *renderer = new Q3DSRenderer(isVisible(), m_presentation->d_ptr->streamProxy());

    connect(renderer, &Q3DSRenderer::enterSlide,
            m_presentation->d_ptr, &Q3DSPresentationPrivate::handleSlideEntered);
    connect(renderer, &Q3DSRenderer::exitSlide,
            m_presentation, &Q3DSPresentation::slideExited);
    connect(renderer, &Q3DSRenderer::requestResponse,
            this, &Q3DSView::requestResponseHandler);
    return renderer;
}

bool Q3DSView::isRunning() const
{
    return m_isRunning;
}

/** Emit QML `runningChanged` and `frameUpdate` and signals.
 *  This method is called every frame, and emits the `frameUpdate` signal every frame,
 *  regardless of plugin visibility. This allows a hidden Qt3DSView to still process
 *  information every frame, even though the Renderer is not rendering.
 *
 *  To prevent expensive onFrameUpdate handlers from being processed when hidden,
 *  add an early return to the top like:
 *
 *      onFrameUpdate: {
 *          if (!visible) return;
 *          ...
 *      }
 */
void Q3DSView::tick()
{
    if (m_emitRunningChange) {
        Q_EMIT runningChanged(true);
        m_emitRunningChange = false;
        m_isRunning = true;
    }

    // Don't call onFrameUpdate until after onInitialize has been called
    if (m_isRunning) {
        // Give QML an opportunity to change Qt3DS values every frame
        Q_EMIT frameUpdate();
    }
}

// Copies the list of commands previously queued up. Called by Q3DSRenderer::synchronize().
void Q3DSView::getCommands(bool emitInitialize, CommandQueue &renderQueue)
{
    if (emitInitialize)
        m_emitRunningChange = true;

    renderQueue.copyCommands(m_pendingCommands);
    m_pendingCommands.clear();
}

void Q3DSView::mousePressEvent(QMouseEvent *event)
{
    if (!m_ignoreMouseEvents) {
        if (m_pixelRatio != 1.0) {
            QMouseEvent scaledEvent(event->type(), event->pos() * m_pixelRatio,
                                    event->button(), event->buttons(), event->modifiers());
            m_presentation->mousePressEvent(&scaledEvent);
        } else {
            m_presentation->mousePressEvent(event);
        }
    }
}

void Q3DSView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_ignoreMouseEvents) {
        if (m_pixelRatio != 1.0) {
            QMouseEvent scaledEvent(event->type(), event->pos() * m_pixelRatio,
                                    event->button(), event->buttons(), event->modifiers());
            m_presentation->mouseReleaseEvent(&scaledEvent);
        } else {
            m_presentation->mouseReleaseEvent(event);
        }
    }
}

void Q3DSView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_ignoreMouseEvents) {
        if (m_pixelRatio != 1.0) {
            QMouseEvent scaledEvent(event->type(), event->pos() * m_pixelRatio,
                                    event->button(), event->buttons(), event->modifiers());
            m_presentation->mouseMoveEvent(&scaledEvent);
        } else {
            m_presentation->mouseMoveEvent(event);
        }
    }
}

void Q3DSView::wheelEvent(QWheelEvent *event)
{
    if (!m_ignoreWheelEvents)
        m_presentation->wheelEvent(event);
}

void Q3DSView::keyPressEvent(QKeyEvent *event)
{
    if (m_ignoreKeyboardEvents)
        return;
    m_presentation->keyPressEvent(event);
}

void Q3DSView::keyReleaseEvent(QKeyEvent *event)
{
    if (m_ignoreKeyboardEvents)
        return;
    if (!event->isAutoRepeat())
        m_presentation->keyReleaseEvent(event);
}

QT_END_NAMESPACE
