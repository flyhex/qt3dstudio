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
#include "UICStateDebugStreams.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "EASTL/string.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace uic;
using namespace uic::state;
using namespace uic::state::debugger;

namespace {

struct MultiProtocolMessageTypes
{
    enum Enum {
        UnknownMessageType = 0,
        NewProtocol = 1,
        ProtocolMessage = 1 << 2,
    };
};

struct SMultiProtocolMessageFlags : public NVFlags<MultiProtocolMessageTypes::Enum, QT3DSU32>
{
    bool IsNewProtocol() { return this->operator&(MultiProtocolMessageTypes::NewProtocol); }
    void SetNewProtocol(bool inValue)
    {
        this->clearOrSet(inValue, MultiProtocolMessageTypes::NewProtocol);
    }

    bool IsProtocolMessage() { return this->operator&(MultiProtocolMessageTypes::ProtocolMessage); }
    void SetProtocolMessage(bool inValue)
    {
        this->clearOrSet(inValue, MultiProtocolMessageTypes::ProtocolMessage);
    }
};

struct SMultiProtocolInitializer
{
    static QT3DSU16 GetCurrentMultiProtocolVersion() { return 1; }

    QT3DSU64 m_TimeNumerator;
    QT3DSU64 m_TimeDenominator;
    QT3DSU32 m_ProtocolVersion;

    SMultiProtocolInitializer()
        : m_TimeNumerator(Time::sCounterFreq.mNumerator)
        , m_TimeDenominator(Time::sCounterFreq.mDenominator)
        , m_ProtocolVersion(GetCurrentMultiProtocolVersion())
    {
    }
};

struct SMultiProtocolMessageHeader
{

    SMultiProtocolMessageFlags m_Flags;
    QT3DSU32 m_Size;
    QT3DSU32 m_ProtocolId;
    QT3DSU64 m_Timestamp;
    SMultiProtocolMessageHeader(MultiProtocolMessageTypes::Enum inMessageType, QT3DSU32 size,
                                QT3DSU32 protocolId, QT3DSU64 timestamp)
        : m_Size(size)
        , m_ProtocolId(protocolId)
        , m_Timestamp(timestamp)
    {
        m_Flags.clearOrSet(true, inMessageType);
    }
    SMultiProtocolMessageHeader() {}
};

struct IProtocolMessageHandler
{
protected:
    virtual ~IProtocolMessageHandler() {}
public:
    virtual void OnMessageReceived(SDebugStreamMessage msgData) = 0;
};

struct IProtocolHandler
{
protected:
    virtual ~IProtocolHandler() {}
public:
    virtual void OnNewProtocol(CRegisteredString inProtocolName) = 0;
};

struct SSharedStreamImpl : public NVRefCounted
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<SocketStream> m_Stream;
    IOutStream &m_WriteStream;
    SMultiProtocolInitializer m_Initializer;
    eastl::hash_map<CRegisteredString, QT3DSU32> m_ProtocolIdMap;
    eastl::hash_map<QT3DSU32, IProtocolMessageHandler *> m_MessageHandlers;
    MemoryBuffer<> m_ReadBuffer;
    IStringTable &m_StringTable;
    IProtocolHandler *m_ProtocolHandler;
    QT3DSU32 m_NextProtocolId;
    QT3DSI32 mRefCount;

    SSharedStreamImpl(NVFoundationBase &fnd, SocketStream &stream, IOutStream &writeStream,
                      IStringTable &strTable, IProtocolHandler &pHandler)
        : m_Foundation(fnd)
        , m_Stream(stream)
        , m_WriteStream(writeStream)
        , m_ReadBuffer(ForwardingAllocator(fnd.getAllocator(), "ReadBuffer"))
        , m_StringTable(strTable)
        , m_ProtocolHandler(&pHandler)
        , m_NextProtocolId(1)
        , mRefCount(0)
    {
        NVConstDataRef<QT3DSU8> msgData = toU8DataRef(m_Initializer);
        bool streamValid = m_Stream->Write(msgData);
        if (streamValid == false)
            m_Stream = NULL;
    }
    ~SSharedStreamImpl() {}

    bool Initialize()
    {
        if (m_Stream) {
            NVDataRef<QT3DSU8> msgData = toU8DataRef(m_Initializer);
            QT3DSU32 numBytes = m_Stream->Read(msgData);
            if (numBytes != sizeof(SMultiProtocolInitializer)
                || m_Initializer.m_ProtocolVersion
                    > SMultiProtocolInitializer::GetCurrentMultiProtocolVersion()) {
                m_Stream = NULL;
                return false;
            }
            return true;
        }
        return false;
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    QT3DSU32 GetIdForProtocol(CRegisteredString protocol)
    {
        if (protocol.IsValid() == false)
            return 0;
        eastl::pair<eastl::hash_map<CRegisteredString, QT3DSU32>::iterator, bool> inserter =
            m_ProtocolIdMap.insert(eastl::make_pair(protocol, m_NextProtocolId));
        if (inserter.second) {
            QT3DSU32 newId = m_NextProtocolId;
            ++m_NextProtocolId;
            if (m_Stream) {
                QT3DSU32 msgLen = (QT3DSU32)strlen(protocol) + 1;
                NVConstDataRef<QT3DSU8> writeBuf(reinterpret_cast<const QT3DSU8 *>(protocol.c_str()),
                                              msgLen);
                WriteMessage(MultiProtocolMessageTypes::NewProtocol, writeBuf, newId);
            }
        }
        return inserter.first->second;
    }

    CRegisteredString GetProtocolForId(QT3DSU32 id)
    {
        for (eastl::hash_map<CRegisteredString, QT3DSU32>::iterator iter = m_ProtocolIdMap.begin(),
                                                                 end = m_ProtocolIdMap.end();
             iter != end; ++iter) {
            if (iter->second == id)
                return iter->first;
        }
        return CRegisteredString();
    }

    void AddMessageHandler(CRegisteredString protocol, IProtocolMessageHandler &hdl)
    {
        m_MessageHandlers.insert(eastl::make_pair(GetIdForProtocol(protocol), &hdl));
    }

    void RemoveMessageHandler(CRegisteredString protocol)
    {
        m_MessageHandlers.erase(GetIdForProtocol(protocol));
    }

    IProtocolMessageHandler *GetMessageHandler(CRegisteredString protocol)
    {
        return GetMessageHandler(GetIdForProtocol(protocol));
    }

    IProtocolMessageHandler *GetMessageHandler(QT3DSU32 protocolId)
    {
        eastl::hash_map<QT3DSU32, IProtocolMessageHandler *>::iterator iter =
            m_MessageHandlers.find(protocolId);
        if (iter != m_MessageHandlers.end())
            return iter->second;
        return NULL;
    }

    void ProtocolHandlerLeaving() { m_ProtocolHandler = NULL; }

    void DispatchMessage(SMultiProtocolMessageHeader inHeader, NVConstDataRef<QT3DSU8> msg)
    {
        if (inHeader.m_Flags.IsNewProtocol()) {
            char *pId = reinterpret_cast<char *>(const_cast<QT3DSU8 *>(msg.begin()));
            // Ensure null terminated, which should be done anyway but we don't know it will be.
            pId[inHeader.m_Size] = 0;
            CRegisteredString protocolName = m_StringTable.RegisterStr(pId);
            eastl::pair<eastl::hash_map<CRegisteredString, QT3DSU32>::iterator, bool> inserter =
                m_ProtocolIdMap.insert(eastl::make_pair(protocolName, inHeader.m_ProtocolId));
            if (inserter.second == false) {
                // remap id to higher id to reduce the chance of conflicts.
                QT3DSU32 potentialNewId = NVMax(inserter.first->second, inHeader.m_ProtocolId);
                if (potentialNewId != inserter.first->second) {
                    m_NextProtocolId = NVMax(m_NextProtocolId, potentialNewId + 1);
                    CRegisteredString existing = GetProtocolForId(potentialNewId);
                    if (existing.IsValid()) {
                        m_ProtocolIdMap.erase(protocolName);
                        GetIdForProtocol(protocolName);
                        return;
                    } else {
                        inserter.first->second = potentialNewId;
                    }
                }
            } else {
                if (m_ProtocolHandler != NULL) {
                    m_ProtocolHandler->OnNewProtocol(protocolName);
                }
            }
        } else {
            IProtocolMessageHandler *handler = GetMessageHandler(inHeader.m_ProtocolId);
            if (handler != NULL)
                handler->OnMessageReceived(SDebugStreamMessage(inHeader.m_Timestamp, msg));
        }
    }

    NVDataRef<QT3DSU8> ReadChunk(NVDataRef<QT3DSU8> target)
    {
        QT3DSU32 totalRead = 0;
        do {
            NVDataRef<QT3DSU8> nextBuf(target.begin() + totalRead, target.size() - totalRead);
            QT3DSU32 readResult = m_Stream->Read(nextBuf);
            totalRead += readResult;
            if (totalRead < target.size()) {
                totalRead = totalRead;
            }
        } while (connected() && totalRead < target.size());

        return toDataRef(m_ReadBuffer.begin(), totalRead);
    }

    NVDataRef<QT3DSU8> ReadChunk(QT3DSU32 size)
    {
        m_ReadBuffer.reserve(size);
        return ReadChunk(toDataRef(m_ReadBuffer.begin(), size));
    }

    virtual SDebugStreamMessage WaitForNextMessage(CRegisteredString protocol)
    {
        QT3DSU32 msgId = GetIdForProtocol(protocol);

        while (m_Stream) {
            SMultiProtocolMessageHeader header;
            NVDataRef<QT3DSU8> buf = toU8DataRef(header);
            buf = ReadChunk(buf);
            if (buf.size() < sizeof(header)) {
                m_Stream = NULL;
                QT3DS_ASSERT(false);
            } else {
                NVDataRef<QT3DSU8> readResult = ReadChunk(header.m_Size);
                if (readResult.mSize != header.m_Size) {
                    m_Stream = NULL;
                    QT3DS_ASSERT(false);
                } else {
                    if (header.m_ProtocolId == msgId) {
                        SDebugStreamMessage message;
                        message.m_Timestamp = header.m_Timestamp;
                        message.m_Data = readResult;
                        return message;
                    } else
                        DispatchMessage(header, readResult);
                }
            }
        }

        return SDebugStreamMessage();
    }

    virtual void MessagePump()
    {
        if (m_Stream == NULL)
            return;
        bool lastMessage = true;
        do {
            SMultiProtocolMessageHeader header;
            NVDataRef<QT3DSU8> buf = toU8DataRef(header);
            QT3DSU32 amountRead = m_Stream->nonBlockingRead(buf);
            if (amountRead == 0) {
                if (m_Stream->connected() == false)
                    m_Stream = NULL;
                lastMessage = false;
            } else {
                // read the rest of the header.
                QT3DSU32 leftover = buf.size() - amountRead;
                if (leftover) {
                    NVDataRef<QT3DSU8> nextPiece(buf.begin() + amountRead, leftover);
                    nextPiece = ReadChunk(nextPiece);
                    amountRead += nextPiece.size();
                }

                if (amountRead < sizeof(SMultiProtocolMessageHeader)) {
                    m_Stream = NULL;
                    QT3DS_ASSERT(false);

                } else {
                    NVDataRef<QT3DSU8> msgData = ReadChunk(header.m_Size);
                    if (msgData.size() == header.m_Size) {
                        DispatchMessage(header, msgData);
                    } else {
                        m_Stream = NULL;
                        QT3DS_ASSERT(false);
                    }
                }
            }

        } while (lastMessage && m_Stream);
    }

    SMultiProtocolMessageHeader CreateHeader(MultiProtocolMessageTypes::Enum type, QT3DSU32 size,
                                             QT3DSU32 protocolId)
    {
        SMultiProtocolMessageHeader retval;
        retval.m_Flags.clearOrSet(true, type);
        retval.m_ProtocolId = protocolId;
        retval.m_Size = size;
        retval.m_Timestamp = Time::getCurrentCounterValue();
        return retval;
    }

    bool WriteMessage(MultiProtocolMessageTypes::Enum type, NVConstDataRef<QT3DSU8> data,
                      QT3DSU32 protocolId)
    {
        if (connected()) {
            SMultiProtocolMessageHeader header(CreateHeader(type, data.size(), protocolId));
            NVConstDataRef<QT3DSU8> writeBuf = toU8DataRef(header);
            bool success = m_WriteStream.Write(writeBuf);
            if (success) {
                success = m_WriteStream.Write(data);
            }
            if (!success)
                m_Stream = NULL;
            return success;
        }
        return false;
    }

    virtual bool Write(CRegisteredString protocol, NVConstDataRef<QT3DSU8> data)
    {
        return WriteMessage(MultiProtocolMessageTypes::ProtocolMessage, data,
                            GetIdForProtocol(protocol));
    }

    bool connected() { return m_Stream != NULL && m_Stream->connected(); }
};

struct SMultiProtocolSocketStreamImpl : public IMultiProtocolSocketStream,
                                        public IProtocolMessageHandler
{
    NVFoundationBase &m_Foundation;
    CRegisteredString m_Protocol;
    IDebugStreamListener *m_Listener;
    NVScopedRefCounted<SSharedStreamImpl> m_SharedStream;
    bool m_StreamValid;
    QT3DSI32 mRefCount;

    SMultiProtocolSocketStreamImpl(NVFoundationBase &fnd, CRegisteredString protocol,
                                   IDebugStreamListener *listener, SSharedStreamImpl &stream)
        : m_Foundation(fnd)
        , m_Protocol(protocol)
        , m_Listener(listener)
        , m_SharedStream(stream)
        , m_StreamValid(true)
        , mRefCount(0)
    {
        m_SharedStream->AddMessageHandler(m_Protocol, *this);
    }

    ~SMultiProtocolSocketStreamImpl() { m_SharedStream->RemoveMessageHandler(m_Protocol); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    IDebugStreamListener *GetListener() override { return m_Listener; }
    CRegisteredString GetProtocolName() override { return m_Protocol; }

    void SetListener(IDebugStreamListener *listener) override { m_Listener = listener; }

    bool Write(NVConstDataRef<QT3DSU8> data) override
    {
        if (m_StreamValid)
            m_StreamValid = m_SharedStream->Write(m_Protocol, data);

        return m_StreamValid;
    }

    SDebugStreamMessage WaitForNextMessage() override
    {
        if (m_StreamValid)
            return m_SharedStream->WaitForNextMessage(m_Protocol);
        return SDebugStreamMessage();
    }

    void OnMessageReceived(SDebugStreamMessage data) override
    {
        if (m_Listener)
            m_Listener->OnMessageReceived(data);
    }

    bool Connected() override { return m_SharedStream->connected(); }
};

struct SMultiProtocolSocketImpl : public IMultiProtocolSocket, public IProtocolHandler
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<SSharedStreamImpl> m_SharedStream;
    NVScopedRefCounted<IMultiProtocolSocketListener> m_ProtocolListener;
    QT3DSI32 mRefCount;
    SMultiProtocolSocketImpl(NVFoundationBase &fnd, SocketStream &inStream, IStringTable &strTable,
                             IMultiProtocolSocketListener *protocolListener)
        : m_Foundation(fnd)
        , m_ProtocolListener(protocolListener)
        , mRefCount(0)
    {
        // At some point I may switch the writer to a buffered stream, at least on the client side.
        m_SharedStream = QT3DS_NEW(m_Foundation.getAllocator(), SSharedStreamImpl)(
            m_Foundation, inStream, inStream, strTable, *this);
    }

    ~SMultiProtocolSocketImpl() { m_SharedStream->ProtocolHandlerLeaving(); }

    bool Initialize() override { return m_SharedStream->Initialize(); }

    bool Connected() override { return m_SharedStream->connected(); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    virtual NVScopedRefCounted<IMultiProtocolSocketStream>
    CreateProtocol(const char *name, IDebugStreamListener *inListener) override
    {
        NVScopedRefCounted<IMultiProtocolSocketStream> retval = GetProtocol(name);
        if (retval) {
            QT3DS_ASSERT(false);
            return retval;
        }
        CRegisteredString protocolName = m_SharedStream->m_StringTable.RegisterStr(name);
        if (protocolName.IsValid() == false) {
            QT3DS_ASSERT(false);
            return retval;
        }
        SMultiProtocolSocketStreamImpl *newStream =
            QT3DS_NEW(m_Foundation.getAllocator(), SMultiProtocolSocketStreamImpl)(
                m_Foundation, protocolName, inListener, *m_SharedStream);
        return newStream;
    }

    NVScopedRefCounted<IMultiProtocolSocketStream> GetProtocol(const char *name) override
    {
        CRegisteredString protocolName = m_SharedStream->m_StringTable.RegisterStr(name);
        IProtocolMessageHandler *handler = m_SharedStream->GetMessageHandler(protocolName);
        if (handler) {
            SMultiProtocolSocketStreamImpl *theImpl =
                static_cast<SMultiProtocolSocketStreamImpl *>(handler);
            return theImpl;
        }
        return NVScopedRefCounted<IMultiProtocolSocketStream>();
    }

    void OnNewProtocol(CRegisteredString inProtocolName) override
    {
        if (m_ProtocolListener) {
            // We can expect the user to call create protocol at this point.
            IDebugStreamListener *handler = m_ProtocolListener->OnNewProtocol(inProtocolName);
            if (handler) {
                SMultiProtocolSocketStreamImpl *newStream =
                    QT3DS_NEW(m_Foundation.getAllocator(), SMultiProtocolSocketStreamImpl)(
                        m_Foundation, inProtocolName, handler, *m_SharedStream);
                m_ProtocolListener->OnNewProtocolStream(inProtocolName, *newStream);
            }
        }
    }

    CounterFrequencyToTensOfNanos SourceConversion() override
    {
        return CounterFrequencyToTensOfNanos(m_SharedStream->m_Initializer.m_TimeNumerator,
                                             m_SharedStream->m_Initializer.m_TimeDenominator);
    }

    void MessagePump() override { m_SharedStream->MessagePump(); }
};

struct SMultiProtocolStreamLuaCompatImpl : public IMultiProtocolStreamLuaCompat,
                                           public IDebugStreamListener
{
    NVFoundationBase &m_Foundation;
    eastl::string m_MessageBuffer;
    eastl::string m_CurrentMessage;
    QT3DSU32 m_Timeout;
    NVScopedRefCounted<IMultiProtocolSocketStream> m_Stream;
    QT3DSI32 mRefCount;

    SMultiProtocolStreamLuaCompatImpl(NVFoundationBase &fnd, IMultiProtocolSocket &socket)
        : m_Foundation(fnd)
        , m_Timeout(1000)
        , mRefCount(0)
    {
        m_Stream = socket.CreateProtocol(getProtocolName(), this);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void settimeout(QT3DSU32 value = 0) override { m_Timeout = value; }
    QT3DSU32 gettimeout() override { return m_Timeout; }
    // returns next line of data
    // or error
    eastl::pair<const char *, LuaStreamError::_Enum> receive(QT3DSU32 numBytes) override
    {
        if (m_Stream->Connected() == false)
            return eastl::make_pair("", LuaStreamError::Closed);

        m_CurrentMessage.clear();

        while (m_Stream->Connected()) {
            if (numBytes == 0) {
                eastl::string::size_type pos = m_MessageBuffer.find('\n');
                if (pos != eastl::string::npos || m_MessageBuffer.size()) {
                    if (pos == eastl::string::npos)
                        pos = m_MessageBuffer.size();
                    m_CurrentMessage = m_MessageBuffer.substr(0, pos);
                    m_MessageBuffer.erase(m_MessageBuffer.begin(),
                                          m_MessageBuffer.begin() + pos + 1);
                }
                if (m_CurrentMessage.size())
                    return eastl::make_pair(m_CurrentMessage.c_str(), LuaStreamError::NoLuaError);
            } else {
                QT3DSU32 required = numBytes - m_CurrentMessage.size();
                QT3DSU32 possible = NVMin(required, (QT3DSU32)m_MessageBuffer.size());
                m_CurrentMessage.append(m_MessageBuffer.begin(),
                                        m_MessageBuffer.begin() + possible);
                m_MessageBuffer.erase(m_MessageBuffer.begin(), m_MessageBuffer.begin() + possible);
                if (m_CurrentMessage.size() == numBytes)
                    return eastl::make_pair(m_CurrentMessage.c_str(), LuaStreamError::NoLuaError);
            }

            // nonblocking mode
            if (numBytes == 0 && m_Timeout == 0) {
                return eastl::make_pair("", LuaStreamError::Timeout);
            } else {
                SDebugStreamMessage nextMessage = m_Stream->WaitForNextMessage();
                OnMessageReceived(nextMessage);
            }
        }
        return eastl::make_pair("", LuaStreamError::Closed);
    }

    void send(const char *data) override
    {
        if (isTrivial(data))
            return;
        QT3DSU32 len = (QT3DSU32)strlen(data);
        m_Stream->Write(data, len);
    }

    bool hasData() override { return m_MessageBuffer.empty() == false; }

    virtual IDebugStreamListener &GetListener() { return *this; }

    void OnMessageReceived(const SDebugStreamMessage &msg) override
    {
        if (msg.m_Data.size()) {
            const char *dataBegin = reinterpret_cast<const char *>(msg.m_Data.begin());
            const char *dataEnd = dataBegin + msg.m_Data.size();
            // eastl::string temp;
            // temp.append( dataBegin, dataEnd );
            // m_Foundation.error( QT3DS_INTERNAL_ERROR, "Received: %s", temp.c_str() );
            m_MessageBuffer.append(dataBegin, dataEnd);
        }
    }
};

int MultiProtocolStreamLuaCompatGC(lua_State *state)
{
    int top = lua_gettop(state);
    luaL_checktype(state, -1, LUA_TUSERDATA);
    IMultiProtocolStreamLuaCompat **stream =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_touserdata(state, -1));
    if (stream && *stream)
        (*stream)->release();
    lua_settop(state, top);
    return 0;
}

int MultiProtocolStreamLuaReceive(lua_State *state)
{
    int top = lua_gettop(state);
    luaL_checktype(state, 1, LUA_TTABLE);
    lua_getfield(state, 1, "__streamimpl");
    IMultiProtocolStreamLuaCompat **stream =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_touserdata(state, -1));
    eastl::pair<const char *, LuaStreamError::_Enum> data("", LuaStreamError::Closed);
    if (stream && *stream) {
        QT3DSU32 numBytes = 0;
        if (top > 1) {
            int amount = lua_tointeger(state, 2);
            if (amount < 0) {
                QT3DS_ASSERT(false);
            }
            numBytes = static_cast<QT3DSU32>(amount);
        }
        data = (*stream)->receive(numBytes);
    }
    if (data.second == LuaStreamError::NoLuaError) {
        lua_pushstring(state, data.first);
        return 1;
    } else {
        lua_pushnil(state);
        const char *errorStr = "closed";
        if (data.second == LuaStreamError::Timeout)
            errorStr = "timeout";
        lua_pushstring(state, errorStr);
        return 2;
    }
}

int MultiProtocolStreamLuaSend(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TTABLE);
    luaL_checktype(state, 2, LUA_TSTRING);
    lua_getfield(state, 1, "__streamimpl");
    IMultiProtocolStreamLuaCompat **stream =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_touserdata(state, -1));
    const char *data = lua_tostring(state, 2);
    if (stream && *stream)
        (*stream)->send(data);
    return 0;
}

int MultiProtocolStreamLuaSettimeout(lua_State *state)
{
    int top = lua_gettop(state);
    luaL_checktype(state, 1, LUA_TTABLE);
    if (top == 2)
        luaL_checktype(state, 2, LUA_TNUMBER);

    lua_getfield(state, 1, "__streamimpl");
    IMultiProtocolStreamLuaCompat **stream =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_touserdata(state, -1));
    QT3DSU32 timeout = QT3DS_MAX_U32;
    if (top == 2)
        timeout = static_cast<QT3DSU32>(lua_tonumber(state, 2) * 1000);
    if (stream && *stream)
        (*stream)->settimeout(timeout);
    return 0;
}

int MultiProtocolStreamLuaGettimeout(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TTABLE);
    lua_getfield(state, 1, "__streamimpl");
    IMultiProtocolStreamLuaCompat **stream =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_touserdata(state, -1));
    if (stream && *stream) {
        lua_pushnumber(state, static_cast<lua_Number>((*stream)->gettimeout()));
        return 1;
    }
    return 0;
}

int MultiProtocolStreamLuaHasData(lua_State *state)
{
    luaL_checktype(state, 1, LUA_TTABLE);
    lua_getfield(state, 1, "__streamimpl");
    IMultiProtocolStreamLuaCompat **stream =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_touserdata(state, -1));
    if (stream && *stream) {
        bool data = (*stream)->hasData();
        lua_pushboolean(state, data ? 1 : 0);
        return 1;
    }
    return 0;
}
}

NVScopedRefCounted<IMultiProtocolSocket>
IMultiProtocolSocket::CreateProtocolSocket(NVFoundationBase &fnd, SocketStream &inStream,
                                           IStringTable &strTable,
                                           IMultiProtocolSocketListener *protocolListener)
{
    return QT3DS_NEW(fnd.getAllocator(), SMultiProtocolSocketImpl)(fnd, inStream, strTable,
                                                                protocolListener);
}

NVScopedRefCounted<IMultiProtocolStreamLuaCompat>
IMultiProtocolStreamLuaCompat::Create(NVFoundationBase &fnd, IMultiProtocolSocket &socket)
{
    return QT3DS_NEW(fnd.getAllocator(), SMultiProtocolStreamLuaCompatImpl)(fnd, socket);
}

void IMultiProtocolStreamLuaCompat::Bind(lua_State *state, IMultiProtocolStreamLuaCompat &stream)
{
    int top = lua_gettop(state);
    lua_newtable(state);
    IMultiProtocolStreamLuaCompat **theStreamAddress =
        reinterpret_cast<IMultiProtocolStreamLuaCompat **>(lua_newuserdata(state, sizeof(&stream)));
    *theStreamAddress = &stream;
    luaL_getmetatable(state, "IMultiProtocolStreamLuaCompat");
    if (lua_isnil(state, -1)) {
        lua_pop(state, 1);
        luaL_newmetatable(state, "IMultiProtocolStreamLuaCompat");
        lua_pushcfunction(state, MultiProtocolStreamLuaCompatGC);
        lua_setfield(state, -2, "__gc");
    }
    lua_setmetatable(state, -2);
    stream.addRef();
    lua_setfield(state, -2, "__streamimpl");
    lua_pushcfunction(state, MultiProtocolStreamLuaReceive);
    lua_setfield(state, -2, "receive");
    lua_pushcfunction(state, MultiProtocolStreamLuaSend);
    lua_setfield(state, -2, "send");
    lua_pushcfunction(state, MultiProtocolStreamLuaGettimeout);
    lua_setfield(state, -2, "gettimeout");
    lua_pushcfunction(state, MultiProtocolStreamLuaSettimeout);
    lua_setfield(state, -2, "settimeout");
    lua_pushcfunction(state, MultiProtocolStreamLuaHasData);
    lua_setfield(state, -2, "hasdata");
    int rettop = lua_gettop(state);
    QT3DS_ASSERT(rettop - top == 1);
    luaL_checktype(state, -1, LUA_TTABLE);
    (void)top;
    (void)rettop;
}
