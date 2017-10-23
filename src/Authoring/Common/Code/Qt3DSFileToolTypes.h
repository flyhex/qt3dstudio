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
#pragma once
#ifndef INCLUDED_QT3DS_FILETOOL_ENUMS_H
#define INCLUDED_QT3DS_FILETOOL_ENUMS_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/IOStreams.h"

namespace Q3DStudio {

struct FileInfoFlagValues
{
    enum Enum {
        Exists = 1,
        IsDirectory = 1 << 1,
        CanRead = 1 << 2,
        CanWrite = 1 << 3,
        IsHidden = 1 << 4,
    };
};

using qt3ds::NVFlags;
using qt3ds::QT3DSU32;
using qt3ds::QT3DSU64;
struct SFileInfoFlags : public NVFlags<FileInfoFlagValues::Enum, int>
{
    SFileInfoFlags(int value = 0)
        : NVFlags(value)
    {
    }

    bool Exists() const { return *this & FileInfoFlagValues::Exists; }
    bool IsDirectory() const { return Exists() && (*this & FileInfoFlagValues::IsDirectory); }
    bool IsFile() const
    {
        return Exists() && (QT3DSU32)((*this & FileInfoFlagValues::IsDirectory)) == 0;
    }
    bool CanRead() const { return Exists() && (*this & FileInfoFlagValues::CanRead); }
    bool CanWrite() const { return Exists() && (*this & FileInfoFlagValues::CanWrite); }
    bool IsHidden() const { return Exists() && (*this & FileInfoFlagValues::IsHidden); }
};

struct SFileData
{
    QT3DSU64 m_Length;
    QT3DSU64 m_LastModTime;
    QT3DSU64 m_CreateTime;
    SFileData()
        : m_Length(0)
        , m_LastModTime(0)
        , m_CreateTime(0)
    {
    }
    SFileData(QT3DSU64 len, QT3DSU64 lastMod, QT3DSU64 create)
        : m_Length(len)
        , m_LastModTime(lastMod)
        , m_CreateTime(create)
    {
    }
};

struct FileModificationType
{
    enum Enum {
        Unknown = 0,
        Created,
        Modified, // file data changed
        InfoChanged, // file info changed
        Destroyed,
        NoChange,
    };
};

struct FileErrorCodes
{
    enum Enum {
        NoError,
        SourceNotExist,
        SourceNotReadable,
        DestNotWriteable,
    };
};
}
#endif
