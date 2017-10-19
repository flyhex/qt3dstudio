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
#ifndef QT3DS_LUA_DATAMODLE_CACHE_H
#define QT3DS_LUA_DATAMODLE_CACHE_H
#include "Qt3DSLuaDebugger.h"
#include "Qt3DSLuaDebuggerProtocol.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/FileTools.h"
#include "foundation/StringConversionImpl.h"
#include "EASTL/set.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace qt3ds::state;
using namespace qt3ds::state::debugger;
/**
 *	The caching system is fairly simple.  Basically, the sender side keeps track of table and
 *function info values and sends the entire table
 *	or function info if something is different than expected.
 *
 *	On top of this design you have a system that just sends item pointers when possible, and it
 *allows the server side to
 *	request the actual data behind the pointers when necessary.  This minimizes the amount of
 *information sent.
 *
 *	The server side needs to know when the lua side has ended its update so that it knows to ask
 *the lua side
 *	when to re-check various tables for changes.
 *
 */

namespace {

typedef eastl::hash_map<const void *, eastl::vector<STableEntry>> TTableToDataMap;
typedef eastl::hash_map<const void *, lua::SFunctionInfo> TFunctionToInfoMap;
typedef eastl::hash_set<const void *> TVoidPtrSet;

struct SRuntimeDataCache : public ILuaRuntimeDataCache
{
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    IDebugOutStream &m_Stream;
    QT3DSU64 m_ClientId;
    lua::SLuaDebuggerProtocolWriter m_ProtocolWriter;
    TDebugStr m_WorkStr;
    TDebugStr m_RelativeStr;
    TDebugStr m_BaseStr;
    QT3DSI32 mRefCount;

    eastl::set<const void *> m_ScannedTables;
    TTableToDataMap m_TableCache;
    TFunctionToInfoMap m_FunctionCache;
    char m_ConvertBuffer[256];

    SRuntimeDataCache(NVFoundationBase &fnd, IStringTable &strTable, IDebugOutStream &stream,
                      QT3DSU64 cid, const char *inProjectDir)
        : m_Foundation(fnd)
        , m_StringTable(strTable)
        , m_Stream(stream)
        , m_ClientId(cid)
        , m_ProtocolWriter(stream, fnd.getAllocator())
        , m_BaseStr(nonNull(inProjectDir))
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    QT3DSU64 ToClientId() const { return m_ClientId; }

    template <typename TMsgType>
    void WriteMessage(const TMsgType &msg)
    {
        m_ProtocolWriter.WriteMessage(ToClientId(), const_cast<TMsgType &>(msg));
    }

    SDatamodelValue ToValue(lua_State &state, QT3DSI32 stackIdx)
    {
        switch (lua_type(&state, stackIdx)) {
        case LUA_TBOOLEAN:
            return lua_toboolean(&state, stackIdx) ? true : false;
        case LUA_TNIL:
            return SNil();

        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
            return reinterpret_cast<SDatamodelUserData *>(lua_touserdata(&state, stackIdx));

        case LUA_TNUMBER:
            return lua_tonumber(&state, stackIdx);
        case LUA_TSTRING:
            return TDebugStr(lua_tostring(&state, stackIdx));
        case LUA_TTABLE:
            return reinterpret_cast<SDatamodelTable *>((void *)lua_topointer(&state, stackIdx));
        case LUA_TFUNCTION:
            return reinterpret_cast<SDatamodelFunction *>((void *)lua_topointer(&state, stackIdx));
        case LUA_TTHREAD:
            return reinterpret_cast<SDatamodelThread *>((void *)lua_topointer(&state, stackIdx));
        default:
            QT3DS_ASSERT(false);
            return SDatamodelValue();
        }
    }

    static const char *GetPointerMapName() { return "Qt3DS-runtime-cache-ptr-map"; }

    const char *PtrToKey(const void *ptr)
    {
        QT3DSU64 ptrVal = static_cast<QT3DSU64>(reinterpret_cast<size_t>(ptr));
        StringConversion<QT3DSU64>().ToStr(ptrVal, toDataRef(m_ConvertBuffer, 256));
        return m_ConvertBuffer;
    }
    // take the object at the top of the stack (either function or table)
    // and store it away in a registry map.  This allows us to defer scanning the object
    // potentially forever
    void MapPointerToLuaObject(lua_State &state, QT3DSI32 stackIdx)
    {
        const void *ptrValue = lua_topointer(&state, stackIdx);

        if (m_ScannedTables.find(ptrValue) != m_ScannedTables.end())
            return;

        if (lua_type(&state, stackIdx) == LUA_TTABLE
            || lua_type(&state, stackIdx) == LUA_TFUNCTION) {
            lua_pushvalue(&state, stackIdx);
            lua_getfield(&state, LUA_REGISTRYINDEX, GetPointerMapName());
            if (lua_isnil(&state, -1)) {
                lua_pop(&state, 1);
                lua_newtable(&state);
                lua_pushvalue(&state, -1);
                lua_setfield(&state, LUA_REGISTRYINDEX, GetPointerMapName());
            }
            QT3DS_ASSERT(lua_type(&state, -1) == LUA_TTABLE);
            lua_insert(&state, -2);
            lua_setfield(&state, -2, PtrToKey(ptrValue));
            // knock the registry table off the stack.
            lua_pop(&state, 1);
        }
    }

    // If function fails, then nil is left on stack
    void GetLuaObjectFromPointer(lua_State &state, void *ptrAddr)
    {
        lua_getfield(&state, LUA_REGISTRYINDEX, GetPointerMapName());
        if (lua_istable(&state, -1)) {
            lua_getfield(&state, -1, PtrToKey(ptrAddr));
            // knock the original table off the map.
            lua_insert(&state, -2);
            lua_pop(&state, 1);
        }
    }

    void CacheTable(lua_State &state, QT3DSI32 stackIdx)
    {
        QT3DS_ASSERT(lua_type(&state, stackIdx) == LUA_TTABLE);
        int tableTop = lua_gettop(&state);
        if (lua_type(&state, stackIdx) == LUA_TTABLE) {
            const void *tableAddr = lua_topointer(&state, stackIdx);
            if (m_ScannedTables.find(tableAddr) != m_ScannedTables.end())
                return;
            m_ScannedTables.insert(tableAddr);
            SDatamodelTable *theTable = (SDatamodelTable *)tableAddr;
            eastl::vector<STableEntry> &existing =
                m_TableCache.insert(eastl::make_pair(tableAddr, eastl::vector<STableEntry>()))
                    .first->second;

            bool sendTable = false;
            // push the table to the top so the relative addressing will work.
            lua_pushvalue(&state, stackIdx);
            // Iterate through keys ensuring that we get a valid table entry for each key.
            lua_pushnil(&state);
            QT3DSU32 idx = 0;
            while (lua_next(&state, -2) != 0) {
                SDatamodelValue theValue(ToValue(state, -1));
                int originalTop = lua_gettop(&state);
                if (theValue.getType() == DatamodelValueTypes::Table
                    || theValue.getType() == DatamodelValueTypes::Function) {
                    // store it off for later.
                    MapPointerToLuaObject(state, -1);
                }

                int newtop = lua_gettop(&state);
                QT3DS_ASSERT(originalTop == newtop);
                (void)newtop;
                (void)originalTop;

                // dup the key so the tostring function doesn't change the actual table key type
                lua_pushvalue(&state, -2);
                const char *keyVal = lua_tostring(&state, -1);
                STableEntry entry(keyVal, theValue);
                if (idx >= existing.size() || ((existing[idx] == entry) == false)) {
                    if (idx >= existing.size())
                        existing.push_back(entry);
                    else
                        existing[idx] = entry;
                    sendTable = true;
                }
                ++idx;
                // pop value and dupped key off the stack.
                lua_pop(&state, 2);
            }
            // pop table off top.
            lua_pop(&state, 1);

            if (existing.size() != idx) {
                sendTable = true;
                existing.resize(idx);
            }
            if (sendTable) {
                lua::STableData theData(*theTable,
                                        toConstDataRef(existing.data(), existing.size()));
                WriteMessage(lua::SUpdateTable(theData));
            }
        }
        int finalTop = lua_gettop(&state);
        QT3DS_ASSERT(finalTop == tableTop);
        (void)finalTop;
        (void)tableTop;
    }

    CRegisteredString ToRelative(const char *inFilePath)
    {
        m_RelativeStr.assign(nonNull(inFilePath));
        CFileTools::GetRelativeFromBase(m_BaseStr, m_RelativeStr, m_WorkStr);
        return m_StringTable.RegisterStr(m_WorkStr.c_str());
    }

    void CacheFunction(lua_State &state, QT3DSI32 stackIdx)
    {
        QT3DS_ASSERT(lua_type(&state, stackIdx) == LUA_TFUNCTION);
        if (lua_type(&state, stackIdx) == LUA_TFUNCTION) {
            const void *fn_addr = lua_topointer(&state, stackIdx);
            if (m_ScannedTables.find(fn_addr) != m_ScannedTables.end())
                return;
            m_ScannedTables.insert(fn_addr);

            lua_getfenv(&state, stackIdx);
            SDatamodelTable *env = (SDatamodelTable *)lua_topointer(&state, -1);
            MapPointerToLuaObject(state, -1);
            lua_pop(&state, 1);
            SDatamodelFunction *fn = (SDatamodelFunction *)lua_topointer(&state, stackIdx);

            // dup the function to top of stack for the getinfo call.
            lua_pushvalue(&state, stackIdx);

            lua_Debug ar;
            lua_getinfo(&state, ">S", &ar);
            SFileAndLine fnData;
            if (!isTrivial(ar.source) && ar.source[0] == '@') {
                fnData.m_File = ToRelative(ar.source + 1);
                fnData.m_Line = ar.linedefined;
            } else if (!isTrivial(ar.source)) {
                fnData.m_File = m_StringTable.RegisterStr(ar.source);
            }

            lua::SFunctionInfo theInfo(*fn, fnData, *env);
            lua::SFunctionInfo &existingInfo(
                m_FunctionCache.insert(eastl::make_pair(fn_addr, lua::SFunctionInfo()))
                    .first->second);
            if (existingInfo != theInfo) {
                existingInfo = theInfo;
                WriteMessage(lua::SUpdateFunction(theInfo));
            }
        }
    }

    void BeginUpdate() override { WriteMessage(lua::SBeginCacheUpdate()); }

    SDatamodelValue CacheValue(lua_State &state, QT3DSI32 stackIdx = -1) override
    {
        switch (lua_type(&state, stackIdx)) {
        case LUA_TTABLE:
            CacheTable(state, stackIdx);
            break;
        case LUA_TFUNCTION:
            CacheFunction(state, stackIdx);
            break;
        default:
            break;
        }
        return ToValue(state, stackIdx);
    }

    void EndUpdate(lua_State &state) override
    {
        m_ScannedTables.clear();
        lua_pushnil(&state);
        lua_setfield(&state, LUA_REGISTRYINDEX, GetPointerMapName());
    }

    bool IsCacheMessage(lua::SLuaDebugMessageHeader &header) override
    {
        return header.m_MessageName == lua::SProtocolMessageNames::RequestPointerValue;
    }

    void HandleCacheMessage(lua_State *state, lua::SLuaDebuggerProtocolReader &reader,
                                    lua::SLuaDebugMessageHeader & /*header*/) override
    {
        int top = lua_gettop(state);
        (void)top;
        lua::SRequestPointerValue theMessage = reader.ReadMessage<lua::SRequestPointerValue>();
        void *ptrValue = NULL;
        switch (theMessage.m_Data.getType()) {
        case DatamodelValueTypes::Function:
            ptrValue = theMessage.m_Data.getData<SDatamodelFunction *>();
            break;
        case DatamodelValueTypes::Table:
            ptrValue = theMessage.m_Data.getData<SDatamodelTable *>();
            break;
        case DatamodelValueTypes::CFunction:
            ptrValue = theMessage.m_Data.getData<SDatamodelCFunction *>();
            break;
        case DatamodelValueTypes::Thread:
            ptrValue = theMessage.m_Data.getData<SDatamodelThread *>();
            break;
        default:
            break;
        }

        if (ptrValue == NULL || state == NULL)
            WriteMessage(lua::SUnknownPointerValue());
        else {
            // Get the object to be TOS
            GetLuaObjectFromPointer(*state, ptrValue);
            CacheValue(*state);
            lua_pop(state, 1);
        }
        QT3DS_ASSERT(lua_gettop(state) == top);
        WriteMessage(lua::SEndPointerRequest());
    }
};

struct SArchitectCache : public ILuaArchitectDataCache
{
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    TTableToDataMap m_TableCache;
    TFunctionToInfoMap m_FunctionInfo;
    IDebugOutStream &m_Stream;
    ILuaArchitectDataCacheListener &m_Listener;
    QT3DSU64 m_ClientId;
    lua::SLuaDebuggerProtocolWriter m_ProtocolWriter;
    TVoidPtrSet m_UpdatedPointers;
    QT3DSI32 mRefCount;

    SArchitectCache(NVFoundationBase &fnd, IStringTable &strTable, IDebugOutStream &stream,
                    ILuaArchitectDataCacheListener &listener, QT3DSU64 inClientId)
        : m_Foundation(fnd)
        , m_StringTable(strTable)
        , m_Stream(stream)
        , m_Listener(listener)
        , m_ClientId(inClientId)
        , m_ProtocolWriter(stream, fnd.getAllocator())
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    QT3DSU64 ToClientId() const { return m_ClientId; }

    template <typename TMsgType>
    void WriteMessage(const TMsgType &msg)
    {
        m_ProtocolWriter.WriteMessage(ToClientId(), const_cast<TMsgType &>(msg));
    }

    void RequestPointerValue(const SDatamodelValue &inValue)
    {
        WriteMessage(lua::SRequestPointerValue(inValue));
        while (m_Stream.Connected()) {
            SDebugStreamMessage message = m_Stream.WaitForNextMessage();
            if (message.m_Data.size() == 0)
                return;
            lua::SLuaDebuggerProtocolReader theReader(message.m_Data, m_StringTable);
            while (theReader.HasData()) {
                lua::SLuaDebugMessageHeader theHeader = theReader.ReadHeader();
                if (theHeader.m_MessageName == lua::SProtocolMessageNames::EndPointerRequest)
                    return;
                if (IsCacheMessage(theHeader)) {
                    HandleCacheMessage(theReader, theHeader);
                } else
                    m_Listener.HandleMessage(theReader, theHeader);
            }
        }
    }

    template <typename TDataType>
    void EnsurePointerIsUpToDate(TDataType &dtype)
    {
        const void *ptrAddr = &dtype;
        bool needsUpdate = m_UpdatedPointers.insert(ptrAddr).second;
        if (needsUpdate) {
            RequestPointerValue(&dtype);
        }
    }

    SFunctionInfo GetFunctionInfo(SDatamodelFunction &inFunction) override
    {
        EnsurePointerIsUpToDate(inFunction);
        TFunctionToInfoMap::iterator iter = m_FunctionInfo.find((void *)&inFunction);
        if (iter != m_FunctionInfo.end())
            return SFunctionInfo(*iter->second.m_Environment, iter->second.m_DefinitionLocation);
        return SFunctionInfo();
    }

    NVConstDataRef<STableEntry> GetTableValues(SDatamodelTable &inTable) override
    {
        EnsurePointerIsUpToDate(inTable);
        TTableToDataMap::iterator iter = m_TableCache.find((const void *)&inTable);
        if (iter != m_TableCache.end())
            return toConstDataRef(iter->second.data(), (QT3DSU32)iter->second.size());

        return NVConstDataRef<STableEntry>();
    }

    // Allow transparent handling of cache protocol.
    bool IsCacheMessage(lua::SLuaDebugMessageHeader &header) override
    {
        return header.m_MessageName == lua::SProtocolMessageNames::UpdateFunction
            || header.m_MessageName == lua::SProtocolMessageNames::UpdateTable
            || header.m_MessageName == lua::SProtocolMessageNames::EndPointerRequest
            || header.m_MessageName == lua::SProtocolMessageNames::BeginCacheUpdate;
    }

    void HandleUpdateFunction(const lua::SUpdateFunction &inFunction)
    {
        // Hard set it to new value regardless of what is in there now.
        const void *ptrAddr = inFunction.m_Data.m_Function;
        m_FunctionInfo.insert(eastl::make_pair(ptrAddr, inFunction.m_Data)).first->second =
            inFunction.m_Data;
        m_UpdatedPointers.insert(ptrAddr);
    }

    void HandleUpdateTable(const lua::SUpdateTable &inTable)
    {
        const void *ptrAddr = inTable.m_Data.m_Table;
        m_TableCache.insert(eastl::make_pair(ptrAddr, eastl::vector<STableEntry>()))
            .first->second.assign(inTable.m_Data.m_TableData.begin(),
                                  inTable.m_Data.m_TableData.end());
        m_UpdatedPointers.insert(ptrAddr);
    }

    void HandleCacheMessage(lua::SLuaDebuggerProtocolReader &reader,
                                    lua::SLuaDebugMessageHeader &header) override
    {
        switch (header.m_MessageName) {
        case lua::SProtocolMessageNames::UpdateFunction:
            HandleUpdateFunction(reader.ReadMessage<lua::SUpdateFunction>());
            break;
        case lua::SProtocolMessageNames::UpdateTable:
            HandleUpdateTable(reader.ReadMessage<lua::SUpdateTable>());
            break;
        case lua::SProtocolMessageNames::EndPointerRequest:
            break;
        case lua::SProtocolMessageNames::BeginCacheUpdate:
            m_UpdatedPointers.clear();
            break;
        default:
            QT3DS_ASSERT(false);
            return;
        }
    }
};
}

ILuaRuntimeDataCache &ILuaRuntimeDataCache::Create(NVFoundationBase &fnd, IStringTable &strTable,
                                                   IDebugOutStream &stream, QT3DSU64 clientId,
                                                   const char *inProjectDir)
{
    return *QT3DS_NEW(fnd.getAllocator(), SRuntimeDataCache)(fnd, strTable, stream, clientId,
                                                          inProjectDir);
}

ILuaArchitectDataCache &ILuaArchitectDataCache::Create(NVFoundationBase &fnd,
                                                       IStringTable &strTable,
                                                       IDebugOutStream &stream,
                                                       ILuaArchitectDataCacheListener &listener,
                                                       QT3DSU64 inClientId)
{
    return *QT3DS_NEW(fnd.getAllocator(), SArchitectCache)(fnd, strTable, stream, listener,
                                                        inClientId);
}

#endif
