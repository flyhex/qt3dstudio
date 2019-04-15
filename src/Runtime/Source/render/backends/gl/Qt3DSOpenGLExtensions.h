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

#ifndef QT3DSOPENGLEXTENSIONS_H
#define QT3DSOPENGLEXTENSIONS_H

#include <QtOpenGLExtensions/QtOpenGLExtensions>

/* Some OpenGL extensions that are not (yet) found in Qt's OpenGL extensions.
 * These should be auto-generated and added to QtOpenGLExtensions module */
class Qt3DSOpenGLExtensionsPrivate : public QAbstractOpenGLExtensionPrivate
{
public:
    void (QOPENGLF_APIENTRYP BlendBarrierNV)();
    GLenum (QOPENGLF_APIENTRYP PathGlyphIndexArrayNV)(
        GLuint, GLenum, const void*, GLbitfield, GLuint, GLsizei, GLuint,
        GLfloat);
    GLenum (QOPENGLF_APIENTRYP PathGlyphIndexRangeNV)(
        GLenum, const void*, GLbitfield, GLuint, GLfloat, GLuint[2]);

#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
    void (QOPENGLF_APIENTRYP PatchParameteriEXT)(GLenum, GLint);
    void (QOPENGLF_APIENTRYP QueryCounterEXT)(GLuint, GLenum);
    void (QOPENGLF_APIENTRYP GetQueryObjectui64vEXT)(GLuint, GLenum,
        GLuint64 *);
    GLuint (QOPENGLF_APIENTRYP GenPathsNV)(GLsizei);
    void (QOPENGLF_APIENTRYP DeletePathsNV)(GLuint, GLsizei);
    void (QOPENGLF_APIENTRYP PathCommandsNV)(GLuint, GLsizei, const GLubyte *,
        GLsizei, GLenum, const void *);
    void (QOPENGLF_APIENTRYP PathGlyphsNV)(GLuint, GLenum, const void *,
        GLbitfield, GLsizei, GLenum, const void *, GLenum, GLuint, GLfloat);
    void (QOPENGLF_APIENTRYP PathGlyphRangeNV)(GLuint, GLenum, const void *,
        GLbitfield, GLuint, GLsizei, GLenum, GLuint, GLfloat);
    void (QOPENGLF_APIENTRYP PathParameterfNV)(GLuint, GLenum, GLfloat);
    void (QOPENGLF_APIENTRYP PathStencilDepthOffsetNV)(GLfloat, GLfloat);
    void (QOPENGLF_APIENTRYP StencilFillPathNV)(GLuint, GLenum, GLuint);
    void (QOPENGLF_APIENTRYP StencilStrokePathNV)(GLuint, GLint, GLuint);
    void (QOPENGLF_APIENTRYP StencilFillPathInstancedNV)(GLsizei, GLenum,
        const void *, GLuint, GLenum, GLuint, GLenum, const GLfloat *);
    void (QOPENGLF_APIENTRYP StencilStrokePathInstancedNV)(GLsizei, GLenum,
        const void *, GLuint, GLint, GLuint, GLenum, const GLfloat *);
    void (QOPENGLF_APIENTRYP PathCoverDepthFuncNV)(GLenum);
    void (QOPENGLF_APIENTRYP CoverFillPathInstancedNV)(GLsizei, GLenum,
        const void *, GLuint, GLenum, GLenum, const GLfloat *);
    void (QOPENGLF_APIENTRYP CoverStrokePathInstancedNV)(GLsizei, GLenum,
        const void *, GLuint, GLenum, GLenum, const GLfloat *);
    void (QOPENGLF_APIENTRYP GetPathParameterfvNV)(GLuint, GLenum, GLfloat *);
    void (QOPENGLF_APIENTRYP GetPathMetricsNV)(GLbitfield, GLsizei, GLenum,
        const void *, GLuint, GLsizei, GLfloat *);
    void (QOPENGLF_APIENTRYP GetPathMetricRangeNV)(GLbitfield, GLuint, GLsizei,
        GLsizei, GLfloat *);
    void (QOPENGLF_APIENTRYP GetPathSpacingNV)(GLenum, GLsizei, GLenum,
        const void *, GLuint, GLfloat, GLfloat, GLenum, GLfloat *);
    void (QOPENGLF_APIENTRYP BindVertexArrayOES) (GLuint array);
    void (QOPENGLF_APIENTRYP DeleteVertexArraysOES) (GLsizei n, const GLuint *arrays);
    void (QOPENGLF_APIENTRYP GenVertexArraysOES) (GLsizei n, GLuint *arrays);
    GLboolean (QOPENGLF_APIENTRYP IsVertexArrayOES) (GLuint array);
#endif
};

class Qt3DSOpenGLExtensions : public QAbstractOpenGLExtension
{
public:
    Qt3DSOpenGLExtensions();

    bool initializeOpenGLFunctions() override;

    void glBlendBarrierNV();
    GLenum glPathGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget,
        const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex,
        GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
    GLenum glPathGlyphIndexRangeNV(GLenum fontTarget, const void *fontName,
        GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale,
        GLuint baseAndCount[2]);

protected:
      Q_DECLARE_PRIVATE(Qt3DSOpenGLExtensions)
};

inline void Qt3DSOpenGLExtensions::glBlendBarrierNV()
{
    Q_D(Qt3DSOpenGLExtensions);
    d->BlendBarrierNV();
}

inline GLenum Qt3DSOpenGLExtensions::glPathGlyphIndexArrayNV(
    GLuint firstPathName, GLenum fontTarget, const void *fontName,
    GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs,
    GLuint pathParameterTemplate, GLfloat emScale)
{
    Q_D(Qt3DSOpenGLExtensions);
    return d->PathGlyphIndexArrayNV(firstPathName, fontTarget, fontName,
        fontStyle, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale);
}

inline GLenum Qt3DSOpenGLExtensions::glPathGlyphIndexRangeNV(GLenum fontTarget,
    const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate,
    GLfloat emScale, GLuint baseAndCount[2])
{
    Q_D(Qt3DSOpenGLExtensions);
    return d->PathGlyphIndexRangeNV(fontTarget, fontName, fontStyle,
        pathParameterTemplate, emScale, baseAndCount);
}

#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
class Qt3DSOpenGLES2Extensions : public Qt3DSOpenGLExtensions
{
public:
    Qt3DSOpenGLES2Extensions();

    // tesselation shader
    void glPatchParameteriEXT(GLenum pname, GLint value);

    // timer
    void glQueryCounterEXT(GLuint id, GLenum target);
    void glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64 *params);

    // nv paths

    GLuint glGenPathsNV(GLsizei range);
    void glDeletePathsNV(GLuint path, GLsizei range);
    void glPathCommandsNV(GLuint path, GLsizei numCommands,
        const GLubyte *commands, GLsizei numCoords, GLenum coordType,
        const void *coords);
    void glPathGlyphsNV(GLuint firstPathName, GLenum fontTarget,
        const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs,
        GLenum type, const void *charcodes, GLenum handleMissingGlyphs,
        GLuint pathParameterTemplate, GLfloat emScale);
    void glPathGlyphRangeNV(GLuint firstPathName, GLenum fontTarget,
        const void *fontName, GLbitfield fontStyle, GLuint firstGlyph,
        GLsizei numGlyphs, GLenum handleMissingGlyphs,
        GLuint pathParameterTemplate, GLfloat emScale);
    void glPathParameterfNV(GLuint path, GLenum pname, GLfloat value);
    void glPathStencilDepthOffsetNV(GLfloat factor, GLfloat units);
    void glStencilFillPathNV(GLuint path, GLenum fillMode, GLuint mask);
    void glStencilStrokePathNV(GLuint path, GLint reference, GLuint mask);
    void glStencilFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType,
        const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask,
        GLenum transformType, const GLfloat *transformValues);
    void glStencilStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType,
        const void *paths, GLuint pathBase, GLint reference, GLuint mask,
        GLenum transformType, const GLfloat *transformValues);
    void glPathCoverDepthFuncNV(GLenum func);
    void glCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType,
        const void *paths, GLuint pathBase, GLenum coverMode,
        GLenum transformType, const GLfloat *transformValues);
    void glCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType,
        const void *paths, GLuint pathBase, GLenum coverMode,
        GLenum transformType, const GLfloat *transformValues);
    void glGetPathParameterfvNV(GLuint path, GLenum pname, GLfloat *value);
    void glGetPathMetricsNV(GLbitfield metricQueryMask, GLsizei numPaths,
        GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride,
        GLfloat *metrics);
    void glGetPathMetricRangeNV(GLbitfield metricQueryMask,
        GLuint firstPathName, GLsizei numPaths, GLsizei stride,
        GLfloat *metrics);
    void glGetPathSpacingNV(GLenum pathListMode, GLsizei numPaths,
        GLenum pathNameType, const void *paths, GLuint pathBase,
        GLfloat advanceScale, GLfloat kerningScale, GLenum transformType,
        GLfloat *returnedSpacing);
    void glBindVertexArrayOES(GLuint array);
    void glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays);
    void glGenVertexArraysOES(GLsizei n, GLuint *arrays);
    GLboolean glIsVertexArrayOES(GLuint array);

    bool initializeOpenGLFunctions() Q_DECL_FINAL;
};

inline void Qt3DSOpenGLES2Extensions::glPatchParameteriEXT(GLenum pname,
    GLint value)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PatchParameteriEXT(pname, value);
}

inline void Qt3DSOpenGLES2Extensions::glQueryCounterEXT(GLuint id,
    GLenum target)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->QueryCounterEXT(id, target);
}

inline void Qt3DSOpenGLES2Extensions::glGetQueryObjectui64vEXT(GLuint id,
    GLenum pname, GLuint64 *params)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->GetQueryObjectui64vEXT(id, pname, params);
}

inline GLuint Qt3DSOpenGLES2Extensions::glGenPathsNV(GLsizei range)
{
    Q_D(Qt3DSOpenGLExtensions);
    return d->GenPathsNV(range);
}

inline void Qt3DSOpenGLES2Extensions::glDeletePathsNV(GLuint path,
    GLsizei range)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->DeletePathsNV(path, range);
}

inline void Qt3DSOpenGLES2Extensions::glPathCommandsNV(GLuint path,
    GLsizei numCommands, const GLubyte *commands, GLsizei numCoords,
    GLenum coordType, const void *coords)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PathCommandsNV(path, numCommands, commands, numCoords, coordType,
        coords);
}

inline void Qt3DSOpenGLES2Extensions::glPathGlyphsNV(GLuint firstPathName,
    GLenum fontTarget, const void *fontName, GLbitfield fontStyle,
    GLsizei numGlyphs, GLenum type, const void *charcodes,
    GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PathGlyphsNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs,
        type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale);
}

inline void Qt3DSOpenGLES2Extensions::glPathGlyphRangeNV(GLuint firstPathName,
    GLenum fontTarget, const void *fontName, GLbitfield fontStyle,
    GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs,
    GLuint pathParameterTemplate, GLfloat emScale)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PathGlyphRangeNV(firstPathName, fontTarget, fontName, fontStyle,
        firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate,
        emScale);
}

inline void Qt3DSOpenGLES2Extensions::glPathParameterfNV(GLuint path,
    GLenum pname, GLfloat value)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PathParameterfNV(path, pname, value);
}

inline void Qt3DSOpenGLES2Extensions::glPathStencilDepthOffsetNV(GLfloat factor,
    GLfloat units)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PathStencilDepthOffsetNV(factor, units);
}

inline void Qt3DSOpenGLES2Extensions::glStencilFillPathNV(GLuint path,
    GLenum fillMode, GLuint mask)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->StencilFillPathNV(path, fillMode, mask);
}

inline void Qt3DSOpenGLES2Extensions::glStencilStrokePathNV(GLuint path,
    GLint reference, GLuint mask)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->StencilStrokePathNV(path, reference, mask);
}

inline void Qt3DSOpenGLES2Extensions::glStencilFillPathInstancedNV(
    GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase,
    GLenum fillMode, GLuint mask, GLenum transformType,
    const GLfloat *transformValues)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->StencilFillPathInstancedNV(numPaths, pathNameType, paths, pathBase,
        fillMode, mask, transformType, transformValues);
}

inline void Qt3DSOpenGLES2Extensions::glStencilStrokePathInstancedNV(
    GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase,
    GLint reference, GLuint mask, GLenum transformType,
    const GLfloat *transformValues)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->StencilStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase,
        reference, mask, transformType, transformValues);
}

inline void Qt3DSOpenGLES2Extensions::glPathCoverDepthFuncNV(GLenum func)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->PathCoverDepthFuncNV(func);
}

inline void Qt3DSOpenGLES2Extensions::glCoverFillPathInstancedNV(
    GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase,
    GLenum coverMode, GLenum transformType, const GLfloat *transformValues)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->CoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase,
        coverMode, transformType, transformValues);
}

inline void Qt3DSOpenGLES2Extensions::glCoverStrokePathInstancedNV(
    GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase,
    GLenum coverMode, GLenum transformType, const GLfloat *transformValues)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->CoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase,
        coverMode, transformType, transformValues);
}

inline void Qt3DSOpenGLES2Extensions::glGetPathParameterfvNV(GLuint path,
    GLenum pname, GLfloat *value)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->GetPathParameterfvNV(path, pname, value);
}

inline void Qt3DSOpenGLES2Extensions::glGetPathMetricsNV(
    GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType,
    const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->GetPathMetricsNV(metricQueryMask, numPaths, pathNameType, paths,
        pathBase, stride, metrics);
}

inline void Qt3DSOpenGLES2Extensions::glGetPathMetricRangeNV(
    GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths,
    GLsizei stride, GLfloat *metrics)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->GetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, stride,
        metrics);
}

inline void Qt3DSOpenGLES2Extensions::glGetPathSpacingNV(GLenum pathListMode,
    GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase,
    GLfloat advanceScale, GLfloat kerningScale, GLenum transformType,
    GLfloat *returnedSpacing)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->GetPathSpacingNV(pathListMode, numPaths, pathNameType, paths, pathBase,
        advanceScale, kerningScale, transformType, returnedSpacing);
}

inline void Qt3DSOpenGLES2Extensions::glBindVertexArrayOES(GLuint array)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->BindVertexArrayOES(array);
}

inline void Qt3DSOpenGLES2Extensions::glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->DeleteVertexArraysOES(n, arrays);
}

inline void Qt3DSOpenGLES2Extensions::glGenVertexArraysOES(GLsizei n, GLuint *arrays)
{
    Q_D(Qt3DSOpenGLExtensions);
    d->GenVertexArraysOES(n, arrays);
}

inline GLboolean Qt3DSOpenGLES2Extensions::glIsVertexArrayOES(GLuint array)
{
    Q_D(Qt3DSOpenGLExtensions);
    return d->IsVertexArrayOES(array);
}

#endif // QT_OPENGL_ES

#endif // QT3DSOPENGLEXTENSIONS_H
