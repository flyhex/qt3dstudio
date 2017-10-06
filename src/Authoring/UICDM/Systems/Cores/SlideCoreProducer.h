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
#ifndef SLIDECOREPRODUCERH
#define SLIDECOREPRODUCERH
#include "UICDMTransactions.h"
#include "SimpleSlideCore.h"
#include "UICDMSignals.h"

namespace UICDM {
struct SlideInstancePropertyKey
{
    int m_Slide;
    int m_Instance;
    int m_Property;
    SlideInstancePropertyKey(int slide, int inst, int prop)
        : m_Slide(slide)
        , m_Instance(inst)
        , m_Property(prop)
    {
    }
    SlideInstancePropertyKey()
        : m_Slide(0)
        , m_Instance(0)
        , m_Property(0)
    {
    }

    bool operator==(const SlideInstancePropertyKey &inOther) const
    {
        return m_Slide == inOther.m_Slide && m_Instance == inOther.m_Instance
            && m_Property == inOther.m_Property;
    }
};

struct SlideInstancePropertyKeyHasher
{
    std::size_t operator()(const SlideInstancePropertyKey &inEntry) const
    {
        return std::hash<int>()(inEntry.m_Slide) ^ std::hash<int>()(inEntry.m_Instance)
            ^ std::hash<int>()(inEntry.m_Property);
    }
};

typedef std::shared_ptr<IMergeableTransaction<SInternValue>> TSlidePropertyMergeMapEntry;
typedef std::unordered_map<SlideInstancePropertyKey, TSlidePropertyMergeMapEntry,
                             SlideInstancePropertyKeyHasher>
    TSlidePropertyMergeMap;

class CSlideCoreProducer : public ISlideCore,
                           public ITransactionProducer,
                           public ISlideCoreSignalProvider
{
    Q_DISABLE_COPY(CSlideCoreProducer)

    TTransactionConsumerPtr m_Consumer;
    TSimpleSlideCorePtr m_Data;
    TSignalItemPtr m_SlideCoreSignaller;
    TSlidePropertyMergeMap m_PropertyMergeMap;

public:
    CSlideCoreProducer(TStringTablePtr inStrTable)
        : m_Data(new CSimpleSlideCore(inStrTable))
    {
        InitSignaller();
    }

    IStringTable &GetStringTable() const override { return m_Data->GetStringTable(); }
    TStringTablePtr GetStringTablePtr() const override { return m_Data->GetStringTablePtr(); }

    TSimpleSlideCorePtr GetTransactionlessSlideCore() { return m_Data; }
    TSimpleSlideCorePtr GetTransactionlessSlideCore() const { return m_Data; }

    CUICDMSlideHandle CreateSlide(CUICDMInstanceHandle inInstance) override;
    CUICDMInstanceHandle GetSlideInstance(CUICDMSlideHandle inSlide) const override;
    CUICDMSlideHandle GetSlideByInstance(CUICDMInstanceHandle inSlide) const override;
    void DeleteSlide(CUICDMSlideHandle inSlide, TInstanceHandleList &outInstances) override;
    void GetSlides(TSlideHandleList &outSlides) const override;

    float GetSlideTime(CUICDMSlideHandle inSlide) const override;
    void SetSlideTime(CUICDMSlideHandle inSlide, float inNewTime) override;

    void DeriveSlide(CUICDMSlideHandle inSlide, CUICDMSlideHandle inParent, int inIndex = -1) override;
    CUICDMSlideHandle GetParentSlide(CUICDMSlideHandle inSlide) const override;
    void GetChildSlides(CUICDMSlideHandle inSlide, TSlideHandleList &outChildren) const override;
    int GetChildIndex(CUICDMSlideHandle inParent, CUICDMSlideHandle inChild) const override;

    bool GetInstancePropertyValue(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inHandle,
                                  CUICDMPropertyHandle inProperty, SValue &outValue) const override;
    void SetInstancePropertyValue(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inHandle,
                                  CUICDMPropertyHandle inProperty, const SValue &inValue) override;
    void ForceSetInstancePropertyValue(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inHandle,
                                       CUICDMPropertyHandle inProperty, const SValue &inValue) override;
    bool GetSpecificInstancePropertyValue(CUICDMSlideHandle inSlide,
                                          CUICDMInstanceHandle inInstance,
                                          CUICDMPropertyHandle inProperty, SValue &outValue) const override;
    void GetSpecificInstancePropertyValues(CUICDMSlideHandle inSlide,
                                           CUICDMInstanceHandle inInstance,
                                           TPropertyHandleValuePairList &outValues) override
    {
        return m_Data->GetSpecificInstancePropertyValues(inSlide, inInstance, outValues);
    }
    bool ContainsProperty(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inHandle,
                          CUICDMPropertyHandle inProperty) const override;
    void GetSlidePropertyEntries(CUICDMSlideHandle inSlide, TSlideEntryList &outEntries) const override;

    void PushPropertyValueToChildren(CUICDMSlideHandle inParent, CUICDMInstanceHandle inHandle,
                                     CUICDMPropertyHandle inProperty, const SValue &inValue) override;
    void ClearChildrenPropertyValues(CUICDMSlideHandle inParent, CUICDMInstanceHandle inHandle,
                                     CUICDMPropertyHandle inProperty) override;
    void DeleteAllInstanceEntries(CUICDMInstanceHandle inHandle) override;
    void DeleteAllPropertyEntries(CUICDMPropertyHandle inHandle) override;
    void DeleteAllInstancePropertyEntries(const TInstanceHandleList &inInstances,
                                          const TPropertyHandleList &inProperties) override;

    void GetIntersectingProperties(CUICDMSlideHandle inSlide1, CUICDMSlideHandle inSlide2,
                                   TSlideEntryList &outEntries) const override;
    void PushIntersectingProperties(CUICDMSlideHandle inSlide1, CUICDMSlideHandle inSlide2,
                                    CUICDMSlideHandle inDestination) override;
    void CopyProperties(CUICDMSlideHandle inSourceSlide, CUICDMInstanceHandle inSourceInstance,
                        CUICDMSlideHandle inDestSlide, CUICDMInstanceHandle inDestInstance) override;

    bool IsSlide(CUICDMSlideHandle inSlide) const override;

    bool HandleValid(int inHandle) const override;

    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    //===================================================================
    // Signals
    //===================================================================

    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(CUICDMSlideHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectBeforeSlideDeleted(const std::function<void(CUICDMSlideHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(CUICDMSlideHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectSlideDerived(
        const std::function<void(CUICDMSlideHandle, CUICDMSlideHandle, int)> &inCallback) override;
    TSignalConnectionPtr ConnectInstancePropertyValueSet(
        const std::function<void(CUICDMSlideHandle, CUICDMInstanceHandle, CUICDMPropertyHandle,
                                   const SValue &)> &inCallback) override;
    TSignalConnectionPtr ConnectInstancePropertyValueRemoved(
        const std::function<void(CUICDMSlideHandle, CUICDMInstanceHandle, CUICDMPropertyHandle,
                                   const SValue &)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectSlideTimeChanged(const std::function<void(CUICDMSlideHandle)> &inCallback) override;

private:
    inline void DoForceSetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                                CUICDMInstanceHandle inHandle,
                                                CUICDMPropertyHandle inProperty,
                                                const SValue &inValue);
    void InitSignaller();
    ISlideCoreSignalProvider *GetSignalProvider();
    ISlideCoreSignalSender *GetSignalSender();
};
}

#endif
