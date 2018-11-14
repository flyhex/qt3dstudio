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

#include "Qt3DSCommonPrecompile.h"
#include "PathConstructionHelper.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "StackTokenizer.h"

//==============================================================================
////	 Constants
//==============================================================================
static const QString SOURCEPATHDELIMITER = QStringLiteral("."); // can only be single char!
static const QString SOURCEPATHESCAPECHAR = QStringLiteral("\\"); // can only be single char!
static const QString SOURCEPATHSCENE = QStringLiteral("Scene");
static const QString SOURCEPATHPARENT = QStringLiteral("parent");
static const QString SOURCEPATHTHIS = QStringLiteral("this");

//=============================================================================
/**
 *	Create a absolute path reference string
 */
QString
CPathConstructionHelper::BuildAbsoluteReferenceString(CDoc *inDoc,
                                                      qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    CClientDataModelBridge *theBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();
    QString theNameStart;
    QString theNameEnd = EscapeAssetName(theBridge->GetName(inInstance));

    qt3dsdm::Qt3DSDMInstanceHandle theParentInstance = theBridge->GetParentInstance(inInstance);
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
QString CPathConstructionHelper::BuildRelativeReferenceString(
    CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::Qt3DSDMInstanceHandle inRootInstance)
{
    QString theAbsRelPath = BuildAbsoluteReferenceString(inDoc, inInstance);
    QString theAbsRootPath = BuildAbsoluteReferenceString(inDoc, inRootInstance);

    return CPathConstructionHelper::BuildRelativeReferenceString(theAbsRelPath, theAbsRootPath);
}

//=============================================================================
/**
 *  helper method to create the relative path reference string based on its
 *  absolute path string and the root object absolute path string
 */
QString
CPathConstructionHelper::BuildRelativeReferenceString(const QString &inAbsObjPath,
                                                      const QString &inAbsRootPath)
{
    QString theRelPath; // SOURCEPATHTHIS;
    CStackTokenizer theAbsRelTokenizer(inAbsObjPath, SOURCEPATHDELIMITER.at(0),
                                       SOURCEPATHESCAPECHAR.at(0));
    CStackTokenizer theAbsRootTokenizer(inAbsRootPath, SOURCEPATHDELIMITER.at(0),
                                        SOURCEPATHESCAPECHAR.at(0));

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
        QString theRelPathRemaining;
        while (theAbsRelTokenizer.HasNextPartition()) {
            theRelPathRemaining += SOURCEPATHDELIMITER;
            theRelPathRemaining +=
                CPathConstructionHelper::EscapeAssetName(theAbsRelTokenizer.GetCurrentPartition());
            ++theAbsRelTokenizer;
        }

        // Add the appropriate number of "parents" and a "this"
        while (theAbsRootTokenizer.HasNextPartition()) {
            if (!theRelPath.isEmpty())
                theRelPath += SOURCEPATHDELIMITER;
            theRelPath += SOURCEPATHPARENT;
            ++theAbsRootTokenizer;
        }

        // Append the remaining path, if neccessary
        if (!theRelPathRemaining.isEmpty())
            theRelPath += theRelPathRemaining;
    }

    if (theRelPath.isEmpty())
        theRelPath = SOURCEPATHTHIS;
    else if (theRelPath.startsWith(QLatin1Char('.')))
        theRelPath = SOURCEPATHTHIS + theRelPath;

    return theRelPath;
}

//=============================================================================
/**
 *	Format the string correctly by appending the appropriate escape character
 */
QString CPathConstructionHelper::EscapeAssetName(const QString &inAssetName)
{
    static QString theEscapedEscape(SOURCEPATHESCAPECHAR + SOURCEPATHESCAPECHAR);
    QString asset = inAssetName;
    asset.replace(SOURCEPATHESCAPECHAR, theEscapedEscape); // move these to const strings

    static QString theEscapedDelimiter(SOURCEPATHESCAPECHAR + SOURCEPATHDELIMITER);
    asset.replace(SOURCEPATHDELIMITER, theEscapedDelimiter);
    return asset;
}

//=============================================================================
/**
 *	Return the string value that specifies "this"
 */
const QString &CPathConstructionHelper::GetThisString()
{
    return SOURCEPATHTHIS;
}

//=============================================================================
/**
 *	Return the string value that specifies the escape character
 */
const QString &CPathConstructionHelper::GetEscapeChar()
{
    return SOURCEPATHESCAPECHAR;
}

//=============================================================================
/**
 *	Return the string value that specifies "Scene"
 */
const QString &CPathConstructionHelper::GetSceneString()
{
    return SOURCEPATHSCENE;
}

//=============================================================================
/**
 *	Return the string value that specifies "parent"
 */
const QString &CPathConstructionHelper::GetParentString()
{
    return SOURCEPATHPARENT;
}

//=============================================================================
/**
 *	Return the string value that specifies teh delimiter
 */
const QString &CPathConstructionHelper::GetPathDelimiter()
{
    return SOURCEPATHDELIMITER;
}
