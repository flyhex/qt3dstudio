/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef UIA_DATAMODEL_VALUE_H
#define UIA_DATAMODEL_VALUE_H
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "Qt3DSUIADatamodel.h"

namespace qt3ds {
namespace app {

    using namespace Q3DStudio;

    template <typename TDatatype>
    struct SDatamodelValueTypeMap
    {
    };

#define QT3DS_UIA_DATAMODEL_TYPE_MAP(type, enumname)                                                 \
    template <>                                                                                    \
    struct SDatamodelValueTypeMap<type>                                                            \
    {                                                                                              \
        static Q3DStudio::ERuntimeDataModelDataType GetType() { return Q3DStudio::enumname; }      \
    };

    struct SGuid
    {
        long m_Data[4];
    };

    struct SObjectRef
    {
        CRegisteredString m_Presentation;
        CRegisteredString m_Id;
    };

    class CStringOrInt
    {
        bool m_IsString;
        CRegisteredString m_String;
        int m_IntValue;

    public:
        CStringOrInt(CRegisteredString val = CRegisteredString())
            : m_IsString(true)
            , m_String(val)
            , m_IntValue(0)
        {
        }
        CStringOrInt(int val)
            : m_IsString(false)
            , m_IntValue(val)
        {
        }
        CStringOrInt(const CStringOrInt &other)
            : m_IsString(other.m_IsString)
            , m_String(other.m_String)
            , m_IntValue(other.m_IntValue)
        {
        }
        CStringOrInt &operator=(const CStringOrInt &other)
        {
            m_IsString = other.m_IsString;
            m_String = other.m_String;
            m_IntValue = other.m_IntValue;
            return *this;
        }
        bool IsString() const { return m_IsString; }
        CRegisteredString StringValue() const
        {
            QT3DS_ASSERT(m_IsString);
            return m_String;
        }
        int IntValue() const
        {
            QT3DS_ASSERT(m_IsString == false);
            return m_IntValue;
        }
    };

    QT3DS_UIA_DATAMODEL_TYPE_MAP(float, ERuntimeDataModelDataTypeFloat);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(QT3DSVec2, ERuntimeDataModelDataTypeFloat2);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(QT3DSVec3, ERuntimeDataModelDataTypeFloat3);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(QT3DSI32, ERuntimeDataModelDataTypeLong);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(eastl::string, ERuntimeDataModelDataTypeString);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(bool, ERuntimeDataModelDataTypeBool);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(SGuid, ERuntimeDataModelDataTypeLong4);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(CRegisteredString, ERuntimeDataModelDataTypeStringRef);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(SObjectRef, ERuntimeDataModelDataTypeObjectRef);
    QT3DS_UIA_DATAMODEL_TYPE_MAP(CStringOrInt, ERuntimeDataModelDataTypeStringOrInt);

    struct SDatamodelValueUnionTraits
    {
        typedef ERuntimeDataModelDataType TIdType;
        enum {
            TBufferSize = sizeof(eastl::string),
        };

        static TIdType getNoDataId() { return ERuntimeDataModelDataTypeNone; }

        template <typename TDataType>
        static TIdType getType()
        {
            return SDatamodelValueTypeMap<TDataType>().GetType();
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case ERuntimeDataModelDataTypeFloat:
                return inVisitor(*NVUnionCast<float *>(inData));
            case ERuntimeDataModelDataTypeFloat2:
                return inVisitor(*NVUnionCast<QT3DSVec2 *>(inData));
            case ERuntimeDataModelDataTypeFloat3:
                return inVisitor(*NVUnionCast<QT3DSVec3 *>(inData));
            case ERuntimeDataModelDataTypeLong:
                return inVisitor(*NVUnionCast<QT3DSI32 *>(inData));
            case ERuntimeDataModelDataTypeString:
                return inVisitor(*NVUnionCast<eastl::string *>(inData));
            case ERuntimeDataModelDataTypeBool:
                return inVisitor(*NVUnionCast<bool *>(inData));
            case ERuntimeDataModelDataTypeLong4:
                return inVisitor(*NVUnionCast<SGuid *>(inData));
            case ERuntimeDataModelDataTypeStringRef:
                return inVisitor(*NVUnionCast<CRegisteredString *>(inData));
            case ERuntimeDataModelDataTypeObjectRef:
                return inVisitor(*NVUnionCast<SObjectRef *>(inData));
            case ERuntimeDataModelDataTypeStringOrInt:
                return inVisitor(*NVUnionCast<CStringOrInt *>(inData));
            default:
                QT3DS_ASSERT(false);
            case ERuntimeDataModelDataTypeNone:
                return inVisitor();
            }
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case ERuntimeDataModelDataTypeFloat:
                return inVisitor(*NVUnionCast<const float *>(inData));
            case ERuntimeDataModelDataTypeFloat2:
                return inVisitor(*NVUnionCast<const QT3DSVec2 *>(inData));
            case ERuntimeDataModelDataTypeFloat3:
                return inVisitor(*NVUnionCast<const QT3DSVec3 *>(inData));
            case ERuntimeDataModelDataTypeLong:
                return inVisitor(*NVUnionCast<const QT3DSI32 *>(inData));
            case ERuntimeDataModelDataTypeString:
                return inVisitor(*NVUnionCast<const eastl::string *>(inData));
            case ERuntimeDataModelDataTypeBool:
                return inVisitor(*NVUnionCast<const bool *>(inData));
            case ERuntimeDataModelDataTypeLong4:
                return inVisitor(*NVUnionCast<const SGuid *>(inData));
            case ERuntimeDataModelDataTypeStringRef:
                return inVisitor(*NVUnionCast<const CRegisteredString *>(inData));
            case ERuntimeDataModelDataTypeObjectRef:
                return inVisitor(*NVUnionCast<const SObjectRef *>(inData));
            case ERuntimeDataModelDataTypeStringOrInt:
                return inVisitor(*NVUnionCast<const CStringOrInt *>(inData));
            default:
                QT3DS_ASSERT(false);
            case ERuntimeDataModelDataTypeNone:
                return inVisitor();
            }
        }
    };
}
}

// need some specializations in the original nv foundation namespace
namespace qt3ds {
namespace foundation {

    template <>
    struct DestructTraits<qt3ds::app::SGuid>
    {
        void destruct(qt3ds::app::SGuid &) {}
    };
    template <>
    struct DestructTraits<qt3ds::app::SObjectRef>
    {
        void destruct(qt3ds::app::SObjectRef &) {}
    };
    template <>
    struct DestructTraits<qt3ds::app::CStringOrInt>
    {
        void destruct(qt3ds::app::CStringOrInt &) {}
    };
    template <>
    struct DestructTraits<CRegisteredString>
    {
        void destruct(CRegisteredString &) {}
    };
}
}

namespace qt3ds {
namespace app {

    typedef qt3ds::foundation::
        DiscriminatedUnion<qt3ds::foundation::
                               DiscriminatedUnionGenericBase<SDatamodelValueUnionTraits,
                                                             SDatamodelValueUnionTraits::
                                                                 TBufferSize>,
                           SDatamodelValueUnionTraits::TBufferSize>
            TDatamodelUnionType;

    struct SDatamodelValue : public TDatamodelUnionType
    {
        SDatamodelValue() {}
        SDatamodelValue(const SDatamodelValue &other)
            : TDatamodelUnionType(static_cast<const TDatamodelUnionType &>(other))
        {
        }
        SDatamodelValue(float other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(QT3DSVec2 other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(QT3DSVec3 other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(QT3DSI32 other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(const eastl::string &other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(bool other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(SGuid other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(CRegisteredString other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(SObjectRef other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue(CStringOrInt other)
            : TDatamodelUnionType(other)
        {
        }
        SDatamodelValue &operator=(const SDatamodelValue &other)
        {
            TDatamodelUnionType::operator=(other);
            return *this;
        }
    };
}
}
#endif
