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
#ifndef QT3DS_STUDIO_VISUAL_AID_WIDGET_H
#define QT3DS_STUDIO_VISUAL_AID_WIDGET_H

#include "StudioWidget.h"
#include "foundation/Qt3DSContainers.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include "Qt3DSRenderNode.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"


namespace qt3ds {
namespace widgets {

struct SVisualAidWidget
{
    SNode *m_node;
    NVScopedRefCounted<NVRenderInputAssembler> m_billboard;
    NVScopedRefCounted<NVRenderInputAssembler> m_cameraBox;
    NVScopedRefCounted<NVRenderInputAssembler> m_directionalLight;
    NVScopedRefCounted<NVRenderInputAssembler> m_pointLight;
    NVScopedRefCounted<NVRenderInputAssembler> m_areaLight;
    NVScopedRefCounted<NVRenderShaderProgram> m_renderCameraShader;
    NVScopedRefCounted<NVRenderShaderProgram> m_renderShader;
    NVScopedRefCounted<NVRenderShaderProgram> m_billboardShader;
    NVScopedRefCounted<NVRenderTexture2D> m_billboardCameraTexture;
    NVScopedRefCounted<NVRenderTexture2D> m_billboardLightTexture;

    bool m_selected;
    NVAllocatorCallback &m_allocator;
    volatile QT3DSI32 mRefCount;

    SVisualAidWidget(NVAllocatorCallback &inAlloc);

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_allocator)

    NVConstDataRef<NVRenderVertexBufferEntry> getCameraBoxAttributesAndStride(QT3DSU32 &stride);
    NVConstDataRef<NVRenderVertexBufferEntry> getLightAttributesAndStride(QT3DSU32 &stride);
    NVConstDataRef<NVRenderVertexBufferEntry> getBillboardAttributesAndStride(QT3DSU32 &stride);
    void renderCamera(SNode *node, IRenderWidgetContext &inWidgetContext,
                      NVRenderContext &inRenderContext);
    void renderLight(SNode *node, IRenderWidgetContext &inWidgetContext,
                     NVRenderContext &inRenderContext);
    void renderBillboard(SNode *node, IRenderWidgetContext &inWidgetContext,
                         NVRenderContext &inRenderContext);
    void SetNode(SNode *inNode);
    NVRenderShaderProgram *createRenderShader(IRenderWidgetContext &inWidgetContext,
                                              NVRenderContext &inRenderContext);
    NVRenderShaderProgram *createBillboardShader(IRenderWidgetContext &inWidgetContext,
                                                 NVRenderContext &inRenderContext);
    NVRenderShaderProgram *createRenderCameraShader(IRenderWidgetContext &inWidgetContext,
                                                    NVRenderContext &inRenderContext);
    NVRenderInputAssembler *createCameraBox(IRenderWidgetContext &inWidgetContext,
                                            NVRenderContext &inRenderContext);
    NVRenderInputAssembler *createBillboard(IRenderWidgetContext &inWidgetContext,
                                            NVRenderContext &inRenderContext);
    NVRenderInputAssembler *createDirectionalLight(IRenderWidgetContext &inWidgetContext,
                                                   NVRenderContext &inRenderContext);
    NVRenderInputAssembler *createPointLight(IRenderWidgetContext &inWidgetContext,
                                             NVRenderContext &inRenderContext);
    NVRenderInputAssembler *createAreaLight(IRenderWidgetContext &inWidgetContext,
                                            NVRenderContext &inRenderContext);
    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext);

    bool pick(IRenderWidgetContext &inWidgetContext, float &dist,
              QT3DSVec2 viewport, QT3DSVec2 pos);

    void setSelected(bool selected)
    {
        m_selected = selected;
    }

    static SVisualAidWidget &CreateVisualAidWidget(NVAllocatorCallback &inAlloc);
};

}
}


#endif // QT3DS_STUDIO_VISUAL_AID_WIDGET_H
