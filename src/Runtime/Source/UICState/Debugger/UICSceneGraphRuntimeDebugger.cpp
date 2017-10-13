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
#include "UICSceneGraphDebugger.h"
#include "UICSceneGraphDebuggerValue.h"
#include "UICSceneGraphDebuggerProtocol.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "EASTL/sort.h"

using namespace qt3ds::state;
using namespace qt3ds::state::debugger;

namespace {

typedef eastl::pair<QT3DSU64, CStringHandle> TElemStringHandlePair;

struct SRegisteredIDComparator
{
    bool operator()(const TElemStringHandlePair &lhs, const TElemStringHandlePair &rhs) const
    {
        return lhs.first < rhs.first;
    }
};

struct SPresentationIdMap
{
    void *m_Presentation;
    CRegisteredString m_PresentationId;
    nvvector<TElemStringHandlePair> m_RegisteredIdBackingStore;
    NVDataRef<TElemStringHandlePair> m_RegisteredIds;
    SPresentationIdMap(NVAllocatorCallback &alloc)
        : m_RegisteredIdBackingStore(alloc, "registered ids")
    {
    }
};

struct RuntimeDebuggerImpl : public ISceneGraphRuntimeDebugger
{
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    NVScopedRefCounted<IDebugOutStream> m_OutStream;
    nvvector<SElemMap> m_ElemMapBuffer;
    nvvector<SPresentationIdMap *> m_PresentationIdMap;
    nvvector<SValueUpdate> m_ValueUpdates;
    // Filter mechanism so we don't send the same thing twice.
    nvhash_map<void *, eastl::vector<SValueUpdate>> m_LastUpdates;
    SSGProtocolWriter *m_Writer;
    SSAutoDeallocatorAllocator m_DataAllocator;
    nvhash_map<void *, CStringHandle> m_ElemToNameMap;
    QT3DSI32 mRefCount;

    RuntimeDebuggerImpl(NVFoundationBase &fnd, IStringTable &strt)
        : m_Foundation(fnd)
        , m_StringTable(strt)
        , m_ElemMapBuffer(fnd.getAllocator(), "elem map buffer")
        , m_PresentationIdMap(fnd.getAllocator(), "Presentations")
        , m_ValueUpdates(fnd.getAllocator(), "Value updates")
        , m_LastUpdates(fnd.getAllocator(), "Last updates")
        , m_Writer(NULL)
        , m_DataAllocator(fnd)
        , m_ElemToNameMap(fnd.getAllocator(), "ElemToNameMap")
        , mRefCount(0)
    {
    }

    ~RuntimeDebuggerImpl()
    {
        Disconnect();
        for (QT3DSU32 idx = 0, end = m_PresentationIdMap.size(); idx < end; ++idx)
            delete m_PresentationIdMap[idx];
        // the auto-deallocator takes care of the load data.
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void Disconnect()
    {
        if (m_Writer)
            delete m_Writer;
        m_Writer = NULL;
    }

    bool CheckConnection()
    {
        if (m_OutStream) {
            bool connected = m_OutStream->Connected();
            if (!connected)
                Disconnect();

            return connected;
        }
        return false;
    }

    void MapPresentationId(void *presentation, const char *id) override
    {
        CRegisteredString newId = m_StringTable.RegisterStr(nonNull(id));
        for (QT3DSU32 idx = 0, end = m_PresentationIdMap.size(); idx < end; ++idx) {
            if (m_PresentationIdMap[idx]->m_Presentation == presentation) {
                m_PresentationIdMap[idx]->m_PresentationId = newId;
                return;
            }
        }

        SPresentationIdMap *map = new SPresentationIdMap(m_Foundation.getAllocator());
        map->m_Presentation = presentation;
        map->m_PresentationId = newId;
        m_PresentationIdMap.push_back(map);
    }

    void SendElemIdMap(SPresentationIdMap &map)
    {
        if (m_ElemMapBuffer.size() && CheckConnection()) {
            SIdUpdate theUpdate;
            theUpdate.m_Presentation = (QT3DSU64)map.m_Presentation;
            theUpdate.m_PresentationId = map.m_PresentationId;
            theUpdate.m_IdUpdates = m_ElemMapBuffer;
            m_Writer->Write(theUpdate);
        }
        m_ElemMapBuffer.clear();
    }

    void MapElementIds(void *presentation, NVConstDataRef<SGElemIdMap> inIds) override
    {
        SPresentationIdMap *map = NULL;
        for (QT3DSU32 idx = 0, end = m_PresentationIdMap.size(); idx < end && map == NULL; ++idx)
            if (m_PresentationIdMap[idx]->m_Presentation == presentation)
                map = m_PresentationIdMap[idx];
        if (map == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        m_ElemMapBuffer.clear();
        m_ElemToNameMap.clear();
        for (QT3DSU32 idx = 0, end = inIds.size(); idx < end; ++idx) {
            const SGElemIdMap &item(inIds[idx]);
            SElemMap newMap;
            newMap.m_Elem = (QT3DSU64)item.m_Elem;
            CStringHandle idHandle = m_StringTable.GetHandle(nonNull(item.m_Id));
            newMap.m_Id = m_StringTable.HandleToStr(idHandle);
            m_ElemMapBuffer.push_back(newMap);
            m_ElemToNameMap[item.m_Elem] = idHandle;
        }
        SendElemIdMap(*map);

        // store them for later.
        map->m_RegisteredIdBackingStore.reserve(m_ElemToNameMap.size());
        for (nvhash_map<void *, CStringHandle>::iterator iter = m_ElemToNameMap.begin(),
                                                         end = m_ElemToNameMap.end();
             iter != end; ++iter) {
            map->m_RegisteredIdBackingStore.push_back(
                eastl::make_pair((QT3DSU64)iter->first, iter->second));
        }

        eastl::sort(map->m_RegisteredIdBackingStore.begin(), map->m_RegisteredIdBackingStore.end(),
                    SRegisteredIDComparator());

        map->m_RegisteredIds = NVDataRef<TElemStringHandlePair>(
            map->m_RegisteredIdBackingStore.data(), (QT3DSU32)map->m_RegisteredIdBackingStore.size());
    }

    static bool Equals(const SValueUpdate &lhs, const SValueUpdate &rhs)
    {
        return lhs.m_Hash == rhs.m_Hash && lhs.m_Value == rhs.m_Value;
    }

    void OnPropertyChanged(void *elem, NVConstDataRef<SSGPropertyChange> changes) override
    {
        if (CheckConnection() == false)
            return;
        eastl::vector<SValueUpdate> &updates = m_LastUpdates[elem];
        updates.resize(changes.size());
        for (QT3DSU32 changeIdx = 0, changeEnd = changes.size(); changeIdx < changeEnd; ++changeIdx) {
            SValueUpdate theUpdate;
            theUpdate.m_Hash = changes[changeIdx].m_Hash;
            theUpdate.m_Value = changes[changeIdx].m_Value;
            if (Equals(theUpdate, updates[changeIdx]) == false) {
                updates[changeIdx] = theUpdate;
                m_ValueUpdates.push_back(theUpdate);
            }
        }
        if (m_ValueUpdates.size()) {
            SElemUpdate theUpdate;
            theUpdate.m_Elem = (QT3DSU64)elem;
            theUpdate.m_Updates = m_ValueUpdates;
            m_Writer->Write(theUpdate);
            m_ValueUpdates.clear();
        }
    }

    void SendPresentation(SPresentationIdMap &pres)
    {
        if (m_OutStream) {
            m_ElemMapBuffer.clear();
            for (TElemStringHandlePair *iter = pres.m_RegisteredIds.begin(),
                                       *end = pres.m_RegisteredIds.end();
                 iter != end; ++iter) {
                SElemMap newMap;
                newMap.m_Elem = (QT3DSU64)iter->first;
                newMap.m_Id = m_StringTable.HandleToStr(iter->second);
                m_ElemMapBuffer.push_back(newMap);
            }
            SendElemIdMap(pres);
        }
    }

    void OnConnection(IDebugOutStream &outStream) override
    {
        Disconnect();
        m_OutStream = outStream;
        m_Writer = new SSGProtocolWriter(outStream, m_Foundation.getAllocator());
        m_Writer->WriteInitialization();
        for (QT3DSU32 idx = 0, end = m_PresentationIdMap.size(); idx < end; ++idx) {
            SPresentationIdMap *map = m_PresentationIdMap[idx];
            SendPresentation(*map);
        }
    }

    bool IsConnected() override { return CheckConnection(); }

    void BinarySave(IOutStream &ioStream) override
    {
        qt3ds::foundation::SWriteBuffer theWriteBuffer(m_Foundation.getAllocator(),
                                                    "BinarySave::writebuffer");

        theWriteBuffer.writeZeros(4); // Overall binary size
        theWriteBuffer.write((QT3DSU32)m_PresentationIdMap.size());
        for (QT3DSU32 idx = 0, end = m_PresentationIdMap.size(); idx < end; ++idx) {
            // There is no use to writing out the presentation pointer address.
            /*
                    void*
               m_Presentation;
                    CRegisteredString						m_PresentationId;
                    nvhash_map<void*, CRegisteredString>	m_RegisteredIds;
            */

            SPresentationIdMap *map = m_PresentationIdMap[idx];
            CRegisteredString presId(map->m_PresentationId);
            presId.Remap(m_StringTable.GetRemapMap());
            theWriteBuffer.write(presId);
            theWriteBuffer.write((QT3DSU32)map->m_RegisteredIds.size());
            theWriteBuffer.write(map->m_RegisteredIds.begin(), map->m_RegisteredIds.size());
        }

        QT3DSU32 totalSize = theWriteBuffer.size();
        QT3DSU32 *data = (QT3DSU32 *)theWriteBuffer.begin();
        data[0] = totalSize - 4;
        ioStream.Write((QT3DSU8 *)data, totalSize);
    }

    void BinaryLoad(IInStream &ioStream, NVDataRef<QT3DSU8> strTableData) override
    {
        QT3DSU32 length;
        ioStream.Read(length);
        QT3DSU8 *data = (QT3DSU8 *)m_DataAllocator.allocate(length, "Binaryload", __FILE__, __LINE__);
        ioStream.Read(data, length);
        SDataReader theReader(data, data + length);
        QT3DSU32 numPresentations = theReader.LoadRef<QT3DSU32>();
        QT3DS_ASSERT(m_PresentationIdMap.size() == 0);
        m_PresentationIdMap.resize(numPresentations);
        for (QT3DSU32 idx = 0, end = numPresentations; idx < end; ++idx) {
            m_PresentationIdMap[idx] = new SPresentationIdMap(m_Foundation.getAllocator());
            SPresentationIdMap *map = m_PresentationIdMap[idx];
            map->m_PresentationId = theReader.LoadRef<CRegisteredString>();
            map->m_PresentationId.Remap(strTableData);
            QT3DSU32 numElems = theReader.LoadRef<QT3DSU32>();
            map->m_RegisteredIds =
                toDataRef((TElemStringHandlePair *)theReader.m_CurrentPtr, numElems);
            theReader.m_CurrentPtr += numElems * sizeof(TElemStringHandlePair);
        }
    }

    void BinaryLoadPresentation(void *presPtr, const char *id, size_t elemOffset) override
    {
        CRegisteredString presId(m_StringTable.RegisterStr(id));
        SPresentationIdMap *foundPres = NULL;
        for (QT3DSU32 idx = 0, end = (QT3DSU32)m_PresentationIdMap.size(); idx < end && foundPres == NULL;
             ++idx) {
            if (m_PresentationIdMap[idx]->m_PresentationId == presId)
                foundPres = m_PresentationIdMap[idx];
        }
        if (foundPres == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        foundPres->m_Presentation = presPtr;
        nvvector<eastl::pair<void *, CRegisteredString>> newElemIds(m_Foundation.getAllocator(),
                                                                    "Temp load map");
        newElemIds.reserve(foundPres->m_RegisteredIds.size());
        for (TElemStringHandlePair *iter = foundPres->m_RegisteredIds.begin(),
                                   *end = foundPres->m_RegisteredIds.end();
             iter != end; ++iter) {
            size_t oldId = (size_t)iter->first;
            size_t newId = elemOffset + oldId;
            iter->first = (QT3DSU64)newId;
        }
        SendPresentation(*foundPres);
    }

    void EndFrame() override
    {
        if (CheckConnection())
            m_Writer->WriteFrame();
    }

    void OnMessageReceived(const SDebugStreamMessage &) override { QT3DS_ASSERT(false); }
};
}

ISceneGraphRuntimeDebugger &ISceneGraphRuntimeDebugger::Create(NVFoundationBase &fnd,
                                                               IStringTable &strTable)
{
    return *QT3DS_NEW(fnd.getAllocator(), RuntimeDebuggerImpl)(fnd, strTable);
}
