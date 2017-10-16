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
#include "UICStateDebuggerProtocol.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSAtomic.h"

using namespace uic::state;
using namespace uic::state::debugger;

namespace {
MallocAllocator g_MallocAlloc;

struct SDebugStrInStream : public IInStream
{
    size_t m_Pos;
    const TDebugStr &m_Str;
    SDebugStrInStream(const TDebugStr &inStr)
        : m_Pos(0)
        , m_Str(inStr)
    {
    }
    virtual QT3DSU32 Read(NVDataRef<QT3DSU8> data)
    {
        size_t available = NVMin((size_t)data.size(), m_Str.size() - m_Pos);
        if (available) {
            memCopy(data.begin(), m_Str.data() + m_Pos, (QT3DSU32)available);
            m_Pos += available;
        }
        return (QT3DSU32)available;
    }
};

struct SDebuggerMaster : public IDebuggerMaster
{
    typedef eastl::hash_map<QT3DSI32, NVScopedRefCounted<IDebuggedInterpreter>> TIdInterpreterMap;

    QT3DSI32 mRefCount;
    NVScopedRefCounted<IDebugOutStream> m_OutStream;
    TIdInterpreterMap m_Interpreters;
    IDebuggerMasterListener &m_Listener;
    SMessageSerializer m_Serializer;
    SMessageParser<SDebuggerMaster> m_Parser;
    TDebugStr m_LogStr;
    TDebugStr m_MessageStr;
    bool m_Invalid;

    SDebuggerMaster(IDebugOutStream &inStream, IDebuggerMasterListener &listener)
        : mRefCount(0)
        , m_OutStream(inStream)
        , m_Listener(listener)
        , m_Invalid(false)
    {
    }

    virtual ~SDebuggerMaster()
    {
        // Disconnect();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(g_MallocAlloc);

    template <typename TMessageType>
    void SendMessage(QT3DSI32 inStreamId, const TMessageType &inMessage)
    {
        m_Serializer.Serialize(inStreamId, inMessage, 0);
        m_OutStream->Write(m_Serializer.ToRawMessage());
    }

    virtual void OnMessageReceived(const SDebugStreamMessage &inMessage)
    {
        if (m_Invalid)
            return;
        m_Parser.Parse(inMessage.m_Data, *this);
    }

    void ReleaseAllInterpreters()
    {
        for (TIdInterpreterMap::iterator iter = m_Interpreters.begin(), end = m_Interpreters.end();
             iter != end; ++iter) {
            iter->second->Disconnect();
            m_Listener.OnInterpreterDisconnected(*iter->second.mPtr);
        }
        m_Interpreters.clear();
    }

    virtual void Disconnect()
    {
        SendMessage(-1, SDisconnect());
        ReleaseAllInterpreters();
    }

    IDebuggedInterpreter *FindInterpreter(QT3DSI32 inStreamId)
    {
        TIdInterpreterMap::iterator iter = m_Interpreters.find(inStreamId);
        if (iter != m_Interpreters.end())
            return iter->second.mPtr;
        return NULL;
    }

// Set of ignored messages
#define IGNORE_DEBUG_MESSAGE_TYPE(tname)                                                           \
    void OnMessage(QT3DSU32, QT3DSU64, const S##tname &) { QT3DS_ASSERT(false); }

    // this are outgoing messages; we shouldn't be receiving them.
    IGNORE_DEBUG_MESSAGE_TYPE(SetBreakpoint)
    IGNORE_DEBUG_MESSAGE_TYPE(ClearBreakpoint)
    IGNORE_DEBUG_MESSAGE_TYPE(BreakOnMicrostep)
    IGNORE_DEBUG_MESSAGE_TYPE(ClearAllBreakpoints)
    IGNORE_DEBUG_MESSAGE_TYPE(Continue)

    void OnMessage(QT3DSU32 sid, QT3DSU64, const SConnect &inMessage)
    {
        SDebugStrInStream theStream(inMessage.m_SCXMLData);
        TEditorPtr theEditor(IEditor::CreateEditor(inMessage.m_Filename.c_str(), theStream));
        if (theEditor) {
            IDebuggedInterpreter &theInterpreter = IDebuggedInterpreter::Create(
                g_MallocAlloc, *m_OutStream.mPtr, theEditor, inMessage.m_Filename, sid,
                toConstDataRef(inMessage.m_Configuration.data(),
                               (QT3DSU32)inMessage.m_Configuration.size()),
                m_Listener);
            m_Interpreters.insert(eastl::make_pair(sid, &theInterpreter));
            m_Listener.OnInterpreterConnected(theInterpreter);
        } else {
            m_LogStr.assign("Connection failed: ");
            m_LogStr.append(inMessage.m_Filename);
            // log the failure somehow.
            m_Listener.OnLog(NULL, m_LogStr);
        }
    }

    void OnMessage(QT3DSU32 sid, QT3DSU64, const SBreakpointHit &inMessage)
    {
        IDebuggedInterpreter *interp = FindInterpreter(sid);
        if (interp)
            interp->BreakpointHit(inMessage.m_Breakpoint);
    }

    void OnMessage(QT3DSU32, QT3DSU64, const SInitialization &inMessage)
    {
        if (inMessage.m_Version != SInitialization::GetCurrentVersion()) {
            QT3DS_ASSERT(false);
            m_Invalid = true;
        }
    }

    void OnMessage(QT3DSU32 sid, QT3DSU64, const SDebugLog &inMessage)
    {
        IDebuggedInterpreter *interp = FindInterpreter(sid);
        m_Listener.OnLog(interp, inMessage.m_Message);
    }

#define FORWARD_INTERPRETER_EVENT(evnType, interpFun)                                              \
    void OnMessage(QT3DSI32 sid, QT3DSU64, const evnType &inMsg)                                         \
    {                                                                                              \
        IDebuggedInterpreter *interp = FindInterpreter(sid);                                       \
        if (interp)                                                                                \
            interp->interpFun(inMsg);                                                              \
    }

    FORWARD_INTERPRETER_EVENT(SEventQueued, OnEventQueued);
    FORWARD_INTERPRETER_EVENT(SBeginStep, OnBeginStep);
    FORWARD_INTERPRETER_EVENT(SBeginMicrostep, OnBeginMicrostep);
    FORWARD_INTERPRETER_EVENT(SMicrostepEvent, OnMicrostepEvent);
    FORWARD_INTERPRETER_EVENT(SMicrostepData, OnMicrostepData);
    FORWARD_INTERPRETER_EVENT(SEndMicrostep, OnEndMicrostep);
    FORWARD_INTERPRETER_EVENT(SModifyTableValues, OnModifyTable);

    void OnMessage(QT3DSI32 sid, QT3DSU64, const SDisconnect &)
    {
        if (sid > 0) {
            IDebuggedInterpreter *interp = FindInterpreter(sid);
            if (interp) {
                m_Listener.OnInterpreterDisconnected(*interp);
                m_Interpreters.erase(sid);
            }
        } else {
            ReleaseAllInterpreters();
        }
    }

    void error(const char8_t *inPrefix, const char8_t *inSuffix)
    {
        m_LogStr.assign(nonNull(inPrefix));
        m_LogStr.append(nonNull(inSuffix));
        m_Listener.OnLog(NULL, m_LogStr);
    }
};
}

IDebuggerMaster &IDebuggerMaster::CreateMaster(IDebugOutStream &outStream,
                                               IDebuggerMasterListener &inListener)
{
    return *QT3DS_NEW(g_MallocAlloc, SDebuggerMaster)(outStream, inListener);
}