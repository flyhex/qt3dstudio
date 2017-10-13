/****************************************************************************
**
** Copyright (C) 2005 NVIDIA Corporation.
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
#include "PathConstructionHelper.h"
#include "Doc.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "StackTokenizer.h"

//==============================================================================
////	 Constants
//==============================================================================
static const Q3DStudio::CString SOURCEPATHDELIMITER(L"."); // can only be single char!
static const Q3DStudio::CString SOURCEPATHESCAPECHAR(L"\\"); // can only be single char!
static const Q3DStudio::CString SOURCEPATHSCENE(L"Scene");
static const Q3DStudio::CString SOURCEPATHPARENT(L"parent");
static const Q3DStudio::CString SOURCEPATHTHIS(L"this");

//=============================================================================
/**
 *	Create a absolute path reference string
 */
Q3DStudio::CString
CPathConstructionHelper::BuildAbsoluteReferenceString(CDoc *inDoc,
                                                      qt3dsdm::CUICDMInstanceHandle inInstance)
{
    CClientDataModelBridge *theBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();
    Q3DStudio::CString theNameStart;
    Q3DStudio::CString theNameEnd = EscapeAssetName(theBridge->GetName(inInstance));

    qt3dsdm::CUICDMInstanceHandle theParentInstance = theBridge->GetParentInstance(inInstance);
    if (theParentInstance.Valid()) {
        theNameStart = BuildAbsoluteReferenceString(inDoc, theParentInstance);
        theNameStart += ".";
    }
    theNameStart += theNameEnd;

    return theNameStart;
}

//=============================================================================
/**
 *	Create a relative path reference string
 */
Q3DStudio::CString CPathConstructionHelper::BuildRelativeReferenceString(
    CDoc *inDoc, qt3dsdm::CUICDMInstanceHandle inInstance, qt3dsdm::CUICDMInstanceHandle inRootInstance)
{
    Q3DStudio::CString theAbsRelPath = BuildAbsoluteReferenceString(inDoc, inInstance);
    Q3DStudio::CString theAbsRootPath = BuildAbsoluteReferenceString(inDoc, inRootInstance);

    return CPathConstructionHelper::BuildRelativeReferenceString(theAbsRelPath, theAbsRootPath);
}

//=============================================================================
/**
 *	helper method to create therelative path reference string based on it's
 *	absolute path string and the root object absolute path string
 */
Q3DStudio::CString
CPathConstructionHelper::BuildRelativeReferenceString(Q3DStudio::CString &inAbsObjPath,
                                                      Q3DStudio::CString &inAbsRootPath)
{
    Q3DStudio::CString theRelPath = ""; // SOURCEPATHTHIS;
    CStackTokenizer theAbsRelTokenizer(inAbsObjPath, SOURCEPATHDELIMITER.GetAt(0),
                                       SOURCEPATHESCAPECHAR.GetAt(0));
    CStackTokenizer theAbsRootTokenizer(inAbsRootPath, SOURCEPATHDELIMITER.GetAt(0),
                                        SOURCEPATHESCAPECHAR.GetAt(0));

    // Validate the strings
    if (theAbsRelTokenizer.HasNextPartition() && theAbsRootTokenizer.HasNextPartition()) {
        // Bypass all common roots
        while (theAbsRelTokenizer.HasNextPartition() && theAbsRootTokenizer.HasNextPartition()
               && theAbsRelTokenizer.GetCurrentPartition()
                   == theAbsRootTokenizer.GetCurrentPartition()) {
            ++theAbsRelTokenizer;
            ++theAbsRootTokenizer;
        }

        // Grab the remaining path from the relative item
        Q3DStudio::CString theRelPathRemaining;
        while (theAbsRelTokenizer.HasNextPartition()) {
            theRelPathRemaining += SOURCEPATHDELIMITER;
            theRelPathRemaining +=
                CPathConstructionHelper::EscapeAssetName(theAbsRelTokenizer.GetCurrentPartition());
            ++theAbsRelTokenizer;
        }

        // Add the appropriate number of "parents" and a "this"
        while (theAbsRootTokenizer.HasNextPartition()) {
            if (theRelPath.Length() > 0)
                theRelPath += SOURCEPATHDELIMITER;
            theRelPath += SOURCEPATHPARENT;
            ++theAbsRootTokenizer;
        }

        // Append the remaining path, if neccessary
        if (theRelPathRemaining.Length()) {
            theRelPath += theRelPathRemaining;
        }
    }

    if (theRelPath.Length() == 0)
        theRelPath = SOURCEPATHTHIS;
    else if (theRelPath[(long)0] == '.') {
        Q3DStudio::CString theTempString = theRelPath;
        theRelPath = SOURCEPATHTHIS;
        theRelPath += theTempString;
    }

    return theRelPath;
}

//=============================================================================
/**
 *	Format the string correctly by appending the appropriate escape character
 */
Q3DStudio::CString CPathConstructionHelper::EscapeAssetName(Q3DStudio::CString inAssetName)
{
    static Q3DStudio::CString theEscapedEscape(SOURCEPATHESCAPECHAR + SOURCEPATHESCAPECHAR);
    inAssetName.Replace(SOURCEPATHESCAPECHAR, theEscapedEscape); // move these to const strings

    static Q3DStudio::CString theEscapedDelimiter(SOURCEPATHESCAPECHAR + SOURCEPATHDELIMITER);
    inAssetName.Replace(SOURCEPATHDELIMITER, theEscapedDelimiter);
    return inAssetName;
}

//=============================================================================
/**
 *	Return the string value that specifies "this"
 */
const Q3DStudio::CString &CPathConstructionHelper::GetThisString()
{
    return SOURCEPATHTHIS;
}

//=============================================================================
/**
 *	Return the string value that specifies the escape character
 */
const Q3DStudio::CString &CPathConstructionHelper::GetEscapeChar()
{
    return SOURCEPATHESCAPECHAR;
}

//=============================================================================
/**
 *	Return the string value that specifies "Scene"
 */
const Q3DStudio::CString &CPathConstructionHelper::GetSceneString()
{
    return SOURCEPATHSCENE;
}

//=============================================================================
/**
 *	Return the string value that specifies "parent"
 */
const Q3DStudio::CString &CPathConstructionHelper::GetParentString()
{
    return SOURCEPATHPARENT;
}

//=============================================================================
/**
 *	Return the string value that specifies teh delimiter
 */
const Q3DStudio::CString &CPathConstructionHelper::GetPathDelimiter()
{
    return SOURCEPATHDELIMITER;
}