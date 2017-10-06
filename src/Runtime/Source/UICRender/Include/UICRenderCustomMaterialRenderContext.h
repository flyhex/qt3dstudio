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
#ifndef UIC_RENDER_CUSTOM_MATERIAL_RENDER_CONTEXT_H
#define UIC_RENDER_CUSTOM_MATERIAL_RENDER_CONTEXT_H
#include "UICRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSMat44.h"
#include "foundation/Qt3DSMat33.h"
#include "UICRenderShaderKeys.h"

namespace uic {
namespace render {

    struct SLayerRenderData;

    struct SCustomMaterialRenderContext
    {
        // The lights and camera will not change per layer,
        // so that information can be set once for all the shaders.
        const SLayer &m_Layer;
        const SLayerRenderData &m_LayerData;
        NVDataRef<SLight *> m_Lights;
        const SCamera &m_Camera;

        // Per-object information.
        const SModel &m_Model;
        const SRenderSubset &m_Subset;
        const QT3DSMat44 &m_ModelViewProjection;
        const QT3DSMat44 &m_ModelMatrix; ///< model to world transformation
        const QT3DSMat33 &m_NormalMatrix;
        const SCustomMaterial &m_Material;
        const NVRenderTexture2D *m_DepthTexture;
        const NVRenderTexture2D *m_AOTexture;
        SShaderDefaultMaterialKey m_MaterialKey;
        SRenderableImage *m_FirstImage;

        SCustomMaterialRenderContext(
            const SLayer &layer, const SLayerRenderData &data, NVDataRef<SLight *> lights,
            const SCamera &cam, const SModel &m, const SRenderSubset &subset, const QT3DSMat44 &mvp,
            const QT3DSMat44 &world, const QT3DSMat33 &nm, const SCustomMaterial &material,
            const NVRenderTexture2D *depthTex, const NVRenderTexture2D *aoTex,
            SShaderDefaultMaterialKey inMaterialKey, SRenderableImage *inFirstImage = NULL)
            : m_Layer(layer)
            , m_LayerData(data)
            , m_Lights(lights)
            , m_Camera(cam)
            , m_Model(m)
            , m_Subset(subset)
            , m_ModelViewProjection(mvp)
            , m_ModelMatrix(world)
            , m_NormalMatrix(nm)
            , m_Material(material)
            , m_DepthTexture(depthTex)
            , m_AOTexture(aoTex)
            , m_MaterialKey(inMaterialKey)
            , m_FirstImage(inFirstImage)
        {
        }
    };
}
}

#endif
