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

//==============================================================================
//	Includes
//==============================================================================
#include "UICFile.h"
#ifdef KDAB_TEMPORARILY_REMOVED
#include <shellapi.h> // ShellExecute, etc.
#include <Urlmon.h> // CoInternetCombineURL, etc.
#endif
#include "IOLibraryException.h"
#include <sys/stat.h> // _stat

// CUICFile originally uses ***W for file ops
// but this only works for win2k and above (Bug2994)
// if using plain version is giving too much problem, uncomment the line below
//#define USE_WIDE_VERSION

//==============================================================================
//	Static variables
//==============================================================================
TFilePathList CUICFile::s_TempFilePathList;

//==============================================================================
//	Constants
//==============================================================================
#ifdef KDAB_TEMPORARILY_REMOVED
const Q3DStudio::CString CUICFile::CURRENT_DIR(".\\");
const Q3DStudio::CString CUICFile::PARENT_DIR("..\\");

//=============================================================================
/**
 * Create a new file from the path.
 * @param inIsPosix ignored.
 * @param inAddBase ignored.
 */
CUICFile::CUICFile(const Q3DStudio::CString &inPathName, bool inIsPosix, bool inAddBase)
{
    Q_UNUSED(inIsPosix);
    Q_UNUSED(inAddBase);

    m_Path = inPathName;
}

//=============================================================================
/**
 * Create a file by combining the two paths.
 */
CUICFile::CUICFile(Q3DStudio::CString inPathName, Q3DStudio::CString inName)
{
    CUICFile theBasePath(inPathName);
    CUICFile theFile = Combine(theBasePath, inName);
    m_Path = theFile.GetPath();
}

//=============================================================================
/**
 * Create a file by combining the base path with a relative path.
 */
CUICFile::CUICFile(const CUICFile &inBasePath, const Q3DStudio::CString &inPathname, bool inIsPosix)
{
    Q_UNUSED(inIsPosix);

    CUICFile theFile(Combine(inBasePath, inPathname));
    m_Path = theFile.GetPath();
}

CUICFile::CUICFile(const CUICFile &inFile)
{
    m_Path = inFile.m_Path;
}

//=============================================================================
/**
 * Get an iterator for all the sub-files of this directory.
 */
CFileIterator CUICFile::GetSubItems() const
{
    return CFileIterator(this);
}

//=============================================================================
/**
 * Destructor
 */
CUICFile::~CUICFile()
{
}

bool CUICFile::operator==(const CUICFile &inRHS) const
{
#ifdef _WIN32
    return GetAbsolutePath().CompareNoCase(inRHS.GetAbsolutePath());
#else
    return GetAbsolutePath() == inRHS.GetAbsolutePath();
#endif
}

//=============================================================================
/**
 * Returns true if this is a file and can be read.
 */
bool CUICFile::CanRead() const
{
    return IsFile();
}

//=============================================================================
/**
 * Returns true if this is a file and can be written to.
 */
bool CUICFile::CanWrite() const
{
    long theAttributes = ::GetFileAttributesW(GetAbsolutePath());
    return (theAttributes != INVALID_FILE_ATTRIBUTES) && !(theAttributes & FILE_ATTRIBUTE_READONLY);
}

//=============================================================================
/**
 *	Delete this file from the file system. This will also perform a recursive
 *	delete on the sub folders and files if it is a folder
 *
 *	@return	true if the deletion of file/folder is successful, else false
 */
bool CUICFile::DeleteFile() const
{
    BOOL theFileDeleted = FALSE;

    // check if AKFile to delete is a folder type, if it is, we want to recusively delete all its
    // subfolder
    if (!IsFile()) {
        CFileIterator theIterator = GetSubItems();

        for (; !theIterator.IsDone(); ++theIterator) {
            CUICFile theFile = theIterator.GetCurrent();
            theFile.DeleteFile();
        }
        theFileDeleted = ::RemoveDirectory(m_Path);
    } else {
        // delete the requested file or the main folder
        theFileDeleted = ::DeleteFile(m_Path);
    }

    // erase it from this list
    s_TempFilePathList.erase(m_Path);

    return theFileDeleted == TRUE;
}

//=============================================================================
/**
 * Check to see if this file or directory exists.
 */
bool CUICFile::Exists() const
{
#ifdef USE_WIDE_VERSION
    long theAttributes = ::GetFileAttributesW(GetAbsolutePath());
#else
    long theAttributes = ::GetFileAttributes(GetAbsolutePath());
#endif
    return theAttributes != INVALID_FILE_ATTRIBUTES;
}

//=============================================================================
/**
 * Get the fully qualified absolute path.
 * This should resolve all relative parts of the path.
 */
Q3DStudio::CString CUICFile::GetAbsolutePath() const
{
    return m_Path;
}

//=============================================================================
/**
 * @see GetAbsolutePath.
 */
Q3DStudio::CString CUICFile::GetAbsolutePosixPath() const
{
    return GetAbsolutePath();
}

//=============================================================================
/**
 * Get the filename section of this file, ignoring all drives and directories.
 */
Q3DStudio::CString CUICFile::GetName() const
{
    // back/forward slashes counts in a file path (a badly formed, but still acceptable in win32)
    // apparently, m_Path.ReverseFind( "\\/" ); doesnt work as required
    // since no file name would consist of '/', a hack here is better than changing ReverseFind
    // the ideal code would be:
    //
    // std::wstring theWString = m_Path.GetWidthString( ); // no luck...
    // long theIdx = static_cast<long>( theWString.find_last_of( L"\\/" ) );
    // if ( theIdx == std::wstring::npos )
    //	theIdx = -1;
    // return m_Path.Extract( theIdx + 1 );

    // so here's the little hack
    long theIdx1 = m_Path.ReverseFind('\\');
    long theIdx2 = m_Path.ReverseFind('/');
    long theIdx = -1;
    if (theIdx1 != Q3DStudio::CString::ENDOFSTRING) {
        if (theIdx2 != Q3DStudio::CString::ENDOFSTRING) {
            // both exist, so take the larger one
            theIdx = (theIdx1 > theIdx2) ? theIdx1 : theIdx2;
        } else {
            theIdx = theIdx1;
        }
    } else {
        if (theIdx2 != Q3DStudio::CString::ENDOFSTRING) {
            theIdx = theIdx2;
        } else {
            theIdx = -1;
        }
    }
    return m_Path.Extract(theIdx + 1);

    // the original version
    // long theIdx = m_Path.ReverseFind( '\\' );
    // if ( theIdx != Q3DStudio::CString::ENDOFSTRING )
    //	return m_Path.Extract( theIdx + 1 );
    // return m_Path;
}

//=============================================================================
/**
 * Get the file extension, without the period.
 */
Q3DStudio::CString CUICFile::GetExtension() const
{
    long theIdx = m_Path.ReverseFind('.');
    if (theIdx != Q3DStudio::CString::ENDOFSTRING)
        return m_Path.Extract(theIdx + 1);

    return "";
}

//=============================================================================
/**
 * Get the underlying path for this file, this may include relativity.
 */
Q3DStudio::CString CUICFile::GetPath() const
{
    return m_Path;
}

//=============================================================================
/**
 * Returns true if this file exists and is not a directory.
 * The param inCheckForAlias is not used in Windows
 */
bool CUICFile::IsFile(bool inCheckForAlias /*true*/) const
{
    Q_UNUSED(inCheckForAlias);
#ifdef USE_WIDE_VERSION
    long theAttributes = ::GetFileAttributesW(GetAbsolutePath());
#else
    long theAttributes = ::GetFileAttributes(GetAbsolutePath());
#endif
    return (theAttributes != INVALID_FILE_ATTRIBUTES)
        && !(theAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

//=============================================================================
/**
 * Check to see if this file or directory is hidden.
 */
bool CUICFile::IsHidden() const
{
#ifdef USE_WIDE_VERSION
    long theAttributes = ::GetFileAttributesW(GetAbsolutePath());
#else
    long theAttributes = ::GetFileAttributes(GetAbsolutePath());
#endif
    return (theAttributes != INVALID_FILE_ATTRIBUTES) && (theAttributes & FILE_ATTRIBUTE_HIDDEN);
}

//=============================================================================
/**
 * Get the size of this file in bytes.
 */
long CUICFile::Length() const
{
    HANDLE theFile = OpenFileReadHandle();
    long theFilesize = ::GetFileSize(theFile, NULL);
    ::CloseHandle(theFile);

    return theFilesize;
}

//=============================================================================
/**
 * Rename (or move) this file to the other file.
 */
void CUICFile::RenameTo(const CUICFile &inDestination)
{
#ifdef USE_WIDE_VERSION

    ::MoveFileW(GetAbsolutePath(), inDestination.GetAbsolutePath());

#else
    // note that destination file must NOT exist
    if (!::MoveFile(GetAbsolutePath(), inDestination.GetAbsolutePath()))
        throw CIOException();
#endif
}

//=============================================================================
/**
 * Copy this file to the other file, leaving this file intact.
 */
void CUICFile::CopyTo(const CUICFile &inDestination)
{
#ifdef USE_WIDE_VERSION
    if (!::CopyFileW(GetAbsolutePath(), inDestination.GetAbsolutePath(), FALSE))
#else
    if (!::CopyFile(GetAbsolutePath(), inDestination.GetAbsolutePath(), FALSE))
#endif
        throw CIOException();
}

//=============================================================================
/**
 * Make this file read only. or unmark the read only
 */
void CUICFile::SetReadOnly(bool inReadOnlyFlag)
{
#ifdef USE_WIDE_VERSION

    DWORD theFileAttributes = ::GetFileAttributesW(GetAbsolutePath());
    // If we are setting the file to be readonly
    if (inReadOnlyFlag)
        ::SetFileAttributesW(GetAbsolutePath(), theFileAttributes | FILE_ATTRIBUTE_READONLY);
    else
        ::SetFileAttributesW(GetAbsolutePath(), theFileAttributes & !FILE_ATTRIBUTE_READONLY);

#else

    DWORD theFileAttributes = ::GetFileAttributes(GetAbsolutePath());
    // If we are setting the file to be readonly
    if (inReadOnlyFlag)
        ::SetFileAttributes(GetAbsolutePath(), theFileAttributes | FILE_ATTRIBUTE_READONLY);
    else
        ::SetFileAttributes(GetAbsolutePath(), theFileAttributes & !FILE_ATTRIBUTE_READONLY);

#endif
}

//=============================================================================
/**
 * Get the location of where this application resides.
 */
CUICFile CUICFile::GetApplicationDirectory()
{
    Q3DStudio::CString theModuleDir;
    wchar_t theModulePath[MAX_PATH] = { '\0' };
    DWORD theRetVal = 0;

    theRetVal = ::GetModuleFileNameW(NULL, theModulePath, MAX_PATH - 1);
    theModulePath[theRetVal] = 0;

    theModuleDir = theModulePath;
    int theDirSeparatorIdx = theModuleDir.ReverseFind('\\');
    if (theDirSeparatorIdx != Q3DStudio::CString::ENDOFSTRING)
        theModuleDir = theModuleDir.Extract(0, theDirSeparatorIdx + 1);

    return theModuleDir;
}

//=============================================================================
/**
 * Create a temporary file from where the system holds it's temp files.
 * @param inExtension the file extension that should be used.
 */
CUICFile CUICFile::GetTemporaryFile(const Q3DStudio::CString &inExtension)
{
    wchar_t theTempPath[MAX_PATH] = { '\0' };
    // char theTempFileName[MAX_PATH] = {'\0'};

    // Get the temporary file directory name
    ::GetTempPath(sizeof(theTempPath), theTempPath);

    Q3DStudio::CString theTempFile(theTempPath);
    theTempFile += L"Untitled." + inExtension;
    /*
            // Build a temporary filename
            ::GetTempFileName( theTempPath, "~ui", 0, theTempFileName );

            theTempFile = Q3DStudio::CString( theTempFileName );

            theTempFile += ".";
            theTempFile += inExtension;
    */
    // Ensure the file does not exist
    ::DeleteFile(theTempFile);

    s_TempFilePathList.insert(theTempFile);

    return CUICFile(theTempFile);
}

CUICFile CUICFile::GetTemporaryFile()
{
    wchar_t theTempPath[MAX_PATH] = { '\0' };
    wchar_t theTempFileName[MAX_PATH] = { '\0' };

    // Get the temporary file directory name
    ::GetTempPath(MAX_PATH, theTempPath);

    // Build a temporary filename
    if (::GetTempFileName(theTempPath, L"~ui", 0, theTempFileName) != 0) {
        Q3DStudio::CString theTempFile(theTempFileName);
        s_TempFilePathList.insert(theTempFile);
        return CUICFile(theTempFile);
    }
    // return "" if the system fails us.
    return CUICFile("");
}

//=============================================================================
/**
 * Get the URL representing this file.
 */
CURL CUICFile::GetURL() const
{
    Q3DStudio::CString thePath(m_Path);

    // Return a fully qualified path if m_Path is relative.
    if ((thePath.Find(CURRENT_DIR) == 0) || (thePath.Find(PARENT_DIR) == 0)) {
        if (thePath.Find(CURRENT_DIR) == 0)
            thePath.Delete(0, CURRENT_DIR.Length());

        thePath.Insert(0, CUICFile::GetApplicationDirectory().GetAbsolutePosixPath());
    }

    return CURL(thePath);
}

//=============================================================================
/**
 * Request the filesystem to open this file in whatever manner it chooses with
 * the associated application.
 */
void CUICFile::Execute() const
{
    Q3DStudio::CString sFile = GetAbsolutePath();

    SHELLEXECUTEINFO sei;
    ::ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = ::GetActiveWindow();
    sei.nShow = SW_SHOW;
    sei.lpFile = sFile;
    sei.fMask = SEE_MASK_INVOKEIDLIST;
    ::ShellExecuteEx(&sei);
}

//=============================================================================
/**
 * Combine the file and relative path together into another file.
 */
CUICFile CUICFile::Combine(const CUICFile &inBasePath, const Q3DStudio::CString &inRelativePath)
{
    Q3DStudio::CString theFullPath;

    const int MAX_BUFFER_SIZE = 512;

    DWORD theBufferSize;
    WCHAR theResultBuffer[MAX_BUFFER_SIZE];

    wchar_t *theWorkingDir =
        const_cast<wchar_t *>(static_cast<const wchar_t *>(inBasePath.GetAbsolutePath()));
    wchar_t *theRelPath = const_cast<wchar_t *>(static_cast<const wchar_t *>(inRelativePath));

    // This method is a bit of black magic, but seems to work well for client.
    HRESULT theResult = CoInternetCombineUrl(theWorkingDir, theRelPath, 0, theResultBuffer,
                                             MAX_BUFFER_SIZE, &theBufferSize, 0);
    if (SUCCEEDED(theResult)) {
        // I can find no reference to the flags parameter for CoInternetCombineUrl, maybe
        // it does something, maybe not, anyway, to get rid of the %20 for spaces (and other
        // special characters) that this function creates, we need to call the following function

        theResult = CoInternetParseUrl(theResultBuffer, PARSE_UNESCAPE, 0, theResultBuffer,
                                       MAX_BUFFER_SIZE, &theBufferSize, 0);
        theResult = CoInternetParseUrl(theResultBuffer, PARSE_PATH_FROM_URL, 0, theResultBuffer,
                                       MAX_BUFFER_SIZE, &theBufferSize, 0);

        theFullPath = theResultBuffer;
    }

    return theFullPath;
}

//=============================================================================
/**
 * Get a file handle for this file that can be used for reading.
 * The handle must be closed with CloseHandle when finished.
 */
HANDLE CUICFile::OpenFileReadHandle() const
{
#ifdef USE_WIDE_VERSION

    return ::CreateFileW(GetAbsolutePath(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, NULL);

#else

    return ::CreateFile(GetAbsolutePath(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

#endif
}

//=============================================================================
/**
 * Get a file handle for this file that can be used for writing.
 * The handle must be closed with CloseHandle when finished.
 */
HANDLE CUICFile::OpenFileWriteHandle() const
{
#ifdef USE_WIDE_VERSION

    return ::CreateFileW(GetAbsolutePath(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

#else

    return ::CreateFile(GetAbsolutePath(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

#endif
}

//=============================================================================
/**
 *	Clear all temp files that have been created so far by calling the GetTemporaryFile methods.
 *	This would only clear the temp files that were created during the current program session,
 *	and not any previous files created by anything prior to this session.
 */
void CUICFile::ClearCurrentTempCache()
{
    TFilePathListIterator theTempFileIterator;

    if (!s_TempFilePathList.empty()) {
        // Delete all temp files created so far
        for (theTempFileIterator = s_TempFilePathList.begin();
             theTempFileIterator != s_TempFilePathList.end(); ++theTempFileIterator)
            ::DeleteFile((*theTempFileIterator));

        s_TempFilePathList.clear();
    }
}

void CUICFile::AddTempFile(const Q3DStudio::CString &inFile)
{
    s_TempFilePathList.insert(inFile);
}

//=============================================================================
/**
 *	Checks if a path is relative or not.
 *	Filename-only strings have no path separators and are considered relative.
 *  @param inPath path to check
 *  @return bool true to indicate this is a relative path
 */
bool CUICFile::IsPathRelative(const Q3DStudio::CString &inPath)
{
    bool theIsRelative = false;

    if (!inPath.IsEmpty()) {
        // relative paths start with a '.' or is just a filename entry
        theIsRelative =
            (inPath.GetAt(0) == L'.') || (inPath.Find('\\') == Q3DStudio::CString::ENDOFSTRING);
    }

    return theIsRelative;
}

//=============================================================================
/**
 * Retrieves the file 'stat' struct that contains useful information.
 * @param inStat stat struct to fill
 * @return true if file stats are successfully obtained
 */
bool CUICFile::GetFileStat(struct _stat *inStat) const
{
    return ::_wstat(m_Path, inStat) == 0;
}
#endif
