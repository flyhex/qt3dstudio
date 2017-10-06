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
#include "TestCmdUtils.h"
#include "Doc.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "GraphUtils.h"

//=============================================================================
/**
 * Returns the nearest integer to a float, e.g. 3.4 -> 3; -2.7 -> -3;
 * @param inX a float value
 * @return the nearest integer
 */
// long round( float inX ) { return inX > 0 ? (long)(inX + 0.5) : (long)(inX - 0.5); };

//=============================================================================
/**
 * Uses the command interface to add a child asset to a parent object on a
 * particular slide and sets attributes on the new child object
 * @param inParent the to/destination object, e.g. the scene, a layer
 * @param inChild the from/source asset, e.g. a sphere
 * @param inSlideIndex a slide index where zero is the master slide
 * @return a pointer to the new child object or NULL if not added
 */
UICDM::CUICDMInstanceHandle TestCmdUtilsAddObject(CAsset *inParent, CAsset *inChild,
                                                  long inSlideIndex)
{
    Q_UNUSED(inParent);
    Q_UNUSED(inChild);
    Q_UNUSED(inSlideIndex);
    return {};
    // CCmdAddObject addObjectCmd( inParent->GetDoc( ), inParent->GetInstanceHandle( ),
    // inChild->GetInstanceHandle( ), inSlideIndex );
    // addObjectCmd.Do( );
    // return addObjectCmd.GetAddedInstance( );
}

//==============================================================================
/**
 * Packages up a property's attributes into a string-based fingerprint that can
 * be used for QA comparison
 *		@param inProperty the property in question
 *		@return a string representing the property name and other measurable attributes
 */
//==============================================================================
// TODO: UICDM Fingerprint

//==============================================================================
/**
 *	Flatten a list of strings into a single comma-delimited string
 *		@param inFingerprint a list of strings
  *		@return a single string representing the list of strings
 */
//==============================================================================
Q3DStudio::CString TestCmdUtilsFlatten(const TFingerprint &inFingerprint)
{
    Q3DStudio::CString theString = "(";
    TFingerprint::const_iterator theIter = inFingerprint.begin();
    for (; theIter != inFingerprint.end(); ++theIter)
        theString += *theIter + ",";
    theString += ")";
    return theString;
}

//==============================================================================
/**
 *	Compare two fingerprints for equality. Useful in debug mode
 *  to see which metrics do not match
 *		@param inFingerprint1 the first list of strings
 *		@param inFingerprint2 the second list of strings
  *		@return T if the lists are identical, otherwise F
 */
//==============================================================================
bool TestCmdUtilsCompare(const TFingerprint &inFingerprint1, const TFingerprint &inFingerprint2)
{
    Q3DStudio::CString theString1 = TestCmdUtilsFlatten(inFingerprint1);
    Q3DStudio::CString theString2 = TestCmdUtilsFlatten(inFingerprint2);
    bool theSame = (theString1 == theString2);
    if (!theSame) {
        qDebug("\n");
        qDebug(QString::fromWCharArray(theString1.c_str()).toLatin1().data());
        qDebug("\n");
        qDebug(QString::fromWCharArray(theString2.c_str()).toLatin1().data());
    }
    return theSame;
}
