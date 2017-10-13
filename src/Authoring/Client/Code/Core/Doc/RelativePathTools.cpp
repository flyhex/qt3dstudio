/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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
//	 Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	 Includes
//==============================================================================
#include "RelativePathTools.h"
#include "PathConstructionHelper.h"
#include "StackTokenizer.h"
#include "Doc.h"
#include "IObjectReferenceHelper.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "UICDMSlides.h"
#include "UICDMDataCore.h"

namespace {
bool IsEmpty(const qt3dsdm::SObjectRefType &inObjectRefValue)
{
    switch (inObjectRefValue.GetReferenceType()) {
    case qt3dsdm::ObjectReferenceType::Absolute: {
        qt3dsdm::SLong4 theValue = qt3dsdm::get<qt3dsdm::SLong4>(inObjectRefValue.m_Value);
        return theValue.m_Longs[0] == theValue.m_Longs[1]
            && theValue.m_Longs[1] == theValue.m_Longs[2]
            && theValue.m_Longs[2] == theValue.m_Longs[3] && theValue.m_Longs[3] == 0;
    } break;
    case qt3dsdm::ObjectReferenceType::Relative: {
        qt3dsdm::TDataStrPtr thePath = qt3dsdm::get<qt3dsdm::TDataStrPtr>(inObjectRefValue.m_Value);
        if (!thePath || thePath->GetLength() == 0)
            return true;
    } break;
    case qt3dsdm::ObjectReferenceType::Unknown:
        return true;
        break;
    }
    return false;
}
}
//=============================================================================
/**
 *	Determine if the asset reference is an GUID type
 */
bool CRelativePathTools::IsGUID(const qt3dsdm::SObjectRefType &inObjectRefValue)
{
    return inObjectRefValue.GetReferenceType() == qt3dsdm::ObjectReferenceType::Absolute;
}

//=============================================================================
/**
 *	Determine if the asset reference is relative
 */
bool CRelativePathTools::IsRelativePath(const qt3dsdm::SObjectRefType &inObjectRefValue)
{
    return !IsGUID(inObjectRefValue);
}

//=============================================================================
/**
 *	Return the path type
 */
CRelativePathTools::EPathType
CRelativePathTools::GetPathType(const qt3dsdm::SObjectRefType &inObjectRefValue)
{
    // default to absolute pathing.
    if (IsEmpty(inObjectRefValue))
        return EPATHTYPE_GUID;

    if (IsGUID(inObjectRefValue))
        return EPATHTYPE_GUID;
    else
        return EPATHTYPE_RELATIVE;
}

//=============================================================================
/**
 *	Build a object reference path, via Ids
 */
Q3DStudio::CString
CRelativePathTools::BuildReferenceString(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                         const qt3dsdm::CUICDMInstanceHandle inRootInstance,
                                         EPathType inPathType, CDoc *inDoc)
{
    switch (inPathType) {
    case EPATHTYPE_GUID:
        return BuildAbsoluteReferenceString(inInstance, inDoc);
    case EPATHTYPE_RELATIVE:
        return BuildRelativeReferenceString(inInstance, inRootInstance, inDoc);
    };
    return L"";
}

//=============================================================================
/**
 *	Note that this uses object names rather than nice names, because script access is via
 *property names, in the runtime.
 */
Q3DStudio::CString
CRelativePathTools::BuildAbsoluteReferenceString(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                                 CDoc *inDoc)
{
    if (inInstance.Valid() == false)
        return L"";
    Q3DStudio::CString theNameStart;
    Q3DStudio::CString theNameEnd(
        CPathConstructionHelper::EscapeAssetName(LookupObjectName(inInstance, inDoc)));

    qt3dsdm::CUICDMInstanceHandle theParentInstance =
        inDoc->GetStudioSystem()->GetClientDataModelBridge()->GetParentInstance(inInstance);
    if (theParentInstance.Valid())
        theNameStart = BuildAbsoluteReferenceString(theParentInstance, inDoc) + ".";
    theNameStart += theNameEnd;
    return theNameStart;
}

Q3DStudio::CString
CRelativePathTools::BuildRelativeReferenceString(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                                 const qt3dsdm::CUICDMInstanceHandle inRootInstance,
                                                 CDoc *inDoc)
{
    Q3DStudio::CString theAbsRelPath(BuildAbsoluteReferenceString(inInstance, inDoc));
    Q3DStudio::CString theAbsRootPath(BuildAbsoluteReferenceString(inRootInstance, inDoc));
    return CPathConstructionHelper::BuildRelativeReferenceString(theAbsRelPath, theAbsRootPath);
}

//=============================================================================
/**
 *	Parse the specified path and return the id of the specified object, if the
 *	path does not points an object, the immediate object it manages to travesed to
 *	is return, worst case, the Scene is returned.
 *	@param inRootInstance it the based object passed in, mainly used for relative path
 *	@param inString the path to be resolved
 *	@param outPathType is the path type, either absolute or relative
 *	@param outIsFullyResolved defines whether the inString can completely resolved to an object
 *	@return theId of the object (or the object it manages to traverse to) N.B. this is required
 *	for the Object Ref Picker to highlight that parent object if that token is invalid.
 */
qt3dsdm::CUICDMInstanceHandle CRelativePathTools::FindAssetInstanceByObjectPath(
    CDoc *inDoc, const qt3dsdm::CUICDMInstanceHandle &inRootInstance,
    const Q3DStudio::CString &inString, EPathType &outPathType, bool &outIsResolved,
    const IObjectReferenceHelper *inHelper /*= NULL */)
{
    CClientDataModelBridge *theBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();

    CStackTokenizer theTokenizer(inString, CPathConstructionHelper::GetPathDelimiter().GetAt(0),
                                 CPathConstructionHelper::GetEscapeChar().GetAt(0));
    // Default to the scene if asset cannot be found.
    qt3dsdm::CUICDMInstanceHandle theFoundInstance;

    if (theTokenizer.HasNextPartition()) {
        Q3DStudio::CString theCurrentToken(theTokenizer.GetCurrentPartition());

        // this is the default path type
        outPathType = EPATHTYPE_RELATIVE;

        outIsResolved = false;

        // Start parsing from this object
        if (theCurrentToken.Compare(CPathConstructionHelper::GetThisString(), false)) {
            outIsResolved = true;
            theFoundInstance = inRootInstance;
            outPathType = EPATHTYPE_RELATIVE;
        } else if (theCurrentToken.Compare(CPathConstructionHelper::GetParentString(), false)) {
            outIsResolved = true;
            theFoundInstance = theBridge->GetParentInstance(inRootInstance);
            outPathType = EPATHTYPE_RELATIVE;

            // Special case for the scene
            if (!theFoundInstance.Valid())
                theFoundInstance = inRootInstance;
        }

        // otherwise, just use the scene (if it's "Scene" or otherwise)
        ++theTokenizer;
    }

    if (theFoundInstance.Valid()) {
        qt3dsdm::CUICDMInstanceHandle theTimeParent =
            theBridge->GetOwningComponentInstance(theFoundInstance);
        if (theTimeParent.Valid()) {
            qt3dsdm::CUICDMSlideHandle theCurrentSlide =
                theBridge->GetComponentActiveSlide(theTimeParent);
            return DoFindAssetInstanceByObjectPath(inDoc, theFoundInstance, theTimeParent,
                                                   theCurrentSlide, theTokenizer, outIsResolved,
                                                   inHelper);
        } else
            return DoFindAssetInstanceByObjectPath(inDoc, theFoundInstance, theTimeParent, 0,
                                                   theTokenizer, outIsResolved, inHelper);
    } else {
        return inDoc->GetSceneInstance();
    }
}

qt3dsdm::SObjectRefType
CRelativePathTools::CreateAssetRefValue(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                        const qt3dsdm::CUICDMInstanceHandle inRootInstance,
                                        EPathType inPathType, CDoc *inDoc)
{
    qt3dsdm::SObjectRefType theAssetRefValue;
    switch (inPathType) {
    case EPATHTYPE_GUID: {
        Q3DStudio::TGUIDPacked thePacked(
            inDoc->GetStudioSystem()->GetClientDataModelBridge()->GetGUID(inInstance));
        qt3dsdm::SLong4 theGuid(thePacked.Data1, thePacked.Data2, thePacked.Data3, thePacked.Data4);
        theAssetRefValue = theGuid;
        break;
    }
    case EPATHTYPE_RELATIVE: {
        theAssetRefValue = qt3dsdm::TDataStrPtr(
            new qt3dsdm::CDataStr(BuildRelativeReferenceString(inInstance, inRootInstance, inDoc)));
        break;
    }
    default:
        break;
    }
    return theAssetRefValue;
}

//==============================================================================
/**
 *	Helper function for FindAssetInstanceByObjectPath( ) method.
 *	Given a root object and a tokenizer for parsing a path, this method will
 *	recurse down a tree of assets until it finds the desired one.
 *	@param	inRootAsset	the asset to start searching from
 *	@param	ioTokenizer	a string tokenizer to help parse path of asset.
 *	@return	the source ID of the final asset
 *	@see	FindAssetInstanceByObjectPath
 */
qt3dsdm::CUICDMInstanceHandle CRelativePathTools::DoFindAssetInstanceByObjectPath(
    CDoc *inDoc, const qt3dsdm::CUICDMInstanceHandle &inRootInstance,
    const qt3dsdm::CUICDMInstanceHandle inTimeParentInstance, qt3dsdm::CUICDMSlideHandle inSlide,
    CStackTokenizer &ioTokenizer, bool &outIsResolved, const IObjectReferenceHelper *inHelper)
{
    CClientDataModelBridge *theBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();

    qt3dsdm::CUICDMInstanceHandle theFoundInstance = inRootInstance;
    // There is another object to parse
    if (inRootInstance.Valid() && ioTokenizer.HasNextPartition()) {
        Q3DStudio::CString theCurrentToken(ioTokenizer.GetCurrentPartition());
        if (theCurrentToken.Length()) {
            outIsResolved = false;
            // Parent asset
            if (theCurrentToken.Compare(CPathConstructionHelper::GetParentString(), false)) {
                ++ioTokenizer;
                qt3dsdm::CUICDMInstanceHandle theParentInstance =
                    theBridge->GetParentInstance(inRootInstance);
                if (theParentInstance.Valid()) {
                    outIsResolved = true;
                    theFoundInstance = DoFindAssetInstanceByObjectPath(
                        inDoc, theParentInstance, inTimeParentInstance, inSlide, ioTokenizer,
                        outIsResolved, inHelper);
                }
            }
            // Find the asset by name
            else {
                Q3DStudio::CString theDesiredAssetName(ioTokenizer.GetCurrentPartition());

                if (!outIsResolved && inHelper) {
                    qt3dsdm::TInstanceHandleList theChildList;
                    if (inHelper->GetChildInstanceList(inRootInstance, theChildList, inSlide,
                                                       inTimeParentInstance)) {
                        for (size_t theIndex = 0; theIndex < theChildList.size(); ++theIndex) {
                            Q3DStudio::CString theCurrentAssetName(
                                LookupObjectName(theChildList[theIndex], inDoc));
                            if (theDesiredAssetName.Compare(theCurrentAssetName, false)) {
                                ++ioTokenizer;
                                outIsResolved = true;
                                theFoundInstance = DoFindAssetInstanceByObjectPath(
                                    inDoc, theChildList[theIndex], inTimeParentInstance, inSlide,
                                    ioTokenizer, outIsResolved, inHelper);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return outIsResolved ? theFoundInstance : qt3dsdm::CUICDMInstanceHandle(0);
}

//==============================================================================
/**
 * Figures out the object name, used for script access. so that paths are valid in the runtime.
 */
Q3DStudio::CString
CRelativePathTools::LookupObjectName(const qt3dsdm::CUICDMInstanceHandle inInstance, CDoc *inDoc)
{
    qt3dsdm::IPropertySystem *thePropertySystem = inDoc->GetStudioSystem()->GetPropertySystem();
    CClientDataModelBridge *theClientBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();
    if (theClientBridge->IsImageInstance(inInstance)) {
        qt3dsdm::CUICDMInstanceHandle theParentInstance;
        qt3dsdm::CUICDMPropertyHandle theProperty;
        if (!theClientBridge->GetMaterialFromImageInstance(inInstance, theParentInstance,
                                                           theProperty))
            theClientBridge->GetLayerFromImageProbeInstance(inInstance, theParentInstance,
                                                            theProperty);
        qt3dsdm::TCharStr theName = thePropertySystem->GetName(theProperty);
        return theName.c_str();
    } else {
        qt3dsdm::SValue theNameValue;
        thePropertySystem->GetInstancePropertyValue(inInstance, theClientBridge->GetNameProperty(),
                                                    theNameValue);
        qt3dsdm::TDataStrPtr theName = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theNameValue);
        return theName->GetData();
    }
}
