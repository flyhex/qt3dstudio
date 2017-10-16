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
#ifndef UIC_STATE_VISUAL_BINDING_CONTEXT_VALUES_H
#define UIC_STATE_VISUAL_BINDING_CONTEXT_VALUES_H
#include "Qt3DSStateVisualBindingContext.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "Qt3DSStateVisualBindingContextCommands.h"

namespace qt3ds {
namespace state {

    template <typename TDataType>
    struct SVisualStateCommandTypeMap
    {
    };

    template <>
    struct SVisualStateCommandTypeMap<SGotoSlide>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::GotoSlide;
        }
    };
    template <>
    struct SVisualStateCommandTypeMap<SCallFunction>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::CallFunction;
        }
    };
    template <>
    struct SVisualStateCommandTypeMap<SSetAttribute>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::SetAttribute;
        }
    };
    template <>
    struct SVisualStateCommandTypeMap<SGotoSlideRelative>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::GotoSlideRelative;
        }
    };
    template <>
    struct SVisualStateCommandTypeMap<SFireEvent>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::FireEvent;
        }
    };
    template <>
    struct SVisualStateCommandTypeMap<SPresentationAttribute>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::PresentationAttribute;
        }
    };
    template <>
    struct SVisualStateCommandTypeMap<SPlaySound>
    {
        static VisualStateCommandTypes::Enum GetType()
        {
            return VisualStateCommandTypes::PlaySound;
        }
    };

    struct SVisualStateCommandUnionTraits
    {
        typedef VisualStateCommandTypes::Enum TIdType;
        enum {
            TBufferSize = sizeof(SGotoSlideRelative),
        };

        static TIdType getNoDataId() { return VisualStateCommandTypes::NoVisualStateCommand; }

        template <typename TDataType>
        static TIdType getType()
        {
            return SVisualStateCommandTypeMap<TDataType>().GetType();
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case VisualStateCommandTypes::GotoSlide:
                return inVisitor(*NVUnionCast<SGotoSlide *>(inData));
            case VisualStateCommandTypes::CallFunction:
                return inVisitor(*NVUnionCast<SCallFunction *>(inData));
            case VisualStateCommandTypes::SetAttribute:
                return inVisitor(*NVUnionCast<SSetAttribute *>(inData));
            case VisualStateCommandTypes::GotoSlideRelative:
                return inVisitor(*NVUnionCast<SGotoSlideRelative *>(inData));
            case VisualStateCommandTypes::FireEvent:
                return inVisitor(*NVUnionCast<SFireEvent *>(inData));
            case VisualStateCommandTypes::PresentationAttribute:
                return inVisitor(*NVUnionCast<SPresentationAttribute *>(inData));
            case VisualStateCommandTypes::PlaySound:
                return inVisitor(*NVUnionCast<SPlaySound *>(inData));
            default:
                QT3DS_ASSERT(false);
            case VisualStateCommandTypes::NoVisualStateCommand:
                return inVisitor();
            }
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case VisualStateCommandTypes::GotoSlide:
                return inVisitor(*NVUnionCast<const SGotoSlide *>(inData));
            case VisualStateCommandTypes::CallFunction:
                return inVisitor(*NVUnionCast<const SCallFunction *>(inData));
            case VisualStateCommandTypes::SetAttribute:
                return inVisitor(*NVUnionCast<const SSetAttribute *>(inData));
            case VisualStateCommandTypes::GotoSlideRelative:
                return inVisitor(*NVUnionCast<const SGotoSlideRelative *>(inData));
            case VisualStateCommandTypes::FireEvent:
                return inVisitor(*NVUnionCast<const SFireEvent *>(inData));
            case VisualStateCommandTypes::PresentationAttribute:
                return inVisitor(*NVUnionCast<const SPresentationAttribute *>(inData));
            case VisualStateCommandTypes::PlaySound:
                return inVisitor(*NVUnionCast<const SPlaySound *>(inData));
            default:
                QT3DS_ASSERT(false);
            case VisualStateCommandTypes::NoVisualStateCommand:
                return inVisitor();
            }
        }
    };

    typedef qt3ds::foundation::
        DiscriminatedUnion<qt3ds::foundation::
                               DiscriminatedUnionGenericBase<SVisualStateCommandUnionTraits,
                                                             SVisualStateCommandUnionTraits::
                                                                 TBufferSize>,
                           SVisualStateCommandUnionTraits::TBufferSize>
            TVisualStateCommandUnionType;

    struct SVisualStateCommand : public TVisualStateCommandUnionType
    {
        SVisualStateCommand() {}

        SVisualStateCommand(const SVisualStateCommand &inOther)
            : TVisualStateCommandUnionType(
                  static_cast<const TVisualStateCommandUnionType &>(inOther))
        {
        }

        template <typename TDataType>
        SVisualStateCommand(const TDataType &inDt)
            : TVisualStateCommandUnionType(inDt)
        {
        }

        SVisualStateCommand &operator=(const SVisualStateCommand &inOther)
        {
            TVisualStateCommandUnionType::operator=(inOther);
            return *this;
        }

        bool operator==(const SVisualStateCommand &inOther) const
        {
            return TVisualStateCommandUnionType::operator==(inOther);
        }
        bool operator!=(const SVisualStateCommand &inOther) const
        {
            return TVisualStateCommandUnionType::operator!=(inOther);
        }

        bool empty() const { return getType() == VisualStateCommandTypes::NoVisualStateCommand; }
    };
}
}
#ifndef _INTEGRITYPLATFORM
namespace qt3ds {
namespace foundation {

    template <>
    struct DestructTraits<qt3ds::state::SGotoSlide>
    {
        void destruct(qt3ds::state::SGotoSlide &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::SCallFunction>
    {
        void destruct(qt3ds::state::SCallFunction &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::SSetAttribute>
    {
        void destruct(qt3ds::state::SSetAttribute &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::SGotoSlideRelative>
    {
        void destruct(qt3ds::state::SGotoSlideRelative &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::SFireEvent>
    {
        void destruct(qt3ds::state::SFireEvent &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::SPresentationAttribute>
    {
        void destruct(qt3ds::state::SPresentationAttribute &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::SPlaySound>
    {
        void destruct(qt3ds::state::SPlaySound &) {}
    };
}
}
#endif
#endif