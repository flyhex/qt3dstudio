/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#ifndef INCLUDED_PRODUCT_INFO_H
#define INCLUDED_PRODUCT_INFO_H 1

#include <string>
#include <ostream>
#include "Qt3DSId.h"

class CProductInfo
{
public:
    CProductInfo(std::string inName, unsigned long inMajorNumber, unsigned long inMinorNumber,
                 unsigned long inIterationNumber, unsigned long inBuildNumber);

#ifdef WIN32
    CProductInfo(Q3DStudio::CId inID);
    CProductInfo(const char *inModulePath);
#endif

    std::string GetName() { return m_Name; }

    void SetTabCount(unsigned long inTabCount) { m_TabCount = inTabCount; }

    unsigned long GetMajorVersion() { return m_MajorVersion; }

    unsigned long GetMinorVersion() { return m_MinorVersion; }

    unsigned long GetBuildNumber() { return m_BuildNumber; }

    unsigned long GetIterationNumber() { return m_IterationNumber; }

    friend std::ostream &operator<<(std::ostream &inStream, CProductInfo *inProduct)
    {
        for (unsigned long i = 0; i < inProduct->m_TabCount; ++i)
            inStream << std::string("\t");

        char theMajVer[10];
        sprintf(theMajVer, "%lu", inProduct->m_MajorVersion);
        char theMinVer[10];
        sprintf(theMinVer, "%lu", inProduct->m_MinorVersion);
        char theIterNum[10];
        sprintf(theIterNum, "%lu", inProduct->m_IterationNumber);
        char theBuildNum[10];
        sprintf(theBuildNum, "%lu", inProduct->m_BuildNumber);

        inStream << std::string("<Module moduleName=\"") << inProduct->m_Name;
        inStream << std::string("\" moduleVersion=\"");
        // put this next line back in if we ever have a module version
        inStream /*<< std::string( "." )*/ << std::string(theMajVer);
        inStream << std::string(".") << std::string(theMinVer);
        inStream << std::string(".") << std::string(theIterNum);
        inStream << std::string(".") << std::string(theBuildNum);
        inStream << std::string("\"/>") << std::endl;
        return inStream;
    }

    std::string NameFromPath(std::string &inPath)
    {
#ifdef WIN32
        char theFileName[_MAX_FNAME];
        char theExt[_MAX_EXT];
        char theFullFileName[_MAX_PATH];

        // Remove the file name and extension
        _splitpath(inPath.c_str(), NULL, NULL, theFileName, theExt);
        _makepath(theFullFileName, NULL, NULL, theFileName, theExt);

        return std::string(theFullFileName);
#else
        return inPath;
#endif
    }

protected:
    std::string m_Name;
    std::string m_Path;
    unsigned long m_BuildNumber;
    unsigned long m_MajorVersion;
    unsigned long m_MinorVersion;
    unsigned long m_IterationNumber;

    unsigned long m_TabCount;
};
#endif //__PRODUCT_INFO_H_
