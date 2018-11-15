/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "Qt3DSImportLibPrecompile.h"
#include "Qt3DSImportContainers.h"
#include "Qt3DSImportMesh.h"
#include "Qt3DSFileTools.h"
#include "foundation/TrackingAllocator.h"

using namespace qt3dsimp;
using namespace Q3DStudio;

Mesh *Mesh::Load(const QString &inFilePath)
{
    QFile stream(inFilePath);
    if (!stream.open(QFile::ReadOnly)) {
        Q_ASSERT(false);
        return NULL;
    }
    MallocAllocator allocator;
    return Load(allocator, stream);
}

bool Mesh::Save(const QString &inFilePath) const
{
    QFile stream(inFilePath);
    if (!stream.open(QFile::ReadWrite | QFile::Truncate)) {
        Q_ASSERT(false);
        return false;
    }
    Save(stream);
    return true;
}

// You can save multiple meshes in a file.  Each mesh returns an incrementing
// integer for the multi file.  The original meshes aren't changed, and the file
// is appended to.
QT3DSU32 Mesh::SaveMulti(const QString &inFilePath) const
{
    MallocAllocator allocator;
    QFileInfo thePath(inFilePath);
    QFile::OpenMode theFlags = thePath.exists() ? (QFile::ReadWrite | QFile::Append)
                                                : QFile::WriteOnly;
    QFile stream(inFilePath);
    if (!stream.open(theFlags)) {
        Q_ASSERT(false);
        return 0;
    }
    return SaveMulti(allocator, stream);
}

// Load a single mesh using c file API and malloc/free.
SMultiLoadResult Mesh::LoadMulti(const QString &inFilePath, QT3DSU32 inId)
{
    MallocAllocator allocator;
    QFile stream(inFilePath);
    if (!stream.open(QFile::ReadOnly)) {
        Q_ASSERT(false);
        return {};
    }
    return LoadMulti(allocator, stream, inId);
}

// Load a multi header from a file using malloc.  Header needs to be freed using free.
MeshMultiHeader *Mesh::LoadMultiHeader(const QString &inFilePath)
{
    MallocAllocator allocator;
    QFile stream(inFilePath);
    if (!stream.open(QFile::ReadOnly)) {
        Q_ASSERT(false);
        return {};
    }
    return LoadMultiHeader(allocator, stream);
}

QT3DSU32 Mesh::GetHighestMultiVersion(const QString &inFilePath)
{
    MallocAllocator allocator;
    QFile stream(inFilePath);
    if (!stream.open(QFile::ReadOnly)) {
        Q_ASSERT(false);
        return {};
    }
    return GetHighestMultiVersion(allocator, stream);
}
