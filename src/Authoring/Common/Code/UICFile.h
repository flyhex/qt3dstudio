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
#ifndef INCLUDED_UIC_FILE_H
#define INCLUDED_UIC_FILE_H 1

#pragma once

#include "UICString.h"

#include <set>
#include "UICString.h"
#include "FileIterator.h"
#include "PlatformTypes.h"

//=========================================================================================
//	Typedefs
//=========================================================================================
typedef std::set<Q3DStudio::CString> TFilePathList;
typedef TFilePathList::iterator TFilePathListIterator;

class CUICFile
{

public:
    typedef void OSErr; // this may be better typedefed to HRESULT

    static const Q3DStudio::CString CURRENT_DIR;
    static const Q3DStudio::CString PARENT_DIR;

    CUICFile(const Q3DStudio::CString &inPathName, bool inIsPosix = false, bool inAddBase = true);
    CUICFile(const Q3DStudio::CString &inPathName, const Q3DStudio::CString &inName);
    CUICFile(const CUICFile &inBasePath, const Q3DStudio::CString &inPathname,
             bool inIsPosix = false);
    CUICFile(const CUICFile &inFile);

    CFileIterator GetSubItems() const;

    ~CUICFile();

    bool operator==(const CUICFile &inRHS) const;

    bool CanRead() const;
    bool CanWrite() const;
    bool DeleteFile() const;
    bool Exists() const;

    Q3DStudio::CString GetAbsolutePath() const;
    Q3DStudio::CString GetAbsolutePosixPath() const;

    Q3DStudio::CString GetName() const;
    Q3DStudio::CString GetExtension() const;
    Q3DStudio::CString GetPath() const;

    bool IsFile(bool inCheckForAlias = true) const;
    bool IsHidden() const;
    long Length() const;

    OSErr MoveTo(const CUICFile &inDestination);
    void CopyTo(const CUICFile &inDestination);

    void Execute() const;

    OSErr SetReadOnly(bool inReadOnlyFlag);

    static CUICFile GetApplicationDirectory();
    static CUICFile GetTemporaryFile(const Q3DStudio::CString &inExtension);
    static CUICFile GetTemporaryFile();
    static bool IsPathRelative(const Q3DStudio::CString &inPath);

    QUrl GetURL() const;

    static CUICFile Combine(const CUICFile &inFile, const Q3DStudio::CString &inRelPath);
    HANDLE OpenFileReadHandle() const;
    HANDLE OpenFileWriteHandle() const;
    void RenameTo(const CUICFile &inDestination);
    bool GetFileStat(struct _stat *inStat) const;

    static void ClearCurrentTempCache();

    static void AddTempFile(const Q3DStudio::CString &inFile);

    // protected functions
protected:
    static TFilePathList s_TempFilePathList; ///< List of temporary files that gets created; this
                                             ///should be cleared at end of program execution by
                                             ///calling ClearTempCache
    Q3DStudio::CString m_Path;
};

#endif // INCLUDED_UIC_FILE_H
