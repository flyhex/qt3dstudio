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

#include "stdafx.h"
#include "StudioGradientWidget.h"

namespace qt3ds {
namespace widgets {

using namespace qt3ds::render;

NVConstDataRef<NVRenderVertexBufferEntry>
SGradientWidget::GetVertexBufferAttributesAndStride(QT3DSU32 &stride)
{
    static NVRenderVertexBufferEntry theEntries[] = {
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3)
    };

    stride = 3 * sizeof(QT3DSF32);

    return toConstDataRef(theEntries, 1);
}

NVRenderInputAssembler *SGradientWidget::CreateSphere(IRenderWidgetContext &inWidgetContext,
                                                      NVRenderContext &inRenderContext,
                                                      QT3DSF32 radius)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr("Gradient");
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval)
        return retval;

    nvvector<QT3DSVec3> theVertexData(m_allocator, "SGradientWidget::theVertexData");
    nvvector<QT3DSI8> theIndexData(m_allocator, "SGradientWidget::theIndexData");

    // the sphere is just a box
    theVertexData.push_back(QT3DSVec3(-radius, radius, -radius));
    theVertexData.push_back(QT3DSVec3(radius, radius, -radius));
    theVertexData.push_back(QT3DSVec3(radius, -radius, -radius));
    theVertexData.push_back(QT3DSVec3(-radius, -radius, -radius));
    theVertexData.push_back(QT3DSVec3(-radius, radius, radius));
    theVertexData.push_back(QT3DSVec3(radius, radius, radius));
    theVertexData.push_back(QT3DSVec3(radius, -radius, radius));
    theVertexData.push_back(QT3DSVec3(-radius, -radius, radius));

    theIndexData.push_back(0); theIndexData.push_back(1); theIndexData.push_back(2);
    theIndexData.push_back(0); theIndexData.push_back(2); theIndexData.push_back(3);

    theIndexData.push_back(1); theIndexData.push_back(5); theIndexData.push_back(6);
    theIndexData.push_back(1); theIndexData.push_back(6); theIndexData.push_back(2);

    theIndexData.push_back(5); theIndexData.push_back(4); theIndexData.push_back(7);
    theIndexData.push_back(5); theIndexData.push_back(7); theIndexData.push_back(6);

    theIndexData.push_back(4); theIndexData.push_back(0); theIndexData.push_back(3);
    theIndexData.push_back(4); theIndexData.push_back(3); theIndexData.push_back(7);

    theIndexData.push_back(4); theIndexData.push_back(5); theIndexData.push_back(1);
    theIndexData.push_back(4); theIndexData.push_back(1); theIndexData.push_back(0);

    theIndexData.push_back(6); theIndexData.push_back(7); theIndexData.push_back(3);
    theIndexData.push_back(6); theIndexData.push_back(3); theIndexData.push_back(2);

    QT3DSU32 stride;
    QT3DSU32 offset = 0;
    NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
        GetVertexBufferAttributesAndStride(stride));
    NVRenderVertexBuffer *theVertexBuffer = &inWidgetContext.GetOrCreateVertexBuffer(
        theItemName, stride, toU8DataRef(theVertexData.begin(), theVertexData.size()));
    NVRenderIndexBuffer *theIndexBuffer = &inWidgetContext.GetOrCreateIndexBuffer(
        theItemName, NVRenderComponentTypes::QT3DSU8, theIndexData.size(),
        toU8DataRef(theIndexData.begin(), theIndexData.size()));
    retval = &inWidgetContext.GetOrCreateInputAssembler(
        theItemName, theAttribLayout, toConstDataRef(&theVertexBuffer, 1), theIndexBuffer,
        toConstDataRef(&stride, 1), toConstDataRef(&offset, 1));

    return retval;
}

NVRenderShaderProgram *SGradientWidget::CreateGradientShader(IRenderWidgetContext &inWidgetContext,
                                                             NVRenderContext &inRenderContext)
{
    CRegisteredString itemName = inRenderContext.GetStringTable().RegisterStr("GradientShader");
    NVRenderShaderProgram *retval = inWidgetContext.GetShader(itemName);
    if (retval)
        return retval;

    IShaderProgramGenerator &generator(inWidgetContext.GetProgramGenerator());
    generator.BeginProgram();
    IShaderStageGenerator &vertGenerator(
        *generator.GetStage(ShaderGeneratorStages::Vertex));
    IShaderStageGenerator &fragGenerator(
        *generator.GetStage(ShaderGeneratorStages::Fragment));
    vertGenerator.AddIncoming("attr_pos", "vec3");
    vertGenerator.AddOutgoing("pos", "vec3");
    vertGenerator.AddUniform("normalMatrix", "mat3");
    vertGenerator.AddUniform("projectionMatrix", "mat4");
    vertGenerator.AddUniform("drawMode", "int");
    // These are required in order to scale the scale widget the way we want to scale it.
    vertGenerator.Append("void main() {");
    vertGenerator.Append("\tpos = attr_pos;");
    vertGenerator.Append("\tif (drawMode == 0)");
    vertGenerator.Append("\t\tgl_Position = projectionMatrix * vec4(normalMatrix * pos, 1.0);");
    vertGenerator.Append("\telse\n\t\tgl_Position = vec4(pos.x > 0 ? 1.0 : -1.0, pos.y > 0 \
                          ? 1.0 : -1.0, 0.0, 1.0);");
    vertGenerator.Append("}");
    fragGenerator.AddIncoming("pos", "vec3");
    fragGenerator.AddUniform("color0", "vec3");
    fragGenerator.AddUniform("color1", "vec3");
    fragGenerator.AddUniform("color2", "vec3");
    fragGenerator.AddUniform("color3", "vec3");
    fragGenerator.AddUniform("drawMode", "int");
    fragGenerator.Append("void main() {");
    fragGenerator.Append("\tvec3 npos = normalize(pos);");
    fragGenerator.Append("\tvec3 color = vec3(0.0);");
    fragGenerator.Append("\tif (drawMode == 0) {");
    fragGenerator.Append("\t\tif (npos.y > 0.0)");
    fragGenerator.Append("\t\t\tcolor = mix(color1, color0, pow(npos.y, 0.25));");
    fragGenerator.Append("\t\telse\n\t\t\tcolor = mix(color3, color2, pow(-npos.y, 0.5));");
    fragGenerator.Append("\t} else {\n\t\tcolor = mix(color3, color0, 0.5 * npos.y + 0.5);\n\t}");
    fragGenerator.Append("\tgl_FragColor.rgb = color;");
    fragGenerator.Append("\tgl_FragColor.a = 1.0;");
    fragGenerator.Append("}");
    return inWidgetContext.CompileAndStoreShader(itemName);
}

void SGradientWidget::Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext,
                             bool fullScreenQuad)
{
    if (m_sphere == nullptr) {
        m_sphere = CreateSphere(inWidgetContext, inRenderContext, 100.0f);
        m_shader = CreateGradientShader(inWidgetContext, inRenderContext);

        inRenderContext.SetActiveShader(m_shader);
        m_shader->SetPropertyValue("color0", QT3DSVec3(0.6, 0.6, 0.6));
        m_shader->SetPropertyValue("color1", QT3DSVec3(0.4, 0.4, 0.4));
        m_shader->SetPropertyValue("color2", QT3DSVec3(0.1, 0.1, 0.1));
        m_shader->SetPropertyValue("color3", QT3DSVec3(0.35, 0.35, 0.35));
    }

    inRenderContext.SetDepthWriteEnabled(false);
    inRenderContext.SetDepthTestEnabled(false);
    inRenderContext.SetBlendingEnabled(false);
    inRenderContext.SetCullingEnabled(false);
    inRenderContext.SetActiveShader(m_shader);

    if (fullScreenQuad) {
        // draw fullscreen quad
        m_shader->SetPropertyValue("drawMode", 1);
        inRenderContext.SetInputAssembler(m_sphere);
        inRenderContext.Draw(NVRenderDrawMode::Triangles, 6, 0);
    } else {
        // draw sphere
        SWidgetRenderInformation info
                = inWidgetContext.GetWidgetRenderInformation(*m_node, QT3DSVec3(),
                                                             RenderWidgetModes::Global);
        m_shader->SetPropertyValue("drawMode", 0);
        m_shader->SetPropertyValue("normalMatrix", info.m_NormalMatrix);
        m_shader->SetPropertyValue("projectionMatrix", info.m_PureProjection);

        inRenderContext.SetInputAssembler(m_sphere);

        inRenderContext.Draw(NVRenderDrawMode::Triangles,
                             m_sphere->GetIndexCount(), 0);
    }
}


SGradientWidget &SGradientWidget::CreateGradientWidget(NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, SGradientWidget)(inAlloc);
}

}
}
