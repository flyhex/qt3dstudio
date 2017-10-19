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
// Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSFile.h"
#include "Qt3DSFileTools.h"

#include "IOLibraryException.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QUrl>

//==============================================================================
// Static variables
//==============================================================================
TFilePathList CUICFile::s_TempFilePathList;

//==============================================================================
// Constants
//==============================================================================

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

    QString path = inPathName.toQString();
#ifndef Q_OS_WIN
    path.replace('\\', '/');
#endif
    m_Path = Q3DStudio::CString::fromQString(QDir::toNativeSeparators(path));
}

//=============================================================================
/**
 * Create a file by combining the two paths.
 */
CUICFile::CUICFile(const Q3DStudio::CString &inPathName, const Q3DStudio::CString &inName)
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
    QFileInfo info(m_Path.toQString());
    return info.isWritable();
}

//=============================================================================
/**
 * Delete this file from the file system. This will also perform a recursive
 * delete on the sub folders and files if it is a folder
 *
 * @return true if the deletion of file/folder is successful, else false
 */
bool CUICFile::DeleteFile() const
{
    BOOL theFileDeleted = FALSE;

    // check if AKFile to delete is a folder type, if it is, we want to recusively delete all its
    // subfolder
    if (!IsFile()) {
        theFileDeleted = QDir(m_Path.toQString()).removeRecursively();
    } else {
        // delete the requested file or the main folder
        theFileDeleted = QFile::remove(m_Path.toQString());
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
    QFileInfo info(m_Path.toQString());
    return info.exists();
}

//=============================================================================
/**
 * Get the fully qualified absolute path.
 * This should resolve all relative parts of the path.
 */
Q3DStudio::CString CUICFile::GetAbsolutePath() const
{
    const QFileInfo fi(m_Path.toQString());
    if (fi.isDir())
        return Q3DStudio::CString::fromQString(fi.absoluteFilePath() + QDir::separator());
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
    QFileInfo info(m_Path.toQString());
    return Q3DStudio::CString::fromQString(info.fileName());
}

//=============================================================================
/**
 * Get the file extension, without the period.
 */
Q3DStudio::CString CUICFile::GetExtension() const
{
    QFileInfo info(m_Path.toQString());
    return Q3DStudio::CString::fromQString(info.suffix());
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
    QFileInfo info(m_Path.toQString());
    return info.isFile();
}

//=============================================================================
/**
 * Check to see if this file or directory is hidden.
 */
bool CUICFile::IsHidden() const
{
    QFileInfo info(m_Path.toQString());
    return info.isHidden();
}

//=============================================================================
/**
 * Get the size of this file in bytes.
 */
long CUICFile::Length() const
{
    QFileInfo info(m_Path.toQString());
    return info.size();
}

//=============================================================================
/**
 * Rename (or move) this file to the other file.
 */
void CUICFile::RenameTo(const CUICFile &inDestination)
{
    if (!QFile::rename(m_Path.toQString(), inDestination.GetAbsolutePath().toQString())) {
        throw CIOException();
    }
}

//=============================================================================
/**
 * Copy this file to the other file, leaving this file intact.
 */
void CUICFile::CopyTo(const CUICFile &inDestination)
{
    const QString destination(inDestination.GetAbsolutePath().toQString());
    if (QFile::exists(destination))
        QFile::remove(destination);
    if (!QFile::copy(m_Path.toQString(), destination))
        throw CIOException();
}

//=============================================================================
/**
 * Make this file read only. or unmark the read only
 */
void CUICFile::SetReadOnly(bool inReadOnlyFlag)
{
    const QString qpath(m_Path.toQString());
    QFile::Permissions perm = QFile::permissions(qpath);
    if (inReadOnlyFlag) {
        perm &= ~QFile::WriteOwner;
    } else {
        perm |= QFile::WriteOwner;
    }
    QFile::setPermissions(qpath, perm);
}

//=============================================================================
/**
 * Get the location of where this application resides.
 */
CUICFile CUICFile::GetApplicationDirectory()
{
#ifdef Q_OS_MACOS
    QDir appDir(qApp->applicationDirPath());
    if (appDir.dirName() == "MacOS") {
        appDir.cdUp();
        appDir.cd("Resources");
    }
    return CUICFile(Q3DStudio::CString::fromQString(appDir.absolutePath()));
#else
    return CUICFile(Q3DStudio::CString::fromQString(qApp->applicationDirPath()));
#endif
}

//=============================================================================
/**
 * Create a temporary file from where the system holds it's temp files.
 * @param inExtension the file extension that should be used.
 */
CUICFile CUICFile::GetTemporaryFile(const Q3DStudio::CString &inExtension)
{
    QTemporaryFile tempFile(QDir::tempPath() + "/~uiXXXXXX" + inExtension.toQString());
    tempFile.setAutoRemove(false);
    tempFile.open(); // force creation of the actual file name
    return CUICFile(Q3DStudio::CString::fromQString(tempFile.fileName()));
}

CUICFile CUICFile::GetTemporaryFile()
{
    QTemporaryFile tempFile(QDir::tempPath() + "/~uiXXXXXX");
    tempFile.setAutoRemove(false);
    tempFile.open(); // force creation of the actual file name
    return CUICFile(Q3DStudio::CString::fromQString(tempFile.fileName()));
}

//=============================================================================
/**
 * Get the URL representing this file.
 */
QUrl CUICFile::GetURL() const
{
    return QUrl::fromLocalFile(m_Path.toQString());
}

//=============================================================================
/**
 * Request the filesystem to open this file in whatever manner it chooses with
 * the associated application.
 */
void CUICFile::Execute() const
{
    Q3DStudio::CString sFile = GetAbsolutePath();
    QUrl url = QUrl::fromLocalFile(sFile.toQString());
    QDesktopServices::openUrl(url);
}

//=============================================================================
/**
 * Combine the file and relative path together into another file.
 */
CUICFile CUICFile::Combine(const CUICFile &inBasePath, const Q3DStudio::CString &inRelativePath)
{
    QDir basePath(inBasePath.GetAbsolutePath().toQString());
    QString rel = basePath.absoluteFilePath(inRelativePath.toQString());
    return CUICFile(Q3DStudio::CString::fromQString(rel));
}

//=============================================================================
/**
 * Get a file handle for this file that can be used for reading.
 * The handle must be closed with CloseHandle when finished.
 */
HANDLE CUICFile::OpenFileReadHandle() const
{
    qFatal("implement me");
    return nullptr;
}

//=============================================================================
/**
 * Get a file handle for this file that can be used for writing.
 * The handle must be closed with CloseHandle when finished.
 */
HANDLE CUICFile::OpenFileWriteHandle() const
{
    qFatal("implement me");
    return nullptr;
}

//=============================================================================
/**
 * Clear all temp files that have been created so far by calling the GetTemporaryFile methods.
 * This would only clear the temp files that were created during the current program session,
 * and not any previous files created by anything prior to this session.
 */
void CUICFile::ClearCurrentTempCache()
{
    if (!s_TempFilePathList.empty()) {
        // Delete all temp files created so far
        for (auto file : s_TempFilePathList) {
            QFile::remove(file.toQString());
        }

        s_TempFilePathList.clear();
    }
}

void CUICFile::AddTempFile(const Q3DStudio::CString &inFile)
{
    s_TempFilePathList.insert(inFile);
}

//=============================================================================
/**
 * Checks if a path is relative or not.
 * Filename-only strings have no path separators and are considered relative.
 * @param inPath path to check
 * @return bool true to indicate this is a relative path
 */
bool CUICFile::IsPathRelative(const Q3DStudio::CString &inPath)
{
    QFileInfo info(inPath.toQString());
    return info.isRelative();
}

//=============================================================================
/**
 * Retrieves the file 'stat' struct that contains useful information.
 * @param inStat stat struct to fill
 * @return true if file stats are successfully obtained
 */
bool CUICFile::GetFileStat(struct _stat *inStat) const
{
#ifdef KDAB_TEMPORARILY_REMOVED
    return ::_wstat(m_Path, inStat) == 0;
#else
    return false;
#endif
}
