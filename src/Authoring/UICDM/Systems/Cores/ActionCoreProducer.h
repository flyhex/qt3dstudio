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
#ifndef ACTIONCOREPRODUCERH
#define ACTIONCOREPRODUCERH

#include "SimpleActionCore.h"
#include "UICDMTransactions.h"
#include "UICDMSignals.h"
#include "UICDMStringTable.h"

namespace qt3dsdm {
class CActionCoreProducer : public IActionCore,
                            public ITransactionProducer,
                            public IActionCoreSignalProvider
{
    Q_DISABLE_COPY(CActionCoreProducer)

    TSimpleActionCorePtr m_Data;
    TTransactionConsumerPtr m_Consumer;
    TSignalItemPtr m_Signaller;

public:
    CActionCoreProducer(TStringTablePtr inStringTable)
        : m_Data(new CSimpleActionCore(inStringTable))
    {
        InitSignaller();
    }

    TSimpleActionCorePtr GetTransactionlessActionCore() const { return m_Data; }

    // IActionCore implementation
    IStringTable &GetStringTable() const override { return m_Data->GetStringTable(); }
    TStringTablePtr GetStringTablePtr() const override { return m_Data->GetStringTablePtr(); }
    // Action
    CUICDMActionHandle CreateAction(Qt3DSDMInstanceHandle inInstance, CUICDMSlideHandle inSlide,
                                    Qt3DSDMInstanceHandle inOwner, SLong4 inTriggerTargetObjects) override;
    void DeleteAction(CUICDMActionHandle inAction, Qt3DSDMInstanceHandle &outInstance) override;
    const SActionInfo &GetActionInfo(CUICDMActionHandle inAction) const override;
    void GetActions(CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                    TActionHandleList &outActions) const override;
    void GetActions(CUICDMSlideHandle inSlide, TActionHandleList &outActions) const override;
    void GetActions(Qt3DSDMInstanceHandle inOwner, TActionHandleList &outActions) const override;
    void GetActions(TActionHandleList &outActions) const override;
    Qt3DSDMInstanceHandle GetActionInstance(CUICDMActionHandle inAction) const override;
    CUICDMActionHandle GetActionByInstance(Qt3DSDMInstanceHandle inActionInstance) const override;

    // Action Properties
    void SetTriggerObject(CUICDMActionHandle inAction, const SObjectRefType &inTriggerObject) override;
    void SetTargetObject(CUICDMActionHandle inAction, const SObjectRefType &inTargetObject) override;
    void SetEvent(CUICDMActionHandle inAction, const wstring &inEventHandle) override;
    void SetHandler(CUICDMActionHandle inAction, const wstring &inHandlerHandle) override;

    // Action Argument
    CUICDMHandlerArgHandle AddHandlerArgument(CUICDMActionHandle inAction, const TCharStr &inName,
                                              HandlerArgumentType::Value inArgType,
                                              DataModelDataType::Value inValueType) override;
    void RemoveHandlerArgument(CUICDMHandlerArgHandle inHandlerArgument) override;
    const SHandlerArgumentInfo &
    GetHandlerArgumentInfo(CUICDMHandlerArgHandle inHandlerArgument) const override;
    void GetHandlerArguments(CUICDMActionHandle inAction,
                             THandlerArgHandleList &outHandlerArguments) const override;

    // Action Argument Properties
    void GetHandlerArgumentValue(CUICDMHandlerArgHandle inHandlerArgument, SValue &outValue) const override;
    void SetHandlerArgumentValue(CUICDMHandlerArgHandle inHandlerArgument, const SValue &inValue) override;

    // CHandleBase
    bool HandleValid(int inHandle) const override;

    // ITransactionProducer implementation
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    TSignalConnectionPtr ConnectTriggerObjectSet(
        const std::function<void(CUICDMActionHandle, SObjectRefType &)> &inCallback) override;
    TSignalConnectionPtr ConnectTargetObjectSet(
        const std::function<void(CUICDMActionHandle, SObjectRefType &)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(CUICDMActionHandle, const wstring &)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(CUICDMActionHandle, const wstring &)> &inCallback) override;

    TSignalConnectionPtr ConnectHandlerArgumentAdded(
        const std::function<void(CUICDMActionHandle, CUICDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override;
    TSignalConnectionPtr ConnectHandlerArgumentRemoved(
        const std::function<void(CUICDMActionHandle, CUICDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override;
    TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(CUICDMHandlerArgHandle, const SValue &)> &inCallback) override;

private:
    void InitSignaller();
    IActionCoreSignalProvider *GetSignalProvider();
    IActionCoreSignalSender *GetSignalSender();
};
}

#endif
