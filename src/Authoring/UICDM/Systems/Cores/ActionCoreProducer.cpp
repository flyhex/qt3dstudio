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
#include "ActionCoreProducer.h"
#include "HandleSystemTransactions.h"
#include "VectorTransactions.h"
#include "SignalsImpl.h"

using namespace std;

namespace UICDM {

CUICDMActionHandle CActionCoreProducer::CreateAction(CUICDMInstanceHandle inInstance,
                                                     CUICDMSlideHandle inSlide,
                                                     CUICDMInstanceHandle inOwner,
                                                     SLong4 inTriggerTargetObjects)
{
    CUICDMActionHandle retval =
        m_Data->CreateAction(inInstance, inSlide, inOwner, inTriggerTargetObjects);
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    return retval;
}

void CActionCoreProducer::DeleteAction(CUICDMActionHandle inAction,
                                       CUICDMInstanceHandle &outInstance)
{
    // Ensure action exists
    SAction *theAction = CSimpleActionCore::GetActionNF(inAction, m_Data->m_Objects);
    CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inAction, m_Data->m_Objects);
    do_all(theAction->m_ActionInfo.m_HandlerArgs,
           std::bind(DoCreateHandleDeleteTransaction, __FILE__, __LINE__, m_Consumer,
                     std::placeholders::_1, std::ref(m_Data->m_Objects)));
    m_Data->DeleteAction(inAction, outInstance);
}

const SActionInfo &CActionCoreProducer::GetActionInfo(CUICDMActionHandle inAction) const
{
    return m_Data->GetActionInfo(inAction);
}

void CActionCoreProducer::GetActions(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                                     TActionHandleList &outActions) const
{
    return m_Data->GetActions(inSlide, inOwner, outActions);
}

void CActionCoreProducer::GetActions(CUICDMSlideHandle inSlide, TActionHandleList &outActions) const
{
    return m_Data->GetActions(inSlide, outActions);
}

void CActionCoreProducer::GetActions(CUICDMInstanceHandle inOwner,
                                     TActionHandleList &outActions) const
{
    return m_Data->GetActions(inOwner, outActions);
}

void CActionCoreProducer::GetActions(TActionHandleList &outActions) const
{
    return m_Data->GetActions(outActions);
}

CUICDMInstanceHandle CActionCoreProducer::GetActionInstance(CUICDMActionHandle inAction) const
{
    return m_Data->GetActionInstance(inAction);
}

CUICDMActionHandle
CActionCoreProducer::GetActionByInstance(CUICDMInstanceHandle inActionInstance) const
{
    return m_Data->GetActionByInstance(inActionInstance);
}

void CActionCoreProducer::SetTriggerObject(CUICDMActionHandle inAction,
                                           const SObjectRefType &inTriggerObject)
{
    SAction *theAction = CSimpleActionCore::GetActionNF(inAction, m_Data->m_Objects);
    if (m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSimpleActionCore::SetTriggerObject, m_Data, inAction, inTriggerObject),
            std::bind(&CSimpleActionCore::SetTriggerObject, m_Data, inAction,
                        theAction->m_ActionInfo.m_TriggerObject))));
    }
    m_Data->SetTriggerObject(inAction, inTriggerObject);
    GetSignalSender()->SendTriggerObjectSet(inAction, theAction->m_ActionInfo.m_TriggerObject);
}

void CActionCoreProducer::SetTargetObject(CUICDMActionHandle inAction,
                                          const SObjectRefType &inTargetObject)
{
    SAction *theAction = CSimpleActionCore::GetActionNF(inAction, m_Data->m_Objects);
    if (m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSimpleActionCore::SetTargetObject, m_Data, inAction, inTargetObject),
            std::bind(&CSimpleActionCore::SetTargetObject, m_Data, inAction,
                        theAction->m_ActionInfo.m_TargetObject))));
    }
    m_Data->SetTargetObject(inAction, inTargetObject);
    GetSignalSender()->SendTargetObjectSet(inAction, theAction->m_ActionInfo.m_TargetObject);
}

void CActionCoreProducer::SetEvent(CUICDMActionHandle inAction, const wstring &inEventHandle)
{
    SAction *theAction = CSimpleActionCore::GetActionNF(inAction, m_Data->m_Objects);
    if (m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSimpleActionCore::SetEvent, m_Data, inAction, inEventHandle),
            std::bind(&CSimpleActionCore::SetEvent, m_Data, inAction,
                        theAction->m_ActionInfo.m_Event))));
    }
    m_Data->SetEvent(inAction, inEventHandle);
    GetSignalSender()->SendEventSet(inAction, inEventHandle);
}

void CActionCoreProducer::SetHandler(CUICDMActionHandle inAction, const wstring &inHandlerHandle)
{
    SAction *theAction = CSimpleActionCore::GetActionNF(inAction, m_Data->m_Objects);
    if (m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSimpleActionCore::SetHandler, m_Data, inAction, inHandlerHandle),
            std::bind(&CSimpleActionCore::SetHandler, m_Data, inAction,
                        theAction->m_ActionInfo.m_Handler))));
    }
    m_Data->SetHandler(inAction, inHandlerHandle);
    GetSignalSender()->SendHandlerSet(inAction, inHandlerHandle);
}

CUICDMHandlerArgHandle CActionCoreProducer::AddHandlerArgument(CUICDMActionHandle inAction,
                                                               const TCharStr &inName,
                                                               HandlerArgumentType::Value inArgType,
                                                               DataModelDataType::Value inValueType)
{
    CUICDMHandlerArgHandle retval =
        m_Data->AddHandlerArgument(inAction, inName, inArgType, inValueType);
    SAction *theAction = CSimpleActionCore::GetActionNF(inAction, m_Data->m_Objects);
    CreateVecInsertTransaction<CUICDMHandlerArgHandle>(__FILE__, __LINE__, m_Consumer, retval,
                                                       theAction->m_ActionInfo.m_HandlerArgs);
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    GetSignalSender()->SendHandlerArgumentAdded(inAction, retval, inName, inArgType, inValueType);
    return retval;
}

void CActionCoreProducer::RemoveHandlerArgument(CUICDMHandlerArgHandle inHandlerArgument)
{
    SHandlerArgument *theHandlerArgument =
        CSimpleActionCore::GetHandlerArgumentNF(inHandlerArgument, m_Data->m_Objects);
    SAction *theAction = CSimpleActionCore::GetActionNF(
        theHandlerArgument->m_HandlerArgInfo.m_Action, m_Data->m_Objects);
    if (exists(theAction->m_ActionInfo.m_HandlerArgs,
               std::bind(equal_to<CUICDMHandlerArgHandle>(), inHandlerArgument,
                         std::placeholders::_1))) {
        CreateVecEraseTransaction<CUICDMHandlerArgHandle>(__FILE__, __LINE__, m_Consumer,
                                                          inHandlerArgument,
                                                          theAction->m_ActionInfo.m_HandlerArgs);
        CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inHandlerArgument, m_Data->m_Objects);
    }
    m_Data->RemoveHandlerArgument(inHandlerArgument);
    GetSignalSender()->SendHandlerArgumentRemoved(theAction->m_Handle, theHandlerArgument->m_Handle,
                                                  theHandlerArgument->m_HandlerArgInfo.m_Name,
                                                  theHandlerArgument->m_HandlerArgInfo.m_ArgType,
                                                  theHandlerArgument->m_HandlerArgInfo.m_ValueType);
}

const SHandlerArgumentInfo &
CActionCoreProducer::GetHandlerArgumentInfo(CUICDMHandlerArgHandle inHandlerArgument) const
{
    return m_Data->GetHandlerArgumentInfo(inHandlerArgument);
}

void CActionCoreProducer::GetHandlerArguments(CUICDMActionHandle inAction,
                                              THandlerArgHandleList &outHandlerArguments) const
{
    return m_Data->GetHandlerArguments(inAction, outHandlerArguments);
}

void CActionCoreProducer::GetHandlerArgumentValue(CUICDMHandlerArgHandle inHandlerArgument,
                                                  SValue &outValue) const
{
    return m_Data->GetHandlerArgumentValue(inHandlerArgument, outValue);
}

void CActionCoreProducer::SetHandlerArgumentValue(CUICDMHandlerArgHandle inHandlerArgument,
                                                  const SValue &inValue)
{
    SHandlerArgument *theHandlerArgument =
        CSimpleActionCore::GetHandlerArgumentNF(inHandlerArgument, m_Data->m_Objects);
    if (m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSimpleActionCore::SetHandlerArgumentValue, m_Data, inHandlerArgument,
                        inValue),
            std::bind(&CSimpleActionCore::SetHandlerArgumentValue, m_Data, inHandlerArgument,
                        theHandlerArgument->m_HandlerArgInfo.m_Value))));
    }
    m_Data->SetHandlerArgumentValue(inHandlerArgument, inValue);
    GetSignalSender()->SendHandlerArgumentValueSet(inHandlerArgument, inValue);
}

// CHandleBase
bool CActionCoreProducer::HandleValid(int inHandle) const
{
    return m_Data->HandleValid(inHandle);
}

// ITransactionProducer implementation
void CActionCoreProducer::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_Consumer = inConsumer;
}

TSignalConnectionPtr CActionCoreProducer::ConnectTriggerObjectSet(
    const std::function<void(CUICDMActionHandle, SObjectRefType &)> &inCallback)
{
    return GetSignalProvider()->ConnectTriggerObjectSet(inCallback);
}
TSignalConnectionPtr CActionCoreProducer::ConnectTargetObjectSet(
    const std::function<void(CUICDMActionHandle, SObjectRefType &)> &inCallback)
{
    return GetSignalProvider()->ConnectTargetObjectSet(inCallback);
}
TSignalConnectionPtr CActionCoreProducer::ConnectEventSet(
    const std::function<void(CUICDMActionHandle, const wstring &)> &inCallback)
{
    return GetSignalProvider()->ConnectEventSet(inCallback);
}
TSignalConnectionPtr CActionCoreProducer::ConnectHandlerSet(
    const std::function<void(CUICDMActionHandle, const wstring &)> &inCallback)
{
    return GetSignalProvider()->ConnectHandlerSet(inCallback);
}

TSignalConnectionPtr CActionCoreProducer::ConnectHandlerArgumentAdded(
    const std::function<void(CUICDMActionHandle, CUICDMHandlerArgHandle, const TCharStr &,
                               HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback)
{
    return GetSignalProvider()->ConnectHandlerArgumentAdded(inCallback);
}
TSignalConnectionPtr CActionCoreProducer::ConnectHandlerArgumentRemoved(
    const std::function<void(CUICDMActionHandle, CUICDMHandlerArgHandle, const TCharStr &,
                               HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback)
{
    return GetSignalProvider()->ConnectHandlerArgumentRemoved(inCallback);
}
TSignalConnectionPtr CActionCoreProducer::ConnectHandlerArgumentValueSet(
    const std::function<void(CUICDMHandlerArgHandle, const SValue &)> &inCallback)
{
    return GetSignalProvider()->ConnectHandlerArgumentValueSet(inCallback);
}

void CActionCoreProducer::InitSignaller()
{
    m_Signaller = CreateActionCoreSignaller();
}

IActionCoreSignalProvider *CActionCoreProducer::GetSignalProvider()
{
    return dynamic_cast<IActionCoreSignalProvider *>(m_Signaller.get());
}

IActionCoreSignalSender *CActionCoreProducer::GetSignalSender()
{
    return dynamic_cast<IActionCoreSignalSender *>(m_Signaller.get());
}
}
