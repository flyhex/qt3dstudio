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

#ifdef WIN32
#include "DLLVersion.h"
#endif
#include "ProductInstance.h"

CProductInstance::CProductInstance(std::string inName)
{
    m_Name = inName;
    m_TabCount = 0;
}

CProductInstance::~CProductInstance()
{
    // Kill the List of Products.
}

void CProductInstance::AddInstance(CProductInstance *inChild)
{
    m_ChildInstances.push_back(inChild);
}

#ifdef WIN32
void CProductInstance::AddProduct(const char *inModulePath)
{

    try {
        CDLLVersion theVersion(inModulePath);
        CProductInfo *theProduct = new CProductInfo(
            inModulePath, theVersion.GetMajorVersion(), theVersion.GetMinorVersion(),
            theVersion.GetIterationNumber(), theVersion.GetBuildNumber());
        m_Modules.push_back(theProduct);
    } catch (...) {
    }
}
#endif

void CProductInstance::AddProduct(const char *inModulePath, long inMajorVersion,
                                  long inMinorVersion, long inIteration, long inBuild)
{

    try {
        CProductInfo *theProduct =
            new CProductInfo(inModulePath, inMajorVersion, inMinorVersion, inIteration, inBuild);
        m_Modules.push_back(theProduct);
    } catch (...) {
    }
}

#ifdef WIN32
void CProductInstance::AddProduct(Q3DStudio::CId inID)
{

    try {
        CProductInfo *theProduct = new CProductInfo(inID);
        m_Modules.push_back(theProduct);
    } catch (...) {
    }
}
#endif

/*std::ostream& operator<<( std::ostream& inStream, CProductInstance* inProductInstance )
{

        unsigned long theCount = inProductInstance->m_Modules.size();
        unsigned long theIndex;

        for ( theIndex = 0; theIndex < inProductInstance->m_TabCount; ++theIndex )
                inStream << "\t";


        inStream << "<productInstance name=\"" << inProductInstance->m_Name.data() << "\">" <<
std::endl;

        for ( theIndex = 0; theIndex < theCount; theIndex++)
        {
                inProductInstance->m_Modules[theIndex]->SetTabCount(inProductInstance->m_TabCount +
1);
                inStream << (inProductInstance->m_Modules[theIndex]);
        }

        for ( theIndex = 0; theIndex < inProductInstance->m_TabCount; ++theIndex )
                inStream << "\t";

        inStream << "</productInstance >" << std::endl;

        return inStream;
}*/
