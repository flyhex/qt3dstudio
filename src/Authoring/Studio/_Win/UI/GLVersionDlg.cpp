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
//	Includes
//==============================================================================
#include "stdafx.h"
#include "GLVersionDlg.h"

CGLVersionDlg::CGLVersionDlg(CWnd *pParent /*=nullptr*/)
    : CDialog(CGLVersionDlg::IDD, pParent)
{
    m_Title = _T("");
    m_Message = _T("");
    m_Icon = nullptr;
    m_DontShowAgain = FALSE;
}

CGLVersionDlg::~CGLVersionDlg()
{
}

void CGLVersionDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_GL_VERSION_ICON, m_WarningIcon);
    DDX_Text(pDX, IDC_GL_VERSION_MESSAGE, m_Message);
    DDX_Check(pDX, IDC_CHECK1, m_DontShowAgain);
}

BEGIN_MESSAGE_MAP(CGLVersionDlg, CDialog)
END_MESSAGE_MAP()

BOOL CGLVersionDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the title
    this->SetWindowText(m_Title);

    // Set icon
    m_WarningIcon.SetIcon(::AfxGetApp()->LoadStandardIcon(m_Icon));

    return TRUE; // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CGLVersionDlg::Initialize(const Q3DStudio::CString &inTitle,
                               const Q3DStudio::CString &inMessage, bool inErrorIcon)
{
    // Set title and message
    m_Title = CString(inTitle);
    m_Message = CString(inMessage);

    // Set which icon to load
    if (inErrorIcon)
        m_Icon = IDI_ERROR;
    else
        m_Icon = IDI_WARNING;
}

BOOL CGLVersionDlg::GetDontShowAgain()
{
    return m_DontShowAgain;
}
