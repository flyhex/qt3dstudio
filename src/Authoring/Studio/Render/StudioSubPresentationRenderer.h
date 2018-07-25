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
#ifndef QT3DS_STUDIO_SUBPRESENTATION_RENDERER_H
#define QT3DS_STUDIO_SUBPRESENTATION_RENDERER_H

#include "Qt3DSOffscreenRenderManager.h"
#include "Qt3DSRenderContextCore.h"
#include "q3dssurfaceviewer.h"

#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopenglvertexarrayobject.h>
#include <QtGui/qopenglbuffer.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglcontext.h>
#include <QtCore/qscopedpointer.h>

class RendererThread;

class StudioSubpresentationRenderer : public qt3ds::render::IOffscreenRenderer
{
public:
    StudioSubpresentationRenderer(qt3ds::render::IQt3DSRenderContext &inRenderContext,
                                  const QString &id, const QString &presentation,
                                  const QString &path);
    ~StudioSubpresentationRenderer();

    void addRef() override
    {
        qt3ds::render::atomicIncrement(&mRefCount);
    }
    void release() override
    {
        qt3ds::render::QT3DSI32 value = qt3ds::render::atomicDecrement(&mRefCount);
        if (value <= 0)
            qt3ds::render::NVDelete(m_renderContext.GetAllocator(), this);
    }

    qt3ds::render::CRegisteredString GetOffscreenRendererType() override;
    qt3ds::render::SOffscreenRendererEnvironment
    GetDesiredEnvironment(qt3ds::render::QT3DSVec2 inPresentationScaleFactor) override;

    qt3ds::render::SOffscreenRenderFlags NeedsRender(
            const qt3ds::render::SOffscreenRendererEnvironment &inEnvironment,
            qt3ds::render::QT3DSVec2 inPresentationScaleFactor,
            const qt3ds::render::SRenderInstanceId instanceId) override;

    void Render(const qt3ds::render::SOffscreenRendererEnvironment &inEnvironment,
                qt3ds::render::NVRenderContext &inRenderContext,
                qt3ds::render::QT3DSVec2 inPresentationScaleFactor,
                qt3ds::render::SScene::RenderClearCommand inColorBufferNeedsClear,
                const qt3ds::render::SRenderInstanceId instanceId) override;
    void RenderWithClear(const qt3ds::render::SOffscreenRendererEnvironment &inEnvironment,
                         qt3ds::render::NVRenderContext &inRenderContext,
                         qt3ds::render::QT3DSVec2 inPresentationScaleFactor,
                         qt3ds::render::SScene::RenderClearCommand inColorBufferNeedsClear,
                         qt3ds::render::QT3DSVec3 inclearColor,
                         const qt3ds::render::SRenderInstanceId instanceId) override;

    qt3ds::render::IGraphObjectPickQuery *GetGraphObjectPickQuery(
            const qt3ds::render::SRenderInstanceId instanceId) override
    {
        Q_UNUSED(instanceId);
        return nullptr;
    }

    bool Pick(const qt3ds::render::QT3DSVec2 &inMouseCoords,
              const qt3ds::render::QT3DSVec2 &inViewportDimensions,
              const qt3ds::render::SRenderInstanceId instanceId) override
    {
        Q_UNUSED(inMouseCoords);
        Q_UNUSED(inViewportDimensions);
        Q_UNUSED(instanceId);
        return false;
    }

    void addCallback(IOffscreenRendererCallback *cb) override
    {
        m_callback = cb;
    }

private:
    void initializeFboCopy();
    void initialize();

    qt3ds::render::IQt3DSRenderContext &m_renderContext;
    QString m_id;
    QString m_presentation;
    QString m_path;

    QScopedPointer<RendererThread> m_thread;
    QOpenGLShaderProgram *m_program;
    QOpenGLVertexArrayObject *m_vao;
    QOpenGLBuffer *m_vertices;
    IOffscreenRendererCallback *m_callback;
    qt3ds::render::CRegisteredString m_rendererType;
    volatile qt3ds::render::QT3DSI32 mRefCount;
};

#endif
