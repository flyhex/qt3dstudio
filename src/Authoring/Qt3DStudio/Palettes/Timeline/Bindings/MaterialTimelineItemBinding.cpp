/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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

#include "Qt3DSCommonPrecompile.h"
#include "MaterialTimelineItemBinding.h"
#include "TimelineTranslationManager.h"
#include "ImageTimelineItemBinding.h"
#include "EmptyTimelineTimebar.h"

// Data model specific
#include "IDoc.h"
#include "ClientDataModelBridge.h"
#include "DropSource.h"

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMStudioSystem.h"

#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMDataCore.h"
#include "StudioFullSystem.h"
#include "StudioCoreSystem.h"

using namespace qt3dsdm;

CMaterialTimelineItemBinding::CMaterialTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                                           Qt3DSDMInstanceHandle inDataHandle)
    : Qt3DSDMTimelineItemBinding(inMgr, inDataHandle)
{
    qt3dsdm::IPropertySystem *thePropertySystem = m_TransMgr->GetStudioSystem()->GetPropertySystem();
    TPropertyHandleList theProperties;
    thePropertySystem->GetAggregateInstanceProperties(inDataHandle, theProperties);

    size_t thePropertyCount = theProperties.size();
    for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount; ++thePropertyIndex) {
        Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];

        AdditionalMetaDataType::Value theAdditionalMetaDataType =
            thePropertySystem->GetAdditionalMetaDataType(inDataHandle, theProperty);

        if (theAdditionalMetaDataType == AdditionalMetaDataType::Image) {
            TCharStr theName(thePropertySystem->GetName(theProperty));
            TCharStr theFormalName(thePropertySystem->GetFormalName(inDataHandle, theProperty));
            TNameFormalNamePair thePair = std::make_tuple(theName, theFormalName);
            m_ImageNameFormalNamePairs.push_back(thePair);
        }
    }
}

CMaterialTimelineItemBinding::~CMaterialTimelineItemBinding()
{
}

ITimelineTimebar *CMaterialTimelineItemBinding::GetTimebar()
{ // No timebars on materials
    return new CEmptyTimelineTimebar();
}

bool CMaterialTimelineItemBinding::ShowToggleControls() const
{
    // Materials have no toggle controls, by design
    return false;
}

bool ImageSlotIsFilled(qt3dsdm::IPropertySystem *inPropertySystem, Qt3DSDMInstanceHandle inInstance,
                       const TCharStr &inStr)
{
    Qt3DSDMPropertyHandle theProperty =
        inPropertySystem->GetAggregateInstancePropertyByName(inInstance, inStr);
    SValue theValue;
    inPropertySystem->GetInstancePropertyValue(inInstance, theProperty, theValue);

    // Prevent assertion down the path when changing from edited standard material to reference material
    if (qt3dsdm::GetValueType(theValue) == DataModelDataType::None)
        return false;

    SLong4 theLong4 = qt3dsdm::get<SLong4>(theValue);
    bool theReturn = theLong4.m_Longs[0] != 0 || theLong4.m_Longs[1] != 0
        || theLong4.m_Longs[2] != 0 || theLong4.m_Longs[3] != 0;

    return theReturn;
}

long CMaterialTimelineItemBinding::GetChildrenCount()
{
    long theReturnCount = 0;
    if (m_TransMgr->GetStudioSystem()->IsInstance(m_DataHandle)) {
        qt3dsdm::IPropertySystem *thePropertySystem =
            m_TransMgr->GetStudioSystem()->GetPropertySystem();
        size_t theSlotCount = m_ImageNameFormalNamePairs.size();
        for (size_t theSlotIndex = 0; theSlotIndex < theSlotCount; ++theSlotIndex) {
            if (ImageSlotIsFilled(thePropertySystem, m_DataHandle,
                                  std::get<0>(m_ImageNameFormalNamePairs[theSlotIndex]))) {
                ++theReturnCount;
            }
        }
    }

    return theReturnCount;
}

ITimelineItemBinding *CMaterialTimelineItemBinding::GetChild(long inIndex)
{
    qt3dsdm::IPropertySystem *thePropertySystem = m_TransMgr->GetStudioSystem()->GetPropertySystem();

    size_t theSlotCursor = 0;
    size_t theSlotCount = m_ImageNameFormalNamePairs.size();
    for (size_t theSlotIndex = 0; theSlotIndex < theSlotCount; ++theSlotIndex) {
        if (ImageSlotIsFilled(thePropertySystem, m_DataHandle,
                              std::get<0>(m_ImageNameFormalNamePairs[theSlotIndex]))) {
            inIndex--;

            if (inIndex < 0) {
                theSlotCursor = theSlotIndex;
                break;
            }
        }
    }
    Qt3DSDMPropertyHandle theImageProperty = thePropertySystem->GetAggregateInstancePropertyByName(
        m_DataHandle, std::get<0>(m_ImageNameFormalNamePairs[theSlotCursor]));
    return GetOrCreateImageBinding(
        theImageProperty, std::get<1>(m_ImageNameFormalNamePairs[theSlotCursor]).wide_str());
}

QList<ITimelineItemBinding *> CMaterialTimelineItemBinding::GetChildren()
{
    int childCount = GetChildrenCount();
    QList<ITimelineItemBinding *> retlist;
    retlist.reserve(childCount);
    for (int i = 0; i < childCount; ++i)
        retlist.append(GetChild(i));

    return retlist;
}

void CMaterialTimelineItemBinding::OnAddChild(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    using namespace qt3dsdm;
    CClientDataModelBridge *theBridge = m_TransMgr->GetStudioSystem()->GetClientDataModelBridge();
    // This is handled via the OnPropertyChanged call below
    if (theBridge->IsImageInstance(inInstance))
        return;
    else
        Qt3DSDMTimelineItemBinding::OnAddChild(inInstance);
}

void CMaterialTimelineItemBinding::OnPropertyChanged(Qt3DSDMPropertyHandle inPropertyHandle)
{
    Qt3DSDMTimelineItemBinding::OnPropertyChanged(inPropertyHandle);
}

void CMaterialTimelineItemBinding::OnPropertyLinked(Qt3DSDMPropertyHandle inPropertyHandle)
{
    Qt3DSDMTimelineItemBinding::OnPropertyLinked(inPropertyHandle);
}

qt3dsdm::Qt3DSDMInstanceHandle
CMaterialTimelineItemBinding::GetImage(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle)
{
    qt3dsdm::IPropertySystem *thePropertySystem = m_TransMgr->GetStudioSystem()->GetPropertySystem();
    SValue theImageValue;
    thePropertySystem->GetInstancePropertyValue(m_DataHandle, inPropertyHandle, theImageValue);
    SLong4 theImageLong4 = qt3dsdm::get<SLong4>(theImageValue);
    return m_TransMgr->GetStudioSystem()->GetClientDataModelBridge()->GetImageInstanceByGUID(
        theImageLong4);
}

ITimelineItemBinding *
CMaterialTimelineItemBinding::GetOrCreateImageBinding(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
                                                      const wchar_t *inName)
{
    qt3dsdm::Qt3DSDMInstanceHandle theImageInstance = GetImage(inPropertyHandle);
    ITimelineItemBinding *theImageTimelineRow = m_TransMgr->GetBinding(theImageInstance);
    if (!theImageTimelineRow) // create
    {
        theImageTimelineRow = m_TransMgr->GetOrCreate(theImageInstance);
        // Set the name, by spec: the nice name.
        theImageTimelineRow->GetTimelineItem()->SetName(inName);
        CImageTimelineItemBinding *theImageBinding =
            dynamic_cast<CImageTimelineItemBinding *>(theImageTimelineRow);
        if (theImageBinding)
            theImageBinding->SetPropertyHandle(inPropertyHandle);
    }
    return theImageTimelineRow;
}
