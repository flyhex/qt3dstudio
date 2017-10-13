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
#ifndef ACTIONSYSTEMH
#define ACTIONSYSTEMH
#include "UICDMActionSystem.h"
#include "UICDMActionCore.h"
#include "UICDMDataCore.h"
#include "UICDMSlideCore.h"
#include "UICDMSlideGraphCore.h"
#include "UICDMSlides.h"
#include "UICDMTransactions.h"
#include "SignalsImpl.h"

namespace qt3dsdm {

class CActionSystem : public IActionSystem
{
    TDataCorePtr m_DataCore;
    TSlideCorePtr m_SlideCore;
    TSlideGraphCorePtr m_SlideGraphCore;
    TActionCorePtr m_ActionCore;

    TSlideSystemPtr m_SlideSystem;

    CUICDMInstanceHandle m_ActionInstance;
    CUICDMPropertyHandle m_ActionEyeball;

    std::shared_ptr<ISignalItem> m_Signaller;

public:
    CActionSystem(TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                  TSlideGraphCorePtr inSlideGraphCore, TActionCorePtr inActionCore,
                  TSlideSystemPtr inSlideSystem, CUICDMInstanceHandle inActionInstance,
                  CUICDMPropertyHandle inActionEyeball);

    CUICDMActionHandle CreateAction(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                                            SLong4 inTriggerTargetObjects) override;
    void DeleteAction(CUICDMActionHandle inAction) override;
    void GetActions(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inOwner,
                            TActionHandleList &outActions) const override;

    bool GetActionEyeballValue(CUICDMSlideHandle inActiveSlide,
                                       CUICDMActionHandle inAction) const override;
    void SetActionEyeballValue(CUICDMSlideHandle inActiveSlide, CUICDMActionHandle inAction,
                                       bool inValue) override;

    virtual IActionSystemSignalProvider *GetSignalProvider();

private:
    virtual IActionSystemSignalSender *GetSignalSender();

    CActionSystem(const CActionSystem&) = delete;
    CActionSystem& operator=(const CActionSystem&) = delete;
};
}

#endif
