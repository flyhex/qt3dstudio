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
//	Prefix
//==============================================================================

#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================

#ifndef INCLUDED_CRASH_INFO_H
#include "CrashInfo.h"
#endif
//#include "StudioInstance.h"
#include "ProductInstance.h"

#include <strstream>

#include "Win32Config.h"

//=============================================================================
/**
 * Constructor
 */
//=============================================================================
CCrashInfo::CCrashInfo()
    : m_ProductInstance(NULL)
{
    //	m_ProductInstance = CStudioInstance::GenerateProductInstance();
    m_Config = new CWin32Config;

    m_DeterminedConfig = false;
}

CCrashInfo::~CCrashInfo()
{
    delete m_Config;
    delete m_ProductInstance;
}
//=============================================================================
/**
 * Setter for the product instance
 * @param inProductInstance the product instance for this crash report
 */
void CCrashInfo::SetProductInstance(CProductInstance *inProductInstance)
{
    m_ProductInstance = inProductInstance;
}

//=============================================================================
/**
 * @param inEmailAddress the email address for the victim.
 */
//=============================================================================
void CCrashInfo::SetEmailAddress(const Q3DStudio::CString &inEmailAddress)
{
    m_EmailAddress = inEmailAddress;
}

//=============================================================================
/**
 * @param inDescription the description of the crash.
 */
//=============================================================================
void CCrashInfo::SetDescription(const Q3DStudio::CString &inDescription)
{
    m_Description = inDescription;
}

//=============================================================================
/**
 * @param inErrorString one line info on what the crash was.
 */
//=============================================================================
void CCrashInfo::SetErrorString(const Q3DStudio::CString &inErrorString)
{
    m_ErrorString = inErrorString;
}

//=============================================================================
/**
 * Call this if the configuration is to be set, do not call it without the
 * user's explicit permision.
 */
//=============================================================================
void CCrashInfo::DetermineConfig()
{
    m_Config->DetermineConfig();
    m_DeterminedConfig = true;
}

//=============================================================================
/**
 * @param the stack trace of the crash.
 */
//=============================================================================
void CCrashInfo::SetStackTrace(const Q3DStudio::CString &inStackTrace)
{
    m_StackTrace = inStackTrace;
}

//=============================================================================
/**
 * Get all this info in XML format.
 */
//=============================================================================
Q3DStudio::CString CCrashInfo::GetXMLifiedInfo()
{
    Q3DStudio::CString theInfo = "<crashreport>\n";

    Q3DStudio::CString theEmailAddress;
    theEmailAddress.Format(_LSTR("\t<contact emailAddress=\"%ls\"/>\n"),
                           static_cast<const wchar_t *>(m_EmailAddress));

    Q3DStudio::CString theDescription;
    theDescription.Format(
        _LSTR("\t<description errorString=\"%ls\">\n\t\t<![CDATA[\n%ls\n\t\t]]>\n\t</description>\n"),
        static_cast<const wchar_t *>(m_ErrorString), static_cast<const wchar_t *>(m_Description));

    Q3DStudio::CString theProductInstance;
    if (m_ProductInstance) {
        std::ostrstream theProductStream;
        m_ProductInstance->SetTabCount(1);

        theProductStream << m_ProductInstance;
        char *theProductString = theProductStream.str();
        theProductString[theProductStream.pcount()] = 0;
        theProductInstance = theProductString;
        theProductInstance += "\n";
    }

    Q3DStudio::CString theConfiguration;
    if (m_DeterminedConfig) {
        std::ostrstream theStream;
        m_Config->SetTabCount(1);
        m_Config->WriteXML(theStream); // new way
        // theStream << &m_Config;					//old way
        char *theConfigString = theStream.str();
        theConfigString[theStream.pcount()] = 0;
        theConfiguration = theConfigString;
        theConfiguration += "\n";
    }

    Q3DStudio::CString theStackTrace;
    theStackTrace.Format(_LSTR("\t<stacktrace>\n\t\t<![CDATA[\n%ls\n\t\t]]>\n\t</stacktrace>\n"),
                         static_cast<const wchar_t *>(m_StackTrace));

    theInfo += theEmailAddress;
    theInfo += theDescription;
    theInfo += theProductInstance;
    theInfo += theConfiguration;
    theInfo += theStackTrace;
    theInfo += "</crashreport>";

    return theInfo;
}
