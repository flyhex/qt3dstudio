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

#include "render/backends/gl/Qt3DSOpenGLExtensions.h"

Qt3DSOpenGLExtensions::Qt3DSOpenGLExtensions()
    : QAbstractOpenGLExtension(*(new Qt3DSOpenGLExtensionsPrivate))
{
}

bool Qt3DSOpenGLExtensions::initializeOpenGLFunctions()
{
    if (isInitialized())
        return true;

    QT_PREPEND_NAMESPACE(QOpenGLContext) *context =
            QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext();
    if (!context) {
        qWarning("A current OpenGL context is required to resolve functions");
        return false;
    }

    Q_D(Qt3DSOpenGLExtensions);

    d->BlendBarrierNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)()>(
        context->getProcAddress("glBlendBarrierNV"));
    d->PathGlyphIndexArrayNV = reinterpret_cast<GLenum (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, const void*, GLbitfield, GLuint, GLsizei, GLuint,
        GLfloat)>(
            context->getProcAddress("glPathGlyphIndexArrayNV"));
    d->PathGlyphIndexRangeNV = reinterpret_cast<GLenum (QOPENGLF_APIENTRYP)(
        GLenum, const void*, GLbitfield, GLuint, GLfloat, GLuint[2])>(
            context->getProcAddress("glPathGlyphIndexRangeNV"));
    QAbstractOpenGLExtension::initializeOpenGLFunctions();
    return true;
}

#if defined(QT_OPENGL_ES)
Qt3DSOpenGLES2Extensions::Qt3DSOpenGLES2Extensions()
{
}

bool Qt3DSOpenGLES2Extensions::initializeOpenGLFunctions()
{
    if (isInitialized())
        return true;

    QT_PREPEND_NAMESPACE(QOpenGLContext) *context =
            QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext();
    if (!context) {
        qWarning("A current OpenGL context is required to resolve functions");
        return false;
    }

    Q_D(Qt3DSOpenGLExtensions);

#if defined(QT_OPENGL_ES)
    d->PatchParameteriEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLenum, GLint)>(
            context->getProcAddress("glPatchParameteriEXT"));
    d->QueryCounterEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum)>(
            context->getProcAddress("glQueryCounterEXT"));
    d->GetQueryObjectui64vEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, GLuint64 *)>(
            context->getProcAddress("glGetQueryObjectui64vEXT"));
    d->GenPathsNV = reinterpret_cast<GLuint (QOPENGLF_APIENTRYP)(
        GLsizei)>(
            context->getProcAddress("glGenPathsNV"));
    d->DeletePathsNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLsizei)>(
            context->getProcAddress("glDeletePathsNV"));
    d->PathCommandsNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLsizei, const GLubyte *, GLsizei, GLenum, const void *)>(
            context->getProcAddress("glPathCommandsNV"));
    d->PathGlyphsNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, const void *, GLbitfield, GLsizei, GLenum, const void *,
        GLenum, GLuint, GLfloat)>(
            context->getProcAddress("glPathGlyphsNV"));
    d->PathGlyphRangeNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, const void *, GLbitfield, GLuint, GLsizei, GLenum,
        GLuint, GLfloat)>(
            context->getProcAddress("glPathGlyphRangeNV"));
    d->PathParameterfNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, GLfloat)>(
            context->getProcAddress("glPathParameterfNV"));
    d->PathStencilDepthOffsetNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLfloat, GLfloat)>(
            context->getProcAddress("glPathStencilDepthOffsetNV"));
    d->StencilFillPathNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, GLuint)>(
            context->getProcAddress("glStencilFillPathNV"));
    d->StencilStrokePathNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLint, GLuint)>(
            context->getProcAddress("glStencilStrokePathNV"));
    d->StencilFillPathInstancedNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLsizei, GLenum, const void *, GLuint, GLenum, GLuint, GLenum,
        const GLfloat *)>(
            context->getProcAddress("glStencilFillPathInstancedNV"));
    d->StencilStrokePathInstancedNV
        = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei, GLenum,
            const void *, GLuint, GLint, GLuint, GLenum, const GLfloat *)>(
                context->getProcAddress("glStencilStrokePathInstancedNV"));
    d->PathCoverDepthFuncNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLenum func)>(
            context->getProcAddress("glPathCoverDepthFuncNV"));
    d->CoverFillPathInstancedNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLsizei, GLenum, const void *, GLuint, GLenum, GLenum,
        const GLfloat *)>(
            context->getProcAddress("glCoverFillPathInstancedNV"));
    d->CoverStrokePathInstancedNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLsizei, GLenum, const void *, GLuint, GLenum, GLenum,
        const GLfloat *)>(
            context->getProcAddress("glCoverStrokePathInstancedNV"));
    d->GetPathParameterfvNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLuint, GLenum, GLfloat *)>(
            context->getProcAddress("glGetPathParameterfvNV"));
    d->GetPathMetricsNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLbitfield, GLsizei, GLenum, const void *, GLuint, GLsizei, GLfloat *)>(
            context->getProcAddress("glGetPathMetricsNV"));
    d->GetPathMetricRangeNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLbitfield, GLuint, GLsizei, GLsizei, GLfloat *)>(
            context->getProcAddress("glGetPathMetricRangeNV"));
    d->GetPathSpacingNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(
        GLenum, GLsizei, GLenum, const void *, GLuint, GLfloat, GLfloat, GLenum,
        GLfloat *)>(
            context->getProcAddress("glGetPathSpacingNV"));
    d->BindVertexArrayOES = reinterpret_cast<void (QOPENGLF_APIENTRYP)
        (GLuint)>(
            context->getProcAddress("glBindVertexArrayOES"));
    d->DeleteVertexArraysOES = reinterpret_cast<void (QOPENGLF_APIENTRYP)
        (GLsizei, const GLuint *)>(
            context->getProcAddress("glDeleteVertexArraysOES"));
    d->GenVertexArraysOES = reinterpret_cast<void (QOPENGLF_APIENTRYP)
        (GLsizei, GLuint *)>(
            context->getProcAddress("glGenVertexArraysOES"));
    d->IsVertexArrayOES = reinterpret_cast<GLboolean (QOPENGLF_APIENTRYP)
        (GLuint)>(
            context->getProcAddress("glIsVertexArrayOES"));
#endif
    Qt3DSOpenGLExtensions::initializeOpenGLFunctions();
    return true;
}
#endif // QT_OPENGL_ES
