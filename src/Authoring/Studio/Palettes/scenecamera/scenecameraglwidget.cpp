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

#include "Qt3DSCommonPrecompile.h"
#include "scenecameraglwidget.h"
#include "StudioApp.h"
#include "WGLRenderContext.h"
#include "StudioPreferences.h"
#include "Q3DStudioRenderer.h"
#include "StudioProjectSettings.h"
#include "StudioUtils.h"

#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopengltexture.h>
#include <QtGui/qopenglbuffer.h>
#include <QtGui/qopenglvertexarrayobject.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qoffscreensurface.h>
#include <QtWidgets/qmessagebox.h>

const QVector4D defaultTextureOffset = QVector4D(0.0f, 0.0f, 1.0f, 1.0f);
const QVector2D defaultGeometryOffset = QVector2D(1.0f, 1.0f);

SceneCameraGlWidget::SceneCameraGlWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_textureOffset(defaultTextureOffset)
    , m_geometryOffset(defaultGeometryOffset)
{
    QSurfaceFormat format; // TODO get proper format, old: = CWGLRenderContext::selectSurfaceFormat(this);
    format.setSamples(1); // We want pixel perfect view, not aliased one
    setFormat(format);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

SceneCameraGlWidget::~SceneCameraGlWidget()
{
    cleanup();
    m_renderer = std::shared_ptr<Q3DStudio::IStudioRenderer>();
}

void SceneCameraGlWidget::requestRender()
{
    if (m_renderer && m_renderer->IsInitialized())
        m_renderer->RequestRender();
}

void SceneCameraGlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    QObject::connect(context(), &QOpenGLContext::aboutToBeDestroyed,
                     this, &SceneCameraGlWidget::cleanup);

    if (!m_renderer)
        m_renderer = Q3DStudio::IStudioRenderer::CreateStudioRenderer(true);
    if (!m_renderer->IsInitialized())
        m_renderer->initialize(this, m_hasPresentation);

    m_program = new QOpenGLShaderProgram();
    if (!m_program->addShaderFromSourceCode(
                QOpenGLShader::Vertex,
                "#version 330 core\n"
                "in vec2 vertexPos;\n"
                "in vec2 vertexTexCoord;\n"
                "uniform vec4 uTexOffset;\n"
                "uniform vec4 uGeomOffset;\n"
                "out vec2 texCoord;\n"
                "void main(void)\n"
                "{\n"
                "  gl_Position = vec4(uGeomOffset.xy + vertexPos * uGeomOffset.zw, 0.0, 1.0);\n"
                "  texCoord = vec2(uTexOffset.z * vertexTexCoord.x + uTexOffset.x,\n"
                "                  uTexOffset.w * vertexTexCoord.y + uTexOffset.y);\n"
                "}")) {
        qWarning() << __FUNCTION__ << "Failed to add vertex shader for scene camera preview";
        return;
    }
    if (!m_program->addShaderFromSourceCode(
                QOpenGLShader::Fragment,
                "#version 330 core\n"
                "in vec2 texCoord;\n"
                "uniform sampler2D uSampler;\n"
                "out vec4 fragColor;\n"
                "void main(void) {\n"
                "  vec4 oc = texture(uSampler, texCoord);\n"
                "  fragColor = vec4(oc);\n"
                "}")) {

        qWarning() << __FUNCTION__ << "Failed to add fragment shader for scene camera preview";
        return;
    }
    if (!m_program->link()) {
        qWarning() << __FUNCTION__ << "Failed to link program for scene camera preview";
        return;
    }
    if (!m_program->bind()) {
        qWarning() << __FUNCTION__ << "Failed to bind program for scene camera preview";
        return;
    } else {
        GLint vertexAtt = GLint(m_program->attributeLocation("vertexPos"));
        GLint uvAtt = GLint(m_program->attributeLocation("vertexTexCoord"));
        m_uniformTextureOffset = GLint(m_program->uniformLocation("uTexOffset"));
        m_uniformGeometryOffset = GLint(m_program->uniformLocation("uGeomOffset"));
        m_program->setUniformValue("uSampler", 0);

        m_vao = new QOpenGLVertexArrayObject;
        if (m_vao->create()) {
            m_vao->bind();
            m_vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            if (m_vertexBuffer->create() && m_vertexBuffer->bind()) {
                GLfloat vertexBuffer[] = {-1.0f, 1.0f,
                                          -1.0f, -1.0f,
                                          1.0f, 1.0f,
                                          1.0f, -1.0f};
                m_vertexBuffer->allocate(vertexBuffer, 8 * sizeof(GLfloat));
                glEnableVertexAttribArray(vertexAtt);
                glVertexAttribPointer(vertexAtt, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
            } else {
                qWarning() << __FUNCTION__
                           << "Failed to create/bind vertex buffer for scene camera preview";
                return;
            }
            m_uvBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            if (m_uvBuffer->create() && m_uvBuffer->bind()) {
                GLfloat uvBuffer[] = {0.0f, 1.0f,
                                      0.0f, 0.0f,
                                      1.0f, 1.0f,
                                      1.0f, 0.0f};
                m_uvBuffer->allocate(uvBuffer, 8 * sizeof(GLfloat));
                glEnableVertexAttribArray(uvAtt);
                glVertexAttribPointer(uvAtt, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
            } else {
                qWarning() << __FUNCTION__
                           << "Failed to create/bind UV buffer for scene camera preview";
                return;
            }

            m_vao->release();
        } else {
            qWarning() << __FUNCTION__ << "Failed to create/bind vertex array object";
            return;
        }
    }

    const QColor matteColor = CStudioPreferences::matteColor();
    glClearColor(matteColor.redF(), matteColor.greenF(), matteColor.blueF(), 1.0f);
}

void SceneCameraGlWidget::paintGL()
{
    if (m_renderer && m_renderer->IsInitialized()) {
        m_fboSize = g_StudioApp.GetCore()->GetStudioProjectSettings()->getPresentationSize();
        if (!m_fbo || m_fboSize != m_fbo->size()) {
            delete m_fbo;
            m_fbo = new QOpenGLFramebufferObject(m_fboSize,
                                                 QOpenGLFramebufferObject::CombinedDepthStencil);
            m_renderer->SetViewRect(QRect(0, 0, m_fboSize.width(), m_fboSize.height()), m_fboSize);
        }
        m_fbo->bind();
        m_renderer->renderNow();
        m_fbo->bindDefault();

        // Clean the OpenGL state
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glViewport(0, 0, width() * m_pixelRatio, height() * m_pixelRatio);

        m_program->bind();
        m_vao->bind();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_fbo->texture());

        m_program->setUniformValueArray(m_uniformTextureOffset, &m_textureOffset, 1);
        m_program->setUniformValueArray(m_uniformGeometryOffset, &m_geometryOffset, 1);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_vao->release();
        m_program->release();
    }
}

void SceneCameraGlWidget::resizeGL(int w, int h)
{
    // We need to update immediately to avoid flicker
     update();

    const qreal pixelRatio = StudioUtils::devicePixelRatio(window()->windowHandle());
    if (pixelRatio != m_pixelRatio) {
        m_pixelRatio = pixelRatio;
        delete m_fbo;
        m_fbo = nullptr;
    }

    if (m_renderer && m_renderer->IsInitialized())
        m_renderer->RequestRender();
}

void SceneCameraGlWidget::cleanup()
{
    makeCurrent();

    delete m_fbo;
    delete m_program;
    delete m_vertexBuffer;
    delete m_uvBuffer;
    delete m_vao;
    m_fbo = nullptr;
    m_program = nullptr;
    m_vertexBuffer = nullptr;
    m_uvBuffer = nullptr;
    m_vao = nullptr;
    m_uniformTextureOffset = 0;
    m_uniformGeometryOffset = 0;
    m_textureOffset = defaultTextureOffset;
    m_geometryOffset = defaultGeometryOffset;

    if (m_renderer)
        m_renderer->Close();

    doneCurrent();
}
