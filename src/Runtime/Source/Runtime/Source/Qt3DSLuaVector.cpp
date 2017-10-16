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
#include "Qt3DSLuaVector.h"
#include "Qt3DSVector3.h"
#include "Qt3DSLuaMatrix.h"
#include "Qt3DSDataLogger.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const CHAR *CLuaVector::STRING_VECTOR_TYPE = "VectorType";
const CHAR *CLuaVector::STRING_VECTOR_CLASS = "Vector";
const CHAR *STRING_ERROR_VECTOR_EXPECTED = "'Vector' expected.";

const CHAR *STRING_ERROR_WRONG_NUMBER_ARUGMENT = "Wrong number of arguments given.";
const CHAR *STRING_ERROR_ACCESS_NON_MEMBER = "Attempt to access a non member.";

//==============================================================================
/**
 *	Registers the type "Vector" in Lua's global name space
 *	@param inLuaState		the Lua state, required for interaction with lua
 */
void CLuaVector::RegisterLibrary(lua_State *inLuaState)
{
    static const luaL_Reg theVectorLibrary[] = { { "new", Create }, { NULL, NULL } };

    static const luaL_Reg theVectorMethods[] = { { "setVector", SetVector },
                                                 { "set", Set },
                                                 { "add", Add },
                                                 { "subtract", Subtract },
                                                 { "scale", Scale },
                                                 { "equals", Equals },
                                                 { "dot", DotProduct },
                                                 { "cross", CrossProduct },
                                                 { "normalize", Normalize },
                                                 { "length", Length },
                                                 { "lengthSquared", LengthSquared },
                                                 { "distance", Distance },
                                                 { "distanceSquared", DistanceSquared },
                                                 { "minVector", MinVector },
                                                 { "maxVector", MaxVector },
                                                 { "linear", Linear },
                                                 { "transform", Transform },
                                                 { NULL, NULL } };

    INT32 theTop = lua_gettop(inLuaState);

    luaL_newmetatable(inLuaState,
                      STRING_VECTOR_TYPE); // Create a new metatable for this Vector type
    luaL_register(inLuaState, NULL,
                  theVectorMethods); // Registers the additional methods to this metatable

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

    luaL_register(inLuaState, STRING_VECTOR_CLASS,
                  theVectorLibrary); // register the 'Vector' class type with one method 'new'

    lua_settop(inLuaState, theTop);
}

//==============================================================================
/**
 *	Creates a new Lua Vector object and places it on the top of the Lua stack
 *	3 ways to creat the new vector:
 *	1) Vector.new( ) 				-- a new vector with x,y and z all initialized to
 *0
 *	2) Vector.new( inVector ) 		-- a new vector with x,y and z all initialized to the
 *values of inVector
 *	3) Vector.new( inX, inY, inZ )	-- a new vector with x,y and z all initialized to the
 *values of inX, inY and inZ
 *
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of results pushed to the stack
 */
int CLuaVector::Create(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    INT32 theTop = lua_gettop(inLuaState);

    RuntimeVector3 &theVector = CreateVector(inLuaState);

    INT32 theOffset = lua_istable(inLuaState, 1) ? 1 : 0;

    if (theTop == 0 + theOffset) {
        theVector.m_X = 0.0f;
        theVector.m_Y = 0.0f;
        theVector.m_Z = 0.0f;
    } else if (theTop == 1 + theOffset) {
        RuntimeVector3 &theSource = CheckVector(inLuaState, 1 + theOffset);
        theVector = theSource;
    } else if (theTop == 3 + theOffset) {
        theVector.m_X = static_cast<FLOAT>(luaL_checknumber(inLuaState, 1 + theOffset));
        theVector.m_Y = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2 + theOffset));
        theVector.m_Z = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3 + theOffset));
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    Q3DStudio_ASSERT(lua_gettop(inLuaState) == theTop + 1);
    return 1;
}

//==============================================================================
/**
 *	Gets the value of x or y or z and place it on the top of lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::GetValue(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVector = CheckVector(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);

    if (::strcmp(theParam, "x") == 0)
        lua_pushnumber(inLuaState, theVector.m_X);
    else if (::strcmp(theParam, "y") == 0)
        lua_pushnumber(inLuaState, theVector.m_Y);
    else if (::strcmp(theParam, "z") == 0)
        lua_pushnumber(inLuaState, theVector.m_Z);
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
int CLuaVector::SetValue(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVector = CheckVector(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);
    FLOAT theValue = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));

    if (::strcmp(theParam, "x") == 0)
        theVector.m_X = theValue;
    else if (::strcmp(theParam, "y") == 0)
        theVector.m_Y = theValue;
    else if (::strcmp(theParam, "z") == 0)
        theVector.m_Z = theValue;
    else
        luaL_error(inLuaState, STRING_ERROR_ACCESS_NON_MEMBER);

    return 0;
}

//==============================================================================
/**
 *	Set the vector's value to the value of inVector which is on the top of the
 *	Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::SetVector(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theDestination = CheckVector(inLuaState);
    RuntimeVector3 &theSource = CheckVector(inLuaState, 2);

    theDestination = theSource;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the vector's value to the value of inX, inY, inZ which are on the top
 *	of the Lua stack respectively
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Set(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theDestination = CheckVector(inLuaState);

    theDestination.m_X = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
    theDestination.m_Y = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
    theDestination.m_Z = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Adds the 2 vectors on the Lua stack, and stores the result in the first
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Add(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1); // 1 = Vector A
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2); // 2 = Vector B

    theVectorA += theVectorB;
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
int CLuaVector::AddNew(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1); // 1 = Vector A
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2); // 2 = Vector B
    RuntimeVector3 &theResult = CreateVector(inLuaState);

    theResult = theVectorA + theVectorB;
    return 1;
}

//==============================================================================
/**
 *	Subtracts the 2nd vector on the Lua stack from the 1st, and stores the
 *	result in the first
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Subtract(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1); // 1 = Vector A
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2); // 2 = Vector B

    theVectorA -= theVectorB;
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
int CLuaVector::SubtractNew(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1); // 1 = Vector A
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2); // 2 = Vector B
    RuntimeVector3 &theResult = CreateVector(inLuaState);

    theResult = theVectorA - theVectorB;
    return 1;
}

//==============================================================================
/**
 *	Scales the vector with the factor that's placed on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Scale(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState);
    FLOAT theFactor = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));

    theVectorA *= theFactor;
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
int CLuaVector::ScaleNew(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState);
    FLOAT theFactor = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
    RuntimeVector3 &theResult = CreateVector(inLuaState);

    theResult = theVectorA * theFactor;
    return 1;
}

//==============================================================================
/**
 *	Checks for equality of 2 vectors
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Equals(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    lua_pushboolean(inLuaState, theVectorA == theVectorB);
    return 1;
}

//==============================================================================
/**
 *	Calculates the dot product of the first vector with the 2nd vector and places
 *	the FLOAT result on the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::DotProduct(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    lua_pushnumber(inLuaState, theVectorA.DotProduct(theVectorB));
    return 1;
}

//==============================================================================
/**
 *	Calculates the cross product of the first vector with the 2nd vector and
 *	places the Vector result on the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::CrossProduct(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    theVectorA.CrossProduct(theVectorB);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Makes the vector a unit vector
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Normalize(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState);

    theVectorA.Normalize();
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Calculates the length (magnitude) of the vector and places the FLOAT result
 *	on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Length(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState);

    lua_pushnumber(inLuaState, theVectorA.Length());
    return 1;
}

//==============================================================================
/**
 *	Calculates the squared length (magnitude) of the vector and places the FLOAT
 *	result on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::LengthSquared(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState);

    lua_pushnumber(inLuaState, theVectorA.LengthSquared());
    return 1;
}

//==============================================================================
/**
 *	Calculates the distance between 2 vectors and places the FLOAT result
 *	on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Distance(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    lua_pushnumber(inLuaState, theVectorA.Distance(theVectorB));
    return 1;
}

//==============================================================================
/**
 *	Calculates the squared distance between 2 vectors and places the FLOAT result
 *	on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::DistanceSquared(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    lua_pushnumber(inLuaState, theVectorA.DistanceSquared(theVectorB));
    return 1;
}

//==============================================================================
/**
 *	Modifies the first vector to contain the min values of the 1st and 2nd vectors
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::MinVector(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    theVectorA.Minimize(theVectorB);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Modifies the first vector to contain the max values of the 1st and 2nd vectors
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::MaxVector(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);

    theVectorA.Maximize(theVectorB);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Modifies the first vector by performing a linear interpolation between the
 *	1st vector and the 2nd vector
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Linear(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeVector3 &theVectorB = CheckVector(inLuaState, 2);
    FLOAT theFactor = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));

    theVectorA.InterpolateLinear(theVectorB, theFactor);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Transform the vector with the matrix on the 2nd position on the stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::Transform(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVectorA = CheckVector(inLuaState, 1);
    RuntimeMatrix &theMatrixB = CLuaMatrix::CheckMatrix(inLuaState, 2);

    theVectorA.Transform(theMatrixB);
    lua_pushvalue(inLuaState, 1);
    return 1;
}
//==============================================================================
/**
 *	Formats the Vector object into a string and place the string on the top of Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaVector::ToString(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 &theVector = CheckVector(inLuaState);

    lua_pushfstring(inLuaState, "Vector: %f, %f, %f", theVector.m_X, theVector.m_Y, theVector.m_Z);

    return 1;
}

//==============================================================================
/**
 *	Helper to check the type of the userdata is of STRING_VECTOR_TYPE
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inIndex			the index to where on the stack to check
 *	@return the reference to the userdata pointed to by inIndex
 */
RuntimeVector3 &CLuaVector::CheckVector(lua_State *inLuaState, const INT32 inIndex)
{
    // PerfLogLuaMathEvent1( DATALOGGER_LUA_VECTOR );

    RuntimeVector3 *theVector =
        reinterpret_cast<RuntimeVector3 *>(luaL_checkudata(inLuaState, inIndex, STRING_VECTOR_TYPE));
    luaL_argcheck(inLuaState, theVector != NULL, 1, STRING_ERROR_VECTOR_EXPECTED);
    return *theVector;
}

//==============================================================================
/**
 *	Helper to create a new userdata of STRING_VECTOR_TYPE on the top of the stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the reference to the userdata pointed to by inIndex
 */
RuntimeVector3 &CLuaVector::CreateVector(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_VECTOR);

    RuntimeVector3 *theVector =
        reinterpret_cast<RuntimeVector3 *>(lua_newuserdata(inLuaState, sizeof(RuntimeVector3)));

    // associate the metatable with CVector3
    luaL_getmetatable(inLuaState, STRING_VECTOR_TYPE);
    lua_setmetatable(inLuaState, -2);

    return *theVector;
}

} // namespace Q3DStudio
