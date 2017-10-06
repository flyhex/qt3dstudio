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

#if !defined(AFX_REPORTDLG_H__B8564DEF_2599_4419_9E15_4AF38BE7066C__INCLUDED_)
#define AFX_REPORTDLG_H__B8564DEF_2599_4419_9E15_4AF38BE7066C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReportDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReportDlg dialog

class CReportDlg : public CDialog
{
    // Construction
public:
    CReportDlg(CWnd *pParent = nullptr); // standard constructor

    COLORREF m_Color_Background;
    COLORREF m_Color_Text;
    COLORREF m_Color_Gray;
    COLORREF m_Color_Dark;
    CBrush m_Brush;
    CFont m_MediumFont2;

    // Dialog Data
    //{{AFX_DATA(CReportDlg)
    enum { IDD = IDD_REPORTDLG };
    CButton m_SendSysInfo;
    CEdit m_Description;
    CString m_Info;
    CString m_DescHeader;
    CString m_EmailAddress;
    CString m_EmailHeader;
    //}}AFX_DATA

    void SetErrorMessage(CString inErrorMessage);
    void SetStackTrace(CString inStackTrace);

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CReportDlg)
protected:
    virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CReportDlg)
    afx_msg void OnSubmit();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CString m_ErrorMessage;
    CString m_StackTrace;

public:
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    CString m_SysInfoText;
    //	afx_msg void OnStnClickedReportdlg();
    afx_msg void OnStnClickedReportdlg();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPORTDLG_H__B8564DEF_2599_4419_9E15_4AF38BE7066C__INCLUDED_)
