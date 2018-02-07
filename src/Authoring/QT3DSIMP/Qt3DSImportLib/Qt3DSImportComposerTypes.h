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
#pragma once
#ifndef QT3DS_IMPORT_COMPOSER_TYPES_H
#define QT3DS_IMPORT_COMPOSER_TYPES_H
#include "Qt3DSDMComposerTypeDefinitions.h"

// Define the subset of types that the import library supports.
namespace qt3dsimp {
using namespace qt3dsdm;

template <typename TDataType>
struct SImportPropertyDefinition
{
    const ComposerPropertyNames::Enum m_Name;
    ComposerPropertyNames::Enum GetName() const { return m_Name; }

    SImportPropertyDefinition(ComposerPropertyNames::Enum inName)
        : m_Name(inName)
    {
    }
};

template <ComposerPropertyNames::Enum TEnumName, typename TDataType>
struct SSpecificImportPropertyDefinition : public SImportPropertyDefinition<TDataType>
{
    SSpecificImportPropertyDefinition()
        : SImportPropertyDefinition<TDataType>(TEnumName)
    {
    }
};

#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    SSpecificImportPropertyDefinition<ComposerPropertyNames::name, dtype> memberName;

#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)

#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)                  \
    HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)

struct SImportAsset
{
    ITERATE_COMPOSER_NAMED_PROPERTIES
    ITERATE_COMPOSER_ASSET_PROPERTIES
    virtual DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName);
    virtual DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName);
    virtual ComposerObjectTypes::Enum GetObjectType() { return ComposerObjectTypes::Asset; }
    const wchar_t *GetObjectName() { return ComposerObjectTypes::Convert(GetObjectType()); }
};

struct SImportNode : public SImportAsset
{
    ITERATE_COMPOSER_NODE_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::Node; }
};

struct SImportGroup : public SImportNode
{
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::Group; }
};

struct SImportModel : public SImportNode
{
    ITERATE_COMPOSER_MODEL_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::Model; }
};

struct SImportMaterial : public SImportAsset
{
    ITERATE_COMPOSER_MATERIAL_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::Material; }
};

struct SImportImage : public SImportAsset
{
    ITERATE_COMPOSER_IMAGE_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::Image; }
};

struct SImportPath : public SImportNode
{
    ITERATE_COMPOSER_PATH_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::Path; }
};

struct SImportSubPath : public SImportAsset
{
    ITERATE_COMPOSER_PATH_SUBPATH_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override { return ComposerObjectTypes::SubPath; }
};

struct SImportPathAnchorPoint : public SImportAsset
{
    ITERATE_COMPOSER_PATH_ANCHOR_POINT_PROPERTIES
    DataModelDataType::Value GetPropertyDataType(const wchar_t *inPropertyName) override;
    DataModelDataType::Value GetPropertyDataType(const char8_t *inPropertyName) override;
    ComposerObjectTypes::Enum GetObjectType() override
    {
        return ComposerObjectTypes::PathAnchorPoint;
    }
};

#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT

template <typename TOperator>
inline void ImportVisitPropertyType(ComposerPropertyNames::Enum inName, TOperator &inOperator)
{
    switch (inName) {
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    case ComposerPropertyNames::name:                                                              \
        inOperator.template Handle<dtype>(inName);                                                 \
        return;
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)
#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)

        ITERATE_COMPOSER_NAMED_PROPERTIES
        ITERATE_COMPOSER_ASSET_PROPERTIES
        ITERATE_COMPOSER_NODE_PROPERTIES
        ITERATE_COMPOSER_MATERIAL_PROPERTIES
        ITERATE_COMPOSER_IMAGE_PROPERTIES
        ITERATE_COMPOSER_PATH_PROPERTIES
        ITERATE_COMPOSER_PATH_SUBPATH_PROPERTIES
        ITERATE_COMPOSER_PATH_ANCHOR_POINT_PROPERTIES

#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
    }
    QT3DS_ASSERT(false);
}

struct SImportComposerTypes
{
    SImportAsset m_Asset;
    SImportGroup m_Group;
    SImportModel m_Model;
    SImportNode m_Node;
    SImportMaterial m_Material;
    SImportImage m_Image;
    SImportPath m_Path;
    SImportSubPath m_SubPath;
    SImportPathAnchorPoint m_PathAnchorPoint;

    SImportAsset &GetImportAssetForType(ComposerObjectTypes::Enum inTypeName);
};
};

#endif
