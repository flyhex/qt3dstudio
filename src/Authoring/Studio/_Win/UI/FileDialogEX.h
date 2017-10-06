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
#if !defined(AFX_FILEDIALOGEX_H__4CF114A7_E3C1_44AA_8A73_92CF2480D010__INCLUDED_)
#define AFX_FILEDIALOGEX_H__4CF114A7_E3C1_44AA_8A73_92CF2480D010__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct OPENFILENAMEEX : public OPENFILENAME
{
    void *pvReserved;
    DWORD dwReserved;
    DWORD FlagsEx;
};

//==============================================================================
/**
 *	CFileDialog: Extends the regular CFileDialog for win2k dialog box
 */
//==============================================================================
class CFileDialogEx : public CFileDialog
{
    DECLARE_DYNAMIC(CFileDialogEx)

public:
    typedef std::vector<CUICFile> TFILELIST;
    OPENFILENAMEEX m_ofnEx; ///< Extra structure passed in for win 2k dialog

    //==========================================================================
    //	Methods
    //==========================================================================
    CFileDialogEx(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
                  LPCTSTR lpszDefExt = nullptr, LPCTSTR lpszFileName = nullptr,
                  DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = nullptr,
                  CWnd *pParentWnd = nullptr);

    virtual ~CFileDialogEx();

    void EnableCreateDirectoryCheckbox();
    virtual void SetInitialDirectory(const Q3DStudio::CString &inDir);
    virtual void SetWindowTitle(const Q3DStudio::CString &inTitle);
    virtual void SetFilterIndex(const LONG inFilterIndex);
    virtual BOOL OnInitDialog();
    long GetLastFilterIndex();
    void RetrieveFileList(TFILELIST &outFileList) const;

    virtual INT_PTR DoModal();
    virtual BOOL OnFileNameOK();
    void IsSaving() { m_IsSaving = TRUE; }
    bool WasCreateNewDirectoryChecked() { return m_WasCreateNewDirectoryChecked; }

protected:
    BOOL m_IsSaving;
    Q3DStudio::CString m_InitialDir;
    Q3DStudio::CString m_Title;
    LONG m_FilterIndex;
    long m_LastFilterIndex; //< the last filter index for when a file was selected
    wchar_t *m_FileListBuffer;
    bool m_CreateDirectoryCheckboxEnabled;
    bool m_WasCreateNewDirectoryChecked;
    // We have to cache a CPreferences here because we reroute the registry during
    // doModal in order to make the file dialogue behavior differently.
    // see RegKeyOverrider
    CPreferences m_Preferences;

    //==========================================================================
    //	Members
    //==========================================================================

    // Generated message map functions
    //{{AFX_MSG(CFileDialogEx)
    afx_msg void OnCreateNewDirectory();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEDIALOGEX_H__4CF114A7_E3C1_44AA_8A73_92CF2480D010__INCLUDED_)
