/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderCustomMaterialRenderContext.h"
#include "Qt3DSRenderContextCore.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSRenderCustomMaterial.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderResourceManager.h"
#include "Qt3DSRenderMesh.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderLayer.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderComputeShader.h"
#include "foundation/PreAllocatedAllocator.h"
#include "foundation/SerializationTypes.h"
#include "foundation/Qt3DSTime.h"
#include "foundation/StringConversionImpl.h" //snprintf
#include "Qt3DSRenderDynamicObjectSystemUtil.h"
#include "Qt3DSRenderableImage.h"
#include "../RendererImpl/Qt3DSVertexPipelineImpl.h"
#include "../RendererImpl/Qt3DSRendererImplLayerRenderData.h"
#include "Qt3DSRenderCustomMaterialShaderGenerator.h"
#include "Qt3DSRenderModel.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace qt3ds::render;
using namespace qt3ds::render::dynamic;
using qt3ds::render::NVRenderContextScopedProperty;
using qt3ds::render::NVRenderCachedShaderProperty;
using qt3ds::render::NVRenderCachedShaderBuffer;

SCustomMaterialVertexPipeline::SCustomMaterialVertexPipeline(IUICRenderContext *inContext,
                                                             TessModeValues::Enum inTessMode)
    : SVertexPipelineImpl(
          inContext->GetAllocator(), inContext->GetCustomMaterialShaderGenerator(),
          inContext->GetShaderProgramGenerator(), inContext->GetStringTable(), false)
    , m_UICContext(inContext)
    , m_TessMode(TessModeValues::NoTess)
{
    if (m_UICContext->GetRenderContext().IsTessellationSupported()) {
        m_TessMode = inTessMode;
    }

    if (m_UICContext->GetRenderContext().IsGeometryStageSupported()
        && m_TessMode != TessModeValues::NoTess) {
        m_Wireframe = inContext->GetWireframeMode();
    }
}

void SCustomMaterialVertexPipeline::InitializeTessControlShader()
{
    if (m_TessMode == TessModeValues::NoTess
        || ProgramGenerator().GetStage(ShaderGeneratorStages::TessControl) == NULL) {
        return;
    }

    IShaderStageGenerator &tessCtrlShader(
        *ProgramGenerator().GetStage(ShaderGeneratorStages::TessControl));

    tessCtrlShader.AddUniform("tessLevelInner", "float");
    tessCtrlShader.AddUniform("tessLevelOuter", "float");

    SetupTessIncludes(ShaderGeneratorStages::TessControl, m_TessMode);

    tessCtrlShader.Append("void main() {\n");

    tessCtrlShader.Append("\tctWorldPos[0] = varWorldPos[0];");
    tessCtrlShader.Append("\tctWorldPos[1] = varWorldPos[1];");
    tessCtrlShader.Append("\tctWorldPos[2] = varWorldPos[2];");

    if (m_TessMode == TessModeValues::TessPhong || m_TessMode == TessModeValues::TessNPatch) {
        tessCtrlShader.Append("\tctNorm[0] = varObjectNormal[0];");
        tessCtrlShader.Append("\tctNorm[1] = varObjectNormal[1];");
        tessCtrlShader.Append("\tctNorm[2] = varObjectNormal[2];");
    }
    if (m_TessMode == TessModeValues::TessNPatch) {
        tessCtrlShader.Append("\tctTangent[0] = varObjTangent[0];");
        tessCtrlShader.Append("\tctTangent[1] = varObjTangent[1];");
        tessCtrlShader.Append("\tctTangent[2] = varObjTangent[2];");
    }

    tessCtrlShader.Append(
        "\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
    tessCtrlShader.Append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
}

void SCustomMaterialVertexPipeline::InitializeTessEvaluationShader()
{
    if (m_TessMode == TessModeValues::NoTess
        || ProgramGenerator().GetStage(ShaderGeneratorStages::TessEval) == NULL) {
        return;
    }

    IShaderStageGenerator &tessEvalShader(
        *ProgramGenerator().GetStage(ShaderGeneratorStages::TessEval));

    tessEvalShader.AddUniform("model_view_projection", "mat4");
    tessEvalShader.AddUniform("normal_matrix", "mat3");

    SetupTessIncludes(ShaderGeneratorStages::TessEval, m_TessMode);

    if (m_TessMode == TessModeValues::TessLinear && m_DisplacementImage) {
        tessEvalShader.AddInclude("defaultMaterialFileDisplacementTexture.glsllib");
        tessEvalShader.AddUniform("model_matrix", "mat4");
        tessEvalShader.AddUniform("displace_tiling", "vec3");
        tessEvalShader.AddUniform("displaceAmount", "float");
        tessEvalShader.AddUniform(m_DisplacementImage->m_Image.m_ImageShaderName.c_str(),
                                  "sampler2D");
    }

    tessEvalShader.Append("void main() {");

    if (m_TessMode == TessModeValues::TessNPatch) {
        tessEvalShader.Append("\tctNorm[0] = varObjectNormalTC[0];");
        tessEvalShader.Append("\tctNorm[1] = varObjectNormalTC[1];");
        tessEvalShader.Append("\tctNorm[2] = varObjectNormalTC[2];");

        tessEvalShader.Append("\tctTangent[0] = varTangentTC[0];");
        tessEvalShader.Append("\tctTangent[1] = varTangentTC[1];");
        tessEvalShader.Append("\tctTangent[2] = varTangentTC[2];");
    }

    tessEvalShader.Append("\tvec4 pos = tessShader( );\n");
}

void SCustomMaterialVertexPipeline::FinalizeTessControlShader()
{
    IShaderStageGenerator &tessCtrlShader(
        *ProgramGenerator().GetStage(ShaderGeneratorStages::TessControl));
    // add varyings we must pass through
    typedef TStrTableStrMap::const_iterator TParamIter;
    for (TParamIter iter = m_InterpolationParameters.begin(),
                    end = m_InterpolationParameters.end();
         iter != end; ++iter) {
        tessCtrlShader << "\t" << iter->first.c_str()
                       << "TC[gl_InvocationID] = " << iter->first.c_str()
                       << "[gl_InvocationID];\n";
    }
}

void SCustomMaterialVertexPipeline::FinalizeTessEvaluationShader()
{
    IShaderStageGenerator &tessEvalShader(
        *ProgramGenerator().GetStage(ShaderGeneratorStages::TessEval));

    eastl::string outExt("");
    if (ProgramGenerator().GetEnabledStages() & ShaderGeneratorStages::Geometry)
        outExt = "TE";

    // add varyings we must pass through
    typedef TStrTableStrMap::const_iterator TParamIter;
    if (m_TessMode == TessModeValues::TessNPatch) {
        for (TParamIter iter = m_InterpolationParameters.begin(),
                        end = m_InterpolationParameters.end();
             iter != end; ++iter) {
            tessEvalShader << "\t" << iter->first.c_str() << outExt.c_str()
                           << " = gl_TessCoord.z * " << iter->first.c_str() << "TC[0] + ";
            tessEvalShader << "gl_TessCoord.x * " << iter->first.c_str() << "TC[1] + ";
            tessEvalShader << "gl_TessCoord.y * " << iter->first.c_str() << "TC[2];\n";
        }

        // transform the normal
        if (m_GenerationFlags & GenerationFlagValues::WorldNormal)
            tessEvalShader << "\n\tvarNormal" << outExt.c_str()
                           << " = normalize(normal_matrix * teNorm);\n";
        // transform the tangent
        if (m_GenerationFlags & GenerationFlagValues::TangentBinormal) {
            tessEvalShader << "\n\tvarTangent" << outExt.c_str()
                           << " = normalize(normal_matrix * teTangent);\n";
            // transform the binormal
            tessEvalShader << "\n\tvarBinormal" << outExt.c_str()
                           << " = normalize(normal_matrix * teBinormal);\n";
        }
    } else {
        for (TParamIter iter = m_InterpolationParameters.begin(),
                        end = m_InterpolationParameters.end();
             iter != end; ++iter) {
            tessEvalShader << "\t" << iter->first.c_str() << outExt.c_str()
                           << " = gl_TessCoord.x * " << iter->first.c_str() << "TC[0] + ";
            tessEvalShader << "gl_TessCoord.y * " << iter->first.c_str() << "TC[1] + ";
            tessEvalShader << "gl_TessCoord.z * " << iter->first.c_str() << "TC[2];\n";
        }

        // displacement mapping makes only sense with linear tessellation
        if (m_TessMode == TessModeValues::TessLinear && m_DisplacementImage) {
            tessEvalShader
                << "\ttexture_coordinate_info tmp = textureCoordinateInfo( varTexCoord0"
                << outExt.c_str() << ", varTangent" << outExt.c_str() << ", varBinormal"
                << outExt.c_str() << " );" << Endl;
            tessEvalShader << "\ttmp = transformCoordinate( rotationTranslationScale( vec3( "
                              "0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, "
                              "0.000000 ), displace_tiling ), tmp);"
                           << Endl;

            tessEvalShader << "\tpos.xyz = defaultMaterialFileDisplacementTexture( "
                           << m_DisplacementImage->m_Image.m_ImageShaderName.c_str()
                           << ", displaceAmount, "
                           << "tmp.position.xy";
            tessEvalShader << ", varObjectNormal" << outExt.c_str() << ", pos.xyz );" << Endl;
            tessEvalShader << "\tvarWorldPos" << outExt.c_str() << "= (model_matrix * pos).xyz;"
                           << Endl;
        }

        // transform the normal
        tessEvalShader << "\n\tvarNormal" << outExt.c_str()
                       << " = normalize(normal_matrix * varObjectNormal" << outExt.c_str()
                       << ");\n";
    }

    tessEvalShader.Append("\tgl_Position = model_view_projection * pos;\n");
}

// Responsible for beginning all vertex and fragment generation (void main() { etc).
void SCustomMaterialVertexPipeline::BeginVertexGeneration(QT3DSU32 displacementImageIdx,
                                   SRenderableImage *displacementImage)
{
    m_DisplacementIdx = displacementImageIdx;
    m_DisplacementImage = displacementImage;

    TShaderGeneratorStageFlags theStages(IShaderProgramGenerator::DefaultFlags());

    if (m_TessMode != TessModeValues::NoTess) {
        theStages |= ShaderGeneratorStages::TessControl;
        theStages |= ShaderGeneratorStages::TessEval;
    }
    if (m_Wireframe) {
        theStages |= ShaderGeneratorStages::Geometry;
    }

    ProgramGenerator().BeginProgram(theStages);

    if (m_TessMode != TessModeValues::NoTess) {
        InitializeTessControlShader();
        InitializeTessEvaluationShader();
    }
    if (m_Wireframe) {
        InitializeWireframeGeometryShader();
    }

    IShaderStageGenerator &vertexShader(Vertex());

    // thinks we need
    vertexShader.AddInclude("viewProperties.glsllib");
    vertexShader.AddInclude("customMaterial.glsllib");

    vertexShader.AddIncoming("attr_pos", "vec3");
    vertexShader << "void main()" << Endl << "{" << Endl;

    if (displacementImage) {
        GenerateUVCoords(0);
        if (!HasTessellation()) {
            vertexShader.AddUniform("displaceAmount", "float");
            vertexShader.AddUniform("displace_tiling", "vec3");
            // we create the world position setup here
            // because it will be replaced with the displaced position
            SetCode(GenerationFlagValues::WorldPosition);
            vertexShader.AddUniform("model_matrix", "mat4");

            vertexShader.AddInclude("defaultMaterialFileDisplacementTexture.glsllib");
            vertexShader.AddUniform(displacementImage->m_Image.m_ImageShaderName.c_str(),
                                    "sampler2D");

            vertexShader << "\ttexture_coordinate_info tmp = textureCoordinateInfo( texCoord0, "
                            "varTangent, varBinormal );"
                         << Endl;
            vertexShader << "\ttmp = transformCoordinate( rotationTranslationScale( vec3( "
                            "0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, "
                            "0.000000 ), displace_tiling ), tmp);"
                         << Endl;

            vertexShader << "\tvec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                         << displacementImage->m_Image.m_ImageShaderName.c_str()
                         << ", displaceAmount, "
                         << "tmp.position.xy"
                         << ", attr_norm, attr_pos );" << Endl;

            AddInterpolationParameter("varWorldPos", "vec3");
            vertexShader.Append("\tvec3 local_model_world_position = (model_matrix * "
                                "vec4(displacedPos, 1.0)).xyz;");
            AssignOutput("varWorldPos", "local_model_world_position");
        }
    }

    if (HasTessellation()) {
        vertexShader.Append("\tgl_Position = vec4(attr_pos, 1.0);");
    } else {
        vertexShader.AddUniform("model_view_projection", "mat4");
        if (displacementImage)
            vertexShader.Append(
                "\tgl_Position = model_view_projection * vec4(displacedPos, 1.0);");
        else
            vertexShader.Append("\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
    }

    if (HasTessellation()) {
        GenerateWorldPosition();
        GenerateWorldNormal();
        GenerateObjectNormal();
        GenerateVarTangentAndBinormal();
    }
}

void SCustomMaterialVertexPipeline::BeginFragmentGeneration()
{
    Fragment() << "void main()" << Endl << "{" << Endl;
}

void SCustomMaterialVertexPipeline::AssignOutput(const char8_t *inVarName,
                                                 const char8_t *inVarValue)
{
    Vertex() << "\t" << inVarName << " = " << inVarValue << ";\n";
}

void SCustomMaterialVertexPipeline::GenerateUVCoords(QT3DSU32 inUVSet)
{
    if (inUVSet == 0 && SetCode(GenerationFlagValues::UVCoords))
        return;
    if (inUVSet == 1 && SetCode(GenerationFlagValues::UVCoords1))
        return;

    QT3DS_ASSERT(inUVSet == 0 || inUVSet == 1);

    if (inUVSet == 0)
        AddInterpolationParameter("varTexCoord0", "vec3");
    else if (inUVSet == 1)
        AddInterpolationParameter("varTexCoord1", "vec3");

    DoGenerateUVCoords(inUVSet);
}

void SCustomMaterialVertexPipeline::GenerateWorldNormal()
{
    if (SetCode(GenerationFlagValues::WorldNormal))
        return;
    AddInterpolationParameter("varNormal", "vec3");
    DoGenerateWorldNormal();
}

void SCustomMaterialVertexPipeline::GenerateObjectNormal()
{
    if (SetCode(GenerationFlagValues::ObjectNormal))
        return;
    DoGenerateObjectNormal();
}

void SCustomMaterialVertexPipeline::GenerateVarTangentAndBinormal()
{
    if (SetCode(GenerationFlagValues::TangentBinormal))
        return;
    AddInterpolationParameter("varTangent", "vec3");
    AddInterpolationParameter("varBinormal", "vec3");
    AddInterpolationParameter("varObjTangent", "vec3");
    AddInterpolationParameter("varObjBinormal", "vec3");
    DoGenerateVarTangentAndBinormal();
}

void SCustomMaterialVertexPipeline::GenerateWorldPosition()
{
    if (SetCode(GenerationFlagValues::WorldPosition))
        return;

    ActiveStage().AddUniform("model_matrix", "mat4");
    AddInterpolationParameter("varWorldPos", "vec3");
    AddInterpolationParameter("varObjPos", "vec3");
    DoGenerateWorldPosition();
}

// responsible for closing all vertex and fragment generation
void SCustomMaterialVertexPipeline::EndVertexGeneration()
{
    if (HasTessellation()) {
        // finalize tess control shader
        FinalizeTessControlShader();
        // finalize tess evaluation shader
        FinalizeTessEvaluationShader();

        TessControl().Append("}");
        TessEval().Append("}");

        if (m_Wireframe) {
            // finalize geometry shader
            FinalizeWireframeGeometryShader();
            Geometry().Append("}");
        }
    }

    Vertex().Append("}");
}

void SCustomMaterialVertexPipeline::EndFragmentGeneration()
{
    Fragment().Append("}");
}

IShaderStageGenerator &SCustomMaterialVertexPipeline::ActiveStage()
{
    return Vertex();
}

void SCustomMaterialVertexPipeline::AddInterpolationParameter(const char8_t *inName,
                                                              const char8_t *inType)
{
    m_InterpolationParameters.insert(eastl::make_pair(Str(inName), Str(inType)));
    Vertex().AddOutgoing(inName, inType);
    Fragment().AddIncoming(inName, inType);

    if (HasTessellation()) {
        eastl::string nameBuilder(inName);
        nameBuilder.append("TC");
        TessControl().AddOutgoing(nameBuilder.c_str(), inType);

        nameBuilder.assign(inName);
        if (ProgramGenerator().GetEnabledStages() & ShaderGeneratorStages::Geometry) {
            nameBuilder.append("TE");
            Geometry().AddOutgoing(inName, inType);
        }
        TessEval().AddOutgoing(nameBuilder.c_str(), inType);
    }
}

void SCustomMaterialVertexPipeline::DoGenerateUVCoords(QT3DSU32 inUVSet)
{
    QT3DS_ASSERT(inUVSet == 0 || inUVSet == 1);

    if (inUVSet == 0) {
        Vertex().AddIncoming("attr_uv0", "vec2");
        Vertex() << "\tvec3 texCoord0 = vec3( attr_uv0, 0.0 );" << Endl;
        AssignOutput("varTexCoord0", "texCoord0");
    } else if (inUVSet == 1) {
        Vertex().AddIncoming("attr_uv1", "vec2");
        Vertex() << "\tvec3 texCoord1 = vec3( attr_uv1, 1.0 );" << Endl;
        AssignOutput("varTexCoord1", "texCoord1");
    }
}

void SCustomMaterialVertexPipeline::DoGenerateWorldNormal()
{
    IShaderStageGenerator &vertexGenerator(Vertex());
    vertexGenerator.AddIncoming("attr_norm", "vec3");
    vertexGenerator.AddUniform("normal_matrix", "mat3");

    if (HasTessellation() == false) {
        Vertex().Append("\tvarNormal = normalize( normal_matrix * attr_norm );");
    }
}

void SCustomMaterialVertexPipeline::DoGenerateObjectNormal()
{
    AddInterpolationParameter("varObjectNormal", "vec3");
    Vertex().Append("\tvarObjectNormal = attr_norm;");
}

void SCustomMaterialVertexPipeline::DoGenerateWorldPosition()
{
    Vertex().Append("\tvarObjPos = attr_pos;");
    Vertex().Append("\tvec4 worldPos = (model_matrix * vec4(attr_pos, 1.0));");
    AssignOutput("varWorldPos", "worldPos.xyz");
}

void SCustomMaterialVertexPipeline::DoGenerateVarTangentAndBinormal()
{
    Vertex().AddIncoming("attr_textan", "vec3");
    Vertex().AddIncoming("attr_binormal", "vec3");

    Vertex() << "\tvarTangent = normal_matrix * attr_textan;" << Endl
             << "\tvarBinormal = normal_matrix * attr_binormal;" << Endl;

    Vertex() << "\tvarObjTangent = attr_textan;" << Endl << "\tvarObjBinormal = attr_binormal;"
             << Endl;
}


struct SMaterialClass
{
    NVAllocatorCallback *m_Allocator;
    IDynamicObjectClass *m_Class;
    bool m_HasTransparency;
    bool m_HasRefraction;
    bool m_HasDisplacement;
    bool m_AlwaysDirty;
    QT3DSU32 m_ShaderKey;
    QT3DSU32 m_LayerCount;
    QT3DSI32 mRefCount;
    SMaterialClass(NVAllocatorCallback &alloc, IDynamicObjectClass &inCls)
        : m_Allocator(&alloc)
        , m_Class(&inCls)
        , m_HasTransparency(false)
        , m_HasRefraction(false)
        , m_HasDisplacement(false)
        , m_AlwaysDirty(false)
        , m_ShaderKey(0)
        , m_LayerCount(0)
        , mRefCount(0)
    {
    }

    ~SMaterialClass() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(*m_Allocator)

    void AfterWrite()
    {
        m_Allocator = NULL;
        m_Class = NULL;
        mRefCount = 0;
    }

    void AfterRead(NVAllocatorCallback &alloc, IDynamicObjectClass &inCls)
    {
        m_Allocator = &alloc;
        m_Class = &inCls;
        mRefCount = 0;
    }
};

typedef nvhash_map<CRegisteredString, NVScopedRefCounted<SMaterialClass>> TStringMaterialMap;
typedef eastl::pair<CRegisteredString, CRegisteredString> TStrStrPair;

namespace eastl {
template <>
struct hash<TStrStrPair>
{
    size_t operator()(const TStrStrPair &item) const
    {
        return hash<CRegisteredString>()(item.first) ^ hash<CRegisteredString>()(item.second);
    }
};
}

struct SShaderMapKey
{
    TStrStrPair m_Name;
    eastl::vector<SShaderPreprocessorFeature> m_Features;
    TessModeValues::Enum m_TessMode;
    bool m_WireframeMode;
    SShaderDefaultMaterialKey m_MaterialKey;
    size_t m_HashCode;
    SShaderMapKey(TStrStrPair inName, TShaderFeatureSet inFeatures, TessModeValues::Enum inTessMode,
                  bool inWireframeMode, SShaderDefaultMaterialKey inMaterialKey)
        : m_Name(inName)
        , m_Features(inFeatures.begin(), inFeatures.end())
        , m_TessMode(inTessMode)
        , m_WireframeMode(inWireframeMode)
        , m_MaterialKey(inMaterialKey)
    {
        m_HashCode = eastl::hash<TStrStrPair>()(m_Name)
            ^ HashShaderFeatureSet(toDataRef(m_Features.data(), (QT3DSU32)m_Features.size()))
            ^ eastl::hash<QT3DSU32>()(m_TessMode) ^ eastl::hash<bool>()(m_WireframeMode)
            ^ eastl::hash<size_t>()(inMaterialKey.hash());
    }
    bool operator==(const SShaderMapKey &inKey) const
    {
        return m_Name == inKey.m_Name && m_Features == inKey.m_Features
            && m_TessMode == inKey.m_TessMode && m_WireframeMode == inKey.m_WireframeMode
            && m_MaterialKey == inKey.m_MaterialKey;
    }
};

namespace eastl {
template <>
struct hash<SShaderMapKey>
{
    size_t operator()(const SShaderMapKey &inKey) const { return inKey.m_HashCode; }
};
}

namespace {

struct SCustomMaterialTextureData
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    qt3ds::render::NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Sampler;
    qt3ds::render::NVRenderTexture2D *m_Texture;
    bool m_needsMips;
    volatile QT3DSI32 mRefCount;

    SCustomMaterialTextureData(NVRenderShaderProgram &inShader, NVRenderTexture2D *inTexture,
                               const char *inTexName, bool needMips)
        : m_Shader(inShader)
        , m_Sampler(inTexName, inShader)
        , m_Texture(inTexture)
        , m_needsMips(needMips)
        , mRefCount(0)
    {
    }

    void Set(const SPropertyDefinition *inDefinition)
    {
        if (m_Texture && inDefinition) {
            m_Texture->SetMagFilter(inDefinition->m_MagFilterOp);
            m_Texture->SetMinFilter(inDefinition->m_MinFilterOp);
            m_Texture->SetTextureWrapS(inDefinition->m_CoordOp);
            m_Texture->SetTextureWrapT(inDefinition->m_CoordOp);
        } else if (m_Texture) {
            // set some defaults
            m_Texture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
            m_Texture->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            m_Texture->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);
        }

        if ((m_Texture->GetNumMipmaps() == 0) && m_needsMips)
            m_Texture->GenerateMipmaps();

        m_Sampler.Set(m_Texture);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())

    static SCustomMaterialTextureData CreateTextureEntry(NVRenderShaderProgram &inShader,
                                                         NVRenderTexture2D *inTexture,
                                                         const char *inTexName, bool needMips)
    {
        return SCustomMaterialTextureData(inShader, inTexture, inTexName, needMips);
    }
};

typedef eastl::pair<CRegisteredString, NVScopedRefCounted<SCustomMaterialTextureData>>
    TCustomMaterialTextureEntry;

/**
 *	Cached tessellation property lookups this is on a per mesh base
 */
struct SCustomMaterialsTessellationProperties
{
    NVRenderCachedShaderProperty<QT3DSF32> m_EdgeTessLevel; ///< tesselation value for the edges
    NVRenderCachedShaderProperty<QT3DSF32> m_InsideTessLevel; ///< tesselation value for the inside
    NVRenderCachedShaderProperty<QT3DSF32>
        m_PhongBlend; ///< blending between linear and phong component
    NVRenderCachedShaderProperty<QT3DSVec2>
        m_DistanceRange; ///< distance range for min and max tess level
    NVRenderCachedShaderProperty<QT3DSF32> m_DisableCulling; ///< if set to 1.0 this disables backface
                                                          ///culling optimization in the tess shader

    SCustomMaterialsTessellationProperties() {}
    SCustomMaterialsTessellationProperties(NVRenderShaderProgram &inShader)
        : m_EdgeTessLevel("tessLevelOuter", inShader)
        , m_InsideTessLevel("tessLevelInner", inShader)
        , m_PhongBlend("phongBlend", inShader)
        , m_DistanceRange("distanceRange", inShader)
        , m_DisableCulling("disableCulling", inShader)
    {
    }
};

/* We setup some shared state on the custom material shaders */
struct SCustomMaterialShader
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ModelMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewProjMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewMatrix;
    NVRenderCachedShaderProperty<QT3DSMat33> m_NormalMatrix;
    NVRenderCachedShaderProperty<QT3DSVec3> m_CameraPos;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ProjMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewportMatrix;
    NVRenderCachedShaderProperty<QT3DSVec2> m_CamProperties;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_DepthTexture;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_AOTexture;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LightProbe;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeProps;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeOpts;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeRot;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeOfs;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LightProbe2;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbe2Props;
    NVRenderCachedShaderProperty<QT3DSI32> m_LightCount;
    NVRenderCachedShaderProperty<QT3DSI32> m_AreaLightCount;
    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_AoShadowParams;
    SCustomMaterialsTessellationProperties m_Tessellation;
    SDynamicShaderProgramFlags m_ProgramFlags;
    volatile QT3DSI32 mRefCount;
    SCustomMaterialShader(NVRenderShaderProgram &inShader, SDynamicShaderProgramFlags inFlags)
        : m_Shader(inShader)
        , m_ModelMatrix("model_matrix", inShader)
        , m_ViewProjMatrix("model_view_projection", inShader)
        , m_ViewMatrix("view_matrix", inShader)
        , m_NormalMatrix("normal_matrix", inShader)
        , m_CameraPos("camera_position", inShader)
        , m_ProjMatrix("view_projection_matrix", inShader)
        , m_ViewportMatrix("viewport_matrix", inShader)
        , m_CamProperties("camera_properties", inShader)
        , m_DepthTexture("depth_sampler", inShader)
        , m_AOTexture("ao_sampler", inShader)
        , m_LightProbe("light_probe", inShader)
        , m_LightProbeProps("light_probe_props", inShader)
        , m_LightProbeOpts("light_probe_opts", inShader)
        , m_LightProbeRot("light_probe_rotation", inShader)
        , m_LightProbeOfs("light_probe_offset", inShader)
        , m_LightProbe2("light_probe2", inShader)
        , m_LightProbe2Props("light_probe2_props", inShader)
        , m_LightCount("uNumLights", inShader)
        , m_AreaLightCount("uNumAreaLights", inShader)
        , m_AoShadowParams("cbAoShadow", inShader)
        , m_Tessellation(inShader)
        , m_ProgramFlags(inFlags)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())
};

struct SMaterialOrComputeShader
{
    SCustomMaterialShader *m_MaterialShader;
    NVRenderShaderProgram *m_ComputeShader;
    SMaterialOrComputeShader()
        : m_MaterialShader(NULL)
        , m_ComputeShader(NULL)
    {
    }
    SMaterialOrComputeShader(SCustomMaterialShader &inMaterialShader)
        : m_MaterialShader(&inMaterialShader)
        , m_ComputeShader(NULL)
    {
    }
    SMaterialOrComputeShader(NVRenderShaderProgram &inComputeShader)
        : m_MaterialShader(NULL)
        , m_ComputeShader(&inComputeShader)
    {
        QT3DS_ASSERT(inComputeShader.GetProgramType() == NVRenderShaderProgram::ProgramType::Compute);
    }
    bool IsValid() const { return m_MaterialShader || m_ComputeShader; }
    bool IsComputeShader() const { return m_ComputeShader != NULL; }
    bool IsMaterialShader() const { return m_MaterialShader != NULL; }
    SCustomMaterialShader &MaterialShader()
    {
        QT3DS_ASSERT(IsMaterialShader());
        return *m_MaterialShader;
    }
    NVRenderShaderProgram &ComputeShader()
    {
        QT3DS_ASSERT(IsComputeShader());
        return *m_ComputeShader;
    }
};

struct SCustomMaterialBuffer
{
    CRegisteredString m_Name;
    NVScopedRefCounted<NVRenderFrameBuffer> m_FrameBuffer;
    NVScopedRefCounted<NVRenderTexture2D> m_Texture;
    SAllocateBufferFlags m_Flags;

    SCustomMaterialBuffer(CRegisteredString inName, NVRenderFrameBuffer &inFb,
                          NVRenderTexture2D &inTexture, SAllocateBufferFlags inFlags)
        : m_Name(inName)
        , m_FrameBuffer(&inFb)
        , m_Texture(&inTexture)
        , m_Flags(inFlags)
    {
    }
    SCustomMaterialBuffer() {}
};

struct SMaterialSystem;
typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderVertexBuffer>>
    TStringVertexBufferMap;
typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderInputAssembler>>
    TStringAssemblerMap;

struct SMaterialLuaContext
{
    SCustomMaterial &m_Material;
    SMaterialClass &m_Class;
    lua_State &m_LuaState;
    QT3DSU32 m_LuaId;
    SMaterialSystem &m_MaterialSystem;
    TStringVertexBufferMap m_VertexBuffers;
    TStringAssemblerMap m_InputAssemblers;
    QT3DSI32 m_RefCount;

    // Per frame render args.

    SCustomMaterialRenderContext *m_RenderContext;
    NVRenderFrameBuffer *m_Target;
    TShaderFeatureSet m_FeatureSet;
    SMaterialOrComputeShader m_CurrentShader;
    NVRenderInputAssembler *m_CurrentInputAssembler;

    SMaterialLuaContext(SCustomMaterial &inMaterial, SMaterialClass &inClass, lua_State &inLuaState,
                        QT3DSU32 inLuaId, SMaterialSystem &inMaterialSystem,
                        NVAllocatorCallback &inAllocator)
        : m_Material(inMaterial)
        , m_Class(inClass)
        , m_LuaState(inLuaState)
        , m_LuaId(inLuaId)
        , m_MaterialSystem(inMaterialSystem)
        , m_VertexBuffers(inAllocator, "m_VertexBuffers")
        , m_InputAssemblers(inAllocator, "m_InputAssemblers")
        , m_RefCount(0)
        , m_RenderContext(NULL)
        , m_Target(NULL)
        , m_CurrentInputAssembler(NULL)
    {
    }
    void Render(SCustomMaterialRenderContext &inRc, NVRenderFrameBuffer *inFb,
                TShaderFeatureSet inFS)
    {
        m_RenderContext = &inRc;
        m_Target = inFb;
        m_FeatureSet = inFS;
        m_CurrentShader = SMaterialOrComputeShader();
        m_CurrentInputAssembler = NULL;
        DoRender();
    }

    void addRef() { ++m_RefCount; }
    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0)
            DeleteThis();
    }

    ~SMaterialLuaContext();
    void DeleteThis();
    void DoRender();
    bool GetOrCreateVertexBuffer(const char *inIdStr, QT3DSU32 inSize, QT3DSU32 inStride);
    NVRenderVertexBuffer *GetVertexBuffer(const char *inIdStr);
    void OnActivationChange(bool inActive);
};

struct STopScope
{
    lua_State *m_LuaState;
    int m_Top;
    STopScope(lua_State *inState)
        : m_LuaState(inState)
        , m_Top(lua_gettop(inState))
    {
    }
    ~STopScope() { lua_settop(m_LuaState, m_Top); }
};

static void *LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize);

struct SStringMemoryBarrierFlagMap
{
    const char8_t *m_Name;
    qt3ds::render::NVRenderBufferBarrierValues::Enum m_Value;
    SStringMemoryBarrierFlagMap(const char8_t *nm,
                                qt3ds::render::NVRenderBufferBarrierValues::Enum val)
        : m_Name(nm)
        , m_Value(val)
    {
    }
};

SStringMemoryBarrierFlagMap g_StringMemoryFlagMap[] = {
    SStringMemoryBarrierFlagMap("vertex_attribute",
                                qt3ds::render::NVRenderBufferBarrierValues::VertexAttribArray),
    SStringMemoryBarrierFlagMap("element_array",
                                qt3ds::render::NVRenderBufferBarrierValues::ElementArray),
    SStringMemoryBarrierFlagMap("uniform_buffer",
                                qt3ds::render::NVRenderBufferBarrierValues::UniformBuffer),
    SStringMemoryBarrierFlagMap("texture_fetch",
                                qt3ds::render::NVRenderBufferBarrierValues::TextureFetch),
    SStringMemoryBarrierFlagMap("shader_image_access",
                                qt3ds::render::NVRenderBufferBarrierValues::ShaderImageAccess),
    SStringMemoryBarrierFlagMap("command_buffer",
                                qt3ds::render::NVRenderBufferBarrierValues::CommandBuffer),
    SStringMemoryBarrierFlagMap("pixel_buffer",
                                qt3ds::render::NVRenderBufferBarrierValues::PixelBuffer),
    SStringMemoryBarrierFlagMap("texture_update",
                                qt3ds::render::NVRenderBufferBarrierValues::TextureUpdate),
    SStringMemoryBarrierFlagMap("buffer_update",
                                qt3ds::render::NVRenderBufferBarrierValues::BufferUpdate),
    SStringMemoryBarrierFlagMap("frame_buffer",
                                qt3ds::render::NVRenderBufferBarrierValues::Framebuffer),
    SStringMemoryBarrierFlagMap("transform_feedback",
                                qt3ds::render::NVRenderBufferBarrierValues::TransformFeedback),
    SStringMemoryBarrierFlagMap("atomic_counter",
                                qt3ds::render::NVRenderBufferBarrierValues::AtomicCounter),
    SStringMemoryBarrierFlagMap("shader_storage",
                                qt3ds::render::NVRenderBufferBarrierValues::ShaderStorage),
};

QT3DSU32 g_StringMemoryFlagMapSize = sizeof(g_StringMemoryFlagMap) / sizeof(*g_StringMemoryFlagMap);

struct SStringBlendFuncMap
{
    const char8_t *m_Name;
    qt3ds::render::NVRenderSrcBlendFunc::Enum m_Value;
    SStringBlendFuncMap(const char8_t *nm, qt3ds::render::NVRenderSrcBlendFunc::Enum val)
        : m_Name(nm)
        , m_Value(val)
    {
    }
};

SStringBlendFuncMap g_BlendFuncMap[] = {
#define QT3DS_RENDER_HANDLE_BLEND_FUNC(nm)                                                            \
    SStringBlendFuncMap(#nm, qt3ds::render::NVRenderSrcBlendFunc::nm),
#define QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC(nm)                                                        \
    SStringBlendFuncMap(#nm, qt3ds::render::NVRenderSrcBlendFunc::nm),
    QT3DS_RENDER_ITERATE_BLEND_FUNC
#undef QT3DS_RENDER_HANDLE_BLEND_FUNC
#undef QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC
};

QT3DSU32 g_BlendFuncMapSize = sizeof(g_BlendFuncMap) / sizeof(*g_BlendFuncMap);

struct SMaterialSystem : public ICustomMaterialSystem
{
    typedef nvhash_map<SShaderMapKey, NVScopedRefCounted<SCustomMaterialShader>> TShaderMap;
    typedef nvhash_map<SCustomMaterial *, NVScopedRefCounted<SMaterialLuaContext>>
        TMaterialLuaContextMap;
    typedef eastl::pair<CRegisteredString, SImage *> TAllocatedImageEntry;

    IUICRenderContextCore &m_CoreContext;
    IUICRenderContext *m_UICContext;
    mutable qt3ds::render::SPreAllocatedAllocator m_Allocator;
    TStringMaterialMap m_StringMaterialMap;
    TShaderMap m_ShaderMap;
    TMaterialLuaContextMap m_MaterialLuaContextMap;
    lua_State *m_LuaState;
    nvvector<TCustomMaterialTextureEntry> m_TextureEntries;
    nvvector<SCustomMaterialBuffer> m_AllocatedBuffers;
    nvvector<TAllocatedImageEntry> m_AllocatedImages;
    bool m_UseFastBlits;
    eastl::string m_ShaderNameBuilder;
    QT3DSU64 m_LastFrameTime;
    QT3DSF32 m_MillisecondsSinceLastFrame;
    QT3DSI32 mRefCount;

    SMaterialSystem(IUICRenderContextCore &ct)
        : m_CoreContext(ct)
        , m_UICContext(NULL)
        , m_Allocator(ct.GetAllocator())
        , m_StringMaterialMap(ct.GetAllocator(), "SMaterialSystem::m_StringMaterialMap")
        , m_ShaderMap(ct.GetAllocator(), "SMaterialSystem::m_ShaderMap")
        , m_MaterialLuaContextMap(ct.GetAllocator(), "SMaterialSystem::m_MaterialLuaContextMap")
        , m_LuaState(NULL)
        , m_TextureEntries(ct.GetAllocator(), "SMaterialSystem::m_TextureEntries")
        , m_AllocatedBuffers(ct.GetAllocator(), "SMaterialSystem::m_AllocatedBuffers")
        , m_AllocatedImages(ct.GetAllocator(), "SMaterialSystem::m_AllocatedImages")
        , m_UseFastBlits(true)
        , m_LastFrameTime(0)
        , m_MillisecondsSinceLastFrame(0)
        , mRefCount(0)
    {
    }

    ~SMaterialSystem()
    {
        while (m_AllocatedBuffers.size())
            m_AllocatedBuffers.replace_with_last(0);

        for (QT3DSU32 idx = 0; idx < m_AllocatedImages.size(); ++idx) {
            SImage *pImage = m_AllocatedImages[idx].second;
            QT3DS_FREE(m_CoreContext.GetAllocator(), pImage);
        }
        m_AllocatedImages.clear();

        if (m_LuaState)
            lua_close(m_LuaState);
        m_LuaState = NULL;
    }

    void ReleaseBuffer(QT3DSU32 inIdx)
    {
        // Don't call this on MaterialSystem destroy.
        // This causes issues for scene liftime buffers
        // because the resource manager is destroyed before
        IResourceManager &theManager(m_UICContext->GetResourceManager());
        SCustomMaterialBuffer &theEntry(m_AllocatedBuffers[inIdx]);
        theEntry.m_FrameBuffer->Attach(NVRenderFrameBufferAttachments::Color0,
                                       NVRenderTextureOrRenderBuffer());

        theManager.Release(*theEntry.m_FrameBuffer);
        theManager.Release(*theEntry.m_Texture);
        m_AllocatedBuffers.replace_with_last(inIdx);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_CoreContext.GetAllocator())

    bool IsMaterialRegistered(CRegisteredString inStr) override
    {
        return m_StringMaterialMap.find(inStr) != m_StringMaterialMap.end();
    }

    bool RegisterMaterialClass(CRegisteredString inName,
                                       NVConstDataRef<dynamic::SPropertyDeclaration> inProperties) override
    {
        if (IsMaterialRegistered(inName))
            return false;
        m_CoreContext.GetDynamicObjectSystemCore().Register(
            inName, inProperties, sizeof(SCustomMaterial), GraphObjectTypes::CustomMaterial);
        IDynamicObjectClass *theClass =
            m_CoreContext.GetDynamicObjectSystemCore().GetDynamicObjectClass(inName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }
        SMaterialClass *theNewClass = QT3DS_NEW(m_Allocator, SMaterialClass)(m_Allocator, *theClass);
        m_StringMaterialMap.insert(eastl::make_pair(inName, theNewClass));
        return true;
    }

    SMaterialClass *GetMaterialClass(CRegisteredString inStr)
    {
        TStringMaterialMap::iterator theIter = m_StringMaterialMap.find(inStr);
        if (theIter != m_StringMaterialMap.end())
            return theIter->second;
        return NULL;
    }

    const SMaterialClass *GetMaterialClass(CRegisteredString inStr) const
    {
        return const_cast<SMaterialSystem *>(this)->GetMaterialClass(inStr);
    }

    virtual NVConstDataRef<SPropertyDefinition>
    GetCustomMaterialProperties(CRegisteredString inCustomMaterialName) const override
    {
        IDynamicObjectClass *theMaterialClass =
            m_CoreContext.GetDynamicObjectSystemCore().GetDynamicObjectClass(inCustomMaterialName);

        if (theMaterialClass)
            return theMaterialClass->GetProperties();

        return NVConstDataRef<SPropertyDefinition>();
    }

    virtual QT3DSU32 FindBuffer(CRegisteredString inName)
    {
        for (QT3DSU32 idx = 0, end = m_AllocatedBuffers.size(); idx < end; ++idx)
            if (m_AllocatedBuffers[idx].m_Name == inName)
                return idx;
        return m_AllocatedBuffers.size();
    }

    virtual QT3DSU32 FindAllocatedImage(CRegisteredString inName)
    {
        for (QT3DSU32 idx = 0, end = m_AllocatedImages.size(); idx < end; ++idx)
            if (m_AllocatedImages[idx].first == inName)
                return idx;
        return QT3DSU32(-1);
    }

    virtual bool TextureNeedsMips(const SPropertyDefinition *inPropDec,
                                  qt3ds::render::NVRenderTexture2D *inTexture)
    {
        if (inPropDec && inTexture) {
            return bool((inPropDec->m_MinFilterOp == NVRenderTextureMinifyingOp::LinearMipmapLinear)
                        && (inTexture->GetNumMipmaps() == 0));
        }

        return false;
    }

    virtual void SetTexture(NVRenderShaderProgram &inShader, CRegisteredString inPropName,
                            NVRenderTexture2D *inTexture,
                            const SPropertyDefinition *inPropDec = NULL, bool needMips = false)
    {
        SCustomMaterialTextureData *theTextureEntry(NULL);
        for (QT3DSU32 idx = 0, end = m_TextureEntries.size(); idx < end && theTextureEntry == NULL;
             ++idx) {
            if (m_TextureEntries[idx].first == inPropName
                && m_TextureEntries[idx].second->m_Shader.mPtr == &inShader
                && m_TextureEntries[idx].second->m_Texture == inTexture) {
                theTextureEntry = m_TextureEntries[idx].second;
                break;
            }
        }
        if (theTextureEntry == NULL) {
            NVScopedRefCounted<SCustomMaterialTextureData> theNewEntry =
                QT3DS_NEW(m_CoreContext.GetAllocator(), SCustomMaterialTextureData)(
                    SCustomMaterialTextureData::CreateTextureEntry(inShader, inTexture, inPropName,
                                                                   needMips));
            m_TextureEntries.push_back(eastl::make_pair(inPropName, theNewEntry));
            theTextureEntry = theNewEntry.mPtr;
        }
        theTextureEntry->Set(inPropDec);
    }

    void SetPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName,
                                      NVConstDataRef<CRegisteredString> inNames) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetPropertyEnumNames(inName, inPropName,
                                                                        inNames);
    }

    void SetPropertyTextureSettings(CRegisteredString inName, CRegisteredString inPropName,
                                            CRegisteredString inPropPath,
                                            NVRenderTextureTypeValue::Enum inTexType,
                                            NVRenderTextureCoordOp::Enum inCoordOp,
                                            NVRenderTextureMagnifyingOp::Enum inMagFilterOp,
                                            NVRenderTextureMinifyingOp::Enum inMinFilterOp) override
    {
        SMaterialClass *theClass = GetMaterialClass(inName);
        if (theClass && inTexType == NVRenderTextureTypeValue::Displace) {
            theClass->m_HasDisplacement = true;
        }
        m_CoreContext.GetDynamicObjectSystemCore().SetPropertyTextureSettings(
            inName, inPropName, inPropPath, inTexType, inCoordOp, inMagFilterOp, inMinFilterOp);
    }

    void SetMaterialClassShader(CRegisteredString inName, const char8_t *inShaderType,
                                        const char8_t *inShaderVersion, const char8_t *inShaderData,
                                        bool inHasGeomShader, bool inIsComputeShader) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetShaderData(inName, inShaderData, inShaderType,
                                                                 inShaderVersion, inHasGeomShader,
                                                                 inIsComputeShader);
    }

    SCustomMaterial *CreateCustomMaterial(CRegisteredString inName,
                                                  NVAllocatorCallback &inSceneGraphAllocator) override
    {
        SCustomMaterial *theMaterial = static_cast<SCustomMaterial *>(
            m_CoreContext.GetDynamicObjectSystemCore().CreateInstance(inName,
                                                                      inSceneGraphAllocator));
        SMaterialClass *theClass = GetMaterialClass(inName);

        if (theMaterial) {
            QT3DSU32 key = 0, count = 0;
            if (theClass) {
                key = theClass->m_ShaderKey;
                count = theClass->m_LayerCount;
            }
            theMaterial->Initialize(key, count);
        }

        return theMaterial;
    }

    void SetCustomMaterialTransparency(CRegisteredString inName, bool inHasTransparency) override
    {
        SMaterialClass *theClass = GetMaterialClass(inName);

        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        theClass->m_HasTransparency = inHasTransparency;
    }

    void SetCustomMaterialRefraction(CRegisteredString inName, bool inHasRefraction) override
    {
        SMaterialClass *theClass = GetMaterialClass(inName);

        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        theClass->m_HasRefraction = inHasRefraction;
    }

    void SetCustomMaterialAlwaysDirty(CRegisteredString inName, bool inIsAlwaysDirty) override
    {
        SMaterialClass *theClass = GetMaterialClass(inName);

        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        theClass->m_AlwaysDirty = inIsAlwaysDirty;
    }

    void SetCustomMaterialShaderKey(CRegisteredString inName, QT3DSU32 inShaderKey) override
    {
        SMaterialClass *theClass = GetMaterialClass(inName);

        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        theClass->m_ShaderKey = inShaderKey;
    }

    void SetCustomMaterialLayerCount(CRegisteredString inName, QT3DSU32 inLayerCount) override
    {
        SMaterialClass *theClass = GetMaterialClass(inName);

        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        theClass->m_LayerCount = inLayerCount;
    }

    void SetCustomMaterialCommands(CRegisteredString inName,
                                   NVConstDataRef<dynamic::SCommand *> inCommands) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetRenderCommands(inName, inCommands);
    }

    CRegisteredString GetShaderCacheKey(CRenderString &inShaderKeyBuffer, const char8_t *inId,
                                        const char8_t *inProgramMacro,
                                        const dynamic::SDynamicShaderProgramFlags &inFlags)
    {
        inShaderKeyBuffer.assign(inId);
        if (inProgramMacro && *inProgramMacro) {
            inShaderKeyBuffer.append("#");
            inShaderKeyBuffer.append(inProgramMacro);
        }
        if (inFlags.IsTessellationEnabled()) {
            inShaderKeyBuffer.append("#");
            inShaderKeyBuffer.append(TessModeValues::toString(inFlags.m_TessMode));
        }
        if (inFlags.IsGeometryShaderEnabled() && inFlags.m_WireframeMode) {
            inShaderKeyBuffer.append("#");
            inShaderKeyBuffer.append(inFlags.wireframeToString(inFlags.m_WireframeMode));
        }

        return m_CoreContext.GetStringTable().RegisterStr(inShaderKeyBuffer.c_str());
    }

    NVRenderShaderProgram *GetShader(SCustomMaterialRenderContext &inRenderContext,
                                     const SCustomMaterial &inMaterial,
                                     const SBindShader &inCommand, TShaderFeatureSet inFeatureSet,
                                     const dynamic::SDynamicShaderProgramFlags &inFlags)
    {
        ICustomMaterialShaderGenerator &theMaterialGenerator(
            m_UICContext->GetCustomMaterialShaderGenerator());

        // generate key
        CRenderString theShaderKeyBuffer;
        CRegisteredString theKey = GetShaderCacheKey(theShaderKeyBuffer, inCommand.m_ShaderPath,
                                                     inCommand.m_ShaderDefine, inFlags);

        SCustomMaterialVertexPipeline thePipeline(m_UICContext,
                                                  inRenderContext.m_Model.m_TessellationMode);

        NVRenderShaderProgram *theProgram = theMaterialGenerator.GenerateShader(
            inMaterial, inRenderContext.m_MaterialKey, thePipeline, inFeatureSet,
            inRenderContext.m_Lights, inRenderContext.m_FirstImage,
            (inMaterial.m_hasTransparency || inMaterial.m_hasRefraction),
            "custom material pipeline-- ", inCommand.m_ShaderPath.c_str());

        return theProgram;
    }

    SMaterialOrComputeShader BindShader(SCustomMaterialRenderContext &inRenderContext,
                                        const SCustomMaterial &inMaterial,
                                        const SBindShader &inCommand,
                                        TShaderFeatureSet inFeatureSet)
    {
        NVRenderShaderProgram *theProgram = NULL;

        SDynamicShaderProgramFlags theFlags(inRenderContext.m_Model.m_TessellationMode,
                                            inRenderContext.m_Subset.m_WireframeMode);
        theFlags.SetTessellationEnabled(inRenderContext.m_Model.m_TessellationMode
                                        != TessModeValues::NoTess);
        theFlags.SetGeometryShaderEnabled(inRenderContext.m_Subset.m_WireframeMode);

        SShaderMapKey skey = SShaderMapKey(
            TStrStrPair(inCommand.m_ShaderPath, inCommand.m_ShaderDefine), inFeatureSet,
            theFlags.m_TessMode, theFlags.m_WireframeMode, inRenderContext.m_MaterialKey);
        eastl::pair<TShaderMap::iterator, bool> theInsertResult(m_ShaderMap.insert(
            eastl::make_pair(skey, NVScopedRefCounted<SCustomMaterialShader>(NULL))));

        if (theInsertResult.second) {
            theProgram = GetShader(inRenderContext, inMaterial, inCommand, inFeatureSet, theFlags);

            if (theProgram) {
                theInsertResult.first->second =
                    QT3DS_NEW(m_Allocator, SCustomMaterialShader)(*theProgram, theFlags);
            }
        } else if (theInsertResult.first->second)
            theProgram = theInsertResult.first->second->m_Shader;

        if (theProgram) {
            if (theProgram->GetProgramType() == NVRenderShaderProgram::ProgramType::Graphics) {
                if (theInsertResult.first->second) {
                    NVRenderContext &theContext(m_UICContext->GetRenderContext());
                    theContext.SetActiveShader(theInsertResult.first->second->m_Shader);
                }

                return *theInsertResult.first->second;
            } else {
                NVRenderContext &theContext(m_UICContext->GetRenderContext());
                theContext.SetActiveShader(theProgram);
                return *(static_cast<NVRenderShaderProgram *>(theProgram));
            }
        }
        return SMaterialOrComputeShader();
    }

    SMaterialOrComputeShader BindShaderFromLua(SCustomMaterialRenderContext &inRenderContext,
                                               const SBindShader &inCommand,
                                               TShaderFeatureSet inFeatureSet)
    {
        SDynamicShaderProgramFlags theFlags(inRenderContext.m_Model.m_TessellationMode,
                                            inRenderContext.m_Subset.m_WireframeMode);
        theFlags.SetTessellationEnabled(inRenderContext.m_Model.m_TessellationMode
                                        != TessModeValues::NoTess);
        theFlags.SetGeometryShaderEnabled(inRenderContext.m_Subset.m_WireframeMode);

        TShaderAndFlags theProgramAndFlags =
            m_UICContext->GetDynamicObjectSystem().GetShaderProgram(
                inCommand.m_ShaderPath, inCommand.m_ShaderDefine, inFeatureSet, theFlags);

        NVRenderShaderProgram *theProgram = theProgramAndFlags.first;
        if (theProgram) {
            if (theProgram->GetProgramType() == NVRenderShaderProgram::ProgramType::Graphics) {
                SShaderDefaultMaterialKey theMaterialKey = 0;
                SShaderMapKey skey = SShaderMapKey(
                    TStrStrPair(inCommand.m_ShaderPath, inCommand.m_ShaderDefine), inFeatureSet,
                    theFlags.m_TessMode, theFlags.m_WireframeMode, theMaterialKey);
                eastl::pair<TShaderMap::iterator, bool> theInsertResult(m_ShaderMap.insert(
                    eastl::make_pair(skey, NVScopedRefCounted<SCustomMaterialShader>(NULL))));

                if (theInsertResult.second) {
                    if (theProgram)
                        theInsertResult.first->second = QT3DS_NEW(m_Allocator, SCustomMaterialShader)(
                            *theProgram, theProgramAndFlags.second);
                }

                if (theInsertResult.first->second) {
                    NVRenderContext &theContext(m_UICContext->GetRenderContext());
                    theContext.SetActiveShader(theInsertResult.first->second->m_Shader);
                }

                return *theInsertResult.first->second;
            } else {
                NVRenderContext &theContext(m_UICContext->GetRenderContext());
                theContext.SetActiveShader(theProgram);
                return *(static_cast<NVRenderShaderProgram *>(theProgram));
            }
        }
        return SMaterialOrComputeShader();
    }

    void DoApplyInstanceValue(SCustomMaterial & /* inMaterial */, QT3DSU8 *inDataPtr,
                              CRegisteredString inPropertyName,
                              NVRenderShaderDataTypes::Enum inPropertyType,
                              NVRenderShaderProgram &inShader,
                              const SPropertyDefinition &inDefinition)
    {
        qt3ds::render::NVRenderShaderConstantBase *theConstant =
            inShader.GetShaderConstant(inPropertyName);
        using namespace qt3ds::render;
        if (theConstant) {
            if (theConstant->GetShaderConstantType() == inPropertyType) {
                if (inPropertyType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                    StaticAssert<sizeof(CRegisteredString)
                                 == sizeof(NVRenderTexture2DPtr)>::valid_expression();
                    CRegisteredString *theStrPtr = reinterpret_cast<CRegisteredString *>(inDataPtr);
                    IBufferManager &theBufferManager(m_UICContext->GetBufferManager());
                    NVRenderTexture2D *theTexture = NULL;

                    if (theStrPtr->IsValid()) {
                        SImageTextureData theTextureData =
                            theBufferManager.LoadRenderImage(*theStrPtr);
                        if (theTextureData.m_Texture) {
                            theTexture = theTextureData.m_Texture;
                            SetTexture(inShader, inPropertyName, theTexture, &inDefinition,
                                       TextureNeedsMips(&inDefinition, theTexture));
                        }
                    }
                } else {
                    switch (inPropertyType) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(type)                                                           \
    case NVRenderShaderDataTypes::type:                                                            \
        inShader.SetPropertyValue(theConstant, *(reinterpret_cast<type *>(inDataPtr)));            \
        break;
                        ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
                    default:
                        QT3DS_ASSERT(false);
                        break;
                    }
                }
            } else {
                qCCritical(INVALID_OPERATION,
                    "CustomMaterial ApplyInstanceValue command datatype and "
                    "shader datatypes differ for property %s",
                    inPropertyName.c_str());
                QT3DS_ASSERT(false);
            }
        }
    }

    void ApplyInstanceValue(SCustomMaterial &inMaterial, SMaterialClass &inClass,
                            NVRenderShaderProgram &inShader, const SApplyInstanceValue &inCommand)
    {
        // sanity check
        if (inCommand.m_PropertyName.IsValid()) {
            bool canGetData =
                inCommand.m_ValueOffset + getSizeofShaderDataType(inCommand.m_ValueType)
                <= inMaterial.m_DataSectionByteSize;
            if (canGetData == false) {
                QT3DS_ASSERT(false);
                return;
            }
            QT3DSU8 *dataPtr = inMaterial.GetDataSectionBegin() + inCommand.m_ValueOffset;
            const SPropertyDefinition *theDefinition =
                inClass.m_Class->FindPropertyByName(inCommand.m_PropertyName);
            if (theDefinition)
                DoApplyInstanceValue(inMaterial, dataPtr, inCommand.m_PropertyName,
                                     inCommand.m_ValueType, inShader, *theDefinition);
        } else {
            NVConstDataRef<SPropertyDefinition> theDefs = inClass.m_Class->GetProperties();
            for (QT3DSU32 idx = 0, end = theDefs.size(); idx < end; ++idx) {
                const SPropertyDefinition &theDefinition(theDefs[idx]);
                qt3ds::render::NVRenderShaderConstantBase *theConstant =
                    inShader.GetShaderConstant(theDefinition.m_Name);

                // This is fine, the property wasn't found and we continue, no problem.
                if (!theConstant)
                    continue;
                QT3DSU8 *dataPtr = inMaterial.GetDataSectionBegin() + theDefinition.m_Offset;
                DoApplyInstanceValue(inMaterial, dataPtr, theDefinition.m_Name,
                                     theDefinition.m_DataType, inShader, theDefinition);
            }
        }
    }

    void ApplyBlending(const SApplyBlending &inCommand)
    {
        NVRenderContext &theContext(m_UICContext->GetRenderContext());

        theContext.SetBlendingEnabled(true);

        qt3ds::render::NVRenderBlendFunctionArgument blendFunc =
            qt3ds::render::NVRenderBlendFunctionArgument(
                inCommand.m_SrcBlendFunc, inCommand.m_DstBlendFunc, inCommand.m_SrcBlendFunc,
                inCommand.m_DstBlendFunc);

        qt3ds::render::NVRenderBlendEquationArgument blendEqu(NVRenderBlendEquation::Add,
                                                           NVRenderBlendEquation::Add);

        theContext.SetBlendFunction(blendFunc);
        theContext.SetBlendEquation(blendEqu);
    }

    // we currently only bind a source texture
    const NVRenderTexture2D *ApplyBufferValue(const SCustomMaterial &inMaterial,
                                              NVRenderShaderProgram &inShader,
                                              const SApplyBufferValue &inCommand,
                                              const NVRenderTexture2D *inSourceTexture)
    {
        const NVRenderTexture2D *theTexture = NULL;

        if (inCommand.m_BufferName.IsValid()) {
            QT3DSU32 bufferIdx = FindBuffer(inCommand.m_BufferName);
            if (bufferIdx < m_AllocatedBuffers.size()) {
                SCustomMaterialBuffer &theEntry(m_AllocatedBuffers[bufferIdx]);
                theTexture = theEntry.m_Texture;
            } else {
                // we must have allocated the read target before
                qCCritical(INTERNAL_ERROR,
                    "CustomMaterial: ApplyBufferValue: Failed to setup read target");
                QT3DS_ASSERT(false);
            }
        } else
            theTexture = inSourceTexture;

        if (inCommand.m_ParamName.IsValid()) {
            qt3ds::render::NVRenderShaderConstantBase *theConstant =
                inShader.GetShaderConstant(inCommand.m_ParamName);

            if (theConstant) {
                if (theConstant->GetShaderConstantType()
                    != NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                    qCCritical(INVALID_OPERATION,
                        "CustomMaterial %s: Binding buffer to parameter %s that is not a texture",
                        inMaterial.m_ClassName.c_str(), inCommand.m_ParamName.c_str());
                    QT3DS_ASSERT(false);
                } else {
                    SetTexture(inShader, inCommand.m_ParamName,
                               const_cast<NVRenderTexture2D *>(theTexture));
                }
            }
        }

        return theTexture;
    }

    void AllocateBuffer(const SAllocateBuffer &inCommand, NVRenderFrameBuffer *inTarget)
    {
        STextureDetails theSourceTextureDetails;
        NVRenderTexture2D *theTexture = NULL;
        // get color attachment we always assume at location 0
        if (inTarget) {
            NVRenderTextureOrRenderBuffer theSourceTexture =
                inTarget->GetAttachment(NVRenderFrameBufferAttachments::Color0);
            // we need a texture
            if (theSourceTexture.HasTexture2D()) {
                theSourceTextureDetails = theSourceTexture.GetTexture2D()->GetTextureDetails();
            } else {
                qCCritical(INVALID_OPERATION, "CustomMaterial %s: Invalid source texture",
                    inCommand.m_Name.c_str());
                QT3DS_ASSERT(false);
                return;
            }
        } else {
            NVRenderContext &theContext = m_UICContext->GetRenderContext();
            // if we allocate a buffer based on the default target use viewport to get the dimension
            NVRenderRect theViewport(theContext.GetViewport());
            theSourceTextureDetails.m_Height = theViewport.m_Height;
            theSourceTextureDetails.m_Width = theViewport.m_Width;
        }

        QT3DSU32 theWidth = (QT3DSU32)(theSourceTextureDetails.m_Width * inCommand.m_SizeMultiplier);
        QT3DSU32 theHeight = (QT3DSU32)(theSourceTextureDetails.m_Height * inCommand.m_SizeMultiplier);
        NVRenderTextureFormats::Enum theFormat = inCommand.m_Format;
        if (theFormat == NVRenderTextureFormats::Unknown)
            theFormat = theSourceTextureDetails.m_Format;
        IResourceManager &theResourceManager(m_UICContext->GetResourceManager());
        // size intentionally requiried every loop;
        QT3DSU32 bufferIdx = FindBuffer(inCommand.m_Name);
        if (bufferIdx < m_AllocatedBuffers.size()) {
            SCustomMaterialBuffer &theEntry(m_AllocatedBuffers[bufferIdx]);
            STextureDetails theDetails = theEntry.m_Texture->GetTextureDetails();
            if (theDetails.m_Width == theWidth && theDetails.m_Height == theHeight
                && theDetails.m_Format == theFormat) {
                theTexture = theEntry.m_Texture;
            } else {
                ReleaseBuffer(bufferIdx);
            }
        }

        if (theTexture == NULL) {
            NVRenderFrameBuffer *theFB(theResourceManager.AllocateFrameBuffer());
            NVRenderTexture2D *theTexture(
                theResourceManager.AllocateTexture2D(theWidth, theHeight, theFormat));
            theTexture->SetMagFilter(inCommand.m_FilterOp);
            theTexture->SetMinFilter(
                static_cast<NVRenderTextureMinifyingOp::Enum>(inCommand.m_FilterOp));
            theTexture->SetTextureWrapS(inCommand.m_TexCoordOp);
            theTexture->SetTextureWrapT(inCommand.m_TexCoordOp);
            theFB->Attach(NVRenderFrameBufferAttachments::Color0, *theTexture);
            m_AllocatedBuffers.push_back(SCustomMaterialBuffer(
                inCommand.m_Name, *theFB, *theTexture, inCommand.m_BufferFlags));
        }
    }

    NVRenderFrameBuffer *BindBuffer(const SCustomMaterial &inMaterial, const SBindBuffer &inCommand,
                                    bool &outClearTarget, QT3DSVec2 &outDestSize)
    {
        NVRenderFrameBuffer *theBuffer = NULL;
        NVRenderTexture2D *theTexture = NULL;

        // search for the buffer
        QT3DSU32 bufferIdx = FindBuffer(inCommand.m_BufferName);
        if (bufferIdx < m_AllocatedBuffers.size()) {
            theBuffer = m_AllocatedBuffers[bufferIdx].m_FrameBuffer;
            theTexture = m_AllocatedBuffers[bufferIdx].m_Texture;
        }

        if (theBuffer == NULL) {
            qCCritical(INVALID_OPERATION, "Effect %s: Failed to find buffer %s for bind",
                inMaterial.m_ClassName.c_str(), inCommand.m_BufferName.c_str());
            QT3DS_ASSERT(false);
            return NULL;
        }

        if (theTexture) {
            STextureDetails theDetails(theTexture->GetTextureDetails());
            m_UICContext->GetRenderContext().SetViewport(
                NVRenderRect(0, 0, (QT3DSU32)theDetails.m_Width, (QT3DSU32)theDetails.m_Height));
            outDestSize = QT3DSVec2((QT3DSF32)theDetails.m_Width, (QT3DSF32)theDetails.m_Height);
            outClearTarget = inCommand.m_NeedsClear;
        }

        return theBuffer;
    }

    void computeScreenCoverage(SCustomMaterialRenderContext &inRenderContext, QT3DSI32 *xMin,
                               QT3DSI32 *yMin, QT3DSI32 *xMax, QT3DSI32 *yMax)
    {
        NVRenderContext &theContext(m_UICContext->GetRenderContext());
        TNVBounds2BoxPoints outPoints;
        QT3DSVec4 projMin(QT3DS_MAX_REAL);
        QT3DSVec4 projMax(-QT3DS_MAX_REAL);

        // get points
        inRenderContext.m_Subset.m_Bounds.expand(outPoints);
        for (QT3DSU32 idx = 0; idx < 8; ++idx) {
            QT3DSVec4 homPoint(outPoints[idx], 1.0);
            QT3DSVec4 projPoint = inRenderContext.m_ModelViewProjection.transform(homPoint);
            projPoint /= projPoint.w;

            if (projMin.x > projPoint.x)
                projMin.x = projPoint.x;
            if (projMin.y > projPoint.y)
                projMin.y = projPoint.y;
            if (projMin.z > projPoint.z)
                projMin.z = projPoint.z;

            if (projMax.x < projPoint.x)
                projMax.x = projPoint.x;
            if (projMax.y < projPoint.y)
                projMax.y = projPoint.y;
            if (projMax.z < projPoint.z)
                projMax.z = projPoint.z;
        }

        NVRenderRect theViewport(theContext.GetViewport());
        QT3DSI32 x1 = QT3DSI32(projMax.x * (theViewport.m_Width / 2)
                         + (theViewport.m_X + (theViewport.m_Width / 2)));
        QT3DSI32 y1 = QT3DSI32(projMax.y * (theViewport.m_Height / 2)
                         + (theViewport.m_Y + (theViewport.m_Height / 2)));

        QT3DSI32 x2 = QT3DSI32(projMin.x * (theViewport.m_Width / 2)
                         + (theViewport.m_X + (theViewport.m_Width / 2)));
        QT3DSI32 y2 = QT3DSI32(projMin.y * (theViewport.m_Height / 2)
                         + (theViewport.m_Y + (theViewport.m_Height / 2)));

        if (x1 > x2) {
            *xMin = x2;
            *xMax = x1;
        } else {
            *xMin = x1;
            *xMax = x2;
        }
        if (y1 > y2) {
            *yMin = y2;
            *yMax = y1;
        } else {
            *yMin = y1;
            *yMax = y2;
        }
    }

    void BlitFramebuffer(SCustomMaterialRenderContext &inRenderContext,
                         const SApplyBlitFramebuffer &inCommand, NVRenderFrameBuffer *inTarget)
    {
        NVRenderContext &theContext(m_UICContext->GetRenderContext());
        // we change the read/render targets here
        NVRenderContextScopedProperty<NVRenderFrameBuffer *> __framebuffer(
            theContext, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
        // we may alter scissor
        NVRenderContextScopedProperty<bool> theScissorEnabled(
            theContext, &NVRenderContext::IsScissorTestEnabled,
            &NVRenderContext::SetScissorTestEnabled);

        if (inCommand.m_DestBufferName.IsValid()) {
            QT3DSU32 bufferIdx = FindBuffer(inCommand.m_DestBufferName);
            if (bufferIdx < m_AllocatedBuffers.size()) {
                SCustomMaterialBuffer &theEntry(m_AllocatedBuffers[bufferIdx]);
                theContext.SetRenderTarget(theEntry.m_FrameBuffer);
            } else {
                // we must have allocated the read target before
                qCCritical(INTERNAL_ERROR,
                    "CustomMaterial: BlitFramebuffer: Failed to setup render target");
                QT3DS_ASSERT(false);
            }
        } else {
            // our dest is the default render target
            theContext.SetRenderTarget(inTarget);
        }

        if (inCommand.m_SourceBufferName.IsValid()) {
            QT3DSU32 bufferIdx = FindBuffer(inCommand.m_SourceBufferName);
            if (bufferIdx < m_AllocatedBuffers.size()) {
                SCustomMaterialBuffer &theEntry(m_AllocatedBuffers[bufferIdx]);
                theContext.SetReadTarget(theEntry.m_FrameBuffer);
                theContext.SetReadBuffer(NVReadFaces::Color0);
            } else {
                // we must have allocated the read target before
                qCCritical(INTERNAL_ERROR,
                    "CustomMaterial: BlitFramebuffer: Failed to setup read target");
                QT3DS_ASSERT(false);
            }
        } else {
            // our source is the default read target
            // depending on what we render we assume color0 or back
            theContext.SetReadTarget(inTarget);
            NVReadFaces::Enum value = (inTarget) ? NVReadFaces::Color0 : NVReadFaces::Back;
            theContext.SetReadBuffer(value);
        }

        NVRenderRect theViewport(theContext.GetViewport());
        theContext.SetScissorTestEnabled(false);

        if (!m_UseFastBlits) {
            // only copy sreen amount of pixels
            QT3DSI32 xMin, yMin, xMax, yMax;
            computeScreenCoverage(inRenderContext, &xMin, &yMin, &xMax, &yMax);

            // same dimension
            theContext.BlitFramebuffer(xMin, yMin, xMax, yMax, xMin, yMin, xMax, yMax,
                                       NVRenderClearValues::Color,
                                       NVRenderTextureMagnifyingOp::Nearest);
        } else {
            // same dimension
            theContext.BlitFramebuffer(
                theViewport.m_X, theViewport.m_Y, theViewport.m_X + theViewport.m_Width,
                theViewport.m_Y + theViewport.m_Height, theViewport.m_X, theViewport.m_Y,
                theViewport.m_X + theViewport.m_Width, theViewport.m_Y + theViewport.m_Height,
                NVRenderClearValues::Color, NVRenderTextureMagnifyingOp::Nearest);
        }
    }

    SLayerGlobalRenderProperties
    GetLayerGlobalRenderProperties(SCustomMaterialRenderContext &inRenderContext)
    {
        const SLayer &theLayer = inRenderContext.m_Layer;
        const SLayerRenderData &theData = inRenderContext.m_LayerData;

        return SLayerGlobalRenderProperties(
            theLayer, const_cast<SCamera &>(inRenderContext.m_Camera), theData.m_CameraDirection,
            inRenderContext.m_Lights, NVDataRef<QT3DSVec3>(), theData.m_ShadowMapManager.mPtr,
            const_cast<NVRenderTexture2D *>(inRenderContext.m_DepthTexture),
            const_cast<NVRenderTexture2D *>(inRenderContext.m_AOTexture), theLayer.m_LightProbe,
            theLayer.m_LightProbe2, theLayer.m_ProbeHorizon, theLayer.m_ProbeBright,
            theLayer.m_Probe2Window, theLayer.m_Probe2Pos, theLayer.m_Probe2Fade,
            theLayer.m_ProbeFov);
    }

    void RenderPass(SCustomMaterialRenderContext &inRenderContext, SCustomMaterialShader &inShader,
                    NVRenderTexture2D * /* inSourceTexture */
                    ,
                    NVRenderFrameBuffer *inFrameBuffer, bool inRenderTargetNeedsClear,
                    NVRenderInputAssembler &inAssembler, QT3DSU32 inCount, QT3DSU32 inOffset)
    {
        NVRenderContext &theContext(m_UICContext->GetRenderContext());
        theContext.SetRenderTarget(inFrameBuffer);

        QT3DSVec4 clearColor(0.0);
        NVRenderContextScopedProperty<QT3DSVec4> __clearColor(
            theContext, &NVRenderContext::GetClearColor, &NVRenderContext::SetClearColor,
            clearColor);
        if (inRenderTargetNeedsClear) {
            theContext.Clear(qt3ds::render::NVRenderClearValues::Color);
        }

        ICustomMaterialShaderGenerator &theMaterialGenerator(
            m_UICContext->GetCustomMaterialShaderGenerator());

        theMaterialGenerator.SetMaterialProperties(
            *inShader.m_Shader, inRenderContext.m_Material, QT3DSVec2(1.0, 1.0),
            inRenderContext.m_ModelViewProjection, inRenderContext.m_NormalMatrix,
            inRenderContext.m_ModelMatrix, inRenderContext.m_FirstImage, 1.0,
            GetLayerGlobalRenderProperties(inRenderContext));

        // I think the prim type should always be fetched from the
        // current mesh subset setup because there you get the actual draw mode
        // for this frame
        NVRenderDrawMode::Enum theDrawMode = inAssembler.GetPrimitiveType();

        // tesselation
        if (inRenderContext.m_Subset.m_PrimitiveType == NVRenderDrawMode::Patches) {
            QT3DSVec2 camProps(inRenderContext.m_Camera.m_ClipNear,
                            inRenderContext.m_Camera.m_ClipFar);
            theDrawMode = inRenderContext.m_Subset.m_PrimitiveType;
            inShader.m_Tessellation.m_EdgeTessLevel.Set(inRenderContext.m_Subset.m_EdgeTessFactor);
            inShader.m_Tessellation.m_InsideTessLevel.Set(
                inRenderContext.m_Subset.m_InnerTessFactor);
            // the blend value is hardcoded
            inShader.m_Tessellation.m_PhongBlend.Set(0.75);
            // this should finally be based on some user input
            inShader.m_Tessellation.m_DistanceRange.Set(camProps);
            // enable culling
            inShader.m_Tessellation.m_DisableCulling.Set(0.0);
        }

        if (inRenderContext.m_Subset.m_WireframeMode) {
            NVRenderRect theViewport(theContext.GetViewport());
            QT3DSMat44 vpMatrix;
            vpMatrix.column0 = QT3DSVec4((float)theViewport.m_Width / 2.0f, 0.0, 0.0, 0.0);
            vpMatrix.column1 = QT3DSVec4(0.0, (float)theViewport.m_Height / 2.0f, 0.0, 0.0);
            vpMatrix.column2 = QT3DSVec4(0.0, 0.0, 1.0, 0.0);
            vpMatrix.column3 =
                QT3DSVec4((float)theViewport.m_Width / 2.0f + (float)theViewport.m_X,
                       (float)theViewport.m_Height / 2.0f + (float)theViewport.m_Y, 0.0, 1.0);

            inShader.m_ViewportMatrix.Set(vpMatrix);
        }

        theContext.SetInputAssembler(&inAssembler);

        theContext.SetCullingEnabled(true);
        QT3DSU32 count = inCount;
        QT3DSU32 offset = inOffset;

        theContext.Draw(theDrawMode, count, offset);
    }

    void DoRenderCustomMaterial(SCustomMaterialRenderContext &inRenderContext,
                                const SCustomMaterial &inMaterial, SMaterialClass &inClass,
                                NVRenderFrameBuffer *inTarget, TShaderFeatureSet inFeatureSet)
    {
        NVRenderContext &theContext = m_UICContext->GetRenderContext();
        SCustomMaterialShader *theCurrentShader(NULL);

        NVRenderFrameBuffer *theCurrentRenderTarget(inTarget);
        NVRenderRect theOriginalViewport(theContext.GetViewport());
        NVRenderTexture2D *theCurrentSourceTexture = 0;

        // for refrative materials we come from the transparent render path
        // but we do not want to do blending
        bool wasBlendingEnabled = theContext.IsBlendingEnabled();
        if (inMaterial.m_hasRefraction)
            theContext.SetBlendingEnabled(false);

        NVRenderContextScopedProperty<NVRenderFrameBuffer *> __framebuffer(
            theContext, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
        NVRenderContextScopedProperty<NVRenderRect> __viewport(
            theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport);

        QT3DSVec2 theDestSize;
        bool theRenderTargetNeedsClear = false;

        NVConstDataRef<dynamic::SCommand *> theCommands(inClass.m_Class->GetRenderCommands());
        for (QT3DSU32 commandIdx = 0, commandEnd = theCommands.size(); commandIdx < commandEnd;
             ++commandIdx) {
            const SCommand &theCommand(*theCommands[commandIdx]);

            switch (theCommand.m_Type) {
            case CommandTypes::AllocateBuffer:
                AllocateBuffer(static_cast<const SAllocateBuffer &>(theCommand), inTarget);
                break;
            case CommandTypes::BindBuffer:
                theCurrentRenderTarget =
                    BindBuffer(inMaterial, static_cast<const SBindBuffer &>(theCommand),
                               theRenderTargetNeedsClear, theDestSize);
                break;
            case CommandTypes::BindTarget:
                // Restore the previous render target and info.
                theCurrentRenderTarget = inTarget;
                theContext.SetViewport(theOriginalViewport);
                break;
            case CommandTypes::BindShader: {
                theCurrentShader = NULL;
                SMaterialOrComputeShader theBindResult =
                    BindShader(inRenderContext, inMaterial,
                               static_cast<const SBindShader &>(theCommand), inFeatureSet);
                if (theBindResult.IsMaterialShader())
                    theCurrentShader = &theBindResult.MaterialShader();
            } break;
            case CommandTypes::ApplyInstanceValue:
                // we apply the property update explicitly at the render pass
                break;
            case CommandTypes::Render:
                if (theCurrentShader) {
                    RenderPass(inRenderContext, *theCurrentShader, theCurrentSourceTexture,
                               theCurrentRenderTarget, theRenderTargetNeedsClear,
                               *inRenderContext.m_Subset.m_InputAssembler,
                               inRenderContext.m_Subset.m_Count, inRenderContext.m_Subset.m_Offset);
                }
                // reset
                theRenderTargetNeedsClear = false;
                break;
            case CommandTypes::ApplyBlending:
                ApplyBlending(static_cast<const SApplyBlending &>(theCommand));
                break;
            case CommandTypes::ApplyBufferValue:
                if (theCurrentShader)
                    ApplyBufferValue(inMaterial, *theCurrentShader->m_Shader,
                                     static_cast<const SApplyBufferValue &>(theCommand),
                                     theCurrentSourceTexture);
                break;
            case CommandTypes::ApplyBlitFramebuffer:
                BlitFramebuffer(inRenderContext,
                                static_cast<const SApplyBlitFramebuffer &>(theCommand), inTarget);
                break;
            case CommandTypes::LuaCommand: {
                if (m_LuaState == NULL) {
                    m_LuaState = lua_newstate(LuaAlloc, this);
                    luaL_openlibs(m_LuaState);
                    lua_pushlightuserdata(m_LuaState, this);
                    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "__materialSystem");
                    RegisterGlobalFunctions();
                    lua_newtable(m_LuaState);
                    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "__materialcontexts");
                }
                SCustomMaterial &theMaterial = const_cast<SCustomMaterial &>(inMaterial);
                eastl::pair<TMaterialLuaContextMap::iterator, bool> inserter =
                    m_MaterialLuaContextMap.insert(
                        eastl::make_pair(&theMaterial, NVScopedRefCounted<SMaterialLuaContext>()));
                if (inserter.second) {
                    QT3DSU32 contextId = (QT3DSU32)m_MaterialLuaContextMap.size();
                    NVScopedRefCounted<SMaterialLuaContext> theNewContext =
                        QT3DS_NEW(m_CoreContext.GetAllocator(),
                               SMaterialLuaContext)(theMaterial, inClass, *m_LuaState, contextId,
                                                    *this, m_CoreContext.GetAllocator());
                    // Attempt to compile the lua.
                    // first, create a new table that will be the global context
                    const char *initScript = "local P = {} \n"
                                             "setmetatable(P, {__index = _G})\n"
                                             "return P\n";
                    luaL_loadstring(m_LuaState, initScript);
                    int callResult = lua_pcall(m_LuaState, 0, 1, 0);
                    if (callResult) {
                        const char *error = lua_tostring(m_LuaState, -1);
                        qCCritical(INTERNAL_ERROR, "Error initializing lua state: %s", error);
                        return;
                    }
                    lua_pushlightuserdata(m_LuaState, theNewContext.mPtr);
                    lua_setfield(m_LuaState, -2, "__context");
                    // Store the context table in the context list.
                    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__materialcontexts");
                    lua_pushvalue(m_LuaState, -2);
                    lua_rawseti(m_LuaState, -2, contextId);
                    lua_pop(m_LuaState, 1);

                    const SLuaCommand &theLuaCommand = static_cast<const SLuaCommand &>(theCommand);
                    int loadResult = luaL_loadstring(m_LuaState, theLuaCommand.m_LuaScript.c_str());
                    if (loadResult) {
                        const char *error = lua_tostring(m_LuaState, -1);
                        qCCritical(INTERNAL_ERROR, "Error initializing lua state for material: %s: %s",
                            inMaterial.m_Id.c_str(), error);
                        return;
                    }
                    // Swap the table and the function so fenv call works.
                    lua_insert(m_LuaState, -2);
                    QT3DS_ASSERT(lua_isfunction(m_LuaState, -2));
                    QT3DS_ASSERT(lua_istable(m_LuaState, -1));
                    lua_getfield(m_LuaState, -1, "__context");
                    QT3DS_ASSERT(!lua_isnil(m_LuaState, -1));
                    lua_pop(m_LuaState, 1);
                    lua_setfenv(m_LuaState, -2);
                    QT3DS_ASSERT(lua_isfunction(m_LuaState, -1));

                    callResult = lua_pcall(m_LuaState, 0, 0, 0);
                    if (callResult) {
                        const char *error = lua_tostring(m_LuaState, -1);
                        qCCritical(INTERNAL_ERROR, "Error initializing lua state for material: %s: %s",
                            inMaterial.m_Id.c_str(), error);
                        return;
                    }

                    inserter.first->second = theNewContext;
                }
                if (inserter.first->second)
                    inserter.first->second->Render(inRenderContext, inTarget, inFeatureSet);
            } break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }

        if (inMaterial.m_hasRefraction)
            theContext.SetBlendingEnabled(wasBlendingEnabled);

        // Release any per-frame buffers
        for (QT3DSU32 idx = 0; idx < m_AllocatedBuffers.size(); ++idx) {
            if (m_AllocatedBuffers[idx].m_Flags.IsSceneLifetime() == false) {
                ReleaseBuffer(idx);
                --idx;
            }
        }
    }

    const char *GetShaderName(const SCustomMaterial &inMaterial) override
    {
        SMaterialClass *theClass = GetMaterialClass(inMaterial.m_ClassName);
        if (!theClass)
            return NULL;

        NVConstDataRef<dynamic::SCommand *> theCommands = theClass->m_Class->GetRenderCommands();
        TShaderAndFlags thePrepassShader;
        for (QT3DSU32 idx = 0, end = theCommands.size();
             idx < end && thePrepassShader.first.mPtr == NULL; ++idx) {
            const SCommand &theCommand = *theCommands[idx];
            if (theCommand.m_Type == CommandTypes::BindShader) {
                const SBindShader &theBindCommand = static_cast<const SBindShader &>(theCommand);
                return theBindCommand.m_ShaderPath.c_str();
            }
        }

        QT3DS_ASSERT(false);
        return NULL;
    }

    void ApplyShaderPropertyValues(const SCustomMaterial &inMaterial,
                                           NVRenderShaderProgram &inProgram) override
    {
        SMaterialClass *theClass = GetMaterialClass(inMaterial.m_ClassName);
        if (!theClass)
            return;

        SApplyInstanceValue applier;
        ApplyInstanceValue(const_cast<SCustomMaterial &>(inMaterial), *theClass, inProgram,
                           applier);
    }

    virtual void PrepareTextureForRender(SMaterialClass &inClass, SCustomMaterial &inMaterial)
    {
        NVConstDataRef<SPropertyDefinition> thePropDefs = inClass.m_Class->GetProperties();
        for (QT3DSU32 idx = 0, end = thePropDefs.size(); idx < end; ++idx) {
            if (thePropDefs[idx].m_DataType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                if (thePropDefs[idx].m_TexUsageType == NVRenderTextureTypeValue::Displace) {
                    SImage *pImage = NULL;

                    // we only do this to not miss if "None" is selected
                    CRegisteredString theStrPtr = *reinterpret_cast<CRegisteredString *>(
                        inMaterial.GetDataSectionBegin() + thePropDefs[idx].m_Offset);

                    if (theStrPtr.IsValid()) {

                        QT3DSU32 index = FindAllocatedImage(thePropDefs[idx].m_ImagePath);
                        if (index == QT3DSU32(-1)) {
                            pImage = QT3DS_NEW(m_CoreContext.GetAllocator(), SImage)();
                            m_AllocatedImages.push_back(
                                eastl::make_pair(thePropDefs[idx].m_ImagePath, pImage));
                        } else
                            pImage = m_AllocatedImages[index].second;

                        if (inMaterial.m_DisplacementMap != pImage) {
                            inMaterial.m_DisplacementMap = pImage;
                            inMaterial.m_DisplacementMap->m_ImagePath =
                                thePropDefs[idx].m_ImagePath;
                            inMaterial.m_DisplacementMap->m_ImageShaderName =
                                thePropDefs[idx].m_Name; // this is our name in the shader
                            inMaterial.m_DisplacementMap->m_VerticalTilingMode =
                                thePropDefs[idx].m_CoordOp;
                            inMaterial.m_DisplacementMap->m_HorizontalTilingMode =
                                thePropDefs[idx].m_CoordOp;
                        }
                    } else {
                        inMaterial.m_DisplacementMap = NULL;
                    }
                } else if (thePropDefs[idx].m_TexUsageType == NVRenderTextureTypeValue::Emissive2) {
                    SImage *pImage = NULL;

                    // we only do this to not miss if "None" is selected
                    CRegisteredString theStrPtr = *reinterpret_cast<CRegisteredString *>(
                        inMaterial.GetDataSectionBegin() + thePropDefs[idx].m_Offset);

                    if (theStrPtr.IsValid()) {
                        QT3DSU32 index = FindAllocatedImage(thePropDefs[idx].m_ImagePath);
                        if (index == QT3DSU32(-1)) {
                            pImage = QT3DS_NEW(m_CoreContext.GetAllocator(), SImage)();
                            m_AllocatedImages.push_back(
                                eastl::make_pair(thePropDefs[idx].m_ImagePath, pImage));
                        } else
                            pImage = m_AllocatedImages[index].second;

                        if (inMaterial.m_EmissiveMap2 != pImage) {
                            inMaterial.m_EmissiveMap2 = pImage;
                            inMaterial.m_EmissiveMap2->m_ImagePath = thePropDefs[idx].m_ImagePath;
                            inMaterial.m_EmissiveMap2->m_ImageShaderName =
                                thePropDefs[idx].m_Name; // this is our name in the shader
                            inMaterial.m_EmissiveMap2->m_VerticalTilingMode =
                                thePropDefs[idx].m_CoordOp;
                            inMaterial.m_EmissiveMap2->m_HorizontalTilingMode =
                                thePropDefs[idx].m_CoordOp;
                        }
                    } else {
                        inMaterial.m_EmissiveMap2 = NULL;
                    }
                }
            }
        }
    }

    virtual void PrepareDisplacementForRender(SMaterialClass &inClass, SCustomMaterial &inMaterial)
    {
        if (inMaterial.m_DisplacementMap == NULL)
            return;

        // our displacement mappin in MDL has fixed naming
        NVConstDataRef<SPropertyDefinition> thePropDefs = inClass.m_Class->GetProperties();
        for (QT3DSU32 idx = 0, end = thePropDefs.size(); idx < end; ++idx) {
            if (thePropDefs[idx].m_DataType == NVRenderShaderDataTypes::QT3DSF32
                && AreEqual(thePropDefs[idx].m_Name.c_str(), "displaceAmount")) {
                QT3DSF32 theValue = *reinterpret_cast<const QT3DSF32 *>(inMaterial.GetDataSectionBegin()
                                                                  + thePropDefs[idx].m_Offset);
                inMaterial.m_DisplaceAmount = theValue;
            } else if (thePropDefs[idx].m_DataType == NVRenderShaderDataTypes::QT3DSVec3
                       && AreEqual(thePropDefs[idx].m_Name.c_str(), "displace_tiling")) {
                QT3DSVec3 theValue = *reinterpret_cast<const QT3DSVec3 *>(inMaterial.GetDataSectionBegin()
                                                                    + thePropDefs[idx].m_Offset);
                if (theValue.x != inMaterial.m_DisplacementMap->m_Scale.x
                    || theValue.y != inMaterial.m_DisplacementMap->m_Scale.y) {
                    inMaterial.m_DisplacementMap->m_Scale = QT3DSVec2(theValue.x, theValue.y);
                    inMaterial.m_DisplacementMap->m_Flags.SetTransformDirty(true);
                }
            }
        }
    }

    void PrepareMaterialForRender(SMaterialClass &inClass, SCustomMaterial &inMaterial)
    {
        PrepareTextureForRender(inClass, inMaterial);

        if (inClass.m_HasDisplacement)
            PrepareDisplacementForRender(inClass, inMaterial);
    }

    // Returns true if the material is dirty and thus will produce a different render result
    // than previously.  This effects things like progressive AA.
    // TODO - return more information, specifically about transparency (object is transparent,
    // object is completely transparent
    bool PrepareForRender(const SModel & /*inModel*/, const SRenderSubset & /*inSubset*/,
                                  SCustomMaterial &inMaterial, bool clearMaterialDirtyFlags) override
    {
        SMaterialClass *theMaterialClass = GetMaterialClass(inMaterial.m_ClassName);
        if (theMaterialClass == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }

        PrepareMaterialForRender(*theMaterialClass, inMaterial);

        inMaterial.m_hasTransparency = theMaterialClass->m_HasTransparency;
        inMaterial.m_hasRefraction = theMaterialClass->m_HasRefraction;
        inMaterial.m_hasVolumetricDF = false;

        bool wasDirty = inMaterial.IsDirty() || theMaterialClass->m_AlwaysDirty;
        if (clearMaterialDirtyFlags)
            inMaterial.UpdateDirtyForFrame();

        return wasDirty;
    }

    // TODO - handle UIC specific features such as vertex offsets for prog-aa and opacity.
    void RenderSubset(SCustomMaterialRenderContext &inRenderContext,
                              TShaderFeatureSet inFeatureSet) override
    {
        SMaterialClass *theClass = GetMaterialClass(inRenderContext.m_Material.m_ClassName);

        // Ensure that our overall render context comes back no matter what the client does.
        qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderBlendFunctionArgument>
            __blendFunction(m_UICContext->GetRenderContext(), &NVRenderContext::GetBlendFunction,
                            &NVRenderContext::SetBlendFunction,
                            qt3ds::render::NVRenderBlendFunctionArgument());
        qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderBlendEquationArgument>
            __blendEquation(m_UICContext->GetRenderContext(), &NVRenderContext::GetBlendEquation,
                            &NVRenderContext::SetBlendEquation,
                            qt3ds::render::NVRenderBlendEquationArgument());

        NVRenderContextScopedProperty<bool> theBlendEnabled(m_UICContext->GetRenderContext(),
                                                            &NVRenderContext::IsBlendingEnabled,
                                                            &NVRenderContext::SetBlendingEnabled);

        DoRenderCustomMaterial(inRenderContext, inRenderContext.m_Material, *theClass,
                               m_UICContext->GetRenderContext().GetRenderTarget(), inFeatureSet);
    }

    bool RenderDepthPrepass(const QT3DSMat44 &inMVP, const SCustomMaterial &inMaterial,
                                    const SRenderSubset &inSubset) override
    {
        SMaterialClass *theClass = GetMaterialClass(inMaterial.m_ClassName);
        NVConstDataRef<dynamic::SCommand *> theCommands = theClass->m_Class->GetRenderCommands();
        TShaderAndFlags thePrepassShader;
        for (QT3DSU32 idx = 0, end = theCommands.size();
             idx < end && thePrepassShader.first.mPtr == NULL; ++idx) {
            const SCommand &theCommand = *theCommands[idx];
            if (theCommand.m_Type == CommandTypes::BindShader) {
                const SBindShader &theBindCommand = static_cast<const SBindShader &>(theCommand);
                thePrepassShader = m_UICContext->GetDynamicObjectSystem().GetDepthPrepassShader(
                    theBindCommand.m_ShaderPath, CRegisteredString(), TShaderFeatureSet());
            }
        }

        if (thePrepassShader.first.mPtr == NULL)
            return false;

        NVRenderContext &theContext = m_UICContext->GetRenderContext();
        NVRenderShaderProgram &theProgram = *thePrepassShader.first;
        theContext.SetActiveShader(&theProgram);
        theProgram.SetPropertyValue("model_view_projection", inMVP);
        theContext.SetInputAssembler(inSubset.m_InputAssemblerPoints);
        theContext.Draw(NVRenderDrawMode::Lines, inSubset.m_PosVertexBuffer->GetNumVertexes(), 0);
        return true;
    }

    void OnMaterialActivationChange(const SCustomMaterial &inMaterial, bool inActive) override
    {
        SMaterialClass *theClass = GetMaterialClass(inMaterial.m_ClassName);
        NVConstDataRef<dynamic::SCommand *> theCommands = theClass->m_Class->GetRenderCommands();
        for (QT3DSU32 idx = 0, end = theCommands.size(); idx < end; ++idx) {
            const SCommand &theCommand(*theCommands[idx]);
            if (theCommand.m_Type == CommandTypes::LuaCommand) {
                SCustomMaterial &theMaterial = const_cast<SCustomMaterial &>(inMaterial);
                TMaterialLuaContextMap::iterator theIterator =
                    m_MaterialLuaContextMap.find(&theMaterial);
                if (theIterator != m_MaterialLuaContextMap.end()
                    && theIterator->second.mPtr != NULL)
                    theIterator->second->OnActivationChange(inActive);

                break;
            }
        }
    }

    void EndFrame() override
    {
        QT3DSU64 currentFrameTime = qt3ds::foundation::Time::getCurrentTimeInTensOfNanoSeconds();
        if (m_LastFrameTime) {
            QT3DSU64 timePassed = currentFrameTime - m_LastFrameTime;
            m_MillisecondsSinceLastFrame = static_cast<QT3DSF32>(timePassed / 100000.0);
        }
        m_LastFrameTime = currentFrameTime;
    }

    void Save(qt3ds::render::SWriteBuffer &ioBuffer,
                      const qt3ds::render::SStrRemapMap &inRemapMap,
                      const char8_t * /*inProjectDir*/) const override
    {
        QT3DSU32 offset = ioBuffer.size();
        ioBuffer.write((QT3DSU32)m_StringMaterialMap.size());
        for (TStringMaterialMap::const_iterator iter = m_StringMaterialMap.begin(),
                                                end = m_StringMaterialMap.end();
             iter != end; ++iter) {
            size_t nameOffset = ioBuffer.size() - offset;
            (void)nameOffset;
            CRegisteredString materialName(iter->first);
            materialName.Remap(inRemapMap);
            ioBuffer.write(materialName);
            const SMaterialClass *materialClass = iter->second.mPtr;
            QT3DSU32 offset = ioBuffer.size();
            ioBuffer.write(*materialClass);
            QT3DSU8 *materialOffset = ioBuffer.begin() + offset;
            SMaterialClass *writtenClass = (SMaterialClass *)materialOffset;
            writtenClass->AfterWrite();
            ioBuffer.align(4);
        }
    }

    void Load(NVDataRef<QT3DSU8> inData, CStrTableOrDataRef inStrDataBlock,
                      const char8_t * /*inProjectDir*/) override
    {
        m_Allocator.m_PreAllocatedBlock = inData;
        m_Allocator.m_OwnsMemory = false;
        qt3ds::render::SDataReader theReader(inData.begin(), inData.end());
        QT3DSU32 numMaterialClasses = theReader.LoadRef<QT3DSU32>();
        for (QT3DSU32 idx = 0; idx < numMaterialClasses; ++idx) {
            CRegisteredString clsName = theReader.LoadRef<CRegisteredString>();
            clsName.Remap(inStrDataBlock);
            IDynamicObjectClass *theDynamicCls =
                m_CoreContext.GetDynamicObjectSystemCore().GetDynamicObjectClass(clsName);
            SMaterialClass *theReadClass = theReader.Load<SMaterialClass>();
            theReader.Align(4);
            if (theDynamicCls) {
                theReadClass->AfterRead(m_Allocator, *theDynamicCls);
                m_StringMaterialMap.insert(eastl::make_pair(clsName, theReadClass));
            }
        }
    }

    ICustomMaterialSystem &GetCustomMaterialSystem(IUICRenderContext &inContext) override
    {
        m_UICContext = &inContext;

        // check for fast blits
        NVRenderContext &theContext = m_UICContext->GetRenderContext();
        m_UseFastBlits = theContext.GetRenderBackendCap(
            qt3ds::render::NVRenderBackend::NVRenderBackendCaps::FastBlits);

        return *this;
    }

    //=======================================================================
    // Begin material system lua functions
    //=======================================================================
    static SMaterialSystem &GetMaterialSystem(lua_State *inState)
    {
        STopScope __scope(inState);
        lua_getfield(inState, LUA_REGISTRYINDEX, "__materialSystem");
        return *reinterpret_cast<SMaterialSystem *>(lua_touserdata(inState, -1));
    }

    static SMaterialLuaContext *GetMaterialContext(lua_State *inState)
    {
        STopScope __scope(inState);
        // hack, but I don't want to work around this elegantly right now.  The problem is that
        // the global index indexes to a fixed table, not the actually current acting globals table.
        // To get that, I would need to use getfenv and getinfo and friends which is not worth it.
        lua_getfield(inState, LUA_REGISTRYINDEX, "__current_context");
        SMaterialLuaContext *retval =
            reinterpret_cast<SMaterialLuaContext *>(lua_touserdata(inState, -1));
        return retval;
    }

    // Leave the context table TOS
    static void PushMaterialContextTable(lua_State *inState, QT3DSU32 inContextId)
    {
        lua_getfield(inState, LUA_REGISTRYINDEX, "__materialcontexts");
        lua_rawgeti(inState, -1, inContextId);
        lua_insert(inState, -2); // swap the table of contexts and the context table
        lua_pop(
            inState,
            1); // get rid of the table of contexts, leaving just the context table top of stack.
    }

    void RegisterGlobalFunctions()
    {
        lua_pushcfunction(m_LuaState, LuaOutput);
        lua_pushvalue(m_LuaState, -1);
        lua_setfield(m_LuaState, LUA_GLOBALSINDEX, "output");
        // Really, we should have done this a long time ago, overridden print.
        lua_setfield(m_LuaState, LUA_GLOBALSINDEX, "print");

        lua_register(m_LuaState, "getOrCreateVertexBuffer", LuaGetOrCreateVertexBuffer);
        lua_register(m_LuaState, "bindShader", LuaBindShader);
        lua_register(m_LuaState, "bindComputeInput", LuaBindComputeInput);
        lua_register(m_LuaState, "applyInstanceValues", LuaApplyInstanceValues);
        lua_register(m_LuaState, "executeCompute", LuaExecuteCompute);
        lua_register(m_LuaState, "memoryBarrier", LuaMemoryBarrier);
        lua_register(m_LuaState, "getOrCreateInputAssembler", LuaGetOrCreateInputAssembler);
        lua_register(m_LuaState, "bindInputAssembler", LuaBindInputAssembler);
        lua_register(m_LuaState, "draw", LuaDraw);
        lua_register(m_LuaState, "setBlendFunction", LuaSetBlendFunction);
        lua_register(m_LuaState, "getGlobalTransform", LuaGetGlobalTransform);
        lua_register(m_LuaState, "setUniform", LuaSetUniform);
        lua_register(m_LuaState, "invertMatrix", LuaInvertMatrix);
        lua_register(m_LuaState, "getAttribute", LuaGetAttribute);
        lua_pushboolean(m_LuaState, m_UICContext->IsAuthoringMode() ? 1 : 0);
        lua_setfield(m_LuaState, LUA_GLOBALSINDEX, "authoring_mode");
    }

    static int LuaOutput(lua_State *inState)
    {
        STopScope __scope(inState);

        lua_concat(inState, lua_gettop(inState));
        const char *message = lua_tostring(inState, -1);
        qCDebug(TRACE_INFO, "%s", nonNull(message));
        return 0;
    }

    static int LuaGetOrCreateVertexBuffer(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        luaL_checktype(inState, 1, LUA_TSTRING);
        luaL_checktype(inState, 2, LUA_TNUMBER);
        luaL_checktype(inState, 3, LUA_TNUMBER);
        const char *idStr = lua_tostring(inState, 1);
        int bufferSize = (int)lua_tonumber(inState, 2);
        int bufferStride = (int)lua_tonumber(inState, 3);
        bool created = currentContext->GetOrCreateVertexBuffer(idStr, bufferSize, bufferStride);
        lua_pushboolean(inState, created ? 1 : 0);
        return 1;
    }

    CRegisteredString GetShaderName(const char *inObjectName, const char8_t *inShaderSpecificName)
    {
        m_ShaderNameBuilder.assign(inObjectName);
        m_ShaderNameBuilder.append(" - ");
        m_ShaderNameBuilder.append(inShaderSpecificName);
        return m_CoreContext.GetStringTable().RegisterStr(m_ShaderNameBuilder.c_str());
    }

    static int LuaBindShader(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        luaL_checktype(inState, 1, LUA_TSTRING);
        const char *shaderIdStr = lua_tostring(inState, 1);
        const char *className = currentContext->m_RenderContext->m_Material.m_ClassName;
        SBindShader theBindCommand;
        theBindCommand.m_ShaderPath =
            currentContext->m_MaterialSystem.GetShaderName(className, shaderIdStr);
        // We don't care about the result.
        currentContext->m_CurrentShader = currentContext->m_MaterialSystem.BindShaderFromLua(
            *currentContext->m_RenderContext, theBindCommand, currentContext->m_FeatureSet);

        return 0;
    }

    static int LuaBindComputeInput(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        luaL_checktype(inState, 1, LUA_TSTRING);
        luaL_checktype(inState, 2, LUA_TNUMBER);
        const char *bufferIdStr = lua_tostring(inState, 1);
        QT3DSU32 bufferIndex = (QT3DSU32)lua_tonumber(inState, 2);
        qt3ds::render::NVRenderDataBuffer *baseBuffer = currentContext->GetVertexBuffer(bufferIdStr);
        if (!baseBuffer) {
            QT3DS_ASSERT(false);
            return 0;
        }

        if (currentContext->m_CurrentShader.IsComputeShader())
            currentContext->m_CurrentShader.ComputeShader().BindComputeInput(baseBuffer,
                                                                             bufferIndex);

        return 0;
    }

    static int LuaApplyInstanceValues(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        if (currentContext->m_CurrentShader.IsValid()) {
            SApplyInstanceValue applier;
            NVRenderShaderProgram *theProgram(NULL);
            if (currentContext->m_CurrentShader.IsMaterialShader())
                theProgram = currentContext->m_CurrentShader.MaterialShader().m_Shader;
            else
                theProgram = &currentContext->m_CurrentShader.ComputeShader();

            currentContext->m_MaterialSystem.ApplyInstanceValue(
                currentContext->m_Material, currentContext->m_Class, *theProgram, applier);
        }
        return 0;
    }

    static int LuaExecuteCompute(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        int numArgs = lua_gettop(inState);
        QT3DSU32 workgroupX = 1, workgroupY = 1, workgroupZ = 1;
        switch (numArgs) {
        case 3:
            luaL_checknumber(inState, 3);
            workgroupZ = (QT3DSU32)lua_tonumber(inState, 3);
        case 2:
            luaL_checknumber(inState, 2);
            workgroupY = (QT3DSU32)lua_tonumber(inState, 2);
        case 1:
        default:
            luaL_checknumber(inState, 1);
            workgroupX = (QT3DSU32)lua_tonumber(inState, 1);
            break;
        }

        if (currentContext->m_CurrentShader.IsComputeShader()) {
            NVRenderContext &theContext =
                currentContext->m_MaterialSystem.m_UICContext->GetRenderContext();
            theContext.DispatchCompute(&currentContext->m_CurrentShader.ComputeShader(), workgroupX,
                                       workgroupY, workgroupZ);
        }
        return 0;
    }

    static int LuaMemoryBarrier(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        QT3DSU32 flags = 0;
        int numArgs = lua_gettop(inState);
        for (int idx = 0; idx < numArgs; ++idx) {
            const char *data = lua_tostring(inState, idx + 1);
            for (QT3DSU32 flagIdx = 0, flagEnd = g_StringMemoryFlagMapSize; flagIdx < flagEnd;
                 ++flagIdx) {
                if (AreEqual(g_StringMemoryFlagMap[flagIdx].m_Name, data)) {
                    flags = flags | (QT3DSU32)g_StringMemoryFlagMap[flagIdx].m_Value;
                    break;
                }
            }
        }
        if (flags == 0)
            flags = (QT3DSU32)qt3ds::render::NVRenderBufferBarrierValues::All;
        NVRenderContext &theContext =
            currentContext->m_MaterialSystem.m_UICContext->GetRenderContext();
        theContext.SetMemoryBarrier(qt3ds::render::NVRenderBufferBarrierFlags(flags));
        return 0;
    }

    static int LuaGetOrCreateInputAssembler(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        luaL_checktype(inState, 1, LUA_TSTRING);
        const char *assemblerIdStr = lua_tostring(inState, 1);
        IStringTable &theStrTable(currentContext->m_MaterialSystem.m_CoreContext.GetStringTable());
        NVRenderContext &theRenderContext =
            currentContext->m_MaterialSystem.m_UICContext->GetRenderContext();
        CRegisteredString assemblerId = theStrTable.RegisterStr(assemblerIdStr);
        eastl::pair<TStringAssemblerMap::iterator, bool> inserter =
            currentContext->m_InputAssemblers.insert(
                eastl::make_pair(assemblerId, NVScopedRefCounted<NVRenderInputAssembler>()));
        if (inserter.second) {
            // This one is pretty hardcore.
            NVRenderVertexBuffer *theVertexBuffer(
                currentContext->GetVertexBuffer(lua_tostring(inState, 2)));
            // NVRenderIndexBuffer* theIndexBuffer( NULL );
            eastl::vector<qt3ds::render::NVRenderVertexBufferEntry> entries;
            // Build the set of vbuf entries.
            luaL_checktype(inState, 3, LUA_TTABLE);
            lua_pushvalue(inState, 3);
            // Iterate through outer table values.  WE don't care about keys.
            lua_pushnil(inState);
            while (lua_next(inState, -2)) {
                int TOS = lua_gettop(inState);
                // Can't use checktype with relative indexes.
                luaL_checktype(inState, TOS, LUA_TTABLE);
                qt3ds::render::NVRenderVertexBufferEntry theEntry;
                // Ensure defaults are setup intelligently.
                theEntry.m_ComponentType = qt3ds::render::NVRenderComponentTypes::QT3DSF32;
                theEntry.m_FirstItemOffset = 0;
                // iterate through inner table
                lua_pushnil(inState);
                while (lua_next(inState, -2)) {
                    // dup the key
                    lua_pushvalue(inState, -2);
                    const char *keyName = lua_tostring(inState, -1);
                    if (AreEqual(keyName, "name")) {
                        const char *valueStr = lua_tostring(inState, -2);
                        theEntry.m_Name = theStrTable.RegisterStr(valueStr);
                    } else if (AreEqual(keyName, "type")) // ignored for now; they are all float
                    {

                    } else if (AreEqual(keyName, "offset")) {
                        theEntry.m_FirstItemOffset = (QT3DSU32)lua_tonumber(inState, -2);
                    } else if (AreEqual(keyName, "num_components")) {
                        theEntry.m_NumComponents = (QT3DSU32)lua_tonumber(inState, -2);
                    }
                    // pop duplicated key and value off stack.
                    lua_pop(inState, 2);
                }
                // Pop both last table key and current value off stack
                if (!isTrivial(theEntry.m_Name))
                    entries.push_back(theEntry);
                lua_settop(inState, TOS - 1);
            }
            // pop leftover key off stack.
            lua_pop(inState, 1);

            QT3DSU32 stride = lua_tonumber(inState, 5);
            QT3DSU32 offset = lua_tonumber(inState, 6);
            const char *primTypeStr = lua_tostring(inState, 7);
            NVRenderDrawMode::Enum theDrawMode(NVRenderDrawMode::Triangles);
            if (AreEqual(primTypeStr, "points"))
                theDrawMode = NVRenderDrawMode::Points;
            else if (AreEqual(primTypeStr, "lines"))
                theDrawMode = NVRenderDrawMode::Lines;
            else if (AreEqual(primTypeStr, "triangles"))
                theDrawMode = NVRenderDrawMode::Triangles;

            NVRenderAttribLayout *theLayout = theRenderContext.CreateAttributeLayout(
                toConstDataRef(entries.data(), entries.size()));
            inserter.first->second = theRenderContext.CreateInputAssembler(
                theLayout, toConstDataRef(&theVertexBuffer, 1), NULL, toConstDataRef(stride),
                toConstDataRef(offset), theDrawMode);
        }
        lua_pushboolean(inState, inserter.second ? 1 : 0);
        return 1;
    }

    static int LuaBindInputAssembler(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        IStringTable &theStrTable(currentContext->m_MaterialSystem.m_CoreContext.GetStringTable());
        luaL_checktype(inState, 1, LUA_TSTRING);
        const char *assemblerId = lua_tostring(inState, 1);
        currentContext->m_CurrentInputAssembler = NULL;
        if (AreEqual(assemblerId, "source_assembler")) {
            currentContext->m_CurrentInputAssembler =
                currentContext->m_RenderContext->m_Subset.m_InputAssembler;
        } else {
            CRegisteredString assemblerIdStr = theStrTable.RegisterStr(assemblerId);
            TStringAssemblerMap::iterator iter =
                currentContext->m_InputAssemblers.find(assemblerIdStr);
            if (iter != currentContext->m_InputAssemblers.end())
                currentContext->m_CurrentInputAssembler = iter->second;
        }
        return 0;
    }
    static qt3ds::render::NVRenderSrcBlendFunc::Enum ToBlendFunc(const char8_t *str)
    {
        for (QT3DSU32 idx = 0, end = g_BlendFuncMapSize; idx < end; ++idx) {
            if (AreEqual(str, g_BlendFuncMap[idx].m_Name))
                return g_BlendFuncMap[idx].m_Value;
        }
        QT3DS_ASSERT(false);
        return qt3ds::render::NVRenderSrcBlendFunc::Unknown;
    }

    static int LuaSetBlendFunction(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        NVRenderContext &theContext(
            currentContext->m_MaterialSystem.m_UICContext->GetRenderContext());
        if (lua_gettop(inState) == 0) {
            theContext.SetBlendingEnabled(false);
        } else {
            const char8_t *srcEquation = lua_tostring(inState, 1);
            const char8_t *destEquation = lua_tostring(inState, 2);
            theContext.SetBlendingEnabled(true);
            qt3ds::render::NVRenderSrcBlendFunc::Enum srcFunc(ToBlendFunc(srcEquation));
            qt3ds::render::NVRenderDstBlendFunc::Enum destFunc(
                static_cast<qt3ds::render::NVRenderDstBlendFunc::Enum>(ToBlendFunc(destEquation)));
            qt3ds::render::NVRenderBlendFunctionArgument args(srcFunc, destFunc, srcFunc, destFunc);
            qt3ds::render::NVRenderBlendEquationArgument blendEqu(NVRenderBlendEquation::Add,
                                                               NVRenderBlendEquation::Add);
            theContext.SetBlendFunction(args);
            theContext.SetBlendEquation(blendEqu);
        }
        return 0;
    }

    static int LuaDraw(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        if (!currentContext->m_CurrentShader.IsMaterialShader()) {
            QT3DS_ASSERT(false);
            return 0;
        }
        if (!currentContext->m_CurrentInputAssembler) {
            currentContext->m_CurrentInputAssembler =
                currentContext->m_RenderContext->m_Subset.m_InputAssembler;
        }

        QT3DSU32 count = (QT3DSU32)lua_tonumber(inState, 1);
        QT3DSU32 offset = (QT3DSU32)lua_tonumber(inState, 2);
        currentContext->m_MaterialSystem.RenderPass(
            *currentContext->m_RenderContext, currentContext->m_CurrentShader.MaterialShader(),
            NULL, currentContext->m_Target, false, *currentContext->m_CurrentInputAssembler, count,
            offset);
        return 0;
    }

    static int LuaGetGlobalTransform(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        const QT3DSF32 *matData = currentContext->m_RenderContext->m_ModelMatrix.front();
        // had better be packed...
        size_t byteSize = sizeof(currentContext->m_RenderContext->m_ModelMatrix);
        QT3DSU8 *dataPtr = reinterpret_cast<QT3DSU8 *>(lua_newuserdata(inState, byteSize));
        memCopy(dataPtr, matData, (QT3DSU32)byteSize);
        return 1;
    }

    static int LuaSetUniform(lua_State *inState)
    {
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        if (currentContext->m_CurrentShader.IsValid() == false) {
            lua_pushstring(inState, "No bound shader");
            lua_error(inState);
        }
        luaL_checktype(inState, 1, LUA_TSTRING);
        const char *uniName = lua_tostring(inState, 1);
        NVRenderShaderProgram *theProgram = NULL;
        if (currentContext->m_CurrentShader.IsComputeShader())
            theProgram = &currentContext->m_CurrentShader.ComputeShader();
        else
            theProgram = currentContext->m_CurrentShader.MaterialShader().m_Shader.mPtr;
        NVRenderShaderConstantBase *theBase = theProgram->GetShaderConstant(uniName);
        // We don't error here because that makes editing and testing really tough.
        if (theBase) {
            NVRenderShaderDataTypes::Enum theConstantType = theBase->GetShaderConstantType();
            // Someone less lazy than me can fill the rest of this in
            switch (theConstantType) {
            case NVRenderShaderDataTypes::QT3DSF32: {
                if (theBase->m_ElementCount == 1) {
                    QT3DSF32 theValue = lua_tonumber(inState, 2);
                    theProgram->SetConstantValue(theBase, theValue, 1);
                }
            } break;
            case NVRenderShaderDataTypes::QT3DSMat44: {
                if (theBase->m_ElementCount == 1) {
                    luaL_checktype(inState, 2, LUA_TUSERDATA);
                    size_t matLen = lua_objlen(inState, 2);
                    if (matLen == sizeof(QT3DSMat44)) {
                        QT3DSMat44 *dataPtr = (QT3DSMat44 *)lua_touserdata(inState, 2);
                        theProgram->SetConstantValue(theBase, *dataPtr, 1);
                    } else {
                        lua_pushstring(inState, "Invalid matrix length for matrix uniform");
                        lua_error(inState);
                    }
                }
            } break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
        return 0;
    }

    static int LuaInvertMatrix(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TUSERDATA);
        size_t matLen = lua_objlen(inState, 1);
        if (matLen == sizeof(QT3DSMat44)) {
            QT3DSMat44 *dataPtr = (QT3DSMat44 *)lua_touserdata(inState, 1);
            QT3DSMat44 newMatrix = dataPtr->getInverse();
            void *newPtr = lua_newuserdata(inState, matLen);
            memCopy(newPtr, &newMatrix, (QT3DSU32)matLen);
            return 1;
        } else {
            lua_pushstring(inState, "Invalid matrix length for matrix uniform");
            lua_error(inState);
        }
        return 0;
    }

    static int LuaGetAttribute(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TSTRING);
        const char *attName = lua_tostring(inState, 1);
        SMaterialLuaContext *currentContext = GetMaterialContext(inState);
        if (!currentContext) {
            QT3DS_ASSERT(false);
            return 0;
        }
        CRegisteredString propName =
            currentContext->m_MaterialSystem.m_UICContext->GetStringTable().RegisterStr(attName);
        NVConstDataRef<dynamic::SPropertyDefinition> props =
            currentContext->m_Class.m_Class->GetProperties();
        for (QT3DSU32 idx = 0, end = props.size(); idx < end; ++idx) {
            if (propName == props[idx].m_Name) {
                switch (props[idx].m_DataType) {
                case NVRenderShaderDataTypes::QT3DSRenderBool: {
                    bool theValue = *reinterpret_cast<const bool *>(
                        currentContext->m_Material.GetDataSectionBegin() + props[idx].m_Offset);
                    lua_pushboolean(inState, theValue ? 1 : 0);
                    return 1;
                }
                case NVRenderShaderDataTypes::QT3DSF32: {
                    QT3DSF32 theValue = *reinterpret_cast<const QT3DSF32 *>(
                        currentContext->m_Material.GetDataSectionBegin() + props[idx].m_Offset);
                    lua_pushnumber(inState, theValue);
                    return 1;
                }
                default:
                    lua_pushstring(inState, "Property is of incorrect type");
                    lua_error(inState);
                }
            }
        }
        lua_pushstring(inState, "Failed to find property");
        lua_error(inState);
        return 0;
    }
};

static void *LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    SMaterialSystem *ctx = reinterpret_cast<SMaterialSystem *>(ud);
    if (nsize == 0) {
        if (ptr)
            ctx->m_CoreContext.GetAllocator().deallocate(ptr);
        return NULL;
    } else {
        if (nsize < osize && ptr)
            return ptr;

        void *newMem =
            ctx->m_CoreContext.GetAllocator().allocate(nsize, "lua memory", __FILE__, __LINE__);
        if (osize && ptr) {
            size_t copyAmt = NVMin(osize, nsize);
            memCopy(newMem, ptr, (QT3DSU32)copyAmt);
            ctx->m_CoreContext.GetAllocator().deallocate(ptr);
        }
        return newMem;
    }
}

SMaterialLuaContext::~SMaterialLuaContext()
{
}
void SMaterialLuaContext::DeleteThis()
{
    NVAllocatorCallback &theCallback = m_MaterialSystem.m_CoreContext.GetAllocator();
    NVDelete(theCallback, this);
}
void SMaterialLuaContext::DoRender()
{
    STopScope __scope(&m_LuaState);
    m_MaterialSystem.PushMaterialContextTable(m_MaterialSystem.m_LuaState, m_LuaId);
    // Update with per-render information
    QT3DSU32 numEntries = m_RenderContext->m_Subset.m_Count;
    switch (m_RenderContext->m_Subset.m_PrimitiveType) {
    default:
        QT3DS_ASSERT(false);
    case NVRenderDrawMode::Points:
        break;
    case NVRenderDrawMode::Lines:
        numEntries /= 2;
        break;
    case NVRenderDrawMode::Triangles:
        numEntries /= 3;
        break;
    }
    lua_pushnumber(&m_LuaState, numEntries);
    lua_setfield(&m_LuaState, -2, "source_mesh_entries");
    lua_pushnumber(&m_LuaState, m_RenderContext->m_Subset.m_VertexBuffer->GetNumVertexes());
    lua_setfield(&m_LuaState, -2, "source_mesh_vertices");
    lua_pushnumber(&m_LuaState, m_MaterialSystem.m_MillisecondsSinceLastFrame);
    lua_setfield(&m_LuaState, -2, "milliseconds_since_last_frame");
    lua_getfield(&m_LuaState, -1, "onRender");
    if (lua_isfunction(&m_LuaState, -1)) {
        lua_pushlightuserdata(&m_LuaState, this);
        lua_setfield(&m_LuaState, LUA_REGISTRYINDEX, "__current_context");
        int callResult = lua_pcall(&m_LuaState, 0, 0, 0);
        if (callResult) {
            qCCritical(INVALID_OPERATION, "%s", lua_tostring(&m_LuaState, -1));
        }
    }
}

void SMaterialLuaContext::OnActivationChange(bool inActive)
{
    STopScope __scope(&m_LuaState);
    m_MaterialSystem.PushMaterialContextTable(m_MaterialSystem.m_LuaState, m_LuaId);
    const char *fnName = inActive ? "onActivate" : "onDeactivate";
    lua_getfield(&m_LuaState, -1, fnName);
    if (lua_isfunction(&m_LuaState, -1)) {
        lua_pushlightuserdata(&m_LuaState, this);
        lua_setfield(&m_LuaState, LUA_REGISTRYINDEX, "__current_context");
        int callResult = lua_pcall(&m_LuaState, 0, 0, 0);
        if (callResult) {
            const char *message = lua_tostring(&m_LuaState, -1);
            qCCritical(INVALID_OPERATION, "%s", message);
        }
    }
}

bool SMaterialLuaContext::GetOrCreateVertexBuffer(const char *inIdStr, QT3DSU32 inSize, QT3DSU32 inStride)
{
    CRegisteredString mapKey = m_MaterialSystem.m_CoreContext.GetStringTable().RegisterStr(inIdStr);
    eastl::pair<TStringVertexBufferMap::iterator, bool> inserter = m_VertexBuffers.insert(
        eastl::make_pair(mapKey, NVScopedRefCounted<NVRenderVertexBuffer>()));
    if (inserter.second) {
        inserter.first->second =
            m_MaterialSystem.m_UICContext->GetRenderContext().CreateVertexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Dynamic, inSize, inStride);
    }
    return inserter.second;
}

NVRenderVertexBuffer *SMaterialLuaContext::GetVertexBuffer(const char *inIdStr)
{
    if (AreEqual("source_mesh", nonNull(inIdStr)))
        return m_RenderContext->m_Subset.m_VertexBuffer;

    CRegisteredString mapKey = m_MaterialSystem.m_CoreContext.GetStringTable().RegisterStr(inIdStr);
    TStringVertexBufferMap::iterator theFind = m_VertexBuffers.find(mapKey);
    if (theFind != m_VertexBuffers.end())
        return theFind->second.mPtr;
    return NULL;
}
}

ICustomMaterialSystemCore &
ICustomMaterialSystemCore::CreateCustomMaterialSystemCore(IUICRenderContextCore &ctx)
{
    return *QT3DS_NEW(ctx.GetAllocator(), SMaterialSystem)(ctx);
}
