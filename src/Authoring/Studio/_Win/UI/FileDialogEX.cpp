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
//	Includes
//==============================================================================
#include "stdafx.h"
#include "Strings.h"
#include <afxpriv.h>
#include "FileDialogEX.h"
#include "resource.h"
#include "windowsx.h"
#include "Userenv.h"
#include "Commdlg.h"
#include "CdErr.h"
#include "Preferences.h"

static const long SFILELISTBUFFERSIZE =
    2048; // Size of the char buffer used to store selected filenames
// static BOOL IsWin2000();

/////////////////////////////////////////////////////////////////////////////
// CFileDialogEx

IMPLEMENT_DYNAMIC(CFileDialogEx, CFileDialog)

//==============================================================================
/**
 *	CFileDialogEx: constructor
 */
//==============================================================================
CFileDialogEx::CFileDialogEx(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
                             DWORD dwFlags, LPCTSTR lpszFilter, CWnd *pParentWnd)
    : CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
    , m_IsSaving(FALSE)
    , m_LastFilterIndex(1)
    , m_FileListBuffer(nullptr)
    , m_WasCreateNewDirectoryChecked(false)
    , m_CreateDirectoryCheckboxEnabled(false)
    , m_Preferences(CPreferences::GetUserPreferences())
{
    m_IsSaving = !bOpenFileDialog; // if not openfiledlg, is saving
    m_FilterIndex = 0; // init to 0; index starts from 1
    m_ofnEx.pvReserved = nullptr;
    m_ofnEx.dwReserved = 0;
    m_ofnEx.FlagsEx = 0;

    if (dwFlags & OFN_ALLOWMULTISELECT) {
        // Create buffer for multiple selection
        m_FileListBuffer = new TCHAR[SFILELISTBUFFERSIZE];
        m_FileListBuffer[0] = '\0';

        GetOFN().lpstrFile = m_FileListBuffer;
        GetOFN().nMaxFile = SFILELISTBUFFERSIZE;
    }
}

CFileDialogEx::~CFileDialogEx()
{
    if (m_FileListBuffer) {
        delete[] m_FileListBuffer;
    }
}

BEGIN_MESSAGE_MAP(CFileDialogEx, CFileDialog)
//{{AFX_MSG_MAP(CFileDialogEX)
ON_COMMAND(IDC_CHECK1, OnCreateNewDirectory)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
/**
 *	IsWin2000: function to determine if we are on win2k
 *
 *	@return	true if we are on win2k
 */
//==============================================================================
// BOOL IsWin2000 ()
//{
//   OSVERSIONINFOEX theOsvi;
//   BOOL OsVersionInfoEx;
//
//   // Try calling GetVersionEx using the OSVERSIONINFOEX structure,
//   // which is supported on Windows 2000.
//   //
//   // If that fails, try using the OSVERSIONINFO structure.
//
//   ZeroMemory( &theOsvi, sizeof( OSVERSIONINFOEX ) );
//   theOsvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
//   OsVersionInfoEx = GetVersionEx ( ( OSVERSIONINFO* ) &theOsvi );
//
//   if( !OsVersionInfoEx )
//   {
//      // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
//
//      theOsvi.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
//      if ( !GetVersionEx( ( OSVERSIONINFO* ) &theOsvi) )
//         return FALSE;
//   }
//
//   switch ( theOsvi.dwPlatformId )
//   {
//      case VER_PLATFORM_WIN32_NT:
//
//         if ( theOsvi.dwMajorVersion >= 5 )
//            return TRUE;
//
//         break;
//   }
//   return FALSE;
//}

void CFileDialogEx::EnableCreateDirectoryCheckbox()
{
    m_CreateDirectoryCheckboxEnabled = true;
}

// http://msdn.microsoft.com/en-us/magazine/cc300434.aspx
struct RegKeyOverrider
{
    const char *getRegKeyName()
    {
        return "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\ComDlg32\\PlacesBar";
    }
    HKEY m_TempKey;

    RegKeyOverrider()
    {
        memset(&m_TempKey, 0, sizeof(HKEY));
        long newKeyResult =
            RegCreateKeyEx(HKEY_CURRENT_USER, L"UIComposer_FileDialog", 0, nullptr,
                           REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &m_TempKey, nullptr);
        if (newKeyResult == ERROR_SUCCESS) {
            long overrideResult = RegOverridePredefKey(HKEY_CURRENT_USER, m_TempKey);
            ASSERT(overrideResult == ERROR_SUCCESS);
            if (overrideResult == ERROR_SUCCESS) {
                // Customize the places bar now in or overridden key.
                HKEY placesKey;
                long placesResult =
                    RegCreateKeyEx(HKEY_CURRENT_USER, CString(getRegKeyName()), 0, nullptr,
                                   REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &placesKey, nullptr);
                if (placesResult == ERROR_SUCCESS) {
                    TCHAR szHomeDirBuf[MAX_PATH] = { 0 };

                    HANDLE hToken = 0;
                    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

                    DWORD BufSize = MAX_PATH;
                    GetUserProfileDirectory(hToken, szHomeDirBuf, &BufSize);

                    CloseHandle(hToken);

                    wcscat(szHomeDirBuf, L"\\Links");

                    size_t len = wcslen(szHomeDirBuf);
                    RegSetValueEx(placesKey, L"Place0", 0, REG_SZ, (const BYTE *)szHomeDirBuf,
                                  (DWORD)len);

                    TCHAR buffer[32] = { 0 };
                    DWORD folderIds[] = { 8, 0, 17, 18 };

                    for (int idx = 0; idx < 4; ++idx) {
                        swprintf_s(buffer, L"Place%d", idx + 1);
                        long setKeyResult =
                            RegSetValueEx(placesKey, buffer, 0, REG_DWORD,
                                          (const BYTE *)(folderIds + idx), sizeof(DWORD));
                        ASSERT(setKeyResult == ERROR_SUCCESS);
                    }
                }
                RegCloseKey(placesKey);
            }
        }
    }
    ~RegKeyOverrider()
    {
        RegOverridePredefKey(HKEY_CURRENT_USER, nullptr);
        RegDeleteKey(m_TempKey, nullptr);
        RegCloseKey(m_TempKey);
    }
};
struct SErrorMap
{
    DWORD m_Error;
    const char *m_Name;
};
static SErrorMap g_ErrorMap[] = {
#define MAKE_STRUCT(name) { name, #name },
    MAKE_STRUCT(CDERR_DIALOGFAILURE) MAKE_STRUCT(CDERR_FINDRESFAILURE)
        MAKE_STRUCT(CDERR_INITIALIZATION) MAKE_STRUCT(CDERR_LOADRESFAILURE)
            MAKE_STRUCT(CDERR_LOADSTRFAILURE) MAKE_STRUCT(CDERR_LOCKRESFAILURE)
                MAKE_STRUCT(CDERR_MEMALLOCFAILURE) MAKE_STRUCT(CDERR_MEMLOCKFAILURE)
                    MAKE_STRUCT(CDERR_NOHINSTANCE) MAKE_STRUCT(CDERR_NOHOOK)
                        MAKE_STRUCT(CDERR_NOTEMPLATE) MAKE_STRUCT(CDERR_STRUCTSIZE)
                            MAKE_STRUCT(FNERR_BUFFERTOOSMALL) MAKE_STRUCT(FNERR_INVALIDFILENAME)
                                MAKE_STRUCT(FNERR_SUBCLASSFAILURE)
#undef MAKE_STRUCT
                                    { 0, nullptr },
};
//==============================================================================
/**
 *	IsWin2000: Overloaded CFileDialog Function
 *
 *	adds extra parameter if the operating system is win2k
 *
 *	@return	true for file open... false for file save as
 */
//==============================================================================
INT_PTR CFileDialogEx::DoModal()
{
    ASSERT_VALID(this);
    ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
    ASSERT(m_ofn.lpfnHook != nullptr); // can still be a user hook

    // zero out the file buffer for consistent parsing later
    ASSERT(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
    DWORD nOffset = (DWORD)_tcslen(m_ofn.lpstrFile) + 1;
    ASSERT(nOffset <= m_ofn.nMaxFile);
    memset(m_ofn.lpstrFile + nOffset, 0, (m_ofn.nMaxFile - nOffset) * sizeof(TCHAR));

    if (!m_InitialDir.IsEmpty())
        m_ofn.lpstrInitialDir = m_InitialDir;

    // If the title has been specified, change it from the OS default
    if (!m_Title.IsEmpty())
        m_ofn.lpstrTitle = m_Title;

    // If filterIndex has been changed, use it instead of the default
    if (m_FilterIndex != 0)
        m_ofn.nFilterIndex = m_FilterIndex;

    if (m_CreateDirectoryCheckboxEnabled) {
        // Begin customization for the checkbox for new project dialog
        m_ofn.hInstance = GetModuleHandle(nullptr);
        m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_NEWPROJEXT);
        m_ofn.Flags |= OFN_ENABLETEMPLATE;
    }

    // WINBUG: This is a special case for the file open/save dialog,
    //  which sometimes pumps while it is coming up but before it has
    //  disabled the main window.
    HWND hWndFocus = ::GetFocus();
    BOOL bEnableParent = FALSE;

    // OVerride the places bar so that is always shows favorites.
    RegKeyOverrider regOverrider;
    m_ofn.hwndOwner = PreModal();
    AfxUnhookWindowCreate();
    if (m_ofn.hwndOwner != nullptr && ::IsWindowEnabled(m_ofn.hwndOwner)) {
        bEnableParent = TRUE;
        ::EnableWindow(m_ofn.hwndOwner, FALSE);
    }

    _AFX_THREAD_STATE *pThreadState = AfxGetThreadState();
    ASSERT(pThreadState->m_pAlternateWndInit == nullptr);

    if (m_ofn.Flags & OFN_EXPLORER)
        pThreadState->m_pAlternateWndInit = this;
    else
        AfxHookWindowCreate(this);

    // This is whre is differs for 2000
    ::memset(&m_ofnEx, 0, sizeof(m_ofnEx));
    ::memcpy(&m_ofnEx, &m_ofn, sizeof(m_ofn));
    // if ( IsWin2000() ){
    //	m_ofnEx.lStructSize = sizeof( m_ofnEx );
    //}

    int nResult;
    if (m_bOpenFileDialog)
        nResult = ::GetOpenFileName(&m_ofnEx);
    else
        nResult = ::GetSaveFileName(&m_ofnEx);

    if (nResult == 0) {
        DWORD error = ::CommDlgExtendedError();
        if (error) {
            SErrorMap *theError = nullptr;
            for (theError = g_ErrorMap; theError->m_Name; ++theError) {
                if (theError->m_Error == error) {
                    OutputDebugStringA("Massive failure in FileDialogEX!!\n");
                    OutputDebugStringA(theError->m_Name);
                    ASSERT(false);
                }
            }
        }
    }

    // Copy back m_ofnEx -> m_ofn
    ::memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
    m_ofn.lStructSize = sizeof(m_ofn);

    if (nResult)
        ASSERT(pThreadState->m_pAlternateWndInit == nullptr);
    pThreadState->m_pAlternateWndInit = nullptr;

    // WINBUG: Second part of special case for file open/save dialog.
    if (bEnableParent)
        ::EnableWindow(m_ofn.hwndOwner, TRUE);
    if (::IsWindow(hWndFocus))
        ::SetFocus(hWndFocus);

    PostModal();

    if (nResult) {
        m_LastFilterIndex = GetOFN().nFilterIndex;
    }

    return nResult ? nResult : IDCANCEL;
}

BOOL CFileDialogEx::OnFileNameOK()
{
    CFileStatus theFileStatus;
    CString theFilePath = Q3DStudio::CString(GetOFN().lpstrFile);
    // CString		theFilePath = this->GetPathName(); // Somehow, this does not return the filename
    // with extension
    BOOL theReturnValue = FALSE;

    if (m_IsSaving && CFile::GetStatus(theFilePath, theFileStatus)) {
        if (theFileStatus.m_attribute & CFile::readOnly) {
            Q3DStudio::CString theMsgTitle;
            Q3DStudio::CString theMsg;
            Q3DStudio::CString theFormattedText;
            Q3DStudio::CString theFileNameText = GetOFN().lpstrFileTitle;
            // Q3DStudio::CString theFileNameText = GetFileName( );

            theMsgTitle = ::LoadResourceString(IDS_PROJNAME);
            theMsg = ::LoadResourceString(IDS_SAVE_READONLY_WARNING);
            theFormattedText.Format(theMsg, static_cast<const wchar_t *>(theFileNameText));

            ::MessageBox(this->GetSafeHwnd(), theFormattedText, theMsgTitle,
                         MB_OK | MB_ICONEXCLAMATION);
            theReturnValue = TRUE;
        }
    }

    return theReturnValue;
}

void CFileDialogEx::SetInitialDirectory(const Q3DStudio::CString &inDir)
{
    Q3DStudio::CFilePath thePath(inDir);
    if (thePath.IsFile())
        thePath = thePath.GetDirectory();
    m_InitialDir = thePath;
}

void CFileDialogEx::OnCreateNewDirectory()
{
    m_WasCreateNewDirectoryChecked = !m_WasCreateNewDirectoryChecked;
    m_Preferences.SetValue("CreateDirForProject", m_WasCreateNewDirectoryChecked);
}

//=============================================================================
/**
 * Allows the user to set the title of the dialog.  By default, the title is
 * determined by the first parameter of the constructor (bOpenFileDialog).
 * TRUE specifies a file open dialog and FALSE specifies a Save As dialog.
 * The title of the window is then generated by the OS.  This function enables
 * you to set the title to any string you want, overriding the OS value.
 * @param inTitle new title for this dialog; if an empty string is specified,
 * the OS generated title will be displayed.
 */
void CFileDialogEx::SetWindowTitle(const Q3DStudio::CString &inTitle)
{
    m_Title = inTitle;
}

//=============================================================================
/**
 * Sets the filter to be selected when the dialog is opened, if specified.
 * The filter is indexed from 1 to n.
 *
 * @param inFilterIndex the index of the filter to be applied as the default
 * extension to be used when displaying the dialog.
 * @author AT May 29 2003
 */
void CFileDialogEx::SetFilterIndex(const LONG inFilterIndex)
{
    m_FilterIndex = inFilterIndex;
}

BOOL CFileDialogEx::OnInitDialog()
{
    BOOL retval = CFileDialog::OnInitDialog();
    if (m_CreateDirectoryCheckboxEnabled) {
        CWnd *theWnd = GetDlgItem(IDC_CHECK1);
        if (theWnd) {
            m_WasCreateNewDirectoryChecked =
                m_Preferences.GetValue("CreateDirForProject", true) ? 1 : 0;
            Button_SetCheck(theWnd->GetSafeHwnd(), m_WasCreateNewDirectoryChecked ? 1 : 0);
        }
    }
    return retval;
}

//=============================================================================
/**
 * Gets the filter index when the dialog is opened.
 * The filter is indexed from 1 to n.
 * The member was set at the end of DoModal.
 *
 * @return the filter index, starting from 1 to n
 */
long CFileDialogEx::GetLastFilterIndex()
{
    return m_LastFilterIndex;
}

//=============================================================================
/**
 * Retrieves the list of files selected during multiple selection
 * @param outFileList vector to hold filenames
 */
void CFileDialogEx::RetrieveFileList(TFILELIST &outFileList) const
{
    outFileList.clear();
    POSITION theStartPosition = GetStartPosition();

    Q3DStudio::CString theFileName;
    while (nullptr != theStartPosition) {
        theFileName = GetNextPathName(theStartPosition);
        outFileList.push_back(CUICFile(theFileName));
    }
}
