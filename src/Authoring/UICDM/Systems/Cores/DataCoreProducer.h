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
#pragma once
#ifndef DATACOREPRODUCERH
#define DATACOREPRODUCERH

#include "UICDMTransactions.h"
#include "SimpleDataCore.h"
#include "UICDMSignals.h"
#include "SimpleSlideCore.h"

namespace UICDM {

class CDataCoreProducer : public IDataCore,
                          public ITransactionProducer,
                          public IInstancePropertyCoreSignalProvider,
                          public IDataCoreSignalProvider
{
    typedef std::shared_ptr<IMergeableTransaction<TPropertyValuePair>> TPropertyMergeMapEntry;
    typedef std::unordered_map<TSlideInstancePropertyPair, TPropertyMergeMapEntry>
        TPropertyMergeMap;

    TTransactionConsumerPtr m_Consumer;
    TSimpleDataCorePtr m_Data;
    TSignalItemPtr m_InstancePropertyCoreSignaller;
    TSignalItemPtr m_DataCoreSignaller;

    TPropertyMergeMap m_PropertyMergeMap;

public:
    CDataCoreProducer(TStringTablePtr inStringTable)
        : m_Data(new CSimpleDataCore(inStringTable))
    {
        InitSignallers();
    }
    virtual ~CDataCoreProducer() {}

    IStringTable &GetStringTable() const override { return m_Data->GetStringTable(); }
    std::shared_ptr<IStringTable> GetStringTablePtr() const override
    {
        return m_Data->GetStringTablePtr();
    }

    // IHandleBase
    bool HandleValid(int inHandle) const override;

    // IInstancePropertyCore
    //===============================================================
    CUICDMPropertyHandle GetAggregateInstancePropertyByName(CUICDMInstanceHandle inInstance,
                                                            const TCharStr &inStr) const override;
    void GetAggregateInstanceProperties(CUICDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const override;
    void GetSpecificInstancePropertyValues(CUICDMInstanceHandle inHandle,
                                                   TPropertyHandleValuePairList &outValues) override;
    bool HasAggregateInstanceProperty(CUICDMInstanceHandle inInstance,
                                              CUICDMPropertyHandle inProperty) const override;

    void CheckValue(CUICDMInstanceHandle inInstance, CUICDMPropertyHandle inProperty,
                    const SValue &inValue) const override;
    bool GetInstancePropertyValue(CUICDMInstanceHandle inHandle,
                                          CUICDMPropertyHandle inProperty, SValue &outValue) const override;
    void SetInstancePropertyValue(CUICDMInstanceHandle inHandle,
                                          CUICDMPropertyHandle inProperty, const SValue &inValue) override;
    //===============================================================

    // IDataCore
    //===============================================================
    CUICDMInstanceHandle CreateInstance(CUICDMInstanceHandle hdl = CUICDMInstanceHandle()) override;
    void GetInstances(TInstanceHandleList &outInstances) const override;
    void GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                         CUICDMInstanceHandle inParent) const override;
    void DeleteInstance(CUICDMInstanceHandle inHandle) override;

    void DeriveInstance(CUICDMInstanceHandle inInstance, CUICDMInstanceHandle inParent) override;
    void GetInstanceParents(CUICDMInstanceHandle inHandle,
                                    TInstanceHandleList &outParents) const override;
    bool IsInstanceOrDerivedFrom(CUICDMInstanceHandle inInstance,
                                         CUICDMInstanceHandle inParent) const override;

    CUICDMPropertyHandle AddProperty(CUICDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) override;
    void GetInstanceProperties(CUICDMInstanceHandle inInstance,
                                       TPropertyHandleList &outProperties) const override;
    const SUICDMPropertyDefinition &GetProperty(CUICDMPropertyHandle inProperty) const override;
    void RemoveProperty(CUICDMPropertyHandle inProperty) override;
    void CopyInstanceProperties(CUICDMInstanceHandle inSrcInstance,
                                        CUICDMInstanceHandle inDestInstance) override;

    void RemoveCachedValues(CUICDMInstanceHandle inInstance) override
    {
        m_Data->RemoveCachedValues(inInstance);
    }
    bool IsInstance(int inHandle) const override { return m_Data->IsInstance(inHandle); }
    bool IsProperty(int inHandle) const override { return m_Data->IsProperty(inHandle); }
    //===============================================================

    //===============================================================
    //	Set the current consumer
    //===============================================================
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    //===============================================================
    //	Return a serializable data model for load/save
    //===============================================================
    virtual TSimpleDataCorePtr GetTransactionlessDataCore() { return m_Data; }
    virtual TSimpleDataCorePtr GetTransactionlessDataCore() const { return m_Data; }

    //===============================================================
    //	Signal provider implementation
    //===============================================================
    TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(CUICDMInstanceHandle, CUICDMPropertyHandle, const SValue &)>
            &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(CUICDMInstanceHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(CUICDMInstanceHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectBeforeInstanceDeleted(const std::function<void(CUICDMInstanceHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectInstanceDerived(
        const std::function<void(CUICDMInstanceHandle, CUICDMInstanceHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectInstanceParentRemoved(
        const std::function<void(CUICDMInstanceHandle, CUICDMInstanceHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectPropertyAdded(const std::function<void(CUICDMInstanceHandle, CUICDMPropertyHandle,
                                                    TCharPtr, DataModelDataType::Value)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectPropertyRemoved(const std::function<void(CUICDMInstanceHandle, CUICDMPropertyHandle,
                                                      TCharPtr, DataModelDataType::Value)> &inCallback) override;

private:
    CDataCoreProducer(const CDataCoreProducer&) = delete;
    CDataCoreProducer& operator=(const CDataCoreProducer&) = delete;

    template <typename TTransactionType>
    inline void RunWithConsumer(TTransactionType inTransaction)
    {
        UICDM::RunWithConsumer(m_Consumer, inTransaction);
    }

    void InitSignallers();
    IInstancePropertyCoreSignalProvider *GetPropertyCoreProvider();
    IInstancePropertyCoreSignalSender *GetPropertyCoreSender();
    IDataCoreSignalProvider *GetDataCoreProvider();
    IDataCoreSignalSender *GetDataCoreSender();
};
}

#endif
