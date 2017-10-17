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
#include "Qt3DSLuaDebugger.h"
#include "Qt3DSLuaDebuggerProtocol.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSLuaDebuggerImpl.h"
#include "foundation/Qt3DSAtomic.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace qt3ds::foundation;
using namespace qt3ds::state::debugger;
using namespace qt3ds::state::debugger::lua;
using namespace qt3ds;

namespace {

struct STopScope
{
    lua_State *m_LuaState;
    int m_Top;
    STopScope(lua_State *inState)
        : m_LuaState(inState)
        , m_Top(lua_gettop(inState))
    {
    }
    ~STopScope() { lua_settop(m_LuaState, m_Top); }
};

struct SActiveBreakInfo
{
    enum Enum {
        None = 0,
        Step,
        StepInto,
    };
    Enum m_Reason;
    SFileAndLine m_PreviousBreak;
    SActiveBreakInfo()
        : m_Reason(None)
    {
    }
    SActiveBreakInfo(Enum reason, const SFileAndLine data)
        : m_Reason(reason)
        , m_PreviousBreak(data)
    {
    }
};

struct BreakTestTypes
{
    enum Enum {
        UnknownBreakTestType,
        BreakRequested,
        StepOut,
        StepOver,
    };
};

struct SBreakRequestedTest
{
};
struct SStepOutTest
{
    QT3DSI32 m_StackDepth;
    SStepOutTest()
        : m_StackDepth(0)
    {
    }
    SStepOutTest(QT3DSI32 sd)
        : m_StackDepth(sd)
    {
    }
};
struct SStepOverTest
{
    QT3DSI32 m_StackDepth;
    SFileAndLine m_Line;
    SStepOverTest()
        : m_StackDepth(0)
    {
    }
    SStepOverTest(QT3DSI32 sd, SFileAndLine line)
        : m_StackDepth(sd)
        , m_Line(line)
    {
    }
};

template <typename TDataType>
struct BreakTestTypeId
{
};

template <>
struct BreakTestTypeId<SBreakRequestedTest>
{
    static BreakTestTypes::Enum GetType() { return BreakTestTypes::BreakRequested; }
};
template <>
struct BreakTestTypeId<SStepOutTest>
{
    static BreakTestTypes::Enum GetType() { return BreakTestTypes::StepOut; }
};
template <>
struct BreakTestTypeId<SStepOverTest>
{
    static BreakTestTypes::Enum GetType() { return BreakTestTypes::StepOver; }
};
}
namespace qt3ds {
namespace foundation {
    template <>
    struct DestructTraits<SBreakRequestedTest>
    {
        static void destruct(SBreakRequestedTest &) {}
    };
    template <>
    struct DestructTraits<SStepOutTest>
    {
        static void destruct(SStepOutTest &) {}
    };
    template <>
    struct DestructTraits<SStepOverTest>
    {
        static void destruct(SStepOverTest &) {}
    };
}
}

namespace {

struct SBreakTestValueTraits
{
    typedef BreakTestTypes::Enum TIdType;
    enum {
        TBufferSize = sizeof(SStepOverTest),
    };

    static TIdType getNoDataId() { return BreakTestTypes::UnknownBreakTestType; }

    template <typename TDataType>
    static TIdType getType()
    {
        return BreakTestTypeId<TDataType>().GetType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case BreakTestTypes::BreakRequested:
            return inVisitor(*NVUnionCast<SBreakRequestedTest *>(inData));
        case BreakTestTypes::StepOut:
            return inVisitor(*NVUnionCast<SStepOutTest *>(inData));
        case BreakTestTypes::StepOver:
            return inVisitor(*NVUnionCast<SStepOverTest *>(inData));
        default:
            QT3DS_ASSERT(false);
        case BreakTestTypes::UnknownBreakTestType:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case BreakTestTypes::BreakRequested:
            return inVisitor(*NVUnionCast<const SBreakRequestedTest *>(inData));
        case BreakTestTypes::StepOut:
            return inVisitor(*NVUnionCast<const SStepOutTest *>(inData));
        case BreakTestTypes::StepOver:
            return inVisitor(*NVUnionCast<const SStepOverTest *>(inData));
        default:
            QT3DS_ASSERT(false);
        case BreakTestTypes::UnknownBreakTestType:
            return inVisitor();
        }
    }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<SBreakTestValueTraits,
                                                         SBreakTestValueTraits::TBufferSize>,
                       SBreakTestValueTraits::TBufferSize>
        TBreakTestUnionType;

struct SBreakTest : public TBreakTestUnionType
{
    typedef TBreakTestUnionType base;
    SBreakTest() {}
    SBreakTest(SBreakRequestedTest value)
        : base(value)
    {
    }
    SBreakTest(SStepOutTest value)
        : base(value)
    {
    }
    SBreakTest(SStepOverTest value)
        : base(value)
    {
    }
    SBreakTest(const SBreakTest &other)
        : base(static_cast<const base &>(other))
    {
    }
    SBreakTest &operator=(const SBreakTest &other)
    {
        base::operator=(other);
        return *this;
    }
};

const char *GetCachedEnvironmentName()
{
    return "lua_debugger_cached_environment";
}

struct SDataCacheUpdateWatcher
{
    lua_State &m_LuaState;
    ILuaRuntimeDataCache &m_DataCache;
    SDataCacheUpdateWatcher(lua_State &state, ILuaRuntimeDataCache &dc)
        : m_LuaState(state)
        , m_DataCache(dc)
    {
        m_DataCache.BeginUpdate();
    }
    ~SDataCacheUpdateWatcher()
    {
        m_DataCache.EndUpdate(m_LuaState);
        lua_pushnil(&m_LuaState);
        lua_setfield(&m_LuaState, LUA_REGISTRYINDEX, GetCachedEnvironmentName());
    }
};

struct SLuaInternalBreakpoint : public SLuaBreakpoint
{
    bool m_CompileError;
    SLuaInternalBreakpoint()
        : m_CompileError(false)
    {
    }
    SLuaInternalBreakpoint(const SLuaBreakpoint &other)
        : SLuaBreakpoint(other)
        , m_CompileError(false)
    {
    }
    SLuaInternalBreakpoint &operator=(const SLuaBreakpoint &other)
    {
        SLuaBreakpoint::operator=(other);
        m_CompileError = false;
        return *this;
    }
};

struct SWatchModifier
{
    eastl::string m_Str;

    eastl::string modify(const char *inStr)
    {
        m_Str.assign(inStr);

        eastl::string::size_type nonSpacePos = m_Str.find_first_not_of(" \n\r\t");
        // eatwhite
        if (nonSpacePos != eastl::string::npos && nonSpacePos > 0) {
            m_Str.erase(m_Str.begin(), m_Str.begin() + nonSpacePos);
        }

        if (m_Str.size()) {
            if (m_Str.find("return ")) {
                m_Str.insert(0, "return ");
            }
        }

        return m_Str;
    }
};

struct SConsoleModifier
{

    eastl::string m_Str;

    eastl::string modify(const char *inStr, int nPass = 1)
    {

        m_Str.assign(inStr);
        m_Str.ltrim();
        if (m_Str.size()) {
            if (nPass == 1) {
                if (m_Str[0] == '=') {
                    m_Str.erase(m_Str.begin(), m_Str.begin() + 1);
                    m_Str.ltrim();
                }

                eastl::replace(m_Str.begin(), m_Str.end(), '\n', ' ');
            } else {
                if (m_Str.find("return ")) {
                    m_Str.insert(0, "return ");
                }
            }
        }

        return m_Str;
    }
};

// Runtime side LuaDebugger client
// provides handlers for server commands from Architect
// Registers a debug hook that performs break test
struct SLuaSideDebugger : public ILuaDebugger, public IDebugStreamListener
{
    typedef eastl::hash_map<SFileAndLine, SLuaInternalBreakpoint, eastl::hash<SFileAndLine>,
                            eastl::equal_to<SFileAndLine>, ForwardingAllocator>
        TBreakpointMap;
    typedef eastl::vector<CRegisteredString, ForwardingAllocator> TWatchList;
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IDebugOutStream> m_Stream;
    lua_State *m_LuaState;
    IStringTable &m_StringTable;
    SLuaDebuggerProtocolWriter m_ProtocolWriter;
    CRegisteredString m_ApplicationDir;
    TBreakpointMap m_Breakpoints;
    lua::TBreakpointList m_BreakpointList;
    TWatchList m_Watches;
    Option<SActiveBreakInfo> m_ActiveBreakInfo;
    NVScopedRefCounted<ILuaRuntimeDataCache> m_DataCache;
    SStatementExecuteData m_StatementExecutionResult;

    SBreakTest m_BreakTest;

    TDebugStr m_BaseStr;
    TDebugStr m_RelativeStr;
    TDebugStr m_WorkStr;
    TDebugStr m_SrcStr;

    ILuaSideListener *m_Listener;
    bool m_IgnoreDebugHook;
    lua_State *m_BrokenState;

    QT3DSI32 mRefCount;

    struct SStackState
    {
        lua_State *&m_Value;
        SStackState(lua_State *&inValue)
            : m_Value(inValue)
        {
        }
        ~SStackState() { m_Value = NULL; }
    };

    struct SStackBool
    {
        bool &m_Value;
        SStackBool(bool &inValue)
            : m_Value(inValue)
        {
            m_Value = true;
        }
        ~SStackBool() { m_Value = false; }
    };

    SLuaSideDebugger(NVFoundationBase &fnd, IDebugOutStream &inStream, lua_State *inState,
                     IStringTable &inStrTable, const char *inApplicationDirectory,
                     ILuaSideListener *inListener)
        : m_Foundation(fnd)
        , m_Stream(inStream)
        , m_LuaState(inState)
        , m_StringTable(inStrTable)
        , m_ProtocolWriter(inStream, fnd.getAllocator())
        , m_ApplicationDir(m_StringTable.RegisterStr(inApplicationDirectory))
        , m_Breakpoints(ForwardingAllocator(fnd.getAllocator(), "LuaDebuggerBreakpoints"))
        , m_Watches(ForwardingAllocator(fnd.getAllocator(), "LuaDebuggerWatches"))
        , m_Listener(inListener)
        , m_IgnoreDebugHook(false)
        , m_BrokenState(NULL)
        , mRefCount(0)
    {
        lua_pushlightuserdata(m_LuaState, this);
        lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "uic_lua_debugger_impl");
        lua_sethook(m_LuaState, DebugHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
        m_BaseStr.assign(m_ApplicationDir.c_str());
        m_Stream->SetListener(this);
        m_DataCache = ILuaRuntimeDataCache::Create(fnd, inStrTable, inStream, ToClientId(),
                                                   inApplicationDirectory);
        WriteMessage(lua::SInitialization(CurrentDebuggerProtocolVersion()));
        // The client starts broken so we can update breakpoints and such before any lua code runs.
        SDataCacheUpdateWatcher watcher(*m_LuaState, *m_DataCache);
        HandleBroken(*inState, -1);
    }

    ~SLuaSideDebugger() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    //////////////////////////////////////////////////////////////////////////////////////
    //// IDebugStreamListener
    //////////////////////////////////////////////////////////////////////////////////////
    void OnMessageReceived(const SDebugStreamMessage &inMessage) override
    {
        SLuaDebuggerProtocolReader theReader(inMessage.m_Data, m_StringTable);
        while (theReader.HasData()) {
            SLuaDebugMessageHeader theHeader = theReader.ReadHeader();
            HandleNextMessage(theReader, theHeader);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //// Dispatch asynchronous messages
    //////////////////////////////////////////////////////////////////////////////////////
    void HandleNextMessage(SLuaDebuggerProtocolReader &inReader, SLuaDebugMessageHeader &inMessage)
    {
        if (m_DataCache->IsCacheMessage(inMessage)) {
            m_DataCache->HandleCacheMessage(m_BrokenState, inReader, inMessage);
        } else {
            switch (inMessage.m_MessageName) {
#define QT3DS_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(name)                                             \
    case SProtocolMessageNames::name:                                                              \
        Handle##name(inReader);                                                                    \
        break;
                QT3DS_LUA_DEBUGGER_ITERATE_PROTOCOL_MESSAGES
#undef QT3DS_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
    }

#define QT3DS_LUA_IGNORE_MESSAGE(name)                                                               \
    void Handle##name(SLuaDebuggerProtocolReader &) { QT3DS_ASSERT(false); }
    QT3DS_LUA_IGNORE_MESSAGE(Initialization)
    QT3DS_LUA_IGNORE_MESSAGE(AllBreakpoints)
    QT3DS_LUA_IGNORE_MESSAGE(Continue)
    QT3DS_LUA_IGNORE_MESSAGE(OnBreak)
    QT3DS_LUA_IGNORE_MESSAGE(OnClientConnect)
    QT3DS_LUA_IGNORE_MESSAGE(UpdateTable)
    QT3DS_LUA_IGNORE_MESSAGE(UpdateFunction)
    QT3DS_LUA_IGNORE_MESSAGE(RequestStackTrace)
    QT3DS_LUA_IGNORE_MESSAGE(StackTrace)
    QT3DS_LUA_IGNORE_MESSAGE(StepInto)
    QT3DS_LUA_IGNORE_MESSAGE(StepOver)
    QT3DS_LUA_IGNORE_MESSAGE(StepOut)
    QT3DS_LUA_IGNORE_MESSAGE(WatchUpdated)
    QT3DS_LUA_IGNORE_MESSAGE(StatementExecuted)
    QT3DS_LUA_IGNORE_MESSAGE(RequestPointerValue)
    QT3DS_LUA_IGNORE_MESSAGE(UnknownPointerValue)
    QT3DS_LUA_IGNORE_MESSAGE(EndPointerRequest)
    QT3DS_LUA_IGNORE_MESSAGE(BeginCacheUpdate)

    void HandleRequestBreak(SLuaDebuggerProtocolReader &)
    {
        m_BreakTest = SBreakTest(SBreakRequestedTest());
    }

    void HandleSetBreakpoint(SLuaDebuggerProtocolReader &inReader)
    {
        SSetBreakpoint message(inReader.ReadMessage<SSetBreakpoint>());
        Option<SLuaBreakpoint> absolute =
            ToAbsolute(message.m_Breakpoint.m_File.c_str(), message.m_Breakpoint.m_Line,
                       message.m_Breakpoint.m_Condition.c_str());
        if (absolute.hasValue()) {
            // insert a new breakpoint *or* override the existing breakpoint's member data
            // with new information.
            m_Breakpoints.insert(eastl::make_pair(*absolute, *absolute)).first->second = *absolute;
        }
    }

    void HandleRequestAllBreakpoints(SLuaDebuggerProtocolReader &inReader)
    {
        SRequestAllBreakpoints message(inReader.ReadMessage<SRequestAllBreakpoints>());
        m_BreakpointList.clear();
        m_BreakpointList.reserve(m_Breakpoints.size());
        for (TBreakpointMap::iterator iter = m_Breakpoints.begin(), end = m_Breakpoints.end();
             iter != end; ++iter) {
            m_BreakpointList.push_back(SLuaBreakpoint(ToRelative(iter->second.m_File.c_str()),
                                                      iter->second.m_Line,
                                                      iter->second.m_Condition));
        }
        SAllBreakpoints output(m_BreakpointList);
        WriteMessage(output);
    }

    void HandleClearBreakpoint(SLuaDebuggerProtocolReader &inReader)
    {
        SClearBreakpoint message(inReader.ReadMessage<SClearBreakpoint>());
        Option<SLuaBreakpoint> absolute =
            ToAbsolute(message.m_Breakpoint.m_File.c_str(), message.m_Breakpoint.m_Line);
        if (absolute.hasValue())
            m_Breakpoints.erase(absolute);
    }

    void HandleClearAllBreakpoints(SLuaDebuggerProtocolReader &inReader)
    {
        SClearAllBreakpoints message(inReader.ReadMessage<SClearAllBreakpoints>());
        m_Breakpoints.clear();
    }

    void HandleAddWatch(SLuaDebuggerProtocolReader &inReader)
    {
        SAddWatch message(inReader.ReadMessage<SAddWatch>());
        if (message.m_Expression.IsValid()) {
            TWatchList::iterator iter =
                eastl::find(m_Watches.begin(), m_Watches.end(), message.m_Expression);
            if (iter == m_Watches.end()) {
                m_Watches.push_back(message.m_Expression);
                if (m_BrokenState) {
                    EvaluateWatchExpression(*m_BrokenState, message.m_Expression);
                }
            }
        }
    }

    void HandleRemoveWatch(SLuaDebuggerProtocolReader &inReader)
    {
        SRemoveWatch message(inReader.ReadMessage<SRemoveWatch>());
        TWatchList::iterator iter =
            eastl::find(m_Watches.begin(), m_Watches.end(), message.m_Expression);
        if (iter != m_Watches.end())
            m_Watches.erase(iter);
    }

    void HandleExecuteStatement(SLuaDebuggerProtocolReader &inReader)
    {
        SExecuteStatement message(inReader.ReadMessage<SExecuteStatement>());
        if (m_BrokenState) {
            EvaluateConsoleExpression(*m_BrokenState, message.m_Expression);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //// hook
    //////////////////////////////////////////////////////////////////////////////////////

    static void DebugHook(lua_State *state, lua_Debug *debugInfo)
    {
        SLuaSideDebugger *theThis = GetThisFromLua(state);
        if (theThis && theThis->m_IgnoreDebugHook == false) {
            switch (debugInfo->event) {
            case LUA_HOOKLINE: {
                // The focus of debug hook is check for break
                theThis->CheckForBreak(state, debugInfo);
            } break;

            default:
                break;
            }
        }
    }

    // Do not use m_LuaState here.  when someone executes things that has a side effect
    // of creating a new lua state at times, and you need to use the lua state passed in
    // on the stack, on the m_LuaState variable on this object.
    void CheckForBreak(lua_State *state, lua_Debug * /*hookInfo*/)
    {
        lua_Debug stackInfo;
        memset(&stackInfo, 0, sizeof(stackInfo));
        if (lua_getstack(state, 0, &stackInfo)) {
            // Ensure the datacache is cleared when we return to normal execution as we don't
            // know what values change in the lua itself.
            // This also resets the cached environment table.
            SDataCacheUpdateWatcher cacheWatcher(*state, *m_DataCache);

            // Are the files and such local or absolute?
            // How does this work when things are loaded from the APK?
            lua_getinfo(state, "nSlu", &stackInfo);
            // The breakpoint check only works on source files
            // which always start with an @ symbol.
            TBreakpointMap::iterator iter = m_Breakpoints.end();
            Option<SLuaBreakpoint> theBreakpoint;
            if (stackInfo.currentline >= 0 && !isTrivial(stackInfo.source)
                && stackInfo.source[0] == '@') {
                theBreakpoint = ToAbsolute(stackInfo.source + 1, stackInfo.currentline);
                if (theBreakpoint.hasValue()) {
                    SLuaBreakpoint theKey(theBreakpoint->m_File, theBreakpoint->m_Line);
                    iter = m_Breakpoints.find(theKey);
                }
            }

            if (iter != m_Breakpoints.end()
                || m_BreakTest.getType() != BreakTestTypes::UnknownBreakTestType) {

                bool shouldDoBreak = true;

                if (iter == m_Breakpoints.end()) {
                    switch (m_BreakTest.getType()) {
                    case BreakTestTypes::BreakRequested:
                        break;
                    case BreakTestTypes::StepOut: {
                        QT3DSI32 stackDepth = GetStackDepth(*state);
                        SStepOutTest &theTest = *m_BreakTest.getDataPtr<SStepOutTest>();
                        // We have popped up the stack once
                        shouldDoBreak = theTest.m_StackDepth > stackDepth;
                    } break;
                    case BreakTestTypes::StepOver: {
                        SStepOverTest &theTest = *m_BreakTest.getDataPtr<SStepOverTest>();
                        QT3DSI32 stackDepth = GetStackDepth(*state);
                        SLuaBreakpoint current = GetCurrentLocation(*state);
                        // Stack depth is equal but we have moved around the file
                        // stack depth is less than (we have popped up).
                        shouldDoBreak = (stackDepth == theTest.m_StackDepth
                                         && (current.m_File == theTest.m_Line.m_File))
                            || theTest.m_StackDepth > stackDepth;
                    } break;
                    default:
                        QT3DS_ASSERT(false);
                        break;
                    }
                } else if (iter->second.m_Condition.IsValid()) {
                    Option<SDatamodelValue> theValue;
                    if (iter->second.m_CompileError == false)
                        theValue = ExecuteStringToValue(*state, iter->second.m_Condition.c_str(),
                                                        "Breakpoint");

                    if (theValue.hasValue())
                        shouldDoBreak = DatamodelValueToBoolean(*theValue);
                    else {
                        iter->second.m_CompileError = true;
                        shouldDoBreak = false;
                    }
                }

                // now break hits
                // update stack (variables), re-evaluate watches and send to server
                if (shouldDoBreak) {
                    QT3DSU64 curTime = qt3ds::foundation::Time::getCurrentCounterValue();
                    m_BrokenState = state;
                    SStackState __broken(m_BrokenState);
                    eastl::pair<QT3DSI32, SLuaBreakpoint> traceResult = PerformStackTrace(*state);
                    m_BreakTest = SBreakTest();
                    WriteMessage(lua::SOnBreak(traceResult.second));
                    UpdateWatchExpressions(*state);
                    QT3DSU64 updateTime = qt3ds::foundation::Time::getCurrentCounterValue() - curTime;
                    double milliseconds = updateTime / 100000.0;
                    qCInfo(TRACE_INFO, "Initial breakpoint update took: %fms\n", milliseconds);
                    HandleBroken(*state, traceResult.first, traceResult.second);
                    fflush(stdout); // flush stdout after handle broken message only
                }
            }
        }
    }

    // This blocks Runtime with blocking read: NextBrokenMessage()
    // Until Continue, Step Into, Step Over, Step Out is received
    void HandleBroken(lua_State &inState, QT3DSI32 inStackDepth,
                      SLuaBreakpoint inCurrentBreakpoint = SLuaBreakpoint())
    {
        if (m_Listener)
            m_Listener->OnBreak();
        m_BrokenState = &inState;
        SStackState __broken(m_BrokenState);
        bool broken = true;
        for (SDebugStreamMessage theMessage = NextBrokenMessage(broken);
             m_Stream->Connected() && theMessage.m_Data.size();
             theMessage = NextBrokenMessage(broken)) {
            SLuaDebuggerProtocolReader theReader(theMessage.m_Data, m_StringTable);
            while (theReader.HasData()) {
                SLuaDebugMessageHeader theHeader = theReader.ReadHeader();
                switch (theHeader.m_MessageName) {
                case SProtocolMessageNames::Continue:
                    broken = false;
                    break;
                case SProtocolMessageNames::StepInto:
                    broken = false;
                    m_BreakTest = SBreakTest(SBreakRequestedTest());
                    break;
                case SProtocolMessageNames::StepOut:
                    broken = false;
                    m_BreakTest = SBreakTest(SStepOutTest(GetStackDepth(inState, inStackDepth)));
                    break;
                case SProtocolMessageNames::StepOver:
                    broken = false;
                    m_BreakTest = SBreakTest(
                        SStepOverTest(GetStackDepth(inState, inStackDepth), inCurrentBreakpoint));
                    break;
                default:
                    HandleNextMessage(theReader, theHeader);
                }
            }
        }
    }

    SDebugStreamMessage NextBrokenMessage(bool broken)
    {
        if (broken)
            return m_Stream->WaitForNextMessage();
        return SDebugStreamMessage();
    }

    // update call stack, with non-detailed variables send back
    eastl::pair<QT3DSI32, SLuaBreakpoint> PerformStackTrace(lua_State &state)
    {
        // Here we go!!!
        lua_Debug stackInfo;
        // Kind of expensive operation.
        eastl::vector<STempStackInfo> stackData;
        SLuaBreakpoint currentPoint;

        for (QT3DSU32 idx = 0; idx < 100 && lua_getstack(&state, idx, &stackInfo)
             && lua_getinfo(&state, "Slnuf", &stackInfo);
             ++idx) {
            stackData.push_back(STempStackInfo());
            STempStackInfo &currentInfo(stackData.back());
            // Note the function is TOS
            currentInfo.m_Function = m_DataCache->CacheValue(state);
            currentInfo.m_CurrentLine = stackInfo.currentline;
            currentInfo.m_CallType = m_StringTable.RegisterStr(stackInfo.what);

            int localIndex = 1;
            const char *localName(NULL);

            while ((localName = lua_getupvalue(&state, -1, localIndex)) != NULL) {
                ++localIndex;
                if (!isTrivial(localName) && localName[0] != '(') {
                    currentInfo.m_UpVars.push_back(
                        STableEntry(localName, m_DataCache->CacheValue(state)));
                    lua_pop(&state, 1);
                }
            }

            // pop function off stack since we don't need it any more.
            lua_pop(&state, 1);

            localIndex = 1;
            while ((localName = lua_getlocal(&state, &stackInfo, localIndex)) != NULL) {
                ++localIndex;
                // avoid unnamed or internal variables.
                if (!isTrivial(localName) && localName[0] != '(') {
                    currentInfo.m_LocalVars.push_back(
                        STableEntry(localName, m_DataCache->CacheValue(state)));
                }
                lua_pop(&state, 1);
            }

            if (idx == 0) {
                if (!isTrivial(stackInfo.source) && stackInfo.source[0] == '@') {
                    currentPoint.m_File = ToRelative(stackInfo.source + 1);
                    currentPoint.m_Line = stackInfo.currentline;
                }
            }
        }
        eastl::vector<SLuaStackInfo> outgoingStackData;
        outgoingStackData.assign(stackData.begin(), stackData.end());
        SStackTraceData theStackData(
            toConstDataRef(outgoingStackData.data(), (QT3DSU32)outgoingStackData.size()));
        WriteMessage(lua::SStackTrace(theStackData));
        return eastl::make_pair((QT3DSI32)stackData.size(), currentPoint);
    }

    // re-evaluate watches and send back
    void UpdateWatchExpressions(lua_State &state)
    {
        for (TWatchList::iterator iter = m_Watches.begin(), end = m_Watches.end(); iter != end;
             ++iter) {
            EvaluateWatchExpression(state, *iter);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //// helper functions for calculation
    //////////////////////////////////////////////////////////////////////////////////////
    bool RestoreEnvironmentTable(lua_State &state)
    {
        int oldTop = lua_gettop(&state);
        (void)oldTop;
        bool bChange = false;
        lua_getfield(&state, LUA_REGISTRYINDEX, GetCachedEnvironmentName());
        if (lua_istable(&state, -1)) {
            lua_Debug stackInfo;
            if (lua_getstack(&state, 0, &stackInfo) && lua_getinfo(&state, "Slnuf", &stackInfo)) {
                // run through locals and such and update function environment if changed
                // upvars require the function to be TOS (top of stack).
                int localIndex = 1;
                const char *localName(NULL);
                while ((localName = lua_getupvalue(&state, -1, localIndex)) != NULL) {

                    if (!isTrivial(localName) && localName[0] != '(') {
                        lua_getfield(&state, -3, localName);
                        if (!lua_equal(&state, -1, -2)) {
                            lua_setupvalue(&state, -3, localIndex);
                            bChange = true;
                        } else
                            lua_pop(&state, 1);
                    }
                    ++localIndex;
                    lua_pop(&state, 1);
                }
                QT3DS_ASSERT(lua_type(&state, -1) == LUA_TFUNCTION);
                // pop function off the stack because we don't need it any more.
                lua_pop(&state, 1);

                localIndex = 1;
                while ((localName = lua_getlocal(&state, &stackInfo, localIndex)) != NULL) {
                    // avoid unnamed or internal variables.
                    if (!isTrivial(localName) && localName[0] != '(') {
                        lua_getfield(&state, -2, localName);
                        if (!lua_equal(&state, -1, -2)) {
                            lua_setlocal(&state, &stackInfo, localIndex);
                            bChange = true;
                        } else
                            lua_pop(&state, 1);
                    }
                    ++localIndex;
                    lua_pop(&state, 1);
                }
            }
        }

        lua_pop(&state, 1);
        QT3DS_ASSERT(lua_gettop(&state) == oldTop);
        return bChange;
    }

    bool BuildEnvironmentTable(lua_State &state)
    {
        int oldTop = lua_gettop(&state);
        (void)oldTop;
        lua_getfield(&state, LUA_REGISTRYINDEX, GetCachedEnvironmentName());
        if (lua_isnil(&state, -1)) {
            lua_pop(&state, 1);
            lua_Debug stackInfo;
            if (lua_getstack(&state, 0, &stackInfo) && lua_getinfo(&state, "Slnuf", &stackInfo)) {
                // create metatable for globals table
                lua_newtable(&state);
                // get the current function globals
                lua_getfenv(&state, -2);
                lua_setfield(&state, -2, "__index");
                // stack now should just have the new metatable on it.so push new table
                lua_newtable(&state);
                // reverse table order of new table and old table.
                lua_insert(&state, -2);
                // set the metatable.
                lua_setmetatable(&state, -2);

                // setup new print function
                lua_pushcfunction(&state, DebuggerPrint);
                lua_setfield(&state, -2, "print");

                // Bring the function to TOS
                lua_insert(&state, -2);
                QT3DS_ASSERT(lua_gettop(&state) == oldTop + 2);
                // Ensure it is what we think it should be.
                QT3DS_ASSERT(lua_type(&state, -1) == LUA_TFUNCTION);
                // ensure the table is what we think it is.
                QT3DS_ASSERT(lua_type(&state, -2) == LUA_TTABLE);

                // now run through locals and such and add them to new global table
                // upvars require the function to be TOS (top of stack).
                int localIndex = 1;
                const char *localName(NULL);
                while ((localName = lua_getupvalue(&state, -1, localIndex)) != NULL) {
                    ++localIndex;
                    if (!isTrivial(localName) && localName[0] != '(')
                        lua_setfield(&state, -3, localName);
                    else
                        lua_pop(&state, 1);
                }
                QT3DS_ASSERT(lua_type(&state, -1) == LUA_TFUNCTION);
                // pop function off the stack because we don't need it any more.
                lua_pop(&state, 1);

                localIndex = 1;
                while ((localName = lua_getlocal(&state, &stackInfo, localIndex)) != NULL) {
                    ++localIndex;
                    // avoid unnamed or internal variables.
                    if (!isTrivial(localName) && localName[0] != '(')
                        lua_setfield(&state, -2, localName);
                    else
                        lua_pop(&state, 1);
                }
                // OK, that should have built the global table and left nothing on the stack
                QT3DS_ASSERT(lua_type(&state, -1) == LUA_TTABLE);
                QT3DS_ASSERT(lua_gettop(&state) == oldTop + 1);
                // dup the table to store it.
                lua_pushvalue(&state, -1);
                lua_setfield(&state, LUA_REGISTRYINDEX, GetCachedEnvironmentName());
            } else
                return false;
        }
        QT3DS_ASSERT(lua_gettop(&state) == oldTop + 1);
        QT3DS_ASSERT(lua_type(&state, -1) == LUA_TTABLE);
        return true;
    }

    SStatementExecuteData ExecuteString(lua_State &state, const char *str, bool needNil = true)
    {
        // Build the TOS execution environment.
        STopScope theTopScope(&state);
        m_StatementExecutionResult = SStatementExecuteData();
        m_StatementExecutionResult.m_Expression = m_StringTable.RegisterStr(nonNull(str));
        if (BuildEnvironmentTable(state)) {
            SStackBool __ignoreHook(m_IgnoreDebugHook);

            int errorNum = luaL_loadstring(&state, str);
            if (errorNum) {
                const char *errorMessageFromLua = lua_tostring(&state, -1);
                m_StatementExecutionResult.m_Error.assign(errorMessageFromLua);
                return m_StatementExecutionResult;
            }
            lua_insert(&state, -2);
            QT3DS_ASSERT(lua_type(&state, -1) == LUA_TTABLE);
            lua_setfenv(&state, -2);
            QT3DS_ASSERT(lua_type(&state, -1) == LUA_TFUNCTION);
            int beforePCall = lua_gettop(&state);
            errorNum = lua_pcall(&state, 0, LUA_MULTRET, 0);
            int afterPCall = lua_gettop(&state);
            if (errorNum) {
                const char *errorMessageFromLua = lua_tostring(&state, -1);
                m_StatementExecutionResult.m_Error.assign(errorMessageFromLua);
                return m_StatementExecutionResult;
            }
            // Else, if no errors are present, then cache the value and return it.
            if (afterPCall >= beforePCall) {
                int numResults = afterPCall - beforePCall + 1;
                for (int idx = 0, end = numResults; idx < end; ++idx) {
                    int relativeIndex = idx - numResults;
                    m_StatementExecutionResult.m_Values.push_back(
                        m_DataCache->CacheValue(state, relativeIndex));
                }
            } else if (needNil) {
                m_StatementExecutionResult.m_Values.push_back(SDatamodelValue(SNil()));
            }
        }
        return m_StatementExecutionResult;
    }

    Option<SDatamodelValue> ExecuteStringToValue(lua_State &state, const char *str,
                                                 const char *errorHeader)
    {
        SStatementExecuteData theData = ExecuteString(state, SWatchModifier().modify(str).c_str());
        if (theData.m_Error.size()) {
            qCCritical(INVALID_OPERATION, "%s %s:%s", nonNull(errorHeader), nonNull(str),
                theData.m_Error.c_str());
        } else {
            if (theData.m_Values.size())
                return theData.m_Values[0];
            else
                return SDatamodelValue(SNil());
        }
        return Option<SDatamodelValue>();
    }

    void EvaluateWatchExpression(lua_State &state, CRegisteredString exp)
    {
        if (exp.IsValid()) {
            // We explicitly do not want watch expression errors coming out.
            Option<SDatamodelValue> retval = ExecuteStringToValue(state, exp.c_str(), NULL);
            SWatchUpdateData theData;
            theData.m_Expression = exp;
            if (retval.hasValue())
                theData.m_Value = *retval;
            WriteMessage(SWatchUpdated(theData));
        }
    }

    void EvaluateConsoleExpression(lua_State &state, CRegisteredString exp)
    {
        if (exp.IsValid()) {
            SConsoleModifier theModifier;
            eastl::string str = theModifier.modify(exp.c_str());

            SStatementExecuteData theData =
                ExecuteString(state, theModifier.modify(str.c_str(), 2).c_str(), false);

            if (theData.m_Error.size() || !theData.m_Values.size()) {
                // 2nd pass w/o 'return'
                theData = ExecuteString(state, str.c_str(), false);
            }

            WriteMessage(SStatementExecuted(theData));

            if (!theData.m_Error.size()) {
                // reflect new environment on palettes
                if (m_BrokenState && RestoreEnvironmentTable(*m_BrokenState)) {
                    eastl::pair<QT3DSI32, SLuaBreakpoint> traceResult =
                        PerformStackTrace(*m_BrokenState);
                    UpdateWatchExpressions(*m_BrokenState);
                    WriteMessage(lua::SOnBreak(traceResult.second));
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //// other functions
    //////////////////////////////////////////////////////////////////////////////////////
    QT3DSI32 GetStackDepth(lua_State &state, QT3DSI32 inStackDepth = -1)
    {
        if (inStackDepth != -1)
            return inStackDepth;

        lua_Debug stackInfo;
        QT3DSI32 retval = 0;
        for (QT3DSU32 idx = 0; idx < 100 && lua_getstack(&state, idx, &stackInfo)
             && lua_getinfo(&state, "Slnuf", &stackInfo);
             ++idx) {
            ++retval;
            // empty loop on purpose.
        }
        return retval;
    }

    SLuaBreakpoint GetCurrentLocation(lua_State &state)
    {
        lua_Debug stackInfo;
        SLuaBreakpoint retval;
        if (lua_getstack(&state, 0, &stackInfo) && lua_getinfo(&state, "Slnuf", &stackInfo)
            && !isTrivial(stackInfo.source) && stackInfo.source[0] == '@') {
            retval.m_File = ToRelative(stackInfo.source + 1);
            retval.m_Line = stackInfo.currentline;
        }
        return retval;
    }

    virtual void Disconnect()
    {
        UninstallDebugHook();
        m_Stream = NULL;
    }

    void UninstallDebugHook()
    {
        lua_sethook(m_LuaState, DebugHook, 0, 0);
        lua_pushlightuserdata(m_LuaState, NULL);
        lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "uic_lua_debugger_impl");
    }

    template <typename TMsgType>
    void WriteMessage(const TMsgType &msg)
    {
        if (m_Stream)
            m_ProtocolWriter.WriteMessage(ToClientId(), const_cast<TMsgType &>(msg));
    }

    QT3DSU64 ToClientId() const { return static_cast<QT3DSU64>(reinterpret_cast<size_t>(this)); }

    // Absolute/relative path conversion
    void ToPlatformPath(TDebugStr &outString)
    {
        for (TStr::size_type pos = outString.find('\\'); pos != TStr::npos;
             pos = outString.find('\\', pos + 1))
            outString.replace(outString.begin() + pos, outString.begin() + pos + 1, "/");
    }

    Option<SLuaBreakpoint> ToAbsolute(const SLuaBreakpoint &inBreakpoint)
    {
        if (inBreakpoint.m_File.IsValid()) {
            CFileTools::CombineBaseAndRelative(m_ApplicationDir, inBreakpoint.m_File, m_WorkStr);
            ToPlatformPath(m_WorkStr);
            SLuaBreakpoint result(m_StringTable.RegisterStr(m_WorkStr.c_str()), inBreakpoint.m_Line,
                                  inBreakpoint.m_Condition);
            return result;
        }
        return Empty();
    }

    CRegisteredString ToRelative(const char *inFilePath)
    {
        m_RelativeStr.assign(nonNull(inFilePath));
        CFileTools::GetRelativeFromBase(m_BaseStr, m_RelativeStr, m_WorkStr);
        return m_StringTable.RegisterStr(m_WorkStr.c_str());
    }

    Option<SLuaBreakpoint> ToAbsolute(const char *inFileRelativePath, int inLine,
                                      const char *inCondition = "")
    {
        return ToAbsolute(SLuaBreakpoint(m_StringTable.RegisterStr(nonNull(inFileRelativePath)),
                                         inLine, m_StringTable.RegisterStr(inCondition)));
    }

    static SLuaSideDebugger *GetThisFromLua(lua_State *state)
    {
        lua_getfield(state, LUA_REGISTRYINDEX, "uic_lua_debugger_impl");
        SLuaSideDebugger *retval = reinterpret_cast<SLuaSideDebugger *>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        return retval;
    }

    static int DebuggerPrint(lua_State *state)
    {
        lua_getfield(state, LUA_REGISTRYINDEX, "uic_lua_debugger_impl");
        SLuaSideDebugger &theDebugger =
            *reinterpret_cast<SLuaSideDebugger *>(lua_touserdata(state, -1));
        lua_pop(state, 1);
        int numArgs = lua_gettop(state);
        lua_getglobal(state, "tostring");
        eastl::string printResult;
        for (int i = 1; i <= numArgs; i++) {
            const char *s;
            lua_pushvalue(state, -1); /* function to be called */
            lua_pushvalue(state, i); /* value to print */
            lua_call(state, 1, 1);
            s = lua_tostring(state, -1); /* get result */
            if (s == NULL)
                return luaL_error(state,
                                  LUA_QL("tostring") " must return a string to " LUA_QL("print"));

            if (i > 1)
                printResult.append("\t");

            printResult.append(nonNull(s));
            lua_pop(state, 1); /* pop result */
        }
        if (printResult.empty() == false)
            theDebugger.m_StatementExecutionResult.m_PrintStatements.push_back(printResult);

        return 0;
    }

    static bool DatamodelValueToBoolean(const SDatamodelValue &value)
    {
        switch (value.getType()) {
        case DatamodelValueTypes::UnknownType:
            return false;
        case DatamodelValueTypes::Nil:
            return false;
        case DatamodelValueTypes::Boolean:
            return value.getData<bool>();
        case DatamodelValueTypes::Number:
            return value.getData<float>() != 0.0f;
        default:
            return true;
        }
    }
};
}

ILuaDebugger &ILuaDebugger::CreateLuaSideDebugger(NVFoundationBase &inFoundation,
                                                  IDebugOutStream &inStream, lua_State *inState,
                                                  IStringTable &inStrTable,
                                                  const char *inApplicationDirectory,
                                                  ILuaSideListener *inListener)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SLuaSideDebugger)(
        inFoundation, inStream, inState, inStrTable, inApplicationDirectory, inListener);
}
