/****************************************************************************
**
** Copyright (C) 1999-2012 NVIDIA Corporation.
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
// Includes
//==============================================================================

#pragma once

#include "UICImportSceneGraphTranslation.h"
#include "UICImportTranslationCommon.h"

namespace UICIMP {
typedef struct _SFaceMaterialInfo
{
    long m_StartFace;
    long m_FaceCount;
    std::vector<int> m_MatElementsIDs;
    std::vector<int> m_MatIDs;

    _SFaceMaterialInfo()
        : m_StartFace(-1)
        , m_FaceCount(-1)
    {
    }

    _SFaceMaterialInfo(const _SFaceMaterialInfo &info)
        : m_StartFace(info.m_StartFace)
        , m_FaceCount(info.m_FaceCount)
    {
        m_MatElementsIDs.resize(info.m_MatElementsIDs.size());
        for (size_t i = 0; i < info.m_MatElementsIDs.size(); i++) {
            m_MatElementsIDs[i] = info.m_MatElementsIDs[i];
        }

        m_MatIDs.resize(info.m_MatIDs.size());
        for (size_t i = 0; i < info.m_MatIDs.size(); i++) {
            m_MatIDs[i] = info.m_MatIDs[i];
        }
    }

    bool operator==(const _SFaceMaterialInfo &info)
    {
        // Note: by intension we only compare the material id's
        if (info.m_MatIDs.size() != m_MatIDs.size()) {
            return false;
        }

        for (size_t i = 0; i < info.m_MatIDs.size(); i++) {
            if (m_MatIDs[i] != info.m_MatIDs[i])
                return false;
        }

        return true;
    }

} SFaceMaterialInfo;

typedef struct _SVertexWeightInfo
{
    long m_JointID;
    float m_Weight;
    SVector3 m_Position;

    _SVertexWeightInfo(int jointID, float weight, FbxVector4 pos)
        : m_JointID(jointID)
        , m_Weight(weight)
    {
        m_Position[0] = (float)pos[0];
        m_Position[1] = (float)pos[1];
        m_Position[2] = (float)pos[2];
    }
    // copy constructor
    _SVertexWeightInfo(const _SVertexWeightInfo &info)
        : m_JointID(info.m_JointID)
        , m_Weight(info.m_Weight)
        , m_Position(info.m_Position)
    {
    }

} SVertexWeightInfo;

void ReadFloat3(FbxDouble3 inArray, float *outFloat)
{
    outFloat[0] = (float)inArray[0];
    outFloat[1] = (float)inArray[1];
    outFloat[2] = (float)inArray[2];
}

void ReadFMatrix4(FbxAMatrix inMatrix, float *outFloat)
{
    for (int theRow = 0; theRow < 4; theRow++)
        for (int theColumn = 0; theColumn < 4; theColumn++)
            outFloat[(theRow * 4) + theColumn] = (float)inMatrix.Get(theColumn, theRow);
}

void ReadFMatrix4T(FbxAMatrix inMatrix, float *outFloat)
{
    for (int theRow = 0; theRow < 4; theRow++)
        for (int theColumn = 0; theColumn < 4; theColumn++)
            outFloat[(theRow * 4) + theColumn] = (float)inMatrix.Get(theRow, theColumn);
}

void WriteFloat4(std::vector<float> &outVector, unsigned long inFaceIndex, const float *inValues)
{
    inFaceIndex *= 4;

    outVector[inFaceIndex] = inValues[0];
    outVector[inFaceIndex + 1] = inValues[1];
    outVector[inFaceIndex + 2] = inValues[2];
    outVector[inFaceIndex + 3] = inValues[3];
}

void WriteFloat3(std::vector<float> &outVector, unsigned long inFaceIndex, const float *inValues)
{
    inFaceIndex *= 3;

    outVector[inFaceIndex] = inValues[0];
    outVector[inFaceIndex + 1] = inValues[1];
    outVector[inFaceIndex + 2] = inValues[2];
}

void WriteFloat2(std::vector<float> &outVector, unsigned long inFaceIndex, const float *inValues)
{
    inFaceIndex *= 2;

    outVector[inFaceIndex] = inValues[0];
    outVector[inFaceIndex + 1] = inValues[1];
}

#define PushAnimationCurve(theAnimCurvesList, inProperty, inComponent)                             \
    {                                                                                              \
        theAnimCurve = inNode->Lcl##inProperty.GetCurve(inAnimLayer,                               \
                                                        FBXSDK_CURVENODE_COMPONENT_##inComponent); \
        if (theAnimCurve) {                                                                        \
            theAnimCurvesList.push_back(theAnimCurve);                                             \
        }                                                                                          \
    }
}
