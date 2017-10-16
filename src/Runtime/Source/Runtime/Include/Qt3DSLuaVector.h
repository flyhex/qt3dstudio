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

#pragma once

//==============================================================================
//	Lua Includes
//==============================================================================
#include "Qt3DSLuaIncludes.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class RuntimeVector3;

//==============================================================================
/**
 *	The "Vector" type implementation for Lua
 */
class CLuaVector
{
    //==============================================================================
    //	Constants
    //==============================================================================
public:
    static const CHAR *STRING_VECTOR_TYPE;
    static const CHAR *STRING_VECTOR_CLASS;

    //==============================================================================
    //	Methods
    //==============================================================================
protected: // Construction
    CLuaVector();
    CLuaVector(const CLuaVector &);
    CLuaVector &operator=(const CLuaVector &);
    ~CLuaVector();

public: // Vector Registration
    static void RegisterLibrary(lua_State *inLuaState);

protected: // Creation / Initialization
    static int Create(lua_State *inLuaState);
    static int GetValue(lua_State *inLuaState);
    static int SetValue(lua_State *inLuaState);
    static int SetVector(lua_State *inLuaState);
    static int Set(lua_State *inLuaState);

protected: // Operators
    static int Add(lua_State *inLuaState);
    static int AddNew(lua_State *inLuaState);
    static int Subtract(lua_State *inLuaState);
    static int SubtractNew(lua_State *inLuaState);
    static int Scale(lua_State *inLuaState);
    static int ScaleNew(lua_State *inLuaState);
    static int Equals(lua_State *inLuaState);
    static int DotProduct(lua_State *inLuaState);
    static int CrossProduct(lua_State *inLuaState);

protected: // Functions
    static int Normalize(lua_State *inLuaState);
    static int Length(lua_State *inLuaState);
    static int LengthSquared(lua_State *inLuaState);
    static int Distance(lua_State *inLuaState);
    static int DistanceSquared(lua_State *inLuaState);
    static int MinVector(lua_State *inLuaState);
    static int MaxVector(lua_State *inLuaState);
    static int Linear(lua_State *inLuaState);
    static int Transform(lua_State *inLuaState);

protected: // Format
    static int ToString(lua_State *inLuaState);

public: // Helpers
    static RuntimeVector3 &CheckVector(lua_State *inLuaState, const INT32 inIndex = 1);
    static RuntimeVector3 &CreateVector(lua_State *inLuaState);
};

} // namespace Q3DStudio
