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
#ifndef UIC_STATE_INTERPRETER_H
#define UIC_STATE_INTERPRETER_H
#pragma once
#include "Qt3DSState.h"
#include "Qt3DSStateTypes.h"
#include "Qt3DSStateSignalConnection.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace state {
    struct InterpreterEventTypes
    {
        enum Enum {
            UnknownInterpreterEvent = 0,
            StateEnter,
            StateExit,
            Transition,
        };
    };

    struct IStateInterpreterEventHandler
    {
    protected:
        virtual ~IStateInterpreterEventHandler() {}

    public:
        virtual void OnInterpreterEvent(InterpreterEventTypes::Enum inEvent,
                                        CRegisteredString inEventId) = 0;
    };

    class IStateInterpreter : public NVRefCounted
    {
    protected:
        virtual ~IStateInterpreter() {}
    public:
        // Setup of the state system, you can add a set of roots to the state graph.
        virtual NVConstDataRef<SStateNode *> GetConfiguration() = 0;
        // State context is referenced by this object.
        // We can optionally validate the transitions to ensure that no transition can put us into
        // an invalid state
        // and so that simple loops don't exist.  It is highly recommended to use this; it is done
        // once via startup and
        // shouldn't add appreciably to the initialization time but it makes the transition system a
        // lot more robust.
        virtual bool Initialize(IStateContext &inContext, bool inValidateTransitions = true) = 0;
        // Bring the state machine to the internal state.
        // Returns the list of states for the initial configuration
        virtual NVConstDataRef<SStateNode *> Start() = 0;

        // Execute transitions, internal events and external events until nothing is left to
        // be done.
        virtual NVConstDataRef<SStateNode *> Execute() = 0;

        virtual bool IsRunning() const = 0;
        virtual bool EventsPending() const = 0;

        // Queue an event with an optional delay (0 means no delay).
        // If the event has a delay then it can optionally be cancelled.
        virtual void QueueEvent(const char8_t *inEventName, QT3DSU64 inDelay,
                                CRegisteredString inCancelId, bool inIsExternal = true) = 0;
        virtual void QueueEvent(TEventPtr inEvent, QT3DSU64 inDelay, CRegisteredString inCancelId,
                                bool inIsExternal = true) = 0;
        virtual void QueueEvent(TEventPtr inEvent, bool inIsExternal = true) = 0;
        virtual void QueueEvent(const char8_t *inEventName, bool inIsExternal = true) = 0;

        // Cancel an event with a particular id.  Only cancels delayed events that have not fired
        // yet.
        virtual void CancelEvent(CRegisteredString inCancelId) = 0;

        // When the connection gets destroyed, the hander will get no more events.
        virtual TSignalConnectionPtr
        RegisterEventHandler(IStateInterpreterEventHandler &inHandler) = 0;

        virtual debugger::IStateMachineDebugInterface &GetDebugInterface() = 0;

        virtual NVFoundationBase &GetFoundation() = 0;

        virtual IScriptContext &GetScriptContext() = 0;

        virtual IStateContext *GetStateContext() = 0;

        // The only code that needs to happen in the state system is the code that executes a
        // condition.
        static IStateInterpreter &Create(NVFoundationBase &inFnd, IStringTable &inStrTable,
                                         IScriptContext &inScriptContext,
                                         IExecutionContext &inExecutionContext);
    };
}
}

#endif
