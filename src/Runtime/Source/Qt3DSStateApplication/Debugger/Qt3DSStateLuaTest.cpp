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
#include "Qt3DSState.h"
#include "Qt3DSStateTest.h"
#include "Qt3DSStateTestCommon.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace qt3ds::state::test;
using namespace qt3ds::state;

namespace {

enum EPathType { eLuaPath, eCPath };
const char *const sm_PathType[] = { "path", "cpath" };
const char *const sm_PathTypeExtension[] = {
    "lua",
#ifdef _WIN32
    "dll",
#else
    "so"
#endif
};

// stack exchange
// inspired by
// http://stackoverflow.com/questions/4125971/setting-the-global-lua-path-variable-from-c-c
int appendLuaPath(lua_State *L, const char *path, EPathType inPathType)
{
    if (L == NULL)
        return 0;
    lua_getglobal(L, "package");
    lua_getfield(L, -1, sm_PathType[inPathType]);
    eastl::string cur_path = lua_tostring(L, -1); // grab path string from top of stack
    cur_path.append(1, ';'); // add separator
    cur_path.append(path); // add additional path
    cur_path.append("/?.");
    cur_path.append(sm_PathTypeExtension[inPathType]);
    lua_pop(L, 1);
    lua_pushstring(L, cur_path.c_str()); // set new patth
    lua_setfield(L, -2, sm_PathType[inPathType]);
    lua_pop(L, 1); // get rid of package table from top of stack
    return 0; // all done!
}

int Output(lua_State *inState)
{
    lua_concat(inState, lua_gettop(inState));
    const char *message = lua_tostring(inState, -1);
    lua_getfield(inState, LUA_REGISTRYINDEX, "uic_test_logger");
    IDataLogger *theLogger = (IDataLogger *)lua_touserdata(inState, -1);
    lua_getfield(inState, LUA_REGISTRYINDEX, "uic_test_line");
    int line = (int)lua_tonumber(inState, -1);
    lua_getfield(inState, LUA_REGISTRYINDEX, "uic_test_file");
    const char *file = lua_tostring(inState, -1);
    if (theLogger) {
        theLogger->Log(LogType::Info, file, line, message);
    }
    return 0;
}

// this function doesn't return
int TestError(lua_State *inState)
{
    const char *message = lua_tostring(inState, -2);
    int stackDepth = (int)lua_tonumber(inState, -1);
    lua_getfield(inState, LUA_REGISTRYINDEX, "uic_test_logger");
    IDataLogger *theLogger = (IDataLogger *)lua_touserdata(inState, -1);
    lua_getfield(inState, LUA_REGISTRYINDEX, "uic_test_file");
    const char *file = lua_tostring(inState, -1);

    lua_Debug ar;
    lua_getstack(inState, stackDepth, &ar);
    lua_getinfo(inState, "nSl", &ar);
    int line = ar.currentline;
    if (theLogger) {
        theLogger->Log(LogType::Error, file, line, message);
    }
    // and pow; no return.
    lua_error(inState);
    return 0;
}

Option<STestResults> RunTest(const char8_t *inFullPath, const char8_t *inRootDir,
                             const char8_t *inLunityPath, IDataLogger &inLogger)
{
    CAllocator nvAlloc;
    NVScopedRefCounted<NVFoundation> nvFoundation(
        NVCreateFoundation(QT3DS_FOUNDATION_VERSION, nvAlloc));
    SLuaContext theContext(*nvFoundation);

    luaL_loadfile(theContext, inLunityPath);
    int failure = lua_pcall(theContext, 0, 0, 0);
    if (failure) {
        inLogger.Log(LogType::Error, inLunityPath, 0, "Failed to load lunity test library");
        return Option<STestResults>();
    }

    eastl::string relPath;
    eastl::string fullPathStr(inFullPath);
    eastl::string inProjectDir(inRootDir);
    CFileTools::GetRelativeFromBase(inProjectDir, fullPathStr, relPath);
    eastl::string scriptsDir(inRootDir);
    scriptsDir.append("/scripts");
    appendLuaPath(theContext, scriptsDir.c_str(), eLuaPath);
    const char8_t *fname = relPath.c_str();

    // setup the test output system.
    lua_pushlightuserdata(theContext, &inLogger);
    lua_setfield(theContext, LUA_REGISTRYINDEX, "uic_test_logger");
    lua_pushstring(theContext, fname);
    lua_setfield(theContext, LUA_REGISTRYINDEX, "uic_test_file");
    lua_pushnumber(theContext, 0.f);
    lua_setfield(theContext, LUA_REGISTRYINDEX, "uic_test_line");
    lua_pushcfunction(theContext, Output);
    lua_setfield(theContext, LUA_GLOBALSINDEX, "output");
    lua_pushcfunction(theContext, Output);
    // Really, we should have done this a long time ago, overridden print.
    lua_setfield(theContext, LUA_GLOBALSINDEX, "print");

    lua_pushcfunction(theContext, TestError);
    lua_setfield(theContext, LUA_GLOBALSINDEX, "testError");

    luaL_loadfile(theContext, inFullPath);
    // function at top of stack

    // push new global table
    lua_newtable(theContext);
    // create new metatable
    lua_newtable(theContext);
    lua_getfield(theContext, LUA_GLOBALSINDEX, "_G");
    lua_setfield(theContext, -2, "__index");
    // the metatable is at the top of the stack, with the new table underneath
    lua_setmetatable(theContext, -2);

    // dup the new table and put it in the test context
    lua_pushvalue(theContext, -1);
    lua_setfield(theContext, LUA_REGISTRYINDEX, "test_context");

    // now we only have the global table on the stack with the function underneath.
    QT3DS_ASSERT(lua_istable(theContext, -1));
    QT3DS_ASSERT(lua_isfunction(theContext, -2));
    lua_setfenv(theContext, -2);

    failure = lua_pcall(theContext, 0, 0, 0);
    eastl::string logStr;
    if (failure) {
        const char8_t *failureMessage = lua_tostring(theContext, -1);
        logStr.assign("Failed to load test file: ");
        logStr.append(failureMessage);
        inLogger.Log(LogType::Error, fname, 0, logStr.c_str());
        return Option<STestResults>();
    }
    /* - useful for debugging
    QT3DS_ASSERT( lua_istable( theContext, -1 ) );
    lua_pushnil( theContext );
    while( lua_next( theContext, -2 ) != 0 )
    {
            const char* valVal = lua_tostring(theContext, -1);
            lua_pushvalue( theContext, -2 );
            const char* keyVal = lua_tostring( theContext, -1 );
            lua_pop( theContext, 2 );

            logStr.assign( "Global: " );
            logStr.append( keyVal );
            logStr.append( " -> " );
            logStr.append( valVal );
            inLogger.Log( LogType::Info, fname, 0, logStr.c_str() );
            (void)valVal;
            (void)keyVal;
    }
    lua_pop( theContext, 1 );
    */
    // Find the run all tests function
    lua_getfield(theContext, LUA_GLOBALSINDEX, "__runAllTests");
    QT3DS_ASSERT(lua_isfunction(theContext, -1));
    lua_getfield(theContext, LUA_REGISTRYINDEX, "test_context");
    failure = lua_pcall(theContext, 1, 3, 0);
    if (failure) {
        const char8_t *failureMessage = lua_tostring(theContext, -1);
        logStr.assign("Failed to run unit tests: ");
        logStr.append(failureMessage);
        inLogger.Log(LogType::Error, fname, 0, logStr.c_str());
        return Option<STestResults>();
    }
    int numTests = (int)lua_tonumber(theContext, -3);
    int testsPassed = (int)lua_tonumber(theContext, -2);
    int assertsPassed = (int)lua_tonumber(theContext, -1);

    (void)assertsPassed;

    return STestResults((QT3DSU32)numTests, (QT3DSU32)testsPassed);
}
}

Option<STestResults> ILuaTest::RunFile(const char8_t *inFullPath, const char8_t *inRootDir,
                                       const char8_t *inLunityPath, IDataLogger &inLogger)
{
    return RunTest(inFullPath, inRootDir, inLunityPath, inLogger);
}
