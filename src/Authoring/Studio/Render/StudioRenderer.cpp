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
#include "stdafx.h"
#include "StudioRendererImpl.h"
#include "StudioRendererTranslation.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "StudioUtils.h"

#include <QDebug>

#ifdef _WIN32
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#endif
using namespace qt3ds::studio;

namespace {

const QT3DSU32 g_WheelFactor = 10; // the wheel zoom factor

struct SEditCameraDefinition
{
    EditCameraTypes::Enum m_Type;
    // Directional cameras have a direction they point
    QT3DSVec3 m_Direction; // not normalized
    QString m_Name;
};

SEditCameraDefinition g_EditCameraDefinitions[] = {
    { EditCameraTypes::Perspective, QT3DSVec3(1, -1, -1), QObject::tr("Perspective View") },
    { EditCameraTypes::Orthographic, QT3DSVec3(1, -1, -1), QObject::tr("Orthographic View") },
    { EditCameraTypes::Directional, QT3DSVec3(0, -1, 0), QObject::tr("Top View") },
    { EditCameraTypes::Directional, QT3DSVec3(0, 1, 0), QObject::tr("Bottom View") },
    { EditCameraTypes::Directional, QT3DSVec3(1, 0, 0), QObject::tr("Left View") },
    { EditCameraTypes::Directional, QT3DSVec3(-1, 0, 0), QObject::tr("Right View") },
    { EditCameraTypes::Directional, QT3DSVec3(0, 0, -1), QObject::tr("Front View") },
    { EditCameraTypes::Directional, QT3DSVec3(0, 0, 1), QObject::tr("Back View") },
};
QT3DSU32 g_NumEditCameras = sizeof(g_EditCameraDefinitions) / sizeof(*g_EditCameraDefinitions);

struct SRendererImpl : public IStudioRenderer,
                       public IDataModelListener,
                       public IReloadListener,
                       public CPresentationChangeListener,
                       public CSceneDragListener,
                       public CToolbarChangeListener
{
    typedef eastl::vector<Option<SEditCameraPersistentInformation>> TEditCameraInfoList;
    std::shared_ptr<CWGLRenderContext> m_RenderContext;
    NVScopedRefCounted<IUICRenderContext> m_UICContext;
    QRect m_Rect;
    CDispatch &m_Dispatch;
    CDoc &m_Doc;
    std::shared_ptr<STranslation> m_Translation;
    CPt m_MouseDownPoint;
    CPt m_PreviousMousePoint;
    bool m_HasPresentation;
    bool m_Closed;
    CUpdateableDocumentEditor m_UpdatableEditor;
    MovementTypes::Enum m_LastDragToolMode;
    bool m_MaybeDragStart;
    TEditCameraInfoList m_EditCameraInformation;
    QT3DSI32 m_EditCameraIndex;
    SEditCameraPersistentInformation m_MouseDownCameraInformation;
    SStudioPickValue m_PickResult;
    bool m_RenderRequested;
    int m_LastToolMode;
    bool m_GuidesEnabled;
    UICDM::TSignalConnectionPtr m_SelectionSignal;

    SRendererImpl()
        : m_Dispatch(*g_StudioApp.GetCore()->GetDispatch())
        , m_Doc(*g_StudioApp.GetCore()->GetDoc())
        , m_HasPresentation(false)
        , m_Closed(false)
        , m_UpdatableEditor(m_Doc)
        , m_LastDragToolMode(MovementTypes::Unknown)
        , m_MaybeDragStart(false)
        , m_EditCameraIndex(-1)
        , m_RenderRequested(false)
        , m_LastToolMode(0)
        , m_GuidesEnabled(true)
    {
        m_Dispatch.AddReloadListener(this);
        m_Dispatch.AddDataModelListener(this);
        m_Dispatch.AddPresentationChangeListener(this);
        m_SelectionSignal =
            m_Dispatch.ConnectSelectionChange(std::bind(&SRendererImpl::OnSelectionChange, this));
        m_Dispatch.AddSceneDragListener(this);
        m_Dispatch.AddToolbarChangeListener(this);
    }
    ~SRendererImpl()
    {
        Close();
        m_Dispatch.RemoveDataModelListener(this);
        m_Dispatch.RemovePresentationChangeListener(this);
        m_Dispatch.RemoveSceneDragListener(this);
        m_Dispatch.RemoveToolbarChangeListener(this);
    }

    // IDocSceneGraph
    QT3DSVec3 GetIntendedPosition(UICDM::CUICDMInstanceHandle inHandle, CPt inPoint) override
    {
        if (m_Translation)
            return m_Translation->GetIntendedPosition(inHandle, inPoint);

        return QT3DSVec3(0, 0, 0);
    }

    ITextRenderer *GetTextRenderer() override
    {
        if (m_UICContext.mPtr)
            return m_UICContext->GetTextRenderer();
        return NULL;
    }
    // The buffer manager may not be available
    IBufferManager *GetBufferManager() override
    {
        if (m_UICContext.mPtr)
            return &m_UICContext->GetBufferManager();
        return NULL;
    }

    IPathManager *GetPathManager() override
    {
        if (m_UICContext.mPtr)
            return &m_UICContext->GetPathManager();
        return NULL;
    }

    qt3ds::foundation::IStringTable *GetRenderStringTable() override
    {
        if (m_UICContext.mPtr)
            return &m_UICContext->GetStringTable();
        return NULL;
    }

    bool IsInitialized() override { return m_UICContext.mPtr != NULL; }

    void Initialize(QWidget *inWindow) override
    {
        if (m_Closed)
            return;
        QT3DS_ASSERT(!m_RenderContext);
        QT3DS_ASSERT(m_UICContext.mPtr == NULL);
        try {
            m_RenderContext = std::make_shared<CWGLRenderContext>(inWindow);

            Q3DStudio::CString theResourcePath = Q3DStudio::CString::fromQString(resourcePath());
            NVScopedRefCounted<qt3ds::render::IUICRenderContextCore> theCore =
                qt3ds::render::IUICRenderContextCore::Create(
                    m_RenderContext->GetRenderContext().GetFoundation(),
                    m_RenderContext->GetRenderContext().GetStringTable());

            // Create text renderer
            qt3ds::render::ITextRendererCore &theTextRenderer(
                        qt3ds::render::ITextRendererCore::CreateQtTextRenderer(
                            m_RenderContext->GetRenderContext().GetFoundation(),
                            m_RenderContext->GetRenderContext().GetStringTable()));
            theCore->SetTextRendererCore(theTextRenderer);

            m_UICContext = theCore->CreateRenderContext(
                m_RenderContext->GetRenderContext(),
                m_RenderContext->GetRenderContext().GetStringTable().RegisterStr(
                    theResourcePath.c_str()));

            // Allow the artist to interact with the top level objects alone.
            m_UICContext->GetRenderer().PickRenderPlugins(false);

            SetupTextRenderer();

            m_UICContext->SetAuthoringMode(true);

            InitializePointerTags(m_UICContext->GetStringTable());
            SetViewRect(m_Rect);
#ifdef KDAB_TEMPORARILY_REMOVE
            // KDAB_TODO the below call asserts on windows
            m_RenderContext->GetRenderContext().SetClearColor(QT3DSVec4(0, 0, 0, 1));
#endif
            if (m_HasPresentation)
                CreateTranslator();

            // Notify that renderer has been initialized
            m_Dispatch.FireOnRendererInitialized();
        } catch (...) {
            m_UICContext = nullptr;
            m_RenderContext = std::shared_ptr<CWGLRenderContext>();
            throw;
        }
    }

    void SetViewRect(const QRect &inRect) override
    {
        if (m_RenderContext)
            m_RenderContext->resized();

        m_Rect = inRect;
        if (IsInitialized()) {
            m_RenderContext->BeginRender();
            NVRenderContext &theContext = m_RenderContext->GetRenderContext();
            theContext.SetViewport(qt3ds::render::NVRenderRect(0, 0, inRect.width(),
                                                            inRect.height()));
            SetTranslationViewport();
            m_RenderContext->EndRender();
        }
    }
    // Request that this object renders.  May be ignored if a transaction
    // is ongoing so we don't get multiple rendering per transaction.
    void RequestRender() override
    {
        if (m_RenderContext)
            m_RenderContext->requestRender();
    }

    void RenderRequestedRender()
    {
        if (m_RenderRequested) {
            m_RenderContext->requestRender();
        }
    }

    void RenderNow() override
    {
        Render();
    }

    void MakeContextCurrent() override
    {
        m_RenderContext->BeginRender();
    }

    void ReleaseContext()
    {
        m_RenderContext->EndRender();
    }

    void Render()
    {
        m_RenderRequested = false;
        if (!m_Closed && IsInitialized()) {
            m_RenderContext->BeginRender();
            if (m_Translation)
                m_Translation->PreRender();
            NVRenderContext &theContext = m_RenderContext->GetRenderContext();
            theContext.SetDepthWriteEnabled(true);
            theContext.Clear(qt3ds::render::NVRenderClearFlags(
                qt3ds::render::NVRenderClearValues::Color | qt3ds::render::NVRenderClearValues::Depth));
            if (m_Translation) {
                m_Translation->Render(m_PickResult.GetWidgetId(), m_GuidesEnabled);
            }
            m_RenderContext->EndRender();
        }
    }
    void GetEditCameraList(QStringList &outCameras) override
    {
        outCameras.clear();
        for (QT3DSU32 idx = 0; idx < g_NumEditCameras; ++idx)
            outCameras.push_back(g_EditCameraDefinitions[idx].m_Name);
    }
    void SetEnableEditLight(bool inEnableLight) override
    {
        CStudioPreferences::SetEditViewFillMode(inEnableLight);
        if (m_Translation)
            m_Translation->m_EditLightEnabled = inEnableLight;
        RequestRender();
    }

    bool DoesEditCameraSupportRotation(QT3DSI32 inIndex) override
    {
        if (inIndex >= 0 && inIndex < (QT3DSI32)g_NumEditCameras)
            return g_EditCameraDefinitions[inIndex].m_Type != EditCameraTypes::Directional;
        return false;
    }

    bool AreGuidesEnabled() const override { return m_GuidesEnabled; }

    void SetGuidesEnabled(bool val) override { m_GuidesEnabled = val; }

    bool AreGuidesEditable() const override { return m_Doc.IsValid() ? m_Doc.GetDocumentReader().AreGuidesEditable() : false; }

    void SetGuidesEditable(bool val) override { if (m_Doc.IsValid()) m_Doc.GetDocumentReader().SetGuidesEditable(val); }

    // Setting the camera to -1 disables the edit cameras
    // So setting the camera to 0- (numcameras - 1) will set change the active
    // edit camera.
    void SetEditCamera(QT3DSI32 inIndex) override
    {
        QT3DSI32 oldIndex = m_EditCameraIndex;
        m_EditCameraIndex = NVMin(inIndex, (QT3DSI32)g_NumEditCameras);
        // save the old edit camera information
        if (oldIndex != m_EditCameraIndex && m_Translation && m_Translation->m_EditCameraEnabled) {
            while (m_EditCameraInformation.size() <= (QT3DSU32)oldIndex)
                m_EditCameraInformation.push_back(Empty());

            m_EditCameraInformation[oldIndex] = m_Translation->m_EditCameraInfo;
        }

        ApplyEditCameraIndex();
        RequestRender();
    }

    QT3DSI32 GetEditCamera() const override
    {
        if (m_EditCameraIndex >= 0 && m_EditCameraIndex < (QT3DSI32)g_NumEditCameras)
            return m_EditCameraIndex;
        return -1;
    }

    bool IsEditLightEnabled() const override
    {
        return GetEditCamera() >= 0 && CStudioPreferences::GetEditViewFillMode();
    }

    void EditCameraZoomToFit() override
    {
        UICDM::CUICDMInstanceHandle theInstance = m_Doc.GetSelectedInstance();
        if (!m_Translation || m_Translation->m_EditCameraEnabled == false)
            return;
        // If we aren't pointed at a node then bounce up the asset graph till we are.
        while (theInstance.Valid() && m_Translation->GetOrCreateTranslator(theInstance)
               && GraphObjectTypes::IsNodeType(
                      m_Translation->GetOrCreateTranslator(theInstance)->GetGraphObject().m_Type)
                   == false) {
            theInstance = m_Translation->m_AssetGraph.GetParent(theInstance);
        }
        // If we still aren't pointed at a node then use the active layer.
        if (theInstance.Valid() == false
            || m_Translation->GetOrCreateTranslator(theInstance) == nullptr
            || GraphObjectTypes::IsNodeType(
                   m_Translation->GetOrCreateTranslator(theInstance)->GetGraphObject().m_Type)
                == false)
            theInstance = m_Doc.GetActiveLayer();

        // If we *still* aren't pointed at a node then bail.
        if (m_Translation->GetOrCreateTranslator(theInstance) == nullptr
            || GraphObjectTypes::IsNodeType(
                   m_Translation->GetOrCreateTranslator(theInstance)->GetGraphObject().m_Type)
                == false)
            return;

        SNode &theNode = static_cast<SNode &>(
            m_Translation->GetOrCreateTranslator(theInstance)->GetGraphObject());
        qt3ds::NVBounds3 theBounds;
        theBounds.setEmpty();
        if (theNode.m_Type == GraphObjectTypes::Layer) {
            SNode *theEditLayer = m_Translation->GetEditCameraLayer();
            if (theEditLayer) {
                for (SNode *theChild = theEditLayer->m_FirstChild; theChild;
                     theChild = theChild->m_NextSibling) {
                    qt3ds::NVBounds3 childBounds = theChild->GetBounds(
                        m_UICContext->GetBufferManager(), m_UICContext->GetPathManager());
                    if (childBounds.isEmpty() == false) {
                        childBounds.transform(theChild->m_GlobalTransform);
                        theBounds.include(childBounds);
                    }
                }
            }
        } else
            theBounds =
                theNode.GetBounds(m_UICContext->GetBufferManager(), m_UICContext->GetPathManager());
        QT3DSVec3 theCenter = theNode.m_GlobalTransform.transform(theBounds.getCenter());
        // Center the edit camera so that it points directly at the bounds center point
        m_Translation->m_EditCameraInfo.m_Position = theCenter;
        // Now we need to adjust the camera's zoom such that the view frustum contains the bounding
        // box.
        // But to do that I need to figure out what the view frustum is at -600 units from the near
        // clip plane

        QT3DSVec3 theExtents = theBounds.getExtents();
        // get the largest extent and then some addition so things fit nicely in the viewport.
        QT3DSF32 theMaxPossibleRadius = theExtents.magnitude();
        // easiest case, the viewport dimensions map directly to the
        m_Translation->m_EditCameraInfo.m_ViewRadius = theMaxPossibleRadius;
        RequestRender();
    }

    // This must be safe to call from multiple places
    void Close() override
    {
        m_Closed = true;
        m_Translation = std::shared_ptr<STranslation>();
        m_UICContext = nullptr;
        m_RenderContext = std::shared_ptr<CWGLRenderContext>();
    }

    // Data model listener

    // Fired before a large group of notifications come out so views can
    // only refresh their view once.
    void OnBeginDataModelNotifications()  override {}
    // Fired after a large gruop of notifications (onInstancePropertyChanged, etc) come out
    // so views can be careful about refreshing their data and there view
    void OnEndDataModelNotifications()  override { Render(); }

    // Fired during 3d drag or mouse move events (or keyframe drag) or likewise
    // events so that views that need to update based on the new data can.
    void OnImmediateRefreshInstanceSingle(UICDM::CUICDMInstanceHandle inInstance) override
    {
        if (m_Translation) {
            m_Translation->MarkDirty(inInstance);
            // Pass to translation system
            Render();
        }
    }
    // Same thing, but fired when more than one instance is being refreshed.
    void OnImmediateRefreshInstanceMultiple(UICDM::CUICDMInstanceHandle *inInstance,
                                                    long inInstanceCount) override
    {
        // Pass to translation system
        if (m_Translation) {
            m_Translation->MarkDirty(inInstance, inInstanceCount);
            // Pass to translation system
            Render();
        }
        Render();
    }

    void OnReloadEffectInstance(UICDM::CUICDMInstanceHandle inInstance) override
    {
        if (m_Translation)
            m_Translation->ReleaseEffect(inInstance);
    }

    void ApplyEditCameraIndex()
    {
        if (!m_Translation)
            return;
        if (m_EditCameraIndex < 0 || m_EditCameraIndex >= (QT3DSI32)g_NumEditCameras)
            m_Translation->m_EditCameraEnabled = false;
        else {
            const SEditCameraDefinition &theDefinition(g_EditCameraDefinitions[m_EditCameraIndex]);

            while ((size_t)m_EditCameraIndex >= m_EditCameraInformation.size())
                m_EditCameraInformation.push_back(Empty());

            Option<SEditCameraPersistentInformation> &theCameraInfo =
                m_EditCameraInformation[m_EditCameraIndex];

            if (!theCameraInfo.hasValue()) {
                theCameraInfo = SEditCameraPersistentInformation();
                // TODO - consider resizing clip planes to scene so we use the depth buffer more
                // accurately
                // or consider requesting a larger depth buffer from the windowing system.
                // Setup the camera
                theCameraInfo->m_Direction = theDefinition.m_Direction;
                theCameraInfo->m_CameraType = theDefinition.m_Type;
            }

            m_Translation->m_EditCameraEnabled = true;
            m_Translation->m_EditCameraInfo = theCameraInfo;
        }
    }

    void SetTranslationViewport()
    {
        if (m_Translation) {
            m_Translation->SetViewport(QT3DSF32(m_Rect.right() - m_Rect.left()),
                                       QT3DSF32(m_Rect.bottom() - m_Rect.top()));
        }
    }

    void CreateTranslator()
    {
        if (!m_Translation) {
            if (m_UICContext.mPtr) {
                m_Translation = std::make_shared<STranslation>(std::ref(*this),
                                                                 std::ref(*m_UICContext.mPtr));
                m_Translation->m_EditLightEnabled = CStudioPreferences::GetEditViewFillMode();
                ApplyEditCameraIndex();
                SetTranslationViewport();
            }
        }
    }

    void SetupTextRenderer()
    {
        if (m_UICContext.mPtr && m_UICContext->GetTextRenderer()) {
            m_UICContext->GetTextRenderer()->ClearProjectFontDirectories();
            Q3DStudio::CString theDocDir = m_Doc.GetDocumentDirectory();
            if (theDocDir.Length()) {
                // Add the installed font folders from the res dir.
                Q3DStudio::CString thePath(Q3DStudio::CString::fromQString(
                                               resourcePath() + QStringLiteral("/Font")));
                m_UICContext->GetTextRenderer()->AddSystemFontDirectory(
                    m_UICContext->GetStringTable().RegisterStr(thePath.c_str()));
                m_UICContext->GetTextRenderer()->AddProjectFontDirectory(
                    m_UICContext->GetStringTable().RegisterStr(theDocDir.c_str()));
            }
        }
    }

    //==========================================================================
    /**
     * New presentation is being created.
     */
    void OnNewPresentation() override
    {
        OnClosingPresentation();
        m_HasPresentation = true;
        // Reset edit camera information.
        m_EditCameraInformation.clear();
        // Rebuild translation
        CreateTranslator();
        SetupTextRenderer();
        RequestRender();
    }

    //==========================================================================
    /**
     * The current presentation is being closed.
     */
    void OnClosingPresentation() override
    {
        // Destroy translation
        m_Translation = std::shared_ptr<STranslation>();
        m_HasPresentation = false;
    }

    void OnSelectionChange() { RequestRender(); }

    UICDM::CUICDMInstanceHandle GetAnchorPointFromPick(SPathPick &inPick)
    {
        return m_Translation->GetAnchorPoint(inPick);
    }

    //==========================================================================
    // CSceneDragListener
    //==========================================================================
    void OnSceneMouseDown(SceneDragSenderType::Enum inSenderType, QPoint inPoint, int) override
    {
        if (m_Translation == NULL)
            return;

        inPoint.setX(inPoint.x() * devicePixelRatio());
        inPoint.setY(inPoint.y() * devicePixelRatio());

        m_PickResult = SStudioPickValue();
        TranslationSelectMode::Enum theSelectMode = TranslationSelectMode::Group;
        switch (g_StudioApp.GetSelectMode()) {
        case STUDIO_SELECTMODE_ENTITY:
            theSelectMode = TranslationSelectMode::Single;
            break;
        case STUDIO_SELECTMODE_GROUP:
            theSelectMode = TranslationSelectMode::Group;
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
        if (inSenderType == SceneDragSenderType::SceneWindow
            && m_Translation->GetPickArea(inPoint) == PickTargetAreas::Presentation) {
            m_RenderContext->BeginRender();
            m_PickResult = m_Translation->Pick(inPoint, theSelectMode);
            m_RenderContext->EndRender();
            // If we definitely did not pick a widget.
            if (m_PickResult.getType() == StudioPickValueTypes::Instance) {
                UICDM::CUICDMInstanceHandle theHandle(m_PickResult.getData<CUICDMInstanceHandle>());

                if (theHandle != m_Doc.GetSelectedInstance())
                    m_Doc.SelectUICDMObject(theHandle);
            } else if (m_PickResult.getType() == StudioPickValueTypes::Guide)
                m_Doc.NotifySelectionChanged(m_PickResult.getData<UICDM::CUICDMGuideHandle>());
            else if (m_PickResult.getType() == StudioPickValueTypes::Path) {
                SPathPick thePick = m_PickResult.getData<SPathPick>();
                UICDM::CUICDMInstanceHandle theAnchorHandle =
                    m_Translation->GetAnchorPoint(thePick);
                if (theAnchorHandle.Valid() && theAnchorHandle != m_Doc.GetSelectedInstance()) {
                    m_Doc.SelectUICDMObject(theAnchorHandle);
                }
            }
            RequestRender();
        } else {
            qt3ds::foundation::Option<UICDM::SGuideInfo> pickResult =
                m_Translation->PickRulers(inPoint);
            if (pickResult.hasValue() == false)
                m_Translation->PrepareForDrag();
            else {
                Q3DStudio::IDocumentEditor &docEditor(
                    m_UpdatableEditor.EnsureEditor(L"Create Guide", __FILE__, __LINE__));
                CUICDMGuideHandle newGuide = docEditor.CreateGuide(*pickResult);
                m_PickResult = SStudioPickValue(newGuide);
                m_Doc.NotifySelectionChanged(newGuide);
            }
        }
        m_LastDragToolMode = MovementTypes::Unknown;
        m_MaybeDragStart = true;
        m_MouseDownPoint = inPoint;
        m_PreviousMousePoint = inPoint;
        m_MouseDownCameraInformation = m_Translation->m_EditCameraInfo;
        m_LastToolMode = g_StudioApp.GetToolMode();
    }

    void OnSceneMouseDrag(SceneDragSenderType::Enum, QPoint inPoint, int inToolMode,
                                  int inFlags) override
    {
        if (m_Translation == NULL)
            return;

        inPoint.setX(inPoint.x() * devicePixelRatio());
        inPoint.setY(inPoint.y() * devicePixelRatio());

        if (m_MaybeDragStart) {
            // Dragging in the first 5 pixels will be ignored to avoid unconsciously accidental
            // moves
            CPt theDragDistance = inPoint - m_MouseDownPoint;
            if (m_PickResult.getType() == StudioPickValueTypes::Widget
                || inToolMode != STUDIO_TOOLMODE_SCALE) {
                if (theDragDistance.x * theDragDistance.x + theDragDistance.y * theDragDistance.y
                    <= 25)
                    return;
            } else {
                if (abs(theDragDistance.y) <= 5)
                    return;
            }
        }

        m_MaybeDragStart = false;

        // If the tool mode changes then we throw out the last widget pick if there was one.
        if (m_LastToolMode != inToolMode)
            m_PickResult = SStudioPickValue();
        m_LastToolMode = inToolMode;

        // General dragging
        if (m_PickResult.getType() == StudioPickValueTypes::Instance
            || m_PickResult.getType()
                == StudioPickValueTypes::UnknownValueType) // matte drag and widget drag
        {
            // Not sure what right-click drag does in the scene.
            bool isEditCamera = m_Translation->m_EditCameraEnabled;
            int theCameraToolMode = isEditCamera ? (inToolMode & (STUDIO_CAMERATOOL_MASK)) : 0;
            bool rightClick = (inFlags & CHotKeys::MOUSE_RBUTTON) != 0;

            if (theCameraToolMode == 0) {
                if (m_Doc.GetDocumentReader().IsInstance(m_Doc.GetSelectedInstance())) {
                    bool rightClick = (inFlags & CHotKeys::MOUSE_RBUTTON) != 0;
                    MovementTypes::Enum theMovement(MovementTypes::Unknown);

                    switch (inToolMode) {
                    default:
                        QT3DS_ASSERT(false);
                        break;
                    case STUDIO_TOOLMODE_MOVE:
                        if (rightClick)
                            theMovement = MovementTypes::TranslateAlongCameraDirection;
                        else
                            theMovement = MovementTypes::Translate;
                        break;
                    case STUDIO_TOOLMODE_SCALE:
                        if (rightClick)
                            theMovement = MovementTypes::ScaleZ;
                        else
                            theMovement = MovementTypes::Scale;
                        break;
                    case STUDIO_TOOLMODE_ROTATE:
                        if (rightClick)
                            theMovement = MovementTypes::RotationAboutCameraDirection;
                        else
                            theMovement = MovementTypes::Rotation;
                        break;
                    }

                    if (theMovement != MovementTypes::Unknown) {
                        bool theLockToAxis = (inFlags & CHotKeys::MODIFIER_SHIFT) != 0;

                        if (m_LastDragToolMode != MovementTypes::Unknown
                            && theMovement != m_LastDragToolMode) {
                            m_UpdatableEditor.RollbackEditor();
                            m_MouseDownPoint = inPoint;
                        }

                        m_LastDragToolMode = theMovement;

                        switch (theMovement) {
                        case MovementTypes::TranslateAlongCameraDirection:
                            m_Translation->TranslateSelectedInstanceAlongCameraDirection(
                                m_MouseDownPoint, inPoint, m_UpdatableEditor);
                            break;
                        case MovementTypes::Translate:
                            m_Translation->TranslateSelectedInstance(
                                m_MouseDownPoint, inPoint, m_UpdatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::ScaleZ:
                            m_Translation->ScaleSelectedInstanceZ(m_MouseDownPoint, inPoint,
                                                                  m_UpdatableEditor);
                            break;
                        case MovementTypes::Scale:
                            m_Translation->ScaleSelectedInstance(m_MouseDownPoint, inPoint,
                                                                 m_UpdatableEditor);
                            break;
                        case MovementTypes::Rotation:
                            m_Translation->RotateSelectedInstance(m_MouseDownPoint,
                                                                  m_PreviousMousePoint, inPoint,
                                                                  m_UpdatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::RotationAboutCameraDirection:
                            m_Translation->RotateSelectedInstanceAboutCameraDirectionVector(
                                m_PreviousMousePoint, inPoint, m_UpdatableEditor);
                            break;
                        }
                    }
                }
            } else {
                QT3DSF32 theXDistance = static_cast<QT3DSF32>(inPoint.x() - m_MouseDownPoint.x);
                QT3DSF32 theYDistance = static_cast<QT3DSF32>(inPoint.y() - m_MouseDownPoint.y);
                QT3DSF32 theSubsetXDistance = static_cast<QT3DSF32>(inPoint.x() - m_PreviousMousePoint.x);
                QT3DSF32 theSubsetYDistance = static_cast<QT3DSF32>(inPoint.y() - m_PreviousMousePoint.y);

                // Edit cameras are not implemented.
                switch (theCameraToolMode) {
                case STUDIO_TOOLMODE_CAMERA_PAN: {
                    QT3DSVec3 theXAxis =
                        m_Translation->m_EditCamera.m_GlobalTransform.column0.getXYZ();
                    QT3DSVec3 theYAxis =
                        m_Translation->m_EditCamera.m_GlobalTransform.column1.getXYZ();
                    QT3DSVec3 theXChange = -1.0f * theXAxis * theXDistance;
                    QT3DSVec3 theYChange = theYAxis * theYDistance;
                    QT3DSVec3 theDiff = theXChange + theYChange;
                    m_Translation->m_EditCameraInfo.m_Position =
                        m_MouseDownCameraInformation.m_Position + theDiff;
                    RequestRender();
                } break;
                case STUDIO_TOOLMODE_CAMERA_ZOOM: {
                    QT3DSF32 theMultiplier = 1.0f + theSubsetYDistance / 40.0f;
                    m_Translation->m_EditCameraInfo.m_ViewRadius =
                        NVMax(.0001f, m_Translation->m_EditCameraInfo.m_ViewRadius * theMultiplier);
                    RequestRender();
                } break;
                case STUDIO_TOOLMODE_CAMERA_ROTATE: {
                    if (m_Translation->m_EditCameraInfo.SupportsRotation()) {
                        if (rightClick == false) {
                            QT3DSVec3 theXAxis = QT3DSVec3(1, 0, 0);
                            QT3DSVec3 theYAxis = QT3DSVec3(0, 1, 0);
                            // Rotate about the center; we will just rotation the direction vector.
                            QT3DSQuat theXRotation(-1.0f * theSubsetXDistance * g_RotationScaleFactor
                                                    / 20.0f,
                                                theYAxis);
                            QT3DSQuat theYRotation(-1.0f * theSubsetYDistance * g_RotationScaleFactor
                                                    / 20.0f,
                                                theXAxis);
                            m_Translation->m_EditCameraInfo.m_Rotation =
                                m_MouseDownCameraInformation.m_Rotation
                                * (theXRotation * theYRotation);
                        } else {
                            QT3DSVec3 theZAxis = QT3DSVec3(0, 0, 1);
                            QT3DSQuat theZRotation(
                                -1.0f * theYDistance * g_RotationScaleFactor / 20.0f, theZAxis);
                            m_Translation->m_EditCameraInfo.m_Rotation =
                                m_MouseDownCameraInformation.m_Rotation * theZRotation;
                        }
                        // Rotations need to be incremental and relative else things don't rotate
                        // intuitively.
                        m_MouseDownCameraInformation.m_Rotation =
                            m_Translation->m_EditCameraInfo.m_Rotation;
                        RequestRender();
                    }
                } break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }
        } // if ( m_PickResult.m_WidgetId.hasValue() == false )

        // We need to do widget-specific dragging.
        else if (m_PickResult.getType() == StudioPickValueTypes::Widget) {
            m_Translation->PerformWidgetDrag(m_PickResult.GetWidgetId(), m_MouseDownPoint,
                                             m_PreviousMousePoint, inPoint, m_UpdatableEditor);
        } else if (m_PickResult.getType() == StudioPickValueTypes::Guide) {
            m_Translation->PerformGuideDrag(m_PickResult.getData<CUICDMGuideHandle>(), inPoint,
                                            m_UpdatableEditor);
        } else if (m_PickResult.getType() == StudioPickValueTypes::Path) {
            SPathPick thePick = m_PickResult.getData<SPathPick>();
            m_Translation->PerformPathDrag(thePick, m_MouseDownPoint, m_PreviousMousePoint, inPoint,
                                           m_UpdatableEditor);
        }
        m_PreviousMousePoint = inPoint;
    }

    void OnSceneMouseUp(SceneDragSenderType::Enum) override
    {
        m_MaybeDragStart = false;
        CUICDMGuideHandle theSelectedGuide;
        if (m_PickResult.getType() == StudioPickValueTypes::Guide) {
            theSelectedGuide = m_PickResult.getData<CUICDMGuideHandle>();
            m_Translation->CheckGuideInPresentationRect(theSelectedGuide, m_UpdatableEditor);
        }
        m_UpdatableEditor.CommitEditor();
        m_PickResult = SStudioPickValue();
        if (m_Translation)
            m_Translation->EndDrag();
        if (theSelectedGuide.GetHandleValue()) {
            // Get rid of selection if things aren't editable.
            if (m_Doc.GetDocumentReader().AreGuidesEditable())
                m_Doc.NotifySelectionChanged(theSelectedGuide);
            else
                m_Doc.NotifySelectionChanged();
        }
        RequestRender();
    }

    void OnSceneMouseDblClick(SceneDragSenderType::Enum inSenderType, QPoint inPoint) override
    {
        if (inSenderType == SceneDragSenderType::SceneWindow && m_Translation) {
            inPoint.setX(inPoint.x() * devicePixelRatio());
            inPoint.setY(inPoint.y() * devicePixelRatio());
            m_RenderContext->BeginRender();
            SStudioPickValue theResult(
                m_Translation->Pick(inPoint, TranslationSelectMode::NestedComponentSingle));
            m_RenderContext->EndRender();

            if (theResult.getType() == StudioPickValueTypes::Instance)
                m_Doc.SelectAndNavigateToUICDMObject(theResult.getData<CUICDMInstanceHandle>());
            else if (theResult.getType() == StudioPickValueTypes::Path) {
                SPathPick thePickValue = theResult.getData<SPathPick>();
                UICDM::CUICDMInstanceHandle theAnchorHandle =
                    m_Translation->GetAnchorPoint(thePickValue);
                if (theAnchorHandle.Valid() && theAnchorHandle != m_Doc.GetSelectedInstance()) {
                    m_Doc.SelectUICDMObject(theAnchorHandle);
                }
            }
        }
    }

    void OnSceneMouseWheel(SceneDragSenderType::Enum inSenderType, short inDelta,
                                   int inToolMode) override
    {
        ASSERT(inSenderType == SceneDragSenderType::Matte);
        if (inToolMode == STUDIO_TOOLMODE_CAMERA_ZOOM && m_Translation) {
            QT3DSF32 theMultiplier = 1.0f - inDelta / static_cast<QT3DSF32>(120 * g_WheelFactor);
            m_Translation->m_EditCameraInfo.m_ViewRadius =
                NVMax(.0001f, m_Translation->m_EditCameraInfo.m_ViewRadius * theMultiplier);
            RequestRender();
        }
    }

    void OnNudge(ENudgeDirection inNudgeDirection, int inToolMode, int inFlags) override
    {
        if (m_Translation)
            m_Translation->OnNudge(inNudgeDirection, inToolMode, inFlags, m_UpdatableEditor);
    }

    void OnNudgeDone() override
    {
        if (m_Translation)
            m_Translation->OnNudgeFinished();
        m_UpdatableEditor.CommitEditor();
    }

    void OnToolbarChange()  override { RequestRender(); }
};
}

std::shared_ptr<IStudioRenderer> IStudioRenderer::CreateStudioRenderer()
{
    return std::make_shared<SRendererImpl>();
}
