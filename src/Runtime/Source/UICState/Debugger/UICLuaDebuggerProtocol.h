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
#ifndef UIC_LUA_DEBUGGER_PROTOCOL_H
#define UIC_LUA_DEBUGGER_PROTOCOL_H
#include "UICLuaDebugger.h"
#include "foundation/Qt3DSTime.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "foundation/SerializationTypes.h"

namespace uic {
namespace state {
    namespace debugger {
        namespace lua {

#define UIC_LUA_DEBUGGER_ITERATE_PROTOCOL_MESSAGES                                                 \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(Initialization)                                       \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(SetBreakpoint)                                        \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(RequestBreak)                                         \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(RequestAllBreakpoints)                                \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(AllBreakpoints)                                       \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(ClearBreakpoint)                                      \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(ClearAllBreakpoints)                                  \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(Continue)                                             \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(OnBreak)                                              \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(UpdateTable)                                          \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(UpdateFunction)                                       \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(RequestStackTrace)                                    \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(StackTrace)                                           \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(StepInto)                                             \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(StepOver)                                             \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(StepOut)                                              \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(AddWatch)                                             \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(RemoveWatch)                                          \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(WatchUpdated)                                         \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(ExecuteStatement)                                     \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(StatementExecuted)                                    \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(RequestPointerValue)                                  \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(UnknownPointerValue)                                  \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(EndPointerRequest)                                    \
    UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(BeginCacheUpdate)

            struct SProtocolMessageNames
            {
                enum Enum {
                    UnknownMessageName = 0,
#define UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(name) name,
                    UIC_LUA_DEBUGGER_ITERATE_PROTOCOL_MESSAGES
#undef UIC_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE
                };
            };

            struct SLuaDebugMessageHeader
            {
                SProtocolMessageNames::Enum m_MessageName;
                QT3DSU32 m_Padding;
                QT3DSU64 m_ClientId;
                SLuaDebugMessageHeader(
                    SProtocolMessageNames::Enum name = SProtocolMessageNames::UnknownMessageName,
                    QT3DSU64 cid = 0)
                    : m_MessageName(name)
                    , m_Padding(0)
                    , m_ClientId(cid)
                {
                }
                bool operator==(const SLuaDebugMessageHeader &inOther) const
                {
                    return m_MessageName == inOther.m_MessageName
                        && m_ClientId == inOther.m_ClientId;
                }
            };

            template <typename TDType>
            struct SMessageTypeToNameMap
            {
            };

            typedef eastl::vector<SLuaBreakpoint> TBreakpointList;

            inline QT3DSU32 CurrentDebuggerProtocolVersion() { return 1; }

            struct STableData
            {
                SDatamodelTable *m_Table;
                NVConstDataRef<STableEntry> m_TableData;
                STableData()
                    : m_Table(NULL)
                {
                }
                STableData(SDatamodelTable &t, NVConstDataRef<STableEntry> td)
                    : m_Table(&t)
                    , m_TableData(td)
                {
                }
                bool operator==(const STableData &other) const { return m_Table == other.m_Table; }
            };

            struct SFunctionInfo
            {
                SDatamodelFunction *m_Function;
                SFileAndLine m_DefinitionLocation;
                SDatamodelTable *m_Environment;
                SFunctionInfo()
                    : m_Function(NULL)
                    , m_Environment(NULL)
                {
                }

                SFunctionInfo(SDatamodelFunction &fn, SFileAndLine loc, SDatamodelTable &env)
                    : m_Function(&fn)
                    , m_DefinitionLocation(loc)
                    , m_Environment(&env)
                {
                }

                bool operator==(const SFunctionInfo &other) const
                {
                    return m_Function == other.m_Function
                        && m_DefinitionLocation == other.m_DefinitionLocation
                        && m_Environment == other.m_Environment;
                }
                bool operator!=(const SFunctionInfo &other) const { return !(operator==(other)); }
            };

            struct SStackTraceData
            {
                NVConstDataRef<SLuaStackInfo> m_Stack;
                SStackTraceData() {}
                SStackTraceData(NVConstDataRef<SLuaStackInfo> st)
                    : m_Stack(st)
                {
                }
                bool operator==(const SStackTraceData &) const { return false; }
            };

            struct SWatchUpdateData
            {
                CRegisteredString m_Expression;
                SDatamodelValue m_Value;
                SWatchUpdateData(CRegisteredString exp, const SDatamodelValue &val)
                    : m_Expression(exp)
                    , m_Value(val)
                {
                }
                SWatchUpdateData() {}
                bool operator==(const SWatchUpdateData &other) const
                {
                    return m_Expression == other.m_Expression && m_Value == other.m_Value;
                }
            };

            struct SStatementExecuteData
            {
                CRegisteredString m_Expression;
                eastl::string m_Error;
                eastl::vector<eastl::string> m_PrintStatements;
                eastl::vector<SDatamodelValue> m_Values;
                SStatementExecuteData() {}
                SStatementExecuteData(CRegisteredString expr, const eastl::string &errorMsg,
                                      const eastl::vector<eastl::string> &inPrintStatements,
                                      const eastl::vector<SDatamodelValue> &inExecResults)
                    : m_Expression(expr)
                    , m_Error(errorMsg)
                    , m_PrintStatements(inPrintStatements)
                    , m_Values(inExecResults)
                {
                }
                bool operator==(const SStatementExecuteData &other) const
                {
                    return m_Expression == other.m_Expression && m_Error == other.m_Error
                        && m_PrintStatements == other.m_PrintStatements
                        && m_Values == other.m_Values;
                }
            };

#define UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(msgName)                                 \
    struct S##msgName                                                                              \
    {                                                                                              \
        S##msgName() {}                                                                            \
        ~S##msgName() {}                                                                           \
        template <typename TVisitor>                                                               \
        void Visit(TVisitor &)                                                                     \
        {                                                                                          \
        }                                                                                          \
        bool operator==(const SInitialization &) const { return true; }                            \
    };                                                                                             \
    template <>                                                                                    \
    struct SMessageTypeToNameMap<S##msgName>                                                       \
    {                                                                                              \
        enum { MessageName = SProtocolMessageNames::msgName };                                     \
    };

#define UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(msgName, ptype, pname)                         \
    struct S##msgName                                                                              \
    {                                                                                              \
        ptype m_##pname;                                                                           \
        S##msgName(const ptype &in_##pname = ptype())                                              \
            : m_##pname(in_##pname)                                                                \
        {                                                                                          \
        }                                                                                          \
        ~S##msgName() {}                                                                           \
        template <typename TVisitor>                                                               \
        void Visit(TVisitor &inVisitor)                                                            \
        {                                                                                          \
            inVisitor.visit(m_##pname);                                                            \
        }                                                                                          \
        bool operator==(const S##msgName &other) const { return m_##pname == other.m_##pname; }    \
    };                                                                                             \
    template <>                                                                                    \
    struct SMessageTypeToNameMap<S##msgName>                                                       \
    {                                                                                              \
        enum { MessageName = SProtocolMessageNames::msgName };                                     \
    };

            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(Initialization, QT3DSU32, Version);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(SetBreakpoint, SLuaBreakpoint, Breakpoint);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(RequestAllBreakpoints);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(RequestBreak);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(AllBreakpoints, TBreakpointList,
                                                        Breakpoints);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(ClearBreakpoint, SLuaBreakpoint,
                                                        Breakpoint);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(ClearAllBreakpoints);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(Continue);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(OnBreak, SFileAndLine, Position);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(UpdateTable, STableData, Data);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(UpdateFunction, SFunctionInfo, Data);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(RequestStackTrace);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(StackTrace, SStackTraceData, Data);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(StepInto);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(StepOver);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(StepOut);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(AddWatch, CRegisteredString, Expression);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(RemoveWatch, CRegisteredString, Expression);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(WatchUpdated, SWatchUpdateData, Data);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(ExecuteStatement, CRegisteredString,
                                                        Expression);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(StatementExecuted, SStatementExecuteData,
                                                        Data);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_MESSAGE_1(RequestPointerValue, SDatamodelValue, Data);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(UnknownPointerValue);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(EndPointerRequest);
            UIC_LUA_DEBUGGER_PROTOCOL_DECLARE_TRIVIAL_MESSAGE(BeginCacheUpdate);

            struct SLuaDebuggerProtocolWriter
            {
                IOutStream &m_Stream;
                MemoryBuffer<> m_WriteBuffer;
                SLuaDebuggerProtocolWriter(IOutStream &s, NVAllocatorCallback &alloc)
                    : m_Stream(s)
                    , m_WriteBuffer(ForwardingAllocator(alloc, "LuaWriteBuffer"))
                {
                }
                void visit(QT3DSU32 data) { m_WriteBuffer.write(data); }

                template <typename TDataType>
                void visitStr(const TDataType &str)
                {
                    QT3DSU32 len = (QT3DSU32)strlen(str.c_str());
                    if (len)
                        ++len;
                    m_WriteBuffer.write(len);
                    // include the null character.  This allows far easier reading of the stream
                    // from a memory buffer read from the stream.
                    if (len)
                        m_WriteBuffer.write(str.c_str(), len);
                }

                void visit(const TDebugStr &str) { visitStr(str); }

                void visit(CRegisteredString str) { visitStr(str); }

                template <typename TDatatype>
                static QT3DSU64 ToU64(TDatatype *ptr)
                {
                    return static_cast<QT3DSU64>(reinterpret_cast<size_t>(ptr));
                }

                void visit(SDatamodelTable *table) { m_WriteBuffer.write(ToU64(table)); }

                void visit(SDatamodelFunction *table) { m_WriteBuffer.write(ToU64(table)); }

                void visit(const SDatamodelValue &value)
                {
                    QT3DSU32 type = (QT3DSU32)value.getType();
                    visit(type);
                    switch (type) {
                    case DatamodelValueTypes::UnknownType:
                    case DatamodelValueTypes::Nil:
                        break;
                    case DatamodelValueTypes::Boolean:
                        m_WriteBuffer.write(value.getData<bool>());
                        break;
                    case DatamodelValueTypes::Number:
                        m_WriteBuffer.write(value.getData<float>());
                        break;
                    case DatamodelValueTypes::String:
                        visit(value.getData<TDebugStr>());
                        break;
                    case DatamodelValueTypes::Table:
                        visit(value.getData<SDatamodelTable *>());
                        break;
                    case DatamodelValueTypes::UserData:
                        m_WriteBuffer.write(ToU64(value.getData<SDatamodelUserData *>()));
                        break;
                    case DatamodelValueTypes::Function:
                        visit(value.getData<SDatamodelFunction *>());
                        break;
                    case DatamodelValueTypes::CFunction:
                        m_WriteBuffer.write(ToU64(value.getData<SDatamodelCFunction *>()));
                        break;
                    case DatamodelValueTypes::Thread:
                        m_WriteBuffer.write(ToU64(value.getData<SDatamodelThread *>()));
                        break;
                    }
                }

                void visit(const SFileAndLine &data)
                {
                    visit(data.m_File);
                    m_WriteBuffer.write(data.m_Line);
                }

                void visit(const STableEntry &data)
                {
                    visit(data.m_Key);
                    visit(data.m_Value);
                }

                void visit(NVConstDataRef<STableEntry> data)
                {
                    m_WriteBuffer.write((QT3DSU32)data.size());
                    for (QT3DSU32 idx = 0, end = (QT3DSU32)data.size(); idx < end; ++idx) {
                        visit(data[idx]);
                    }
                }

                void visit(const STableData &data)
                {
                    visit(data.m_Table);
                    visit(data.m_TableData);
                }

                void visit(const SFunctionInfo &data)
                {
                    visit(data.m_Function);
                    visit(data.m_Environment);
                    visit(data.m_DefinitionLocation);
                }

                void visit(const SLuaBreakpoint &data)
                {
                    visit(static_cast<const SFileAndLine &>(data));
                    visit(data.m_Condition);
                }

                void visit(const TBreakpointList &data)
                {
                    QT3DSU32 len = (QT3DSU32)data.size();
                    visit(len);
                    for (QT3DSU32 idx = 0, end = data.size(); idx < end; ++idx)
                        visit(data[idx]);
                }

                void visit(const SLuaStackInfo &data)
                {
                    visit(data.m_Function);
                    visit(data.m_CurrentLine);
                    visit(data.m_CallType);
                    visit(data.m_LocalVars);
                    visit(data.m_UpVars);
                }

                void visit(NVConstDataRef<SLuaStackInfo> data)
                {
                    QT3DSU32 len = data.size();
                    visit(len);
                    for (QT3DSU32 idx = 0, end = len; idx < end; ++idx) {
                        visit(data[idx]);
                    }
                }

                void visit(const SStackTraceData &data) { visit(data.m_Stack); }

                void visit(const SWatchUpdateData &data)
                {
                    visit(data.m_Expression);
                    visit(data.m_Value);
                }

                template <typename TVecData>
                void visitVector(const eastl::vector<TVecData> &inVec)
                {
                    QT3DSU32 len = (QT3DSU32)inVec.size();
                    visit(len);
                    for (QT3DSU32 idx = 0, end = inVec.size(); idx < end; ++idx) {
                        visit(inVec[idx]);
                    }
                }

                void visit(const SStatementExecuteData &data)
                {
                    visit(data.m_Expression);
                    visit(data.m_Error);
                    visitVector(data.m_PrintStatements);
                    visitVector(data.m_Values);
                }

                template <typename TMsgType>
                void WriteMessage(QT3DSU64 clientId, TMsgType &msg)
                {
                    SLuaDebugMessageHeader theHeader(
                        static_cast<SProtocolMessageNames::Enum>(
                            SMessageTypeToNameMap<TMsgType>::MessageName),
                        clientId);
                    m_WriteBuffer.write(theHeader);
                    msg.Visit(*this);
                    Flush();
                };

                void Flush()
                {
                    if (m_WriteBuffer.size()) {
                        m_Stream.Write((NVConstDataRef<QT3DSU8>)m_WriteBuffer);
                        m_WriteBuffer.clear();
                    }
                }
            };

            struct STempStackInfo
            {
                SDatamodelValue m_Function;
                QT3DSI32 m_CurrentLine;
                CRegisteredString m_CallType;
                eastl::vector<STableEntry> m_LocalVars;
                eastl::vector<STableEntry> m_UpVars;
                STempStackInfo()
                    : m_CurrentLine(0)
                {
                }
                STempStackInfo(const SLuaStackInfo &info)
                    : m_Function(info.m_Function)
                    , m_CurrentLine(info.m_CurrentLine)
                    , m_CallType(info.m_CallType)
                {

                    m_LocalVars.assign(info.m_LocalVars.begin(), info.m_LocalVars.end());
                    m_UpVars.assign(info.m_UpVars.begin(), info.m_UpVars.end());
                }
                operator SLuaStackInfo() const
                {
                    return SLuaStackInfo(
                        m_Function, m_CurrentLine, m_CallType,
                        toConstDataRef(m_LocalVars.data(), (QT3DSU32)m_LocalVars.size()),
                        toConstDataRef(m_UpVars.data(), (QT3DSU32)m_UpVars.size()));
                }
            };

            struct SLuaDebuggerProtocolReader
            {
                IStringTable &m_StringTable;
                TDebugStr m_TempStr;
                SDataReader m_Reader;
                eastl::vector<STableEntry> m_TableUpdateList;
                eastl::vector<STempStackInfo> m_StackInfo;
                eastl::vector<SLuaStackInfo> m_OutgoingInfo;

                SLuaDebuggerProtocolReader(NVConstDataRef<QT3DSU8> msg, IStringTable &strTable)
                    : m_StringTable(strTable)
                    , m_Reader(const_cast<QT3DSU8 *>(msg.begin()), const_cast<QT3DSU8 *>(msg.end()))
                {
                }

                bool HasData() const { return m_Reader.m_CurrentPtr < m_Reader.m_EndPtr; }

                SLuaDebugMessageHeader ReadHeader()
                {
                    return m_Reader.LoadRef<SLuaDebugMessageHeader>();
                }

                void visit(QT3DSU32 &data) { data = m_Reader.LoadRef<QT3DSU32>(); }

                void visit(SFileAndLine &data)
                {
                    visit(data.m_File);
                    data.m_Line = m_Reader.LoadRef<QT3DSU32>();
                }

                void visit(SLuaBreakpoint &data)
                {
                    visit(static_cast<SFileAndLine &>(data));
                    visit(data.m_Condition);
                }

                void visit(TBreakpointList &data)
                {
                    QT3DSU32 len = m_Reader.LoadRef<QT3DSU32>();
                    data.resize(len);
                    for (QT3DSU32 idx = 0, end = data.size(); idx < end; ++idx)
                        visit(data[idx]);
                }

                template <typename Datatype>
                Datatype *visitOpaquePtr()
                {
                    QT3DSU64 data = m_Reader.LoadRef<QT3DSU64>();
                    return reinterpret_cast<Datatype *>(static_cast<size_t>(data));
                }

                void visit(SDatamodelTable *&item) { item = visitOpaquePtr<SDatamodelTable>(); }

                void visit(SDatamodelFunction *&item)
                {
                    item = visitOpaquePtr<SDatamodelFunction>();
                }

                void visit(SDatamodelValue &item)
                {
                    QT3DSU32 dataType = m_Reader.LoadRef<QT3DSU32>();
                    switch (dataType) {
                    case DatamodelValueTypes::UnknownType:
                        item = SDatamodelValue();
                        break;
                    case DatamodelValueTypes::Nil:
                        item = SDatamodelValue(SNil());
                        break;
                    case DatamodelValueTypes::Boolean:
                        item = SDatamodelValue(m_Reader.LoadRef<bool>());
                        break;
                    case DatamodelValueTypes::Number:
                        item = SDatamodelValue(m_Reader.LoadRef<float>());
                        break;
                    case DatamodelValueTypes::String: {
                        TDebugStr temp;
                        visit(temp);
                        item = SDatamodelValue(temp);
                    } break;
                    case DatamodelValueTypes::Table:
                        item = SDatamodelValue(visitOpaquePtr<SDatamodelTable>());
                        break;
                    case DatamodelValueTypes::UserData:
                        item = SDatamodelValue(visitOpaquePtr<SDatamodelUserData>());
                        break;
                    case DatamodelValueTypes::Function:
                        item = SDatamodelValue(visitOpaquePtr<SDatamodelFunction>());
                        break;
                    case DatamodelValueTypes::CFunction:
                        item = SDatamodelValue(visitOpaquePtr<SDatamodelCFunction>());
                        break;
                    case DatamodelValueTypes::Thread:
                        item = SDatamodelValue(visitOpaquePtr<SDatamodelThread>());
                        break;
                    default:
                        // probably going off the rails here.
                        QT3DS_ASSERT(false);
                        break;
                    }
                }

                void visit(TDebugStr &item)
                {
                    QT3DSU32 len = m_Reader.LoadRef<QT3DSU32>();
                    if (len) {
                        const char *strData = (const char *)m_Reader.m_CurrentPtr;
                        m_Reader.m_CurrentPtr += len;
                        item.assign(strData, strData + len);
                    }
                }

                void visit(CRegisteredString &item)
                {
                    QT3DSU32 len = m_Reader.LoadRef<QT3DSU32>();
                    if (len) {
                        const char *strData = (const char *)m_Reader.m_CurrentPtr;
                        m_Reader.m_CurrentPtr += len;
                        item = m_StringTable.RegisterStr(strData);
                    }
                }

                void visit(QT3DSI32 &item) { item = m_Reader.LoadRef<QT3DSI32>(); }

                void visit(STableEntry &item)
                {
                    visit(item.m_Key);
                    visit(item.m_Value);
                }

                void visit(NVConstDataRef<STableEntry> &item)
                {
                    m_TableUpdateList.clear();
                    QT3DSU32 count = m_Reader.LoadRef<QT3DSU32>();
                    m_TableUpdateList.resize(count);
                    for (QT3DSU32 idx = 0, end = count; idx < end; ++idx)
                        visit(m_TableUpdateList[idx]);

                    item =
                        toConstDataRef(m_TableUpdateList.data(), (QT3DSU32)m_TableUpdateList.size());
                }

                void visit(lua::STableData &data)
                {
                    visit(data.m_Table);
                    visit(data.m_TableData);
                }

                void visit(lua::SFunctionInfo &item)
                {
                    visit(item.m_Function);
                    visit(item.m_Environment);
                    visit(item.m_DefinitionLocation);
                }

                void visit(eastl::vector<STableEntry> &item)
                {
                    QT3DSU32 len = m_Reader.LoadRef<QT3DSU32>();
                    if (len) {
                        item.resize(len);
                        for (QT3DSU32 idx = 0, end = len; idx < end; ++idx)
                            visit(item[idx]);
                    }
                }

                void visit(STempStackInfo &info)
                {
                    visit(info.m_Function);
                    visit(info.m_CurrentLine);
                    visit(info.m_CallType);
                    visit(info.m_LocalVars);
                    visit(info.m_UpVars);
                }

                void visit(NVConstDataRef<SLuaStackInfo> &item)
                {
                    m_StackInfo.clear();
                    QT3DSU32 len = m_Reader.LoadRef<QT3DSU32>();
                    m_StackInfo.resize(len);
                    for (QT3DSU32 idx = 0, end = len; idx < end; ++idx) {
                        visit(m_StackInfo[idx]);
                    }

                    m_OutgoingInfo.assign(m_StackInfo.begin(), m_StackInfo.end());
                    item = toConstDataRef(m_OutgoingInfo.data(), (QT3DSU32)m_OutgoingInfo.size());
                }

                void visit(lua::SStackTraceData &data) { visit(data.m_Stack); }

                void visit(SWatchUpdateData &data)
                {
                    visit(data.m_Expression);
                    visit(data.m_Value);
                }

                template <typename TVecData>
                void visitVec(eastl::vector<TVecData> &inVec)
                {
                    QT3DSU32 len = 0;
                    visit(len);
                    inVec.resize(len);
                    for (QT3DSU32 idx = 0, end = len; idx < end; ++idx) {
                        visit(inVec[idx]);
                    }
                }

                void visit(SStatementExecuteData &data)
                {
                    visit(data.m_Expression);
                    visit(data.m_Error);
                    visitVec(data.m_PrintStatements);
                    visitVec(data.m_Values);
                }

                template <typename TMsgType>
                TMsgType ReadMessage()
                {
                    TMsgType retval;
                    retval.Visit(*this);
                    return retval;
                }
            };
        }
    }
}
}
#endif