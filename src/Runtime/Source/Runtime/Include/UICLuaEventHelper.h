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

//==============================================================================
//	Studio Includes
//==============================================================================
#include "UICLuaEngine.h"
#include "UICEvent.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Helper class handling event operations
 */
class CLuaEventHelper
{
    //==============================================================================
    //	Methods
    //==============================================================================
private: // Constructors
    CLuaEventHelper();
    CLuaEventHelper(const CLuaEventHelper &);
    CLuaEventHelper &operator=(const CLuaEventHelper &);
    virtual ~CLuaEventHelper();

public: // Lua Global Functions
    static int FireEvent(lua_State *inLuaState);
    static int RegisterForEvent(lua_State *inLuaState);
    static int UnRegisterForEvent(lua_State *inLuaState);
    // static int				RegisterForChange( lua_State* inLuaState );
    // static int				UnRegisterForChange( lua_State* inLuaState );

protected: // Callbacks
    static void EventCallback(void *inContextData, SEventCommand &ioEvent);
    static void ChangeCallback(const SAttributeKey &inAttributeKey, UVariant inOldValue,
                               UVariant inNewValue, void *inContextData);

protected: // Helper
    static INT32 FindCallbackData(lua_State *inLuaState, CLuaEngine::TCallbackDataList *inDataList,
                                  CLuaEngine::SCallbackData *inCriteria,
                                  const INT32 inFunctionIndex);
    static void EventRegistration(lua_State *inLuaState, const INT32 inRegister);
    // static void				ChangeRegistration( lua_State* inLuaState, const INT32 inRegister
    // );
    static BOOL IsFunctionInSelfTable(lua_State *inLuaState, const INT32 inFunctionIndex,
                                      const INT32 inSelfIndex);
    static void PushSelfOnTop(lua_State *inLuaState, const INT32 inSelfIndex);
    static TElement *GetSelfElement(lua_State *inLuaState, const INT32 inSelfIndex);
};

} // namespace Q3DStudio
