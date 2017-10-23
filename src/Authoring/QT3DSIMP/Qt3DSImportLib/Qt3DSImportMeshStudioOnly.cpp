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
#include "Qt3DSFileToolsSeekableMeshBufIOStream.h"

using namespace qt3dsimp;
using namespace Q3DStudio;

Mesh *Mesh::Load(const char *inFilePath)
{
    Q3DStudio::CFilePath thePath(inFilePath);
    CFileSeekableIOStream stream(thePath, FileReadFlags());
    if (stream.IsOpen() == false) {
        QT3DS_ASSERT(false);
        return NULL;
    }
    MallocAllocator allocator;
    return Load(allocator, stream);
}

bool Mesh::Save(const char *inFilePath) const
{
    CFileSeekableIOStream stream(inFilePath, FileWriteFlags());
    if (stream.IsOpen() == false) {
        QT3DS_ASSERT(false);
        return false;
    }
    Save(stream);
    return true;
}

// You can save multiple meshes in a file.  Each mesh returns an incrementing
// integer for the multi file.  The original meshes aren't changed, and the file
// is appended to.
QT3DSU32 Mesh::SaveMulti(const char *inFilePath) const
{
    MallocAllocator allocator;
    Q3DStudio::CFilePath thePath(inFilePath);
    FileOpenFlags theFlags = thePath.Exists() ? FileAppendFlags() : FileWriteFlags();
    CFileSeekableIOStream stream(thePath, theFlags);
    return SaveMulti(allocator, stream);
}

// Load a single mesh using c file API and malloc/free.
SMultiLoadResult Mesh::LoadMulti(const char *inFilePath, QT3DSU32 inId)
{
    MallocAllocator allocator;
    Q3DStudio::CFilePath thePath(inFilePath);
    CFileSeekableIOStream stream(thePath, FileReadFlags());
    return LoadMulti(allocator, stream, inId);
}

// Load a multi header from a file using malloc.  Header needs to be freed using free.
MeshMultiHeader *Mesh::LoadMultiHeader(const char *inFilePath)
{
    MallocAllocator allocator;
    Q3DStudio::CFilePath thePath(inFilePath);
    CFileSeekableIOStream stream(thePath, FileReadFlags());
    return LoadMultiHeader(allocator, stream);
}

QT3DSU32 Mesh::GetHighestMultiVersion(const char *inFilePath)
{
    MallocAllocator allocator;
    Q3DStudio::CFilePath thePath(inFilePath);
    CFileSeekableIOStream stream(thePath, FileReadFlags());
    return GetHighestMultiVersion(allocator, stream);
}

void Qt3DSFileToolsSeekableMeshBufIOStream::SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inEnum)
{
    m_File->SetPosition(inOffset, (Q3DStudio::SeekPosition::Enum)inEnum);
}
QT3DSI64 Qt3DSFileToolsSeekableMeshBufIOStream::GetPosition() const
{
    return m_File->GetPosition();
}
QT3DSU32 Qt3DSFileToolsSeekableMeshBufIOStream::Read(NVDataRef<QT3DSU8> data)
{
    return m_File->Read(data.begin(), data.size());
}
bool Qt3DSFileToolsSeekableMeshBufIOStream::Write(NVConstDataRef<QT3DSU8> data)
{
    return m_File->Write(data.begin(), data.size()) == data.size();
}
