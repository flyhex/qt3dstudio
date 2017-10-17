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
#ifndef QT3DS_STATE_EDITOR_VALUE_H
#define QT3DS_STATE_EDITOR_VALUE_H
#pragma once
#include "Qt3DSState.h"
#include "UICStateEditor.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "foundation/Qt3DSVec3.h"

namespace qt3ds {
namespace state {
    namespace editor {

        struct ValueTypes
        {
            enum Enum {
                NoEditorValue = 0,
                String,
                ObjPtr,
                ObjPtrList,
                Vec2,
                Vec2List,
                Vec3,
                Boolean,
                U32,
            };
        };

        template <typename TDataType>
        struct SValueTypeMap
        {
            static ValueTypes::Enum GetType() { return ValueTypes::NoEditorValue; }
        };

        template <>
        struct SValueTypeMap<TEditorStr>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::String; }
        };
        template <>
        struct SValueTypeMap<TObjPtr>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::ObjPtr; }
        };
        template <>
        struct SValueTypeMap<TObjList>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::ObjPtrList; }
        };
        template <>
        struct SValueTypeMap<QT3DSVec2>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::Vec2; }
        };
        template <>
        struct SValueTypeMap<TVec2List>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::Vec2List; }
        };
        template <>
        struct SValueTypeMap<QT3DSVec3>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::Vec3; }
        };
        template <>
        struct SValueTypeMap<bool>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::Boolean; }
        };
        template <>
        struct SValueTypeMap<QT3DSU32>
        {
            static ValueTypes::Enum GetType() { return ValueTypes::U32; }
        };

        struct SValueUnionTraits
        {
            typedef ValueTypes::Enum TIdType;
            enum {
                TBufferSize = sizeof(TVec2List),
            };

            static TIdType getNoDataId() { return ValueTypes::NoEditorValue; }

            template <typename TDataType>
            static TIdType getType()
            {
                return SValueTypeMap<TDataType>().GetType();
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case ValueTypes::String:
                    return inVisitor(*NVUnionCast<TEditorStr *>(inData));
                case ValueTypes::ObjPtr:
                    return inVisitor(*NVUnionCast<TObjPtr *>(inData));
                case ValueTypes::ObjPtrList:
                    return inVisitor(*NVUnionCast<TObjList *>(inData));
                case ValueTypes::Vec2:
                    return inVisitor(*NVUnionCast<QT3DSVec2 *>(inData));
                case ValueTypes::Vec2List:
                    return inVisitor(*NVUnionCast<TVec2List *>(inData));
                case ValueTypes::Vec3:
                    return inVisitor(*NVUnionCast<QT3DSVec3 *>(inData));
                case ValueTypes::Boolean:
                    return inVisitor(*NVUnionCast<bool *>(inData));
                case ValueTypes::U32:
                    return inVisitor(*NVUnionCast<QT3DSU32 *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case ValueTypes::NoEditorValue:
                    return inVisitor();
                }
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case ValueTypes::String:
                    return inVisitor(*NVUnionCast<const TEditorStr *>(inData));
                case ValueTypes::ObjPtr:
                    return inVisitor(*NVUnionCast<const TObjPtr *>(inData));
                case ValueTypes::ObjPtrList:
                    return inVisitor(*NVUnionCast<const TObjList *>(inData));
                case ValueTypes::Vec2:
                    return inVisitor(*NVUnionCast<const QT3DSVec2 *>(inData));
                case ValueTypes::Vec2List:
                    return inVisitor(*NVUnionCast<const TVec2List *>(inData));
                case ValueTypes::Vec3:
                    return inVisitor(*NVUnionCast<const QT3DSVec3 *>(inData));
                case ValueTypes::Boolean:
                    return inVisitor(*NVUnionCast<const bool *>(inData));
                case ValueTypes::U32:
                    return inVisitor(*NVUnionCast<const QT3DSU32 *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case ValueTypes::NoEditorValue:
                    return inVisitor();
                }
            }
        };
    }
}
}

#ifndef _INTEGRITYPLATFORM
// need some specializations in the original nv foundation namespace
namespace qt3ds {
namespace foundation {

    template <>
    struct DestructTraits<QT3DSVec2>
    {
        void destruct(QT3DSVec2 &) {}
    };
    template <>
    struct DestructTraits<QT3DSVec3>
    {
        void destruct(QT3DSVec3 &) {}
    };
}
}
#endif

namespace qt3ds {
namespace state {
    namespace editor {

        typedef qt3ds::foundation::
            DiscriminatedUnion<qt3ds::foundation::
                                   DiscriminatedUnionGenericBase<SValueUnionTraits,
                                                                 SValueUnionTraits::TBufferSize>,
                               SValueUnionTraits::TBufferSize>
                TEditorUnionType;

        struct SValue : public TEditorUnionType
        {
            SValue() {}

            SValue(const SValue &inOther)
                : TEditorUnionType(static_cast<const TEditorUnionType &>(inOther))
            {
            }

            SValue(const char8_t *inOther)
                : TEditorUnionType(TEditorStr(inOther))
            {
            }

            template <typename TDataType>
            SValue(const TDataType &inDt)
                : TEditorUnionType(inDt)
            {
            }

            SValue &operator=(const SValue &inOther)
            {
                TEditorUnionType::operator=(inOther);
                return *this;
            }

            bool operator==(const SValue &inOther) const
            {
                return TEditorUnionType::operator==(inOther);
            }
            bool operator!=(const SValue &inOther) const
            {
                return TEditorUnionType::operator!=(inOther);
            }

            bool empty() const { return getType() == ValueTypes::NoEditorValue; }
        };

        struct SValueOpt : public Option<SValue>
        {
            SValueOpt(const SValueOpt &inOther)
                : Option<SValue>(inOther)
            {
            }
            SValueOpt(const Empty &)
                : Option<SValue>()
            {
            }
            SValueOpt() {}
            template <typename TDataType>
            SValueOpt(const TDataType &inOther)
                : Option<SValue>(SValue(inOther))
            {
            }
            SValueOpt &operator=(const SValueOpt &inOther)
            {
                Option<SValue>::operator=(inOther);
                return *this;
            }
        };
    }
}
}

#endif
