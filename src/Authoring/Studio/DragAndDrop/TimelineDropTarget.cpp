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

#include "Qt3DSCommonPrecompile.h"
#include "TimelineDropTarget.h"
#include "StudioApp.h"
#include "DropSource.h"
#include "HotKeys.h"
#include "Core.h"
#include "Doc.h"
#include "IDropTargetHelper.h"
#include "GraphUtils.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMSlides.h"

// Timeline stuff

//===============================================================================
/**
 * 	This get called on every DragWithin.
 *	Note: the source will validate the target instead of the otherway around.
 *	This is because the DropSource knows how to get information from itself without
 *	creating an asset. This is mainly for DropSources that have a lazy creation idiom.
 *  like files.
 *	@param the DropSource in question.
 *	@return true if the DropSource likes the DropTarget.
 */
bool CTimeLineDropTarget::Accept(CDropSource &inSource)
{
    bool theDropFlag = inSource.ValidateTarget(this);
    return theDropFlag;
}

//===============================================================================
/**
 *  This is where it actually happens.
 *  Note: At this point either everything should be verified, and setup in the dropsource.
 *  Then the only thing left to do is to get the Assets and move/copy or connect them.
 *  Or the dropsource needs the target to perform the actual drop.
 *  Note that if the Control key is depressed, the start time follows the current view time(
 *i.e. playhead position )
 *
 *  @param inSource the Object in question.
 *  @return true if the drop was successful .
 */
bool CTimeLineDropTarget::Drop(CDropSource &inSource)
{
    qt3dsdm::Qt3DSDMInstanceHandle theTargetInstance = GetInstance();

    if (theTargetInstance.Valid()) {
        CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
        qt3dsdm::ISlideSystem *theSlideSystem = theDoc->GetStudioSystem()->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theSlide = theDoc->GetActiveSlide();
        if (!theSlideSystem->IsMasterSlide(theSlide)
            && (inSource.GetCurrentFlags() & CHotKeys::MODIFIER_ALT)) {
            if (CanAddToMaster()) {
                qt3dsdm::Qt3DSDMSlideHandle theMasterSlideHandle =
                    theSlideSystem->GetMasterSlide(theSlide);
                if (theMasterSlideHandle.Valid())
                    theSlide = theMasterSlideHandle;
            }
        }
        CCmd *theCmd = inSource.GenerateAssetCommand(theTargetInstance, m_Destination, theSlide);
        if (theCmd)
            g_StudioApp.GetCore()->ExecuteCommand(theCmd);
    }

    return true;
}

//===============================================================================
/**
 * 	 This will get the objec ttype from the Asset.
 *	 Note: The asset can change all of the time, so i always ask the asset for its type.
 *	@return the Studio object type.
 */
long CTimeLineDropTarget::GetObjectType()
{
    qt3dsdm::Qt3DSDMInstanceHandle theTargetInstance = GetTargetInstance();
    if (theTargetInstance.Valid()) {
        CClientDataModelBridge *theBridge =
            g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
        return theBridge->GetObjectType(theTargetInstance);
    }

    return m_ObjectType;
}

//===============================================================================
/**
 *		Check to see if the Asset is a relative of our asset.
 * 		@return true if the inAsset is a parent grandparent...etc. of this asset.
 */
bool CTimeLineDropTarget::IsRelative(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    bool theReturn = false; ///< Default return value.
    qt3dsdm::Qt3DSDMInstanceHandle theThisInstance = GetInstance();

    // This will check to see if the inAsset is already some sort of parent grandparent....etc.
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
bool CTimeLineDropTarget::IsSelf(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    qt3dsdm::Qt3DSDMInstanceHandle theThisInstance = GetInstance();
    // true if self.....
    return (theThisInstance == inInstance);
}

//===============================================================================
/**
 *	This method is used to detirmine validity for dropping on Master items.
 *	We do not allow Master items to be dropped on non-master targets because
 *	it does not make any hierarchical sense.  Also checks for NULL and Scene object.
 *	(The scene object reports as a master object so that we can re-arrange layers (On the master
 *slide).)
 * Changed checking for scene to checking for root object, which may be a component. This
 * would allow for the root component (as in edit component) to re-arrange its children, even if
 *either
 * party is not a master.
 *	@param inAsset The Asset to check.
 *	@return true if we are the same.
 */
bool CTimeLineDropTarget::IsMaster()
{
    if (!m_Instance.Valid())
        return true;
    else if (m_Instance == g_StudioApp.GetCore()->GetDoc()->GetActiveRootInstance())
        return true;
    else {
        qt3dsdm::ISlideSystem *theSlideSystem =
            g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theSlide = theSlideSystem->GetAssociatedSlide(m_Instance);
        return theSlideSystem->IsMasterSlide(theSlide);
    }
}

void CTimeLineDropTarget::SetDestination(EDROPDESTINATION inDestination)
{
    m_Destination = inDestination;
}

EDROPDESTINATION CTimeLineDropTarget::GetDestination() const
{
    return m_Destination;
}

//===============================================================================
/**
 *	Figure out the destination (parent) asset that the drop is to occur.
 *	In the case that the drop occurs ON this asset, then the to-be-dropped asset becomes a child
 *of m_Asset
 *	Otherwise, this m_Asset ends up being a sibling
 */
qt3dsdm::Qt3DSDMInstanceHandle CTimeLineDropTarget::GetTargetInstance()
{
    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    if (!theInstance.Valid())
        return 0;

    CClientDataModelBridge *theBridge =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    bool theIsActiveComponent = false;
    bool theIsComponent = (theBridge->GetObjectType(theInstance) == OBJTYPE_COMPONENT);
    if (theIsComponent)
        theIsActiveComponent = theBridge->IsActiveComponent(theInstance);

    // If the drop destination is ON, it will be valid if this is not a component or it's in the
    // component timeline
    if (m_Destination == EDROPDESTINATION_ON) {
        if (!theIsComponent || theIsActiveComponent)
            return theInstance;
        else
            return 0;
    }

    // if target is a component, and we want to insert it above/below, and we are viewing this
    // component, then it's an invalid operation
    // thus set the target to 0.
    if (theIsActiveComponent)
        return 0;

    return theBridge->GetParentInstance(theInstance);
}

// LASTTIMELINE RELATED CODE
