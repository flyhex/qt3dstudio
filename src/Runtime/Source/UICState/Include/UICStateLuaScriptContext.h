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
#ifndef UIC_STATE_LUA_SCRIPT_CONTEXT_H
#define UIC_STATE_LUA_SCRIPT_CONTEXT_H
#pragma once
#include "UICState.h"
#include "UICStateScriptContext.h"

struct lua_State;

namespace qt3ds {
namespace state {

    class ILuaScriptContext : public IScriptContext
    {
    public:
        static const char8_t *GetLuaContextTypeName() { return "lua5.1"; }
        // Dump the global state table's keys and values
        virtual void DumpGlobalState() = 0;

        // Push the state machine's global table onto the stack.
        virtual void GetGlobalTable() = 0;

        static ILuaScriptContext &CreateLuaScriptContext(NVFoundationBase &inFoundation,
                                                         IStringTable &inStrTable);
        // Create a lua script context, chaining our information to another global state.
        // In this case we won't close the state on exit.
        static ILuaScriptContext &CreateLuaScriptContext(NVFoundationBase &inFoundation,
                                                         IStringTable &inStrTable,
                                                         lua_State *inState,
                                                         int inGlobalTableIdx = 0);

        // Export bindings that allow us to call the lua system.
        // Exports these bindings to the global context:
        // - parseSCXML{fullpathtofile} - parses an SCXML file and returns a lua object
        //
        // with at least these functions
        //
        // -- onAfterEnter - (callback) property called as function after state enter if valid.
        // -- onBeforeExit - (callback) property called as function before state exit if valid.
        //
        // -- start {dm} start the state machine, optionally taking a datatable to use as the state
        // machine's datatable.
        // -- set(name,val) set the datamodel at name to value
        // -- fireEvent(val) - val must have 'name' property set, will fire 'name' event in state
        // machine.
        // -- step() - step the state machine.
        static void ExportLuaBindings(lua_State *inState);
        // Export the parse function so clients can bind it wherever they like into their lua
        // system.
        // Optionally pass in a debugger for the system to use if a debug connection occurs.
        static int ParseSCXML(lua_State *inState, debugger::IDebugger *inDebugger = NULL);
        static int ParseSCXML(lua_State *inState, debugger::IDebugger *inDebugger,
                              NVFoundationBase &inFoundation, IStringTable &inStrTable);
        static int Initialize(lua_State *inState, const char8_t *inId = 0);
        static int Start(lua_State *inState);
        // Given the lua state, the top item on the stack should be the table returned from the
        // parseSCXML function
        static IStateInterpreter *GetInterpreterFromBindings(lua_State *inState);

        // Bind the lua interpreter into the lua state.  The interpreter object is now at the top
        // of the stack, so be sure to settop back when you are finished.
        static void Bind(IStateInterpreter &inInterpreter, lua_State *inState);
    };
}
}

#endif