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
#include "Qt3DSRenderShaderCodeGenerator.h"

using namespace qt3ds::render;

using eastl::make_pair;

SShaderCodeGeneratorBase::SShaderCodeGeneratorBase(IStringTable &inStringTable,
                                                   NVAllocatorCallback &alloc,
                                                   qt3ds::render::NVRenderContextType ctxType)
    : m_StringTable(inStringTable)
    , m_Codes(alloc, "SShaderCodeGenerator::m_Codes")
    , m_Includes(alloc, "SShaderCodeGenerator::m_Includes")
    , m_Uniforms(alloc, "SShaderCodeGenerator::m_Uniforms")
    , m_ConstantBuffers(alloc, "SShaderCodeGenerator::m_ConstantBuffers")
    , m_ConstantBufferParams(alloc, "SShaderCodeGenerator::m_ConstantBufferParams")
    , m_Attributes(alloc, "SShaderCodeGenerator::m_Uniforms")
    , m_RenderContextType(ctxType)
{
}
void SShaderCodeGeneratorBase::Begin()
{
    m_Uniforms.clear();
    GetVaryings().clear();
    m_Attributes.clear();
    m_Includes.clear();
    m_Codes.clear();
    m_FinalShaderBuilder.clear();
    m_CodeBuilder.clear();
    m_ConstantBuffers.clear();
    m_ConstantBufferParams.clear();
}
void SShaderCodeGeneratorBase::Append(const char *data)
{
    m_CodeBuilder.append(data);
    m_CodeBuilder.append("\n");
}
// don't add the newline
void SShaderCodeGeneratorBase::AppendPartial(const char *data)
{
    m_CodeBuilder.append(data);
}
void SShaderCodeGeneratorBase::AddUniform(const char *name, const char *type)
{
    m_Uniforms.insert(make_pair(m_StringTable.RegisterStr(name), m_StringTable.RegisterStr(type)));
}
void SShaderCodeGeneratorBase::AddUniform(TStrType &name, const char *type)
{
    AddUniform(name.c_str(), type);
}

void SShaderCodeGeneratorBase::AddConstantBuffer(const char *name, const char *layout)
{
    m_ConstantBuffers.insert(
        make_pair(m_StringTable.RegisterStr(name), m_StringTable.RegisterStr(layout)));
}
void SShaderCodeGeneratorBase::AddConstantBufferParam(const char *cbName, const char *paramName,
                                                      const char *type)
{
    TParamPair theParamPair(m_StringTable.RegisterStr(paramName), m_StringTable.RegisterStr(type));
    TConstantBufferParamPair theBufferParamPair(m_StringTable.RegisterStr(cbName), theParamPair);
    m_ConstantBufferParams.push_back(theBufferParamPair);
}
void SShaderCodeGeneratorBase::AddAttribute(const char *name, const char *type)
{
    m_Attributes.insert(
        make_pair(m_StringTable.RegisterStr(name), m_StringTable.RegisterStr(type)));
}
void SShaderCodeGeneratorBase::AddAttribute(TStrType &name, const char *type)
{
    AddAttribute(name.c_str(), type);
}
void SShaderCodeGeneratorBase::AddVarying(const char *name, const char *type)
{
    GetVaryings().insert(
        make_pair(m_StringTable.RegisterStr(name), m_StringTable.RegisterStr(type)));
}
void SShaderCodeGeneratorBase::AddVarying(TStrType &name, const char *type)
{
    AddVarying(name.c_str(), type);
}
void SShaderCodeGeneratorBase::AddLocalVariable(const char *name, const char *type, int tabCount)
{
    for (; tabCount >= 0; --tabCount)
        m_CodeBuilder.append("\t");
    m_CodeBuilder.append(type);
    m_CodeBuilder.append(" ");
    m_CodeBuilder.append(name);
    m_CodeBuilder.append(";\n");
}

void SShaderCodeGeneratorBase::AddInclude(const char *name)
{
    m_Includes.insert(m_StringTable.RegisterStr(name));
}
void SShaderCodeGeneratorBase::AddInclude(TStrType &name)
{
    AddInclude(name.c_str());
}
void SShaderCodeGeneratorBase::AddLocalVariable(TStrType &name, const char *type, int tabCount)
{
    AddLocalVariable(name.c_str(), type, tabCount);
}
bool SShaderCodeGeneratorBase::HasCode(Enum value)
{
    return m_Codes.contains(value);
}
void SShaderCodeGeneratorBase::SetCode(Enum value)
{
    m_Codes.insert((QT3DSU32)value);
}

void SShaderCodeGeneratorBase::SetupWorldPosition()
{
    if (!HasCode(WorldPosition)) {
        SetCode(WorldPosition);
        AddUniform("model_matrix", "mat4");
        Append("\tvec3 varWorldPos = (model_matrix * vec4(attr_pos, 1.0)).xyz;");
    }
}

void SShaderCodeGeneratorBase::GenerateViewVector()
{
    if (!HasCode(ViewVector)) {
        SetCode(ViewVector);
        SetupWorldPosition();
        AddInclude("viewProperties.glsllib");
        Append("\tvec3 view_vector = normalize(camera_position - varWorldPos);");
    }
}

void SShaderCodeGeneratorBase::GenerateWorldNormal()
{
    if (!HasCode(WorldNormal)) {
        SetCode(WorldNormal);
        AddAttribute("attr_norm", "vec3");
        AddUniform("normal_matrix", "mat3");
        Append("\tvec3 world_normal = normalize(normal_matrix * objectNormal).xyz;");
    }
}

void SShaderCodeGeneratorBase::GenerateEnvMapReflection(SShaderCodeGeneratorBase &inFragmentShader)
{
    if (!HasCode(EnvMapReflection)) {
        SetCode(EnvMapReflection);
        SetupWorldPosition();
        GenerateWorldNormal();
        AddInclude("viewProperties.glsllib");
        AddVarying("var_object_to_camera", "vec3");
        Append("\tvar_object_to_camera = normalize( varWorldPos - camera_position );");
        // World normal cannot be relied upon in the vertex shader because of bump maps.
        inFragmentShader.Append("\tvec3 environment_map_reflection = reflect( "
                                "vec3(var_object_to_camera.x, var_object_to_camera.y, "
                                "var_object_to_camera.z), world_normal.xyz );");
        inFragmentShader.Append("\tenvironment_map_reflection *= vec3( 0.5, 0.5, 0 );");
        inFragmentShader.Append("\tenvironment_map_reflection += vec3( 0.5, 0.5, 1.0 );");
    }
}

void SShaderCodeGeneratorBase::GenerateUVCoords()
{
    if (!HasCode(UVCoords)) {
        SetCode(UVCoords);
        AddAttribute("attr_uv0", "vec2");
        Append("\tvec2 uv_coords = attr_uv0;");
    }
}

void SShaderCodeGeneratorBase::GenerateTextureSwizzle(NVRenderTextureSwizzleMode::Enum swizzleMode,
                                                      eastl::basic_string<char8_t> &texSwizzle,
                                                      eastl::basic_string<char8_t> &lookupSwizzle)
{
    qt3ds::render::NVRenderContextType deprecatedContextFlags(NVRenderContextValues::GL2
                                                           | NVRenderContextValues::GLES2);

    if (!(m_RenderContextType & deprecatedContextFlags)) {
        switch (swizzleMode) {
        case NVRenderTextureSwizzleMode::L8toR8:
        case NVRenderTextureSwizzleMode::L16toR16:
            texSwizzle.append(".rgb");
            lookupSwizzle.append(".rrr");
            break;
        case NVRenderTextureSwizzleMode::L8A8toRG8:
            texSwizzle.append(".rgba");
            lookupSwizzle.append(".rrrg");
            break;
        case NVRenderTextureSwizzleMode::A8toR8:
            texSwizzle.append(".a");
            lookupSwizzle.append(".r");
            break;
        default:
            break;
        }
    }
}

void SShaderCodeGeneratorBase::GenerateShadedWireframeBase()
{
    // how this all work see
    // http://developer.download.nvidia.com/SDK/10.5/direct3d/Source/SolidWireframe/Doc/SolidWireframe.pdf
    Append("// project points to screen space\n"
           "\tvec3 p0 = vec3(viewport_matrix * (gl_in[0].gl_Position / gl_in[0].gl_Position.w));\n"
           "\tvec3 p1 = vec3(viewport_matrix * (gl_in[1].gl_Position / gl_in[1].gl_Position.w));\n"
           "\tvec3 p2 = vec3(viewport_matrix * (gl_in[2].gl_Position / gl_in[2].gl_Position.w));\n"
           "// compute triangle heights\n"
           "\tfloat e1 = length(p1 - p2);\n"
           "\tfloat e2 = length(p2 - p0);\n"
           "\tfloat e3 = length(p1 - p0);\n"
           "\tfloat alpha = acos( (e2*e2 + e3*e3 - e1*e1) / (2.0*e2*e3) );\n"
           "\tfloat beta = acos( (e1*e1 + e3*e3 - e2*e2) / (2.0*e1*e3) );\n"
           "\tfloat ha = abs( e3 * sin( beta ) );\n"
           "\tfloat hb = abs( e3 * sin( alpha ) );\n"
           "\tfloat hc = abs( e2 * sin( alpha ) );\n");
}

void SShaderCodeGeneratorBase::AddShaderItemMap(const char *itemType,
                                                const TStrTableStrMap &itemMap)
{
    m_FinalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end;
         ++iter) {
        m_FinalShaderBuilder.append(itemType);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->second);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->first);
        m_FinalShaderBuilder.append(";\n");
    }
}

void SShaderCodeGeneratorBase::AddShaderConstantBufferItemMap(
    const char *itemType, const TStrTableStrMap &cbMap, TConstantBufferParamArray cbParamsArray)
{
    m_FinalShaderBuilder.append("\n");

    // iterate over all constant buffers
    for (TStrTableStrMap::const_iterator iter = cbMap.begin(), end = cbMap.end(); iter != end;
         ++iter) {
        m_FinalShaderBuilder.append(iter->second);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(itemType);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->first);
        m_FinalShaderBuilder.append(" {\n");
        // iterate over all param entries and add match
        for (TConstantBufferParamArray::const_iterator iter1 = cbParamsArray.begin(),
                                                       end = cbParamsArray.end();
             iter1 != end; ++iter1) {
            if (iter1->first == iter->first) {
                m_FinalShaderBuilder.append(iter1->second.second);
                m_FinalShaderBuilder.append(" ");
                m_FinalShaderBuilder.append(iter1->second.first);
                m_FinalShaderBuilder.append(";\n");
            }
        }

        m_FinalShaderBuilder.append("};\n");
    }
}

const char *SShaderCodeGeneratorBase::BuildShaderSource()
{
    for (nvhash_set<CRegisteredString>::const_iterator iter = m_Includes.begin(),
                                                       end = m_Includes.end();
         iter != end; ++iter) {
        m_FinalShaderBuilder.append("#include \"");
        m_FinalShaderBuilder.append(iter->c_str());
        m_FinalShaderBuilder.append("\"\n");
    }
    AddShaderItemMap("attribute", m_Attributes);
    AddShaderItemMap("uniform", m_Uniforms);
    AddShaderConstantBufferItemMap("uniform", m_ConstantBuffers, m_ConstantBufferParams);
    AddShaderItemMap("varying", GetVaryings());
    m_FinalShaderBuilder.append("\n");
    m_FinalShaderBuilder.append(m_CodeBuilder);
    return m_FinalShaderBuilder.c_str();
}
SShaderCodeGeneratorBase &SShaderCodeGeneratorBase::operator<<(const char *data)
{
    m_CodeBuilder.append(data);
    return *this;
}
SShaderCodeGeneratorBase &SShaderCodeGeneratorBase::operator<<(const TStrType &data)
{
    m_CodeBuilder.append(data);
    return *this;
}

SShaderCodeGeneratorBase &SShaderCodeGeneratorBase::operator<<(const SEndlType & /*data*/)
{
    m_CodeBuilder.append("\n");
    return *this;
}

SShaderVertexCodeGenerator::SShaderVertexCodeGenerator(IStringTable &inStringTable,
                                                       NVAllocatorCallback &alloc,
                                                       qt3ds::render::NVRenderContextType ctxType)
    : SShaderCodeGeneratorBase(inStringTable, alloc, ctxType)
    , m_Varyings(alloc, "SShaderVertexCodeGenerator::m_Varyings")
{
}
TStrTableStrMap &SShaderVertexCodeGenerator::GetVaryings()
{
    return m_Varyings;
}

SShaderTessControlCodeGenerator::SShaderTessControlCodeGenerator(
    SShaderVertexCodeGenerator &vert, NVAllocatorCallback &alloc,
    qt3ds::render::NVRenderContextType ctxType)
    : SShaderCodeGeneratorBase(vert.m_StringTable, alloc, ctxType)
    , m_VertGenerator(vert)
    , m_Varyings(alloc, "SShaderTessControlCodeGenerator::m_Varyings")
{
}

// overwritten from base
void SShaderTessControlCodeGenerator::AddShaderItemMap(const char *itemType,
                                                       const TStrTableStrMap &itemMap)
{
    eastl::string extVtx("");
    eastl::string extTC("");
    eastl::string type(itemType);
    if (!type.compare("varying")) {
        extVtx = "[]";
        extTC = "TC[]";
        itemType = "attribute";
    }

    m_FinalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end;
         ++iter) {
        m_FinalShaderBuilder.append(itemType);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->second);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->first);
        m_FinalShaderBuilder.append(extVtx.c_str());
        m_FinalShaderBuilder.append(";\n");
    }

    // if this is varyings write output of tess control shader
    if (!extVtx.empty()) {
        m_FinalShaderBuilder.append("\n");
        itemType = "varying";

        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end();
             iter != end; ++iter) {
            m_FinalShaderBuilder.append(itemType);
            m_FinalShaderBuilder.append(" ");
            m_FinalShaderBuilder.append(iter->second);
            m_FinalShaderBuilder.append(" ");
            m_FinalShaderBuilder.append(iter->first);
            m_FinalShaderBuilder.append(extTC.c_str());
            m_FinalShaderBuilder.append(";\n");
        }
    }
}
TStrTableStrMap &SShaderTessControlCodeGenerator::GetVaryings()
{
    return m_VertGenerator.m_Varyings;
}

SShaderTessEvalCodeGenerator::SShaderTessEvalCodeGenerator(SShaderTessControlCodeGenerator &tc,
                                                           NVAllocatorCallback &alloc,
                                                           qt3ds::render::NVRenderContextType ctxType)
    : SShaderCodeGeneratorBase(tc.m_StringTable, alloc, ctxType)
    , m_TessControlGenerator(tc)
    , m_hasGeometryStage(false)
{
}
// overwritten from base
void SShaderTessEvalCodeGenerator::AddShaderItemMap(const char *itemType,
                                                    const TStrTableStrMap &itemMap)
{
    eastl::string extTC("");
    eastl::string extTE("");
    eastl::string type(itemType);
    if (!type.compare("varying")) {
        extTC = "TC[]";
        itemType = "attribute";
    }
    if (m_hasGeometryStage) {
        extTE = "TE";
    }

    m_FinalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end;
         ++iter) {
        m_FinalShaderBuilder.append(itemType);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->second);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->first);
        m_FinalShaderBuilder.append(extTC.c_str());
        m_FinalShaderBuilder.append(";\n");
    }

    // if this are varyings write output of tess eval shader
    if (!extTC.empty()) {
        m_FinalShaderBuilder.append("\n");
        itemType = "varying";

        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end();
             iter != end; ++iter) {
            m_FinalShaderBuilder.append(itemType);
            m_FinalShaderBuilder.append(" ");
            m_FinalShaderBuilder.append(iter->second);
            m_FinalShaderBuilder.append(" ");
            m_FinalShaderBuilder.append(iter->first);
            m_FinalShaderBuilder.append(extTE.c_str());
            m_FinalShaderBuilder.append(";\n");
        }
    }
}
TStrTableStrMap &SShaderTessEvalCodeGenerator::GetVaryings()
{
    return m_TessControlGenerator.m_VertGenerator.GetVaryings();
}
void SShaderTessEvalCodeGenerator::SetGeometryStage(bool hasGeometryStage)
{
    m_hasGeometryStage = hasGeometryStage;
}

SShaderGeometryCodeGenerator::SShaderGeometryCodeGenerator(SShaderVertexCodeGenerator &vert,
                                                           NVAllocatorCallback &alloc,
                                                           qt3ds::render::NVRenderContextType ctxType)
    : SShaderCodeGeneratorBase(vert.m_StringTable, alloc, ctxType)
    , m_VertGenerator(vert)
    , m_hasTessellationStage(true)
{
}

// overwritten from base
void SShaderGeometryCodeGenerator::AddShaderItemMap(const char *itemType,
                                                    const TStrTableStrMap &itemMap)
{
    eastl::string inExt("");
    eastl::string type(itemType);
    if (!type.compare("varying")) {
        itemType = "attribute";
        if (m_hasTessellationStage)
            inExt = "TE[]";
        else
            inExt = "[]";
    }

    m_FinalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end;
         ++iter) {
        m_FinalShaderBuilder.append(itemType);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->second);
        m_FinalShaderBuilder.append(" ");
        m_FinalShaderBuilder.append(iter->first);
        m_FinalShaderBuilder.append(inExt.c_str());
        m_FinalShaderBuilder.append(";\n");
    }

    // if this are varyings write output of geometry shader
    if (!type.compare("varying")) {
        m_FinalShaderBuilder.append("\n");
        itemType = "varying";

        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end();
             iter != end; ++iter) {
            m_FinalShaderBuilder.append(itemType);
            m_FinalShaderBuilder.append(" ");
            m_FinalShaderBuilder.append(iter->second);
            m_FinalShaderBuilder.append(" ");
            m_FinalShaderBuilder.append(iter->first);
            m_FinalShaderBuilder.append(";\n");
        }
    }
}
TStrTableStrMap &SShaderGeometryCodeGenerator::GetVaryings()
{
    return m_VertGenerator.m_Varyings;
}
void SShaderGeometryCodeGenerator::SetTessellationStage(bool hasTessellationStage)
{
    m_hasTessellationStage = hasTessellationStage;
}

SShaderFragmentCodeGenerator::SShaderFragmentCodeGenerator(SShaderVertexCodeGenerator &vert,
                                                           NVAllocatorCallback &alloc,
                                                           qt3ds::render::NVRenderContextType ctxType)
    : SShaderCodeGeneratorBase(vert.m_StringTable, alloc, ctxType)
    , m_VertGenerator(vert)
{
}
TStrTableStrMap &SShaderFragmentCodeGenerator::GetVaryings()
{
    return m_VertGenerator.m_Varyings;
}
