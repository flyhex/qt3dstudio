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

#ifndef STUDIO_GRADIENT_WIDGET_H
#define STUDIO_GRADIENT_WIDGET_H

#include "Qt3DSCommonPrecompile.h"
#include "StudioWidgetImpl.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "Qt3DSRenderNode.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "StudioUtils.h"

namespace qt3ds {
namespace widgets {

class SGradientWidget
{
    NVAllocatorCallback &m_allocator;
    NVRenderInputAssembler *m_sphere;
    NVRenderShaderProgram *m_shader;
    SNode *m_node;

    volatile QT3DSI32 mRefCount;

public:
    SGradientWidget(NVAllocatorCallback &inAlloc)
        : m_allocator(inAlloc)
        , m_sphere(nullptr)
        , m_shader(nullptr)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_allocator)

    NVConstDataRef<qt3ds::render::NVRenderVertexBufferEntry>
    GetVertexBufferAttributesAndStride(QT3DSU32 &stride);
    NVRenderInputAssembler *CreateSphere(IRenderWidgetContext &inWidgetContext,
                                         NVRenderContext &inRenderContext,
                                         QT3DSF32 radius);

    NVRenderShaderProgram *CreateGradientShader(IRenderWidgetContext &inWidgetContext,
                                                NVRenderContext &inRenderContext);

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext,
                bool fullScreenQuad);

    void SetNode(SNode &inNode)
    {
        m_node = &inNode;
    }

    static SGradientWidget &CreateGradientWidget(NVAllocatorCallback &inAlloc);
};

}
}

#endif // STUDIO_GRADIENT_WIDGET_H
