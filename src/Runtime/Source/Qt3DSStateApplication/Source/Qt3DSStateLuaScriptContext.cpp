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
#include "Qt3DSStateLuaScriptContext.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Utils.h"
#include "foundation/StringTable.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "foundation/TrackingAllocator.h"
#include "Qt3DSStateExecutionContext.h"
#include "Qt3DSStateInterpreter.h"
#include "EASTL/string.h"
#include "EASTL/map.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "Qt3DSStateXMLIO.h"
#include "foundation/XML.h"
#include "foundation/IOStreams.h"
#include "Qt3DSStateDebugger.h"
#include "Qt3DSStateDebuggerValues.h"
#include "Qt3DSStateContext.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace qt3ds::state;
using namespace qt3ds::state::debugger;

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

struct SScriptContext;

struct SLuaScriptEvent : public IScriptEvent
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<SScriptContext> m_Context;
    QT3DSI32 m_Id;
    volatile QT3DSI32 mRefCount;

    static const char *StaticType() { return "Lua Script Event"; }
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    SLuaScriptEvent(SScriptContext &inContext, QT3DSI32 inId, CRegisteredString inName);
    virtual ~SLuaScriptEvent();

    CRegisteredString RegisterString(const char8_t *inStr);
    void GetEventTable();
    void GetDataTable();
    // new table is top of stack.
    // popped off by function.
    void SetDataTable();
    CRegisteredString GetName() const override;
    CRegisteredString GetEventType() const override;
    bool SetParam(CRegisteredString inName, const char8_t *inExpression) override;
    bool SetParamStr(CRegisteredString inName, const char8_t *inExpression) override;
    bool SetDataExpr(const char8_t *inExpression) override;
    bool SetDataStr(const char8_t *inStr) override;
};

struct SScriptContext : public ILuaScriptContext
{
    typedef nvhash_map<size_t, int, eastl::hash<size_t>, eastl::equal_to<size_t>> TStrIntMap;
    typedef eastl::map<TDebugStr, SDatamodelValue> TDataModelTable;
    typedef eastl::hash_map<SDatamodelTable *, TDataModelTable> TTableMap;
    typedef eastl::hash_set<SDatamodelTable *> TTableHashSet;

    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IStringTable> m_StringTable;
    QT3DSI32 mRefCount;
    lua_State *m_LuaState;
    const char8_t *m_CurPtr;
    const char8_t *m_EndPtr;
    // Index into the compiled scripts array for this str ptr.
    TStrIntMap m_CompiledCodeMap;
    typedef eastl::basic_string<char8_t, ForwardingAllocator> TStrType;
    TStrType m_Workspace;
    TStrType m_Script;
    TStrType m_ErrorString;
    TStrType m_Id;
    IStateInterpreter *m_Interpreter;
    char8_t m_ContextName[256];
    bool m_OwnsLuaState;
    TDebugStr m_TempDebugStr;
    TTableMap m_TableMap;
    // Set ensures we don't visit the same table twice during update tables.
    TTableHashSet m_VisitedTables;
    QT3DSI32 m_NextScriptId;

    static void *LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
    {
        SScriptContext *ctx = reinterpret_cast<SScriptContext *>(ud);
        if (nsize == 0) {
            if (ptr)
                ctx->m_Foundation.getAllocator().deallocate(ptr);
            return NULL;
        } else {
            if (nsize < osize && ptr)
                return ptr;

            void *newMem =
                ctx->m_Foundation.getAllocator().allocate(nsize, "lua memory", __FILE__, __LINE__);
            if (osize && ptr) {
                size_t copyAmt = NVMin(osize, nsize);
                memCopy(newMem, ptr, (QT3DSU32)copyAmt);
                ctx->m_Foundation.getAllocator().deallocate(ptr);
            }
            return newMem;
        }
    }

    static const char *LuaReader(lua_State * /*state*/, void *data, size_t *size)
    {
        SScriptContext *ctx = reinterpret_cast<SScriptContext *>(data);
        const char *retval = (const char *)ctx->m_CurPtr;
        *size = ctx->m_EndPtr - ctx->m_CurPtr;
        ctx->m_CurPtr = ctx->m_EndPtr;
        if (*size == 0)
            retval = NULL;
        return retval;
    }

    struct SNodeFinder
    {
        CRegisteredString m_Name;
        SNodeFinder(CRegisteredString nm)
            : m_Name(nm)
        {
        }
        bool operator()(const SStateNode *inNode) const { return inNode->m_Id == m_Name; }
    };

    bool In(const char8_t *inStateName)
    {
        NVConstDataRef<SStateNode *> nodes = m_Interpreter->GetConfiguration();
        CRegisteredString name = m_StringTable->RegisterStr(inStateName);
        SStateNode *const *theFind = eastl::find_if(nodes.begin(), nodes.end(), SNodeFinder(name));
        return theFind != nodes.end();
    }

    static int LuaIn(lua_State *state)
    {
        int numargs = lua_gettop(state);
        QT3DS_ASSERT(numargs >= 2);
        if (numargs >= 2) {
            void *ctx = lua_touserdata(state, 1);
            SScriptContext *context = reinterpret_cast<SScriptContext *>(ctx);
            if (context != NULL && context->m_Interpreter != NULL) {
                const char *stateName = lua_tostring(state, 2);
                bool findResult = context->In(stateName);
                lua_pushboolean(state, (int)findResult);
                return 1;
            }
        }
        return 0;
    }

    struct SScopedStackTop
    {
        lua_State *m_LuaState;
        int m_Top;
        SScopedStackTop(lua_State *inState)
            : m_LuaState(inState)
            , m_Top(lua_gettop(inState))
        {
        }
        ~SScopedStackTop() { lua_settop(m_LuaState, m_Top); }
    };

    void GetScriptCacheTable()
    {
        int top = lua_gettop(m_LuaState);
        GetGlobalTable();
        lua_getfield(m_LuaState, -1, "Qt3DSstate_scriptcache");
        if (lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1); // pop the nil
            lua_newtable(m_LuaState);
            lua_pushvalue(m_LuaState, -1); // dup the table
            // stack layout -
            // newtable,newtable,globaltable
            // store it in our global table.
            lua_setfield(m_LuaState, -3, "Qt3DSstate_scriptcache");
        }
        // stack layout - newtable, globaltable
        // pop the global table off now that we have the script cache table.
        lua_insert(m_LuaState, -2);
        lua_pop(m_LuaState, 1);
        QT3DS_ASSERT(lua_gettop(m_LuaState) == top + 1);
        (void)top;
    }

    void InitLuaState(int inGlobalTableIdx)
    {
        SScopedStackTop __initTop(m_LuaState);

        if (inGlobalTableIdx)
            lua_pushvalue(m_LuaState, inGlobalTableIdx);
        else {
            const char *initScript = "local P = {} \n"
                                     "setmetatable(P, {__index = _G})\n"
                                     "return P\n";
            luaL_loadstring(m_LuaState, initScript);
            int callResult = lua_pcall(m_LuaState, 0, 1, 0);
            if (callResult) {
                qCCritical(INTERNAL_ERROR, "Error initializing lua state: %s", lua_tostring(m_LuaState, -1));
            }
        }

        int theType = lua_type(m_LuaState, -1);
        (void)theType;

        // setup the global table
        lua_pushcfunction(m_LuaState, LuaIn);
        lua_setfield(m_LuaState, -2, "__InImpl");

        const char *inScript = "local inFunc = function(stateName)\n"
                               "    return __InImpl(uicstate_scriptcontext, stateName)\n"
                               "  end\n"
                               "return inFunc\n";
        luaL_loadstring(m_LuaState, inScript);
        int callResult = lua_pcall(m_LuaState, 0, 1, 0);
        if (callResult) {
            qCCritical(INTERNAL_ERROR, "Error initializing lua state: %s", lua_tostring(m_LuaState, -1));
        }
        QT3DS_ASSERT(lua_isfunction(m_LuaState, -1));
        // set the function wrapper to that script.
        lua_setfield(m_LuaState, -2, "In");
        lua_pushlightuserdata(m_LuaState, this);
        lua_setfield(m_LuaState, -2, "Qt3DSstate_scriptcontext");

        // Set the function environment for the lua table.
        int top = lua_gettop(m_LuaState);
        lua_getfield(m_LuaState, -1, "In");
        QT3DS_ASSERT(lua_isfunction(m_LuaState, -1));
        lua_pushvalue(m_LuaState, -2);
        QT3DS_ASSERT(lua_isfunction(m_LuaState, -2));
        QT3DS_ASSERT(lua_istable(m_LuaState, -1));
        // set the environment of the 'In' function to be the 'P' table.
        lua_setfenv(m_LuaState, -2);
        lua_pop(m_LuaState, 1);
        QT3DS_ASSERT(top == lua_gettop(m_LuaState));

        // store the global table somewhere we can get to it.  WE get the global table of all lua
        // contexts
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "Qt3DSstate_luacontexts");
        if (lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1);
            lua_newtable(m_LuaState);
            lua_pushvalue(m_LuaState, -1);
            lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "Qt3DSstate_luacontexts");
        }
        (void)top;
        // move the table of all lua contexts before the global table.
        lua_insert(m_LuaState, -2);
        // store the global table in the master table of all lua contexts
        sprintf(m_ContextName, "%p", (void *)this);
        lua_setfield(m_LuaState, -2, m_ContextName);
    }

    SScriptContext(NVFoundationBase &inFnd, IStringTable &inStrTable, lua_State *inState = NULL,
                   int inGlobalTableIdx = 0)
        : m_Foundation(inFnd)
        , m_StringTable(inStrTable)
        , mRefCount(0)
        , m_CurPtr(NULL)
        , m_EndPtr(NULL)
        , m_CompiledCodeMap(inFnd.getAllocator(), "SScriptContext::m_CompiledCodeMap")
        , m_Workspace(ForwardingAllocator(inFnd.getAllocator(), "SScriptContext::m_Workspace"))
        , m_Script(ForwardingAllocator(inFnd.getAllocator(), "SScriptContext::m_Script"))
        , m_ErrorString(ForwardingAllocator(inFnd.getAllocator(), "SScriptContext::m_ErrorString"))
        , m_Id(ForwardingAllocator(inFnd.getAllocator(), "SScriptContext::m_Id"))
        , m_Interpreter(NULL)
        , m_NextScriptId(1)
    {
        if (inState) {
            m_LuaState = inState;
            m_OwnsLuaState = false;
        } else {
            m_LuaState = lua_newstate(LuaAlloc, this);
            luaL_openlibs(m_LuaState);
            m_OwnsLuaState = true;
        }
        InitLuaState(inGlobalTableIdx);
    }

    ~SScriptContext()
    {
        if (m_LuaState && m_OwnsLuaState)
            lua_close(m_LuaState);
        m_LuaState = NULL;
    }

    void SetId(const char8_t *inId) { m_Id.assign(nonNull(inId)); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void GetGlobalTable(lua_State *inState)
    {
        lua_getfield(inState, LUA_REGISTRYINDEX, "Qt3DSstate_luacontexts");
        lua_getfield(inState, -1, m_ContextName);
        lua_insert(inState, -2);
        // pop the registery table off the system.
        lua_pop(inState, 1);
    }
    // get the global table and leave it on top ofthe stack.
    void GetGlobalTable() override { GetGlobalTable(m_LuaState); }

    // Compile a script.  If returns true, compiled script is top of stack
    bool CompileScript(const char8_t *inScript, bool withRet = true)
    {
        if (inScript && *inScript) {
            m_Script.assign(inScript);
            if (withRet) {
                if (m_Script.find("return ") != 0)
                    m_Script.insert(0, "return ");
            }

            int top = lua_gettop(m_LuaState);
            int loadResult = luaL_loadstring(m_LuaState, m_Script.c_str());
            top = lua_gettop(m_LuaState);
            if (loadResult != 0) {
                m_ErrorString.assign("Failed to compile ");
                m_ErrorString.append(inScript);
                m_ErrorString.append(": ");
                m_ErrorString.append(lua_tostring(m_LuaState, -1));
                lua_pop(m_LuaState, 1);
                qCCritical(INVALID_OPERATION, "%s", m_ErrorString.c_str());
                return false;
            }
            return true;
        }
        return false;
    }

    bool ExecutePCall(const char8_t *inCond, int inArgCount, int inResultCount)
    {
        GetGlobalTable();
        lua_istable(m_LuaState, -1);
        lua_setfenv(m_LuaState, -2);
        int pcallResult = lua_pcall(m_LuaState, inArgCount, inResultCount, 0);
        if (pcallResult != 0) {
            const char *errorStr = lua_tostring(m_LuaState, -1);
            m_ErrorString.assign("Failure during execution of \"");
            m_ErrorString.append(inCond);
            m_ErrorString.append("\", ");
            m_ErrorString.append(errorStr);
            qCCritical(INVALID_OPERATION, "%s", m_ErrorString.c_str());
            return false;
        }
        return true;
    }

    void ExecuteCall(int inArgCount, int inResultCount)
    {
        GetGlobalTable();
        lua_istable(m_LuaState, -1);
        lua_setfenv(m_LuaState, -2);
        lua_call(m_LuaState, inArgCount, inResultCount);
    }

    int ExecuteStr(const char8_t *inCond, bool withRet) override
    {
        int originalTop = lua_gettop(m_LuaState);
        if (GetOrCacheScript(inCond, withRet) && ExecutePCall(inCond, 0, 1))
            return lua_gettop(m_LuaState) - originalTop;

        return 0;
    }

    // Should functions returning options return othing
    Option<bool> ExecuteCondition(const char8_t *inCond) override
    {
        if (inCond && *inCond) {
            int stackSize = ExecuteStr(inCond, true);
            if (stackSize) {
                int boolval = lua_toboolean(m_LuaState, -1);
                if (stackSize > 0)
                    lua_pop(m_LuaState, stackSize);
                return boolval ? true : false;
            }
        }
        return Empty();
    }

    // Used for logging.
    SScriptExecutionResult ExecuteExpressionToString(const char8_t *inExpr) override
    {
        if (inExpr && *inExpr) {
            int stackSize = ExecuteStr(inExpr, true);
            if (stackSize) {
                size_t strLen = 0;
                const char8_t *str = lua_tolstring(m_LuaState, -1, &strLen);
                if (!str)
                    str = "";
                m_Workspace.assign(str);
                lua_pop(m_LuaState, stackSize);
                return SScriptExecutionResult(m_Workspace.c_str(), "");
            }
            return SScriptExecutionResult("", m_ErrorString.c_str());
        }
        return SScriptExecutionResult("", "no expression to execute");
    }

    // If return value is false, error is signaled with GetErrorInfo.
    bool Assign(const char8_t *inVariable, const char8_t *inExpr) override
    {
        int top, newtop;
        top = lua_gettop(m_LuaState);
        int stacksize = ExecuteStr(inExpr, true);
        if (stacksize) {
            GetGlobalTable();
            lua_insert(m_LuaState, -2);
            lua_setfield(m_LuaState, -2, inVariable);
            lua_pop(m_LuaState, 1);
            newtop = lua_gettop(m_LuaState);
            QT3DS_ASSERT(newtop == top);
            return true;
        } else
            return false;
    }

    void AssignStr(const char8_t *inVariable, const char8_t *inStr) override
    {
        if (isTrivial(inVariable)) {
            QT3DS_ASSERT(false);
            return;
        }
        STopScope __stack(m_LuaState);
        GetGlobalTable();
        lua_pushstring(m_LuaState, inStr);
        lua_setfield(m_LuaState, -2, inVariable);
    }

    bool GetOrCacheScript(const char8_t *inScript, bool withRet)
    {
        TStrIntMap::iterator iter = m_CompiledCodeMap.find(reinterpret_cast<size_t>(inScript));
        if (iter == m_CompiledCodeMap.end()) {
            bool compilationSuccess = CompileScript(inScript, withRet);
            if (compilationSuccess == false) {
                m_CompiledCodeMap.insert(eastl::make_pair(reinterpret_cast<size_t>(inScript), -1));
                return false;
            }
            GetScriptCacheTable();
            int numScripts = (int)lua_objlen(m_LuaState, -1) + 1;
            // Copy the executable to the top of the stack
            QT3DS_ASSERT(lua_isfunction(m_LuaState, -2));
            QT3DS_ASSERT(lua_istable(m_LuaState, -1));
            lua_pushvalue(m_LuaState, -2);
            lua_rawseti(m_LuaState, -2, numScripts);
            QT3DS_ASSERT(lua_istable(m_LuaState, -1));
            m_CompiledCodeMap.insert(
                eastl::make_pair(reinterpret_cast<size_t>(inScript), numScripts));
            // Pop the scriptcache table off the stack leaving only the executable
            lua_rawgeti(m_LuaState, -1, numScripts);
            QT3DS_ASSERT(lua_isfunction(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
            lua_pop(m_LuaState, 1);
        } else {
            // We already failed to compile this script.
            if (iter->second < 0)
                return false;

            GetScriptCacheTable();
            lua_rawgeti(m_LuaState, -1, iter->second);
            QT3DS_ASSERT(lua_isfunction(m_LuaState, -1));
            // Move the compiled script *below* the table
            lua_insert(m_LuaState, -2);
            // now pop the table, leaving only the compiled script in its place.
            lua_pop(m_LuaState, 1);
        }
        return true;
    }

    bool ExecuteScript(const char8_t *inScript) override
    {
        if (GetOrCacheScript(inScript, false))
            return ExecutePCall(inScript, 0, 0);

        return false;
    }

    void CleanupForeach()
    {
        // Pop the table off the stack;
        // the lua_next would have popped the key already.
        lua_pop(m_LuaState, 1);
    }

    // Expect a key, then value.
    // Leaves the key on the stack.
    void AssignForEach(const char8_t *inItem, const char8_t *inIdxVar)
    {
        int top = lua_gettop(m_LuaState);
        GetGlobalTable();
        // put the global table underneath everything
        lua_insert(m_LuaState, -3);
        // Set the value into the table, popping it off the stack
        lua_setfield(m_LuaState, -3, inItem);
        if (!isTrivial(inIdxVar)) {
            // duplicate the key reference, we have to leave the key on the stack
            lua_pushvalue(m_LuaState, -1);
            lua_setfield(m_LuaState, -3, inIdxVar);
        }
        // push the key that is now on top of the stack under the table.
        lua_insert(m_LuaState, -2);
        lua_pop(m_LuaState, 1);
        QT3DS_ASSERT(lua_gettop(m_LuaState) == top - 1);
        (void)top;
    }

    Option<bool> BeginForeach(const char8_t *inArray, const char8_t *inItem,
                                      const char8_t *inIdxVar = "") override
    {
        if (isTrivial(inItem) || isTrivial(inArray))
            return false;
        GetGlobalTable();
        lua_getfield(m_LuaState, -1, inArray);
        lua_insert(m_LuaState, -2);
        // Pop the global table off the stack.
        lua_pop(m_LuaState, 1);

        if (lua_istable(m_LuaState, -1) == 0) {
            lua_pop(m_LuaState, 1);
            return Empty();
        }

        lua_pushnil(m_LuaState);

        int success = lua_next(m_LuaState, -2);

        if (!success) {
            CleanupForeach();
            return false;
        }

        AssignForEach(inItem, inIdxVar);

        return true;
    }

    Option<bool> NextForeach(const char8_t *inItem, const char8_t *inIdxVar = "") override
    {
        if (lua_istable(m_LuaState, -2) == false) {
            QT3DS_ASSERT(false);
            return Empty();
        }
        int success = lua_next(m_LuaState, -2);

        if (!success) {
            CleanupForeach();
            return false;
        }

        AssignForEach(inItem, inIdxVar);

        return true;
    }

    void CancelForeach() override
    {
        // Called when something caused an error and we should cancel the foreach loop
        // We should have a key and table on the stack.
        lua_pop(m_LuaState, 2);
    }

    void SetCurrentEvent(TEventPtr inEvent) override
    {
        STopScope __stackScope(m_LuaState);
        if (!inEvent) {
            QT3DS_ASSERT(false);
            return;
        }
        if (AreEqual(inEvent->GetEventType().c_str(), SLuaScriptEvent::StaticType())) {
            SLuaScriptEvent &theEvent = static_cast<SLuaScriptEvent &>(*inEvent.mPtr);
            theEvent.GetEventTable();
        } else {
            lua_newtable(m_LuaState);
            lua_pushstring(m_LuaState, inEvent->GetName().c_str());
            lua_setfield(m_LuaState, -2, "name");
        }
        QT3DS_ASSERT(lua_istable(m_LuaState, -1));
        // End result is that the event table is top of stack
        GetGlobalTable();
        lua_insert(m_LuaState, -2);
        lua_setfield(m_LuaState, -2, "_event");
    }
    void ClearCurrentEvent() override
    {
        STopScope __stackScope(m_LuaState);
        GetGlobalTable();
        lua_pushnil(m_LuaState);
        lua_setfield(m_LuaState, -2, "_event");
    }
    // postcondition is that uicstate_scriptids[retval] == nil
    QT3DSI32 FindUnusedScriptId()
    {
        STopScope __stackScope(m_LuaState);
        GetGlobalTable();
        lua_getfield(m_LuaState, -1, "Qt3DSstate_scriptids");
        if (lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1);
            lua_newtable(m_LuaState);
            lua_pushvalue(m_LuaState, -1);
            lua_setfield(m_LuaState, -3, "Qt3DSstate_scriptids");
        }
        lua_rawgeti(m_LuaState, -1, m_NextScriptId);
        while (!lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1);
            ++m_NextScriptId;
            lua_rawgeti(m_LuaState, -1, m_NextScriptId);
        }
        QT3DSI32 retval = m_NextScriptId;
        ++m_NextScriptId;
        return retval;
    }

    // Item is expected to be at top of stack
    // End result is that the item is still stop of stack (we don't pop the item)
    void SetScriptIdItem(QT3DSI32 inId)
    {
        STopScope __stackScope(m_LuaState);
        int topOfStack = lua_gettop(m_LuaState);
        GetGlobalTable();
        lua_getfield(m_LuaState, -1, "Qt3DSstate_scriptids");
        if (!lua_istable(m_LuaState, -1)) {
            QT3DS_ASSERT(false);
            return;
        }
        lua_pushvalue(m_LuaState, topOfStack);
        lua_rawseti(m_LuaState, -2, inId);
    }

    // Returns with the script id item at the top of the stack
    void GetScriptIdItem(QT3DSI32 inId)
    {
        GetGlobalTable();
        lua_getfield(m_LuaState, -1, "Qt3DSstate_scriptids");
        if (!lua_istable(m_LuaState, -1)) {
            QT3DS_ASSERT(false);
            lua_pop(m_LuaState, 2);
            return;
        }
        lua_rawgeti(m_LuaState, -1, inId);
    }

    // postcondition is that uicstate_scriptids[inId] == nil
    void ReleaseScriptIdItem(QT3DSI32 inId)
    {
        STopScope __stackScope(m_LuaState);
        GetGlobalTable();
        lua_getfield(m_LuaState, -1, "Qt3DSstate_scriptids");
        if (!lua_isnil(m_LuaState, -1)) {
            lua_pushnil(m_LuaState);
            lua_rawseti(m_LuaState, -2, inId);
        }
    }
    // Create an event we can attach extra data do.
    IScriptEvent *CreateScriptEvent(CRegisteredString inName) override
    {
        return QT3DS_NEW(m_Foundation.getAllocator(), SLuaScriptEvent)(*this, FindUnusedScriptId(),
                                                                    inName);
    }

    void SetInterpreter(IStateInterpreter &inInterpreter) override
    {
        m_Interpreter = &inInterpreter;
    }

    const char8_t *GetErrorInfo() override { return m_ErrorString.c_str(); }

    void DumpGlobalState() override
    {
        // Get our table onto the stack.
        GetGlobalTable();
        qCInfo(TRACE_INFO, "Begin global state dump");
        lua_pushnil(m_LuaState);
        while (lua_next(m_LuaState, -2) != 0) {
            // duplicate the key so that we don't mess it up.
            const char *keyType = lua_typename(m_LuaState, lua_type(m_LuaState, -2));
            const char *valType = lua_typename(m_LuaState, lua_type(m_LuaState, -1));
            lua_pushvalue(m_LuaState, -2);
            const char *keyVal = lua_tostring(m_LuaState, lua_gettop(m_LuaState));
            const char *valVal = lua_tostring(m_LuaState, lua_gettop(m_LuaState) - 1);
            qCInfo(TRACE_INFO, "%s -> %s, %s -> %s", keyType, valType, keyVal, valVal);
            lua_pop(m_LuaState, 2);
        }
        // pop the global table from the stack.
        lua_pop(m_LuaState, 1);
        qCInfo(TRACE_INFO, "End global state dump");
    }

    CRegisteredString GetContextType() override
    {
        return m_StringTable->RegisterStr(ILuaScriptContext::GetLuaContextTypeName());
    }

    void OnTableValue(SDatamodelTable *inTable, const char *inKey, int inValueIndex,
                      IScriptStateListener &inListener)
    {
        m_TempDebugStr.assign(nonNull(inKey));

        SDatamodelValue theValue;
        switch (lua_type(m_LuaState, inValueIndex)) {
        case LUA_TNIL:
            theValue = SNil();
            break;
        case LUA_TNUMBER:
            theValue = lua_tonumber(m_LuaState, inValueIndex);
            break;
        case LUA_TBOOLEAN:
            theValue = lua_toboolean(m_LuaState, inValueIndex) ? true : false;
            break;
        case LUA_TSTRING:
            theValue = TDebugStr(lua_tostring(m_LuaState, inValueIndex));
            break;
        case LUA_TTABLE:
            theValue = (SDatamodelTable *)lua_topointer(m_LuaState, inValueIndex);
            break;
        case LUA_TFUNCTION:
            theValue = (SDatamodelFunction *)lua_topointer(m_LuaState, inValueIndex);
            break;
        case LUA_TUSERDATA:
            theValue = (SDatamodelUserData *)lua_touserdata(m_LuaState, inValueIndex);
            break;
        case LUA_TTHREAD:
            break;
        case LUA_TLIGHTUSERDATA:
            theValue = (SDatamodelUserData *)lua_touserdata(m_LuaState, inValueIndex);
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
        TDataModelTable &theTable =
            m_TableMap.insert(eastl::make_pair(inTable, TDataModelTable())).first->second;
        TDataModelTable::iterator iter = theTable.find(m_TempDebugStr);
        if (iter == theTable.end() || iter->second != theValue) {
            bool comparison = false;
            if (iter != theTable.end()) {
                comparison = iter->second != theValue;
            }
            (void)comparison;
            inListener.SetKey(inTable, m_TempDebugStr, theValue);
            theTable[m_TempDebugStr] = theValue;
        }
    }

    // Table to dump is expected to be on the top of the stack.
    void DumpTable(IScriptStateListener &inListener, SDatamodelTable *inTable)
    {
        if (m_VisitedTables.find(inTable) != m_VisitedTables.end())
            return;

        STopScope __topScope(m_LuaState);
        m_VisitedTables.insert(inTable);
        lua_pushnil(m_LuaState);
        while (lua_next(m_LuaState, -2) != 0) {
            if (lua_type(m_LuaState, -1) == LUA_TTABLE)
                DumpTable(inListener, (SDatamodelTable *)lua_topointer(m_LuaState, -1));
            // dup the key so the tostring function doesn't change the actual table key type
            lua_pushvalue(m_LuaState, -2);
            const char *keyVal = lua_tostring(m_LuaState, -1);
            OnTableValue(inTable, keyVal, -2, inListener);
            lua_pop(m_LuaState, 2);
        }
    }

    void DumpState(IScriptStateListener &inListener) override
    {
        STopScope __topScope(m_LuaState);
        m_VisitedTables.clear();
        GetGlobalTable();
        DumpTable(inListener, NULL);
    }
};

SLuaScriptEvent::SLuaScriptEvent(SScriptContext &inContext, QT3DSI32 inId, CRegisteredString inName)
    : m_Foundation(inContext.m_Foundation)
    , m_Context(inContext)
    , m_Id(inId)
    , mRefCount(0)
{
    STopScope __stackScope(inContext.m_LuaState);
    lua_newtable(inContext.m_LuaState);
    // set name value
    lua_pushstring(m_Context->m_LuaState, inName.c_str());
    lua_setfield(m_Context->m_LuaState, -2, "name");
    // Ensure the event table is stored.
    m_Context->SetScriptIdItem(m_Id);
    lua_pop(inContext.m_LuaState, 1);
}

SLuaScriptEvent::~SLuaScriptEvent()
{
    m_Context->ReleaseScriptIdItem(m_Id);
}

CRegisteredString SLuaScriptEvent::RegisterString(const char8_t *inStr)
{
    return m_Context->m_StringTable->RegisterStr(inStr);
}

void SLuaScriptEvent::GetEventTable()
{
    m_Context->GetScriptIdItem(m_Id);
}

void SLuaScriptEvent::GetDataTable()
{
    m_Context->GetScriptIdItem(m_Id);
    lua_getfield(m_Context->m_LuaState, -1, "data");
    if (!lua_istable(m_Context->m_LuaState, -1)) {
        lua_pop(m_Context->m_LuaState, 1);
        // set data table
        lua_newtable(m_Context->m_LuaState);
        lua_setfield(m_Context->m_LuaState, -2, "data");
        lua_getfield(m_Context->m_LuaState, -1, "data");
    }
}

void SLuaScriptEvent::SetDataTable()
{
    {
        STopScope __topScope(m_Context->m_LuaState);
        // Assuming new table is top of stack.
        m_Context->GetScriptIdItem(m_Id);
        lua_pushvalue(m_Context->m_LuaState, __topScope.m_Top);
        lua_setfield(m_Context->m_LuaState, -2, "data");
    }
    // pop the argument off the stack.
    lua_pop(m_Context->m_LuaState, 1);
}

CRegisteredString SLuaScriptEvent::GetName() const
{
    STopScope __stackScope(m_Context->m_LuaState);
    const_cast<SLuaScriptEvent &>(*this).GetEventTable();
    lua_getfield(m_Context->m_LuaState, -1, "name");
    const char *theStr = lua_tostring(m_Context->m_LuaState, -1);
    return const_cast<SLuaScriptEvent &>(*this).RegisterString(theStr);
}

CRegisteredString SLuaScriptEvent::GetEventType() const
{
    return const_cast<SLuaScriptEvent &>(*this).RegisterString(SLuaScriptEvent::StaticType());
}

bool SLuaScriptEvent::SetParam(CRegisteredString inName, const char8_t *inExpression)
{
    STopScope __stackScope(m_Context->m_LuaState);
    GetDataTable();
    int stackIncrement = m_Context->ExecuteStr(inExpression, true);
    if (stackIncrement) {
        lua_setfield(m_Context->m_LuaState, -2, inName.c_str());
        return true;
    } else {
        return false;
    }
}

bool SLuaScriptEvent::SetParamStr(CRegisteredString inName, const char8_t *inStr)
{
    STopScope __stackScope(m_Context->m_LuaState);
    GetDataTable();
    lua_pushstring(m_Context->m_LuaState, nonNull(inStr));
    lua_setfield(m_Context->m_LuaState, -2, inName.c_str());
    return true;
}

bool SLuaScriptEvent::SetDataExpr(const char8_t *inExpression)
{
    STopScope __stackScope(m_Context->m_LuaState);
    GetEventTable();
    int stackIncrement = m_Context->ExecuteStr(inExpression, true);
    if (stackIncrement) {
        lua_setfield(m_Context->m_LuaState, -2, "data");
        return true;
    } else {
        return false;
    }
}

bool SLuaScriptEvent::SetDataStr(const char8_t *inStr)
{
    STopScope __stackScope(m_Context->m_LuaState);
    GetEventTable();
    lua_pushstring(m_Context->m_LuaState, inStr);
    lua_setfield(m_Context->m_LuaState, -2, "data");
    return true;
}

struct SFoundationLogger : public IStateLogger
{
    NVFoundationBase &m_Foundation;
    SFoundationLogger(NVFoundationBase &inFnd)
        : m_Foundation(inFnd)
    {
    }
    void addRef() override {}
    void release() override {}
    void Log(const char8_t *inLabel, const char8_t *inExpression) override
    {
        if (inLabel && inExpression)
            qCInfo(TRACE_INFO, "%s - %s", inLabel, inExpression);
    }
};

struct SLuaInterpreterObject : public IStateInterpreterEventHandler
{
    CAllocator allocator;
    NVScopedRefCounted<NVFoundation> __foundation;
    NVFoundationBase *m_ExternalFoundation;
    SFoundationLogger m_Logger;
    SSAutoDeallocatorAllocator graphAllocator;
    NVScopedRefCounted<IStateContext> m_StateContext;
    NVScopedRefCounted<debugger::IDebugger> m_Debugger;
    NVScopedRefCounted<SScriptContext> m_ScriptContext;
    NVScopedRefCounted<IStateInterpreter> m_Interpreter;
    TSignalConnectionPtr m_EventConnection;
    eastl::string m_ErrorString;

    NVFoundationBase &GetFoundation()
    {
        if (m_ExternalFoundation)
            return *m_ExternalFoundation;
        return *__foundation;
    }

    SLuaInterpreterObject(debugger::IDebugger *inDebugger, NVFoundationBase *inFoundation)
        : allocator()
        , __foundation(NVCreateFoundation(QT3DS_FOUNDATION_VERSION, allocator))
        , m_ExternalFoundation(inFoundation)
        , m_Logger(GetFoundation())
        , graphAllocator(GetFoundation())
        , m_Debugger(inDebugger)
    {
    }

    ~SLuaInterpreterObject()
    {
        if (m_Debugger)
            m_Debugger->Disconnect(m_Interpreter->GetDebugInterface());
    }

    void OnInterpreterEvent(InterpreterEventTypes::Enum inEvent,
                                    CRegisteredString inEventId) override
    {
        const char *functionName = "";
        switch (inEvent) {
        case InterpreterEventTypes::StateEnter:
            functionName = "onAfterEnter";
            break;
        case InterpreterEventTypes::StateExit:
            functionName = "onBeforeExit";
            break;
        case InterpreterEventTypes::Transition:
            return;
        default:
            QT3DS_ASSERT(false);
            return;
        }
        int top = lua_gettop(m_ScriptContext->m_LuaState);
        m_ScriptContext->GetGlobalTable();
        lua_getfield(m_ScriptContext->m_LuaState, -1, "Qt3DSstate_interpreterobject");
        QT3DS_ASSERT(lua_istable(m_ScriptContext->m_LuaState, -1));
        lua_getfield(m_ScriptContext->m_LuaState, -1, functionName);
        if (lua_isfunction(m_ScriptContext->m_LuaState, -1)) {
            lua_pushstring(m_ScriptContext->m_LuaState, inEventId.c_str());
            int success = lua_pcall(m_ScriptContext->m_LuaState, 1, 0, 0);
            if (success != 0) {
                const char *errorStr = lua_tostring(m_ScriptContext->m_LuaState, -1);
                m_ErrorString.assign("Failure during execution of \"");
                m_ErrorString.append(functionName);
                m_ErrorString.append("\", ");
                m_ErrorString.append(errorStr);
                qCCritical(INVALID_OPERATION, m_ErrorString.c_str());
                QT3DS_ASSERT(false);
            }
        }
        lua_settop(m_ScriptContext->m_LuaState, top);
    }

    static int OnGC(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TUSERDATA);
        SLuaInterpreterObject *theInterpreter = (SLuaInterpreterObject *)lua_touserdata(inState, 1);
        theInterpreter->~SLuaInterpreterObject();
        return 0;
    }

    static SLuaInterpreterObject *GetObjectFromTable(lua_State *inState, int inTableIdx)
    {
        lua_getfield(inState, inTableIdx, "__implementation");
        luaL_checktype(inState, -1, LUA_TUSERDATA);
        SLuaInterpreterObject *retval = (SLuaInterpreterObject *)lua_touserdata(inState, -1);
        lua_pop(inState, 1);
        return retval;
    }

    void DoStart(lua_State *inState, const char8_t *inId = "")
    {
        int tableIndex = 0;
        // note that the first arg was the 'this' pointer
        if (lua_gettop(inState) != 1 && lua_istable(inState, 2)) {
            tableIndex = 2;
        }
        m_ScriptContext = QT3DS_NEW(GetFoundation().getAllocator(), SScriptContext)(
            GetFoundation(), *m_StateContext->GetDOMFactory()->GetStringTable(), inState,
            tableIndex);
        // Append us to the global table.
        m_ScriptContext->GetGlobalTable();
        m_ScriptContext->SetId(inId);
        // This first argument was the 'this' table.
        lua_pushvalue(inState, 1);
        lua_setfield(inState, -2, "Qt3DSstate_interpreterobject");
        lua_pop(inState, 1);
        m_Interpreter = IStateInterpreter::Create(
            GetFoundation(), *m_StateContext->GetDOMFactory()->GetStringTable(), *m_ScriptContext,
            IExecutionContext::Create(GetFoundation(),
                                      *m_StateContext->GetDOMFactory()->GetStringTable(), m_Logger,
                                      *m_ScriptContext));

        m_EventConnection = m_Interpreter->RegisterEventHandler(*this);
        m_Interpreter->Initialize(*m_StateContext);
        if (m_Debugger)
            m_Debugger->Connect(m_Interpreter->GetDebugInterface());
    }

    void DoBind(IStateInterpreter &inInterpreter, lua_State *inState)
    {
        int theTop = lua_gettop(inState);
        // bad if someone is using a non-lua script context.
        m_ScriptContext = static_cast<SScriptContext &>(inInterpreter.GetScriptContext());
        m_ScriptContext->GetGlobalTable();
        lua_pushvalue(inState, theTop);
        lua_setfield(inState, -2, "Qt3DSstate_interpreterobject");
        m_Interpreter = inInterpreter;
        m_EventConnection = m_Interpreter->RegisterEventHandler(*this);
        lua_settop(inState, theTop);
    }

    // All of these expect the table to be the first argument
    static int Initialize(lua_State *inState, const char8_t *inId)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        if (!isTrivial(inId)) {
            lua_pushstring(inState, inId);
            lua_setfield(inState, 1, "id");
        }
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself)
            myself->DoStart(inState, inId);
        return 0;
    }

    static int Start(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself) {
            myself->DoStart(inState);
            if (myself->m_Interpreter)
                myself->m_Interpreter->Start();
        }
        return 0;
    }

    void DoSet(lua_State *inState)
    {
        if (lua_gettop(inState) > 2) {
            m_ScriptContext->GetGlobalTable(inState);
            lua_insert(inState, -3);
            lua_settable(inState, -3);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    static int Set(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself)
            myself->DoSet(inState);
        return 0;
    }

    int DoGet(lua_State *inState)
    {
        if (lua_gettop(inState) > 1) {
            m_ScriptContext->GetGlobalTable(inState);
            // set the global table underneath the top of the stack so the top
            // still has the 'get' argument.
            lua_insert(inState, -2);
            lua_gettable(inState, -2);
            return 1;
        } else {
            QT3DS_ASSERT(false);
        }
        return 0;
    }

    static int Get(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself)
            return myself->DoGet(inState);
        return 0;
    }

    void DoFireEvent(lua_State *inState)
    {
        STopScope __stackScope(inState);
        if (__stackScope.m_Top > 1) {
            const char *evtName = lua_tostring(inState, 2);
            // Table is set to the third arg, whatever that may be.
            if (__stackScope.m_Top > 2) {
                SLuaScriptEvent *theEvent =
                    static_cast<SLuaScriptEvent *>(m_ScriptContext->CreateScriptEvent(
                        m_ScriptContext->m_StringTable->RegisterStr(evtName)));
                lua_pushvalue(inState, 3);
                theEvent->SetDataTable();
                m_Interpreter->QueueEvent(theEvent);
            } else {
                m_Interpreter->QueueEvent(evtName);
            }
        }
    }

    static int FireEvent(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself)
            myself->DoFireEvent(inState);
        return 0;
    }

    void DoStep(lua_State * /*inState*/) { m_Interpreter->Execute(); }

    static int Step(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself)
            myself->DoStep(inState);
        return 0;
    }

    // We have to do things manually here because the lua state isn't the same
    // when called from the console (!!).
    bool DoEval(lua_State *inState)
    {
        luaL_checktype(inState, -1, LUA_TSTRING);
        const char *evalStr = lua_tostring(inState, -1);
        eastl::string compileStr("return ");
        compileStr.append(evalStr);
        int failure = luaL_loadstring(inState, compileStr.c_str());
        if (failure) {
            qCCritical(INVALID_OPERATION, "Failed to eval %s: %s", evalStr, lua_tostring(inState, -1));
            return false;
        }
        m_ScriptContext->GetGlobalTable(inState);
        lua_setfenv(inState, -2);
        failure = lua_pcall(inState, 0, 1, 0);
        if (failure) {
            qCCritical(INVALID_OPERATION, "Failed to eval %s: %s", evalStr, lua_tostring(inState, -1));
            return false;
        }
        return true;
    }

    static int Eval(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself) {
            if (myself->DoEval(inState))
                return 1;
        }
        return 0;
    }

    static int In(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        luaL_checktype(inState, 2, LUA_TSTRING);
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (myself) {
            bool result = myself->m_ScriptContext->In(lua_tostring(inState, 2));
            lua_pushboolean(inState, (int)result);
            return 1;
        }
        return 0;
    }

    static int Configuration(lua_State *inState)
    {
        luaL_checktype(inState, 1, LUA_TTABLE);
        bool onlyAtomic = false;
        if (lua_gettop(inState) == 2)
            onlyAtomic = lua_toboolean(inState, 2) ? true : false;
        SLuaInterpreterObject *myself = GetObjectFromTable(inState, 1);
        if (!myself)
            return 0;
        NVConstDataRef<SStateNode *> theConfiguration = myself->m_Interpreter->GetConfiguration();
        lua_newtable(inState);
        for (QT3DSU32 idx = 0, end = theConfiguration.size(); idx < end; ++idx) {
            bool isStateAtomic = theConfiguration[idx]->IsAtomic();
            if (!onlyAtomic || isStateAtomic) {
                lua_pushstring(inState, theConfiguration[idx]->m_Id.c_str());
                lua_pushnumber(inState, idx + 1);
                lua_settable(inState, -3);
            }
        }
        return 1;
    }

    // Bind the interpreter c functions into the interpreter table.
    // the interpreter is assumed to be userdata at the top of the stack.
    static void BindCFunctions(lua_State *inState)
    {
        // Don't create the interpreter yet because it requires a script context and we
        // don't know what the datatable table is going to be.
        lua_newtable(inState);
        // Switch the order of the user data and this table
        lua_insert(inState, -2);
        lua_setfield(inState, -2, "__implementation");
        lua_pushcfunction(inState, Start);
        lua_setfield(inState, -2, "start");
        lua_pushcfunction(inState, Set);
        lua_setfield(inState, -2, "set");
        lua_pushcfunction(inState, Get);
        lua_setfield(inState, -2, "get");
        lua_pushcfunction(inState, FireEvent);
        lua_setfield(inState, -2, "fireEvent");
        lua_pushcfunction(inState, Step);
        lua_setfield(inState, -2, "step");
        lua_pushcfunction(inState, Eval);
        lua_setfield(inState, -2, "eval");
        lua_pushcfunction(inState, In);
        lua_setfield(inState, -2, "active");
        lua_pushcfunction(inState, Configuration);
        lua_setfield(inState, -2, "configuration");
    }

    static SLuaInterpreterObject *CreateInterpreter(lua_State *inState,
                                                    debugger::IDebugger *inDebugger,
                                                    NVFoundationBase *inFoundation)
    {
        SLuaInterpreterObject *retval =
            (SLuaInterpreterObject *)lua_newuserdata(inState, sizeof(SLuaInterpreterObject));
        new (retval) SLuaInterpreterObject(inDebugger, inFoundation);

        int top = lua_gettop(inState);
        luaL_getmetatable(inState, "SLuaInterpreterObjectUserData");
        if (lua_isnil(inState, -1)) {
            lua_pop(inState, 1);
            luaL_newmetatable(inState, "SLuaInterpreterObjectUserData");
            QT3DS_ASSERT(lua_istable(inState, -1));
            lua_pushcfunction(inState, OnGC);
            lua_setfield(inState, -2, "__gc");
        }
        // setup correct destruction of the user data
        lua_setmetatable(inState, -2);
        lua_settop(inState, top);
        return retval;
    }

    static int Bind(IStateInterpreter &inInterpreter, lua_State *inState)
    {
        SLuaInterpreterObject *retval =
            CreateInterpreter(inState, NULL, &inInterpreter.GetFoundation());
        BindCFunctions(inState);
        retval->DoBind(inInterpreter, inState);
        return 1;
    }

    static int Parse(lua_State *inState, debugger::IDebugger *inDebugger = NULL,
                     NVFoundationBase *inFoundation = NULL, IStringTable *inStrTable = NULL)
    {
        luaL_checktype(inState, 1, LUA_TSTRING);
        const char *path = lua_tostring(inState, 1);
        CFileSeekableIOStream theStream(path, FileReadFlags());
        if (theStream.IsOpen() == false)
            luaL_error(inState, "Qt3DSState::parse - Failed to open %s", path);
        SLuaInterpreterObject *retval = CreateInterpreter(inState, inDebugger, inFoundation);
        int top = lua_gettop(inState);
        retval->m_StateContext = IStateContext::Load(
            retval->graphAllocator, retval->GetFoundation(), theStream, path, inStrTable);
        if (retval->m_StateContext.mPtr == NULL) {
            luaL_error(inState, "Qt3DSState::parse - Failed to parse %s", path);
        }
        BindCFunctions(inState);
        QT3DS_ASSERT(top == lua_gettop(inState));
        (void)top;
        return 1;
    }
    static int RawParse(lua_State *inState) { return Parse(inState); }
};

MallocAllocator g_MallocAlloc;
}

ILuaScriptContext &ILuaScriptContext::CreateLuaScriptContext(NVFoundationBase &inFoundation,
                                                             IStringTable &inStrTable)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SScriptContext)(inFoundation, inStrTable);
}

ILuaScriptContext &ILuaScriptContext::CreateLuaScriptContext(NVFoundationBase &inFoundation,
                                                             IStringTable &inStrTable,
                                                             lua_State *inState,
                                                             int inGlobalTableIdx)
{
    if (inState == NULL) {
        QT3DS_ASSERT(false);
    }
    return *QT3DS_NEW(inFoundation.getAllocator(), SScriptContext)(inFoundation, inStrTable, inState,
                                                                inGlobalTableIdx);
}

void ILuaScriptContext::ExportLuaBindings(lua_State *inState)
{
    lua_pushcfunction(inState, SLuaInterpreterObject::RawParse);
    lua_setglobal(inState, "parseSCXML");
}

int ILuaScriptContext::ParseSCXML(lua_State *inState, debugger::IDebugger *inDebugger)
{
    return SLuaInterpreterObject::Parse(inState, inDebugger);
}

int ILuaScriptContext::ParseSCXML(lua_State *inState, debugger::IDebugger *inDebugger,
                                  NVFoundationBase &inFoundation, IStringTable &inStrTable)
{
    return SLuaInterpreterObject::Parse(inState, inDebugger, &inFoundation, &inStrTable);
}

int ILuaScriptContext::Initialize(lua_State *inState, const char8_t *inID)
{
    return SLuaInterpreterObject::Initialize(inState, inID);
}

int ILuaScriptContext::Start(lua_State *inState)
{
    return SLuaInterpreterObject::Start(inState);
}

void ILuaScriptContext::Bind(IStateInterpreter &inInterpreter, lua_State *inState)
{
    SLuaInterpreterObject::Bind(inInterpreter, inState);
}

IStateInterpreter *ILuaScriptContext::GetInterpreterFromBindings(lua_State *inState)
{
    if (lua_istable(inState, -1)) {
        lua_getfield(inState, -1, "__implementation");
        if (lua_isuserdata(inState, -1)) {
            SLuaInterpreterObject *theObject = (SLuaInterpreterObject *)lua_touserdata(inState, -1);
            if (theObject)
                return theObject->m_Interpreter.mPtr;
        }
    } else {
        QT3DS_ASSERT(false);
    }
    return NULL;
}
