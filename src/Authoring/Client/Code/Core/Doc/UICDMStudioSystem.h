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
#ifndef UICDMSTUDIOSYSTEMH
#define UICDMSTUDIOSYSTEMH
#include "UICDMTransactions.h"
#include "UICDMHandles.h"

class CDoc;
class CClientDataModelBridge;

namespace qt3dsdm {
class IPropertySystem;
class ISlideCore;
class ISlideGraphCore;
class ISlideSystem;
class IInstancePropertyCore;
class ITransactionConsumer;
class IAnimationCore;
class IActionCore;
class CStudioFullSystem;
class IStudioFullSystemSignalProvider;
class IStudioAnimationSystem;
class IActionSystem;
class ISignalConnection;
class IMetaData;

class CStudioSystem : public ITransactionProducer
{
    CDoc *m_Doc;
    std::shared_ptr<CClientDataModelBridge> m_Bridge;
    std::shared_ptr<CStudioFullSystem> m_StudioSystem;
    // Properties that refer to instance may require special handling.

public:
    CStudioSystem(CDoc *inDoc);
    // Call before load.
    void ResetDatabase();

    IMetaData *GetActionMetaData();
    CClientDataModelBridge *GetClientDataModelBridge() const { return m_Bridge.get(); }
    ISlideSystem *GetSlideSystem();
    ISlideCore *GetSlideCore();
    IPropertySystem *GetPropertySystem();
    IStudioFullSystemSignalProvider *GetFullSystemSignalProvider();
    IAnimationCore *GetAnimationCore();
    IStudioAnimationSystem *GetAnimationSystem();
    IActionCore *GetActionCore();
    IActionSystem *GetActionSystem();

    CStudioFullSystem *GetFullSystem() { return m_StudioSystem.get(); }

    void SetConsumer(TTransactionConsumerPtr inConsumer) override;
    bool IsInstance(Qt3DSDMInstanceHandle inInstance) const;
};
};

#endif
