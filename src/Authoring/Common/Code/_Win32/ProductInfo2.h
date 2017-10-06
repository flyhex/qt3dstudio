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
#ifndef INCLUDED_PRODUCT_INFO2_H
#define INCLUDED_PRODUCT_INFO2_H 1

class CProductInfo2
{
public:
    CProductInfo2(std::string inVersion)
    {
        TCHAR theModuleFilename[MAX_PATH];
        ::GetModuleFileName(NULL, theModuleFilename, sizeof(theModuleFilename));

        m_Description = theModuleFilename;

        std::string::size_type thePos = m_Description.rfind("\\");
        m_Description.erase(0, thePos + 1);

        m_Version = inVersion;

#ifdef _WIN32
        m_Platform = "Windows";
#elif _MAC
        m_Platform = "Macintosh";
#endif

        m_TabCount = 0;
    }

    std::string GetDescription() { return m_Description; }

    std::string GetVersion() { return m_Version; }

    std::string GetPlatform() { return m_Platform; }

    void SetTabCount(unsigned long inTabCount) { m_TabCount = inTabCount; }

    friend std::ostream &operator<<(std::ostream &inStream, CProductInfo2 *inProduct)
    {
        for (unsigned long i = 0; i < inProduct->m_TabCount; ++i)
            inStream << "\t";

        inStream << "<Product Description=\"" << inProduct->m_Description.c_str() << "\" Version=\""
                 << inProduct->m_Version.c_str() << "\" Platform=\""
                 << inProduct->m_Platform.c_str() << "\"/>" << std::endl;

        return inStream;
    }

protected:
    std::string m_Description;
    std::string m_Version;
    std::string m_Platform;

    unsigned long m_TabCount;
};
#endif //__PRODUCT_INFO_H2_