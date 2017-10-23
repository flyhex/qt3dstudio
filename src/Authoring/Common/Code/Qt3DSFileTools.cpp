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

#include "stdafx.h"
#include <string>
#include "Qt3DSFileTools.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "foundation/Qt3DSMath.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QCoreApplication>

// #ifndef __ATLMISC_H__
using qt3ds::NVMax;
using qt3ds::QT3DSU8;

namespace {
const Q3DStudio::CString updirSearch = L"\\..\\";
const Q3DStudio::CString curdirSearch = L"\\.\\";
#ifdef _WIN32
const Q3DStudio::CString updir = L"..\\";
const Q3DStudio::CString curdir = L".\\";
const wchar_t pathSep = '\\';
const Q3DStudio::CString pathSepStr = L"\\";
#else
const Q3DStudio::CString updir = L"../";
const Q3DStudio::CString curdir = L"./";
const wchar_t pathSep = '/';
const Q3DStudio::CString pathSepStr = L"/";
#endif
const Q3DStudio::CString UNCPathSepStr = L"\\\\";
#ifdef _WIN32
const wchar_t alternateSep = '/';
#else
const wchar_t alternateSep = '\\';
#endif
const Q3DStudio::CString fileUnsafeChars = L"\\/.:";
const wchar_t identifierSep = '#';
const Q3DStudio::CString veryLongPathPrefix = "\\\\?\\";

long FindPreviousPathSep(const Q3DStudio::CString &path)
{
    long startOff = Q3DStudio::CString::npos;
    // If the end of the path is a slash,
    if (path.Length() && path[path.Length() - 1] == pathSep)
        startOff = path.Length() - 2;
    return path.rfind(pathSep, startOff);
}

bool RecurseCreateDirectory(const Q3DStudio::CFilePath &dirName)
{
    return QDir().mkpath(dirName.toQString());
}
}

namespace Q3DStudio {

const CString CFilePathTokenizer::s_DirectoryDelimiters = L"\\/";

CFilePath::CFilePath(const CFilePath &str)
    : CString(str)
{
}

const CFilePath &CFilePath::operator=(const CFilePath &strSrc)
{
    CString::operator=(strSrc);
    return *this;
}

void CFilePath::RemoveTrailingBackslash()
{
    if (GetAt(Length() - 1) == QDir::separator().toLatin1())
        Assign(Extract(0, Length() - 1));
}

void CFilePath::AddTrailingBackslash()
{
    const auto separator = QDir::separator().toLatin1();
    if (GetAt(Length()) != separator)
        Concat(separator);
}

CString CFilePath::GetDrive() const
{
#ifdef _WIN32
    Q3DStudio::CString theReturnString;
    wchar_t theDrive[_MAX_DRIVE];

    // Remove the file name and extension
    _wsplitpath(GetPathWithoutIdentifier().c_str(), theDrive, NULL, NULL, NULL);

    theReturnString = theDrive;

    return theReturnString;
#else
    return CString();
#endif
}

CFilePath CFilePath::GetDirectory() const
{
    const CFilePath &path(*this);
    CFilePath::size_type pos = FindPreviousPathSep(path);
    if (pos == CFilePath::npos)
        return L"";
    if (pos == 1 && path[0] == '.') {
        if (path.size() > 2)
            return L".\\";
        return L"";
    }
    return path.substr(0, pos);
}

CString CFilePath::GetFileStem() const
{
    QFileInfo fi(QString::fromWCharArray(GetPathWithoutIdentifier().c_str()));
    return fi.baseName().toLatin1().data();
}

CString CFilePath::GetFileName() const
{
    CFilePath path(GetPathWithoutIdentifier());
    CFilePath::size_type pos = FindPreviousPathSep(path);
    if (pos == CFilePath::npos)
        return path;
    return path.substr(pos + 1);
}

CString DoGetExtension(const CFilePath &inPath)
{
    const CFilePath &fullName(inPath);
    CFilePath::size_type lastSlash = fullName.rfind(pathSep);
    CFilePath::size_type period = fullName.rfind('.');
    if (period != CFilePath::npos) {
        if (lastSlash == CFilePath::npos || period > lastSlash) {
            if (period != CFilePath::npos)
                return fullName.substr(period + 1);
        }
    }
    return L"";
}

CString CFilePath::GetExtension() const
{
    return DoGetExtension(GetPathWithoutIdentifier());
}

CString CFilePath::GetSuffix() const
{
    return DoGetExtension(*this);
}

// Get a file path you can give to systems that don't understand the identifier
CFilePath CFilePath::GetPathWithoutIdentifier() const
{
    long pos = rfind(identifierSep);
    if (pos != npos)
        return substr(0, pos);
    return *this;
}

// Set the identifer appended to the end of this file path.
void CFilePath::SetIdentifier(const CString &inIdentifier)
{
    CFilePath theCanonicalPath(GetPathWithoutIdentifier());
    if (inIdentifier.size()) {
        theCanonicalPath.append(identifierSep);
        theCanonicalPath.append(inIdentifier);
    }
    *this = theCanonicalPath;
}

// Get the identifier appended to the end of this file path.
CString CFilePath::GetIdentifier() const
{
    long pos = rfind(identifierSep);
    if (pos != npos)
        return substr(pos + 1);
    return Q3DStudio::CString();
}

unsigned long CFilePath::GetULIdentifier() const
{
    CString id(GetIdentifier());
    if (id.size() == 0)
        return 0;
    QT3DSU32 retval;
    WStrOps<QT3DSU32>().StrTo(id.GetCharStar(), retval);
    return retval;
}

void CFilePath::SetIdentifier(unsigned long inIdentifier)
{
    QT3DSU32 id(inIdentifier);
    wchar_t idBuf[16];
    WStrOps<QT3DSU32>().ToStr(id, qt3ds::foundation::toDataRef(idBuf, 16));
    SetIdentifier(idBuf);
}

bool CFilePath::GetTempDirectory()
{
    CString path = QDir::tempPath().toLatin1().data();
    StrAssign(path);

    return TRUE;
}

bool CFilePath::GetTemporaryFileName(const CString &inPrefix, const CString &inDirectory)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    wchar_t szPath[_MAX_PATH] = L"\0";
    wchar_t szFile[_MAX_PATH] = L"\0";

    if (inDirectory == NULL)
        ::GetTempPathW(sizeof(szPath), szPath);
    else
        wcscpy(szPath, inDirectory);

    ::GetTempFileNameW(szPath, inPrefix, 0, szFile);

    *this = CString(szFile);

    return TRUE;
#else
    return FALSE;
#endif
}

bool CFilePath::GetModuleFilePath(/*HMODULE inModuleHandle*/)
{
    StrAssign(CString::fromQString(qApp->applicationFilePath()));
    return true;
}

//==============================================================================
/**
*	Obtiains the sourcepath of the module who's currently running.  For example
*	the code currently exexuting might be within a DLL, but somebody may have
*	loaded that DLL, like Internet Explorer.
*/
bool CFilePath::GetContainerModuleFilePath()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    TCHAR theModuleFileName[_MAX_PATH];

    ::GetModuleFileName(NULL, theModuleFileName, sizeof(theModuleFileName));

    *this = theModuleFileName;

    return TRUE;
#else
    return FALSE;
#endif
}

//==============================================================================
/**
*	Helper function to strip of "file" protocol so that this is just a system file path
*/
void CFilePath::EnsureNonFileURL(Q3DStudio::CString &ioFilePath)
{
    if (ioFilePath.Find(L"file://") != Q3DStudio::CString::ENDOFSTRING) {
        ioFilePath.Delete(0, 7); // length of file protocol
    }
}

//==============================================================================
/**
*	Converts the absolute path this string represents into a relative path.
*   An absolute path is required as a base. If conversion is not sucessful, the
*	original absolute path is not changed.
*	@param inBaseAbsolute absolute path to base off from
*	@return bool to indicate successful conversion or not
*/
void CFilePath::ConvertToRelative(const CFilePath &inBasePath)
{
    if (inBasePath.IsEmpty()) // nothing to make relative to
        return;

    QT3DS_ASSERT(inBasePath.IsAbsolute());
    QT3DS_ASSERT(IsAbsolute());

    QDir basePathDir(inBasePath.toQString());
    QString relPath = basePathDir.relativeFilePath(toQString());
    *this = fromQString(relPath);
}

bool CFilePath::IsInSubDirectory(const CFilePath &inBasePath) const
{
    if (IsEmpty() || inBasePath.IsEmpty()) // nothing to compare
        return false;

    CFilePath thePath(GetPathWithoutIdentifier().c_str());

    // Get base directory if it is a file
    CFilePath theBaseDirectory(inBasePath.c_str());
    if (inBasePath.IsFile())
        theBaseDirectory = inBasePath.GetDirectory();

    // Remove any trailing backslash so that we are comparing on the same ground
    // just in case if we are comparing between same folder.
    thePath.RemoveTrailingBackslash();
    theBaseDirectory.RemoveTrailingBackslash();
    if (thePath == theBaseDirectory)
        return true;

    // Add trailing backslash because we want to compare different folder
    theBaseDirectory.AddTrailingBackslash();

    // Check if the path is in the base directory
    // Use caseless comparison because it's windows
    return ((thePath.Length() >= theBaseDirectory.Length())
            && theBaseDirectory.Compare(thePath.Extract(0, theBaseDirectory.Length()), false));
}

//==============================================================================
/**
*	Retrieves the current directory for the current process.
*	@return TRUE always
*/
bool CFilePath::GetCurrentDir()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    wchar_t szPath[_MAX_PATH] = L"\0";
    ::GetCurrentDirectoryW(sizeof(szPath) / sizeof(wchar_t), szPath);
    StrAssign(szPath);
#ifdef _WIN32
    *this += L"\\";
#else
    *this += L"/";
#endif //_WIN32
    return TRUE;
#else
    return FALSE;
#endif
}

//==============================================================================
/**
*	Resolves if this and inOtherPath are on the same drive.
*	@return true if this and inOtherPath are on the same drive.
*/
bool CFilePath::IsOnSameDrive(const CFilePath &inOtherPath) const
{
    CFilePath theOtherPath(inOtherPath);
    return GetDrive().Compare(theOtherPath.GetDrive(), false);
}

void CFilePath::Normalize()
{
    CString &path(*this);
    EnsureNonFileURL(path);
    CFilePath retval;
    retval.reserve(path.size());
    bool lastSlash = false;
    bool normalSep = false;
    size_t index = 0;
    for (CString::const_iterator iter = path.begin(), end = path.end(); iter != end;
         ++iter, ++index) {
        wchar_t current = *iter;
        if (current == pathSep || current == alternateSep) {
            if (lastSlash == false || (normalSep && (index < 2)))
                retval.Concat(pathSep);
            lastSlash = true;
            normalSep = current == pathSep;
        } else {
            normalSep = false;
            lastSlash = false;
            retval.Concat(current);
        }
    }
    // Resolve and remove ./ indicators that don't start at the beginning of the string
    for (long curdirPos = retval.find(curdirSearch); curdirPos != CString::npos;
         curdirPos = retval.find(curdirSearch, curdirPos))
        retval = retval.erase(curdirPos, 2);

    // Resolve updir indicators until we can't any more
    for (long updirPos = retval.find(updirSearch); updirPos != CString::npos;
         updirPos = retval.find(updirSearch, updirPos)) {
        long lastSlash = CString::npos;
        // catch ./../
        if (updirPos < 2) {
            retval = retval.erase(0, updirPos + 1);
            break;
        }
        // for all other ../ conditions
        else {
            lastSlash = retval.rfind(pathSep, updirPos - 1);
            if (lastSlash == CString::npos)
                break;
            retval = retval.erase(lastSlash, updirPos - lastSlash + 3);
            updirPos = lastSlash;
        }
    }
    *this = retval;
}

bool CFilePath::IsNormalized() const
{
    const CFilePath &path(*this);
#ifdef _WIN32
    if (path.find('/') != CFilePath::npos)
#else
    if (path.find('\\') != CFilePath::npos)
#endif
        return false;

    CFilePath::size_type findResult = path.find(updir);
    if (findResult != 0 && findResult != CFilePath::npos)
        return false;

    findResult = path.find(UNCPathSepStr);
    if (findResult != 0 && findResult != CFilePath::npos)
        return false;

    return true;
}

bool CFilePath::IsAbsolute() const
{
    const CString &path(*this);
    if (path.Length() > 1) {
#ifdef _WIN32
        if (path[1] == ':')
            return true;
        if (path[0] == path[1] && path[1] == pathSep)
            return true;
#else
        if (path[0] == pathSep)
            return true;
#endif
    }
    return false;
}
void CFilePath::CombineBaseAndRelative(const CFilePath &basePath)
{
    QT3DS_ASSERT(basePath.IsAbsolute());
    CFilePath &relativePath(*this);
    if (relativePath.IsAbsolute())
        return;
    Q3DStudio::CFilePath retval;
    retval.reserve(basePath.Length() + relativePath.Length());
    retval.assign(basePath);
    if (retval.Length() && retval[retval.Length() - 1] != '\\')
        retval.append(pathSepStr);
    retval.append(relativePath);
    retval.Normalize();
    *this = retval;
}
// If we are absolute, we are done.
// If not, ConvertToAbsolute using getcwd.
void CFilePath::ConvertToAbsolute()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    if (IsAbsolute())
        return;
    wchar_t pathBuf[1024];
    _wgetcwd(pathBuf, 1024);
    CombineBaseAndRelative(CString(pathBuf));
#endif
}
// Make a save file stem from this string.  Involves replacing characters
// that are illegal (:,\\,//,etc).
CString CFilePath::MakeSafeFileStem(const CString &name)
{
    CFilePath retval;
    retval.reserve(name.Length());
    for (Q3DStudio::CString::const_iterator iter = name.begin(), end = name.end(); iter != end;
         ++iter) {
        wchar_t strItem(*iter);
        if (fileUnsafeChars.find_first_of(strItem) != Q3DStudio::CString::npos)
            retval.append(L"_", 1);
        else
            retval.append(&strItem, 1);
    }
    return retval;
}

// Create this directory, recursively creating parent directories
// if necessary.
bool CFilePath::CreateDir(bool recurse) const
{
    QDir d(GetPathWithoutIdentifier().toQString());
    if (d.exists())
        return true;

    if (recurse)
        return RecurseCreateDirectory(CString(GetPathWithoutIdentifier().c_str()));
    d.cdUp();
    return d.mkdir(QFileInfo(GetPathWithoutIdentifier().toQString()).fileName());
}
// Returns true if exists and is directory
bool CFilePath::IsDirectory() const
{
    return GetFileFlags().IsDirectory();
}
// returns true if exists and is a file
bool CFilePath::IsFile() const
{
    return GetFileFlags().IsFile();
}
// If the file doesn't exist, create it.
// If it does exist, update its modification time.
void CFilePath::Touch() const
{
    QFile f(GetPathWithoutIdentifier().toQString());
    f.open(QIODevice::ReadWrite);
}
// Returns true if this exists on the filesystem and is a directory or file.
bool CFilePath::Exists() const
{
    return GetFileFlags().Exists();
}

// Delete this file from the filesystem
bool CFilePath::DeleteThisFile()
{
    return QFile::remove(QString::fromWCharArray(GetPathWithoutIdentifier().c_str())) != 0;
}
// Delete this directory
bool CFilePath::DeleteThisDirectory(bool recursive)
{
    bool retval = true;
    if (recursive) {
        std::vector<CFilePath> subFiles;
        ListFilesAndDirectories(subFiles);
        for (size_t idx = 0; idx < subFiles.size(); ++idx) {
            if (subFiles[idx].IsFile())
                retval = retval && subFiles[idx].DeleteThisFile();
            else
                retval = retval && subFiles[idx].DeleteThisDirectory(true);
        }
    }
    QDir dir(QString::fromWCharArray(GetPathWithoutIdentifier().c_str()));
    dir.cdUp();
    retval = retval && dir.rmdir(QDir(QString::fromWCharArray(GetPathWithoutIdentifier().c_str())).dirName());
    return retval;
}

void CFilePath::ListFilesAndDirectories(std::vector<CFilePath> &files) const
{
    if (!IsDirectory()) {
        return;
    }
    CString findPath(GetPathWithoutIdentifier());
    QDirIterator di(findPath.toQString(), QDir::NoDotAndDotDot | QDir::AllEntries);
    while (di.hasNext())
        files.push_back(CString::fromQString(di.next()));
}

bool CFilePath::FindLatestModifiedFileInDirectory(CFilePath &file) const
{
    if (!IsDirectory()) {
        return false;
    }

    QDateTime theLatestFileTime;
    bool foundFile = false;

    CString findPath(GetPathWithoutIdentifier());
    QDirIterator di(findPath.toQString(), QDir::NoDotAndDotDot | QDir::AllEntries);
    while (di.hasNext()) {
        auto ffd = di.fileInfo();
        if (theLatestFileTime < ffd.lastModified()) {
            theLatestFileTime = ffd.lastModified();
            file = CString::fromQString(ffd.absoluteFilePath());
            foundFile = true;
        }
    }

    return foundFile;
}

void CFilePath::RecursivelyFindFilesOfType(const wchar_t **inExtensionList,
                                           std::vector<CFilePath> &files, bool inMakeRelative,
                                           bool inIncludeDirectories) const
{
    if (!IsDirectory()) {
        QT3DS_ASSERT(false);
        return;
    }
    std::vector<CFilePath> directoryVector;
    std::vector<CFilePath> nextDirectoryVector;
    std::vector<CFilePath> resultsVector;

    directoryVector.push_back(*this);
    // breadth first search.
    while (directoryVector.empty() == false && files.size() < 10000) {
        for (size_t dirIdx = 0, dirEnd = directoryVector.size();
             dirIdx < dirEnd && files.size() < 10000; ++dirIdx) {
            resultsVector.clear();
            directoryVector[dirIdx].ListFilesAndDirectories(resultsVector);
            for (size_t resultIdx = 0, resultEnd = resultsVector.size(); resultIdx < resultEnd;
                 ++resultIdx) {
                const CFilePath &result(resultsVector[resultIdx]);
                if (result.IsDirectory()) {
                    nextDirectoryVector.push_back(result);
                    if (inIncludeDirectories)
                        files.push_back(result);
                } else {
                    Q3DStudio::CString extension(result.GetExtension());
                    if (inExtensionList == NULL || *inExtensionList == NULL) {
                        files.push_back(result);
                    } else {
                        for (const wchar_t **specificExt = inExtensionList; *specificExt;
                             ++specificExt) {
                            if (extension == *specificExt) {
                                files.push_back(result);
                                break;
                            }
                        }
                    }
                }
            }
        }
        std::swap(directoryVector, nextDirectoryVector);
        nextDirectoryVector.clear();
    }
    std::stable_sort(files.begin(), files.end());
    if (inMakeRelative) {
        for (size_t idx = 0, end = files.size(); idx < end; ++idx)
            files[idx] = GetRelativePathFromBase(*this, files[idx]);
    }
}

void CFilePath::FindDirectoryDifferences(
    const std::vector<SFileModificationRecord> &inOldDifferences,
    std::vector<SFileModificationRecord> &outNewDifferences, volatile bool *inCancel) const
{
    std::vector<SFileModificationRecord> theOutput;
    std::vector<CFilePath> theFiles;

    // Assume we will have about as many results as last time.
    theFiles.reserve(inOldDifferences.size());
    if (IsDirectory())
        RecursivelyFindFilesOfType(NULL, theFiles, false, true);

    // Else we have no files and the new file list should be empty
    // This will send out destroyed messages for every file in the list
    // once.

    theOutput.reserve(NVMax(inOldDifferences.size(), theFiles.size()));
    // Given that we know theFiles are sorted and inOldDifferences is sorted,
    // run through inOldDifferences and theFiles exactly one, putting new values into theOutput.
    // Algorithm below is O(max(inOldDifferences.size(), theFiles.size()) )

    size_t numOldDifferences = inOldDifferences.size();
    size_t numFiles = theFiles.size();
    size_t oldDiffIdx = 0;
    size_t fileIdx = 0;
    volatile bool theLocalCancel = false;
    if (inCancel == NULL)
        inCancel = &theLocalCancel;

    while (oldDiffIdx < numOldDifferences && fileIdx < numFiles && !*inCancel) {
        const SFileModificationRecord &oldRecord = inOldDifferences[oldDiffIdx];
        const CFilePath &theFile = theFiles[fileIdx];
        if (oldRecord.m_File < theFile) {
            // Stop recording destroyed files if the message has been sent once
            if (oldRecord.m_ModificationType != FileModificationType::Destroyed)
                theOutput.push_back(SFileModificationRecord(oldRecord.m_File, oldRecord.m_FileInfo,
                                                            oldRecord.m_FileData,
                                                            FileModificationType::Destroyed));

            ++oldDiffIdx;
        } else {
            SFileData newData;
            if (theFile.IsFile())
                newData = theFile.GetFileData();

            SFileInfoFlags theInfo = theFile.GetFileFlags();
            FileModificationType::Enum theFileModType = FileModificationType::NoChange;
            if (theFile < oldRecord.m_File) {
                theFileModType = FileModificationType::Created;
                ++fileIdx;
            } else // We have matching files
            {
                if (oldRecord.m_ModificationType == FileModificationType::Destroyed)
                    theFileModType = FileModificationType::Created;
                else if (newData.m_LastModTime != oldRecord.m_FileData.m_LastModTime)
                    theFileModType = FileModificationType::Modified;
                else if (theInfo != oldRecord.m_FileInfo)
                    theFileModType = FileModificationType::InfoChanged;

                ++fileIdx;
                ++oldDiffIdx;
            }
            theOutput.push_back(SFileModificationRecord(theFile, theInfo, newData, theFileModType));
        }
    }
    for (; oldDiffIdx < numOldDifferences && !*inCancel; ++oldDiffIdx) {
        const SFileModificationRecord &oldRecord = inOldDifferences[oldDiffIdx];
        if (oldRecord.m_ModificationType != FileModificationType::Destroyed)
            theOutput.push_back(SFileModificationRecord(oldRecord.m_File, oldRecord.m_FileInfo,
                                                        oldRecord.m_FileData,
                                                        FileModificationType::Destroyed));
    }
    for (; fileIdx < numFiles && !*inCancel; ++fileIdx) {
        const CFilePath &theFile = theFiles[fileIdx];
        theOutput.push_back(SFileModificationRecord(
            theFile, theFile.GetFileFlags(), theFile.GetFileData(), FileModificationType::Created));
    }
    std::swap(theOutput, outNewDifferences);
}

SFileInfoFlags CFilePath::GetFileFlags() const
{
    QFileInfo fi(GetPathWithoutIdentifier().toQString());
    if (!fi.exists())
        return 0; // doesn't exist
    int atts = FileInfoFlagValues::Exists;
    if (fi.isDir())
        atts += FileInfoFlagValues::IsDirectory;
    else {
        atts += FileInfoFlagValues::CanRead;
        if (fi.isWritable())
            atts += FileInfoFlagValues::CanWrite;
    }
    if (fi.isHidden())
        atts += FileInfoFlagValues::IsHidden;
    return atts;
}

SFileData CFilePath::GetFileData() const
{
#ifdef KDAB_TEMPORARILY_REMOVED
    HANDLE file = CreateFileW(GetPathWithoutIdentifier().c_str(), GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return SFileData();
    StaticAssert<sizeof(FILETIME) == sizeof(QT3DSU64)>::valid_expression();
    QT3DSU64 create;
    QT3DSU64 lastWrite;
    GetFileTime(file, reinterpret_cast<FILETIME *>(&create), NULL,
                reinterpret_cast<FILETIME *>(&lastWrite));

    // LE code alert
    QT3DSU64 fileSize = 0;
    DWORD *sizePtr(reinterpret_cast<DWORD *>(&fileSize));
    sizePtr[0] = GetFileSize(file, sizePtr + 1);
    CloseHandle(file);
    return SFileData(fileSize, lastWrite, create);
#else
    return {};
#endif
}

CFilePath CFilePath::GetUserApplicationDirectory()
{
    return CFilePath(CString::fromQString(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))));
}

SFile::SFile(const QSharedPointer<QFile> &of, const CFilePath &path)
    : m_OpenFile(of)
    , m_Path(path)
{
}
SFile::~SFile()
{
    if (m_OpenFile)
        Close(m_OpenFile);
}
QT3DSU32 SFile::Read(void *buffPtr, QT3DSU32 byteSize)
{
    return ReadData(m_OpenFile, buffPtr, byteSize);
}
QT3DSU32 SFile::Write(const void *buffPtr, QT3DSU32 byteSize)
{
    return WriteData(m_OpenFile, buffPtr, byteSize);
}

QSharedPointer<QFile> SFile::OpenForRead(const CFilePath &inPath)
{
    QSharedPointer<QFile> f(new QFile(inPath.toQString()));
    if (!f->open(QFile::ReadOnly)) {
        return nullptr;
    }
    return f;
}

QSharedPointer<QFile> SFile::OpenForWrite(const CFilePath &inFullPath, FileOpenFlags fileFlags)
{
    QFile::OpenMode mode = QIODevice::ReadWrite;
    if (fileFlags & FileOpenFlagValues::Truncate)
        mode |= QIODevice::Truncate;
    bool truncate = (QT3DSU32)((fileFlags & FileOpenFlagValues::Truncate)) != 0;
    bool open = (QT3DSU32)((fileFlags & FileOpenFlagValues::Open)) != 0;
    bool create = (QT3DSU32)((fileFlags & FileOpenFlagValues::Create)) != 0;

    QSharedPointer<QFile> file(new QFile(inFullPath.GetPathWithoutIdentifier().toQString()));
    if (!create && !file->exists() || !file->open(mode)) {
        return nullptr;
    }
    // If we aren't truncating, then we seek to the end to append.
    if (truncate == false && open == true) {
        file->seek(file->size());
    }
    return file;
}

// Copy src to dest, close both src and dest, and
// return the number of bytes copied.
QT3DSU64 SFile::Copy(const QSharedPointer<QFile> &destFile, const QSharedPointer<QFile> &srcFile)
{
    QT3DSU8 buffer[4096];
    QT3DSU32 numBytes = 0;
    QT3DSU64 total = 0;
    do {
        numBytes = ReadData(srcFile, buffer, 4096);
        WriteData(destFile, buffer, numBytes);
        total += numBytes;
    } while (numBytes == 4096);
    Close(srcFile);
    Close(destFile);
    return total;
}

// Attemt to write data, return the number of bytes written.
QT3DSU32 SFile::WriteData(const QSharedPointer<QFile> &fileHandle, const void *data, QT3DSU32 byteSize)
{
    QT3DS_ASSERT(fileHandle);
    if (byteSize && fileHandle) {
        return fileHandle->write(reinterpret_cast<const char *>(data), byteSize);
    }
    return 0;
}

// Attempt to read data, return the number of byte read
QT3DSU32 SFile::ReadData(const QSharedPointer<QFile> &fileHandle, void *data, QT3DSU32 byteSize)
{
    QT3DS_ASSERT(fileHandle);
    if (byteSize && fileHandle) {
        return fileHandle->read(reinterpret_cast<char *>(data), byteSize);
    }
    return 0;
}

DWORD SeekPosToMoveMethod(SeekPosition::Enum inEnum)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    switch (inEnum) {
    case SeekPosition::Begin:
        return FILE_BEGIN;
    case SeekPosition::Current:
        return FILE_CURRENT;
    case SeekPosition::End:
        return FILE_END;
    }
    QT3DS_ASSERT(false);
    return FILE_BEGIN;
#endif
    return {};
}

QT3DSI64 SFile::GetPosition(const QSharedPointer<QFile> &fileHandle)
{
    QT3DS_ASSERT(fileHandle);
    if (fileHandle) {
        return fileHandle->pos();
    }
    return 0;
}

void SFile::SetPosition(const QSharedPointer<QFile> &fileHandle, QT3DSI64 inOffset, SeekPosition::Enum inSeekPos)
{
    QT3DS_ASSERT(fileHandle);
    if (fileHandle) {
        switch (inSeekPos) {
        case SeekPosition::Begin:
            fileHandle->seek(inOffset);
            break;
        case SeekPosition::Current:
            fileHandle->seek(fileHandle->pos() + inOffset);
            break;
        case SeekPosition::End:
            fileHandle->seek(fileHandle->size() + inOffset);
            break;
        }
    }
}

void SFile::SetFileTimeToCurrentTime(const QSharedPointer<QFile> &fileHandle)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    FILETIME ft;
    SYSTEMTIME st;

    GetSystemTime(&st); // Gets the current system time
    SystemTimeToFileTime(&st, &ft); // Converts the current system time to file time format
    SetFileTime((HANDLE)fileHandle, // Sets last-write time of the file
                (LPFILETIME)NULL, // to the converted current system time
                (LPFILETIME)NULL, &ft);
#endif
}

// Close the file handle.
void SFile::Close(const QSharedPointer<QFile> &fileHandle)
{
    QT3DS_ASSERT(fileHandle);
    fileHandle->close();
}

std::shared_ptr<SFile> SFile::Wrap(const QSharedPointer<QFile> &inFileHandle, const CFilePath &path)
{
    return std::make_shared<SFile>(inFileHandle, path);
}

// Open the file and return an os-specific handle.
// in windows, this uses CreateFile and returns the file handle.
// It returns the opened file because this eliminates a race condition where
// this function returns the file name at the same time another process returns
// the file name and then we have two objects thinking they own the file.
// Function appends an index in betwee nthe file stem and the extension
// in order to generate a possibly unique name.
// params:
// inDestDir -> points to destination directory (and is a directory)
// fstem -> file stem to write to
// inExt -> extension to use after stem + unique append apparatus.
TFilePtr SFileTools::FindUniqueDestFile(const CFilePath &inDestDirectory, const CString &inFStem,
                                        const CString &inExt)
{
    if (!inDestDirectory.Exists())
        inDestDirectory.CreateDir(true);
    if (!inDestDirectory.Exists())
        return TFilePtr();
    wchar_t period = '.';
    Q3DStudio::CString stem;
    stem.reserve(inDestDirectory.Length() + inFStem.Length() + 1);
    stem.assign(inDestDirectory);
    if (stem.Length() && stem[stem.Length() - 1] != '\\')
        stem.append(&pathSep, 1);
    stem.append(inFStem);

    Q3DStudio::CString retval;
    retval.reserve(stem.Length() + inExt.Length() + 10);
    retval.assign(stem);

    retval.append(&period, 1);
    retval.append(inExt);

    // Force creation of new file or failure.
    auto handle = SFile::OpenForWrite(retval, FileOpenFlagValues::Create);
    QT3DSU32 idx = 1;
    while (handle == NULL && idx < 1000) {
        wchar_t buffer[10];
        QT3DSU32 numChars = swprintf(buffer, 10, L"_%03d", idx);
        ++idx;

#ifdef _WIN32
        // On windows we need to limit the path
        if (stem.size() > MAX_PATH) {
            retval.assign(veryLongPathPrefix);
            retval.append(stem, MAX_PATH - (inDestDirectory.Length() + 10));
        } else
#endif
            retval.assign(stem);

        retval.append(buffer, numChars);
        retval.append(&period, 1);
        retval.append(inExt);
        handle = SFile::OpenForWrite(retval, FileOpenFlagValues::Create);
    }
    if (handle == NULL) {
        retval.assign(inFStem);
        retval.append(L"_999", 4);
        return FindUniqueDestFile(inDestDirectory, retval, inExt);
    }
    return std::make_shared<SFile>(handle, retval);
}

QString SFileTools::FindUniqueDestFile(const QDir &inDestDirectory, const QString &inFStem,
                                       const QString &inExt)
{
    if (!inDestDirectory.exists())
        return {};

    QString retval = QStringLiteral("%1.%2").arg(inFStem, inExt);
    int idx = 1;
    while (inDestDirectory.exists(retval) && idx < 1000) {
        retval = QStringLiteral("%1_%2.%3").arg(inFStem).arg(idx, 3, 10, QLatin1Char('0')).arg(inExt);
        ++idx;
    }
    if (inDestDirectory.exists(retval)) {
        return FindUniqueDestFile(inDestDirectory, inFStem + QLatin1String("_999"), inExt);
    }

    return inDestDirectory.filePath(retval);
}

// Similar to above but takes care of finding the stem and extension from the full source CFilePath
TFilePtr SFileTools::FindUniqueDestFile(const CFilePath &inDestDir,
                                        const CFilePath &inSrcFullFilePath)
{
    return FindUniqueDestFile(inDestDir, inSrcFullFilePath.GetFileStem(),
                              inSrcFullFilePath.GetExtension());
}

QString SFileTools::FindUniqueDestFile(const QDir &inDestDir,
                                       const QString &inSrcFullFilePath)
{
    QFileInfo fileInfo(inSrcFullFilePath);
    return FindUniqueDestFile(inDestDir, fileInfo.completeBaseName(), fileInfo.suffix());
}

// Find a unique destination directory.  This directory was guaranteed not to exist before this call
// and we are guaranteeing that it was created for this call; i.e. there can't be a race condition
// We expect dest dir and dir name to be normalized.
// We also expect that inDestDirectory exists.  Function results are L"" otherwise
// If the function succeeds, it returns the absolute CFilePath of the new directory.
// If it fails, it returns L"";
CFilePath SFileTools::FindUniqueDestDirectory(const CFilePath &inDestDirectory,
                                              const CString &inDirName)
{
    if (inDestDirectory.IsDirectory() == false) {
        QT3DS_ASSERT(false);
        return L"";
    }

    Q3DStudio::CString retval;
    retval.reserve(inDestDirectory.Length() + inDirName.Length() + 2);
    retval = CFilePath::CombineBaseAndRelative(inDestDirectory, inDirName);
#ifdef KDAB_TEMPORARILY_REMOVED
    BOOL success = ::CreateDirectoryW(retval, NULL);
    if (success)
        return CFilePath::GetAbsolutePath(retval);
    DWORD error = GetLastError();
    if (error == ERROR_PATH_NOT_FOUND) {
        QT3DS_ASSERT(false);
        return L"";
    }
    QT3DSU32 idx = 0;
    Q3DStudio::CString stem(retval);
    // Eventually we will succeed
    while (success == FALSE) {
        ++idx;
        retval = stem;
        wchar_t buffer[10] = { 0 };
        swprintf(buffer, 10, L"_%03d", idx);
        retval.append(buffer);
        success = ::CreateDirectoryW(retval, NULL);
    }
#endif
    return CFilePath::GetAbsolutePath(retval);
}

QDir SFileTools::FindUniqueDestDirectory(const QDir &inDestDirectory,
                                         const QString &inDirName)
{
    if (inDestDirectory.exists() == false) {
        QT3DS_ASSERT(false);
        return {};
    }

    QString retval = inDirName;

    if (inDestDirectory.exists(retval) == true) {
        int idx = 0;
        // Eventually we will succeed
        while (true) {
            ++idx;
            retval = QStringLiteral("%1_%2").arg(inDirName).arg(idx, 3, 10, QLatin1Char('0'));
            if (inDestDirectory.exists(retval) == false)
                break;
        }
    }

    inDestDirectory.mkdir(retval);

    return QDir(inDestDirectory.filePath(retval));
}

// Copy the full file FilePaths.
// Same file flags as OpenFileForWrite
SFileErrorCodeAndNumBytes SFileTools::Copy(const CFilePath &destFile, FileOpenFlags dstFileFlags,
                                           const CFilePath &srcFile)
{
    QFileInfo srcInfo(srcFile.toQString());
    if (!srcInfo.exists())
        return FileErrorCodes::SourceNotExist;

    if (!srcInfo.isReadable())
        return FileErrorCodes::SourceNotReadable;

    QString destFileString = destFile.toQString();
    if (QFile::exists(destFileString)) {
        bool ok = QFile::remove(destFileString);
        if (!ok)
            return FileErrorCodes::DestNotWriteable;
    }

    bool ok = QFile::copy(srcFile.toQString(), destFileString);
    if (ok) {
        QFileInfo destInfo(destFile.toQString());
        return destInfo.size();
    }

    return FileErrorCodes::DestNotWriteable;
}

// Find a unique dest file based on the src file stem and extension but in the destination directory
// then copy that file.  Return the file name
// this -> points to destination directory
// srcFile -> src file to copy
SFileErrorCodeFileNameAndNumBytes SFileTools::FindAndCopyDestFile(const CFilePath &inDestDir,
                                                                  const CFilePath &inSrcFile)
{
    TFilePtr dest(FindUniqueDestFile(inDestDir, inSrcFile));
    if (dest == NULL)
        return SFileErrorCodeFileNameAndNumBytes(FileErrorCodes::DestNotWriteable);
    auto srcFile = SFile::OpenForRead(inSrcFile);
    if (srcFile == NULL)
        return SFileErrorCodeFileNameAndNumBytes(FileErrorCodes::SourceNotReadable, 0,
                                                 dest->m_Path);
    else {
        QT3DSU64 nb = SFile::Copy(dest->m_OpenFile, srcFile);
        dest->m_OpenFile = NULL;
        return SFileErrorCodeFileNameAndNumBytes(FileErrorCodes::NoError, nb, dest->m_Path);
    }
}

bool SFileTools::FindAndCopyDestFile(const QDir &inDestDir,
                                     const QString &inSrcFile)
{
    QString dest = FindUniqueDestFile(inDestDir, inSrcFile);
    if (dest.isEmpty())
        return false;
    return QFile::copy(inSrcFile, dest);
}

QDebug operator<<(QDebug stream, const CFilePath &s)
{
    stream << "CFilePath(" << s.toQString() << ")";
    return stream;
}

}
