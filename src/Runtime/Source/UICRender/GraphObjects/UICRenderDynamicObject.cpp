/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "UICRender.h"
#include "UICRenderDynamicObject.h"
#include "UICRenderDynamicObjectSystem.h"
#include "foundation/FileTools.h"
#include "UICRenderString.h"

#include <QtCore/qdir.h>

using namespace uic;
using namespace uic::render;

SDynamicObject::SDynamicObject(GraphObjectTypes::Enum inType, CRegisteredString inObjName,
                               QT3DSU32 inDSByteSize, QT3DSU32 thisObjSize)
    : SGraphObject(inType)
    , m_ClassName(inObjName)
    , m_DataSectionByteSize(inDSByteSize)
    , m_ThisObjectSize(thisObjSize)
{
}

template <typename TDataType>
void SDynamicObject::SetPropertyValueT(const dynamic::SPropertyDefinition &inDefinition,
                                       const TDataType &inValue)
{
    if (sizeof(inValue) != inDefinition.m_ByteSize) {
        QT3DS_ASSERT(false);
        return;
    }
    memCopy(GetDataSectionBegin() + inDefinition.m_Offset, &inValue, sizeof(inValue));
}

void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      bool inValue)
{
    SetPropertyValueT(inDefinition, inValue);
}

void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      QT3DSF32 inValue)
{
    SetPropertyValueT(inDefinition, inValue);
}
void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      QT3DSF32 inValue, QT3DSU32 inOffset)
{
    if (sizeof(QT3DSF32) > (inDefinition.m_ByteSize - inOffset)) {
        QT3DS_ASSERT(false);
        return;
    }
    memCopy(GetDataSectionBegin() + inDefinition.m_Offset + inOffset, &inValue, sizeof(inValue));
}
void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      const QT3DSVec2 &inValue)
{
    SetPropertyValueT(inDefinition, inValue);
}
void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      const QT3DSVec3 &inValue)
{
    SetPropertyValueT(inDefinition, inValue);
}
void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      const QT3DSVec4 &inValue)
{
    SetPropertyValueT(inDefinition, inValue);
}
void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      QT3DSI32 inValue)
{
    SetPropertyValueT(inDefinition, inValue);
}
void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      CRegisteredString inValue)
{
    QT3DS_ASSERT(inDefinition.m_DataType == NVRenderShaderDataTypes::NVRenderTexture2DPtr);
    SetPropertyValueT(inDefinition, inValue);
}
template <typename TStrType>
void SDynamicObject::SetStrPropertyValueT(dynamic::SPropertyDefinition &inDefinition,
                                          const char8_t *inValue, const char8_t *inProjectDir,
                                          TStrType &ioWorkspace, IStringTable &inStrTable)
{
    if (inValue == NULL)
        inValue = "";
    if (inDefinition.m_DataType == NVRenderShaderDataTypes::QT3DSI32) {
        NVConstDataRef<CRegisteredString> theEnumValues = inDefinition.m_EnumValueNames;
        for (QT3DSI32 idx = 0, end = (QT3DSI32)theEnumValues.size(); idx < end; ++idx) {
            if (strcmp(theEnumValues[idx].c_str(), inValue) == 0) {
                SetPropertyValueT(inDefinition, idx);
                break;
            }
        }
    } else if (inDefinition.m_DataType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
        if (inProjectDir == NULL)
            inProjectDir = "";
        if (CFileTools::RequiresCombineBaseAndRelative(inValue)) {
            QString absolute = QDir(inProjectDir).filePath(inValue);
            ioWorkspace.assign(absolute.toLatin1().constData());
            SetPropertyValueT(inDefinition, inStrTable.RegisterStr(ioWorkspace.c_str()));
            // We also adjust the image path in the definition
            // I could not find a better place
            inDefinition.m_ImagePath = inStrTable.RegisterStr(ioWorkspace.c_str());
        } else {
            SetPropertyValueT(inDefinition, inStrTable.RegisterStr(inValue));
        }
    } else if (inDefinition.m_DataType == NVRenderShaderDataTypes::NVRenderImage2DPtr) {
        SetPropertyValueT(inDefinition, inStrTable.RegisterStr(inValue));
    } else if (inDefinition.m_DataType == NVRenderShaderDataTypes::NVRenderDataBufferPtr) {
        SetPropertyValueT(inDefinition, inStrTable.RegisterStr(inValue));
    } else {
        QT3DS_ASSERT(false);
    }
}

void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      const char8_t *inValue, const char8_t *inProjectDir,
                                      CRenderString &ioWorkspace, IStringTable &inStrTable)
{
    SetStrPropertyValueT(const_cast<dynamic::SPropertyDefinition &>(inDefinition), inValue,
                         inProjectDir, ioWorkspace, inStrTable);
}

void SDynamicObject::SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                                      const char8_t *inValue, const char8_t *inProjectDir,
                                      eastl::string &ioWorkspace, IStringTable &inStrTable)
{
    SetStrPropertyValueT(const_cast<dynamic::SPropertyDefinition &>(inDefinition), inValue,
                         inProjectDir, ioWorkspace, inStrTable);
}
