/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>

#include "UICLuaIncludes.h"
#include "foundation/Qt3DSMath.h"

#include "EventSystem.h"
#include "EventPollingSystem.h"
#include "EventPollingSystemLuaBinding.h"

using namespace uic::evt;

namespace {
// No need to use static in an anonymous namespace

int GetCInterface(lua_State *inState);
// Get a set of events from the poller.  The poller will simply run through and ask each provider
// for events and then begin returning the set of events.
// Parameters:
//  Instance of event system interface
//  Max number of events expected
// Returns:
//  Array of event objects.
// Remarks: the number of events returned won't exceed the input count; if there are less events
// than
// the input count, less events will be returned; if no event, an empty array will be returned.
int GetNextEvents(lua_State *inState);

void TranslateEventValue(lua_State *inState, const SUICEventSystemEventValue &theValue)
{
    switch (theValue.m_Type) {
    case EUICEventSystemEventTypesNumber:
        lua_pushnumber(inState, theValue.m_Number);
        break;
    case EUICEventSystemEventTypesString:
        lua_pushstring(inState, theValue.m_String ? theValue.m_String : "");
        break;
    default:
        QT3DS_ASSERT(false);
    }
}

void TranslateEvent(lua_State *inState, SUICEventSystemEvent &theEvent)
{
    int top = lua_gettop(inState);
    // Put the result table of event on top of the lua stack
    // If the data of the same name occur more than once, they will be packed into an array(table)
    // in order,
    // otherwise just a string or number object.
    lua_newtable(inState); // The result table of event
    for (int i = 0; i < theEvent.m_NumData; ++i) {
        const SUICEventSystemEventData &theValue(theEvent.m_Data[i]);
        const char *theFetchableString(theValue.m_Name.m_Data);
        lua_getfield(inState, -1, theFetchableString);
        // Is the string already in the table?
        if (lua_isnil(inState, -1)) {
            // no, it is not
            lua_pop(inState, 1);
            TranslateEventValue(inState, theValue.m_Value);
            lua_setfield(inState, -2, theFetchableString);
        } else {
            if (!lua_istable(inState, -1)) {
                // create new table
                lua_newtable(inState);
                // move it below the existing value on the stack
                lua_insert(inState, -2);
                // set the existing value into the new table at location 1
                lua_rawseti(inState, -2, 1);
                // duplicate the table reference on the stack
                lua_pushvalue(inState, -1);
                // Set the table as the new value under the string name in the original table.
                lua_setfield(inState, -3, theFetchableString);
                // this leaves two tables on the stack, the return value
                // and the new table.
            }
            // table of new values is now on the stack.
            // Get the length of the value array.
            int len = static_cast<int>(lua_objlen(inState, -1));
            // Get the new value
            TranslateEventValue(inState, theValue.m_Value);
            lua_rawseti(inState, -2, len + 1);
            // Leave only the return value on the stack.
            lua_pop(inState, 1);
            QT3DS_ASSERT(lua_gettop(inState) == top + 1);
            QT3DS_ASSERT(lua_istable(inState, -1));
        }
    }
    (void)top;
}

int GetNextEvents(lua_State *inState)
{
    // Parameter:
    //   Poller(C)
    //   number of events expected
    // Return: an array of events
    static const size_t MAX_EVENTS_ONCE = 512;
    int top = lua_gettop(inState);

    luaL_checktype(inState, 1, LUA_TTABLE);

    size_t theMaxCount = MAX_EVENTS_ONCE;
    if (top == 2) {
        luaL_checktype(inState, 2, LUA_TNUMBER);
        int theValue = lua_tointeger(inState, 2);
        theValue = qt3ds::NVMax(0, theValue);
        theMaxCount = qt3ds::NVMin(theMaxCount, (size_t)theValue);
    }

    lua_getfield(inState, 1, POLLER_TABLE_ENTRY);
    luaL_checktype(inState, -1, LUA_TLIGHTUSERDATA);

    SUICEventSystemEventPoller *thePollerC =
        (SUICEventSystemEventPoller *)lua_touserdata(inState, -1);
    IEventSystem *thePoller = 0;
    lua_newtable(inState);
    if (!thePollerC || (thePoller = reinterpret_cast<IEventSystem *>(thePollerC->m_Poller)) == 0
        || theMaxCount <= 0)
        return 1;

    SUICEventSystemEvent *theEvents[MAX_EVENTS_ONCE];
    size_t theCount = thePoller->GetNextEvents(theEvents, theMaxCount);
    for (qt3ds::QT3DSU32 theIndex = 0; theIndex < theCount; ++theIndex) {
        TranslateEvent(inState, *theEvents[theIndex]);
        lua_rawseti(inState, -2, theIndex + 1);
    }
    return 1;
}

// int
// GetCInterface( lua_State* inState )
//{
//	// Parameter: Poller(C++)
//	// Return: c function table in light user data
//	static const size_t	MAX_EVENTS_ONCE = 512;
//	luaL_checktype( inState, 1, LUA_TTABLE );
//	lua_getfield( inState, 1, pollerTableEntry );
//	luaL_checktype( inState, -1, LUA_TLIGHTUSERDATA );
//	IEventSystem	*thePoller = (IEventSystem*)lua_touserdata( inState, -1 );
//	if ( thePoller )
//	{
//		lua_pushlightuserdata( inState, thePoller->GetCInterface() );
//		return 1;
//	}
//	return 0;
//}
}

void uic::evt::SLuaEventPollerBinding::WrapEventPoller(lua_State *inState, IEventSystem &inPoller)
{
    lua_newtable(inState);
    lua_pushlightuserdata(inState, inPoller.GetCInterface()); // Store the C poller, for Lua will
                                                              // hand over it to provider factories
                                                              // directly.
    lua_setfield(inState, -2, POLLER_TABLE_ENTRY);
    lua_pushcfunction(inState, GetNextEvents);
    lua_setfield(inState, -2, "getNextEvents");
    //	lua_pushcfunction( inState, GetCInterface );
    //	lua_setfield( inState, -2, "getCInterface" );
}
