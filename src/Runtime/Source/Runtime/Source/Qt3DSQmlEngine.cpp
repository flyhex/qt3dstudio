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

#include "RuntimePrefix.h"

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSQmlEngine.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSSceneManager.h"
#include "Qt3DSVector3.h"
#include "Qt3DSColor.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSDataLogger.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSSystem.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSStateLuaScriptContext.h"
#include "Qt3DSStateDebugger.h"
#include "EventPollingSystemLuaBinding.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSSlideSystem.h"

#include "EASTL/vector.h"
#include "EASTL/list.h"
#include <sys/stat.h>
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
#include "Qt3DSStateDebugStreams.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "Qt3DSRenderThreadPool.h"
#include "Qt3DSRenderImageBatchLoader.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSAudioPlayer.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSParametersSystem.h"
#include "Qt3DSQmlElementHelper.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include "q3dsqmlscript.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

#if defined(_DEBUG) || (_PROFILE)
#define Q3DStudio_LOG_EVENT(S) // something
#else
#define Q3DStudio_LOG_EVENT(S)
#endif

using qt3ds::runtime::IApplicationCore;
using qt3ds::runtime::IApplication;
using namespace qt3ds;

namespace __SQmlEngineImpl_Basic_Structs__ {
    struct LuaScopedLock
    {
        qt3ds::foundation::Mutex *m_Mutex;

        LuaScopedLock(qt3ds::foundation::Mutex *mtx)
            : m_Mutex(mtx)
        {
            if (m_Mutex)
                m_Mutex->lock();
        }

        ~LuaScopedLock()
        {
            if (m_Mutex)
                m_Mutex->unlock();
        }
    };
}

using namespace __SQmlEngineImpl_Basic_Structs__;
#define QML_ENGINE_MULTITHREAD_PROTECT_METHOD LuaScopedLock __locker(m_MultithreadedMutex);

//==============================================================================
// Callback handling
/**
*	Constructor
*/
CScriptCallbacks::CScriptCallbacks()
    : m_CallbackList(0, 0, "CallbackList")
{
}

/**
*	Destructor
*/
CScriptCallbacks::~CScriptCallbacks()
{
    FOR_ARRAY(SFrameCallbackEntry *, theEntry, m_CallbackList)
    Q3DStudio_delete(*theEntry, SFrameCallbackEntry);

    m_CallbackList.Clear();
}

bool CScriptCallbacks::RegisterCallback(Q3DStudio::UINT32 callbackType,
                                        const TScriptCallback inCallback, void *inUserData)
{
    // currently we only support these two types of callbacks
    if (callbackType != SCRIPT_ON_INITIALIZE && callbackType != SCRIPT_ON_UPDATE) {
        return false;
    }

    SFrameCallbackEntry *theFrameCallbackEntry = NULL;

    try {
        // note we don't care if someone registers the same function twice.
        theFrameCallbackEntry = Q3DStudio_new(SFrameCallbackEntry) SFrameCallbackEntry();

        if (!theFrameCallbackEntry) {
            throw "failed to allocated SFrameCallbackEntry";
        }

        SScriptCallbackEntry *theScriptCallbackEntry =
            Q3DStudio_new(SScriptCallbackEntry) SScriptCallbackEntry();
        if (!theScriptCallbackEntry) {
            throw "failed to allocated SScriptCallbackEntry";
        }

        theScriptCallbackEntry->m_CallbackType = callbackType;
        theScriptCallbackEntry->m_Function = inCallback;
        theScriptCallbackEntry->m_UserData = inUserData;
        theScriptCallbackEntry->m_Processed = false;

        theFrameCallbackEntry->m_Callbacks.Push(theScriptCallbackEntry);

        m_CallbackList.Push(theFrameCallbackEntry);

    } catch (const char *) {
        if (theFrameCallbackEntry)
            Q3DStudio_delete(theFrameCallbackEntry, SFrameCallbackEntry);

        return false;
    }

    return true;
}

void CScriptCallbacks::UnregisterCallback(Q3DStudio::UINT32, const TScriptCallback)
{
    // not used so far
}

void CScriptCallbacks::ProcessCallbacks()
{
    // Call onInitialize function
    FOR_ARRAY(SFrameCallbackEntry *, theFrameEntry, m_CallbackList)
    {
        FOR_ARRAY(SScriptCallbackEntry *, theEntry, (*theFrameEntry)->m_Callbacks)
        {
            if (((*theEntry)->m_CallbackType == SCRIPT_ON_INITIALIZE) && !(*theEntry)->m_Processed
                && (*theEntry)->m_Function) {
                (*theEntry)->m_Function((*theEntry)->m_UserData);
                (*theEntry)->m_Processed = true;
            }
        }
    }

    // Call onUpdate functions
    FOR_ARRAY(SFrameCallbackEntry *, theFrameEntry, m_CallbackList)
    {
        FOR_ARRAY(SScriptCallbackEntry *, theEntry, (*theFrameEntry)->m_Callbacks)
        {
            // int type = (*theEntry)->m_CallbackType;
            if ((*theEntry)->m_CallbackType == SCRIPT_ON_UPDATE && (*theEntry)->m_Function) {
                (*theEntry)->m_Function((*theEntry)->m_UserData);
            }
        }
    }
}

//==============================================================================
//*	End of helper class handling element operations
//==============================================================================

//==============================================================================
/**
*	Helper class handling command operations
*/
class CQmlCommandHelper
{
    //==============================================================================
    //	Methods
    //==============================================================================
private: // Constructors
    CQmlCommandHelper();
    virtual ~CQmlCommandHelper();

public: // static Functions
    static bool SetupGotoSlideCommand(TElement &inElement, const char *slideName,
                                      const SScriptEngineGotoSlideArgs &inArgs);
    static bool SetupGotoSlideCommand(TElement &inElement, Q3DStudio::INT32 inSlideIndex,
                                      const SScriptEngineGotoSlideArgs &inArgs);

protected: // Static Hidden Helpers
    // static TElement*	FireCommand(TEventCommandHash inCommand);

public: // Static Helpers
    static TElement *GetComponentParent(TElement *inParent);
};

//==============================================================================
/**
*	Constructor
*/
CQmlCommandHelper::CQmlCommandHelper()
{
}

//==============================================================================
/**
*	Destructor
*/
CQmlCommandHelper::~CQmlCommandHelper()
{
}

static Option<Q3DStudio::UINT8> FindSlide(TElement &inElement, const char *slideName)
{
    IPresentation *thePresentation = inElement.GetBelongedPresentation();

    int theSlideHashName = thePresentation->GetApplication().HashString(slideName);
    TComponent *theComponent = thePresentation->GetComponentManager().GetComponent(&inElement);
    UINT8 theSlideIndex =
        thePresentation->GetSlideSystem().FindSlide(*theComponent, theSlideHashName);
    if (theSlideIndex >= theComponent->GetSlideCount()) {
        qt3ds::foundation::CRegisteredString elemPath = thePresentation->GetElementPath(inElement);
        if (elemPath.IsValid()) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "QMLCommandHelper: goToSlide: Unable to find slide "
                    << slideName <<"on element " << elemPath.c_str();
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "QMLCommandHelper: goToSlide: Unable to find slide "
                    << slideName;
        }
        return Empty();
    }

    return theSlideIndex;
}

TElement *CQmlCommandHelper::GetComponentParent(TElement *inElement)
{
    Q3DStudio_ASSERT(inElement);
    return &inElement->GetComponentParent();
}

bool CQmlCommandHelper::SetupGotoSlideCommand(TElement &inElement, Q3DStudio::INT32 inSlideIndex,
                                              const SScriptEngineGotoSlideArgs &inArgs)
{
    IPresentation *thePresentation = inElement.GetBelongedPresentation();
    TElement *theTarget = GetComponentParent(&inElement);
    SComponentGotoSlideData theSlideData(inSlideIndex);
    theSlideData.m_Mode = inArgs.m_Mode;
    theSlideData.m_Paused = inArgs.m_Paused;
    theSlideData.m_Rate = inArgs.m_Rate;
    theSlideData.m_Reverse = inArgs.m_Reverse;
    theSlideData.m_StartTime = inArgs.m_StartTime;
    // Resolve playthroughto if it has a valid value.
    if (!isTrivial(inArgs.m_PlaythroughTo)) {
        if (AreEqual(inArgs.m_PlaythroughTo, "next"))
            theSlideData.m_PlaythroughTo = -1;
        else if (AreEqual(inArgs.m_PlaythroughTo, "previous"))
            theSlideData.m_PlaythroughTo = -2;
        else {
            // Find the slide if possible.  If not, then just error leave things as they are.

            Option<UINT8> theSlideIndex = FindSlide(inElement, inArgs.m_PlaythroughTo);
            if (theSlideIndex.hasValue())
                theSlideData.m_PlaythroughTo = *theSlideIndex;
        }
    }
    thePresentation->GetComponentManager().SetupComponentGotoSlideCommand(theTarget, theSlideData);
    UVariant theArg1;
    UVariant theArg2;
    qt3ds::intrinsics::memZero(&theArg1, sizeof(UVariant));
    qt3ds::intrinsics::memZero(&theArg2, sizeof(UVariant));
    thePresentation->FireCommand(COMMAND_GOTOSLIDE, theTarget, &theArg1, &theArg2);
    return true;
}

bool CQmlCommandHelper::SetupGotoSlideCommand(TElement &inElement, const char *slideName,
                                              const SScriptEngineGotoSlideArgs &inArgs)
{
    Option<UINT8> theSlideIndex = FindSlide(inElement, slideName);
    if (theSlideIndex.hasValue())
        return SetupGotoSlideCommand(inElement, *theSlideIndex, inArgs);

    return false;
}

//==============================================================================
//*	End of helper class handling command operations
//==============================================================================

struct SEmitSignalData : public NVRefCounted
{
    NVAllocatorCallback &m_Alloc;
    volatile QT3DSI32 mRefCount;
    INT32 m_SignalNameHandler; ///< The name of the signal to emit
    TElement *m_TargetElement; ///< The source element where the event is going to occur

    SEmitSignalData(NVAllocatorCallback &alloc, INT32 inSignalName, TElement *inElement)
        : m_Alloc(alloc)
        , mRefCount(0)
        , m_SignalNameHandler(inSignalName)
        , m_TargetElement(inElement)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Alloc)
};

class CQmlEngineImpl : public CQmlEngine
{
    typedef NVScopedRefCounted<SEmitSignalData> TEmitSignalPtr;
    typedef eastl::list<TEmitSignalPtr, ForwardingAllocator> TEmitSignalQueue;

    static const int MAX_ACTION_QUEUE_SIZE = 100;

private:
    qt3ds::NVFoundationBase &m_Foundation;
    IApplication *m_Application;
    IApplicationCore *m_ApplicationCore;
    qt3ds::foundation::Mutex m_PreloadMutex;
    qt3ds::foundation::Mutex *m_MultithreadedMutex;

    TEmitSignalQueue m_EmitSignalDataList;

    QT3DSI32 mRefCount;

    CScriptCallbacks m_ScriptCallbacks;

    QQmlEngine m_engine;
    QMap<QString, QQmlComponent *> m_components;
    QVector<Q3DSQmlScript *> m_scripts;

public:
    CQmlEngineImpl(NVFoundationBase &fnd, ITimeProvider &inTimeProvider);

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    // functions from IScriptBridge
    void EnableMultithreadedAccess() override;
    void DisableMultithreadedAccess() override;

    void SetApplicationCore(qt3ds::runtime::IApplicationCore &inApplication) override;
    void SetApplication(qt3ds::runtime::IApplication &inApplication) override;
    qt3ds::runtime::IApplication *GetApplication() override;

    void BeginPreloadScripts(const eastl::vector<const char *> &,
                                     qt3ds::render::IThreadPool &, const char *) override {}
    void EndPreloadScripts() override {}
    eastl::vector<eastl::string> GetLoadedScripts() override
    {
        eastl::vector<eastl::string> retval;
        return retval;
    }

    void LoadScript(IPresentation *presentation, TElement *element, const CHAR *path) override;
    Q3DStudio::INT32 InitializeApplicationBehavior(const char *) override
    {
        return -1;
    }

    void ProcessFrameCallbacks(IPresentation *) override;
    void ExecuteApplicationScriptFunction(Q3DStudio::INT32, const char *) override {}
    void CallFunction(const char *, const char *,
                              CScriptEngineCallFunctionArgRetriever &) override {}

    void ProcessSignal(IPresentation *inPresentation,
                       const SEventCommand &inCommand) override;
    void ProcessCustomActions(IPresentation *inPresentation,
                                      const SEventCommand &inCommand) override;
    void ProcessCustomCallback(IPresentation *, const SEventCommand &) override {}

    void SetTableForElement(TElement &, ILuaScriptTableProvider &) override {}
    void SetAttribute(const char *element, const char *attName, const char *value) override;
    bool GetAttribute(const char *element, const char *attName, char *value) override;
    void FireEvent(const char *element, const char *evtName) override;

    void GotoSlide(const char *component, const char *slideName,
                           const SScriptEngineGotoSlideArgs &inArgs) override;
    void GotoSlideRelative(const char *, bool, bool, const SScriptEngineGotoSlideArgs &) override;

    void SetPresentationAttribute(const char *, const char *, const char *) override;

    // No need to implement here, as sound playing is done in Qt3DSViewerApp
    bool PlaySoundFile(const char *) override { return false; }

    void EnableDebugging(qt3ds::state::debugger::IMultiProtocolSocket &) override {}
    void EnableProfiling() override {}
    void StepGC() override {}

    // functions from CQMLEngine
    bool PeekSignal(TElement *&outElement, char *&outName) override;
    void GotoSlideIndex(const char *component, const Q3DStudio::INT32 slideIndex,
                                const SScriptEngineGotoSlideArgs &inArgs) override;
    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName) override;
    void GotoTime(const char *component, const Q3DStudio::FLOAT time) override;
    bool RegisterCallback(Q3DStudio::UINT32 callbackType, const TScriptCallback inCallback,
                                  void *inUserData) override;
    void Shutdown(qt3ds::NVFoundationBase &inFoundation) override;

private:
    void createComponent(QQmlComponent *component, TElement *element);
    TElement *getTarget(const char *component);
};

CQmlEngineImpl::CQmlEngineImpl(NVFoundationBase &fnd, ITimeProvider &)
    : m_Foundation(fnd)
    , m_Application(NULL)
    , m_ApplicationCore(NULL)
    , m_PreloadMutex(fnd.getAllocator())
    , m_MultithreadedMutex(NULL)
    , m_EmitSignalDataList(
          ForwardingAllocator(fnd.getAllocator(), "CQmlEngineImpl::m_EmitSignalDataList"))
    , mRefCount(0)
{
    qmlRegisterType<Q3DSQmlBehavior>("QtStudio3D.Behavior", 1, 0, "Behavior");
}

void CQmlEngineImpl::Shutdown(qt3ds::NVFoundationBase &inFoundation)
{
}

void CQmlEngineImpl::EnableMultithreadedAccess()
{
    m_MultithreadedMutex = &m_PreloadMutex;
}

void CQmlEngineImpl::DisableMultithreadedAccess()
{
    m_MultithreadedMutex = NULL;
}

void CQmlEngineImpl::SetApplicationCore(qt3ds::runtime::IApplicationCore &inApplication)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_ApplicationCore = &inApplication;
}

void CQmlEngineImpl::SetApplication(qt3ds::runtime::IApplication &inApplication)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_Application = &inApplication;
}

qt3ds::runtime::IApplication *CQmlEngineImpl::GetApplication()
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    return m_Application;
}

void CQmlEngineImpl::SetAttribute(const char *element, const char *attName, const char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    TElement *theTarget = getTarget(element);
    if (theTarget) {
        bool success = CQmlElementHelper::SetAttribute(theTarget, attName, value, false);
        if (!success) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "CQmlEngineImpl::SetAttribute: "
                    << "failed to set attribute on element"
                    << element << ":" << attName << ":" << value;
        }
    }
}

bool CQmlEngineImpl::GetAttribute(const char *element, const char *attName, char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    TElement *theTarget = getTarget(element);
    if (theTarget) {
        bool success = CQmlElementHelper::GetAttribute(theTarget, attName, value);
        if (!success) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "CQmlEngineImpl::SetAttribute: "
                    << "failed to set attribute on element"
                    << element << ":" << attName << ":" << value;
        }
        return success;
    }

    return false;
}

void CQmlEngineImpl::LoadScript(IPresentation *presentation, TElement *element, const CHAR *path)
{
    QString projectPath(presentation->GetApplication().GetProjectDirectory().c_str());
    QString sourcePath(projectPath + "/" + path);
    sourcePath.replace('\\', '/');

    TElement *parent = element->GetParent();
    if (!parent)
        return;

    if (!m_components.contains(sourcePath)) {
        m_components[sourcePath] = new QQmlComponent(&m_engine, QUrl::fromLocalFile(sourcePath),
                                                     &m_engine);
    }

    QQmlComponent *component = m_components[sourcePath];
    if (component->status() == QQmlComponent::Ready) {
           createComponent(component, element);
    } else if (component->status() == QQmlComponent::Error) {
        qWarning() << "Error while loading script"
                   << component->url().toString()
                   << ":" << component->errorString();
    } else {
        QObject::connect(component, &QQmlComponent::statusChanged,
            [this, component, element] (QQmlComponent::Status status) {
                if (status == QQmlComponent::Ready) {
                    createComponent(component, element);
                } else {
                    qWarning() << "Error while loading script"
                               << component->url().toString()
                               << ":" << component->errorString();
                }
            }
        );
    }
}

void CQmlEngineImpl::FireEvent(const char *element, const char *evtName)
{
    TElement *theElement = getTarget(element);
    if (theElement && theElement->GetActive() == true) {
        IPresentation *thePresentation = theElement->GetBelongedPresentation();
        thePresentation->FireEvent(CHash::HashEventCommand(evtName), theElement);
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::FireEvent: "
                << "failed to find element: "
                << element << " " << evtName;
    }
}

void CQmlEngineImpl::GotoSlide(const char *component, const char *slideName,
                               const SScriptEngineGotoSlideArgs &inArgs)
{
    TElement *theTarget = getTarget(component);
    if (theTarget) {
        CQmlCommandHelper::SetupGotoSlideCommand(*theTarget, slideName, inArgs);
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::GotoSlide: Unable to find component " << component;
    }
}

void CQmlEngineImpl::GotoSlideRelative(const char *component, bool inNextSlide, bool inWrap,
                                       const SScriptEngineGotoSlideArgs &inArgs)
{
    TElement *theTarget = getTarget(component);
    if (theTarget) {
        theTarget = &theTarget->GetComponentParent();
        if (theTarget && theTarget->GetActive()) {
            TComponent *theComponent = static_cast<TComponent *>(theTarget);
            Q3DStudio::INT32 theSlide = theComponent->GetCurrentSlide();
            Q3DStudio::INT32 theSlideCount = theComponent->GetSlideCount();
            theSlide = inNextSlide ? theSlide + 1 : theSlide - 1;
            if (theSlide < 1) {
                if (inWrap)
                    theSlide = theSlideCount - 1;
                else
                    theSlide = 1;
            } else if (theSlide == theSlideCount) {
                if (inWrap)
                    theSlide = 1;
                else
                    theSlide = theSlideCount - 1;
            }
            if (theSlide != theComponent->GetCurrentSlide()) {
                CQmlCommandHelper::SetupGotoSlideCommand(*theTarget, theSlide, inArgs);
            }
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "QmlEngine::GotoSlideRelative: Component is not active: " << component;
        }
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "QmlEngine::GotoSlideRelative: failed to find component: " << component;
    }
}

void CQmlEngineImpl::SetPresentationAttribute(const char *presId, const char *, const char *value)
{
    if (isTrivial(presId))
        return;
    if (presId[0] == '#')
        ++presId;
    CPresentation *thePresentation = m_Application->GetPresentationById(presId);
    if (thePresentation) {
        bool active = AreEqualCaseless(nonNull(value), "True");
        thePresentation->SetActive(active);
    }
}

void CQmlEngineImpl::GotoSlideIndex(const char *component, const Q3DStudio::INT32 slideIndex,
                                    const SScriptEngineGotoSlideArgs &inArgs)
{
    TElement *theTarget = getTarget(component);
    if (theTarget) {
        CQmlCommandHelper::SetupGotoSlideCommand(*theTarget, slideIndex, inArgs);
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::GotoSlide: Unable to find component " << component;
    }
}

bool CQmlEngineImpl::GetSlideInfo(const char *element, int &currentIndex, int &previousIndex,
                                  QString &currentName, QString &previousName)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    TElement *theTarget = getTarget(element);
    if (theTarget && theTarget->IsComponent()) {
        IPresentation *thePresentation = theTarget->GetBelongedPresentation();
        TComponent *theComponent = thePresentation->GetComponentManager().GetComponent(theTarget);
        currentIndex = int(theComponent->GetCurrentSlide());
        previousIndex = int(theComponent->GetPreviousSlide());
        if (currentIndex > 0) {
            currentName = QString::fromLocal8Bit(
                        thePresentation->GetSlideSystem().GetSlideName(
                            qt3ds::runtime::SSlideKey(*theComponent, QT3DSU32(currentIndex))));
        } else {
            currentName.clear();
        }
        if (previousIndex > 0) {
            previousName = QString::fromLocal8Bit(
                        thePresentation->GetSlideSystem().GetSlideName(
                            qt3ds::runtime::SSlideKey(*theComponent, QT3DSU32(previousIndex))));
        } else {
            previousName.clear();
        }
        return true;
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::GetSlideInfo: Supplied element is not a component " << element;
    }
    return false;
}

void CQmlEngineImpl::GotoTime(const char *component, const Q3DStudio::FLOAT time)
{
    TElement *theTarget = getTarget(component);
    if (theTarget && theTarget->GetActive()) {
        UVariant theArg1;
        UVariant theArg2;

        IPresentation *thePresentation = theTarget->GetBelongedPresentation();

        theArg1.m_INT32 = static_cast<INT32>(time * 1000);

        TElement *theParentTarget = &theTarget->GetComponentParent();

        thePresentation->FireCommand(COMMAND_GOTOTIME, theParentTarget, &theArg1, &theArg2);
    }
}

bool CQmlEngineImpl::RegisterCallback(Q3DStudio::UINT32 callbackType,
                                      const TScriptCallback inCallback, void *inUserData)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    return m_ScriptCallbacks.RegisterCallback(callbackType, inCallback, inUserData);
}

void CQmlEngineImpl::ProcessFrameCallbacks(IPresentation *)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_ScriptCallbacks.ProcessCallbacks();

    for (Q3DSQmlScript *script : m_scripts)
        script->update();
}

void CQmlEngineImpl::ProcessSignal(IPresentation *inPresentation,
                                   const SEventCommand &inCommand)
{
    using qt3ds::runtime::TIdValuePair;
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    TElement *theElement = inCommand.m_Target; // the element that is a behavior

    CPresentation *thePresentation = (CPresentation *)inPresentation;
    // IParametersSystem& theParametersManager = thePresentation->GetParametersSystem();
    qt3ds::foundation::IStringTable &theStrTable(thePresentation->GetStringTable());

    CRegisteredString theSignal = theStrTable.HandleToStr(inCommand.m_Arg1.m_INT32);

    if (!theSignal.IsValid() || m_EmitSignalDataList.size() > CQmlEngineImpl::MAX_ACTION_QUEUE_SIZE)
        return;

    TEmitSignalPtr theTemp = QT3DS_NEW(m_Foundation.getAllocator(), SEmitSignalData)(
        m_Foundation.getAllocator(), inCommand.m_Arg1.m_INT32, theElement);
    m_EmitSignalDataList.push_back(theTemp);
}


//==============================================================================
/**
*   Handle custom actions
*   @param inCommand                carrier of command and parameter information
*/
void CQmlEngineImpl::ProcessCustomActions(IPresentation *presentation,
                                          const SEventCommand &command)
{
    TElement *element = command.m_Target;
    for (Q3DSQmlScript *script : m_scripts) {
        if (script->hasBehavior(element)) {
            IParametersSystem &parametersManager
                    = static_cast<CPresentation *>(presentation)->GetParametersSystem();
            INT32 groupId = command.m_Arg1.m_INT32;
            UINT32 numParams = parametersManager.GetNumParameters(groupId);
            if (numParams == 0) {
                QT3DS_ASSERT(false);
                return;
            }

            qt3ds::runtime::TIdValuePair tempData = parametersManager.GetParameter(groupId, 0);
            if (tempData.first != CHash::HashString("string")) {
                QT3DS_ASSERT(false);
                return;
            }

            CRegisteredString functionName = presentation->GetStringTable().HandleToStr(
                        tempData.second.m_StringHandle);
            script->call(functionName.c_str());
            return;
        }
    }
}

bool CQmlEngineImpl::PeekSignal(TElement *&outElement, char *&outName)
{
    if (m_EmitSignalDataList.empty())
        return false;

    NVScopedRefCounted<SEmitSignalData> theAction = m_EmitSignalDataList.front();
    m_EmitSignalDataList.pop_front();

    outElement = theAction->m_TargetElement;
    if (outElement) {
        IPresentation *thePresentation = outElement->GetBelongedPresentation();
        qt3ds::foundation::IStringTable &theStrTable(thePresentation->GetStringTable());
        CRegisteredString theSignal = theStrTable.HandleToStr(theAction->m_SignalNameHandler);
        outName = (char *)theSignal.c_str();
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::PeekCustomAction: Unable to find element: EmitSignal queue error";
        outName = NULL;
    }

    return true;
}

void CQmlEngineImpl::createComponent(QQmlComponent *component, TElement *element)
{
    TElement *parent = element->GetParent();
    if (!parent)
        return;

    QQmlContext *context = new QQmlContext(&m_engine, &m_engine);
    QObject *obj = component->beginCreate(context);
    if (!obj) {
        context->deleteLater();
        return;
    }
    Q3DSQmlBehavior *behaviorObject = qobject_cast<Q3DSQmlBehavior *>(obj);
    if (behaviorObject == nullptr) {
        obj->deleteLater();
        context->deleteLater();
        return;
    }

    auto script = new Q3DSQmlScript(*this, *behaviorObject, *element, *parent);
    component->completeCreate();
    behaviorObject->setScript(script);

    script->setParent(component);
    obj->setParent(script);
    m_scripts.push_back(script);
}

TElement *CQmlEngineImpl::getTarget(const char *component) {
    TElement *target = NULL;
    QStringList split = QString(component).split(":");
    if (split.size() > 1) {
        target = CQmlElementHelper::GetElement(
                    *m_Application,
                    m_Application->GetPresentationById(split.at(0).toStdString().c_str()),
                    split.at(1).toStdString().c_str(), NULL);
    } else {
        target = CQmlElementHelper::GetElement(
                    *m_Application,
                    m_Application->GetPrimaryPresentation(),
                    split.at(0).toStdString().c_str(), NULL);
    }
    return target;
}

/**
* @brief Create QML engine
*
* @param[in] inFoundation		Pointer to foundation
* @param[in] inTimeProvider		Pointer to time provider
*
* @return  no return
*/
CQmlEngine *CQmlEngine::Create(qt3ds::NVFoundationBase &inFoundation, ITimeProvider &inTimeProvider)
{
    return QT3DS_NEW(inFoundation.getAllocator(), CQmlEngineImpl)(inFoundation, inTimeProvider);
}
}
