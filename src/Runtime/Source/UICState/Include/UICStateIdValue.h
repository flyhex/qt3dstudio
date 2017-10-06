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
#pragma once
#ifndef UIC_STATE_ID_TYPE_VALUE_H
#define UIC_STATE_ID_TYPE_VALUE_H
#include "UICState.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"

// Discriminated union to unify different things that may be identified by an id
namespace uic {
namespace state {

    struct IdValueTypes
    {
        enum Enum {
            NoIdValue = 0,
            StateNode,
            Send,
        };
    };

    template <typename TDataType>
    struct SIdValueTypeMap
    {
        static IdValueTypes::Enum GetType() { return IdValueTypes::NoIdValue; }
    };

    template <>
    struct SIdValueTypeMap<SStateNode *>
    {
        static IdValueTypes::Enum GetType() { return IdValueTypes::StateNode; }
    };
    template <>
    struct SIdValueTypeMap<SSend *>
    {
        static IdValueTypes::Enum GetType() { return IdValueTypes::Send; }
    };

    struct SIdValueUnionTraits
    {
        typedef IdValueTypes::Enum TIdType;
        enum {
            TBufferSize = sizeof(void *),
        };

        static TIdType getNoDataId() { return IdValueTypes::NoIdValue; }

        template <typename TDataType>
        static TIdType getType()
        {
            return SIdValueTypeMap<TDataType>().GetType();
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case IdValueTypes::StateNode:
                return inVisitor(*NVUnionCast<SStateNode **>(inData));
            case IdValueTypes::Send:
                return inVisitor(*NVUnionCast<SSend **>(inData));
            default:
                QT3DS_ASSERT(false);
            case IdValueTypes::NoIdValue:
                return inVisitor();
            }
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case IdValueTypes::StateNode:
                return inVisitor(*NVUnionCast<const SStateNode **>(inData));
            case IdValueTypes::Send:
                return inVisitor(*NVUnionCast<const SSend **>(inData));
            default:
                QT3DS_ASSERT(false);
            case IdValueTypes::NoIdValue:
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
    struct DestructTraits<uic::state::SStateNode *>
    {
        void destruct(uic::state::SStateNode *) {}
    };
    template <>
    struct DestructTraits<uic::state::SSend *>
    {
        void destruct(uic::state::SSend *) {}
    };
}
}

namespace uic {
namespace state {

    typedef qt3ds::foundation::
        DiscriminatedUnion<qt3ds::foundation::
                               DiscriminatedUnionGenericBase<SIdValueUnionTraits,
                                                             SIdValueUnionTraits::TBufferSize>,
                           SIdValueUnionTraits::TBufferSize>
            TIdUnionType;

    struct SIdValue : public TIdUnionType
    {
        SIdValue() {}

        SIdValue(const SIdValue &inOther)
            : TIdUnionType(static_cast<const TIdUnionType &>(inOther))
        {
        }

        SIdValue(SStateNode *inDt)
            : TIdUnionType(inDt)
        {
        }
        SIdValue(SSend *inDt)
            : TIdUnionType(inDt)
        {
        }

        SIdValue &operator=(const SIdValue &inOther)
        {
            TIdUnionType::operator=(inOther);
            return *this;
        }

        bool operator==(const SIdValue &inOther) const { return TIdUnionType::operator==(inOther); }
        bool operator!=(const SIdValue &inOther) const { return TIdUnionType::operator!=(inOther); }

        bool empty() const { return getType() == IdValueTypes::NoIdValue; }
    };
}
}

#endif
