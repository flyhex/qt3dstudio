/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#include "TimelineFilter.h"
#include "Bindings/ITimelineItemProperty.h"
#include "Bindings/ITimelineItem.h"

//=============================================================================
/**
 * Constructor
 */
CFilter::CFilter()
    : m_ShowBehaviors(true)
    , m_ShowMaterials(true)
    , m_ShowProperties(true)
    , m_ShowShy(true)
    , m_ShowLocked(true)
    , m_ShowVisible(true)
    , m_IsExpanded(true)
{
}

//=============================================================================
/**
 * Destructor
 */
CFilter::~CFilter()
{
}

//=============================================================================
/**
 * @return true if behaviors should be shown, false if they should be hidden
 */
bool CFilter::GetBehaviors() const
{
    return m_ShowBehaviors;
}

//=============================================================================
/**
 * @return true if materials should be shown, false if they should be hidden
 */
bool CFilter::GetMaterials() const
{
    return m_ShowMaterials;
}

//=============================================================================
/**
 * @return true if properties should be shown, false if they should be hidden
 */
bool CFilter::GetProperties() const
{
    return m_ShowProperties;
}

//=============================================================================
/**
 * @return true if shy objects should still be shown, false if they should be hidden
 */
bool CFilter::GetShy() const
{
    return m_ShowShy;
}

//=============================================================================
/**
 * @return true if locked objects should still be shown, false if they should be hidden
 */
bool CFilter::GetLocked() const
{
    return m_ShowLocked;
}

//=============================================================================
/**
 * Sets whether or not to show behaviors on objects.
 * @param inShow true to show behaviors, false to hide behaviors
 */
void CFilter::SetBehaviors(bool inShow /*= true*/)
{
    m_ShowBehaviors = inShow;
}

//=============================================================================
/**
 * Sets whether or not to show materials on objects.
 * @param inShow true to show materials, false to hide materials
 */
void CFilter::SetMaterials(bool inShow /*= true*/)
{
    m_ShowMaterials = inShow;
}

//=============================================================================
/**
 * Sets whether or not to show properties on objects.
 * @param inShow true to show properties, false to hide properties
 */
void CFilter::SetProperties(bool inShow /*= true*/)
{
    m_ShowProperties = inShow;
}

//=============================================================================
/**
 * Sets whether or not to show objects with the shy flag.
 * @param inShow true to show shy objects, false to hide shy objects
 */
void CFilter::SetShy(bool inShow /*= true*/)
{
    m_ShowShy = inShow;
}

//=============================================================================
/**
 * Sets whether or not to show objects with the locked flag.
 * @param inShow true to show locked objects, false to hide locked objects
 */
void CFilter::SetLocked(bool inShow /*= true*/)
{
    m_ShowLocked = inShow;
}

//=============================================================================
/**
 * Sets whether or not the parent object is expanded.
 * If the parent is not expanded then nothing should be visible.
 */
void CFilter::SetExpanded(bool inIsExpanded /*= true*/)
{
    m_IsExpanded = inIsExpanded;
}

//=============================================================================
/**
 * Sets whether or not to show objects that are not visible.
 * @param inIsVisible true to show non-visible objects, false to hide them.
 */
void CFilter::SetVisible(bool inIsVisible /*= true*/)
{
    m_ShowVisible = inIsVisible;
}

//=============================================================================
/**
 * Gets whether or not non-visible objects should be displayed.
 * @return true if non-visible objects should be displayed.
 */
bool CFilter::GetVisible() const
{
    return m_ShowVisible;
}

//=============================================================================
/**
 * Gets whether or not the parent object is expanded.
 * If the parent is not expanded then nothing should be visible.
 */
bool CFilter::IsExpanded() const
{
    return m_IsExpanded;
}

//=============================================================================
/**
 * Checks to see if the specified property should be displayed or not.
 * @return true if the property should be visible.
 */
bool CFilter::Filter(ITimelineItemProperty *inTimelineItemProperty) const
{
    Q_UNUSED(inTimelineItemProperty);

    bool theVisibleFlag = GetProperties();
    theVisibleFlag &= IsExpanded();

    return theVisibleFlag;
}

//=============================================================================
/**
 * Checks to see if the specified state should be displayed or not.
 * @return true if the state should be visible.
 */
bool CFilter::Filter(ITimelineItem *inTimelineItem) const
{
    bool theVisibleFlag = true;

    // If this row is shy, we need to check the filter for shy objects
    if (inTimelineItem->IsShy())
        theVisibleFlag &= GetShy();

    // If this row is locked, we need to check the filter for locked objects
    if (inTimelineItem->IsLocked())
        theVisibleFlag &= GetLocked();

    // This is for hiding visible eye toggle objects
    if (!inTimelineItem->IsVisible())
        theVisibleFlag &= GetVisible();

    // If this row is a behavior, we need to check the filter for behaviors
    if (inTimelineItem->GetObjectType() == OBJTYPE_BEHAVIOR)
        theVisibleFlag &= GetBehaviors();

    // If this row is a material, we need to check the filter for materials
    if (inTimelineItem->GetObjectType() == OBJTYPE_MATERIAL)
        theVisibleFlag &= GetMaterials();

    return theVisibleFlag;
}