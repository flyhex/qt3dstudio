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
#ifndef QT3DS_STUDIO_RENDERER_WIDGET_H
#define QT3DS_STUDIO_RENDERER_WIDGET_H
#pragma once
#include "Qt3DSRenderWidgets.h"
#include "foundation/Qt3DSRefCounted.h"
#include "StudioPickValues.h"

namespace qt3ds {
namespace widgets {
    using namespace qt3ds::render;

    struct StudioWidgetTypes
    {
        enum Enum {
            Unknown,
            Translation,
            Scale,
            Rotation
        };
    };

    // These are also the ids used as colors in the pick image.
    struct StudioWidgetComponentIds
    {
        enum Enum {
            NoId = 0,
            XAxis,
            YAxis,
            ZAxis,
            XPlane,
            YPlane,
            ZPlane,
            CameraPlane,
            LastId,
        };
    };

    // Functionality shared between the path widget and the various manipulation gadgets
    class IStudioWidgetBase : public IRenderWidget, public NVRefCounted
    {
    public:
        virtual void SetNode(SNode &inNode) = 0;
        virtual void RenderPick(const QT3DSMat44 &inProjPreMult, NVRenderContext &inRenderContext,
                                QSize inWinDimensions) = 0;
        virtual qt3ds::studio::SStudioPickValue PickIndexToPickValue(QT3DSU32 inPickIndex) = 0;
    };

    typedef nvvector<QT3DSVec4> TResultVecType;

    struct SWidgetRenderSetupResult
    {
        QT3DSMat44 m_TranslationScale;
        QT3DSMat44 m_CameraTranslationScale;
        QT3DSMat44 m_PureProjection;
        SWidgetRenderInformation m_WidgetInfo;
        QT3DSMat44 m_SetupResult;

        SWidgetRenderSetupResult() {}
        SWidgetRenderSetupResult(IRenderWidgetContext &inWidgetContext, SNode &inNode,
                                 RenderWidgetModes::Enum inWidgetMode);
    };

    class IStudioWidget : public IStudioWidgetBase
    {
    public:
        static CRegisteredString GetSharedShaderName(IStringTable &inStrTable);
        static CRegisteredString GetSharedPickShaderName(IStringTable &inStrTable);
        static NVRenderShaderProgram *CreateWidgetShader(IRenderWidgetContext &inWidgetContext,
                                                         NVRenderContext &inRenderContext);
        static NVRenderShaderProgram *CreateWidgetPickShader(IRenderWidgetContext &inWidgetContext,
                                                             NVRenderContext &inRenderContext);
        static NVConstDataRef<qt3ds::render::NVRenderVertexBufferEntry>
        GetVertexBufferAttributesAndStride(QT3DSU32 &stride);
        static NVRenderInputAssembler *
        CreateRingedDisc(NVAllocatorCallback &inAllocator, IRenderWidgetContext &inWidgetContext,
                         NVRenderContext &inRenderContext, QT3DSVec3 inDirection, QT3DSVec3 inCenterPt,
                         QT3DSF32 inInnerRadius, QT3DSF32 inOuterRadius, QT3DSF32 inDiscColor,
                         QT3DSF32 inRingColor, const char *inItemName);
        static NVRenderInputAssembler *
        CreateAxis(NVAllocatorCallback &inAllocator, IRenderWidgetContext &inWidgetContext,
                   NVRenderContext &inRenderContext, const QT3DSVec3 &inAxisDirection,
                   QT3DSF32 inAxisStartOffset, QT3DSF32 inAxisLength, QT3DSF32 inTriLength,
                   QT3DSF32 inAxisWidth, QT3DSF32 inTriWidth, const char *inAxisName);
        static void CreateRect(QT3DSVec3 rectStart, QT3DSVec3 rectEnd, QT3DSVec3 orth1, QT3DSF32 axisHalfWidth,
                               QT3DSF32 inColorIndex, TResultVecType &outResult);
        static void CreateTriangle(QT3DSVec3 triStart, QT3DSVec3 triEnd, QT3DSVec3 orth1, QT3DSF32 triHalfWidth,
                                   QT3DSF32 inColorIndex, TResultVecType &outResult);

        void SetNode(SNode &inNode) override = 0;
        virtual StudioWidgetTypes::Enum GetWidgetType() const = 0;
        virtual void SetSubComponentId(int inSubComponentId) = 0;
        virtual void SetRenderWidgetMode(RenderWidgetModes::Enum inSpace) = 0;
        virtual RenderWidgetModes::Enum GetRenderWidgetMode() const = 0;
        // When we render the axis, we can scale the axis item itself
        virtual void SetAxisScale(const QT3DSVec3 &inNewScale) = 0;
        // Set the start/end positions of the rotation arc so the rotation gadget can show
        // the angle and optionally display an angle readout.  The start direction should
        // be a normalized direction in world space, and the angle should be in radians
        // inRotationAxis is expected to be a normalized direction in world space.
        virtual void SetRotationEdges(const QT3DSVec3 &inStartDirection, const QT3DSVec3 &inRotationAxis,
                                      QT3DSF32 inAngleRad, QT3DSF32 inEndLineLen) = 0;
        virtual void ClearRotationEdges() = 0;
        virtual bool isNodeBehindCamera() const = 0;

        static IStudioWidget &CreateTranslationWidget(NVAllocatorCallback &inAlloc);
        static IStudioWidget &CreateRotationWidget(NVAllocatorCallback &inAlloc);
        static IStudioWidget &CreateScaleWidget(NVAllocatorCallback &inAlloc);
    };
}
}

#endif
