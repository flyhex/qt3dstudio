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

#include "stdafx.h"

#include "ProductInfo.h"
#ifdef WIN32
#include "DLLVersion.h"
#endif

CProductInfo::CProductInfo(std::string inName, unsigned long inMajorNumber,
                           unsigned long inMinorNumber, unsigned long inIterationNumber,
                           unsigned long inBuildNumber)
    : m_BuildNumber(inBuildNumber)
    , m_MajorVersion(inMajorNumber)
    , m_MinorVersion(inMinorNumber)
    , m_IterationNumber(inIterationNumber)
    , m_TabCount(0)
{
    m_Path = inName;
    m_Name = NameFromPath(m_Path);
    m_TabCount = 0;
}

#ifdef WIN32
//=============================================================================
/**
 * Create a product info from a GUID.
 * This looks up the GUID and finds the DLL associated with it then gets the
 * DLLVersion for that DLL. The name of the product info is set as the location
 * of the DLL.
 * @param inGUID the GUID to query for.
 */
//=============================================================================
CProductInfo::CProductInfo(Q3DStudio::CId inID)
    : m_BuildNumber(0)
    , m_MajorVersion(0)
    , m_MinorVersion(0)
    , m_IterationNumber(0)
    , m_TabCount(0)
{
    Q3DStudio::CString theIDString(inID.ToString());
    char theModuleLocation[512];
    Q3DStudio::CString theRegKeyString("CLSID\\");
    theRegKeyString += theIDString;
    theRegKeyString += "\\InProcServer32";

    HKEY theRegKey;

    long theRegResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, theRegKeyString, 0, KEY_READ, &theRegKey);
    if (theRegResult == ERROR_SUCCESS) {
        DWORD theKeyType;
        DWORD theModuleSize = sizeof(theModuleLocation);

        // Query the default value
        theRegResult =
            RegQueryValueEx(theRegKey, NULL, NULL, &theKeyType,
                            reinterpret_cast<unsigned char *>(theModuleLocation), &theModuleSize);
        if (theRegResult == ERROR_SUCCESS) {
            CDLLVersion theVersion(theModuleLocation);

            m_Path = theModuleLocation;
            m_Name = NameFromPath(m_Path);

            m_MajorVersion = theVersion.GetMajorVersion();
            m_MinorVersion = theVersion.GetMinorVersion();
            m_IterationNumber = theVersion.GetIterationNumber();
            m_BuildNumber = theVersion.GetBuildNumber();
        }
    }
}

//=============================================================================
/**
 * Constructor from a module string.
 * This will lookup the module, load it and attempt to query the version from
 * it.
 * @param inModulePath the path of the module to query.
 */
CProductInfo::CProductInfo(const char *inModulePath)
    : m_BuildNumber(0)
    , m_MajorVersion(0)
    , m_MinorVersion(0)
    , m_IterationNumber(0)
    , m_TabCount(0)
{
    CDLLVersion theVersion(inModulePath);

    m_Path = inModulePath;
    m_Name = NameFromPath(m_Path);

    m_MajorVersion = theVersion.GetMajorVersion();
    m_MinorVersion = theVersion.GetMinorVersion();
    m_IterationNumber = theVersion.GetIterationNumber();
    m_BuildNumber = theVersion.GetBuildNumber();
}
#endif
