/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "PathImportTranslator.h"
#include "UICImportComposerTypes.h"
#include "foundation/IOStreams.h"
#include "DynamicLua.h"
#include "UICImport.h"
#include "UICImportPath.h"

using namespace Q3DStudio;
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace UICIMP;

SPathImportTranslator::SPathImportTranslator(const QString &srcFile, IDynamicLua &inLuaState,
                                             qt3ds::NVFoundationBase &inFoundation)
    : m_Foundation(inFoundation)
    , m_SourceFile(srcFile)
    , m_LuaState(inLuaState)
    , m_Import(NULL)
{
}

SPathImportTranslator::~SPathImportTranslator()
{
}

void SPathImportTranslator::SetName(TInstanceHandle inItem, const wchar_t *inNameBase)
{
    m_Import->SetInstancePropertyValue(inItem, m_ObjectTypes.m_Asset.m_NameProp,
                                       Q3DStudio::CString(inNameBase));
}

void SPathImportTranslator::SetUniqueName(TInstanceHandle inItem, const char8_t *inNameBase,
                                          eastl::vector<Q3DStudio::CString> &inExistingNames)
{
    Q3DStudio::CString theName(inNameBase);
    QT3DSU32 idx = 1;
    while (eastl::find(inExistingNames.begin(), inExistingNames.end(), theName)
           != inExistingNames.end()) {
        char8_t nameBuffer[64];
        sprintf(nameBuffer, "%d", idx);
        ++idx;
        theName.assign(inNameBase);
        theName.append("_");
        theName.append(nameBuffer);
    }

    m_Import->SetInstancePropertyValue(inItem, m_ObjectTypes.m_Asset.m_NameProp, theName);
    inExistingNames.push_back(theName);
}

QT3DSU64 SPathImportTranslator::GetChild(TInstanceHandle inItem, QT3DSI32 inIndex)
{
    QT3DSU32 numChildren = m_Import->GetNumChildren(inItem);
    m_ChildVector.resize(numChildren);
    m_Import->GetChildren(inItem, toDataRef(m_ChildVector.data(), m_ChildVector.size()));
    if (inIndex < (QT3DSI32)m_ChildVector.size())
        return m_ChildVector[inIndex].m_Handle;
    return 0;
}

QT3DSU64 SPathImportTranslator::CreateSceneGraphInstance(
    UICDM::ComposerObjectTypes::Enum inObjectType, SPathImportTranslator::TInstanceHandle inParent,
    const Q3DStudio::CString &inId)
{
    Q3DStudio::CString theIdStr(inId);
    QT3DSU32 idx = 1;
    while (m_Import->FindAnyInstanceById(theIdStr).hasValue()) {
        char8_t nameBuffer[64];
        sprintf(nameBuffer, "%d", idx);
        ++idx;
        theIdStr.assign(inId);
        theIdStr.append("_");
        theIdStr.append(nameBuffer);
    }
    TInstanceHandle retval = m_Import->CreateInstance(theIdStr, inObjectType);
    if (inParent)
        m_Import->AddChild(inParent, retval);
    return retval;
}

void SPathImportTranslator::ParseSVGGroupChildren(TInstanceHandle inNewItem)
{
    IDynamicLua::Scope __itemScope(m_LuaState);
    QT3DSI32 numChildren = m_LuaState.GetNumChildrenFromTopOfStackTable();
    eastl::vector<Q3DStudio::CString> existingNames;
    // parse children in reverse order so they will render correctly.
    for (QT3DSI32 idx = 0; idx < numChildren; ++idx) {
        QT3DSI32 relIdx = numChildren - idx;
        m_LuaState.GetChildFromTopOfStackTable(relIdx);
        ParseSVGItem(inNewItem, existingNames);
        m_LuaState.SetTop(__itemScope.m_Top);
    }
}

QT3DSU64 SPathImportTranslator::ParseSVGGroup(TInstanceHandle inParent,
                                           eastl::vector<Q3DStudio::CString> &inExistingNames,
                                           bool inRoot)
{
    eastl::string itemType = m_LuaState.StringFromTopOfStackTable("TYPE");
    eastl::string itemName = m_LuaState.StringFromTopOfStackTable("name");
    eastl::string idItem(itemName);
    if (itemName.size() == 0)
        itemName = "Group";
    // bool success = m_LuaState.GetChildFromTopOfStackTable( 1 );
    if (inRoot)
        idItem = "__import__root__";
    TInstanceHandle retval =
        CreateSceneGraphInstance(UICDM::ComposerObjectTypes::Group, inParent, idItem.c_str());
    SetUniqueName(retval, itemName.c_str(), inExistingNames);
    ParseSVGGroupChildren(retval);
    return retval;
}

namespace {
QT3DSF32 ToUICDM(Option<QT3DSF32> inData, QT3DSF32 inDefault)
{
    if (inData.hasValue())
        return *inData;
    return inDefault;
}
SFloat3 ToUICDM(const QT3DSVec3 &inData)
{
    return SFloat3(inData.x, inData.y, inData.z);
}
bool ToUICDM(Option<bool> inData, bool inDefault)
{
    if (inData.hasValue())
        return *inData;
    return inDefault;
}
}

QT3DSU64 SPathImportTranslator::ParseSVGPath(TInstanceHandle inParent,
                                          eastl::vector<Q3DStudio::CString> &inExistingNames)
{
    QT3DSF32 strokeWidth = ToUICDM(m_LuaState.NumberFromTopOfStackTable("stroke-width"), 1.0f);
    QT3DSF32 pathOpacity = ToUICDM(m_LuaState.NumberFromTopOfStackTable("opacity"), 100.0f);
    QT3DSF32 fillOpacity = ToUICDM(m_LuaState.NumberFromTopOfStackTable("fill-opacity"), 100.0f);
    QT3DSF32 strokeOpacity = ToUICDM(m_LuaState.NumberFromTopOfStackTable("stroke-opacity"), 100.0f);
    Option<QT3DSVec3> fillColor = m_LuaState.Vec3FromTopOfStackTable("fill");
    Option<QT3DSVec3> strokeColor = m_LuaState.Vec3FromTopOfStackTable("stroke");
    eastl::string pathName = m_LuaState.StringFromTopOfStackTable("name");
    QT3DSI32 childIndex = 1;
    eastl::vector<Q3DStudio::CString> existingNames;
    bool hasStroke = strokeColor.hasValue();
    bool hasFill = fillColor.hasValue();
    if (!hasStroke && !hasFill)
        return TInstanceHandle();
    TInstanceHandle retval =
        CreateSceneGraphInstance(ComposerObjectTypes::Path, inParent, pathName.c_str());
    SetUniqueName(retval, pathName.c_str(), inExistingNames);
    CreateSceneGraphInstance(ComposerObjectTypes::Material, retval, "Stroke");
    {
        TInstanceHandle strokeMaterial = GetChild(retval, 0);
        SetName(strokeMaterial, L"Stroke");
    }
    eastl::vector<qt3ds::QT3DSVec2> thePoints;
    if (!m_Builder)
        m_Builder = UICIMP::IPathBufferBuilder::CreateBuilder(m_Foundation);
    else
        m_Builder->Clear();

    for (bool success = m_LuaState.GetChildFromTopOfStackTable(childIndex); success;
         ++childIndex, success = m_LuaState.GetChildFromTopOfStackTable(childIndex)) {
        bool isClosed = ToUICDM(m_LuaState.BooleanFromTopOfStackTable("closepath"), true);
        // Note that this pops the child off the stack so it is no longer available as it is the
        // first argument
        // to the coordinates function.
        if (!m_LuaState.ExecuteFunction("SVGPARSER", "coordinates", 1, 1))
            continue;
        int theTop1 = m_LuaState.GetTop();
        thePoints.clear();
        m_LuaState.ParseFloatingPointPairArray(thePoints);
        int theTop2 = m_LuaState.GetTop();
        QT3DSU32 numAnchors = (thePoints.size() / 3) + 1;
        if (thePoints.size() == 0)
            numAnchors = 0;

        for (QT3DSU32 idx = 0, end = numAnchors; idx < end; ++idx) {
            QT3DSU32 pointIdx = idx * 3;
            QT3DSVec2 position(thePoints[pointIdx]);

            if (idx) {
                QT3DSVec2 c1(thePoints[pointIdx - 2]);
                QT3DSVec2 c2(thePoints[pointIdx - 1]);
                m_Builder->CubicCurveTo(c1, c2, position);
            } else
                m_Builder->MoveTo(position);
        }
        if (isClosed)
            m_Builder->Close();
    }

    Q3DStudio::CString relativePath;
    UICIMP::InstanceDesc thePathItem = m_Import->GetInstanceByHandle(retval);
    UICIMP::SPathBuffer theBuffer(m_Builder->GetPathBuffer());
    relativePath = m_Import->AddPathBuffer(theBuffer, thePathItem.m_Id).m_Value;
    m_Import->SetInstancePropertyValue(retval, m_ObjectTypes.m_Asset.m_SourcePath, relativePath);
    m_Import->SetInstancePropertyValue(retval, m_ObjectTypes.m_Node.m_Opacity, pathOpacity);
    m_Import->SetInstancePropertyValue(retval, m_ObjectTypes.m_Path.m_Width, strokeWidth);

    if (hasFill) {
        m_Import->SetInstancePropertyValue(retval, m_ObjectTypes.m_Path.m_PathType, L"Painted");
        QT3DSI32 fillMaterialIndex = 1;
        if (hasStroke == false) {
            fillMaterialIndex = 0;
            m_Import->SetInstancePropertyValue(retval, m_ObjectTypes.m_Path.m_PaintStyle,
                                               L"Filled");
        } else {
            m_Import->SetInstancePropertyValue(retval, m_ObjectTypes.m_Path.m_PaintStyle,
                                               L"Filled and Stroked");
            CreateSceneGraphInstance(UICDM::ComposerObjectTypes::Material, retval, "Fill");
        }

        TInstanceHandle theFillMaterial = GetChild(retval, fillMaterialIndex);
        m_Import->SetInstancePropertyValue(theFillMaterial, m_ObjectTypes.m_Material.m_DiffuseColor,
                                           ToUICDM(*fillColor));
        m_Import->SetInstancePropertyValue(theFillMaterial, m_ObjectTypes.m_Material.m_Opacity,
                                           fillOpacity);
    }
    if (hasStroke) {
        TInstanceHandle theStrokeMaterial = GetChild(retval, 0);
        m_Import->SetInstancePropertyValue(
            theStrokeMaterial, m_ObjectTypes.m_Material.m_DiffuseColor, ToUICDM(*strokeColor));
        m_Import->SetInstancePropertyValue(theStrokeMaterial, m_ObjectTypes.m_Material.m_Opacity,
                                           strokeOpacity);
    }
    return retval;
}

QT3DSU64 SPathImportTranslator::ParseSVGItem(TInstanceHandle inParent,
                                          eastl::vector<Q3DStudio::CString> &inExistingNames,
                                          bool inIsRoot)
{
    IDynamicLua::Scope __itemScope(m_LuaState);
    eastl::string objType = m_LuaState.StringFromTopOfStackTable("TYPE");
    if (objType == "group")
        return ParseSVGGroup(inParent, inExistingNames, inIsRoot);
    else
        return ParseSVGPath(inParent, inExistingNames);
}

bool SPathImportTranslator::PerformTranslation(UICIMP::Import &import)
{
    m_Import = &import;
    eastl::vector<QT3DSU8> fileData;
    {
        QFile theFile(m_SourceFile);
        if (theFile.open(QIODevice::ReadOnly)) {
            QT3DSU8 buf[1024];

            for (qint64 amountRead = theFile.read(reinterpret_cast<char *>(buf), 1024); amountRead;
                 amountRead = theFile.read(reinterpret_cast<char *>(buf), 1024)) {
                fileData.insert(fileData.end(), buf, buf + amountRead);
            }
        }
    }
    if (fileData.size() == 0)
        return false;

    fileData.push_back(0);
    fileData.push_back(0); // just in case utf-16

    IDynamicLua::Scope __parseScope(m_LuaState);

    eastl::vector<QT3DSVec2> thePoints;
    TIMPHandle retval(0);

    if (m_LuaState.ExecuteFunction("SVGPARSER", "pathsfromxml", (const char8_t *)fileData.data())) {
        // Build list of names already under parent.
        eastl::vector<Q3DStudio::CString> existingNames;
        TIMPHandle newItem = ParseSVGItem(0, existingNames, true);
        if (!retval)
            retval = newItem;
    }
    return retval ? true : false;
}
