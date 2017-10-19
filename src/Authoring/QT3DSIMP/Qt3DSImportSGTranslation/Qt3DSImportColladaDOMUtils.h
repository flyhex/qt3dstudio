/****************************************************************************
**
** Copyright (C) 1999-2009 NVIDIA Corporation.
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
// Includes
//==============================================================================
#pragma once
#include "Qt3DSImportSceneGraphTranslation.h"
#include "Qt3DSImportTranslationCommon.h"
#include "Qt3DSImportColladaAnimationUtils.h"

namespace qt3dsimp {

typedef struct _SSourceArrayInfo
{
    long m_POffset;
    long m_Stride;
    std::vector<long> m_Offset;
    domFloat_arrayRef m_Array;

    _SSourceArrayInfo()
        : m_POffset(-1)
        , m_Stride(-1)
        , m_Array(NULL)
    {
    }
} SSourceArrayInfo;

//==============================================================================
/**
 *	Attempts to retrieve an identifier string from an daeElement.
 *	The precedence is to try to use 'name' first, failing that the 'id' and lastly
 *	the 'sid'.
 */
const char *GetNameOrIDOrSid(const daeElement *inElement)
{
    static std::string theBuffer;

    daeElement *theElement = const_cast<daeElement *>(inElement);

    theBuffer.clear();

    if (theElement->isAttributeSet("name"))
        theElement->getAttribute("name", theBuffer);
    else if (theElement->isAttributeSet("id"))
        theElement->getAttribute("id", theBuffer);
    else if (theElement->isAttributeSet("sid"))
        theElement->getAttribute("sid", theBuffer);
    else
        return theElement->getElementName();

    return theBuffer.c_str();
}

static inline float GetFloatFromElementChar(const daeElementRef inElement)
{
    return static_cast<float>(::atof(inElement->getCharData().c_str()));
}

static inline int GetIntFromElementChar(const daeElementRef inElement)
{
    return ::atoi(inElement->getCharData().c_str());
}

//==============================================================================
/**
 *	Helper function to retrieve the appropriate source array for this input semantic.
 */
void GetSourceArrayInfo(
    const domSource *inDomSource, const TStringList &inParamIdentifiers, long inPOffset,
    SSourceArrayInfo &outArrayInfo,
    std::function<void(ESceneGraphWarningCode, const char *)> inLogWarningFunction = NULL)
{
    if (outArrayInfo.m_POffset > -1) // we don't support multiple array infos (ex: collada file with
                                     // multiple TEXCOORD for the same triangle)
    {
        if (inLogWarningFunction)
            inLogWarningFunction(ESceneGraphWarningCode_TrianglesDuplicateSemantic,
                                 GetNameOrIDOrSid(inDomSource));
        return;
    }

    outArrayInfo.m_POffset = inPOffset;
    outArrayInfo.m_Stride =
        (long)inParamIdentifiers.size(); // Assumption in case domTechnique_common does not exist

    outArrayInfo.m_Array = inDomSource->getFloat_array(); // TODO: Support other array types?

    for (long theIdentifierIndex = 0; theIdentifierIndex < (long)inParamIdentifiers.size();
         ++theIdentifierIndex)
        outArrayInfo.m_Offset.push_back(-1);

    const domSource::domTechnique_commonRef theTechniqueCommonRef =
        inDomSource->getTechnique_common();
    if (theTechniqueCommonRef) {
        domAccessorRef theAccessorRef = theTechniqueCommonRef->getAccessor();
        outArrayInfo.m_Stride = (long)theAccessorRef->getStride();

        const domParam_Array &theParamsArray = theAccessorRef->getParam_array();
        long theParamCount = (long)theParamsArray.getCount();

        // Search for required identifiers and their relative indicies within the array
        for (long theIdentifierIndex = 0; theIdentifierIndex < (long)inParamIdentifiers.size();
             ++theIdentifierIndex) {
            for (long theParamIndex = 0; theParamIndex < theParamCount; ++theParamIndex) {
                // Skip param indicies that have already been assigned, since repeats of the same
                // param is possible
                if (std::find(outArrayInfo.m_Offset.begin(), outArrayInfo.m_Offset.end(),
                              theParamIndex)
                    != outArrayInfo.m_Offset.end())
                    continue;

                const daeString &theParamName = theParamsArray[theParamIndex]->getName();

                if (!theParamName)
                    outArrayInfo.m_Offset[theIdentifierIndex] = theParamIndex;
                // Valid param name present
                else if (qstricmp(theParamName, inParamIdentifiers[theIdentifierIndex].c_str())
                         == 0) {
                    outArrayInfo.m_Offset[theIdentifierIndex] =
                        theParamIndex; // Get the actual offset within the params for identifiers
                                       // that we are interested in
                    break;
                }
            }
        }
    }
}

//==============================================================================
/**
 *	Helper function to retrieve the appropriate source array for this input semantic.
 */
void GetDomSourceArrayAndVectorCapacity(
    const domInputLocalOffsetRef inInputOffsetRef, const TStringList &inParamIdentifiers,
    SSourceArrayInfo &outArrayInfo,
    std::function<void(ESceneGraphWarningCode, const char *)> inLogWarningFunction = NULL)
{
    domSource *theDomSource = NULL;

    daeElement *theInputSource = inInputOffsetRef->getSource().getElement();
    switch (GetElementType(*theInputSource)) {
    case COLLADA_TYPE::SOURCE:
        theDomSource = daeSafeCast<domSource>(theInputSource);
        break;
    default:
        break;
    }

    GetSourceArrayInfo(theDomSource, inParamIdentifiers, (long)inInputOffsetRef->getOffset(),
                       outArrayInfo, inLogWarningFunction);
}

//==============================================================================
/**
 *	Helper function to retrieve the appropriate source array for this input semantic.
 */
void GetDomSourceArrayInfo(const domInputLocal_Array &inInputsArray, const char *inSemantic,
                           const TStringList &inParamIdentifiers, SSourceArrayInfo &outArrayInfo)
{
    long theInputOffsetsCount = (long)inInputsArray.getCount();

    // Retrieve the appropriate source array depending on the semantic name (inputs with offsets)
    for (long theIndex = 0; theIndex < theInputOffsetsCount; ++theIndex) {
        const domInputLocalRef theInputRef = inInputsArray[theIndex];
        const char *theSemantic = theInputRef->getSemantic();

        if (::strcmp(theSemantic, inSemantic) == 0) {
            domSource *theDomSource = daeSafeCast<domSource>(theInputRef->getSource().getElement());
            GetSourceArrayInfo(theDomSource, inParamIdentifiers, -1, outArrayInfo);
            break;
        }
    }

    return;
}

//==============================================================================
/**
 *	Helper function to push raw float values into outVector.
 */
void Push3Floats(std::vector<float> &outVector, unsigned long inFaceIndex, const float *inValues)
{
    inFaceIndex *= 3;

    outVector[inFaceIndex] = inValues[0];
    outVector[inFaceIndex + 1] = inValues[1];
    outVector[inFaceIndex + 2] = inValues[2];
}

//==============================================================================
/**
 *	Helper function to push raw float values into outVector.
 */
void Push2Floats(std::vector<float> &outVector, unsigned long inFaceIndex, const float *inValues)
{
    inFaceIndex *= 2;

    outVector[inFaceIndex] = inValues[0];
    outVector[inFaceIndex + 1] = inValues[1];
}

//==============================================================================
/**
 *	Retrieves float values from a source array given the array index.
 */
void GetFaceTupleValue(float *outValues, const SSourceArrayInfo &inSourceArray,
                       unsigned long inArrayIndex)
{
    inArrayIndex %= ULONG_MAX; // Fixes any overflowed indicies due to bad Collada data.

    long theIdentifiersCount = (long)inSourceArray.m_Offset.size();
    inArrayIndex *= inSourceArray.m_Stride;

    //////////////////////////////////////////////////////////////////////////////
    // This helps some bogus .dae files load. Today we're loading:
    // //sw/devtools/metools/public/UIComposer/Main/Test/Collada/Maya/2009/NextGen/SimpleTexture.dae
    while (inArrayIndex >= inSourceArray.m_Array->getCount())
        inArrayIndex -= inSourceArray.m_Stride;
    //////////////////////////////////////////////////////////////////////////////

    for (long theIdentifierIndex = 0; theIdentifierIndex < theIdentifiersCount;
         ++theIdentifierIndex) {
        outValues[theIdentifierIndex] =
            (float)inSourceArray.m_Array
                ->getValue()[inArrayIndex + inSourceArray.m_Offset[theIdentifierIndex]];
    }
}

//==============================================================================
/**
 *	Retrieves float values from a source array given the array index.
 *  Here we assume a packed source array with 3 components
 */
void GetFaceTupleValue(float *outValues, const std::vector<SVector3> &inSourceArray,
                       unsigned long inArrayIndex)
{
    inArrayIndex %= ULONG_MAX; // Fixes any overflowed indicies due to bad Collada data.

    outValues[0] = inSourceArray[inArrayIndex][0];
    outValues[1] = inSourceArray[inArrayIndex][1];
    outValues[2] = inSourceArray[inArrayIndex][2];
}

//==============================================================================
/**
 *	Find an element using it's SID.
 */
bool FindElementBySid(const xsNCName &inSidString, daeElement *&outElement,
                      const daeElement *inContainerElement)
{
    outElement = NULL;

    outElement = daeSidRef(inSidString, const_cast<daeElement *>(inContainerElement)).resolve().elt;
    if (outElement != NULL)
        return true;
    else
        return false;
}

//==============================================================================
/**
 *	Recursively travel through parents in an attempt to find an element using it's SID.
 *	inStartElement defines where to start looking for.
 *	inStartAtType and inEndAtAtype determines which element types to start and end the seatch.
 *	All elements in the middle of the two are searched as well.
 *	Supplying a -1 for inStartAtType starts the search immediately from inStartElement.
 */
bool RecursiveFindElementBySid(const xsNCName &inSidString, daeElement *&outElement,
                               daeElement *inStartElement, const daeInt inStartAtType,
                               const daeInt inEndAtType, bool inBegin = false)
{
    if (inStartElement == NULL)
        return false;

    daeInt theStartType = inStartElement->typeID();

    if (theStartType == inStartAtType
        || inStartAtType == -1) // -1 means start the search immediately
        inBegin = true;

    if (inBegin) {
        if (FindElementBySid(inSidString, outElement, inStartElement))
            return true;
        else if (theStartType != inEndAtType)
            return RecursiveFindElementBySid(inSidString, outElement, inStartElement->getParent(),
                                             inStartAtType, inEndAtType, inBegin);
    } else
        return RecursiveFindElementBySid(inSidString, outElement, inStartElement->getParent(),
                                         inStartAtType, inEndAtType, inBegin);

    return false;
}

//==============================================================================
/**
 *	Helper macros for accessing color, float and texture params for profile_common
 *	material properties.
 */
// Begin macro defination
#define BeginCommonTechniqueObject(inProfileObject, inTechniqueType)                               \
    {                                                                                              \
        const domProfile_COMMON::domTechnique::dom##inTechniqueType##Ref theTechniqueObject =      \
            inProfileObject->get##inTechniqueType();                                               \
        if (theTechniqueObject != NULL) {                                                          \
            SMaterialParameters theMaterialParameters(                                             \
                EMatCommonProfileTechnique_##inTechniqueType);                                     \
            ProcessProfileCommonTechniqueExtra(inProfileObject, theMaterialParameters.m_Extra);

#define EndCommonTechniqueObject()                                                                 \
    SetMaterial(theMaterialParameters);                                                            \
    }                                                                                              \
    }

#define ProcessFloatParam(inParamName)                                                             \
    {                                                                                              \
        domCommon_float_or_param_typeRef theFloatOrParamRef =                                      \
            theTechniqueObject->get##inParamName();                                                \
        if (theFloatOrParamRef) {                                                                  \
            domCommon_float_or_param_type::domFloatRef theFloatRef =                               \
                theFloatOrParamRef->getFloat();                                                    \
            if (theFloatRef)                                                                       \
                theMaterialParameters.m_##inParamName.SetValue((float)theFloatRef->getValue());    \
        }                                                                                          \
    }

#define ProcessColorOrTextureParam(inParamName, inMapType)                                         \
    GetColorOrTextureParamInfo(theTechniqueObject->get##inParamName(),                             \
                               theMaterialParameters.m_##inParamName, inMapType);

#define ProcessTransparentParam(inParamName)                                                       \
    domCommon_transparent_typeRef theTransparentParamRef = theTechniqueObject->get##inParamName(); \
    if (theTransparentParamRef) {                                                                  \
        theMaterialParameters.m_##inParamName##OpaqueType.SetValue(                                \
            theTransparentParamRef->getOpaque());                                                  \
        GetColorOrTextureParamInfo(theTransparentParamRef, theMaterialParameters.m_##inParamName,  \
                                   ETextureMapTypeOpacity);                                        \
    }

// End macro definations
}
