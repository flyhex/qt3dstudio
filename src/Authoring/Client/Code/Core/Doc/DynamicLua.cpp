/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "stdafx.h"
#include "DynamicLua.h"
#define _NOMINMAX
#include "foundation/Qt3DSMath.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "WINDOWS/DynamicLibLoader.h"
#include "EASTL/string.h"
#include "foundation/Utils.h"

using namespace Q3DStudio;
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::intrinsics;

namespace {
struct lua_State;

#define LUA_REGISTRYINDEX (-10000)
#define LUA_ENVIRONINDEX (-10001)
#define LUA_GLOBALSINDEX (-10002)
#define lua_upvalueindex(i) (LUA_GLOBALSINDEX - (i))
#define lua_tostring(L, i) lua_tolstring(L, (i), NULL)
#define lua_setglobal(L, s) lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_getglobal(L, s) lua_getfield(L, LUA_GLOBALSINDEX, (s))
#define LUA_TNONE (-1)

#define LUA_TNIL 0
#define LUA_TBOOLEAN 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER 3
#define LUA_TSTRING 4
#define LUA_TTABLE 5
#define LUA_TFUNCTION 6
#define LUA_TUSERDATA 7
#define LUA_TTHREAD 8

#define lua_isfunction(L, n) (lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L, n) (lua_type(L, (n)) == LUA_TTABLE)
#define lua_isnumber(L, n) (lua_type(L, (n)) == LUA_TNUMBER)
#define lua_islightuserdata(L, n) (lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L, n) (lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L, n) (lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L, n) (lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L, n) (lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type(L, (n)) <= 0)
#define lua_pop(L, n) lua_settop(L, -(n)-1)

struct DynLua : public IDynamicLua
{

    typedef void *(*TAllocFn)(void *ud, void *ptr, size_t osize, size_t nsize);
    typedef lua_State *(*TOpenFn)(TAllocFn, void *ud);
    typedef void (*TCloseFn)(lua_State *st);
    typedef int (*TIntLuaFn)(lua_State *st);
    typedef void (*TVoidLuaIntFn)(lua_State *st, int arg);
    typedef void (*TVoidLuaVoidPtrFn)(lua_State *L, void *p);
    typedef void (*TVoidLuaIntCharPtrFn)(lua_State *L, int idx, const char *k);
    typedef void (*TVoidLuaCharPtrFn)(lua_State *L, const char *k);
    typedef int (*TIntLuaIntIntIntFn)(lua_State *L, int nargs, int nresults, int errfunc);
    typedef const char *(*TCharPtrLuaIntSizeTFn)(lua_State *L, int idx, size_t *len);
    typedef int (*TIntLuaCharPtrFn)(lua_State *L, const char *filename);
    typedef int (*TIntLuaIntFn)(lua_State *L, int idx);
    typedef float (*TFloatLuaIntFn)(lua_State *L, int idx);
    typedef void (*TVoidLuaIntIntFn)(lua_State *L, int index, int n);
    typedef void (*TVoidLuaFloatFn)(lua_State *L, float num);

#define ITERATE_REQUIRED_LUA_FUNCTIONS                                                             \
    HANDLE_LUA_FUNCTION(lua_newstate, TOpenFn)                                                     \
    HANDLE_LUA_FUNCTION(lua_close, TCloseFn)                                                       \
    HANDLE_LUA_FUNCTION(lua_gettop, TIntLuaFn)                                                     \
    HANDLE_LUA_FUNCTION(lua_settop, TVoidLuaIntFn)                                                 \
    HANDLE_LUA_FUNCTION(luaL_openlibs, TCloseFn)                                                   \
    HANDLE_LUA_FUNCTION(lua_pushlightuserdata, TVoidLuaVoidPtrFn)                                  \
    HANDLE_LUA_FUNCTION(lua_setfield, TVoidLuaIntCharPtrFn)                                        \
    HANDLE_LUA_FUNCTION(lua_getfield, TVoidLuaIntCharPtrFn)                                        \
    HANDLE_LUA_FUNCTION(lua_pushstring, TVoidLuaCharPtrFn)                                         \
    HANDLE_LUA_FUNCTION(lua_pushnumber, TVoidLuaFloatFn)                                           \
    HANDLE_LUA_FUNCTION(lua_pushnil, TCloseFn)                                                     \
    HANDLE_LUA_FUNCTION(lua_pcall, TIntLuaIntIntIntFn)                                             \
    HANDLE_LUA_FUNCTION(lua_tolstring, TCharPtrLuaIntSizeTFn)                                      \
    HANDLE_LUA_FUNCTION(luaL_loadfile, TIntLuaCharPtrFn)                                           \
    HANDLE_LUA_FUNCTION(lua_type, TIntLuaIntFn)                                                    \
    HANDLE_LUA_FUNCTION(lua_next, TIntLuaIntFn)                                                    \
    HANDLE_LUA_FUNCTION(lua_insert, TVoidLuaIntFn)                                                 \
    HANDLE_LUA_FUNCTION(lua_tonumber, TFloatLuaIntFn)                                              \
    HANDLE_LUA_FUNCTION(lua_toboolean, TIntLuaIntFn)                                               \
    HANDLE_LUA_FUNCTION(lua_rawgeti, TVoidLuaIntIntFn)                                             \
    HANDLE_LUA_FUNCTION(lua_objlen, TIntLuaIntFn)

#define HANDLE_LUA_FUNCTION(name, type) type name;

    ITERATE_REQUIRED_LUA_FUNCTIONS

#undef HANDLE_LUA_FUNCTION

    NVFoundationBase &m_Foundation;
    uic::render::CLoadedDynamicLibrary *m_LuaLibrary;
    lua_State *m_LuaState;
    eastl::string m_TempStr;

    static void *LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
    {
        DynLua &theLua = *reinterpret_cast<DynLua *>(ud);
        NVAllocatorCallback &theAlloc = theLua.m_Foundation.getAllocator();
        if (nsize == 0) {
            if (ptr)
                theAlloc.deallocate(ptr);
            return NULL;
        } else {
            if (nsize < osize && ptr)
                return ptr;

            void *newMem = theAlloc.allocate(nsize, "lua_mem", __FILE__, __LINE__);
            if (osize && ptr) {
                size_t copyAmt = NVMin(osize, nsize);
                memCopy(newMem, ptr, (QT3DSU32)copyAmt);
                theAlloc.deallocate(ptr);
            }
            return newMem;
        }
    }

    void *FindLuaFunction(const char *inName, NVFoundationBase &fnd)
    {
        void *retval = m_LuaLibrary->FindFunction(inName);
        if (!retval)
            qCCritical(INVALID_OPERATION, "Failed to find lua function %s", inName);
        return retval;
    }

    DynLua(NVFoundationBase &fnd, const char8_t *pathToLuaDll)
        : m_Foundation(fnd)
        , m_LuaLibrary(uic::render::CLoadedDynamicLibrary::Create(pathToLuaDll, fnd))
        , m_LuaState(NULL)
    {
        if (m_LuaLibrary) {
            bool success = true;
#define HANDLE_LUA_FUNCTION(name, type)                                                            \
    name = reinterpret_cast<type>(FindLuaFunction(#name, fnd));                                    \
    if (name == NULL)                                                                              \
    success = false;

            ITERATE_REQUIRED_LUA_FUNCTIONS

        #undef HANDLE_LUA_FUNCTION

                    if (success) {
                m_LuaState = (*lua_newstate)(LuaAlloc, this);
                luaL_openlibs(m_LuaState);
            }
        }
    }

    ~DynLua()
    {
        if (m_LuaState)
            lua_close(m_LuaState);
        m_LuaState = NULL;
        if (m_LuaLibrary)
            NVDelete(m_Foundation.getAllocator(), m_LuaLibrary);
        m_LuaLibrary = NULL;
    }

    void AddLibraryPath(const char8_t *inPath, PathType inType) override
    {
        const char8_t *extName = inType == LuaModule ? "lua" : "dll";
        const char8_t *fieldName = inType == LuaModule ? "path" : "cpath";
        lua_getglobal(m_LuaState, "package");
        lua_getfield(m_LuaState, -1,
                     fieldName); // get field "path"/"cpath" from table at top of stack (-1)
        const char8_t *outOriginalPath =
                lua_tostring(m_LuaState, -1); // grab path string from top of stack

        eastl::string cur_path(outOriginalPath);
        cur_path.append(";");
        cur_path.append(inPath);
        cur_path.append("/?.");
        cur_path.append(extName);

        lua_pop(m_LuaState, 1); // get rid of the string on the stack we just pushed on line 3
        lua_pushstring(m_LuaState, cur_path.c_str()); // push the new one
        lua_setfield(
                    m_LuaState, -2,
                    fieldName); // set the field "path"/"cpath" in table at -2 with value at top of stack
        lua_pop(m_LuaState, 1); // get rid of package table from top of stack
    }

    bool LoadLuaFile(const char8_t *inFname, const char8_t *inGlobalVarName) override
    {
        luaL_loadfile(m_LuaState, inFname);
        int error = lua_pcall(m_LuaState, 0, 1, 0);
        if (error) {
            const char8_t *errorStr = lua_tostring(m_LuaState, -1);
            qCWarning(WARNING, "Error loading lua file: %s", errorStr);
            return false;
        } else {
            lua_setfield(m_LuaState, LUA_GLOBALSINDEX, inGlobalVarName);
        }
        lua_settop(m_LuaState, 0);
        return true;
    }

    int GetTop() override { return lua_gettop(m_LuaState); }
    void SetTop(int inTop) override { lua_settop(m_LuaState, inTop); }

    Option<QT3DSF32> NumberFromTopOfStackTable(const char8_t *inKeyName) override
    {
        lua_getfield(m_LuaState, -1, inKeyName);
        Option<QT3DSF32> retval;
        if (!lua_isnil(m_LuaState, -1))
            retval = lua_tonumber(m_LuaState, -1);
        lua_pop(m_LuaState, 1);
        return retval;
    }

    Option<QT3DSVec3> Vec3FromTopOfStackTable(const char8_t *inKeyName) override
    {
        lua_getfield(m_LuaState, -1, inKeyName);
        Option<QT3DSVec3> retval;
        if (lua_istable(m_LuaState, -1)) {
            QT3DSVec3 result;
            lua_getfield(m_LuaState, -1, "r");
            if (lua_isnumber(m_LuaState, -1))
                result.x = lua_tonumber(m_LuaState, -1);
            lua_pop(m_LuaState, 1);
            lua_getfield(m_LuaState, -1, "g");
            if (lua_isnumber(m_LuaState, -1))
                result.y = lua_tonumber(m_LuaState, -1);
            lua_pop(m_LuaState, 1);
            lua_getfield(m_LuaState, -1, "b");
            if (lua_isnumber(m_LuaState, -1))
                result.z = lua_tonumber(m_LuaState, -1);
            lua_pop(m_LuaState, 1);
            retval = result;
        }
        lua_pop(m_LuaState, 1);
        return retval;
    }

    qt3ds::foundation::Option<bool> BooleanFromTopOfStackTable(const char8_t *inKeyName) override
    {
        QT3DS_ASSERT(lua_istable(m_LuaState, -1));
        lua_getfield(m_LuaState, -1, inKeyName);
        bool retval = lua_toboolean(m_LuaState, -1) ? true : false;
        lua_pop(m_LuaState, 1);
        return retval;
    }

    eastl::string StringFromTopOfStackTable(const char8_t *inKeyName) override
    {
        QT3DS_ASSERT(lua_istable(m_LuaState, -1));
        lua_getfield(m_LuaState, -1, inKeyName);
        eastl::string retval;
        if (!lua_isnil(m_LuaState, -1)) {
            retval.assign(nonNull(lua_tostring(m_LuaState, -1)));
        }
        lua_pop(m_LuaState, 1);
        return retval;
    }
    // inIndex is 1 based, not zero based.  Uses rawgeti so don't expect metadata to work.
    // Returns true if the child is nonnull.  If it returns false, it pops the null off the stack
    // leaving the table.
    bool GetChildFromTopOfStackTable(QT3DSI32 inIndex) override
    {
        QT3DS_ASSERT(lua_istable(m_LuaState, -1));
        lua_rawgeti(m_LuaState, -1, inIndex);
        if (lua_isnil(m_LuaState, -1)) {
            lua_pop(m_LuaState, 1);
            return false;
        }
        return true;
    }

    int GetNumChildrenFromTopOfStackTable() override
    {
        QT3DS_ASSERT(lua_istable(m_LuaState, -1));
        return lua_objlen(m_LuaState, -1);
    }

    bool ExecuteFunction(const char8_t *inGlobalVarName, const char8_t *fnName,
                                 const char8_t *fnArg) override
    {
        int top = lua_gettop(m_LuaState);
        lua_getglobal(m_LuaState, inGlobalVarName);
        if (lua_isnil(m_LuaState, -1)) {
            lua_settop(m_LuaState, top);
            return false;
        }
        lua_getfield(m_LuaState, -1, fnName);

        if (!lua_isfunction(m_LuaState, -1)) {
            lua_settop(m_LuaState, top);
            return false;
        }

        lua_pushstring(m_LuaState, fnArg);
        int error = lua_pcall(m_LuaState, 1, 1, 0);
        if (error) {
            const char8_t *errorStr = lua_tostring(m_LuaState, -1);
            qCWarning(WARNING, "Error loading lua file: %s", errorStr);
            return false;
        }
        return true;
    }

    bool ExecuteFunction(const char8_t *inGlobalVarName, const char8_t *fnName, int numArgs,
                                 int numResults) override
    {
        int top = lua_gettop(m_LuaState);
        lua_getglobal(m_LuaState, inGlobalVarName);
        if (lua_isnil(m_LuaState, -1)) {
            lua_settop(m_LuaState, top);
            return false;
        }
        lua_getfield(m_LuaState, -1, fnName);
        if (!lua_isfunction(m_LuaState, -1)) {
            lua_settop(m_LuaState, top);
            return false;
        }
        // get rid of table.
        lua_insert(m_LuaState, -2);
        lua_pop(m_LuaState, 1);
        if (!lua_isfunction(m_LuaState, -1)) {
            lua_settop(m_LuaState, top);
            return false;
        }
        // insert the arguments in same order above the function definition
        // Currently:
        // arg0
        // argn
        // fn
        for (int idx = 0, end = numArgs; idx < end; ++idx)
            lua_insert(m_LuaState, -1 - numArgs);
        // now:
        // fn
        // arg0,
        // argn
        int error = lua_pcall(m_LuaState, numArgs, numResults, 0);
        if (error) {
            const char8_t *errorStr = lua_tostring(m_LuaState, -1);
            qCWarning(WARNING, "Error loading lua file: %s", errorStr);
            return false;
        }
        return true;
    }

    bool BeginTableIteration() override
    {
        lua_pushnil(m_LuaState);
        return lua_next(m_LuaState, -2) ? true : false;
    }

    bool NextTableIteration(int numPops) override
    {
        // pop the value off the stack
        if (numPops > 0)
            lua_pop(m_LuaState, numPops);
        return lua_next(m_LuaState, -2) ? true : false;
    }

    void ParseFloatingPointPairArray(eastl::vector<QT3DSVec2> &outFloats) override
    {
        for (bool success = BeginTableIteration(); success; success = NextTableIteration(0)) {
            int top = lua_gettop(m_LuaState);
            QT3DSVec2 result(0, 0);
            if (BeginTableIteration()) {
                result.x = lua_tonumber(m_LuaState, -1);
                if (NextTableIteration(1))
                    result.y = lua_tonumber(m_LuaState, -1);
            }
            // Pop the value off, not certain where everything ends up.
            lua_settop(m_LuaState, top - 1);
            outFloats.push_back(result);
        }
        lua_pop(m_LuaState, 1);
    }

    void Release() override
    {
        NVAllocatorCallback *cback = &m_Foundation.getAllocator();
        NVDelete(*cback, this);
    }
};
}

IDynamicLua *IDynamicLua::CreateDynamicLua(qt3ds::NVFoundationBase &fnd, const char8_t *dllPath)
{
    DynLua *retval = QT3DS_NEW(fnd.getAllocator(), DynLua)(fnd, dllPath);
    // If the lookup system was successful.
    if (retval->m_LuaState)
        return retval;

    retval->Release();
    return NULL;
}
