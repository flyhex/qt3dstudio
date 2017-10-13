/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "Qt3DSStateEngine.h"

#include <cassert>

#ifdef ANDROID
#include <android/log.h>
#endif

#include "foundation/XML.h"
#include "foundation/FileTools.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSTime.h"

#include "UICKernelTypes.h"
#include "UICStateVisualBindingContext.h"
#include "UICTypes.h"
#include "UICTimer.h"
#include "UICStateInterpreter.h"
#include "EventPollingSystem.h"

#include "Qt3DSStateContext.h"
#include "Qt3DSStateFactory.h"
#include "Qt3DSStateLuaEngine.h"
#include "Qt3DSStateApplication.h"

namespace {

using namespace qt3ds::state;
using qt3ds::NVFoundationBase;
using qt3ds::foundation::NVConstDataRef;
using qt3ds::foundation::IDOMReader;
using qt3ds::foundation::CFileTools;
using qt3ds::foundation::Time;

struct AssetValueTypes
{
    enum Enum {
        NoAssetValue = 0,
        SCXML,
        Behavior,
    };
};

struct SAssetBase
{
    CRegisteredString m_Id;
    CRegisteredString m_Src;
    SAssetBase(CRegisteredString inId = CRegisteredString(),
               CRegisteredString inSrc = CRegisteredString())
        : m_Id(inId)
        , m_Src(inSrc)
    {
    }
    bool operator==(const SAssetBase &inOther) const
    {
        return m_Id == inOther.m_Id && m_Src == inOther.m_Src;
    }
    template <typename TRemapper>
    void Remap(TRemapper &item)
    {
        item.Remap(m_Id);
        item.Remap(m_Src);
    }
};

struct SSCXMLAsset : public SAssetBase
{
    CRegisteredString m_Datamodel;
    SSCXMLAsset(CRegisteredString inId = CRegisteredString(),
                CRegisteredString inRelPath = CRegisteredString(),
                CRegisteredString inDatamodel = CRegisteredString())
        : SAssetBase(inId, inRelPath)
        , m_Datamodel(inDatamodel)
    {
    }
    bool operator==(const SSCXMLAsset &inOther) const
    {
        return SAssetBase::operator==(inOther) && m_Datamodel == inOther.m_Datamodel;
    }
    template <typename TRemapper>
    void Remap(TRemapper &item)
    {
        SAssetBase::Remap(item);
        item.Remap(m_Datamodel);
    }
};

struct SBehaviorAsset : public SAssetBase
{
    Q3DStudio::INT32 m_Handle;
    SBehaviorAsset(CRegisteredString inId = CRegisteredString(),
                   CRegisteredString inRelPath = CRegisteredString(), Q3DStudio::INT32 inHandle = 0)
        : SAssetBase(inId, inRelPath)
        , m_Handle(inHandle)
    {
    }
    bool operator==(const SBehaviorAsset &inOther) const
    {
        return SAssetBase::operator==(inOther) && m_Handle == inOther.m_Handle;
    }
    template <typename TRemapper>
    void Remap(TRemapper &item)
    {
        m_Handle = -1;
        SAssetBase::Remap(item);
    }
};
}

namespace qt3ds {
namespace foundation {
    template <>
    struct DestructTraits<SSCXMLAsset>
    {
        void destruct(SSCXMLAsset &) {}
    };
    template <>
    struct DestructTraits<SBehaviorAsset>
    {
        void destruct(SBehaviorAsset &) {}
    };
}
}

namespace {

template <typename TDataType>
struct SAssetValueTypeMap
{
};

struct SAssetValueUnionTraits
{
    typedef AssetValueTypes::Enum TIdType;
    enum {
        TBufferSize = sizeof(SSCXMLAsset),
    };

    static TIdType getNoDataId() { return AssetValueTypes::NoAssetValue; }

    template <typename TDataType>
    static TIdType getType()
    {
        return SAssetValueTypeMap<TDataType>().GetType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case AssetValueTypes::SCXML:
            return inVisitor(*NVUnionCast<SSCXMLAsset *>(inData));
        case AssetValueTypes::Behavior:
            return inVisitor(*NVUnionCast<SBehaviorAsset *>(inData));
        default:
            QT3DS_ASSERT(false);
        case AssetValueTypes::NoAssetValue:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case AssetValueTypes::SCXML:
            return inVisitor(*NVUnionCast<const SSCXMLAsset *>(inData));
        case AssetValueTypes::Behavior:
            return inVisitor(*NVUnionCast<const SBehaviorAsset *>(inData));
        default:
            QT3DS_ASSERT(false);
        case AssetValueTypes::NoAssetValue:
            return inVisitor();
        }
    }
};

template <>
struct SAssetValueTypeMap<SSCXMLAsset>
{
    static AssetValueTypes::Enum GetType() { return AssetValueTypes::SCXML; }
};
template <>
struct SAssetValueTypeMap<SBehaviorAsset>
{
    static AssetValueTypes::Enum GetType() { return AssetValueTypes::Behavior; }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<SAssetValueUnionTraits,
                                                         SAssetValueUnionTraits::TBufferSize>,
                       SAssetValueUnionTraits::TBufferSize>
        TAssetValueUnionType;

struct SAssetValue : public TAssetValueUnionType
{
    SAssetValue() {}

    SAssetValue(const SAssetValue &inOther)
        : TAssetValueUnionType(static_cast<const TAssetValueUnionType &>(inOther))
    {
    }

    template <typename TDataType>
    SAssetValue(const TDataType &inDt)
        : TAssetValueUnionType(inDt)
    {
    }

    SAssetValue &operator=(const SAssetValue &inOther)
    {
        TAssetValueUnionType::operator=(inOther);
        return *this;
    }

    bool operator==(const SAssetValue &inOther) const
    {
        return TAssetValueUnionType::operator==(inOther);
    }
    bool operator!=(const SAssetValue &inOther) const
    {
        return TAssetValueUnionType::operator!=(inOther);
    }

    bool empty() const { return getType() == AssetValueTypes::NoAssetValue; }

    CRegisteredString GetSource()
    {
        if (empty())
            return CRegisteredString();
        return reinterpret_cast<SAssetBase *>(m_Data)->m_Src;
    }
};

struct SRefCountedAssetValue : public SAssetValue
{
    NVFoundationBase &m_Foundation;
    QT3DSI32 mRefCount;
    SRefCountedAssetValue(NVFoundationBase &fnd)
        : SAssetValue()
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }
    SRefCountedAssetValue(NVFoundationBase &fnd, const SAssetValue &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }
    SRefCountedAssetValue(NVFoundationBase &fnd, const SBehaviorAsset &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }
    SRefCountedAssetValue(NVFoundationBase &fnd, const SSCXMLAsset &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())
};

struct SAppXMLErrorHandler : public qt3ds::foundation::CXmlErrorHandler
{
    NVFoundationBase &m_Foundation;
    const char8_t *m_FilePath;
    SAppXMLErrorHandler(NVFoundationBase &fnd, const char8_t *filePath)
        : m_Foundation(fnd)
        , m_FilePath(filePath)
    {
    }

    void OnXmlError(TXMLCharPtr errorName, int line, int /*column*/) override
    {
        qCCritical(INVALID_OPERATION, m_FilePath, line, "%s", errorName);
    }
};

// State event handler
struct SStateEventHandlerKey
{
    SStateEventHandlerKey(INDDStateEngine::EStateEvent inEvent,
                          const qt3ds::foundation::CRegisteredString &inEventId)
        : m_Event(inEvent)
        , m_EventId(inEventId)
    {
    }
    INDDStateEngine::EStateEvent m_Event;
    qt3ds::foundation::CRegisteredString m_EventId;
    bool operator<(const SStateEventHandlerKey &inArg) const
    {
        if (m_Event != inArg.m_Event)
            return m_Event < inArg.m_Event;
        return m_EventId < inArg.m_EventId;
    }
};
struct SStateCallbackConnection;
typedef eastl::multimap<SStateEventHandlerKey, SStateCallbackConnection *> TStateEventHandlerMap;

struct SStateInterpreterResource;

struct SStateCallbackConnection : public INDDStateEngine::IStateEventHandlerConnection
{
    NVAllocatorCallback &m_Alloc;
    SStateInterpreterResource *m_StateInterpreterResource;
    INDDStateEngine::IStateEventHandler &m_Handler;

    volatile QT3DSI32 mRefCount;

    SStateCallbackConnection(NVAllocatorCallback &inAlloc,
                             SStateInterpreterResource &inStateInterpreterResource,
                             INDDStateEngine::IStateEventHandler &inHandler)
        : m_Alloc(inAlloc)
        , m_StateInterpreterResource(&inStateInterpreterResource)
        , m_Handler(inHandler)
        , mRefCount(0)
    {
    }
    virtual ~SStateCallbackConnection();
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Alloc)
};

struct SStateInterpreterEventHandler : public qt3ds::state::IStateInterpreterEventHandler
{
public:
    SStateInterpreterResource &m_StateInterpreterResource;

    SStateInterpreterEventHandler(SStateInterpreterResource &inStateInterpreterResource)
        : m_StateInterpreterResource(inStateInterpreterResource)
    {
    }

    virtual ~SStateInterpreterEventHandler() {}

    void OnInterpreterEvent(qt3ds::state::InterpreterEventTypes::Enum inEvent,
                                    qt3ds::foundation::CRegisteredString inEventId) override;
};

struct SStateEventHandlersWithLock
{
    SStateEventHandlersWithLock(qt3ds::NVAllocatorCallback &inAllocator)
        : m_HandlersLock(inAllocator)
    {
    }
    TStateEventHandlerMap m_StateEventHandlers;
    qt3ds::foundation::Mutex m_HandlersLock;
};

struct SStateInterpreterResource : public qt3ds::foundation::NVRefCounted
{
public:
    qt3ds::NVAllocatorCallback &m_Allocator;
    qt3ds::state::IStateInterpreter *m_StateInterpreter;

private:
    SStateEventHandlersWithLock m_StateEventHandlersWithLock;

public:
    SStateInterpreterEventHandler *m_StateInterpreterEventHandler;
    qt3ds::state::TSignalConnectionPtr m_SignalConnection;

    volatile QT3DSI32 mRefCount;

    SStateInterpreterResource(qt3ds::NVAllocatorCallback &inAllocator)
        : m_Allocator(inAllocator)
        , m_StateInterpreter(0)
        , m_StateEventHandlersWithLock(m_Allocator)
        , m_StateInterpreterEventHandler(0)
        , m_SignalConnection(0)
        , mRefCount(0)
    {
        m_StateInterpreterEventHandler = QT3DS_NEW(m_Allocator, SStateInterpreterEventHandler)(*this);
    }
    virtual ~SStateInterpreterResource()
    {
        m_StateInterpreter = 0;
        m_SignalConnection = 0;
        SStateInterpreterEventHandler *theStateInterpreterEventHandler =
            m_StateInterpreterEventHandler;
        m_StateInterpreterEventHandler = 0;
        NVDelete(m_Allocator, theStateInterpreterEventHandler);
        {
            qt3ds::foundation::Mutex::ScopedLock theLock(m_StateEventHandlersWithLock.m_HandlersLock);
            for (TStateEventHandlerMap::iterator
                     theIter = m_StateEventHandlersWithLock.m_StateEventHandlers.begin(),
                     theEnd = m_StateEventHandlersWithLock.m_StateEventHandlers.end();
                 theIter != theEnd; ++theIter)
                theIter->second->m_StateInterpreterResource = 0;
        }
    }

    void DeleteHandlerByConnection(SStateCallbackConnection &inConnection)
    {
        qt3ds::foundation::Mutex::ScopedLock theLock(m_StateEventHandlersWithLock.m_HandlersLock);
        TStateEventHandlerMap &theStateEventHandlers =
            m_StateEventHandlersWithLock.m_StateEventHandlers;
        for (TStateEventHandlerMap::iterator theIter = theStateEventHandlers.begin(),
                                             theEnd = theStateEventHandlers.end();
             theIter != theEnd;) {
            if (theIter->second == &inConnection)
                theStateEventHandlers.erase(theIter++);
            else
                ++theIter;
        }
    }

    bool InsertConnection(const SStateEventHandlerKey &inKey,
                          SStateCallbackConnection &inConnection)
    {
        qt3ds::foundation::Mutex::ScopedLock theLock(m_StateEventHandlersWithLock.m_HandlersLock);
        eastl::pair<TStateEventHandlerMap::iterator, bool> theResult =
            m_StateEventHandlersWithLock.m_StateEventHandlers.insert(
                eastl::make_pair<SStateEventHandlerKey, SStateCallbackConnection *>(inKey,
                                                                                    &inConnection));
        return theResult.second;
    }

    void TriggerCallbackOnEvent(qt3ds::state::InterpreterEventTypes::Enum inEvent,
                                const qt3ds::foundation::CRegisteredString &inEventId)
    {
        qt3ds::foundation::Mutex::ScopedLock theLock(m_StateEventHandlersWithLock.m_HandlersLock);
        eastl::pair<TStateEventHandlerMap::iterator, TStateEventHandlerMap::iterator> theRange =
            m_StateEventHandlersWithLock.m_StateEventHandlers.equal_range(
                SStateEventHandlerKey((INDDStateEngine::EStateEvent)inEvent, inEventId));
        for (TStateEventHandlerMap::iterator theIter = theRange.first; theIter != theRange.second;
             ++theIter)
            theIter->second->m_Handler.OnEvent();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)
};

SStateCallbackConnection::~SStateCallbackConnection()
{
    if (m_StateInterpreterResource) {
        m_StateInterpreterResource->DeleteHandlerByConnection(*this);
    }
}

void SStateInterpreterEventHandler::OnInterpreterEvent(
    qt3ds::state::InterpreterEventTypes::Enum inEvent, qt3ds::foundation::CRegisteredString inEventId)
{
    qCDebug (qt3ds::TRACE_INFO) << "SStateInterpreterEventHandler: OnInterpreterEvent: event type: "
                                << inEvent <<  " event ID: " << inEventId.c_str();
    m_StateInterpreterResource.TriggerCallbackOnEvent(inEvent, inEventId);
}

typedef eastl::map<qt3ds::foundation::CRegisteredString,
                   qt3ds::foundation::NVScopedRefCounted<SStateInterpreterResource>>
    TStateInterpreterResourceMap;

struct SStateInterpreterCreateCallback
    : public qt3ds::state::INDDStateFactory::IStateInterpreterCreateCallback
{
public:
    TStateInterpreterResourceMap &m_StateInterpreterResourceMap;
    SStateInterpreterCreateCallback(TStateInterpreterResourceMap &inStateInterpreterResourceMap)
        : m_StateInterpreterResourceMap(inStateInterpreterResourceMap)
    {
    }
    virtual ~SStateInterpreterCreateCallback() {}
    void OnCreate(const qt3ds::foundation::CRegisteredString &inId,
                          qt3ds::state::IStateInterpreter &inStateInterpreter) override
    {
        TStateInterpreterResourceMap::iterator thePos = m_StateInterpreterResourceMap.find(inId);
        if (thePos == m_StateInterpreterResourceMap.end()) {
            qCCritical(qt3ds::TRACE_INFO)
                    << "SStateInterpreterCreateCallback::OnCreate: Interpreter ID not found";
            return;
        }
        thePos->second->m_StateInterpreter = &inStateInterpreter;
        thePos->second->m_SignalConnection = inStateInterpreter.RegisterEventHandler(
            *thePos->second->m_StateInterpreterEventHandler);
    }
};

struct SApp : public qt3ds::state::INDDStateApplication
{
public:
    typedef nvvector<eastl::pair<CRegisteredString, NVScopedRefCounted<SRefCountedAssetValue>>>
        TIdAssetList;
    typedef nvhash_map<CRegisteredString, NVScopedRefCounted<SRefCountedAssetValue>> TIdAssetMap;

    SNDDStateContext &m_Context;
    TStateInterpreterResourceMap &m_StateInterpreters;
    TIdAssetList m_OrderedAssets;
    TIdAssetMap m_AssetMap;
    nvvector<eastl::pair<SBehaviorAsset, bool>> m_Behaviors;
    Q3DStudio::INT32 m_FrameCount;
    QT3DSU64 m_LastFrameStartTime;
    QT3DSU64 m_ThisFrameStartTime;
    double m_MillisecondsSinceLastFrame;
    bool m_IsLoaded;

    QT3DSI32 mRefCount;

    SApp(SNDDStateContext &inContext, TStateInterpreterResourceMap &inStateInterpreters)
        : m_Context(inContext)
        , m_StateInterpreters(inStateInterpreters)
        , m_OrderedAssets(m_Context.GetAllocator(), "SApp::m_OrderedAssets")
        , m_AssetMap(m_Context.GetAllocator(), "SApp::m_AssetMap")
        , m_Behaviors(m_Context.GetAllocator(), "SApp::m_Behaviors")
        , m_FrameCount(0)
        , m_LastFrameStartTime(0)
        , m_ThisFrameStartTime(0)
        , m_MillisecondsSinceLastFrame(0)
        , m_IsLoaded(false)
        , mRefCount(0)
    {
    }
    virtual ~SApp()
    {
        qCDebug (qt3ds::TRACE_INFO) << "SApp: destructing";
    }

    double GetMillisecondsSinceLastFrame() override { return m_MillisecondsSinceLastFrame; }

    Q3DStudio::INT32 GetFrameCount() override { return m_FrameCount; }

    CRegisteredString RegisterStr(const char8_t *inStr)
    {
        return m_Context.GetStringTable().RegisterStr(inStr);
    }

    bool LoadUIA(IDOMReader &inReader, NVFoundationBase &fnd)
    {
        m_IsLoaded = false;
        NVConstDataRef<qt3ds::state::SElementReference> theStateReferences;
        {
            IDOMReader::Scope __preparseScope(inReader);
            theStateReferences =
                m_Context.m_Factory->GetVisualStateContext().PreParseDocument(inReader);
        }
        {
            IDOMReader::Scope __assetsScope(inReader);
            if (!inReader.MoveToFirstChild("assets")) {
                qCCritical(INVALID_OPERATION, "UIA input xml doesn't contain <assets> tag; load failed");
                return m_IsLoaded;
            }

            eastl::string pathString;

            for (bool success = inReader.MoveToFirstChild(); success;
                 success = inReader.MoveToNextSibling()) {
                IDOMReader::Scope __assetScope(inReader);
                const char8_t *itemId("");
                inReader.UnregisteredAtt("id", itemId);
                const char8_t *src = "";
                inReader.UnregisteredAtt("src", src);
                pathString.clear();
                if (!isTrivial(src))
                    CFileTools::CombineBaseAndRelative(m_Context.GetProjectDir(), src, pathString);

                const char8_t *assetName = inReader.GetElementName();
                if (AreEqual(assetName, "statemachine")) {
                    const char8_t *dm = "";
                    inReader.UnregisteredAtt("datamodel", dm);
                    LoadSCXML(itemId, src, dm, false);
                    RegisterAsset(
                        SSCXMLAsset(RegisterStr(itemId), RegisterStr(src), RegisterStr(dm)));
                } else if (AreEqual(assetName, "behavior")) {
                    SBehaviorAsset theAsset(RegisterStr(itemId), RegisterStr(src), 0);
                    RegisterAsset(theAsset);
                } else {
                    qCWarning(WARNING, "Unrecognized <assets> child %s", assetName);
                }
            }
        } // end assets scope

        {
            IDOMReader::Scope __machineScope(inReader);
            m_Context.m_Factory->GetVisualStateContext().LoadVisualStateMapping(inReader);
        }

        for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
            SAssetValue &theAsset = *m_OrderedAssets[idx].second;
            eastl::string thePathStr;
            CFileTools::CombineBaseAndRelative(m_Context.GetProjectDir(), theAsset.GetSource(),
                                               thePathStr);
            switch (theAsset.getType()) {
            case AssetValueTypes::Behavior: {
                SBehaviorAsset &theBehaviorAsset = *theAsset.getDataPtr<SBehaviorAsset>();
                Q3DStudio::INT32 scriptId =
                    m_Context.m_Factory->GetScriptEngine().InitializeApplicationBehavior(
                        theBehaviorAsset.m_Src);
                if (scriptId == 0)
                    qCCritical(INVALID_OPERATION, "Unable to load application behavior %s",
                        theBehaviorAsset.m_Src.c_str());
                else {
                    theBehaviorAsset.m_Handle = scriptId;
                    m_Behaviors.push_back(eastl::make_pair(theBehaviorAsset, false));
                }
            } break;
            // SCXML, NoAssetValue do not need processing here
            default:
                break;
            }
        }
        m_IsLoaded = true;
        return m_IsLoaded;
    }

    bool LoadSCXML(const char *inId, const char *inSourcePath, const char *inDatamodel,
                   bool inIsStandalone = true)
    {
        if (inIsStandalone)
            m_IsLoaded = false;

        m_Context.m_Factory->GetVisualStateContext().LoadStateMachine(inId, inSourcePath,
                                                                      nonNull(inDatamodel));
        m_StateInterpreters.insert(
            eastl::make_pair<qt3ds::foundation::CRegisteredString,
                             qt3ds::foundation::NVScopedRefCounted<SStateInterpreterResource>>(
                RegisterStr(inId), QT3DS_NEW(m_Context.GetAllocator(),
                                          SStateInterpreterResource)(m_Context.GetAllocator())));

        if (inIsStandalone) {
            m_IsLoaded = true;
            return m_IsLoaded;
        } else
            return true;
    }

    bool IsLoaded() { return m_IsLoaded; }

    void Update()
    {
        if (!IsLoaded()) {
            qCCritical(qt3ds::TRACE_INFO) << "CNDDStateEngine: Application not loaded";
            return;
        }
        m_ThisFrameStartTime = qt3ds::foundation::Time::getCurrentCounterValue();
        if (m_LastFrameStartTime) {
            QT3DSU64 durationSinceLastFrame = m_ThisFrameStartTime - m_LastFrameStartTime;
            m_MillisecondsSinceLastFrame =
                qt3ds::foundation::Time::sCounterFreq.toTensOfNanos(durationSinceLastFrame)
                * (1.0 / 100000.0);
        } else
            m_MillisecondsSinceLastFrame = 0;

        // if ( m_ProtocolSocket ) m_ProtocolSocket->MessagePump();
        ++m_FrameCount;

        // Update any state systems
        m_Context.m_Factory->GetVisualStateContext().Initialize();

        // First off, update any application level behaviors.
        INDDStateScriptBridge &theScriptEngine = m_Context.m_Factory->GetScriptEngine();
        for (QT3DSU32 idx = 0, end = m_Behaviors.size(); idx < end; ++idx) {
            eastl::pair<SBehaviorAsset, bool> &entry(m_Behaviors[idx]);
            if (!entry.second) {
                entry.second = true;
                theScriptEngine.ExecuteApplicationScriptFunction(entry.first.m_Handle,
                                                                 "onInitialize");
            }
        }

        // Start any state systems
        m_Context.m_Factory->GetVisualStateContext().Start();

        for (QT3DSU32 idx = 0, end = m_Behaviors.size(); idx < end; ++idx) {
            eastl::pair<SBehaviorAsset, bool> &entry(m_Behaviors[idx]);
            theScriptEngine.ExecuteApplicationScriptFunction(entry.first.m_Handle, "onUpdate");
        }

        m_Context.m_Factory->GetVisualStateContext().Update();

        // if ( m_ProtocolSocket ) {
        //	m_ProtocolSocket->MessagePump();
        //	if ( m_ProtocolSocket->Connected() == false )
        //		exit( 0 );
        //}

        // if ( m_ProtocolSocket ) m_ProtocolSocket->MessagePump();

        if (!m_Context.m_Factory->GetEventSystem().GetAndClearEventFetchedFlag())
            m_Context.m_Factory->GetEventSystem().PurgeEvents(); // GetNextEvents of event system
                                                                 // has not been called in this
                                                                 // round, so clear events to avoid
                                                                 // events to be piled up

        // QT3DSU64 updateEndTime = qt3ds::foundation::Time::getCurrentCounterValue();
        // QT3DSU64 updateDuration = updateEndTime - m_ThisFrameStartTime;
        // double millis = qt3ds::foundation::Time::sCounterFreq.toTensOfNanos( updateDuration ) * (1.0
        // / 100000);

        //(void)millis;

        /*if ( millis > 30.0 )
        {
                m_CoreFactory->GetFoundation().error( NVErrorCode::eDEBUG_INFO, __FILE__, __LINE__,
        "UIC Long Frame: %3.2fms", millis );
                //Useful for figuring out where the frame time comes from.
                m_CoreFactory->GetPerfTimer().OutputTimerData();

        }
        else*/
        //	m_CoreFactory->GetPerfTimer().ResetTimerData();

        fflush(stdout);
        m_LastFrameStartTime = m_ThisFrameStartTime;

        theScriptEngine.StepGC();
    }

    template <typename TAssetType>
    void RegisterAsset(const TAssetType &inAsset)
    {
        NVScopedRefCounted<SRefCountedAssetValue> theValue(QT3DS_NEW(
            m_Context.GetAllocator(), SRefCountedAssetValue)(*m_Context.m_Foundation, inAsset));
        if (inAsset.m_Id.IsValid()) {
            m_AssetMap.insert(eastl::make_pair(inAsset.m_Id, theValue));
        }

        m_OrderedAssets.push_back(eastl::make_pair(inAsset.m_Id, theValue));
    }

    void addRef() override { atomicIncrement(&mRefCount); }

    void release() override
    {
        atomicDecrement(&mRefCount);
        if (mRefCount <= 0) {
            NVDelete(m_Context.GetAllocator(), this);
        }
    }
};

class CNDDStateEngine : public qt3ds::state::INDDStateEngine
{
public:
    qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> m_Foundation;
    qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> m_StringTable;
    qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IInputStreamFactory> m_InputStreamFactory;
    Q3DStudio::ITimeProvider &m_TimeProvider;

    qt3ds::foundation::NVScopedRefCounted<SNDDStateContext> m_Context;
    qt3ds::foundation::NVScopedRefCounted<SApp> m_App;
    eastl::string m_FileName;

    bool m_Verbose;

    TStateInterpreterResourceMap m_StateInterpreters;
    SStateInterpreterCreateCallback m_StateInterpreterCreateCallback;

    CNDDStateEngine(
        qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> inFoundation,
        qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> inStringTable,
        qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IInputStreamFactory> inInputStreamFactory,
        Q3DStudio::ITimeProvider &inTimeProvider);
    virtual ~CNDDStateEngine();

    bool Load(const char *inApplicationPath) override;
    bool Step() override;
    virtual TStateEventHandlerConnectionPtr
    RegisterEventHandler(EStateEvent inEvent, const char *inEventId, IStateEventHandler &inHandler) override;
    virtual TStateEventHandlerConnectionPtr RegisterEventHandler(EStateEvent inEvent,
                                                                 const char *inEventId,
                                                                 IStateEventHandler &inHandler,
                                                                 const char *inMachineId);
    void FireEvent(const char *inEventName, unsigned long long inDelay,
                           const char *inCancelId, bool inIsExternal) override;
    virtual void FireEvent(const char *inEventName, unsigned long long inDelay,
                           const char *inCancelId, bool inIsExternal, const char *inMachineId);
    void FireEvent(const char *inEventName, bool inIsExternal) override;
    virtual void FireEvent(const char *inEventName, bool inIsExternal, const char *inMachineId);
    void CancelEvent(const char *inCancelId) override;
    virtual void CancelEvent(const char *inCancelId, const char *inMachineId);

    bool ShouldEnableHeavyLogging() { return m_Verbose; }
};


CNDDStateEngine::CNDDStateEngine(
    qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> inFoundation,
    qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> inStringTable,
    qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IInputStreamFactory> inInputStreamFactory,
    Q3DStudio::ITimeProvider &inTimeProvider)
    : m_Foundation(inFoundation)
    , m_StringTable(inStringTable)
    , m_InputStreamFactory(inInputStreamFactory)
    , m_TimeProvider(inTimeProvider)
    , m_Context(0)
    , m_App(0)
    , m_Verbose(true)
    , m_StateInterpreterCreateCallback(m_StateInterpreters)
{
}

CNDDStateEngine::~CNDDStateEngine()
{
    qCDebug (qt3ds::TRACE_INFO) << "CNDDStateEngine destructing";
    m_App = 0;
    m_Context = 0;
}

bool CNDDStateEngine::Load(const char *inApplicationPath)
{
    qCDebug (qt3ds::TRACE_INFO) << "CNDDStateEngine: Load";

    // SStackPerfTimer __loadTimer( m_CoreFactory->GetPerfTimer(), "Application: Begin Load" );
    //(void)inCommandLine;

    m_StateInterpreters.clear();
    m_App = 0;
    m_Context = 0;

    eastl::string directory;
    eastl::string filename;
    eastl::string extension;
    CFileTools::Split(inApplicationPath, directory, filename, extension);
    eastl::string projectDirectory(directory);
    size_t binaryPos = projectDirectory.rfind("binary");
    if (binaryPos != eastl::string::npos && binaryPos >= projectDirectory.size() - 7)
        CFileTools::GetDirectory(projectDirectory);

    m_Context = QT3DS_NEW(m_Foundation->getAllocator(),
                       SNDDStateContext)(m_Foundation, m_StringTable, m_InputStreamFactory,
                                         m_TimeProvider, projectDirectory.c_str());
    m_Context->m_Factory->SetStateInterpreterCreateCallback(m_StateInterpreterCreateCallback);

    m_Context->m_InputStreamFactory->AddSearchDirectory(projectDirectory.c_str());
    NVFoundationBase &fnd(*m_Foundation);
    // m_CommandLineSettings.Parse( inCommandLine );

    m_FileName = filename;
    bool retval = false;
    if (extension.comparei("scxml") == 0) {
        m_App = QT3DS_NEW(fnd.getAllocator(), SApp)(*m_Context, m_StateInterpreters);
        m_Context->m_Factory->SetApplication(m_App);
        retval = m_App->LoadSCXML("logic_main", (filename + "." + extension).c_str(), "");
    }
    // else if ( extension.comparei( "uia" ) == 0 )
    //{
    //	//ConnectDebugger();
    //	CFileSeekableIOStream inputStream( inApplicationPath, FileReadFlags() );
    //	if ( inputStream.IsOpen() )
    //	{
    //		NVScopedRefCounted<IStringTable> strTable( IStringTable::CreateStringTable(
    //fnd.getAllocator() ) );
    //		NVScopedRefCounted<IDOMFactory> domFactory( IDOMFactory::CreateDOMFactory(
    //fnd.getAllocator(), strTable ) );
    //		SAppXMLErrorHandler errorHandler( fnd, inApplicationPath );
    //		eastl::pair<SNamespacePairNode*, SDOMElement*> readResult = CDOMSerializer::Read(
    //*domFactory, inputStream, &errorHandler );
    //		if ( !readResult.second )  {
    //			fnd.error( QT3DS_INVALID_PARAMETER, "%s doesn't appear to be valid xml", inApplicationPath
    //);
    //		}
    //		else {
    //			m_App = QT3DS_NEW( fnd.getAllocator(), SApp )( *m_Context, m_StateInterpreters
    //);
    //			m_Context->m_Factory->SetApplication( m_App );

    //			NVScopedRefCounted<IDOMReader> domReader = IDOMReader::CreateDOMReader(
    //fnd.getAllocator(), *readResult.second, strTable, domFactory );
    //			retval = m_App->LoadUIA( *domReader, fnd );
    //		}
    //	}
    //	else
    //	{
    //		fnd.error( QT3DS_INVALID_PARAMETER, "Unable to open input file %s", inApplicationPath
    //);
    //	}
    //}
    // else if ( extension.comparei( "uiab" ) == 0 )
    //{
    //	retval = loadbinary( infilepath );
    //}
    else {
        QT3DS_ASSERT(false);
    }
    m_Context->m_Factory->GetScriptEngine();
    return retval;
}

bool CNDDStateEngine::Step()
{
    if (!m_App) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine: Application not initialized "
                << "may due to unsuccessful load";
        return false;
    }
    m_App->Update();
    return true;
}

INDDStateEngine::TStateEventHandlerConnectionPtr
CNDDStateEngine::RegisterEventHandler(EStateEvent inEvent, const char *inEventId,
                                      IStateEventHandler &inHandler)
{
    return RegisterEventHandler(inEvent, inEventId, inHandler, "logic_main");
}

INDDStateEngine::TStateEventHandlerConnectionPtr
CNDDStateEngine::RegisterEventHandler(EStateEvent inEvent, const char *inEventId,
                                      IStateEventHandler &inHandler, const char *inMachineId)
{
    TStateInterpreterResourceMap::iterator thePos =
        m_StateInterpreters.find(m_StringTable->RegisterStr(inMachineId));
    if (thePos == m_StateInterpreters.end()) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::RegisterEventHandler: "
                << "Cannot find state machine " << inMachineId;
        return 0;
    }
    SStateInterpreterResource *theStateInterpreterResource = thePos->second;
    if (!theStateInterpreterResource) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::RegisterEventHandler: "
                << "Find null state machine " << inMachineId;
        return 0;
    }
    SStateCallbackConnection *theConnection =
        QT3DS_NEW(m_Foundation->getAllocator(), SStateCallbackConnection)(
            m_Foundation->getAllocator(), *theStateInterpreterResource, inHandler);
    theStateInterpreterResource->InsertConnection(
        SStateEventHandlerKey(inEvent, m_StringTable->RegisterStr(inEventId)), *theConnection);
    return theConnection;
}

void CNDDStateEngine::FireEvent(const char *inEventName, unsigned long long inDelay,
                                const char *inCancelId, bool inIsExternal)
{
    FireEvent(inEventName, inDelay, inCancelId, inIsExternal, "logic_main");
}

void CNDDStateEngine::FireEvent(const char *inEventName, unsigned long long inDelay,
                                const char *inCancelId, bool inIsExternal, const char *inMachineId)
{
    if (!inEventName || !inCancelId || !inMachineId) {
        qCCritical (qt3ds::INVALID_PARAMETER) << "CNDDStateEngine::FireEvent: null parameter";
        return;
    }
    TStateInterpreterResourceMap::iterator thePos =
        m_StateInterpreters.find(m_StringTable->RegisterStr(inMachineId));
    if (thePos == m_StateInterpreters.end()) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: Cannot find state machine " << inMachineId;
        return;
    }
    SStateInterpreterResource *theStateInterpreterResource = thePos->second;
    if (!theStateInterpreterResource) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: Find null state machine " << inMachineId;
        return;
    }
    qt3ds::state::IStateInterpreter *theStateInterpreter =
        theStateInterpreterResource->m_StateInterpreter;
    if (!theStateInterpreter) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: State interpreter not ready";
        return;
    }
    theStateInterpreter->QueueEvent(inEventName, (qt3ds::QT3DSU64)inDelay,
                                    m_StringTable->RegisterStr(inCancelId), inIsExternal);
}

void CNDDStateEngine::FireEvent(const char *inEventName, bool inIsExternal)
{
    FireEvent(inEventName, inIsExternal, "logic_main");
}

void CNDDStateEngine::FireEvent(const char *inEventName, bool inIsExternal, const char *inMachineId)
{
    if (!inEventName || !inMachineId) {
        qCCritical (qt3ds::INVALID_PARAMETER) << "CNDDStateEngine::FireEvent: null parameter";
        return;
    }
    TStateInterpreterResourceMap::iterator thePos =
        m_StateInterpreters.find(m_StringTable->RegisterStr(inMachineId));
    if (thePos == m_StateInterpreters.end()) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: Cannot find state machine " << inMachineId;
        return;
    }
    SStateInterpreterResource *theStateInterpreterResource = thePos->second;
    if (!theStateInterpreterResource) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: Find null state machine " << inMachineId;
        return;
    }
    qt3ds::state::IStateInterpreter *theStateInterpreter =
        theStateInterpreterResource->m_StateInterpreter;
    if (!theStateInterpreter) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: State interpreter not ready";
        return;
    }
    theStateInterpreter->QueueEvent(inEventName, inIsExternal);
}

void CNDDStateEngine::CancelEvent(const char *inCancelId)
{
    CancelEvent(inCancelId, "logic_main");
}

void CNDDStateEngine::CancelEvent(const char *inCancelId, const char *inMachineId)
{
    if (!inCancelId || !inMachineId) {
        qCCritical (qt3ds::INVALID_PARAMETER) << "CNDDStateEngine::FireEvent: null parameter";
        return;
    }
    TStateInterpreterResourceMap::iterator thePos =
        m_StateInterpreters.find(m_StringTable->RegisterStr(inMachineId));
    if (thePos == m_StateInterpreters.end()) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: Cannot find state machine " << inMachineId;
        return;
    }
    SStateInterpreterResource *theStateInterpreterResource = thePos->second;
    if (!theStateInterpreterResource) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: Find null state machine " << inMachineId;
        return;
    }
    qt3ds::state::IStateInterpreter *theStateInterpreter =
        theStateInterpreterResource->m_StateInterpreter;
    if (!theStateInterpreter) {
        qCCritical (qt3ds::INVALID_OPERATION)
                << "CNDDStateEngine::FireEvent: State interpreter not ready";
        return;
    }
    theStateInterpreter->CancelEvent(m_StringTable->RegisterStr(inCancelId));
}
}

namespace {

class SUICFNDTimer : public Q3DStudio::ITimeProvider
{
    Q3DStudio::INT64 GetCurrentTimeMicroSeconds() override
    {
        return qt3ds::foundation::Time::getCurrentTimeInTensOfNanoSeconds() / 100;
    }
};

qt3ds::foundation::MallocAllocator g_BaseAllocator;

class CNDDStateEngineDefault : public qt3ds::state::INDDStateEngine
{
public:
    qt3ds::foundation::CAllocator m_Allocator;
    qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> m_Foundation;
    SUICFNDTimer m_TimeProvider;
    CNDDStateEngine &m_RealEngine;

    CNDDStateEngineDefault()
        : m_Allocator()
        , m_Foundation(NVCreateFoundation(QT3DS_FOUNDATION_VERSION, m_Allocator))
        , m_RealEngine(*QT3DS_NEW(m_Foundation->getAllocator(), CNDDStateEngine)(
              m_Foundation, &qt3ds::foundation::IStringTable::CreateStringTable(m_Allocator),
              &qt3ds::state::IInputStreamFactory::Create(*m_Foundation), m_TimeProvider))
    {
    }
    virtual ~CNDDStateEngineDefault() { NVDelete(m_Foundation->getAllocator(), &m_RealEngine); }

    bool Load(const char *inApplicationPath) override
    {
        return m_RealEngine.Load(inApplicationPath);
    }
    bool Step() override { return m_RealEngine.Step(); }
    virtual TStateEventHandlerConnectionPtr
    RegisterEventHandler(EStateEvent inEvent, const char *inEventId, IStateEventHandler &inHandler) override
    {
        return m_RealEngine.RegisterEventHandler(inEvent, inEventId, inHandler);
    }
    void FireEvent(const char *inEventName, unsigned long long inDelay,
                           const char *inCancelId, bool inIsExternal = true) override
    {
        m_RealEngine.FireEvent(inEventName, inDelay, inCancelId, inIsExternal);
    }
    void FireEvent(const char *inEventName, bool inIsExternal = true) override
    {
        m_RealEngine.FireEvent(inEventName, inIsExternal);
    }
    void CancelEvent(const char *inCancelId) override { m_RealEngine.CancelEvent(inCancelId); }
};
}

namespace qt3ds {
namespace state {

    INDDStateEngine *INDDStateEngine::Create(
        qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> inFoundation,
        qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> inStringTable,
        qt3ds::foundation::NVScopedRefCounted<qt3ds::state::IInputStreamFactory> inInputStreamFactory,
        Q3DStudio::ITimeProvider &inTimeProvider)
    {
        return QT3DS_NEW(inFoundation->getAllocator(), CNDDStateEngine)(
            inFoundation, inStringTable, inInputStreamFactory, inTimeProvider);
    }

    INDDStateEngine *INDDStateEngine::Create() { return new CNDDStateEngineDefault(); }
}
}
