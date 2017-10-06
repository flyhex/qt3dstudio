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
#ifndef UIC_STATE_DEBUGGER_PROTOCOL_H
#define UIC_STATE_DEBUGGER_PROTOCOL_H
#include "UICStateDebugger.h"
#include "UICStateDebuggerValues.h"
#include "foundation/StringConversion.h"
#include "foundation/StringConversionImpl.h"
#include "foundation/Qt3DSTime.h"

// Stream protocol, regardless of binary vs. text.
namespace uic {
namespace state {
    namespace debugger {

        struct SInitialization
        {
            QT3DSI32 m_Version;
            QT3DSI32 m_Padding;
            CounterFrequencyToTensOfNanos m_TimerFrequency;
            static QT3DSI32 GetCurrentVersion() { return 1; }
            SInitialization()
                : m_Version(GetCurrentVersion())
                , m_Padding(0)
                , m_TimerFrequency(Time::sCounterFreq)
            {
            }
            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visit(m_Version);
                inVisitor.visit(m_TimerFrequency.mNumerator);
                inVisitor.visit(m_TimerFrequency.mDenominator);
            }
            bool operator==(const SInitialization &inOther) const
            {
                return m_Version == inOther.m_Version
                    && m_TimerFrequency.mNumerator == inOther.m_TimerFrequency.mNumerator
                    && m_TimerFrequency.mDenominator == inOther.m_TimerFrequency.mDenominator;
            }
        };

        // Listener interface.  For on connect, we send the current timestamp for that state
        // machine.
        struct SConnect
        {
            TDebugStr m_Filename;
            TDebugStr m_SCXMLData; // scxml file contents
            TDebugStrList m_Configuration;
            SConnect() {}
            SConnect(const TDebugStr &fn, const TDebugStr &xmlData, const TDebugStrList &config)
                : m_Filename(fn)
                , m_SCXMLData(xmlData)
                , m_Configuration(config)
            {
            }

            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.randomText(m_Filename);
                inVisitor.randomText(m_SCXMLData);
                inVisitor.visitIdList(m_Configuration);
            }
            bool operator==(const SConnect &inOther) const
            {
                return m_Filename == inOther.m_Filename && m_SCXMLData == inOther.m_SCXMLData
                    && m_Configuration == inOther.m_Configuration;
            }
        };

        struct SBreakpointHit
        {
            SBreakpoint m_Breakpoint;
            SBreakpointHit() {}
            SBreakpointHit(const SBreakpoint &bp)
                : m_Breakpoint(bp)
            {
            }

            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visit(m_Breakpoint);
            }
            bool operator==(const SBreakpointHit &inOther) const
            {
                return m_Breakpoint == inOther.m_Breakpoint;
            }
        };

        struct SDebugLog
        {
            TDebugStr m_Message;
            SDebugLog() {}
            SDebugLog(const TDebugStr &msg)
                : m_Message(msg)
            {
            }
            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.randomText(m_Message);
            }
            bool operator==(const SDebugLog &inOther) const
            {
                return m_Message == inOther.m_Message;
            }
        };

        struct SModifyTableValues
        {
            SDatamodelTable *m_TablePtr;
            TTableModificationList m_Modifications;

            SModifyTableValues(SDatamodelTable *inTable = NULL,
                               const TTableModificationList &inList = TTableModificationList())
                : m_TablePtr(inTable)
                , m_Modifications(inList)
            {
            }

            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visit(m_TablePtr);
                inVisitor.visit(m_Modifications);
            }

            bool operator==(const SModifyTableValues &inOther) const
            {
                return m_TablePtr == inOther.m_TablePtr
                    && m_Modifications == inOther.m_Modifications;
            }
        };

        struct SBeginStep
        {
            SBeginStep() {}
            template <typename TVisitor>
            void Visit(TVisitor &)
            {
            }

            bool operator==(const SBeginStep &) const { return true; }
        };

        struct SBeginMicrostep
        {
            SBeginMicrostep() {}
            template <typename TVisitor>
            void Visit(TVisitor &)
            {
            }

            bool operator==(const SBeginMicrostep &) const { return true; }
        };

        struct SMicrostepEvent
        {
            TDebugStr m_Event;
            bool m_Internal;

            SMicrostepEvent(const TDebugStr &inEvent = TDebugStr(), bool inIsInternal = false)
                : m_Event(inEvent)
                , m_Internal(inIsInternal)
            {
            }

            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visitId(m_Event);
                inVisitor.visit(m_Internal);
            }

            bool operator==(const SMicrostepEvent &inOther) const
            {
                return m_Event == inOther.m_Event && m_Internal == inOther.m_Internal;
            }
        };

        struct SMicrostepData
        {
            TTransitionIdList m_Transitions;
            TDebugStrList m_ExitStates;
            TDebugStrList m_EnterStates;

            SMicrostepData() {}
            SMicrostepData(const TTransitionIdList &inTransitions,
                           const TDebugStrList &inExitStates, const TDebugStrList &inEnterStates)
                : m_Transitions(inTransitions)
                , m_ExitStates(inExitStates)
                , m_EnterStates(inEnterStates)
            {
            }

            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                // Eventless transitions cause us to need to always write the string length.
                inVisitor.visit(m_Transitions);
                inVisitor.visitIdList(m_ExitStates);
                inVisitor.visitIdList(m_EnterStates);
            }
            bool operator==(const SMicrostepData &inOther) const
            {
                return m_Transitions == inOther.m_Transitions
                    && m_ExitStates == inOther.m_ExitStates
                    && m_EnterStates == inOther.m_EnterStates;
            }
        };

        struct SEndMicrostep
        {
            SEndMicrostep() {}
            template <typename TVisitor>
            void Visit(TVisitor & /*inVisitor*/)
            {
            }

            bool operator==(const SEndMicrostep &) const { return true; }
        };

        struct SEventQueued
        {
            TDebugStr m_Event;
            bool m_Internal;
            SEventQueued()
                : m_Internal(false)
            {
            }
            SEventQueued(const TDebugStr &evt, bool intern)
                : m_Event(evt)
                , m_Internal(intern)
            {
            }
            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visitEventName(m_Event);
                inVisitor.visit(m_Internal);
            }
            bool operator==(const SEventQueued &inOther) const
            {
                return m_Event == inOther.m_Event && m_Internal == inOther.m_Internal;
            }
        };

        // Debug interface

        struct SSetBreakpoint
        {
            SBreakpoint m_Breakpoint;
            SSetBreakpoint() {}
            SSetBreakpoint(const SBreakpoint &bp)
                : m_Breakpoint(bp)
            {
            }
            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visit(m_Breakpoint);
            }
            bool operator==(const SSetBreakpoint &inOther) const
            {
                return m_Breakpoint == inOther.m_Breakpoint;
            }
        };

        struct SClearBreakpoint
        {
            SBreakpoint m_Breakpoint;
            SClearBreakpoint() {}
            SClearBreakpoint(const SBreakpoint &bp)
                : m_Breakpoint(bp)
            {
            }
            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visit(m_Breakpoint);
            }
            bool operator==(const SClearBreakpoint &inOther) const
            {
                return m_Breakpoint == inOther.m_Breakpoint;
            }
        };

        struct SClearAllBreakpoints
        {
            SClearAllBreakpoints() {}
            template <typename TVisitor>
            void Visit(TVisitor & /*inVisitor*/)
            {
            }
            bool operator==(const SClearAllBreakpoints & /*inOther*/) const { return true; }
        };

        struct SBreakOnMicrostep
        {
            bool m_Value;
            SBreakOnMicrostep(bool val = false)
                : m_Value(val)
            {
            }
            template <typename TVisitor>
            void Visit(TVisitor &inVisitor)
            {
                inVisitor.visit(m_Value);
            }
            bool operator==(const SBreakOnMicrostep &inOther) const
            {
                return m_Value == inOther.m_Value;
            }
        };

        struct SContinue
        {
            SContinue() {}
            template <typename TVisitor>
            void Visit(TVisitor & /*inVisitor*/)
            {
            }
            bool operator==(const SContinue & /*inOther*/) const { return true; }
        };

        struct SDisconnect
        {
            SDisconnect() {}
            template <typename TVisitor>
            void Visit(TVisitor & /*inVisitor*/)
            {
            }
            bool operator==(const SDisconnect & /*inOther*/) const { return true; }
        };

        // Visitors to handle the breakpoint subtypes.
        // The empty top visitor will force a compile error if there is a breakpoint type we don't
        // recognize.
        template <typename TDataType>
        struct SBreakpointSerializationVisitor
        {
        };

        template <>
        struct SBreakpointSerializationVisitor<SStateEnterBreakpoint>
        {
            enum { Type = BreakpointTypes::StateEnter };
            template <typename TVisitor>
            static void Visit(TVisitor &inVisitor, SStateEnterBreakpoint &inBp)
            {
                inVisitor.visitId(inBp.m_ObjectId);
            }
        };

        template <>
        struct SBreakpointSerializationVisitor<SStateExitBreakpoint>
        {
            enum { Type = BreakpointTypes::StateExit };
            template <typename TVisitor>
            static void Visit(TVisitor &inVisitor, SStateExitBreakpoint &inBp)
            {
                inVisitor.visitId(inBp.m_ObjectId);
            }
        };

        template <>
        struct SBreakpointSerializationVisitor<STransitionId>
        {
            enum { Type = BreakpointTypes::Transition };
            template <typename TVisitor>
            static void Visit(TVisitor &inVisitor, STransitionId &inBp)
            {
                inVisitor.visitId(inBp.m_StateId);
                inVisitor.visit(inBp.m_TransitionIndex);
            }
        };

#define ITERATE_BREAKPOINT_TYPES                                                                   \
    HANDLE_BREAKPOINT_TYPE(StateEnter, SStateEnterBreakpoint)                                      \
    HANDLE_BREAKPOINT_TYPE(StateExit, SStateExitBreakpoint)                                        \
    HANDLE_BREAKPOINT_TYPE(Transition, STransitionId)

#define ITERATE_DEBUG_MESSAGE_TYPES                                                                \
    HANDLE_DEBUG_MESSAGE_TYPE(Initialization)                                                      \
    HANDLE_DEBUG_MESSAGE_TYPE(Connect)                                                             \
    HANDLE_DEBUG_MESSAGE_TYPE(BreakpointHit)                                                       \
    HANDLE_DEBUG_MESSAGE_TYPE(DebugLog)                                                            \
    HANDLE_DEBUG_MESSAGE_TYPE(ModifyTableValues)                                                   \
    HANDLE_DEBUG_MESSAGE_TYPE(BeginStep)                                                           \
    HANDLE_DEBUG_MESSAGE_TYPE(BeginMicrostep)                                                      \
    HANDLE_DEBUG_MESSAGE_TYPE(MicrostepEvent)                                                      \
    HANDLE_DEBUG_MESSAGE_TYPE(MicrostepData)                                                       \
    HANDLE_DEBUG_MESSAGE_TYPE(EndMicrostep)                                                        \
    HANDLE_DEBUG_MESSAGE_TYPE(EventQueued)                                                         \
    HANDLE_DEBUG_MESSAGE_TYPE(SetBreakpoint)                                                       \
    HANDLE_DEBUG_MESSAGE_TYPE(ClearBreakpoint)                                                     \
    HANDLE_DEBUG_MESSAGE_TYPE(ClearAllBreakpoints)                                                 \
    HANDLE_DEBUG_MESSAGE_TYPE(Disconnect)                                                          \
    HANDLE_DEBUG_MESSAGE_TYPE(BreakOnMicrostep)                                                    \
    HANDLE_DEBUG_MESSAGE_TYPE(Continue)

        template <typename TStructType>
        struct STypeToName
        {
        };

#define HANDLE_BREAKPOINT_TYPE(enumType, structType)                                               \
    template <>                                                                                    \
    struct STypeToName<structType>                                                                 \
    {                                                                                              \
        static const char8_t *GetName() { return #enumType; }                                      \
    };
#define HANDLE_DEBUG_MESSAGE_TYPE(msgType)                                                         \
    template <>                                                                                    \
    struct STypeToName<S##msgType>                                                                 \
    {                                                                                              \
        static const char8_t *GetName() { return #msgType; }                                       \
    };

        ITERATE_BREAKPOINT_TYPES
        ITERATE_DEBUG_MESSAGE_TYPES

#undef HANDLE_BREAKPOINT_TYPE
#undef HANDLE_DEBUG_MESSAGE_TYPE

        template <typename TMessageHandler>
        struct SMessageParser
        {
            TDebugStr m_MessageHeaderStr;
            TDebugStr m_TimestampStr;
            TDebugStr m_ParseStr;
            TDebugStr m_BreakpointTypeStr;
            TDebugStr m_TempStr[4];
            TMessageHandler *m_CurrentHandler;
            bool m_Valid;
            static void Trim(TDebugStr &inStr)
            {
                if (inStr.empty())
                    return;
                eastl::string::size_type nonSpace = inStr.find_first_not_of(' ');
                eastl::string::size_type lastNonSpace = inStr.find_last_not_of(' ');
                if (nonSpace == TDebugStr::npos || lastNonSpace == TDebugStr::npos) {
                    inStr.clear();
                    return;
                }

                eastl::string::size_type msgLen = lastNonSpace - nonSpace;
                inStr.erase(inStr.begin(), inStr.begin() + nonSpace);
                inStr.resize(msgLen + 1);
            }

            void TrimForward(eastl::string::size_type startPos)
            {
                eastl::string::size_type nextPos = m_ParseStr.find_first_not_of(' ', startPos);
                if (nextPos != TDebugStr::npos)
                    m_ParseStr.erase(m_ParseStr.begin(), m_ParseStr.begin() + nextPos);
                else
                    m_ParseStr.clear();
            }

            void TrimForward()
            {
                eastl::string::size_type nextSpace = m_ParseStr.find_first_of(' ');
                TrimForward(nextSpace);
            }

            void visitId(TDebugStr &inStr)
            {
                eastl::string::size_type nextSpace = m_ParseStr.find_first_of(' ');
                if (nextSpace == TDebugStr::npos)
                    nextSpace = m_ParseStr.size();
                inStr.assign(m_ParseStr.begin(), m_ParseStr.begin() + nextSpace);
                TrimForward(nextSpace);
            }
            void visit(TDebugStr &inStr) { randomText(inStr); }
            void visit(QT3DSI32 &inData)
            {
                StringConversion<QT3DSI32>().StrTo(m_ParseStr.c_str(), inData);
                TrimForward();
            }
            void visit(QT3DSU32 &inData)
            {
                StringConversion<QT3DSU32>().StrTo(m_ParseStr.c_str(), inData);
                TrimForward();
            }

            void visit(QT3DSU64 &inData)
            {
                StringConversion<QT3DSU64>().StrTo(m_ParseStr.c_str(), inData);
                TrimForward();
            }

            void visit(float &inData)
            {
                StringConversion<QT3DSF32>().StrTo(m_ParseStr.c_str(), inData);
                TrimForward();
            }

            void visit(bool &inData)
            {
                if (CompareNChars(m_ParseStr.c_str(), "True", 4) == 0)
                    inData = true;
                else
                    inData = false;

                TrimForward();
            }

            template <typename TPtrType>
            void visitOpaquePtr(TPtrType *&inPtr)
            {
                QT3DSU64 temp;
                visit(temp);
                inPtr = reinterpret_cast<TPtrType *>(static_cast<size_t>(temp));
            }

            void visit(SDatamodelTable *&inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelUserData *&inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelFunction *&inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelCFunction *&inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelThread *&inData) { visitOpaquePtr(inData); }
            void visit(SNil &) {}

            void visit(SDatamodelValue &inValue)
            {
                visitId(m_TempStr[0]);
#define HANDLE_DATAMODEL_VALUE_TYPE(name, dtype)                                                   \
    if (AreEqual(m_TempStr[0].c_str(), #name)) {                                                   \
        dtype theData;                                                                             \
        visit(theData);                                                                            \
        inValue = theData;                                                                         \
    } else
                ITERATE_DATAMODEL_VALUE_TYPES
#undef HANDLE_DATAMODEL_VALUE_TYPE
                {
                    QT3DS_ASSERT(false);
                }
            }

            void visit(TTableModificationList &inModifications)
            {
                QT3DSU32 numMods = 0;
                visit(numMods);
                inModifications.resize(numMods);
                for (QT3DSU32 idx = 0, end = inModifications.size(); idx < end; ++idx) {
                    STableModification &theModification(inModifications[idx]);
                    visitId(m_TempStr[0]);
                    if (AreEqual(m_TempStr[0].c_str(), "SetKey"))
                        theModification.m_Type = TableModificationType::SetKey;
                    else if (AreEqual(m_TempStr[0].c_str(), "RemoveKey"))
                        theModification.m_Type = TableModificationType::RemoveKey;
                    else {
                        QT3DS_ASSERT(false);
                        continue;
                    }
                    visit(theModification.m_Entry.m_Key);
                    if (theModification.m_Type == TableModificationType::SetKey)
                        visit(theModification.m_Entry.m_Value);
                }
            }

            void visit(TTableEntryList &inEntryList)
            {
                QT3DSU32 numMods = 0;
                visit(numMods);
                inEntryList.resize(numMods);
                for (QT3DSU32 idx = 0, end = inEntryList.size(); idx < end; ++idx) {
                    STableEntry &theEntry(inEntryList[idx]);
                    visit(theEntry.m_Key);
                    visit(theEntry.m_Value);
                }
            }

            void randomText(TDebugStr &outStr)
            {
                QT3DSU32 strLen = 0;
                visit(strLen);
                outStr.assign(m_ParseStr.begin(), m_ParseStr.begin() + strLen);
                TrimForward(strLen);
            }

            void visitIdList(TDebugStrList &inList)
            {
                QT3DSU32 numItems = 0;
                visit(numItems);
                inList.resize(numItems);
                for (QT3DSU32 idx = 0, end = numItems; idx < end; ++idx)
                    visitId(inList[idx]);
            }

            void visit(TTransitionIdList &inList)
            {
                QT3DSU32 numItems = 0;
                visit(numItems);
                inList.resize(numItems);
                for (QT3DSU32 idx = 0, end = numItems; idx < end; ++idx)
                    inList[idx] = BreakpointTransition();
            }

            void visitEventName(TDebugStr &outStr) { visitId(outStr); }

            SStateEnterBreakpoint BreakpointStateEnter()
            {
                SStateEnterBreakpoint retval;
                SBreakpointSerializationVisitor<SStateEnterBreakpoint>::Visit(*this, retval);
                return retval;
            }

            SStateExitBreakpoint BreakpointStateExit()
            {
                SStateExitBreakpoint retval;
                SBreakpointSerializationVisitor<SStateExitBreakpoint>::Visit(*this, retval);
                return retval;
            }

            STransitionId BreakpointTransition()
            {
                STransitionId retval;
                SBreakpointSerializationVisitor<STransitionId>::Visit(*this, retval);
                return retval;
            }

            void visit(SBreakpoint &breakpoint)
            {
                visitId(m_BreakpointTypeStr);
#define HANDLE_BREAKPOINT_TYPE(enumType, bpType)                                                   \
    if (AreEqual(STypeToName<bpType>::GetName(), m_BreakpointTypeStr.c_str())) {                   \
        breakpoint = Breakpoint##enumType();                                                       \
    } else
                ITERATE_BREAKPOINT_TYPES
#undef HANDLE_BREAKPOINT_TYPE
                // else is defined in the iterate breakpoints.
                {
                    m_Valid = false;
                    m_CurrentHandler->error("Unrecognized breakpoint type: ",
                                            m_BreakpointTypeStr.c_str());
                }
            }

            template <typename TDataType>
            void DoParseT(TDebugStr &inStr, QT3DSU64 inTimestamp, QT3DSI32 streamId,
                          TMessageHandler &inHandler)
            {
                TDataType theType;
                m_ParseStr = inStr;
                m_Valid = true;
                m_CurrentHandler = &inHandler;
                theType.Visit(*this);
                if (m_Valid)
                    inHandler.OnMessage(streamId, inTimestamp, theType);
            }

            // destructive parsing.
            void Parse(NVConstDataRef<QT3DSU8> data, TMessageHandler &inHandler)
            {
                m_ParseStr.assign((const char *)data.begin(), (const char *)data.end());
                Trim(m_ParseStr);
                size_t spacePos = m_ParseStr.find_first_of(' ');
                if (spacePos == TDebugStr::npos) {
                    m_ParseStr.assign((const char *)data.begin(), (const char *)data.end());
                    inHandler.error("Failed to parse message: ", m_ParseStr.c_str());
                    return;
                }
                qt3ds::QT3DSI32 streamId = 0;
                visitId(m_MessageHeaderStr);
                visit(streamId);
                qt3ds::QT3DSU64 timestamp = 0;
                visit(timestamp);
#define HANDLE_DEBUG_MESSAGE_TYPE(name)                                                            \
    if (AreEqual(STypeToName<S##name>::GetName(), m_MessageHeaderStr.c_str())) {                   \
        DoParseT<S##name>(m_ParseStr, timestamp, streamId, inHandler);                             \
    } else
                ITERATE_DEBUG_MESSAGE_TYPES
#undef HANDLE_DEBUG_MESSAGE_TYPE
                {
                    inHandler.error("Failed to parse message type: ", m_MessageHeaderStr.c_str());
                }
            }
        };

        struct SMessageSerializer
        {
            TDebugStr m_Message;
            void appendSpace() { m_Message.append(1, ' '); }

            void visitId(const TDebugStr &inStr)
            {
                m_Message.append(inStr);
                appendSpace();
            }
            template <typename TDataType>
            void rawTypeToStr(const TDataType &inType)
            {
                char8_t buf[256] = { 0 };
                StringConversion<TDataType>().ToStr(inType, toDataRef(buf, 256));
                m_Message.append(buf);
                appendSpace();
            }
            void visit(QT3DSI32 inData) { rawTypeToStr(inData); }
            void visit(QT3DSU32 inData) { rawTypeToStr(inData); }

            void visit(QT3DSU64 inData) { rawTypeToStr(inData); }

            void visit(bool inData) { rawTypeToStr(inData); }

            void visit(const TDebugStr &inStr) { randomText(inStr); }

            void visit(const SNil &) {}

            void visit(const TTransitionIdList &inData)
            {
                QT3DSU32 len = (QT3DSU32)inData.size();
                visit(len);
                for (QT3DSU32 idx = 0; idx < len; ++idx)
                    Breakpoint(inData[idx]);
            }

            void randomText(const TDebugStr &outStr)
            {
                QT3DSU32 len = (QT3DSU32)outStr.size();
                visit(len);
                if (len)
                    m_Message.append(outStr);
                appendSpace();
            }

            void visitIdList(const TDebugStrList &inList)
            {
                QT3DSU32 numItems = (QT3DSU32)inList.size();
                visit(numItems);
                for (QT3DSU32 idx = 0; idx < numItems; ++idx)
                    visitId(inList[idx]);
            }

            void visitEventName(const TDebugStr &outStr) { visitId(outStr); }
            void visit(float inValue) { rawTypeToStr(inValue); }
            template <typename TPtrType>
            void visitOpaquePtr(const TPtrType *inPtr)
            {
                size_t ptrValue = reinterpret_cast<size_t>(inPtr);
                QT3DSU64 fullValue = static_cast<QT3DSU64>(ptrValue);
                visit(fullValue);
            }

            void visit(SDatamodelTable *inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelUserData *inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelFunction *inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelCFunction *inData) { visitOpaquePtr(inData); }
            void visit(SDatamodelThread *inData) { visitOpaquePtr(inData); }

            void visit(const SDatamodelValue &inValue)
            {
                switch (inValue.getType()) {
#define HANDLE_DATAMODEL_VALUE_TYPE(name, dtype)                                                   \
    case DatamodelValueTypes::name: {                                                              \
        m_Message.append(#name);                                                                   \
        appendSpace();                                                                             \
        visit(inValue.getData<dtype>());                                                           \
        break;                                                                                     \
    }
                    ITERATE_DATAMODEL_VALUE_TYPES
#undef HANDLE_DATAMODEL_VALUE_TYPE
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }

            void visit(const TTableModificationList &inModifications)
            {
                QT3DSU32 numItems = (QT3DSU32)inModifications.size();
                visit(numItems);
                for (QT3DSU32 idx = 0, end = inModifications.size(); idx < end; ++idx) {
                    const STableModification &theMod(inModifications[idx]);
                    switch (theMod.m_Type) {
                    case TableModificationType::SetKey: {
                        m_Message.append("SetKey");
                        appendSpace();
                        visit(theMod.m_Entry.m_Key);
                        visit(theMod.m_Entry.m_Value);
                        break;
                    }
                    case TableModificationType::RemoveKey: {
                        m_Message.append("RemoveKey");
                        appendSpace();
                        visit(theMod.m_Entry.m_Key);
                        break;
                    }
                    default:
                        QT3DS_ASSERT(false);
                        m_Message.append("badvalue");
                        appendSpace();
                        continue;
                    }
                }
            }
            void visit(TTableEntryList &inEntries)
            {
                QT3DSU32 numItems = (QT3DSU32)inEntries.size();
                visit(numItems);
                for (QT3DSU32 idx = 0, end = inEntries.size(); idx < end; ++idx) {
                    const STableEntry &theEntry(inEntries[idx]);
                    visit(theEntry.m_Key);
                    visit(theEntry.m_Value);
                }
            }

            template <typename TBreakpointType>
            void Breakpoint(const TBreakpointType &inBp)
            {
                SBreakpointSerializationVisitor<TBreakpointType>::Visit(
                    *this, const_cast<TBreakpointType &>(inBp));
            }

            void visit(const SBreakpoint &inBp)
            {
                switch (inBp.getType()) {
#define HANDLE_BREAKPOINT_TYPE(enumName, typeName)                                                 \
    case BreakpointTypes::enumName:                                                                \
        m_Message.append(STypeToName<typeName>::GetName());                                        \
        appendSpace();                                                                             \
        Breakpoint(inBp.getData<typeName>());                                                      \
        break;
                    ITERATE_BREAKPOINT_TYPES
#undef HANDLE_BREAKPOINT_TYPE
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }

            template <typename TMsgType>
            void Serialize(QT3DSI32 inStreamId, const TMsgType &inMsg, QT3DSU64 inTimestamp)
            {
                m_Message.clear();
                m_Message.append(STypeToName<TMsgType>::GetName());
                appendSpace();
                visit(inStreamId);
                visit(inTimestamp);
                const_cast<TMsgType &>(inMsg).Visit(*this);
            }

            NVConstDataRef<QT3DSU8> ToRawMessage()
            {
                if (m_Message.size() == 0)
                    return NVConstDataRef<QT3DSU8>();
                return NVConstDataRef<QT3DSU8>(reinterpret_cast<const QT3DSU8 *>(m_Message.c_str()),
                                            (QT3DSU32)m_Message.size());
            }
        };

        template <typename TLhs, typename TRhs>
        struct SMsgComparer
        {
            static bool Compare(const TLhs &, const TRhs &) { return false; }
        };
#define HANDLE_DEBUG_MESSAGE_TYPE(typeName)                                                        \
    template <>                                                                                    \
    struct SMsgComparer<S##typeName, S##typeName>                                                  \
    {                                                                                              \
        static bool Compare(const S##typeName &lhs, const S##typeName &rhs) { return lhs == rhs; } \
    };
        ITERATE_DEBUG_MESSAGE_TYPES;
#undef HANDLE_DEBUG_MESSAGE_TYPE

        template <typename TMessageType>
        struct STestMessageHandler
        {
            bool m_Result;
            QT3DSU64 m_Timestamp;
            QT3DSU32 m_StreamId;
            const TMessageType &m_ExpectedResult;

            STestMessageHandler(QT3DSU32 sid, QT3DSU64 ts, const TMessageType &res)
                : m_Result(false)
                , m_Timestamp(ts)
                , m_StreamId(sid)
                , m_ExpectedResult(res)
            {
            }

            template <typename TCrapType>
            void OnMessage(QT3DSU32 sid, QT3DSU64 ts, const TCrapType &msgType)
            {
                if (ts == m_Timestamp && m_StreamId == sid)
                    m_Result =
                        SMsgComparer<TCrapType, TMessageType>::Compare(msgType, m_ExpectedResult);
            }

            void error(const char8_t *, const char8_t *) {}
        };
    }
}
}

#endif