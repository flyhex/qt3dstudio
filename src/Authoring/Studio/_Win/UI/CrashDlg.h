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

#ifndef INCLUDED_CRASH_DIALOG_H
#define INCLUDED_CRASH_DIALOG_H
#include "afxwin.h"
#include <string.h>

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCrashDlg dialog

class CCrashDlg : public CDialog
{
    // Construction
public:
    CCrashDlg(CWnd *pParent = nullptr); // standard constructor

    // Dialog Data
    //{{AFX_DATA(CCrashDlg)
    enum { IDD = IDD_CRASHDLG };
    CStatic m_CrashIcon;
    CString m_Header;
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCrashDlg)
protected:
    virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
public:
    void SetErrorMessage(CString inErrorMessage);
    void SetFilename(CString inFilename);
    void SetStackTrace(CString inStackTrace);
    void SplitLongPath(int controlId, CString &text);

protected:
    CString m_ErrorMessage;
    CString m_FilenameSaved;
    CString m_StackTrace;

    COLORREF m_Color_Background;
    COLORREF m_Color_Text;
    COLORREF m_Color_Gray;
    COLORREF m_Color_Dark;
    CBrush m_Brush;
    CFont m_MediumFont2;

    // Generated message map functions
    //{{AFX_MSG(CCrashDlg)
    afx_msg void OnSave();
    afx_msg void OnExit();
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
    CString m_Filename;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // INCLUDED_CRASH_DIALOG_H
