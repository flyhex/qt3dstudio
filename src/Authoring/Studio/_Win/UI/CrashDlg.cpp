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
#include "Strings.h"

//==============================================================================
//	Includes
//==============================================================================
#include "MainFrm.h"
#include "CrashDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CCrashDlg dialog

CCrashDlg::CCrashDlg(CWnd *pParent /*=nullptr*/)
    : CDialog(CCrashDlg::IDD, pParent)
    , m_Filename(_T(""))
{
    //{{AFX_DATA_INIT(CCrashDlg)
    m_Header = _T("");
    //}}AFX_DATA_INIT

    m_MediumFont2.CreatePointFont(88, CString(CStudioPreferences::GetFontFaceName()));

    m_Color_Background = CStudioPreferences::GetDarkBaseColor();
    m_Color_Text = CStudioPreferences::GetMasterColor();
    m_Color_Gray = CStudioPreferences::GetNormalColor();
    m_Color_Dark = CStudioPreferences::GetInactiveColor();
}

void CCrashDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCrashDlg)
    DDX_Control(pDX, IDC_CRASHICON, m_CrashIcon);
    DDX_Text(pDX, IDC_HEADER, m_Header);
    DDX_Text(pDX, IDC_HEADER2, m_FilenameSaved);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCrashDlg, CDialog)
//{{AFX_MSG_MAP(CCrashDlg)
ON_BN_CLICKED(IDEXIT, OnExit)
//}}AFX_MSG_MAP
//	ON_STN_CLICKED(IDC_HEADER, &CCrashDlg::OnStnClickedHeader)
ON_WM_ERASEBKGND()
ON_WM_CTLCOLOR()
ON_WM_DESTROY()
END_MESSAGE_MAP()

//==============================================================================
/**
 * Set the description of the crash.
 * @param inErrorMessage the error message.
 */
//==============================================================================
void CCrashDlg::SetErrorMessage(CString inErrorMessage)
{
    m_ErrorMessage = inErrorMessage;
}

//==============================================================================
/**
 * Set the project filename saved
 * @param inFilename the filename of the crashed project
 */
//==============================================================================
void CCrashDlg::SetFilename(CString inFilename)
{
    m_Filename = inFilename;
}

//==============================================================================
/**
 * Set the stack trace for the crash.
 * @param inStackTrace the stack trace.
 */
//==============================================================================
void CCrashDlg::SetStackTrace(CString inStackTrace)
{
    m_StackTrace = inStackTrace;
}

/////////////////////////////////////////////////////////////////////////////
// CCrashDlg message handlers

void CCrashDlg::OnExit()
{
    this->EndDialog(0);
    _exit(EXIT_FAILURE);
}

void CCrashDlg::SplitLongPath(int controlId, CString &text)
{

    // Get the device context of your control.
    CDC *dc = GetDlgItem(controlId)->GetDC();
    CRect filenameTextRect;
    GetDlgItem(controlId)->GetClientRect(filenameTextRect);

    HGDIOBJ hOldFont = dc->SelectObject(&m_MediumFont2);

    CRect tmpStringRect(0, 0, 0, 0);
    CString workingString = text;

    CString tmpString;
    std::vector<int> insertPosition;
    int previousOffset = 0;

    for (int i = 0; i < workingString.GetLength(); ++i) {
        tmpString = workingString.Mid(previousOffset, i - previousOffset);
        dc->DrawText(tmpString, &tmpStringRect, DT_CALCRECT);
        if (tmpStringRect.Width() + 5 > filenameTextRect.Width()) {
            previousOffset = i;
            insertPosition.push_back(i - 1);
        }
    }

    // adds in all the \n
    for (int i = (int)insertPosition.size() - 1; i >= 0; --i) {
        text.Insert(insertPosition[i], '\n');
    }

    dc->SelectObject(hOldFont);
}

BOOL CCrashDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_Brush.CreateSolidBrush(m_Color_Background);
    GetDlgItem(IDC_HEADER)->SetFont(&m_MediumFont2);
    GetDlgItem(IDC_HEADER2)->SetFont(&m_MediumFont2);

    // m_Header = ::LoadResourceString( IDS_ERROR_MSGPASSING ).GetMulti( );
    m_Header = m_ErrorMessage;
    m_FilenameSaved = m_Filename;

    CString theTitle = ::LoadResourceString(IDS_ERROR_MSGTITLE);
    this->SetWindowText(theTitle);

    // Show the exclamation point icon
    m_CrashIcon.SetIcon(::LoadIcon(nullptr, IDI_ERROR));

    SplitLongPath(IDC_HEADER2, m_FilenameSaved);
    SplitLongPath(IDC_HEADER, m_Header);

    this->UpdateData(FALSE);

    return TRUE;
}

BOOL CCrashDlg::OnEraseBkgnd(CDC *pDC)
{
    CRect theClientRect;
    GetClientRect(&theClientRect);
    pDC->FillSolidRect(theClientRect, m_Color_Background);

    return TRUE;
}

HBRUSH CCrashDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT)
{
    int theCtrlID = pWnd->GetDlgCtrlID();
    if (theCtrlID == IDREPORT || theCtrlID == IDEXIT || theCtrlID == IDC_LINE_CRASHDLG) {
        pDC->SetBkMode(TRANSPARENT); // for area just behind the text
        pDC->SetTextColor(m_Color_Text);
    } else if (theCtrlID == IDC_HEADER || theCtrlID == IDC_HEADER2) {
        pDC->SetBkMode(TRANSPARENT); // for area just behind the text
        pDC->SetTextColor(m_Color_Gray);
    }

    return m_Brush;
}

void CCrashDlg::OnDestroy()
{
    CDialog::OnDestroy();

    m_Brush.DeleteObject();
}
