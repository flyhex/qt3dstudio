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
#ifndef QT3DS_STUDIO_RENDERER_WIDGET_IMPL_H
#define QT3DS_STUDIO_RENDERER_WIDGET_IMPL_H
#pragma once

#include "StudioWidget.h"
#include "foundation/Qt3DSContainers.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include "Qt3DSRenderNode.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "Qt3DSRenderCamera.h"
#include "StudioPreferences.h"

namespace qt3ds {
namespace widgets {

    typedef nvvector<QT3DSVec4> TResultVecType;

    struct SRotationWedge
    {
        QT3DSVec3 m_StartDirection; // world space position
        QT3DSVec3 m_RotationAxis;
        QT3DSF32 m_Angle; // angle in radians.
        QT3DSF32 m_EndLineLen;
        SRotationWedge() {}
        SRotationWedge(const QT3DSVec3 &inStartDirection, const QT3DSVec3 &inRotationAxis, QT3DSF32 inAngle,
                       QT3DSF32 inEndLineLen)
            : m_StartDirection(inStartDirection)
            , m_RotationAxis(inRotationAxis)
            , m_Angle(inAngle)
            , m_EndLineLen(inEndLineLen)
        {
        }
    };

    struct SImmediateVertex
    {
        QT3DSVec3 m_Position;
        QT3DSVec4 m_Color;
        SImmediateVertex(const QT3DSVec3 &inPosition, const QT3DSVec4 &inColor)
            : m_Position(inPosition)
            , m_Color(inColor)
        {
        }
        SImmediateVertex() {}
    };

    template <StudioWidgetTypes::Enum TWidgetType>
    struct SStudioWidgetImpl : public IStudioWidget
    {
        NVAllocatorCallback &m_Allocator;
        NVRenderShaderProgram *m_Shader;
        NVRenderShaderProgram *m_PickShader;
        QT3DSMat44 m_TranslationScale;
        QT3DSMat44 m_CameraTranslationScale;
        QT3DSMat44 m_PureProjection;
        SWidgetRenderInformation m_WidgetInfo;
        StudioWidgetComponentIds::Enum m_Highlight;
        RenderWidgetModes::Enum m_WidgetMode;

        QT3DSVec3 m_AxisScale;
        Option<SRotationWedge> m_RotationWedge;
        nvvector<SImmediateVertex> m_ImmediateBuffer;
        NVRenderVertexBuffer *m_ImmediateVertexBuffer;
        NVRenderInputAssembler *m_ImmediateInputAssembler;
        NVRenderShaderProgram *m_ImmediateShader;

        SStudioWidgetImpl(NVAllocatorCallback &inAlloc)
            : m_Allocator(inAlloc)
            , m_Shader(NULL)
            , m_PickShader(NULL)
            , m_Highlight(StudioWidgetComponentIds::NoId)
            , m_WidgetMode(RenderWidgetModes::Local)
            , m_AxisScale(QT3DSVec3(1, 1, 1))
            , m_ImmediateBuffer(m_Allocator, "STranslationWidget::theVertexData")
            , m_ImmediateVertexBuffer(NULL)
            , m_ImmediateInputAssembler(NULL)
            , m_ImmediateShader(NULL)
        {
        }

        void SetNode(SNode &inNode) override { m_Node = &inNode; }

        void SetSubComponentId(int inId) override
        {
            if (inId > 0 && inId < (int)StudioWidgetComponentIds::LastId)
                m_Highlight = static_cast<StudioWidgetComponentIds::Enum>(inId);
            else
                m_Highlight = StudioWidgetComponentIds::NoId;
        }

        StudioWidgetTypes::Enum GetWidgetType() const override { return TWidgetType; }
        qt3ds::studio::SStudioPickValue PickIndexToPickValue(QT3DSU32 inPickIndex) override
        {
            return qt3ds::studio::SWidgetPick((QT3DSI32)inPickIndex);
        }

        void SetupRender(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext)
        {
            m_Shader = IStudioWidget::CreateWidgetShader(inWidgetContext, inRenderContext);
            m_PickShader = IStudioWidget::CreateWidgetPickShader(inWidgetContext, inRenderContext);
        }

        QT3DSMat44 SetupMVP(IRenderWidgetContext &inWidgetContext)
        {
            SWidgetRenderSetupResult theSetup(inWidgetContext, *m_Node, m_WidgetMode);
            m_TranslationScale = theSetup.m_TranslationScale;
            m_CameraTranslationScale = theSetup.m_CameraTranslationScale;
            m_PureProjection = theSetup.m_PureProjection;
            m_WidgetInfo = theSetup.m_WidgetInfo;
            return theSetup.m_SetupResult;
        }

        void RenderSingleToneGeometry(StudioWidgetComponentIds::Enum inId,
                                      const QT3DSVec3 &inOriginalColor,
                                      NVRenderContext &inRenderContext,
                                      NVRenderInputAssembler *inGeometryAssembly)
        {
            bool isHighlighted = inId == m_Highlight;
            QT3DSVec3 theColor = isHighlighted ? QT3DSVec3(1, 1, 0) : inOriginalColor;

            m_Shader->SetPropertyValue("color0", theColor);
            inRenderContext.SetInputAssembler(inGeometryAssembly);
            inRenderContext.Draw(qt3ds::render::NVRenderDrawMode::Triangles,
                                 inGeometryAssembly->GetVertexCount(), 0);
        };

        void RenderTwoToneGeometry(StudioWidgetComponentIds::Enum inId, QT3DSVec3 inColor0,
                                   QT3DSVec3 inColor1, NVRenderContext &inRenderContext,
                                   NVRenderInputAssembler *inGeometryAssembly)
        {
            bool isHighlighted = inId == m_Highlight;
            if (isHighlighted) {
                inColor0 = inColor1 = QT3DSVec3(1, 1, 0);
            }

            m_Shader->SetPropertyValue("color0", inColor0);
            m_Shader->SetPropertyValue("color1", inColor1);
            inRenderContext.SetInputAssembler(inGeometryAssembly);
            inRenderContext.Draw(qt3ds::render::NVRenderDrawMode::Triangles,
                                 inGeometryAssembly->GetVertexCount(), 0);
        };

        void SetRenderWidgetMode(RenderWidgetModes::Enum inSpace) override { m_WidgetMode = inSpace; }

        RenderWidgetModes::Enum GetRenderWidgetMode() const override { return m_WidgetMode; }

        void RenderPickBuffer(StudioWidgetComponentIds::Enum inId,
                              NVRenderInputAssembler *inGeometryAssembly,
                              NVRenderContext &inRenderContext)
        {
            QT3DSI32 theObjectId = inId;
            m_PickShader->SetPropertyValue("object_id", theObjectId);
            inRenderContext.SetInputAssembler(inGeometryAssembly);
            inRenderContext.Draw(qt3ds::render::NVRenderDrawMode::Triangles,
                                 inGeometryAssembly->GetVertexCount(), 0);
        }

        void BeginImmediateDrawing(IRenderWidgetContext &, NVRenderContext &)
        {
            m_ImmediateBuffer.clear();
        }

        void DrawImmediateRect(const QT3DSVec3 &rectStart, const QT3DSVec3 &rectEnd, const QT3DSVec3 &orth1,
                               QT3DSF32 axisHalfWidth, const QT3DSVec4 &inColor)
        {
            StaticAssert<sizeof(SImmediateVertex) == 7 * sizeof(QT3DSF32)>::valid_expression();
            m_ImmediateBuffer.push_back(
                SImmediateVertex(rectStart + orth1 * axisHalfWidth, inColor));
            m_ImmediateBuffer.push_back(SImmediateVertex(rectEnd + orth1 * axisHalfWidth, inColor));
            m_ImmediateBuffer.push_back(SImmediateVertex(rectEnd - orth1 * axisHalfWidth, inColor));
            m_ImmediateBuffer.push_back(SImmediateVertex(rectEnd - orth1 * axisHalfWidth, inColor));
            m_ImmediateBuffer.push_back(
                SImmediateVertex(rectStart - orth1 * axisHalfWidth, inColor));
            m_ImmediateBuffer.push_back(
                SImmediateVertex(rectStart + orth1 * axisHalfWidth, inColor));
        }

        void DrawImmediateLine(const QT3DSVec3 &inStart, const QT3DSVec3 &inEnd, QT3DSF32 inWidth,
                               const QT3DSVec4 &inColor)
        {
            QT3DSVec3 theDir = inEnd - inStart;
            theDir.normalize();
            QT3DSVec3 theTemp = theDir.cross(QT3DSVec3(0, 0, 1));
            QT3DSF32 theTempLen = theTemp.normalize();
            if (theTempLen < .01f) {
                theTemp = theDir.cross(QT3DSVec3(0, 1, 0));
                theTemp.normalize();
            }
            QT3DSVec3 rectStart(inStart);
            QT3DSVec3 rectEnd(inEnd);
            QT3DSVec3 orth1 = theDir.cross(theTemp);
            QT3DSVec3 orth2 = orth1.cross(theDir);
            orth1.normalize();
            orth2.normalize();
            QT3DSF32 axisHalfWidth = inWidth / 2.0f;
            DrawImmediateRect(rectStart, rectEnd, orth1, axisHalfWidth, inColor);
            DrawImmediateRect(rectStart, rectEnd, orth2, axisHalfWidth, inColor);
        }

        void DrawFilledArc(const QT3DSVec3 &inStartPos, const QT3DSVec3 &inStartDirection, QT3DSF32 inArcLen,
                           const QT3DSVec3 &inRotationAxis, QT3DSF32 inAngle, const QT3DSVec4 &inFillColor)
        {
            // 25 small triangles per 180 degrees
            QT3DSF32 arcLen = (QT3DSF32)(M_PI / 25.0f);
            QT3DSU32 increments = qMax((QT3DSU32)1, (QT3DSU32)((fabs(inArcLen) / arcLen) + .5f));
            QT3DSF32 angleMultiplier = inAngle / (QT3DSF32)increments;
            for (QT3DSU32 idx = 0; idx < increments; ++idx) {
                QT3DSF32 localAngle = angleMultiplier * idx;
                QT3DSF32 nextAngle = angleMultiplier * (idx + 1);
                QT3DSQuat theQuat(localAngle, inRotationAxis);
                QT3DSQuat nextQuat(nextAngle, inRotationAxis);
                QT3DSVec3 startDir = theQuat.rotate(inStartDirection);
                QT3DSVec3 endDir = nextQuat.rotate(inStartDirection);
                QT3DSVec3 arcStart = inStartPos + (startDir * inArcLen);
                QT3DSVec3 arcEnd = inStartPos + (endDir * inArcLen);
                m_ImmediateBuffer.push_back(SImmediateVertex(inStartPos, inFillColor));
                m_ImmediateBuffer.push_back(SImmediateVertex(arcStart, inFillColor));
                m_ImmediateBuffer.push_back(SImmediateVertex(arcEnd, inFillColor));
            }
        }

        void EndImmediateDrawing(IRenderWidgetContext &inWidgetContext,
                                 NVRenderContext &inRenderContext, const QT3DSMat44 &inProjection)
        {
            static qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                      qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
                qt3ds::render::NVRenderVertexBufferEntry(
                    "attr_color", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 4, 12),
            };

            if (m_ImmediateBuffer.empty())
                return;

            CRegisteredString theShaderName(
                inRenderContext.GetStringTable().RegisterStr("StudioWidgetImmedateShader"));
            m_ImmediateShader = inWidgetContext.GetShader(theShaderName);

            if (m_ImmediateShader == nullptr) {
                qt3ds::render::IShaderProgramGenerator &theGenerator(
                    inWidgetContext.GetProgramGenerator());
                theGenerator.BeginProgram();
                qt3ds::render::IShaderStageGenerator &theVertexGenerator(
                    *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Vertex));
                qt3ds::render::IShaderStageGenerator &theFragmentGenerator(
                    *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Fragment));
                theVertexGenerator.AddIncoming("attr_pos", "vec3");
                theVertexGenerator.AddIncoming("attr_color", "vec4");
                theVertexGenerator.AddUniform("model_view_projection", "mat4");
                theVertexGenerator.AddOutgoing("vertex_color", "vec4");
                theVertexGenerator.Append("void main() {");
                theVertexGenerator.Append(
                    "\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
                theVertexGenerator.Append("\tvertex_color = attr_color;");
                theVertexGenerator.Append("}");
                theFragmentGenerator.Append("void main() {");
                theFragmentGenerator.Append("\tgl_FragColor = vertex_color;");
                theFragmentGenerator.Append("}");

                m_ImmediateShader = inWidgetContext.CompileAndStoreShader(theShaderName);
            }

            CRegisteredString theBufferName =
                inRenderContext.GetStringTable().RegisterStr("StudioWidgetImmediateBuffer");
            m_ImmediateVertexBuffer = &inWidgetContext.GetOrCreateVertexBuffer(
                theBufferName, 3 * sizeof(QT3DSF32) + 4 * sizeof(QT3DSF32),
                toU8DataRef(m_ImmediateBuffer.begin(), m_ImmediateBuffer.size()));

            if (!m_ImmediateInputAssembler) {
                QT3DSU32 stride = m_ImmediateVertexBuffer->GetStride();
                QT3DSU32 offset = 0;
                NVRenderAttribLayout *theAttribLayout =
                    &inWidgetContext.CreateAttributeLayout(toConstDataRef(theEntries, 2));

                CRegisteredString theString =
                    inRenderContext.GetStringTable().RegisterStr("StudioWidgetImmediateBuffer");
                m_ImmediateInputAssembler = &inWidgetContext.GetOrCreateInputAssembler(
                    theString, theAttribLayout, toConstDataRef(&m_ImmediateVertexBuffer, 1), nullptr,
                    toConstDataRef(&stride, 1), toConstDataRef(&offset, 1));
            }

            if (m_ImmediateShader && m_ImmediateInputAssembler) {
                inRenderContext.SetActiveShader(m_ImmediateShader);
                m_ImmediateShader->SetPropertyValue("model_view_projection", inProjection);
                inRenderContext.SetInputAssembler(m_ImmediateInputAssembler);
                inRenderContext.Draw(NVRenderDrawMode::Triangles,
                                     m_ImmediateInputAssembler->GetVertexCount(), 0);
            }
        }

        void SetAxisScale(const QT3DSVec3 &inAxisScale) override { m_AxisScale = inAxisScale; }

        void SetRotationEdges(const QT3DSVec3 &inStartDirection, const QT3DSVec3 &inRotationAxis,
                                      QT3DSF32 inAngleRad, QT3DSF32 inEndLineLen) override
        {
            m_RotationWedge =
                SRotationWedge(inStartDirection, inRotationAxis, inAngleRad, inEndLineLen);
        }

        void ClearRotationEdges() override { m_RotationWedge = Empty(); }

        bool isNodeBehindCamera() const override
        {
            return m_WidgetInfo.m_Camera && m_Node
                    && m_WidgetInfo.m_Camera->GetDirection().dot(
                        (m_Node->GetGlobalPos() - m_WidgetInfo.m_Camera->GetGlobalPos())) > 0.f;
        }

        static inline QT3DSVec3 ToGLSLColor(const QColor &c)
        {
            return QT3DSVec3(c.redF(), c.greenF(), c.blueF());
        }

        static QT3DSVec3 GetXAxisColor() { return ToGLSLColor(CStudioPreferences::GetXAxisColor()); }
        static QT3DSVec3 GetYAxisColor() { return ToGLSLColor(CStudioPreferences::GetYAxisColor()); }
        static QT3DSVec3 GetZAxisColor() { return ToGLSLColor(CStudioPreferences::GetZAxisColor()); }

        static inline QT3DSF32 GetDiscPos() { return 65.0f; }
        static inline QT3DSF32 GetDiscRadius() { return 7.0f; }
        static inline QT3DSF32 GetDiscRingRadius() { return GetDiscRadius() + 2.0f; }
    };
}
}

#endif
