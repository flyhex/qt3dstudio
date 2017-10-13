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
#include "UICState.h"
#include "UICStateTestCommon.h"
#include "UICStateLuaScriptContext.h"
#include "UICStateTest.h"
#include "foundation/IOStreams.h"
#include "EASTL/string.h"
#include "foundation/Utils.h"
#include "foundation/FileTools.h"
#include "foundation/XML.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/StringTable.h"
#include "UICStateContext.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "UICStateExecutionContext.h"
#include "UICStateInterpreter.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace qt3ds::state::test;
using namespace qt3ds::state;
using qt3ds::foundation::CFileSeekableIOStream;
using qt3ds::foundation::FileReadFlags;
using qt3ds::foundation::nonNull;
using qt3ds::foundation::CFileTools;
using qt3ds::foundation::CDOMSerializer;
using qt3ds::foundation::SDOMElement;
using qt3ds::foundation::SDOMAttribute;
using qt3ds::NVAllocatorCallback;
using qt3ds::foundation::CAllocator;
using qt3ds::foundation::NVScopedRefCounted;
using qt3ds::NVFoundation;
using qt3ds::NVFoundationBase;
using qt3ds::foundation::IStringTable;
using qt3ds::foundation::IDOMFactory;
using qt3ds::foundation::IDOMReader;
using qt3ds::foundation::SNamespacePairNode;
using qt3ds::foundation::SSAutoDeallocatorAllocator;

namespace {

struct XMLHandler : public qt3ds::foundation::CXmlErrorHandler
{
    IDataLogger &m_Logger;
    const char8_t *m_File;
    eastl::string m_ErrorString;
    XMLHandler(IDataLogger &logger, const char8_t *fname)
        : m_Logger(logger)
        , m_File(fname)
    {
    }

    void OnXmlError(qt3ds::foundation::TXMLCharPtr errorName, int line, int /*column*/) override
    {
        m_ErrorString.assign("Failed to parse test file: ");
        m_ErrorString.append(m_File);
        m_Logger.Log(LogType::Error, m_File, line, errorName);
    }
};

struct StateLogger : public IStateLogger
{
    lua_State *m_LuaState;
    eastl::string m_LogString;
    QT3DSI32 mRefCount;
    StateLogger(lua_State *state)
        : m_LuaState(state)
        , mRefCount(0)
    {
    }
    void addRef() override { ++mRefCount; }
    void release() override
    {
        --mRefCount;
        if (mRefCount <= 0)
            delete this;
    }

    void Log(const char8_t *inLabel, const char8_t *inExpression) override
    {
        int theTop = lua_gettop(m_LuaState);
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "uic_test_logger");
        IDataLogger *theLogger = (IDataLogger *)lua_touserdata(m_LuaState, -1);
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "uic_test_line");
        int line = (int)lua_tonumber(m_LuaState, -1);
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "uic_test_file");
        const char *file = lua_tostring(m_LuaState, -1);
        lua_settop(m_LuaState, theTop);
        m_LogString.assign(nonNull(inLabel));
        m_LogString.append(" - ");
        m_LogString.assign(nonNull(inExpression));
        if (theLogger) {
            theLogger->Log(LogType::Info, file, line, m_LogString.c_str());
        }
    }
};

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

const char8_t *whitespaceChars = "\n \r\t";

eastl::string::size_type FindNextNonWhite(eastl::string &str, eastl::string::size_type pos)
{
    eastl::string::size_type retval = str.find_first_of(whitespaceChars, pos);
    if (retval != eastl::string::npos)
        retval = str.find_first_of(whitespaceChars, retval);

    if (retval == str.size())
        retval = eastl::string::npos;

    return retval;
}

int SetCurrentLine(NVScopedRefCounted<IDOMReader> domReader, lua_State *inContext)
{
    int line = domReader->GetElement()->m_Line;
    int theTop = lua_gettop(inContext);
    lua_pushnumber(inContext, line);
    lua_setfield(inContext, LUA_REGISTRYINDEX, "uic_test_line");
    lua_settop(inContext, theTop);
    return line;
}

Option<STestResults> RunTest(const char8_t *inFullPath, const char8_t *inRoot,
                             IDataLogger &inLogger)
{
    CFileSeekableIOStream theXMLStream(inFullPath, FileReadFlags());
    eastl::string relPath;
    eastl::string fullPathStr(inFullPath);
    eastl::string inProjectDir(inRoot);
    CFileTools::GetRelativeFromBase(inProjectDir, fullPathStr, relPath);
    const char8_t *fname = relPath.c_str();
    eastl::string logString;
    if (!theXMLStream.IsOpen()) {
        logString.assign("Unable to open test file: ");
        logString.append(nonNull(fname));
        inLogger.Log(LogType::Error, fname, 0, logString.c_str());
        return Option<STestResults>();
    }
    CAllocator nvAlloc;
    NVScopedRefCounted<NVFoundation> nvFoundation(
        NVCreateFoundation(QT3DS_FOUNDATION_VERSION, nvAlloc));
    NVScopedRefCounted<IStringTable> strTable(
        IStringTable::CreateStringTable(nvFoundation->getAllocator()));
    NVScopedRefCounted<IDOMFactory> domFactory(
        IDOMFactory::CreateDOMFactory(nvFoundation->getAllocator(), strTable));
    XMLHandler theXMLHandler(inLogger, fname);
    eastl::pair<SNamespacePairNode *, SDOMElement *> readResult =
        CDOMSerializer::Read(*domFactory, theXMLStream, &theXMLHandler);
    if (!readResult.second) {
        return Option<STestResults>();
    }
    NVScopedRefCounted<IDOMReader> domReader(IDOMReader::CreateDOMReader(
        nvFoundation->getAllocator(), *readResult.second, strTable, domFactory));

    const char8_t *relDocument = 0;

    if (!AreEqual("scxml-test", domReader->GetElementName().c_str())
        || !domReader->UnregisteredAtt("document", relDocument)) {
        logString.assign("Root element in file is not scxml-test: ");
        logString.append(fname);
        inLogger.Log(LogType::Error, fname, 0, logString.c_str());
        return Option<STestResults>();
    }

    eastl::string fullDocPath;
    qt3ds::foundation::CFileTools::CombineBaseAndRelative(inRoot, relDocument, fullDocPath);
    CFileSeekableIOStream theSCXMLStream(fullDocPath.c_str(), FileReadFlags());
    if (!theSCXMLStream.IsOpen()) {
        logString.assign("Unable to open scxml file: ");
        logString.append(nonNull(fullDocPath.c_str()));
        inLogger.Log(LogType::Error, fname, 0, logString.c_str());
        return Option<STestResults>();
    }

    SSAutoDeallocatorAllocator theGraphAlloc(*nvFoundation);

    NVScopedRefCounted<IStateContext> theStateContext = IStateContext::Load(
        theGraphAlloc, *nvFoundation, theSCXMLStream, fullDocPath.c_str(), strTable.mPtr);
    if (!theStateContext) {
        // it would have logged the error via foundation already
        return Option<STestResults>();
    }

    // Build the lua context.
    SLuaContext theContext(*nvFoundation);
    int theDMStackIdx = 0;
    {
        IDOMReader::Scope __dmScope(domReader);
        const char8_t *luaInit = NULL;
        if (domReader->MoveToFirstChild("datamodel"))
            domReader->Value(luaInit);
        if (!isTrivial(luaInit)) {
            eastl::string luaEval(luaInit);
            luaL_loadstring(theContext, luaEval.c_str());

            int failure = lua_pcall(theContext, 0, 1, 0);
            if (failure) {
                const char *errorMsg = lua_tostring(theContext, -1);
                logString.assign("Failed to create datamodel: ");
                logString.append(errorMsg);
                inLogger.Log(LogType::Error, fname, 0, logString.c_str());
                return Option<STestResults>();
            }
            theDMStackIdx = lua_gettop(theContext);
        }
        if (theDMStackIdx == 0) {
            lua_newtable(theContext);
            theDMStackIdx = lua_gettop(theContext);
        }
        // create an output functions
        lua_pushlightuserdata(theContext, &inLogger);
        lua_setfield(theContext, LUA_REGISTRYINDEX, "uic_test_logger");
        lua_pushstring(theContext, fname);
        lua_setfield(theContext, LUA_REGISTRYINDEX, "uic_test_file");
        lua_pushnumber(theContext, 0.f);
        lua_setfield(theContext, LUA_REGISTRYINDEX, "uic_test_line");
        lua_pushcfunction(theContext, Output);
        lua_pushvalue(theContext, -1);
        lua_setfield(theContext, theDMStackIdx, "output");
        // Really, we should have done this a long time ago, overridden print.
        lua_setfield(theContext, theDMStackIdx, "print");
    }
    ILuaScriptContext &theScriptContext = ILuaScriptContext::CreateLuaScriptContext(
        *nvFoundation, *strTable, theContext, theDMStackIdx);
    lua_settop(theContext, 0);
    StateLogger &theStateLogger(*(new StateLogger(theContext)));
    IExecutionContext &theExecutionContext =
        IExecutionContext::Create(*nvFoundation, *strTable, theStateLogger, theScriptContext);
    IStateInterpreter &theInterpreter =
        IStateInterpreter::Create(*nvFoundation, *strTable, theScriptContext, theExecutionContext);
    NVScopedRefCounted<IStateInterpreter> __interpWatcher(theInterpreter);
    theInterpreter.Initialize(*theStateContext);
    theInterpreter.Start();

    // Bind it to the script context under engine.
    theScriptContext.GetGlobalTable();
    ILuaScriptContext::Bind(theInterpreter, theContext);
    lua_setfield(theContext, -2, "machine");
    lua_settop(theContext, 0);

    // Rather tedious, but it worked...

    for (bool success = domReader->MoveToFirstChild("step"); success;
         success = domReader->MoveToNextSibling("step")) {
        IDOMReader::Scope __stepScope(domReader);
        const char8_t *evtName = NULL;
        domReader->UnregisteredAtt("event", evtName);
        evtName = nonNull(evtName);
        if (!isTrivial(evtName))
            theInterpreter.QueueEvent(evtName);
        int theLine = SetCurrentLine(domReader, theContext);
        NVConstDataRef<SStateNode *> theConfig = theInterpreter.Execute();
        const char8_t *inList = NULL;
        if (domReader->UnregisteredAtt("in", inList)) {
            eastl::string theList(inList);
            eastl::string theItem;
            for (eastl::string::size_type pos = theList.find_first_not_of(whitespaceChars);
                 pos != eastl::string::npos; pos = FindNextNonWhite(theList, pos)) {
                eastl::string::size_type nextWhite = theList.find_first_of(whitespaceChars, pos);
                if (nextWhite == eastl::string::npos)
                    nextWhite = theList.size();
                theItem = theList.substr(pos, nextWhite - pos);

                if (theItem.size()) {
                    bool found = false;
                    for (QT3DSU32 idx = 0, end = theConfig.size(); idx < end && !found; ++idx) {
                        if (AreEqual(theConfig[idx]->m_Id.c_str(), theItem.c_str()))
                            found = true;
                    }
                    if (!found) {
                        logString.assign("Assertion failed: item \"");
                        logString.append(theItem.c_str());
                        logString.append("\" not found in machine configuration");
                        inLogger.Log(LogType::Error, fname, theLine, logString.c_str());
                        return STestResults(1, 0);
                    }
                }
            }
        }
        for (bool assertSuccess = domReader->MoveToFirstChild("assert"); assertSuccess;
             assertSuccess = domReader->MoveToNextSibling("assert")) {
            IDOMReader::Scope __assertScope(domReader);
            theLine = SetCurrentLine(domReader, theContext);
            const char8_t *condExpr = NULL;
            domReader->UnregisteredAtt("cond", condExpr);

            if (isTrivial(condExpr))
                domReader->Value(condExpr);

            if (!isTrivial(condExpr)) {
                Option<bool> condResult = theScriptContext.ExecuteCondition(condExpr);
                if (!condResult.hasValue()) {
                    logString.assign("Assertion failed: condition \"");
                    logString.append(condExpr);
                    logString.append("\" failed to execute");
                    inLogger.Log(LogType::Error, fname, theLine, logString.c_str());
                    return STestResults(1, 0);
                }
                if (!condResult.getValue()) {
                    logString.assign("Assertion failed: condition \"");
                    logString.append(condExpr);
                    logString.append("\" evaluated to false");
                    inLogger.Log(LogType::Error, fname, theLine, logString.c_str());
                    return STestResults(1, 0);
                }
            }
        }
    }

    return STestResults(1, 1);
}
}

Option<STestResults> IDataTest::RunFile(const char8_t *fname, const char8_t *inRootDir,
                                        IDataLogger &inLogger)
{
    return RunTest(fname, inRootDir, inLogger);
}
