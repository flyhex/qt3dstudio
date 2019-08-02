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
#include "StudioHelperGridWidget.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include <QtGui/qcolor.h>

using namespace qt3ds::render;

namespace qt3ds {
namespace widgets {

SHelperGridWidget::SHelperGridWidget(NVAllocatorCallback &inAlloc)
    : IRenderWidget()
    , m_rotation(QT3DSMat44::createIdentity())
    , m_dirty(true)
    , m_allocator(inAlloc)
    , mRefCount(0)
{
}

void SHelperGridWidget::setNode(SNode *node)
{
    m_Node = node;
}

void SHelperGridWidget::rotate(float angleRadians, const QT3DSVec3 &axis)
{
    m_rotation = QT3DSMat44::createIdentity();
    if (angleRadians != 0.f)
        m_rotation.rotate(angleRadians, axis);
}

void SHelperGridWidget::setColors(const QColor &gridColor, const QColor &xColor,
                                  const QColor &yColor)
{
    QT3DSVec3 theGridColor = QT3DSVec3(gridColor.redF(), gridColor.greenF(), gridColor.blueF());
    QT3DSVec3 theXColor = QT3DSVec3(xColor.redF(), xColor.greenF(), xColor.blueF());
    QT3DSVec3 theYColor = QT3DSVec3(yColor.redF(), yColor.greenF(), yColor.blueF());

    if (theGridColor != m_gridColor || theXColor != m_xColor || theYColor != m_yColor)
        m_dirty = true;

    m_gridColor = theGridColor;
    m_xColor = theXColor;
    m_yColor = theYColor;
}

void SHelperGridWidget::setLines(int count, float spacing)
{
    if (count != m_lineCount || spacing != m_lineSpacing)
        m_dirty = true;

    m_lineCount = count;
    m_lineSpacing = spacing;
}

void SHelperGridWidget::setupShader(IRenderWidgetContext &context)
{
    m_shader = context.GetShader(m_itemName);
    if (!m_shader) {
        qt3ds::render::IShaderProgramGenerator &generator(context.GetProgramGenerator());
        generator.BeginProgram();
        qt3ds::render::IShaderStageGenerator &vertexGenerator(
            *generator.GetStage(qt3ds::render::ShaderGeneratorStages::Vertex));
        qt3ds::render::IShaderStageGenerator &fragmentGenerator(
            *generator.GetStage(qt3ds::render::ShaderGeneratorStages::Fragment));
        vertexGenerator.AddIncoming("attr_pos", "vec3");
        vertexGenerator.AddIncoming("attr_color", "vec3");
        vertexGenerator.AddOutgoing("output_color", "vec3");
        vertexGenerator.AddUniform("model_view_projection", "mat4");
        vertexGenerator.Append("void main() {");
        vertexGenerator.Append(
            "\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
        vertexGenerator.Append("\toutput_color = attr_color;");
        vertexGenerator.Append("}");
        fragmentGenerator.Append("void main() {");
        fragmentGenerator.Append("\tgl_FragColor.rgb = output_color;");
        fragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
        fragmentGenerator.Append("}");
        m_shader = context.CompileAndStoreShader(m_itemName);
    }
}

void SHelperGridWidget::setupGraphicsObjects(IRenderWidgetContext &context,
                                             NVDataRef<QT3DSVec3> lines)
{
    qt3ds::render::NVRenderVertexBufferEntry entries[] = {
        qt3ds::render::NVRenderVertexBufferEntry(
                "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
        qt3ds::render::NVRenderVertexBufferEntry(
                "attr_color", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3, 12),
    };

    NVRenderVertexBuffer *vertexBuffer = &context.GetOrCreateVertexBuffer(
        m_itemName, 6 * sizeof(QT3DSF32), toU8DataRef(lines.begin(), lines.size()));
    m_inputAssembler = context.GetInputAssembler(m_itemName);
    if (!m_inputAssembler && vertexBuffer) {
        // create our attribute layout
        NVRenderAttribLayout *attribLayout
            = &context.CreateAttributeLayout(toConstDataRef(entries, 2));

        QT3DSU32 strides = vertexBuffer->GetStride();
        QT3DSU32 offsets = 0;
        m_inputAssembler = &context.GetOrCreateInputAssembler(
            m_itemName, attribLayout, toConstDataRef(&vertexBuffer, 1), nullptr,
            toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    }
}

void SHelperGridWidget::Render(IRenderWidgetContext &widgetContext, NVRenderContext &renderContext)
{
    m_itemName = renderContext.GetStringTable().RegisterStr("SHelperGridWidget");

    setupShader(widgetContext);

    if (m_shader) {
        SWidgetRenderInformation theInfo(widgetContext.GetWidgetRenderInformation(
            *m_Node, QT3DSVec3(0), RenderWidgetModes::Local));

        QT3DSMat44 nodeToCamera = theInfo.m_NodeParentToCamera * m_Node->m_LocalTransform
                * m_rotation;

        if (m_dirty) {
            m_dirty = false;
            // Line data is line end points and color for each point
            const int lineCount = m_lineCount * 2 + 1;
            const int centerLineIndex = m_lineCount;
            const float lineStart = float(m_lineCount) * -m_lineSpacing;
            const int totalCount = lineCount * 8;
            m_lineData.resize(totalCount);
            // Create grid of lines along y-plane. Center lines are colored according to axes.
            for (int i = 0; i < lineCount; ++i) {
                int idx = i * 8;
                m_lineData[idx] = QT3DSVec3(lineStart + m_lineSpacing * i, 0.f, lineStart);
                m_lineData[idx + 2] = QT3DSVec3(lineStart + m_lineSpacing * i, 0.f, -lineStart);
                m_lineData[idx + 4] = QT3DSVec3(lineStart, 0.f, lineStart + m_lineSpacing * i);
                m_lineData[idx + 6] = QT3DSVec3(-lineStart, 0.f, lineStart + m_lineSpacing * i);
                if (i == centerLineIndex) {
                    m_lineData[idx + 1] = m_yColor;
                    m_lineData[idx + 3] = m_yColor;
                    m_lineData[idx + 5] = m_xColor;
                    m_lineData[idx + 7] = m_xColor;
                } else {
                    m_lineData[idx + 1] = m_gridColor;
                    m_lineData[idx + 3] = m_gridColor;
                    m_lineData[idx + 5] = m_gridColor;
                    m_lineData[idx + 7] = m_gridColor;
                }
            }
            setupGraphicsObjects(widgetContext,
                                 toDataRef(static_cast<QT3DSVec3 *>(m_lineData.data()),
                                           QT3DSU32(totalCount)));
        }

        if (m_inputAssembler) {
            renderContext.SetBlendingEnabled(false);
            renderContext.SetDepthWriteEnabled(true);
            renderContext.SetDepthTestEnabled(true);
            renderContext.SetCullingEnabled(false);
            renderContext.SetActiveShader(m_shader);
            m_shader->SetPropertyValue("model_view_projection",
                                       theInfo.m_LayerProjection * nodeToCamera);
            renderContext.SetInputAssembler(m_inputAssembler);
            renderContext.Draw(qt3ds::render::NVRenderDrawMode::Lines, m_lineCount * 8 + 8, 0);
        }
    }
}

SHelperGridWidget &SHelperGridWidget::createHelperGridWidget(NVAllocatorCallback &alloc)
{
    return *QT3DS_NEW(alloc, SHelperGridWidget)(alloc);
}

}
}
