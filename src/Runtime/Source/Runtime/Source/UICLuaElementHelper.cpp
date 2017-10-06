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
#include "UICLuaElementHelper.h"
#include "UICLuaCommandHelper.h"
#include "UICLuaEngine.h"
#include "UICPresentation.h"
#include "UICAttributeHashes.h"
#include "UICApplication.h"
#include "UICCommandEventTypes.h"
#include "UICRuntimeFactory.h"
#include "UICElementSystem.h"

#include "UICDataLogger.h"

using namespace qt3ds;

//==============================================================================
// Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const char PRESENTATION_DELIMITER = ':';
const char NODE_DELIMITER = '.';
const TStringHash RESERVED_THIS = CHash::HashString("this");
const TStringHash RESERVED_PARENT = CHash::HashString("parent");
const TStringHash RESERVED_SCENE = CHash::HashString("Scene");
const TAttributeHash SPECIAL_ATTRIBUTE_EXPLICIT_ACTIVE = CHash::HashAttribute("active");
const TAttributeHash SPECIAL_ATTRIBUTE_GLOBAL_ACTIVE = CHash::HashAttribute("globalactive");
const char *STRINGS_ELEMENT_TYPE[] = { "unknown",  "node",     "camera",          "light",
                                       "text",     "material", "texture",         "component",
                                       "behavior", "path",     "pathanchorpoint", "subpath" };

//==============================================================================
/**
 *	Constructor
 */
CLuaElementHelper::CLuaElementHelper()
{
}

//==============================================================================
/**
 *	Destructor
 */
CLuaElementHelper::~CLuaElementHelper()
{
}

TElement *CLuaElementHelper::GetElement(uic::runtime::IApplication &inApplication,
                                        IPresentation *inDefaultPresentation, const char *inPath,
                                        TElement *inStartElement)
{
    if (inPath == NULL || *inPath == 0)
        return NULL;
    const char *thePath(inPath);
    const char *theSubPath = NULL;
    IPresentation *thePresentation = NULL;
    size_t thePathLength = ::strlen(thePath) + 1;
    char *theToken =
        Q3DStudio_allocate_desc(CHAR, thePathLength, "Lua:TempPath"); // Temporary token storage
    // Try to get the specified presentation
    theSubPath = ::strchr(thePath, PRESENTATION_DELIMITER);
    TElement *theElement = inStartElement;
    if (theSubPath != NULL) {
        UINT32 theSubPathLength = static_cast<UINT32>(theSubPath - thePath);

        ::strncpy(theToken, thePath, theSubPathLength);
        theToken[theSubPathLength] = '\0';

        thePath = theSubPath + 1;

        const CHAR *thePresentationName = theToken;

        thePresentation = inApplication.GetPresentationById(thePresentationName);
    }
    if (thePresentation == NULL)
        thePresentation = inDefaultPresentation;

    // Return nil if the inStartElement is not in the specified presentation
    if (theElement != NULL
        && (theSubPath == NULL && theElement->GetBelongedPresentation() != thePresentation)) {
        thePresentation = theElement->GetBelongedPresentation();
    }

    if (thePresentation == NULL)
        return NULL;

    TStringHash theName;
    INT32 theParseCounter = 0;

    while (thePath != NULL && thePath[0] != '\0') {
        ++theParseCounter;

        // Do some strtok() work here
        theSubPath = ::strchr(thePath, NODE_DELIMITER);
        if (theSubPath) {
            UINT32 theSubPathLength = static_cast<UINT32>(theSubPath - thePath);
            Q3DStudio_ASSERT(theSubPathLength < thePathLength);

            ::strncpy(theToken, thePath, theSubPathLength);
            theToken[theSubPathLength] = '\0';

            thePath = theSubPath + 1;
        } else {
            ::strcpy(theToken, thePath);
            thePath = NULL;
        }

        // Hash the token and do some element searching
        theName = CHash::HashString(theToken);

        if (theName == RESERVED_PARENT) {
            if (theElement)
                theElement = theElement->GetParent();
        } else if (theName == RESERVED_THIS) {
            ;
        } else {
            if (theName == RESERVED_SCENE && theParseCounter == 1)
                theElement =
                    thePresentation->GetRoot(); // theElement is NULL, so using absolute path

            else if (theElement)
                theElement = theElement->FindChild(theName); // Using relative path
        }

        if (!theElement)
            thePath = NULL;
    } // while

    Q3DStudio_free(theToken, CHAR, thePathLength);
    return theElement;
}
//==============================================================================
/**
 *	Gets the ElementRef of the element specified in the path
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaElementHelper::GetElement(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_GETELEMENT);

    const INT32 ARG_PATH = 1;
    const INT32 ARG_START = 2;

    int theStackTop = lua_gettop(inLuaState);

    // Will exit function immediately once any of these luaL_check.. function fails
    const char *thePath = luaL_checkstring(inLuaState, ARG_PATH);
    TElement *theElement = NULL;
    CPresentation *thePresentation = CLuaEngine::GetCurrentPresentation(inLuaState);

    // Parse arguments
    if (theStackTop == 2) {
        luaL_checktype(inLuaState, ARG_START, LUA_TLIGHTUSERDATA);
        theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_START));
    }
    lua_getglobal(inLuaState, "UICApplication");
    uic::runtime::IApplication *theApp =
        static_cast<uic::runtime::IApplication *>(lua_touserdata(inLuaState, -1));

    if (theApp)
        theElement = GetElement(*theApp, thePresentation, thePath, theElement);

    if (theElement)
        lua_pushlightuserdata(inLuaState, theElement);
    else
        lua_pushnil(inLuaState);

    return 1;
}

//==============================================================================
/**
 *	Gets the element type
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaElementHelper::GetElementType(lua_State *inLuaState)
{
    const INT32 ARG_ELEMENT = 1;
    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    lua_pushstring(inLuaState, theElement->GetTypeDescription().m_TypeName.c_str());
    return 1;
}

//==============================================================================
/**
 *	Gets the attribute value of the specified element
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaElementHelper::GetAttribute(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_GETATTRIBUTE);
    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_ATTRIBUTE_NAME = 2;

    // will exit function immediately once any of these luaL_check.. function fails
    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    const char *attName = luaL_checkstring(inLuaState, ARG_ATTRIBUTE_NAME);
    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = CHash::HashAttribute(attName);

    IPresentation *thePresentation = theElement->GetBelongedPresentation();
    uic::runtime::IApplication &theApplication = thePresentation->GetApplication();

    if (theAttributeKey.m_Hash == SPECIAL_ATTRIBUTE_GLOBAL_ACTIVE)
        lua_pushboolean(inLuaState, theElement->GetActive());
    else if (theAttributeKey.m_Hash == SPECIAL_ATTRIBUTE_EXPLICIT_ACTIVE)
        lua_pushboolean(inLuaState, theElement->IsExplicitActive());
    else if (theAttributeKey.m_Hash == Q3DStudio::ATTRIBUTE_NAME) {
        lua_pushstring(inLuaState, theElement->m_Name.c_str());
    } else if (theAttributeKey.m_Hash == Q3DStudio::CHash::HashAttribute("path")) {
        lua_pushstring(inLuaState, theElement->m_Path.c_str());
    } else {
        Option<uic::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo =
            theElement->FindProperty(theAttributeKey.m_Hash);
        if (thePropertyInfo.hasValue()) {
            UVariant *theValuePtr = thePropertyInfo->second;
            switch (thePropertyInfo->first.m_Type) {
            case ATTRIBUTETYPE_INT32:
            case ATTRIBUTETYPE_HASH:
                lua_pushinteger(inLuaState, theValuePtr->m_INT32);
                break;

            case ATTRIBUTETYPE_FLOAT:
                lua_pushnumber(inLuaState, theValuePtr->m_FLOAT);
                break;

            case ATTRIBUTETYPE_BOOL:
                lua_pushboolean(inLuaState, theValuePtr->m_INT32 != 0);
                break;

            case ATTRIBUTETYPE_STRING:
                lua_pushstring(inLuaState, thePresentation->GetStringTable()
                                               .HandleToStr(theValuePtr->m_StringHandle)
                                               .c_str());
                break;

            case ATTRIBUTETYPE_POINTER:
                lua_pushlightuserdata(inLuaState, theValuePtr->m_VoidPointer);
                break;

            case ATTRIBUTETYPE_ELEMENTREF:
                lua_pushlightuserdata(
                    inLuaState, theApplication.GetElementByHandle(theValuePtr->m_ElementHandle));
                break;

            case ATTRIBUTETYPE_NONE:
            case ATTRIBUTETYPE_DATADRIVEN_PARENT:
            case ATTRIBUTETYPE_DATADRIVEN_CHILD:
            case ATTRIBUTETYPECOUNT:
            default:
                luaL_error(inLuaState, "getAttribute: Attribute has no type!");
                break;
            }
        } else
            luaL_error(inLuaState, "getAttribute: Attribute \"%s\" not found.",
                       lua_tostring(inLuaState, ARG_ATTRIBUTE_NAME));
    }

    return 1;
}

bool CLuaElementHelper::SetAttribute(lua_State *inLuaState, TElement *theElement,
                                     const char *theAttName, bool inDelay)
{
    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = CHash::HashAttribute(theAttName);

    // Early out for our single 'read only' attribute
    if (ATTRIBUTE_URI == theAttributeKey.m_Hash) {
        // we didn't push anything onto the lua stack
        return false;
    }

    const int ARG_VALUE = -1;

    if (theAttributeKey.m_Hash == SPECIAL_ATTRIBUTE_EXPLICIT_ACTIVE) {
        if (lua_type(inLuaState, ARG_VALUE) != LUA_TBOOLEAN) {
            qCCritical(INVALID_OPERATION, "setAttribute: expected boolean");
            return false;
        }
        theElement->SetFlag(ELEMENTFLAG_EXPLICITACTIVE,
                            (lua_toboolean(inLuaState, ARG_VALUE) ? true : false));
    } else {
        Option<uic::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo =
            theElement->FindProperty(theAttributeKey.m_Hash);
        if (thePropertyInfo.hasValue()) {
            UVariant theNewValue;
            EAttributeType theAttributeType = thePropertyInfo->first.m_Type;
            switch (theAttributeType) {
            case ATTRIBUTETYPE_INT32:
            case ATTRIBUTETYPE_HASH:
                theNewValue.m_INT32 = luaL_checkinteger(inLuaState, ARG_VALUE);
                break;

            case ATTRIBUTETYPE_FLOAT:
                theNewValue.m_FLOAT = static_cast<FLOAT>(luaL_checknumber(inLuaState, ARG_VALUE));
                break;

            case ATTRIBUTETYPE_BOOL:
                luaL_checktype(inLuaState, ARG_VALUE, LUA_TBOOLEAN);
                theNewValue.m_INT32 = lua_toboolean(inLuaState, ARG_VALUE) ? 1 : 0;
                break;

            case ATTRIBUTETYPE_STRING:
                theNewValue.m_StringHandle =
                    theElement->GetBelongedPresentation()->GetStringTable().GetHandle(
                        luaL_checkstring(inLuaState, ARG_VALUE));
                break;

            case ATTRIBUTETYPE_POINTER:
                if (lua_type(inLuaState, ARG_VALUE) != LUA_TLIGHTUSERDATA) {
                    qCCritical(INVALID_OPERATION, "setAttribute: expected light user data");
                    return false;
                }
                theNewValue.m_VoidPointer = lua_touserdata(inLuaState, ARG_VALUE);
                break;

            case ATTRIBUTETYPE_ELEMENTREF:
                qCCritical(INVALID_OPERATION, "setAttribute: ElementRef attributes are read only.");
                return false;
                break;

            case ATTRIBUTETYPE_NONE:
            case ATTRIBUTETYPE_DATADRIVEN_PARENT:
            case ATTRIBUTETYPE_DATADRIVEN_CHILD:
            case ATTRIBUTETYPECOUNT:
            default:
                qCCritical(INVALID_OPERATION, "setAttribute: Attribute has no type!");
                return false;
                break;
            }
            if (inDelay) {
                UVariant arg1;
                arg1.m_INT32 = theAttributeKey.m_Hash;
                theElement->GetBelongedPresentation()->FireCommand(
                    COMMAND_SETPROPERTY, theElement, &arg1, &theNewValue, ATTRIBUTETYPE_HASH,
                    theAttributeType);
            } else {
                theElement->SetAttribute(*thePropertyInfo, theNewValue);
            }

        } else {
            return false;
        }
    }
    return true;
}

//==============================================================================
/**
 *	Sets the attribute value of the specified element
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaElementHelper::SetAttribute(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_SETATTRIBUTE);
    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_ATTRIBUTE_NAME = 2;

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));
    const char *theAttName = luaL_checkstring(inLuaState, ARG_ATTRIBUTE_NAME);
    if (!SetAttribute(inLuaState, theElement, theAttName, false)) {
        luaL_error(inLuaState, "setAttribute: Attribute \"%s\" not found.", theAttName);
    }
    return 0;
}

//==============================================================================
/**
 *	Checks if element has a specific attribute
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaElementHelper::HasAttribute(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_HASATTRIBUTE);

    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_ATTRIBUTE_NAME = 2;

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = CHash::HashAttribute(luaL_checkstring(inLuaState, ARG_ATTRIBUTE_NAME));

    UVariant theValue;
    lua_pushboolean(inLuaState, theElement->GetAttribute(theAttributeKey.m_Hash, theValue));

    return 1;
}

//==============================================================================
/**
 *	Gets the children for the specified element
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the table containing all the children that was pushed on the stack
 */
int CLuaElementHelper::GetChildren(lua_State *inLuaState)
{
    const INT32 ARG_PARENT = 1;

    luaL_checktype(inLuaState, ARG_PARENT, LUA_TLIGHTUSERDATA);

    TElement *theParent = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_PARENT));

    INT32 theIndex = 0;
    lua_newtable(inLuaState); // Top = New Table
    int theTop = lua_gettop(inLuaState);
    for (TElement *theChild = theParent->m_Child; theChild; theChild = theChild->m_Sibling) {
        lua_pushlightuserdata(inLuaState, theChild); // Top + 1 = theChild
        lua_rawseti(inLuaState, theTop, ++theIndex);
    }
    return 1;
}

//==============================================================================
/**
 *	Gets the current slide number of the element. If element isn't a component, it
 *	walks up the tree to find one that is.
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 2 - the slide number and the slide name that was pushed on the stack
 */
int CLuaElementHelper::GetCurrentSlide(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_GETCURRENTSLIDE);

    luaL_checktype(inLuaState, 1, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, 1));

    IPresentation *thePresentation = theElement->GetBelongedPresentation();
    theElement = &theElement->GetComponentParent();
    int slideNum = thePresentation->GetComponentManager().GetCurrentSlide(theElement);
    const char *slideName = thePresentation->GetComponentManager().GetCurrentSlideName(theElement);
    lua_pushinteger(inLuaState, slideNum);
    lua_pushstring(inLuaState, slideName);

    return 2;
}

//==============================================================================
/**
 *	Gets the current time of the element. If element isn't a component, it walks
 *	up the tree to find one that is.
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 3 - the current time of the element in seconds, the pause state and
 *				the pingpong direction that was pushed on the stack
 */
int CLuaElementHelper::GetTime(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_GETTIME);
    luaL_checktype(inLuaState, 1, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, 1));

    lua_pushnumber(inLuaState, theElement->GetInnerTime() / 1000.0f);

    TComponent &theComponent = static_cast<TComponent &>(theElement->GetComponentParent());

    lua_pushboolean(inLuaState, theComponent.GetPaused());
    lua_pushboolean(inLuaState, theComponent.GetPlayBackDirection());

    return 3;
}

} // namespace Q3DStudio
