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
#ifndef ANIMATIONCOREPRODUCERH
#define ANIMATIONCOREPRODUCERH
#include "SimpleAnimationCore.h"
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMSignals.h"

namespace qt3dsdm {
class CAnimationCoreProducer : public IAnimationCore,
                               public ITransactionProducer,
                               public IAnimationCoreSignalProvider
{
    Q_DISABLE_COPY(CAnimationCoreProducer)

    typedef std::shared_ptr<IMergeableTransaction<TKeyframe>> TKeyframeDataMergeMapEntry;
    typedef std::unordered_map<int, TKeyframeDataMergeMapEntry> TKeyframeDataMergeMap;

    TSimpleAnimationCorePtr m_Data;
    TTransactionConsumerPtr m_Consumer;
    TSignalItemPtr m_Signaller;
    TKeyframeDataMergeMap m_KeyframeMergeMap;

public:
    CAnimationCoreProducer()
        : m_Data(new CSimpleAnimationCore())
    {
        InitSignaller();
    }
    CAnimationCoreProducer(TStringTablePtr strTable)
        : m_Data(new CSimpleAnimationCore(strTable))
    {
        InitSignaller();
    }

    TSimpleAnimationCorePtr GetTransactionlessAnimationCore() const { return m_Data; }

    // IAnimationManger implementation

    CUICDMAnimationHandle CreateAnimation(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                          EAnimationType inAnimationType,
                                          bool inFirstKeyframeDynamic) override;
    void DeleteAnimation(CUICDMAnimationHandle inAnimation) override;
    CUICDMAnimationHandle GetAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, size_t inIndex) const override;
    SAnimationInfo GetAnimationInfo(CUICDMAnimationHandle inAnimation) const override;
    void GetAnimations(TAnimationHandleList &outAnimations) const override;
    void GetAnimations(TAnimationInfoList &outAnimations, Qt3DSDMSlideHandle inMaster,
                       Qt3DSDMSlideHandle inSlide) const override;
    void GetSpecificInstanceAnimations(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                       TAnimationHandleList &outAnimations) override
    {
        m_Data->GetSpecificInstanceAnimations(inSlide, inInstance, outAnimations);
    }

    void SetFirstKeyframeDynamic(CUICDMAnimationHandle inAnimation, bool inValue) override;

    CUICDMKeyframeHandle InsertKeyframe(CUICDMAnimationHandle inAnimation,
                                        const TKeyframe &inKeyframe) override;
    void EraseKeyframe(CUICDMKeyframeHandle) override;
    void DeleteAllKeyframes(CUICDMAnimationHandle inAnimation) override;
    CUICDMAnimationHandle GetAnimationForKeyframe(CUICDMKeyframeHandle inKeyframe) const override;
    TKeyframe GetKeyframeData(CUICDMKeyframeHandle inKeyframe) const override;
    void SetKeyframeData(CUICDMKeyframeHandle inKeyframe, const TKeyframe &inData) override;
    void GetKeyframes(CUICDMAnimationHandle inAnimation, TKeyframeHandleList &outKeyframes) const override;
    size_t GetKeyframeCount(CUICDMAnimationHandle inAnimation) const override;
    bool IsFirstKeyframe(CUICDMKeyframeHandle inKeyframe) const override
    {
        return m_Data->IsFirstKeyframe(inKeyframe);
    }
    void OffsetAnimations(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                          long inMillisecondOffset) override;

    void SetIsArtistEdited(CUICDMAnimationHandle inAnimation, bool inEdited = true) override;
    bool IsArtistEdited(CUICDMAnimationHandle inAnimation) const override;

    // Animation Evaluation.
    float EvaluateAnimation(CUICDMAnimationHandle inAnimation, float inSeconds) const override;

    bool KeyframeValid(CUICDMKeyframeHandle inKeyframe) const override;
    bool AnimationValid(CUICDMAnimationHandle inAnimation) const override;

    void CopyAnimations(Qt3DSDMSlideHandle inSourceSlide, Qt3DSDMInstanceHandle inSourceInstance,
                        Qt3DSDMSlideHandle inDestSlide, Qt3DSDMInstanceHandle inDestInstance) override;

    // ITransactionProducer implementation
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    TSignalConnectionPtr ConnectAnimationCreated(
        const std::function<void(CUICDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectBeforeAnimationDeleted(const std::function<void(CUICDMAnimationHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectAnimationDeleted(
        const std::function<void(CUICDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) override;
    TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(CUICDMAnimationHandle, CUICDMKeyframeHandle, const TKeyframe &)>
            &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectBeforeKeyframeErased(const std::function<void(CUICDMKeyframeHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(CUICDMAnimationHandle, CUICDMKeyframeHandle, const TKeyframe &)>
            &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectBeforeAllKeyframesErased(const std::function<void(CUICDMAnimationHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectKeyframeUpdated(
        const std::function<void(CUICDMKeyframeHandle, const TKeyframe &)> &inCallback) override;
    TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(CUICDMAnimationHandle, bool)> &inCallback) override;

private:
    void InitSignaller();
    IAnimationCoreSignalProvider *GetSignalProvider();
    IAnimationCoreSignalSender *GetSignalSender();
};
}

#endif
