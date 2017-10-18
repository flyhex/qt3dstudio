/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
// Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
// Includes
//==============================================================================
#include "DataModelObjectReferenceHelper.h"
#include "Core.h"
#include "Doc.h"
#include "GraphUtils.h"

// UICDM
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "UICDMSlides.h"
#include "StudioFullSystem.h"
#include "UICDMDataCore.h"
#include "Graph.h"

inline void GetAllowableParentSlides(CClientDataModelBridge *inBridge,
                                     qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                     qt3dsdm::TSlideHandleList &inList)
{
    if (!inInstance.Valid())
        return;

    qt3dsdm::CUICDMSlideHandle theSlide = 0;
    qt3dsdm::Qt3DSDMInstanceHandle theTimeParent = inBridge->GetOwningComponentInstance(inInstance);
    if (theTimeParent.Valid()) {
        if (inBridge->IsMaster(inInstance)) {
            // we use master time context if parent is a master object
            theSlide = inBridge->GetComponentSlide(theTimeParent, 0);
        } else // else, we use the current time context of the parent
            theSlide = inBridge->GetComponentActiveSlide(theTimeParent);

        // depth first, so that we have Scene at the beginning of the list.
        GetAllowableParentSlides(inBridge, theTimeParent, inList);
    }

    if (theSlide.Valid())
        inList.push_back(theSlide);
}

CObjectReferenceHelper::CObjectReferenceHelper(CDoc *inDoc)
    : m_Doc(inDoc)
{
}

CObjectReferenceHelper::~CObjectReferenceHelper()
{
}

IObjectReferenceHelper::SObjectRefInfo
CObjectReferenceHelper::GetInfo(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance) const
{
    // UICDM
    using namespace qt3dsdm;
    CClientDataModelBridge *theClientBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();

    if (inInstance.Valid()) {
        IObjectReferenceHelper::SObjectRefInfo theInfo;
        IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
        // Type
        theInfo.m_Type = theClientBridge->GetObjectType(inInstance);
        // Name
        if (theInfo.m_Type != OBJTYPE_IMAGE) {
            SValue theNameValue;
            thePropertySystem->GetInstancePropertyValue(
                inInstance, theClientBridge->GetNameProperty(), theNameValue);
            TDataStrPtr theName = qt3dsdm::get<TDataStrPtr>(theNameValue);
            if (theName)
                theInfo.m_Name = theName->GetData();
        } else {
            qt3dsdm::Qt3DSDMInstanceHandle theParentInstance;
            qt3dsdm::Qt3DSDMPropertyHandle theProperty;
            if (!theClientBridge->GetMaterialFromImageInstance(inInstance, theParentInstance,
                                                               theProperty))
                theClientBridge->GetLayerFromImageProbeInstance(inInstance, theParentInstance,
                                                                theProperty);
            if (theParentInstance.Valid() && theProperty.Valid())
                theInfo.m_Name =
                    thePropertySystem->GetFormalName(theParentInstance, theProperty).c_str();
        }
        // Master, by checking if guid property is linked.
        theInfo.m_Master = m_Doc->GetStudioSystem()->GetSlideSystem()->IsPropertyLinked(
            inInstance, theClientBridge->GetIdProperty());

        return theInfo;
    }

    return IObjectReferenceHelper::SObjectRefInfo();
}

// Return the list of slide handles that is 'accessible' via this base id.
qt3dsdm::TSlideHandleList
CObjectReferenceHelper::GetSlideList(const qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    qt3dsdm::TSlideHandleList theList;
    if (inInstance.Valid()) {
        CClientDataModelBridge *theBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
        qt3dsdm::CUICDMSlideHandle theCurrentSlide = m_Doc->GetActiveSlide();
        if (theBridge->IsComponentInstance(inInstance)) {
            // if selected object is a component, and we are inside it
            GetAllowableParentSlides(theBridge, inInstance, theList);
        } else {
            // not a component, or we are outside of a component
            GetAllowableParentSlides(theBridge, theBridge->GetOwningComponentInstance(inInstance),
                                     theList);
        }
        theList.push_back(theCurrentSlide); // this is always appended
    }
    return theList;
}

// Return all children under inSlideIndex
bool CObjectReferenceHelper::GetChildInstanceList(
    const qt3dsdm::Qt3DSDMInstanceHandle &inInstance, qt3dsdm::TInstanceHandleList &outList,
    qt3dsdm::CUICDMSlideHandle inSlide, const qt3dsdm::Qt3DSDMInstanceHandle &inOwningInstance) const
{
    (void)inOwningInstance;
    CClientDataModelBridge *theClientBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
    if (inInstance.Valid()) {
        //		ASSERT(0); // Should this work for more than Materials?
        if (theClientBridge->IsMaterialInstance(inInstance)) { // UICDM objects
            long theSlideIndex = m_Doc->GetStudioSystem()->GetSlideSystem()->GetSlideIndex(inSlide);
            GetPropertyAsChildrenList(inInstance, outList, theSlideIndex);
            return true;
            // no support for actual hierarchy.
        }
        Q3DStudio::CGraphIterator theChildren;
        m_Doc->GetAssetGraph()->GetChildren(theChildren, inInstance);
        for (; !theChildren.IsDone(); ++theChildren) {
            outList.push_back(theChildren.GetCurrent());
        }
        return true;
    }
    return false;
}

//==============================================================================
/**
 * Figures out the object (displayed) name for a given instance
 */
Q3DStudio::CString
CObjectReferenceHelper::LookupObjectFormalName(const qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    qt3dsdm::IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    CClientDataModelBridge *theClientBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
    if (theClientBridge->IsImageInstance(inInstance)) {
        qt3dsdm::Qt3DSDMInstanceHandle theParentInstance;
        qt3dsdm::Qt3DSDMPropertyHandle theProperty;
        if (!theClientBridge->GetMaterialFromImageInstance(inInstance, theParentInstance,
                                                           theProperty))
            theClientBridge->GetLayerFromImageProbeInstance(inInstance, theParentInstance,
                                                            theProperty);
        qt3dsdm::TCharStr theFormalName =
            thePropertySystem->GetFormalName(theParentInstance, theProperty);
        return theFormalName.c_str();
    } else {
        qt3dsdm::SValue theNameValue;
        thePropertySystem->GetInstancePropertyValue(inInstance, theClientBridge->GetNameProperty(),
                                                    theNameValue);
        if (GetValueType(theNameValue) == qt3dsdm::DataModelDataType::String) {
            qt3dsdm::TDataStrPtr theName = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theNameValue);
            return theName->GetData();
        }
        return L"";
    }
}

//==============================================================================
/**
 * String returned for displaying relative path values in the Object Ref Picker
 */
Q3DStudio::CString CObjectReferenceHelper::GetObjectReferenceString(
    const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance, CRelativePathTools::EPathType inPathType,
    const qt3dsdm::Qt3DSDMInstanceHandle &inInstance) const
{
    return CRelativePathTools::BuildReferenceString(inInstance, inBaseInstance, inPathType, m_Doc);
}

//==============================================================================
/**
 * NOTE: inId is never a UICDM object, till we support dynamic properties OR actions for UICDM
 * objects.
 */
bool CObjectReferenceHelper::ResolvePath(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                                         const Q3DStudio::CString &inPathValue,
                                         CRelativePathTools::EPathType &outType,
                                         qt3dsdm::Qt3DSDMInstanceHandle &outResolvedInstance)
{
    if (inInstance.Valid()) {
        bool theFullResolvedFlag;
        outResolvedInstance = CRelativePathTools::FindAssetInstanceByObjectPath(
            m_Doc, inInstance, inPathValue, outType, theFullResolvedFlag, this);
        return outResolvedInstance.Valid();
    }
    return false;
}
using namespace qt3dsdm;

qt3dsdm::Qt3DSDMInstanceHandle
CObjectReferenceHelper::Resolve(const qt3dsdm::SValue &inObjectRefValue,
                                const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance) const
{
    CClientDataModelBridge *theBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
    if (inBaseInstance.Valid()) {
        SValue theRefValue(inObjectRefValue);
        qt3dsdm::DataModelDataType::Value theValueType = GetValueType(inObjectRefValue);
        if (theValueType == DataModelDataType::ObjectRef) {
            const SObjectRefType &theRef(qt3dsdm::get<SObjectRefType>(inObjectRefValue));
            switch (theRef.GetReferenceType()) {
            case ObjectReferenceType::Absolute:
                theRefValue = qt3dsdm::get<SLong4>(theRef.m_Value);
                break;
            case ObjectReferenceType::Relative:
                theRefValue = qt3dsdm::get<TDataStrPtr>(theRef.m_Value);
                break;
            default:
                break;
            }
            theValueType = GetValueType(theRefValue);
        }
        if (theValueType == qt3dsdm::DataModelDataType::String) // route it through the function that
                                                             // can determine both old client and
                                                             // new UICDM objects
        {
            bool theFullResolvedFlag = false;
            CRelativePathTools::EPathType theUnusedPathType;
            return CRelativePathTools::FindAssetInstanceByObjectPath(
                m_Doc, inBaseInstance, qt3dsdm::get<qt3dsdm::TDataStrPtr>(theRefValue)->GetData(),
                theUnusedPathType, theFullResolvedFlag, this);
        } else if (theValueType == qt3dsdm::DataModelDataType::Long) {
            return qt3dsdm::get<qt3ds::QT3DSI32>(theRefValue);
        } else if (theValueType == qt3dsdm::DataModelDataType::Long4) {
            qt3dsdm::SLong4 theGuid = qt3dsdm::get<qt3dsdm::SLong4>(theRefValue);
            return theBridge->GetInstanceByGUID(theGuid);
        }
    }
    return qt3dsdm::Qt3DSDMInstanceHandle();
}

//==============================================================================
/**
 * This is triggered when values are changed in the Object Ref picker, make sure the path type is
 * preserved.
 */
qt3dsdm::SObjectRefType
CObjectReferenceHelper::GetAssetRefValue(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                                         const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance,
                                         CRelativePathTools::EPathType inPathType) const
{
    return CRelativePathTools::CreateAssetRefValue(inInstance, inBaseInstance, inPathType, m_Doc);
}

void CObjectReferenceHelper::GetPropertyAsChildrenList(
    const qt3dsdm::Qt3DSDMInstanceHandle &inInstance, qt3dsdm::TInstanceHandleList &outList,
    long inSlideIndex) const
{
    if (inInstance.Valid()) {
        CClientDataModelBridge *theClientBridge =
            m_Doc->GetStudioSystem()->GetClientDataModelBridge();
        using namespace qt3dsdm;

        IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();

        ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
        TPropertyHandleList theProperties;
        thePropertySystem->GetAggregateInstanceProperties(inInstance, theProperties);

        for (size_t thePropertyIndex = 0; thePropertyIndex < theProperties.size();
             ++thePropertyIndex) {
            Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];
            AdditionalMetaDataType::Value theAdditionalMetaDataType =
                thePropertySystem->GetAdditionalMetaDataType(inInstance, theProperty);

            if (theAdditionalMetaDataType == AdditionalMetaDataType::Image) {
                SValue theValue;
                if ((inSlideIndex >= 0
                     && theSlideSystem->GetSlidePropertyValue(inSlideIndex, inInstance, theProperty,
                                                              theValue))
                    || m_Doc->GetStudioSystem()->GetFullSystem()->GetCanonicalInstancePropertyValue(
                           inInstance, theProperty, theValue)) {
                    SLong4 theLong4 = qt3dsdm::get<SLong4>(theValue);
                    if (theLong4.m_Longs[0] != 0 || theLong4.m_Longs[1] != 0
                        || theLong4.m_Longs[2] != 0 || theLong4.m_Longs[3] != 0) {
                        Qt3DSDMInstanceHandle theImageInstance =
                            theClientBridge->GetInstanceByGUID(theLong4);
                        outList.push_back(theImageInstance);
                    }
                }
            }
        }
    }
}
