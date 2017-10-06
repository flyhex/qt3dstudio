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
#include "UICLuaHelper.h"
#include "UICApplication.h"
#include "foundation/Qt3DSLogging.h"

//==============================================================================
// Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
CLuaHelper::CLuaHelper()
{
}

//==============================================================================
/**
 *	Destructor
 */
CLuaHelper::~CLuaHelper()
{
}

//==============================================================================
/**
 *	Push the correct type of the UVariant value onto the Lua stack.
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inType				the value type
 *	@param inValue				the value to push onto the stack.
 *	@return the number of values pushed onto the stack.
 */
INT32 CLuaHelper::PushArgument(lua_State *inLuaState, const UINT8 inType, UVariant &inValue,
                               qt3ds::foundation::IStringTable &strTable,
                               uic::runtime::IApplication &application)
{
    INT32 theResult = 1;

    switch (inType) {
    case ATTRIBUTETYPE_INT32:
    case ATTRIBUTETYPE_HASH:
        lua_pushinteger(inLuaState, inValue.m_INT32);
        break;

    case ATTRIBUTETYPE_FLOAT:
        lua_pushnumber(inLuaState, inValue.m_FLOAT);
        break;

    case ATTRIBUTETYPE_BOOL:
        lua_pushboolean(inLuaState, inValue.m_INT32 != 0);
        break;

    case ATTRIBUTETYPE_STRING:
        lua_pushstring(inLuaState, strTable.HandleToStr(inValue.m_StringHandle));
        break;

    case ATTRIBUTETYPE_POINTER:
        lua_pushlightuserdata(inLuaState, inValue.m_VoidPointer);
        break;

    case ATTRIBUTETYPE_ELEMENTREF:
        lua_pushlightuserdata(inLuaState, application.GetElementByHandle(inValue.m_ElementHandle));
        break;

    case ATTRIBUTETYPE_NONE:
        lua_pushnil(inLuaState);
        break;

    default:
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Pushed event argument value of unknown type: "<< inType;
        theResult = 0;
        break;
    }

    return theResult;
}

//==============================================================================
/**
 *	Get an argument from the Lua stack
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inIndex				the Lua stack index where the argument is
 *	@param outValue				the value to store the argument
 *	@return the type of the argument that is retrieved
 */
UINT8 CLuaHelper::GetArgument(lua_State *inLuaState, INT32 inIndex, UVariant &outValue,
                              qt3ds::foundation::IStringTable &strTable, uic::runtime::IApplication &)
{
    UINT8 theType = ATTRIBUTETYPE_NONE;
    switch (lua_type(inLuaState, inIndex)) {
    case LUA_TNUMBER:
        outValue.m_FLOAT = static_cast<FLOAT>(lua_tonumber(inLuaState, inIndex));
        theType = ATTRIBUTETYPE_FLOAT;
        break;

    case LUA_TBOOLEAN:
        outValue.m_INT32 = lua_toboolean(inLuaState, inIndex) ? 1 : 0;
        theType = ATTRIBUTETYPE_BOOL;
        break;

    case LUA_TSTRING:
        outValue.m_StringHandle = strTable.GetHandle(lua_tostring(inLuaState, inIndex));
        theType = ATTRIBUTETYPE_STRING;
        break;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
        outValue.m_VoidPointer = lua_touserdata(inLuaState, inIndex);
        theType = ATTRIBUTETYPE_POINTER;
        break;

    case LUA_TNIL:
        outValue.m_INT32 = 0;
        theType = ATTRIBUTETYPE_NONE;
        break;

    default:
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Got event argument value of unknown type: "
                << lua_type(inLuaState, inIndex);
        break;
    }

    return theType;
}

} // namespace Q3DStudio
