/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#ifndef INCLUDED_QT3DS_FILETOOLS_H
#define INCLUDED_QT3DS_FILETOOLS_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSString.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSFlags.h"
#include "Qt3DSFileToolTypes.h"
#include "Qt3DSDMWStrOpsImpl.h"

class QDir;

namespace Q3DStudio {
using qt3ds::QT3DSI64;
using qt3ds::foundation::SeekPosition;
using qt3ds::foundation::FileOpenFlags;
using qt3ds::foundation::FileOpenFlagValues;
using qt3dsdm::WStrOps;

struct SFileModificationRecord;

/**
 *	SFile tools to abstract dealing with the filesystem.  You should not be calling fstat
 *	or stat, fopen, CreateFile, CreateDirectory, etc.  Those calls need to go through
 *	this object.  File paths can also have identifiers trailing them.
 *  Anything after a # is interprented as an identifier.  All file operations still work
 *	with the identifer included.
 */
class CFilePath : public CString
{
public:
    //////Begin public API

    CFilePath()
        : CString(){}
    CFilePath(const wchar_t *inPath)
        : CString(inPath)
    {
        Normalize();
    }
    CFilePath(const char *inPath)
        : CString(inPath)
    {
        Normalize();
    }
    CFilePath(const CString &szString)
        : CString(szString)
    {
        Normalize();
    }
    CFilePath(const QString &string)
        : CString(string.toStdWString().c_str())
    {
        Normalize();
    }
    CFilePath(const CFilePath &szString)
        : CString(szString)
    {
        Normalize();
    }

    const CFilePath &operator=(const CFilePath &strSrc);

    CFilePath GetDirectory() const;
    CString GetFileName() const;
    CString GetFileStem() const; // no extension, test.png -> test
    CString GetExtension() const;
    // Get a file path you can give to systems that don't understand the identifier
    CFilePath GetPathWithoutIdentifier() const;
    // Set the identifer appended to the end of this file path.
    void SetIdentifier(const CString &inIdentifier);
    // Get the identifier appended to the end of this file path.
    CString GetIdentifier() const;
    void SetIdentifier(unsigned long inIdentifier);

    static void EnsureNonFileURL(CString &ioFileFilePath);

    bool GetModuleFilePath();

    void ConvertToRelative(const CFilePath &inBaseAbsolute);
    static CFilePath GetRelativePathFromBase(const CFilePath &inBase, const CFilePath &inPath)
    {
        CFilePath retval(inPath);
        retval.ConvertToRelative(inBase);
        return retval;
    }

    /**
    *	Return true if this string is in subdirectory of inBasePath
    *	For example, basepath is C:\Folder\Project.uip
    *	This string is C:\Folder\Resources\Image.png
    *	The function should return true.
    *	@param inBasePath absolute path to base off from
    *	@return bool to indicate if is in subdirectory of inBasePath
    */
    bool IsInSubDirectory(const CFilePath &inBasePath) const;

    void CombineBaseAndRelative(const CFilePath &inBase);

    // Given a base CFilePath, convert it to an absolute CFilePath.
    static CFilePath CombineBaseAndRelative(const CFilePath &inBase, const CFilePath &inRelative)
    {
        CFilePath retval(inRelative);
        retval.CombineBaseAndRelative(inBase);
        return retval;
    }

    bool IsAbsolute() const;
    // If we are absolute, we are done.
    // If not, CombineBaseAndRelative using getcwd.
    void ConvertToAbsolute();
    static CFilePath GetAbsolutePath(const CFilePath &inBaseFilePath)
    {
        CFilePath retval(inBaseFilePath);
        retval.ConvertToAbsolute();
        return retval;
    }
    // Make a save file stem from this string.  Involves replacing characters
    // that are illegal (:,\\,//,etc).
    static CString MakeSafeFileStem(const CString &name);

    // Create this directory, recursively creating parent directories
    // if necessary.
    bool CreateDir(bool recurse) const;
    // Returns true if exists and is directory
    bool IsDirectory() const;
    // returns true if exists and is a file
    bool IsFile() const;

    // If the file doesn't exist, create an empty file at this location.
    // If the file does exist, update its modification time to the current time.
    void Touch() const;
    // Returns true if this exists on the filesystem and is a directory or file.
    bool Exists() const;
    // Delete this file from the filesystem
    bool DeleteThisFile();
    // Delete this directory
    bool DeleteThisDirectory(bool recursive);

    // If this is a directory, list all contents.
    // ignore special files ".\\" and "..\\"
    // Returns absolute paths combined with this
    // this.combineBaseAndRelative( result );
    void ListFilesAndDirectories(std::vector<CFilePath> &files) const;

    // Returns absolute paths
    // This object has to be a directory
    // extension list needs to be null terminated
    // Returns results alphabetically sorted
    // If makeRelative is true, returns paths relative to
    // this directory.
    // Else returns absolute paths.
    // ExtensionList should not contain the "." in ".png" for example.
    // It should just contain { L"png", NULL }
    // An empty extension list means return all files.
    void RecursivelyFindFilesOfType(const wchar_t **inExtensionList, std::vector<CFilePath> &files,
                                    bool inMakeRelative, bool inIncludeDirectories = false) const;

    // Given the list of differences from last time this function was called (which may be empty)
    // and the list of new differences to put new results into, recursively diff this directory's
    // contents with the last time this function was called.
    // IF this is a file, the function asserts and immediately returns.
    // All file paths are absolute paths!!
    // Returns all files (and directories) recursively, sorted by name, for which either no record
    // exists
    // or for which the record entry has changed it's value since last time.
    // inOldDifferences and outNewDifferences may point to the same location.
    // We are expecting inOldDifferences to be sorted by filename; do not change the order of
    // the return value *or* call std::stable_sort( ) on the vector before you call this
    // method
    // If at any point inCancel is true then this function will abort at that moment.
    void FindDirectoryDifferences(const std::vector<SFileModificationRecord> &inOldDifferences,
                                  std::vector<SFileModificationRecord> &outNewDifferences,
                                  volatile bool *inCancel = NULL) const;

    SFileInfoFlags GetFileFlags() const;
    // Requires opening the file!!!
    // Not valid for things that aren't files.
    SFileData GetFileData() const;

    // Get the directory where applications can write data.
    static CFilePath GetUserApplicationDirectory();

private:
    void Normalize();
};

struct SFileModificationRecord
{
    CFilePath m_File;
    SFileInfoFlags m_FileInfo;
    SFileData m_FileData;
    FileModificationType::Enum m_ModificationType;

    SFileModificationRecord()
        : m_ModificationType(FileModificationType::Unknown)
    {
    }

    SFileModificationRecord(const CFilePath &inFile, const SFileInfoFlags &inFileInfo,
                            const SFileData &inFileData, FileModificationType::Enum modType)
        : m_File(inFile)
        , m_FileInfo(inFileInfo)
        , m_FileData(inFileData)
        , m_ModificationType(modType)
    {
    }
    // Order lexographically
    bool operator<(const SFileModificationRecord &inOther) const { return m_File < inOther.m_File; }
};

typedef std::vector<SFileModificationRecord> TFileModificationList;

struct SFile
{
    Q_DISABLE_COPY(SFile)

    QSharedPointer<QFile> m_OpenFile;
    CFilePath m_Path;

    SFile(const QSharedPointer<QFile> &of, const CFilePath &path);
    ~SFile();
    QT3DSU32 Read(void *buffPtr, QT3DSU32 byteSize);
    QT3DSU32 Write(const void *buffPtr, QT3DSU32 byteSize);

    QT3DSI64 GetPosition() { return GetPosition(m_OpenFile); }
    void SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inSeekPos)
    {
        SetPosition(m_OpenFile, inOffset, inSeekPos);
    }

    static QSharedPointer<QFile> OpenForRead(const CFilePath &inPath);

    static QSharedPointer<QFile>
    OpenForWrite(const CFilePath &inPath,
                 FileOpenFlags fileFlags = FileOpenFlags(FileOpenFlagValues::Open
                                                         | FileOpenFlagValues::Create
                                                         | FileOpenFlagValues::Truncate));

    // Copy src to dest, close both src and dest, and
    // return the number of bytes copied.
    static QT3DSU64 Copy(const QSharedPointer<QFile> &destFile, const QSharedPointer<QFile> &srcFile);

    // Attemt to write data, return the number of bytes written.
    static QT3DSU32 WriteData(const QSharedPointer<QFile> &fileHandle, const void *data, QT3DSU32 byteSize);

    // Attempt to read data, return the number of byte read
    static QT3DSU32 ReadData(const QSharedPointer<QFile> &fileHandle, void *data, QT3DSU32 byteSize);

    static QT3DSI64 GetPosition(const QSharedPointer<QFile> &fileHandle);

    static void SetPosition(const QSharedPointer<QFile> &fileHandle, QT3DSI64 inOffset, SeekPosition::Enum inSeekPos);

    // Close the file handle.
    static void Close(const QSharedPointer<QFile> &fileHandle);

    // Set file modification time to the current time.
    static void SetFileTimeToCurrentTime(const QSharedPointer<QFile> &fileHandle);

    static std::shared_ptr<SFile> Wrap(const QSharedPointer<QFile> &inFileHandle, const CFilePath &path);
};

typedef std::shared_ptr<SFile> TFilePtr;

struct SFileErrorCodeAndNumBytes
{
    FileErrorCodes::Enum m_Error;
    QT3DSU64 m_NumBytes;
    SFileErrorCodeAndNumBytes(QT3DSU64 nb)
        : m_Error(FileErrorCodes::NoError)
        , m_NumBytes(nb)
    {
    }
    SFileErrorCodeAndNumBytes(FileErrorCodes::Enum val)
        : m_Error(val)
        , m_NumBytes(0)
    {
    }
};

struct SFileErrorCodeFileNameAndNumBytes
{
    FileErrorCodes::Enum m_Error;
    CFilePath m_DestFilename;
    QT3DSU64 m_NumBytes;
    SFileErrorCodeFileNameAndNumBytes(FileErrorCodes::Enum er, QT3DSU64 nb = 0,
                                      const CString &dfn = CString())
        : m_Error(er)
        , m_DestFilename(dfn)
        , m_NumBytes(nb)
    {
    }
};

struct SFileTools
{
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
    static TFilePtr FindUniqueDestFile(const CFilePath &inDestDir, const CString &inFStem,
                                       const CString &inExt);
    static QString FindUniqueDestFile(const QDir &inDestDir, const QString &inFStem,
                                      const QString &inExt);
    // Similar to above but takes care of finding the stem and extension from the full source
    // CFilePath
    static TFilePtr FindUniqueDestFile(const CFilePath &inDestDir,
                                       const CFilePath &inSrcFullFilePath);
    static QString FindUniqueDestFile(const QDir &inDestDir,
                                      const QString &inSrcFullFilePath);

    // Find a unique destination directory.  This directory was guaranteed not to exist before this
    // call
    // and we are guaranteeing that it was created for this call; i.e. there can't be a race
    // condition
    // We expect dest dir and dir name to be normalized.
    // We also expect that inDestDirectory exists.  Function results are L"" otherwise
    // If the function succeeds, it returns the absolute CFilePath of the new directory.
    // If it fails, it returns L"";
    static CFilePath FindUniqueDestDirectory(const CFilePath &inDestDir, const CString &inDirName);

    static QDir FindUniqueDestDirectory(const QDir &inDestDir, const QString &inDirName);

    // Copy the full file FilePaths.
    // Same file flags as SFile::OpenForWrite
    static SFileErrorCodeAndNumBytes Copy(const CFilePath &destFile, FileOpenFlags fileFlags,
                                          const CFilePath &srcFile);

    // Find a unique dest file based on the src file stem and extension but in the destination
    // directory
    // then copy that file.  Return the file name
    // this -> points to destination directory
    // srcFile -> src file to copy
    static SFileErrorCodeFileNameAndNumBytes FindAndCopyDestFile(const CFilePath &inDestDir,
                                                                 const CFilePath &inSrcFile);
    static bool FindAndCopyDestFile(const QDir &inDestDir,
                                    const QString &inSrcFile);
};

}

#endif // INCLUDED_QT3DS_FILETOOLS_H
