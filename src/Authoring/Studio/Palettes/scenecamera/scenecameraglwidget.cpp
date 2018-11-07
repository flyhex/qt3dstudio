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
#include "IStudioRenderer.h"
#include "WGLRenderContext.h"
#include "StudioPreferences.h"

#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopengltexture.h>
#include <QtGui/qopenglbuffer.h>
#include <QtGui/qopenglvertexarrayobject.h>

const QVector4D defaultTextureOffset = QVector4D(0.0f, 0.0f, 1.0f, 1.0f);
const QVector2D defaultGeometryOffset = QVector2D(1.0f, 1.0f);

SceneCameraGlWidget::SceneCameraGlWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_textureOffset(defaultTextureOffset)
    , m_geometryOffset(defaultGeometryOffset)
{
    QSurfaceFormat format = CWGLRenderContext::selectSurfaceFormat(this);
    format.setSamples(1); // We want pixel perfect view, not aliased one
    setFormat(format);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

SceneCameraGlWidget::~SceneCameraGlWidget()
{
    cleanup();
}

void SceneCameraGlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    QObject::connect(context(), &QOpenGLContext::aboutToBeDestroyed,
                     this, &SceneCameraGlWidget::cleanup);

    m_program = new QOpenGLShaderProgram();
    if (!m_program->addShaderFromSourceCode(
                QOpenGLShader::Vertex,
                "#version 110\n"
                "attribute highp vec2 aVertex;\n"
                "attribute highp vec2 aUV;\n"
                "uniform highp vec4 uTexOffset;\n"
                "uniform highp vec4 uGeomOffset;\n"
                "varying highp vec2 vUV;\n"
                "void main(void)\n"
                "{\n"
                "  gl_Position = vec4(uGeomOffset.xy + aVertex * uGeomOffset.zw, 0.0, 1.0);\n"
                "  vUV = vec2(uTexOffset.z * aUV.x + uTexOffset.x,\n"
                "             uTexOffset.w * aUV.y + uTexOffset.y);\n"
                "}")) {
        qWarning() << __FUNCTION__ << "Failed to add vertex shader for scene camera preview";
        return;
    }
    if (!m_program->addShaderFromSourceCode(
                QOpenGLShader::Fragment,
                "#version 110\n"
                "varying highp vec2 vUV;\n"
                "uniform sampler2D uSampler;\n"
                "void main(void) {\n"
                "  lowp vec4 oc = texture2D(uSampler, vUV);\n"
                "  gl_FragColor = vec4(oc);\n"
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
        GLint vertexAtt = GLint(m_program->attributeLocation("aVertex"));
        GLint uvAtt = GLint(m_program->attributeLocation("aUV"));
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
    Q3DStudio::IStudioRenderer &renderer(g_StudioApp.getRenderer());
    if (renderer.IsInitialized()) {
        m_vao->bind();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        QSize fboSize;
        qt3ds::QT3DSU32 textureId;
        renderer.getFullSizePreviewFbo(fboSize, textureId);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GLuint(textureId));

        m_program->setUniformValueArray(m_uniformTextureOffset, &m_textureOffset, 1);
        m_program->setUniformValueArray(m_uniformGeometryOffset, &m_geometryOffset, 1);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_vao->release();
    }
}

void SceneCameraGlWidget::resizeGL(int, int)
{
    // We need to update immediately to avoid flicker
    update();
}

void SceneCameraGlWidget::cleanup()
{
    makeCurrent();

    delete m_program;
    delete m_vertexBuffer;
    delete m_uvBuffer;
    delete m_vao;
    m_program = nullptr;
    m_vertexBuffer = nullptr;
    m_uvBuffer = nullptr;
    m_vao = nullptr;
    m_uniformTextureOffset = 0;
    m_uniformGeometryOffset = 0;
    m_textureOffset = defaultTextureOffset;
    m_geometryOffset = defaultGeometryOffset;

    doneCurrent();
}
