/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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

#include "stdafx.h"
#include "TimelineBreadCrumbProvider.h"
#include "Core.h"

// Link to data model
#include "Doc.h"
#include "StudioApp.h"
#include "Cmd.h"
#include "ResourceCache.h"
#include "Strings.h"
#include "StringLoader.h"
#include "CColor.h"

#include "ClientDataModelBridge.h"
#include "UICDMStudioSystem.h"
#include "UICDMSlides.h"
#include "CmdActivateSlide.h"

using namespace qt3dsdm;

//=============================================================================
/**
 *	Constructor
 */
CTimelineBreadCrumbProvider::CTimelineBreadCrumbProvider(CDoc *inDoc)
    : m_Doc(inDoc)
{
}

//=============================================================================
/**
 */
CTimelineBreadCrumbProvider::~CTimelineBreadCrumbProvider()
{
}

//=============================================================================
/**
 * determine the color and text string for this breadcrumb
 */
static inline void FillBreadCrumb(SBreadCrumb &outBreadCrumb,
                                  qt3dsdm::CUICDMInstanceHandle inInstance, CDoc *inDoc)
{
    // Get the MasterSlide Handle associated with inAsset
    CClientDataModelBridge *theBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();
    ISlideSystem *theSlideSystem = inDoc->GetStudioSystem()->GetSlideSystem();
    Q3DStudio::CId theId = theBridge->GetGUID(inInstance);
    qt3dsdm::CUICDMSlideHandle theMasterSlide =
        theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theId));
    ASSERT(theMasterSlide.Valid()); // it should be valid because inAsset should be OBJTYPE_SCENE or
                                    // non-library OBJTYPE_COMPONENT

    // Get the active slide index of the master slide. Master Slide always has index 0
    long theActiveIndex = theSlideSystem->GetActiveSlideIndex(theMasterSlide);
    bool theIsMaster = (theActiveIndex == 0);

    // Determine the color
    outBreadCrumb.m_Color =
        theIsMaster ? CColor(0, 0, 255) : CColor(0, 0, 0); // blue for master, black otherwise

    // Determine the text string
    outBreadCrumb.m_String = theBridge->GetName(inInstance).toQString();
    outBreadCrumb.m_String += " (";
    if (theIsMaster)
        outBreadCrumb.m_String += ::LoadResourceString(IDS_OBJTYPE_MASTER).toQString();
    else {
        CUICDMSlideHandle theActiveSlide =
            theSlideSystem->GetSlideByIndex(theMasterSlide, theActiveIndex);
        CUICDMInstanceHandle theInstanceHandle = theSlideSystem->GetSlideInstance(theActiveSlide);
        ASSERT(theInstanceHandle.Valid());
        outBreadCrumb.m_String += theBridge->GetName(theInstanceHandle).toQString();
    }
    outBreadCrumb.m_String += ")";
}

//=============================================================================
/**
 * return the trail of breadcrumb.
 * This constructs a list of the "time context tree" from Scene down to the current active time
 * context.
 * @param inRefresh true to refresh the list, false to get existing.
 */
CTimelineBreadCrumbProvider::TTrailList
CTimelineBreadCrumbProvider::GetTrail(bool inRefresh /*= true */)
{
    if (inRefresh)
        RefreshSlideList();

    TTrailList theList;
    for (size_t theIndex = 0; theIndex < m_BreadCrumbList.size(); ++theIndex) {
        SBreadCrumb theBreadCrumb;
        FillBreadCrumb(theBreadCrumb, m_BreadCrumbList[theIndex], m_Doc);
        theList.push_back(theBreadCrumb);
    }
    return theList;
}

//=============================================================================
/**
 * switch current time context to the one 'represented' by the breadcrumbs.
 * @param inTrailIndex index into the trail list
 */
void CTimelineBreadCrumbProvider::OnBreadCrumbClicked(long inTrailIndex)
{
    if (inTrailIndex >= 0 && inTrailIndex < (long)m_BreadCrumbList.size()) {
        CCmdActivateSlide *theCmd = new CCmdActivateSlide(m_Doc, m_BreadCrumbList[inTrailIndex]);
        theCmd->SetForceRefresh(false);
        m_Doc->GetCore()->ExecuteCommand(theCmd, false);
    }
}

QPixmap CTimelineBreadCrumbProvider::GetRootImage() const
{
    return CResourceCache::GetInstance()->GetBitmap("breadcrumb_component_scene.png");
}

QPixmap CTimelineBreadCrumbProvider::GetBreadCrumbImage() const
{
    return CResourceCache::GetInstance()->GetBitmap("breadcrumb_component_button.png");
}

QPixmap CTimelineBreadCrumbProvider::GetSeparatorImage() const
{
    return CResourceCache::GetInstance()->GetBitmap("breadcrumb_component_colon_button.png");
}

QPixmap CTimelineBreadCrumbProvider::GetActiveBreadCrumbImage() const
{
    return CResourceCache::GetInstance()->GetBitmap("breadcrumb_component_grey_button.png");
}

//=============================================================================
/**
 * Called when active time context is changed.
 */
void CTimelineBreadCrumbProvider::RefreshSlideList()
{
    ClearSlideList();

    qt3dsdm::CUICDMInstanceHandle theActiveRoot = m_Doc->GetActiveRootInstance();
    if (!theActiveRoot.Valid())
        return;
    FillSlideList(theActiveRoot);
}

//=============================================================================
/**
 * Callback that inAsset has its name changed, fire off a signal to the UI control.
 * All the assets' signals are connected to this object and we'll let the UI control check iterate
 * through the list for changes and refresh.
 * Alternative we can set up additional classes that listens to specific assets and only the asset
 * affected refreshed. the former is easier for now.
 */
void CTimelineBreadCrumbProvider::OnNameDirty()
{
    SigBreadCrumbUpdate();
}

void CTimelineBreadCrumbProvider::ClearSlideList()
{
    m_Connections.clear();
    m_BreadCrumbList.clear();
}

//=============================================================================
/**
 * This will recurse up the time context tree, so that we can fill the list in a top-down (i.e
 * Scene) first manner
 */
void CTimelineBreadCrumbProvider::FillSlideList(qt3dsdm::CUICDMInstanceHandle inInstance)
{
    if (!inInstance.Valid())
        return;

    CClientDataModelBridge *theBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    Q3DStudio::CId theId = theBridge->GetGUID(inInstance);

    // Recurse
    FillSlideList(theBridge->GetParentComponent(inInstance));

    m_BreadCrumbList.push_back(inInstance);

    CUICDMPropertyHandle theNameProp =
        m_Doc->GetStudioSystem()->GetClientDataModelBridge()->GetNameProperty();
    IStudioFullSystemSignalProvider *theEngine =
        m_Doc->GetStudioSystem()->GetFullSystemSignalProvider();
    std::function<void(CUICDMInstanceHandle, CUICDMPropertyHandle)> theSetter(
        std::bind(&CTimelineBreadCrumbProvider::OnNameDirty, this));

    // Listen to name changes on the Asset
    m_Connections.push_back(theEngine->ConnectInstancePropertyValue(
        std::bind(qt3dsdm::MaybackCallbackInstancePropertyValue<std::function<void(
                        CUICDMInstanceHandle, CUICDMPropertyHandle)>>,
                    std::placeholders::_1, std::placeholders::_2, inInstance, theNameProp, theSetter)));

    // Listen to name changes on the non-master Slides
    qt3dsdm::CUICDMSlideHandle theMasterSlide =
        theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theId));
    long theSlideCount = (long)theSlideSystem->GetSlideCount(theMasterSlide);

    for (long theIndex = 1; theIndex < theSlideCount; ++theIndex) {
        CUICDMSlideHandle theSlide = theSlideSystem->GetSlideByIndex(theMasterSlide, theIndex);
        CUICDMInstanceHandle theSlideInstance = theSlideSystem->GetSlideInstance(theSlide);
        m_Connections.push_back(theEngine->ConnectInstancePropertyValue(
            std::bind(qt3dsdm::MaybackCallbackInstancePropertyValue<std::function<void(
                            CUICDMInstanceHandle, CUICDMPropertyHandle)>>,
                        std::placeholders::_1, std::placeholders::_2, theSlideInstance, theNameProp, theSetter)));
    }
}
