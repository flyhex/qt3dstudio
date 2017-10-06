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
#ifndef INSTANCEPROPERTYCORESYSTEMH
#define INSTANCEPROPERTYCORESYSTEMH

#include "UICDMSlides.h"
#include "UICDMAnimation.h"
#include "UICDMSignals.h"
#include "UICDMDataCore.h"

namespace UICDM {

typedef std::tuple<CUICDMInstanceHandle, CUICDMPropertyHandle, SValue> TTemporaryPropertyValue;
typedef std::vector<TTemporaryPropertyValue> TTemporaryPropertyValueList;

/**
 *	Get/set instance properties taking the slide and slide graph, and animation systems into
 *account.
 *	Also takes care of notifying external entities when a property changes.
 */
class CStudioPropertySystem : public IPropertySystem
{
    std::shared_ptr<IMetaData> m_MetaData;
    TDataCorePtr m_DataCore;

    TSlideSystemPtr m_SlideSystem;
    TStudioAnimationSystemPtr m_StudioAnimationSystem;

    TSignalItemPtr m_PropertyCoreSignaller;
    TSignalItemPtr m_ImmediateModePropertyCoreSignaller;

public:
    CStudioPropertySystem(std::shared_ptr<IMetaData> inMetaData, TDataCorePtr inDataCore,
                          TSlideSystemPtr inSlideSystem,
                          TStudioAnimationSystemPtr inStudioAnimationSystem);

    IInstancePropertyCoreSignalProvider *GetPropertyCoreSignalProvider()
    {
        return dynamic_cast<IInstancePropertyCoreSignalProvider *>(m_PropertyCoreSignaller.get());
    }
    // The immediate signaller is used before changes have been committed for very live feedback.
    IInstancePropertyCoreSignalProvider *GetImmediatePropertyCoreSignalProvider()
    {
        return dynamic_cast<IInstancePropertyCoreSignalProvider *>(
            m_ImmediateModePropertyCoreSignaller.get());
    }

    IInstancePropertyCoreSignalSender *GetPropertyCoreSignalSender()
    {
        return dynamic_cast<IInstancePropertyCoreSignalSender *>(m_PropertyCoreSignaller.get());
    }

    DataModelDataType::Value GetDataType(CUICDMPropertyHandle inProperty) const override;
    TCharStr GetName(CUICDMPropertyHandle inProperty) const override;
    TCharStr GetFormalName(CUICDMInstanceHandle inInstance,
                                   CUICDMPropertyHandle inProperty) const override;
    virtual AdditionalMetaDataType::Value
    GetAdditionalMetaDataType(CUICDMInstanceHandle inInstance,
                              CUICDMPropertyHandle inProperty) const override;
    TMetaDataData GetAdditionalMetaDataData(CUICDMInstanceHandle inInstance,
                                                    CUICDMPropertyHandle inProperty) const override;
    CUICDMInstanceHandle GetPropertyOwner(CUICDMPropertyHandle inProperty) const override;

    CUICDMPropertyHandle GetAggregateInstancePropertyByName(CUICDMInstanceHandle inInstance,
                                                                    const TCharStr &inStr) const override;
    void GetAggregateInstanceProperties(CUICDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const override;
    bool HasAggregateInstanceProperty(CUICDMInstanceHandle inInstance,
                                              CUICDMPropertyHandle inProperty) const override;

    bool GetInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                          CUICDMPropertyHandle inProperty, SValue &outValue) const override;
    void SetInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                          CUICDMPropertyHandle inProperty, const SValue &inValue) override;

    CUICDMInstanceHandle CreateInstance() override;
    void GetInstances(TInstanceHandleList &outInstances) const override;
    void DeleteInstance(CUICDMInstanceHandle inHandle) override;

    void DeriveInstance(CUICDMInstanceHandle inInstance, CUICDMInstanceHandle inParent) override;
    bool IsInstanceOrDerivedFrom(CUICDMInstanceHandle inInstance,
                                         CUICDMInstanceHandle inParent) const override;

    CUICDMPropertyHandle AddProperty(CUICDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) override;
    bool HandleValid(int inHandle) const override;

    // Get the instance property value from the slide that owns the instance or the data core if the
    // slide doesn't have the value
    bool GetCanonicalInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                           CUICDMPropertyHandle inProperty, SValue &outValue) const;

private:
    static bool DerivedGuidMatches(UICDM::IDataCore &inDataCore,
                                   UICDM::CUICDMInstanceHandle inInstance,
                                   UICDM::CUICDMPropertyHandle inProperty, UICDM::SLong4 inGuid);
    CStudioPropertySystem(const CStudioPropertySystem&) = delete;
    CStudioPropertySystem& operator=(const CStudioPropertySystem&) = delete;
};
}

#endif
