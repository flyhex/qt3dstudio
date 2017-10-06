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

//=============================================================================
// Prefix
//=============================================================================
#include "stdafx.h"

#pragma warning(disable : 4127)

//==============================================================================
//	Includes
//==============================================================================
#include "PreviewOutputDlg.h"
#include "StudioClipboard.h"
#include "UICFileTools.h"
#include "StudioApp.h"
#include "Doc.h"
#include "Mainfrm.h"
#include "Dialogs.h"
#include "Strings.h"
#include "Resource.h"
#include "Core.h"

// CPreviewOutput dialog

IMPLEMENT_DYNAMIC(CPreviewOutputDlg, CDialog)

CPreviewOutputDlg::CPreviewOutputDlg(const Q3DStudio::CString &inCmd,
                                     const Q3DStudio::CString &inExportedFile,
                                     CPreviewHelper::EExecMode inViewMode)
    : CDialog(CPreviewOutputDlg::IDD, nullptr)
    , m_Cmd(inCmd)
    , m_ExportedFile(inExportedFile)
    , m_ViewMode(inViewMode)
    , m_ReadThreadHandle(nullptr)
    , m_PreviewProcessHandle(nullptr)
{
    m_OutputReadThreadParam.m_ReadHandle = nullptr;
    m_OutputReadThreadParam.m_Context = nullptr;
}

CPreviewOutputDlg::~CPreviewOutputDlg()
{
    if (m_ReadThreadHandle) {
        TerminateThread(m_ReadThreadHandle, 0);
        m_ReadThreadHandle = nullptr;
    }

    if (m_OutputReadThreadParam.m_ReadHandle) {
        CloseHandle(m_OutputReadThreadParam.m_ReadHandle);
        m_OutputReadThreadParam.m_ReadHandle = nullptr;
    }
}

void CPreviewOutputDlg::DoDataExchange(CDataExchange *pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PREVIEW_STATUS, m_ListCtrl);
}

//=============================================================================
/**
 *	Message Handler when dialog is initially opened.
 */
//=============================================================================
BOOL CPreviewOutputDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the dialog header
    Q3DStudio::CString theFile = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath().GetName();
    if (theFile == "")
        theFile = ::LoadResourceString(IDS_UNTITLED_DOCUMENT_TITLE);
    Q3DStudio::CString theDlgHeader;
    Q3DStudio::CString theOperation;
    switch (m_ViewMode) {
    case CPreviewHelper::EXECMODE_PREVIEW:
        theOperation = ::LoadResourceString(IDS_CONFIG_EXPORT_PREVIEW);
        break;

    case CPreviewHelper::EXECMODE_DEPLOY:
        theOperation = ::LoadResourceString(IDS_CONFIG_EXPORT_DEPLOY);
        break;
    }
    theDlgHeader.Format(::LoadResourceString(IDS_CONFIG_PREVIEW_TITLE),
                        static_cast<const wchar_t *>(theOperation),
                        static_cast<const wchar_t *>(theFile));
    SetWindowText(theDlgHeader);

    // Prepare the list control
    COLORREF theColor = 0x00C0C0C0;
    m_ListCtrl.SetBkColor(theColor);
    m_ListCtrl.SetTextBkColor(theColor);
    CRect theRect;
    m_ListCtrl.GetClientRect(theRect);
    m_ListCtrl.InsertColumn(0, L"", LVCFMT_LEFT,
                            theRect.Width() * 2); // Double the width to see more

    // Start the preview
    if (!ExecutePreview()) {
        CUICFile thePreviewerFile(CUICFile::GetApplicationDirectory(), m_ExportedFile);
        Q3DStudio::CString theMessage;
        theMessage.Format(::LoadResourceString(IDS_CONFIG_PREVIEW_ERROR_MSG),
                          static_cast<const wchar_t *>(thePreviewerFile.GetAbsolutePosixPath()));
        g_StudioApp.GetDialogs()->DisplayMessageBox(
            ::LoadResourceString(IDS_CONFIG_PREVIEW_ERROR_TITLE), theMessage,
            CUICMessageBox::ICON_ERROR, false);
    }

    return TRUE;
}

//=============================================================================
/**
 *	Handler for copying
 */
//=============================================================================
void CPreviewOutputDlg::OnCopyText()
{
    try {
        Q3DStudio::CString theText = "";
        int theNumEntry = m_ListCtrl.GetItemCount();
        for (int theIndex = 0; theIndex < theNumEntry; ++theIndex) {
            theText += m_ListCtrl.GetItemText(theIndex, 0);
            theText += "\r\n";
        }
        CStudioClipboard::CopyTextToClipboard(theText);
    } catch (...) {
    }
}

//=============================================================================
/**
 *	Handler for OK Button
 */
//=============================================================================
void CPreviewOutputDlg::OnOK()
{
    TerminateThread(m_ReadThreadHandle, 0);
    CDialog::OnOK();
    m_ReadThreadHandle = nullptr;
}

BEGIN_MESSAGE_MAP(CPreviewOutputDlg, CDialog)
ON_BN_CLICKED(IDC_BUTTON_COPY_TEXT, OnCopyText)
END_MESSAGE_MAP()

//==============================================================================
/**
 *	Spawn the process to execute the preview operation and redirect it's output
 *	to the list control
 */
//==============================================================================
BOOL CPreviewOutputDlg::ExecutePreview()
{
    HANDLE theOutputReadHandleTmp;
    HANDLE theOutputReadHandle;
    HANDLE theOutputWriteHandle;

    // Set up the security attributes struct.
    SECURITY_ATTRIBUTES theSecurityAttr;
    theSecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    theSecurityAttr.lpSecurityDescriptor = nullptr;
    theSecurityAttr.bInheritHandle = TRUE;

    // Create the child output pipe.
    if (!CreatePipe(&theOutputReadHandleTmp, &theOutputWriteHandle, &theSecurityAttr, 0))
        return FALSE;

    // Create new output read handle. Set the Properties to FALSE.
    // Otherwise, the child inherits the properties and, as a result,
    // non-closeable handles to the pipes are created.
    if (!DuplicateHandle(GetCurrentProcess(), theOutputReadHandleTmp, GetCurrentProcess(),
                         &theOutputReadHandle, // Address of new handle.
                         0, FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
        return FALSE;

    // Close inheritable copies of the handles you do not want to be inherited.
    CloseHandle(theOutputReadHandleTmp);

    LaunchPreviewProcess(theOutputWriteHandle, m_Cmd);

    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    CloseHandle(theOutputWriteHandle);

    // Spawn a thread to busy read the preivew process output.
    DWORD theThreadId;
    m_OutputReadThreadParam.m_Context = this;
    m_OutputReadThreadParam.m_ReadHandle = theOutputReadHandle;
    m_ReadThreadHandle = CreateThread(nullptr, 0, CPreviewOutputDlg::ReadOutputThread,
                                      (LPVOID)&m_OutputReadThreadParam, 0, &theThreadId);
    return TRUE;
}

//==============================================================================
/**
 *	Callback when the preview operation has completed.
 */
//==============================================================================
void CPreviewOutputDlg::PreviewProcessTerminated()
{
    DWORD theExitCode;
    GetExitCodeProcess(m_PreviewProcessHandle, &theExitCode);
    if (theExitCode == 0)
        PostMessage(WM_CLOSE); // close the window
    else {
        Q3DStudio::CString theOperation;
        switch (m_ViewMode) {
        case CPreviewHelper::EXECMODE_PREVIEW:
            theOperation = ::LoadResourceString(IDS_CONFIG_EXPORT_PREVIEW);
            break;

        case CPreviewHelper::EXECMODE_DEPLOY:
            theOperation = ::LoadResourceString(IDS_CONFIG_EXPORT_DEPLOY);
            break;
        }
        Q3DStudio::CString theMessage;
        theMessage.Format(::LoadResourceString(IDS_CONFIG_PREVIEW_ERROR_TEXT),
                          static_cast<const wchar_t *>(theOperation));
        DisplayOutput(theMessage);
    }
}

//==============================================================================
/**
 *	Write the string into the list control.
 *	@param inString		the string to display
 */
//==============================================================================
void CPreviewOutputDlg::DisplayOutput(CString inStr)
{
    const wchar_t *DELIMITERS = L"\r\t\n";

    wchar_t theBuffer[512] = { 0 };
    ::wcscpy(theBuffer, inStr);

    wchar_t *theToken = ::wcstok(theBuffer, DELIMITERS);
    while (theToken) {
        m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), theToken);
        theToken = ::wcstok(nullptr, DELIMITERS);
    }

    m_ListCtrl.EnsureVisible(m_ListCtrl.GetItemCount() - 1, FALSE);
}

//==============================================================================
/**
 *	Spawn the process to execute the preview operation
 *	@param inStdOutput		the handle to write the output to
 *	@param inCmd			the command to execute
 *	@return true is spawned successfully, else false
 */
//==============================================================================
BOOL CPreviewOutputDlg::LaunchPreviewProcess(HANDLE inStdOutput, Q3DStudio::CString &inCmd)
{
    // Further usability hax, you love them.
    DisplayOutput(L"Checking for .NET Framework install:");
    HKEY theHKey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      L"SOFTWARE\\Microsoft\\NET Framework Setup\\NDP", 0,
                                      KEY_QUERY_VALUE, &theHKey)) {
        DisplayOutput(L" - OK: .NET Framework present.");
    } else {
        DisplayOutput(L" - WARNING: .NET Framework Missing?");
    }

    // Launch the process with the working directory set to the parent directory of current document
    CUICFile theDocument = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath();

    Q3DStudio::CString theWorkingPath;
    if (theDocument.Exists())
        theWorkingPath = theDocument.GetAbsolutePath();
    else
        theWorkingPath = m_ExportedFile;

    CUICFile thePreviewerFile(CUICFile::GetApplicationDirectory(), theWorkingPath);
    Q3DStudio::CFilePath theWorkingDirectory(thePreviewerFile.GetAbsolutePosixPath());

    PROCESS_INFORMATION theProcessInfo;
    STARTUPINFO theStartupInfo;

    // Set up the start up info struct.
    ZeroMemory(&theStartupInfo, sizeof(STARTUPINFO));
    theStartupInfo.cb = sizeof(STARTUPINFO);
    theStartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    theStartupInfo.hStdOutput = inStdOutput;
    theStartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE) /*hChildStdIn*/;
    theStartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    theStartupInfo.wShowWindow = SW_HIDE;

    if (!CreateProcess(nullptr, const_cast<wchar_t *>(inCmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr,
                       theWorkingDirectory.GetDirectory(), &theStartupInfo, &theProcessInfo))
        return FALSE;

    // Close any unnecessary handles.
    CloseHandle(theProcessInfo.hThread);
    m_PreviewProcessHandle = theProcessInfo.hProcess;
    return TRUE;
}

//==============================================================================
/**
 *	Spawn the thread to busy read the output and write to the list control
 *	@param inThreadParam	the parameter used by this thread
 *	@return 1
 */
//==============================================================================
DWORD WINAPI CPreviewOutputDlg::ReadOutputThread(LPVOID inThreadParam)
{
    char theBuffer[512];
    DWORD theBytesRead;
    SThreadParam *theParam = (SThreadParam *)inThreadParam;
    HANDLE thePipeRead = theParam->m_ReadHandle;
    CPreviewOutputDlg *theDlg = (CPreviewOutputDlg *)(theParam->m_Context);

    while (TRUE) {
        if (!ReadFile(thePipeRead, theBuffer, sizeof(theBuffer) - sizeof(char), &theBytesRead, nullptr)
            || !theBytesRead) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                theDlg->PreviewProcessTerminated();
                CloseHandle(thePipeRead);
                theParam->m_ReadHandle = nullptr;
                break; // pipe done - normal exit path.
            } else
                theDlg->DisplayOutput(::LoadResourceString(
                    IDS_CONFIG_PREVIEW_REDIRECT_ERROR)); // Something bad happened.
        } else {
            if (theBytesRead) {
                theBuffer[theBytesRead] = '\0'; // Follow input with a nullptr.
                theDlg->DisplayOutput(CString(theBuffer));
            }
        }
    }
    return 1;
}
