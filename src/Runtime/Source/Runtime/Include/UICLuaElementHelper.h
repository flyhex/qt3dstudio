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
#include "UICLuaIncludes.h"

namespace qt3ds {
namespace runtime {
    class IApplication;
}
}
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

class IPresentation;
//==============================================================================
/**
 *	Helper class handling element operations
 */
class CLuaElementHelper
{
    //==============================================================================
    //	Methods
    //==============================================================================
private: // Constructors
    CLuaElementHelper();
    CLuaElementHelper(const CLuaElementHelper &);
    CLuaElementHelper &operator=(const CLuaElementHelper &);
    virtual ~CLuaElementHelper();

public: // Lua Global Functions
    static TElement *GetElement(qt3ds::runtime::IApplication &inApplication,
                                IPresentation *inDefaultPresentation, const char *inPath,
                                TElement *inStartElement = NULL);
    static int GetElement(lua_State *inLuaState);
    static int GetElementType(lua_State *inLuaState);
    static int GetAttribute(lua_State *inLuaState);
    static int SetAttribute(lua_State *inLuaState);
    // Set attribute, expecting the attribute value to be top of the stack.
    static bool SetAttribute(lua_State *inLuaState, TElement *inElement, const char *inAttribute,
                             bool inDelay);
    static int HasAttribute(lua_State *inLuaState);
    static int GetChildren(lua_State *inLuaState);
    static int GetCurrentSlide(lua_State *inLuaState);
    static int GetTime(lua_State *inLuaState);
    static int CalculateBoundingBox(lua_State *inLuaState);
    static int CalculateGlobalTransform(lua_State *inLuaState);
};

} // namespace Q3DStudio
