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
#ifndef QT3DS_RENDER_CUSTOM_MATERIAL_H
#define QT3DS_RENDER_CUSTOM_MATERIAL_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderDynamicObject.h"
#include "Qt3DSRenderImage.h"
#include "Qt3DSRenderLightmaps.h"
#include "foundation/Qt3DSFlags.h"

namespace qt3ds {
namespace render {

    // IMPORTANT: These flags matches the key produced by a MDL export file
    struct SCustomMaterialShaderKeyValues
    {
        enum Enum {
            diffuse = 1 << 0,
            specular = 1 << 1,
            glossy = 1 << 2,
            cutout = 1 << 3,
            refraction = 1 << 4,
            transparent = 1 << 5,
            displace = 1 << 6,
            volumetric = 1 << 7,
            transmissive = 1 << 8,
        };
    };

    typedef NVFlags<SCustomMaterialShaderKeyValues::Enum, QT3DSU32> SCustomMaterialShaderKeyFlags;

    struct SCustomMaterial : public SDynamicObject
    {
    private:
        // These objects are only created via the dynamic object system.
        SCustomMaterial(const SCustomMaterial &);
        SCustomMaterial &operator=(const SCustomMaterial &);
        SCustomMaterial();

    public:
        // lightmap section
        SLightmaps m_Lightmaps;
        // material section
        bool m_hasTransparency;
        bool m_hasRefraction;
        bool m_hasVolumetricDF;
        SImage *m_IblProbe;
        SImage *m_EmissiveMap2;
        SImage *m_DisplacementMap;
        QT3DSF32 m_DisplaceAmount; ///< depends on the object size

        SGraphObject *m_NextSibling;

        SCustomMaterialShaderKeyFlags m_ShaderKeyValues; ///< input from MDL files
        QT3DSU32 m_LayerCount; ///< input from MDL files

        void Initialize(QT3DSU32 inKey, QT3DSU32 inLayerCount)
        {
            m_Lightmaps.m_LightmapIndirect = NULL;
            m_Lightmaps.m_LightmapRadiosity = NULL;
            m_Lightmaps.m_LightmapShadow = NULL;
            m_hasTransparency = false;
            m_hasRefraction = false;
            m_hasVolumetricDF = false;
            m_NextSibling = NULL;
            m_DirtyFlagWithInFrame = m_Flags.IsDirty();
            m_IblProbe = NULL;
            m_EmissiveMap2 = NULL;
            m_DisplacementMap = NULL;
            m_DisplaceAmount = 0.0;
            m_ShaderKeyValues = (SCustomMaterialShaderKeyFlags)inKey;
            m_LayerCount = inLayerCount;
        }

        bool IsDielectric() const
        {
            return m_ShaderKeyValues & SCustomMaterialShaderKeyValues::diffuse;
        }
        bool IsSpecularEnabled() const
        {
            return m_ShaderKeyValues & SCustomMaterialShaderKeyValues::specular;
        }
        bool IsCutOutEnabled() const
        {
            return m_ShaderKeyValues & SCustomMaterialShaderKeyValues::cutout;
        }
        bool IsVolumetric() const
        {
            return m_ShaderKeyValues & SCustomMaterialShaderKeyValues::volumetric;
        }
        bool IsTransmissive() const
        {
            return m_ShaderKeyValues & SCustomMaterialShaderKeyValues::transmissive;
        }
        bool HasLighting() const { return true; }

        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SDynamicObject::Remap(inRemapper);
            m_Lightmaps.Remap(inRemapper);
            inRemapper.Remap(m_IblProbe);
            inRemapper.RemapMaterial(m_NextSibling);
            inRemapper.Remap(m_EmissiveMap2);
            inRemapper.Remap(m_DisplacementMap);
        }

        // Dirty
        bool m_DirtyFlagWithInFrame;
        bool IsDirty() const { return m_Flags.IsDirty() || m_DirtyFlagWithInFrame; }
        void UpdateDirtyForFrame()
        {
            m_DirtyFlagWithInFrame = m_Flags.IsDirty();
            m_Flags.SetDirty(false);
        }
    };
}
}
#endif
