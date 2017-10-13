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
#ifndef UICDMACTIONCOREH
#define UICDMACTIONCOREH

#include "UICDMHandles.h"
#include "HandleSystemBase.h"
#include "UICDMActionInfo.h"

namespace qt3dsdm {
const long REFERENCED_AS_OWNER = 0x01;
const long REFERENCED_AS_TRIGGER = 0x02;
const long REFERENCED_AS_TARGET = 0x04;
class IStringTable;

/**
 *	ActionCore
 */
class IActionCore : public IHandleBase
{
public:
    virtual ~IActionCore() {}

    virtual TStringTablePtr GetStringTablePtr() const = 0;
    virtual IStringTable &GetStringTable() const = 0;
    // Action
    virtual CUICDMActionHandle CreateAction(CUICDMInstanceHandle inInstance,
                                            CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                                            SLong4 inTriggerTargetObjects) = 0;
    virtual void DeleteAction(CUICDMActionHandle inAction, CUICDMInstanceHandle &outInstance) = 0;
    virtual const SActionInfo &GetActionInfo(CUICDMActionHandle inAction) const = 0;
    virtual void GetActions(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                            TActionHandleList &outActions) const = 0;
    virtual void GetActions(CUICDMSlideHandle inSlide, TActionHandleList &outActions) const = 0;
    virtual void GetActions(CUICDMInstanceHandle inOwner, TActionHandleList &outActions) const = 0;
    virtual void GetActions(TActionHandleList &outActions) const = 0;

    // Return the instance that was allocated for this action.
    virtual CUICDMInstanceHandle GetActionInstance(CUICDMActionHandle inAction) const = 0;
    // Reverse lookup into the action system so you can match actions to instances.
    virtual CUICDMActionHandle GetActionByInstance(CUICDMInstanceHandle inActionInstance) const = 0;

    // Action Properties
    virtual void SetTriggerObject(CUICDMActionHandle inAction,
                                  const SObjectRefType &inTriggerObject) = 0;
    virtual void SetTargetObject(CUICDMActionHandle inAction,
                                 const SObjectRefType &inTargetObject) = 0;
    virtual void SetEvent(CUICDMActionHandle inAction, const wstring &inEventName) = 0;
    virtual void SetHandler(CUICDMActionHandle inAction, const wstring &inHandlerName) = 0;

    // Handler Argument
    virtual CUICDMHandlerArgHandle AddHandlerArgument(CUICDMActionHandle inAction,
                                                      const TCharStr &inName,
                                                      HandlerArgumentType::Value inArgType,
                                                      DataModelDataType::Value inValueType) = 0;
    virtual void RemoveHandlerArgument(CUICDMHandlerArgHandle inHandlerArgument) = 0;
    virtual const SHandlerArgumentInfo &
    GetHandlerArgumentInfo(CUICDMHandlerArgHandle inHandlerArgument) const = 0;
    virtual void GetHandlerArguments(CUICDMActionHandle inAction,
                                     THandlerArgHandleList &outHandlerArguments) const = 0;

    // Handler Argument Properties
    virtual void GetHandlerArgumentValue(CUICDMHandlerArgHandle inHandlerArgument,
                                         SValue &outValue) const = 0;
    virtual void SetHandlerArgumentValue(CUICDMHandlerArgHandle inHandlerArgument,
                                         const SValue &inValue) = 0;
};

typedef std::shared_ptr<IActionCore> TActionCorePtr;
}

#endif
