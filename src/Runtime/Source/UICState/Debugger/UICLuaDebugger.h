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
#ifndef UIC_LUA_DEBUGGER_H
#define UIC_LUA_DEBUGGER_H
#include "UICStateDebugger.h"
#include "UICStateDebuggerValues.h"

struct lua_State;

namespace qt3ds {
namespace state {
    namespace debugger {
        namespace lua {
            struct SLuaDebugMessageHeader;
            struct SLuaDebuggerProtocolReader;
        }

        typedef eastl::string TDebugStr;

        struct SFileAndLine
        {
            CRegisteredString m_File;
            int m_Line;

            SFileAndLine(CRegisteredString inFile = CRegisteredString(), int inLine = 0)
                : m_File(inFile)
                , m_Line(inLine)
            {
            }
            bool operator==(const SFileAndLine &other) const
            {
                return m_File == other.m_File && m_Line == other.m_Line;
            }
        };

        struct SLuaBreakpoint : public SFileAndLine
        {
            CRegisteredString m_Condition;
            SLuaBreakpoint(CRegisteredString inFile = CRegisteredString(), int inLine = 0,
                           CRegisteredString inCondition = CRegisteredString())
                : SFileAndLine(inFile, inLine)
                , m_Condition(inCondition)
            {
            }
        };

        class ILuaSideListener
        {
        protected:
            virtual ~ILuaSideListener() {}
        public:
            virtual void OnBreak() = 0;
        };

        // Runtime side of the equation

        class ILuaDebugger : public NVRefCounted
        {
        public:
            // Installs ourself as the debug hook and waits for a command from the other end of the
            // stream.
            // We are stored under a special registry variable; there can be only one debugger per
            // lua state.
            static ILuaDebugger &CreateLuaSideDebugger(NVFoundationBase &inFoundation,
                                                       IDebugOutStream &inStream,
                                                       lua_State *inState, IStringTable &inStrTable,
                                                       const char *inApplicationDirectory,
                                                       ILuaSideListener *inListener = NULL);
        };

        struct SFunctionInfo
        {
            SDatamodelTable *m_Environment;
            SFileAndLine m_DefinitionInfo;
            SFunctionInfo()
                : m_Environment(NULL)
            {
            }
            SFunctionInfo(SDatamodelTable &env, SFileAndLine def)
                : m_Environment(&env)
                , m_DefinitionInfo(def)
            {
            }
        };

        struct SLuaStackInfo
        {
            SDatamodelValue m_Function;
            QT3DSI32 m_CurrentLine;
            CRegisteredString m_CallType;
            NVConstDataRef<STableEntry> m_LocalVars;
            NVConstDataRef<STableEntry> m_UpVars;
            SLuaStackInfo()
                : m_CurrentLine(0)
            {
            }
            SLuaStackInfo(SDatamodelValue fn, QT3DSI32 cl, CRegisteredString calltype,
                          NVConstDataRef<STableEntry> lv, NVConstDataRef<STableEntry> uv)
                : m_Function(fn)
                , m_CurrentLine(cl)
                , m_CallType(calltype)
                , m_LocalVars(lv)
                , m_UpVars(uv)
            {
            }
        };

        // Architect side of the equation.

        class ILuaArchitectDebugClient;

        class ILuaProcessInfo
        {
        protected:
            virtual ~ILuaProcessInfo() {}
        public:
            // Only valid while broken.  The stack trace is from top down, retval[0] is the
            // currently
            // executing function...
            virtual NVConstDataRef<SLuaStackInfo> StackTrace(QT3DSI32 limit = 100) = 0;
            virtual SFunctionInfo GetFunctionInfo(SDatamodelFunction &inFunction) = 0;
            virtual NVConstDataRef<STableEntry> GetTableValues(SDatamodelTable &inTable) = 0;
        };

        class ILuaArchitectDebugClientListener : public NVRefCounted
        {
        public:
            virtual void OnConnect(ILuaArchitectDebugClient &inClient) = 0;
            // Do not hold onto the process info object.  Getting process info only makes sense when
            // the process is stopped, there is no guarantee
            // the process info object will be valid or the same in between breakpoint calls.
            virtual void OnClientBreak(ILuaArchitectDebugClient &inClient,
                                       const SLuaBreakpoint &inBreakpoint) = 0;
            virtual void OnWatchUpdated(ILuaArchitectDebugClient &inClient,
                                        CRegisteredString inExpression,
                                        const SDatamodelValue &value) = 0;
            virtual void OnStatementExecuted(ILuaArchitectDebugClient &inClient,
                                             CRegisteredString inExpression,
                                             const eastl::string &inErrorMsg,
                                             const eastl::vector<eastl::string> &inPrintStatements,
                                             const eastl::vector<SDatamodelValue> &inResults) = 0;
            // Gets called for both step and step-into
            virtual void OnDisconnect(ILuaArchitectDebugClient &inClient) = 0;
        };

        // Interface to running lua system.
        class ILuaArchitectDebugClient : public NVRefCounted
        {
        public:
            virtual void SetBreakpoint(const char *inFileRelativePath, int inLine,
                                       const char *inCondition) = 0;
            virtual NVConstDataRef<SLuaBreakpoint> GetBreakpoints() = 0;
            virtual void ClearBreakpoint(const char *inFileRelativePath, int inLine) = 0;
            virtual void ClearAllBreakpoints() = 0;
            virtual void RequestBreak() = 0;
            virtual bool IsPaused() = 0;
            // May return NULL!!
            // Returns NULL when IsPaused() == false
            virtual ILuaProcessInfo *GetProcessInfo() = 0;
            virtual void Continue() = 0;
            virtual void StepInto() = 0;
            virtual void StepOver() = 0;
            virtual void StepOut() = 0;
            virtual void AddWatch(const char *inExpression) = 0;
            virtual void RemoveWatch(const char *inExpression) = 0;
            virtual void ExecuteStatement(const char *inExpression) = 0;
        };

        // Lua debug system that sits in architect, created when

        class ILuaArchitectDebugServer : public NVRefCounted
        {
        public:
            virtual void SetListener(ILuaArchitectDebugClientListener &inListener) = 0;
            static const char *LuaProtocolName() { return "LuaDebugger"; }
            static ILuaArchitectDebugServer &CreateLuaServer(IDebugOutStream &outStream);
        };

        // Implementation objects used to keept track of lua state.
        class ILuaRuntimeDataCache : public NVRefCounted
        {
        protected:
            virtual ~ILuaRuntimeDataCache() {}
        public:
            // Cache the value at the current stack position.
            // Note that value types aren't cached, just tables and functions at this point.
            // This may generate a (possibly large) set of messages to the underlying stream.
            // In the future it would be awesome if this object hooked into the lua system's garbage
            // collection
            // write barrier to automatically update table values as they actually change.
            virtual void BeginUpdate() = 0;
            virtual SDatamodelValue CacheValue(lua_State &state, QT3DSI32 stackIdx = -1) = 0;
            virtual void EndUpdate(lua_State &state) = 0;

            virtual bool IsCacheMessage(lua::SLuaDebugMessageHeader &header) = 0;
            virtual void HandleCacheMessage(lua_State *state,
                                            lua::SLuaDebuggerProtocolReader &reader,
                                            lua::SLuaDebugMessageHeader &header) = 0;
            static ILuaRuntimeDataCache &Create(NVFoundationBase &fnd, IStringTable &strTable,
                                                IDebugOutStream &stream, QT3DSU64 clientId,
                                                const char *inProjectDir);
        };

        class ILuaArchitectDataCacheListener
        {
        protected:
            virtual ~ILuaArchitectDataCacheListener() {}
        public:
            virtual void HandleMessage(lua::SLuaDebuggerProtocolReader &reader,
                                       lua::SLuaDebugMessageHeader &header) = 0;
        };

        class ILuaArchitectDataCache : public NVRefCounted
        {
        protected:
            virtual ~ILuaArchitectDataCache() {}
        public:
            virtual SFunctionInfo GetFunctionInfo(SDatamodelFunction &inFunction) = 0;
            virtual NVConstDataRef<STableEntry> GetTableValues(SDatamodelTable &inTable) = 0;

            // Allow transparent handling of cache protocol.
            virtual bool IsCacheMessage(lua::SLuaDebugMessageHeader &header) = 0;
            virtual void HandleCacheMessage(lua::SLuaDebuggerProtocolReader &reader,
                                            lua::SLuaDebugMessageHeader &header) = 0;

            static ILuaArchitectDataCache &Create(NVFoundationBase &fnd, IStringTable &strTable,
                                                  IDebugOutStream &stream,
                                                  ILuaArchitectDataCacheListener &listener,
                                                  QT3DSU64 inClientId);
        };
    }
}
}

#endif