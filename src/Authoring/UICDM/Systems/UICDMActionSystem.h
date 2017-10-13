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
#ifndef UICDMACTIONSYSTEMH
#define UICDMACTIONSYSTEMH
#include "UICDMHandles.h"
#include "UICDMActionCore.h"

namespace qt3dsdm {
/**
 *	ActionSystem acts as a wrapper or helper around ActionCore. ActionSystem will call
 *	ActionCore and perform other necessary setups. It also has the knowledge of other
 *	system or core so there are some synchronization being done here.
 *
 *	When in doubts which one to use (ActionSystem or ActionCore), always use ActionSystem
 */
class IActionSystem
{
public:
    virtual ~IActionSystem() {}

    // CreateAction will create Action's InstanceHandle and Action's ActionHandle and do some
    // necessary setup
    virtual CUICDMActionHandle CreateAction(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                                            SLong4 inTriggerTargetObjects) = 0;
    // DeleteAction will delete both Action's ActionHandle and Action's InstanceHandle
    virtual void DeleteAction(CUICDMActionHandle inAction) = 0;
    // Return all actions that belong to a certain instance in a certain slide + the master slide
    virtual void GetActions(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                            TActionHandleList &outActions) const = 0;

    // Get/set action eyeball property value
    virtual bool GetActionEyeballValue(CUICDMSlideHandle inActiveSlide,
                                       CUICDMActionHandle inAction) const = 0;
    virtual void SetActionEyeballValue(CUICDMSlideHandle inActiveSlide, CUICDMActionHandle inAction,
                                       bool inValue) = 0;
};

typedef std::shared_ptr<IActionSystem> TActionSystemPtr;
}

#endif