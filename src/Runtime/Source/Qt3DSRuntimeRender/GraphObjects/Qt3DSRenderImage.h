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
#ifndef QT3DS_RENDER_IMAGE_H
#define QT3DS_RENDER_IMAGE_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderGraphObject.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "Qt3DSRenderNode.h"
#include "foundation/Qt3DSVec2.h"
#include "Qt3DSRenderImageTextureData.h"
#include "EASTL/utility.h"

namespace qt3ds {
namespace render {
    class IUICRenderContext;
    class IOffscreenRenderManager;
    class IOffscreenRenderer;
    struct ImageMappingModes
    {
        enum Enum {
            Normal = 0, // UV mapping
            Environment = 1,
            LightProbe = 2,
        };
    };

    struct SImage : public SGraphObject
    {
        // Complete path to the file;
        //*not* relative to the presentation directory
        CRegisteredString m_ImagePath;
        CRegisteredString m_ImageShaderName; ///< for custom materials we don't generate the name

        // Presentation id.
        CRegisteredString m_OffscreenRendererId; // overrides source path if available
        SRenderPlugin *m_RenderPlugin; // Overrides everything if available.
        IOffscreenRenderer *m_LastFrameOffscreenRenderer;
        SGraphObject *m_Parent;

        SImageTextureData m_TextureData;

        NodeFlags m_Flags; // only dirty, transform dirty, and active apply

        QT3DSVec2 m_Scale;
        QT3DSVec2 m_Pivot;
        QT3DSF32 m_Rotation; // Radians.
        QT3DSVec2 m_Position;
        ImageMappingModes::Enum m_MappingMode;
        NVRenderTextureCoordOp::Enum m_HorizontalTilingMode;
        NVRenderTextureCoordOp::Enum m_VerticalTilingMode;

        // Setting any of the above variables means this object is dirty.
        // Setting any of the vec2 properties means this object's transform is dirty

        QT3DSMat44 m_TextureTransform;

        SImage();
        // Renders the sub presentation
        // Or finds the image.
        // and sets up the texture transform
        bool ClearDirty(IBufferManager &inBufferManager, IOffscreenRenderManager &inRenderManager,
                        IRenderPluginManager &pluginManager, bool forIbl = false);

        void CalculateTextureTransform();

        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SGraphObject::Remap(inRemapper);
            inRemapper.Remap(m_ImagePath);
            inRemapper.Remap(m_OffscreenRendererId);
            // Null out objects that should be null when loading from file.
            inRemapper.NullPtr(m_LastFrameOffscreenRenderer);
            inRemapper.NullPtr(m_TextureData.m_Texture);
            inRemapper.Remap(m_RenderPlugin);
            inRemapper.Remap(m_Parent);
        }
    };
}
}

#endif
