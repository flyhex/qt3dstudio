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
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSSlideSystem.h"

#include "EASTL/vector.h"
#include "EASTL/list.h"
#include <sys/stat.h>
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
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
#include "q3dsqmlscript.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qjsengine.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qfileinfo.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

#if defined(_DEBUG) || (_PROFILE)
#define Q3DStudio_LOG_EVENT(S) // something
#else
#define Q3DStudio_LOG_EVENT(S)
#endif

using qt3ds::runtime::IApplication;
using namespace qt3ds;

namespace __SQmlEngineImpl_Basic_Structs__ {
    struct QmlScopedLock
    {
        qt3ds::foundation::Mutex *m_Mutex;

        QmlScopedLock(qt3ds::foundation::Mutex *mtx)
            : m_Mutex(mtx)
        {
            if (m_Mutex)
                m_Mutex->lock();
        }

        ~QmlScopedLock()
        {
            if (m_Mutex)
                m_Mutex->unlock();
        }
    };
}

using namespace __SQmlEngineImpl_Basic_Structs__;
#define QML_ENGINE_MULTITHREAD_PROTECT_METHOD QmlScopedLock __locker(m_MultithreadedMutex);

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
    IApplication *m_ApplicationCore;
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

    void SetApplicationCore(qt3ds::runtime::IApplication &inApplication) override;
    void SetApplication(qt3ds::runtime::IApplication &inApplication) override;
    qt3ds::runtime::IApplication *GetApplication() override;
    void Initialize() override;

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

    void SetTableForElement(TElement &, IScriptTableProvider &) override {}
    void SetAttribute(TElement *target, const char *attName, const char *value) override;
    void SetAttribute(const char *element, const char *attName, const char *value) override;
    bool GetAttribute(const char *element, const char *attName, char *value) override;
    void FireEvent(const char *element, const char *evtName) override;
    void SetDataInputValue(const QString &name, const QVariant &value,
                           Q3DSDataInput::ValueRole valueRole) override;

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
    void listAllElements(TElement *root, QList<TElement *> &elements);
    void initializeDataInputsInPresentation(CPresentation &presentation, bool isPrimary);
    // Splits down vector attributes to components as Runtime does not really
    // handle vectors at this level anymore
    bool getAttributeVector3(QVector<QByteArray> &outAttVec, const QByteArray &attName,
                             TElement *elem);
    bool getAttributeVector2(QVector<QByteArray> &outAttVec, const QByteArray &attName,
                             TElement *elem);
    // build and evaluate Evaluator datainput type expression
    QJSValue buildJSFunc(const QString &userFunc);
    // pass controller name for error reporting purposes
    QVariant callJSFunc(const QString &controllerName, qt3ds::runtime::DataInputDef &diDef,
                        const QVariant::Type type);
    // matches all numeric datatypes so we do not get datatype mismatch when JS
    // decides to change result datatype (f.ex from double to int when result
    // fractional part for specific input values happens to be exactly zero)
    bool isMatchingDatatype(QVariant::Type resultType, QVariant::Type propertyType);
    // find out which datainputs are used in the expression
    QVector<QString> resolveDependentDatainputs(const QString &expression,
                                                const QString &controllerName);
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
    qmlRegisterType<Q3DSQmlBehavior, 1>("QtStudio3D.Behavior", 1, 1, "Behavior");
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

void CQmlEngineImpl::SetApplicationCore(runtime::IApplication &inApplication)
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

void CQmlEngineImpl::Initialize()
{
    // Gather data input controlled properties
    QList<CPresentation *> presentations = m_Application->GetPresentationList();

    for (int i = 0; i < presentations.size(); ++i)
        initializeDataInputsInPresentation(*presentations[i], i == 0);
}

void CQmlEngineImpl::SetAttribute(TElement *target, const char *attName, const char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    if (target) {
        bool success = CQmlElementHelper::SetAttribute(target, attName, value, false);
        if (!success) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "CQmlEngineImpl::SetAttribute: "
                    << "failed to set attribute on element"
                    << target << ":" << attName << ":" << value;
        }
    }
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
    QString presPath = QFileInfo(presentation->GetFilePath()).absolutePath();

    QString sourcePath(presPath + QLatin1Char('/') + path);
    sourcePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

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

void CQmlEngineImpl::SetDataInputValue(
        const QString &name, const QVariant &value,
        Q3DSDataInput::ValueRole valueRole = Q3DSDataInput::ValueRole::Value)
{
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    if (diMap.contains(name)) {
        qt3ds::runtime::DataInputDef &diDef = diMap[name];
        switch (valueRole) {
        case Q3DSDataInput::ValueRole::Value: { // switch (valueRole)
            diDef.value = value;
            const QVector<qt3ds::runtime::DataInputControlledAttribute> &ctrlAtt
                    = diDef.controlledAttributes;
            for (const qt3ds::runtime::DataInputControlledAttribute &ctrlElem : ctrlAtt) {
                switch (ctrlElem.propertyType) {
                case ATTRIBUTETYPE_DATAINPUT_TIMELINE: {
                    // Quietly ignore other than number type data inputs when adjusting timeline
                    if (diDef.type == qt3ds::runtime::DataInputTypeRangedNumber) {
                        TTimeUnit endTime = 0;
                        TElement *element = getTarget(ctrlElem.elementPath.constData());
                        TComponent *component = static_cast<TComponent *>(element);
                        endTime = component->GetTimePolicy().GetLoopingDuration();

                        // Normalize the value to dataInput range
                        qreal newTime = qreal(endTime) * (qreal(value.toFloat() - diDef.min)
                                                          / qreal(diDef.max - diDef.min));
                        GotoTime(ctrlElem.elementPath.constData(), float(newTime / 1000.0));
                    }
                    break;
                }
                case ATTRIBUTETYPE_DATAINPUT_SLIDE: {
                    // Quietly ignore other than string type when adjusting slide
                    if (diDef.type == qt3ds::runtime::DataInputTypeString) {
                        const QByteArray valueStr = value.toString().toUtf8();
                        GotoSlide(ctrlElem.elementPath.constData(), valueStr.constData(),
                                  SScriptEngineGotoSlideArgs());
                    }
                    break;
                }
                    // Silently ignore invalid incoming type if it does not
                    // match with the datainput type except with type Variant, for which
                    // the incoming value is cast to target property type without checking.
                    // Caveat emptor.

                    // For Evaluator, typecheck the JS evaluation result to see if it
                    // matches with the target property.

                    // Handle ranged number similarly to generic float
                    // if it is bound to properties other
                    // than timeline animation i.e. disregard range min and max
                case ATTRIBUTETYPE_FLOAT: {
                    float valueFloat;
                    if (diDef.type == qt3ds::runtime::DataInputTypeFloat
                            || diDef.type == qt3ds::runtime::DataInputTypeRangedNumber
                            || diDef.type == qt3ds::runtime::DataInputTypeVariant) {
                        valueFloat = value.toFloat();
                    } else if (diDef.type == qt3ds::runtime::DataInputTypeEvaluator) {
                        valueFloat = callJSFunc(name, diDef, QVariant::Type::Double).toFloat();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }

                    SetAttribute(ctrlElem.elementPath.constData(),
                                 ctrlElem.attributeName.first().constData(),
                                 reinterpret_cast<const char *>(&valueFloat));
                    break;
                }
                case ATTRIBUTETYPE_FLOAT3: {
                    QVector3D valueVec;
                    if (diDef.type == qt3ds::runtime::DataInputTypeVector3
                            || diDef.type == qt3ds::runtime::DataInputTypeVariant) {
                        valueVec = value.value<QVector3D>();
                    } else if (diDef.type == qt3ds::runtime::DataInputTypeEvaluator) {
                        const QVariant res = callJSFunc(name, diDef, QVariant::Type::Vector3D);
                        valueVec = res.value<QVector3D>();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }
                    // Set the values of vector attribute components separately
                    for (int i = 0; i < 3; i++) {
                        const float val = valueVec[i];
                        SetAttribute(ctrlElem.elementPath.constData(),
                                     ctrlElem.attributeName[i].constData(),
                                     reinterpret_cast<const char *>(&val));
                    }
                    break;
                }
                case ATTRIBUTETYPE_FLOAT2:
                {
                    QVector2D valueVec;
                    if (diDef.type == qt3ds::runtime::DataInputTypeVector2
                            || diDef.type == qt3ds::runtime::DataInputTypeVariant) {
                        valueVec = value.value<QVector2D>();
                    } else if (diDef.type == qt3ds::runtime::DataInputTypeEvaluator) {
                        const QVariant res = callJSFunc(name, diDef, QVariant::Type::Vector2D);
                        valueVec = res.value<QVector2D>();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }
                    // Set the values of vector attribute components separately
                    for (int i = 0; i < 2; i++) {
                        const float val = valueVec[i];
                        SetAttribute(ctrlElem.elementPath.constData(),
                                     ctrlElem.attributeName[i].constData(),
                                     reinterpret_cast<const char *>(&val));
                    }
                    break;
                }
                case ATTRIBUTETYPE_BOOL: {
                    bool valueBool;
                    if (diDef.type == qt3ds::runtime::DataInputTypeBoolean
                            || diDef.type == qt3ds::runtime::DataInputTypeVariant) {
                        valueBool = value.toBool();
                    } else if (diDef.type == qt3ds::runtime::DataInputTypeEvaluator) {
                        valueBool = callJSFunc(name, diDef, QVariant::Type::Bool).toBool();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }

                    SetAttribute(ctrlElem.elementPath.constData(),
                                 ctrlElem.attributeName.first().constData(),
                                 reinterpret_cast<const char *>(&valueBool));
                    break;
                }
                case ATTRIBUTETYPE_STRING: {
                    QByteArray valueStr;
                    // Allow scalar number types also as inputs to string attribute
                    if (diDef.type == qt3ds::runtime::DataInputTypeString
                            || diDef.type == qt3ds::runtime::DataInputTypeRangedNumber
                            || diDef.type == qt3ds::runtime::DataInputTypeFloat
                            || diDef.type == qt3ds::runtime::DataInputTypeVariant) {
                        valueStr = value.toString().toUtf8();
                    } else if (diDef.type == qt3ds::runtime::DataInputTypeEvaluator) {
                        valueStr = callJSFunc(name, diDef, QVariant::Type::String)
                                .toString().toUtf8();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }

                    SetAttribute(ctrlElem.elementPath.constData(),
                                 ctrlElem.attributeName.first().constData(),
                                 valueStr.constData());
                    break;
                }
                default:
                    QT3DS_ALWAYS_ASSERT_MESSAGE("Unexpected data input type");
                    break;
                }
            }

            // Trigger re-evaluation of Evaluator datainputs that use this datainput
            // as source data. Do this by calling setDataInputValue for evaluator
            // with the current set value of the Evaluator (_not_ the evaluator result)
            for (auto dependent : diDef.dependents) {
                // Dependent list also contains the name of this datainput if
                // the value of this datainput is used as source data. In this case
                // obviously do not cause infinite recursion.
                if (dependent != name)
                    SetDataInputValue(dependent, diMap[dependent].value);
            }
            break;
        }
        case Q3DSDataInput::ValueRole::Max: { // switch (valueRole)
            diDef.max = value.toFloat();
            break;
        }
        case Q3DSDataInput::ValueRole::Min: { // switch (valueRole)
            diDef.min = value.toFloat();
            break;
        }
        }
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

void CQmlEngineImpl::listAllElements(TElement *root, QList<TElement *> &elements)
{
    elements.append(root);
    TElement *nextChild = root->GetChild();
    while (nextChild) {
        listAllElements(nextChild, elements);
        nextChild = nextChild->GetSibling();
    }
}

void CQmlEngineImpl::initializeDataInputsInPresentation(CPresentation &presentation,
                                                        bool isPrimary)
{
    TElement *parent = presentation.GetRoot();
    QList<TElement *> elements;
    listAllElements(parent, elements);
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    qt3ds::foundation::IStringTable &strTable(presentation.GetStringTable());

    for (TElement *element : qAsConst(elements)) {
        Option<QT3DSU32> ctrlIndex = element->FindPropertyIndex(ATTRIBUTE_CONTROLLEDPROPERTY);
        if (ctrlIndex.hasValue()) {
            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> propertyInfo =
                    element->GetPropertyByIndex(*ctrlIndex);
            UVariant *valuePtr = propertyInfo->second;
            QString valueStr =
                    QString::fromUtf8(strTable.HandleToStr(valuePtr->m_StringHandle));
            if (!valueStr.isEmpty()) {
                QStringList splitValues = valueStr.split(QChar(' '));
                for (int i = 1; i < splitValues.size(); i += 2) {
                    QString controllerName = splitValues[i - 1];
                    // remove datainput name prefix "$"
                    controllerName.remove(0, 1);
                    if (diMap.contains(controllerName)) {
                        qt3ds::runtime::DataInputControlledAttribute ctrlElem;
                        if (!isPrimary) {
                            // Prepend presentation id to element path
                            ctrlElem.elementPath = presentation.GetName();
                            ctrlElem.elementPath.append(QByteArrayLiteral(":"));
                        }
                        ctrlElem.attributeName.append(splitValues[i].toUtf8());
                        if (ctrlElem.attributeName.first() == QByteArrayLiteral("@timeline")) {
                            ctrlElem.propertyType = ATTRIBUTETYPE_DATAINPUT_TIMELINE;
                            TElement *component = &element->GetComponentParent();
                            ctrlElem.elementPath.append(component->m_Path);
                        } else if (ctrlElem.attributeName.first() == QByteArrayLiteral("@slide")) {
                            ctrlElem.propertyType = ATTRIBUTETYPE_DATAINPUT_SLIDE;
                            TElement *component = &element->GetComponentParent();
                            ctrlElem.elementPath.append(component->m_Path);
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInputTypeVector3) {
                            // special handling for vector datatype to handle
                            // expansion from <propertyname> to <propertyname>.x .y .z
                            QVector<QByteArray> attVec;
                            bool success = getAttributeVector3(
                                attVec, ctrlElem.attributeName.first().constData(),
                                element);
                            if (!attVec.empty() && success) {
                                ctrlElem.attributeName = attVec;
                                ctrlElem.elementPath.append(element->m_Path);
                                ctrlElem.propertyType = ATTRIBUTETYPE_FLOAT3;
                            } else {
                                qWarning() << __FUNCTION__ << "Property "
                                           << ctrlElem.attributeName.first()
                                           << " was not expanded to vector";
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                            }
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInputTypeVector2) {
                            // special handling for vector datatype to handle
                            // expansion from <propertyname> to <propertyname>.x .y
                            QVector<QByteArray> attVec;
                            bool success = getAttributeVector2(
                                attVec, ctrlElem.attributeName.first().constData(),
                                element);
                            if (!attVec.empty() && success) {
                                ctrlElem.attributeName = attVec;
                                ctrlElem.elementPath.append(element->m_Path);
                                ctrlElem.propertyType = ATTRIBUTETYPE_FLOAT2;
                            } else {
                                qWarning() << __FUNCTION__ << "Property "
                                    << ctrlElem.attributeName.first()
                                    << " was not expanded to vector";
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                            }
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInputTypeEvaluator) {
                            diMap[controllerName].evalFunc
                                = buildJSFunc(diMap[controllerName].evaluator);
                            auto referencedDIs = resolveDependentDatainputs(
                                        diMap[controllerName].evaluator, controllerName);
                            // add this evaluator datainput to the dependent list
                            // for those datainputs that are used in the expression
                            // for this evaluator
                            for (auto ref : referencedDIs)
                                diMap[ref].dependents.append(controllerName);

                            ctrlElem.elementPath.append(element->m_Path);
                            TStringHash attHash = CHash::HashAttribute(
                                ctrlElem.attributeName.first().constData());
                            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> attInfo
                                = element->FindProperty(attHash);
                            if (attInfo.hasValue())
                                ctrlElem.propertyType = attInfo->first.m_Type;
                        } else {
                            // all other scalar datatypes
                            ctrlElem.elementPath.append(element->m_Path);
                            TStringHash attHash = CHash::HashAttribute(
                                ctrlElem.attributeName.first().constData());
                            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> attInfo
                                = element->FindProperty(attHash);
                            if (attInfo.hasValue()) {
                                ctrlElem.propertyType = attInfo->first.m_Type;
                            } else {
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                                qWarning() << __FUNCTION__ << "Property "
                                           << ctrlElem.attributeName.first() << " not existing!";
                            }
                        }
                        qt3ds::runtime::DataInputDef &diDef = diMap[controllerName];
                        diDef.controlledAttributes.append(ctrlElem);
                    }
                }
            }
        }
    }
}

// Bit clumsy way of getting from "position" to "position .x .y .z" and enabling datainput
// support for vectorized types. UIP parser has already thrown away all vector
// type attributes and at this point we are operating with scalar components only.
// We check if this element has a property attName.x or attName.r to find out it
// we should expand property attName to XYZ or RGB vector
bool CQmlEngineImpl::getAttributeVector3(QVector<QByteArray> &outAttVec,
                                         const QByteArray &attName,
                                         TElement *elem)
{
    auto hashName = Q3DStudio::CHash::HashAttribute(attName + ".x");

    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".x");
        outAttVec.append(attName + ".y");
        outAttVec.append(attName + ".z");
        return true;
    }
    hashName = Q3DStudio::CHash::HashAttribute(attName + ".r");
    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".r");
        outAttVec.append(attName + ".g");
        outAttVec.append(attName + ".b");
        return true;
    }
    return false;
}

bool CQmlEngineImpl::getAttributeVector2(QVector<QByteArray> &outAttVec,
                                         const QByteArray &attName,
                                         TElement *elem)
{
    auto hashName = Q3DStudio::CHash::HashAttribute(attName + ".x");

    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".x");
        outAttVec.append(attName + ".y");
        return true;
    }

    hashName = Q3DStudio::CHash::HashAttribute(attName + ".u");
    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".u");
        outAttVec.append(attName + ".v");
        return true;
    }
    return false;
}

QJSValue CQmlEngineImpl::buildJSFunc(const QString &userFunc)
{
    auto res = this->m_engine.evaluate(userFunc);
    if (res.isError()) {
        qWarning() << __FUNCTION__
            << "Uncaught exception during datainput evaluation. Evaluator function" << userFunc;
    }
    return res;
}

QVariant CQmlEngineImpl::callJSFunc(const QString &controllerName,
                                    qt3ds::runtime::DataInputDef &diDef,
                                    const QVariant::Type type)
{
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    QJSValueList args;
    QVector<QString> sourceDIs = resolveDependentDatainputs(diDef.evaluator, controllerName);

    // get the most recent set values for datainput sources (arguments) in the expression
    for (auto diVal : sourceDIs)
        args << this->m_engine.toScriptValue(diMap[diVal].value);

    if (diDef.evalFunc.isCallable()) {
        QJSValue res = diDef.evalFunc.call(args);
        if (res.isError()) {
            qWarning() << __FUNCTION__ << "Error during datainput" << controllerName
                       << "evaluator call:" << res.toString() << "\nEvaluator function"
                       << diDef.evaluator;
            return QVariant::Invalid;
        }

        QVariant ret = res.toVariant();
        if (ret.isValid() && isMatchingDatatype(ret.type(), type)) {
            // further check if the result is valid number
            if (ret.type() == QVariant::Double && qIsNaN(res.toNumber())) {
                qWarning() << __FUNCTION__ << "Datainput" << controllerName << "evaluator"
                           << "result not a number (NaN)."
                           << "\nEvaluator function" << diDef.evaluator;
                return QVariant::Invalid;
            } else {
                return ret;
            }
        } else {
            qWarning() << __FUNCTION__ << "Datainput" << controllerName << "evaluator"
                       << "result not valid or matching with target attribute type. Result type"
                       << QVariant::typeToName(ret.type()) << " target attribute type "
                       << QVariant::typeToName(type) << "\nEvaluator function" << diDef.evaluator;
        }
    } else {
        qWarning() << __FUNCTION__ << "Datainput" << controllerName << "evaluator"
                   << diDef.evaluator << " not valid callable";
    }
    return QVariant::Invalid;
}

bool CQmlEngineImpl::isMatchingDatatype(QVariant::Type resultType, QVariant::Type propertyType)
{
    if (resultType == propertyType)
        return true;
    // Allow binding from numeric datainput to string target
    if ((resultType == QVariant::Double || resultType == QVariant::Int
         || resultType == QVariant::LongLong)
        && (propertyType == QVariant::Double || propertyType == QVariant::Int
            || propertyType == QVariant::LongLong || propertyType == QVariant::String)) {
        return true;
    }
    return false;
}

QVector<QString> CQmlEngineImpl::resolveDependentDatainputs(const QString &expression,
                                                            const QString &controllerName)
{
    QVector<QString> ret;
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    if (!expression.contains("function", Qt::CaseInsensitive)) {
        qWarning() << __FUNCTION__ << "Function keyword not found in datainput"
                   << controllerName << "evaluator";
        return QVector<QString>();
    }

    int argListStart = expression.indexOf("function(") + 9;
    int argListStop = expression.indexOf(')', argListStart);
    QString argstr = expression.mid(argListStart , argListStop - argListStart);
    QStringList args = argstr.split(',');

    for (auto di : args) {
        auto diTrim = di.trimmed();
        if (diMap.contains(diTrim)) {
            if (diMap[diTrim].type == qt3ds::runtime::DataInputTypeEvaluator
                && diTrim != controllerName) {
                qWarning() << __FUNCTION__ << "Invalid evaluator function in" << controllerName
                           << ". Another evaluator is used as source data.";
            } else {
                ret.append(diTrim);
            }
        } else {
            qWarning() << __FUNCTION__ << "Evaluator in" << controllerName << "evaluator"
                       << "is using unknown datainput" << diTrim << " as input argument name";
        }
    }
    return ret;
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
