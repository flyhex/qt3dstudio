/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include "Q3DSPlayerWidget.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"

#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopengltexture.h>
#include <QtGui/qopenglbuffer.h>
#include <QtGui/qopenglvertexarrayobject.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopenglpaintdevice.h>
#include <QtGui/qpainter.h>
#include <QtGui/qvector2d.h>
#include <QtWidgets/qmessagebox.h>

const QVector2D defaultGeometryOffset = QVector2D(1.0f, 1.0f);

namespace Q3DStudio {

Q3DSPlayerWidget::Q3DSPlayerWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

Q3DSPlayerWidget::~Q3DSPlayerWidget()
{
    cleanup();
}

void Q3DSPlayerWidget::maybeInvalidateFbo(const QSize &size)
{
    m_fboSize = size;
    if (m_fboPixelRatio != StudioUtils::devicePixelRatio(window()->windowHandle())
            || !m_fbo || m_fboSize != m_fbo->size()) {
        m_invalidateFbo = true;
    }
}

void Q3DSPlayerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    QObject::connect(context(), &QOpenGLContext::aboutToBeDestroyed,
                     this, &Q3DSPlayerWidget::cleanup);

    Q3DStudio::IStudioRenderer &renderer(g_StudioApp.getRenderer());
    if (!renderer.IsInitialized()) {
        try {
            renderer.initialize(this, false);
        } catch (...) {
            QMessageBox::critical(this, tr("Fatal Error"),
                                  tr("Unable to initialize OpenGL.\nThis may be because your "
                                     "graphic device is not sufficient, or simply because your "
                                     "driver is too old.\n\nPlease try upgrading your graphics "
                                     "driver and try again."));
            exit(1);
        }
    }

    m_program = new QOpenGLShaderProgram();
    if (!m_program->addShaderFromSourceCode(
                QOpenGLShader::Vertex,
                "#version 330 core\n"
                "in vec2 vertexPos;\n"
                "in vec2 vertexTexCoord;\n"
                "uniform vec2 uGeomOffset;\n"
                "out vec2 texCoord;\n"
                "void main(void)\n"
                "{\n"
                "  gl_Position = vec4(vertexPos * uGeomOffset.xy, 0.0, 1.0);\n"
                "  texCoord = vec2(vertexTexCoord);\n"
                "}")) {
        qWarning() << __FUNCTION__ << "Failed to add vertex shader for scene view";
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

        qWarning() << __FUNCTION__ << "Failed to add fragment shader for scene view";
        return;
    }
    if (!m_program->link()) {
        qWarning() << __FUNCTION__ << "Failed to link program for scene view";
        return;
    }
    if (!m_program->bind()) {
        qWarning() << __FUNCTION__ << "Failed to bind program for scene view";
        return;
    } else {
        GLint vertexAtt = GLint(m_program->attributeLocation("vertexPos"));
        GLint uvAtt = GLint(m_program->attributeLocation("vertexTexCoord"));
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
                m_vertexBuffer->release();
            } else {
                qWarning() << __FUNCTION__
                           << "Failed to create/bind vertex buffer for scene view";
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
                m_uvBuffer->release();
            } else {
                qWarning() << __FUNCTION__
                           << "Failed to create/bind UV buffer for scene view";
                return;
            }

            m_vao->release();
        } else {
            qWarning() << __FUNCTION__ << "Failed to create/bind vertex array object";
            return;
        }
        m_program->release();
    }

    const QColor matteColor = CStudioPreferences::matteColor();
    glClearColor(matteColor.redF(), matteColor.greenF(), matteColor.blueF(), 1.0f);
}

void Q3DSPlayerWidget::paintGL()
{
    Q3DStudio::IStudioRenderer &renderer(g_StudioApp.getRenderer());
    if (renderer.IsInitialized()) {
        if (!m_fbo || m_invalidateFbo)
            resizeGL(width(), height());
        m_fbo->bind();
        renderer.renderNow();
        m_fbo->bindDefault();

        // Clean the OpenGL state
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glViewport(0, 0, width() * m_fboPixelRatio, height() * m_fboPixelRatio);

        m_program->bind();
        m_vao->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_fbo->texture());

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_vao->release();
        m_program->release();

        QOpenGLPaintDevice device;
        device.setSize(size() * m_fboPixelRatio);
        QPainter painter(&device);

        renderer.drawRulersAndGuides(&painter);
    }
}

void Q3DSPlayerWidget::resizeGL(int w, int h)
{
    const qreal pixelRatio = StudioUtils::devicePixelRatio(window()->windowHandle());
    QSize fboSize;
    if (m_fboSize.isValid()) {
        fboSize = m_fboSize;
    } else {
        fboSize.setWidth(int(pixelRatio * w));
        fboSize.setHeight(int(pixelRatio * h));
    }

    if (!m_fbo || m_fbo->size() != fboSize) {
        delete m_fbo;
        m_fbo = new QOpenGLFramebufferObject(fboSize,
                                             QOpenGLFramebufferObject::CombinedDepthStencil);
        m_fboPixelRatio = pixelRatio;
        m_invalidateFbo = false;
    }

    QVector2D geometryOffset = defaultGeometryOffset;
    int wScaled = w * pixelRatio;
    int hScaled = h * pixelRatio;
    if (m_fbo->size() != QSize(wScaled, hScaled)) {
        qreal diffX = qreal(wScaled - m_fbo->size().width()) / qreal(wScaled);
        qreal diffY = qreal(hScaled - m_fbo->size().height()) / qreal(hScaled);
        geometryOffset.setX(float(1.0 - diffX));
        geometryOffset.setY(float(1.0 - diffY));
    }

    m_program->bind();
    m_program->setUniformValueArray(m_uniformGeometryOffset, &geometryOffset, 1);
    m_program->release();
}

void Q3DSPlayerWidget::cleanup()
{
    makeCurrent();

    delete m_fbo;
    delete m_program;
    delete m_vertexBuffer;
    delete m_uvBuffer;
    delete m_vao;
    m_program = nullptr;
    m_vertexBuffer = nullptr;
    m_uvBuffer = nullptr;
    m_vao = nullptr;
    m_fbo = nullptr;
    m_fboPixelRatio = 0;
    m_uniformGeometryOffset = 0;

    doneCurrent();
}

}
