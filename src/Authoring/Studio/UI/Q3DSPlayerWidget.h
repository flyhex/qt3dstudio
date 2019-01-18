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

#ifndef PLAYER_WIDGET_H
#define PLAYER_WIDGET_H

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurfaceformat.h>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLBuffer)
QT_FORWARD_DECLARE_CLASS(QOpenGLVertexArrayObject)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)

namespace Q3DStudio {

class Q3DSPlayerWidget : public QOpenGLWidget, QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit Q3DSPlayerWidget(QWidget *parent = nullptr);
    ~Q3DSPlayerWidget();

    void maybeInvalidateFbo();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int, int) override;

private:
    void cleanup();

    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLBuffer *m_vertexBuffer = nullptr;
    QOpenGLBuffer *m_uvBuffer = nullptr;
    QOpenGLVertexArrayObject *m_vao = nullptr;
    QOpenGLFramebufferObject *m_fbo = nullptr;
    qreal m_fboPixelRatio = 0.0;
    bool m_invalidateFbo = false;
};

}

#endif
