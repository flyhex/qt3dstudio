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
#include "StudioWidget.h"
#include "Qt3DSRenderWidgets.h"
#include "Qt3DSRenderContextCore.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"

using namespace qt3ds::widgets;

void IStudioWidget::CreateRect(QT3DSVec3 rectStart, QT3DSVec3 rectEnd, QT3DSVec3 orth1, QT3DSF32 axisHalfWidth,
                               QT3DSF32 inColorIndex, TResultVecType &outResult)
{
    outResult.push_back(QT3DSVec4(rectStart + orth1 * axisHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(rectEnd + orth1 * axisHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(rectEnd - orth1 * axisHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(rectEnd - orth1 * axisHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(rectStart - orth1 * axisHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(rectStart + orth1 * axisHalfWidth, inColorIndex));
}

void IStudioWidget::CreateTriangle(QT3DSVec3 triStart, QT3DSVec3 triEnd, QT3DSVec3 orth1, QT3DSF32 triHalfWidth,
                                   QT3DSF32 inColorIndex, TResultVecType &outResult)
{
    outResult.push_back(QT3DSVec4(triStart + orth1 * triHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(triStart - orth1 * triHalfWidth, inColorIndex));
    outResult.push_back(QT3DSVec4(triEnd, inColorIndex));
}

NVRenderInputAssembler *IStudioWidget::CreateRingedDisc(
    NVAllocatorCallback &inAllocator, IRenderWidgetContext &inWidgetContext,
    NVRenderContext &inRenderContext, QT3DSVec3 inDirection, QT3DSVec3 inCenterPt, QT3DSF32 inInnerRadius,
    QT3DSF32 inOuterRadius, QT3DSF32 inDiscColor, QT3DSF32 inRingColor, const char *inItemName)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr(inItemName);
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval) {
        return retval;
    }
    TResultVecType theVertexData(inAllocator, "STranslationWidget::theVertexData");
    QT3DS_ASSERT(inInnerRadius < inOuterRadius);
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

        QT3DSVec3 discStart = inCenterPt + startDir * inInnerRadius;
        QT3DSVec3 discEnd = inCenterPt + endDir * inInnerRadius;
        QT3DSVec3 ringStart = inCenterPt + startDir * inOuterRadius;
        QT3DSVec3 ringEnd = inCenterPt + endDir * inOuterRadius;

        // Create the Triangles
        // disc first
        theVertexData.push_back(QT3DSVec4(inCenterPt, inDiscColor));
        theVertexData.push_back(QT3DSVec4(discStart, inDiscColor));
        theVertexData.push_back(QT3DSVec4(discEnd, inDiscColor));

        // Now two tris for the ring
        theVertexData.push_back(QT3DSVec4(discStart, inRingColor));
        theVertexData.push_back(QT3DSVec4(ringStart, inRingColor));
        theVertexData.push_back(QT3DSVec4(ringEnd, inRingColor));
        theVertexData.push_back(QT3DSVec4(ringEnd, inRingColor));
        theVertexData.push_back(QT3DSVec4(discEnd, inRingColor));
        theVertexData.push_back(QT3DSVec4(discStart, inRingColor));
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

// Create an axis with a triangle at the top.  Really we create two axis that are orthogonal to each
// other.
NVRenderInputAssembler *
IStudioWidget::CreateAxis(NVAllocatorCallback &inAllocator, IRenderWidgetContext &inWidgetContext,
                          NVRenderContext &inRenderContext, const QT3DSVec3 &inAxisDirection,
                          QT3DSF32 inAxisStartOffset, QT3DSF32 inAxisLength, QT3DSF32 inTriLength,
                          QT3DSF32 inAxisWidth, QT3DSF32 inTriWidth, const char *inAxisName)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr(inAxisName);
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval) {
        return retval;
    }

    TResultVecType theVertexData(inAllocator, "STranslationWidget::theVertexData");
    QT3DSVec3 orth1 = inAxisDirection.cross(QT3DSVec3(0, 0, 1));
    if (orth1.magnitudeSquared() < .05f)
        orth1 = inAxisDirection.cross(QT3DSVec3(0, 1, 0));
    QT3DSVec3 orth2 = inAxisDirection.cross(orth1);

    // Draw a rect that starts at inAxisStartOffset
    QT3DSVec3 rectStart = inAxisDirection * inAxisStartOffset;
    // Rect end is also tri start, obviously
    QT3DSVec3 rectEnd = inAxisDirection * (inAxisStartOffset + inAxisLength);
    QT3DSVec3 triEnd = inAxisDirection * (inAxisStartOffset + inAxisLength + inTriLength);

    QT3DSF32 axisHalfWidth = inAxisWidth / 2.0f;
    QT3DSF32 triHalfWidth = inTriWidth / 2.0f;
    CreateRect(rectStart, rectEnd, orth1, axisHalfWidth, 0.0f, theVertexData);
    CreateTriangle(rectEnd, triEnd, orth1, triHalfWidth, 0.0f, theVertexData);
    CreateRect(rectStart, rectEnd, orth2, axisHalfWidth, 0.0f, theVertexData);
    CreateTriangle(rectEnd, triEnd, orth2, triHalfWidth, 0.0f, theVertexData);

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
};

SWidgetRenderSetupResult::SWidgetRenderSetupResult(IRenderWidgetContext &inWidgetContext,
                                                   SNode &inNode,
                                                   RenderWidgetModes::Enum inWidgetMode)
{
    m_WidgetInfo =
        inWidgetContext.GetWidgetRenderInformation(inNode, QT3DSVec3(0, 0, 0), inWidgetMode);
    QT3DSMat44 theTranslationScale(QT3DSMat44::createIdentity());
    QT3DSMat33 theRotationMult(QT3DSMat33::createIdentity());
    bool includeNodeRotation = inWidgetMode == RenderWidgetModes::Local ? true : false;
    if (includeNodeRotation) {
        QT3DSMat44 theNodeRotation;
        inNode.CalculateRotationMatrix(theNodeRotation);
        if (inNode.m_Flags.IsLeftHanded()) {
            SNode::FlipCoordinateSystem(theNodeRotation);
        }
        theRotationMult =
            QT3DSMat33(theNodeRotation.column0.getXYZ(), theNodeRotation.column1.getXYZ(),
                    theNodeRotation.column2.getXYZ());
    }

    QT3DSMat33 theRotationMatrix = m_WidgetInfo.m_NormalMatrix * theRotationMult;
    QT3DSMat33 theScaleMatrix(QT3DSMat33::createIdentity());
    theScaleMatrix.column0[0] = m_WidgetInfo.m_Scale;
    theScaleMatrix.column1[1] = m_WidgetInfo.m_Scale;
    theScaleMatrix.column2[2] = m_WidgetInfo.m_Scale;
    QT3DSMat33 theCombined = theRotationMatrix * theScaleMatrix;
    theTranslationScale.column0 = QT3DSVec4(theCombined.column0, 0.0f);
    theTranslationScale.column1 = QT3DSVec4(theCombined.column1, 0.0f);
    theTranslationScale.column2 = QT3DSVec4(theCombined.column2, 0.0f);
    theTranslationScale.column3.x = m_WidgetInfo.m_Position.x;
    theTranslationScale.column3.y = m_WidgetInfo.m_Position.y;
    theTranslationScale.column3.z = m_WidgetInfo.m_Position.z;
    m_TranslationScale = theTranslationScale;
    m_PureProjection = m_WidgetInfo.m_PureProjection;

    QT3DSMat44 theCameraTransScale(QT3DSMat44::createIdentity());
    theCameraTransScale.column0 = QT3DSVec4(m_WidgetInfo.m_LookAtMatrix.column0, 0.0f);
    theCameraTransScale.column1 = QT3DSVec4(m_WidgetInfo.m_LookAtMatrix.column1, 0.0f);
    theCameraTransScale.column2 = QT3DSVec4(m_WidgetInfo.m_LookAtMatrix.column2, 0.0f);
    theCameraTransScale.column3.x = m_WidgetInfo.m_Position.x;
    theCameraTransScale.column3.y = m_WidgetInfo.m_Position.y;
    theCameraTransScale.column3.z = m_WidgetInfo.m_Position.z;
    theCameraTransScale.column0[0] = m_WidgetInfo.m_Scale;
    theCameraTransScale.column1[1] = m_WidgetInfo.m_Scale;
    theCameraTransScale.column2[2] = m_WidgetInfo.m_Scale;
    m_CameraTranslationScale = theCameraTransScale;
    m_SetupResult = m_WidgetInfo.m_LayerProjection * theTranslationScale;
}

CRegisteredString IStudioWidget::GetSharedShaderName(IStringTable &inStrTable)
{
    return inStrTable.RegisterStr("IStudioWidget Shader");
}

CRegisteredString IStudioWidget::GetSharedPickShaderName(IStringTable &inStrTable)
{
    return inStrTable.RegisterStr("IStudioWidget Pick Shader");
}

NVRenderShaderProgram *IStudioWidget::CreateWidgetShader(IRenderWidgetContext &inWidgetContext,
                                                         NVRenderContext &inRenderContext)
{
    CRegisteredString theSharedName(GetSharedShaderName(inRenderContext.GetStringTable()));
    NVRenderShaderProgram *retval = inWidgetContext.GetShader(theSharedName);
    if (retval)
        return retval;
    qt3ds::render::IShaderProgramGenerator &theGenerator(inWidgetContext.GetProgramGenerator());
    theGenerator.BeginProgram();
    qt3ds::render::IShaderStageGenerator &theVertexGenerator(
        *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Vertex));
    qt3ds::render::IShaderStageGenerator &theFragmentGenerator(
        *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Fragment));
    theVertexGenerator.AddIncoming("attr_pos", "vec3");
    theVertexGenerator.AddIncoming("attr_color_index", "float");
    theVertexGenerator.AddOutgoing("output_color_index", "float");
    theVertexGenerator.AddUniform("model_view_projection", "mat4");
    // These are required in order to scale the scale widget the way we want to scale it.
    theVertexGenerator.AddUniform("attr_pos_add_start", "float");
    theVertexGenerator.AddUniform("attr_pos_add_amount", "vec3");
    theVertexGenerator.Append("void main() {");
    theVertexGenerator
        << "\tvec3 thePos = attr_pos;" << Endl
        << "\tif ( length(thePos) > attr_pos_add_start ) thePos = thePos + attr_pos_add_amount;"
        << Endl;
    theVertexGenerator.Append("\tgl_Position = model_view_projection * vec4(thePos, 1.0);");
    theVertexGenerator.Append("\toutput_color_index = attr_color_index;");
    theVertexGenerator.Append("}");
    theFragmentGenerator.AddUniform("color0", "vec3");
    theFragmentGenerator.AddUniform("color1", "vec3");
    theFragmentGenerator.Append("void main() {");
    theFragmentGenerator.Append("\tgl_FragColor.rgb = output_color_index > 0.0 ? color1 : color0;");
    theFragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
    theFragmentGenerator.Append("}");
    return inWidgetContext.CompileAndStoreShader(theSharedName);
}

NVRenderShaderProgram *IStudioWidget::CreateWidgetPickShader(IRenderWidgetContext &inWidgetContext,
                                                             NVRenderContext &inRenderContext)
{
    CRegisteredString theSharedName(GetSharedPickShaderName(inRenderContext.GetStringTable()));
    NVRenderShaderProgram *retval = inWidgetContext.GetShader(theSharedName);
    if (retval)
        return retval;
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
    theFragmentGenerator.AddUniform("object_id", "int");
    theFragmentGenerator.Append("void main() {");
    if (inRenderContext.GetRenderContextType() == NVRenderContextValues::GLES2) {
        theFragmentGenerator.Append("int moddiv = object_id / 256;");
        theFragmentGenerator.Append(
                    "\tgl_FragColor.r = float(object_id - moddiv * 256)/255.0;");
    } else {
        theFragmentGenerator.Append("\tgl_FragColor.r = float(object_id % 256)/255.0;");
    }
    theFragmentGenerator.Append("\tgl_FragColor.g = float(object_id / 256)/255.0;");
    theFragmentGenerator.Append("\tgl_FragColor.b = 0.0;");
    theFragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
    theFragmentGenerator.Append("}");
    return inWidgetContext.CompileAndStoreShader(theSharedName);
}

NVConstDataRef<qt3ds::render::NVRenderVertexBufferEntry>
IStudioWidget::GetVertexBufferAttributesAndStride(QT3DSU32 &stride)
{
    static qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
        qt3ds::render::NVRenderVertexBufferEntry("attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32,
                                              3),
        qt3ds::render::NVRenderVertexBufferEntry("attr_color_index",
                                              qt3ds::render::NVRenderComponentTypes::QT3DSF32, 1, 12),
    };

    stride = 3 * sizeof(QT3DSF32) + 1 * sizeof(QT3DSF32);

    return toConstDataRef(theEntries, 2);
}
