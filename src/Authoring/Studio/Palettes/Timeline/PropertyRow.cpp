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
#include "TimelineControl.h"
#include "PropertyColorControl.h"
#include "PropertyToggleControl.h"
#include "PropertyTreeControl.h"
#include "PropertyTimebarRow.h"
#include "BlankControl.h"
#include "StudioPreferences.h"
#include "StateRow.h"
#include "Bindings/ITimelineItemProperty.h"

//=============================================================================
/**
 * Create a property row for inProperty.
 * @param inProperty the property that this is displaying.
 */
CPropertyRow::CPropertyRow(ITimelineItemProperty *inProperty)
    : m_Highlighted(false)
    , m_DetailedView(false)
    , m_TimeRatio(0.0f)
    , m_Property(inProperty)
{
    m_IsViewable = true;

    m_TreeControl = new CPropertyTreeControl(this);
    m_ToggleControl = new CPropertyToggleControl(this);
    m_TimebarRow = new CPropertyTimebarRow(this);

    m_PropertyColorControl = new CPropertyColorControl(this);
    long theTimebarHeight = CStudioPreferences::GetRowSize();

    // do not set absolute size because it messes up the timeline
    m_TreeControl->SetSize(CPt(500, theTimebarHeight));
    m_PropertyColorControl->SetAbsoluteSize(CPt(theTimebarHeight, theTimebarHeight));
    m_ToggleControl->SetAbsoluteSize(CPt(57, theTimebarHeight));
    m_TimebarRow->SetSize(CPt(0, theTimebarHeight));

    // sk: setting controls names' seem to be only useful for debugging.
    // Q3DStudio::CString thePropName( inProperty->GetName( ) );
    // m_TreeControl->SetName( thePropName );
    // m_TimebarRow->SetName( thePropName + "TimebarRow" );
}

CPropertyRow::~CPropertyRow()
{
    delete m_TreeControl;
    delete m_ToggleControl;
    delete m_PropertyColorControl;
    delete m_TimebarRow;
}

//=============================================================================
/**
 * Get the left hand color control for this row.
 * @return the color control for this row.
 */
CControl *CPropertyRow::GetColorControl()
{
    return m_PropertyColorControl;
}

//=============================================================================
/**
 * Get the tree control object for this row.
 * @return the tree control object for this row.
 */
CControl *CPropertyRow::GetTreeControl()
{
    return m_TreeControl;
}

//=============================================================================
/**
 * Get the toggle control object for this row.
 * @return the toggle control object for this row.
 */
CControl *CPropertyRow::GetToggleControl()
{
    return m_ToggleControl;
}

//=============================================================================
/**
 * Get the timebar control for this row.
 * @return the timebar control for this row.
 */
CControl *CPropertyRow::GetTimebarControl()
{
    return m_TimebarRow;
}

//=============================================================================
/**
 * Set the amount that this row is indented.
 * This handles the tree indenting.
 * @param inIndent the amount of indent for this row.
 */
void CPropertyRow::SetIndent(long inIndent)
{
    m_TreeControl->SetIndent(inIndent);
}

//=============================================================================
/**
 * Notification from one of the child controls that the mouse just entered.
 * This is used to handle the highlighting of the entire row.
 */
void CPropertyRow::OnMouseOver()
{
    // Only change if change is needed
    if (!m_Highlighted) {
        m_TreeControl->SetHighlighted(true);
        m_ToggleControl->SetHighlighted(true);
        m_TimebarRow->SetHighlighted(true);

        m_Highlighted = true;
    }
}

//=============================================================================
/**
 * Notification from one of the child controls that the mouse just left.
 * This is used to handle the highlighting of the entire row.
 */
void CPropertyRow::OnMouseOut()
{
    // Only change is change is needed.
    if (m_Highlighted) {
        m_TreeControl->SetHighlighted(false);
        m_ToggleControl->SetHighlighted(false);
        m_TimebarRow->SetHighlighted(false);

        m_Highlighted = false;
    }
}

//=============================================================================
/**
 * Double click handler for the property row
 */
void CPropertyRow::OnMouseDoubleClick()
{
    SetDetailedView(!m_DetailedView);
}

//=============================================================================
/**
 * Expands the row out so a more detailed view of the animation tracks can be seen
 *
 *	@param inIsInDetailedView true if we are switching to a detailed view
 */
void CPropertyRow::SetDetailedView(bool inIsDetailedView)
{
    m_DetailedView = inIsDetailedView;

    if (m_DetailedView) {
        long theHeight = 100;
        m_PropertyColorControl->SetAbsoluteSize(
            CPt(m_PropertyColorControl->GetSize().x, theHeight));
        m_TreeControl->SetSize(CPt(m_TreeControl->GetSize().x, theHeight));
        m_ToggleControl->SetAbsoluteSize(CPt(m_ToggleControl->GetSize().x, theHeight));
        m_TimebarRow->SetAbsoluteSize(CPt(m_TimebarRow->GetSize().x, theHeight));

        m_TimebarRow->SetDetailedView(true);
    } else {
        long theDefaultHeight = CStudioPreferences::GetRowSize();

        m_PropertyColorControl->SetAbsoluteSize(
            CPt(m_PropertyColorControl->GetSize().x, theDefaultHeight));
        m_TreeControl->SetSize(CPt(m_TreeControl->GetSize().x, theDefaultHeight));
        m_ToggleControl->SetAbsoluteSize(CPt(m_ToggleControl->GetSize().x, theDefaultHeight));
        m_TimebarRow->SetAbsoluteSize(CPt(m_TimebarRow->GetSize().x, theDefaultHeight));

        m_TimebarRow->SetDetailedView(false);
    }

    GetTopControl()->OnLayoutChanged();
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
    m_TreeControl->SetVisible(theVisibleFlag);
    m_TimebarRow->SetVisible(theVisibleFlag);
    m_ToggleControl->SetVisible(theVisibleFlag);
    m_PropertyColorControl->SetVisible(theVisibleFlag);
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
        m_TimebarRow->SelectAllKeys();
    else
        m_Property->ClearKeySelection();
}

ISnappingListProvider *CPropertyRow::GetSnappingListProvider() const
{
    return &m_TimebarRow->GetSnappingListProvider();
}

void CPropertyRow::SetSnappingListProvider(ISnappingListProvider *inProvider)
{
    m_TimebarRow->SetSnappingListProvider(inProvider);
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
    m_TimebarRow->SetTimeRatio(inTimeRatio);
}

//=============================================================================
/**
 * Selects all keyframes of this property that are inside inRect
 *
 * @param inRect theRect to check
 */
void CPropertyRow::SelectKeysInRect(CRct inRect, bool inModifierKeyDown)
{
    CRct theOffsetRect = inRect;
    CRct myRect(CPt(0, 0), m_TimebarRow->GetSize());
    theOffsetRect.Offset(-m_TimebarRow->GetPosition());
    m_TimebarRow->SelectKeysInRect(theOffsetRect, inModifierKeyDown);
}

//=============================================================================
/**
 * CommitSelections: commits all the property keyframe selections by setting their
 *                   previous selection state to the current selection state.
 *					 This will prevent the keyframes in the current selection
 *from
 *					 switching states as we select other keyframes.
 *
 * @param NONE
 * @return NONE
 */
void CPropertyRow::CommitSelections()
{
    m_TimebarRow->CommitSelections();
}

//=============================================================================
/**
 * Selects all keyframes
 */
void CPropertyRow::SelectAllKeys()
{
    m_TimebarRow->SelectAllKeys();
}

//=============================================================================
/**
 * Deletes keyframes on this property row
 */
void CPropertyRow::DeleteAllKeys()
{
    m_Property->DeleteAllKeys();
    m_TimebarRow->Invalidate();
}

//=============================================================================
/**
 * Sets all the child control enable states
 * @param inEnabled the state to set the controls to
 */
void CPropertyRow::SetEnabled(bool inEnabled)
{
    m_TreeControl->SetEnabled(inEnabled);
    m_ToggleControl->SetEnabled(inEnabled);
    m_PropertyColorControl->SetEnabled(inEnabled);
    m_TimebarRow->SetEnabled(inEnabled);
}

//=============================================================================
/**
 * Callback from the ITimelineProperty.
 */
void CPropertyRow::Refresh()
{
    if (m_TimebarRow->IsVisible())
        m_TimebarRow->SetDirty(true);
}
