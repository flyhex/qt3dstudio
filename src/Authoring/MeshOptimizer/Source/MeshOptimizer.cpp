/****************************************************************************
**
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

#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include "Qt3DSFileTools.h"
#include "StandardExtensions.h"
#include <map>
#include "Qt3DSImportMesh.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMStringTable.h"
#include "render/NvRenderBaseTypes.h"
#include "TrackingAllocator.h"
#include "StandardExtensions.h"
#include "Qt3DSLogging.h"

using namespace Q3DStudio;
using namespace std;
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::render;

const CString uipExtension = "uip";
const CString meshExtension = "mesh";

bool IsNotUIPFile(const CFilePath &inFile)
{
    if (inFile.GetExtension().CompareNoCase(uipExtension) == false)
        return true;
    return false;
}

using namespace qt3dsdm;
typedef map<Q3DStudio::CFilePath, vector<QT3DSU32>> TMeshFileToIdMap;

void ScanChildrenForSourcepath(qt3dsdm::IDOMReader &inReader, TMeshFileToIdMap &inMap,
                               const CFilePath &inProjectDirectory)
{
    qt3dsdm::IDOMReader::Scope elemScope(inReader);
    const wchar_t *sourcePathAtt = NULL;
    if (inReader.UnregisteredAtt(L"sourcepath", sourcePathAtt)) {
        if (sourcePathAtt && *sourcePathAtt) {
            if (sourcePathAtt[0] != '#') {
                CFilePath theFullPath(
                    CFilePath::CombineBaseAndRelative(inProjectDirectory, sourcePathAtt));
                if (theFullPath.GetExtension().CompareNoCase(meshExtension)) {
                    if (theFullPath.IsFile()) {
                        QT3DSU32 theMeshId = theFullPath.GetULIdentifier();
                        pair<TMeshFileToIdMap::iterator, bool> insertResult = inMap.insert(
                            make_pair(theFullPath.GetPathWithoutIdentifier(), vector<QT3DSU32>()));
                        insert_unique(insertResult.first->second, theMeshId);
                    } else {
                        qCWarning(qt3ds::WARNING) << "Failed to find mesh: "
                                     << CString(sourcePathAtt).GetMulti();
                        // go on, ignore it
                    }
                }
            }
        }
    }
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling())
        ScanChildrenForSourcepath(inReader, inMap, inProjectDirectory);
}

void FindMeshFiles(const CFilePath &inUIPPath, TMeshFileToIdMap &inMeshMap,
                   const CFilePath &inProjectDirectory)
{
    qCInfo(qt3ds::TRACE_INFO) << "Scanning file " << inUIPPath.GetCharStar() << " for mesh files";
    std::shared_ptr<qt3dsdm::IStringTable> theTable(qt3dsdm::IStringTable::CreateStringTable());
    std::shared_ptr<qt3dsdm::IDOMFactory> theFactory(
        qt3dsdm::IDOMFactory::CreateDOMFactory(theTable));
    CFileSeekableIOStream theStream(inUIPPath, FileOpenFlagValues::Open);
    if (!theStream.IsOpen()) {
        QT3DS_ASSERT(false);
        return;
    }

    qt3dsdm::SDOMElement *theElement = CDOMSerializer::Read(*theFactory, theStream);
    if (!theElement) {
        QT3DS_ASSERT(false);
        return;
    }
    std::shared_ptr<qt3dsdm::IDOMReader> theReader(
        qt3dsdm::IDOMReader::CreateDOMReader(*theElement, theTable, theFactory));
    if (!theReader->MoveToFirstChild(L"Project")) {
        QT3DS_ASSERT(false);
        return;
    }
    {
        qt3dsdm::IDOMReader::Scope projectScope(*theReader);
        if (!theReader->MoveToFirstChild(L"Graph")) {
            QT3DS_ASSERT(false);
            return;
        }
        ScanChildrenForSourcepath(*theReader, inMeshMap, inProjectDirectory);
    }
    {
        qt3dsdm::IDOMReader::Scope projectScope(*theReader);
        if (theReader->MoveToFirstChild(L"Logic"))
            ScanChildrenForSourcepath(*theReader, inMeshMap, inProjectDirectory);
    }
}

struct SMeshScope
{
    qt3dsimp::Mesh *m_Mesh;
    SMeshScope(qt3dsimp::Mesh *mesh)
        : m_Mesh(mesh)
    {
    }
    ~SMeshScope()
    {
        if (m_Mesh)
            free(m_Mesh);
    }
};

int main(int c, char **v)
{
    CFilePath theProjectDirectory;
    if (c > 1)
        theProjectDirectory = v[1];
    else
        theProjectDirectory.GetCurrentDir();

    if (theProjectDirectory.size() == 0)
        return -1;
    if (theProjectDirectory[0] == '"')
        theProjectDirectory.erase(0, 1);

    if (theProjectDirectory.size() == 0)
        return -1;

    if (theProjectDirectory[theProjectDirectory.size() - 1] == '"')
        theProjectDirectory.erase(theProjectDirectory.size() - 1, 1);

    vector<CFilePath> theDirectoryFiles;

    theProjectDirectory.ListFilesAndDirectories(theDirectoryFiles);
    erase_if(theDirectoryFiles, IsNotUIPFile);

    if (theDirectoryFiles.size() == 0) {
        qCCritical(qt3ds::INTERNAL_ERROR) << "Directory: " << theProjectDirectory.GetMulti()
                   << " doesn't appear to contain UIP files";
        return -1;
    }
    TMeshFileToIdMap theMeshMap;
    for (size_t idx = 0, end = theDirectoryFiles.size(); idx < end; ++idx)
        FindMeshFiles(theDirectoryFiles[idx], theMeshMap, theProjectDirectory);

    qCInfo(qt3ds::TRACE_INFO) << "Found " << theMeshMap.size() << " mesh files";

    qt3dsimp::MeshBuilder &theBuilder = qt3dsimp::MeshBuilder::CreateMeshBuilder();
    vector<QT3DSU16> theIndexes;
    MallocAllocator theAllocator;
    QT3DSU32 theFileIndex = 1;
    QT3DSU32 theFileCount = theMeshMap.size();
    for (TMeshFileToIdMap::iterator theIter = theMeshMap.begin(), theEnd = theMeshMap.end();
         theIter != theEnd; ++theIter, ++theFileIndex) {
        qCInfo(qt3ds::TRACE_INFO) << "Optimizing mesh " << theFileIndex << " of " << theFileCount;
        const vector<QT3DSU32> &theIdList(theIter->second);
        CFilePath theTempFile(theIter->first);
        theTempFile.append(L".temp");
        qCInfo(qt3ds::TRACE_INFO) << "Optimizing mesh file: " << theIter->first.GetMulti();
        for (size_t meshIdx = 0, meshEnd = theIdList.size(); meshIdx < meshEnd; ++meshIdx) {
            theBuilder.Reset();
            QT3DSU32 theMeshId(theIdList[meshIdx]);
            qCInfo(qt3ds::TRACE_INFO) << "Optimizing mesh: " << theMeshId;
            qt3dsimp::Mesh *theMesh = qt3dsimp::Mesh::LoadMulti(theIter->first.GetMulti(), theMeshId);
            if (!theMesh) {
                qCWarning(qt3ds::WARNING) << "Failed to load mesh";
                QT3DS_ASSERT(false);
                continue;
            }
            SMeshScope theMeshScope(theMesh);
            theBuilder.SetVertexBuffer(theMesh->m_VertexBuffer.m_Entries,
                                       theMesh->m_VertexBuffer.m_Stride,
                                       theMesh->m_VertexBuffer.m_Data);

            theBuilder.SetIndexBuffer(NVConstDataRef<QT3DSU8>(theMesh->m_IndexBuffer.m_Data.begin(),
                                                           theMesh->m_IndexBuffer.m_Data.size()),
                                      theMesh->m_IndexBuffer.m_ComponentType);

            for (QT3DSU32 jointIdx = 0, jointEnd = theMesh->m_Joints.size(); jointIdx < jointEnd;
                 ++jointIdx) {
                const qt3dsimp::Joint &theJoint = theMesh->m_Joints[jointIdx];
                theBuilder.AddJoint(theJoint.m_JointID, theJoint.m_ParentID, theJoint.m_invBindPose,
                                    theJoint.m_localToGlobalBoneSpace);
            }

            for (QT3DSU32 subsetIdx = 0; subsetIdx < theMesh->m_Subsets.size(); ++subsetIdx) {
                theBuilder.AddMeshSubset(theMesh->m_Subsets[subsetIdx].m_Name.begin(),
                                         theMesh->m_Subsets[subsetIdx].m_Count,
                                         theMesh->m_Subsets[subsetIdx].m_Offset,
                                         theMesh->m_Subsets[subsetIdx].m_Bounds);
            }
            theBuilder.SetDrawParameters(theMesh->m_DrawMode, theMesh->m_Winding);
            theBuilder.OptimizeMesh();
            qt3dsimp::Mesh theOptimizedMesh(theBuilder.GetMesh());
            FileOpenFlags theFlags = FileWriteFlags();
            if (meshIdx)
                theFlags = FileAppendFlags();
            CFileSeekableIOStream theIOStream(theTempFile.GetMulti(), theFlags);
            if (!theIOStream.IsOpen()) {
                qCWarning(qt3ds::WARNING) << "Failed to open temp mesh file";
                QT3DS_ASSERT(false);
            } else
                theOptimizedMesh.SaveMulti(theAllocator, theIOStream, theMeshId);
        }

        CFilePath theSrcFile(theIter->first);
        qCInfo(qt3ds::TRACE_INFO) << "Overwriting source file";
        BOOL success = ::CopyFileExW(theTempFile, theIter->first, NULL, NULL, NULL, 0);
        if (!success)
            qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to write to file: " << theSrcFile.GetMulti()
                       << ", is this file locked by source control?";
        else
            qCInfo(qt3ds::TRACE_INFO) << "Successfully overwritten original mesh file!";
        theTempFile.DeleteThisFile();
    }

    theBuilder.Release();

    return 0;
}
