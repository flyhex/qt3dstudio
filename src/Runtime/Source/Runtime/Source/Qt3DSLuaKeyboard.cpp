/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "RuntimePrefix.h"
#include "Qt3DSLuaEngine.h"
#include "Qt3DSInputFrame.h"
#include "Qt3DSLuaKeyboard.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSApplication.h"

//==============================================================================
//	Namespace
//==============================================================================
using namespace Q3DStudio;
using qt3ds::runtime::IApplication;
//==============================================================================
//	Statics
//==============================================================================

// This list needs to be in sync with EKeyCode (AKInputFrame.h)
// TODO: Maybe auto-gen this in the future
static const char *s_KeyCodeIdentifier[] = { "KEY_NOKEY",
                                             "KEY_ESCAPE",
                                             "KEY_1",
                                             "KEY_2",
                                             "KEY_3",
                                             "KEY_4",
                                             "KEY_5",
                                             "KEY_6",
                                             "KEY_7",
                                             "KEY_8",
                                             "KEY_9",
                                             "KEY_0",
                                             "KEY_SUBTRACT",
                                             "KEY_EQUALS",
                                             "KEY_BACK",
                                             "KEY_TAB",
                                             "KEY_Q",
                                             "KEY_W",
                                             "KEY_E",
                                             "KEY_R",
                                             "KEY_T",
                                             "KEY_Y",
                                             "KEY_U",
                                             "KEY_I",
                                             "KEY_O",
                                             "KEY_P",
                                             "KEY_LBRACKET",
                                             "KEY_RBRACKET",
                                             "KEY_RETURN",
                                             "KEY_LCONTROL",
                                             "KEY_A",
                                             "KEY_S",
                                             "KEY_D",
                                             "KEY_F",
                                             "KEY_G",
                                             "KEY_H",
                                             "KEY_J",
                                             "KEY_K",
                                             "KEY_L",
                                             "KEY_SEMICOLON",
                                             "KEY_APOSTROPHE",
                                             "KEY_GRAVE",
                                             "KEY_LSHIFT",
                                             "KEY_BACKSLASH",
                                             "KEY_Z",
                                             "KEY_X",
                                             "KEY_C",
                                             "KEY_V",
                                             "KEY_B",
                                             "KEY_N",
                                             "KEY_M",
                                             "KEY_COMMA",
                                             "KEY_PERIOD",
                                             "KEY_SLASH",
                                             "KEY_RSHIFT",
                                             "KEY_MULTIPLY",
                                             "KEY_LALT",
                                             "KEY_SPACE",
                                             "KEY_CAPITAL",
                                             "KEY_F1",
                                             "KEY_F2", // 60
                                             "KEY_F3",
                                             "KEY_F4",
                                             "KEY_F5",
                                             "KEY_F6",
                                             "KEY_F7",
                                             "KEY_F8",
                                             "KEY_F9",
                                             "KEY_F10",
                                             "KEY_NUMLOCK",
                                             "KEY_SCROLL", // 70
                                             "KEY_NUMPAD7",
                                             "KEY_NUMPAD8",
                                             "KEY_NUMPAD9",
                                             "KEY_NUMPADSUBTRACT",
                                             "KEY_NUMPAD4",
                                             "KEY_NUMPAD5",
                                             "KEY_NUMPAD6",
                                             "KEY_NUMPADADD",
                                             "KEY_NUMPAD1",
                                             "KEY_NUMPAD2", // 80
                                             "KEY_NUMPAD3",
                                             "KEY_NUMPAD0",
                                             "KEY_NUMPADDECIMAL",
                                             "KEY_NOOP",
                                             "KEY_ZENKAKUHANKAKU",
                                             "KEY_102ND",
                                             "KEY_F11",
                                             "KEY_F12",
                                             "KEY_F13",
                                             "KEY_F14", // 90
                                             "KEY_HIRAGANA",
                                             "KEY_HENKAN",
                                             "KEY_KATAKANAHIRAGANA",
                                             "KEY_MUHENKAN",
                                             "KEY_KPJPCOMMA",
                                             "KEY_NUMPADENTER",
                                             "KEY_RCONTROL",
                                             "KEY_NUMPADDIVIDE",
                                             "KEY_PRINTSCREEN",
                                             "KEY_RALT", // 100
                                             "KEY_LINEFEED",
                                             "KEY_HOME",
                                             "KEY_UP",
                                             "KEY_PGUP",
                                             "KEY_LEFT",
                                             "KEY_RIGHT",
                                             "KEY_END",
                                             "KEY_DOWN",
                                             "KEY_PGDN",
                                             "KEY_INSERT", // 110
                                             "KEY_DELETE",
                                             "KEY_MACRO",
                                             "KEY_MUTE",
                                             "KEY_VOLUMEDOWN",
                                             "KEY_VOLUMEUP",
                                             "KEY_POWER",
                                             "KEY_KPEQUAL",
                                             "KEY_KPPLUSMINUS",
                                             "KEY_PAUSE",
                                             "KEY_SCALE" };

//==============================================================================
/**
 *	Registers global keycodes
 */
void CLuaKeyboard::RegisterLibrary(lua_State *inLuaState)
{
    for (long theCount = 0; theCount < KEY_TOTAL_COUNT; ++theCount) {
        lua_pushinteger(inLuaState, theCount);
        lua_setglobal(inLuaState, s_KeyCodeIdentifier[theCount]);
    }

    // Register the hook function
    lua_register(inLuaState, "getKeyState", GetKeyState);
}

//==============================================================================
/**
 *	Gets the Key Table
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - boolean to indicate if the key is pressed
 */
int CLuaKeyboard::GetKeyState(lua_State *inLuaState)
{
    const INT32 ARG_KEYCODE = 1;

    luaL_checktype(inLuaState, ARG_KEYCODE, LUA_TNUMBER);
    long theKeyCode = lua_tointeger(inLuaState, ARG_KEYCODE);

    // Check if the keycode is valid
    if (theKeyCode >= KEY_TOTAL_COUNT || theKeyCode < KEY_NOKEY) {
        qCInfo(qt3ds::TRACE_INFO) << "Lua: getKeyState: The keycode you entered is invalid";
        theKeyCode = KEY_NOKEY;
    }

    // Check the key state
    CPresentation *thePresentation = CLuaEngine::GetCurrentPresentation(inLuaState);
    IApplication &theApplication = thePresentation->GetApplication();
    SKeyInputFrame theInput = theApplication.GetInputEngine().GetKeyInputFrame();

    lua_pushboolean(inLuaState,
                    KEY_NOKEY ? false : (theInput.GetKeyCount(theKeyCode) > 0 ? true : false));

    return 1;
}
