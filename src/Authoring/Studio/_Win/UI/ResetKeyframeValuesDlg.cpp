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
#include "ResetKeyframeValuesDlg.h"
#include ".\resetkeyframevaluesdlg.h"

IMPLEMENT_DYNAMIC(CResetKeyframeValuesDlg, CDialog)
CResetKeyframeValuesDlg::CResetKeyframeValuesDlg(CWnd *pParent /*=nullptr*/)
    : CDialog(CResetKeyframeValuesDlg::IDD, pParent)
{
}

CResetKeyframeValuesDlg::~CResetKeyframeValuesDlg()
{
}

void CResetKeyframeValuesDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RESETKEYFRAMEVALUES_DLG_ICON, m_WarningIcon);
    DDX_Control(pDX, IDCANCEL, m_CancelButton);
}

BEGIN_MESSAGE_MAP(CResetKeyframeValuesDlg, CDialog)
END_MESSAGE_MAP()

BOOL CResetKeyframeValuesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_WarningIcon.SetIcon(::AfxGetApp()->LoadStandardIcon(IDI_WARNING));
    m_CancelButton.SetButtonStyle(BS_DEFPUSHBUTTON);

    return TRUE; // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
