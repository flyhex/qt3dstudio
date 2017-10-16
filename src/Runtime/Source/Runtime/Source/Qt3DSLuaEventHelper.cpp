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

#include "RuntimePrefix.h"

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSLuaEventHelper.h"
#include "Qt3DSLuaHelper.h"
#include "Qt3DSLuaEngine.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSHash.h"

//==============================================================================
// Namespace
//==============================================================================
namespace Q3DStudio {

namespace {
    static int ErrorChannel(lua_State *inLuaState)
    {
        qCritical(qt3ds::INVALID_OPERATION) << "Lua: ErrorChannel: " << lua_tostring(inLuaState, -1);
        lua_pop(inLuaState, 1);
        return 0;
    }
}

//==============================================================================
//	Constants
//==============================================================================

extern const INT32 KEY_BEHAVIORS;
extern const INT32 KEY_EVENTDATA_LIST;
extern const INT32 KEY_CHANGEDATA_LIST;
extern const INT32 KEY_CALLBACKS;

extern const char *KEY_ELEMENT;
extern const char *KEY_SELF;
extern const char *KEY_SCRIPT_INDEX;

extern const THashValue HASH_ONINITIALIZE;
extern const THashValue HASH_ONACTIVATE;
extern const THashValue HASH_ONDEACTIVATE;
extern const THashValue HASH_ONUPDATE;

extern const UINT32 LENGTH_KEY_ELEMENT;
extern const UINT32 LENGTH_KEY_SELF;
extern const UINT32 LENGTH_KEY_SCRIPT_INDEX;

const INT32 EVENTHELPER_REGISTER = 0;
const INT32 EVENTHELPER_UNREGISTER = 1;
const CHAR *EVENTHELPER_STRINGS[] = { "registerFor", "unregisterFor" };

//==============================================================================
/**
 *	Constructor
 */
CLuaEventHelper::CLuaEventHelper()
{
}

//==============================================================================
/**
 *	Destructor
 */
CLuaEventHelper::~CLuaEventHelper()
{
}

//==============================================================================
/**
 *	Fires an event into the presentation
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaEventHelper::FireEvent(lua_State *inLuaState)
{
    const INT32 ARG_EVENT = 1;
    const INT32 ARG_TARGET = 2;
    const INT32 ARG_ARG1 = 3;
    const INT32 ARG_ARG2 = 4;

    INT32 theTop = lua_gettop(inLuaState);

    const CHAR *theEvent = luaL_checkstring(inLuaState, ARG_EVENT);

    luaL_checktype(inLuaState, ARG_TARGET, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_TARGET));

    UINT8 theArg1Type = 0;
    UINT8 theArg2Type = 0;
    UVariant theArg1;
    UVariant theArg2;
    qt3ds::foundation::IStringTable &theStrTable(
        theElement->GetBelongedPresentation()->GetStringTable());
    qt3ds::runtime::IApplication &theApplication(
        theElement->GetBelongedPresentation()->GetApplication());

    if (theTop == 3)
        theArg1Type =
            CLuaHelper::GetArgument(inLuaState, ARG_ARG1, theArg1, theStrTable, theApplication);
    else if (theTop == 4) {
        theArg1Type =
            CLuaHelper::GetArgument(inLuaState, ARG_ARG1, theArg1, theStrTable, theApplication);
        theArg2Type =
            CLuaHelper::GetArgument(inLuaState, ARG_ARG2, theArg2, theStrTable, theApplication);
    }

    IPresentation *thePresentation = theElement->GetBelongedPresentation();
    thePresentation->FireEvent(CHash::HashEventCommand(theEvent), theElement,
                               theArg1Type == 0 ? 0 : &theArg1, theArg2Type == 0 ? 0 : &theArg2,
                               static_cast<EAttributeType>(theArg1Type),
                               static_cast<EAttributeType>(theArg2Type));

    lua_settop(inLuaState, 0);
    return 0;
}

//==============================================================================
/**
 *	Register function to receive a callback when an Event occurs
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the return boolean result of the call is pushed on the stack.
 *				true if sucessful, false if otherwise
 */
int CLuaEventHelper::RegisterForEvent(lua_State *inLuaState)
{
    EventRegistration(inLuaState, EVENTHELPER_REGISTER);
    return 1;
}

//==============================================================================
/**
 *	UnRegister function to receive a callback when an Event occurs
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the return boolean result of the call is pushed on the stack.
 *				true if sucessful, false if otherwise
 */
int CLuaEventHelper::UnRegisterForEvent(lua_State *inLuaState)
{
    EventRegistration(inLuaState, EVENTHELPER_UNREGISTER);
    return 1;
}

#if 0
//==============================================================================
/**
 *	Register function to receive a callback when a change on the element occurs
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the return boolean result of the call is pushed on the stack.
 *				true if sucessful, false if otherwise
 */
int CLuaEventHelper::RegisterForChange( lua_State* inLuaState )
{
	ChangeRegistration( inLuaState, EVENTHELPER_REGISTER );
	return 1;
}

//==============================================================================
/**
 *	UnRegister function to receive a callback when a change on the element occurs
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the return boolean result of the call is pushed on the stack.
 *				true if sucessful, false if otherwise
 */
int CLuaEventHelper::UnRegisterForChange( lua_State* inLuaState )
{
	ChangeRegistration( inLuaState, EVENTHELPER_UNREGISTER );
	return 1;
}

#endif

//==============================================================================
/**
 *	The callback function for RegisterForEvent
 *	@param inContextData		the associated data when the event is registered (
 *SCallbackData )
 *	@param ioEvent				the event that was fired
 */
void CLuaEventHelper::EventCallback(void *inContextData, SEventCommand &ioEvent)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_EVENTCALLBACK);
    CLuaEngine::SCallbackData &theData =
        *reinterpret_cast<CLuaEngine::SCallbackData *>(inContextData);
    if (theData.m_SelfElement && !theData.m_SelfElement->GetActive())
        return;

    INT32 theArgumentCount = 0;
    lua_State *theLuaState = theData.m_LuaState;

    INT32 theTop = lua_gettop(theLuaState);
    lua_pushcfunction(theLuaState, ErrorChannel); // Top + 1 = error handler
    lua_rawgeti(theLuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // Top + 2 = BehaviorsT
    lua_rawgeti(theLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS); // Top + 3 = CallbackT
    lua_rawgeti(theLuaState, -1, theData.m_FunctionReference); // Top + 4 = Callback function

    if (!theData.m_Global) {
        lua_pushlstring(theLuaState, KEY_SELF, LENGTH_KEY_SELF); // Top + 5 = "self"
        lua_rawgeti(theLuaState, theTop + 2, theData.m_ScriptIndex); // Top + 6 = ScriptT
        lua_rawset(theLuaState, LUA_GLOBALSINDEX); // [size = Top + 4]

        lua_rawgeti(theLuaState, theTop + 2, theData.m_ScriptIndex); // Top + 5 = ScriptT
        ++theArgumentCount;
    } else {
        lua_pushnil(theLuaState);
        ++theArgumentCount;
    }

    lua_pushlightuserdata(theLuaState, ioEvent.m_Target); // 6 or 5 = element ref
    ++theArgumentCount;

    qt3ds::foundation::IStringTable &theStrTable(
        ioEvent.m_Target->GetBelongedPresentation()->GetStringTable());
    qt3ds::runtime::IApplication &theApplication(
        ioEvent.m_Target->GetBelongedPresentation()->GetApplication());

    theArgumentCount += CLuaHelper::PushArgument(theLuaState, ioEvent.m_Arg1Type, ioEvent.m_Arg1,
                                                 theStrTable, theApplication);
    theArgumentCount += CLuaHelper::PushArgument(theLuaState, ioEvent.m_Arg2Type, ioEvent.m_Arg2,
                                                 theStrTable, theApplication);

    lua_pcall(theLuaState, theArgumentCount, 0, theTop + 1); // [size = 3]

    // Remove whatever stuff on the stack before exiting
    lua_settop(theLuaState, theTop);
}

//==============================================================================
/**
 *	The callback function for RegisterForChange
 *	@param inAttributeKey		the attribute that was changed
 *	@param inOldValue			the value before the attribute was changed
 *	@param inNewValue			the value after the attribute was changed
 *	@param inContextData		the associated data when the element is registered (
 *SCallbackData )
 */
void CLuaEventHelper::ChangeCallback(const SAttributeKey &inAttributeKey, UVariant inOldValue,
                                     UVariant inNewValue, void *inContextData)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_CHANGECALLBACK);
    CLuaEngine::SCallbackData &theData =
        *reinterpret_cast<CLuaEngine::SCallbackData *>(inContextData);

    if (theData.m_SelfElement && !theData.m_SelfElement->GetActive())
        return;

    // Added to check for property change
    if (theData.m_EventAttributeHash && theData.m_EventAttributeHash != inAttributeKey.m_Hash)
        return;

    if (theData.m_TargetElement == NULL)
        return;

    qt3ds::foundation::IStringTable &theStrTable(
        theData.m_TargetElement->GetBelongedPresentation()->GetStringTable());

    INT32 theArgumentCount = 0;
    lua_State *theLuaState = theData.m_LuaState;

    INT32 theTop = lua_gettop(theLuaState);
    lua_pushcfunction(theLuaState, ErrorChannel); // Top + 1 = error handler
    lua_rawgeti(theLuaState, LUA_REGISTRYINDEX, KEY_BEHAVIORS); // Top + 2 = BehaviorsT
    lua_rawgeti(theLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS); // Top + 3 = CallbackT
    lua_rawgeti(theLuaState, -1, theData.m_FunctionReference); // Top + 4 = Callback function

    if (!theData.m_Global) {
        lua_pushlstring(theLuaState, KEY_SELF, LENGTH_KEY_SELF); // Top + 5 = "self"
        lua_rawgeti(theLuaState, theTop + 2, theData.m_ScriptIndex); // Top + 6 = ScriptT
        lua_rawset(theLuaState, LUA_GLOBALSINDEX); // [size = Top + 4]

        lua_rawgeti(theLuaState, theTop + 2, theData.m_ScriptIndex); // Top + 5 = ScriptT
        ++theArgumentCount;
    } else {
        lua_pushnil(theLuaState);
        ++theArgumentCount;
    }

    lua_pushlightuserdata(theLuaState, theData.m_TargetElement); // Top + 6 or 5 = element ref
    lua_pushinteger(theLuaState,
                    static_cast<INT32>(inAttributeKey.m_Hash)); // Top + 7 or 6 = Attribute hash

    switch (inAttributeKey.m_Type) {
    case ATTRIBUTETYPE_INT32:
    case ATTRIBUTETYPE_HASH:
        lua_pushinteger(theLuaState, inOldValue.m_INT32); // Top + 8 or 7 = old value
        lua_pushinteger(theLuaState, inNewValue.m_INT32); // Top + 9 or 8 = new value
        break;

    case ATTRIBUTETYPE_FLOAT:
        lua_pushnumber(theLuaState, inOldValue.m_FLOAT); // Top + 8 or 7 = old value
        lua_pushnumber(theLuaState, inNewValue.m_FLOAT); // Top + 9 or 8 = new value
        break;

    case ATTRIBUTETYPE_BOOL:
        lua_pushboolean(theLuaState, inOldValue.m_INT32 != 0); // Top + 8 or 7 = old value
        lua_pushboolean(theLuaState, inNewValue.m_INT32 != 0); // Top + 9 or 8 = new value
        break;

    case ATTRIBUTETYPE_STRING:
        lua_pushstring(theLuaState, theStrTable.HandleToStr(
                                        inOldValue.m_StringHandle)); // Top + 8 or 7 = old value
        lua_pushstring(theLuaState, theStrTable.HandleToStr(
                                        inNewValue.m_StringHandle)); // Top + 9 or 8 = new value
        break;

    case ATTRIBUTETYPE_POINTER:
        lua_pushlightuserdata(theLuaState, inOldValue.m_VoidPointer); // Top + 8 or 7 = old value
        lua_pushlightuserdata(theLuaState, inNewValue.m_VoidPointer); // Top + 9 or 8 = new value
        break;

    default:
        qCritical(qt3ds::INVALID_OPERATION)
                << "Callback function received an attribute of unknown type: "
                << inAttributeKey.m_Type;
        return;
    }

    theArgumentCount += 4;
    lua_pcall(theLuaState, theArgumentCount, 0, theTop + 1);

    // Remove whatever stuff on the stack before exiting
    lua_settop(theLuaState, theTop);
}

//==============================================================================
/**
 *	Finds the Data in the data list
 *	Note: The top of the stack must be ScriptT
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inDataList		the data list to look for the data
 *	@param inCriteria		the data to look for
 *	@param inFunctionIndex	the index to where the function is on the stack
 *	@return the index of the data in the list, -1 if the data is not found
 */
INT32 CLuaEventHelper::FindCallbackData(lua_State *inLuaState,
                                        CLuaEngine::TCallbackDataList *inDataList,
                                        CLuaEngine::SCallbackData *inCriteria,
                                        const INT32 inFunctionIndex)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_FINDCALLBACKDATA);

    INT32 theTop = lua_gettop(inLuaState);
    lua_rawgeti(inLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS); // theTop + 1 = CallbackT

    INT32 theIndex = -1;
    for (INT32 theCounter = 0; theCounter < inDataList->GetCount(); ++theCounter) {
        CLuaEngine::SCallbackData *theListData = (*inDataList)[theCounter];

        if (inCriteria->m_TargetElement == theListData->m_TargetElement
            && inCriteria->m_EventAttributeHash == theListData->m_EventAttributeHash
            && inCriteria->m_ScriptIndex == theListData->m_ScriptIndex
            && inCriteria->m_Global == theListData->m_Global) {
            // Perform function equality check
            // Get the callback function from the callback table
            lua_rawgeti(inLuaState, theTop + 1, theListData->m_FunctionReference);

            // compare the function contained in the listData with the incoming function on the
            // stack
            if (lua_equal(inLuaState, -1, inFunctionIndex)) {
                theIndex = theCounter;
                break;
            }
        }
    }

    lua_settop(inLuaState, theTop);

    return theIndex;
}

//==============================================================================
/**
 *	Register or unregister a callback on an event
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inRegister		0 to register the callback, 1 to unregister
 */
void CLuaEventHelper::EventRegistration(lua_State *inLuaState, const INT32 inRegister)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_EVENTREGISTRATION);
    const INT32 ARG_CALLBACK_FUNCTION = 3;
    const INT32 ARG_EVENT_NAME = 1;
    const INT32 ARG_TARGET_ELEMENT = 2;
    const INT32 ARG_SELF_ELEMENT = 4; // optional argument, if not supplied, use the global self

    const CHAR *theEventName = luaL_checkstring(inLuaState, ARG_EVENT_NAME);
    TEventCommandHash theEventHash = CHash::HashEventCommand(theEventName);

    if (inRegister == EVENTHELPER_REGISTER) {
        if ((theEventHash == HASH_ONDEACTIVATE) || (theEventHash == HASH_ONINITIALIZE)
            || (theEventHash == HASH_ONACTIVATE) || (theEventHash == HASH_ONUPDATE)) {
            // the "output" function
            lua_getglobal(inLuaState, "output");
            lua_pushfstring(inLuaState,
                            "ERROR: %sEvent: Auto event [%s] registration is not allowed.",
                            EVENTHELPER_STRINGS[inRegister], theEventName);
            lua_call(inLuaState, 1, 0);
            lua_pushboolean(inLuaState, false);
            return;
        }
    }

    INT32 theSelfIndex = (lua_gettop(inLuaState) == ARG_SELF_ELEMENT) ? ARG_SELF_ELEMENT : 0;

    luaL_checktype(inLuaState, ARG_CALLBACK_FUNCTION, LUA_TFUNCTION);
    luaL_checktype(inLuaState, ARG_TARGET_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theTargetElement =
        reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_TARGET_ELEMENT));

    lua_rawgeti(inLuaState, LUA_REGISTRYINDEX, KEY_EVENTDATA_LIST);
    CLuaEngine::TCallbackDataList *theEventDataList =
        reinterpret_cast<CLuaEngine::TCallbackDataList *>(lua_touserdata(inLuaState, -1));
    lua_pop(inLuaState, 1);

    // Create the callback context data
    CLuaEngine::SCallbackData *theData =
        Q3DStudio_allocate_desc(CLuaEngine::SCallbackData, 1, "Lua:Callback");
    theData->m_EventAttributeHash = theEventHash;
    theData->m_TargetElement = theTargetElement;
    theData->m_LuaState = inLuaState;
    theData->m_SelfElement = GetSelfElement(inLuaState, theSelfIndex);

    INT32 theError = 0;
    IPresentation *thePresentation = theTargetElement->GetBelongedPresentation();

    // Check if the function specified is a registered "self" function
    if (IsFunctionInSelfTable(inLuaState, ARG_CALLBACK_FUNCTION, theSelfIndex)) {
        // The function is a registered "self" function, so get the script index
        PushSelfOnTop(inLuaState, theSelfIndex);

        lua_pushlstring(inLuaState, KEY_SCRIPT_INDEX,
                        LENGTH_KEY_SCRIPT_INDEX); // theTop + 2 = "__scriptIndex"
        lua_rawget(inLuaState, -2); // theTop + 2 = script index

        if (lua_isnumber(inLuaState, -1)) {
            theData->m_ScriptIndex = static_cast<INT16>(lua_tointeger(inLuaState, -1));
            theData->m_Global = 0;
        } else
            theError = 2;
    } else {
        theData->m_ScriptIndex = 0;
        theData->m_Global = 1;
    }

    if (!theError) // if there's no error
    {
        // Find the callback data
        INT32 theIndex =
            FindCallbackData(inLuaState, theEventDataList, theData, ARG_CALLBACK_FUNCTION);

        if (inRegister == EVENTHELPER_REGISTER) // Register
        {
            if (theIndex < 0) // register when not found
            {
                // create a reference to this function which will be stored in the callback table
                lua_rawgeti(inLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS); // theTop + 2 = CallbackT
                lua_pushvalue(inLuaState, ARG_CALLBACK_FUNCTION); // theTop + 3 = Callback function
                theData->m_FunctionReference = luaL_ref(inLuaState, -2); // [size = theTop + 2]

                thePresentation->RegisterEventCallback(theTargetElement, theEventHash,
                                                       EventCallback, theData);
                theEventDataList->Push(theData);
            } else // an event has already been registered, so unregister it and register again to
                   // maintain the callback order
            {
                CLuaEngine::SCallbackData *theRegisteredData = (*theEventDataList)[theIndex];
                thePresentation->UnregisterEventCallback(theTargetElement, theEventHash,
                                                         EventCallback, theRegisteredData);
                thePresentation->RegisterEventCallback(theTargetElement, theEventHash,
                                                       EventCallback, theRegisteredData);
            }
        } else // Unregister
        {
            if (theIndex >= 0) // unregister when found
            {
                CLuaEngine::SCallbackData *theRegisteredData = (*theEventDataList)[theIndex];

                thePresentation->UnregisterEventCallback(theTargetElement, theEventHash,
                                                         EventCallback, theRegisteredData);
                theEventDataList->Remove(theIndex);

                // Unref and remove the function stored in the callback table
                lua_rawgeti(inLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS); // theTop + 2 = CallbackT
                luaL_unref(inLuaState, -1, theRegisteredData->m_FunctionReference);

                CLuaEngine::FreeCallbackData(theRegisteredData);
                CLuaEngine::FreeCallbackData(theData);
            } else
                theError = 3;
        }
    }

    if (theError) {
        CLuaEngine::FreeCallbackData(theData);

        // The "output" function
        lua_getglobal(inLuaState, "output");
        switch (theError) {
        // case 1:
        //	lua_pushfstring( inLuaState, "ERROR: %sEvent: Event, callback function and target
        //element has been registered", EVENTHELPER_STRINGS[inRegister] );
        //	break;

        case 2:
            lua_pushfstring(inLuaState, "ERROR: %sEvent: Script index not found",
                            EVENTHELPER_STRINGS[inRegister]);
            break;

        case 3:
            lua_pushfstring(inLuaState,
                            "WARNING: %sEvent: Attempt to unregister an unregistered event [%s]",
                            EVENTHELPER_STRINGS[inRegister], theEventName);
            break;

        default: // no errors
            break;
        }

        lua_call(inLuaState, 1, 0);
    }

    lua_pushboolean(inLuaState, theError == 0);
}

//==============================================================================
/**
 *	Register or unregister a callback on element attributes changes
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inRegister		0 to register the callback, 1 to unregister
 */
/*
void CLuaEventHelper::ChangeRegistration( lua_State* inLuaState, const INT32 inRegister )
{
        PerfLogLuaEvent1( DATALOGGER_LUA_CHANGEREGISTRATION );
        const INT32 ARG_ATTRIBUTE			= 2;
        const INT32 ARG_TARGET_ELEMENT		= 1;
        INT32 ARG_CALLBACK_FUNCTION			= 2;

        luaL_checktype( inLuaState, ARG_TARGET_ELEMENT, LUA_TLIGHTUSERDATA );

        const CHAR* theAttributeString = NULL;
        INT32 theSelfIndex = 0;
        INT32 theTop = lua_gettop( inLuaState );
        if ( theTop >= 3 )
        {
                // the 2nd parameter, which is a string attribute is optional, so check if it is
supplied
                if ( lua_type( inLuaState, ARG_ATTRIBUTE ) == LUA_TSTRING )
                {
                        ARG_CALLBACK_FUNCTION = 3;
                        theAttributeString = luaL_checkstring( inLuaState, ARG_ATTRIBUTE );
                }
                // the last argument for "self" is optional, and if not supplied, use the global
self
                if ( lua_type( inLuaState, theTop ) == LUA_TTABLE )
                        theSelfIndex = theTop;
        }

        luaL_checktype( inLuaState, ARG_CALLBACK_FUNCTION, LUA_TFUNCTION );

        TElement* theTargetElement = reinterpret_cast<TElement*>( lua_touserdata( inLuaState,
ARG_TARGET_ELEMENT ) );

        lua_rawgeti( inLuaState, LUA_REGISTRYINDEX, KEY_CHANGEDATA_LIST );
        CLuaEngine::TCallbackDataList* theChangeDataList =
reinterpret_cast<CLuaEngine::TCallbackDataList*>( lua_touserdata( inLuaState, -1 ) );
        lua_pop( inLuaState, 1 );

        CLuaEngine::SCallbackData* theData = Q3DStudio_allocate_desc( CLuaEngine::SCallbackData, 1,
"Lua:Desc" );

        theData->m_TargetElement = theTargetElement;
        theData->m_LuaState = inLuaState;
        if ( theAttributeString )
                theData->m_EventAttributeHash = CHash::HashAttribute( theAttributeString );
        else
                theData->m_EventAttributeHash = 0;
        theData->m_SelfElement = GetSelfElement( inLuaState, theSelfIndex );

        INT32 theError = 0;
        CPresentation* thePresentation = theTargetElement->GetBelongedPresentation();

        // Check if the function specified is a registered "self" function
        if ( IsFunctionInSelfTable( inLuaState, ARG_CALLBACK_FUNCTION, theSelfIndex ) )
        {
                // The function is a registered "self" function, so get the script index
                PushSelfOnTop( inLuaState, theSelfIndex );

                lua_pushlstring( inLuaState, KEY_SCRIPT_INDEX, LENGTH_KEY_SCRIPT_INDEX );	//
theTop + 2 = "__scriptIndex"
                lua_rawget( inLuaState, -2 );
// theTop + 2 = script index

                if ( lua_isnumber( inLuaState, -1 ) )
                {
                        theData->m_ScriptIndex = static_cast<INT16>( lua_tointeger( inLuaState, -1 )
);
                        theData->m_Global = 0;
                }
                else
                        theError = 2;
        }
        else
        {
                theData->m_ScriptIndex = 0;
                theData->m_Global = 1;
        }

        if ( !theError ) // if there's no error
        {
                // Find the callback data
                INT32 theIndex = FindCallbackData( inLuaState, theChangeDataList, theData,
ARG_CALLBACK_FUNCTION );


                if ( inRegister == EVENTHELPER_REGISTER ) // Register
                {
                        if ( theIndex < 0 ) // register when not found
                        {
                                // create a reference to this function which will be stored in the
callback table
                                lua_rawgeti( inLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS );
// theTop + 2 = CallbackT
                                lua_pushvalue( inLuaState, ARG_CALLBACK_FUNCTION );
// theTop + 3 = Callback function
                                theData->m_FunctionReference = luaL_ref( inLuaState, -2 );
// [size = theTop + 2]

                                thePresentation->GetElementManager(
).RegisterAttributeChangeCallback( theTargetElement, ChangeCallback, theData );
                                theChangeDataList->Push( theData );
                        }
                        else // a change has already been registered, so unregister it and register
again to maintain the callback order
                        {
                                CLuaEngine::SCallbackData* theRegisteredData = ( *theChangeDataList
)[theIndex];
                                thePresentation->GetElementManager(
).UnregisterAttributeChangeCallback( theTargetElement, ChangeCallback, theRegisteredData );
                                thePresentation->GetElementManager(
).RegisterAttributeChangeCallback( theTargetElement, ChangeCallback, theRegisteredData );
                        }
                }
                else // Unregister
                {
                        if ( theIndex >= 0 ) // unregister when found
                        {
                                CLuaEngine::SCallbackData* theRegisteredData = ( *theChangeDataList
)[theIndex];

                                thePresentation->GetElementManager(
).UnregisterAttributeChangeCallback( theTargetElement, ChangeCallback, theRegisteredData );
                                theChangeDataList->Remove( theIndex );

                                // Unref and remove the function stored in the callback table
                                lua_rawgeti( inLuaState, LUA_REGISTRYINDEX, KEY_CALLBACKS );
// theTop + 2 = CallbackT
                                luaL_unref( inLuaState, -1, theRegisteredData->m_FunctionReference
);

                                CLuaEngine::FreeCallbackData( theRegisteredData );
                                CLuaEngine::FreeCallbackData( theData );
                        }
                        else
                                theError = 3;
                }
        }

        if ( theError )
        {
                CLuaEngine::FreeCallbackData( theData );

                // the "output" function
                lua_getglobal( inLuaState, "output" );
                switch ( theError )
                {
                        //case 1:
                        //	lua_pushfstring( inLuaState, "ERROR: %sChange: Callback function
and target element has been registered", EVENTHELPER_STRINGS[inRegister] );
                        //	break;

                        case 2:
                                lua_pushfstring( inLuaState, "ERROR: %sChange: Script index not
found", EVENTHELPER_STRINGS[inRegister] );
                                break;

                        case 3:
                                lua_pushfstring( inLuaState, "WARNING: %sChange: Attempt to
unregister an unregistered target element [%p]", EVENTHELPER_STRINGS[inRegister], theTargetElement
);
                                break;

                        default:
                                break;
                }

                lua_call( inLuaState, 1, 0 );
        }

        lua_pushboolean( inLuaState, theError == 0 );
}*/

//==============================================================================
/**
 *	Checks if the function at inFunctionIndex is in the self table which is on the top of the
 *stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inFunctionIndex	index pointing to the function object in the stack
 *	@param inSelfIndex		index pointing to the self object in the stack, 0 to use the
 *global self
 *	@return true if the function is in the self table, false if otherwise
 */
BOOL CLuaEventHelper::IsFunctionInSelfTable(lua_State *inLuaState, const INT32 inFunctionIndex,
                                            const INT32 inSelfIndex)
{
    INT32 theTop = lua_gettop(inLuaState);

    // theTop = "self"
    PushSelfOnTop(inLuaState, inSelfIndex);

    BOOL theResult = false;

    // Push a nil to start the loop
    lua_pushnil(inLuaState); // theTop + 2 = nil

    // while there's still item's in the table
    while (lua_next(inLuaState, -2)) // theTop + 2 = key, theTop + 3 = value
    {
        if (lua_equal(inLuaState, -1, inFunctionIndex)) {
            theResult = true;
            break;
        } else
            lua_pop(inLuaState,
                    1); // leave the key to seed the next round	// [size = theTop + 2]
    }

    lua_settop(inLuaState, theTop);

    return theResult;
}

//==============================================================================
/**
 *	Push "self" as specified by inSelfIndex onto the top of stack. The caller is responsible for
 *restoring the stack.
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inSelfIndex		index pointing to the self object in the stack, 0 to use the
 *global self
 *	@return true if the function is in the self table, false if otherwise
 */
void CLuaEventHelper::PushSelfOnTop(lua_State *inLuaState, const INT32 inSelfIndex)
{
    if (inSelfIndex == 0) {
        lua_pushlstring(inLuaState, KEY_SELF, LENGTH_KEY_SELF);
        lua_rawget(inLuaState, LUA_ENVIRONINDEX);
    } else
        lua_settop(inLuaState, inSelfIndex);
}

//==============================================================================
/**
 *	Gets the behavior element ( self.element ) from the current ScriptT
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inSelfIndex		index pointing to the self object in the stack, 0 to use the
 *global self
 *	@return the behavior element pointer
 */
TElement *CLuaEventHelper::GetSelfElement(lua_State *inLuaState, const INT32 inSelfIndex)
{
    INT32 theTop = lua_gettop(inLuaState);

    if (inSelfIndex == 0) {
        lua_pushlstring(inLuaState, KEY_SELF, LENGTH_KEY_SELF); // theTop + 1 = "self"
        lua_rawget(inLuaState, LUA_GLOBALSINDEX); // theTop + 1 = ScriptT
    } else
        lua_settop(inLuaState, inSelfIndex);

    lua_pushlstring(inLuaState, KEY_ELEMENT, LENGTH_KEY_ELEMENT); // theTop + 2 = "element"
    lua_rawget(inLuaState, -2); // theTop + 2 = self.element ( behavior )

    TElement *theSelfElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, -1));
    lua_settop(inLuaState, theTop);
    return theSelfElement;
}

} // namespace Q3DStudio
