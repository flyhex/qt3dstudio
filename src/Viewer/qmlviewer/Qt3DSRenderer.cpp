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

#include "Qt3DSRenderer.h"
#include "UICViewerApp.h"
#include "UICAudioPlayerImpl.h"

#include <QtStudio3D/private/q3dscommandqueue_p.h>
#include <QtStudio3D/private/q3dsviewersettings_p.h>
#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtStudio3D/private/studioutils_p.h>

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopenglcontext.h>
#include <QtQuick/qquickwindow.h>

using namespace UICViewer;

QT_BEGIN_NAMESPACE

Q3DSRenderer::Q3DSRenderer(bool visibleFlag, uic::UICAssetVisitor *assetVisitor)
    : m_visibleFlag(visibleFlag)
    , m_initElements(false)
    , m_runtime(0)
    , m_window(nullptr)
    , m_initialized(false)
    , m_visitor(assetVisitor)
    , m_settings(new Q3DSViewerSettings(this))
    , m_presentation(new Q3DSPresentation(this))
{
}

Q3DSRenderer::~Q3DSRenderer()
{
    releaseRuntime();
}

QOpenGLFramebufferObject *Q3DSRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat theFormat;
    theFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QOpenGLFramebufferObject *frameBuffer =
            new QOpenGLFramebufferObject(size, theFormat);
    if (m_runtime && m_runtime->IsInitialised())
        m_runtime->setOffscreenId(frameBuffer->handle());
    return frameBuffer;
}

/** Pull pending commands from the plugin.
 *  Invoked automatically by the QML scene graph.
 *
 *  This is the only place where it is valid for the Q3DSView plugin and render to communicate.
 */
void Q3DSRenderer::synchronize(QQuickFramebufferObject *inView)
{
    // Passing m_InitElements here is a bit of a hack to easily set the flag on the plugin.
    static_cast<Q3DSView *>(inView)->getCommands(m_initElements, m_commands);

    if (m_commands.m_sourceChanged) {
        releaseRuntime();
        // Need to update source here rather than processCommands, as source is needed for init
        m_presentation->setSource(m_commands.m_source);
        m_initialized = false;
    }

    m_initElements = false;

    // We need a handle to the window to be able to reset the GL state inside of Draw().
    // See https://bugreports.qt.io/browse/QTBUG-47213
    if (!m_window)
        m_window = inView->window();
}

void Q3DSRenderer::releaseRuntime()
{
    m_settings->d_ptr->setViewerApp(nullptr);
    m_presentation->d_ptr->setViewerApp(nullptr);

    if (m_runtime) {
        m_runtime->Release();
        m_runtime = nullptr;
    }
}

/** Invoked by the QML scene graph indicating that it's time to render.
 *  Calls `draw()` if the plugin is visible, or `processCommands()` otherwise.
 *
 *  Note that this will still render if the plugin is opacity:0. To avoid this,
 *  add a line to your QML to hide the object when opacity is zero, like:
 *
 *      visible: opacity != 0
 */
void Q3DSRenderer::render()
{
    // We may start in a non visible state but we still need
    // to init the runtime otherwise the commands are never processed
    if (!m_initialized)
        m_initialized = initializeRuntime(this->framebufferObject());

    // Don't render if the plugin is hidden; however, if hidden, but sure
    // to process pending commands so we can be shown again.
    if (m_visibleFlag)
        draw();
    else
        processCommands();
}

/** Cause Qt3DS runtime to render content.
 *  Initializes GL and the runtime when called the first time.
 */
void Q3DSRenderer::draw()
{
    if (m_runtime && m_runtime->IsInitialised() && m_window) {
        if (m_initialized)
            m_runtime->RestoreState();
        m_runtime->Render();
        m_runtime->SaveState();

        m_window->resetOpenGLState();
    }
}

bool Q3DSRenderer::initializeRuntime(QOpenGLFramebufferObject *inFbo)
{
    m_runtime = &UICViewerApp::Create(nullptr, new CUICAudioPlayerImpl());
    Q_ASSERT(m_runtime);

    int theWidth = inFbo->width();
    int theHeight = inFbo->height();

    const QString localSource = Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source());

    if (!m_runtime->InitializeApp(theWidth, theHeight, QOpenGLContext::currentContext()->format(),
                                  inFbo->handle(), localSource, m_visitor)) {
        releaseRuntime();
        return false;
    }

    m_runtime->RegisterScriptCallback(UICViewer::ViewerCallbackType::Enum::CALLBACK_ON_INIT,
                                      reinterpret_cast<qml_Function>(Q3DSRenderer::onInitHandler),
                                      this);
    m_runtime->RegisterScriptCallback(UICViewer::ViewerCallbackType::Enum::CALLBACK_ON_UPDATE,
                                      reinterpret_cast<qml_Function>(Q3DSRenderer::onUpdateHandler),
                                      this);

    m_settings->d_ptr->setViewerApp(m_runtime);
    m_presentation->d_ptr->setViewerApp(m_runtime, false);

    // Connect signals
    connect(m_runtime, &UICViewer::UICViewerApp::SigSlideEntered,
            this, &Q3DSRenderer::enterSlide);
    connect(m_runtime, &UICViewer::UICViewerApp::SigSlideExited,
            this, &Q3DSRenderer::exitSlide);

    return true;
}

/** Accept user commands (e.g. setAttribute) needed to initialize the code.
 *
 *  If we attempt to run Qt3DS methods like setAttribute() before the runtime
 *  has been initialized, they will be lost. This method is the correct place
 *  to accept user commands.
 *
 *  Currently this method just sets a flag needed to pass a flag to the
 *  plugin next time syncronize() is called, which eventually gets the plugin
 *  to emit an `runningChanged` signal the next time `tick()` is called.
 *  As a result, code specified in an `onRunningChanged` handler may be run
 *  after one or more frames have already rendered.
 */
void Q3DSRenderer::onInitHandler(void *userData)
{
    Q3DSRenderer *theRenderer = static_cast<Q3DSRenderer *>(userData);
    theRenderer->setInitElements(true);
}

/** Accept the latest pending user commands (e.g. setAttribute).
 *
 *  This method just calls `ProcessCommands` to avoid unnecessary
 *  pointer dereferencing and accessor methods (or public member variables).
 */
void Q3DSRenderer::onUpdateHandler(void *userData)
{
    Q3DSRenderer *theRenderer = static_cast<Q3DSRenderer *>(userData);
    theRenderer->processCommands();
}

/** Apply commands queued up by the user (e.g. setAttribute).
 *
 *  Note that these commands are executed even if the plugin is not visible,
 *  in part to allow changes to the visible flag to be noticed, but also
 *  to allow specialty code to continue to be queued up even when not rendering.
 */
void Q3DSRenderer::processCommands()
{
    if (!m_runtime)
        return;

    if (m_commands.m_visibleChanged)
        m_visibleFlag = m_commands.m_visible;

    if (QOpenGLFramebufferObject *inFbo = this->framebufferObject()) {
        if (inFbo->isValid() && (inFbo->width() != m_runtime->GetWindowWidth()
                                 || inFbo->height() != m_runtime->GetWindowHeight())) {
            m_runtime->Resize(inFbo->width(), inFbo->height());
        }
    }

    if (m_commands.m_scaleModeChanged)
        m_settings->setScaleMode(m_commands.m_scaleMode);
    if (m_commands.m_shadeModeChanged)
        m_settings->setShadeMode(m_commands.m_shadeMode);
    if (m_commands.m_matteColorChanged)
        m_settings->setMatteColor(m_commands.m_matteColor);
    if (m_commands.m_showRenderStatsChanged)
        m_settings->setShowRenderStats(m_commands.m_showRenderStats);

    if (m_commands.m_globalAnimationTimeChanged)
        m_presentation->setGlobalAnimationTime(m_commands.m_globalAnimationTime);

    // Send scene graph changes over to Q3DS
    for (int i = 0; i < m_commands.size(); i++) {
        const ElementCommand &cmd = m_commands.commandAt(i);
        switch (cmd.m_commandType) {
        case CommandType_SetAttribute:
            m_presentation->setAttribute(cmd.m_elementPath, cmd.m_stringValue, cmd.m_variantValue);
            break;
        case CommandType_SetPresentationActive:
            m_presentation->setPresentationActive(cmd.m_elementPath, cmd.m_boolValue);
            break;
        case CommandType_GoToTime:
            m_presentation->goToTime(cmd.m_elementPath, cmd.m_floatValue);
            break;
        case CommandType_GoToSlide:
            m_presentation->goToSlide(cmd.m_elementPath, cmd.m_intValues[0]);
            break;
        case CommandType_GoToSlideByName:
            m_presentation->goToSlide(cmd.m_elementPath, cmd.m_stringValue);
            break;
        case CommandType_GoToSlideRelative:
            m_presentation->goToSlide(cmd.m_elementPath, bool(cmd.m_intValues[0]),
                    bool(cmd.m_intValues[1]));
            break;
        case CommandType_FireEvent:
            m_presentation->fireEvent(cmd.m_elementPath, cmd.m_stringValue);
            break;
        case CommandType_MousePress:
            m_runtime->HandleMousePress(cmd.m_intValues[0],
                    cmd.m_intValues[1], cmd.m_intValues[2], true);
            break;
        case CommandType_MouseRelease:
            m_runtime->HandleMousePress(cmd.m_intValues[0],
                    cmd.m_intValues[1], cmd.m_intValues[2], false);
            break;
        case CommandType_MouseMove:
            m_runtime->HandleMouseMove(cmd.m_intValues[0], cmd.m_intValues[1], true);
            break;
        case CommandType_MouseWheel:
            m_runtime->HandleMouseWheel(cmd.m_intValues[0], cmd.m_intValues[1],
                    bool(cmd.m_intValues[2]), cmd.m_intValues[3]);
            break;
        case CommandType_KeyPress:
            m_runtime->HandleKeyInput(Q3DStudio::EKeyCode(cmd.m_intValues[0]), true);
            break;
        case CommandType_KeyRelease:
            m_runtime->HandleKeyInput(Q3DStudio::EKeyCode(cmd.m_intValues[0]), false);
            break;
        case CommandType_RequestSlideInfo: {
            int current = 0;
            int previous = 0;
            QString currentName;
            QString previousName;
            const QByteArray path(cmd.m_elementPath.toUtf8());
            m_runtime->GetSlideInfo(path, current, previous, currentName, previousName);
            QVariantList *requestData = new QVariantList();
            requestData->append(QVariant(current));
            requestData->append(QVariant(previous));
            requestData->append(QVariant(currentName));
            requestData->append(QVariant(previousName));

            Q_EMIT requestResponse(cmd.m_elementPath, cmd.m_commandType, requestData);

            break;
        }
        default:
            qWarning() << __FUNCTION__ << "Unrecognized CommandType in command list!";
        }
    }
}

QT_END_NAMESPACE
