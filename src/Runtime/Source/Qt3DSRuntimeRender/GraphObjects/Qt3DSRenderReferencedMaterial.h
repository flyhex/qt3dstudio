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
#ifndef UIC_RENDER_REFERENCED_MATERIAL_H
#define UIC_RENDER_REFERENCED_MATERIAL_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderGraphObject.h"
#include "Qt3DSRenderMaterialDirty.h"

namespace qt3ds {
namespace render {

    struct SReferencedMaterial : SGraphObject
    {
        CMaterialDirty m_Dirty;
        SGraphObject *m_ReferencedMaterial;
        SGraphObject *m_NextSibling;
        SReferencedMaterial()
            : SGraphObject(GraphObjectTypes::ReferencedMaterial)
            , m_ReferencedMaterial(NULL)
            , m_NextSibling(NULL)
        {
        }

        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SGraphObject::Remap(inRemapper);
            inRemapper.RemapMaterial(m_ReferencedMaterial);
            inRemapper.RemapMaterial(m_NextSibling);
        }
    };
}
}

#endif // UIC_RENDER_REFERENCED_MATERIAL_H
