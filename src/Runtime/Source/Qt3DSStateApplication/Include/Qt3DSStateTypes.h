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
#ifndef QT3DS_STATE_TYPES_H
#define QT3DS_STATE_TYPES_H
#include "Qt3DSState.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/StringTable.h"
#include "foundation/TaggedPointer.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Utils.h"
#include "Qt3DSStateIdValue.h"

namespace qt3ds {
namespace state {

    // Link list definitions.
    DEFINE_INVASIVE_LIST(StateNode);
    DEFINE_INVASIVE_SINGLE_LIST(OnEntry);
    DEFINE_INVASIVE_SINGLE_LIST(OnExit);
    DEFINE_INVASIVE_SINGLE_LIST(DataModel);
    DEFINE_INVASIVE_LIST(Invoke);

    // Externally defined objects - These objects are defined by the implementation of the
    // interpreter and the execution context.  They are not used and cannot be manipulated by
    // core state types.

    // Standard content defined in another file so as to not clutter up the core state definitions.
    struct SExecutableContent;
    DEFINE_INVASIVE_LIST(ExecutableContent);
    struct SScript;
    struct SDataModel;

    // Defined by the execution context; These objects can be defined by the implementation of the
    // scripting
    // system.
    struct STransitionCondition;

    // Defined by the interpreter.  Really just a placeholder so the interpreter can cache
    // item-specific data on an object.
    struct SInterpreterData;

    // We *have* to keep some subset of the data in document order because the algorithms
    // defined in the specification rely on this.
    struct StateNodeTypes
    {
        enum Enum {
            UnknownType,
            State,
            Parallel,
            Transition,
            Final,
            SCXML,
            History,
        };
        static bool CanHaveChildren(StateNodeTypes::Enum val)
        {
            return val == State || val == Parallel || val == SCXML;
        }
        static bool CanHaveTransitions(StateNodeTypes::Enum val)
        {
            return val == State || val == Parallel;
        }
        static bool IsStateType(StateNodeTypes::Enum val)
        {
            return CanHaveChildren(val) || val == Final;
        }
        static bool IsTransitionType(StateNodeTypes::Enum val) { return val == Transition; }
        static bool CanHaveInitializeNode(StateNodeTypes::Enum val) { return val == State; }
        static bool IsAtomicType(StateNodeTypes::Enum val) { return val == State || val == Final; }
    };

    struct SState;
    struct SParallel;
    struct STransition;
    struct SFinal;
    struct SSCXML;
    struct SHistory;

    template <typename TDataType>
    struct SStateNodeTypeMap
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::UnknownType; }
    };

    template <>
    struct SStateNodeTypeMap<SState>
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::State; }
    };
    template <>
    struct SStateNodeTypeMap<SParallel>
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::Parallel; }
    };
    template <>
    struct SStateNodeTypeMap<STransition>
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::Transition; }
    };
    template <>
    struct SStateNodeTypeMap<SFinal>
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::Final; }
    };
    template <>
    struct SStateNodeTypeMap<SSCXML>
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::SCXML; }
    };
    template <>
    struct SStateNodeTypeMap<SHistory>
    {
        static StateNodeTypes::Enum GetStateNodeType() { return StateNodeTypes::History; }
    };
    // Some editor info has to be contained on the state node.  Description I am fine with eliding
    // but
    // most of the other information needs to sit on the nodes themselves.
    struct StateNodeFlagValues
    {
        enum Enum {
            HasNothing = 0,
            HasPosition = 1,
            HasDimension = 1 << 1,
            HasColor = 1 << 2,
            HasEndPosition = 1 << 3,
        };
    };

    struct SStateNodeFlags : NVFlags<StateNodeFlagValues::Enum, QT3DSU32>
    {
        bool HasPosition() const { return this->operator&(StateNodeFlagValues::HasPosition); }
        void SetHasPosition(bool val) { clearOrSet(val, StateNodeFlagValues::HasPosition); }
        bool HasDimension() const { return this->operator&(StateNodeFlagValues::HasDimension); }
        void SetHasDimension(bool val) { clearOrSet(val, StateNodeFlagValues::HasDimension); }
        bool HasColor() const { return this->operator&(StateNodeFlagValues::HasColor); }
        void SetHasColor(bool val) { clearOrSet(val, StateNodeFlagValues::HasColor); }
        bool HasEndPosition() const { return this->operator&(StateNodeFlagValues::HasEndPosition); }
        void SetHasEndPosition(bool val) { clearOrSet(val, StateNodeFlagValues::HasEndPosition); }
    };

    struct SStateNode
    {
        const StateNodeTypes::Enum m_Type;
        CRegisteredString m_Id;
        SStateNode *m_Parent;
        SStateNode *m_NextSibling;
        SStateNode *m_PreviousSibling;
        SInterpreterData *m_InterpreterData;
        SStateNodeFlags m_StateNodeFlags;
        QT3DSVec2 m_Position;
        QT3DSVec2 m_Dimension;
        QT3DSVec3 m_Color;

        SStateNode(StateNodeTypes::Enum inType)
            : m_Type(inType)
            , m_Parent(NULL)
            , m_NextSibling(NULL)
            , m_PreviousSibling(NULL)
            , m_InterpreterData(NULL)
        {
        }

        template <typename TDataType>
        TDataType *CastTo()
        {
            if (m_Type == SStateNodeTypeMap<TDataType>::GetStateNodeType())
                return static_cast<TDataType *>(this);
            QT3DS_ASSERT(false);
            return NULL;
        }

        template <typename TDataType>
        const TDataType *CastTo() const
        {
            if (m_Type == SStateNodeTypeMap<TDataType>::GetStateNodeType())
                return static_cast<const TDataType *>(this);
            QT3DS_ASSERT(false);
            return NULL;
        }
        // Helper functions that take care of the drama around getting the various
        // shared properties
        bool IsCompound() const;
        bool IsAtomic() const;
        void AppendChild(SStateNode &inChild);
        TOnEntryList *GetOnEntryList();
        TOnExitList *GetOnExitList();
        STransition *GetInitialTransition();
        const char8_t *GetInitialExpression();
        TStateNodeList *GetChildren();
        SDataModel *GetDataModel();
        Option<QT3DSVec2> GetPosition() const
        {
            if (m_StateNodeFlags.HasPosition())
                return m_Position;
            return Empty();
        }
        void SetPosition(const Option<QT3DSVec2> &pos)
        {
            if (pos.hasValue()) {
                m_Position = *pos;
            }
            m_StateNodeFlags.SetHasPosition(pos.hasValue());
        }
        Option<QT3DSVec2> GetDimension() const
        {
            if (m_StateNodeFlags.HasDimension())
                return m_Dimension;
            return Empty();
        }
        void SetDimension(const Option<QT3DSVec2> &pos)
        {
            if (pos.hasValue()) {
                m_Dimension = *pos;
            }
            m_StateNodeFlags.SetHasDimension(pos.hasValue());
        }
        Option<QT3DSVec3> GetColor() const
        {
            if (m_StateNodeFlags.HasColor())
                return m_Color;
            return Empty();
        }
        void SetColor(const Option<QT3DSVec3> &pos)
        {
            if (pos.hasValue()) {
                m_Color = *pos;
            }
            m_StateNodeFlags.SetHasColor(pos.hasValue());
        }
    };

    struct SEntryExitBase : public SStateNode
    {
        TOnEntryList m_OnEntry;
        TOnExitList m_OnExit;
        SEntryExitBase(StateNodeTypes::Enum inType)
            : SStateNode(inType)
        {
        }
    };

    // State and parallel objects can have states as children.  Nothing aside from
    // SCXML can have this.
    struct SStateParallelBase : public SEntryExitBase
    {
        TStateNodeList m_Children;
        SDataModel *m_DataModel;
        TInvokeList m_InvokeList;

        SStateParallelBase(StateNodeTypes::Enum inType)
            : SEntryExitBase(inType)
            , m_DataModel(NULL)
        {
        }

        void RemoveChild(SStateNode &inChild)
        {
            QT3DS_ASSERT(inChild.m_Parent == this);
            m_Children.remove(inChild);
            inChild.m_Parent = NULL;
        }

        void AppendChild(SStateNode &inChild, SStateNode *inLoc = NULL)
        {
            if (inChild.m_Parent != NULL)
                static_cast<SStateParallelBase *>(inChild.m_Parent)->RemoveChild(inChild);
            if (inLoc)
                m_Children.insert_after(*inLoc, inChild);
            else
                m_Children.push_back(inChild);
            inChild.m_Parent = this;
        }

        void PrependChild(SStateNode &inChild, SStateNode *inLoc = NULL)
        {
            if (inChild.m_Parent != NULL)
                static_cast<SStateParallelBase *>(inChild.m_Parent)->RemoveChild(inChild);
            if (inLoc)
                m_Children.insert_before(*inLoc, inChild);
            else
                m_Children.push_front(inChild);
            inChild.m_Parent = this;
        }

        bool IsAtomic() const
        {
            for (TStateNodeList::const_iterator iter = m_Children.begin(), end = m_Children.end();
                 iter != end; ++iter) {
                if (iter->m_Type != StateNodeTypes::Transition)
                    return false;
            }
            return true;
        }
        bool IsCompound() const { return !IsAtomic(); }
    };

    struct SCXMLFlagValues
    {
        enum Enum {
            Late = 1,
        };
    };

    struct SSCXMLFlags : public NVFlags<SCXMLFlagValues::Enum, QT3DSU32>
    {
        SSCXMLFlags() {}

        void SetLateBinding(bool inValue) { clearOrSet(inValue, SCXMLFlagValues::Late); }
        bool IsLateBinding() const { return this->operator&(SCXMLFlagValues::Late); }
    };

    // Begin standard object definitions

    struct SSCXML : public SStateNode
    {
        STransition *m_Initial;
        CRegisteredString m_Name;
        TStateNodeList m_Children;
        SScript *m_Script;
        SSCXMLFlags m_Flags;
        SDataModel *m_DataModel;
        QT3DSI32 m_UICVersion;
        const char8_t *m_Filename;
        const char8_t *m_InitialExpr;

        static QT3DSI32 GetCurrentUICVersion() { return 1; }
        SSCXML()
            : SStateNode(StateNodeTypes::SCXML)
            , m_Initial(NULL)
            , m_Script(NULL)
            , m_DataModel(NULL)
            , m_UICVersion(GetCurrentUICVersion())
            , m_InitialExpr(NULL)
        {
        }

        void RemoveChild(SStateNode &inChild)
        {
            QT3DS_ASSERT(inChild.m_Parent == this);
            m_Children.remove(inChild);
            inChild.m_Parent = NULL;
        }

        void AppendChild(SStateNode &inChild, SStateNode *inLoc = NULL)
        {
            if (inChild.m_Parent != NULL)
                static_cast<SStateParallelBase *>(inChild.m_Parent)->RemoveChild(inChild);
            if (inLoc)
                m_Children.insert_after(*inLoc, inChild);
            else
                m_Children.push_back(inChild);
            inChild.m_Parent = this;
        }
    };

    struct SState : public SStateParallelBase
    {
        // transition, state, parallel, and final are all handed by SStateNode
        STransition *m_Initial;
        const char8_t *m_InitialExpr;

        SState()
            : SStateParallelBase(StateNodeTypes::State)
            , m_Initial(NULL)
            , m_InitialExpr(NULL)
        {
        }
    };

    struct SParallel : public SStateParallelBase
    {
        SParallel()
            : SStateParallelBase(StateNodeTypes::Parallel)
        {
        }
    };

    struct TransitionFlagValues
    {
        enum Enum {
            Internal = 1,
        };
    };

    struct STransitionFlags : public NVFlags<TransitionFlagValues::Enum, QT3DSU32>
    {
        STransitionFlags() {}

        void SetInternal(bool inValue) { clearOrSet(inValue, TransitionFlagValues::Internal); }
        bool IsInternal() const { return this->operator&(TransitionFlagValues::Internal); }
    };

    struct STransition : public SStateNode
    {
        CRegisteredString m_Event;
        const char8_t *m_Condition;
        NVConstDataRef<SStateNode *> m_Target;
        STransitionFlags m_Flags;
        TExecutableContentList m_ExecutableContent;
        NVConstDataRef<QT3DSVec2> m_Path;
        // If we have multiple end targets, on order to lay them out we need a branch point
        // along with the number of control points in each target.  The first index points to the
        // branch point in the path vector, then next index tells the number of control points
        // for the first end point, etc.
        NVConstDataRef<QT3DSU32> m_PathIndexes;
        QT3DSVec2 m_EndPosition;
        // m_Source of the transition is its parent

        STransition()
            : SStateNode(StateNodeTypes::Transition)
            , m_Condition(NULL)
            , m_EndPosition(0, 0)
        {
        }

        SStateNode *GetSource() { return m_Parent; }

        Option<QT3DSVec2> GetEndPosition() const
        {
            if (m_StateNodeFlags.HasEndPosition())
                return m_EndPosition;
            return Empty();
        }
        void SetEndPosition(const Option<QT3DSVec2> &pos)
        {
            if (pos.hasValue()) {
                m_EndPosition = *pos;
            }
            m_StateNodeFlags.SetHasEndPosition(pos.hasValue());
        }
    };

    // TODO: DoneData
    struct SFinal : public SEntryExitBase
    {
        SFinal()
            : SEntryExitBase(StateNodeTypes::Final)
        {
        }
    };

    struct HistoryFlagValues
    {
        enum Enum {
            Deep = 1,
        };
    };

    struct SHistoryFlags : public NVFlags<HistoryFlagValues::Enum, QT3DSU32>
    {
        SHistoryFlags() {}

        void SetDeep(bool inValue) { clearOrSet(inValue, HistoryFlagValues::Deep); }
        bool IsDeep() const { return this->operator&(HistoryFlagValues::Deep); }
    };

    struct SHistory : public SStateNode
    {
        SHistoryFlags m_Flags;
        qt3ds::foundation::STaggedPointer m_UserData;
        STransition *m_Transition;

        SHistory()
            : SStateNode(StateNodeTypes::History)
            , m_Transition(NULL)
        {
        }
    };

    struct SOnEntry
    {
        SOnEntry *m_NextSibling;
        TExecutableContentList m_ExecutableContent;
        qt3ds::foundation::STaggedPointer m_UserData;
        SOnEntry()
            : m_NextSibling(NULL)
        {
        }
    };

    struct SOnExit
    {
        SOnExit *m_NextSibling;
        TExecutableContentList m_ExecutableContent;
        qt3ds::foundation::STaggedPointer m_UserData;
        SOnExit()
            : m_NextSibling(NULL)
        {
        }
    };

    struct SInvoke
    {
        SInvoke *m_NextSibling;
        SInvoke *m_PreviousSibling;
        // Will have either SCXML content or dom content but not both.
        SSCXML *m_SCXMLContent;
        SDOMElement *m_DOMContent;
        SInvoke()
            : m_NextSibling(NULL)
            , m_PreviousSibling(NULL)
            , m_SCXMLContent(NULL)
            , m_DOMContent(NULL)
        {
        }
    };

    // Events, because they created both inside and outside
    // the state system by various parties.
    class IEvent : public NVRefCounted
    {
    protected:
        virtual ~IEvent() {}
    public:
        virtual CRegisteredString GetName() const = 0;
        // Optional type string for rtti.  No string means this is an opaque event
        // that cannot be safely cast to any specific event type.
        virtual CRegisteredString GetEventType() const { return CRegisteredString(); }
    };

    typedef NVScopedRefCounted<IEvent> TEventPtr;

    struct SIdValue;

    typedef nvhash_map<CRegisteredString, SIdValue> TIDStateMap;

    struct SDOMElementNode
    {
        SDOMElementNode *m_NextNode;
        SDOMElement *m_Element;
        SDOMElementNode(SDOMElement *elem = NULL)
            : m_NextNode(NULL)
            , m_Element(elem)
        {
        }
    };

    struct SDOMAttributeNode
    {
        SDOMAttributeNode *m_NextNode;
        SDOMAttribute *m_Attribute;
        SDOMAttributeNode(SDOMAttribute *inAtt = NULL)
            : m_NextNode(NULL)
            , m_Attribute(inAtt)
        {
        }
    };
    DEFINE_INVASIVE_SINGLE_LIST(DOMElementNode);
    DEFINE_INVASIVE_SINGLE_LIST(DOMAttributeNode);

    inline bool SStateNode::IsCompound() const
    {
        if (m_Type == StateNodeTypes::SCXML)
            return true;
        if (m_Type == StateNodeTypes::State)
            return !IsAtomic();
        return false;
    }

    inline bool SStateNode::IsAtomic() const
    {
        if (m_Type == StateNodeTypes::SCXML)
            return false;

        if (m_Type == StateNodeTypes::State) {
            const SState *theState = CastTo<SState>();
            for (TStateNodeList::iterator iter = theState->m_Children.begin(),
                                          end = theState->m_Children.end();
                 iter != end; ++iter) {
                if (iter->m_Type != StateNodeTypes::Transition
                    && iter->m_Type != StateNodeTypes::History)
                    return false;
            }
            return true;
        } else if (m_Type == StateNodeTypes::Final)
            return true;
        return false;
    }

    inline void SStateNode::AppendChild(SStateNode &inChild)
    {
        if (m_Type == StateNodeTypes::State || m_Type == StateNodeTypes::Parallel) {
            static_cast<SStateParallelBase *>(this)->AppendChild(inChild);
        } else if (m_Type == StateNodeTypes::SCXML) {
            static_cast<SSCXML *>(this)->AppendChild(inChild);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    inline TOnEntryList *SStateNode::GetOnEntryList()
    {
        if (m_Type == StateNodeTypes::State || m_Type == StateNodeTypes::Parallel)
            return &static_cast<SStateParallelBase *>(this)->m_OnEntry;
        if (m_Type == StateNodeTypes::Final)
            return &CastTo<SFinal>()->m_OnEntry;
        return NULL;
    }

    inline TOnExitList *SStateNode::GetOnExitList()
    {
        if (m_Type == StateNodeTypes::State || m_Type == StateNodeTypes::Parallel)
            return &static_cast<SStateParallelBase *>(this)->m_OnExit;
        if (m_Type == StateNodeTypes::Final)
            return &CastTo<SFinal>()->m_OnExit;
        return NULL;
    }

    inline STransition *SStateNode::GetInitialTransition()
    {
        if (m_Type == StateNodeTypes::State)
            return static_cast<SState *>(this)->m_Initial;
        if (m_Type == StateNodeTypes::SCXML)
            return static_cast<SSCXML *>(this)->m_Initial;
        return NULL;
    }

    inline const char8_t *SStateNode::GetInitialExpression()
    {
        if (m_Type == StateNodeTypes::State)
            return nonNull(static_cast<SState *>(this)->m_InitialExpr);
        if (m_Type == StateNodeTypes::SCXML)
            return nonNull(static_cast<SSCXML *>(this)->m_InitialExpr);
        return "";
    }

    inline TStateNodeList *SStateNode::GetChildren()
    {
        if (m_Type == StateNodeTypes::State || m_Type == StateNodeTypes::Parallel)
            return &static_cast<SStateParallelBase *>(this)->m_Children;
        if (m_Type == StateNodeTypes::SCXML)
            return &static_cast<SSCXML *>(this)->m_Children;
        return NULL;
    }

    inline SDataModel *SStateNode::GetDataModel()
    {
        if (m_Type == StateNodeTypes::State || m_Type == StateNodeTypes::Parallel)
            return static_cast<SStateParallelBase *>(this)->m_DataModel;
        if (m_Type == StateNodeTypes::SCXML)
            return static_cast<SSCXML *>(this)->m_DataModel;
        return NULL;
    }

    IMPLEMENT_INVASIVE_LIST(Invoke, m_PreviousSibling, m_NextSibling);
    IMPLEMENT_INVASIVE_LIST(StateNode, m_PreviousSibling, m_NextSibling);
    IMPLEMENT_INVASIVE_SINGLE_LIST(OnEntry, m_NextSibling);
    IMPLEMENT_INVASIVE_SINGLE_LIST(OnExit, m_NextSibling);
    IMPLEMENT_INVASIVE_SINGLE_LIST(DOMElementNode, m_NextNode);
    IMPLEMENT_INVASIVE_SINGLE_LIST(DOMAttributeNode, m_NextNode);

    DEFINE_INVASIVE_SINGLE_LIST(Data);

    struct SDataModel
    {
        CRegisteredString m_Id;
        const char8_t *m_Source;
        const char8_t *m_Expression;
        TDataList m_Data;

        SDataModel()
            : m_Source(NULL)
            , m_Expression(NULL)
        {
        }
    };

    struct SData
    {
        CRegisteredString m_Id;
        const char8_t *m_Source;
        const char8_t *m_Expression;
        SData *m_NextSibling;
        SData()
            : m_Source(NULL)
            , m_Expression(NULL)
            , m_NextSibling(NULL)
        {
        }
    };

    IMPLEMENT_INVASIVE_SINGLE_LIST(Data, m_NextSibling);
}
}

#endif
