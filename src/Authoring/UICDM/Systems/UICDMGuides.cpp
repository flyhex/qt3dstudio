/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "UICDMPrefix.h"
#include "UICDMGuides.h"
#include <unordered_map>
#include "boost/signals.hpp"
#include <boost/signals/connection.hpp>
#include "VectorTransactions.h"

using namespace qt3dsdm;

namespace {

struct SBoostSignalConnection : public ISignalConnection
{
    Q_DISABLE_COPY(SBoostSignalConnection)
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection m_connection;
    SBoostSignalConnection(const boost::BOOST_SIGNALS_NAMESPACE::connection &inConnection)
        : m_connection(inConnection)
    {
    }
};

#define CONNECT(x) std::make_shared<SBoostSignalConnection>(x.connect(inCallback))

struct SGuideSystem : public IGuideSystem
{
    typedef std::unordered_map<long, SGuideInfo> TGuideMap;
    typedef std::shared_ptr<IMergeableTransaction<SGuideInfo>> TMergeableTransaction;
    typedef std::unordered_map<long, TMergeableTransaction> TGuideInfoMergeMap;

    long m_NextHandleValue;
    TGuideMap m_Guides;
    bool m_GuidesEditable;

    std::shared_ptr<ITransactionConsumer> m_CurrentTransaction;
    TGuideInfoMergeMap m_GuideMergeMap;

    boost::signal<void(CUICDMGuideHandle, SGuideInfo)> m_GuideCreated;
    boost::signal<void(CUICDMGuideHandle, SGuideInfo)> m_GuideDestroyed;
    boost::signal<void(CUICDMGuideHandle, SGuideInfo)> m_GuideModified;
    boost::signal<void(CUICDMGuideHandle, SGuideInfo)> m_GuideModifiedImmediate;

    SGuideSystem()
        : m_NextHandleValue(0)
        , m_GuidesEditable(true)
    {
    }

    void SignalGuideCreated(long hdl, const SGuideInfo &inInfo) { m_GuideCreated(hdl, inInfo); }

    void SignalGuideDestroyed(long hdl, const SGuideInfo &inInfo) { m_GuideDestroyed(hdl, inInfo); }

    void SignalGuideModified(long hdl, const SGuideInfo &inInfo) { m_GuideModified(hdl, inInfo); }

    CUICDMGuideHandle CreateGuide() override
    {
        ++m_NextHandleValue;
        std::pair<long, SGuideInfo> entry(std::make_pair(m_NextHandleValue, SGuideInfo()));
        m_Guides.insert(entry);
        if (m_CurrentTransaction) {
            CreateHashMapInsertTransaction(__FILE__, __LINE__, m_CurrentTransaction, entry,
                                           m_Guides);
            m_CurrentTransaction->OnDoNotification(std::bind(
                &SGuideSystem::SignalGuideCreated, this, m_NextHandleValue, SGuideInfo()));
            m_CurrentTransaction->OnUndoNotification(std::bind(
                &SGuideSystem::SignalGuideDestroyed, this, m_NextHandleValue, SGuideInfo()));
        }

        return m_NextHandleValue;
    }

    SGuideInfo *InternalGetGuideInfo(CUICDMGuideHandle inGuideHandle)
    {
        TGuideMap::iterator theFind = m_Guides.find((long)inGuideHandle.GetHandleValue());
        if (theFind != m_Guides.end())
            return &theFind->second;
        return NULL;
    }

    const SGuideInfo *InternalGetGuideInfo(CUICDMGuideHandle inGuideHandle) const
    {
        return const_cast<SGuideSystem &>(*this).InternalGetGuideInfo(inGuideHandle);
    }

    void SetGuideInfo(CUICDMGuideHandle inGuideHandle, const SGuideInfo &info) override
    {
        SGuideInfo *existing = InternalGetGuideInfo(inGuideHandle);
        long theHdlValue = (long)inGuideHandle.GetHandleValue();
        TGuideInfoMergeMap::iterator iter = m_GuideMergeMap.find(theHdlValue);
        if (iter != m_GuideMergeMap.end()) {
            iter->second->Update(info);
            *existing = info;
        } else {
            if (!existing) {
                QT3DS_ASSERT(false);
                return;
            }
            SGuideInfo oldValue(*existing);
            *existing = info;
            if (m_CurrentTransaction) {
                TMergeableTransaction newTransaction =
                    CreateHashMapSwapTransaction(__FILE__, __LINE__, m_CurrentTransaction,
                                                 theHdlValue, oldValue, info, m_Guides);
                m_GuideMergeMap.insert(std::make_pair(theHdlValue, newTransaction));
                m_CurrentTransaction->OnDoNotification(
                    std::bind(&SGuideSystem::SignalGuideModified, this, theHdlValue, info));
                m_CurrentTransaction->OnUndoNotification(
                    std::bind(&SGuideSystem::SignalGuideModified, this, theHdlValue, oldValue));
            }
        }
        if (AreUICDMSignalsEnabled())
            m_GuideModifiedImmediate(theHdlValue, info);
    }

    SGuideInfo GetGuideInfo(CUICDMGuideHandle inGuideHandle) const override
    {
        const SGuideInfo *existing = InternalGetGuideInfo(inGuideHandle);
        if (existing)
            return *existing;
        QT3DS_ASSERT(false);
        return SGuideInfo();
    }

    void DeleteGuide(CUICDMGuideHandle inGuideHandle) override
    {

        SGuideInfo *existing = InternalGetGuideInfo(inGuideHandle);
        if (!existing) {
            QT3DS_ASSERT(false);
            return;
        }
        long theHdlValue = (long)inGuideHandle.GetHandleValue();
        SGuideInfo oldValue = *existing;
        m_Guides.erase(theHdlValue);

        if (m_CurrentTransaction) {
            std::pair<long, SGuideInfo> entry(std::make_pair(theHdlValue, oldValue));
            CreateHashMapEraseTransaction(__FILE__, __LINE__, m_CurrentTransaction, entry,
                                          m_Guides);
            m_CurrentTransaction->OnDoNotification(
                std::bind(&SGuideSystem::SignalGuideDestroyed, this, theHdlValue, oldValue));
            m_CurrentTransaction->OnUndoNotification(
                std::bind(&SGuideSystem::SignalGuideCreated, this, theHdlValue, oldValue));
        }
    }

    TGuideHandleList GetAllGuides() const override
    {
        TGuideHandleList retval;
        for (TGuideMap::const_iterator iter = m_Guides.begin(), end = m_Guides.end(); iter != end;
             ++iter)
            retval.push_back(iter->first);
        return retval;
    }

    bool IsGuideValid(CUICDMGuideHandle inGuideHandle) const override
    {
        return InternalGetGuideInfo(inGuideHandle) != NULL;
    }
    bool AreGuidesEditable() const override { return m_GuidesEditable; }
    void SetGuidesEditable(bool val) override { m_GuidesEditable = val; }

    // Undo/Redo
    void SetConsumer(std::shared_ptr<ITransactionConsumer> inConsumer) override
    {
        m_CurrentTransaction = inConsumer;
        m_GuideMergeMap.clear();
    }

    // These are events coming from undo/redo operations, not events coming directly from the
    // modification of the guides
    virtual TSignalConnectionPtr
    ConnectGuideCreated(const std::function<void(CUICDMGuideHandle, SGuideInfo)> &inCallback) override
    {
        return CONNECT(m_GuideCreated);
    }

    virtual TSignalConnectionPtr
    ConnectGuideDestroyed(const std::function<void(CUICDMGuideHandle, SGuideInfo)> &inCallback) override
    {
        return CONNECT(m_GuideDestroyed);
    }

    virtual TSignalConnectionPtr
    ConnectGuideModified(const std::function<void(CUICDMGuideHandle, SGuideInfo)> &inCallback) override
    {
        return CONNECT(m_GuideModified);
    }

    // Signal happens immediately instead of on undo/redo, used for live-update of the inspector
    // palette
    TSignalConnectionPtr ConnectGuideModifiedImmediate(
        const std::function<void(CUICDMGuideHandle, SGuideInfo)> &inCallback) override
    {
        return CONNECT(m_GuideModifiedImmediate);
    }
};
}

shared_ptr<IGuideSystem> IGuideSystem::CreateGuideSystem()
{
    return std::make_shared<SGuideSystem>();
}
