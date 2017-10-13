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
#include "UICDMHandles.h"
namespace qt3ds {
namespace render {
    class IPathManager;
}
}

namespace UICDM {
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
    virtual void SerializeScene(UICDM::IDOMWriter &inWriter) = 0;
    // Write properties into the active slide until we get to a slide owner, then create new slides.
    // inActiveSlide may be zero if the top item we find happens to be a scene or a component.
    // The graph roots hold the top items
    virtual void SerializeScene(UICDM::IDOMReader &inReader,
                                const Q3DStudio::CFilePath &inDocumentDirectory,
                                int inUIPVersion) = 0;

    // Write this instance and its children (and possibly its slides) to a writer.
    // Equivalent to the older partial serialization system
    virtual void SerializeSceneGraphObjects(UICDM::IDOMWriter &inWriter,
                                            const UICDM::TInstanceHandleList &inInstances,
                                            UICDM::CUICDMSlideHandle inActiveSlide) = 0;

    // Read a partial serialization into this slide, attaching the instance as the last child of the
    // new root.
    virtual UICDM::TInstanceHandleList SerializeSceneGraphObject(
        UICDM::IDOMReader &inReader, const Q3DStudio::CFilePath &inDocumentDirectory,
        UICDM::CUICDMInstanceHandle inNewRoot, UICDM::CUICDMSlideHandle inActiveSlide) = 0;

    // Save and load just a single action
    virtual void SerializeAction(UICDM::IDOMWriter &inWriter, UICDM::CUICDMSlideHandle inSlide,
                                 UICDM::CUICDMActionHandle inAction) = 0;
    // Load a new action onto this root object
    virtual UICDM::CUICDMActionHandle SerializeAction(UICDM::IDOMReader &inReader,
                                                      UICDM::CUICDMInstanceHandle inNewRoot,
                                                      UICDM::CUICDMSlideHandle inSlide) = 0;

    virtual void SerializeSlide(UICDM::IDOMWriter &inWriter, UICDM::CUICDMSlideHandle inSlide) = 0;

    virtual UICDM::CUICDMSlideHandle SerializeSlide(UICDM::IDOMReader &inReader,
                                                    const CFilePath &inDocumentDirectory,
                                                    UICDM::CUICDMSlideHandle inMaster,
                                                    int inNewIndex = -1) = 0;

    friend class std::shared_ptr<IComposerSerializer>;

    static std::shared_ptr<IComposerSerializer> CreateGraphSlideSerializer(
        UICDM::IDataCore &inDataCore, UICDM::IMetaData &inMetaData, UICDM::ISlideCore &inSlideCore,
        UICDM::IAnimationCore &inAnimationCore, UICDM::IActionCore &inActionCore,
        CGraph &inAssetGraph, UICDM::ISlideSystem &inSlideSystem,
        UICDM::IActionSystem &inActionSystem, UICDM::ISlideGraphCore &inSlideGraphCore,
        UICDM::SComposerObjectDefinitions &inObjectDefinitions,
        std::shared_ptr<Q3DStudio::IImportFailedHandler> inFailedHandler,
        UICDM::IGuideSystem &inGuideSystem, qt3ds::render::IPathManager &inPathManager);
};
}

#endif
