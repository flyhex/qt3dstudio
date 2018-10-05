/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
#ifndef QT3DS_STUDIO_PICK_VALUES_H
#define QT3DS_STUDIO_PICK_VALUES_H
#pragma once
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "foundation/Qt3DSUnionCast.h"
#include "Qt3DSDMHandles.h"
#include "StaticMaxSize.h"

namespace Q3DStudio {

    struct StudioPickValueTypes
    {
        enum Enum {
            UnknownValueType = 0,
            Instance,
            Widget,
            Guide,
            Path,
            Pending,
        };
    };

    struct SWidgetPick
    {
        qt3ds::QT3DSI32 m_WidgetId;
        SWidgetPick(qt3ds::QT3DSI32 id = 0)
            : m_WidgetId(id)
        {
        }
    };

    struct SPathPick
    {
        enum EAnchorProperty {
            Anchor = 0,
            IncomingControl,
            OutgoingControl,
        };

        qt3ds::QT3DSU32 m_AnchorIndex;
        EAnchorProperty m_Property;

        SPathPick()
            : m_AnchorIndex(0)
            , m_Property(Anchor)
        {
        }

        SPathPick(qt3ds::QT3DSU32 ai, EAnchorProperty p)
            : m_AnchorIndex(ai)
            , m_Property(p)
        {
        }
    };

    template <typename TDataType>
    struct SStudioPickValueTypeMap
    {
    };

    template <>
    struct SStudioPickValueTypeMap<qt3dsdm::Qt3DSDMInstanceHandle>
    {
        static StudioPickValueTypes::Enum GetType() { return StudioPickValueTypes::Instance; }
    };

    template <>
    struct SStudioPickValueTypeMap<SWidgetPick>
    {
        static StudioPickValueTypes::Enum GetType() { return StudioPickValueTypes::Widget; }
    };

    template <>
    struct SStudioPickValueTypeMap<qt3dsdm::Qt3DSDMGuideHandle>
    {
        static StudioPickValueTypes::Enum GetType() { return StudioPickValueTypes::Guide; }
    };

    template <>
    struct SStudioPickValueTypeMap<SPathPick>
    {
        static StudioPickValueTypes::Enum GetType() { return StudioPickValueTypes::Path; }
    };

    template <>
    struct SStudioPickValueTypeMap<StudioPickValueTypes::Enum>
    {
        static StudioPickValueTypes::Enum GetType() { return StudioPickValueTypes::Pending; }
    };

    struct SStudioPickValueTraits
    {
        typedef StudioPickValueTypes::Enum TIdType;
        enum {
            TBufferSize = Q3DStudio::StaticMaxSize<qt3dsdm::Qt3DSDMInstanceHandle,
                                                   SWidgetPick,
                                                   qt3dsdm::Qt3DSDMGuideHandle,
                                                   SPathPick>::value
        };

        static TIdType getNoDataId() { return StudioPickValueTypes::UnknownValueType; }

        template <typename TDataType>
        static TIdType getType()
        {
            return SStudioPickValueTypeMap<TDataType>().GetType();
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case StudioPickValueTypes::Instance:
                return inVisitor(*qt3ds::NVUnionCast<qt3dsdm::Qt3DSDMInstanceHandle *>(inData));
            case StudioPickValueTypes::Widget:
                return inVisitor(*qt3ds::NVUnionCast<SWidgetPick *>(inData));
            case StudioPickValueTypes::Guide:
                return inVisitor(*qt3ds::NVUnionCast<qt3dsdm::Qt3DSDMGuideHandle *>(inData));
            case StudioPickValueTypes::Path:
                return inVisitor(*qt3ds::NVUnionCast<SPathPick *>(inData));
            default:
                QT3DS_ASSERT(false);
            case StudioPickValueTypes::Pending:
                break;
            case StudioPickValueTypes::UnknownValueType:
                return inVisitor();
            }
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case StudioPickValueTypes::Instance:
                return inVisitor(*qt3ds::NVUnionCast<const qt3dsdm::Qt3DSDMInstanceHandle *>(inData));
            case StudioPickValueTypes::Widget:
                return inVisitor(*qt3ds::NVUnionCast<const SWidgetPick *>(inData));
            case StudioPickValueTypes::Guide:
                return inVisitor(*qt3ds::NVUnionCast<const qt3dsdm::Qt3DSDMGuideHandle *>(inData));
            case StudioPickValueTypes::Path:
                return inVisitor(*qt3ds::NVUnionCast<const SPathPick *>(inData));
            default:
                QT3DS_ASSERT(false);
            case StudioPickValueTypes::Pending:
                break;
            case StudioPickValueTypes::UnknownValueType:
                return inVisitor();
            }
        }
    };

    typedef qt3ds::foundation::
        DiscriminatedUnion<qt3ds::foundation::
                               DiscriminatedUnionGenericBase<SStudioPickValueTraits,
                                                             SStudioPickValueTraits::TBufferSize>,
                           SStudioPickValueTraits::TBufferSize>
            TStudioPickValueType;

    struct SStudioPickValue : public TStudioPickValueType
    {
        SStudioPickValue() {}
        SStudioPickValue(qt3dsdm::Qt3DSDMInstanceHandle inst)
            : TStudioPickValueType(inst)
        {
        }
        SStudioPickValue(SWidgetPick inst)
            : TStudioPickValueType(inst)
        {
        }
        SStudioPickValue(qt3dsdm::Qt3DSDMGuideHandle inst)
            : TStudioPickValueType(inst)
        {
        }
        SStudioPickValue(SPathPick inst)
            : TStudioPickValueType(inst)
        {
        }
        SStudioPickValue(StudioPickValueTypes::Enum inst)
            : TStudioPickValueType(inst)
        {
        }
        SStudioPickValue(const SStudioPickValue &other)
            : TStudioPickValueType(static_cast<const TStudioPickValueType &>(other))
        {
        }
        SStudioPickValue &operator=(const SStudioPickValue &other)
        {
            TStudioPickValueType::operator=(static_cast<const TStudioPickValueType &>(other));
            return *this;
        }
        int GetWidgetId() const
        {
            if (getType() == StudioPickValueTypes::Widget)
                return getData<SWidgetPick>().m_WidgetId;
            return 0;
        }
    };
}

#endif
