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
class RuntimeMatrix;

//==============================================================================
/**
 *	The "Matrix" type for implementation for Lua
 */
class CLuaMatrix
{
protected: // Construction
    CLuaMatrix();
    CLuaMatrix(const CLuaMatrix &);
    CLuaMatrix &operator=(const CLuaMatrix &);
    ~CLuaMatrix();

public: // Matrix Registration
    static void RegisterLibrary(lua_State *inLuaState);

protected: // Creation / Initialization
    static int Create(lua_State *inLuaState);
    static int GetValue(lua_State *inLuaState);
    static int SetValue(lua_State *inLuaState);
    static int SetMatrix(lua_State *inLuaState);
    static int Set(lua_State *inLuaState);
    static int SetScale(lua_State *inLuaState);
    static int SetRotate(lua_State *inLuaState);
    static int SetTranslate(lua_State *inLuaState);

protected: // Operators
    static int Equals(lua_State *inLuaState);

protected: // Functions
    static int Identity(lua_State *inLuaState);
    static int Invert(lua_State *inLuaState);
    static int IsIdentity(lua_State *inLuaState);
    static int Multiply(lua_State *inLuaState);
    static int Scale(lua_State *inLuaState);
    static int Translate(lua_State *inLuaState);
    static int Rotate(lua_State *inLuaState);
    static int Transpose(lua_State *inLuaState);

protected: // Format
    static int ToString(lua_State *inLuaState);

protected: // Hidden Helpers
    static RuntimeMatrix &CreateMatrix(lua_State *inLuaState);

public: // Helpers
    static RuntimeMatrix &CheckMatrix(lua_State *inLuaState, const INT32 inIndex = 1);
};

} // namespace Q3DStudio
