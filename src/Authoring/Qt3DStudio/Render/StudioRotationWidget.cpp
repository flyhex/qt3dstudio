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
#include "Qt3DSCommonPrecompile.h"
#include "StudioWidgetImpl.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "Qt3DSRenderNode.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "Qt3DSRenderCamera.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"

using namespace qt3ds::widgets;

namespace {

struct SRotationWidget : public SStudioWidgetImpl<StudioWidgetTypes::Rotation>
{
    typedef SStudioWidgetImpl<StudioWidgetTypes::Rotation> TBase;
    NVRenderInputAssembler *m_XAxis;
    NVRenderInputAssembler *m_YAxis;
    NVRenderInputAssembler *m_ZAxis;
    NVRenderInputAssembler *m_CameraAxis;
    // We use a rect to clear the Z buffer.
    NVRenderInputAssembler *m_CameraRect;

    NVRenderShaderProgram *m_ZClearShader;

    volatile QT3DSI32 mRefCount;

    SRotationWidget(NVAllocatorCallback &inAlloc)
        : TBase(inAlloc)
        , m_XAxis(nullptr)
        , m_YAxis(nullptr)
        , m_ZAxis(nullptr)
        , m_CameraAxis(nullptr)
        , m_CameraRect(nullptr)
        , m_ZClearShader(nullptr)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)

    NVRenderInputAssembler *CreateRing(IRenderWidgetContext &inWidgetContext,
                                       NVRenderContext &inRenderContext, QT3DSVec3 inDirection,
                                       QT3DSF32 inInnerRadius, QT3DSF32 inOuterRadius, QT3DSF32 inRingColor,
                                       const char *inRingName)
    {
        QT3DS_ASSERT(inInnerRadius <= inOuterRadius);
        CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr(inRingName);
        NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
        if (retval) {
            return retval;
        }

        TResultVecType theVertexData(m_Allocator, "SRotationWidget::theVertexData");

        QT3DSI32 numSubDivisions = 50;
        QT3DSF32 arcRad = 360.0f / (QT3DSF32)numSubDivisions;
        TORAD(arcRad);
        QT3DSVec3 tempCross = inDirection.cross(QT3DSVec3(0, 1, 0));
        if (tempCross.magnitudeSquared() < .05f)
            tempCross = inDirection.cross(QT3DSVec3(1, 0, 0));

        QT3DSVec3 upDir = inDirection.cross(tempCross);
        QT3DSVec3 leftDir = upDir.cross(inDirection);
        upDir.normalize();
        leftDir.normalize();

        QT3DSF32 ringWidth = inOuterRadius - inInnerRadius;
        QT3DSF32 ringHalfWidth = ringWidth / 2.0f;
        QT3DSF32 middleRadius = inInnerRadius + ringHalfWidth;

        for (QT3DSI32 idx = 0, numLooper = numSubDivisions; idx < numLooper; ++idx) {
            QT3DSF32 startDeg = idx * 360.0f / numSubDivisions;
            QT3DSF32 endDeg = (idx + 1) * 360.0f / numSubDivisions;
            QT3DSF32 startRad(startDeg);
            QT3DSF32 endRad(endDeg);
            TORAD(startRad);
            TORAD(endRad);
            QT3DSF32 startSin = NVSin(startRad);
            QT3DSF32 endSin = NVSin(endRad);
            QT3DSF32 startCos = NVCos(startRad);
            QT3DSF32 endCos = NVCos(endRad);

            QT3DSVec3 startDir = startSin * upDir + startCos * leftDir;
            QT3DSVec3 endDir = endSin * upDir + endCos * leftDir;

            QT3DSVec3 discStart = startDir * inInnerRadius;
            QT3DSVec3 discEnd = endDir * inInnerRadius;
            QT3DSVec3 ringStart = startDir * inOuterRadius;
            QT3DSVec3 ringEnd = endDir * inOuterRadius;

            QT3DSVec3 middleStart = startDir * (middleRadius);
            QT3DSVec3 middleEnd = endDir * (middleRadius);
            QT3DSVec3 middleTopLeft = middleStart + inDirection * ringHalfWidth;
            QT3DSVec3 middleTopRight = middleStart - inDirection * ringHalfWidth;
            QT3DSVec3 middleBottomLeft = middleEnd + inDirection * ringHalfWidth;
            QT3DSVec3 middleBottomRight = middleEnd - inDirection * ringHalfWidth;

            // Now two tris for the ring
            theVertexData.push_back(QT3DSVec4(discStart, inRingColor));
            theVertexData.push_back(QT3DSVec4(ringStart, inRingColor));
            theVertexData.push_back(QT3DSVec4(ringEnd, inRingColor));
            theVertexData.push_back(QT3DSVec4(ringEnd, inRingColor));
            theVertexData.push_back(QT3DSVec4(discEnd, inRingColor));
            theVertexData.push_back(QT3DSVec4(discStart, inRingColor));
            // Two tris for the ring that is perpendicular to the viewer
            theVertexData.push_back(QT3DSVec4(middleTopLeft, inRingColor));
            theVertexData.push_back(QT3DSVec4(middleTopRight, inRingColor));
            theVertexData.push_back(QT3DSVec4(middleBottomRight, inRingColor));
            theVertexData.push_back(QT3DSVec4(middleBottomRight, inRingColor));
            theVertexData.push_back(QT3DSVec4(middleBottomLeft, inRingColor));
            theVertexData.push_back(QT3DSVec4(middleTopLeft, inRingColor));
        }

        QT3DSU32 stride;
        QT3DSU32 offset = 0;
        NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
                    IStudioWidget::GetVertexBufferAttributesAndStride(stride));
        NVRenderVertexBuffer *theVertexBuffer = &inWidgetContext.GetOrCreateVertexBuffer(
                    theItemName, stride, toU8DataRef(theVertexData.begin(), theVertexData.size()));
        retval = &inWidgetContext.GetOrCreateInputAssembler(
                    theItemName, theAttribLayout, toConstDataRef(&theVertexBuffer, 1), nullptr,
                    toConstDataRef(&stride, 1), toConstDataRef(&offset, 1));

        return retval;
    }

    NVRenderInputAssembler *CreateRect(IRenderWidgetContext &inWidgetContext,
                                       NVRenderContext &inRenderContext, QT3DSVec3 inDirection,
                                       QT3DSF32 inHalfWidth, const char *inItemName)
    {
        CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr(inItemName);
        NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
        if (retval) {
            return retval;
        }

        QT3DSVec3 tempCross = inDirection.cross(QT3DSVec3(0, 1, 0));
        if (tempCross.magnitudeSquared() < .05f)
            tempCross = inDirection.cross(QT3DSVec3(1, 0, 0));

        QT3DSVec3 upDir = inDirection.cross(tempCross);
        QT3DSVec3 leftDir = upDir.cross(inDirection);

        TResultVecType theVertexData(m_Allocator, "SRotationWidget::theVertexData");

        theVertexData.push_back(QT3DSVec4(upDir * inHalfWidth, 0.0f));
        theVertexData.push_back(QT3DSVec4(leftDir * inHalfWidth, 0.0f));
        theVertexData.push_back(QT3DSVec4(upDir * -1.0f * inHalfWidth, 0.0f));

        theVertexData.push_back(QT3DSVec4(upDir * -1.0f * inHalfWidth, 0.0f));
        theVertexData.push_back(QT3DSVec4(leftDir * -1.0f * inHalfWidth, 0.0f));
        theVertexData.push_back(QT3DSVec4(upDir * inHalfWidth, 0.0f));

        QT3DSU32 stride;
        QT3DSU32 offset = 0;
        NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
                    IStudioWidget::GetVertexBufferAttributesAndStride(stride));
        NVRenderVertexBuffer *theVertexBuffer = &inWidgetContext.GetOrCreateVertexBuffer(
                    theItemName, stride, toU8DataRef(theVertexData.begin(), theVertexData.size()));

        retval = &inWidgetContext.GetOrCreateInputAssembler(
                    theItemName, theAttribLayout, toConstDataRef(&theVertexBuffer, 1), nullptr,
                    toConstDataRef(&stride, 1), toConstDataRef(&offset, 1));
        return retval;
    }

    NVRenderShaderProgram *CreateZClearShader(IRenderWidgetContext &inWidgetContext,
                                              NVRenderContext &inRenderContext,
                                              const char *inItemName)
    {
        CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr(inItemName);
        NVRenderShaderProgram *retval = inWidgetContext.GetShader(theItemName);
        if (retval) {
            return retval;
        }

        qt3ds::render::IShaderProgramGenerator &theGenerator(inWidgetContext.GetProgramGenerator());
        theGenerator.BeginProgram();
        qt3ds::render::IShaderStageGenerator &theVertexGenerator(
                    *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Vertex));
        qt3ds::render::IShaderStageGenerator &theFragmentGenerator(
                    *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Fragment));
        theVertexGenerator.AddIncoming("attr_pos", "vec3");
        theVertexGenerator.AddUniform("model_view_projection", "mat4");
        theVertexGenerator.Append("void main() {");
        theVertexGenerator.Append("\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
        theVertexGenerator.Append("}");
        theFragmentGenerator.Append("void main() {");
        theFragmentGenerator.Append("\tgl_FragColor.rgb = vec3(0.0, 0.0, 0.0);");
        theFragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
        theFragmentGenerator.Append("}");
        return inWidgetContext.CompileAndStoreShader(theItemName);
    }

    static inline QT3DSVec3 ToFixedCameraPos(const QT3DSVec3 &inCameraPos, const SCamera &inCamera)
    {
        if (inCamera.m_Flags.IsOrthographic()) {
            return QT3DSVec3(inCameraPos.x, inCameraPos.y, -600.f);
        }
        QT3DSF32 multiplier = -600.f / inCameraPos.z;
        return inCameraPos * multiplier;
    }

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext) override
    {
        // Widgets have to clear the depth buffer; they shouldn't interact with other components
        // but they should self-occlude.
        inRenderContext.SetDepthWriteEnabled(true);
        inRenderContext.Clear(qt3ds::render::NVRenderClearValues::Depth);
        inRenderContext.SetBlendFunction(
                    qt3ds::render::NVRenderBlendFunctionArgument(
                        qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
                        qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
                        qt3ds::render::NVRenderSrcBlendFunc::One,
                        qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha));
        inRenderContext.SetBlendEquation(
                    qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));

        float pixelRatio = float(StudioUtils::devicePixelRatio());
        QT3DSF32 theRingRadius = 2 * CStudioPreferences::getSelectorLineLength() * pixelRatio;
        QT3DSF32 theRingWidth = CStudioPreferences::getSelectorLineWidth() * pixelRatio;
        QT3DSF32 theRingInner = theRingRadius;
        QT3DSF32 theRingOuter = theRingRadius + theRingWidth;
        if (m_XAxis == nullptr) {
            TBase::SetupRender(inWidgetContext, inRenderContext);
            m_PickShader = IStudioWidget::CreateWidgetPickShader(inWidgetContext, inRenderContext);
            m_XAxis = CreateRing(inWidgetContext, inRenderContext, QT3DSVec3(-1, 0, 0),
                                 theRingInner, theRingOuter, 0.0f, "RotationWidgetXAxis");
            m_YAxis = CreateRing(inWidgetContext, inRenderContext, QT3DSVec3(0, -1, 0),
                                 theRingInner, theRingOuter, 0.0f, "RotationWidgetYAxis");
            m_ZAxis = CreateRing(inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1),
                                 theRingInner, theRingOuter, 0.0f, "RotationWidgetZAxis");
            m_CameraAxis =
                    CreateRing(inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1),
                               theRingInner + 5, theRingOuter + 5, 0.0f,
                               "RotationWidgetCameraAxis");
            m_CameraRect = CreateRect(inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1), 200.0f,
                                      "RotationWidgetZClear");
            m_ZClearShader =
                    CreateZClearShader(inWidgetContext, inRenderContext, "RotationWidgetZClear");
        }
        QT3DSVec3 theXColor(GetXAxisColor());
        QT3DSVec3 theYColor(GetYAxisColor());
        QT3DSVec3 theZColor(GetZAxisColor());
        QT3DSVec3 theRingColor(QT3DSVec3(.8f, .8f, .8f));

        QT3DSMat44 theMVP = TBase::SetupMVP(inWidgetContext);

        if (isNodeBehindCamera())
            return;

        inRenderContext.SetCullingEnabled(false);
        QT3DSMat44 theCameraMVP = m_WidgetInfo.m_LayerProjection * m_CameraTranslationScale;

        inRenderContext.SetBlendingEnabled(false);
        inRenderContext.SetColorWritesEnabled(false);
        inRenderContext.SetDepthTestEnabled(false);
        inRenderContext.SetDepthWriteEnabled(true);
        inRenderContext.SetActiveShader(m_ZClearShader);
        inRenderContext.SetInputAssembler(m_CameraRect);
        m_ZClearShader->SetPropertyValue("model_view_projection", theCameraMVP);
        inRenderContext.Draw(NVRenderDrawMode::Triangles, m_CameraRect->GetVertexCount(), 0);

        inRenderContext.SetColorWritesEnabled(true);
        inRenderContext.SetActiveShader(m_Shader);
        m_Shader->SetPropertyValue("model_view_projection", theCameraMVP);
        inRenderContext.SetDepthTestEnabled(false);
        inRenderContext.SetDepthWriteEnabled(false);
        RenderSingleToneGeometry(StudioWidgetComponentIds::CameraPlane, theRingColor,
                                 inRenderContext, m_CameraAxis);

        inRenderContext.SetDepthTestEnabled(true);
        inRenderContext.SetActiveShader(m_Shader);
        m_Shader->SetPropertyValue("model_view_projection", theMVP);
        RenderSingleToneGeometry(StudioWidgetComponentIds::XAxis, theXColor, inRenderContext,
                                 m_XAxis);
        RenderSingleToneGeometry(StudioWidgetComponentIds::YAxis, theYColor, inRenderContext,
                                 m_YAxis);
        RenderSingleToneGeometry(StudioWidgetComponentIds::ZAxis, theZColor, inRenderContext,
                                 m_ZAxis);

        if (m_RotationWedge.hasValue()) {
            BeginImmediateDrawing(inWidgetContext, inRenderContext);

            QT3DSMat33 theMVPRotation(m_WidgetInfo.m_CameraGlobalInverse.column0.getXYZ(),
                                      m_WidgetInfo.m_CameraGlobalInverse.column1.getXYZ(),
                                      m_WidgetInfo.m_CameraGlobalInverse.column2.getXYZ());
            theMVPRotation = theMVPRotation.getInverse().getTranspose();

            QT3DSVec3 theRotationAxis = theMVPRotation.transform(m_RotationWedge->m_RotationAxis);
            QT3DSVec3 theStartDirection = theMVPRotation.transform(m_RotationWedge->m_StartDirection);
            theRotationAxis.normalize();
            theStartDirection.normalize();
            inRenderContext.SetDepthWriteEnabled(true);
            inRenderContext.Clear(qt3ds::render::NVRenderClearValues::Depth);
            inRenderContext.SetDepthWriteEnabled(false);
            inRenderContext.SetDepthTestEnabled(false);
            inRenderContext.SetBlendingEnabled(true);
            QT3DSVec4 lineColor(1.0f, 1.0f, 1.0f, .7f);
            QT3DSVec4 fillColor(1.0f, 1.0f, 1.0f, .2f);
            switch (m_Highlight) {
            default:
                break;
            case StudioWidgetComponentIds::XAxis:
                lineColor = QT3DSVec4(theXColor, .7f);
                fillColor = QT3DSVec4(theXColor, .2f);
                break;
            case StudioWidgetComponentIds::YAxis:
                lineColor = QT3DSVec4(theYColor, .7f);
                fillColor = QT3DSVec4(theYColor, .2f);
                break;
            case StudioWidgetComponentIds::ZAxis:
                lineColor = QT3DSVec4(theZColor, .7f);
                fillColor = QT3DSVec4(theZColor, .2f);
                break;
            }
            QT3DSVec3 theStartPos(m_WidgetInfo.m_Position);
            QT3DSF32 theStartLineLen = theRingOuter * m_WidgetInfo.m_Scale;
            if (m_Highlight == StudioWidgetComponentIds::CameraPlane)
                theStartLineLen = (theRingOuter + 5) * m_WidgetInfo.m_Scale;
            // Get the end line length in camera space.
            QT3DSVec3 theGlobalStart = m_Node->GetGlobalPivot();
            QT3DSQuat theGlobalRot(m_RotationWedge->m_Angle, m_RotationWedge->m_RotationAxis);
            QT3DSVec3 theGlobalDir = theGlobalRot.rotate(m_RotationWedge->m_StartDirection);
            QT3DSVec3 theGlobalEnd = theGlobalStart + theGlobalDir * m_RotationWedge->m_EndLineLen;
            // Transform both start, end into camera space and get the length of the resulting
            // vector
            QT3DSVec3 theCameraEnd = m_WidgetInfo.m_CameraGlobalInverse.transform(theGlobalEnd);
            // Draw lines in world space
            SCamera &theCamera(*m_WidgetInfo.m_Camera);
            QT3DSVec3 lineStart(ToFixedCameraPos(theStartPos, theCamera));
            QT3DSVec3 startLineEnd(
                        ToFixedCameraPos(theStartPos + theStartDirection * theStartLineLen, theCamera));
            QT3DSVec3 endLineEnd(ToFixedCameraPos(theCameraEnd, theCamera));
            DrawImmediateLine(lineStart, startLineEnd, 1.0f, lineColor);
            DrawImmediateLine(lineStart, endLineEnd, 1.0f, lineColor);
            DrawFilledArc(theStartPos, theStartDirection, theStartLineLen, theRotationAxis,
                          m_RotationWedge->m_Angle, fillColor);
            // Now setup the model-view-projection.
            QT3DSMat44 theProjection = m_WidgetInfo.m_LayerProjection;
            EndImmediateDrawing(inWidgetContext, inRenderContext, theProjection);

            // Now we attempt to render some text.  First we format it.

            char textBuffer[25] = { 0 };
            QT3DSF32 angleDeg(m_RotationWedge->m_Angle);
            TODEG(angleDeg);
            sprintf(textBuffer, " %.1f ", -angleDeg); // spaces added for margin
            STextRenderInfo theInfo;
            theInfo.m_Text = inRenderContext.GetStringTable().RegisterStr(textBuffer);
            theInfo.m_HorizontalAlignment = TextHorizontalAlignment::Center;
            theInfo.m_VerticalAlignment = TextVerticalAlignment::Bottom;
            theInfo.m_FontSize = 24.0f * pixelRatio;
            theInfo.m_Font = inRenderContext.GetStringTable().RegisterStr("TitilliumWeb-Regular");
            QT3DSMat44 theTransMatrix(QT3DSMat44::createIdentity());
            theTransMatrix.column3.x = endLineEnd.x;
            theTransMatrix.column3.y = endLineEnd.y;
            theTransMatrix.column3.z = endLineEnd.z;
            // We want to scale the text *down* so that it looks better.
            theTransMatrix.column0[0] = m_WidgetInfo.m_Scale * .8f;
            theTransMatrix.column1[1] = m_WidgetInfo.m_Scale * .8f;
            theTransMatrix.column2[2] = m_WidgetInfo.m_Scale * .8f;
            QT3DSMat44 theTextMVP = theProjection * theTransMatrix;
            inWidgetContext.RenderText(theInfo, QT3DSVec3(1.0f, 1.0f, 1.0f),
                                       QT3DSVec3(.2f, .2f, .2f), theTextMVP);
        }
        m_Highlight = StudioWidgetComponentIds::NoId;
    }

    void RenderPick(const QT3DSMat44 &inProjPremult, NVRenderContext &inRenderContext,
                            QSize /*inWinDimensions*/) override
    {
        if (m_XAxis && m_PickShader) {
            QT3DSMat44 theCameraMVP =
                    inProjPremult * m_WidgetInfo.m_PureProjection * m_CameraTranslationScale;
            QT3DSMat44 theMVP = inProjPremult * m_WidgetInfo.m_PureProjection * m_TranslationScale;

            inRenderContext.SetDepthWriteEnabled(true);
            inRenderContext.Clear(qt3ds::render::NVRenderClearValues::Depth);
            inRenderContext.SetDepthTestEnabled(false);
            inRenderContext.SetDepthWriteEnabled(true);
            inRenderContext.SetBlendingEnabled(true);
            inRenderContext.SetCullingEnabled(false);
            inRenderContext.SetActiveShader(m_ZClearShader);
            m_ZClearShader->SetPropertyValue("model_view_projection", theCameraMVP);
            inRenderContext.SetInputAssembler(m_CameraRect);
            inRenderContext.Draw(NVRenderDrawMode::Triangles, m_CameraRect->GetVertexCount(), 0);

            inRenderContext.SetActiveShader(m_PickShader);
            m_PickShader->SetPropertyValue("model_view_projection", theCameraMVP);
            RenderPickBuffer(StudioWidgetComponentIds::CameraPlane, m_CameraAxis, inRenderContext);

            m_PickShader->SetPropertyValue("model_view_projection", theMVP);

            RenderPickBuffer(StudioWidgetComponentIds::XAxis, m_XAxis, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::YAxis, m_YAxis, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::ZAxis, m_ZAxis, inRenderContext);
        }
    }
};
}

IStudioWidget &IStudioWidget::CreateRotationWidget(NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, SRotationWidget)(inAlloc);
}
