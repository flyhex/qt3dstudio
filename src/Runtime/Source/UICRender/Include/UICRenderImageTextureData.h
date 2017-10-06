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
#ifndef UIC_RENDER_IMAGE_TEXTURE_DATA_H
#define UIC_RENDER_IMAGE_TEXTURE_DATA_H
#include "UICRender.h"
#include "foundation/Qt3DSFlags.h"

namespace uic {
namespace render {

    // forward declararion
    class UICRenderPrefilterTexture;

    struct ImageTextureFlagValues
    {
        enum Enum {
            HasTransparency = 1,
            InvertUVCoords = 1 << 1,
            PreMultiplied = 1 << 2,
        };
    };

    struct SImageTextureFlags : public NVFlags<ImageTextureFlagValues::Enum, QT3DSU32>
    {
        bool HasTransparency() const
        {
            return this->operator&(ImageTextureFlagValues::HasTransparency);
        }
        void SetHasTransparency(bool inValue)
        {
            clearOrSet(inValue, ImageTextureFlagValues::HasTransparency);
        }

        bool IsInvertUVCoords() const
        {
            return this->operator&(ImageTextureFlagValues::InvertUVCoords);
        }
        void SetInvertUVCoords(bool inValue)
        {
            clearOrSet(inValue, ImageTextureFlagValues::InvertUVCoords);
        }

        bool IsPreMultiplied() const
        {
            return this->operator&(ImageTextureFlagValues::PreMultiplied);
        }
        void SetPreMultiplied(bool inValue)
        {
            clearOrSet(inValue, ImageTextureFlagValues::PreMultiplied);
        }
    };

    struct SImageTextureData
    {
        NVRenderTexture2D *m_Texture;
        SImageTextureFlags m_TextureFlags;
        UICRenderPrefilterTexture *m_BSDFMipMap;

        SImageTextureData()
            : m_Texture(NULL)
            , m_BSDFMipMap(NULL)
        {
        }

        bool operator!=(const SImageTextureData &inOther)
        {
            return m_Texture != inOther.m_Texture || m_TextureFlags != inOther.m_TextureFlags
                || m_BSDFMipMap != inOther.m_BSDFMipMap;
        }
    };
}
}

#endif