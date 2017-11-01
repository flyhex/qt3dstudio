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
#include "StudioWidgetImpl.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "Qt3DSRenderNode.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "StudioUtils.h"

using namespace qt3ds::widgets;

namespace {

struct SScaleWidget : public SStudioWidgetImpl<StudioWidgetTypes::Scale>
{
    typedef SStudioWidgetImpl<StudioWidgetTypes::Scale> TBase;

    NVRenderInputAssembler *m_XAxis;
    NVRenderInputAssembler *m_YAxis;
    NVRenderInputAssembler *m_ZAxis;

    NVRenderInputAssembler *m_XPlane;
    NVRenderInputAssembler *m_YPlane;
    NVRenderInputAssembler *m_ZPlane;

    volatile QT3DSI32 mRefCount;

    SScaleWidget(NVAllocatorCallback &inAlloc)
        : TBase(inAlloc)
        , m_XAxis(nullptr)
        , m_YAxis(nullptr)
        , m_ZAxis(nullptr)
        , m_XPlane(nullptr)
        , m_YPlane(nullptr)
        , m_ZPlane(nullptr)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)

    NVRenderInputAssembler *CreateScaleAxis(IRenderWidgetContext &inWidgetContext,
                                            NVRenderContext &inRenderContext,
                                            const QT3DSVec3 &inDirection, QT3DSF32 inStartOffset,
                                            QT3DSF32 inLength, QT3DSF32 inWidth, QT3DSF32 inBoxSideLength,
                                            const char *inAxisName)
    {

        CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr(inAxisName);
        NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
        if (retval) {
            return retval;
        }

        QT3DSVec3 tempCross = inDirection.cross(QT3DSVec3(0, 1, 0));
        if (tempCross.magnitudeSquared() < .05f)
            tempCross = inDirection.cross(QT3DSVec3(1, 0, 0));

        QT3DSVec3 upDir = inDirection.cross(tempCross);
        QT3DSVec3 leftDir = upDir.cross(inDirection);

        TResultVecType theVertexData(m_Allocator, "SScaleWidget::theVertexData");

        QT3DSVec3 rectStart = inDirection * inStartOffset;
        // Rect end is also box start, obviously
        QT3DSVec3 rectEnd = inDirection * (inStartOffset + inLength);
        QT3DSVec3 boxEnd = inDirection * (inStartOffset + inLength + inBoxSideLength);

        QT3DSF32 axisHalfWidth = inWidth / 2.0f;
        // Create the axis
        CreateRect(rectStart, rectEnd, upDir, axisHalfWidth, 0.0f, theVertexData);
        CreateRect(rectStart, rectEnd, leftDir, axisHalfWidth, 0.0f, theVertexData);
        // Create box at the top.
        QT3DSF32 boxSideHalfLength = inBoxSideLength / 2;
        // Get the four sides
        QT3DSVec3 boxRectStart = rectEnd + (leftDir * boxSideHalfLength);
        QT3DSVec3 boxRectEnd = boxEnd + (leftDir * boxSideHalfLength);
        CreateRect(boxRectStart, boxRectEnd, upDir, boxSideHalfLength, 0.0f, theVertexData);

        boxRectStart = rectEnd - leftDir * boxSideHalfLength;
        boxRectEnd = boxEnd - leftDir * boxSideHalfLength;
        CreateRect(boxRectStart, boxRectEnd, upDir, boxSideHalfLength, 0.0f, theVertexData);

        boxRectStart = rectEnd + upDir * boxSideHalfLength;
        boxRectEnd = boxEnd + upDir * boxSideHalfLength;
        CreateRect(boxRectStart, boxRectEnd, leftDir, boxSideHalfLength, 0.0f, theVertexData);

        boxRectStart = rectEnd - upDir * boxSideHalfLength;
        boxRectEnd = boxEnd - upDir * boxSideHalfLength;
        CreateRect(boxRectStart, boxRectEnd, leftDir, boxSideHalfLength, 0.0f, theVertexData);

        // now create the top and bottom
        // bottom
        boxRectStart = rectEnd + upDir * boxSideHalfLength;
        boxRectEnd = rectEnd - upDir * boxSideHalfLength;
        CreateRect(boxRectStart, boxRectEnd, leftDir, boxSideHalfLength, 0.0f, theVertexData);

        // top
        boxRectStart = boxEnd + upDir * boxSideHalfLength;
        boxRectEnd = boxEnd - upDir * boxSideHalfLength;
        CreateRect(boxRectStart, boxRectEnd, leftDir, boxSideHalfLength, 0.0f, theVertexData);

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

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext) override
    {
        inRenderContext.SetDepthWriteEnabled(true);
        inRenderContext.SetDepthTestEnabled(true);
        inRenderContext.Clear(qt3ds::render::NVRenderClearValues::Depth);
        float pixelRatio = float(devicePixelRatio());
        QT3DSF32 axisStart = 20.0f * pixelRatio;
        QT3DSF32 axisLength = 60.0f * pixelRatio;
        QT3DSF32 axisTotalLength = axisStart + axisLength;
        if (m_XAxis == nullptr) {
            TBase::SetupRender(inWidgetContext, inRenderContext);

            QT3DSF32 axisWidth = 2.0f * pixelRatio;
            QT3DSF32 triWidth = 7.0f * pixelRatio;
            m_XAxis = CreateScaleAxis(inWidgetContext, inRenderContext, QT3DSVec3(1, 0, 0), axisStart,
                                      axisLength, axisWidth, triWidth, "ScaleWidgetXAxis");
            m_YAxis = CreateScaleAxis(inWidgetContext, inRenderContext, QT3DSVec3(0, 1, 0), axisStart,
                                      axisLength, axisWidth, triWidth, "ScaleWidgetYAxis");
            m_ZAxis = CreateScaleAxis(inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1), axisStart,
                                      axisLength, axisWidth, triWidth, "ScaleWidgetZAxis");

            QT3DSF32 axisPos = GetDiscPos() * pixelRatio;
            QT3DSF32 axisDiscRadius = GetDiscRadius() * pixelRatio;
            QT3DSF32 axisRingRadius = GetDiscRingRadius() * pixelRatio;
            m_XPlane =
                CreateRingedDisc(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(1, 0, 0),
                                 QT3DSVec3(0, axisPos, -axisPos), axisDiscRadius, axisRingRadius, 0.0f,
                                 1.0f, "ScaleWidgetXPlane");
            m_YPlane =
                CreateRingedDisc(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(0, 1, 0),
                                 QT3DSVec3(axisPos, 0, -axisPos), axisDiscRadius, axisRingRadius, 0.0f,
                                 1.0f, "ScaleWidgetYPlane");
            m_ZPlane =
                CreateRingedDisc(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1),
                                 QT3DSVec3(axisPos, axisPos, 0), axisDiscRadius, axisRingRadius, 0.0f,
                                 1.0f, "ScaleWidgetZPlane");

            inRenderContext.SetActiveShader(m_Shader);
            m_Shader->SetPropertyValue("attr_pos_add_start", axisStart + 1);
        }

        QT3DSMat44 theMVP = TBase::SetupMVP(inWidgetContext);
        inRenderContext.SetBlendingEnabled(false);
        inRenderContext.SetDepthTestEnabled(true);
        inRenderContext.SetDepthWriteEnabled(true);
        inRenderContext.SetCullingEnabled(false);
        inRenderContext.SetActiveShader(m_Shader);
        m_Shader->SetPropertyValue("model_view_projection", theMVP);
        // temporary set color1 to white so we can hopefully see mistakes.
        m_Shader->SetPropertyValue("color1", QT3DSVec3(1, 1, 1));

        QT3DSVec3 theXColor(GetXAxisColor());
        QT3DSVec3 theYColor(GetYAxisColor());
        QT3DSVec3 theZColor(GetZAxisColor());
        QT3DSVec3 theRingColor(QT3DSVec3(.8, .8, .8));
        QT3DSVec3 theEndOffset = QT3DSVec3(axisTotalLength);
        QT3DSVec3 theScaledEnd = QT3DSVec3(theEndOffset.x * m_AxisScale.x, theEndOffset.y * m_AxisScale.y,
                                     theEndOffset.z * m_AxisScale.z);
        QT3DSVec3 theEndAddition = theScaledEnd - theEndOffset;

        m_Shader->SetPropertyValue("attr_pos_add_amount", QT3DSVec3(theEndAddition.x, 0, 0));
        RenderSingleToneGeometry(StudioWidgetComponentIds::XAxis, theXColor, inRenderContext,
                                 m_XAxis);

        m_Shader->SetPropertyValue("attr_pos_add_amount", QT3DSVec3(0, theEndAddition.y, 0));
        RenderSingleToneGeometry(StudioWidgetComponentIds::YAxis, theYColor, inRenderContext,
                                 m_YAxis);

        m_Shader->SetPropertyValue("attr_pos_add_amount", QT3DSVec3(0, 0, -1.0f * theEndAddition.z));
        RenderSingleToneGeometry(StudioWidgetComponentIds::ZAxis, theZColor, inRenderContext,
                                 m_ZAxis);

        m_Shader->SetPropertyValue("attr_pos_add_amount", QT3DSVec3(0, 0, 0));
        RenderTwoToneGeometry(StudioWidgetComponentIds::XPlane, theXColor, theRingColor,
                              inRenderContext, m_XPlane);
        RenderTwoToneGeometry(StudioWidgetComponentIds::YPlane, theYColor, theRingColor,
                              inRenderContext, m_YPlane);
        RenderTwoToneGeometry(StudioWidgetComponentIds::ZPlane, theZColor, theRingColor,
                              inRenderContext, m_ZPlane);

        m_Highlight = StudioWidgetComponentIds::NoId;
    }

    void RenderPick(const QT3DSMat44 &inProjPremult, NVRenderContext &inRenderContext,
                            QSize /*inWinDimensions*/) override
    {
        if (m_XAxis && m_PickShader) {
            inRenderContext.SetDepthWriteEnabled(true);
            inRenderContext.Clear(qt3ds::render::NVRenderClearValues::Depth);
            inRenderContext.SetDepthTestEnabled(true);
            inRenderContext.SetBlendingEnabled(false);
            inRenderContext.SetDepthTestEnabled(true);
            inRenderContext.SetDepthWriteEnabled(true);
            inRenderContext.SetCullingEnabled(false);
            inRenderContext.SetActiveShader(m_PickShader);
            // The projection premultiplication step moves the viewport around till
            // it is centered over the mouse and scales everything *post* rendering (to keep
            // appropriate aspect).
            QT3DSMat44 theMVP = inProjPremult * m_PureProjection * m_TranslationScale;
            m_PickShader->SetPropertyValue("model_view_projection", theMVP);

            RenderPickBuffer(StudioWidgetComponentIds::XAxis, m_XAxis, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::YAxis, m_YAxis, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::ZAxis, m_ZAxis, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::XPlane, m_XPlane, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::YPlane, m_YPlane, inRenderContext);
            RenderPickBuffer(StudioWidgetComponentIds::ZPlane, m_ZPlane, inRenderContext);
        }
    }
};
}

IStudioWidget &IStudioWidget::CreateScaleWidget(NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, SScaleWidget)(inAlloc);
}
