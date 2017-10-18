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
#ifndef SLIDEGRAPHCOREPRODUCERH
#define SLIDEGRAPHCOREPRODUCERH
#include "SimpleSlideGraphCore.h"
#include "UICDMTransactions.h"
#include "UICDMSignals.h"

namespace qt3dsdm {
class CSlideGraphCoreProducer : public ISlideGraphCore,
                                public ITransactionProducer,
                                public ISlideGraphCoreSignalProvider
{
    Q_DISABLE_COPY(CSlideGraphCoreProducer)

    TSimpleSlideGraphCorePtr m_Data;
    TTransactionConsumerPtr m_Consumer;
    TSignalItemPtr m_Signaller;

public:
    CSlideGraphCoreProducer(TStringTablePtr strTable = TStringTablePtr())
        : m_Data(new CSimpleSlideGraphCore(strTable))
    {
        InitSignaller();
    }
    TSimpleSlideGraphCorePtr GetTransactionlessSlideGraphCore() { return m_Data; }
    TSimpleSlideGraphCorePtr GetTransactionlessSlideGraphCore() const { return m_Data; }

    CUICDMSlideGraphHandle CreateSlideGraph(CUICDMSlideHandle inRoot) override;
    CUICDMSlideHandle GetGraphRoot(CUICDMSlideGraphHandle inGraph) const override;
    CUICDMSlideGraphHandle GetSlideGraph(CUICDMSlideHandle inSlide) const override;
    void GetSlideGraphs(TSlideGraphHandleList &outGraphs) const override;
    void DeleteSlideGraph(CUICDMSlideGraphHandle inHandle) override;

    void AssociateInstance(CUICDMSlideGraphHandle inSlideGraph, CUICDMSlideHandle inSlide,
                           Qt3DSDMInstanceHandle inInstance) override;
    void GetAssociatedInstances(CUICDMSlideGraphHandle inSlideGraph,
                                TSlideInstancePairList &outAssociations) const override;
    TGraphSlidePair GetAssociatedGraph(Qt3DSDMInstanceHandle inInstance) const override;
    void DissociateInstance(Qt3DSDMInstanceHandle inInstance) override;

    void SetGraphActiveSlide(CUICDMSlideGraphHandle inGraph, CUICDMSlideHandle inSlide) override;
    CUICDMSlideHandle GetGraphActiveSlide(CUICDMSlideGraphHandle inGraph) const override;

    bool HandleValid(int inHandle) const override;
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    //===================================================================
    // Signals
    //===================================================================
    TSignalConnectionPtr ConnectGraphCreated(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectGraphDeleted(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override;
    TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override;
    TSignalConnectionPtr ConnectGraphActiveSlide(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle)> &inCallback) override;

private:
    void InitSignaller();
    ISlideGraphCoreSignalProvider *GetSignalProvider();
    ISlideGraphCoreSignalSender *GetSignalSender();
};
}

#endif
