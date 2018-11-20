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

#ifndef Q3DS_IMAGE_TEXTURE_DATA_H
#define Q3DS_IMAGE_TEXTURE_DATA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "q3dsimagemanager_p.h"

/*
    This class replaces Qt3DSRenderImageTextureData

    // forward declararion
    class Qt3DSRenderPrefilterTexture;

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
        Qt3DSRenderPrefilterTexture *m_BSDFMipMap;

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
*/

namespace Q3DStudio {

struct Q3DSImageTextureData
{
    Q3DSImageTextureData() {}
    uint m_width = 0;
    uint m_height = 0;
    bool m_valid = false;
    bool m_hasTransparency = false;
    bool m_premultiplied = false;
};

} // namespace Q3DStudio

#endif
