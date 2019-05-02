/****************************************************************************
**
** Copyright (C) 2008-2015 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_DEFAULT_MATERIAL_H
#define QT3DS_RENDER_DEFAULT_MATERIAL_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderGraphObject.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSVec3.h"
#include "Qt3DSRenderMaterialDirty.h"
#include "Qt3DSRenderLightmaps.h"

namespace qt3ds {
namespace render {
    struct DefaultMaterialLighting
    {
        enum Enum {
            NoLighting = 0,
            VertexLighting,
            FragmentLighting
        };
    };
    struct DefaultMaterialBlendMode
    {
        enum Enum {
            Normal = 0,
            Screen,
            Multiply,
            Overlay,
            ColorBurn,
            ColorDodge
        };
    };

    struct DefaultMaterialSpecularModel
    {
        enum Enum {
            Default = 0,
            KGGX,
            KWard
        };
    };

    struct SImage;

    struct QT3DS_AUTOTEST_EXPORT SDefaultMaterial : SGraphObject
    {
        CMaterialDirty m_Dirty;
        // lightmap section
        SLightmaps m_Lightmaps;
        // material section
        SImage *m_IblProbe;
        DefaultMaterialLighting::Enum m_Lighting; // defaults to vertex
        DefaultMaterialBlendMode::Enum m_BlendMode; // defaults to normal
        QT3DSVec4 m_DiffuseColor; // colors are 0-1 normalized
        SImage *m_DiffuseMaps[3];
        QT3DSF32 m_EmissivePower; // 0-100, defaults to 0
        QT3DSVec4 m_EmissiveColor;
        SImage *m_EmissiveMap;
        SImage *m_EmissiveMap2;
        SImage *m_SpecularReflection;
        SImage *m_SpecularMap;
        DefaultMaterialSpecularModel::Enum m_SpecularModel;
        QT3DSVec4 m_SpecularTint;
        QT3DSF32 m_IOR;
        QT3DSF32 m_FresnelPower;
        QT3DSF32 m_SpecularAmount; // 0-??, defaults to 0
        QT3DSF32 m_SpecularRoughness; // 0-??, defaults to 50
        SImage *m_RoughnessMap;
        QT3DSF32 m_Opacity; // 0-1
        SImage *m_OpacityMap;
        SImage *m_BumpMap;
        QT3DSF32 m_BumpAmount; // 0-??
        SImage *m_NormalMap;
        SImage *m_DisplacementMap;
        QT3DSF32 m_DisplaceAmount; // 0-??
        SImage *m_TranslucencyMap;
        QT3DSF32 m_TranslucentFalloff; // 0 - ??
        QT3DSF32 m_DiffuseLightWrap; // 0 - 1
        bool m_VertexColors;
        // Materials are stored as a linked list on models.
        SGraphObject *m_NextSibling;
        SModel *m_Parent;

        SDefaultMaterial();

        bool IsSpecularEnabled() const { return m_SpecularAmount > .01f; }
        bool IsFresnelEnabled() const { return m_FresnelPower > 0.0f; }
        bool IsVertexColorsEnabled() const { return m_VertexColors; }
        bool HasLighting() const { return m_Lighting != DefaultMaterialLighting::NoLighting; }

        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SGraphObject::Remap(inRemapper);
            m_Lightmaps.Remap(inRemapper);
            inRemapper.Remap(m_IblProbe);
            inRemapper.Remap(m_DiffuseMaps[0]);
            inRemapper.Remap(m_DiffuseMaps[1]);
            inRemapper.Remap(m_DiffuseMaps[2]);
            inRemapper.Remap(m_EmissiveMap);
            inRemapper.Remap(m_EmissiveMap2);
            inRemapper.Remap(m_SpecularReflection);
            inRemapper.Remap(m_SpecularMap);
            inRemapper.Remap(m_RoughnessMap);
            inRemapper.Remap(m_OpacityMap);
            inRemapper.Remap(m_BumpMap);
            inRemapper.Remap(m_NormalMap);
            inRemapper.Remap(m_DisplacementMap);
            inRemapper.Remap(m_TranslucencyMap);
            inRemapper.RemapMaterial(m_NextSibling);
            inRemapper.Remap(m_Parent);
        }
    };
}
}

#endif
