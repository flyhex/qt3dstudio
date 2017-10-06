/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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

#ifndef INCLUDED_GL_VERSION_DLG
#define INCLUDED_GL_VERSION_DLG 1

#pragma once

//==============================================================================
//	 Includes
//==============================================================================
#include "resource.h"

//==============================================================================
/**
 *	CGLVersionDlg: Dialog class for showing Open GL Version Warning
 */
//==============================================================================
class CGLVersionDlg : public CDialog
{
public:
    CGLVersionDlg(CWnd *pParent = nullptr); // standard constructor
    virtual ~CGLVersionDlg();

    // Dialog Data
    enum { IDD = IDD_GL_VERSION_DLG };

protected:
    virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV support

    DECLARE_MESSAGE_MAP()
    CStatic m_WarningIcon; // Warning icon
    CString m_Title; // Title for the dialog
    CString m_Message; // Warning message
    BOOL m_DontShowAgain; // Set to true to "Don't show this dialog again"
    LPCTSTR m_Icon; // Which icon to load

public:
    virtual BOOL OnInitDialog();
    void Initialize(const Q3DStudio::CString &inTitle, const Q3DStudio::CString &inMessage,
                    bool inErrorIcon);
    BOOL GetDontShowAgain();
};

#endif // INCLUDED_GL_VERSION_DLG
