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
#ifndef UIC_STATE_DEBUGGER_VALUES_H
#define UIC_STATE_DEBUGGER_VALUES_H
#pragma once
#include "Qt3DSStateDebugger.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"

namespace qt3ds {
namespace state {
    namespace debugger {
        using namespace qt3ds::state::editor;

        // Force compile error if unsupported datatype requested
        template <typename TDataType>
        struct SDebugValueTypeMap
        {
        };

        template <>
        struct SDebugValueTypeMap<TDebugStr>
        {
            static DebugValueTypes::Enum GetType() { return DebugValueTypes::String; }
        };
        template <>
        struct SDebugValueTypeMap<TDebugStrList>
        {
            static DebugValueTypes::Enum GetType() { return DebugValueTypes::StringList; }
        };
        template <>
        struct SDebugValueTypeMap<TTransitionIdList>
        {
            static DebugValueTypes::Enum GetType() { return DebugValueTypes::TransitionIdList; }
        };

        struct SDebugValueUnionTraits
        {
            typedef DebugValueTypes::Enum TIdType;
            enum {
                TBufferSize = sizeof(TDebugStrList),
            };

            static TIdType getNoDataId() { return DebugValueTypes::NoDebugValue; }

            template <typename TDataType>
            static TIdType getType()
            {
                return SDebugValueTypeMap<TDataType>().GetType();
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case DebugValueTypes::String:
                    return inVisitor(*NVUnionCast<TDebugStr *>(inData));
                case DebugValueTypes::StringList:
                    return inVisitor(*NVUnionCast<TDebugStrList *>(inData));
                case DebugValueTypes::TransitionIdList:
                    return inVisitor(*NVUnionCast<TTransitionIdList *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case DebugValueTypes::NoDebugValue:
                    return inVisitor();
                }
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case DebugValueTypes::String:
                    return inVisitor(*NVUnionCast<const TDebugStr *>(inData));
                case DebugValueTypes::StringList:
                    return inVisitor(*NVUnionCast<const TDebugStrList *>(inData));
                case DebugValueTypes::TransitionIdList:
                    return inVisitor(*NVUnionCast<const TTransitionIdList *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case DebugValueTypes::NoDebugValue:
                    return inVisitor();
                }
            }
        };

        typedef qt3ds::foundation::
            DiscriminatedUnion<qt3ds::foundation::
                                   DiscriminatedUnionGenericBase<SDebugValueUnionTraits,
                                                                 SDebugValueUnionTraits::
                                                                     TBufferSize>,
                               SDebugValueUnionTraits::TBufferSize>
                TDebugValueUnionType;

        struct SDebugValue : public TDebugValueUnionType
        {
            SDebugValue() {}

            SDebugValue(const SDebugValue &inOther)
                : TDebugValueUnionType(static_cast<const TDebugValueUnionType &>(inOther))
            {
            }

            SDebugValue(const char8_t *inOther)
                : TDebugValueUnionType(TDebugStr(inOther))
            {
            }

            template <typename TDataType>
            SDebugValue(const TDataType &inDt)
                : TDebugValueUnionType(inDt)
            {
            }

            SDebugValue &operator=(const SDebugValue &inOther)
            {
                TDebugValueUnionType::operator=(inOther);
                return *this;
            }

            bool operator==(const SDebugValue &inOther) const
            {
                return TDebugValueUnionType::operator==(inOther);
            }
            bool operator!=(const SDebugValue &inOther) const
            {
                return TDebugValueUnionType::operator!=(inOther);
            }

            bool empty() const { return getType() == DebugValueTypes::NoDebugValue; }
        };

        struct SDebugValueOpt : public Option<SDebugValue>
        {
            SDebugValueOpt(const SDebugValueOpt &inOther)
                : Option<SDebugValue>(inOther)
            {
            }
            SDebugValueOpt(const Empty &)
                : Option<SDebugValue>()
            {
            }
            SDebugValueOpt() {}
            template <typename TDataType>
            SDebugValueOpt(const TDataType &inOther)
                : Option<SDebugValue>(SDebugValue(inOther))
            {
            }
            SDebugValueOpt &operator=(const SDebugValueOpt &inOther)
            {
                Option<SDebugValue>::operator=(inOther);
                return *this;
            }
        };

        // Breakpoint subtypes

        struct SStateEnterBreakpoint
        {
            TDebugStr m_ObjectId;
            SStateEnterBreakpoint(const TDebugStr &inObj = TDebugStr())
                : m_ObjectId(inObj)
            {
            }
            bool operator==(const SStateEnterBreakpoint &inOther) const
            {
                return m_ObjectId == inOther.m_ObjectId;
            }
        };

        struct SStateExitBreakpoint
        {
            TDebugStr m_ObjectId;
            SStateExitBreakpoint(const TDebugStr &inObj = TDebugStr())
                : m_ObjectId(inObj)
            {
            }
            bool operator==(const SStateExitBreakpoint &inOther) const
            {
                return m_ObjectId == inOther.m_ObjectId;
            }
        };

        template <typename TDataType>
        struct SBreakpointTypeMap
        {
            static BreakpointTypes::Enum GetType()
            {
                return BreakpointTypes::UnknownBreakpointType;
            }
        };

        template <>
        struct SBreakpointTypeMap<SStateEnterBreakpoint>
        {
            static BreakpointTypes::Enum GetType() { return BreakpointTypes::StateEnter; }
        };
        template <>
        struct SBreakpointTypeMap<SStateExitBreakpoint>
        {
            static BreakpointTypes::Enum GetType() { return BreakpointTypes::StateExit; }
        };
        template <>
        struct SBreakpointTypeMap<STransitionId>
        {
            static BreakpointTypes::Enum GetType() { return BreakpointTypes::Transition; }
        };

        struct SBreakpointUnionTraits
        {
            typedef BreakpointTypes::Enum TIdType;
            enum {
                TBufferSize = sizeof(STransitionId),
            };

            static TIdType getNoDataId() { return BreakpointTypes::UnknownBreakpointType; }

            template <typename TDataType>
            static TIdType getType()
            {
                return SBreakpointTypeMap<TDataType>().GetType();
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case BreakpointTypes::StateEnter:
                    return inVisitor(*NVUnionCast<SStateEnterBreakpoint *>(inData));
                case BreakpointTypes::StateExit:
                    return inVisitor(*NVUnionCast<SStateExitBreakpoint *>(inData));
                case BreakpointTypes::Transition:
                    return inVisitor(*NVUnionCast<STransitionId *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case BreakpointTypes::UnknownBreakpointType:
                    return inVisitor();
                }
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
                case BreakpointTypes::StateEnter:
                    return inVisitor(*NVUnionCast<const SStateEnterBreakpoint *>(inData));
                case BreakpointTypes::StateExit:
                    return inVisitor(*NVUnionCast<const SStateExitBreakpoint *>(inData));
                case BreakpointTypes::Transition:
                    return inVisitor(*NVUnionCast<const STransitionId *>(inData));
                default:
                    QT3DS_ASSERT(false);
                case BreakpointTypes::UnknownBreakpointType:
                    return inVisitor();
                }
            }
        };

        typedef qt3ds::foundation::
            DiscriminatedUnion<qt3ds::foundation::
                                   DiscriminatedUnionGenericBase<SBreakpointUnionTraits,
                                                                 SBreakpointUnionTraits::
                                                                     TBufferSize>,
                               SBreakpointUnionTraits::TBufferSize>
                TBreakpointUnionType;

        struct SBreakpoint : public TBreakpointUnionType
        {
            SBreakpoint() {}

            SBreakpoint(const SBreakpoint &inOther)
                : TBreakpointUnionType(static_cast<const TBreakpointUnionType &>(inOther))
            {
            }

            SBreakpoint(const char8_t *inOther)
                : TBreakpointUnionType(TEditorStr(inOther))
            {
            }

            template <typename TDataType>
            SBreakpoint(const TDataType &inDt)
                : TBreakpointUnionType(inDt)
            {
            }

            SBreakpoint &operator=(const SBreakpoint &inOther)
            {
                TBreakpointUnionType::operator=(inOther);
                return *this;
            }

            bool operator==(const SBreakpoint &inOther) const
            {
                return TBreakpointUnionType::operator==(inOther);
            }
            bool operator!=(const SBreakpoint &inOther) const
            {
                return TBreakpointUnionType::operator!=(inOther);
            }

            bool empty() const { return getType() == SBreakpointUnionTraits::getNoDataId(); }
        };

        struct SNil
        {
            bool operator==(const SNil &) const { return true; }
        };

#define ITERATE_DATAMODEL_VALUE_TYPES                                                              \
    HANDLE_DATAMODEL_VALUE_TYPE(Number, float)                                                     \
    HANDLE_DATAMODEL_VALUE_TYPE(String, TDebugStr)                                                 \
    HANDLE_DATAMODEL_VALUE_TYPE(Nil, SNil)                                                         \
    HANDLE_DATAMODEL_VALUE_TYPE(Boolean, bool)                                                     \
    HANDLE_DATAMODEL_VALUE_TYPE(Table, SDatamodelTable *)                                          \
    HANDLE_DATAMODEL_VALUE_TYPE(UserData, SDatamodelUserData *)                                    \
    HANDLE_DATAMODEL_VALUE_TYPE(Function, SDatamodelFunction *)                                    \
    HANDLE_DATAMODEL_VALUE_TYPE(CFunction, SDatamodelCFunction *)                                  \
    HANDLE_DATAMODEL_VALUE_TYPE(Thread, SDatamodelThread *)

        template <typename TDataType>
        struct SDatamodelValueTypeMap
        {
        };

#define HANDLE_DATAMODEL_VALUE_TYPE(name, type)                                                    \
    template <>                                                                                    \
    struct SDatamodelValueTypeMap<type>                                                            \
    {                                                                                              \
        static DatamodelValueTypes::Enum GetType() { return DatamodelValueTypes::name; }           \
    };
        ITERATE_DATAMODEL_VALUE_TYPES
#undef HANDLE_DATAMODEL_VALUE_TYPE

        struct SDatamodelValueUnionTraits
        {
            typedef DatamodelValueTypes::Enum TIdType;
            enum {
                TBufferSize = sizeof(TDebugStr),
            };

            static TIdType getNoDataId() { return DatamodelValueTypes::UnknownType; }

            template <typename TDataType>
            static TIdType getType()
            {
                return SDatamodelValueTypeMap<TDataType>().GetType();
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
#define HANDLE_DATAMODEL_VALUE_TYPE(name, type)                                                    \
    case DatamodelValueTypes::name:                                                                \
        return inVisitor(*NVUnionCast<type *>(inData));
                    ITERATE_DATAMODEL_VALUE_TYPES
#undef HANDLE_DATAMODEL_VALUE_TYPE
                default:
                    QT3DS_ASSERT(false);
                case DatamodelValueTypes::UnknownType:
                    return inVisitor();
                }
            }

            template <typename TRetType, typename TVisitorType>
            static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
            {
                switch (inType) {
#define HANDLE_DATAMODEL_VALUE_TYPE(name, type)                                                    \
    case DatamodelValueTypes::name:                                                                \
        return inVisitor(*NVUnionCast<const type *>(inData));
                    ITERATE_DATAMODEL_VALUE_TYPES
#undef HANDLE_DATAMODEL_VALUE_TYPE
                default:
                    QT3DS_ASSERT(false);
                case DatamodelValueTypes::UnknownType:
                    return inVisitor();
                }
            }
        };

        typedef qt3ds::foundation::
            DiscriminatedUnion<qt3ds::foundation::
                                   DiscriminatedUnionGenericBase<SDatamodelValueUnionTraits,
                                                                 SDatamodelValueUnionTraits::
                                                                     TBufferSize>,
                               SDatamodelValueUnionTraits::TBufferSize>
                TDatamodelValueUnionType;

        struct SDatamodelValue : public TDatamodelValueUnionType
        {
            SDatamodelValue() {}

            SDatamodelValue(const SDatamodelValue &inOther)
                : TDatamodelValueUnionType(static_cast<const TDatamodelValueUnionType &>(inOther))
            {
            }

            SDatamodelValue(const char8_t *inOther)
                : TDatamodelValueUnionType(TDebugStr(inOther))
            {
            }

            template <typename TDataType>
            SDatamodelValue(const TDataType &inDt)
                : TDatamodelValueUnionType(inDt)
            {
            }

            SDatamodelValue &operator=(const SDatamodelValue &inOther)
            {
                TDatamodelValueUnionType::operator=(inOther);
                return *this;
            }

            bool operator==(const SDatamodelValue &inOther) const
            {
                return TDatamodelValueUnionType::operator==(inOther);
            }
            bool operator!=(const SDatamodelValue &inOther) const
            {
                return TDatamodelValueUnionType::operator!=(inOther);
            }

            bool empty() const { return getType() == SDatamodelValueUnionTraits::getNoDataId(); }
        };

        struct STableEntry
        {
            TDebugStr m_Key;
            SDatamodelValue m_Value;
            STableEntry(const TDebugStr &inKey = TDebugStr(),
                        const SDatamodelValue &inValue = SDatamodelValue())
                : m_Key(inKey)
                , m_Value(inValue)
            {
            }

            bool operator==(const STableEntry &inOther) const
            {
                return m_Key == inOther.m_Key && m_Value == inOther.m_Value;
            }
        };

        struct TableModificationType
        {
            enum Enum {
                UnknownModification = 0,
                SetKey = 1,
                RemoveKey = 2,
            };
        };

        struct STableModification
        {
            STableEntry m_Entry;
            TableModificationType::Enum m_Type;
            STableModification(
                const STableEntry &inEntry = STableEntry(),
                TableModificationType::Enum inEnum = TableModificationType::UnknownModification)
                : m_Entry(inEntry)
                , m_Type(inEnum)
            {
            }
            bool operator==(const STableModification &inMod) const
            {
                return inMod.m_Entry == m_Entry && inMod.m_Type == m_Type;
            }
        };

        typedef eastl::vector<STableModification> TTableModificationList;
    }
}
}

#ifndef _INTEGRITYPLATFORM
namespace qt3ds {
namespace foundation {
    template <>
    struct DestructTraits<qt3ds::state::debugger::SNil>
    {
        void destruct(qt3ds::state::debugger::SNil &) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::debugger::SDatamodelTable *>
    {
        void destruct(qt3ds::state::debugger::SDatamodelTable *) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::debugger::SDatamodelUserData *>
    {
        void destruct(qt3ds::state::debugger::SDatamodelUserData *) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::debugger::SDatamodelFunction *>
    {
        void destruct(qt3ds::state::debugger::SDatamodelFunction *) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::debugger::SDatamodelCFunction *>
    {
        void destruct(qt3ds::state::debugger::SDatamodelCFunction *) {}
    };
    template <>
    struct DestructTraits<qt3ds::state::debugger::SDatamodelThread *>
    {
        void destruct(qt3ds::state::debugger::SDatamodelThread *) {}
    };
}
}
#endif

#endif
