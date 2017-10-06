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

#include <stdio.h>
#include "foundation/Qt3DSFoundation.h"
#include "foundation/IOStreams.h"
#include "foundation/StringConversionImpl.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSVec3.h"
#include "UICImportMesh.h"

using namespace qt3ds;
using namespace qt3ds::foundation;

namespace {

eastl::string ReadStream(IInStream &inStream)
{
    eastl::string retval;
    bool good = true;
    do {
        char buffer[1024] = { 0 };
        NVDataRef<QT3DSU8> readData(reinterpret_cast<QT3DSU8 *>(buffer), 1024);
        QT3DSU32 amountRead = inStream.Read(readData);
        if (amountRead == 0)
            good = false;
        else
            retval.append(buffer, buffer + amountRead);

    } while (good);

    return retval;
}

eastl::vector<QT3DSVec3> ParsePoints(eastl::string &data)
{
    eastl::vector<QT3DSVec3> retval;
    while (data.size()) {
        uint32_t pos = data.find("\n");
        if (pos == eastl::string::npos)
            pos = data.size() - 1;
        size_t retvalSize = retval.size();
        (void)retvalSize;

        eastl::string temp = data.substr(2, pos - 2);
        data.erase(data.begin(), data.begin() + pos + 1);
        QT3DSVec3 parseData;
        char *parsePtr = const_cast<char *>(temp.c_str());
        char *nextPtr = NULL;
        parseData[0] = static_cast<float>(strtod(parsePtr, &nextPtr));
        parsePtr = nextPtr;
        parseData[1] = static_cast<float>(strtod(parsePtr, &nextPtr));
        parsePtr = nextPtr;
        parseData[2] = static_cast<float>(strtod(parsePtr, &nextPtr));
        parsePtr = nextPtr;
        retval.push_back(parseData);
    }
    return retval;
}
}

int main(int c, char **v)
{
    if (c < 3)
        return 1;
    const char *startFileName = v[1];
    const char *endFileName = v[2];

    CFileSeekableIOStream startFile(startFileName, FileReadFlags());
    CFileSeekableIOStream endFile(endFileName, FileReadFlags());
    if (!(startFile.IsOpen() && endFile.IsOpen())) {
        puts("failed to open source files");
        return 1;
    }

    eastl::string startDataString = ReadStream(startFile);
    eastl::string endDataString = ReadStream(endFile);

    eastl::vector<QT3DSVec3> startData = ParsePoints(startDataString);
    eastl::vector<QT3DSVec3> endData = ParsePoints(endDataString);

    QT3DS_ASSERT(!startData.empty());
    QT3DS_ASSERT(!endData.empty());
    QT3DS_ASSERT(startData.size() == endData.size());

    UICIMP::MeshBuilder &theBuilder = UICIMP::MeshBuilder::CreateMeshBuilder();
    eastl::vector<QT3DSVec3> meshBuffer;

    NVBounds3 theBounds(NVBounds3::empty());
    for (uint32_t idx = 0, end = startData.size(); idx < end; ++idx) {
        meshBuffer.push_back(startData[idx]);
        theBounds.include(meshBuffer.back());
        meshBuffer.push_back(endData[idx]);
        theBounds.include(meshBuffer.back());
    }
    theBuilder.SetDrawParameters(qt3ds::render::NVRenderDrawMode::Lines,
                                 qt3ds::render::NVRenderWinding::CounterClockwise);
    NVDataRef<QT3DSU8> theData = qt3ds::foundation::toU8DataRef(meshBuffer.data(), meshBuffer.size());
    UICIMP::MeshBuilderVBufEntry theBuilderEntry("attr_pos", theData,
                                                 qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3);
    theBuilder.SetVertexBuffer(toConstDataRef(theBuilderEntry));
    theBuilder.AddMeshSubset(L"points", (QT3DSU32)meshBuffer.size(), 0,
                             theBounds);
    UICIMP::Mesh &theMesh(theBuilder.GetMesh());
    theMesh.SaveMulti(v[3]);
    return 0;
}
