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

#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "PropertyRow.h"

#include "StudioPreferences.h"
#include "StateRow.h"
#include "Bindings/ITimelineItemProperty.h"

//=============================================================================
/**
 * Create a property row for inProperty.
 * @param inProperty the property that this is displaying.
 */
CPropertyRow::CPropertyRow(ITimelineItemProperty *inProperty, CTimelineRow *parent)
    : CTimelineRow(parent)
    , m_TimeRatio(0.0f)
    , m_Property(inProperty)
{
    m_IsViewable = true;
}

CPropertyRow::~CPropertyRow()
{
}

//=============================================================================
/**
 * Filter this property.
 * This controls whether or not this row should be displayed. The filter will
 * be stored and used for future operations that may change whether or not
 * this should be visible.
 * @param inFilter the filter to filter on.
 * @param inFilterChildren true if the call is recursive.
 */
void CPropertyRow::Filter(const CFilter &inFilter, bool inFilterChildren /*= true*/)
{
    Q_UNUSED(inFilterChildren);

    m_Filter = inFilter;

    bool theVisibleFlag = inFilter.Filter(m_Property);
    emit visibleChanged(theVisibleFlag);
    m_IsViewable = theVisibleFlag;
}

//=============================================================================
/**
 * @return true if this property is animated and is not currently being filtered out
 */
bool CPropertyRow::IsViewable() const
{
    return m_Filter.GetProperties();
}

//=============================================================================
/**
 * Call from one of the child controls to select this object.
 * Currently this just causes the Asset to be selected.
 */
void CPropertyRow::Select(bool inIsShiftKeyPressed /*= false */)
{
    m_Property->SetSelected();

    if (inIsShiftKeyPressed)
        emit selectAllKeys();
    else
        m_Property->ClearKeySelection();
}

//=============================================================================
/**
 * Retrieves the background color for the row based upon the type of asset
 * passed in.  Overridden so that properties can have a different background
 * color than their parent asset does.
 * @return background color to use for this row
 */
::CColor CPropertyRow::GetTimebarBackgroundColor()
{
    return CStudioPreferences::GetPropertyBackgroundColor();
}

//=============================================================================
/**
 * Retrieves the background color for the row when the mouse is over the row
 * based upon the type of asset passed in.  Overridden so that properties can
 * have a different highlight background color than their parent asset does.
 * @return background color to use for this row when the mouse is over it
 */
::CColor CPropertyRow::GetTimebarHighlightBackgroundColor()
{
    return CStudioPreferences::GetPropertyMouseOverColor();
}

//=============================================================================
/**
 * Set the amount of time that is represented by a pixel.
 * This modifies the length of this control.
 * @param inTimePerPixel the time per pixel.
 */
void CPropertyRow::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
    emit timeRatioChanged(inTimeRatio);
}

//=============================================================================
/**
 * Deletes keyframes on this property row
 */
void CPropertyRow::DeleteAllKeys()
{
    m_Property->DeleteAllKeys();
    emit deleteAllKeys();
}


//=============================================================================
/**
 * Callback from the ITimelineProperty.
 */
void CPropertyRow::Refresh()
{
    emit refreshRequested();
}

