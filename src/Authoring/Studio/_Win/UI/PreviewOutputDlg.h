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
#ifndef INCLUDED_PREVIEW_OUTPUT_DIALOG_H
#define INCLUDED_PREVIEW_OUTPUT_DIALOG_H 1

//==============================================================================
//	 Includes
//==============================================================================
#include "resource.h"
#include "PreviewHelper.h"

//==============================================================================
//	Forwards
//==============================================================================
namespace Q3DStudio {
class CFilePath;
}

class CPreviewOutputDlg : public CDialog
{
    DECLARE_DYNAMIC(CPreviewOutputDlg)

protected:
    struct SThreadParam
    {
        void *m_Context;
        HANDLE m_ReadHandle;
    };

public:
    CPreviewOutputDlg(const Q3DStudio::CString &inCmd, const Q3DStudio::CString &inExportedFile,
                      CPreviewHelper::EExecMode inViewMode);
    virtual ~CPreviewOutputDlg();

    // Dialog Data
    enum { IDD = IDD_DIALOG_PREVIEW_STATUS };
    CListCtrl m_ListCtrl;

protected:
    virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnCopyText();
    virtual void OnOK();
    DECLARE_MESSAGE_MAP()

protected:
    BOOL ExecutePreview();
    BOOL LaunchPreviewProcess(HANDLE inStdOutput, Q3DStudio::CString &inCmd);
    static DWORD WINAPI ReadOutputThread(LPVOID lpvThreadParam);
    void DisplayOutput(CString inStr);
    void PreviewProcessTerminated();

protected:
    Q3DStudio::CString m_Cmd;
    Q3DStudio::CString m_ExportedFile;
    SThreadParam m_OutputReadThreadParam;
    HANDLE m_PreviewProcessHandle;
    HANDLE m_ReadThreadHandle;
    CPreviewHelper::EExecMode m_ViewMode;
};

#endif // INCLUDED_PREVIEW_OUTPUT_DIALOG_H
