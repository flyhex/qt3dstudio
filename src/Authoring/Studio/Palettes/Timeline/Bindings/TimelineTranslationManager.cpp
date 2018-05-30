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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "TimelineTranslationManager.h"
#include "SlideTimelineItemBinding.h"
#include "GroupTimelineItemBinding.h"
#include "BehaviorTimelineItemBinding.h"
#include "MaterialTimelineItemBinding.h"
#include "ImageTimelineItemBinding.h"
#include "PathAnchorPointTimelineItemBinding.h"
#include "PathTimelineItemBinding.h"
#include "LayerTimelineItemBinding.h"
#include "IDoc.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"

// Link to Data model
#include "ClientDataModelBridge.h"
#include "Qt3DSDMDataCore.h"
#include "Doc.h" //Because we need to access Client Data Model Bridge
#include "StudioApp.h"
#include "Core.h"

using namespace qt3dsdm;

CTimelineTranslationManager::CTimelineTranslationManager()
{
}

CTimelineTranslationManager::~CTimelineTranslationManager()
{
    // clean up all bindings
    Clear();
}

ITimelineItemBinding *CTimelineTranslationManager::GetOrCreate(Qt3DSDMInstanceHandle inInstance)
{
    ITimelineItemBinding *theBinding = GetBinding(inInstance);
    if (!theBinding) {
        Qt3DSDMTimelineItemBinding *theReturn = nullptr;
        qt3dsdm::IPropertySystem *thePropertySystem = GetStudioSystem()->GetPropertySystem();
        Qt3DSDMPropertyHandle theTypeProperty =
            thePropertySystem->GetAggregateInstancePropertyByName(inInstance, L"type");

        SValue theTypeValue;
        thePropertySystem->GetInstancePropertyValue(inInstance, theTypeProperty, theTypeValue);

        std::wstring theWideTypeString(qt3dsdm::get<TDataStrPtr>(theTypeValue)->GetData());

        if (theWideTypeString == L"Material" || theWideTypeString == L"CustomMaterial"
            || theWideTypeString == L"ReferencedMaterial")
            theReturn = new CMaterialTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Image")
            theReturn = new CImageTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Group" || theWideTypeString == L"Component")
            theReturn = new CGroupTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Behavior")
            theReturn = new CBehaviorTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Slide")
            theReturn = new CSlideTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"PathAnchorPoint")
            theReturn = new CPathAnchorPointTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Path")
            theReturn = new CPathTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Layer")
            theReturn = new CLayerTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Model" || theWideTypeString == L"Text"
                 || theWideTypeString == L"Camera" || theWideTypeString == L"Effect"
                 || theWideTypeString == L"Light" || theWideTypeString == L"RenderPlugin"
                 || theWideTypeString == L"Alias" || theWideTypeString == L"SubPath")
            theReturn = new Qt3DSDMTimelineItemBinding(this, inInstance);
        else {
            // Add support for additional DataModel types here.
            ASSERT(0);
        }

        m_InstanceHandleBindingMap.insert(
            std::make_pair(theReturn->GetInstanceHandle(), theReturn));
        theBinding = theReturn;
    }

    return theBinding;
}

//==============================================================================
/**
 * Clear all bindings, typically when a presentation is closed.
 */
void CTimelineTranslationManager::Clear()
{
    // clean up all bindings
    m_InstanceHandleBindingMap.clear();
}

/**
 * @return the Binding object that corresponds to this instance.
 */
Qt3DSDMTimelineItemBinding *
CTimelineTranslationManager::GetBinding(Qt3DSDMInstanceHandle inHandle) const
{
    TInstanceHandleBindingMap::const_iterator theIter = m_InstanceHandleBindingMap.find(inHandle);
    if (theIter != m_InstanceHandleBindingMap.end())
        return theIter->second;
    return nullptr;
}

CDoc *CTimelineTranslationManager::GetDoc() const
{
    return dynamic_cast<CDoc *>(g_StudioApp.GetCore()->GetDoc());
}

CStudioSystem *CTimelineTranslationManager::GetStudioSystem() const
{
    // TODO: figure if we can just deal with IDoc instead of CDoc
    return g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
}

