/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#include "Qt3DSStateDebugger.h"
#include "Qt3DSStateDebuggerValues.h"
#include "Qt3DSStateDebuggerProtocol.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/IOStreams.h"

using namespace qt3ds::state::debugger;
using namespace qt3ds::state;

namespace {

static MallocAllocator g_MallocAlloc;

struct SDebugger : public IDebugger
{
    typedef eastl::vector<NVScopedRefCounted<IStateMachineDebugInterface>> TDebugList;
    typedef eastl::hash_map<IStateMachineDebugInterface *, QT3DSI32> TMachineIdMap;
    typedef eastl::hash_map<QT3DSI32, eastl::pair<NVScopedRefCounted<IStateMachineDebugInterface>,
                                               NVScopedRefCounted<IStateMachineListener>>>
        TIdMachineMap;
    QT3DSI32 mRefCount;
    QT3DSI32 m_NextStateMachineId;
    TDebugList m_StateMachineList;
    TMachineIdMap m_StateMachines;
    TIdMachineMap m_IdToStateMachines;
    QT3DSU64 m_StartTime;
    NVScopedRefCounted<IDebugOutStream> m_OutStream;
    SMessageParser<SDebugger> m_Parser;
    SMessageSerializer m_MessageSerializer;
    TDebugStr m_MessageStr;

    SDebugger()
        : mRefCount(0)
        , m_NextStateMachineId(1)
        , m_StartTime(Time::getCurrentCounterValue())
    {
    }

    virtual ~SDebugger() { DisconnectFromServer(); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(g_MallocAlloc)

    void ConnectStateMachine(IStateMachineDebugInterface &inMachine)
    {
        TMachineIdMap::iterator theIter = m_StateMachines.find(&inMachine);
        if (theIter != m_StateMachines.end())
            return;
        QT3DSI32 theId(m_NextStateMachineId);
        ++m_NextStateMachineId;
        IStateMachineListener *theListener =
            &IStateMachineListener::Create(g_MallocAlloc, theId, *m_OutStream.mPtr, m_StartTime,
                                           *this, inMachine.GetScriptContext());
        inMachine.SetStateMachineListener(theListener);
        m_StateMachines.insert(eastl::make_pair(&inMachine, theId));
        m_IdToStateMachines.insert(
            eastl::make_pair(theId, eastl::make_pair(&inMachine, theListener)));
    }

    void OnServerConnected(IDebugOutStream &inStream) override
    {
        m_OutStream = inStream;
        inStream.SetListener(this);
        m_MessageSerializer.Serialize(0, SInitialization(), m_StartTime);
        m_OutStream->Write(m_MessageSerializer.ToRawMessage());
        for (QT3DSU32 idx = 0, end = m_StateMachineList.size(); idx < end; ++idx)
            ConnectStateMachine(*m_StateMachineList[idx]);
    }

    void Connect(IStateMachineDebugInterface &inMachine) override
    {
        TDebugList::iterator theFind =
            eastl::find(m_StateMachineList.begin(), m_StateMachineList.end(), &inMachine);
        if (theFind == m_StateMachineList.end()) {
            m_StateMachineList.push_back(&inMachine);
            if (m_OutStream)
                ConnectStateMachine(inMachine);
        }
    }

    void OnExternalBreak() override
    {
        if (m_OutStream) {
            for (QT3DSU32 idx = 0, end = m_StateMachineList.size(); idx < end; ++idx)
                m_StateMachineList[idx]->OnExternalBreak();
        }
    }

    void DisconnectStateMachine(IStateMachineDebugInterface &inMachine)
    {
        TMachineIdMap::iterator theIter = m_StateMachines.find(&inMachine);
        if (theIter == m_StateMachines.end())
            return;

        QT3DSI32 theId = theIter->second;
        m_MessageSerializer.Serialize(theId, SDisconnect(),
                                      Time::getCurrentCounterValue() - m_StartTime);
        m_OutStream->Write(m_MessageSerializer.ToRawMessage());
        inMachine.SetStateMachineListener(NULL);
        m_StateMachines.erase(&inMachine);
        m_IdToStateMachines.erase(theId);
    }
    void Disconnect(IStateMachineDebugInterface &inMachine) override
    {
        TDebugList::iterator theFind =
            eastl::find(m_StateMachineList.begin(), m_StateMachineList.end(), &inMachine);
        if (theFind != m_StateMachineList.end()) {
            DisconnectStateMachine(inMachine);
            m_StateMachineList.erase(theFind);
        }
    }

    void DisconnectFromServer() override
    {
        if (m_OutStream) {
            m_MessageSerializer.Serialize(-1, SDisconnect(),
                                          Time::getCurrentCounterValue() - m_StartTime);
            m_OutStream->Write(m_MessageSerializer.ToRawMessage());
        }
        m_StateMachines.clear();
        m_IdToStateMachines.clear();
        m_StateMachineList.clear();
        m_OutStream = NULL;
    }

    void OnMessageReceived(const SDebugStreamMessage &msg) override
    {
        if (msg.m_Data.size())
            m_Parser.Parse(msg.m_Data, *this);
    }

// Set of ignored messages
#define IGNORE_DEBUG_MESSAGE_TYPE(tname)                                                           \
    void OnMessage(QT3DSI32, QT3DSU64, const S##tname &) { QT3DS_ASSERT(false); }

    // this are outgoing messages; we shouldn't be receiving them.
    IGNORE_DEBUG_MESSAGE_TYPE(Initialization)
    IGNORE_DEBUG_MESSAGE_TYPE(Connect)
    IGNORE_DEBUG_MESSAGE_TYPE(BreakpointHit)
    IGNORE_DEBUG_MESSAGE_TYPE(DebugLog)
    IGNORE_DEBUG_MESSAGE_TYPE(EventQueued)
    IGNORE_DEBUG_MESSAGE_TYPE(BeginStep)
    IGNORE_DEBUG_MESSAGE_TYPE(BeginMicrostep)
    IGNORE_DEBUG_MESSAGE_TYPE(MicrostepEvent)
    IGNORE_DEBUG_MESSAGE_TYPE(MicrostepData)
    IGNORE_DEBUG_MESSAGE_TYPE(EndMicrostep)
    IGNORE_DEBUG_MESSAGE_TYPE(ModifyTableValues)

    IStateMachineDebugInterface *GetStateMachine(QT3DSI32 inStreamId)
    {
        TIdMachineMap::iterator theIter = m_IdToStateMachines.find(inStreamId);
        if (theIter == m_IdToStateMachines.end())
            return NULL;
        return theIter->second.first.mPtr;
    }

    IStateMachineListener *GetStateMachineListener(QT3DSI32 inStreamId)
    {
        TIdMachineMap::iterator theIter = m_IdToStateMachines.find(inStreamId);
        if (theIter == m_IdToStateMachines.end())
            return NULL;
        return theIter->second.second.mPtr;
    }

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SSetBreakpoint &inBp)
    {
        IStateMachineListener *iface = GetStateMachineListener(sid);
        if (iface)
            iface->SetBreakpoint(inBp.m_Breakpoint);
    }

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SClearBreakpoint &inBp)
    {
        IStateMachineListener *iface = GetStateMachineListener(sid);
        if (iface)
            iface->ClearBreakpoint(inBp.m_Breakpoint);
    }

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SClearAllBreakpoints &)
    {
        IStateMachineListener *iface = GetStateMachineListener(sid);
        if (iface)
            iface->ClearAllBreakpoints();
    }

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SBreakOnMicrostep &inCmd)
    {
        IStateMachineListener *iface = GetStateMachineListener(sid);
        if (iface)
            iface->SetBreakOnMicrostep(inCmd.m_Value);
    }

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SContinue &)
    {
        IStateMachineListener *iface = GetStateMachineListener(sid);
        if (iface)
            iface->Continue();
    }

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SDisconnect &)
    {
        if (sid != 0 && sid != (QT3DSI32)QT3DS_MAX_U32) {
            IStateMachineDebugInterface *iface = GetStateMachine(sid);
            if (iface)
                Disconnect(*iface);
        } else {
            if (sid == (QT3DSI32)QT3DS_MAX_U32)
                DisconnectFromServer();
        }
    }

    void error(const char8_t *, const char8_t *) {}
};

static inline NVConstDataRef<QT3DSU8> ToRef(const TDebugStr &str)
{
    return NVConstDataRef<QT3DSU8>((const QT3DSU8 *)str.begin(), str.size());
}

bool TestBasicParseRobustness()
{
    typedef STestMessageHandler<SInitialization> THandlerType;
    typedef SMessageParser<THandlerType> TParserType;
    // Just ensure things don't crash.
    SInitialization testType;
    THandlerType theHandler(2, 5, testType);
    TDebugStr theStr;
    TParserType theParser;
    theParser.Parse(ToRef(theStr), theHandler);

    theStr = "Initialization";
    theParser.Parse(ToRef(theStr), theHandler);

    theStr = "Initialization ";
    theParser.Parse(ToRef(theStr), theHandler);
    return true;
}

template <typename TDataType>
bool TestSerialization(const TDataType &testType, SMessageSerializer &ioSerializer,
                       QT3DSU64 inTimestamp)
{
    typedef STestMessageHandler<TDataType> THandlerType;
    typedef SMessageParser<THandlerType> TParserType;

    ioSerializer.Serialize(5, testType, inTimestamp);
    THandlerType theHandler(5, inTimestamp, testType);
    TParserType theParser;
    theParser.Parse(ioSerializer.ToRawMessage(), theHandler);
    return theHandler.m_Result;
}
}

IDebugger &IDebugger::CreateDebugger()
{
    return *QT3DS_NEW(g_MallocAlloc, SDebugger)();
}

bool CDebuggerTests::TestStreamProtocol()
{
    // for each message type, fill on some values, serialize to string and back and see what happens
    TDebugStrList theList;
    theList.push_back("one");
    theList.push_back("two");
    TTransitionIdList theTransList;
    theTransList.push_back(STransitionId("one", 2));
    theTransList.push_back(STransitionId("two", -1));
    SBreakpoint bp1(SStateEnterBreakpoint("one"));
    SBreakpoint bp2(SStateExitBreakpoint("two"));
    SBreakpoint bp3(STransitionId("three", 4));
    SMessageSerializer theSerializer;
    bool retval = TestBasicParseRobustness();
    // breaking out the large && statement to make testing easier.
    retval = retval && TestSerialization(SInitialization(), theSerializer, 5);
    retval = retval && TestSerialization(SConnect("abe.scxml", "<one>hey you</one>", theList),
                                         theSerializer, 6);
    retval = retval
        && TestSerialization(SDebugLog(TDebugStr("Hey joe, where you goin'")), theSerializer, 7);
    retval = retval && TestSerialization(SBeginMicrostep(), theSerializer, 8);
    retval = retval && TestSerialization(SMicrostepEvent("evt1", true), theSerializer, 9);
    retval = retval
        && TestSerialization(SMicrostepData(theTransList, theList, theList), theSerializer, 9);
    retval = retval && TestSerialization(SEndMicrostep(), theSerializer, 10);
    retval = retval && TestSerialization(SEventQueued("evt1", false), theSerializer, 11);
    retval = retval && TestSerialization(SEventQueued("evt1", false), theSerializer, 12);
    retval = retval && TestSerialization(SSetBreakpoint(bp1), theSerializer, 13);
    retval = retval && TestSerialization(SSetBreakpoint(bp2), theSerializer, 14);
    retval = retval && TestSerialization(SSetBreakpoint(bp3), theSerializer, 15);
    retval = retval && TestSerialization(SClearBreakpoint(bp1), theSerializer, 16);
    retval = retval && TestSerialization(SClearAllBreakpoints(), theSerializer, 17);
    return retval;
}
