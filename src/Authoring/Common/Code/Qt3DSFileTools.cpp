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

#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSFileTools.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "foundation/Qt3DSMath.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qcoreapplication.h>

using qt3ds::QT3DSU8;

namespace {
const QChar identifierSep = '#';
const QChar illegalChar = ':'; // Illegal character in a file name on Windows, so disallow it for all
const QChar replaceChar = '_'; // Replace illegal chars with this
Q3DStudio::CString veryLongPathPrefix = "\\\\?\\";

/* From QDir::fromNativeSeparators */
QString fromWindowsSeparators(const QString &pathName)
{
    int i = pathName.indexOf(QLatin1Char('\\'));
    if (i != -1) {
        QString n(pathName);

        QChar * const data = n.data();
        data[i++] = QLatin1Char('/');

        for (; i < n.length(); ++i) {
            if (data[i] == QLatin1Char('\\'))
                data[i] = QLatin1Char('/');
        }

        return n;
    }
    return pathName;
}

/*
 *  This function cleans up the path references in older uip files created by
 *  NDD when the tool was windows specific.
 */
QString normalizeAndCleanPath(const QString &path)
{
    QString ret = path;
    if (ret.startsWith("file://"))
        ret = ret.remove(0, 7);
    if (ret.startsWith("file:\\"))
        ret = ret.remove(0, 6);

    ret = fromWindowsSeparators(ret);
    ret = QDir::cleanPath(ret);
    return ret;
}
}

namespace Q3DStudio {

CFilePath::CFilePath() : QFileInfo() {}

CFilePath::CFilePath(const wchar_t *path)
    : QFileInfo()
{
    normalizeAndSetPath(QString::fromWCharArray(path));
}

CFilePath::CFilePath(const char *path)
    : QFileInfo()
{
    normalizeAndSetPath(path);
}

CFilePath::CFilePath(const CString &path)
    : QFileInfo()
{
    normalizeAndSetPath(path.toQString());
}

CFilePath::CFilePath(const QString &path)
    : QFileInfo()
{
    normalizeAndSetPath(path);
}

void CFilePath::normalizeAndSetPath(const QString &path)
{
    QString ret = normalizeAndCleanPath(path);

    if (ret.contains(identifierSep)) {
        int i = ret.indexOf(identifierSep);
        m_identifier = ret.mid(i + 1);
        ret.truncate(i);
    }

    setFile(ret);
}

CFilePath CFilePath::GetDirectory() const
{
    return CFilePath(path());
}

CString CFilePath::GetFileStem() const
{
    return CString::fromQString(completeBaseName());
}

CString CFilePath::GetFileName() const
{
    return CString::fromQString(fileName());
}

CString CFilePath::GetExtension() const
{
    return CString::fromQString(suffix());
}

CString CFilePath::GetPathWithIdentifier() const
{
    if (m_identifier.isEmpty())
        return CString::fromQString(filePath());

    return CString::fromQString(filePath() + identifierSep + m_identifier);
}

CString CFilePath::GetModuleFilePath()
{
    return CString::fromQString(qApp->applicationFilePath());
}

//==============================================================================
/**
*	Converts the absolute path this string represents into a relative path.
*   An absolute path is required as a base. If conversion is not sucessful, the
*	original absolute path is not changed.
*	@param inBaseAbsolute absolute path to base off from
*	@return bool to indicate successful conversion or not
*/
void CFilePath::ConvertToRelative(const CFilePath &basePath)
{
    QT3DS_ASSERT(basePath.isAbsolute());
    QT3DS_ASSERT(!isRelative());

    QDir basePathDir = basePath.absoluteFilePath();
    QString relPath = basePathDir.relativeFilePath(absoluteFilePath());
    setFile(QDir::cleanPath(relPath));
}

bool CFilePath::IsInSubDirectory(const CFilePath &basePath) const
{
    QT3DS_ASSERT(basePath.isAbsolute());

    QDir basePathDir = basePath.canonicalFilePath();
    return basePathDir.exists()
        && basePathDir.exists(filePath());
}

bool CFilePath::IsAbsolute() const
{
    return isAbsolute();
}

void CFilePath::CombineBaseAndRelative(const CFilePath &basePath)
{
    QT3DS_ASSERT(basePath.isAbsolute());
    QT3DS_ASSERT(isRelative());
    QDir basePathDir = basePath.absoluteFilePath();
    QString absPath = basePathDir.absoluteFilePath(filePath());
    setFile(QDir::cleanPath(absPath));
}

bool CFilePath::ConvertToAbsolute()
{
    return makeAbsolute();
}

CString CFilePath::MakeSafeFileStem(const CString &name)
{
    QString ret = normalizeAndCleanPath(name.toQString());

    ret = ret.replace(illegalChar, replaceChar);

    return CString::fromQString(ret);
}

bool CFilePath::CreateDir(bool recurse) const
{
    QDir d(filePath());
    if (d.exists())
        return true;

    if (recurse)
        return d.mkpath(filePath());
    d.cdUp();
    return d.mkdir(fileName());
}

bool CFilePath::IsDirectory() const
{
    return isDir();
}

bool CFilePath::IsFile() const
{
    return isFile();
}

void CFilePath::Touch() const
{
    QFile f(filePath());
    f.open(QIODevice::ReadWrite);
}

bool CFilePath::Exists() const
{
    return exists();
}

bool CFilePath::DeleteThisFile()
{
    return QFile::remove(filePath());
}

bool CFilePath::DeleteThisDirectory(bool recurse)
{
    QDir d(filePath());
    if (!d.exists())
        return true;

    if (recurse)
        return d.rmpath(filePath());
    d.cdUp();
    return d.mkdir(fileName());
}

void CFilePath::ListFilesAndDirectories(std::vector<CFilePath> &files) const
{
    if (!IsDirectory())
        return;

    CString findPath = CString::fromQString(filePath());
    QDirIterator di(findPath.toQString(), QDir::NoDotAndDotDot | QDir::AllEntries);
    while (di.hasNext())
        files.push_back(CString::fromQString(di.next()));
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

SFileInfoFlags CFilePath::GetFileFlags() const
{
    if (!exists())
        return 0; // doesn't exist
    int atts = FileInfoFlagValues::Exists;
    if (isDir())
        atts += FileInfoFlagValues::IsDirectory;
    else {
        atts += FileInfoFlagValues::CanRead;
        if (isWritable())
            atts += FileInfoFlagValues::CanWrite;
    }
    if (isHidden())
        atts += FileInfoFlagValues::IsHidden;
    return atts;
}

SFileData CFilePath::GetFileData() const
{
    SFileData data;
    data.m_CreateTime = birthTime().toSecsSinceEpoch();
    data.m_LastModTime = lastModified().toSecsSinceEpoch();
    data.m_Length = size();
    return data;
}

CFilePath CFilePath::GetUserApplicationDirectory()
{
    return CFilePath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
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
    QSharedPointer<QFile> f(new QFile(inPath.filePath()));
    if (!f->open(QFile::ReadOnly))
        return nullptr;
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

    QSharedPointer<QFile> file(new QFile(inFullPath.filePath()));
    if ((!create && !file->exists()) || !file->open(mode))
        return nullptr;
    // If we aren't truncating, then we seek to the end to append.
    if (truncate == false && open == true)
        file->seek(file->size());
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
        default:
            // Ignore unknown seek position
            break;
        }
    }
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
                                        const CString &inExt, bool truncate)
{
    if (!inDestDirectory.Exists())
        inDestDirectory.CreateDir(true);
    if (!inDestDirectory.Exists())
        return TFilePtr();
    wchar_t period = '.';
    const wchar_t pathSep = '/';
    Q3DStudio::CString stem;
    Q3DStudio::CString inDestDirectoryPath
        = CString::fromQString(inDestDirectory.filePath());
    stem.reserve(inDestDirectoryPath.Length() + inFStem.Length() + 1);
    stem.assign(inDestDirectoryPath);
    if (stem.Length() && stem[stem.Length() - 1] != '\\')
        stem.append(&pathSep, 1);
    stem.append(inFStem);

    Q3DStudio::CString retval;
    retval.reserve(stem.Length() + inExt.Length() + 10);
    retval.assign(stem);

    retval.append(&period, 1);
    retval.append(inExt);

    // Force creation of new file or failure.
    FileOpenFlags flags = FileOpenFlagValues::Create;
    if (truncate)
        flags |= FileOpenFlagValues::Truncate;
    auto handle = SFile::OpenForWrite(retval, flags);
    QT3DSU32 idx = 1;
    while (handle == NULL && idx < 1000) {
        wchar_t buffer[10];
        QT3DSU32 numChars = swprintf(buffer, 10, L"_%03d", idx);
        ++idx;

#ifdef _WIN32
        // On windows we need to limit the path
        if (stem.size() > MAX_PATH) {
            retval.assign(veryLongPathPrefix);
            retval.append(stem, MAX_PATH - (inDestDirectoryPath.Length() + 10));
        } else
#endif
        {
            retval.assign(stem);
        }

        retval.append(buffer, numChars);
        retval.append(&period, 1);
        retval.append(inExt);
        handle = SFile::OpenForWrite(retval, flags);
    }
    if (handle == NULL) {
        retval.assign(inFStem);
        retval.append(L"_999", 4);
        return FindUniqueDestFile(inDestDirectory, retval, inExt);
    }
    return std::make_shared<SFile>(handle, retval);
}

QString SFileTools::FindUniqueDestFile(const QDir &inDestDirectory, const QString &inFStem,
                                       const QString &inExt, bool truncate)
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
        return FindUniqueDestFile(inDestDirectory, inFStem + QLatin1String("_999"), inExt,
                                  truncate);
    }

    return inDestDirectory.filePath(retval);
}

TFilePtr SFileTools::FindUniqueDestFile(const CFilePath &inDestDir, const CString &inFStem,
                                        const CString &inExt)
{
    return FindUniqueDestFile(inDestDir, inFStem, inExt, false);
}

QString SFileTools::FindUniqueDestFile(const QDir &inDestDir, const QString &inFStem,
                                       const QString &inExt)
{
    return FindUniqueDestFile(inDestDir, inFStem, inExt, false);
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

    QDir uniqueDir = FindUniqueDestDirectory(QDir(inDestDirectory.filePath()),
                                             inDirName.toQString());
    return CFilePath(uniqueDir.absolutePath());
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
    if (!srcFile.exists())
        return FileErrorCodes::SourceNotExist;

    if (!srcFile.isReadable())
        return FileErrorCodes::SourceNotReadable;

    QString destFileString = destFile.filePath();
    if (QFile::exists(destFileString)) {
        bool ok = QFile::remove(destFileString);
        if (!ok)
            return FileErrorCodes::DestNotWriteable;
    }

    bool ok = QFile::copy(srcFile.filePath(), destFileString);
    if (ok)
        return destFile.size();

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
            CString::fromQString(dest->m_Path.filePath()));
    else {
        QT3DSU64 nb = SFile::Copy(dest->m_OpenFile, srcFile);
        dest->m_OpenFile = NULL;
        return SFileErrorCodeFileNameAndNumBytes(FileErrorCodes::NoError, nb,
            CString::fromQString(dest->m_Path.filePath()));
    }
}

bool SFileTools::FindAndCopyDestFile(const QDir &inDestDir, const QString &inSrcFile)
{
    QString destFile = FindUniqueDestFile(inDestDir, inSrcFile);
    if (destFile.isEmpty())
        return false;
    return QFile::copy(inSrcFile, destFile);
}

bool SFileTools::FindAndCopyDestFile(const QDir &inDestDir, const QString &inSrcFile,
                                     QString &outDestFile)
{
    outDestFile = FindUniqueDestFile(inDestDir, inSrcFile);
    if (outDestFile.isEmpty())
        return false;
    return QFile::copy(inSrcFile, outDestFile);
}

}
