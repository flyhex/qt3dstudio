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
#include "Qt3DSImportComposerTypes.h"

using namespace qt3dsimp;

#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    if (AreEqual(inPropertyName, L"" #name))                                                          \
        return TypeToDataType<dtype>();

#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)

#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)                  \
    HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)

DataModelDataType::Value SImportAsset::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_NAMED_PROPERTIES
    ITERATE_COMPOSER_ASSET_PROPERTIES

    QT3DS_ASSERT(false);
    return DataModelDataType::None;
}

DataModelDataType::Value SImportNode::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_NODE_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportModel::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_MODEL_PROPERTIES
    return SImportNode::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportMaterial::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_MATERIAL_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportImage::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_IMAGE_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportPath::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_PATH_PROPERTIES
    return SImportNode::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportSubPath::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_PATH_SUBPATH_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportPathAnchorPoint::GetPropertyDataType(const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_PATH_ANCHOR_POINT_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportControllableObject::GetPropertyDataType(
        const wchar_t *inPropertyName)
{
    ITERATE_COMPOSER_SCENE_PROPERTIES
    ITERATE_COMPOSER_LAYER_PROPERTIES
    ITERATE_COMPOSER_MODEL_PROPERTIES
    ITERATE_COMPOSER_TEXT_PROPERTIES
    ITERATE_COMPOSER_MATERIAL_PROPERTIES
    ITERATE_COMPOSER_NODE_PROPERTIES
    ITERATE_COMPOSER_LIGHT_PROPERTIES
    // Cannot fall back to SImportAsset as we might receive custom material
    // properties which are not known to property system and cause assert.
    return DataModelDataType::None;
}

#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE

#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    if (AreEqual(inPropertyName, #name))                                                           \
        return TypeToDataType<dtype>();

#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)

#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)                  \
    HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)

DataModelDataType::Value SImportAsset::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_NAMED_PROPERTIES
    ITERATE_COMPOSER_ASSET_PROPERTIES

    QT3DS_ASSERT(false);
    return DataModelDataType::None;
}

DataModelDataType::Value SImportNode::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_NODE_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportModel::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_MODEL_PROPERTIES
    return SImportNode::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportMaterial::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_MATERIAL_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportImage::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_IMAGE_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportPath::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_PATH_PROPERTIES
    return SImportNode::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportSubPath::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_PATH_SUBPATH_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportPathAnchorPoint::GetPropertyDataType(const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_PATH_ANCHOR_POINT_PROPERTIES
    return SImportAsset::GetPropertyDataType(inPropertyName);
}

DataModelDataType::Value SImportControllableObject::GetPropertyDataType(
        const char8_t *inPropertyName)
{
    ITERATE_COMPOSER_SCENE_PROPERTIES
    ITERATE_COMPOSER_LAYER_PROPERTIES
    ITERATE_COMPOSER_MODEL_PROPERTIES
    ITERATE_COMPOSER_TEXT_PROPERTIES
    ITERATE_COMPOSER_MATERIAL_PROPERTIES
    ITERATE_COMPOSER_NODE_PROPERTIES
    ITERATE_COMPOSER_LIGHT_PROPERTIES
    // Cannot fall back to SImportAsset as we might receive custom material
    // properties which are not known to property system and cause assert.
    return DataModelDataType::None;
}

#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE

SImportAsset &SImportComposerTypes::GetImportAssetForType(ComposerObjectTypes::Enum inTypeName)
{
    ComposerObjectTypes::Enum theType = inTypeName;
    switch (theType) {
    case ComposerObjectTypes::Asset:
        return m_Asset;
    case ComposerObjectTypes::Group:
        return m_Group;
    case ComposerObjectTypes::Model:
        return m_Model;
    case ComposerObjectTypes::Node:
        return m_Node;
    case ComposerObjectTypes::Image:
        return m_Image;
    case ComposerObjectTypes::Material:
        return m_Material;
    case ComposerObjectTypes::Path:
        return m_Path;
    case ComposerObjectTypes::SubPath:
        return m_SubPath;
    case ComposerObjectTypes::PathAnchorPoint:
        return m_PathAnchorPoint;
    case ComposerObjectTypes::ControllableObject:
        return m_ControllableObject;
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return m_Asset;
}
