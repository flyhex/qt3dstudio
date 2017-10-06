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

#ifndef INCLUDED_PRODUCT_INSTANCE_H
#define INCLUDED_PRODUCT_INSTANCE_H 1

#ifndef INCLUDED_PRODUCT_INFO_H
#include "ProductInfo.h"
#endif

#include <string>
#include <iostream>
#include <vector>

class CProductInstance
{
    typedef std::vector<CProductInfo *> TProductInfo;
    typedef std::vector<CProductInstance *> TProductInstance;

protected:
    TProductInfo m_Modules;
    TProductInstance m_ChildInstances;
    std::string m_Name;
    unsigned long m_TabCount;

public:
    CProductInstance(std::string inName);
    ~CProductInstance();
    void AddInstance(CProductInstance *inChild);
    void AddProduct(const char *inModulePath, long inMajorVersion, long inMinorVersion,
                    long inIteration, long inBuild);

#ifdef WIN32
    void AddProduct(Q3DStudio::CId inID);
    void AddProduct(const char *inModulePath);
#endif

    void SetTabCount(unsigned long inTabCount) { m_TabCount = inTabCount; }

    friend std::ostream &operator<<(std::ostream &inStream, CProductInstance *inProductInstance)
    {

        size_t theCount = inProductInstance->m_Modules.size();
        size_t theIndex;

        for (theIndex = 0; theIndex < inProductInstance->m_TabCount; ++theIndex)
            inStream << "\t";

        inStream << "<Instance name=\"" << inProductInstance->m_Name.data() << "\" ";
#ifdef _DEBUG
        inStream << "instanceType=\"Debug\">" << std::endl;
#else
        inStream << "instanceType=\"Release\">" << std::endl;
#endif

        for (theIndex = 0; theIndex < theCount; theIndex++) {
            inProductInstance->m_Modules[theIndex]->SetTabCount(inProductInstance->m_TabCount + 1);
            inStream << (inProductInstance->m_Modules[theIndex]);
        }

        for (theIndex = 0; theIndex < inProductInstance->m_TabCount; ++theIndex)
            inStream << "\t";

        for (theIndex = 0; theIndex < inProductInstance->m_ChildInstances.size(); theIndex++) {
            inProductInstance->m_ChildInstances[theIndex]->SetTabCount(inProductInstance->m_TabCount
                                                                       + 1);
            inStream << inProductInstance->m_ChildInstances[theIndex];
        }
        inStream << "</Instance >" << std::endl;

        return inStream;
    };
};
#endif // INCLUDED_PRODUCT_INSTANCE_H