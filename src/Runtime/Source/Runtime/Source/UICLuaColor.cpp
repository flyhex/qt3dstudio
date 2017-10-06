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
#include "UICLuaColor.h"
#include "UICColor.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const CHAR *STRING_COLOR_TYPE = "ColorType";
const CHAR *STRING_COLOR_CLASS = "Color";
const CHAR *STRING_ERROR_COLOR_VALUE_EXPECTED = "'Color' value (0 - 255) expected";
const CHAR *STRING_ERROR_COLOR_EXPECTED = "'Color' expected.";

extern const CHAR *STRING_ERROR_WRONG_NUMBER_ARUGMENT;
extern const CHAR *STRING_ERROR_ACCESS_NON_MEMBER;

//==============================================================================
/**
 *	Registers the type "Color" in Lua's global name space
 *	@param inLuaState		the Lua state, required for interaction with lua
 */
void CLuaColor::RegisterLibrary(lua_State *inLuaState)
{
    static const luaL_Reg theColorLibrary[] = { { "new", Create }, { NULL, NULL } };

    static const luaL_Reg theColorMethods[] = {
        { "setColor", SetColor }, { "set", Set }, { "equals", Equals }, { NULL, NULL }
    };

    INT32 theTop = lua_gettop(inLuaState);

    luaL_newmetatable(inLuaState, STRING_COLOR_TYPE); // Create a new metatable for this Color type
    luaL_register(inLuaState, NULL,
                  theColorMethods); // Registers the additional methods to this metatable

    lua_pushstring(inLuaState,
                   "__index"); // The GetValue to check if it's 'x', 'y' or 'z' when indexed
    lua_pushcfunction(inLuaState, GetValue);
    lua_settable(inLuaState, 1);

    lua_pushstring(inLuaState,
                   "__newindex"); // The SetValue to check if it's 'x', 'y' or 'z' when new indexed
    lua_pushcfunction(inLuaState, SetValue);
    lua_settable(inLuaState, 1);

    lua_pushstring(inLuaState, "__tostring"); // Formatting
    lua_pushcfunction(inLuaState, ToString);
    lua_settable(inLuaState, 1);

    lua_pushstring(inLuaState, "__eq"); // Equality operator '=='
    lua_pushcfunction(inLuaState, Equals);
    lua_settable(inLuaState, 1);

    luaL_register(inLuaState, STRING_COLOR_CLASS,
                  theColorLibrary); // register the 'Color' class type with one method 'new'

    lua_settop(inLuaState, theTop);
}

//==============================================================================
/**
 *	Creates a new Lua Color object and places it on the top of the Lua stack
 *	3 ways to create the new color:
 *	1) Color.new( ) 					-- a new color with r, g, and b all
 *initialized to 255 and a initialized to 0
 *	2) Color.new( inColor ) 			-- a new color with r, g, b and a all initialized
 *to the values of inColor
 *	3) Color.new( inR, inG, inB, inA )	-- a new color with r, g, b and a all initialized to
 *the values of inR, inG, inB and inA
 *
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::Create(lua_State *inLuaState)
{
    INT32 theTop = lua_gettop(inLuaState);

    CColor &theColor = CreateColor(inLuaState);

    INT32 theOffset = lua_istable(inLuaState, 1) ? 1 : 0;

    if (theTop == 0 + theOffset) {
        theColor.m_Red = 255;
        theColor.m_Green = 255;
        theColor.m_Blue = 255;
        theColor.m_Alpha = 255;
    } else if (theTop == 1 + theOffset) {
        CColor &theSource = CheckColor(inLuaState, 1 + theOffset);
        theColor = theSource;
    } else if (theTop == 4 + theOffset) {
        theColor.m_Red = GetColorValue(inLuaState, 1 + theOffset);
        theColor.m_Green = GetColorValue(inLuaState, 2 + theOffset);
        theColor.m_Blue = GetColorValue(inLuaState, 3 + theOffset);
        theColor.m_Alpha = GetColorValue(inLuaState, 4 + theOffset);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    Q3DStudio_ASSERT(lua_gettop(inLuaState) == theTop + 1);
    return 1;
}

//==============================================================================
/**
 *	Gets the value of red or green or blue or alpha and places it on the top of the lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::GetValue(lua_State *inLuaState)
{
    CColor &theColor = CheckColor(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);

    if (::strcmp(theParam, "r") == 0)
        lua_pushinteger(inLuaState, theColor.m_Red);
    else if (::strcmp(theParam, "g") == 0)
        lua_pushinteger(inLuaState, theColor.m_Green);
    else if (::strcmp(theParam, "b") == 0)
        lua_pushinteger(inLuaState, theColor.m_Blue);
    else if (::strcmp(theParam, "a") == 0)
        lua_pushinteger(inLuaState, theColor.m_Alpha);
    else {
        lua_getmetatable(inLuaState, 1); // gets the metatable of this user type
        lua_pushvalue(inLuaState, 2); // push the key onto the top of stack
        lua_rawget(inLuaState, -2); // get the associated function, if any

        if (lua_isnil(inLuaState, -1))
            luaL_error(inLuaState, STRING_ERROR_ACCESS_NON_MEMBER);
    }
    return 1;
}

//==============================================================================
/**
 *	Sets the value of red or green or blue or alpha
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::SetValue(lua_State *inLuaState)
{
    CColor &theColor = CheckColor(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);

    if (::strcmp(theParam, "r") == 0)
        theColor.m_Red = GetColorValue(inLuaState, 3);
    else if (::strcmp(theParam, "g") == 0)
        theColor.m_Green = GetColorValue(inLuaState, 3);
    else if (::strcmp(theParam, "b") == 0)
        theColor.m_Blue = GetColorValue(inLuaState, 3);
    else if (::strcmp(theParam, "a") == 0)
        theColor.m_Alpha = GetColorValue(inLuaState, 3);
    else
        luaL_error(inLuaState, STRING_ERROR_ACCESS_NON_MEMBER);

    return 0;
}

//==============================================================================
/**
 *	Set the vector's value to the value of inColor which is on the top of the
 *	Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::SetColor(lua_State *inLuaState)
{
    CColor &theDestination = CheckColor(inLuaState);
    CColor &theSource = CheckColor(inLuaState, 2);

    theDestination = theSource;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the color's value to the value of inR, inG, inB and inA which are on the top
 *	of the Lua stack respectively
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::Set(lua_State *inLuaState)
{
    CColor &theColorA = CheckColor(inLuaState);

    theColorA.m_Red = GetColorValue(inLuaState, 2);
    theColorA.m_Green = GetColorValue(inLuaState, 3);
    theColorA.m_Blue = GetColorValue(inLuaState, 4);
    theColorA.m_Alpha = GetColorValue(inLuaState, 5);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Checks for equality of 2 colors
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::Equals(lua_State *inLuaState)
{
    CColor &theColorA = CheckColor(inLuaState, 1);
    CColor &theColorB = CheckColor(inLuaState, 2);

    BOOL theResult = theColorA.m_Red == theColorB.m_Red && theColorA.m_Green == theColorB.m_Green
        && theColorA.m_Blue == theColorB.m_Blue && theColorA.m_Alpha == theColorB.m_Alpha;

    lua_pushboolean(inLuaState, theResult);
    return 1;
}

//==============================================================================
/**
 *	Formats the Color object into a string and place the string on the top of Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaColor::ToString(lua_State *inLuaState)
{
    CColor &theColor = CheckColor(inLuaState);

    lua_pushfstring(inLuaState, "Color: %d, %d, %d, %d", theColor.m_Red, theColor.m_Green,
                    theColor.m_Blue, theColor.m_Alpha);

    return 1;
}

//==============================================================================
/**
 *	Helper to check the type of the userdata is of STRING_COLOR_TYPE
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inIndex			the index to where on the stack to check
 *	@return the reference to the userdata pointed to by inIndex
 */
CColor &CLuaColor::CheckColor(lua_State *inLuaState, const INT32 inIndex)
{
    CColor *theColor =
        reinterpret_cast<CColor *>(luaL_checkudata(inLuaState, inIndex, STRING_COLOR_TYPE));
    luaL_argcheck(inLuaState, theColor != NULL, 1, STRING_ERROR_COLOR_EXPECTED);
    return *theColor;
}

//==============================================================================
/**
 *	Helper to create a new userdata of STRING_COLOR_TYPE on the top of the stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the reference to the userdata pointed to by inIndex
 */
CColor &CLuaColor::CreateColor(lua_State *inLuaState)
{
    CColor *theColor = reinterpret_cast<CColor *>(lua_newuserdata(inLuaState, sizeof(CColor)));

    // associate the metatable with CColor
    luaL_getmetatable(inLuaState, STRING_COLOR_TYPE);
    lua_setmetatable(inLuaState, -2);

    return *theColor;
}

//==============================================================================
/**
 *	Helper to get a color value from the stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inIndex			the index to where the color value is on the stack
 *	@return the color value
 */
UINT8 CLuaColor::GetColorValue(lua_State *inLuaState, const INT32 inIndex)
{
    INT32 theValue = luaL_checkinteger(inLuaState, inIndex);
    luaL_argcheck(inLuaState, theValue <= 255, 1, STRING_ERROR_COLOR_VALUE_EXPECTED);
    return static_cast<UINT8>(theValue);
}

} // namespace Q3DStudio
