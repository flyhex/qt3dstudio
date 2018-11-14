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
#include "LayerTimelineItemBinding.h"
#include "TimelineTranslationManager.h"
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
                       const QString &inStr)
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
            QString theName(thePropertySystem->GetName(theProperty));
            QString theFormalName(thePropertySystem->GetFormalName(inDataHandle, theProperty));
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
    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        Q3DStudio::CGraphIterator theChildren;
        Qt3DSDMSlideHandle theActiveSlide = m_TransMgr->GetDoc()->GetActiveSlide();
        GetAssetChildrenInTimeParent(theInstance, m_TransMgr->GetDoc(), AmITimeParent(),
                                     theChildren, theActiveSlide);
        theChildren += inIndex;
        return GetOrCreateBinding(theChildren.GetCurrent());
    }
    return nullptr;
}

QList<ITimelineItemBinding *> CLayerTimelineItemBinding::GetChildren()
{
    QList<ITimelineItemBinding *> retlist;
    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        Q3DStudio::CGraphIterator theChildren;
        Qt3DSDMSlideHandle theActiveSlide = m_TransMgr->GetDoc()->GetActiveSlide();
        GetAssetChildrenInTimeParent(theInstance, m_TransMgr->GetDoc(), AmITimeParent(),
                                     theChildren, theActiveSlide);
        int childCount = int(theChildren.GetCount());
        retlist.reserve(childCount);
        for (int i = 0; i < childCount; ++i) {
            retlist.append(GetOrCreateBinding(theChildren.GetCurrent()));
            ++theChildren;
        }
    }

    return retlist;
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
                                                   const QString &inName)
{
    qt3dsdm::Qt3DSDMInstanceHandle theImageInstance = GetImage(inPropertyHandle);
    if (!theImageInstance.Valid())
        return nullptr;
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

ITimelineItemBinding *CLayerTimelineItemBinding::GetOrCreateBinding(Qt3DSDMInstanceHandle instance)
{
    if (instance.Valid()) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
        qt3dsdm::IPropertySystem *thePropertySystem =
                m_TransMgr->GetStudioSystem()->GetPropertySystem();
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
                    thePropertySystem->GetAggregateInstancePropertyByName(
                            instance, QStringLiteral("id"));
                SValue theIdValue;
                thePropertySystem->GetInstancePropertyValue(instance, theTypeProperty, theIdValue);
                theGuid = qt3dsdm::get<qt3dsdm::SLong4>(theIdValue);
            }
            for (size_t theSlotIndex = 0, theSlotCount = m_ImageNameFormalNamePairs.size();
                 theSlotIndex < theSlotCount; ++theSlotIndex) {
                bool theIsMatch = false;
                qt3dsdm::Qt3DSDMPropertyHandle theProperty =
                    std::get<2>(m_ImageNameFormalNamePairs[theSlotIndex]);
                SValue theValue;
                const Qt3DSDMPropertyDefinition &theDefinition(
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
                        theSlideSystem->GetAssociatedSlide(instance);
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
                std::get<1>(m_ImageNameFormalNamePairs[theSlotCursor]));
        } else
            return m_TransMgr->GetOrCreate(instance);
    }
    return nullptr;
}
