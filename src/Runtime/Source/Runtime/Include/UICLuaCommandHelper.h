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
#include "UICIComponentManager.h"
#include "UICLuaEngine.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CPresentation;
class IElementManager;

//==============================================================================
/**
 *	Helper class handling command operations
 */
class CLuaCommandHelper
{
    //==============================================================================
    //	Methods
    //==============================================================================
private: // Constructors
    CLuaCommandHelper();
    CLuaCommandHelper(const CLuaCommandHelper &);
    CLuaCommandHelper &operator=(const CLuaCommandHelper &);
    virtual ~CLuaCommandHelper();

public: // Lua Global Functions
    static int Play(lua_State *inLuaState);
    static int Pause(lua_State *inLuaState);
    static int GoToTime(lua_State *inLuaState);
    static int GoToSlide(lua_State *inLuaState);
    static int GoToNextSlide(lua_State *inLuaState);
    static int GoToPreviousSlide(lua_State *inLuaState);
    static int GoToBackSlide(lua_State *inLuaState);

    static bool SetupGotoSlideCommand(TElement &inElement, const char *slideName,
                                      const SScriptEngineGotoSlideArgs &inArgs);
    static bool SetupGotoSlideCommand(TElement &inElement, Q3DStudio::INT32 inSlide,
                                      const SScriptEngineGotoSlideArgs &inArgs);

protected: // Static Hidden Helpers
    static TElement *FireCommand(lua_State *inLuaState, TEventCommandHash inCommand);

public: // Static Helpers
    static TElement *GetComponentParent(TElement *inParent);
};

} // namespace Q3DStudio
