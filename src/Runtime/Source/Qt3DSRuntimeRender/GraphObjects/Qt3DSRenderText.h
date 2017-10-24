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
#ifndef UIC_RENDER_TEXT_H
#define UIC_RENDER_TEXT_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderNode.h"
#include "Qt3DSRenderTextTypes.h"

namespace qt3ds {
namespace render {

    struct SText : public SNode, public STextRenderInfo
    {
        // Change any of these properties and you can expect
        // that the text will force an expensive re-layer and render.
        // For these you need to set TextDirty.

        // These properties can change every frame with no additional cost.
        QT3DSVec3 m_TextColor;

        // Setup and utilized by the rendering system
        NVRenderTexture2D *m_TextTexture;
        STextTextureDetails m_TextTextureDetails;
        // used for nv path rendering
        NVRenderPathFontItem *m_PathFontItem;
        NVRenderPathFontSpecification *m_PathFontDetails;

        NVBounds3 m_Bounds;

        SText();

        NVBounds3 GetTextBounds() const;

        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SNode::Remap(inRemapper);
            inRemapper.Remap(m_Text);
            inRemapper.Remap(m_Font);
            inRemapper.NullPtr(m_TextTexture);
            inRemapper.NullPtr(m_PathFontItem);
            inRemapper.NullPtr(m_PathFontDetails);
        }
    };
}
}
#endif
