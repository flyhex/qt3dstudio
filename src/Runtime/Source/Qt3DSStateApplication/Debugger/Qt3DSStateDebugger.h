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
#ifndef QT3DS_STATE_DEBUGGER_H
#define QT3DS_STATE_DEBUGGER_H
#pragma once
#include "Qt3DSState.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/StringTable.h"
#include "foundation/IOStreams.h"
#include "Qt3DSStateEditor.h"

namespace qt3ds {
namespace state {
    namespace debugger {
        using namespace editor;

        typedef TEditorStr TDebugStr;
        struct SDebugValue;
        struct SBreakpoint;
        struct SNil;
        struct SDatamodelTable;
        struct SDatamodelUserData;
        struct SDatamodelFunction;
        struct SDatamodelCFunction;
        struct SDatamodelThread;

        typedef eastl::vector<SBreakpoint> TBreakpointList;

        struct BreakpointTypes
        {
            enum Enum {
                UnknownBreakpointType = 0,
                StateEnter,
                StateExit,
                Transition,
            };
        };

        struct SDebugStreamMessage
        {
            // Timestamp needs to be converted using the SourceConversion on the protocol socket.
            QT3DSU64 m_Timestamp;
            NVConstDataRef<QT3DSU8> m_Data;
            SDebugStreamMessage()
                : m_Timestamp(0)
            {
            }
            SDebugStreamMessage(QT3DSU64 ts, NVConstDataRef<QT3DSU8> msg)
                : m_Timestamp(ts)
                , m_Data(msg)
            {
            }
        };

        // Note the stream listeners are not ref counted.
        // it is your job to ensure it lasts as long as debug stream.
        class IDebugStreamListener
        {
        protected:
            virtual ~IDebugStreamListener() {}
        public:
            // Async message interface.  Clients must provide this.  Only called on main thread
            virtual void OnMessageReceived(const SDebugStreamMessage &inMessage) = 0;
        };

        class IDebugOutStream : public NVRefCounted, public IOutStream
        {
        protected:
            virtual ~IDebugOutStream() {}
        public:
            virtual void SetListener(IDebugStreamListener *listener) = 0;
            virtual IDebugStreamListener *GetListener() = 0;
            // return true if the stream is still connected to an output, false if
            // something caused the connection to fail or close.
            virtual SDebugStreamMessage WaitForNextMessage() = 0;
            virtual bool Connected() = 0;
        };

        struct STransitionId
        {
            TDebugStr m_StateId;
            // Index in file order of the transition.
            //-1 means initial or history::transition, depending on if the state is
            // a normal state or history state.
            QT3DSI32 m_TransitionIndex;
            STransitionId(const TDebugStr &inId = TDebugStr(), QT3DSI32 inIdx = -2)
                : m_StateId(inId)
                , m_TransitionIndex(inIdx)
            {
            }
            bool operator==(const STransitionId &inOther) const
            {
                return m_StateId == inOther.m_StateId
                    && m_TransitionIndex == inOther.m_TransitionIndex;
            }
        };

        typedef eastl::vector<STransitionId> TTransitionIdList;

        ///////////////////////////////////////////////////////////////////////
        // Client (runtime) side types
        ///////////////////////////////////////////////////////////////////////

        struct DatamodelValueTypes
        {
            enum Enum {
                UnknownType = 0,
                Nil,
                Boolean,
                Number,
                String,
                Table,
                UserData,
                Function,
                CFunction,
                Thread,
            };
        };

        struct SDatamodelValue;

        struct SDatamodelTable;
        struct SDatamodelUserData;
        struct SDatamodelFunction;
        struct SDatamodelCFunction;
        struct SDatamodelThread;
        struct STableEntry;

        class IScriptStateListener
        {
        protected:
            virtual ~IScriptStateListener() {}
        public:
            virtual void SetKey(void *inTable, const TDebugStr &inStr,
                                const SDatamodelValue &inValue) = 0;
            virtual void RemoveKey(void *inTable, const TDebugStr &inStr) = 0;
        };

        class IDebugger;
        // Information coming from the state machine
        class IStateMachineListener : public NVRefCounted
        {
        protected:
            virtual ~IStateMachineListener() {}
        public:
            // Events coming from the interpreter and sent over the interface.
            virtual void OnConnect(const TDebugStr &SCXMLFilename, const TDebugStr &SCXMLFileData,
                                   NVConstDataRef<TDebugStr> inConfiguration) = 0;

            virtual void Log(const TDebugStr &inStr) = 0;
            virtual void EventQueued(const TDebugStr &inEventName, bool inInternal) = 0;
            virtual void BeginStep() = 0;
            virtual void BeginMicroStep() = 0;
            virtual void SetEvent(const TDebugStr &inEventName, bool inInternal) = 0;
            virtual void SetTransitionSet(NVConstDataRef<STransitionId> inTransitions) = 0;
            virtual void SetExitSet(NVConstDataRef<TDebugStr> inSet) = 0;
            virtual void SetEnterSet(NVConstDataRef<TDebugStr> inSet) = 0;
            // Log statements run through the debugger as well.
            virtual void EndMicroStep() = 0;
            virtual void EndStep() = 0;

            // So far the breakpoints have all been things that are internal to the
            // state machine.
            virtual void SetBreakOnMicrostep(bool inEnableStep) = 0;
            virtual void SetBreakpoint(const SBreakpoint &inBreakpoint) = 0;
            virtual void ClearBreakpoint(const SBreakpoint &inBreakpoint) = 0;
            virtual void ClearAllBreakpoints() = 0;

            // Called internally.
            virtual void Continue() = 0;
            virtual void OnExternalBreak() = 0;

            static IStateMachineListener &Create(NVAllocatorCallback &inAlloc, QT3DSU32 inStreamId,
                                                 IDebugOutStream &inOutStr, QT3DSU64 inStartTime,
                                                 IDebugger &inDebugger,
                                                 IScriptContext &inScriptContext);
        };

        // Information coming from the network stream will call these iterfaces
        // Since we want to debug both c++ and lua state machines, we need a common
        // interface to talk to.
        class IStateMachineDebugInterface : public NVRefCounted
        {
        protected:
            virtual ~IStateMachineDebugInterface() {}
        public:
            virtual void SetStateMachineListener(IStateMachineListener *inListener) = 0;
            virtual void OnExternalBreak() = 0;
            virtual IScriptContext &GetScriptContext() = 0;
        };

        // The debugger element is the object that sits in the debug process interpreter and sends
        // information.
        // There should be only one of these per network connection.
        class IDebugger : public IDebugStreamListener, public NVRefCounted
        {
        protected:
            virtual ~IDebugger() {}
        public:
            virtual void OnServerConnected(IDebugOutStream &inStream) = 0;
            virtual void Connect(IStateMachineDebugInterface &inMachine) = 0;
            // Called when an external entity breaks and we need to update the lua.
            // In this case we want to update all state machine datasets.
            virtual void OnExternalBreak() = 0;
            virtual void Disconnect(IStateMachineDebugInterface &inMachine) = 0;
            // Release any references to any state machines and to the output stream
            virtual void DisconnectFromServer() = 0;

            static IDebugger &CreateDebugger();
        };

        ///////////////////////////////////////////////////////////////////////
        // Server (Architect) side types
        ///////////////////////////////////////////////////////////////////////

        struct DebugValueTypes
        {
            enum Enum { NoDebugValue = 0, String, StringList, TransitionIdList };
        };

        typedef eastl::vector<TDebugStr> TDebugStrList;

        struct STableModification;

        class IDebuggedInterpreter;

        class IDebuggerMasterListener : public NVRefCounted
        {
        protected:
            virtual ~IDebuggerMasterListener() {}
        public:
            virtual void OnInterpreterConnected(IDebuggedInterpreter &inInterpreter) = 0;
            // If no breakpoint then the interpreter was broken on microstep.
            virtual void OnEventQueued(const TDebugStr &inEvent, bool inInternal) = 0;
            virtual void OnInterpreterBroken(IDebuggedInterpreter &inInterpreter,
                                             const Option<SBreakpoint> &inBreakpoint) = 0;
            // Event processed but no microstep was taken.
            virtual void OnEventProcessed(const TDebugStr &inEvent, bool inInternal) = 0;
            virtual void OnBeginStep(IDebuggedInterpreter &inInterpreter) = 0;
            virtual void OnBeginMicrostep(IDebuggedInterpreter &inInterpreter) = 0;
            // Event processed and microstep was taken.
            virtual void OnMicrostep(IDebuggedInterpreter &inInterpreter) = 0;
            // if inmodifications is empty the table needs to be replaced.
            virtual void OnDatamodelChange(IDebuggedInterpreter &inInterpreter,
                                           SDatamodelTable *inTable,
                                           NVConstDataRef<STableModification> inModifications) = 0;
            virtual void OnInterpreterDisconnected(IDebuggedInterpreter &inInterpreter) = 0;
            virtual void OnLog(IDebuggedInterpreter *inInterpreter, const TDebugStr &inMessage) = 0;
        };

        struct STableEntry;

        typedef eastl::vector<STableEntry> TTableEntryList;

        // defined in UICStateDebuggerProtocol.h"
        struct SBeginStep;
        struct SBeginMicrostep;
        struct SMicrostepEvent;
        struct SMicrostepData;
        struct SEndMicrostep;
        struct SEventQueued;
        struct SModifyTableValues;
        struct SDatamodelTable;

        // Represents the data stream coming from a single interpreter
        class IDebuggedInterpreter : public NVRefCounted
        {
        protected:
            virtual ~IDebuggedInterpreter() {}
        public:
            /* Playback interface, leaving until basic functionality is working.
            virtual QT3DSU32 GetStepCount() = 0;
            virtual void SetCurrentStep( QT3DSU32 inStep ) = 0;
            virtual QT3DSU32 GetMicrostepCount() = 0;
            virtual QT3DSU32 SetCurrentMicroStep( QT3DSU32 inStep ) = 0;
            */

            // Get the editor that represents the state graph for this debugged interpreter.
            virtual TEditorPtr GetEditor() = 0;
            virtual TDebugStr GetFilename() const = 0;

            // Used when connecting to a live session.
            virtual void SetBreakpoint(const SBreakpoint &inBreakpoint) = 0;
            virtual void ClearBreakpoint(const SBreakpoint &inBreakpoint) = 0;
            virtual NVConstDataRef<SBreakpoint> GetBreakpoints() = 0;
            virtual void ClearAllBreakpoints() = 0;
            // break at the *end* of the microstep so you can see the data of the microstep.
            virtual void SetBreakOnMicrostep(bool inBreak) = 0;
            virtual bool IsBreakOnMicrostep() const = 0;

            // Return values not valid after *next* call.
            // Null means the root.
            virtual NVConstDataRef<STableEntry>
            GetTableValues(SDatamodelTable *inTable = NULL) const = 0;

            virtual NVConstDataRef<TDebugStr> GetPreviousConfiguration() const = 0;
            virtual NVConstDataRef<TDebugStr> GetConfiguration() const = 0;
            virtual const TDebugStr &GetMicrostepEvent() const = 0;
            virtual NVConstDataRef<STransitionId> GetMicrostepTransitions() const = 0;
            virtual NVConstDataRef<TDebugStr> GetMicrostepEnterStates() const = 0;
            virtual NVConstDataRef<TDebugStr> GetMicrostepExitStates() const = 0;

            virtual bool IsBroken() const = 0;
            virtual void Continue() = 0;
            // Get the set of changed properties since the last time you asked.  Obviously this is
            // assuming there
            // is only one 'you' asking.
            virtual void Disconnect() = 0;

            // Semi-advanced debugger functionality that we don't need for version 1.

            // virtual void SetPropertyValue( const char8_t* inName, const SDebugValue& inValue ) =
            // 0;
            // Set a property value on the running state machine.
            // virtual void SetPropertyValue( TObjPtr inEditorObj, const char8_t* inName, const
            // SValueOpt& inValue ) = 0;

            // Eval code in the current state machine context.  Works when connected live, wouldn't
            // work if you
            // were not connected live.
            // virtual TDebugStr EvalCode( const TDebugStr& inStr ) = 0;

            // Information coming from the stream, clients should not call these functions
            virtual void BreakpointHit(const SBreakpoint &inBreakpoint) = 0;
            virtual void OnEventQueued(const SEventQueued &inMsg) = 0;
            virtual void OnBeginStep(const SBeginStep &inMsg) = 0;
            virtual void OnBeginMicrostep(const SBeginMicrostep &inMsg) = 0;
            virtual void OnMicrostepEvent(const SMicrostepEvent &inMsg) = 0;
            virtual void OnMicrostepData(const SMicrostepData &inMsg) = 0;
            virtual void OnEndMicrostep(const SEndMicrostep &inMsg) = 0;
            virtual void OnModifyTable(const SModifyTableValues &inValues) = 0;

            static IDebuggedInterpreter &Create(NVAllocatorCallback &inAlloc,
                                                IDebugOutStream &inStream, TEditorPtr inEditor,
                                                const TDebugStr &inFilename, QT3DSI32 inStreamId,
                                                NVConstDataRef<TDebugStr> inConfiguration,
                                                IDebuggerMasterListener &inListener);
        };

        // Debugger sites in the master process (Architect process) and controls one or more
        // debuggers
        // for one or more state machines running in the debug process.
        class IDebuggerMaster : public IDebugStreamListener, public NVRefCounted
        {
        protected:
            virtual ~IDebuggerMaster() {}
        public:
            virtual void Disconnect() = 0;

            static IDebuggerMaster &CreateMaster(IDebugOutStream &outStream,
                                                 IDebuggerMasterListener &inListener);
        };

        class CDebuggerTests
        {
        public:
            static bool TestStreamProtocol();
        };
    }
}
}

#endif
