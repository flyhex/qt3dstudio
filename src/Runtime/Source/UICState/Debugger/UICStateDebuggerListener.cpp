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
#include "UICStateDebugger.h"
#include "UICStateDebuggerValues.h"
#include "UICStateDebuggerProtocol.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSAtomic.h"
#include "UICStateScriptContext.h"

using namespace uic::state;
using namespace uic::state::debugger;

namespace {

// Component sits in the state machine and receives messages from the state machine
// itself.
struct SListener : public IStateMachineListener, public IScriptStateListener
{
    typedef eastl::hash_map<SDatamodelTable *, SModifyTableValues> TTableModificationMap;
    NVAllocatorCallback &m_Allocator;
    QT3DSI32 mRefCount;
    QT3DSU32 m_StreamId;
    NVScopedRefCounted<IDebugOutStream> m_OutStream;
    SMessageSerializer m_Serializer;
    TDebugStrList m_DebugStrList;
    TDebugStrList m_DebugStrAltList;
    TTransitionIdList m_TransitionIdList;
    QT3DSU64 m_StartTime;
    bool m_BreakOnMicrostep;
    bool m_Blocking;
    IDebugger &m_Debugger;
    TDebugStr m_BreakStr;
    NVScopedRefCounted<IScriptContext> m_ScriptContext;
    TTableModificationMap m_TableModifications;
    TBreakpointList m_Breakpoints;
    SMicrostepData m_MicrostepData;
    bool m_MicrostepBeginSent;
    bool m_StepSent;

    SListener(NVAllocatorCallback &alloc, QT3DSU32 inStreamId, IDebugOutStream &inOutStr,
              QT3DSU64 inStartTime, IDebugger &inDebugger, IScriptContext &inScriptContext)
        : m_Allocator(alloc)
        , mRefCount(0)
        , m_StreamId(inStreamId)
        , m_OutStream(inOutStr)
        , m_StartTime(inStartTime)
        , m_BreakOnMicrostep(false)
        , m_Blocking(false)
        , m_Debugger(inDebugger)
        , m_ScriptContext(inScriptContext)
        , m_MicrostepBeginSent(false)
        , m_StepSent(false)
    {
    }

    virtual ~SListener() { m_Blocking = false; }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)

    template <typename TDataType>
    static const eastl::vector<TDataType> &RefToList(NVConstDataRef<TDataType> inRef,
                                                     eastl::vector<TDataType> &outList)
    {
        outList.assign(inRef.begin(), inRef.end());
        return outList;
    }

    template <typename TDataType>
    void Serialize(const TDataType &dtype, QT3DSU64 inTime)
    {
        m_Serializer.Serialize(m_StreamId, dtype, inTime);
        m_OutStream->Write(m_Serializer.ToRawMessage());
    }

    template <typename TDataType>
    void Serialize(const TDataType &dtype)
    {
        Serialize(dtype, CurrentTime());
    }

    void OnConnect(const TDebugStr &SCXMLFilename, const TDebugStr &SCXMLFileData,
                           NVConstDataRef<TDebugStr> inConfiguration) override
    {
        Serialize(
            SConnect(SCXMLFilename, SCXMLFileData, RefToList(inConfiguration, m_DebugStrList)));
        DumpScriptState();
    }

    QT3DSU64 CurrentTime() { return Time::getCurrentCounterValue() - m_StartTime; }

    void DumpScriptState()
    {
        // Run through the global state and output any keys that have changed.
        m_TableModifications.clear();
        m_ScriptContext->DumpState(*this);
        for (TTableModificationMap::iterator iter = m_TableModifications.begin(),
                                             end = m_TableModifications.end();
             iter != end; ++iter) {
            Serialize(iter->second);
        }
        m_TableModifications.clear();
    }
    virtual void OnBreakpointHit(const SBreakpoint &inBreakpointId)
    {
        DumpScriptState();
        Serialize(SBreakpointHit(inBreakpointId));
        Break();
    }

    void Log(const TDebugStr &inStr) override { Serialize(SDebugLog(inStr)); }

    void SetBreakpoint(const SBreakpoint &inBreakpoint) override
    {
        m_Breakpoints.push_back(inBreakpoint);
    }

    void ClearBreakpoint(const SBreakpoint &inBreakpoint) override
    {
        TBreakpointList::iterator removeIter =
            eastl::remove(m_Breakpoints.begin(), m_Breakpoints.end(), inBreakpoint);
        if (removeIter != m_Breakpoints.end())
            m_Breakpoints.erase(removeIter, m_Breakpoints.end());
    }
    void ClearAllBreakpoints() override { m_Breakpoints.clear(); }
    void EventQueued(const TDebugStr &inEventName, bool inInternal) override
    {
        Serialize(SEventQueued(inEventName, inInternal));
    }
    void SendMicrostepBegin()
    {
        if (m_MicrostepBeginSent == false) {
            if (m_StepSent == false) {
                m_StepSent = true;
                Serialize(SBeginStep());
            }

            m_MicrostepBeginSent = true;
            Serialize(SBeginMicrostep());
        }
    }

    void BeginStep() override { m_StepSent = false; }

    void BeginMicroStep() override
    {
        m_MicrostepBeginSent = false;
        m_MicrostepData.m_Transitions.clear();
        m_MicrostepData.m_EnterStates.clear();
        m_MicrostepData.m_ExitStates.clear();
    }

    void SetEvent(const TDebugStr &inEvent, bool inInternal) override
    {
        if (!inEvent.empty()) {
            SendMicrostepBegin();
            Serialize(SMicrostepEvent(inEvent, inInternal));
        }
    }
    void SetTransitionSet(NVConstDataRef<STransitionId> inTransitions) override
    {
        SendMicrostepBegin();
        RefToList(inTransitions, m_MicrostepData.m_Transitions);
    }
    void SetExitSet(NVConstDataRef<TDebugStr> inSet) override
    {
        SendMicrostepBegin();
        RefToList(inSet, m_MicrostepData.m_ExitStates);
    }
    void SetEnterSet(NVConstDataRef<TDebugStr> inSet) override
    {
        SendMicrostepBegin();
        RefToList(inSet, m_MicrostepData.m_EnterStates);
    }

    static inline bool FindInList(const TDebugStr &inStr, const TDebugStrList &inStrList)
    {
        if (eastl::find(inStrList.begin(), inStrList.end(), inStr) != inStrList.end())
            return true;
        return false;
    }

    static inline bool FindInList(const STransitionId &inStr, const TTransitionIdList &inStrList)
    {
        if (eastl::find(inStrList.begin(), inStrList.end(), inStr) != inStrList.end())
            return true;
        return false;
    }

    const SBreakpoint *FindHitBreakpoint()
    {
        // Now check for breakpoints.
        for (QT3DSU32 breakIdx = 0, breakEnd = m_Breakpoints.size(); breakIdx < breakEnd; ++breakIdx) {
            const SBreakpoint &theBreakpoint = m_Breakpoints[breakIdx];
            switch (theBreakpoint.getType()) {
            case BreakpointTypes::StateEnter:
                if (FindInList(theBreakpoint.getData<SStateEnterBreakpoint>().m_ObjectId,
                               m_MicrostepData.m_EnterStates))
                    return &theBreakpoint;
            case BreakpointTypes::StateExit:
                if (FindInList(theBreakpoint.getData<SStateExitBreakpoint>().m_ObjectId,
                               m_MicrostepData.m_ExitStates))
                    return &theBreakpoint;
            case BreakpointTypes::Transition:
                if (FindInList(theBreakpoint.getData<STransitionId>(),
                               m_MicrostepData.m_Transitions))
                    return &theBreakpoint;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }

        return NULL;
    }

    // Log statements run through the debugger as well.
    void EndMicroStep() override
    {
        DumpScriptState();
        if (m_MicrostepBeginSent) {
            if (m_MicrostepData.m_Transitions.empty() == false)
                Serialize(m_MicrostepData);

            Serialize(SEndMicrostep());
            const SBreakpoint *theHitBreakpoint = FindHitBreakpoint();
            if (theHitBreakpoint)
                OnBreakpointHit(*theHitBreakpoint);
            else if (m_BreakOnMicrostep)
                Break();
        }
    }

    void EndStep() override { m_StepSent = false; }

    void SetBreakOnMicrostep(bool inEnableStep) override { m_BreakOnMicrostep = inEnableStep; }

    virtual void Break()
    {
        if (!m_Blocking) {
            // We may get disconnected *while* we are breaking.
            // We need to ensure we don't get nuked while we are breaking.
            NVScopedRefCounted<SListener> tempVar(this);
            m_Blocking = true;
            while (m_Blocking) {
                SDebugStreamMessage msg = m_OutStream->WaitForNextMessage();
                // Some out streams will have a reference to the debugger and some will not.
                // If they do, then we do not have to hand our string to the debugger, we can assume
                // the underlying implementation will have done that.  If not, then we need to
                // pass the message to the debugger.
                if (m_Blocking && msg.m_Data.size())
                    m_Debugger.OnMessageReceived(msg);

                if (!m_OutStream->Connected()) {
                    m_Blocking = false;
                    m_Debugger.DisconnectFromServer();
                }
            }
        }
    }

    void Continue() override { m_Blocking = false; }

    void OnExternalBreak() override { DumpScriptState(); }

    // IScriptStateListener
    void SetKey(void *inTable, const TDebugStr &inStr, const SDatamodelValue &inValue) override
    {
        SDatamodelTable *theTable((SDatamodelTable *)inTable);
        SModifyTableValues &theModification =
            m_TableModifications.insert(eastl::make_pair(theTable, SModifyTableValues(theTable)))
                .first->second;
        theModification.m_Modifications.push_back(
            STableModification(STableEntry(inStr, inValue), TableModificationType::SetKey));
    }

    void RemoveKey(void *inTable, const TDebugStr &inStr) override
    {
        SDatamodelTable *theTable((SDatamodelTable *)inTable);
        SModifyTableValues &theModification =
            m_TableModifications.insert(eastl::make_pair(theTable, SModifyTableValues(theTable)))
                .first->second;
        theModification.m_Modifications.push_back(
            STableModification(STableEntry(inStr), TableModificationType::RemoveKey));
    }
};
}

IStateMachineListener &IStateMachineListener::Create(NVAllocatorCallback &inAlloc, QT3DSU32 inStreamId,
                                                     IDebugOutStream &inOutStr, QT3DSU64 inStartTime,
                                                     IDebugger &inDebugger,
                                                     IScriptContext &inScriptContext)
{
    return *QT3DS_NEW(inAlloc, SListener)(inAlloc, inStreamId, inOutStr, inStartTime, inDebugger,
                                       inScriptContext);
}
