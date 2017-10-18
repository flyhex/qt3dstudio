/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSTegraApplication.h"
#include "Qt3DSTegraInputEngine.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSThreadManager.h"
#include "Qt3DSArray.h"
#include "Qt3DSApplication.h"
#include "foundation/FileTools.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSPresentation.h"
#include "EASTL/string.h"
#include "Qt3DSMemory.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderer.h"

#include "Qt3DSDLLManager.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/TrackingAllocator.h"
// For perf log timestamp
#include <time.h>
#include "Qt3DSArray.h"
// For perf log timestamp
#include <time.h>

#if Q_OS_WINDOWS
#include "qt3ds_launcher_defs.h"
#endif

#ifdef _LINUXPLATFORM
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include <QCoreApplication>
#include <QSurfaceFormat>

//==============================================================================
//	Namespace
//==============================================================================
namespace qt3ds {
namespace render {
extern qt3ds::foundation::MallocAllocator g_BaseAllocator;
}
}

namespace Q3DStudio {

namespace {

bool CaselessEqual(const char *lhs, const char *rhs)
{
    if (lhs == NULL)
        lhs = "";
    if (rhs == NULL)
        rhs = "";
    return Q3DStudio_stricmp(lhs, rhs) == 0;
}

CInputEngine *CreateInputEngine()
{
    return Q3DStudio_virtual_new(CTegraInputEngine) CTegraInputEngine();
}

static eastl::string *theAppDir = nullptr;
const eastl::string &GetAppDir(const eastl::string &inAppExe)
{
    if (!theAppDir)
        theAppDir = new eastl::string;
#ifndef __ANDROID__
    theAppDir->assign(inAppExe.empty() == false ? inAppExe : "");
#ifdef Qt3DS_OS_LINUX
    char theBuf[1024] = { 0 };
    int rc = readlink("/proc/self/exe", theBuf, sizeof(theBuf));
    if (rc > 0)
        theAppDir->assign(theBuf);
#endif
#ifdef Qt3DS_OS_QNX
    char theBuf[1024] = { 0 };
    FILE *exefile = fopen("/proc/self/exefile", "r");
    if (exefile != NULL) {
        fgets(theBuf, sizeof(theBuf), exefile);
        fclose(exefile);
        theAppDir->assign(theBuf);
    }
#endif
    eastl::string::size_type pos = theAppDir->find_last_of("\\/");
    if (pos != eastl::string::npos)
        *theAppDir = theAppDir->substr(0, pos);
    theAppDir->append("\\");
#endif
    return *theAppDir;
}
}

using namespace qt3ds;
using namespace qt3ds::foundation;

//==============================================================================
/**
 *	CNDDView
 */
class CNDDView : public INDDView
{
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    ITegraApplicationRenderEngine *m_RenderEngine; ///< Handles all rendering functions
    CTegraInputEngine *m_InputEngine; ///< Handles all user input events
    // Pre graphics init objects
    NVScopedRefCounted<qt3ds::render::IUICRenderFactoryCore> m_RuntimeFactoryCore;
    NVScopedRefCounted<qt3ds::runtime::IApplicationCore>
    m_ApplicationCore; ///< Base application before graphis

    // Post graphics init objects
    NVScopedRefCounted<qt3ds::render::IUICRenderFactory> m_RuntimeFactory;
    NVScopedRefCounted<qt3ds::runtime::IApplication> m_Application; ///< Application after graphics
    CPresentation *m_Presentation; ///< Currently loaded presentation, this should be removed in the future

    CPausingTimeProvider m_TimeProvider;
    IWindowSystem &m_WindowSystem;
    IAudioPlayer *m_AudioPlayer;

    volatile QT3DSI32 mRefCount;

    qt3ds::UICAssetVisitor *m_visitor;
    bool m_showOnScreenStats;

public:
    CNDDView(ITimeProvider &inTimeProvider, IWindowSystem &inWindowSystem,
             IAudioPlayer *inAudioPlayer);
    ~CNDDView();

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(qt3ds::render::g_BaseAllocator);

    bool BeginLoad(const QString &sourcePath) override;
    bool HasOfflineLoadingCompleted() override;
    void InitializeGraphics(const QSurfaceFormat &format) override;

    void Cleanup() override;

    bool CanRender() override;
    void Render() override;
    bool WasLastFrameDirty() override;

    KDint HandleMessage(const KDEvent *inEvent) override;

    void Pause() override;
    void UnPause() override;
    bool IsPaused() override;
    void setAssetVisitor(qt3ds::UICAssetVisitor *) override;

    INT32 GetFrameCount() override;
    void showOnScreenStats(bool) override;

    CInputEngine *GetInputEngine() override;
    // Only valid after InitializeGraphics
    ITegraApplicationRenderEngine *GetTegraRenderEngine() override { return m_RenderEngine; }

    void GoToSlideByName(const char *elementPath, const char *slideName) override;
    void GoToSlideByIndex(const char *elementPath, const int slideIndex) override;
    void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap) override;
    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName) override;
    void SetPresentationAttribute(const char *presId, const char *, const char *value) override;
    void GoToTime(const char *elementPath, const float time) override;
    void SetGlobalAnimationTime(qint64 inMilliSecs) override;
    void SetAttribute(const char *elementPath, const char *attributeName, const char *value) override;
    bool GetAttribute(const char *elementPath, const char *attributeName, void *value) override;
    void FireEvent(const char *element, const char *evtName) override;
    bool PeekCustomAction(char *&outElementPath, char *&outActionName) override;
    bool RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData) override;
    void FireEvent(const TEventCommandHash inEventType, eastl::string inArgument) override;
    bool AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction) override;
    qt3ds::foundation::Option<SPresentationSize> GetPresentationSize() override;

    void BootupPreGraphicsInitObjects();
};

CNDDView::CNDDView(ITimeProvider &inTimeProvider, IWindowSystem &inWindowSystem,
                   IAudioPlayer *inAudioPlayer)
    : m_RenderEngine(NULL)
    , m_InputEngine(NULL)
    , m_Application(NULL)
    , m_Presentation(NULL)
    , m_TimeProvider(inTimeProvider)
    , m_WindowSystem(inWindowSystem)
    , m_AudioPlayer(inAudioPlayer)
    , mRefCount(0)
    , m_visitor(nullptr)
    , m_showOnScreenStats(false)
{
}

CNDDView::~CNDDView()
{
}

bool CNDDView::BeginLoad(const QString &sourcePath)
{
    bool theResult = false;

    // boot up the application
    BootupPreGraphicsInitObjects();

    // If there was a presentation file then we have to load it or something failed.
    if (m_ApplicationCore->BeginLoad(sourcePath.toUtf8()))
        theResult = true;
    else
        theResult = false;

    // If Initialize wasn't successful - this means the CShaderFactory failed to initialize
    // or, the presentation failed to load.
    //
    // NOTE: if no presentation was passed, this is 'ok'
    if (!theResult)
        Cleanup();

    return theResult;
}

bool CNDDView::HasOfflineLoadingCompleted()
{
    if (m_Application.mPtr == NULL) {
        if (m_ApplicationCore)
            return m_ApplicationCore->HasCompletedLoading();
        else
            return false;
    }
    return true;
}

void CNDDView::InitializeGraphics(const QSurfaceFormat &format)
{
    m_ApplicationCore->EndLoad();
    // Next call will initialize the render portion of the scenes.  This *must* have a loaded
    // application to go further as it will bind scene graph data to application data.
    m_RuntimeFactory = m_RuntimeFactoryCore->CreateRenderFactory(format);
    m_Application
            = m_ApplicationCore->CreateApplication(*m_InputEngine, m_AudioPlayer,
                                                   *m_RuntimeFactory);
    m_Application->ResetTime();
    m_RenderEngine = &m_RuntimeFactory->CreateRenderEngine();
    m_Presentation = m_Application->GetPrimaryPresentation();

    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigSlideEntered,
                     signalProxy(), &QINDDViewSignalProxy::SigSlideEntered);
    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigSlideExited,
                     signalProxy(), &QINDDViewSignalProxy::SigSlideExited);

    m_TimeProvider.Reset();
}

void CNDDView::Cleanup()
{
    // Q3DStudio_virtual_delete( m_Timer, CTimer );
    // Q3DStudio_virtual_delete( m_PerfFileStream, CFileStream );
    m_Application = NULL;
    Q3DStudio_virtual_delete(m_InputEngine, CTegraInputEngine);
    if (m_RenderEngine) {
        m_RenderEngine->Release();
        m_RenderEngine = NULL;
    }

    CDLLManager &theDLLManager = CDLLManager::GetDLLManager();
    theDLLManager.Cleanup();
    QObject::disconnect(m_Presentation->signalProxy(), 0, signalProxy(), 0);

    m_InputEngine = NULL;
    m_RenderEngine = NULL;
    m_Presentation = NULL;
}

bool CNDDView::CanRender()
{
    return m_Application.mPtr != NULL;
}

//==============================================================================
/**
 *	nv_main APP-SPECIFIC rendering call
 *  returns KD_TRUE to call egl_render and swap properly, KD_FALSE if there has been no scene update
 *or redraw.
 */
void CNDDView::Render()
{
    if (m_Application.mPtr == NULL) {
        // InitializeGraphics has not been called
        QT3DS_ASSERT(false);
    }

    PerfLogGeneralEvent1(DATALOGGER_FRAME);

    m_Application->UpdateAndRender();

    if (m_showOnScreenStats) {
        ITegraRenderStateManager &manager
                = GetTegraRenderEngine()->GetTegraRenderStateManager();
        manager.PushState();

        SSize dim = m_WindowSystem.GetWindowDimensions();
        manager.SetScissorTestEnabled(false);
        manager.SetViewport(0, 0, dim.m_Width, dim.m_Height);

        QPair<QT3DSF32, QT3DSF32> fps
                = m_RuntimeFactory->GetUICRenderContext().GetFPS();

        QString text;
        QTextStream stream(&text);
        stream << QStringLiteral("Render Statistics: ");
        stream << QString::number(fps.first, 'f', 2);
        stream << " fps, frame count ";
        stream << QString::number(fps.second);

        // bottom left coordinates
        GetTegraRenderEngine()->RenderText2D(
                    dim.m_Width / 4, dim.m_Height - 25, qt3ds::QT3DSVec3(0.0, 1.0, 0.0),
                    text.toLatin1().constData());
        GetTegraRenderEngine()->RenderGpuProfilerStats(
                    20.0, dim.m_Height - 80, qt3ds::QT3DSVec3(0.0, 1.0, 0.0));

        manager.PopState();
    }
}

bool CNDDView::WasLastFrameDirty()
{
    if (m_Application)
        return m_Application->IsApplicationDirty();
    return false;
}

//==============================================================================
/**
 *	nv_main APP-SPECIFIC message call
 *	HandleMessage
 */
KDint CNDDView::HandleMessage(const KDEvent *inEvent)
{
    if (m_Application.mPtr == NULL || m_RenderEngine == NULL)
        return 0;
    KDint theReturn = KD_FALSE;
    switch (inEvent->type) {
    case KD_EVENT_INPUT:
        theReturn = KD_TRUE;
        break;
    case KD_EVENT_INPUT_POINTER:
        m_InputEngine->SetPickInput(static_cast<FLOAT>(inEvent->data.inputpointer.x),
                                    static_cast<FLOAT>(inEvent->data.inputpointer.y), true);
        m_InputEngine->SetPickFlags(inEvent->data.inputpointer.select ? LMOUSE_DOWN : LMOUSE_UP);
        theReturn = KD_TRUE;
        break;
#if !defined(Q_OS_MACOS)
    case KD_EVENT_WINDOW_CLOSE:
        theReturn = KD_FALSE;
        break;
    case KD_EVENT_WINDOW_REDRAW:
    case KD_EVENT_WINDOW_FOCUS:
        theReturn = KD_TRUE;
        break;
    case KD_EVENT_WINDOWPROPERTY_CHANGE:
        if (inEvent->data.windowproperty.pname == KD_WINDOWPROPERTY_SIZE
                && m_Application->GetPrimaryPresentation())
            m_RenderEngine->CheckResize(KD_TRUE, *m_Application->GetPrimaryPresentation());
        theReturn = KD_TRUE;
        break;
#endif
    default:
        kdDefaultEvent(inEvent);
        theReturn = KD_TRUE;
        break;
    }
    m_InputEngine->HandleMessage(inEvent, *m_RenderEngine, m_Application->GetPrimaryPresentation());
    return theReturn;
}

void CNDDView::Pause()
{
    m_TimeProvider.Pause();
}

void CNDDView::UnPause()
{
    m_TimeProvider.UnPause();
}

bool CNDDView::IsPaused()
{
    return m_TimeProvider.IsPaused();
}

INT32 CNDDView::GetFrameCount()
{
    return m_Application->GetFrameCount();
}

void CNDDView::showOnScreenStats(bool show)
{
    m_showOnScreenStats = show;
}

CInputEngine *CNDDView::GetInputEngine()
{
    return m_InputEngine;
}

//==============================================================================
/**
 *	Generates an event in the presentation.
 */
void CNDDView::GoToSlideByName(const char *elementPath, const char *slideName)
{
    if (m_Application) {
        if (!elementPath || !slideName)
            return;

        CPresentation *thePresentation = m_Application->GetPrimaryPresentation();
        IScriptBridge *theBridge = thePresentation->GetScriptBridgeQml();

        if (!theBridge)
            return;

        theBridge->GotoSlide(elementPath, slideName, SScriptEngineGotoSlideArgs());
    }
}

void CNDDView::GoToSlideByIndex(const char *elementPath, const int slideIndex)
{
    if (m_Application) {
        if (!elementPath || slideIndex < 0)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.GotoSlideIndex(elementPath, slideIndex, SScriptEngineGotoSlideArgs());
    }
}

void CNDDView::GoToSlideRelative(const char *elementPath, const bool next, const bool wrap)
{
    if (m_Application) {
        if (!elementPath)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.GotoSlideRelative(elementPath, next, wrap, SScriptEngineGotoSlideArgs());
    }
}

bool CNDDView::GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                            QString &currentName, QString &previousName)
{
    if (m_Application && elementPath) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        return theBridgeEngine.GetSlideInfo(elementPath, currentIndex, previousIndex,
                                            currentName, previousName);
    }
    return false;
}

void CNDDView::SetPresentationAttribute(const char *presId, const char *, const char *value)
{
    if (m_Application) {
        if (!presId || !value)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.SetPresentationAttribute(presId, nullptr, value);
    }
}

bool CNDDView::RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        return theBridgeEngine.RegisterCallback(callbackType, func, inUserData);
    }

    return false;
}

void CNDDView::GoToTime(const char *elementPath, const float time)
{
    if (m_Application) {
        if (!elementPath || time < 0.0)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.GotoTime(elementPath, time);
    }
}

void CNDDView::SetGlobalAnimationTime(qint64 inMilliSecs)
{
    if (m_Application)
        m_Application->SetTimeMilliSecs(inMilliSecs);
}

void CNDDView::SetAttribute(const char *elementPath, const char *attributeName, const char *value)
{
    if (m_Application) {
        if (!elementPath || !attributeName || !value)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.SetAttribute(elementPath, attributeName, value);
    }
}

bool CNDDView::GetAttribute(const char *elementPath, const char *attributeName, void *value)
{
    if (m_Application) {
        if (!elementPath || !attributeName || !value)
            return false;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        return theBridgeEngine.GetAttribute(elementPath, attributeName, (char *)value);
    }

    return false;
}

void CNDDView::FireEvent(const char *element, const char *evtName)
{
    if (m_Application) {
        if (!element || !evtName)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.FireEvent(element, evtName);
    }
}

bool CNDDView::PeekCustomAction(char *&outElementPath, char *&outActionName)
{
    bool actionAvailable = true;

    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        Q3DStudio::TElement *theElement = NULL;
        actionAvailable = theBridgeEngine.PeekSignal(theElement, outActionName);
        if (actionAvailable && theElement)
            outElementPath = (char *)theElement->m_Path.c_str();
    }

    return actionAvailable;
}

void CNDDView::FireEvent(const TEventCommandHash inEventType, eastl::string inArgument)
{
    if (m_Application) {
        CPresentation *thePresentation = m_Application->GetPrimaryPresentation();
        TElement *theScene = thePresentation->GetRoot();
        if (inArgument.empty()) {
            thePresentation->FireEvent(inEventType, theScene, NULL, NULL, ATTRIBUTETYPE_NONE,
                                       ATTRIBUTETYPE_NONE);
        } else {
            UVariant inArg;
            inArg.m_StringHandle = thePresentation->GetStringTable().GetHandle(inArgument.c_str());
            thePresentation->FireEvent(inEventType, theScene, &inArg, NULL, ATTRIBUTETYPE_STRING,
                                       ATTRIBUTETYPE_NONE);
        }
    }
}

bool CNDDView::AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction)
{
    bool theResult = false;
    if (m_Presentation) {
        if (m_Application) {
            // this is lua specific
            Q3DStudio::CLuaEngine &theScriptEngine
                    = static_cast<Q3DStudio::CLuaEngine &>(m_RuntimeFactoryCore->GetScriptEngine());
            theScriptEngine.AddGlobalFunction(inFunctionName, inFunction);
            theResult = true;
        }
    }
    return theResult;
}
qt3ds::foundation::Option<SPresentationSize> CNDDView::GetPresentationSize()
{
    if (m_Application) {
        CPresentation *thePresentation = m_Application->GetPrimaryPresentation();
        if (thePresentation)
            return thePresentation->GetSize();
    }
    return qt3ds::foundation::Empty();
}

//==============================================================================
/**
 *	Perform the initialization steps prior to loading any presentation.
 */
void CNDDView::BootupPreGraphicsInitObjects()
{
    qCInfo(TRACE_INFO) << "CNDDView::BootupPreGraphicsInitObjects: DoInitialize";
    // Create engines and runtime
    const eastl::string &theAppDir = QCoreApplication::applicationDirPath().toLatin1().constData();

    m_RuntimeFactoryCore = qt3ds::render::IUICRenderFactoryCore::CreateRenderFactoryCore(
                theAppDir.c_str(), m_WindowSystem, m_TimeProvider);
    m_ApplicationCore = qt3ds::runtime::IApplicationCore::CreateApplicationCore(*m_RuntimeFactoryCore,
                                                                              theAppDir.c_str());

    if (m_ApplicationCore && m_visitor)
        m_ApplicationCore->setAssetVisitor(m_visitor);

    m_InputEngine = static_cast<CTegraInputEngine *>(CreateInputEngine());
    Q3DStudio_ASSERT(m_InputEngine != NULL);

    qCInfo(TRACE_INFO) << "CNDDView::DoInitialize: Successfully initialized!";
}

void CNDDView::setAssetVisitor(qt3ds::UICAssetVisitor *v)
{
    m_visitor = v;
    if (m_ApplicationCore)
        m_ApplicationCore->setAssetVisitor(v);
}

INDDView &INDDView::Create(ITimeProvider &inProvider, IWindowSystem &inWindowSystem,
                           IAudioPlayer *inAudioPlayer)
{
    return *QT3DS_NEW(qt3ds::render::g_BaseAllocator, CNDDView)(inProvider, inWindowSystem,
                                                              inAudioPlayer);
}

QINDDViewSignalProxy *INDDView::signalProxy()
{
    return &m_SignalProxy;
}

//==============================================================================
/**
 *	CTegraApplication
 */
CTegraApplication::CTegraApplication(ITimeProvider &inProvider, IWindowSystem &inWindowSystem,
                                     IAudioPlayer *inAudioPlayer)
{
    m_NDDView = INDDView::Create(inProvider, inWindowSystem, inAudioPlayer);
}

CTegraApplication::~CTegraApplication()
{
}

KDint CTegraApplication::BeginLoad(const QString &sourcePath)
{
#ifndef QT3DS_NO_SEARCH_PATH
    // We need these later on in case we try to load any files
    // such as images
    NvFSAppendSearchPath("/res");
    NvFSAppendSearchPath("/res/..");
    NvFSAppendSearchPath("/data");
#endif

    KDint theResult = KD_FALSE;

    qCInfo(TRACE_INFO) << "CTegraApplication::BeginLoad: Attempting presentation beginload";

    if (!sourcePath.isEmpty()) {
        // If there was a presentation file then we have to load it or something failed.
        if (m_NDDView->BeginLoad(sourcePath)) {
            qCInfo(TRACE_INFO)
                    << "CTegraApplication::BeginLoad: Successfully begin loading presentation: "
                    << sourcePath;
            theResult = KD_TRUE;
        } else {
            qCInfo(TRACE_INFO) << "CTegraApplication::BeginLoad: Failed to load presentation: "
                               << sourcePath;
            theResult = KD_FALSE;
        }
    } else {
        // If there wasn't, then we are still in an OK state.
        qCInfo(TRACE_INFO) << "CTegraApplication::BeginLoad: Presentation file not provided";
        theResult = KD_TRUE;
    }

    qCInfo(TRACE_INFO) << "CTegraApplication::BeginLoad: End beginload";
    return theResult;
}

void CTegraApplication::InitializeGraphics(const QSurfaceFormat &format)
{
    m_NDDView->InitializeGraphics(format);
}

void CTegraApplication::Render()
{
    m_NDDView->Render();
}

KDint CTegraApplication::HandleMessage(const KDEvent *inEvent)
{
    return m_NDDView->HandleMessage(inEvent);
}
} // namespace Q3DStudio