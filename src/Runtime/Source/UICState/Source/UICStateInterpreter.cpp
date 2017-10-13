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
#include "UICStateTypes.h"
#include "UICStateInterpreter.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Utils.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSTime.h"
#include "EASTL/string.h"
#include "EASTL/set.h"
#include "EASTL/sort.h"
#include "EASTL/list.h"
#include "UICStateExecutionContext.h"
#include "UICStateExecutionTypes.h"
#include "UICStateScriptContext.h"
#include "UICStateSharedImpl.h"
#include "foundation/XML.h"
#include "UICStateIdValue.h"
#include "UICStateDebugger.h"
#include "UICStateXMLIO.h"
#include "UICStateDebuggerValues.h"
#include "UICStateContext.h"
#include "foundation/FastAllocator.h"

using namespace qt3ds::state;
using namespace qt3ds::state::debugger;

#ifdef _DEBUG
#define UIC_LOG_ENTER_EXIT 1
#define UIC_LOG_ACTIVE_EVENT 1
#endif

namespace qt3ds {
namespace state {
    struct SInterpreterData
    {
    };
}
}

namespace {
struct SSimpleEvent : public IEvent
{
    NVAllocatorCallback &m_Alloc;
    volatile QT3DSI32 mRefCount;
    CRegisteredString m_Name;

    SSimpleEvent(NVAllocatorCallback &alloc, CRegisteredString nm)
        : m_Alloc(alloc)
        , mRefCount(0)
        , m_Name(nm)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Alloc)

    CRegisteredString GetName() const override { return m_Name; }
};

struct SStateNodeInterpreterData : public SInterpreterData
{

    QT3DSI32 m_DocumentOrder;
    bool m_Entered;
    SStateNode *m_TransitionSubgraphRoot;

    SStateNodeInterpreterData(QT3DSI32 inDocOrder)
        : m_DocumentOrder(inDocOrder)
        , m_Entered(false)
        , m_TransitionSubgraphRoot(NULL)
    {
    }
};

template <typename TDataType, typename THasher = eastl::hash<TDataType>,
          typename TEqOp = eastl::equal_to<TDataType>>
struct SOrderedSet
{
    typedef eastl::hash_set<TDataType, THasher, TEqOp, ForwardingAllocator> TSetType;
    typedef nvvector<TDataType> TListType;

    TSetType m_Set;
    TListType m_List;
    typedef typename TListType::iterator iterator;

    SOrderedSet(NVAllocatorCallback &alloc, const char *inTypeName)
        : m_Set(ForwardingAllocator(alloc, inTypeName))
        , m_List(alloc, inTypeName)
    {
    }

    bool insert(const TDataType &inDtype)
    {
        if (m_Set.insert(inDtype).second) {
            m_List.push_back(inDtype);
            return true;
        }
        return false;
    }

    void clear()
    {
        m_Set.clear();
        m_List.clear();
    }

    iterator begin() { return m_List.begin(); }
    iterator end() { return m_List.end(); }
    QT3DSU32 size() const { return m_List.size(); }
    bool contains(const TDataType &inType) const { return m_Set.find(inType) != m_Set.end(); }
    TDataType operator[](int inIndex)
    {
        if (inIndex < (int)size())
            return m_List[inIndex];
        return TDataType();
    }
    void erase(const TDataType &inType)
    {
        typename TSetType::iterator iter = m_Set.find(inType);
        if (iter != m_Set.end()) {
            m_Set.erase(iter);
            typename TListType::iterator iter = eastl::find(m_List.begin(), m_List.end(), inType);
            if (iter != m_List.end())
                m_List.erase(iter);
            else {
                QT3DS_ASSERT(false);
            }
        }
    }
};

struct SStateNodeNode
{
    SStateNode *m_Node;
    SStateNodeNode *m_NextSibling;
    SStateNodeNode()
        : m_Node(NULL)
        , m_NextSibling(NULL)
    {
    }
    SStateNodeNode(SStateNode *inNode)
        : m_Node(inNode)
        , m_NextSibling(NULL)
    {
    }
};

DEFINE_INVASIVE_SINGLE_LIST(StateNodeNode);
IMPLEMENT_INVASIVE_SINGLE_LIST(StateNodeNode, m_NextSibling);

struct SFutureEvent
{
    TEventPtr m_Event;
    QT3DSU64 m_FireTime;
    CRegisteredString m_CancelId;
    bool m_IsExternal;
    SFutureEvent(TEventPtr evt, QT3DSU64 ft, CRegisteredString id, bool inExternal)
        : m_Event(evt)
        , m_FireTime(ft)
        , m_CancelId(id)
        , m_IsExternal(inExternal)
    {
    }
    SFutureEvent()
        : m_FireTime((QT3DSU64)-1)
        , m_IsExternal(false)
    {
    }
    // We want the events sorted in *reverse* time order.
    bool operator<(const SFutureEvent &inOther) const { return m_FireTime < inOther.m_FireTime; }
};

struct StateSystem;

typedef SOrderedSet<SStateNode *> TStateNodeSet;
typedef SOrderedSet<STransition *> TTransitionSet;
typedef nvvector<STransition *> TTransitionList;

struct SDebugInterface : public IStateMachineDebugInterface, public IStateLogger
{
    StateSystem &m_StateSystem;
    // No listener means no events and no active breakpoints of course
    NVScopedRefCounted<IStateMachineListener> m_Listener;
    TDebugStr m_LogStr;
    TDebugStr m_TempStr;
    TDebugStrList m_EnterList;
    TDebugStrList m_ExitList;
    TTransitionIdList m_TransitionList;

    SDebugInterface(StateSystem &ss)
        : m_StateSystem(ss)
        , m_Listener(NULL)
    {
    }
    // ignored as this object is part of the state system.
    void addRef() override;
    void release() override;
    bool valid() { return m_Listener.mPtr != NULL; }

    void SetStateMachineListener(IStateMachineListener *inListener) override
    {
        if (m_Listener.mPtr != inListener) {
            m_Listener = inListener;
            if (m_Listener)
                OnConnect();
        }
    }

    // IStateLogger interface
    void Log(const char8_t *inLabel, const char8_t *inExpression) override
    {
        if (m_Listener) {
            m_LogStr.assign(nonNull(inLabel));
            m_LogStr.append(" - ");
            m_LogStr.append(nonNull(inExpression));
            m_Listener->Log(m_LogStr);
        }
    }

    IScriptContext &GetScriptContext() override;

    // Functions implemented after StateSystem in order to take advantage of the StateSystem struct
    // members directly.
    void OnConnect();

    void EventQueued(const char8_t *inEventName, bool inInternal)
    {
        m_Listener->EventQueued(inEventName, inInternal);
    }
    // Functions should not be called in there is not a valid listener
    void BeginStep();
    void BeginMicrostep();
    void SetCurrentEvent(const char8_t *inEventName, bool inInternal);
    void SetTransitionSet(const TTransitionList &inTransitions);
    void SetExitSet(const TStateNodeSet &inSet);
    void SetEnterSet(const TStateNodeSet &inSet);
    // Log statements run through the debugger as well.
    void EndMicrostep();
    void EndStep();
    void OnExternalBreak() override;
};

#define DEBUGGER_CALL(function)                                                                    \
    if (m_DebugInterface.valid())                                                                  \
        m_DebugInterface.function();
#define DEBUGGER_CALL1(function, arg)                                                              \
    if (m_DebugInterface.valid())                                                                  \
        m_DebugInterface.function(arg);
#define DEBUGGER_CALL2(function, arg, arg2)                                                        \
    if (m_DebugInterface.valid())                                                                  \
        m_DebugInterface.function(arg, arg2);

#ifdef _WIN32
#pragma warning(disable : 4355)
#endif
struct SStateSignalSender : public IStateSignalConnection
{
    NVAllocatorCallback &m_Alloc;
    StateSystem *m_System;
    IStateInterpreterEventHandler &m_Handler;
    volatile QT3DSI32 mRefCount;
    SStateSignalSender(NVAllocatorCallback &inAlloc, StateSystem &inSystem,
                       IStateInterpreterEventHandler &inHandler)
        : m_Alloc(inAlloc)
        , m_System(&inSystem)
        , m_Handler(inHandler)
        , mRefCount(0)
    {
    }
    virtual ~SStateSignalSender();
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Alloc)
};

// Note that we explicity don't addref the signal sender internally.  This is by design.
typedef nvvector<SStateSignalSender *> TSignalSenderList;

struct StateSystem : public IStateInterpreter
{
    typedef eastl::basic_string<char8_t, ForwardingAllocator> TStrType;
    typedef Pool<SStateNodeInterpreterData, ForwardingAllocator> TInterperterDataPool;
    typedef eastl::list<TEventPtr, ForwardingAllocator> TEventQueue;
    typedef nvhash_map<SHistory *, TStateNodeNodeList> THistoryMap;
    typedef Pool<SStateNodeNode, ForwardingAllocator> TStateNodeNodePool;
    typedef nvvector<SFutureEvent> TFutureEventList;
    typedef nvhash_set<STransition *> TTransitionHashSet;
    typedef Pool<STransition, ForwardingAllocator> TTransitionPool;

    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IStateContext> m_Context;
    volatile QT3DSI32 mRefCount;
    // Holds the leaves of the current state.
    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IScriptContext> m_ScriptContext;
    NVScopedRefCounted<IExecutionContext> m_ExecutionContext;
    nvvector<SStateNode *> m_ParentComparisonLHS;
    nvvector<SStateNode *> m_ParentComparisonRHS;
    nvvector<SStateNode *> m_AncestorsList;
    nvvector<SStateNode *> m_IDRefList;

    THistoryMap m_HistoryMap;

    SOrderedSet<SStateNode *> m_Configuration;
    nvvector<SStateNode *> m_StatesToInvoke;
    nvvector<SStateNode *> m_TransitionTargetList;
    TTransitionList m_EnabledTransitions;

    TEventQueue m_InternalQueue;
    TEventQueue m_ExternalQueue;

    SOrderedSet<STransition *> m_TransitionSet;

    TStateNodeSet m_StatesToEnter;
    TStateNodeSet m_StatesForDefaultEntry;
    TStateNodeSet m_StatesToExit;

    TInterperterDataPool m_InterpreterDataPool;
    TStateNodeNodePool m_StateNodeNodePool;

    TStrType m_Workspace;

    TFutureEventList m_FutureEvents;

    bool m_Initialized;
    bool m_Running;

    SDebugInterface m_DebugInterface;
    TSignalSenderList m_SignalSenders;

    TTransitionHashSet m_InvalidTransitions;

    TTransitionPool m_TransitionPool;
    nvvector<STransition *> m_TemporaryTransitions;
    nvhash_map<SStateNode *, STransition *> m_StateInitialTransitionMap;
    SFastAllocator<> m_TemporaryAllocator;
    TStrType m_IdSplitter;
    nvvector<SStateNode *> m_TempNodeList;

    StateSystem(NVFoundationBase &inFnd, IStringTable &inStrTable, IScriptContext &inScriptContext,
                IExecutionContext &inExecutionContext)
        : m_Foundation(inFnd)
        , mRefCount(0)
        , m_StringTable(inStrTable)
        , m_ScriptContext(inScriptContext)
        , m_ExecutionContext(inExecutionContext)
        , m_ParentComparisonLHS(inFnd.getAllocator(), "StateSystem::m_ParentComparisonLHS")
        , m_ParentComparisonRHS(inFnd.getAllocator(), "StateSystem::m_ParentComparisonRHS")
        , m_AncestorsList(inFnd.getAllocator(), "StateSystem::m_AncestorsList")
        , m_IDRefList(inFnd.getAllocator(), "StateSystem::m_IDRefList")
        , m_HistoryMap(inFnd.getAllocator(), "StateSystem::m_HistoryMap")
        , m_Configuration(inFnd.getAllocator(), "StateSystem::m_Configuration")
        , m_StatesToInvoke(inFnd.getAllocator(), "StateSystem::m_InvokeList")
        , m_TransitionTargetList(inFnd.getAllocator(), "StateSystem::m_TransitionTargetList")
        , m_EnabledTransitions(inFnd.getAllocator(), "StateSystem::m_TransitionList")
        , m_InternalQueue(ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_InternalQueue"))
        , m_ExternalQueue(ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_ExternalQueue"))
        , m_TransitionSet(inFnd.getAllocator(), "StateSystem::m_TransitionSet")
        , m_StatesToEnter(inFnd.getAllocator(), "StateSystem::m_StatesToEnter")
        , m_StatesForDefaultEntry(inFnd.getAllocator(), "StateSystem::m_StatesForDefaultEntry")
        , m_StatesToExit(inFnd.getAllocator(), "StateSystem::m_StatesToExit")
        , m_InterpreterDataPool(
              ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_InterpretDataPool"))
        , m_StateNodeNodePool(
              ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_StateNodeNodePool"))
        , m_Workspace(ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_Workspace"))
        , m_FutureEvents(inFnd.getAllocator(), "StateSystem::m_FutureEvents")
        , m_Initialized(false)
        , m_Running(false)
        , m_DebugInterface(*this)
        , m_SignalSenders(inFnd.getAllocator(), "StateSystem::m_SignalSenders")
        , m_InvalidTransitions(inFnd.getAllocator(), "StateSystem::m_InvalidTransitions")
        , m_TransitionPool(
              ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_TransitionPool"))
        , m_TemporaryTransitions(inFnd.getAllocator(), "StateSystem::m_TemporaryTransitions")
        , m_StateInitialTransitionMap(inFnd.getAllocator(),
                                      "StateSystem::m_StateInitialTempTransitionMap")
        , m_TemporaryAllocator(inFnd.getAllocator(), "StateSystem::m_TemporaryAllocator")
        , m_IdSplitter(ForwardingAllocator(inFnd.getAllocator(), "StateSystem::m_IdSplitter"))
        , m_TempNodeList(inFnd.getAllocator(), "StateSystem::m_TempNodeList")
    {
        inExecutionContext.SetInterpreter(*this);
        inExecutionContext.SetMachineDebugLogger(m_DebugInterface);
        inScriptContext.SetInterpreter(*this);
    }

    ~StateSystem()
    {
        // Ensure the signallers will not attempt to communicate to this object.
        for (TSignalSenderList::iterator iter = m_SignalSenders.begin(),
                                         end = m_SignalSenders.end();
             iter != end; ++iter)
            (*iter)->m_System = NULL;
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void InitializeDataStructures()
    {
        m_Configuration.clear();
        m_StatesToInvoke.clear();
        m_EnabledTransitions.clear();
        m_Running = true;
    }

    STransition &CreateTemporaryTransition()
    {
        STransition *newTrans = m_TransitionPool.construct(__FILE__, __LINE__);
        m_TemporaryTransitions.push_back(newTrans);
        return *newTrans;
    }

    void ReleaseInitialAndTemporaryTransitions()
    {
        for (QT3DSU32 idx = 0, end = m_TemporaryTransitions.size(); idx < end; ++idx)
            m_TransitionPool.deallocate(m_TemporaryTransitions[idx]);
        m_TemporaryTransitions.clear();
        m_StateInitialTransitionMap.clear();
        m_TemporaryAllocator.reset();
    }

    void FreeHistoryData(SHistory &inHistory)
    {
        THistoryMap::iterator historyData = m_HistoryMap.find(&inHistory);
        if (historyData != m_HistoryMap.end()) {
            for (TStateNodeNodeList::iterator iter = historyData->second.begin(),
                                              end = historyData->second.end();
                 iter != end; ++iter)
                m_StateNodeNodePool.deallocate(&(*iter));
            // Force clear the list.
            historyData->second.m_Head = NULL;
        }
    }

    TStateNodeNodeList *GetHistoryData(SHistory &inHistory)
    {
        THistoryMap::iterator historyData = m_HistoryMap.find(&inHistory);
        if (historyData != m_HistoryMap.end())
            return &historyData->second;
        return NULL;
    }

    TStateNodeNodeList &GetOrCreateHistoryData(SHistory &inHistory)
    {
        THistoryMap::iterator historyData = m_HistoryMap.find(&inHistory);
        if (historyData != m_HistoryMap.end())
            return historyData->second;
        return m_HistoryMap.insert(eastl::make_pair(&inHistory, TStateNodeNodeList()))
            .first->second;
    }

    // Setup of the state system, you can add a set of roots to the state graph.
    NVConstDataRef<SStateNode *> GetConfiguration() override { return m_Configuration.m_List; }

    void GetPathToRoot(SStateNode &state, nvvector<SStateNode *> &inParents, bool inProper)
    {
        inParents.clear();
        SStateNode *stateIter = &state;
        if (inProper)
            stateIter = state.m_Parent;
        for (; stateIter; stateIter = stateIter->m_Parent)
            inParents.push_back(stateIter);
    }

    // Given these two states, get the nearest parent they share in common.
    // Return valid is the index taken from the end of the m_ParentComparison* lists.
    // the index is valid in both of them.
    QT3DSI32 GetSharedParentState(SStateNode &lhs, SStateNode &rhs, bool inProper)
    {
        GetPathToRoot(lhs, m_ParentComparisonLHS, inProper);
        GetPathToRoot(rhs, m_ParentComparisonRHS, inProper);
        if (m_ParentComparisonLHS.empty() || m_ParentComparisonRHS.empty()) {
            QT3DS_ASSERT(false);
            return -1;
        }
        QT3DS_ASSERT(m_ParentComparisonLHS.back() == m_ParentComparisonRHS.back());
        QT3DSI32 retval = 0;
        for (nvvector<SStateNode *>::reverse_iterator lhsComp = m_ParentComparisonLHS.rbegin(),
                                                      rhsComp = m_ParentComparisonRHS.rbegin(),
                                                      lhsEnd = m_ParentComparisonLHS.rend(),
                                                      rhsEnd = m_ParentComparisonRHS.rend();
             lhsComp != lhsEnd && rhsComp != rhsEnd && *lhsComp == *rhsComp;
             ++lhsComp, ++rhsComp, ++retval) {
        }
        // Walk the path to the root backwards and note where it differs.
        return retval - 1;
    }

    void CreateStateNodeInterpreterData(SStateNode &inNode, QT3DSI32 &inIdx)
    {
        inNode.m_InterpreterData = m_InterpreterDataPool.construct(inIdx, __FILE__, __LINE__);
        ++inIdx;
        if (inNode.m_Type == StateNodeTypes::SCXML) {
            SSCXML *item = inNode.CastTo<SSCXML>();
            if (item->m_Initial)
                CreateStateNodeInterpreterData(*item->m_Initial, inIdx);
        } else if (inNode.m_Type == StateNodeTypes::State) {
            SState *item = inNode.CastTo<SState>();
            if (item->m_Initial)
                CreateStateNodeInterpreterData(*item->m_Initial, inIdx);
        }
        if (StateNodeTypes::CanHaveChildren(inNode.m_Type)) {
            SStateParallelBase &theBase = static_cast<SStateParallelBase &>(inNode);
            for (TStateNodeList::iterator iter = theBase.m_Children.begin(),
                                          end = theBase.m_Children.end();
                 iter != end; ++iter)
                CreateStateNodeInterpreterData(*iter, inIdx);
        }
    }

    // Creates the interpreter data for the entire graph.
    void CreateStateNodeInterpreterData()
    {
        if (m_Context->GetRoot() == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        QT3DSI32 nodeIdx = 1;
        CreateStateNodeInterpreterData(*m_Context->GetRoot(), nodeIdx);
    }

    SStateNodeInterpreterData &GetOrCreateInterpreterData(SStateNode &inNode)
    {
        if (!inNode.m_InterpreterData)
            CreateStateNodeInterpreterData();

        return *static_cast<SStateNodeInterpreterData *>(inNode.m_InterpreterData);
    }
    struct SStateNodeSorter
    {
        StateSystem &m_System;
        SStateNodeSorter(StateSystem &inS)
            : m_System(inS)
        {
        }
        bool operator()(SStateNode *lhs, SStateNode *rhs) const
        {
            return m_System.GetOrCreateInterpreterData(*lhs).m_DocumentOrder
                < m_System.GetOrCreateInterpreterData(*rhs).m_DocumentOrder;
        }
    };

    void SortByDocumentOrder(nvvector<SStateNode *> &inList)
    {
        eastl::sort(inList.begin(), inList.end(), SStateNodeSorter(*this));
    }

    void SortConfiguration() { SortByDocumentOrder(m_Configuration.m_List); }

    void RunDataModel(SDataModel &inDM)
    {
        QT3DS_ASSERT(inDM.m_Source == NULL);
        QT3DS_ASSERT(inDM.m_Expression == NULL);
        for (TDataList::iterator iter = inDM.m_Data.begin(), end = inDM.m_Data.end(); iter != end;
             ++iter) {
            if (iter->m_Source == NULL)
                m_ScriptContext->Assign(iter->m_Id.c_str(), iter->m_Expression);
            else {
                QT3DS_ASSERT(false);
            }
        }
    }

    void RecursiveInitializeDataModel(SStateNode &inState)
    {
        SDataModel *theDataModel = inState.GetDataModel();
        if (theDataModel)
            RunDataModel(*theDataModel);

        TStateNodeList *children = inState.GetChildren();
        if (children) {
            for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                 iter != end; ++iter)
                RecursiveInitializeDataModel(*iter);
        }
    }

    // Called during initialization for early binding of the datamodel
    void RecursiveInitializeDataModel()
    {
        QT3DS_ASSERT(m_Context->GetRoot());
        QT3DS_ASSERT(m_Context->GetRoot()->m_Flags.IsLateBinding() == false);
        RecursiveInitializeDataModel(*m_Context->GetRoot());
    }

    void MarkTransitionAsInvalid(STransition &inTransition, SStateNode &inParent, int inIndex)
    {
        if (inIndex >= 0) {
            qCCritical(INVALID_OPERATION, "Detected invalid transition %s:%d",
                inParent.m_Id.c_str(), inIndex);
        } else {
            const char *transitionType = "initial";
            if (inIndex == -1)
                transitionType = "history";
            qCCritical(INVALID_OPERATION, "Detected invalid %s transition on node %s:%d",
                transitionType, inParent.m_Id.c_str(), inIndex);
        }
        m_InvalidTransitions.insert(&inTransition);
    }

    bool IsTransitionValid(STransition &inTransition)
    {
        return m_InvalidTransitions.find(&inTransition) == m_InvalidTransitions.end();
    }

    bool IsTransitionValid(STransition *inTransition)
    {
        if (inTransition)
            return IsTransitionValid(*inTransition);

        // NULL transitions are of course, invalid.
        return false;
    }

    bool ValidateTransitionTargetList(STransition &inTransition, SStateNode &inParent, int inIndex)
    {
        for (QT3DSU32 idx = 0, end = inTransition.m_Target.size(); idx < end; ++idx) {
            for (QT3DSU32 outerIdx = idx + 1; outerIdx < end; ++outerIdx) {
                // We handle this case dynamically during transition activation
                if (IsDescendant(*inTransition.m_Target[idx], *inTransition.m_Target[outerIdx])
                    || IsDescendant(*inTransition.m_Target[outerIdx], *inTransition.m_Target[idx]))
                    continue;

                // If they aren't directly related they must be related indirectly via a parallel.
                SStateNode *theLCA =
                    FindLCA(*inTransition.m_Target[idx], *inTransition.m_Target[outerIdx]);
                if (theLCA && theLCA->m_Type != StateNodeTypes::Parallel) {
                    MarkTransitionAsInvalid(inTransition, inParent, inIndex);
                    return false;
                }
            }
        }
        return true;
    }

    void ValidateInitialOrHistoryTransition(STransition *inTransition, SStateNode &inParent,
                                            bool inIsInitial = false)
    {
        if (inTransition == NULL)
            return;
        int transIndex = inIsInitial ? -2 : -1;
        if (inTransition->m_Target.size() == 0) {
            MarkTransitionAsInvalid(*inTransition, inParent, transIndex);
        } else {
            ValidateTransitionTargetList(*inTransition, inParent, transIndex);
        }
    }

    void ValidateGeneralTransition(STransition &inTransition, SStateNode &inParent, int inIndex)
    {
        // Three distinct ways a transition could be invalid.
        // 1.  targetless, eventless and conditionless
        if (inTransition.m_Target.size() == 0 && inTransition.m_Event.IsValid() == false
            && isTrivial(inTransition.m_Condition))
            MarkTransitionAsInvalid(inTransition, inParent, inIndex);
        else {
            // Else eventless ,targetless, and points back to source state
            if (inTransition.m_Target.size() == 1) {
                if (inTransition.m_Target[0] == inTransition.m_Parent
                    && inTransition.m_Event.IsValid() == false
                    && isTrivial(inTransition.m_Condition)) {
                    MarkTransitionAsInvalid(inTransition, inParent, inIndex);
                }
            } else {
                ValidateTransitionTargetList(inTransition, inParent, inIndex);
            }
        }
    }

    void RecursiveValidateTransitions(SStateNode &inNode, SStateNode *inParent, int inIndex)
    {
        switch (inNode.m_Type) {
        case StateNodeTypes::SCXML:
            ValidateInitialOrHistoryTransition(inNode.GetInitialTransition(), inNode);
            break;
        case StateNodeTypes::State:
            ValidateInitialOrHistoryTransition(inNode.GetInitialTransition(), inNode);
            break;
        case StateNodeTypes::Parallel:
            ValidateInitialOrHistoryTransition(inNode.GetInitialTransition(), inNode);
            break;
        case StateNodeTypes::History:
            ValidateInitialOrHistoryTransition(static_cast<SHistory &>(inNode).m_Transition, inNode,
                                               false);
            break;
        case StateNodeTypes::Transition:
            if (inParent)
                ValidateGeneralTransition(static_cast<STransition &>(inNode), *inParent, inIndex);
            break;
        default: // don't care
            break;
        }
        RecursiveValidateTransitions(inNode, inNode.GetChildren());
    }

    void RecursiveValidateTransitions(SStateNode &inParent, TStateNodeList *inNodes)
    {
        if (inNodes) {
            int idx = 0;
            for (TStateNodeList::iterator iter = inNodes->begin(), end = inNodes->end();
                 iter != end; ++iter, ++idx)
                RecursiveValidateTransitions(*iter, &inParent, idx);
        }
    }

    // The state machine is pretty robust but there are is one category of input that can
    // put the machine in a bad state.  Transitions with either multiple targets or no target
    // or a single target that points back to its owner may be bad depending on different criteria.
    // For multiple targets, we invalidate a transition that puts us into a bad state.
    // For no targets, in invalidate a transition that would cause the interpreter to loop
    // infinitely
    // For a single target, we invalidate a transition if it points back to its source but it has no
    // condition
    // or event.  We don't attempt any more sophisticated analysis at this point although one could
    // conceive of
    // an analysis that could find loops depending on how long you wanted it to run.
    void RecursiveValidateTransitions()
    {
        QT3DS_ASSERT(m_Context->GetRoot());
        RecursiveValidateTransitions(*m_Context->GetRoot(), NULL, 0);
    }

    NVConstDataRef<SStateNode *> GetRefList(NVConstDataRef<SStateNode *> inIds) { return inIds; }

    // unsorted, may contain duplicates.
    NVDataRef<SStateNode *> GetRefList(NVConstDataRef<CRegisteredString> inIds)
    {
        m_IDRefList.clear();
        for (QT3DSU32 idx = 0, end = inIds.size(); idx < end; ++idx) {
            SStateNode *theNode = m_Context->FindStateNode(inIds[idx]);
            if (theNode != NULL) {
                m_IDRefList.push_back(theNode);
            }
        }
        return m_IDRefList;
    }

    NVConstDataRef<SStateNode *> GetTargetStates(STransition *inTransition)
    {
        if (inTransition)
            return inTransition->m_Target;
        return NVDataRef<SStateNode *>();
    }

    static bool IsCompoundState(const SStateNode *inNode)
    {
        if (inNode == NULL)
            return false;
        return inNode->IsCompound();
    }

    static bool IsDescendant(SStateNode &inParent, SStateNode &inChild)
    {
        if (&inChild == &inParent)
            return false;

        for (SStateNode *theNode = &inChild; theNode; theNode = theNode->m_Parent)
            if (theNode == &inParent)
                return true;
        return false;
    }

    static bool AllAreDescendants(SStateNode &inParent, NVConstDataRef<SStateNode *> inList)
    {
        for (QT3DSU32 idx = 0, end = inList.size(); idx < end; ++idx)
            if (IsDescendant(inParent, *inList[idx]) == false)
                return false;
        return true;
    }
    struct SRefListPlusOne
    {
        NVConstDataRef<SStateNode *> m_List;
        SStateNode *m_Extra;

        SRefListPlusOne(NVConstDataRef<SStateNode *> l, SStateNode *e)
            : m_List(l)
            , m_Extra(e)
        {
        }

        QT3DSU32 size() { return m_List.size() + (m_Extra ? 1 : 0); }

        SStateNode *operator[](int idx)
        {
            if (idx < (int)m_List.size())
                return m_List[idx];
            if (idx == (int)m_List.size())
                return m_Extra;
            QT3DS_ASSERT(false);
            return NULL;
        }
    };

    SStateNode *FindLCA(SStateNode &lhs, SStateNode &rhs)
    {
        QT3DSU32 sharedIdx = GetSharedParentState(lhs, rhs, false);
        return *(m_ParentComparisonLHS.rbegin() + sharedIdx);
    }

    SStateNode *FindLCCA(NVConstDataRef<SStateNode *> inList, SStateNode *inExtra = NULL)
    {
        SRefListPlusOne theList(inList, inExtra);
        if (theList.size() == 0)
            return NULL;
        if (theList.size() == 1) {
            for (SStateNode *item = theList[0]; item; item = item->m_Parent) {
                if (item->IsCompound())
                    return item;
            }
        }
        SStateNode *lhs = theList[0];
        for (QT3DSU32 idx = 1, end = theList.size(); idx < end; ++idx) {
            SStateNode *rhs = theList[idx];
            QT3DSU32 sharedIdx = GetSharedParentState(*lhs, *rhs, false);
            lhs = *(m_ParentComparisonLHS.rbegin() + sharedIdx);
        }
        for (SStateNode *item = lhs; item; item = item->m_Parent) {
            if (item->IsCompound())
                return item;
        }
        QT3DS_ASSERT(false);
        return m_Context->GetRoot();
    }
    NVDataRef<SStateNode *> GetProperAncestors(SStateNode &inChild, SStateNode *inStop)
    {
        m_AncestorsList.clear();
        for (SStateNode *parent = inChild.m_Parent; parent && parent != inStop;
             parent = parent->m_Parent)
            m_AncestorsList.push_back(parent);
        return m_AncestorsList;
    }

    bool AddStateToEnterToSet(SStateNode &inNode)
    {
        if (m_Configuration.contains(&inNode) == false) {
            m_StatesToEnter.insert(&inNode);
            return true;
        }
        return false;
    }

    void AddStatesToEnter(SHistory &inState)
    {
        TStateNodeNodeList *history = GetHistoryData(inState);
        if (history) {
            for (TStateNodeNodeList::iterator iter = history->begin(), end = history->end();
                 iter != end; ++iter) {
                AddStatesToEnter(*iter->m_Node);
                NVDataRef<SStateNode *> ancestors =
                    GetProperAncestors(*iter->m_Node, inState.m_Parent);
                for (QT3DSU32 ancIdx = 0, ancEnd = ancestors.size(); ancIdx < ancEnd; ++ancIdx)
                    AddStateToEnterToSet(*ancestors[ancIdx]);
            }
        } else {
            if (IsTransitionValid(inState.m_Transition)) {
                NVConstDataRef<SStateNode *> theList = GetRefList(inState.m_Transition->m_Target);
                for (QT3DSU32 idx = 0, end = theList.size(); idx < end; ++idx) {
                    AddStatesToEnter(*theList[idx]);
                    NVDataRef<SStateNode *> ancestors =
                        GetProperAncestors(*theList[idx], inState.m_Parent);
                    for (QT3DSU32 ancIdx = 0, ancEnd = ancestors.size(); ancIdx < ancEnd; ++ancIdx)
                        AddStateToEnterToSet(*ancestors[ancIdx]);
                }
            } else {
                qCCritical(INVALID_OPERATION,
                    "History node %s with no history, no transition, or invalid transition visited",
                    inState.m_Id.c_str());
                SStateNode *theParent = inState.m_Parent;
                if (theParent != NULL) {
                    if (theParent->m_Type != StateNodeTypes::Parallel) {
                        NVConstDataRef<SStateNode *> theDefaultInitial(
                            GetDefaultInitialState(*theParent));

                        QT3DS_ASSERT(theDefaultInitial.size() != 0);
                        for (QT3DSU32 idx = 0, end = theDefaultInitial.size(); idx < end; ++idx)
                            AddStatesToEnter(*theDefaultInitial[idx]);

                        EnterAncestors(theDefaultInitial, theParent);
                    } else {
                        SParallel *pstate = theParent->CastTo<SParallel>();
                        for (TStateNodeList::iterator iter = pstate->m_Children.begin(),
                                                      end = pstate->m_Children.end();
                             iter != end; ++iter) {
                            if (StateNodeTypes::IsStateType(iter->m_Type))
                                AddStatesToEnter(*iter);
                        }
                    }
                } else {
                    QT3DS_ASSERT(false); // invalid configuration
                }
            }
        }
    }
    NVConstDataRef<SStateNode *> GetDefaultInitialState(SStateNode &inState)
    {
        TStateNodeList *theChildren = inState.GetChildren();
        NVConstDataRef<SStateNode *> retval;
        if (theChildren) {
            for (TStateNodeList::iterator iter = theChildren->begin(), end = theChildren->end();
                 iter != end && retval.size() == 0; ++iter) {
                if (iter->m_Type == StateNodeTypes::State
                    || iter->m_Type == StateNodeTypes::Parallel
                    || iter->m_Type == StateNodeTypes::Final) {
                    SStateNode **newData =
                        reinterpret_cast<SStateNode **>(m_TemporaryAllocator.allocate(
                            sizeof(SStateNode *), "TempNode", __FILE__, __LINE__));
                    newData[0] = &(*iter);
                    retval = toDataRef(newData, 1);
                }
            }
        }
        return retval;
    }

    STransition *GetStateInitialTransition(SStateNode &inNode)
    {
        // Initialexpr, if it exists, takes precedence over the initial transition.
        // they should not both exist but coding defensively, we have to take this into account.
        eastl::pair<nvhash_map<SStateNode *, STransition *>::iterator, bool> inserter =
            m_StateInitialTransitionMap.insert(eastl::make_pair(&inNode, (STransition *)NULL));
        if (inserter.second) {
            const char8_t *initialExpr = inNode.GetInitialExpression();
            if (!isTrivial(initialExpr)) {
                STransition *newTransition = &CreateTemporaryTransition();
                newTransition->m_Parent = &inNode;
                inserter.first->second = newTransition;
                SScriptExecutionResult exprResultData =
                    m_ScriptContext->ExecuteExpressionToString(initialExpr);
                if (exprResultData.Valid()) {
                    const char8_t *exprResult(exprResultData.Result());
                    // split this string into parts and extract ids.
                    m_IdSplitter.assign(nonNull(exprResult));
                    const char8_t *whitespaceData = " \n\t\r";
                    size_t charPos = m_IdSplitter.find_first_not_of(whitespaceData);
                    if (charPos == eastl::string::npos) {
                        m_IdSplitter.clear();
                        m_Workspace.clear();
                    }
                    if (charPos != 0)
                        m_IdSplitter.erase(m_IdSplitter.begin(), m_IdSplitter.begin() + charPos);
                    // Loop runs under assumption that idSplitter is empty or position 0 holds start
                    // of next id
                    while (m_IdSplitter.size()) {
                        // Trim to first character
                        eastl::string::size_type spacePos =
                            m_IdSplitter.find_first_of(whitespaceData);

                        if (spacePos != eastl::string::npos) {
                            charPos = m_IdSplitter.find_first_not_of(whitespaceData, spacePos);
                            m_Workspace = m_IdSplitter.c_str();
                            m_Workspace.resize(spacePos);
                            if (charPos != eastl::string::npos)
                                m_IdSplitter.erase(m_IdSplitter.begin(),
                                                   m_IdSplitter.begin() + charPos);
                            else
                                m_IdSplitter.clear();
                        } else {
                            m_Workspace = m_IdSplitter;
                            m_IdSplitter.clear();
                        }

                        if (m_Workspace.empty() == false) {
                            CRegisteredString stateId =
                                m_StringTable->RegisterStr(m_Workspace.c_str());
                            qt3ds::state::SStateNode *transitionNode =
                                m_Context->FindStateNode(stateId);
                            if (!transitionNode) {
                                m_TempNodeList.clear();
                                m_IdSplitter.clear();
                                qCCritical(INVALID_OPERATION,
                                    "initialexpr=\"%s\" evaluated to \"%s\", but "
                                    "this does not match the states in this "
                                    "document. Using the default initial state "
                                    "instead.",
                                    nonNull(initialExpr), nonNull(exprResult));

                                eastl::string errorBuf;
                                IScriptEvent *newEvent = m_ScriptContext->CreateScriptEvent(
                                    m_StringTable->RegisterStr("error.execution.initialexpr"));
                                newEvent->SetParamStr(m_StringTable->RegisterStr("expr"),
                                                      nonNull(initialExpr));
                                errorBuf.assign(nonNull(exprResult));
                                errorBuf.append("  does not match the states in this document.");
                                newEvent->SetParamStr(m_StringTable->RegisterStr("error"),
                                                      errorBuf.c_str());
                                QueueEvent(*newEvent, false);
                            } else
                                m_TempNodeList.push_back(transitionNode);
                        }
                    }
                    if (m_TempNodeList.empty() == false) {
                        QT3DSU32 allocSize = sizeof(SStateNode *) * m_TempNodeList.size();
                        SStateNode **nodeData =
                            reinterpret_cast<SStateNode **>(m_TemporaryAllocator.allocate(
                                allocSize, "TempNodes", __FILE__, __LINE__));
                        memCopy(nodeData, m_TempNodeList.data(), allocSize);
                        newTransition->m_Target = toDataRef(nodeData, m_TempNodeList.size());
                        m_TempNodeList.clear();
                    }
                    if (newTransition->m_Target.size() != 0) {
                        bool isTransValid = ValidateTransitionTargetList(*newTransition, inNode, 0);
                        if (!isTransValid) {
                            m_InvalidTransitions.erase(newTransition);
                            // Reset the transition so that we get just the default initial state
                            // below
                            newTransition->m_Target = NVConstDataRef<SStateNode *>();
                            // Create appropriate messages and events.
                            qCCritical(INVALID_OPERATION,
                                "initialexpr=\"%s\" evaluated to \"%s\", but this "
                                "results in an invalid transition. Using the "
                                "default initial state instead.",
                                nonNull(initialExpr), nonNull(exprResult));
                            eastl::string errorBuf;
                            IScriptEvent *newEvent = m_ScriptContext->CreateScriptEvent(
                                m_StringTable->RegisterStr("error.execution.initialexpr"));
                            newEvent->SetParamStr(m_StringTable->RegisterStr("expr"),
                                                  nonNull(initialExpr));
                            errorBuf.assign(nonNull(exprResult));
                            errorBuf.append("  results in invalid transition.");
                            newEvent->SetParamStr(m_StringTable->RegisterStr("error"),
                                                  errorBuf.c_str());
                            QueueEvent(*newEvent, false);
                        }
                    }
                } // if script executed successfully
                else {
                    const char8_t *runtimeError = exprResultData.Error();
                    IScriptEvent *newEvent = m_ScriptContext->CreateScriptEvent(
                        m_StringTable->RegisterStr("error.execution.initialexpr"));
                    newEvent->SetParamStr(m_StringTable->RegisterStr("expr"), nonNull(initialExpr));
                    newEvent->SetParamStr(m_StringTable->RegisterStr("error"),
                                          nonNull(runtimeError));
                    QueueEvent(*newEvent, false);
                }
                if (newTransition->m_Target.size() == 0) {
                    newTransition->m_Target = GetDefaultInitialState(inNode);
                }
            }
            if (inserter.first->second == NULL) {
                // Assume already validated
                inserter.first->second = inNode.GetInitialTransition();
            }
        }
        return inserter.first->second;
    }

    void AddStatesToEnter(SStateNode &inState)
    {
        if (inState.m_Type == StateNodeTypes::History)
            AddStatesToEnter(*inState.CastTo<SHistory>());
        else {
            AddStateToEnterToSet(inState);
            if (inState.IsCompound()) {
                m_StatesForDefaultEntry.insert(&inState);
                NVConstDataRef<SStateNode *> targets;
                STransition *initialTrans = GetStateInitialTransition(inState);
                if (IsTransitionValid(initialTrans))
                    targets = GetTargetStates(initialTrans);

                if (targets.size() == 0)
                    targets = GetDefaultInitialState(inState);

                QT3DS_ASSERT(targets.size() != 0);
                for (QT3DSU32 idx = 0, end = targets.size(); idx < end; ++idx)
                    AddStatesToEnter(*targets[idx]);

                EnterAncestors(targets, &inState);
            } else if (inState.m_Type == StateNodeTypes::Parallel) {
                SParallel *pstate = inState.CastTo<SParallel>();
                for (TStateNodeList::iterator iter = pstate->m_Children.begin(),
                                              end = pstate->m_Children.end();
                     iter != end; ++iter) {
                    if (StateNodeTypes::IsStateType(iter->m_Type))
                        AddStatesToEnter(*iter);
                }
            }
        }
    }
    bool AllChildrenInFinalStates(SStateNode &inNode)
    {
        TStateNodeList *children = inNode.GetChildren();
        if (children) {
            for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                 iter != end; ++iter)
                if (!IsInFinalState(*iter))
                    return false;
            return true;
        }
        return true;
    }

    void EraseState(nvvector<SStateNode *> &inList, SStateNode *inItem)
    {
        nvvector<SStateNode *>::iterator iter = eastl::find(inList.begin(), inList.end(), inItem);
        if (iter != inList.end())
            inList.erase(iter);
    }

    void ExitStates(NVDataRef<STransition *> inTransitions)
    {
        m_StatesToExit.clear();
        for (QT3DSU32 idx = 0, end = inTransitions.size(); idx < end; ++idx) {
            STransition &transition(*inTransitions[idx]);
            if (transition.GetSource() == NULL) {
                QT3DS_ASSERT(false);
                continue;
            }
            NVConstDataRef<SStateNode *> theList = GetRefList(transition.m_Target);
            SStateNode *ancestor = GetTransitionSubgraphRoot(transition, theList);
            if (theList.size() && transition.GetSource()) {
                for (QT3DSU32 idx = 0, end = m_Configuration.size(); idx < end; ++idx)
                    if (IsDescendant(*ancestor, *m_Configuration[idx]))
                        m_StatesToExit.insert(m_Configuration[idx]);
            }
        }

        /*
             for s in m_StatesToExit:
                     statesToInvoke.delete(s)
                     */

        SortByDocumentOrder(m_StatesToExit.m_List);
        DEBUGGER_CALL1(SetExitSet, m_StatesToExit);
        // Run through the list in reverse order.
        for (nvvector<SStateNode *>::reverse_iterator iter = m_StatesToExit.m_List.rbegin(),
                                                      end = m_StatesToExit.m_List.rend();
             iter != end; ++iter) {
#ifdef UIC_LOG_ENTER_EXIT
            qCInfo(TRACE_INFO, "Exiting state: %s", (*iter)->m_Id.c_str());
#endif
            TStateNodeList *children = (*iter)->GetChildren();
            if (children) {
                for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                     iter != end; ++iter) {
                    if (iter->m_Type == StateNodeTypes::History) {
                        SHistory &theHistory = *iter->CastTo<SHistory>();
                        FreeHistoryData(theHistory);
                        TStateNodeNodeList &theHistoryData = GetOrCreateHistoryData(theHistory);

                        if (theHistory.m_Parent == NULL) {
                            QT3DS_ASSERT(false);
                            continue;
                        }

                        // If deep, then record the leaves
                        if (theHistory.m_Flags.IsDeep()) {
                            for (TStateNodeSet::iterator configIter = m_Configuration.begin(),
                                                         configEnd = m_Configuration.end();
                                 configIter != configEnd; ++configIter) {
                                if ((*configIter)->IsAtomic()
                                    && IsDescendant(*theHistory.m_Parent, **configIter))
                                    theHistoryData.push_back(*m_StateNodeNodePool.construct(
                                        *configIter, __FILE__, __LINE__));
                            }
                        }
                        // Else record what may be branches.
                        else {
                            for (TStateNodeSet::iterator configIter = m_Configuration.begin(),
                                                         configEnd = m_Configuration.end();
                                 configIter != configEnd; ++configIter) {
                                if ((*configIter)->m_Parent == theHistory.m_Parent)
                                    theHistoryData.push_back(*m_StateNodeNodePool.construct(
                                        *configIter, __FILE__, __LINE__));
                            }
                        }
                    }
                }
            }
        }

        for (nvvector<SStateNode *>::reverse_iterator iter = m_StatesToExit.m_List.rbegin(),
                                                      end = m_StatesToExit.m_List.rend();
             iter != end; ++iter) {
            SStateNode *theState = *iter;
            TOnExitList *theExitList = theState->GetOnExitList();
            if (theExitList)
                m_ExecutionContext->Execute(*theState, *theExitList);
            for (QT3DSU32 idx = 0, end = m_SignalSenders.size(); idx < end; ++idx)
                m_SignalSenders[idx]->m_Handler.OnInterpreterEvent(InterpreterEventTypes::StateExit,
                                                                   theState->m_Id);
            EraseState(m_StatesToInvoke, theState);
            m_Configuration.erase(theState);
        }
    }

    void EnterAncestor(SStateNode &inNode)
    {
        m_StatesToEnter.insert(&inNode);
        if (inNode.m_Type == StateNodeTypes::Parallel) {
            SParallel *anc = inNode.CastTo<SParallel>();
            for (TStateNodeList::iterator childIter = anc->m_Children.begin(),
                                          endChild = anc->m_Children.end();
                 childIter != endChild; ++childIter) {
                if (!StateNodeTypes::IsStateType(childIter->m_Type))
                    continue;
                bool hasDescendent = false;
                for (TStateNodeSet::iterator existingIter = m_StatesToEnter.begin(),
                                             existingEnd = m_StatesToEnter.end();
                     existingIter != existingEnd && hasDescendent == false; ++existingIter) {
                    hasDescendent = IsDescendant(*childIter, *(*existingIter));
                }
                if (hasDescendent == false)
                    AddStatesToEnter(*childIter);
            }
        }
    }

    void EnterAncestors(NVConstDataRef<SStateNode *> inList, SStateNode *ancestor)
    {
        for (QT3DSU32 idx = 0, end = inList.size(); idx < end; ++idx) {
            NVDataRef<SStateNode *> theAncestors = GetProperAncestors(*inList[idx], ancestor);
            for (QT3DSU32 ancIdx = 0, ancEnd = theAncestors.size(); ancIdx < ancEnd; ++ancIdx) {
                EnterAncestor(*theAncestors[ancIdx]);
                // Break earlier if we have gone back and included the ancestor.
                if (theAncestors[ancIdx] == ancestor)
                    break;
            }
        }
    }

    void EnterStates(NVDataRef<STransition *> inTransitions)
    {
        m_StatesToEnter.clear();
        m_StatesForDefaultEntry.clear();

        for (QT3DSU32 idx = 0, end = inTransitions.size(); idx < end; ++idx) {
            STransition &transition = *inTransitions[idx];
            NVConstDataRef<SStateNode *> theList = GetRefList(transition.m_Target);
            if (transition.GetSource() == NULL) {
                QT3DS_ASSERT(false);
                continue;
            }

            // Multi-target transitions present their own set of problems.  One is that a perfectly
            // valid
            // multi-target transition may
            if (theList.size() > 1) {
                m_TransitionTargetList.clear();
                // We have to ensure that if two targets are directly related, we take the most
                // derived one.
                for (QT3DSU32 targetIdx = 0, targetEnd = theList.size(); targetIdx < targetEnd;
                     ++targetIdx) {
                    SStateNode *nextTarget = theList[targetIdx];
                    for (QT3DSU32 takenTargetListIdx = 0,
                               takenTargetListEnd = m_TransitionTargetList.size();
                         takenTargetListIdx < takenTargetListEnd && nextTarget;
                         ++takenTargetListIdx) {
                        SStateNode *previousTarget = m_TransitionTargetList[takenTargetListIdx];
                        // If the previous target is more descendant than the original target
                        if (IsDescendant(*nextTarget, *previousTarget)) {
                            // Then we don't need to consider next target at all and we can
                            // continue.
                            nextTarget = NULL;
                        } else if (IsDescendant(*previousTarget, *nextTarget)) {
                            // If next target derives from previous target, then we remove previous
                            // target from
                            // the list.
                            m_TransitionTargetList.erase(m_TransitionTargetList.begin()
                                                         + takenTargetListIdx);
                            --takenTargetListIdx;
                            takenTargetListEnd = m_TransitionTargetList.size();
                        }
                        // Else we don't care, we will add next target to the list.
                    }
                    if (nextTarget != NULL)
                        m_TransitionTargetList.push_back(nextTarget);
                }
                theList = m_TransitionTargetList;
            }

            for (QT3DSU32 idx = 0, end = theList.size(); idx < end; ++idx)
                AddStatesToEnter(*theList[idx]);
        }

        for (QT3DSU32 idx = 0, end = inTransitions.size(); idx < end; ++idx) {
            STransition &transition = *inTransitions[idx];
            NVConstDataRef<SStateNode *> theList = GetRefList(transition.m_Target);

            if (transition.GetSource() == NULL) {
                QT3DS_ASSERT(false);
                continue;
            }

            SStateNode *ancestor = GetTransitionSubgraphRoot(transition, theList);
            EnterAncestors(theList, ancestor);
        }
        EnterStatesSecondHalf();
    }

    void EnterStatesSecondHalf()
    {
        nvvector<SStateNode *> &theEnterList(m_StatesToEnter.m_List);
        SortByDocumentOrder(theEnterList);

        DEBUGGER_CALL1(SetEnterSet, m_StatesToEnter);
        for (QT3DSU32 idx = 0, end = theEnterList.size(); idx < end; ++idx) {
            SStateNode *theEnterState(theEnterList[idx]);
#ifdef UIC_LOG_ENTER_EXIT
            qCInfo(TRACE_INFO, "Entering state: %s", theEnterState->m_Id.c_str());
#endif
            m_Configuration.insert(theEnterState);
            m_StatesToInvoke.push_back(theEnterState);
            SStateNodeInterpreterData &theData = GetOrCreateInterpreterData(*theEnterState);
            if (theData.m_Entered == false) {
                theData.m_Entered = true;
                if (m_Context->GetRoot()->m_Flags.IsLateBinding()) {
                    SDataModel *theDataModel = theEnterState->GetDataModel();
                    if (theDataModel)
                        RunDataModel(*theDataModel);
                }
            }

            TOnEntryList *theList = theEnterState->GetOnEntryList();
            if (theList)
                m_ExecutionContext->Execute(*theEnterState, *theList);
            for (QT3DSU32 idx = 0, end = m_SignalSenders.size(); idx < end; ++idx)
                m_SignalSenders[idx]->m_Handler.OnInterpreterEvent(
                    InterpreterEventTypes::StateEnter, theEnterState->m_Id);

            if (m_StatesForDefaultEntry.contains(theEnterState)) {
                SState *theState = theEnterState->CastTo<SState>();
                if (theState && IsTransitionValid(theState->m_Initial))
                    m_ExecutionContext->Execute(*theState->m_Initial);
            }
            if (theEnterState->m_Type == StateNodeTypes::Final) {
                SStateNode *parent = theEnterState->m_Parent;
                SStateNode *grandparent = parent->m_Parent;
                if (parent && grandparent) {
                    m_Workspace.assign("done.state.");
                    m_Workspace.append(parent->m_Id);

                    // TODO - donedata
                    QueueEvent(m_Workspace.c_str(), false);

                    if (grandparent && grandparent->m_Type == StateNodeTypes::Parallel
                        && AllChildrenInFinalStates(*grandparent)) {
                        m_Workspace.assign("done.state.");
                        m_Workspace.append(grandparent->m_Id);
                        QueueEvent(m_Workspace.c_str(), false);
                    }
                }
            }
        }

        if (IsInFinalState(m_Context->GetRoot()))
            m_Running = false;
        SortConfiguration();
    }

    bool InConfiguration(SStateNode &inState)
    {
        return eastl::find(m_Configuration.begin(), m_Configuration.end(), &inState)
            != m_Configuration.end();
    }

    bool IsInFinalState(SStateNode &inState)
    {
        if (inState.m_Type == StateNodeTypes::State && inState.IsCompound()) {
            SState *theState = inState.CastTo<SState>();
            for (TStateNodeList::iterator childIter = theState->m_Children.begin(),
                                          endIter = theState->m_Children.end();
                 childIter != endIter; ++childIter) {
                if (IsInFinalState(*childIter))
                    return true;
            }
        } else if (inState.m_Type == StateNodeTypes::SCXML) {
            SSCXML *theState = inState.CastTo<SSCXML>();
            for (TStateNodeList::iterator childIter = theState->m_Children.begin(),
                                          endIter = theState->m_Children.end();
                 childIter != endIter; ++childIter) {
                if (childIter->m_Type == StateNodeTypes::Final && InConfiguration(*childIter))
                    return true;
            }
        } else if (inState.m_Type == StateNodeTypes::Parallel) {
            SParallel *parallel = inState.CastTo<SParallel>();
            for (TStateNodeList::iterator childIter = parallel->m_Children.begin(),
                                          endIter = parallel->m_Children.end();
                 childIter != endIter; ++childIter) {
                if (!IsInFinalState(*childIter))
                    return false;
            }
            return true;
        } else if (inState.m_Type == StateNodeTypes::Final && InConfiguration(inState))
            return true;

        return false;
    }

    bool IsInFinalState(SStateNode *inState)
    {
        if (inState)
            return IsInFinalState(*inState);
        return false;
    }

    SStateNode *GetTransitionSubgraphRoot(STransition &inTransition,
                                          NVConstDataRef<SStateNode *> inTargetList)
    {
        SStateNodeInterpreterData &data = GetOrCreateInterpreterData(inTransition);
        if (data.m_TransitionSubgraphRoot != NULL)
            return data.m_TransitionSubgraphRoot;
        if (inTransition.GetSource() == NULL) {
            QT3DS_ASSERT(false);
            return NULL;
        }
        SStateNode &source = *inTransition.GetSource();
        if (inTransition.m_Target.size() == 0)
            data.m_TransitionSubgraphRoot = &source;
        else {
            if (inTransition.m_Flags.IsInternal() && source.IsCompound()
                && AllAreDescendants(source, inTargetList))
                data.m_TransitionSubgraphRoot = &source;
            else
                data.m_TransitionSubgraphRoot = FindLCCA(inTargetList, &source);
        }

        return data.m_TransitionSubgraphRoot;
    }
    SStateNode *GetTransitionSubgraphRoot(STransition &inTransition)
    {
        SStateNodeInterpreterData &data = GetOrCreateInterpreterData(inTransition);
        if (data.m_TransitionSubgraphRoot != NULL)
            return data.m_TransitionSubgraphRoot;

        return GetTransitionSubgraphRoot(inTransition, GetRefList(inTransition.m_Target));
    }
    enum PreemptRule { AddNew, KeepExisting, ReplaceExisting };

    PreemptRule IsPreempted(STransition &existing, STransition &nextTransition)
    {
        // Targetless transitions can get preempted and can preempt
        SStateNode *existingRoot = GetTransitionSubgraphRoot(existing);
        SStateNode *nextRoot = GetTransitionSubgraphRoot(nextTransition);
        /*
        http://www.w3.org/TR/scxml/#SelectingTransitions

        A transition T is optimally enabled by event E in atomic state S if
        a) T is enabled by E in S and
        b) no transition that precedes T in document order in T's source state is enabled by E in S
        and
        c) no transition is enabled by E in S in any descendant of T's source state.
        */

        // static bool IsDescendant( SStateNode& inParent, SStateNode& inChild )

        if (IsDescendant(*nextRoot, *existingRoot))
            return KeepExisting;
        if (IsDescendant(*existingRoot, *nextRoot))
            return ReplaceExisting;

        // Else these transactions are completely unrelated
        return AddNew;
    }

    void FilterPreempted(TTransitionSet &inTransitionSet, TTransitionList &outTransitions)
    {
        outTransitions.clear();
        for (TTransitionSet::iterator iter = inTransitionSet.begin(), end = inTransitionSet.end();
             iter != end; ++iter) {
            PreemptRule preempted = AddNew;
            QT3DSU32 idx, endIdx;
            for (idx = 0, endIdx = outTransitions.size(); idx < endIdx && preempted == AddNew;
                 ++idx)
                preempted = IsPreempted(*outTransitions[idx], **iter);

            switch (preempted) {
            case KeepExisting: // Ignore the result.
                break;
            case ReplaceExisting:
                // The iteration statement is evaluated before the exit test.
                outTransitions[idx - 1] = *iter;
                break;
            case AddNew:
                outTransitions.push_back(*iter);
                break;
            }
        }
    }

    bool SelectEventlessTransitions(SStateNode &inNode, TTransitionSet &inSet)
    {
        TStateNodeList *theChildren = inNode.GetChildren();
        if (theChildren == NULL) {
            return false;
        }
        for (TStateNodeList::iterator iter = theChildren->begin(), end = theChildren->end();
             iter != end; ++iter) {
            if (iter->m_Type == StateNodeTypes::Transition) {
                STransition &trans = *iter->CastTo<STransition>();
                if (IsTransitionValid(&trans)) {
                    if (trans.m_Event.IsValid() == false) {
                        if (!isTrivial(trans.m_Condition)) {
                            Option<bool> condEval =
                                m_ScriptContext->ExecuteCondition(trans.m_Condition);
                            if (condEval.hasValue()) {
                                if (*condEval) {
                                    inSet.insert(&trans);
                                    return true;
                                }
                            } else {
                                QueueEvent("error.execution", false);
                                return false;
                            }
                        }
                        // Extension for running scxml documents with transitions with nothing but
                        // content
                        else {
                            inSet.insert(&trans);
                            return true;
                        }
                    }
                }
            }
        }
        if (inNode.m_Parent)
            SelectEventlessTransitions(*inNode.m_Parent, inSet);
        return false;
    }

    // Precondition - m_Configuration is in document order.
    // Postcondition - m_EnabledTransitions contains only the transitions selected
    void SelectEventlessTransitions()
    {
        m_TransitionSet.clear();
        static QT3DSU32 callCount = 0;
        ++callCount;
        for (QT3DSU32 idx = 0, end = m_Configuration.size(); idx < end; ++idx) {
            if (idx) {
                QT3DS_ASSERT(GetOrCreateInterpreterData(*m_Configuration[idx]).m_DocumentOrder
                          > GetOrCreateInterpreterData(*m_Configuration[idx - 1]).m_DocumentOrder);
            }
            if (m_Configuration[idx]->IsAtomic() == false)
                continue;
            SelectEventlessTransitions(*m_Configuration[idx], m_TransitionSet);
        }
        m_EnabledTransitions.clear();
        FilterPreempted(m_TransitionSet, m_EnabledTransitions);
    }
    void SelectTransition(SStateNode &inNode, IEvent &inEvent, TTransitionSet &outTransitions)
    {
        TStateNodeList *theChildren = inNode.GetChildren();
        if (theChildren) {
            for (TStateNodeList::iterator iter = theChildren->begin(), end = theChildren->end();
                 iter != end; ++iter) {
                if (iter->m_Type == StateNodeTypes::Transition) {
                    STransition &theTransition = *iter->CastTo<STransition>();
                    if (IsTransitionValid(theTransition)) {
                        if (theTransition.m_Event.IsValid()) {
                            if (qt3ds::state::impl::NameMatches(theTransition.m_Event.c_str(),
                                                              inEvent.GetName().c_str())) {
                                if (!isTrivial(theTransition.m_Condition)) {
                                    Option<bool> condResult = m_ScriptContext->ExecuteCondition(
                                        theTransition.m_Condition);
                                    if (condResult.hasValue()) {
                                        if (*condResult) {
                                            outTransitions.insert(&theTransition);
                                            return;
                                        }
                                    } else {
                                        QueueEvent("error.execution", false);
                                        return;
                                    }
                                } else {
                                    outTransitions.insert(&theTransition);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (inNode.m_Parent)
            SelectTransition(*inNode.m_Parent, inEvent, outTransitions);
    }

    // Precondition is that m_Configuration is in document order.
    void SelectTransitions(IEvent &inEvent)
    {
        m_TransitionSet.clear();
        for (QT3DSU32 idx = 0, end = m_Configuration.size(); idx < end; ++idx) {
            // Ensure that m_Configuration is in document order.
            if (idx) {
                QT3DS_ASSERT(GetOrCreateInterpreterData(*m_Configuration[idx]).m_DocumentOrder
                          > GetOrCreateInterpreterData(*m_Configuration[idx - 1]).m_DocumentOrder);
            }

            if (!m_Configuration[idx]->IsAtomic())
                continue;
            SelectTransition(*m_Configuration[idx], inEvent, m_TransitionSet);
        }

        FilterPreempted(m_TransitionSet, m_EnabledTransitions);
    }

    bool Initialize(IStateContext &inContext, bool inValidateTransitions) override
    {
        m_Initialized = false;
        InitializeDataStructures();
        m_Context = inContext;
        QT3DS_ASSERT(m_Context->GetRoot());
        if (m_Context->GetRoot() == NULL) {
            QT3DS_ASSERT(false);
            m_Running = false;
            return false;
        }

        // Disable transitions that can put us into a bad state
        if (inValidateTransitions)
            RecursiveValidateTransitions();
        m_Initialized = true;
        return true;
    }
    NVConstDataRef<SStateNode *> Start() override
    {
        if (!m_Initialized) {
            QT3DS_ASSERT(false);
            return NVConstDataRef<SStateNode *>();
        }
        m_Running = true;
        SSCXML &rootState = *m_Context->GetRoot();
        if (!rootState.m_Flags.IsLateBinding())
            RecursiveInitializeDataModel();
        eastl::string theSendLocExpr;
        NVConstDataRef<SSend *> theSendData = m_Context->GetSendList();
        for (QT3DSU32 idx = 0, end = theSendData.size(); idx < end; ++idx) {
            SSend &theSend(*theSendData[idx]);
            if (!isTrivial(theSend.m_IdLocation)) {
                QT3DS_ASSERT(theSend.m_Id.IsValid());
                m_ScriptContext->AssignStr(theSend.m_IdLocation, theSend.m_Id.c_str());
            }
        }

        STransition *initialTrans = this->GetStateInitialTransition(rootState);
        if (IsTransitionValid(initialTrans)) {
            m_Configuration.insert(&rootState);
            m_ExecutionContext->Execute(*initialTrans);
            AddStatesToEnter(rootState);
            EnterAncestors(initialTrans->m_Target, &rootState);
            QT3DS_ASSERT(m_EnabledTransitions.empty());
            m_EnabledTransitions.clear();
            m_EnabledTransitions.push_back(initialTrans);
            DEBUGGER_CALL1(SetTransitionSet, m_EnabledTransitions);
            m_EnabledTransitions.clear();
            EnterStatesSecondHalf();
            ReleaseInitialAndTemporaryTransitions();
            DEBUGGER_CALL(EndMicrostep);
            DEBUGGER_CALL(EndStep);
        } else {
            m_Running = false;
            qFatal("Invalid state machine: root initial configuration is invalid");
        }
        return m_Configuration.m_List;
    }

    void Microstep(TTransitionList &inTransitions)
    {
        DEBUGGER_CALL1(SetTransitionSet, inTransitions);
        ExitStates(inTransitions);
        for (QT3DSU32 idx = 0, end = inTransitions.size(); idx < end; ++idx) {
            m_ExecutionContext->Execute(*inTransitions[idx]);
            for (QT3DSU32 sigIdx = 0, sigEnd = m_SignalSenders.size(); sigIdx < sigEnd; ++sigIdx)
                m_SignalSenders[sigIdx]->m_Handler.OnInterpreterEvent(
                    InterpreterEventTypes::Transition, inTransitions[idx]->m_Id);
        }

        EnterStates(inTransitions);
        // Allow them to be selected again at some point.
        inTransitions.clear();
        ReleaseInitialAndTemporaryTransitions();
        DEBUGGER_CALL(EndMicrostep);
    }

    void CheckForDelayedEvents()
    {
        QT3DSU64 currentTime = Time::getCurrentTimeInTensOfNanoSeconds();
        TFutureEventList::iterator removeIter = m_FutureEvents.begin();
        TFutureEventList::iterator endIter = m_FutureEvents.end();
        // Future events list is sorted so all we have to do is iterate forward until
        // fire time is greater than current time.

        for (; removeIter != endIter && removeIter->m_FireTime < currentTime; ++removeIter) {
        }
        // remove iter points either to the end or to the first event who's time has come.
        for (TFutureEventList::iterator evtIter = m_FutureEvents.begin(); evtIter != removeIter;
             ++evtIter) {
            qCInfo(TRACE_INFO, "Sending delayed event: %s", evtIter->m_Event->GetName().c_str());
            QT3DS_ASSERT(evtIter->m_FireTime <= currentTime);
            QueueEvent(evtIter->m_Event, evtIter->m_IsExternal);
        }
        if (removeIter != m_FutureEvents.begin()) {
            m_FutureEvents.erase(m_FutureEvents.begin(), removeIter);
        }
    }

    bool CheckForStable()
    {
        CheckForDelayedEvents();
        if (m_EnabledTransitions.empty()) {
            m_ScriptContext->ClearCurrentEvent();
            DEBUGGER_CALL(BeginMicrostep);
            SelectEventlessTransitions();
        }

        return m_EnabledTransitions.empty() && m_InternalQueue.empty() && m_ExternalQueue.empty();
    }

    // Execute these events, return what state you are in.
    // We process all internal and all external events before returning, so you are free to
    // free or just deal with all userdata after this.
    NVConstDataRef<SStateNode *> Execute() override
    {
        if (m_Running == false)
            return m_Configuration.m_List;

        DEBUGGER_CALL(BeginStep);
        DEBUGGER_CALL(BeginMicrostep);

        m_EnabledTransitions.clear();

        QT3DSU32 iterationCount = 0;
        QT3DSU32 MAX_ITERATION_COUNT = 1000;

        // Here we handle eventless transitions and transitions
        // triggered by internal events until machine is stable
        while (iterationCount < MAX_ITERATION_COUNT && m_Running && !CheckForStable()) {
            ++iterationCount;
            if (m_EnabledTransitions.empty() == false) {
                Microstep(m_EnabledTransitions);
                DEBUGGER_CALL(BeginMicrostep);
            } else if (m_InternalQueue.empty() == false) {
                NVScopedRefCounted<IEvent> theEvent = m_InternalQueue.front();
                m_ScriptContext->SetCurrentEvent(theEvent);
                m_InternalQueue.pop_front();
                DEBUGGER_CALL2(SetCurrentEvent, theEvent->GetName().c_str(), true);
#ifdef UIC_LOG_ACTIVE_EVENT
                qCInfo(TRACE_INFO, "Current event: %s", theEvent->GetName().c_str());
#endif
                SelectTransitions(*theEvent);
            } else if (m_ExternalQueue.empty() == false) {
                NVScopedRefCounted<IEvent> theEvent = m_ExternalQueue.front();
                m_ScriptContext->SetCurrentEvent(theEvent);
                m_ExternalQueue.pop_front();

                DEBUGGER_CALL2(SetCurrentEvent, theEvent->GetName().c_str(), false);

#ifdef UIC_LOG_ACTIVE_EVENT
                qCInfo(TRACE_INFO, "Current event: %s", theEvent->GetName().c_str());
#endif

                /*
                if isCancelEvent(externalEvent)
                        running = false
                        continue
                */

                // TODO datamodel
                // datamodel["_event"] = theEvent;

                // TODO invoke
                /*
                for state in configuration:
                        for inv in state.invoke:
                                if inv.invokeid == externalEvent.invokeid:
                                        applyFinalize(inv, externalEvent)
                                if inv.autoforward:
                                        send(inv.id, externalEvent)
                */
                SelectTransitions(*theEvent);
            }
        }
        if (m_Running == false) {
            for (nvvector<SStateNode *>::reverse_iterator iter = m_Configuration.m_List.rbegin(),
                                                          end = m_Configuration.m_List.rend();
                 iter != end; ++iter) {
                TOnExitList *theExitList = (*iter)->GetOnExitList();
                if (theExitList)
                    m_ExecutionContext->Execute(**iter, *theExitList);
                /*
                for inv in s.invoke:
                        cancelInvoke(inv)
                        */

                // Set final done data.
                /*
                if isFinalState(s) and isScxmlState(s.parent):
                        returnDoneEvent(s.donedata)
                */
            }
        }
        DEBUGGER_CALL(EndStep);
        return m_Configuration.m_List;
    }

    bool EventsPending() const override
    {
        return m_InternalQueue.empty() == false || m_ExternalQueue.empty() == false
            || m_FutureEvents.empty() == false;
    }

    bool IsRunning() const override { return m_Running; }

    void QueueEvent(const char8_t *inEventName, QT3DSU64 inDelay, CRegisteredString inCancelId,
                            bool inIsExternal) override
    {
        TEventPtr theEvent = QT3DS_NEW(m_Foundation.getAllocator(), SSimpleEvent)(
            m_Foundation.getAllocator(), m_StringTable->RegisterStr(inEventName));
        QueueEvent(theEvent, inDelay, inCancelId, inIsExternal);
    }

    void QueueEvent(TEventPtr inEvent, QT3DSU64 inDelay, CRegisteredString inCancelId,
                            bool inIsExternal) override
    {
        if (inDelay == 0) {
            QueueEvent(inEvent, inIsExternal);
        } else {
            static QT3DSU64 sTensOfNanoSecondsInAMillisecond =
                Time::sNumTensOfNanoSecondsInASecond / 1000;
            QT3DSU64 fireTime = Time::getCurrentTimeInTensOfNanoSeconds()
                + inDelay * sTensOfNanoSecondsInAMillisecond;
            SFutureEvent theNewEvent(inEvent, fireTime, inCancelId, inIsExternal);
            TFutureEventList::iterator iter =
                eastl::upper_bound(m_FutureEvents.begin(), m_FutureEvents.end(), theNewEvent);
            m_FutureEvents.insert(iter, theNewEvent);
        }
    }

    void QueueEvent(TEventPtr inEvent, bool inIsExternal) override
    {
        if (inIsExternal)
            m_ExternalQueue.push_back(inEvent);
        else
            m_InternalQueue.push_back(inEvent);
        DEBUGGER_CALL2(EventQueued, inEvent->GetName().c_str(), inIsExternal);
    }

    void QueueEvent(const char8_t *inEventName, bool inIsExternal) override
    {
        TEventPtr theEvent = QT3DS_NEW(m_Foundation.getAllocator(), SSimpleEvent)(
            m_Foundation.getAllocator(), m_StringTable->RegisterStr(inEventName));
        QueueEvent(theEvent, inIsExternal);
    }

    void CancelEvent(CRegisteredString inCancelId) override
    {
        if (inCancelId.IsValid() == false)
            return;
        qCInfo(TRACE_INFO, "Cancel of id %s requested", inCancelId.c_str());
        for (QT3DSU32 idx = 0; idx < m_FutureEvents.size(); ++idx) {
            if (m_FutureEvents[idx].m_CancelId == inCancelId) {
                TFutureEventList::iterator iter = m_FutureEvents.begin() + idx;
                qCInfo(TRACE_INFO, "Cancelling event: %s, %s",
                    iter->m_Event->GetName().c_str(), iter->m_CancelId.c_str());
                m_FutureEvents.erase(iter);
                --idx;
            }
        }
    }

    TSignalConnectionPtr RegisterEventHandler(IStateInterpreterEventHandler &inHandler) override
    {
        SStateSignalSender *sender = QT3DS_NEW(m_Foundation.getAllocator(), SStateSignalSender)(
            m_Foundation.getAllocator(), *this, inHandler);
        m_SignalSenders.push_back(sender);
        return sender;
    }

    IScriptContext &GetScriptContext() override { return *m_ScriptContext; }

    IStateContext *GetStateContext() override { return m_Context; }

    debugger::IStateMachineDebugInterface &GetDebugInterface() override { return m_DebugInterface; }

    NVFoundationBase &GetFoundation() override { return m_Foundation; }
};

////////////////////////////////////////////////////////////////////////////////////
//  SDebugInterface implementation
////////////////////////////////////////////////////////////////////////////////////

void SDebugInterface::addRef()
{
    m_StateSystem.addRef();
}

void SDebugInterface::release()
{
    m_StateSystem.release();
}

struct SDebugStrOutStream : public IOutStream
{
    TDebugStr m_Str;
    bool Write(NVConstDataRef<QT3DSU8> data) override
    {
        m_Str.append((char8_t *)data.begin(), (char8_t *)data.end());
        return true;
    }
};

NVConstDataRef<TDebugStr> ListToRef(const nvvector<SStateNode *> &inList, TDebugStrList &outData)
{
    outData.resize(inList.size());
    for (QT3DSU32 idx = 0, end = inList.size(); idx < end; ++idx)
        outData[idx].assign(nonNull(inList[idx]->m_Id.c_str()));
    return toConstDataRef(outData.data(), outData.size());
}

IScriptContext &SDebugInterface::GetScriptContext()
{
    return *m_StateSystem.m_ScriptContext.mPtr;
}

// Functions implemented after StateSystem in order to take advantage of the StateSystem struct
// members directly.
void SDebugInterface::OnConnect()
{
    TDebugStr theFilename;
    SDebugStrOutStream theOutStream;
    if (m_StateSystem.m_Context->GetRoot())
        theFilename.assign(nonNull(m_StateSystem.m_Context->GetRoot()->m_Filename));
    CXMLIO::SaveSCXMLFile(*m_StateSystem.m_Context, m_StateSystem.m_Foundation,
                          *m_StateSystem.m_StringTable, theOutStream);
    m_Listener->OnConnect(theFilename, theOutStream.m_Str,
                          ListToRef(m_StateSystem.m_Configuration.m_List, m_EnterList));
}

// Functions should not be called in there is not a valid listener
void SDebugInterface::BeginStep()
{
    m_Listener->BeginStep();
}

void SDebugInterface::BeginMicrostep()
{
    m_Listener->BeginMicroStep();
}

void SDebugInterface::SetCurrentEvent(const char8_t *inEventName, bool inInternal)
{
    m_TempStr.assign(nonNull(inEventName));
    m_Listener->SetEvent(m_TempStr, inInternal);
}

STransitionId TransitionToId(const STransition &inTransition)
{
    SStateNode *parent(inTransition.m_Parent);
    STransitionId retval;
    if (parent == NULL) {
        QT3DS_ASSERT(false);
        return retval;
    }

    retval.m_StateId.assign(nonNull(parent->m_Id.c_str()));
    if (&inTransition == parent->GetInitialTransition())
        retval.m_TransitionIndex = -1;
    else if (parent->m_Type == StateNodeTypes::History) {
        SHistory &theHistory = static_cast<SHistory &>(*parent);
        if (&inTransition == theHistory.m_Transition)
            retval.m_TransitionIndex = -1;
    }

    if (retval.m_TransitionIndex == -2) {
        TStateNodeList *childList = parent->GetChildren();
        if (childList) {
            QT3DSI32 index = 0;
            for (TStateNodeList::iterator iter = childList->begin(), end = childList->end();
                 iter != end && retval.m_TransitionIndex == -2; ++iter, ++index) {
                SStateNode &theNode(*iter);
                if (theNode.m_Type == StateNodeTypes::Transition && &inTransition == (&theNode))
                    retval.m_TransitionIndex = index;
            }
        }
    }

    return retval;
}

void SDebugInterface::SetTransitionSet(const TTransitionList &inTransitions)
{
    m_TransitionList.resize(inTransitions.size());
    for (QT3DSU32 idx = 0, end = inTransitions.size(); idx < end; ++idx)
        m_TransitionList[idx] = TransitionToId(*inTransitions[idx]);
    m_Listener->SetTransitionSet(toDataRef(m_TransitionList.data(), m_TransitionList.size()));
}

void SDebugInterface::SetExitSet(const TStateNodeSet &inSet)
{
    NVConstDataRef<TDebugStr> theSet(ListToRef(inSet.m_List, this->m_ExitList));
    m_Listener->SetExitSet(theSet);
}
void SDebugInterface::SetEnterSet(const TStateNodeSet &inSet)
{
    NVConstDataRef<TDebugStr> theSet(ListToRef(inSet.m_List, this->m_EnterList));
    m_Listener->SetEnterSet(theSet);
}
// Log statements run through the debugger as well.
void SDebugInterface::EndMicrostep()
{
    m_Listener->EndMicroStep();
}
void SDebugInterface::EndStep()
{
    m_Listener->EndStep();
}

void SDebugInterface::OnExternalBreak()
{
    if (m_Listener)
        m_Listener->OnExternalBreak();
}

SStateSignalSender::~SStateSignalSender()
{
    if (m_System) {
        m_System->m_SignalSenders.erase(
            eastl::find(m_System->m_SignalSenders.begin(), m_System->m_SignalSenders.end(), this));
    }
}
}

IStateInterpreter &IStateInterpreter::Create(NVFoundationBase &inFnd, IStringTable &inStrTable,
                                             IScriptContext &inScriptContext,
                                             IExecutionContext &inExecutionContext)
{
    return *QT3DS_NEW(inFnd.getAllocator(), StateSystem)(inFnd, inStrTable, inScriptContext,
                                                      inExecutionContext);
}
