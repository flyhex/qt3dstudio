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
#ifndef UIC_RENDER_MODEL_H
#define UIC_RENDER_MODEL_H

#include "UICRenderNode.h"
#include "foundation/StringTable.h"
#include "UICRenderTessModeValues.h"

namespace uic {
namespace render {

    struct SDefaultMaterial;
    class IBufferManager;

    struct SModel : public SNode
    {
        // Complete path to the file;
        //*not* relative to the presentation directory
        CRegisteredString m_MeshPath;
        SGraphObject *m_FirstMaterial;
        QT3DSI32 m_SkeletonRoot;
        TessModeValues::Enum m_TessellationMode;
        QT3DSF32 m_EdgeTess;
        QT3DSF32 m_InnerTess;
        bool m_WireframeMode;

        SModel();

        void AddMaterial(SGraphObject &inMaterial);

        NVBounds3 GetModelBounds(IBufferManager &inManager) const;

        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SNode::Remap(inRemapper);
            inRemapper.RemapMaterial(m_FirstMaterial);
            inRemapper.Remap(m_MeshPath);
        }
    };
}
}

#endif