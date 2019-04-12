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
#include "Qt3DSStateContext.h"
#include "Qt3DSStateXMLIO.h"
#include "foundation/XML.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"

using namespace qt3ds::state;

namespace {

struct SStateContext : public IStateContext
{
    NVFoundationBase &m_Foundation;
    TIDStateMap m_IDStateMap;
    TSendList m_SendList;
    SSCXML *m_Root;

    // the factory needs to be here in order that we can easily just assign variables
    // left and right and make things work.
    NVScopedRefCounted<IDOMFactory> m_DOMFactory;
    TPtrExtensionMap m_ExtensionInfo;
    SNamespacePairNode *m_NamespacePairs;
    QT3DSI32 mRefCount;

    SStateContext(NVFoundationBase &alloc)
        : m_Foundation(alloc)
        , m_IDStateMap(alloc.getAllocator(), "SStateContext::m_IDStateMap")
        , m_SendList(alloc.getAllocator(), "SStateContext::m_SendList")
        , m_Root(NULL)
        , m_DOMFactory(NULL)
        , m_ExtensionInfo(alloc.getAllocator(), "SStateContext::m_ExtensionInfo")
        , m_NamespacePairs(NULL)
        , mRefCount(0)
    {
    }
    virtual ~SStateContext() {}
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())
    void SetDOMFactory(IDOMFactory *inFactory) override;

    void SetRoot(SSCXML &inRoot) override
    {
        QT3DS_ASSERT(m_Root == NULL);
        if (!m_Root)
            m_Root = &inRoot;
    }
    SSCXML *GetRoot() override { return m_Root; }
    void AddSendToList(SSend &inSend) override { m_SendList.push_back(&inSend); }
    NVConstDataRef<SSend *> GetSendList() override
    {
        return toConstDataRef(m_SendList.data(), (QT3DSU32)m_SendList.size());
    }
    IDOMFactory *GetDOMFactory() override { return m_DOMFactory; }
    NVConstDataRef<NVScopedRefCounted<IStateContext>> GetSubContexts() override
    {
        return NVConstDataRef<NVScopedRefCounted<IStateContext>>();
    }

    // UICStateInterpreter.cpp;
    bool InsertId(const CRegisteredString &inId, const SIdValue &inValue) override;
    void EraseId(const CRegisteredString &inId) override;
    bool ContainsId(const CRegisteredString &inStr) override;
    SStateNode *FindStateNode(const CRegisteredString &inStr) override;
    SSend *FindSend(const CRegisteredString &inStr) override;

    SItemExtensionInfo *GetExtensionInfo(void *inItem) override;
    SItemExtensionInfo &GetOrCreateExtensionInfo(void *inItem) override;

    void SetFirstNSNode(SNamespacePairNode &inNode) override { m_NamespacePairs = &inNode; }
    SNamespacePairNode *GetFirstNSNode() override { return m_NamespacePairs; }

    void Save(IOutStream &inOutStream, editor::IEditor *inEditor) override;
};

void SStateContext::SetDOMFactory(IDOMFactory *inFactory)
{
    m_DOMFactory = inFactory;
}

bool SStateContext::InsertId(const CRegisteredString &inId, const SIdValue &inValue)
{
    return m_IDStateMap.insert(eastl::make_pair(inId, inValue)).second;
}

void SStateContext::EraseId(const CRegisteredString &inId)
{
    m_IDStateMap.erase(inId);
}

bool SStateContext::ContainsId(const CRegisteredString &inStr)
{
    return m_IDStateMap.find(inStr) != m_IDStateMap.end();
}

SStateNode *SStateContext::FindStateNode(const CRegisteredString &inStr)
{
    TIDStateMap::iterator iter = m_IDStateMap.find(inStr);

    if (iter != m_IDStateMap.end()) {
        IdValueTypes::Enum typeEnum = iter->second.getType();
        if (typeEnum == IdValueTypes::StateNode)
            return iter->second.getData<SStateNode *>();
    }
    return NULL;
}

SSend *SStateContext::FindSend(const CRegisteredString &inStr)
{
    TIDStateMap::iterator iter = m_IDStateMap.find(inStr);
    if (iter != m_IDStateMap.end() && iter->second.getType() == IdValueTypes::Send)
        return iter->second.getData<SSend *>();
    return NULL;
}

SItemExtensionInfo *SStateContext::GetExtensionInfo(void *inItem)
{
    TPtrExtensionMap::iterator iter = m_ExtensionInfo.find(inItem);
    if (iter != m_ExtensionInfo.end())
        return &iter->second;
    return NULL;
}

SItemExtensionInfo &SStateContext::GetOrCreateExtensionInfo(void *inItem)
{
    return m_ExtensionInfo.insert(eastl::make_pair(inItem, SItemExtensionInfo(inItem)))
        .first->second;
}

void SStateContext::Save(IOutStream &inOutStream, editor::IEditor *inEditor)
{
    CXMLIO::SaveSCXMLFile(*this, m_Foundation, *m_DOMFactory->GetStringTable(), inOutStream,
                          inEditor);
}
}

IStateContext *IStateContext::Load(NVAllocatorCallback &inGraphAllocator,
                                   NVFoundationBase &inFoundation, IInStream &inStream,
                                   const char8_t *inFilename, IStringTable *inStrTable,
                                   editor::IEditor *inEditor)
{

    NVScopedRefCounted<IStringTable> theStringTable = inStrTable;
    if (!theStringTable)
        theStringTable = IStringTable::CreateStringTable(inFoundation.getAllocator());
    NVScopedRefCounted<IDOMFactory> theFactory =
        IDOMFactory::CreateDOMFactory(inFoundation.getAllocator(), theStringTable);
    eastl::pair<SNamespacePairNode *, SDOMElement *> readResult =
        CDOMSerializer::Read(*theFactory, inStream);
    SDOMElement *elem = readResult.second;
    if (elem == NULL)
        return NULL;

    NVScopedRefCounted<IDOMReader> theReader = IDOMReader::CreateDOMReader(
        inFoundation.getAllocator(), *elem, theStringTable, *theFactory);
    IStateContext *retval = QT3DS_NEW(inFoundation.getAllocator(), SStateContext)(inFoundation);
    retval->SetDOMFactory(theFactory.mPtr);
    CXMLIO::LoadSCXMLFile(inGraphAllocator, inFoundation, *theReader, *theStringTable, inFilename,
                          *retval, inEditor);
    if (readResult.first)
        retval->SetFirstNSNode(*readResult.first);
    return retval;
}

IStateContext *IStateContext::Create(NVFoundationBase &inFoundation)
{
    NVScopedRefCounted<IStringTable> theStringTable =
        IStringTable::CreateStringTable(inFoundation.getAllocator());
    NVScopedRefCounted<IDOMFactory> theFactory =
        IDOMFactory::CreateDOMFactory(inFoundation.getAllocator(), theStringTable);

    SStateContext *retval = QT3DS_NEW(inFoundation.getAllocator(), SStateContext)(inFoundation);
    retval->SetDOMFactory(theFactory);
    return retval;
}
