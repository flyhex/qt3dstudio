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

#include "UICDMPrefix.h"
#include "StudioPropertySystem.h"
#include "StudioAnimationSystem.h"
#include "SignalsImpl.h"

using namespace std;

namespace UICDM {

CStudioPropertySystem::CStudioPropertySystem(std::shared_ptr<IMetaData> inMetaData,
                                             TDataCorePtr inDataCore, TSlideSystemPtr inSlideSystem,
                                             TStudioAnimationSystemPtr inStudioAnimationSystem)
    : m_MetaData(inMetaData)
    , m_DataCore(inDataCore)
    , m_SlideSystem(inSlideSystem)
    , m_StudioAnimationSystem(inStudioAnimationSystem)
{
    m_PropertyCoreSignaller = CreatePropertyCoreSignaller();
    m_ImmediateModePropertyCoreSignaller = CreatePropertyCoreSignaller();
}

DataModelDataType::Value CStudioPropertySystem::GetDataType(CUICDMPropertyHandle inProperty) const
{
    if (m_DataCore->IsProperty(inProperty))
        return m_DataCore->GetProperty(inProperty).m_Type;
    return DataModelDataType::None;
}

TCharStr CStudioPropertySystem::GetName(CUICDMPropertyHandle inProperty) const
{
    if (m_DataCore->IsProperty(inProperty))
        return m_DataCore->GetProperty(inProperty).m_Name;
    return TCharStr();
}

TCharStr CStudioPropertySystem::GetFormalName(CUICDMInstanceHandle inInstance,
                                              CUICDMPropertyHandle inProperty) const
{
    if (inInstance.Valid() && inProperty.Valid())
        return m_MetaData->GetFormalName(inInstance, inProperty);
    return TCharStr();
}

AdditionalMetaDataType::Value
CStudioPropertySystem::GetAdditionalMetaDataType(CUICDMInstanceHandle inInstance,
                                                 CUICDMPropertyHandle inProperty) const
{
    return m_MetaData->GetAdditionalMetaDataType(inInstance, inProperty);
}

TMetaDataData
CStudioPropertySystem::GetAdditionalMetaDataData(CUICDMInstanceHandle inInstance,
                                                 CUICDMPropertyHandle inProperty) const
{
    return m_MetaData->GetAdditionalMetaDataData(inInstance, inProperty);
}

CUICDMInstanceHandle CStudioPropertySystem::GetPropertyOwner(CUICDMPropertyHandle inProperty) const
{

    UICDM::SUICDMPropertyDefinition thePropDef = m_DataCore->GetProperty(inProperty);
    return thePropDef.m_Instance;
}

CUICDMPropertyHandle
CStudioPropertySystem::GetAggregateInstancePropertyByName(CUICDMInstanceHandle inInstance,
                                                          const TCharStr &inStr) const
{
    return m_DataCore->GetAggregateInstancePropertyByName(inInstance, inStr);
}

void CStudioPropertySystem::GetAggregateInstanceProperties(CUICDMInstanceHandle inInstance,
                                                           TPropertyHandleList &outProperties) const
{
    m_DataCore->GetAggregateInstanceProperties(inInstance, outProperties);
}

bool CStudioPropertySystem::HasAggregateInstanceProperty(CUICDMInstanceHandle inInstance,
                                                         CUICDMPropertyHandle inProperty) const
{
    return m_DataCore->HasAggregateInstanceProperty(inInstance, inProperty);
}

bool ApplyValueAndReturnTrue(const SValue &inValue, SValue &outValue)
{
    outValue = inValue;
    return true;
}

bool CStudioPropertySystem::GetInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                                     CUICDMPropertyHandle inProperty,
                                                     SValue &outValue) const
{
    if (!m_DataCore->IsInstance(inInstance))
        return false;
    if (!m_DataCore->IsProperty(inProperty))
        return false;

    CUICDMSlideHandle theAnimationSlide;
    theAnimationSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
    SValue theTemp;
    bool retval = SetDefault(GetDataType(inProperty), theTemp);
    if (retval && theAnimationSlide.Valid()
        && m_StudioAnimationSystem->GetAnimatedInstancePropertyValue(theAnimationSlide, inInstance,
                                                                     inProperty, theTemp))
        return ApplyValueAndReturnTrue(theTemp, outValue);
    if (theAnimationSlide.Valid()
        && m_SlideSystem->GetInstancePropertyValue(theAnimationSlide, inInstance, inProperty,
                                                   theTemp))
        return ApplyValueAndReturnTrue(theTemp, outValue);
    return m_DataCore->GetInstancePropertyValue(inInstance, inProperty, outValue);
}

void CStudioPropertySystem::SetInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                                     CUICDMPropertyHandle inProperty,
                                                     const SValue &inValue)
{
    m_DataCore->CheckValue(inInstance, inProperty, inValue);

    CUICDMSlideHandle theApplicableSlide;
    theApplicableSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
    if (theApplicableSlide.Valid()) {
        if (!m_StudioAnimationSystem->SetAnimatedInstancePropertyValue(
                theApplicableSlide, inInstance, inProperty, inValue.toOldSkool())) {
            // Force set on slide where item exists as it doesn't exist on the root.
            m_SlideSystem->ForceSetInstancePropertyValue(theApplicableSlide, inInstance, inProperty,
                                                         inValue.toOldSkool());
        }
    } else {
        m_DataCore->SetInstancePropertyValue(inInstance, inProperty, inValue);
    }
}

CUICDMInstanceHandle CStudioPropertySystem::CreateInstance()
{
    return m_DataCore->CreateInstance();
}

void CStudioPropertySystem::GetInstances(TInstanceHandleList &outInstances) const
{
    m_DataCore->GetInstances(outInstances);
}

void CStudioPropertySystem::DeleteInstance(CUICDMInstanceHandle inHandle)
{
    m_DataCore->DeleteInstance(inHandle);
}

void CStudioPropertySystem::DeriveInstance(CUICDMInstanceHandle inInstance,
                                           CUICDMInstanceHandle inParent)
{
    m_DataCore->DeriveInstance(inInstance, inParent);
}

bool CStudioPropertySystem::IsInstanceOrDerivedFrom(CUICDMInstanceHandle inInstance,
                                                    CUICDMInstanceHandle inParent) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, inParent);
}

CUICDMPropertyHandle CStudioPropertySystem::AddProperty(CUICDMInstanceHandle inInstance,
                                                        TCharPtr inName,
                                                        DataModelDataType::Value inPropType)
{
    return m_DataCore->AddProperty(inInstance, inName, inPropType);
}

bool CStudioPropertySystem::HandleValid(int inHandle) const
{
    return m_DataCore->HandleValid(inHandle);
}

bool CStudioPropertySystem::GetCanonicalInstancePropertyValue(CUICDMInstanceHandle inInstance,
                                                              CUICDMPropertyHandle inProperty,
                                                              SValue &outValue) const
{
    SValue theTempValue;
    if (m_SlideSystem->GetCanonicalInstancePropertyValue(inInstance, inProperty, theTempValue)) {
        outValue = SValue(theTempValue);
        return true;
    }
    return m_DataCore->GetInstancePropertyValue(inInstance, inProperty, outValue);
}
}
