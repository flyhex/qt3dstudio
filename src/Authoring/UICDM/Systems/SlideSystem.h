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
#ifndef SLIDESYSTEMH
#define SLIDESYSTEMH
#include "UICDMDataCore.h"
#include "UICDMSlideCore.h"
#include "UICDMSlideGraphCore.h"
#include "UICDMTransactions.h"
#include "UICDMSlides.h"
#include "UICDMAnimation.h"
#include "StudioPropertySystem.h"
#include "SignalsImpl.h"

/**
 *	The systems aggregate the various cores and provide further information or
 *	integrity checking taking the various cores into account.
 */

namespace qt3dsdm {

typedef std::pair<CUICDMPropertyHandle, TPropertyInstanceInfoPtr> TPropertyHandlePropertyInfoPair;
typedef std::vector<TPropertyHandlePropertyInfoPair> TPropertyHandlePropertyInfoPairList;
/**
 *	Provides more thorough checking and will return an appropriate
 *	slide graph when calling GetSlideGraph
 */
struct SSlideSystem : public ISlideSystem
{
    Q_DISABLE_COPY(SSlideSystem)

    TDataCorePtr m_DataCore; // TODO: We might want to throw this away and use the PropertySystem,
                             // unless we have a clean seperate of IPropertySystem and IDataCore
    TPropertySystemPtr m_PropertySystem;
    TSlideCorePtr m_SlideCore;
    TSlideGraphCorePtr m_SlideGraphCore;
    TAnimationCorePtr m_AnimationCore;
    CUICDMInstanceHandle m_SlideInstance;
    CUICDMPropertyHandle m_ComponentGuid;
    std::shared_ptr<ISignalItem> m_Signaller;
    TPropertyHandlePropertyInfoPairList m_PropertyInfoPairList;
    typedef std::unordered_map<int, int> TIntIntMap;
    TIntIntMap m_SlideSelectedInstances;

    SSlideSystem(TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                 TSlideGraphCorePtr inSlideGraphCore, TAnimationCorePtr inAnimationCore,
                 CUICDMInstanceHandle inSlideInstance,
                 CUICDMPropertyHandle inComponentGuidProperty);

    void SetPropertySystem(TPropertySystemPtr inPropertySystem);

    CUICDMSlideHandle CreateMasterSlide() override;
    CUICDMSlideHandle CreateSlide(CUICDMSlideHandle inMaster, int inIndex = -1) override;
    CUICDMSlideHandle DuplicateSlide(CUICDMSlideHandle inSourceSlide, int inDestIndex = -1) override;
    CUICDMSlideHandle GetMasterSlide(CUICDMSlideHandle inSlide) const override;
    bool IsMasterSlide(CUICDMSlideHandle inSlide) const override;
    CUICDMSlideHandle GetMasterSlideByComponentGuid(SLong4 inGuid) const override;
    // Indexes are 1 based.  Index 0 refers to the master slide; you can't delete this.
    void DeleteSlideByIndex(CUICDMSlideHandle inMaster, size_t inIndex) override;
    void GetSlideReferencedInstances(CUICDMSlideHandle inMaster, size_t inIndex,
                                             TInstanceHandleList &outReferencedInstances) override;
    CUICDMSlideHandle GetSlideByIndex(CUICDMSlideHandle inMaster, size_t inIndex) const override;
    void SetActiveSlide(CUICDMSlideHandle inMaster, size_t inIndex) override;
    size_t GetSlideCount(CUICDMSlideHandle inMaster) const override;
    void RearrangeSlide(CUICDMSlideHandle inMaster, size_t inOldIndex, size_t inNewIndex) override;

    void SetComponentSeconds(CUICDMSlideHandle inSlide, float inSeconds) override;
    float GetComponentSeconds(CUICDMSlideHandle inSlide) const override;
    long GetComponentSecondsLong(CUICDMSlideHandle inSlide) const override;
    // For any given instance, find the current seconds via backtracking to the graph, finding the
    // active
    // slide, and return.
    long GetComponentSecondsLong(CUICDMInstanceHandle inInstance) const override;
    virtual SInstanceSlideInformation
    GetInstanceSlideInformation(CUICDMInstanceHandle inInstance) const override;
    /**
     * Use the instance for storing information such as name, or the GUID of the object
     * this slide links to.
     */
    CUICDMInstanceHandle GetSlideInstance(CUICDMSlideHandle inInstance) const override;
    /**
     *	Reverse lookup into the slide system so you can match slides to instances.
     */
    CUICDMSlideHandle GetSlideByInstance(CUICDMInstanceHandle inInstance) const override;

    /**
     *	Slide may be either a master slide or a normal slide.  This will associate this instance
     *	with this set of slides.  Property lookups (using the above IInstancePropertyCore interface)
     *	will now run through the slide set before hitting the main data core database.
     */
    void AssociateInstanceWithSlide(CUICDMSlideHandle inSlide,
                                            CUICDMInstanceHandle inInstance) override;
    CUICDMSlideHandle GetAssociatedSlide(CUICDMInstanceHandle inInstance) const override;
    void GetAssociatedInstances(
        CUICDMSlideHandle inMaster,
        std::vector<std::pair<CUICDMSlideHandle, CUICDMInstanceHandle>> &outAssociations) const override;
    void GetAssociatedInstances(CUICDMSlideHandle inSlide,
                                        TInstanceHandleList &outAssociations) const override;
    void LinkProperty(CUICDMInstanceHandle inInstance, CUICDMPropertyHandle inProperty) override;
    void UnlinkProperty(CUICDMInstanceHandle inInstance, CUICDMPropertyHandle inProperty) override;
    bool IsPropertyLinked(CUICDMInstanceHandle inInstance,
                                  CUICDMPropertyHandle inProperty) const override;
    bool CanPropertyBeLinked(CUICDMInstanceHandle inInstance,
                                     CUICDMPropertyHandle inProperty) const override;
    bool GetSlidePropertyValue(size_t inSlide, CUICDMInstanceHandle inInstance,
                                       CUICDMPropertyHandle inProperty, SValue &outValue) override;
    void GetUnionOfProperties(CUICDMSlideHandle inSlide1, CUICDMSlideHandle inSlide,
                                      TInstancePropertyPairList &outProperties) const override;

    void SetActiveSlide(CUICDMSlideHandle inSlide) override;
    CUICDMSlideHandle GetAssociatedSlide(CUICDMInstanceHandle inInstance,
                                                 CUICDMPropertyHandle inProperty) const override;

    bool SlideValid(CUICDMSlideHandle inSlide) const override;
    int GetSlideIndex(CUICDMSlideHandle inSlide) const override;
    int GetActiveSlideIndex(CUICDMSlideHandle inMaster) const override;
    CUICDMSlideHandle GetActiveSlide(CUICDMSlideHandle inMaster) const override;
    CUICDMInstanceHandle GetSlideSelectedInstance(CUICDMSlideHandle inSlide) const override;
    void SetSlideSelectedInstance(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inInstance) override;

    CUICDMSlideHandle GetApplicableSlide(CUICDMInstanceHandle inHandle,
                                                 CUICDMPropertyHandle inProperty) override;

    bool GetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                          CUICDMInstanceHandle inInstance,
                                          CUICDMPropertyHandle inProperty, SValue &outValue) const override;
    bool GetCanonicalInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                                   CUICDMPropertyHandle inProperty,
                                                   SValue &outValue) const override;
    void ForceSetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                               CUICDMInstanceHandle inInstance,
                                               CUICDMPropertyHandle inProperty,
                                               const SValue &inValue) override;

    void RegisterPropertyInstance(CUICDMPropertyHandle inPropertyHandle,
                                          TPropertyInstanceInfoPtr inPropertyInfo) override;

    virtual ISlideSystemSignalProvider *GetSignalProvider();

private:
    virtual ISlideSystemSignalSender *GetSignalSender();
    // helper method
    void InsertEntryAndPropertyInstance(const TSlideEntry &inEntry,
                                        TInstanceHandleList &inInstances,
                                        CUICDMSlideHandle inSlide);
    void DeleteReferencedInstances(CUICDMSlideHandle inSlide);
    void GetReferencedInstances(CUICDMSlideHandle inSlide,
                                TInstanceHandleList &outReferencedInstances);
};
}

#endif
