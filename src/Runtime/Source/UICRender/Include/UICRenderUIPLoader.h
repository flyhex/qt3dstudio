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
#ifndef UIC_RENDER_UIP_LOADER_H
#define UIC_RENDER_UIP_LOADER_H

#ifdef UIC_RENDER_ENABLE_LOAD_UIP

#include "UICRender.h"
#include "foundation/StringTable.h"
#include <EASTL/utility.h>
#include "foundation/Qt3DSContainers.h"
#include "UICRenderGraphObject.h"

namespace Q3DStudio {
class IRuntimeMetaData;
}

namespace qt3dsdm {
class IDOMReader;
struct SMetaDataEffect;
struct SMetaDataCustomMaterial;
}

namespace qt3ds {
namespace render {

    class IBufferManager;

    typedef nvhash_map<CRegisteredString, SGraphObject *> TIdObjectMap;

    struct IUIPReferenceResolver
    {
    protected:
        virtual ~IUIPReferenceResolver() {}
    public:
        virtual CRegisteredString ResolveReference(CRegisteredString inStart,
                                                   const char *inReference) = 0;
    };

    struct SPresentation;

    class IUIPLoader
    {
    public:
        // The reader needs to point to the top of the file, we will search
        // several objects that exist at the top level of the uip file.
        // Returns NULL if we were incapable of loading the presentation.
        static SPresentation *
        LoadUIPFile(qt3dsdm::IDOMReader &inReader
                    // the full path, including the filename
                    // to the presentation file
                    ,
                    const char8_t *inFullPathToPresentationFile,
                    Q3DStudio::IRuntimeMetaData &inMetaData, IStringTable &inStrTable,
                    NVFoundationBase &inFoundation
                    // Allocator used for the presentation objects themselves
                    // this allows clients to pre-allocate a block of memory just for
                    // the scene graph
                    ,
                    NVAllocatorCallback &inPresentationAllocator
                    // Map of string ids to objects
                    ,
                    TIdObjectMap &ioObjectMap
                    // Buffer manager to load details about the images
                    ,
                    IBufferManager &inBufferManager
                    // To load effects we need the effect system
                    // and the presentation directory
                    ,
                    IEffectSystem &inEffectSystem, const char8_t *inPresentationDir,
                    IRenderPluginManager &inPluginManager, ICustomMaterialSystem &inMaterialSystem,
                    IDynamicObjectSystem &inDynamicSystem, qt3ds::render::IPathManager &inPathManager
                    // Resolve references to objects; this is done by the main uip loader during
                    // its normal mode of operation so we try to reuse that code.
                    ,
                    IUIPReferenceResolver *inResolver
                    // Set some initial values by going to the master slide then slide 1
                    // Useful for quick testing, sort of equivalent to showing the first frame
                    // of a given presentation
                    ,
                    bool setValuesFromSlides = false);

        static void CreateEffectClassFromMetaEffect(CRegisteredString inEffectName,
                                                    NVFoundationBase &inFoundation,
                                                    IEffectSystem &inEffectSystem,
                                                    const qt3dsdm::SMetaDataEffect &inMetaDataEffect,
                                                    IStringTable &inStrTable);

        static void CreateMaterialClassFromMetaMaterial(
            CRegisteredString inEffectName, NVFoundationBase &inFoundation,
            ICustomMaterialSystem &inEffectSystem,
            const qt3dsdm::SMetaDataCustomMaterial &inMetaDataMaterial, IStringTable &inStrTable);
    };
}
}

#endif
#endif
