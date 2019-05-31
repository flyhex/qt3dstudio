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
#pragma once
#ifndef QT3DS_RENDER_SHADER_CODE_GENERATOR_H
#define QT3DS_RENDER_SHADER_CODE_GENERATOR_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSContainers.h"
#include "EASTL/string.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "StringTools.h"

namespace qt3ds {
namespace render {

    struct SEndlType
    {
    };
    extern SEndlType Endl;

    typedef std::basic_string<char> TStrType;
    typedef eastl::pair<CRegisteredString, CRegisteredString> TParamPair;
    typedef eastl::pair<CRegisteredString, TParamPair> TConstantBufferParamPair;
    typedef nvvector<TConstantBufferParamPair> TConstantBufferParamArray;
    typedef nvhash_map<CRegisteredString, CRegisteredString> TStrTableStrMap;

    struct SShaderCodeGeneratorBase
    {
        enum Enum {
            Unknown = 0,
            Lighting,
            ViewVector,
            WorldNormal,
            WorldPosition,
            EnvMapReflection,
            UVCoords,
        };
        IStringTable &m_StringTable;
        nvhash_set<QT3DSU32> m_Codes; // set of enums we have included.
        nvhash_set<CRegisteredString> m_Includes;
        TStrTableStrMap m_Uniforms;
        TStrTableStrMap m_ConstantBuffers;
        TConstantBufferParamArray m_ConstantBufferParams;
        TStrTableStrMap m_Attributes;
        Qt3DSString m_FinalShaderBuilder;
        TStrType m_CodeBuilder;
        qt3ds::render::NVRenderContextType m_RenderContextType;

        SShaderCodeGeneratorBase(IStringTable &inStringTable, NVAllocatorCallback &alloc,
                                 qt3ds::render::NVRenderContextType ctxType);
        virtual TStrTableStrMap &GetVaryings() = 0;
        void Begin();
        void Append(const char *data);
        // don't add the newline
        void AppendPartial(const char *data);
        void AddConstantBuffer(const char *name, const char *layout);
        void AddConstantBufferParam(const char *cbName, const char *paramName, const char *type);
        void AddUniform(const char *name, const char *type);
        void AddUniform(TStrType &name, const char *type);
        void AddAttribute(const char *name, const char *type);
        void AddAttribute(TStrType &name, const char *type);
        void AddVarying(const char *name, const char *type);
        void AddVarying(TStrType &name, const char *type);
        void AddLocalVariable(const char *name, const char *type, int tabCount = 1);
        void AddLocalVariable(TStrType &name, const char *type, int tabCount = 1);
        void AddInclude(const char *name);
        void AddInclude(TStrType &name);
        bool HasCode(Enum value);
        void SetCode(Enum value);
        void SetupWorldPosition();
        void GenerateViewVector();
        void GenerateWorldNormal();
        void GenerateEnvMapReflection(SShaderCodeGeneratorBase &inFragmentShader);
        void GenerateUVCoords();
        void GenerateTextureSwizzle(NVRenderTextureSwizzleMode::Enum swizzleMode,
                                    eastl::basic_string<char8_t> &texSwizzle,
                                    eastl::basic_string<char8_t> &lookupSwizzle);
        void GenerateShadedWireframeBase();
        void AddLighting();
        const char *BuildShaderSource();
        SShaderCodeGeneratorBase &operator<<(const char *data);
        SShaderCodeGeneratorBase &operator<<(const TStrType &data);
        SShaderCodeGeneratorBase &operator<<(const SEndlType & /*data*/);

    protected:
        virtual void AddShaderItemMap(const char *itemType, const TStrTableStrMap &itemMap);
        void AddShaderConstantBufferItemMap(const char *itemType, const TStrTableStrMap &cbMap,
                                            TConstantBufferParamArray cbParamsArray);
    };

    struct SShaderVertexCodeGenerator : public SShaderCodeGeneratorBase
    {
        TStrTableStrMap m_Varyings;
        SShaderVertexCodeGenerator(IStringTable &inStringTable, NVAllocatorCallback &alloc,
                                   qt3ds::render::NVRenderContextType ctxType);
        TStrTableStrMap &GetVaryings() override;
    };

    struct SShaderTessControlCodeGenerator : public SShaderCodeGeneratorBase
    {
        SShaderVertexCodeGenerator &m_VertGenerator;
        TStrTableStrMap m_Varyings;
        SShaderTessControlCodeGenerator(SShaderVertexCodeGenerator &vert,
                                        NVAllocatorCallback &alloc,
                                        qt3ds::render::NVRenderContextType ctxType);

        void AddShaderItemMap(const char *itemType, const TStrTableStrMap &itemMap) override;
        TStrTableStrMap &GetVaryings() override;
    };

    struct SShaderTessEvalCodeGenerator : public SShaderCodeGeneratorBase
    {
        SShaderTessControlCodeGenerator &m_TessControlGenerator;
        bool m_hasGeometryStage;

        SShaderTessEvalCodeGenerator(SShaderTessControlCodeGenerator &tc,
                                     NVAllocatorCallback &alloc,
                                     qt3ds::render::NVRenderContextType ctxType);

        void AddShaderItemMap(const char *itemType, const TStrTableStrMap &itemMap) override;
        TStrTableStrMap &GetVaryings() override;
        virtual void SetGeometryStage(bool hasGeometryStage);
    };

    struct SShaderGeometryCodeGenerator : public SShaderCodeGeneratorBase
    {
        SShaderVertexCodeGenerator &m_VertGenerator;
        bool m_hasTessellationStage;

        SShaderGeometryCodeGenerator(SShaderVertexCodeGenerator &vert, NVAllocatorCallback &alloc,
                                     qt3ds::render::NVRenderContextType ctxType);

        void AddShaderItemMap(const char *itemType, const TStrTableStrMap &itemMap) override;
        TStrTableStrMap &GetVaryings() override;
        virtual void SetTessellationStage(bool hasTessellationStage);
    };

    struct SShaderFragmentCodeGenerator : public SShaderCodeGeneratorBase
    {
        SShaderVertexCodeGenerator &m_VertGenerator;
        SShaderFragmentCodeGenerator(SShaderVertexCodeGenerator &vert, NVAllocatorCallback &alloc,
                                     qt3ds::render::NVRenderContextType ctxType);
        TStrTableStrMap &GetVaryings() override;
    };
}
}

#endif
