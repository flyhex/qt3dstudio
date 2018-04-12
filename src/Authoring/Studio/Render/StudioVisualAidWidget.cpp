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
#include "StudioWidgetImpl.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "Qt3DSRenderNode.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"
#include "StudioVisualAidWidget.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderLight.h"
#include "OptimizedArithmetic.h"

namespace qt3ds {
namespace widgets {

static const float fepsilon = 1e-6f;
static const float pointLightOuterRadius = 40.0f;
static const float directionalLightRadius = 40.0f;
static const float directionalLightLength = 100.0f;
static const float cameraMaxLength = 1000.0f;

SVisualAidWidget::SVisualAidWidget(NVAllocatorCallback &inAlloc)
    : m_node(nullptr)
    , m_billboard(nullptr)
    , m_cameraBox(nullptr)
    , m_directionalLight(nullptr)
    , m_pointLight(nullptr)
    , m_areaLight(nullptr)
    , m_renderCameraShader(nullptr)
    , m_renderShader(nullptr)
    , m_billboardShader(nullptr)
    , m_billboardCameraTexture(nullptr)
    , m_billboardLightTexture(nullptr)
    , m_selected(false)
    , m_allocator(inAlloc)
    , mRefCount(0)
{

}

NVConstDataRef<NVRenderVertexBufferEntry>
SVisualAidWidget::getCameraBoxAttributesAndStride(QT3DSU32 &stride)
{
    static NVRenderVertexBufferEntry theEntries[] = {
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 4)
    };

    stride = 4 * sizeof(QT3DSF32);

    return toConstDataRef(theEntries, 1);
}

NVConstDataRef<NVRenderVertexBufferEntry>
SVisualAidWidget::getLightAttributesAndStride(QT3DSU32 &stride)
{
    static NVRenderVertexBufferEntry theEntries[] = {
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3)
    };

    stride = 3 * sizeof(QT3DSF32);

    return toConstDataRef(theEntries, 1);
}

NVConstDataRef<NVRenderVertexBufferEntry>
SVisualAidWidget::getBillboardAttributesAndStride(QT3DSU32 &stride)
{
    static NVRenderVertexBufferEntry theEntries[] = {
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3),
        NVRenderVertexBufferEntry("attr_tc", NVRenderComponentTypes::QT3DSF32, 2,
                                  3 * sizeof(QT3DSF32))
    };

    stride = 6 * sizeof(QT3DSF32);

    return toConstDataRef(theEntries, 2);
}


NVRenderInputAssembler *SVisualAidWidget::createCameraBox(IRenderWidgetContext &inWidgetContext,
                                                          NVRenderContext &inRenderContext)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr("CameraBox");
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval)
        return retval;

    nvvector<QT3DSVec4> theVertexData(m_allocator, "SVisualAidWidget::theVertexData");
    nvvector<QT3DSI8> theIndexData(m_allocator, "SVisualAidWidget::theIndexData");

    // 9 vertices, origin and near and far rectangles in ndc
    theVertexData.push_back(QT3DSVec4(0.0, 0.0, 0.0, 0.0));
    theVertexData.push_back(QT3DSVec4(-1.0, -1.0, -1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(-1.0, 1.0, -1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(1.0, 1.0, -1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(1.0, -1.0, -1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(-1.0, -1.0, 1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(-1.0, 1.0, 1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(1.0, 1.0, 1.0, 1.0));
    theVertexData.push_back(QT3DSVec4(1.0, -1.0, 1.0, 1.0));

    // origin to near
    theIndexData.push_back(0); theIndexData.push_back(1);
    theIndexData.push_back(0); theIndexData.push_back(2);
    theIndexData.push_back(0); theIndexData.push_back(3);
    theIndexData.push_back(0); theIndexData.push_back(4);
    // near rect
    theIndexData.push_back(1); theIndexData.push_back(2);
    theIndexData.push_back(2); theIndexData.push_back(3);
    theIndexData.push_back(3); theIndexData.push_back(4);
    theIndexData.push_back(4); theIndexData.push_back(1);
    // near to far
    theIndexData.push_back(1); theIndexData.push_back(5);
    theIndexData.push_back(2); theIndexData.push_back(6);
    theIndexData.push_back(3); theIndexData.push_back(7);
    theIndexData.push_back(4); theIndexData.push_back(8);
    // far rect
    theIndexData.push_back(5); theIndexData.push_back(6);
    theIndexData.push_back(6); theIndexData.push_back(7);
    theIndexData.push_back(7); theIndexData.push_back(8);
    theIndexData.push_back(8); theIndexData.push_back(5);

    QT3DSU32 stride;
    QT3DSU32 offset = 0;
    NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
        getCameraBoxAttributesAndStride(stride));
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

NVRenderInputAssembler *SVisualAidWidget::createBillboard(IRenderWidgetContext &inWidgetContext,
                                                          NVRenderContext &inRenderContext)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr("Billboard");
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval)
        return retval;

    nvvector<QT3DSVec3> theVertexData(m_allocator, "SVisualAidWidget::theVertexData");
    nvvector<QT3DSI8> theIndexData(m_allocator, "SVisualAidWidget::theIndexData");

    theVertexData.push_back(QT3DSVec3(-1.0, -1.0, 0.0));
    theVertexData.push_back(QT3DSVec3(0.0, 0.0, 0.0));
    theVertexData.push_back(QT3DSVec3(-1.0, 1.0, 0.0));
    theVertexData.push_back(QT3DSVec3(0.0, 1.0, 0.0));
    theVertexData.push_back(QT3DSVec3(1.0, 1.0, 0.0));
    theVertexData.push_back(QT3DSVec3(1.0, 1.0, 0.0));
    theVertexData.push_back(QT3DSVec3(1.0, -1.0, 0.0));
    theVertexData.push_back(QT3DSVec3(1.0, 0.0, 0.0));

    theIndexData.push_back(0); theIndexData.push_back(1); theIndexData.push_back(2);
    theIndexData.push_back(0); theIndexData.push_back(3); theIndexData.push_back(2);

    QT3DSU32 stride;
    QT3DSU32 offset = 0;
    NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
        getBillboardAttributesAndStride(stride));
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

NVRenderInputAssembler *SVisualAidWidget::createDirectionalLight(
        IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext)
{
    CRegisteredString theItemName
            = inRenderContext.GetStringTable().RegisterStr("DirectionalLight");
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval)
        return retval;

    nvvector<QT3DSVec3> theVertexData(m_allocator, "SVisualAidWidget::theVertexData");
    nvvector<QT3DSI8> theIndexData(m_allocator, "SVisualAidWidget::theIndexData");

    const int ringVertices = 96;
    int skip = 1;
    int v = 0;

    // disc with parallel lines
    for (int i = 0; i < ringVertices; ++i) {
        float x = directionalLightRadius * cos(2.0 * M_PI * i / ringVertices);
        float y = directionalLightRadius * sin(2.0 * M_PI * i / ringVertices);
        theVertexData.push_back(QT3DSVec3(x, y, 0.0));
        if (i > 0) {
            theIndexData.push_back(v - skip);
            theIndexData.push_back(v);
            skip = 1;
        }
        ++v;
        if ((i%8) == 1) {
            theVertexData.push_back(QT3DSVec3(x, y, -directionalLightLength));
            theIndexData.push_back(v - 1);
            theIndexData.push_back(v);
            skip = 2;
            ++v;
        }
    }
    theIndexData.push_back(v - skip);
    theIndexData.push_back(0);

    QT3DSU32 stride;
    QT3DSU32 offset = 0;
    NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
        getLightAttributesAndStride(stride));
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

NVRenderInputAssembler *SVisualAidWidget::createPointLight(IRenderWidgetContext &inWidgetContext,
                                                           NVRenderContext &inRenderContext)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr("PointLight");
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval)
        return retval;

    nvvector<QT3DSVec3> theVertexData(m_allocator, "SVisualAidWidget::theVertexData");
    nvvector<QT3DSI16> theIndexData(m_allocator, "SVisualAidWidget::theIndexData");

    const int latSlices = 7;
    const float innerRadius = pointLightOuterRadius / 3.0f;
    const float outerRadius = pointLightOuterRadius;

#define lngVar(n) ((n&1) ? -3.0 : 3.0)
#define latVar(n) ((n&1) ? -2.0 : 2.0)

    int v = 0;

    for (int i = 0; i <= latSlices; ++i) {
        float lat = (175 / latSlices) * i + 5 + latVar(i);
        int longSlices = latSlices * 2 * sin(lat / 180 * M_PI);
        lat -= 90.0;

        for (int j = 0; j < longSlices; ++j) {
            float lng = (360 / longSlices) * j + ((j & 1) ? -180.0 : 0.0);

            float q = cos((lngVar(j) + lat) / 180 * M_PI);
            float x = cos(2.0 * lng / 180 * M_PI) * q;
            float y = sin((lngVar(j) + lat) / 180 * M_PI);
            float z = sin(2.0 * lng / 180 * M_PI) * q;

            theVertexData.push_back(QT3DSVec3(x, y, z) * innerRadius);
            theVertexData.push_back(QT3DSVec3(x, y, z) * outerRadius);
            theIndexData.push_back(v);
            theIndexData.push_back(v + 1);
            v += 2;
        }
    }

    QT3DSU32 stride;
    QT3DSU32 offset = 0;
    NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
        getLightAttributesAndStride(stride));
    NVRenderVertexBuffer *theVertexBuffer = &inWidgetContext.GetOrCreateVertexBuffer(
        theItemName, stride, toU8DataRef(theVertexData.begin(), theVertexData.size()));
    NVRenderIndexBuffer *theIndexBuffer = &inWidgetContext.GetOrCreateIndexBuffer(
        theItemName, NVRenderComponentTypes::QT3DSU16, theIndexData.size() * 2,
        toU8DataRef(theIndexData.begin(), theIndexData.size()));
    retval = &inWidgetContext.GetOrCreateInputAssembler(
        theItemName, theAttribLayout, toConstDataRef(&theVertexBuffer, 1), theIndexBuffer,
        toConstDataRef(&stride, 1), toConstDataRef(&offset, 1));

    return retval;
}

NVRenderInputAssembler *SVisualAidWidget::createAreaLight(IRenderWidgetContext &inWidgetContext,
                                                          NVRenderContext &inRenderContext)
{
    CRegisteredString theItemName = inRenderContext.GetStringTable().RegisterStr("AreaLight");
    NVRenderInputAssembler *retval = inWidgetContext.GetInputAssembler(theItemName);
    if (retval)
        return retval;

    nvvector<QT3DSVec3> theVertexData(m_allocator, "SVisualAidWidget::theVertexData");
    nvvector<QT3DSI8> theIndexData(m_allocator, "SVisualAidWidget::theIndexData");

    theVertexData.push_back(QT3DSVec3(-1, -1, 0));
    theVertexData.push_back(QT3DSVec3(1,- 1, 0));
    theVertexData.push_back(QT3DSVec3(1, 1, 0));
    theVertexData.push_back(QT3DSVec3(-1, 1, 0));
    theIndexData.push_back(0); theIndexData.push_back(1);
    theIndexData.push_back(1); theIndexData.push_back(2);
    theIndexData.push_back(2); theIndexData.push_back(3);
    theIndexData.push_back(3); theIndexData.push_back(0);

    QT3DSU32 stride;
    QT3DSU32 offset = 0;
    NVRenderAttribLayout *theAttribLayout = &inWidgetContext.CreateAttributeLayout(
        getLightAttributesAndStride(stride));
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

NVRenderShaderProgram *SVisualAidWidget::createRenderShader(IRenderWidgetContext &inWidgetContext,
                                                            NVRenderContext &inRenderContext)
{
    CRegisteredString itemName = inRenderContext.GetStringTable().RegisterStr("LineShader");
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
    vertGenerator.AddUniform("mvpMatrix", "mat4");

    vertGenerator.Append("void main() {");
    vertGenerator.Append("\tgl_Position = mvpMatrix * vec4(attr_pos, 1.0);");
    vertGenerator.Append("}");

    fragGenerator.AddUniform("color", "vec3");
    fragGenerator.AddUniform("opacity", "float");
    fragGenerator.Append("void main() {");
    fragGenerator.Append("\tgl_FragColor.rgb = color;");
    fragGenerator.Append("\tgl_FragColor.a = opacity;");
    fragGenerator.Append("}");

    return inWidgetContext.CompileAndStoreShader(itemName);
}

NVRenderShaderProgram *SVisualAidWidget::createRenderCameraShader(
        IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext)
{
    CRegisteredString itemName = inRenderContext.GetStringTable().RegisterStr("CameraRenderShader");
    NVRenderShaderProgram *retval = inWidgetContext.GetShader(itemName);
    if (retval)
        return retval;

    IShaderProgramGenerator &generator(inWidgetContext.GetProgramGenerator());
    generator.BeginProgram();
    IShaderStageGenerator &vertGenerator(
        *generator.GetStage(ShaderGeneratorStages::Vertex));
    IShaderStageGenerator &fragGenerator(
        *generator.GetStage(ShaderGeneratorStages::Fragment));
    vertGenerator.AddIncoming("attr_pos", "vec4");
    vertGenerator.AddUniform("invProjMatrix", "mat4");
    vertGenerator.AddUniform("mvpMatrix", "mat4");
    vertGenerator.AddUniform("orthographic", "int");
    vertGenerator.AddUniform("cameraMaxLength", "float");

    vertGenerator.Append("void main() {");
    vertGenerator.Append("\tvec4 pos = vec4(0.0, 0.0, 0.0, 1.0);");
    vertGenerator.Append("\tif (attr_pos.w != 0.0) {");
    vertGenerator.Append("\t\tpos = invProjMatrix * attr_pos;");
    vertGenerator.Append("\t\tpos = pos / pos.w;");
    vertGenerator.Append("\t\tif (orthographic == 0 && length(pos.xyz) > cameraMaxLength) " \
                         "pos.xyz = cameraMaxLength * normalize(pos.xyz);");
    vertGenerator.Append("\t\tif (orthographic == 1 && pos.z < -cameraMaxLength) " \
                         "pos.z = -cameraMaxLength;");
    vertGenerator.Append("\t}");
    vertGenerator.Append("\tgl_Position = mvpMatrix * vec4(pos.xyz, 1.0);");
    vertGenerator.Append("}");

    fragGenerator.AddUniform("color", "vec3");
    fragGenerator.AddUniform("opacity", "float");
    fragGenerator.Append("void main() {");
    fragGenerator.Append("\tgl_FragColor.rgb = color;");
    fragGenerator.Append("\tgl_FragColor.a = opacity;");
    fragGenerator.Append("}");

    return inWidgetContext.CompileAndStoreShader(itemName);
}

NVRenderShaderProgram *SVisualAidWidget::createBillboardShader(
        IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext)
{
    CRegisteredString itemName = inRenderContext.GetStringTable().RegisterStr("BillboardShader");
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
    vertGenerator.AddIncoming("attr_tc", "vec2");
    vertGenerator.AddUniform("billboardMatrix", "mat4");
    vertGenerator.AddUniform("anchor", "vec2");
    vertGenerator.AddOutgoing("texcoord", "vec2");

    vertGenerator.Append("void main() {");
    vertGenerator.Append("texcoord = attr_tc;");
    vertGenerator.Append("\tgl_Position = billboardMatrix * vec4(attr_pos.x + anchor.x, " \
                         "\tattr_pos.y + anchor.y, attr_pos.z, 1.0);");
    vertGenerator.Append("}");

    fragGenerator.AddIncoming("texcoord", "vec2");
    fragGenerator.AddUniform("color", "vec3");
    fragGenerator.AddUniform("opacity", "float");
    fragGenerator.AddUniform("billboardTexture", "sampler2D");
    fragGenerator.Append("void main() {");
    fragGenerator.Append("\tgl_FragColor.rgb = texture2D(billboardTexture, texcoord).rgb;");
    fragGenerator.Append("\tgl_FragColor.a = texture2D(billboardTexture, texcoord).a * opacity;");
    //fragGenerator.Append("\tif (gl_FragColor.a <= 0.005) discard; ");
    fragGenerator.Append("}");

    return inWidgetContext.CompileAndStoreShader(itemName);
}

void SVisualAidWidget::renderCamera(SNode *node, IRenderWidgetContext &inWidgetContext,
                                    NVRenderContext &inRenderContext)
{
    SCamera *camera = static_cast<SCamera *>(node);
    Q_ASSERT(camera);
    QT3DSMat44 projection = camera->m_Projection;
    QT3DSMat44 ip = projection.getInverse();

    if (!m_cameraBox)
        m_cameraBox = createCameraBox(inWidgetContext, inRenderContext);
    if (!m_renderCameraShader)
        m_renderCameraShader = createRenderCameraShader(inWidgetContext, inRenderContext);

    inRenderContext.SetInputAssembler(m_cameraBox);
    inRenderContext.SetActiveShader(m_renderCameraShader);
    inRenderContext.SetBlendingEnabled(false);
    inRenderContext.SetDepthTestEnabled(true);
    inRenderContext.SetDepthWriteEnabled(false);
    inRenderContext.SetCullingEnabled(false);

    SWidgetRenderSetupResult theSetup(inWidgetContext, *node, RenderWidgetModes::Local);

    m_renderCameraShader->SetPropertyValue("cameraMaxLength", cameraMaxLength);
    m_renderCameraShader->SetPropertyValue("orthographic",
                                           camera->m_Flags.IsOrthographic() ? 1 : 0);
    m_renderCameraShader->SetPropertyValue("invProjMatrix", ip);
    m_renderCameraShader->SetPropertyValue("mvpMatrix", theSetup.m_PureProjection
                                                        * theSetup.m_WidgetInfo.m_NodeParentToCamera
                                                        * node->m_GlobalTransform);

    ::CColor color = CStudioPreferences::GetSingleBoundingBoxColor();
    QT3DSVec3 colorVec(color.GetRed() / 255.f,
                       color.GetGreen() / 255.f,
                       color.GetBlue() / 255.f);

    m_renderCameraShader->SetPropertyValue("color", m_selected ? colorVec : QT3DSVec3(1, 1, 1));
    inRenderContext.SetBlendFunction(
                qt3ds::render::NVRenderBlendFunctionArgument(
                    qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
                    qt3ds::render::NVRenderSrcBlendFunc::One,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha));
    inRenderContext.SetBlendEquation(
                qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));
    inRenderContext.SetBlendingEnabled(true);
    m_renderCameraShader->SetPropertyValue("opacity", 0.5f);

    inRenderContext.Draw(NVRenderDrawMode::Lines, m_cameraBox->GetIndexCount(), 0);
}

void SVisualAidWidget::renderBillboard(SNode *node, IRenderWidgetContext &inWidgetContext,
                                       NVRenderContext &inRenderContext)
{
    if (!m_billboard)
        m_billboard = createBillboard(inWidgetContext, inRenderContext);
    if (!m_billboardShader)
        m_billboardShader = createBillboardShader(inWidgetContext, inRenderContext);

    if (!m_billboardCameraTexture) {
        QImage img(":/images/Asset-Camera-Pick.png");
        img = img.mirrored();
        img = img.rgbSwapped();
        m_billboardCameraTexture = inRenderContext.CreateTexture2D();
        NVDataRef<QT3DSU8> data(img.bits(), img.byteCount());
        m_billboardCameraTexture->SetTextureData(data, 0, img.width(), img.height(),
                                           NVRenderTextureFormats::RGBA8);

        img = QImage(":/images/Asset-Light-Pick.png");
        img = img.mirrored();
        img = img.rgbSwapped();
        m_billboardLightTexture = inRenderContext.CreateTexture2D();
        data = NVDataRef<QT3DSU8>(img.bits(), img.byteCount());
        m_billboardLightTexture->SetTextureData(data, 0, img.width(), img.height(),
                                                NVRenderTextureFormats::RGBA8);
    }

    inRenderContext.SetInputAssembler(m_billboard);
    inRenderContext.SetActiveShader(m_billboardShader);
    inRenderContext.SetBlendingEnabled(true);
    inRenderContext.SetDepthTestEnabled(true);
    inRenderContext.SetDepthWriteEnabled(false);
    inRenderContext.SetCullingEnabled(false);
    inRenderContext.SetBlendFunction(
                qt3ds::render::NVRenderBlendFunctionArgument(
                    qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
                    qt3ds::render::NVRenderSrcBlendFunc::Zero,
                    qt3ds::render::NVRenderDstBlendFunc::One));
    inRenderContext.SetBlendEquation(
                qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));

    SWidgetRenderSetupResult theSetup(inWidgetContext, *node, RenderWidgetModes::Local);
    QT3DSMat44 billboardMatrix = QT3DSMat44::createIdentity();
    billboardMatrix.setPosition((theSetup.m_WidgetInfo.m_NodeParentToCamera
                                * node->m_GlobalTransform).getPosition());
    billboardMatrix.scale(QT3DSVec4(8, 8, 1, 1));
    billboardMatrix = theSetup.m_PureProjection * billboardMatrix;

    m_billboardShader->SetPropertyValue("billboardMatrix", billboardMatrix);
    m_billboardShader->SetPropertyValue("billboardTexture",
                                        node->m_Type == GraphObjectTypes::Camera
                                        ? m_billboardCameraTexture.mPtr
                                        : m_billboardLightTexture.mPtr);
    m_billboardShader->SetPropertyValue("opacity", m_selected ? 0.9f : 0.7f);
    m_billboardShader->SetPropertyValue("anchor", QT3DSVec2(-1, -1));

    inRenderContext.Draw(NVRenderDrawMode::Triangles, m_billboard->GetIndexCount(), 0);
}

void SVisualAidWidget::renderLight(SNode *node, IRenderWidgetContext &inWidgetContext,
                                   NVRenderContext &inRenderContext)
{
    SLight *light = static_cast<SLight *>(node);
    Q_ASSERT(light);

    NVRenderInputAssembler *input = nullptr;
    QT3DSMat44 areaScaleMatrix = QT3DSMat44::createIdentity();

    switch (light->m_LightType) {
    case RenderLightTypes::Directional: {
        if (!m_directionalLight)
            m_directionalLight = createDirectionalLight(inWidgetContext, inRenderContext);
        input = m_directionalLight;
    } break;

    case RenderLightTypes::Point: {
        if (!m_pointLight)
            m_pointLight = createPointLight(inWidgetContext, inRenderContext);
        input = m_pointLight;
    } break;

    case RenderLightTypes::Area: {
        const float w = light->m_AreaWidth * 0.5f;
        const float h = light->m_AreaHeight * 0.5f;
        areaScaleMatrix.scale(QT3DSVec4(w, h, 0.0, 1.0));
        if (!m_areaLight)
            m_areaLight = createAreaLight(inWidgetContext, inRenderContext);
        input = m_areaLight;
    } break;

    default:
        break;
    }
    if (input) {
        if (!m_renderShader)
            m_renderShader = createRenderShader(inWidgetContext, inRenderContext);
        inRenderContext.SetInputAssembler(input);
        inRenderContext.SetActiveShader(m_renderShader);
        inRenderContext.SetBlendingEnabled(false);
        inRenderContext.SetDepthTestEnabled(true);
        inRenderContext.SetDepthWriteEnabled(false);
        inRenderContext.SetCullingEnabled(false);

        SWidgetRenderSetupResult theSetup(inWidgetContext, *node, RenderWidgetModes::Local);

        m_renderShader->SetPropertyValue("mvpMatrix", theSetup.m_PureProjection
                                            * theSetup.m_WidgetInfo.m_NodeParentToCamera
                                            * node->m_GlobalTransform
                                            * areaScaleMatrix);

        ::CColor color = CStudioPreferences::GetSingleBoundingBoxColor();
        QT3DSVec3 colorVec(color.GetRed() / 255.f,
                           color.GetGreen() / 255.f,
                           color.GetBlue() / 255.f);

        m_renderShader->SetPropertyValue("color", m_selected ? colorVec : QT3DSVec3(1, 1, 1));

        inRenderContext.SetBlendFunction(
                    qt3ds::render::NVRenderBlendFunctionArgument(
                        qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
                        qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
                        qt3ds::render::NVRenderSrcBlendFunc::One,
                        qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha));
        inRenderContext.SetBlendEquation(
                    qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));
        inRenderContext.SetBlendingEnabled(true);
        m_renderShader->SetPropertyValue("opacity", 0.5f);
        inRenderContext.Draw(NVRenderDrawMode::Lines, input->GetIndexCount(), 0);
    }
}

void SVisualAidWidget::SetNode(SNode *inNode)
{
    if (inNode == m_node)
        return;
    m_node = inNode;
}

void SVisualAidWidget::Render(IRenderWidgetContext &inWidgetContext,
                              NVRenderContext &inRenderContext)
{
    switch (m_node->m_Type) {
    case GraphObjectTypes::Camera:
        renderCamera(m_node, inWidgetContext, inRenderContext);
        break;

    case GraphObjectTypes::Light:
        renderLight(m_node, inWidgetContext, inRenderContext);
        break;

    default:
        break;
    }
    renderBillboard(m_node, inWidgetContext, inRenderContext);
}

bool SVisualAidWidget::pick(IRenderWidgetContext &inWidgetContext, float &dist, QT3DSVec2 viewport,
                            QT3DSVec2 pos)
{
    SWidgetRenderSetupResult theSetup(inWidgetContext, *m_node, RenderWidgetModes::Local);
    SCamera *pickCamera = theSetup.m_WidgetInfo.m_Camera;
    QT3DSMat44 pip = pickCamera->m_Projection.getInverse();
    float x = pos.x / viewport.x;
    float y = 1.0f - pos.y / viewport.y;
    x = 2.0 * x - 1.0;
    y = 2.0 * y - 1.0;

    QT3DSVec2 anchor(-1, -1);

    QT3DSVec4 n(x, y, -1, 1), f(x, y, 1, 1);
    QT3DSVec4 np = pip.transform(n);
    QT3DSVec4 fp = pip.transform(f);

    np = np * (1.0 / np.w);
    fp = fp * (1.0 / fp.w);

    QT3DSMat44 billboardMatrix = QT3DSMat44::createIdentity();
    billboardMatrix.setPosition((theSetup.m_WidgetInfo.m_NodeParentToCamera
                                *m_node->m_GlobalTransform).getPosition());
    billboardMatrix.scale(QT3DSVec4(8, 8, 1, 1));

    QT3DSMat44 toBillboard = billboardMatrix.getInverse();

    np = toBillboard.transform(np);
    fp = toBillboard.transform(fp);

    QT3DSVec3 c = np.getXYZ();
    QT3DSVec3 dir = (fp - np).getXYZ();
    dir.normalize();

    QT3DSVec2 min(anchor.x - 1, anchor.y - 1);
    QT3DSVec2 max(anchor.x + 1, anchor.y + 1);

    if (qAbs(dir.z) > fepsilon) {
        QT3DSVec2 xy = QT3DSVec2(dir.x / dir.z, dir.y / dir.z);
        QT3DSVec2 p = xy * (-c.z) + QT3DSVec2(c.x, c.y);
        if (p.x >= min.x && p.x <= max.x &&
            p.y >= min.y && p.y <= max.y) {
            QT3DSVec3 ip = QT3DSVec3(p.x, p.y, 0) - c;
            dist = ip.magnitude();
            return true;
        }
    }

    return false;
}

SVisualAidWidget &SVisualAidWidget::CreateVisualAidWidget(NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, SVisualAidWidget)(inAlloc);
}

}
}
