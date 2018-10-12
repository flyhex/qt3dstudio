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
#include "Qt3DSCommonPrecompile.h"
#include "StudioRendererImpl.h"
#include "StudioRendererTranslation.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "StudioUtils.h"
#include "Qt3DSMath.h"
#include "Qt3DSOffscreenRenderKey.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "q3dsqmlrender.h"
#include "q3dsqmlstreamproxy.h"
#include "StudioSubPresentationRenderer.h"

#include <QtCore/qdebug.h>

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

struct StudioSubPresentation
{
    SubPresentationRecord subpresentation;
    IOffscreenRenderer *renderer;

    bool operator == (const SubPresentationRecord &r) const
    {
        return r.m_id == subpresentation.m_id &&
               r.m_argsOrSrc == subpresentation.m_argsOrSrc &&
               r.m_type == subpresentation.m_type;
    }
};

struct SRendererImpl : public IStudioRenderer,
                       public IDataModelListener,
                       public IReloadListener,
                       public CPresentationChangeListener,
                       public CSceneDragListener,
                       public CToolbarChangeListener,
                       public IOffscreenRenderer::IOffscreenRendererCallback
{
    typedef eastl::vector<Option<SEditCameraPersistentInformation>> TEditCameraInfoList;
    std::shared_ptr<CWGLRenderContext> m_RenderContext;
    NVScopedRefCounted<IQt3DSRenderContext> m_Context;
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
    qt3dsdm::TSignalConnectionPtr m_SelectionSignal;
    float m_pixelRatio;
    QHash<QString, StudioSubPresentation> m_subpresentations;
    QScopedPointer<Q3DSQmlStreamProxy> m_proxy;
    QMap<QString, int> m_initialFrameMap;

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
        , m_pixelRatio(0.0)
    {
        m_Dispatch.AddReloadListener(this);
        m_Dispatch.AddDataModelListener(this);
        m_Dispatch.AddPresentationChangeListener(this);
        m_SelectionSignal =
            m_Dispatch.ConnectSelectionChange(std::bind(&SRendererImpl::OnSelectionChange, this));
        m_Dispatch.AddSceneDragListener(this);
        m_Dispatch.AddToolbarChangeListener(this);
    }
    ~SRendererImpl() override
    {
        Close();
        m_Dispatch.RemoveDataModelListener(this);
        m_Dispatch.RemovePresentationChangeListener(this);
        m_Dispatch.RemoveSceneDragListener(this);
        m_Dispatch.RemoveToolbarChangeListener(this);
    }

    // IDocSceneGraph
    QT3DSVec3 GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle, CPt inPoint) override
    {
        if (m_Translation)
            return m_Translation->GetIntendedPosition(inHandle, inPoint);

        return QT3DSVec3(0, 0, 0);
    }

    void RegisterSubpresentations(const QVector<SubPresentationRecord> &subpresentations) override
    {
        if (m_proxy.isNull())
            m_proxy.reset(new Q3DSQmlStreamProxy());
        IOffscreenRenderManager &offscreenMgr(m_Context->GetOffscreenRenderManager());
        const QString projectPath = m_Doc.GetCore()->getProjectFile().getProjectPath();
        // setPath expects full path, but strips the filename
        m_proxy->setPath(projectPath + "/dummy.uip");
        QVector<SubPresentationRecord> toUnregister;
        QVector<SubPresentationRecord> toRegister;
        const auto keys = m_subpresentations.keys();
        for (QString key : keys) {
            if (!subpresentations.contains(m_subpresentations[key].subpresentation))
                toUnregister.append(m_subpresentations[key].subpresentation);
        }

        for (int i = 0; i < subpresentations.size(); ++i) {
            if (!m_subpresentations.contains(subpresentations[i].m_id)
                    || !(m_subpresentations[subpresentations[i].m_id] == subpresentations[i])) {
                toRegister.append(subpresentations[i]);
            }
        }

        for (int i = 0; i < toUnregister.size(); ++i) {
            QByteArray data = toUnregister[i].m_id.toLocal8Bit();
            qt3ds::render::CRegisteredString rid
                    = m_Context->GetStringTable().RegisterStr(data.data());
            offscreenMgr.ReleaseOffscreenRenderer(qt3ds::render::SOffscreenRendererKey(rid));
            m_subpresentations.remove(toUnregister[i].m_id);
            m_proxy->unregisterPresentation(toUnregister[i].m_id);
        }

        for (int i = 0; i < toRegister.size(); ++i) {
            QByteArray data = toRegister[i].m_id.toLocal8Bit();
            qt3ds::render::CRegisteredString rid
                    = m_Context->GetStringTable().RegisterStr(data.data());
            if (toRegister[i].m_type == QStringLiteral("presentation-qml")) {
                m_proxy->registerPresentation(toRegister[i].m_id, toRegister[i].m_argsOrSrc);

                qt3ds::render::IOffscreenRenderer *theOffscreenRenderer =
                    QT3DS_NEW(m_Context->GetAllocator(),
                           Q3DSQmlRender)(*m_Context, data.data());
                offscreenMgr.RegisterOffscreenRenderer(
                            qt3ds::render::SOffscreenRendererKey(rid), *theOffscreenRenderer);
                m_subpresentations[toRegister[i].m_id].renderer = theOffscreenRenderer;
                theOffscreenRenderer->addCallback(this);
            } else {
                qt3ds::render::IOffscreenRenderer *theOffscreenRenderer =
                    QT3DS_NEW(m_Context->GetAllocator(),
                           StudioSubpresentationRenderer)(*m_Context, toRegister[i].m_id,
                                                          toRegister[i].m_argsOrSrc,
                                                          projectPath);
                offscreenMgr.RegisterOffscreenRenderer(
                            qt3ds::render::SOffscreenRendererKey(rid), *theOffscreenRenderer);
                m_subpresentations[toRegister[i].m_id].renderer = theOffscreenRenderer;
                theOffscreenRenderer->addCallback(this);
            }
            m_subpresentations[toRegister[i].m_id].subpresentation = toRegister[i];
        }
        // Process qml proxy events so that we have initialized the qml producer,
        // then get the desired environment to initialize the qml renderer.
        QCoreApplication::processEvents();
        for (int i = 0; i < toRegister.size(); ++i) {
            QByteArray data = toRegister[i].m_id.toLocal8Bit();
            if (toRegister[i].m_type == QLatin1String("presentation-qml"))
                m_subpresentations[toRegister[i].m_id].renderer
                        ->GetDesiredEnvironment(QT3DSVec2(1.0f, 1.0f));
        }
        RequestRender();
    }

    void ReleaseOffscreenRenderersForSubpresentations()
    {
        if (!m_Context.mPtr)
            return;

        IOffscreenRenderManager &offscreenMgr(m_Context->GetOffscreenRenderManager());

        QVector<SubPresentationRecord> toUnregister;

        const auto keys = m_subpresentations.keys();
        for (QString key : keys)
            toUnregister.append(m_subpresentations[key].subpresentation);

        for (int i = 0; i < toUnregister.size(); ++i) {
            QByteArray data = toUnregister[i].m_id.toLocal8Bit();
            qt3ds::render::CRegisteredString rid
                    = m_Context->GetStringTable().RegisterStr(data.data());
            offscreenMgr.ReleaseOffscreenRenderer(qt3ds::render::SOffscreenRendererKey(rid));
        }
    }

    void onOffscreenRendererInitialized(const QString &id) override
    {
        // Request render after first frame rendered by the offscreen renderer
        m_initialFrameMap[id] = 1;
    }

    void onOffscreenRendererFrame(const QString &id) override
    {
        if (m_initialFrameMap.contains(id)) {
            RequestRender();
            m_initialFrameMap.remove(id);
        }
    }

    ITextRenderer *GetTextRenderer() override
    {
        if (m_Context.mPtr)
            return m_Context->GetTextRenderer();
        return nullptr;
    }
    // The buffer manager may not be available
    IBufferManager *GetBufferManager() override
    {
        if (m_Context.mPtr)
            return &m_Context->GetBufferManager();
        return nullptr;
    }

    IPathManager *GetPathManager() override
    {
        if (m_Context.mPtr)
            return &m_Context->GetPathManager();
        return nullptr;
    }

    qt3ds::foundation::IStringTable *GetRenderStringTable() override
    {
        if (m_Context.mPtr)
            return &m_Context->GetStringTable();
        return nullptr;
    }

    bool IsInitialized() override { return m_Context.mPtr != nullptr; }

    void Initialize(QWidget *inWindow) override
    {
        if (m_Closed)
            return;
        QT3DS_ASSERT(!m_RenderContext);
        QT3DS_ASSERT(m_Context.mPtr == nullptr);
        try {
            m_RenderContext = std::make_shared<CWGLRenderContext>(inWindow);

            Q3DStudio::CString theResourcePath = Q3DStudio::CString::fromQString(resourcePath());
            NVScopedRefCounted<qt3ds::render::IQt3DSRenderContextCore> theCore =
                qt3ds::render::IQt3DSRenderContextCore::Create(
                    m_RenderContext->GetRenderContext().GetFoundation(),
                    m_RenderContext->GetRenderContext().GetStringTable());

            // Create text renderer
            qt3ds::render::ITextRendererCore &theTextRenderer(
                        qt3ds::render::ITextRendererCore::CreateQtTextRenderer(
                            m_RenderContext->GetRenderContext().GetFoundation(),
                            m_RenderContext->GetRenderContext().GetStringTable()));
            theCore->SetTextRendererCore(theTextRenderer);

            m_Context = theCore->CreateRenderContext(
                m_RenderContext->GetRenderContext(),
                m_RenderContext->GetRenderContext().GetStringTable().RegisterStr(
                    theResourcePath.c_str()));

            // Allow the artist to interact with the top level objects alone.
            m_Context->GetRenderer().PickRenderPlugins(false);

            SetupTextRenderer();

            m_Context->SetAuthoringMode(true);

            InitializePointerTags(m_Context->GetStringTable());
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
            m_Context = nullptr;
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
            m_pixelRatio = devicePixelRatio();
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

    void ReleaseContext() override
    {
        m_RenderContext->EndRender();
    }

    void Render()
    {
        m_RenderRequested = false;
        if (!m_Closed && IsInitialized()) {
            m_RenderContext->BeginRender();
            bool preview = false;
            if (m_Translation) {
                preview = CStudioPreferences::showEditModePreview()
                        && m_Translation->m_EditCameraEnabled
                        && !m_Translation->GetPreviewViewportDimensions().isZero();
                m_Translation->PreRender(preview);
            }
            NVRenderContext &theContext = m_RenderContext->GetRenderContext();
            theContext.SetDepthWriteEnabled(true);
            theContext.Clear(qt3ds::render::NVRenderClearFlags(
                                 qt3ds::render::NVRenderClearValues::Color
                                 | qt3ds::render::NVRenderClearValues::Depth));
            if (m_Translation) {
                // draw scene preview view screen display area layer
                if (preview) {
                    m_Translation->Render(0, false, true, false);
                    m_Translation->PreRender(false);
                }
                m_Translation->Render(m_PickResult.GetWidgetId(), m_GuidesEnabled, false, preview);
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
    void SetPolygonFillModeEnabled(bool inEnableLight) override
    {
        CStudioPreferences::SetEditViewFillMode(inEnableLight);
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
        m_EditCameraIndex = qMin(inIndex, (QT3DSI32)g_NumEditCameras);
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

    bool IsPolygonFillModeEnabled() const override
    {
        return GetEditCamera() >= 0 && CStudioPreferences::GetEditViewFillMode();
    }

    void EditCameraZoomToFit() override
    {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance = m_Doc.GetSelectedInstance();
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
                == false) {
            theInstance = m_Doc.GetActiveLayer();
        }

        // If we *still* aren't pointed at a node then bail.
        if (m_Translation->GetOrCreateTranslator(theInstance) == nullptr
            || GraphObjectTypes::IsNodeType(
                   m_Translation->GetOrCreateTranslator(theInstance)->GetGraphObject().m_Type)
                == false) {
            return;
        }

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
                        m_Context->GetBufferManager(), m_Context->GetPathManager());
                    if (childBounds.isEmpty() == false) {
                        childBounds.transform(theChild->m_GlobalTransform);
                        theBounds.include(childBounds);
                    }
                }
            }
        } else {
            theBounds =
                theNode.GetBounds(m_Context->GetBufferManager(), m_Context->GetPathManager());
        }

        // Fake bounds for non-physical objects
        if (theBounds.isEmpty()) {
            const int dim = 50.0f; // Dimensions of a default sized cube
            theBounds = qt3ds::NVBounds3(QT3DSVec3(-dim, -dim, -dim), QT3DSVec3(dim, dim, dim));
        }

        // Empty groups don't have proper global transform, so we need to recalculate it.
        // For simplicity's sake, we recalculate for all groups, not just empty ones.
        if (theNode.m_Type == GraphObjectTypes::Node)
            theNode.CalculateGlobalVariables();

        QT3DSVec3 theCenter = theNode.m_GlobalTransform.transform(theBounds.getCenter());

        // Center the edit camera so that it points directly at the bounds center point
        m_Translation->m_EditCameraInfo.m_Position = theCenter;
        // Now we need to adjust the camera's zoom such that the view frustum contains the bounding
        // box.
        // But to do that I need to figure out what the view frustum is at -600 units from the near
        // clip plane

        QT3DSVec3 theExtents = theBounds.getExtents().multiply(theNode.m_Scale);

        // get the largest extent and then some addition so things fit nicely in the viewport.
        QT3DSF32 theMaxPossibleRadius = theExtents.magnitude();

        // easiest case, the viewport dimensions map directly to the
        m_Translation->m_EditCameraInfo.m_ViewRadius = theMaxPossibleRadius;
        RequestRender();
    }

    // This must be safe to call from multiple places
    void Close() override
    {
        ReleaseOffscreenRenderersForSubpresentations();
        m_subpresentations.clear();
        m_proxy.reset();
        m_Closed = true;
        m_Translation = std::shared_ptr<STranslation>();
        m_Context = nullptr;
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
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override
    {
        if (m_Translation) {
            m_Translation->MarkDirty(inInstance);
            // Pass to translation system
            Render();
        }
    }
    // Same thing, but fired when more than one instance is being refreshed.
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
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

    void OnReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override
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
                QT3DSVec3 normalizedDir = theDefinition.m_Direction;
                normalizedDir.normalize();
                if (theDefinition.m_Type == EditCameraTypes::Directional) {
                    theCameraInfo->m_Direction = normalizedDir;
                } else {
                    theCameraInfo->m_Direction = QT3DSVec3(0, 0, -1);
                    theCameraInfo->m_xRotation = -qt3ds::NVAtan(normalizedDir.x / normalizedDir.z);
                    theCameraInfo->m_yRotation = qt3ds::NVAsin(normalizedDir.y);
                }
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
            if (m_Context.mPtr) {
                m_Translation = std::make_shared<STranslation>(std::ref(*this),
                                                                 std::ref(*m_Context.mPtr));
                ApplyEditCameraIndex();
                SetTranslationViewport();
            }
        }
    }

    void SetupTextRenderer()
    {
        if (m_Context.mPtr && m_Context->GetTextRenderer()) {
            m_Context->GetTextRenderer()->ClearProjectFontDirectories();
            QString projectPath = g_StudioApp.GetCore()->getProjectFile().getProjectPath();
            if (!projectPath.isEmpty()) {
                // Add the installed font folders from the res dir.
                Q3DStudio::CString thePath(Q3DStudio::CString::fromQString(
                                               resourcePath() + QStringLiteral("/Font")));
                m_Context->GetTextRenderer()->AddSystemFontDirectory(
                    m_Context->GetStringTable().RegisterStr(thePath.c_str()));
                m_Context->GetTextRenderer()->AddProjectFontDirectory(
                    m_Context->GetStringTable().RegisterStr(projectPath.toLatin1().data()));
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
        m_proxy.reset();
        ReleaseOffscreenRenderersForSubpresentations();
        m_subpresentations.clear();
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

    qt3dsdm::Qt3DSDMInstanceHandle GetAnchorPointFromPick(SPathPick &inPick)
    {
        return m_Translation->GetAnchorPoint(inPick);
    }

    //==========================================================================
    // CSceneDragListener
    //==========================================================================
    void OnSceneMouseDown(SceneDragSenderType::Enum inSenderType, QPoint inPoint, int) override
    {
        if (m_Translation == nullptr)
            return;

        inPoint.setX(inPoint.x() * m_pixelRatio);
        inPoint.setY(inPoint.y() * m_pixelRatio);

        m_PickResult = SStudioPickValue();
        if (inSenderType == SceneDragSenderType::SceneWindow) {
            PickTargetAreas::Enum pickArea = m_Translation->GetPickArea(inPoint);
            if (pickArea == PickTargetAreas::Presentation) {
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
                m_RenderContext->BeginRender();
                m_PickResult = m_Translation->Pick(inPoint, theSelectMode);
                m_RenderContext->EndRender();
                // If we definitely did not pick a widget.
                if (m_PickResult.getType() == StudioPickValueTypes::Instance) {
                    qt3dsdm::Qt3DSDMInstanceHandle theHandle(
                                m_PickResult.getData<Qt3DSDMInstanceHandle>());
                    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                        m_Doc.ToggleDataModelObjectToSelection(theHandle);
                    } else {
                        if (m_Doc.getSelectedInstancesCount() > 1)
                            m_Doc.DeselectAllItems(true);

                        if (theHandle != m_Doc.GetSelectedInstance())
                            m_Doc.SelectDataModelObject(theHandle);
                    }
                } else if (m_PickResult.getType() == StudioPickValueTypes::Guide) {
                    m_Doc.NotifySelectionChanged(
                                m_PickResult.getData<qt3dsdm::Qt3DSDMGuideHandle>());
                } else if (m_PickResult.getType() == StudioPickValueTypes::Path) {
                    SPathPick thePick = m_PickResult.getData<SPathPick>();
                    qt3dsdm::Qt3DSDMInstanceHandle theAnchorHandle =
                            m_Translation->GetAnchorPoint(thePick);
                    if (theAnchorHandle.Valid() && theAnchorHandle != m_Doc.GetSelectedInstance())
                        m_Doc.SelectDataModelObject(theAnchorHandle);
                } else if (m_PickResult.getType() == StudioPickValueTypes::UnknownValueType) {
                    m_Doc.DeselectAllItems(true);
                }
                RequestRender();
            } else if (pickArea == PickTargetAreas::Matte) {
                qt3ds::foundation::Option<qt3dsdm::SGuideInfo> pickResult =
                        m_Translation->PickRulers(inPoint);
                if (pickResult.hasValue()) {
                    Q3DStudio::IDocumentEditor &docEditor(
                                m_UpdatableEditor.EnsureEditor(QObject::tr("Create Guide"),
                                                               __FILE__, __LINE__));
                    Qt3DSDMGuideHandle newGuide = docEditor.CreateGuide(*pickResult);
                    m_PickResult = SStudioPickValue(newGuide);
                    m_Doc.NotifySelectionChanged(newGuide);
                } else {
                    m_Doc.DeselectAllItems(true);
                }
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
        if (m_Translation == nullptr)
            return;

        inPoint.setX(inPoint.x() * m_pixelRatio);
        inPoint.setY(inPoint.y() * m_pixelRatio);

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
                if (qAbs(theDragDistance.y) <= 5)
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
                    if (m_Doc.getSelectedInstancesCount() == 1) {
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
                                m_Translation->RotateSelectedInstance(
                                            m_MouseDownPoint, m_PreviousMousePoint, inPoint,
                                            m_UpdatableEditor, theLockToAxis);
                                break;
                            case MovementTypes::RotationAboutCameraDirection:
                                m_Translation->RotateSelectedInstanceAboutCameraDirectionVector(
                                    m_PreviousMousePoint, inPoint, m_UpdatableEditor);
                                break;
                            default:
                                break;
                            }
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
                        qMax(.0001f, m_Translation->m_EditCameraInfo.m_ViewRadius * theMultiplier);
                    RequestRender();
                } break;
                case STUDIO_TOOLMODE_CAMERA_ROTATE: {
                    if (m_Translation->m_EditCameraInfo.SupportsRotation()) {
                        if (!rightClick) {
                            m_Translation->m_EditCameraInfo.m_xRotation =
                                    m_MouseDownCameraInformation.m_xRotation
                                    + (theSubsetXDistance * g_RotationScaleFactor / 20.0f);
                            m_Translation->m_EditCameraInfo.m_yRotation =
                                    m_MouseDownCameraInformation.m_yRotation
                                    - (theSubsetYDistance * g_RotationScaleFactor / 20.0f);
                            // Avoid rounding errors stemming from extremely large rotation angles
                            if (m_Translation->m_EditCameraInfo.m_xRotation < -qt3ds::NVPi)
                                m_Translation->m_EditCameraInfo.m_xRotation += qt3ds::NVPi * 2.0f;
                            if (m_Translation->m_EditCameraInfo.m_xRotation > qt3ds::NVPi)
                                m_Translation->m_EditCameraInfo.m_xRotation -= qt3ds::NVPi * 2.0f;
                            if (m_Translation->m_EditCameraInfo.m_yRotation < -qt3ds::NVPi)
                                m_Translation->m_EditCameraInfo.m_yRotation += qt3ds::NVPi * 2.0f;
                            if (m_Translation->m_EditCameraInfo.m_yRotation > qt3ds::NVPi)
                                m_Translation->m_EditCameraInfo.m_yRotation -= qt3ds::NVPi * 2.0f;
                        }
                        m_MouseDownCameraInformation.m_xRotation =
                            m_Translation->m_EditCameraInfo.m_xRotation;
                        m_MouseDownCameraInformation.m_yRotation =
                            m_Translation->m_EditCameraInfo.m_yRotation;
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
            m_Translation->PerformGuideDrag(m_PickResult.getData<Qt3DSDMGuideHandle>(), inPoint,
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
        Qt3DSDMGuideHandle theSelectedGuide;
        if (m_PickResult.getType() == StudioPickValueTypes::Guide) {
            theSelectedGuide = m_PickResult.getData<Qt3DSDMGuideHandle>();
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
            inPoint.setX(inPoint.x() * m_pixelRatio);
            inPoint.setY(inPoint.y() * m_pixelRatio);
            m_RenderContext->BeginRender();
            SStudioPickValue theResult(
                m_Translation->Pick(inPoint, TranslationSelectMode::NestedComponentSingle));
            m_RenderContext->EndRender();

            if (theResult.getType() == StudioPickValueTypes::Instance)
                m_Doc.SelectAndNavigateToDataModelObject(theResult.getData<Qt3DSDMInstanceHandle>());
            else if (theResult.getType() == StudioPickValueTypes::Path) {
                SPathPick thePickValue = theResult.getData<SPathPick>();
                qt3dsdm::Qt3DSDMInstanceHandle theAnchorHandle =
                    m_Translation->GetAnchorPoint(thePickValue);
                if (theAnchorHandle.Valid() && theAnchorHandle != m_Doc.GetSelectedInstance()) {
                    m_Doc.SelectDataModelObject(theAnchorHandle);
                }
            }
        }
    }

    void OnSceneMouseWheel(SceneDragSenderType::Enum inSenderType, short inDelta,
                                   int inToolMode) override
    {
        Q_ASSERT(inSenderType == SceneDragSenderType::Matte);
        if (inToolMode == STUDIO_TOOLMODE_CAMERA_ZOOM && m_Translation) {
            QT3DSF32 theMultiplier = 1.0f - inDelta / static_cast<QT3DSF32>(120 * g_WheelFactor);
            m_Translation->m_EditCameraInfo.m_ViewRadius =
                qMax(.0001f, m_Translation->m_EditCameraInfo.m_ViewRadius * theMultiplier);
            RequestRender();
        }
    }

    void OnToolbarChange()  override { RequestRender(); }
};
}

std::shared_ptr<IStudioRenderer> IStudioRenderer::CreateStudioRenderer()
{
    return std::make_shared<SRendererImpl>();
}
