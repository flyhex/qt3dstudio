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
#include "Qt3DSStateEditorFoundation.h"
#include "Qt3DSLuaDebuggerProtocol.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSLuaDebuggerImpl.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/ErrorStream.h"
#include "foundation/StringTable.h"

using namespace qt3ds::foundation;
using namespace uic::state::debugger;
using namespace uic::state::debugger::lua;
using namespace qt3ds;

namespace {

struct IDebugClientOwner : public ILuaArchitectDataCacheListener
{
protected:
    virtual ~IDebugClientOwner() {}
public:
    virtual Option<SAllBreakpoints> WaitForAllBreakpoints(QT3DSU64 inClientId) = 0;
};
struct SDebugClient : public ILuaArchitectDebugClient,
                      public ILuaProcessInfo,
                      public ILuaArchitectDataCacheListener
{
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    QT3DSU64 m_ClientId;
    SLuaDebuggerProtocolWriter m_ProtocolWriter;
    IDebugClientOwner &m_Owner;
    lua::TBreakpointList m_BreakpointList;
    NVScopedRefCounted<ILuaArchitectDataCache> m_DataCache;
    eastl::vector<STempStackInfo> m_LastStackTrace;
    eastl::vector<SLuaStackInfo> m_OutgoingStackTrace;
    bool m_IsPaused;

    QT3DSI32 mRefCount;

    SDebugClient(NVFoundationBase &fnd, IStringTable &strTable, QT3DSU64 cid, IDebugOutStream &stream,
                 IDebugClientOwner &outerListener)
        : m_Foundation(fnd)
        , m_StringTable(strTable)
        , m_ClientId(cid)
        , m_ProtocolWriter(stream, fnd.getAllocator())
        , m_Owner(outerListener)
        , m_IsPaused(true) // The client starts paused.
        , mRefCount(0)
    {
        m_DataCache =
            ILuaArchitectDataCache::Create(m_Foundation, m_StringTable, stream, *this, m_ClientId);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator());

    template <typename TMsgType>
    void WriteMessage(const TMsgType &msg)
    {
        m_ProtocolWriter.WriteMessage(ToClientId(), const_cast<TMsgType &>(msg));
    }

    QT3DSU64 ToClientId() const { return m_ClientId; }

    virtual void SetBreakpoint(const char *relativePath, int line, const char *inCondition)
    {
        SSetBreakpoint theMessage(SLuaBreakpoint(m_StringTable.RegisterStr(relativePath), line,
                                                 m_StringTable.RegisterStr(nonNull(inCondition))));
        WriteMessage(theMessage);
    }

    virtual NVConstDataRef<SLuaBreakpoint> GetBreakpoints()
    {
        SRequestAllBreakpoints theMessage;
        WriteMessage(theMessage);
        Option<SAllBreakpoints> retval = m_Owner.WaitForAllBreakpoints(ToClientId());
        m_BreakpointList.clear();
        if (retval.hasValue())
            m_BreakpointList = retval->m_Breakpoints;
        return toConstDataRef(m_BreakpointList.data(), m_BreakpointList.size());
    }

    virtual void ClearBreakpoint(const char *relativePath, int line)
    {
        SClearBreakpoint msg(SLuaBreakpoint(m_StringTable.RegisterStr(relativePath), line));
        WriteMessage(msg);
    }

    virtual void ClearAllBreakpoints() { WriteMessage(SClearAllBreakpoints()); }

    // Process info
    // limit ignored for now
    virtual NVConstDataRef<SLuaStackInfo> StackTrace(QT3DSI32)
    {
        m_OutgoingStackTrace.assign(m_LastStackTrace.begin(), m_LastStackTrace.end());
        return toConstDataRef(m_OutgoingStackTrace.data(), (QT3DSU32)m_OutgoingStackTrace.size());
    }

    virtual uic::state::debugger::SFunctionInfo GetFunctionInfo(SDatamodelFunction &inFunction)
    {
        return m_DataCache->GetFunctionInfo(inFunction);
    }
    virtual NVConstDataRef<STableEntry> GetTableValues(SDatamodelTable &inTable)
    {
        return m_DataCache->GetTableValues(inTable);
    }

    virtual void RequestBreak() { WriteMessage(SRequestBreak()); }

    virtual bool IsPaused() { return m_IsPaused; }

    virtual ILuaProcessInfo *GetProcessInfo()
    {
        if (m_IsPaused)
            return this;
        return NULL;
    }

    virtual void Continue()
    {
        WriteMessage(SContinue());
        m_IsPaused = false;
    }

    virtual void StepInto() { WriteMessage(SStepInto()); }

    virtual void StepOver() { WriteMessage(SStepOver()); }

    virtual void StepOut() { WriteMessage(SStepOut()); }

    virtual void AddWatch(const char *inExpression)
    {
        WriteMessage(SAddWatch(m_StringTable.RegisterStr(inExpression)));
    }

    virtual void RemoveWatch(const char *inExpression)
    {
        WriteMessage(SRemoveWatch(m_StringTable.RegisterStr(inExpression)));
    }

    virtual void ExecuteStatement(const char *inExpression)
    {
        WriteMessage(SExecuteStatement(m_StringTable.RegisterStr(inExpression)));
    }

    virtual void HandleMessage(lua::SLuaDebuggerProtocolReader &reader,
                               lua::SLuaDebugMessageHeader &header)
    {
        if (header.m_ClientId == ToClientId() && m_DataCache->IsCacheMessage(header)) {
            m_DataCache->HandleCacheMessage(reader, header);
        } else {
            m_Owner.HandleMessage(reader, header);
        }
    }
};

static MallocAllocator g_MallocAlloc;

struct SDebugServer : public ILuaArchitectDebugServer,
                      public IDebugStreamListener,
                      public IDebugClientOwner
{
    typedef eastl::hash_map<QT3DSU64, NVScopedRefCounted<SDebugClient>> TIdClientHash;

    SErrorStream m_Errors;
    CAllocator m_Allocator;
    NVScopedRefCounted<NVFoundation> m_Foundation;
    NVScopedRefCounted<IStringTable> m_StrTable;
    TIdClientHash m_Clients;
    NVScopedRefCounted<ILuaArchitectDebugClientListener> m_Listener;
    NVScopedRefCounted<IDebugOutStream> m_OutStream;
    QT3DSI32 mRefCount;

    SDebugServer(IDebugOutStream &stream)
        : m_Allocator(m_Errors)
        , m_Foundation(NVCreateFoundation(QT3DS_FOUNDATION_VERSION, m_Allocator, m_Errors))
        , m_StrTable(IStringTable::CreateStringTable(m_Foundation->getAllocator()))
        , m_OutStream(stream)
        , mRefCount(0)
    {
        m_OutStream->SetListener(this);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(g_MallocAlloc);

    virtual void SetListener(ILuaArchitectDebugClientListener &inListener)
    {
        m_Listener = &inListener;
        for (TIdClientHash::iterator iter = m_Clients.begin(), end = m_Clients.end(); iter != end;
             ++iter) {
            m_Listener->OnConnect(*iter->second);
        }
    }

    SDebugClient *FindClient(QT3DSU64 id)
    {
        TIdClientHash::iterator iter = m_Clients.find(id);
        if (iter != m_Clients.end())
            return iter->second.mPtr;
        return NULL;
    }

#define SERVER_IGNORE_MESSAGE(msgname)                                                             \
    void Handle##msgname(QT3DSU64, const lua::S##msgname &) { QT3DS_ASSERT(false); }
    SERVER_IGNORE_MESSAGE(SetBreakpoint);
    SERVER_IGNORE_MESSAGE(RequestAllBreakpoints);
    SERVER_IGNORE_MESSAGE(RequestBreak);
    SERVER_IGNORE_MESSAGE(ClearBreakpoint);
    SERVER_IGNORE_MESSAGE(ClearAllBreakpoints);
    SERVER_IGNORE_MESSAGE(Continue);
    SERVER_IGNORE_MESSAGE(AllBreakpoints);
    SERVER_IGNORE_MESSAGE(RequestStackTrace)
    SERVER_IGNORE_MESSAGE(UpdateTable);
    SERVER_IGNORE_MESSAGE(UpdateFunction);
    SERVER_IGNORE_MESSAGE(StepInto);
    SERVER_IGNORE_MESSAGE(StepOver);
    SERVER_IGNORE_MESSAGE(StepOut);
    SERVER_IGNORE_MESSAGE(AddWatch);
    SERVER_IGNORE_MESSAGE(RemoveWatch);
    SERVER_IGNORE_MESSAGE(ExecuteStatement);
    SERVER_IGNORE_MESSAGE(RequestPointerValue);
    SERVER_IGNORE_MESSAGE(UnknownPointerValue);
    SERVER_IGNORE_MESSAGE(BeginCacheUpdate);
    SERVER_IGNORE_MESSAGE(EndPointerRequest);

    void HandleInitialization(QT3DSU64 clientId, const lua::SInitialization &)
    {
        SDebugClient *theClient = QT3DS_NEW(m_Foundation->getAllocator(), SDebugClient)(
            *m_Foundation, *m_StrTable, clientId, *m_OutStream, *this);
        m_Clients.insert(eastl::make_pair(clientId, theClient));
        if (m_Listener)
            m_Listener->OnConnect(*theClient);
    }

    void HandleOnBreak(QT3DSU64 clientId, const lua::SOnBreak &msg)
    {
        SDebugClient *client = FindClient(clientId);
        if (client && m_Listener) {
            client->m_IsPaused = true;
            m_Listener->OnClientBreak(*client,
                                      SLuaBreakpoint(msg.m_Position.m_File, msg.m_Position.m_Line));
        }
    }

    void HandleStackTrace(QT3DSU64 clientId, const lua::SStackTrace &msg)
    {
        SDebugClient *client = FindClient(clientId);
        if (client)
            client->m_LastStackTrace.assign(msg.m_Data.m_Stack.begin(), msg.m_Data.m_Stack.end());
    }

    void HandleWatchUpdated(QT3DSU64 clientId, const lua::SWatchUpdated &msg)
    {
        SDebugClient *client = FindClient(clientId);
        if (client && m_Listener) {
            m_Listener->OnWatchUpdated(*client, msg.m_Data.m_Expression, msg.m_Data.m_Value);
        }
    }

    void HandleStatementExecuted(QT3DSU64 clientId, const lua::SStatementExecuted &msg)
    {
        SDebugClient *client = FindClient(clientId);
        if (client && m_Listener) {
            m_Listener->OnStatementExecuted(*client, msg.m_Data.m_Expression, msg.m_Data.m_Error,
                                            msg.m_Data.m_PrintStatements, msg.m_Data.m_Values);
        }
    }

    void HandleNextMessage(SLuaDebuggerProtocolReader &inReader, SLuaDebugMessageHeader &inHeader)
    {
        SDebugClient *client = FindClient(inHeader.m_ClientId);
        if (client && client->m_DataCache->IsCacheMessage(inHeader)) {
            client->m_DataCache->HandleCacheMessage(inReader, inHeader);
        } else {
            switch (inHeader.m_MessageName) {
#define QT3DS_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE(msgname)                                          \
    case SProtocolMessageNames::msgname:                                                           \
        Handle##msgname(inHeader.m_ClientId, inReader.ReadMessage<lua::S##msgname>());             \
        break;
                QT3DS_LUA_DEBUGGER_ITERATE_PROTOCOL_MESSAGES;
#undef QT3DS_LUA_DEBUGGER_HANDLE_PROTOCOL_MESSAGE
            }
        }
    }

    virtual void OnMessageReceived(const SDebugStreamMessage &inMessage)
    {
        SLuaDebuggerProtocolReader theReader(inMessage.m_Data, *m_StrTable);
        while (theReader.HasData()) {
            SLuaDebugMessageHeader theHeader = theReader.ReadHeader();
            HandleNextMessage(theReader, theHeader);
        }
    }

    virtual Option<SAllBreakpoints> WaitForAllBreakpoints(QT3DSU64 inClientId)
    {
        while (m_OutStream->Connected()) {
            SDebugStreamMessage theMessage = m_OutStream->WaitForNextMessage();
            if (theMessage.m_Data.size() == 0)
                return Option<SAllBreakpoints>();

            SLuaDebuggerProtocolReader theReader(theMessage.m_Data, *m_StrTable);
            while (theReader.HasData()) {
                SLuaDebugMessageHeader theHeader = theReader.ReadHeader();
                if (theHeader.m_MessageName == lua::SProtocolMessageNames::AllBreakpoints
                    && theHeader.m_ClientId == inClientId) {
                    return theReader.ReadMessage<lua::SAllBreakpoints>();
                } else
                    HandleNextMessage(theReader, theHeader);
            }
        }
        return Option<SAllBreakpoints>();
    }

    virtual void HandleMessage(lua::SLuaDebuggerProtocolReader &reader,
                               lua::SLuaDebugMessageHeader &header)
    {
        HandleNextMessage(reader, header);
    }
};
}

ILuaArchitectDebugServer &ILuaArchitectDebugServer::CreateLuaServer(IDebugOutStream &stream)
{
    return *QT3DS_NEW(g_MallocAlloc, SDebugServer)(stream);
}