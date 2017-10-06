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

#ifndef INCLUDED_CRASH_INFO_H
#define INCLUDED_CRASH_INFO_H 1

#pragma once

class CProductInstance;

//==============================================================================
//	Includes
//==============================================================================

#include "CompConfig.h"

class CCrashInfo
{
public:
    CCrashInfo();
    ~CCrashInfo();

    void SetEmailAddress(const Q3DStudio::CString &inEmailAddress);
    void SetDescription(const Q3DStudio::CString &inDescription);
    void DetermineConfig();
    void SetStackTrace(const Q3DStudio::CString &inStackTrace);
    void SetErrorString(const Q3DStudio::CString &inErrorString);
    void SetProductInstance(CProductInstance *inProductInstance);

    Q3DStudio::CString GetXMLifiedInfo();

protected:
    Q3DStudio::CString m_EmailAddress;
    Q3DStudio::CString m_Description;
    Q3DStudio::CString m_StackTrace;
    Q3DStudio::CString m_ErrorString;

    CProductInstance *m_ProductInstance;
    CCompConfig *m_Config;
    bool m_DeterminedConfig;
};

#endif // INCLUDED_CRASH_INFO_H
