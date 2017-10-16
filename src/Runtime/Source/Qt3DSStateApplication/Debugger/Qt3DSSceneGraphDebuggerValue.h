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
#ifndef UIC_SCENE_GRAPH_DEBUGGER_VALUE_H
#define UIC_SCENE_GRAPH_DEBUGGER_VALUE_H
#include "Qt3DSSceneGraphDebugger.h"
#include "UICUIADatamodel.h"
#include "UICUIADatamodelValue.h"
#include "UICStateEditorValue.h"

namespace qt3ds {
namespace state {
    namespace debugger {
        struct SGPropertyValueTypes
        {
            enum Enum {
                NoSGValue = 0,
                Float,
                I32,
                String,
                Elem,
            };
        };

        template <typename dtype>
        struct SGValueTypeMap
        {
        };

        template <>
        struct SGValueTypeMap<float>
        {
            static SGPropertyValueTypes::Enum GetType() { return SGPropertyValueTypes::Float; }
        };

        template <>
        struct SGValueTypeMap<QT3DSI32>
        {
            static SGPropertyValueTypes::Enum GetType() { return SGPropertyValueTypes::I32; }
        };

        template <>
        struct SGValueTypeMap<CRegisteredString>
        {
            static SGPropertyValueTypes::Enum GetType() { return SGPropertyValueTypes::String; }
        };

        template <>
        struct SGValueTypeMap<QT3DSU64>
        {
            static SGPropertyValueTypes::Enum GetType() { return SGPropertyValueTypes::Elem; }
        };

        struct SSGValueUnionTraits
        {
            typedef SGPropertyValueTypes::Enum TIdType;
            enum {
                TBufferSize = sizeof(QT3DSU64),
            };

            static TIdType getNoDataId() { return SGPropertyValueTypes::NoSGValue; }

            template <typename TDataType>
            static TIdType getType()
            {
                return SGValueTypeMap<TDataType>().GetType();
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case SGPropertyValueTypes::Float:
                    return inVisitor(*NVUnionCast<float *>(inData));
                case SGPropertyValueTypes::I32:
                    return inVisitor(*NVUnionCast<QT3DSI32 *>(inData));
                case SGPropertyValueTypes::String:
                    return inVisitor(*NVUnionCast<CRegisteredString *>(inData));
                case SGPropertyValueTypes::Elem:
                    return inVisitor(*NVUnionCast<QT3DSU64 *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case SGPropertyValueTypes::NoSGValue:
                    return inVisitor();
                }
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case SGPropertyValueTypes::Float:
                    return inVisitor(*NVUnionCast<const float *>(inData));
                case SGPropertyValueTypes::I32:
                    return inVisitor(*NVUnionCast<const QT3DSI32 *>(inData));
                case SGPropertyValueTypes::String:
                    return inVisitor(*NVUnionCast<const CRegisteredString *>(inData));
                case SGPropertyValueTypes::Elem:
                    return inVisitor(*NVUnionCast<const QT3DSU64 *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case SGPropertyValueTypes::NoSGValue:
                    return inVisitor();
                }
            }
        };

        typedef qt3ds::foundation::
            DiscriminatedUnion<qt3ds::foundation::
                                   DiscriminatedUnionGenericBase<SSGValueUnionTraits,
                                                                 SSGValueUnionTraits::TBufferSize>,
                               SSGValueUnionTraits::TBufferSize>
                TSGValueUnionType;

        struct SSGValue : public TSGValueUnionType
        {
            SSGValue() {}
            SSGValue(const SSGValue &other)
                : TSGValueUnionType(static_cast<const TSGValueUnionType &>(other))
            {
            }
            SSGValue(float val)
                : TSGValueUnionType(val)
            {
            }
            SSGValue(QT3DSI32 val)
                : TSGValueUnionType(val)
            {
            }
            SSGValue(CRegisteredString val)
                : TSGValueUnionType(val)
            {
            }
            SSGValue(QT3DSU64 val)
                : TSGValueUnionType(val)
            {
            }

            SSGValue &operator=(const SSGValue &other)
            {
                TSGValueUnionType::operator=(static_cast<const TSGValueUnionType &>(other));
                return *this;
            }
        };

        struct SSGPropertyChange
        {
            QT3DSI32 m_Hash;
            SSGValue m_Value;
            SSGPropertyChange(QT3DSI32 h, const SSGValue &v)
                : m_Hash(h)
                , m_Value(v)
            {
            }
            SSGPropertyChange()
                : m_Hash(0)
            {
            }
        };
    }
}
}

#endif
