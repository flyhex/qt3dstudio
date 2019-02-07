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
#include "Q3DSEditCamera.h"
#include "Q3DSSelectionWidget.h"
#include "Q3DSManipulationWidget.h"
#include "Q3DSVisualAidWidget.h"
#include "StudioEnums.h"

namespace Q3DStudio
{

class CUpdateableDocumentEditor;
class Q3DStudioRenderer;
class Q3DSGraphObjectTranslator;
class Q3DSLightTranslator;
class Q3DSCameraTranslator;
class Q3DSReferencedMaterialTranslator;

class Q3DSTranslation
{
public:
    Q3DSTranslation(Q3DStudioRenderer &inRenderer,
                    const QSharedPointer<Q3DSUipPresentation> &presentation);

protected:
    void markPropertyDirty(qt3dsdm::Qt3DSDMInstanceHandle instance,
                           qt3dsdm::Qt3DSDMPropertyHandle property);
    void releaseTranslation(qt3dsdm::Qt3DSDMInstanceHandle instance);
    void markGraphInstanceDirty(int instance, int parent);
    void markBeginComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle slide);
    void markComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle);

private:
    void setPresentationData();
    QByteArray getInstanceObjectId(qt3dsdm::Qt3DSDMInstanceHandle instance);
    Q3DSLayerNode *layerForNode(Q3DSGraphObject *node);
    Q3DSCameraNode *cameraForNode(Q3DSGraphObject *node, bool ignoreSelfCamera = false);
    void updateVisualAids();
    void updateForegroundLayerProperties();
    void updateSelectionWidgetProperties();
    void createSelectionWidget();
    void enableManipulationWidget();
    void disableVisualAids();
    void enableVisualAids();

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
    THandleTranslatorPairList &getTranslatorsForInstance(qt3dsdm::Qt3DSDMInstanceHandle instance);
    Q3DSGraphObjectTranslator *createEffectTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                      qt3dsdm::Qt3DSDMInstanceHandle parentClass,
                                                      const QByteArray &id);
    Q3DSGraphObjectTranslator *createCustomMaterialTranslator(
                                                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                    qt3dsdm::Qt3DSDMInstanceHandle parentClass,
                                                    const QByteArray &id);

    Q3DSGraphObject *createAliasGraphObject(qt3dsdm::ComposerObjectTypes::Enum type,
                                            const QByteArray &id);

    void recompileShadersIfRequired(Q3DSGraphObjectTranslator *translator,
                                    const qt3dsdm::SValue &value, const QString &name,
                                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                                    qt3dsdm::Qt3DSDMPropertyHandle property);

    QVector4D calculateWidgetArrowDrag(const QPoint &mousePos, const QVector3D &dragPlane = {},
                                       const QVector3D &arrowDir = {});

    Q3DStudioRenderer &m_studioRenderer;

    CDoc &m_doc;
    IDocumentReader &m_reader;
    qt3dsdm::SComposerObjectDefinitions &m_objectDefinitions;
    qt3dsdm::CStudioSystem &m_studioSystem;
    qt3dsdm::CStudioFullSystem &m_fullSystem;
    Q3DStudio::CGraph &m_assetGraph;
    QSharedPointer<Q3DSEngine> m_engine;
    QSharedPointer<Q3DSUipPresentation> m_presentation;

    // All translator related containers must come after the allocator
    TInstanceToTranslatorMap m_translatorMap;
    TTranslatorDirtySet m_dirtySet;
    TTranslatorDirtySet m_releaseSet;
    Q3DSPresentationData m_presentation_data;
    Q3DSScene *m_scene;
    Q3DStudio::CGraphIterator m_graphIterator;
    QVector<TSignalConnection> m_signalConnections;
    quint32 m_componentSecondsDepth;
    QMap<qt3dsdm::Qt3DSDMInstanceHandle, Q3DSGraphObjectTranslator *> m_slideTranslatorMap;
    QVector<Q3DSReferencedMaterialTranslator *> m_refMatTranslators;
    QMap<qt3dsdm::Qt3DSDMInstanceHandle, Q3DSGraphObjectTranslator *> m_masterSlideMap;
    QHash<qt3dsdm::Qt3DSDMInstanceHandle, QByteArray> m_instanceIdHash;
    QVector<Q3DSLightTranslator *> m_lightTranslators;
    QVector<Q3DSCameraTranslator *> m_cameraTranslators;
    QRect m_rect;
    QSize m_size;
    qreal m_pixelRatio = 0.0;
    QHash<QByteArray, Q3DSCameraNode *> m_editCameras;
    SEditCameraPersistentInformation m_editCameraInfo;
    bool m_editCameraEnabled = false;
    Q3DSGraphObjectTranslator *m_dragTranslator = nullptr;
    Q3DSCameraNode *m_dragCamera = nullptr;
    QPoint m_dragStartMousePos;
    QPoint m_dragPosDiff;
    QMatrix4x4 m_dragNodeGlobalTransform;
    bool m_presentationInit = false;

    Q3DSLayerNode *m_backgroundLayer = nullptr;
    Q3DSLayerNode *m_foregroundLayer = nullptr;
    Q3DSLayerNode *m_foregroundPickingLayer = nullptr;
    Q3DSCameraNode *m_foregroundCamera = nullptr;
    Q3DSCameraNode *m_foregroundPickingCamera = nullptr;
    Q3DSCameraNode *m_selectedCamera = nullptr;
    Q3DSLayerNode *m_selectedLayer = nullptr;
    Q3DSGraphObject *m_selectedObject = nullptr;
    Q3DSModelNode *m_gradient = nullptr;
    Q3DSCustomMaterialInstance *m_gradientMaterial = nullptr;
    long m_toolMode = STUDIO_TOOLMODE_MOVE;
    Q3DSGraphObject *m_pickedWidget = nullptr;
    QColor m_pickedWidgetColor;
    EditCameraTypes m_oldCameraType = EditCameraTypes::SceneCamera;
    Q3DSManipulationWidget m_manipulationWidget;
    Q3DSSelectionWidget m_selectionWidget;

    QVector<Q3DSVisualAidWidget> m_visualAids;
    quint64 m_visualAidIndex = 0;

    struct DragState
    {
        QVector3D t;
        QVector3D s;
        QVector3D r;
    };

    DragState m_beginDragState;
    DragState m_currentDragState;

public:
    qt3dsdm::SComposerObjectDefinitions &objectDefinitions() const
    {
        return m_objectDefinitions;
    }
    Q3DStudio::CGraph &assetGraph() const
    {
        return m_assetGraph;
    }
    IDocumentReader &reader() const
    {
        return m_reader;
    }
    qt3dsdm::CStudioFullSystem &fullSystem() const
    {
        return m_fullSystem;
    }
    Q3DSUipPresentation *presentation() const
    {
        return m_presentation.data();
    }
    void prepareRender(const QRect &rect, const QSize &size, qreal pixelRatio);
    Q3DSGraphObjectTranslator *getOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance);
    void enableEditCamera(const SEditCameraPersistentInformation &info);
    void disableEditCamera();
    SEditCameraPersistentInformation editCameraInfo() const;
    void enableSceneCameras(bool enable);
    void wheelZoom(qreal factor);
    void enableBackgroundLayer();
    void enableForegroundLayer();
    void disableGradient();
    void enableGradient();
    void selectObject(qt3dsdm::Qt3DSDMInstanceHandle instance);
    void unselectObject();
    void releaseTranslator(Q3DSGraphObjectTranslator *translator);
    void clearDirtySet();
    void markDirty(qt3dsdm::Qt3DSDMInstanceHandle instance);
    void markDirty(qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
    {
        for (long idx = 0; idx < inInstanceCount; ++idx)
            markDirty(inInstance[idx]);
    }

    Q3DSGraphObjectTranslator *createTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                Q3DSGraphObjectTranslator *aliasTranslator
                                                    = nullptr);
    Q3DSGraphObjectTranslator *getOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                     qt3dsdm::Qt3DSDMInstanceHandle aliasInstance,
                                                     Q3DSGraphObjectTranslator *aliasTranslator
                                                        = nullptr);

    void prepareDrag(const QPoint &mousePos, Q3DSGraphObjectTranslator *selected = nullptr);
    void prepareWidgetDrag(const QPoint &mousePos, Q3DSGraphObject *obj);
    void endDrag(bool dragReset, CUpdateableDocumentEditor &inEditor);
    void endPickWidget();

    void translateAlongCameraDirection(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                                       CUpdateableDocumentEditor &inEditor);
    void translate(const QPoint &inMouseCoords, CUpdateableDocumentEditor &inEditor,
                   bool inLockToAxis);
    void translateAlongWidget(const QPoint &inMouseCoords,
                              CUpdateableDocumentEditor &inEditor);
    void scaleZ(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                CUpdateableDocumentEditor &inEditor);
    void scale(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
               CUpdateableDocumentEditor &inEditor);
    void scaleAlongWidget(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                          CUpdateableDocumentEditor &inEditor);
    void rotateAboutCameraDirectionVector(const QPoint &inOriginalCoords,
                                          const QPoint &inMouseCoords,
                                          CUpdateableDocumentEditor &inEditor);
    void rotate(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                CUpdateableDocumentEditor &inEditor, bool inLockToAxis);
    void rotateAlongWidget(const QPoint &inOriginalCoords, const QPoint &inMouseCoords,
                           CUpdateableDocumentEditor &inEditor);
};

}
#endif
