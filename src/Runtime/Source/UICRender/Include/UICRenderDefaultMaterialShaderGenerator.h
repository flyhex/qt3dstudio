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
#ifndef UIC_RENDER_DEFAULT_MATERIAL_SHADER_GENERATOR_H
#define UIC_RENDER_DEFAULT_MATERIAL_SHADER_GENERATOR_H
#include "UICRenderMaterialShaderGenerator.h"

namespace uic {
namespace render {

    class UICShadowMap;
    struct SShaderGeneratorGeneratedShader;
    struct SLightConstantProperties;

    class IDefaultMaterialVertexPipeline : public IShaderStageGenerator
    {
    protected:
        virtual ~IDefaultMaterialVertexPipeline() {}
    public:
        // Responsible for beginning all vertex and fragment generation (void main() { etc).
        virtual void BeginVertexGeneration(QT3DSU32 displacementImageIdx,
                                           SRenderableImage *displacementImage) = 0;
        // The fragment shader expects a floating point constant, object_opacity to be defined
        // post this method.
        virtual void BeginFragmentGeneration() = 0;
        // Output variables may be mangled in some circumstances so the shader generation system
        // needs an abstraction
        // mechanism around this.
        virtual void AssignOutput(const char8_t *inVarName, const char8_t *inVarValueExpr) = 0;

        /**
         * @brief Generates UV coordinates in shader code
         *
         * @param[in] inUVSet		index of UV data set
         *
         * @return no return
         */
        virtual void GenerateUVCoords(QT3DSU32 inUVSet = 0) = 0;

        virtual void GenerateEnvMapReflection() = 0;
        virtual void GenerateViewVector() = 0;

        // fragment shader expects varying vertex normal
        // lighting in vertex pipeline expects world_normal
        virtual void GenerateWorldNormal() = 0; // world_normal in both vert and frag shader
        virtual void GenerateObjectNormal() = 0; // object_normal in both vert and frag shader
        virtual void
        GenerateWorldPosition() = 0; // model_world_position in both vert and frag shader
        virtual void GenerateVarTangentAndBinormal() = 0;

        virtual bool HasActiveWireframe() = 0; // varEdgeDistance is a valid entity

        // responsible for closing all vertex and fragment generation
        virtual void EndVertexGeneration() = 0;
        virtual void EndFragmentGeneration() = 0;
    };

    class IDefaultMaterialShaderGenerator : public IMaterialShaderGenerator
    {
    public:
        virtual void AddDisplacementImageUniforms(IShaderStageGenerator &inGenerator,
                                                  QT3DSU32 displacementImageIdx,
                                                  SRenderableImage *displacementImage) = 0;
        SImageVariableNames GetImageVariableNames(QT3DSU32 inIdx) override = 0;
        void GenerateImageUVCoordinates(IShaderStageGenerator &inVertexPipeline, QT3DSU32 idx,
                                                QT3DSU32 uvSet, SRenderableImage &image) override = 0;
        // Transforms attr_pos, attr_norm, and attr_uv0.
        virtual void AddDisplacementMappingForDepthPass(IShaderStageGenerator &inShader) = 0;

        // inPipelineName needs to be unique else the shader cache will just return shaders from
        // different pipelines.
        NVRenderShaderProgram *GenerateShader(
            const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
            IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
            NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage, bool inHasTransparency,
            const char8_t *inVertexPipelineName, const char8_t *inCustomMaterialName = "") override = 0;

        // Also sets the blend function on the render context.
        virtual void
        SetMaterialProperties(NVRenderShaderProgram &inProgram, const SGraphObject &inMaterial,
                              const QT3DSVec2 &inCameraVec, const QT3DSMat44 &inModelViewProjection,
                              const QT3DSMat33 &inNormalMatrix, const QT3DSMat44 &inGlobalTransform,
                              SRenderableImage *inFirstImage, QT3DSF32 inOpacity,
                              SLayerGlobalRenderProperties inRenderProperties) override = 0;

        static IDefaultMaterialShaderGenerator &
        CreateDefaultMaterialShaderGenerator(IUICRenderContext &inRenderContext);

        SLightConstantProperties *GetLightConstantProperties(SShaderGeneratorGeneratedShader &shader);
    };
}
}

#endif
