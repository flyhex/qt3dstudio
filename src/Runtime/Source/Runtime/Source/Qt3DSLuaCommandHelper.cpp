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
#include "Qt3DSLuaCommandHelper.h"
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSApplication.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSSlideSystem.h"

//==============================================================================
// Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
CLuaCommandHelper::CLuaCommandHelper()
{
}

//==============================================================================
/**
 *	Destructor
 */
CLuaCommandHelper::~CLuaCommandHelper()
{
}

//==============================================================================
/**
 *	Start the component time
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::Play(lua_State *inLuaState)
{
    lua_pushlightuserdata(inLuaState, FireCommand(inLuaState, COMMAND_PLAY));
    return 1;
}

//==============================================================================
/**
 *	Pause the component time
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::Pause(lua_State *inLuaState)
{
    lua_pushlightuserdata(inLuaState, FireCommand(inLuaState, COMMAND_PAUSE));
    return 1;
}

//==============================================================================
/**
 *	Go to the time specified
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::GoToTime(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_GOTOTIME);
    lua_pushlightuserdata(inLuaState, FireCommand(inLuaState, COMMAND_GOTOTIME));
    return 1;
}

//==============================================================================
/**
 *	Go to the slide specified
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::GoToSlide(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_GOTOSLIDE);
    FireCommand(inLuaState, COMMAND_GOTOSLIDE);
    return 0;
}

//==============================================================================
/**
 *	Go to the 1 slide after the current slide
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::GoToNextSlide(lua_State *inLuaState)
{
    lua_pushlightuserdata(inLuaState, FireCommand(inLuaState, COMMAND_GOTONEXTSLIDE));
    return 1;
}

//==============================================================================
/**
 *	Go to the 1 slide before the current slide
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::GoToPreviousSlide(lua_State *inLuaState)
{
    lua_pushlightuserdata(inLuaState, FireCommand(inLuaState, COMMAND_GOTOPREVIOUSSLIDE));
    return 1;
}

//==============================================================================
/**
 *	Go to the slide before coming to the current slide
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return the number of items pushed onto the stack
 */
int CLuaCommandHelper::GoToBackSlide(lua_State *inLuaState)
{
    lua_pushlightuserdata(inLuaState, FireCommand(inLuaState, COMMAND_BACKSLIDE));
    return 1;
}

static Option<Q3DStudio::UINT8> FindSlide(TElement &inElement, const char *slideName)
{
    IPresentation *thePresentation = inElement.GetBelongedPresentation();

    int theSlideHashName = thePresentation->GetApplication().HashString(slideName);
    TComponent *theComponent = thePresentation->GetComponentManager().GetComponent(&inElement);
    UINT8 theSlideIndex =
        thePresentation->GetSlideSystem().FindSlide(*theComponent, theSlideHashName);
    if (theSlideIndex >= theComponent->GetSlideCount()) {
        qt3ds::foundation::CRegisteredString elemPath = thePresentation->GetElementPath(inElement);
        if (elemPath.IsValid()) {
            qCCritical(qt3ds::INVALID_PARAMETER)
                    << "LuaCommandHelper: goToSlide: Unable to find slide "
                    << slideName << " on element " << elemPath.c_str();
        } else {
            qCCritical(qt3ds::INVALID_PARAMETER)
                    << "LuaCommandHelper: goToSlide: Unable to find slide " << slideName;
        }
        return Empty();
    }

    return theSlideIndex;
}

bool CLuaCommandHelper::SetupGotoSlideCommand(TElement &inElement, Q3DStudio::INT32 inSlideIndex,
                                              const SScriptEngineGotoSlideArgs &inArgs)
{
    IPresentation *thePresentation = inElement.GetBelongedPresentation();
    TElement *theTarget = GetComponentParent(&inElement);
    SComponentGotoSlideData theSlideData(inSlideIndex);
    theSlideData.m_Mode = inArgs.m_Mode;
    theSlideData.m_Paused = inArgs.m_Paused;
    theSlideData.m_Rate = inArgs.m_Rate;
    theSlideData.m_Reverse = inArgs.m_Reverse;
    theSlideData.m_StartTime = inArgs.m_StartTime;
    // Resolve playthroughto if it has a valid value.
    if (!isTrivial(inArgs.m_PlaythroughTo)) {
        if (AreEqual(inArgs.m_PlaythroughTo, "next"))
            theSlideData.m_PlaythroughTo = -1;
        else if (AreEqual(inArgs.m_PlaythroughTo, "previous"))
            theSlideData.m_PlaythroughTo = -2;
        else {
            // Find the slide if possible.  If not, then just error leave things as they are.

            Option<UINT8> theSlideIndex = FindSlide(inElement, inArgs.m_PlaythroughTo);
            if (theSlideIndex.hasValue())
                theSlideData.m_PlaythroughTo = *theSlideIndex;
        }
    }
    thePresentation->GetComponentManager().SetupComponentGotoSlideCommand(theTarget, theSlideData);
    UVariant theArg1;
    UVariant theArg2;
    qt3ds::intrinsics::memZero(&theArg1, sizeof(UVariant));
    qt3ds::intrinsics::memZero(&theArg2, sizeof(UVariant));
    thePresentation->FireCommand(COMMAND_GOTOSLIDE, theTarget, &theArg1, &theArg2);
    return true;
}

bool CLuaCommandHelper::SetupGotoSlideCommand(TElement &inElement, const char *slideName,
                                              const SScriptEngineGotoSlideArgs &inArgs)
{
    Option<UINT8> theSlideIndex = FindSlide(inElement, slideName);
    if (theSlideIndex.hasValue())
        return SetupGotoSlideCommand(inElement, *theSlideIndex, inArgs);
    return false;
}
//==============================================================================
/**
 *	Fires commands to components
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@param inCommand		the command to fire into the presentation
 *	@return the component that the command was fired upon
 */
TElement *CLuaCommandHelper::FireCommand(lua_State *inLuaState, TEventCommandHash inCommand)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_FIRECOMMAND);

    const INT32 ARG_TARGET_ELEMENT = 1;
    const INT32 ARG_1 = 2;

    luaL_checktype(inLuaState, ARG_TARGET_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theTarget =
        reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_TARGET_ELEMENT));
    UVariant theArg1;
    UVariant theArg2;

    IPresentation *thePresentation = theTarget->GetBelongedPresentation();
    if (theTarget->GetActive()) {
        if (inCommand == COMMAND_GOTOTIME) {
            luaL_checktype(inLuaState, ARG_1, LUA_TNUMBER);

            theArg1.m_INT32 = static_cast<INT32>(lua_tonumber(inLuaState, ARG_1) * 1000);
            // NVLogInfo( "CLuaCommandHelper::FireCommand: GoToTime", "%d", theArg1.m_INT32 );
        } else if (inCommand == COMMAND_GOTOSLIDE) {
            TElement *theComponent = GetComponentParent(theTarget);
            if (lua_isnumber(inLuaState, ARG_1))
                SetupGotoSlideCommand(*theComponent, lua_tointeger(inLuaState, ARG_1),
                                      SScriptEngineGotoSlideArgs());
            else if (lua_isstring(inLuaState, ARG_1))
                SetupGotoSlideCommand(*theComponent, lua_tostring(inLuaState, ARG_1),
                                      SScriptEngineGotoSlideArgs());
            else
                luaL_error(inLuaState, "goToSlide: Invalid type for Argument#2");
            return theTarget;
        }

        theTarget = GetComponentParent(theTarget);

        thePresentation->FireCommand(inCommand, theTarget, &theArg1, &theArg2);
    }

    return theTarget;
}

//==============================================================================
/**
 *	Checks if the given element is a component. If it isn't walks up the tree and looks for one.
 *	@param inElementManager		the object that manages the elements in the presentation
 *	@param inElement			the element to check if it's a component
 *	@return the component
 */
TElement *CLuaCommandHelper::GetComponentParent(TElement *inElement)
{
    Q3DStudio_ASSERT(inElement);
    return &inElement->GetComponentParent();
}

} // namespace Q3DStudio
