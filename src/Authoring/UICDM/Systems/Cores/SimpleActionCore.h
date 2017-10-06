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
#ifndef ACTIONCOREH
#define ACTIONCOREH

#include "UICDMActionCore.h"
#include "HandleSystemBase.h"
#include "UICDMErrors.h"

namespace UICDM {
struct SAction : public CHandleObject
{
    SActionInfo m_ActionInfo;

    SAction() {}

    SAction(int inHandle, CUICDMInstanceHandle inInstance, CUICDMSlideHandle inSlide,
            CUICDMInstanceHandle inOwner)
        : CHandleObject(inHandle)
        , m_ActionInfo(inInstance, inSlide, inOwner)
    {
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeAction;
    EHandleObjectType GetType() override { return s_Type; }
};

struct SHandlerArgument : public CHandleObject
{
    SHandlerArgumentInfo m_HandlerArgInfo;

    SHandlerArgument() {}

    SHandlerArgument(int inHandle, CUICDMActionHandle inAction, const TCharStr &inName,
                     HandlerArgumentType::Value inArgType, DataModelDataType::Value inValueType)
        : CHandleObject(inHandle)
        , m_HandlerArgInfo(inAction, inName, inArgType, inValueType)
    {
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeActionHandlerArgument;
    EHandleObjectType GetType() override { return s_Type; }
};

class CSimpleActionCore : public CHandleBase, public IActionCore
{
    mutable TStringTablePtr m_StringTable;

public: // Use
    CSimpleActionCore(TStringTablePtr strTable)
        : m_StringTable(strTable)
    {
    }

    IStringTable &GetStringTable() const override { return *m_StringTable.get(); }
    TStringTablePtr GetStringTablePtr() const override { return m_StringTable; }
    // Action
    CUICDMActionHandle CreateAction(CUICDMInstanceHandle inInstance, CUICDMSlideHandle inSlide,
                                    CUICDMInstanceHandle inOwner, SLong4 inTriggerTargetObjects) override;
    void DeleteAction(CUICDMActionHandle inAction, CUICDMInstanceHandle &outInstance) override;
    const SActionInfo &GetActionInfo(CUICDMActionHandle inAction) const override;
    void GetActions(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                    TActionHandleList &outActions) const override;
    void GetActions(CUICDMSlideHandle inSlide, TActionHandleList &outActions) const override;
    void GetActions(CUICDMInstanceHandle inOwner, TActionHandleList &outActions) const override;
    void GetActions(TActionHandleList &outActions) const override;

    // Return the instance that was allocated for this action.
    CUICDMInstanceHandle GetActionInstance(CUICDMActionHandle inAction) const override;
    // Reverse lookup into the action system so you can match actions to instances.
    CUICDMActionHandle GetActionByInstance(CUICDMInstanceHandle inActionInstance) const override;

    // Action Properties
    void SetTriggerObject(CUICDMActionHandle inAction, const SObjectRefType &inTriggerObject) override;
    void SetTargetObject(CUICDMActionHandle inAction, const SObjectRefType &inTargetObject) override;
    void SetEvent(CUICDMActionHandle inAction, const wstring &inEvent) override;
    void SetHandler(CUICDMActionHandle inAction, const wstring &inHandler) override;

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
    bool HandleValid(int inHandle) const override { return CHandleBase::HandleValid(inHandle); }

    // Helper functions
    CUICDMActionHandle CreateActionWithHandle(int inHandle, CUICDMInstanceHandle inInstance,
                                              CUICDMSlideHandle inSlide,
                                              CUICDMInstanceHandle inOwner);
    CUICDMHandlerArgHandle AddHandlerArgumentWithHandle(int inHandle, CUICDMActionHandle inAction,
                                                        const TCharStr &inName,
                                                        HandlerArgumentType::Value inArgType,
                                                        DataModelDataType::Value inValueType);

    static SAction *GetActionNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SAction *>(
            GetActionNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SAction *GetActionNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SAction *theAction = GetHandleObject<SAction>(inHandle, inObjects);
        if (theAction)
            return theAction;
        throw ActionNotFound(L"");
    }

    static SHandlerArgument *GetHandlerArgumentNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SHandlerArgument *>(
            GetHandlerArgumentNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SHandlerArgument *GetHandlerArgumentNF(int inHandle,
                                                        const THandleObjectMap &inObjects)
    {
        const SHandlerArgument *theItem = GetHandleObject<SHandlerArgument>(inHandle, inObjects);
        if (theItem)
            return theItem;
        throw HandlerArgumentNotFound(L"");
    }
};

typedef std::shared_ptr<CSimpleActionCore> TSimpleActionCorePtr;
}

#endif
