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
#include "Qt3DSLuaSceneHelper.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSIScene.h"
#include "Qt3DSLuaVector.h"
#include "Qt3DSLuaMatrix.h"
#include "Qt3DSBoundingBox.h"
#include "Qt3DSMatrix.h"
#include "Qt3DSVector3.h"

#include "Qt3DSDataLogger.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const INT32 RECURSION_LIMIT = 2;

//==============================================================================
/**
 *	Calculates the global opacity of a given element
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the calculated global opacity that is pushed onto the stack
 *				or nil if what's passed in is not a node in the scene graph.
 */
int CLuaSceneHelper::CalculateGlobalOpacity(lua_State *inLuaState)
{
    const INT32 ARG_ELEMENT = 1;

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    // TODO: Update the code to use the new UICRender's SNode
    Q3DStudio_ASSERT(false);
    /*
    TElement* theElement = reinterpret_cast<TElement*>( lua_touserdata( inLuaState, ARG_ELEMENT ) );

    // Grab the associated asset
    SAsset* theAsset = reinterpret_cast<SAsset*>( theElement->GetAssociation( ) );

    // Is this really an Element/Asset? (reinterpret_cast is scary)
    Q3DStudio_ASSERT( theElement->GetType() < Q3DStudio::ELEMENTTYPECOUNT );
    Q3DStudio_ASSERT( theAsset->m_Type      < Q3DStudio::ASSETTYPECOUNT   );

    // If the asset is a node, multiply up the chain, otherwise return nil
    if( theAsset->m_NodeFlag )
    {
            FLOAT theOutOpacity = -1;
            SNode* theNode = static_cast<SNode*>( theAsset );
            theOutOpacity = theNode->m_LocalOpacity;
            while ( theNode->m_Parent )
            {
                    theNode = theNode->m_Parent;
                    theOutOpacity *= theNode->m_LocalOpacity;
            }

            // The opacities in the scene graph are stored [0,1]
            theOutOpacity *= 100;
            lua_pushnumber( inLuaState, theOutOpacity );
    }
    else
    {
            lua_pushnil( inLuaState );
    }
    */
    return 1;
}

//==============================================================================
/**
 *	Calculates the bounding box of itself and including all it's children's
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 2 - the min point and the max point of the bounding box was pushed
 *				on the stack
 */
int CLuaSceneHelper::CalculateBoundingBox(lua_State *inLuaState)
{
    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_MIN = 2;
    const INT32 LOCAL_ARG_MAX = 3;
    const INT32 ARG_CHILDRENFLAG = 4;

    INT32 theTop = lua_gettop(inLuaState);

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    RuntimeVector3 &theMinVector = CLuaVector::CheckVector(inLuaState, ARG_MIN);
    RuntimeVector3 &theMaxVector = CLuaVector::CheckVector(inLuaState, LOCAL_ARG_MAX);

    BOOL theOnlySelfFlag = false;
    if (theTop == 4) {
        luaL_checktype(inLuaState, ARG_CHILDRENFLAG, LUA_TBOOLEAN);
        theOnlySelfFlag = lua_toboolean(inLuaState, ARG_CHILDRENFLAG) ? true : false;
    }

    CBoundingBox theBoundingBox;
    IPresentation *thePresentation = theElement->GetBelongedPresentation();
    if (thePresentation->GetScene())
        theBoundingBox = thePresentation->GetScene()->GetBoundingBox(theElement, theOnlySelfFlag);

    theMinVector = theBoundingBox.GetMin();
    theMaxVector = theBoundingBox.GetMax();

    lua_pushvalue(inLuaState, ARG_MIN);
    lua_pushvalue(inLuaState, LOCAL_ARG_MAX);

    return 2;
}

int CLuaSceneHelper::CalculateLocalBoundingBox(lua_State *inLuaState)
{
    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_MIN = 2;
    const INT32 LOCAL_ARG_MAX = 3;
    const INT32 ARG_CHILDRENFLAG = 4;

    INT32 theTop = lua_gettop(inLuaState);

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    RuntimeVector3 &theMinVector = CLuaVector::CheckVector(inLuaState, ARG_MIN);
    RuntimeVector3 &theMaxVector = CLuaVector::CheckVector(inLuaState, LOCAL_ARG_MAX);

    BOOL theOnlySelfFlag = false;
    if (theTop == 4) {
        luaL_checktype(inLuaState, ARG_CHILDRENFLAG, LUA_TBOOLEAN);
        theOnlySelfFlag = lua_toboolean(inLuaState, ARG_CHILDRENFLAG) ? true : false;
    }

    CBoundingBox theBoundingBox;
    IPresentation *thePresentation = theElement->GetBelongedPresentation();
    if (thePresentation->GetScene())
        theBoundingBox =
            thePresentation->GetScene()->GetLocalBoundingBox(theElement, theOnlySelfFlag);

    theMinVector = theBoundingBox.GetMin();
    theMaxVector = theBoundingBox.GetMax();

    lua_pushvalue(inLuaState, ARG_MIN);
    lua_pushvalue(inLuaState, LOCAL_ARG_MAX);

    return 2;
}

//==============================================================================
/**
 *	Calculates the global transform of a given element
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the calculated global transform that was pushed onto the stack.
 */
int CLuaSceneHelper::CalculateGlobalTransform(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_CALCULATEGLOBALTRANSFORM);

    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_MATRIX = 2;

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    RuntimeMatrix &theGlobalTransform = CLuaMatrix::CheckMatrix(inLuaState, ARG_MATRIX);
    theGlobalTransform.Identity();

    IScene *theScene = theElement->GetBelongedPresentation()->GetScene();

    // This return a matrix in the right hand coordinate system.
    theScene->CalculateGlobalTransform(theElement, theGlobalTransform);

    // Change to a left hand coordinate system
    theGlobalTransform.FlipCoordinateSystem();

    lua_pushvalue(inLuaState, ARG_MATRIX);
    return 1;
}

//==============================================================================
/**
 *	Calculates the global transform of a given element
 *	@param inLuaState		the Lua state, required for interaction with lua
 *	@return 1 - the calculated global transform that was pushed onto the stack.
 */
int CLuaSceneHelper::SetLocalTransformMatrix(lua_State *inLuaState)
{
    PerfLogLuaEvent1(DATALOGGER_LUA_SETLOCALTRANSFORMMATRIX);

    const INT32 ARG_ELEMENT = 1;
    const INT32 ARG_MATRIX = 2;

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    RuntimeMatrix &theLocalTransform = CLuaMatrix::CheckMatrix(inLuaState, ARG_MATRIX);

    IScene *theScene = theElement->GetBelongedPresentation()->GetScene();
    theScene->SetLocalTransformMatrix(theElement, theLocalTransform);

    lua_pushvalue(inLuaState, ARG_ELEMENT);
    return 1;
}

//==============================================================================
/**
 *	Get the image info from the renderer
 */
int CLuaSceneHelper::GetImageInfo(lua_State *inLuaState)
{
    const INT32 ARG_ELEMENT = 1;

    INT32 theWidth = 0;
    INT32 theHeight = 0;

    luaL_checktype(inLuaState, ARG_ELEMENT, LUA_TLIGHTUSERDATA);
    TElement *theElement = reinterpret_cast<TElement *>(lua_touserdata(inLuaState, ARG_ELEMENT));

    if (theElement) {
        IPresentation *thePresentation = theElement->GetBelongedPresentation();

        if (thePresentation && thePresentation->GetScene())
            thePresentation->GetScene()->GetImageInfoFromRenderEngine(theElement, theWidth,
                                                                      theHeight);
        else
            qCCritical(qt3ds::INVALID_OPERATION) << "GetImageInfo failed to get a scene//presentation";
    } else {
        qCCritical(qt3ds::INVALID_OPERATION) << "GetImageInfo wasn't passed an image element";
    }

    lua_pushnumber(inLuaState, theWidth);
    lua_pushnumber(inLuaState, theHeight);

    // we pushed 2 numbers to lua
    return 2;
}

} // namespace Q3DStudio
