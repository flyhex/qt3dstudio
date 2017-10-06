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

// ReportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Strings.h"

#include "ReportDlg.h"
#include "CrashInfo.h"

#include "CompConfig.h"

#include "StudioInstance.h"
#include <strstream>

extern char g_DumpPath[MAX_PATH];
/////////////////////////////////////////////////////////////////////////////
// CReportDlg dialog

CReportDlg::CReportDlg(CWnd *pParent /*=nullptr*/)
    : CDialog(CReportDlg::IDD, pParent)
    , m_SysInfoText(_T(""))
{
    //{{AFX_DATA_INIT(CReportDlg)
    m_Info = _T("");
    m_DescHeader = _T("");
    m_EmailAddress = _T("");
    m_EmailHeader = _T("");
    //}}AFX_DATA_INIT

    m_MediumFont2.CreatePointFont(88, CString(CStudioPreferences::GetFontFaceName()));

    m_Color_Background = CStudioPreferences::GetDarkBaseColor();
    m_Color_Text = CStudioPreferences::GetMasterColor();
    m_Color_Gray = CStudioPreferences::GetNormalColor();
    m_Color_Dark = CStudioPreferences::GetInactiveColor();
}

void CReportDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CReportDlg)
    DDX_Control(pDX, IDC_REPORTDLG_SYSINFO, m_SendSysInfo);
    DDX_Control(pDX, IDC_REPORTDLG_DESC, m_Description);
    DDX_Text(pDX, IDC_REPORTDLG_INFO, m_Info);
    DDX_Text(pDX, IDC_REPORTDLG_DESCHEADER, m_DescHeader);
    DDX_Text(pDX, IDC_REPORTDLG_EMAILADDR, m_EmailAddress);
    DDX_Text(pDX, IDC_REPORTDLG_EMAILHEADER, m_EmailHeader);
    //}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_CRASHDLG_SYSINFO_TEXT, m_SysInfoText);
}

BEGIN_MESSAGE_MAP(CReportDlg, CDialog)
//{{AFX_MSG_MAP(CReportDlg)
ON_BN_CLICKED(IDSUBMIT, OnSubmit)
//}}AFX_MSG_MAP
ON_WM_CTLCOLOR()
ON_WM_ERASEBKGND()
//	ON_STN_CLICKED(IDC_REPORTDLG__SYSINFO_TEXT, &CReportDlg::OnStnClickedReportdlg)
ON_STN_CLICKED(IDC_REPORTDLG__SYSINFO_TEXT, &CReportDlg::OnStnClickedReportdlg)
END_MESSAGE_MAP()

//==============================================================================
/**
 * Set the description of the crash.
 * @param inErrorMessage the error message.
 */
//==============================================================================
void CReportDlg::SetErrorMessage(CString inErrorMessage)
{
    m_ErrorMessage = inErrorMessage;
}

//==============================================================================
/**
 * Set the stack trace for the crash.
 * @param inStackTrace the stack trace.
 */
//==============================================================================
void CReportDlg::SetStackTrace(CString inStackTrace)
{
    m_StackTrace = inStackTrace;
}

/////////////////////////////////////////////////////////////////////////////
// CReportDlg message handlers

void CReportDlg::OnSubmit()
{
    CWaitCursor theWaitCursor;

    this->UpdateData(TRUE);
	
	// Actual reporting functionality missing
    
	this->EndDialog(TRUE);
    exit(1);
}

void CReportDlg::OnCancel()
{
    this->EndDialog(FALSE);
}

BOOL CReportDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_Info = ::LoadResourceString(IDS_REPORTDLG_INFO).GetMulti();
    m_DescHeader = ::LoadResourceString(IDS_REPORTDLG_DESCHEADER).GetMulti();
    m_EmailHeader = ::LoadResourceString(IDS_REPORTDLG_EMAILHEADER).GetMulti();

    CString theSysInfoText;
    theSysInfoText = ""; //::LoadResourceString( IDS_REPORTDLG_SYSINFOHEADER ).GetMulti( );
    // this is done this way, as i had problem changing the color of the text of the checkbox.
    // it is now a seperate checkbox and a static text
    // Made a workaround where the static text has notify flag on and would toggle the checkbox
    // control on and off
    // when clicked
    m_SysInfoText = ::LoadResourceString(IDS_REPORTDLG_SYSINFOHEADER).GetMulti();

    m_SendSysInfo.SetWindowText(theSysInfoText);
    m_SendSysInfo.SetFont(&m_MediumFont2);
    m_SendSysInfo.SetCheck(BST_CHECKED);

    m_Brush.CreateSolidBrush(m_Color_Background);
    GetDlgItem(IDC_REPORTDLG_INFO)->SetFont(&m_MediumFont2);
    GetDlgItem(IDC_REPORTDLG_DESCHEADER)->SetFont(&m_MediumFont2);
    GetDlgItem(IDC_REPORTDLG_EMAILADDR)->SetFont(&m_MediumFont2);
    GetDlgItem(IDC_REPORTDLG_EMAILHEADER)->SetFont(&m_MediumFont2);
    GetDlgItem(IDC_REPORTDLG__SYSINFO_TEXT)->SetFont(&m_MediumFont2);

    this->UpdateData(FALSE);

    return TRUE;
}

HBRUSH CReportDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
    Q_UNUSED(nCtlColor);

    int theCtrlID = pWnd->GetDlgCtrlID();
    if (theCtrlID == IDSUBMIT || theCtrlID == IDCANCEL || theCtrlID == IDC_REPORTDLG_SEPERATOR
        || theCtrlID == IDC_REPORTDLG_SYSINFO || theCtrlID == IDC_REPORTDLG__SYSINFO_TEXT
        || theCtrlID == IDC_REPORTDLG_INFO || theCtrlID == IDC_REPORTDLG_DESCHEADER
        || theCtrlID == IDC_REPORTDLG_EMAILHEADER) {
        pDC->SetBkMode(TRANSPARENT); // for area just behind the text
        pDC->SetTextColor(m_Color_Gray);
    } else if (theCtrlID == IDC_REPORTDLG_DESC || theCtrlID == IDC_REPORTDLG_EMAILADDR) {
        pDC->SetBkColor(m_Color_Background);
        pDC->SetBkMode(OPAQUE); // for area just behind the text
        pDC->SetTextColor(m_Color_Text);
    }

    return m_Brush;
}

BOOL CReportDlg::OnEraseBkgnd(CDC *pDC)
{
    CRect theClientRect;
    GetClientRect(&theClientRect);
    pDC->FillSolidRect(theClientRect, m_Color_Background);

    return TRUE;
}

void CReportDlg::OnStnClickedReportdlg()
{
    m_SendSysInfo.SetCheck(!m_SendSysInfo.GetCheck());
}
