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
#include "Qt3DSLuaRotation.h"
#include "Qt3DSLuaVector.h"
#include "Qt3DSVector3.h"
#include <math.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const CHAR *STRING_ROTATION_TYPE = "RotationType";
const CHAR *STRING_ROTATION_CLASS = "Rotation";
const CHAR *STRING_ERROR_ROTATION_EXPECTED = "'Rotation' expected.";

extern const CHAR *STRING_ERROR_WRONG_NUMBER_ARUGMENT;
extern const CHAR *STRING_ERROR_ACCESS_NON_MEMBER;

//==============================================================================
/**
 *	Registers the type "Rotation" in Lua's global name space
 *	@param inLuaState		the Lua state, required for interaction with lua
 */
void CLuaRotation::RegisterLibrary(lua_State *inLuaState)
{
    static const luaL_Reg theRotationLibrary[] = { { "new", Create }, { NULL, NULL } };

    static const luaL_Reg theRotationMethods[] = {
        { "setRotation", SetRotation }, { "set", Set },     { "add", Add },
        { "subtract", Subtract },       { "scale", Scale }, { "equals", Equals },
        { "lookAt", LookAt },           { NULL, NULL }
    };

    INT32 theTop = lua_gettop(inLuaState);

    luaL_newmetatable(inLuaState,
                      STRING_ROTATION_TYPE); // Create a new metatable for this Rotation type
    luaL_register(inLuaState, NULL,
                  theRotationMethods); // Registers the additional methods to this metatable

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

    lua_pushstring(inLuaState, "__add"); // Addition operator '+'
    lua_pushcfunction(inLuaState, AddNew);
    lua_settable(inLuaState, 1);

    lua_pushstring(inLuaState, "__sub"); // Subtraction operator '-'
    lua_pushcfunction(inLuaState, SubtractNew);
    lua_settable(inLuaState, 1);

    lua_pushstring(inLuaState, "__mul"); // Multiplication operator '*'
    lua_pushcfunction(inLuaState, ScaleNew);
    lua_settable(inLuaState, 1);

    lua_pushstring(inLuaState, "__eq"); // Equality operator '=='
    lua_pushcfunction(inLuaState, Equals);
    lua_settable(inLuaState, 1);

    luaL_register(inLuaState, STRING_ROTATION_CLASS,
                  theRotationLibrary); // register the 'Rotation' class type with one method 'new'

    lua_settop(inLuaState, theTop);
}

//==============================================================================
/**
 *	Creates a new Lua Rotation object and places it on the top of the Lua stack
 *	3 ways to create the new rotation:
 *	1) Rotation.new( ) 					-- a new rotation with x, y and z all
 *initialized to 0
 *	2) Rotation.new( inRotation ) 		-- a new rotation with x, y and z all initialized
 *to the values of inRotation
 *	3) Rotation.new( inX, inY, inZ )	-- a new rotation with x, y and z all initialized to
 *the values of inX, inY and inZ
 *
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::Create(lua_State *inLuaState)
{
    INT32 theTop = lua_gettop(inLuaState);

    RuntimeVector3 &theRotation = CreateRotation(inLuaState);

    INT32 theOffset = lua_istable(inLuaState, 1) ? 1 : 0;

    if (theTop == 0 + theOffset) {
        theRotation.m_X = 0.0f;
        theRotation.m_Y = 0.0f;
        theRotation.m_Z = 0.0f;
    } else if (theTop == 1 + theOffset) {
        RuntimeVector3 &theSource = CheckRotation(inLuaState, 1 + theOffset);
        theRotation = theSource;
    } else if (theTop == 3 + theOffset) {
        theRotation.m_X = static_cast<FLOAT>(luaL_checknumber(inLuaState, 1 + theOffset));
        theRotation.m_Y = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2 + theOffset));
        theRotation.m_Z = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3 + theOffset));
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    Q3DStudio_ASSERT(lua_gettop(inLuaState) == theTop + 1);
    return 1;
}

//==============================================================================
/**
 *	Gets the value of x or y or z and places it on the top of the lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::GetValue(lua_State *inLuaState)
{
    RuntimeVector3 &theRotation = CheckRotation(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);

    if (::strcmp(theParam, "x") == 0)
        lua_pushnumber(inLuaState, theRotation.m_X);
    else if (::strcmp(theParam, "y") == 0)
        lua_pushnumber(inLuaState, theRotation.m_Y);
    else if (::strcmp(theParam, "z") == 0)
        lua_pushnumber(inLuaState, theRotation.m_Z);
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
 *	Sets the value of x or y or z
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::SetValue(lua_State *inLuaState)
{
    RuntimeVector3 &theRotation = CheckRotation(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);
    FLOAT theValue = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));

    if (::strcmp(theParam, "x") == 0)
        theRotation.m_X = theValue;
    else if (::strcmp(theParam, "y") == 0)
        theRotation.m_Y = theValue;
    else if (::strcmp(theParam, "z") == 0)
        theRotation.m_Z = theValue;
    else
        luaL_error(inLuaState, STRING_ERROR_ACCESS_NON_MEMBER);

    return 0;
}

//==============================================================================
/**
 *	Set the rotation's value to the value of inRotation which is on the top of the
 *	Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::SetRotation(lua_State *inLuaState)
{
    RuntimeVector3 &theDestination = CheckRotation(inLuaState);
    RuntimeVector3 &theSource = CheckRotation(inLuaState, 2);

    theDestination = theSource;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the rotation's value to the value of inX, inY and inZ which are on the top
 *	of the Lua stack respectively
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::Set(lua_State *inLuaState)
{
    RuntimeVector3 &theDestination = CheckRotation(inLuaState);

    theDestination.m_X = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
    theDestination.m_Y = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
    theDestination.m_Z = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Adds the 2 rotations on the Lua stack, and stores the result in the first
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::Add(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState, 1); // 1 = Rotation A
    RuntimeVector3 &theRotationB = CheckRotation(inLuaState, 2); // 2 = Rotation B

    theRotationA += theRotationB;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Adds the 2 vectors on the Lua stack, and stores the result in a new vector
 *	placed on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::AddNew(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState, 1); // 1 = Rotation A
    RuntimeVector3 &theRotationB = CheckRotation(inLuaState, 2); // 2 = Rotation B
    RuntimeVector3 &theResult = CreateRotation(inLuaState);

    theResult = theRotationA + theRotationB;
    return 1;
}

//==============================================================================
/**
 *	Subtracts the 2nd vector on the Lua stack from the 1st, and stores the
 *	result in the first
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::Subtract(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState, 1); // 1 = Rotation A
    RuntimeVector3 &theRotationB = CheckRotation(inLuaState, 2); // 2 = Rotation B

    theRotationA -= theRotationB;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Subtracts the 2nd vector on the Lua stack from the 1st, and stores the
 *	result in a new vector placed on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::SubtractNew(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState, 1); // 1 = Rotation A
    RuntimeVector3 &theRotationB = CheckRotation(inLuaState, 2); // 2 = Rotation B
    RuntimeVector3 &theResult = CreateRotation(inLuaState);

    theResult = theRotationA - theRotationB;
    return 1;
}

//==============================================================================
/**
 *	Scales the vector with the factor that's placed on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::Scale(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState);
    FLOAT theFactor = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));

    theRotationA *= theFactor;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Scales the vector with the factor that's placed on the top of the Lua stack
 *	and stores the result in a new vector placed on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::ScaleNew(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState);
    FLOAT theFactor = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
    RuntimeVector3 &theResult = CreateRotation(inLuaState);

    theResult = theRotationA * theFactor;
    return 1;
}

//==============================================================================
/**
 *	Checks for equality of 2 rotations
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::Equals(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState, 1);
    RuntimeVector3 &theRotationB = CheckRotation(inLuaState, 2);

    lua_pushboolean(inLuaState, theRotationA == theRotationB);
    return 1;
}

//==============================================================================
/**
 *	Sets the rotation angles to look at a vector
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::LookAt(lua_State *inLuaState)
{
    RuntimeVector3 &theRotationA = CheckRotation(inLuaState, 1);
    RuntimeVector3 &theVector = CLuaVector::CheckVector(inLuaState, 2);

    FLOAT theMag = ::sqrtf(theVector.m_X * theVector.m_X + theVector.m_Z * theVector.m_Z);
#ifdef _TEGRAPLATFORM
    FLOAT thePitch = static_cast<FLOAT>(-::atan2((double)theVector.m_Y, (double)theMag));
    FLOAT theYaw = static_cast<FLOAT>(::atan2((double)theVector.m_X, (double)theVector.m_Z));
#else
    FLOAT thePitch = -::atan2f(theVector.m_Y, theMag);
    FLOAT theYaw = ::atan2f(theVector.m_X, theVector.m_Z);
#endif

    theRotationA.Set(thePitch, theYaw, 0.0f);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Formats the Rotation object into a string and place the string on the top of Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaRotation::ToString(lua_State *inLuaState)
{
    RuntimeVector3 &theRotation = CheckRotation(inLuaState);

    lua_pushfstring(inLuaState, "Rotation: %f, %f, %f", theRotation.m_X, theRotation.m_Y,
                    theRotation.m_Z);
    return 1;
}

//==============================================================================
/**
 *	Helper to check the type of the userdata is of STRING_ROTATION_TYPE
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inIndex			the index to where on the stack to check
 *	@return the reference to the userdata pointed to by inIndex
 */
RuntimeVector3 &CLuaRotation::CheckRotation(lua_State *inLuaState, const INT32 inIndex)
{
    RuntimeVector3 *theRotation =
        reinterpret_cast<RuntimeVector3 *>(luaL_checkudata(inLuaState, inIndex, STRING_ROTATION_TYPE));
    luaL_argcheck(inLuaState, theRotation != NULL, 1, STRING_ERROR_ROTATION_EXPECTED);
    return *theRotation;
}

//==============================================================================
/**
 *	Helper to create a new userdata of STRING_ROTATION_TYPE on the top of the stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the reference to the userdata pointed to by inIndex
 */
RuntimeVector3 &CLuaRotation::CreateRotation(lua_State *inLuaState)
{
    RuntimeVector3 *theRotation =
        reinterpret_cast<RuntimeVector3 *>(lua_newuserdata(inLuaState, sizeof(RuntimeVector3)));

    // associate the metatable with CVector3
    luaL_getmetatable(inLuaState, STRING_ROTATION_TYPE);
    lua_setmetatable(inLuaState, -2);

    return *theRotation;
}

} // namespace Q3DStudio
