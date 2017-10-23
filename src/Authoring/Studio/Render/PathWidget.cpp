/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "PathWidget.h"
#include "Qt3DSRenderWidgets.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderInputAssembler.h"
#include "Qt3DSRenderPath.h"
#include "Qt3DSRenderPathSubPath.h"
#include "Qt3DSRenderPathManager.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"

using namespace qt3ds::widgets;

namespace {

QT3DSVec3 toVec3(QT3DSVec2 inPoint)
{
    return QT3DSVec3(inPoint.x, inPoint.y, 0.0f);
}
QT3DSVec3 toVec3(QT3DSVec2 inPoint, float z)
{
    return QT3DSVec3(inPoint.x, inPoint.y, z);
}

struct SPointEntry
{
    QT3DSVec2 m_Position;
    QT3DSVec3 m_Color;
    QT3DSF32 m_ObjectId;
    SPointEntry(QT3DSVec2 pos, QT3DSVec3 color, size_t objId)
        : m_Position(pos)
        , m_Color(color)
        , m_ObjectId((QT3DSF32)objId)
    {
    }
    SPointEntry() {}
};

struct SPathWidget : public IPathWidget
{
    typedef nvvector<eastl::pair<QT3DSU32, qt3ds::studio::SPathPick::EAnchorProperty>>
        TReverseAnchorBuffer;

    NVAllocatorCallback &m_Allocator;
    IQt3DSRenderContext &m_Context;

    NVScopedRefCounted<NVRenderVertexBuffer> m_PointVertexBuffer;
    NVScopedRefCounted<NVRenderInputAssembler> m_PointAssembler;
    NVScopedRefCounted<NVRenderShaderProgram> m_PointShader;
    NVScopedRefCounted<NVRenderShaderProgram> m_PointPickShader;

    NVScopedRefCounted<NVRenderVertexBuffer> m_LineVertexBuffer;
    NVScopedRefCounted<NVRenderInputAssembler> m_LineAssembler;
    NVScopedRefCounted<NVRenderShaderProgram> m_LineShader;

    nvvector<eastl::pair<QT3DSVec2, QT3DSVec2>> m_LineBuffer;
    nvvector<SPointEntry> m_PointBuffer;
    TReverseAnchorBuffer m_AnchorIndexBuffer;
    SWidgetRenderSetupResult m_RenderSetup;
    QT3DSVec2 m_PointViewportDimensions;
    QT3DSMat44 m_PointMVP;

    QT3DSI32 m_RefCount;
    SPathWidget(NVAllocatorCallback &inAlloc, IQt3DSRenderContext &inRc)
        : m_Allocator(inAlloc)
        , m_Context(inRc)
        , m_LineBuffer(inAlloc, "m_LineBuffer")
        , m_PointBuffer(inAlloc, "m_PointBuffer")
        , m_AnchorIndexBuffer(inAlloc, "m_AnchorIndexBuffer")
        , m_LineShader(nullptr)
        , m_RefCount(0)
    {
    }

    void addRef() override { ++m_RefCount; }
    void release() override
    {
        --m_RefCount;
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Allocator);
            NVDelete(alloc, this);
        }
    }

    void SetNode(SNode &inNode) override { m_Node = &inNode; }

    QT3DSVec3 ToGlobalSpace(QT3DSVec3 inPoint) { return m_Node->m_GlobalTransform.transform(inPoint); }

    QT3DSVec3 ToCameraSpace(QT3DSVec3 inPoint)
    {
        QT3DSVec3 theGlobalPos = m_Node->m_GlobalTransform.transform(inPoint);
        return m_RenderSetup.m_WidgetInfo.m_CameraGlobalInverse.transform(theGlobalPos);
    }

    QT3DSVec3 ToCameraSpace(QT3DSVec2 inPoint)
    {
        return ToCameraSpace(QT3DSVec3(inPoint.x, inPoint.y, 0.0f));
    }

    void GeneratePointVertexGeometrySections(IShaderProgramGenerator &inProgramGenerator)
    {
        IShaderStageGenerator &vertex(*inProgramGenerator.GetStage(ShaderGeneratorStages::Vertex));
        IShaderStageGenerator &geometry(
            *inProgramGenerator.GetStage(ShaderGeneratorStages::Geometry));
        vertex.AddIncoming("attr_pos", "vec2");
        vertex.AddIncoming("attr_color", "vec3");
        vertex.AddIncoming("attr_objid", "float");
        vertex.AddOutgoing("point_color", "vec3");
        vertex.AddOutgoing("object_id", "float");
        vertex.AddUniform("model_view_projection", "mat4");
        vertex << "void main()" << Endl << "{" << Endl
               << "\tgl_Position = model_view_projection * vec4( attr_pos.xy, 0.0, 1.0 );" << Endl
               << "\tpoint_color = attr_color;" << Endl << "\tobject_id = uint(attr_objid);" << Endl
               << "}" << Endl;

        geometry.AddUniform("viewport_dimensions", "vec2");
        geometry.AddUniform("pointsize", "float");
        geometry.AddIncoming("point_color", "vec3");
        geometry.AddIncoming("object_id", "float");
        geometry.AddOutgoing("point_colorGE", "vec3");
        geometry.AddOutgoing("object_idGE", "float");
        geometry.AddOutgoing("uv_coords", "vec2");
        geometry.Append("layout (points) in;");
        geometry.Append("layout (triangle_strip, max_vertices = 4) out;");
        geometry.Append(
            "void main() {"
            "// project points to screen space\n"
            "\tvec2 p0 = vec2(gl_in[0].gl_Position.xy) / gl_in[0].gl_Position.w;\n"
            "\tvec2 increments = (pointsize / viewport_dimensions) / 2.0;\n"
            "\tgl_Position = vec4( p0.x - increments.x, p0.y + increments.y, 0.0, 1.0 );\n"
            "\tpoint_colorGE = point_color[0];\n"
            "\tuv_coords = vec2(0.0, 0.0);\n"
            "\tobject_idGE = object_id[0];\n"
            "\tEmitVertex();\n"
            "\tgl_Position = vec4( p0.x + increments.x, p0.y + increments.y, 0.0, 1.0 );\n"
            "\tpoint_colorGE = point_color[0];\n"
            "\tuv_coords = vec2(1.0, 0.0);\n"
            "\tobject_idGE = object_id[0];\n"
            "\tEmitVertex();\n"
            "\tgl_Position = vec4( p0.x - increments.x, p0.y - increments.y, 0.0, 1.0 );\n"
            "\tpoint_colorGE = point_color[0];\n"
            "\tuv_coords = vec2(0.0, 1.0);\n"
            "\tobject_idGE = object_id[0];\n"
            "\tEmitVertex();\n"
            "\tgl_Position = vec4( p0.x + increments.x, p0.y - increments.y, 0.0, 1.0 );\n"
            "\tpoint_colorGE = point_color[0];\n"
            "\tuv_coords = vec2(1.0, 1.0);\n"
            "\tobject_idGE = object_id[0];\n"
            "\tEmitVertex();\n"
            "\tEndPrimitive();\n"
            "}\n");
    }

    NVRenderShaderProgram *GetPointShader(IShaderProgramGenerator &inProgramGenerator)
    {
        if (m_PointShader)
            return m_PointShader.mPtr;
        inProgramGenerator.BeginProgram(IShaderProgramGenerator::DefaultFlags()
                                        | ShaderGeneratorStages::Geometry);
        GeneratePointVertexGeometrySections(inProgramGenerator);

        IShaderStageGenerator &fragment(
            *inProgramGenerator.GetStage(ShaderGeneratorStages::Fragment));
        fragment.AddIncoming("point_colorGE", "vec3");
        fragment.AddIncoming("uv_coords", "vec2");
        fragment.Append("void main()\n"
                        "{\n"
                        "\tvec2 coords = uv_coords - vec2(.5, .5);\n"
                        "\tfloat coordLen = length( coords );\n"
                        "\tfloat margin = .4;\n"
                        "\tfloat leftover = min( 1.0, (coordLen - margin)/.1 );\n"
                        "\tfloat alpha = coordLen < margin ? 1.0 : mix( 1.0, 0.0, leftover );\n"
                        "\tfragOutput = vec4(point_colorGE, alpha);\n"
                        "}\n");

        m_PointShader = inProgramGenerator.CompileGeneratedShader("path widget point shader");
        return m_PointShader.mPtr;
    }

    NVRenderShaderProgram *GetPointPickShader(IShaderProgramGenerator &inProgramGenerator)
    {
        if (m_PointPickShader)
            return m_PointPickShader.mPtr;
        inProgramGenerator.BeginProgram(IShaderProgramGenerator::DefaultFlags()
                                        | ShaderGeneratorStages::Geometry);
        GeneratePointVertexGeometrySections(inProgramGenerator);

        IShaderStageGenerator &fragment(
            *inProgramGenerator.GetStage(ShaderGeneratorStages::Fragment));
        fragment.AddIncoming("object_idGE", "float");
        fragment.AddIncoming("uv_coords", "vec2");
        fragment.Append("void main()\n"
                        "{\n"
                        "\tvec2 coords = uv_coords - vec2(.5, .5);\n"
                        "\tfloat coordLen = length( coords );\n"
                        "\tfloat margin = .4;\n"
                        "\tuint object_id = coordLen < margin ? uint(object_idGE + 1) : uint(0);\n"
                        "\tfragOutput.r = float(object_id % 256)/255.0;\n"
                        "\tfragOutput.g = float(object_id / 256)/255.0;\n"
                        //"\tfragOutput.g = float(object_id) / 10.0;\n"
                        "\tfragOutput.b = 0.0;\n"
                        "\tfragOutput.a = 1.0;\n"
                        "}\n");
        m_PointPickShader =
            inProgramGenerator.CompileGeneratedShader("path widget point pick shader");
        return m_PointPickShader.mPtr;
    }

    NVRenderShaderProgram *GetLineShader(IShaderProgramGenerator &inProgramGenerator)
    {
        if (m_LineShader)
            return m_LineShader.mPtr;

        inProgramGenerator.BeginProgram(IShaderProgramGenerator::DefaultFlags()
                                        | ShaderGeneratorStages::Geometry);

        IShaderStageGenerator &vertex(*inProgramGenerator.GetStage(ShaderGeneratorStages::Vertex));
        IShaderStageGenerator &geometry(
            *inProgramGenerator.GetStage(ShaderGeneratorStages::Geometry));
        IShaderStageGenerator &fragment(
            *inProgramGenerator.GetStage(ShaderGeneratorStages::Fragment));

        vertex.AddIncoming("attr_pos", "vec2");
        vertex.AddUniform("model_view_projection", "mat4");
        vertex << "void main()" << Endl << "{" << Endl
               << "\tgl_Position = model_view_projection * vec4( attr_pos.xy, 0.0, 1.0 );" << Endl
               << "}" << Endl;

        geometry.AddUniform("viewport_dimensions", "vec2");
        geometry.AddUniform("linewidth", "float");
        geometry.AddOutgoing("uv_coords", "vec2");
        geometry.Append(
            "layout (lines) in;\n"
            "layout (triangle_strip, max_vertices = 4) out;\n"
            "void main()\n"
            "{\n"
            "\tvec2 p0 = vec2(gl_in[0].gl_Position.xy) / gl_in[0].gl_Position.w;\n"
            "\tvec2 p1 = vec2(gl_in[1].gl_Position.xy) / gl_in[1].gl_Position.w;\n"
            "\tvec2 slope = normalize( p1 - p0 );\n"
            "\tvec2 tangent = vec2(slope.y, -slope.x);\n"
            "\tvec2 increments = (linewidth / viewport_dimensions) / 2.0;\n"
            "\tvec2 tangentVec = vec2( tangent.x  * increments.x, tangent.y * increments.y );\n"
            "\tgl_Position = vec4( p0 - tangentVec, 0.0, 1.0);\n"
            "\tuv_coords = vec2(0.0, 0.0);\n"
            "\tEmitVertex();\n"
            "\tgl_Position = vec4( p0 + tangentVec, 0.0, 1.0);\n"
            "\tuv_coords = vec2(1.0, 0.0);\n"
            "\tEmitVertex();\n"
            "\tgl_Position = vec4( p1 - tangentVec, 0.0, 1.0);\n"
            "\tuv_coords = vec2(0.0, 1.0);\n"
            "\tEmitVertex();\n"
            "\tgl_Position = vec4( p1 + tangentVec, 0.0, 1.0);\n"
            "\tuv_coords = vec2(1.0, 1.0);\n"
            "\tEmitVertex();\n"
            "\tEndPrimitive();\n"
            "}\n");

        fragment.AddUniform("line_color", "vec3");
        fragment.AddIncoming("uv_coords", "vec2");
        fragment.Append("void main()\n"
                        "{\n"
                        "\tvec2 coords = uv_coords - vec2(.5, .5);\n"
                        "\tfloat coordLen = abs(coords.x);\n"
                        "\tfloat margin = .1;\n"
                        "\tfloat leftover = min( 1.0, (coordLen - margin)/.3 );\n"
                        "\tleftover = leftover * leftover;\n"
                        "\tfloat alpha = coordLen < margin ? 1.0 : mix( 1.0, 0.0, leftover );\n"
                        "\tfragOutput = vec4(line_color, alpha);\n"
                        "}\n");

        m_LineShader = inProgramGenerator.CompileGeneratedShader("path widget line shader");
        return m_LineShader.mPtr;
    }

    void RenderPointBuffer(NVRenderShaderProgram &inProgram, const QT3DSMat44 &inMVP,
                           NVRenderContext &inRenderContext)
    {
        inRenderContext.SetCullingEnabled(false);
        inRenderContext.SetDepthTestEnabled(false);
        inRenderContext.SetDepthWriteEnabled(false);
        inRenderContext.SetStencilTestEnabled(false);
        inRenderContext.SetBlendingEnabled(true);
        inRenderContext.SetBlendFunction(qt3ds::render::NVRenderBlendFunctionArgument(
            qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
            qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
            qt3ds::render::NVRenderSrcBlendFunc::One,
            qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha));
        inRenderContext.SetBlendEquation(qt3ds::render::NVRenderBlendEquationArgument(
            NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));
        inRenderContext.SetActiveShader(&inProgram);
        inProgram.SetPropertyValue("model_view_projection", inMVP);
        inProgram.SetPropertyValue("pointsize", (QT3DSF32)15.0f);
        inProgram.SetPropertyValue("viewport_dimensions", m_PointViewportDimensions);
        inRenderContext.SetInputAssembler(m_PointAssembler);
        inRenderContext.Draw(NVRenderDrawMode::Points, m_PointBuffer.size(), 0);
    }

    void PushPoint(const SPointEntry &inEntry, QT3DSU32 inAnchorIndex,
                   qt3ds::studio::SPathPick::EAnchorProperty inProperty)
    {
        QT3DSU32 anchorIndex = (QT3DSU32)m_PointBuffer.size();
        m_PointBuffer.push_back(inEntry);
        m_AnchorIndexBuffer.push_back(eastl::make_pair(inAnchorIndex, inProperty));
    }

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext) override
    {
        if (!m_Node)
            return;
        SPath &thePath = static_cast<SPath &>(*m_Node);
        IPathManager &theManager = m_Context.GetPathManager();

        QT3DSVec3 anchorColor(0, 1, 0);
        QT3DSVec3 controlColor(0, 0, 1);
        m_LineBuffer.clear();
        m_PointBuffer.clear();
        // point index -> anchor index
        m_AnchorIndexBuffer.clear();
        QT3DSU32 anchorIndex = 0;

        for (SPathSubPath *theSubPath = thePath.m_FirstSubPath; theSubPath;
             theSubPath = theSubPath->m_NextSubPath) {
            NVDataRef<qt3ds::render::SPathAnchorPoint> thePathBuffer(
                theManager.GetPathSubPathBuffer(*theSubPath));
            if (thePathBuffer.size() == 0)
                return;

            QT3DSU32 numAnchors = thePathBuffer.size();
            for (QT3DSU32 idx = 0, end = numAnchors; idx < end; ++idx) {
                const qt3ds::render::SPathAnchorPoint &theAnchorPoint(thePathBuffer[idx]);
                if (idx > 0) {
                    QT3DSVec2 incoming(qt3ds::render::IPathManagerCore::GetControlPointFromAngleDistance(
                        theAnchorPoint.m_Position, theAnchorPoint.m_IncomingAngle,
                        theAnchorPoint.m_IncomingDistance));
                    PushPoint(SPointEntry(incoming, controlColor, m_PointBuffer.size()),
                              anchorIndex, qt3ds::studio::SPathPick::IncomingControl);
                    m_LineBuffer.push_back(eastl::make_pair(theAnchorPoint.m_Position, incoming));
                }
                PushPoint(SPointEntry(theAnchorPoint.m_Position, anchorColor, m_PointBuffer.size()),
                          anchorIndex, qt3ds::studio::SPathPick::Anchor);
                if (idx < (numAnchors - 1)) {
                    QT3DSVec2 outgoing(qt3ds::render::IPathManagerCore::GetControlPointFromAngleDistance(
                        theAnchorPoint.m_Position, theAnchorPoint.m_OutgoingAngle,
                        theAnchorPoint.m_OutgoingDistance));
                    PushPoint(SPointEntry(outgoing, controlColor, m_PointBuffer.size()),
                              anchorIndex, qt3ds::studio::SPathPick::OutgoingControl);
                    m_LineBuffer.push_back(eastl::make_pair(theAnchorPoint.m_Position, outgoing));
                }
                ++anchorIndex;
            }
        }

        m_RenderSetup = SWidgetRenderSetupResult(inWidgetContext, *m_Node,
                                                 qt3ds::render::RenderWidgetModes::Local);

        m_PointMVP = m_RenderSetup.m_WidgetInfo.m_LayerProjection
            * m_RenderSetup.m_WidgetInfo.m_CameraGlobalInverse * m_Node->m_GlobalTransform;

        NVRenderRect theViewport = inRenderContext.GetViewport();
        m_PointViewportDimensions = QT3DSVec2((QT3DSF32)theViewport.m_Width, (QT3DSF32)theViewport.m_Height);

        if (!m_LineBuffer.empty()) {
            QT3DSU32 vertItemSize = sizeof(QT3DSVec2);
            QT3DSU32 vertBufSize = m_LineBuffer.size() * vertItemSize * 2;
            if ((!m_LineVertexBuffer) || m_LineVertexBuffer->Size() < vertBufSize) {
                m_LineVertexBuffer = inRenderContext.CreateVertexBuffer(
                    qt3ds::render::NVRenderBufferUsageType::Dynamic, vertBufSize, vertItemSize,
                    qt3ds::foundation::toU8DataRef(m_LineBuffer.data(), (QT3DSU32)m_LineBuffer.size()));
                m_LineAssembler = nullptr;
            } else
                m_LineVertexBuffer->UpdateBuffer(
                    qt3ds::foundation::toU8DataRef(m_LineBuffer.data(), (QT3DSU32)m_LineBuffer.size()));

            if (m_LineAssembler == nullptr) {
                qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                    qt3ds::render::NVRenderVertexBufferEntry(
                        "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 0),
                };
                NVRenderAttribLayout *theAttribLayout =
                    &inWidgetContext.CreateAttributeLayout(toConstDataRef(theEntries, 1));
                m_LineAssembler = inRenderContext.CreateInputAssembler(
                    theAttribLayout, toConstDataRef(&m_LineVertexBuffer.mPtr, 1), nullptr,
                    toConstDataRef(vertItemSize), toConstDataRef((QT3DSU32)0));
            }
            inRenderContext.SetInputAssembler(m_LineAssembler);
            NVRenderShaderProgram *lineShader =
                GetLineShader(inWidgetContext.GetProgramGenerator());
            if (lineShader) {
                inRenderContext.SetCullingEnabled(false);
                inRenderContext.SetDepthTestEnabled(false);
                inRenderContext.SetDepthWriteEnabled(false);
                inRenderContext.SetStencilTestEnabled(false);
                inRenderContext.SetBlendingEnabled(true);
                inRenderContext.SetBlendFunction(qt3ds::render::NVRenderBlendFunctionArgument(
                    qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
                    qt3ds::render::NVRenderSrcBlendFunc::One,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha));
                inRenderContext.SetBlendEquation(qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));
                inRenderContext.SetActiveShader(lineShader);
                lineShader->SetPropertyValue("model_view_projection", m_PointMVP);
                lineShader->SetPropertyValue("line_color", QT3DSVec3(1.0, 1.0, 0.0));
                // Note the line needs to be wide enough to account for anti-aliasing.
                lineShader->SetPropertyValue("linewidth", 3.0f);
                lineShader->SetPropertyValue("viewport_dimensions", m_PointViewportDimensions);
                inRenderContext.Draw(NVRenderDrawMode::Lines, m_LineBuffer.size() * 2, 0);
            }
        }
        {
            QT3DSU32 vertItemSize = (sizeof(SPointEntry));
            QT3DSU32 vertBufSize = m_PointBuffer.size() * vertItemSize;
            if ((!m_PointVertexBuffer) || m_PointVertexBuffer->Size() < vertBufSize) {
                m_PointVertexBuffer = inRenderContext.CreateVertexBuffer(
                    qt3ds::render::NVRenderBufferUsageType::Dynamic, vertBufSize, vertItemSize,
                    qt3ds::foundation::toU8DataRef(m_PointBuffer.data(), (QT3DSU32)m_PointBuffer.size()));
                m_PointAssembler = nullptr;
            } else
                m_PointVertexBuffer->UpdateBuffer(
                    qt3ds::foundation::toU8DataRef(m_PointBuffer.data(), (QT3DSU32)m_PointBuffer.size()));
            if (m_PointAssembler == nullptr) {
                qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                    qt3ds::render::NVRenderVertexBufferEntry(
                        "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 0),
                    qt3ds::render::NVRenderVertexBufferEntry(
                        "attr_color", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3, 8),
                    qt3ds::render::NVRenderVertexBufferEntry(
                        "attr_objid", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 1, 20),
                };
                NVRenderAttribLayout *theAttribLayout =
                    &inWidgetContext.CreateAttributeLayout(toConstDataRef(theEntries, 3));
                m_PointAssembler = inRenderContext.CreateInputAssembler(
                    theAttribLayout, toConstDataRef(&m_PointVertexBuffer.mPtr, 1), nullptr,
                    toConstDataRef(vertItemSize), toConstDataRef((QT3DSU32)0));
            }
            NVRenderShaderProgram *thePointShader =
                GetPointShader(inWidgetContext.GetProgramGenerator());
            GetPointPickShader(inWidgetContext.GetProgramGenerator());

            if (thePointShader) {
                m_PointMVP = m_RenderSetup.m_WidgetInfo.m_LayerProjection
                    * m_RenderSetup.m_WidgetInfo.m_CameraGlobalInverse * m_Node->m_GlobalTransform;

                RenderPointBuffer(*m_PointShader, m_PointMVP, inRenderContext);
            }
        }
    }

    void RenderPick(const QT3DSMat44 &inProjPreMult, NVRenderContext &inRenderContext,
                            qt3ds::render::SWindowDimensions inWinDimensions) override
    {
        if (m_PointAssembler == nullptr || m_PointPickShader == nullptr)
            return;
        // The projection premultiplication step moves the viewport around till
        // it is centered over the mouse and scales everything *post* rendering (to keep appropriate
        // aspect).
        QT3DSMat44 theMVP = inProjPreMult * m_PointMVP;
        m_PointViewportDimensions =
            QT3DSVec2((QT3DSF32)inWinDimensions.m_Width, (QT3DSF32)inWinDimensions.m_Height);

        RenderPointBuffer(*m_PointPickShader, theMVP, inRenderContext);
    }

    qt3ds::studio::SStudioPickValue PickIndexToPickValue(QT3DSU32 inPickIndex) override
    {
        inPickIndex -= 1;

        if (inPickIndex < m_AnchorIndexBuffer.size())
            return qt3ds::studio::SPathPick(m_AnchorIndexBuffer[inPickIndex].first,
                                          m_AnchorIndexBuffer[inPickIndex].second);
        else {
            QT3DS_ASSERT(false);
            return qt3ds::studio::SPathPick();
        }
    }
};
}

IPathWidget &IPathWidget::CreatePathWidget(NVAllocatorCallback &inCallback, IQt3DSRenderContext &inRc)
{
    return *QT3DS_NEW(inCallback, SPathWidget)(inCallback, inRc);
}
