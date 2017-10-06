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

#include "stdafx.h"
#include "StudioInstance.h"
#include "StudioPreferences.h"

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

CProductInstance *CStudioInstance::GenerateProductInstance()
{
    CProductInstance *theRetVal = new CProductInstance("UIComposer Studio");
    //	CProductInstance* theClient = new CProductInstance( "Client" );

    //	theRetVal->AddProduct( "AKPluWAV.dll" );
    //	theRetVal->AddProduct( "AKPluIMG.dll" );
    //	theRetVal->AddProduct( "AKPlu3DS.dll" );
    // This section will soon be a call into the client dll
    /*
            theClient->AddProduct( "AKCore.dll" );
            theClient->AddProduct( "AKCorENU.dll" );
            theClient->AddProduct( "AKProENU.dll" );
            theRetVal->AddInstance( theClient );
    */

    long theMajorVersion = 0;
    long theMinorVersion = 0;
    long theIterationNumber = 0;
    long theBuildNumber = 0;
    Q3DStudio::CString theUnparsedNumber;
    Q3DStudio::CString theStudioVersion = CStudioPreferences::GetVersionString();

    int theIndex = theStudioVersion.Find('.');
    if (theIndex != Q3DStudio::CString::ENDOFSTRING) {
        theUnparsedNumber = theStudioVersion.Extract(0, theIndex);
        theMajorVersion = atoi(theUnparsedNumber.GetCharStar());
        theStudioVersion = theStudioVersion.Extract(theIndex + 1);
    }

    theIndex = theStudioVersion.Find('.');
    if (theIndex != Q3DStudio::CString::ENDOFSTRING) {
        theUnparsedNumber = theStudioVersion.Extract(0, theIndex);
        theMinorVersion = atoi(theUnparsedNumber.GetCharStar());
        theStudioVersion = theStudioVersion.Extract(theIndex + 1);
    }

    theIndex = theStudioVersion.Find('.');
    if (theIndex != Q3DStudio::CString::ENDOFSTRING) {
        theUnparsedNumber = theStudioVersion.Extract(0, theIndex);
        theIterationNumber = atoi(theUnparsedNumber.GetCharStar());
        theStudioVersion = theStudioVersion.Extract(theIndex + 1);
    }

    theBuildNumber = atoi(theStudioVersion.GetCharStar());

    theRetVal->AddProduct("Studio.exe", theMajorVersion, theMinorVersion, theIterationNumber,
                          theBuildNumber);
    return theRetVal;
}
