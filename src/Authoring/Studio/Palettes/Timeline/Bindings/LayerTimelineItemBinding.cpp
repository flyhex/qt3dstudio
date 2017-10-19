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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "LayerTimelineItemBinding.h"
#include "TimelineTranslationManager.h"
#include "BaseStateRow.h"
#include "ImageTimelineItemBinding.h"
#include "EmptyTimelineTimebar.h"

// Data model specific
#include "IDoc.h"
#include "ClientDataModelBridge.h"
#include "DropSource.h"
#include "Doc.h"

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMStudioSystem.h"

#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMDataCore.h"
#include "StudioFullSystem.h"
#include "StudioCoreSystem.h"
#include "Qt3DSDMSlides.h"

using namespace qt3dsdm;

namespace {

bool ImageSlotIsFilled(qt3dsdm::IPropertySystem *inPropertySystem, Qt3DSDMInstanceHandle inInstance,
                       const TCharStr &inStr)
{
    Qt3DSDMPropertyHandle theProperty =
        inPropertySystem->GetAggregateInstancePropertyByName(inInstance, inStr);
    SValue theValue;
    inPropertySystem->GetInstancePropertyValue(inInstance, theProperty, theValue);

    SLong4 theLong4 = qt3dsdm::get<SLong4>(theValue);
    bool theReturn = theLong4.m_Longs[0] != 0 || theLong4.m_Longs[1] != 0
        || theLong4.m_Longs[2] != 0 || theLong4.m_Longs[3] != 0;

    return theReturn;
}

// helper function to find the image binding class that 'represents' this property
inline CImageTimelineItemBinding *FindImageBindingByProperty(CBaseStateRow *inRow,
                                                             Qt3DSDMPropertyHandle inProperty)
{
    if (!inRow || !inProperty.Valid())
        return nullptr;

    CImageTimelineItemBinding *theInvalidImageBinding = nullptr;
    for (long theIndex = 0; theIndex < inRow->GetNumNonPropertyRows(); ++theIndex) {
        CImageTimelineItemBinding *theImageBinding = dynamic_cast<CImageTimelineItemBinding *>(
            inRow->GetNonPropertyRow(theIndex)->GetTimelineItemBinding());
        if (theImageBinding && theImageBinding->GetPropertyHandle() == inProperty) {
            theInvalidImageBinding = theImageBinding;
            break;
        }
    }
    return theInvalidImageBinding;
}
}

CLayerTimelineItemBinding::CLayerTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                                     qt3dsdm::Qt3DSDMInstanceHandle inDataHandle)
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
            TNameFormalNamePair thePair =
                std::make_tuple(theName, theFormalName, theProperty);
            m_ImageNameFormalNamePairs.push_back(thePair);
        }
    }
}

CLayerTimelineItemBinding::~CLayerTimelineItemBinding()
{
}

EStudioObjectType CLayerTimelineItemBinding::GetObjectType() const
{
    return OBJTYPE_LAYER;
}

ITimelineItemBinding *CLayerTimelineItemBinding::GetChild(long inIndex)
{
    static const TCharStr theLayerPrefix(L"Layer_");
    qt3dsdm::IPropertySystem *thePropertySystem = m_TransMgr->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        Q3DStudio::CGraphIterator theChildren;
        Qt3DSDMSlideHandle theActiveSlide = m_TransMgr->GetDoc()->GetActiveSlide();
        GetAssetChildrenInTimeParent(theInstance, m_TransMgr->GetDoc(), AmITimeParent(),
                                     theChildren, theActiveSlide);
        theChildren += inIndex;

        qt3dsdm::Qt3DSDMInstanceHandle theChildInstance = theChildren.GetCurrent();
        if (theChildInstance.Valid()) {
            std::shared_ptr<IDataCore> theDataCore =
                m_TransMgr->GetStudioSystem()->GetFullSystem()->GetCoreSystem()->GetDataCore();
            ISlideSystem *theSlideSystem = m_TransMgr->GetStudioSystem()->GetSlideSystem();
            ISlideCore *theSlideCore = m_TransMgr->GetStudioSystem()->GetSlideCore();

            size_t theSlotCursor = (size_t)-1;
            {

                qt3dsdm::IPropertySystem *thePropertySystem =
                    m_TransMgr->GetStudioSystem()->GetPropertySystem();
                qt3dsdm::SLong4 theGuid;
                {
                    Qt3DSDMPropertyHandle theTypeProperty =
                        thePropertySystem->GetAggregateInstancePropertyByName(theChildInstance,
                                                                              L"id");
                    SValue theIdValue;
                    thePropertySystem->GetInstancePropertyValue(theChildInstance, theTypeProperty,
                                                                theIdValue);
                    theGuid = qt3dsdm::get<qt3dsdm::SLong4>(theIdValue);
                }
                for (size_t theSlotIndex = 0, theSlotCount = m_ImageNameFormalNamePairs.size();
                     theSlotIndex < theSlotCount; ++theSlotIndex) {
                    bool theIsMatch = false;
                    qt3dsdm::Qt3DSDMPropertyHandle theProperty =
                        std::get<2>(m_ImageNameFormalNamePairs[theSlotIndex]);
                    SValue theValue;
                    const SUICDMPropertyDefinition &theDefinition(
                        theDataCore->GetProperty(theProperty));
                    if (theDefinition.m_Type == DataModelDataType::Long4) {
                        SValue theDCValue;
                        if (theDataCore->GetInstancePropertyValue(theInstance, theProperty,
                                                                  theDCValue)) {
                            SLong4 thePropGuid = get<SLong4>(theDCValue);
                            if (thePropGuid == theGuid)
                                theIsMatch = true;
                        }
                        Qt3DSDMSlideHandle theSlide =
                            theSlideSystem->GetAssociatedSlide(theChildInstance);
                        Qt3DSDMSlideHandle theMasterSlide = theSlideSystem->GetMasterSlide(theSlide);
                        if (theIsMatch == false && theSlide.Valid()
                            && theSlideCore->GetSpecificInstancePropertyValue(
                                   theSlide, theInstance, theProperty, theValue)) {
                            SLong4 thePropGuid = get<SLong4>(theValue);
                            if (thePropGuid == theGuid)
                                theIsMatch = true;
                        }
                    }
                    if (theIsMatch) {
                        theSlotCursor = theSlotIndex;
                        break;
                    }
                }
            }
            if (theSlotCursor != (size_t)-1) {
                Qt3DSDMPropertyHandle theImageProperty =
                    thePropertySystem->GetAggregateInstancePropertyByName(
                        m_DataHandle, std::get<0>(m_ImageNameFormalNamePairs[theSlotCursor]));
                return GetOrCreateImageBinding(
                    theImageProperty,
                    std::get<1>(m_ImageNameFormalNamePairs[theSlotCursor]).wide_str());
            } else
                return m_TransMgr->GetOrCreate(theChildInstance);
        }
    }
    return nullptr;
}

void CLayerTimelineItemBinding::OnAddChild(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    using namespace qt3dsdm;
    CClientDataModelBridge *theBridge = m_TransMgr->GetStudioSystem()->GetClientDataModelBridge();
    // This is handled via the OnPropertyChanged call below
    if (theBridge->IsImageInstance(inInstance))
        return;
    else
        Qt3DSDMTimelineItemBinding::OnAddChild(inInstance);
}

void CLayerTimelineItemBinding::OnPropertyChanged(Qt3DSDMPropertyHandle inPropertyHandle)
{
    bool theHandled = false;
    if (m_Row) {
        qt3dsdm::IPropertySystem *thePropertySystem =
            m_TransMgr->GetStudioSystem()->GetPropertySystem();
        CClientDataModelBridge *theBridge =
            m_TransMgr->GetStudioSystem()->GetClientDataModelBridge();
        qt3dsdm::TCharStr thePropertyName = thePropertySystem->GetName(inPropertyHandle);
        size_t theSlotCount = m_ImageNameFormalNamePairs.size();
        for (size_t theSlotIndex = 0; theSlotIndex < theSlotCount; ++theSlotIndex) {
            qt3dsdm::TCharStr thePropName = std::get<0>(m_ImageNameFormalNamePairs[theSlotIndex]);
            if (thePropertyName == thePropName) {
                if (ImageSlotIsFilled(thePropertySystem, m_DataHandle, thePropName)) {
                    // already created, bail!
                    if (m_TransMgr->GetBinding(GetImage(inPropertyHandle)))
                        return;

                    // Image property was changed from one non-zero guid value to another, delete
                    // the old and and create a new one
                    CImageTimelineItemBinding *theReplacedImageBinding =
                        FindImageBindingByProperty(m_Row, inPropertyHandle);
                    if (theReplacedImageBinding)
                        m_Row->RemoveChildRow(theReplacedImageBinding);

                    ITimelineItemBinding *theNextImageBinding = nullptr;
                    // Determine if this is inserted somewhere in the existing list.
                    for (size_t theNextImage = theSlotIndex + 1; theNextImage < theSlotCount;
                         ++theNextImage) {
                        qt3dsdm::TCharStr theTempName =
                            std::get<0>(m_ImageNameFormalNamePairs[theNextImage]);
                        if (ImageSlotIsFilled(thePropertySystem, m_DataHandle, theTempName)) {
                            Qt3DSDMPropertyHandle theNextImageProperty =
                                theBridge->GetAggregateInstancePropertyByName(m_DataHandle,
                                                                              theTempName);
                            theNextImageBinding =
                                m_TransMgr->GetBinding(GetImage(theNextImageProperty));
                            break;
                        }
                    }
                    m_Row->AddChildRow(
                        GetOrCreateImageBinding(
                            inPropertyHandle,
                            std::get<1>(m_ImageNameFormalNamePairs[theSlotIndex]).wide_str()),
                        theNextImageBinding);
                } else // check for delete
                {
                    // GetImage will not return anything valid since the value is nuked.
                    // From the UI end, there is no way we can tell which image is associated with
                    // this property, since that is "encapsulated" in the property value.
                    CImageTimelineItemBinding *theInvalidImageBinding =
                        FindImageBindingByProperty(m_Row, inPropertyHandle);
                    if (theInvalidImageBinding)
                        m_Row->RemoveChildRow(theInvalidImageBinding);
                }
                theHandled = true;
                break;
            }
        }
    }
    if (!theHandled)
        Qt3DSDMTimelineItemBinding::OnPropertyChanged(inPropertyHandle);
}

qt3dsdm::Qt3DSDMInstanceHandle
CLayerTimelineItemBinding::GetImage(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle)
{
    qt3dsdm::IPropertySystem *thePropertySystem = m_TransMgr->GetStudioSystem()->GetPropertySystem();
    SValue theImageValue;
    thePropertySystem->GetInstancePropertyValue(m_DataHandle, inPropertyHandle, theImageValue);
    SLong4 theImageLong4 = qt3dsdm::get<SLong4>(theImageValue);
    return m_TransMgr->GetStudioSystem()->GetClientDataModelBridge()->GetImageInstanceByGUID(
        theImageLong4);
}

ITimelineItemBinding *
CLayerTimelineItemBinding::GetOrCreateImageBinding(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
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
