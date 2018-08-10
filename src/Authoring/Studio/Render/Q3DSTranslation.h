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

#ifndef Q3DS_TRANSLATION_H
#define Q3DS_TRANSLATION_H

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

#include "Doc.h"

#include "q3dsruntime2api_p.h"
#include "Qt3DSDMMetaData.h"
#include "IStudioRenderer.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "Q3DSGraphObjectTranslator.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "foundation/Qt3DSOption.h"

/* This class replace STranslation */

#if 0
struct STranslation : public qt3ds::render::IQt3DSRenderNodeFilter
{
    typedef eastl::pair<qt3dsdm::Qt3DSDMInstanceHandle, SGraphObjectTranslator *>
        THandleTranslatorPair;
    typedef eastl::vector<THandleTranslatorPair> THandleTranslatorPairList;
    // Now that we have aliases, one instance handle can map to several translators.  One
    // translator, however, only
    // maps to one instance handle.
    typedef nvhash_map<qt3dsdm::Qt3DSDMInstanceHandle, THandleTranslatorPairList, eastl::hash<int>>
        TInstanceToTranslatorMap;
    IStudioRenderer &m_Renderer;
    IQt3DSRenderContext &m_Context;
    CDoc &m_Doc;
    IDocumentReader &m_Reader;
    SComposerObjectDefinitions &m_ObjectDefinitions;
    qt3dsdm::CStudioSystem &m_StudioSystem;
    qt3dsdm::CStudioFullSystem &m_FullSystem;
    Q3DStudio::CGraph &m_AssetGraph;

    // allocator for scene graph and translators
    qt3ds::foundation::SSAutoDeallocatorAllocator m_Allocator;
    // All translator related containers must come after the allocator
    TInstanceToTranslatorMap m_TranslatorMap;
    TTranslatorDirtySet m_DirtySet;
    qt3ds::render::SPresentation m_Presentation;
    qt3ds::render::SScene *m_Scene;
    Q3DStudio::CGraphIterator m_GraphIterator;
    nvvector<TSignalConnection> m_SignalConnections;
    QT3DSI32 m_ComponentSecondsDepth;
    SNode m_MouseDownNode;
    SCamera m_MouseDownCamera;
    Option<QT3DSMat44> m_MouseDownParentGlobalTransformInverse;
    Option<QT3DSMat33> m_MouseDownParentRotationInverse;
    Option<QT3DSMat33> m_MouseDownGlobalRotation;
    QT3DSI32 m_KeyRepeat;
    bool m_EditCameraEnabled;
    bool m_EditLightEnabled;
    SEditCameraPersistentInformation m_EditCameraInfo;
    SCamera m_EditCamera;
    SLight m_EditLight;
    QT3DSVec2 m_Viewport;
    SEditCameraLayerTranslator *m_EditCameraLayerTranslator;
    Option<SZoomRender> m_ZoomRender;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_TranslationWidget;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_RotationWidget;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_ScaleWidget;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_LastRenderedWidget;
    NVScopedRefCounted<qt3ds::widgets::SGradientWidget> m_GradientWidget;
    NVScopedRefCounted<qt3ds::widgets::SVisualAidWidget> m_VisualAidWidget;

    NVScopedRefCounted<qt3ds::widgets::IPathWidget> m_PathWidget;
    NVScopedRefCounted<qt3ds::render::NVRenderTexture2D> m_PickBuffer;
    Option<SPathAnchorDragInitialValue> m_LastPathDragValue;
    nvvector<qt3ds::QT3DSU8> m_PixelBuffer;
    nvvector<SGraphObjectTranslator *> m_editModeCamerasAndLights;
    QT3DSF32 m_CumulativeRotation;
    eastl::vector<qt3ds::render::SPGGraphObject *> m_GuideContainer;
    qt3ds::foundation::SFastAllocator<> m_GuideAllocator;
    // The rects are maintained from last render because the render context
    // doesn't guarantee the rects it returns are valid outside of begin/end render calls.
    SRulerRect m_OuterRect;
    SRulerRect m_InnerRect; // presentation rect.

    QT3DSVec4 m_rectColor;
    QT3DSVec4 m_lineColor;
    QT3DSVec4 m_guideColor;
    QT3DSVec4 m_selectedGuideColor;
    QT3DSVec4 m_guideFillColor;
    QT3DSVec4 m_selectedGuideFillColor;

    STranslation(IStudioRenderer &inRenderer, IQt3DSRenderContext &inContext);
#endif

namespace Q3DStudio
{
class Q3DStudioRenderer;
class Q3DSGraphObjectTranslator;
class Q3DSTranslation
{
public:
    Q3DSTranslation(Q3DStudioRenderer &inRenderer);

protected:
    void markDirty(qt3dsdm::Qt3DSDMInstanceHandle instance);
    void markPropertyDirty(qt3dsdm::Qt3DSDMInstanceHandle instance,
                           qt3dsdm::Qt3DSDMPropertyHandle property);
    void releaseTranslation(qt3dsdm::Qt3DSDMInstanceHandle instance);
    void markGraphInstanceDirty(int instance, int parent);
    void markBeginComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle slide);
    void markComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle);

private:

    void setPresentationData();
    Q3DSGraphObjectTranslator *createTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance);
    Q3DSGraphObjectTranslator *getOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                          qt3dsdm::Qt3DSDMInstanceHandle aliasInstance);
    void clearDirtySet();
    QByteArray getInstanceObjectId(qt3dsdm::Qt3DSDMInstanceHandle instance);


    struct TranslatorGetDirty
    {
        quint32 operator()(const Q3DSGraphObjectTranslator &inTrans) const
        {
            return inTrans.dirtyIndex();
        }
    };
    struct TranslatorSetDirty
    {
        void operator()(Q3DSGraphObjectTranslator &inTrans, quint32 idx) const
        {
            inTrans.setDirtyIndex(idx);
        }
    };

    struct Q3DSPresentationData
    {
        QString m_id;
        QString m_srcPath;
        QString m_author;
        QString m_company;
        long m_width;
        long m_height;
        Q3DSPresentationData()
            : m_width(800)
            , m_height(480)
        {
        }
    };

    typedef std::shared_ptr<qt3dsdm::ISignalConnection> TSignalConnection;
    typedef qt3ds::foundation::InvasiveSet<Q3DSGraphObjectTranslator, TranslatorGetDirty,
                                           TranslatorSetDirty>
        TTranslatorDirtySet;

    typedef QPair<qt3dsdm::Qt3DSDMInstanceHandle, Q3DSGraphObjectTranslator *>
        THandleTranslatorPair;
    typedef QVector<THandleTranslatorPair> THandleTranslatorPairList;
    typedef QHash<qt3dsdm::Qt3DSDMInstanceHandle, THandleTranslatorPairList>
        TInstanceToTranslatorMap;
    /*
        Now that we have aliases, one instance handle can map to several translators.
        One translator, however, only maps to one instance handle.
    */
    typedef qt3ds::foundation::Option<Q3DSTranslation::THandleTranslatorPair>
        ThandleTranslatorOption;

    ThandleTranslatorOption findTranslator(THandleTranslatorPairList &list,
                                           qt3dsdm::Qt3DSDMInstanceHandle instance);

    Q3DStudioRenderer &m_studioRenderer;

    CDoc &m_doc;
    IDocumentReader &m_reader;
    qt3dsdm::SComposerObjectDefinitions &m_objectDefinitions;
    qt3dsdm::CStudioSystem &m_studioSystem;
    qt3dsdm::CStudioFullSystem &m_fullSystem;
    Q3DStudio::CGraph &m_assetGraph;
    QSharedPointer<Q3DSEngine> m_engine;
    QSharedPointer<Q3DSUipPresentation> m_presentation;

    // allocator for scene graph and translators
    //qt3ds::foundation::SSAutoDeallocatorAllocator m_Allocator;
    // All translator related containers must come after the allocator
    TInstanceToTranslatorMap m_translatorMap;
    TTranslatorDirtySet m_dirtySet;
    Q3DSPresentationData m_presentation_data;
    Q3DSScene *m_scene;
    Q3DStudio::CGraphIterator m_graphIterator;
    QVector<TSignalConnection> m_signalConnections;
    quint32 m_componentSecondsDepth;
    QVector<Q3DSGraphObjectTranslator*> m_slideTranslators;

#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
    SNode m_MouseDownNode;
    SCamera m_MouseDownCamera;
    Option<QT3DSMat44> m_MouseDownParentGlobalTransformInverse;
    Option<QT3DSMat33> m_MouseDownParentRotationInverse;
    Option<QT3DSMat33> m_MouseDownGlobalRotation;
    QT3DSI32 m_KeyRepeat;
    bool m_EditCameraEnabled;
    bool m_EditLightEnabled;
    SEditCameraPersistentInformation m_EditCameraInfo;
    SCamera m_EditCamera;
    SLight m_EditLight;
    QT3DSVec2 m_Viewport;
    SEditCameraLayerTranslator *m_EditCameraLayerTranslator;
    Option<SZoomRender> m_ZoomRender;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_TranslationWidget;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_RotationWidget;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_ScaleWidget;
    NVScopedRefCounted<qt3ds::widgets::IStudioWidget> m_LastRenderedWidget;
    NVScopedRefCounted<qt3ds::widgets::SGradientWidget> m_GradientWidget;
    NVScopedRefCounted<qt3ds::widgets::SVisualAidWidget> m_VisualAidWidget;

    NVScopedRefCounted<qt3ds::widgets::IPathWidget> m_PathWidget;
    NVScopedRefCounted<qt3ds::render::NVRenderTexture2D> m_PickBuffer;
    Option<SPathAnchorDragInitialValue> m_LastPathDragValue;
    nvvector<qt3ds::QT3DSU8> m_PixelBuffer;
    nvvector<SGraphObjectTranslator *> m_editModeCamerasAndLights;
    QT3DSF32 m_CumulativeRotation;
    eastl::vector<qt3ds::render::SPGGraphObject *> m_GuideContainer;
    qt3ds::foundation::SFastAllocator<> m_GuideAllocator;
    // The rects are maintained from last render because the render context
    // doesn't guarantee the rects it returns are valid outside of begin/end render calls.
    SRulerRect m_OuterRect;
    SRulerRect m_InnerRect; // presentation rect.

    QT3DSVec4 m_rectColor;
    QT3DSVec4 m_lineColor;
    QT3DSVec4 m_guideColor;
    QT3DSVec4 m_selectedGuideColor;
    QT3DSVec4 m_guideFillColor;
    QT3DSVec4 m_selectedGuideFillColor;
#endif
public:
    qt3dsdm::SComposerObjectDefinitions &objectDefinitions() const
    {
        return m_objectDefinitions;
    }
    Q3DStudio::CGraph &assetGraph()
    {
        return m_assetGraph;
    }
    IDocumentReader &reader()
    {
        return m_reader;
    }
    Q3DSUipPresentation *presentation() const
    {
        return m_presentation.data();
    }
    void render();
    Q3DSGraphObjectTranslator *getOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance);
};

}
#endif
