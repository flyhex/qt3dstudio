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
#include "Qt3DSLuaMatrix.h"
#include "Qt3DSMatrix.h"
#include "Qt3DSVector3.h"
#include "Qt3DSLuaVector.h"
#include "Qt3DSDataLogger.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const CHAR *STRING_MATRIX_TYPE = "MatrixType";
const CHAR *STRING_MATRIX_CLASS = "Matrix";
const CHAR *STRING_ERROR_MATRIX_EXPECTED = "'Matrix' expected.";

extern const CHAR *STRING_ERROR_WRONG_NUMBER_ARUGMENT;
extern const CHAR *STRING_ERROR_ACCESS_NON_MEMBER;

//==============================================================================
/**
 *	Registers the type "Matrix" in Lua's global name space
 *	@param inLuaState		the Lua state, required for interaction with lua
 */
void CLuaMatrix::RegisterLibrary(lua_State *inLuaState)
{
    static const luaL_Reg theMatrixLibrary[] = { { "new", Create }, { NULL, NULL } };

    static const luaL_Reg theMatrixMethods[] = { { "setMatrix", SetMatrix },
                                                 { "set", Set },
                                                 { "setScale", SetScale },
                                                 { "setRotate", SetRotate },
                                                 { "setTranslate", SetTranslate },
                                                 { "equals", Equals },
                                                 { "identity", Identity },
                                                 { "invert", Invert },
                                                 { "isIdentity", IsIdentity },
                                                 { "multiply", Multiply },
                                                 { "scale", Scale },
                                                 { "translate", Translate },
                                                 { "rotate", Rotate },
                                                 { "transpose", Transpose },
                                                 { NULL, NULL } };

    INT32 theTop = lua_gettop(inLuaState);

    luaL_newmetatable(inLuaState,
                      STRING_MATRIX_TYPE); // Create a new metatable for this Matrix type
    luaL_register(inLuaState, NULL,
                  theMatrixMethods); // Registers the additional methods to this metatable

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

    luaL_register(inLuaState, STRING_MATRIX_CLASS,
                  theMatrixLibrary); // register the 'Matrix' class type with one method 'new'

    lua_settop(inLuaState, theTop);
}

//==============================================================================
/**
 *	Creates a new Lua Matrix object and places it on the top of the Lua stack
 *	3 ways to create the new color:
 *	1) Matrix.new( ) 					-- a new identiy matrix
 *	2) Matrix.new( inMatrix ) 			-- a new matrix with values set to inMatrix
 *
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Create(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    INT32 theTop = lua_gettop(inLuaState);

    RuntimeMatrix &theMatrix = CreateMatrix(inLuaState);

    INT32 theOffset = lua_istable(inLuaState, 1) ? 1 : 0;

    if (theTop == 0 + theOffset)
        theMatrix.Identity();
    else if (theTop == 1 + theOffset) {
        RuntimeMatrix &theSource = CheckMatrix(inLuaState, 1 + theOffset);
        theMatrix = theSource;
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    Q3DStudio_ASSERT(lua_gettop(inLuaState) == theTop + 1);
    return 1;
}

//==============================================================================
/**
 *	Gets a value of the matrix and places it on the top of the lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::GetValue(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrix = CheckMatrix(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);
    BOOL theError = false;
    INT32 theRow = -1;
    INT32 theColumn = -1;

    if (::strlen(theParam) != 3 || theParam[0] != '_')
        theError = true;

    if (!theError) {
        theRow = theParam[1] - '1';
        theColumn = theParam[2] - '1';

        if (theRow < 0 || theRow > 3)
            theError = true;
        else if (theColumn < 0 || theColumn > 3)
            theError = true;
    }

    if (!theError)
        lua_pushnumber(inLuaState, theMatrix.Get(theRow, theColumn));
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
 *	Sets a value of the matrix
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::SetValue(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrix = CheckMatrix(inLuaState);
    const CHAR *theParam = luaL_checkstring(inLuaState, 2);
    UINT8 theValue = static_cast<UINT8>(luaL_checkinteger(inLuaState, 3));
    BOOL theError = false;
    INT32 theRow = -1;
    INT32 theColumn = -1;

    if (::strlen(theParam) != 3 || theParam[0] != '_')
        theError = true;

    if (!theError) {
        theRow = theParam[1] - '1';
        theColumn = theParam[2] - '1';

        if (theRow < 0 || theRow > 3)
            theError = true;
        else if (theColumn < 0 || theColumn > 3)
            theError = true;
    }

    if (!theError)
        theMatrix.Set(theRow, theColumn, theValue);
    else
        luaL_error(inLuaState, STRING_ERROR_ACCESS_NON_MEMBER);

    return 0;
}

//==============================================================================
/**
 *	Set the matrix's value to the value of inMatrix which is on the top of the
 *	Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::SetMatrix(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theDestination = CheckMatrix(inLuaState);
    RuntimeMatrix &theSource = CheckMatrix(inLuaState, 2);

    theDestination = theSource;
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the matrix's value to the value of in11, in12, in13, in14, in21, in22, in23, in24
 *	in31, in32, in33, in34, in41, in42, in43 and in44 which are on the top of
 *	the Lua stack respectively
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Set(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);

    if (lua_gettop(inLuaState) != 17)
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    theMatrixA.Set(0, 0, static_cast<FLOAT>(luaL_checknumber(inLuaState, 2)));
    theMatrixA.Set(0, 1, static_cast<FLOAT>(luaL_checknumber(inLuaState, 3)));
    theMatrixA.Set(0, 2, static_cast<FLOAT>(luaL_checknumber(inLuaState, 4)));
    theMatrixA.Set(0, 3, static_cast<FLOAT>(luaL_checknumber(inLuaState, 5)));

    theMatrixA.Set(1, 0, static_cast<FLOAT>(luaL_checknumber(inLuaState, 6)));
    theMatrixA.Set(1, 1, static_cast<FLOAT>(luaL_checknumber(inLuaState, 7)));
    theMatrixA.Set(1, 2, static_cast<FLOAT>(luaL_checknumber(inLuaState, 8)));
    theMatrixA.Set(1, 3, static_cast<FLOAT>(luaL_checknumber(inLuaState, 9)));

    theMatrixA.Set(2, 0, static_cast<FLOAT>(luaL_checknumber(inLuaState, 10)));
    theMatrixA.Set(2, 1, static_cast<FLOAT>(luaL_checknumber(inLuaState, 11)));
    theMatrixA.Set(2, 2, static_cast<FLOAT>(luaL_checknumber(inLuaState, 12)));
    theMatrixA.Set(2, 3, static_cast<FLOAT>(luaL_checknumber(inLuaState, 13)));

    theMatrixA.Set(3, 0, static_cast<FLOAT>(luaL_checknumber(inLuaState, 14)));
    theMatrixA.Set(3, 1, static_cast<FLOAT>(luaL_checknumber(inLuaState, 15)));
    theMatrixA.Set(3, 2, static_cast<FLOAT>(luaL_checknumber(inLuaState, 16)));
    theMatrixA.Set(3, 3, static_cast<FLOAT>(luaL_checknumber(inLuaState, 17)));

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the matrix's scale portion without affecting the rest
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::SetScale(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    INT32 theTop = lua_gettop(inLuaState);

    if (theTop == 2) {
        RuntimeVector3 &theScale = CLuaVector::CheckVector(inLuaState, 2);
        theMatrixA.SetScale(theScale);
    } else if (theTop == 4) {
        FLOAT theX = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
        FLOAT theY = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
        FLOAT theZ = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
        RuntimeVector3 theScale(theX, theY, theZ);
        theMatrixA.SetScale(theScale);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the matrix's rotation portion
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::SetRotate(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    INT32 theTop = lua_gettop(inLuaState);

    if (theTop == 2) {
        RuntimeVector3 &theRotate = CLuaVector::CheckVector(inLuaState, 2);
        theMatrixA.SetRotate(theRotate, RuntimeMatrix::ROTATIONORDER_YXZ, 0);
    } else if (theTop == 4) {
        FLOAT theX = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
        FLOAT theY = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
        FLOAT theZ = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
        RuntimeVector3 theRotate(theX, theY, theZ);
        theMatrixA.SetRotate(theRotate, RuntimeMatrix::ROTATIONORDER_YXZ, 0);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Set the matrix's translate portion without affecting the rest
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::SetTranslate(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    INT32 theTop = lua_gettop(inLuaState);

    if (theTop == 2) {
        RuntimeVector3 &theTranslate = CLuaVector::CheckVector(inLuaState, 2);
        theMatrixA.SetTranslate(theTranslate);
    } else if (theTop == 4) {
        FLOAT theX = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
        FLOAT theY = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
        FLOAT theZ = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
        RuntimeVector3 theTranslate(theX, theY, theZ);
        theMatrixA.SetTranslate(theTranslate);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Checks for equality of 2 matrixes
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Equals(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState, 1);
    RuntimeMatrix &theMatrixB = CheckMatrix(inLuaState, 2);

    lua_pushboolean(inLuaState, theMatrixA == theMatrixB);
    return 1;
}

//==============================================================================
/**
 *	Resets the matrix to identity matrix
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Identity(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);

    theMatrixA.Identity();
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Inverts the matrix placed on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Invert(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);

    lua_pushnumber(inLuaState, theMatrixA.Invert());
    return 1;
}

//==============================================================================
/**
 *	Checks if the matrix placed on the top of the Lua stack is an identity matrix
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::IsIdentity(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);

    lua_pushboolean(inLuaState, theMatrixA.IsIdentity());
    return 1;
}

//==============================================================================
/**
 *	Multiplies the 2 matrixes on the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Multiply(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState, 1);
    RuntimeMatrix &theMatrixB = CheckMatrix(inLuaState, 2);

    theMatrixA.Multiply(theMatrixB);
    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Performs additional scaling to the matrix on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Scale(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    INT32 theTop = lua_gettop(inLuaState);

    if (theTop == 2) {
        RuntimeVector3 &theScale = CLuaVector::CheckVector(inLuaState, 2);
        theMatrixA.Scale(theScale);
    } else if (theTop == 4) {
        FLOAT theX = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
        FLOAT theY = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
        FLOAT theZ = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
        RuntimeVector3 theScale(theX, theY, theZ);
        theMatrixA.Scale(theScale);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Performs additional translation to the matrix on the top of the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Translate(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    INT32 theTop = lua_gettop(inLuaState);

    if (theTop == 2) {
        RuntimeVector3 &theTranslate = CLuaVector::CheckVector(inLuaState, 2);
        theMatrixA.Translate(theTranslate);
    } else if (theTop == 4) {
        FLOAT theX = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
        FLOAT theY = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
        FLOAT theZ = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
        RuntimeVector3 theTranslate(theX, theY, theZ);
        theMatrixA.Translate(theTranslate);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Performs additional rotation to the matrix on the top of the Lua stack
 *	@param inLuaState	the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Rotate(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    INT32 theTop = lua_gettop(inLuaState);
    RuntimeMatrix theRotationMatrix;

    if (theTop == 2) {
        RuntimeVector3 &theEulerAngles = CLuaVector::CheckVector(inLuaState, 2);
        theRotationMatrix.SetRotate(theEulerAngles, RuntimeMatrix::ROTATIONORDER_YXZ, 0);
        theMatrixA.Multiply(theRotationMatrix);
    } else if (theTop == 4) {
        FLOAT theX = static_cast<FLOAT>(luaL_checknumber(inLuaState, 2));
        FLOAT theY = static_cast<FLOAT>(luaL_checknumber(inLuaState, 3));
        FLOAT theZ = static_cast<FLOAT>(luaL_checknumber(inLuaState, 4));
        RuntimeVector3 theEulerAngles(theX, theY, theZ);
        theRotationMatrix.SetRotate(theEulerAngles, RuntimeMatrix::ROTATIONORDER_YXZ, 0);
        theMatrixA.Multiply(theRotationMatrix);
    } else
        luaL_error(inLuaState, STRING_ERROR_WRONG_NUMBER_ARUGMENT);

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Flips the matrix elements around the identity diagonal
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::Transpose(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrixA = CheckMatrix(inLuaState);
    theMatrixA.Transpose();

    lua_pushvalue(inLuaState, 1);
    return 1;
}

//==============================================================================
/**
 *	Formats the Matrix object into a string and place the string on the top of Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaMatrix::ToString(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix &theMatrix = CheckMatrix(inLuaState);

    lua_pushstring(inLuaState, "string");
    lua_rawget(inLuaState, LUA_GLOBALSINDEX);

    lua_getfield(inLuaState, -1, "format");

    lua_pushstring(inLuaState, "Matrix:\n%8.2f %8.2f %8.2f %8.2f\n%8.2f %8.2f %8.2f %8.2f\n%8.2f "
                               "%8.2f %8.2f %8.2f\n%8.2f %8.2f %8.2f %8.2f");

    for (INT32 theRow = 0; theRow < 4; ++theRow)
        for (INT32 theCol = 0; theCol < 4; ++theCol)
            lua_pushnumber(inLuaState, theMatrix.m_Data[theRow][theCol]);

    lua_pcall(inLuaState, 17, 1, 0);

    return 1;
}

//==============================================================================
/**
 *	Helper to check the type of the userdata is of STRING_MATRIX_TYPE
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inIndex			the index to where on the stack to check
 *	@return the reference to the userdata pointed to by inIndex
 */
RuntimeMatrix &CLuaMatrix::CheckMatrix(lua_State *inLuaState, const INT32 inIndex)
{
    // PerfLogLuaMathEvent1( DATALOGGER_LUA_MATRIX );

    RuntimeMatrix *theMatrix =
        reinterpret_cast<RuntimeMatrix *>(luaL_checkudata(inLuaState, inIndex, STRING_MATRIX_TYPE));
    luaL_argcheck(inLuaState, theMatrix != NULL, 1, STRING_ERROR_MATRIX_EXPECTED);
    return *theMatrix;
}

//==============================================================================
/**
 *	Helper to create a new userdata of STRING_MATRIX_TYPE on the top of the stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the reference to the userdata pointed to by inIndex
 */
RuntimeMatrix &CLuaMatrix::CreateMatrix(lua_State *inLuaState)
{
    PerfLogLuaMathEvent1(DATALOGGER_LUA_MATRIX);

    RuntimeMatrix *theMatrix = reinterpret_cast<RuntimeMatrix *>(lua_newuserdata(inLuaState, sizeof(RuntimeMatrix)));

    // associate the metatable with CMatrix
    luaL_getmetatable(inLuaState, STRING_MATRIX_TYPE);
    lua_setmetatable(inLuaState, -2);

    return *theMatrix;
}

} // namespace Q3DStudio
