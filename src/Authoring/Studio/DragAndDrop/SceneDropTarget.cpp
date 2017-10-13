/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "SceneDropTarget.h"
#include "DropTarget.h"
#include "DropSource.h"

#include "StudioApp.h"
#include "Doc.h"

#include "HotKeys.h"
#include "IDropTargetHelper.h"
#include "Core.h"
#include "GraphUtils.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "UICDMDataCore.h"
#include "UICDMSlides.h"

// Sceneview stuff
//===============================================================================
/**
 * 	Constructor.
 */
CSceneViewDropTarget::CSceneViewDropTarget()
    : m_DropTime(-1)
{
    m_ObjectType = OBJTYPE_LAYER;
    m_DropSourceObjectType = OBJTYPE_UNKNOWN;
}

//===============================================================================
/**
 * 	 This will get the objec ttype from the Asset.
 *	 Note: The asset can change all of the time, so i always ask the asset for its type.
 *	@return the Studio object type.
 */
long CSceneViewDropTarget::GetObjectType()
{
    qt3dsdm::CUICDMInstanceHandle theInstance = GetInstance();

    if (theInstance.Valid()) {
        CClientDataModelBridge *theBridge =
            g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
        m_ObjectType = theBridge->GetObjectType(theInstance);
        return m_ObjectType;
    }
    return OBJTYPE_UNKNOWN;
}

//===============================================================================
/**
 * 	This get called on every DragWithin.
 *	Note: the source will validate the target instead of the otherway around.
 *	This is because the DropSource knows how to get information from itself without
 *	creating an asset. This is mainly for DropSources that have a lazy creation idiom.
 *  like files.
 *	Dropping into "locked" layers are not allowed.
 *	@param inSource	the DropSource in question.
 *	@return true if the DropSource likes the DropTarget.
 */
bool CSceneViewDropTarget::Accept(CDropSource &inSource)
{
    CClientDataModelBridge *theBridge =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();

    // We have to set this so we can adjust the Target to accept this source.
    SetDropSourceObjectType(inSource.GetObjectType());

    bool theAcceptable = false;

    // We don't want to generate an asset right now so let the DropSource ask us if it can drop.
    theAcceptable = inSource.ValidateTarget(this);

    // The DropSource already generated the asset for this in the above.
    if (theAcceptable && m_Instance.Valid()) {
        theAcceptable = !theBridge->IsLockedAtAll(m_Instance);
    }

    return theAcceptable;
}

//===============================================================================
/**
 * 	 This is so the questioned object type can be cached so we can get the correct asset.
 *	@param inObjType the object type of the Questioned object.
 */
void CSceneViewDropTarget::SetDropSourceObjectType(long inObjType)
{
    m_DropSourceObjectType = inObjType;
}

//===============================================================================
/**
 *	This is where is actually happens.
 *	Note: At this point either everything should be verified, and setup in the dropsource.
 *	Then the only thing left to do is to get the Assets and move/copy or connect them.
 *	Or the dropsource needs the target to perform the actual drop.
 *	Note that if the Control key is depressed, the start time follows the current view time(
 *i.e. playhead position )
 *  And if the Alt key (KEY_MENU) is depressed, the object is dropped at the mouse location.
 *
 *	@param inSource the Object in question.
 *	@return true if the drop was successful .
 */
bool CSceneViewDropTarget::Drop(CDropSource &inSource)
{
    // The Parent is a tree control item, so iwe know it can be converted to an Asset.

    // We have to set this so we can adjust the Target to accept this source.
    SetDropSourceObjectType(inSource.GetObjectType());

    qt3dsdm::CUICDMInstanceHandle theTargetInstance = GetInstance();
    if (theTargetInstance.Valid()) {
        CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
        qt3dsdm::ISlideSystem *theSlideSystem = theDoc->GetStudioSystem()->GetSlideSystem();
        qt3dsdm::CUICDMSlideHandle theSlide = theDoc->GetActiveSlide();
        if (!theSlideSystem->IsMasterSlide(theSlide)
            && (inSource.GetCurrentFlags() & CHotKeys::MODIFIER_ALT)) {
            if (CanAddToMaster()) {
                qt3dsdm::CUICDMSlideHandle theMasterSlideHandle =
                    theSlideSystem->GetMasterSlide(theSlide);
                if (theMasterSlideHandle.Valid())
                    theSlide = theMasterSlideHandle;
            }
        }
        CCmd *command =
            inSource.GenerateAssetCommand(theTargetInstance, EDROPDESTINATION_ON, theSlide);
        if (command != nullptr)
            theDoc->GetCore()->ExecuteCommand(command);
    }

    return true;
}

//===============================================================================
/**
 * 	 @return the Asset that we would like the DropSource to drop on to.
 */
qt3dsdm::CUICDMInstanceHandle CSceneViewDropTarget::GetInstance()
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::CUICDMInstanceHandle theRootObject = theDoc->GetActiveRootInstance();
    EStudioObjectType theRootObjType =
        theDoc->GetStudioSystem()->GetClientDataModelBridge()->GetObjectType(theRootObject);

    // Check if the inObjectType can just go ahead and drop onto the Root object.
    if (CStudioObjectTypes::AcceptableParent((EStudioObjectType)m_DropSourceObjectType,
                                             theRootObjType)) {
        m_Instance = theRootObject;
    } else if (theRootObject == theDoc->GetSceneInstance()
               && CStudioObjectTypes::AcceptableParent((EStudioObjectType)m_DropSourceObjectType,
                                                       OBJTYPE_LAYER)) {
        m_Instance = theDoc->GetActiveLayer();
    }

    return m_Instance;
}

//===============================================================================
/**
 *		Check to see if the Asset is a relative of our asset.
 * 		@return true if the inAsset is a parent grandparent...etc. of this asset.
 */
bool CSceneViewDropTarget::IsRelative(qt3dsdm::CUICDMInstanceHandle inInstance)
{
    bool theReturn = false;

    qt3dsdm::CUICDMInstanceHandle theThisInstance = GetInstance();
    // This will check to see if the inAsset is already a parent, grandparent....etc.
    if (theThisInstance.Valid())
        theReturn = IsAscendant(theThisInstance, inInstance,
                                g_StudioApp.GetCore()->GetDoc()->GetAssetGraph());
    return theReturn;
}

//===============================================================================
/**
 * 	 Check to see if the inAsset is our asset.
 *	 @param inAsset The Asset to check.
 *	 @return true if we are the same.
 */
bool CSceneViewDropTarget::IsSelf(qt3dsdm::CUICDMInstanceHandle inInstance)
{
    qt3dsdm::CUICDMInstanceHandle theThisInstance = GetInstance();
    return (theThisInstance == inInstance);
}

//===============================================================================
/**
 * 	 Set the Drop time for all sources.
 *	 @param inDropTime The time to drop the source.
 */
void CSceneViewDropTarget::SetDropTime(long inDropTime)
{
    m_DropTime = inDropTime;
}

//===============================================================================
/**
 *	 @return The time that all sources will be droped.
 */
long CSceneViewDropTarget::GetDropTime()
{
    return m_DropTime;
}

// Last Sceneview related stuff.
