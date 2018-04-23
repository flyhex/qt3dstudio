/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
#ifndef QT3DS_STUDIO_RENDERER_TRANSLATION_H
#define QT3DS_STUDIO_RENDERER_TRANSLATION_H
#pragma once
#include "StudioRendererImpl.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderer.h"
#include "StudioWidget.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "foundation/FastAllocator.h"
#include "StudioPickValues.h"
#include "Qt3DSDMGuides.h"
#include "PathWidget.h"
#include "StudioPreferences.h"
#include "StudioGradientWidget.h"

namespace qt3ds {
namespace studio {
    struct SGraphObjectTranslator;
    extern QT3DSU32 g_GraphObjectTranslatorTag;
    inline void InitializePointerTags(IStringTable &) { g_GraphObjectTranslatorTag = 0x0088BEEF; }
}
}
namespace qt3ds {
namespace render {
    template <>
    struct SPointerTag<qt3ds::studio::SGraphObjectTranslator>
    {
        static QT3DSU32 GetTag() { return qt3ds::studio::g_GraphObjectTranslatorTag; }
    };
}
}

namespace qt3ds {
namespace studio {

    typedef std::shared_ptr<qt3dsdm::ISignalConnection> TSignalConnection;

    struct STranslation;

    struct SGraphObjectTranslator
    {
    protected:
        qt3dsdm::Qt3DSDMInstanceHandle m_InstanceHandle;

    public:
        // This will never be null.  The reason it is a pointer is because
        // alias translators need to switch which graph object they point to
        qt3dsdm::Qt3DSDMInstanceHandle m_AliasInstanceHandle;
        SGraphObject *m_GraphObject;
        QT3DSU32 m_DirtyIndex;
        SGraphObjectTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, SGraphObject &inObj)
            : m_InstanceHandle(inInstance)
            , m_GraphObject(&inObj)
            , m_DirtyIndex(QT3DS_MAX_U32)
        {
            m_GraphObject->m_UserData = qt3ds::render::STaggedPointer(this);
        }
        // The destructors will not be called at this time for most of the objects
        // but they will be released.
        virtual ~SGraphObjectTranslator() {}
        // Push new data into the UIC render graph.
        virtual void PushTranslation(STranslation &inTranslatorContext);
        virtual void AfterRenderGraphIsBuilt(STranslation &) {}
        virtual void SetActive(bool inActive) = 0;
        virtual void ClearChildren() = 0;
        virtual void AppendChild(SGraphObject &inChild) = 0;
        virtual SGraphObject &GetGraphObject() { return *m_GraphObject; }
        virtual SGraphObject &GetNonAliasedGraphObject() { return *m_GraphObject; }
        virtual qt3dsdm::Qt3DSDMInstanceHandle GetInstanceHandle() { return m_InstanceHandle; }
        virtual qt3dsdm::Qt3DSDMInstanceHandle GetSceneGraphInstanceHandle()
        {
            return m_InstanceHandle;
        }
        virtual qt3dsdm::Qt3DSDMInstanceHandle GetPossiblyAliasedInstanceHandle()
        {
            if (m_AliasInstanceHandle.Valid())
                return m_AliasInstanceHandle;
            return GetInstanceHandle();
        }
    };

    struct STranslatorGetDirty
    {
        QT3DSU32 operator()(const SGraphObjectTranslator &inTrans) const
        {
            return inTrans.m_DirtyIndex;
        }
    };
    struct STranslatorSetDirty
    {
        void operator()(SGraphObjectTranslator &inTrans, QT3DSU32 idx) const
        {
            inTrans.m_DirtyIndex = idx;
        }
    };

    typedef InvasiveSet<SGraphObjectTranslator, STranslatorGetDirty, STranslatorSetDirty>
        TTranslatorDirtySet;

    struct TranslationSelectMode
    {
        enum Enum {
            Group = 0,
            Single = 1,
            NestedComponentSingle,
        };
    };

    struct EditCameraTypes
    {
        enum Enum {
            SceneCamera = 0,
            Perspective,
            Orthographic,
            Directional,
        };
    };

    const QT3DSF32 g_EditCameraFOV = 45.0f;
    const QT3DSF32 g_RotationScaleFactor = 2.0f * QT3DSF32(M_PI) / 180.0f;

    struct SEditCameraPersistentInformation
    {
        QT3DSVec3 m_Position;
        QT3DSVec3 m_Direction;
        QT3DSF32 m_ViewRadius;
        EditCameraTypes::Enum m_CameraType;
        NVReal m_xRotation = 0.0f;
        NVReal m_yRotation = 0.0f;
        SEditCameraPersistentInformation()
            : m_Position(0, 0, 0)
            , m_Direction(0, 0, 0)
            , m_ViewRadius(600)
            , m_CameraType(EditCameraTypes::Perspective)
        {
        }

        void ApplyToCamera(SCamera &inCamera, QT3DSVec2 inViewport)
        {
            // Setup shared default values.
            inCamera.m_ClipFar = 2000000.0f;
            inCamera.m_ClipNear = 1.0f;
            if (m_CameraType == EditCameraTypes::Perspective) {
                inCamera.m_FOV = g_EditCameraFOV;
                TORAD(inCamera.m_FOV);
                inCamera.m_Flags.SetOrthographic(false);
            } else {
                inCamera.m_Flags.SetOrthographic(true);
            }

            // The goal is to setup a global transform that
            QT3DSMat44 thePivotMatrix = QT3DSMat44::createIdentity();
            thePivotMatrix.column3.x = m_Position.x;
            thePivotMatrix.column3.y = m_Position.y;
            thePivotMatrix.column3.z = m_Position.z;
            QT3DSMat44 theGlobalTransform = thePivotMatrix;

            QT3DSVec3 theUpDir(0, 1, 0);
            if (m_CameraType == EditCameraTypes::Directional) {
                QT3DSF32 theTestLen = m_Direction.cross(theUpDir).magnitudeSquared();
                if (theTestLen < .01f)
                    theUpDir = QT3DSVec3(0, 0, 1) * m_Direction.dot(QT3DSVec3(0, 1, 0));
                theUpDir.normalize();
            }

            QT3DSMat33 theLookAtMatrix = inCamera.GetLookAtMatrix(theUpDir, m_Direction);
            QT3DSMat44 theRotationTransform =
                    QT3DSMat44(theLookAtMatrix.column0, theLookAtMatrix.column1,
                               theLookAtMatrix.column2, QT3DSVec3(0, 0, 0));

            if (m_CameraType != EditCameraTypes::Directional) {
                theRotationTransform.rotate(-m_xRotation, QT3DSVec3(0.0f, 1.0f, 0.0f));
                theRotationTransform.rotate(m_yRotation, QT3DSVec3(1.0f, 0.0f, 0.0f));
            }

            // The view radius dictates the zoom.
            QT3DSF32 theZoom = 1.0f;
            if (inCamera.m_Flags.IsOrthographic()) {
                QT3DSF32 theViewport = NVMin(inViewport.x, inViewport.y);
                theZoom = (m_ViewRadius * 2.0f) / theViewport;
            } else {
                // We know the hypotenuse is 600.
                // So if we want to zoom the scene, we do this.
                theZoom = m_ViewRadius / (sinf(inCamera.m_FOV / 2.0f) * 600.f);
            }
            QT3DSMat44 theScaleMatrix = QT3DSMat44(QT3DSVec4(theZoom, theZoom, theZoom, 1));
            QT3DSMat44 thePositionMatrix = QT3DSMat44::createIdentity();
            thePositionMatrix.column3.x = m_Position.x;
            thePositionMatrix.column3.y = m_Position.y;
            thePositionMatrix.column3.z = m_Position.z + 600;
            theGlobalTransform = theGlobalTransform * theRotationTransform;
            theGlobalTransform = theGlobalTransform * theScaleMatrix;
            theGlobalTransform = theGlobalTransform * thePivotMatrix.getInverse();
            theGlobalTransform = theGlobalTransform * thePositionMatrix;
            // This works because the camera has no hierarchy.
            inCamera.m_LocalTransform = theGlobalTransform;
            inCamera.m_Flags.SetTransformDirty(false);
            inCamera.MarkDirty(qt3ds::render::NodeTransformDirtyFlag::TransformNotDirty);
        }

        bool IsOrthographic() const { return m_CameraType != EditCameraTypes::Perspective; }

        bool SupportsRotation() const { return m_CameraType != EditCameraTypes::Directional; }
    };
    struct MovementTypes
    {
        enum Enum {
            Unknown = 0,
            Translate,
            TranslateAlongCameraDirection,
            Scale,
            ScaleZ,
            Rotation,
            RotationAboutCameraDirection,
        };
    };

    struct SEditCameraLayerTranslator;
    struct SZoomRender
    {
        CPt m_Point;
        qt3ds::render::SLayer *m_Layer;
        SZoomRender(CPt inPoint, qt3ds::render::SLayer *inLayer)
            : m_Point(inPoint)
            , m_Layer(inLayer)
        {
        }
        SZoomRender()
            : m_Layer(nullptr)
        {
        }
    };

    struct PickTargetAreas
    {
        enum Enum {
            Presentation,
            Matte,
        };
    };

    struct SRulerRect
    {
        QT3DSI32 m_Left;
        QT3DSI32 m_Top;
        QT3DSI32 m_Right;
        QT3DSI32 m_Bottom;
        SRulerRect()
            : m_Left(0)
            , m_Top(0)
            , m_Right(0)
            , m_Bottom(0)
        {
        }
        SRulerRect(QT3DSI32 l, QT3DSI32 t, QT3DSI32 r, QT3DSI32 b)
            : m_Left(l)
            , m_Top(t)
            , m_Right(r)
            , m_Bottom(b)
        {
        }
        bool Contains(QT3DSI32 x, QT3DSI32 y) const
        {
            return x >= m_Left && x <= m_Right && y >= m_Bottom && y <= m_Top;
        }
    };

    struct SDragPreparationResult
    {
        qt3ds::render::IQt3DSRenderer *m_Renderer;
        SNode *m_Node;
        SLayer *m_Layer;
        SCamera *m_Camera;
        qt3ds::render::NVPlane m_Plane;
        QT3DSVec3 m_GlobalPos;
        QT3DSVec3 m_CameraGlobalPos;
        QT3DSVec3 m_CameraDirection;
        QT3DSVec3 m_Axis;
        QT3DSMat44 m_GlobalTransform;
        QT3DSMat33 m_NormalMatrix;
        QT3DSU32 m_AxisIndex;
        qt3ds::widgets::StudioWidgetComponentIds::Enum m_ComponentId;
        qt3ds::widgets::StudioWidgetTypes::Enum m_WidgetType;
        qt3ds::render::RenderWidgetModes::Enum m_WidgetMode;
        SRay m_OriginalRay;
        SRay m_CurrentRay;
        SRay m_PreviousRay;
        Option<QT3DSVec3> m_OriginalPlaneCoords;
        Option<QT3DSVec3> m_CurrentPlaneCoords;
        Option<QT3DSVec3> m_PreviousPlaneCoords;
        bool m_IsPlane;
        SDragPreparationResult() {}
    };

    struct SPathAnchorDragInitialValue
    {
        QT3DSVec2 m_Position;
        float m_IncomingAngle;
        float m_IncomingDistance;
        float m_OutgoingDistance;
        SPathAnchorDragInitialValue() {}
    };

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
        NVScopedRefCounted<qt3ds::widgets::IPathWidget> m_PathWidget;
        NVScopedRefCounted<qt3ds::render::NVRenderTexture2D> m_PickBuffer;
        Option<SPathAnchorDragInitialValue> m_LastPathDragValue;
        nvvector<qt3ds::QT3DSU8> m_PixelBuffer;
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
        void MarkBeginComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle) { ++m_ComponentSecondsDepth; }

        void MarkComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle)
        {
            m_ComponentSecondsDepth = NVMax(0, m_ComponentSecondsDepth - 1);
            if (m_ComponentSecondsDepth == 0)
                RequestRender();
        }

        void ReleaseTranslation(Q3DStudio::TIdentifier inInstance);

        void MarkGraphInstanceDirty(Q3DStudio::TIdentifier inInstance,
                                    Q3DStudio::TIdentifier /*inParent*/)
        {
            MarkDirty(inInstance);
        }

        void MarkDirty(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

        void DoMarkDirty(qt3dsdm::Qt3DSDMInstanceHandle inInstance) {MarkDirty(inInstance);}

        void MarkDirty(qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
        {
            for (long idx = 0; idx < inInstanceCount; ++idx)
                MarkDirty(inInstance[idx]);
        }

        void DrawBoundingBox(SNode &inNode, QT3DSVec3 inColor);
        void DrawLightBoundingBox(SNode &inNode, QT3DSVec3 inColor);

        void DrawChildBoundingBoxes(SNode &inNode)
        {
            ::CColor color = CStudioPreferences::GetGroupBoundingBoxColor();
            QT3DSVec3 colorVec(color.GetRed() / 255.f,
                               color.GetGreen() / 255.f,
                               color.GetBlue() / 255.f);
            for (SNode *theChild = inNode.m_FirstChild; theChild;
                 theChild = theChild->m_NextSibling) {
                if (IncludeNode(*theChild))
                    DrawBoundingBox(*theChild, colorVec);
            }
        }

        void DrawGroupBoundingBoxes(SGraphObjectTranslator &inTranslator)
        {
            SNode &theNode = static_cast<SNode &>(inTranslator.GetGraphObject());
            if (theNode.m_FirstChild) {
                ::CColor color = CStudioPreferences::GetGroupBoundingBoxColor();
                QT3DSVec3 colorVec(color.GetRed() / 255.f,
                                   color.GetGreen() / 255.f,
                                   color.GetBlue() / 255.f);
                DrawBoundingBox(theNode, colorVec);
                if (inTranslator.GetGraphObject().m_Type != GraphObjectTypes::Layer)
                    DrawChildBoundingBoxes(theNode);
            }
        }

        void DrawNonGroupBoundingBoxes(SGraphObjectTranslator &inTranslator)
        {
            SNode &theNode = static_cast<SNode &>(inTranslator.GetGraphObject());
            if (inTranslator.GetGraphObject().m_Type == GraphObjectTypes::Light) {
                ::CColor color = CStudioPreferences::GetLightBoundingBoxColor();
                QT3DSVec3 colorVec(color.GetRed() / 255.f,
                                   color.GetGreen() / 255.f,
                                   color.GetBlue() / 255.f);
                DrawLightBoundingBox(theNode, colorVec);
            } else if (inTranslator.GetGraphObject().m_Type != GraphObjectTypes::Layer) {
                ::CColor color = CStudioPreferences::GetSingleBoundingBoxColor();
                QT3DSVec3 colorVec(color.GetRed() / 255.f,
                                   color.GetGreen() / 255.f,
                                   color.GetBlue() / 255.f);
                DrawBoundingBox(theNode, colorVec);
                DrawChildBoundingBoxes(theNode);
            } else {
                ::CColor color = CStudioPreferences::GetSingleBoundingBoxColor();
                QT3DSVec3 colorVec(color.GetRed() / 255.f,
                                   color.GetGreen() / 255.f,
                                   color.GetBlue() / 255.f);
                m_Context.GetRenderer().RenderLayerRect(
                    static_cast<SLayer &>(inTranslator.GetGraphObject()), colorVec);
            }
        }

        void DrawAxis(SGraphObjectTranslator &inTranslator);

        void SetViewport(QT3DSF32 inWidth, QT3DSF32 inHeight) { m_Viewport = QT3DSVec2(inWidth, inHeight); }

        QT3DSVec2 GetViewportDimensions() { return m_Viewport; }

        void ClearDirtySet()
        {
            // The dirty set may be modified while this operation is taking place in the case of
            // alias nodes.
            for (qt3ds::QT3DSU32 idx = 0; idx < (qt3ds::QT3DSU32)m_DirtySet.size(); ++idx) {
                if (m_Reader.IsInstance(m_DirtySet[idx]->GetInstanceHandle()))
                    m_DirtySet[idx]->PushTranslation(*this);
            }
            m_DirtySet.clear();
        }
        // We build the render graph every time we render.  This may seem wasteful
        void BuildRenderGraph(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                              Qt3DSDMInstanceHandle inAliasHandle = qt3dsdm::Qt3DSDMInstanceHandle());
        void
        BuildRenderGraph(SGraphObjectTranslator &inParent,
                         qt3dsdm::Qt3DSDMInstanceHandle inAliasHandle = qt3dsdm::Qt3DSDMInstanceHandle());
        void
        DeactivateScan(SGraphObjectTranslator &inParent,
                       qt3dsdm::Qt3DSDMInstanceHandle inAliasHandle = qt3dsdm::Qt3DSDMInstanceHandle());
        void PreRender();
        void Render(int inWidgetId, bool inDrawGuides);
        void EndRender();
        void DoPrepareForDrag(SNode *inSelectedNode);
        void ResetWidgets();
        void EndDrag();
        bool IsPathWidgetActive();

        void PrepareForDrag() { DoPrepareForDrag(GetSelectedNode()); }

        SStudioPickValue Pick(CPt inMouseCoords, TranslationSelectMode::Enum inSelectMode);
        Option<QT3DSU32> PickWidget(CPt inMouseCoords, TranslationSelectMode::Enum inSelectMode,
                                 qt3ds::widgets::IStudioWidgetBase &inWidget);

        qt3ds::foundation::Option<qt3dsdm::SGuideInfo> PickRulers(CPt inMouseCoords);

        SNode *GetSelectedNode()
        {
            qt3dsdm::Qt3DSDMInstanceHandle theHandle = m_Doc.GetSelectedInstance();
            SGraphObjectTranslator *theTranslator = GetOrCreateTranslator(theHandle);
            if (theTranslator
                && GraphObjectTypes::IsNodeType(theTranslator->GetGraphObject().m_Type))
                return static_cast<SNode *>(&theTranslator->GetGraphObject());
            return nullptr;
        }
        static inline SFloat3 ToDataModel(const QT3DSVec3 &inValue)
        {
            return SFloat3(inValue.x, inValue.y, inValue.z);
        }

        static inline SFloat3 ToDataModelRotation(const QT3DSVec3 &inValue)
        {
            SFloat3 retval = ToDataModel(inValue);
            TODEG(retval.m_Floats[0]);
            TODEG(retval.m_Floats[1]);
            TODEG(retval.m_Floats[2]);
            return retval;
        }

        void SetPosition(const QT3DSVec3 &inPosition, CUpdateableDocumentEditor &inEditor)
        {
            inEditor.EnsureEditor(L"Set Position", __FILE__, __LINE__)
                .SetInstancePropertyValue(m_Doc.GetSelectedInstance(),
                                          m_ObjectDefinitions.m_Node.m_Position,
                                          ToDataModel(inPosition));
            inEditor.FireImmediateRefresh(m_Doc.GetSelectedInstance());
        }
        void SetRotation(const QT3DSVec3 &inRotation, CUpdateableDocumentEditor &inEditor)
        {
            inEditor.EnsureEditor(L"Set Rotation", __FILE__, __LINE__)
                .SetInstancePropertyValue(m_Doc.GetSelectedInstance(),
                                          m_ObjectDefinitions.m_Node.m_Rotation,
                                          ToDataModelRotation(inRotation));
            inEditor.FireImmediateRefresh(m_Doc.GetSelectedInstance());
        }
        void SetScale(const QT3DSVec3 &inScale, CUpdateableDocumentEditor &inEditor)
        {
            inEditor.EnsureEditor(L"Set Scale", __FILE__, __LINE__)
                .SetInstancePropertyValue(m_Doc.GetSelectedInstance(),
                                          m_ObjectDefinitions.m_Node.m_Scale, ToDataModel(inScale));
            inEditor.FireImmediateRefresh(m_Doc.GetSelectedInstance());
        }

        QT3DSVec3 GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inInstance, CPt inPos);

        void ApplyPositionalChange(QT3DSVec3 inDiff, SNode &inNode,
                                   CUpdateableDocumentEditor &inEditor);

        void TranslateSelectedInstanceAlongCameraDirection(CPt inOriginalCoords, CPt inMouseCoords,
                                                           CUpdateableDocumentEditor &inEditor);

        void TranslateSelectedInstance(CPt inOriginalCoords, CPt inMouseCoords,
                                       CUpdateableDocumentEditor &inEditor, bool inLockToAxis);

        void ScaleSelectedInstanceZ(CPt inOriginalCoords, CPt inMouseCoords,
                                    CUpdateableDocumentEditor &inEditor);

        void ScaleSelectedInstance(CPt inOriginalCoords, CPt inMouseCoords,
                                   CUpdateableDocumentEditor &inEditor);

        void CalculateNodeGlobalRotation(SNode &inNode);

        void ApplyRotationToSelectedInstance(const QT3DSQuat &inFinalRotation, SNode &inNode,
                                             CUpdateableDocumentEditor &inEditor,
                                             bool inIsMouseRelative = true);

        void RotateSelectedInstanceAboutCameraDirectionVector(CPt inPreviousMouseCoords,
                                                              CPt inMouseCoords,
                                                              CUpdateableDocumentEditor &inEditor);

        // This method never feels right to me.  It is difficult to apply it to a single axis (of
        // course for that
        // you can use the inspector palette).
        void RotateSelectedInstance(CPt inOriginalCoords, CPt inPreviousMouseCoords,
                                    CPt inMouseCoords, CUpdateableDocumentEditor &inEditor,
                                    bool inLockToAxis);

        Option<SDragPreparationResult>
        PrepareWidgetDrag(qt3ds::widgets::StudioWidgetComponentIds::Enum inComponentId,
                          qt3ds::widgets::StudioWidgetTypes::Enum inWidgetId,
                          qt3ds::render::RenderWidgetModes::Enum inWidgetMode, SNode &inNode,
                          CPt inOriginalCoords, CPt inPreviousMouseCoords, CPt inMouseCoords);

        void PerformWidgetDrag(int inWidgetSubComponent, CPt inOriginalCoords,
                               CPt inPreviousMouseCoords, CPt inMouseCoords,
                               CUpdateableDocumentEditor &inEditor);

        void PerformGuideDrag(Qt3DSDMGuideHandle inGuide, CPt inPoint,
                              CUpdateableDocumentEditor &inEditor);
        void CheckGuideInPresentationRect(Qt3DSDMGuideHandle inGuide,
                                          CUpdateableDocumentEditor &inEditor);

        void PerformPathDrag(qt3ds::studio::SPathPick &inPathPick, CPt inOriginalCoords,
                             CPt inPreviousMouseCoords, CPt inMouseCoords,
                             CUpdateableDocumentEditor &inEditor);

        void RequestRender()
        {
            if (m_ComponentSecondsDepth == 0)
                m_Renderer.RequestRender();
        }

        void RenderZoomRender(SZoomRender &inRender);

        // IQt3DSRenderNodeFilter
        bool IncludeNode(const SNode &inNode) override;

        PickTargetAreas::Enum GetPickArea(CPt inPoint);

        SNode *GetEditCameraLayer();

        void ReleaseEffect(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
        // Create a new translator for this type.  Do not add to any maps or anything else.
        SGraphObjectTranslator *CreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
        // Returns the canonical translator for a given instance or creates a new translator if none
        // exist.
        SGraphObjectTranslator *GetOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
        // Create a new aliased translator for this type.
        SGraphObjectTranslator *GetOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                      qt3dsdm::Qt3DSDMInstanceHandle inAliasInstance);
        THandleTranslatorPairList &
        GetTranslatorsForInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
        qt3dsdm::Qt3DSDMInstanceHandle GetAnchorPoint(SPathPick &inPick);
        qt3dsdm::Qt3DSDMInstanceHandle GetAnchorPoint(QT3DSU32 inAnchorIndex);
    };

    struct SDisableUseClearColor
    {
        SGraphObjectTranslator *m_SceneTranslator;
        bool m_PreviousUseClearColor;
        bool m_DisableUseClearColor;

        SDisableUseClearColor(STranslation &inTranslation, bool disableUseClearColor)
            : m_SceneTranslator(nullptr)
            , m_PreviousUseClearColor(false)
            , m_DisableUseClearColor(disableUseClearColor)
        {
            if (m_DisableUseClearColor) {
                TIdentifier theRoot = inTranslation.m_AssetGraph.GetRoot(0);
                m_SceneTranslator = inTranslation.GetOrCreateTranslator(theRoot);
                if (m_SceneTranslator) {
                    SScene &theScene = static_cast<SScene &>(m_SceneTranslator->GetGraphObject());
                    m_PreviousUseClearColor = theScene.m_UseClearColor;
                    SetUseClearColor(false);
                }
            }
        }

        ~SDisableUseClearColor()
        {
            if (m_DisableUseClearColor) {
                SetUseClearColor(m_PreviousUseClearColor);
            }
        }

        void SetUseClearColor(bool inUseClearColor)
        {
            if (m_SceneTranslator) {
                SScene &theScene = static_cast<SScene &>(m_SceneTranslator->GetGraphObject());
                theScene.m_UseClearColor = inUseClearColor;
            }
        }
    };
}
}

#endif
