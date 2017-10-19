/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
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
#pragma once
#ifndef COMPOSERSERIALIZERH
#define COMPOSERSERIALIZERH
#include "Qt3DSDMHandles.h"
namespace qt3ds {
namespace render {
    class IPathManager;
}
}

namespace qt3dsdm {
class IDataCore;
class IMetaData;
class ISlideCore;
class IAnimationCore;
class IActionCore;
class IDOMWriter;
class IDOMReader;
class ISlideSystem;
class IActionSystem;
class ISlideGraphCore;
class IGuideSystem;
class SComposerObjectDefinitions;
};

namespace Q3DStudio {
class CGraph;
class CFilePath;
class IImportFailedHandler;

// Serialize instance data on the data core, in the slides (including actions)
// and in the animation system
class IComposerSerializer
{
protected:
    virtual ~IComposerSerializer() {}
public:
    // Empty graph roots means use the actual graph roots in the asset graph
    virtual void SerializeScene(qt3dsdm::IDOMWriter &inWriter) = 0;
    // Write properties into the active slide until we get to a slide owner, then create new slides.
    // inActiveSlide may be zero if the top item we find happens to be a scene or a component.
    // The graph roots hold the top items
    virtual void SerializeScene(qt3dsdm::IDOMReader &inReader,
                                const Q3DStudio::CFilePath &inDocumentDirectory,
                                int inUIPVersion) = 0;

    // Write this instance and its children (and possibly its slides) to a writer.
    // Equivalent to the older partial serialization system
    virtual void SerializeSceneGraphObjects(qt3dsdm::IDOMWriter &inWriter,
                                            const qt3dsdm::TInstanceHandleList &inInstances,
                                            qt3dsdm::Qt3DSDMSlideHandle inActiveSlide) = 0;

    // Read a partial serialization into this slide, attaching the instance as the last child of the
    // new root.
    virtual qt3dsdm::TInstanceHandleList SerializeSceneGraphObject(
        qt3dsdm::IDOMReader &inReader, const Q3DStudio::CFilePath &inDocumentDirectory,
        qt3dsdm::Qt3DSDMInstanceHandle inNewRoot, qt3dsdm::Qt3DSDMSlideHandle inActiveSlide) = 0;

    // Save and load just a single action
    virtual void SerializeAction(qt3dsdm::IDOMWriter &inWriter, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                 qt3dsdm::Qt3DSDMActionHandle inAction) = 0;
    // Load a new action onto this root object
    virtual qt3dsdm::Qt3DSDMActionHandle SerializeAction(qt3dsdm::IDOMReader &inReader,
                                                      qt3dsdm::Qt3DSDMInstanceHandle inNewRoot,
                                                      qt3dsdm::Qt3DSDMSlideHandle inSlide) = 0;

    virtual void SerializeSlide(qt3dsdm::IDOMWriter &inWriter, qt3dsdm::Qt3DSDMSlideHandle inSlide) = 0;

    virtual qt3dsdm::Qt3DSDMSlideHandle SerializeSlide(qt3dsdm::IDOMReader &inReader,
                                                    const CFilePath &inDocumentDirectory,
                                                    qt3dsdm::Qt3DSDMSlideHandle inMaster,
                                                    int inNewIndex = -1) = 0;

    friend class std::shared_ptr<IComposerSerializer>;

    static std::shared_ptr<IComposerSerializer> CreateGraphSlideSerializer(
        qt3dsdm::IDataCore &inDataCore, qt3dsdm::IMetaData &inMetaData, qt3dsdm::ISlideCore &inSlideCore,
        qt3dsdm::IAnimationCore &inAnimationCore, qt3dsdm::IActionCore &inActionCore,
        CGraph &inAssetGraph, qt3dsdm::ISlideSystem &inSlideSystem,
        qt3dsdm::IActionSystem &inActionSystem, qt3dsdm::ISlideGraphCore &inSlideGraphCore,
        qt3dsdm::SComposerObjectDefinitions &inObjectDefinitions,
        std::shared_ptr<Q3DStudio::IImportFailedHandler> inFailedHandler,
        qt3dsdm::IGuideSystem &inGuideSystem, qt3ds::render::IPathManager &inPathManager);
};
}

#endif
