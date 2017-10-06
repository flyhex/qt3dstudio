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
#include "UICRenderShaderCodeGenerator.h"
#include "UICRenderNode.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "StudioUtils.h"

using namespace uic::widgets;

namespace {

struct STranslationWidget : public SStudioWidgetImpl<StudioWidgetTypes::Translation>
{
    typedef SStudioWidgetImpl<StudioWidgetTypes::Translation> TBase;
    NVRenderInputAssembler *m_XAxis;
    NVRenderInputAssembler *m_YAxis;
    NVRenderInputAssembler *m_ZAxis;

    NVRenderInputAssembler *m_XPlane;
    NVRenderInputAssembler *m_YPlane;
    NVRenderInputAssembler *m_ZPlane;

    volatile QT3DSI32 mRefCount;

    STranslationWidget(NVAllocatorCallback &inAlloc)
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

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator);

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext) override
    {
        // Widgets have to clear the depth buffer; they shouldn't interact with other components
        // but they should self-occlude.
        inRenderContext.SetDepthWriteEnabled(true);
        inRenderContext.Clear(qt3ds::render::NVRenderClearValues::Depth);
        inRenderContext.SetDepthTestEnabled(true);
        if (m_XAxis == nullptr) {
            TBase::SetupRender(inWidgetContext, inRenderContext);
            float pixelRatio = float(devicePixelRatio());
            QT3DSF32 axisStart = 20.0f * pixelRatio;
            QT3DSF32 axisLength = 60.0f * pixelRatio;
            QT3DSF32 triLength = 20.0f * pixelRatio;
            QT3DSF32 axisWidth = 2.0f * pixelRatio;
            QT3DSF32 triWidth = 7.0f * pixelRatio;
            m_XAxis = CreateAxis(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(1, 0, 0),
                                 axisStart, axisLength, triLength, axisWidth, triWidth,
                                 "TranslationWidgetXAxis");
            m_YAxis = CreateAxis(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(0, 1, 0),
                                 axisStart, axisLength, triLength, axisWidth, triWidth,
                                 "TranslationWidgetYAxis");
            m_ZAxis = CreateAxis(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1),
                                 axisStart, axisLength, triLength, axisWidth, triWidth,
                                 "TranslationWidgetZAxis");

            QT3DSF32 axisPos = GetDiscPos() * pixelRatio;
            QT3DSF32 axisDiscRadius = GetDiscRadius() * pixelRatio;
            QT3DSF32 axisRingRadius = GetDiscRingRadius() * pixelRatio;
            m_XPlane =
                CreateRingedDisc(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(1, 0, 0),
                                 QT3DSVec3(0, axisPos, -axisPos), axisDiscRadius, axisRingRadius, 0.0f,
                                 1.0f, "TranslationWidgetXPlane");
            m_YPlane =
                CreateRingedDisc(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(0, 1, 0),
                                 QT3DSVec3(axisPos, 0, -axisPos), axisDiscRadius, axisRingRadius, 0.0f,
                                 1.0f, "TranslationWidgetYPlane");
            m_ZPlane =
                CreateRingedDisc(m_Allocator, inWidgetContext, inRenderContext, QT3DSVec3(0, 0, -1),
                                 QT3DSVec3(axisPos, axisPos, 0), axisDiscRadius, axisRingRadius, 0.0f,
                                 1.0f, "TranslationWidgetZPlane");
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

        RenderSingleToneGeometry(StudioWidgetComponentIds::XAxis, theXColor, inRenderContext,
                                 m_XAxis);
        RenderSingleToneGeometry(StudioWidgetComponentIds::YAxis, theYColor, inRenderContext,
                                 m_YAxis);
        RenderSingleToneGeometry(StudioWidgetComponentIds::ZAxis, theZColor, inRenderContext,
                                 m_ZAxis);
        RenderTwoToneGeometry(StudioWidgetComponentIds::XPlane, theXColor, theRingColor,
                              inRenderContext, m_XPlane);
        RenderTwoToneGeometry(StudioWidgetComponentIds::YPlane, theYColor, theRingColor,
                              inRenderContext, m_YPlane);
        RenderTwoToneGeometry(StudioWidgetComponentIds::ZPlane, theZColor, theRingColor,
                              inRenderContext, m_ZPlane);
        m_Highlight = StudioWidgetComponentIds::NoId;
    }

    void RenderPick(const QT3DSMat44 &inProjPremult, NVRenderContext &inRenderContext,
                            uic::render::SWindowDimensions /*inWinDimensions*/) override
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

IStudioWidget &IStudioWidget::CreateTranslationWidget(NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, STranslationWidget)(inAlloc);
}
