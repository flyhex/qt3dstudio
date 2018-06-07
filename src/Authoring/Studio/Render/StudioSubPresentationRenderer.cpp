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

#include "StudioSubPresentationRenderer.h"
#include "render/Qt3DSRenderContext.h"
#include "q3dspresentation.h"
#include "q3dsviewersettings.h"

#include <QtCore/qfileinfo.h>
#include <qxmlstream.h>

using namespace qt3ds::render;

class RendererThread : public QThread
{
public:
    RendererThread(QThread *mainThread)
        : m_running(false)
        , m_updated(false)
        , m_initialized(false)
        , m_mainThread(mainThread)
        , m_semaphore(0)
    {

    }

    QSize readPresentationSize(const QString &url)
    {
        QFile file(url);
        file.open(QFile::Text | QFile::ReadOnly);
        if (!file.isOpen()) {
            qWarning () << file.errorString();
            return QSize();
        }
        QXmlStreamReader reader(&file);
        reader.setNamespaceProcessing(false);

        if (reader.readNextStartElement() && reader.qualifiedName() == QLatin1String("UIP")) {
            if (reader.readNextStartElement()
                    && reader.qualifiedName() == QLatin1String("Project")) {
                if (reader.readNextStartElement()
                        && reader.qualifiedName() == QLatin1String("ProjectSettings")) {
                    const auto attrib = reader.attributes();
                    return QSize(attrib.value(QLatin1String("presentationWidth")).toInt(),
                                 attrib.value(QLatin1String("presentationHeight")).toInt());
                }
            }
        }
        return QSize();
    }

    void initialize(const QString &presentation, const QString &path)
    {
        m_path = path;
        m_presentation = presentation;

        m_surfaceViewer.reset(new Q3DSSurfaceViewer);
        m_context.reset(new QT_PREPEND_NAMESPACE(QOpenGLContext));
        m_surface.reset(new QOffscreenSurface);

        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        if (QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext())
            format = QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext()->format();
        m_context->setShareContext(QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext());
        m_context->setFormat(format);
        m_surface->setFormat(format);
        m_context->create();
        m_surface->create();
        m_context->moveToThread(this);
        m_surface->moveToThread(this);
        m_surfaceViewer->moveToThread(this);
#ifdef Q3DS_PREVIEW_SUBPRESENTATION_RT2
        QObject::connect(m_surfaceViewer.data(), &Q3DSSurfaceViewer::frameUpdate,
                         this, &RendererThread::frameRendered);
#else
        QObject::connect(m_surfaceViewer.data(), &Q3DSSurfaceViewer::frameUpdated,
                         this, &RendererThread::frameRendered);
#endif
        m_initialized = true;
    }

    void run() override
    {
        QFileInfo info(m_path + "/" + m_presentation);
        m_size = readPresentationSize(m_path + "/" + m_presentation);

        m_context->makeCurrent(m_surface.data());

        m_fbo.reset(new QOpenGLFramebufferObject(m_size,
                                                 QOpenGLFramebufferObject::CombinedDepthStencil));

        m_surfaceViewer->setSize(m_size);
        m_surfaceViewer->setAutoSize(false);
        m_surfaceViewer->setUpdateInterval(-1);
        m_surfaceViewer->presentation()->setSource(QUrl::fromLocalFile(info.absoluteFilePath()));
        m_surfaceViewer->settings()->setMatteColor(Qt::transparent);
#ifdef Q3DS_PREVIEW_SUBPRESENTATION_RT2
        m_surfaceViewer->create(m_surface.data(), m_context.data(), m_fbo->handle());
#else
        m_surfaceViewer->initialize(m_surface.data(), m_context.data(), m_fbo->handle());
#endif
        m_running = true;

        m_context->doneCurrent();

        while (true) {
            QMutexLocker lock(&m_mutex);
            if (!m_running)
                break;
            m_context->makeCurrent(m_surface.data());
            if (!m_surfaceViewer->isRunning()) {
                qWarning () << "Subpresentation stopped running. Stopping updating.";
                break;
            }

            m_surfaceViewer->update();

            m_updated = true;
            m_context->doneCurrent();
            lock.unlock();

            // Update approx 30 fps
            QThread::usleep(33000);
        }
        m_fbo.reset();
#ifdef Q3DS_PREVIEW_SUBPRESENTATION_RT2
        m_surfaceViewer->destroy();
#else
        m_surfaceViewer->shutdown();
#endif
        m_surfaceViewer.reset();
        m_context->doneCurrent();
        m_context.reset();
        m_surface->moveToThread(m_mainThread);
        m_semaphore.release();
    }

    void frameRendered()
    {
        m_updated = true;
    }

    QScopedPointer<Q3DSSurfaceViewer> m_surfaceViewer;
    QScopedPointer<QT_PREPEND_NAMESPACE(QOpenGLContext)> m_context;
    QScopedPointer<QOffscreenSurface> m_surface;
    QScopedPointer<QOpenGLFramebufferObject> m_fbo;
    QString m_path;
    QString m_presentation;
    QMutex m_mutex;
    bool m_running, m_updated, m_initialized;
    QThread *m_mainThread;
    QSemaphore m_semaphore;
    QSize m_size;
};

StudioSubpresentationRenderer::StudioSubpresentationRenderer(
        IQt3DSRenderContext &inRenderContext, const QString &id,
        const QString &presentation, const QString &path)
    : m_renderContext(inRenderContext), m_id(id), m_presentation(presentation), m_path(path)
    , m_program(nullptr)
    , m_vao(nullptr)
    , m_vertices(nullptr)
    , m_rendererType(inRenderContext.GetStringTable().RegisterStr("StudioPresentationRenderer"))
    , mRefCount(0)
{
    m_thread.reset(new RendererThread(QThread::currentThread()));
}

StudioSubpresentationRenderer::~StudioSubpresentationRenderer()
{
    QMutexLocker lock(&m_thread->m_mutex);
    if (m_thread->m_running) {
        m_thread->m_running = false;
        lock.unlock();
        m_thread->m_semaphore.acquire();
    }
}

CRegisteredString StudioSubpresentationRenderer::GetOffscreenRendererType()
{
    return m_rendererType;
}

SOffscreenRendererEnvironment
StudioSubpresentationRenderer::GetDesiredEnvironment(QT3DSVec2 inPresentationScaleFactor)
{
    // If we aren't using a clear color, then we are expected to blend with the background
    if (!m_thread->m_initialized) {
        m_thread->initialize(m_presentation, m_path);
        m_thread->start();
    }
    bool hasTransparency = true;
    NVRenderTextureFormats::Enum format =
        hasTransparency ? NVRenderTextureFormats::RGBA8 : NVRenderTextureFormats::RGB8;
    return SOffscreenRendererEnvironment(
                 (QT3DSU32)(m_thread->m_size.width() * inPresentationScaleFactor.x),
                 (QT3DSU32)(m_thread->m_size.height() * inPresentationScaleFactor.y),
                 format, OffscreenRendererDepthValues::Depth24, false,
                 AAModeValues::NoAA);
}

SOffscreenRenderFlags StudioSubpresentationRenderer::NeedsRender(
        const SOffscreenRendererEnvironment &inEnvironment,
        QT3DSVec2 inPresentationScaleFactor,
        const SRenderInstanceId instanceId)
{
    Q_UNUSED(inEnvironment)
    Q_UNUSED(inPresentationScaleFactor)
    Q_UNUSED(instanceId)
    return SOffscreenRenderFlags(true, true);
}

void StudioSubpresentationRenderer::Render(const SOffscreenRendererEnvironment &inEnvironment,
                    NVRenderContext &inRenderContext, QT3DSVec2 inPresentationScaleFactor,
                    SScene::RenderClearCommand inColorBufferNeedsClear,
                    const SRenderInstanceId instanceId)
{
    Q_UNUSED(inEnvironment)
    Q_UNUSED(inPresentationScaleFactor)
    Q_UNUSED(inColorBufferNeedsClear)
    Q_UNUSED(instanceId)
    inRenderContext.PushPropertySet();
    if (!m_thread->m_initialized) {
        m_thread->initialize(m_presentation, m_path);
        m_thread->start();
    }
    QMutexLocker lock(&m_thread->m_mutex);
    if (m_thread->m_initialized && m_thread->m_updated) {
        QT_PREPEND_NAMESPACE(QOpenGLContext) *context
                = QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext();
        QOpenGLFunctions *func = context->functions();
        GLuint texture = m_thread->m_fbo->texture();
        func->glDisable(GL_DEPTH_TEST);
        func->glEnable(GL_BLEND);
        func->glBlendEquation(GL_FUNC_ADD);
        func->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (!m_program)
            initializeFboCopy();

        m_program->bind();

        if (!m_vao) {
            m_vao = new QOpenGLVertexArrayObject;
            m_vao->create();
            m_vao->bind();
            m_vertices->bind();

            m_program->enableAttributeArray(0);
            m_program->enableAttributeArray(1);
            func->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
            func->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                                        (const void *)(4 * sizeof(GLfloat)));
            m_vertices->release();
        } else {
            m_vao->bind();
        }

        func->glActiveTexture(GL_TEXTURE0);
        func->glBindTexture(GL_TEXTURE_2D, texture);
        func->glDrawArrays(GL_TRIANGLES, 0, 6);
        func->glEnable(GL_DEPTH_TEST);
        func->glBindTexture(GL_TEXTURE_2D, 0);

        m_program->release();
        m_vao->release();
    }
    inRenderContext.PopPropertySet(true);
}

void StudioSubpresentationRenderer::RenderWithClear(
                    const SOffscreenRendererEnvironment &inEnvironment,
                    NVRenderContext &inRenderContext, QT3DSVec2 inPresentationScaleFactor,
                    SScene::RenderClearCommand inColorBufferNeedsClear,
                    QT3DSVec3 inclearColor,
                    const SRenderInstanceId instanceId)
{
    inRenderContext.SetClearColor(QT3DSVec4(inclearColor, 1.0));
    Render(inEnvironment, inRenderContext, inPresentationScaleFactor,
           inColorBufferNeedsClear, instanceId);
}

void StudioSubpresentationRenderer::initializeFboCopy()
{
    m_program = new QOpenGLShaderProgram;
    if (m_thread->m_context->format().renderableType() == QSurfaceFormat::OpenGLES) {
        static const char *vsSource =
            "attribute highp vec4 pos;\n"
            "attribute highp vec2 tc;\n"
            "varying lowp vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = tc;\n"
            "   gl_Position = pos;\n"
            "}\n";
        static const char *fsSource =
            "varying highp vec2 texcoord;\n"
            "uniform sampler2D sampler;\n"
            "void main() {\n"
            "   gl_FragColor = texture2D(sampler, texcoord);\n"
            "}\n";
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsSource);
    } else {
        static const char *vsSource =
            "#version 150 core\n"
            "in vec4 pos;\n"
            "in vec2 tc;\n"
            "out vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = tc;\n"
            "   gl_Position = pos;\n"
            "}\n";
        static const char *fsSource =
            "#version 150 core\n"
            "in vec2 texcoord;\n"
            "out vec4 fragColor;\n"
            "uniform sampler2D sampler;\n"
            "void main() {\n"
            "   fragColor = texture(sampler, texcoord);\n"
            "}\n";
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsSource);
    }

    m_program->bindAttributeLocation("pos", 0);
    m_program->bindAttributeLocation("tc", 1);

    if (!m_program->link()) {
        QByteArray logData(m_program->log().toLocal8Bit());
        const char *log = logData.data();
        qFatal("Failed to create shader program: %s", log);
    }

    m_vertices = new QOpenGLBuffer;
    m_vertices->create();
    m_vertices->bind();

    static const float vertices[] =
    {
        -1, -1, 0, 1, 0, 0,
        1, -1, 0, 1, 1, 0,
        1, 1, 0, 1, 1, 1,
        -1, -1, 0, 1, 0, 0,
        1, 1, 0, 1, 1, 1,
        -1, 1, 0, 1, 0, 1,
    };

    m_vertices->allocate(vertices, sizeof(vertices));
    m_vertices->release();
}
