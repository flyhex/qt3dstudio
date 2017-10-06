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
#include "PropertyTimebarRow.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"
#include "PropertyRow.h"
#include "MasterP.h"
#include "KeyframeContextMenu.h"
#include "PropertyTimelineKeyframe.h"
#include "HotKeys.h"
#include "MultiSelectAspect.h"
#include "Bindings/ITimelineItemProperty.h"
#include "IKeyframe.h"
#include "CoreUtils.h"

//=============================================================================
/**
 */
CPropertyTimebarRow::CPropertyTimebarRow(CPropertyRow *inPropertyRow)
    : m_PropertyRow(inPropertyRow)
    , m_DetailedView(inPropertyRow->GetProperty())
    , m_DirtyFlag(false)
    , m_Refreshing(false)
    , m_SnappingListProvider(nullptr)
{
    m_DetailedView.SetVisible(false);

    m_BackgroundColor = m_PropertyRow->GetTimebarBackgroundColor();

    AddChild(&m_DetailedView);
    m_Refreshing = false;
    RefreshKeyframes();
}

CPropertyTimebarRow::~CPropertyTimebarRow()
{
    TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        CPropertyTimelineKeyframe *theDeletedKey = (*thePos);
        RemoveChild(theDeletedKey);
        delete theDeletedKey;
    }
}

//=============================================================================
/**
 * Draws the this row background.
 * @param inRenderer the renderer to draw to.
 */
void CPropertyTimebarRow::Draw(CRenderer *inRenderer)
{
    if (m_DirtyFlag) {
        RefreshKeyframes();
        m_DirtyFlag = false;
    }

    CRct theRect(GetSize());
    UICDM::TDataTypePair theType = m_PropertyRow->GetProperty()->GetType();
    if (theType.first == UICDM::DataModelDataType::Float3
        && theType.second == UICDM::AdditionalMetaDataType::Color) {
        inRenderer->FillSolidRect(CRct(0, 0, theRect.size.x, theRect.size.y - 1),
                                  m_BackgroundColor);
        DrawColor(inRenderer);
    } else {
        inRenderer->FillSolidRect(CRct(0, 0, theRect.size.x, theRect.size.y - 1),
                                  m_BackgroundColor);
    }

    inRenderer->PushPen(CStudioPreferences::GetPropertyFloorColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y - 1));
    inRenderer->PopPen();
}

//=============================================================================
/**
 * Draw a colorized background for this property row.
 * If this is a color object then this will draw the background of this control
 * as the color that the property is. This will draw a gradient between the
 * colors over time to show somewhat what the colors look like.
 * @param inRenderer the renderer to draw to.
 */
void CPropertyTimebarRow::DrawColor(CRenderer *inRenderer)
{
    UICPROFILE(DrawColor);

    ITimelineItemProperty *theProperty = m_PropertyRow->GetProperty();
    ASSERT(theProperty->GetChannelCount() == 3); // sanity check

    CColor thePreviousColor;
    thePreviousColor.SetRed(::dtol(theProperty->GetChannelValueAtTime(0, 0)));
    thePreviousColor.SetGreen(::dtol(theProperty->GetChannelValueAtTime(1, 0)));
    thePreviousColor.SetBlue(::dtol(theProperty->GetChannelValueAtTime(2, 0)));

    long thePreviousTime = 0;
    long theLastPosition = 0;
    CColor theCurrentColor;

    long theKeyframeCount = theProperty->GetKeyframeCount();

    // Go through all the keyframes and draw a gradient from the previous key to the current key.
    // Only use the first channel for the keyframes, assume they are all at the same time.
    for (long theIndex = 0; theIndex < theKeyframeCount; ++theIndex) {
        long theCurrentTime = theProperty->GetKeyframeByIndex(theIndex)->GetTime();
        // Get the color at the specified time.
        theCurrentColor.SetRed(::dtol(theProperty->GetChannelValueAtTime(0, theCurrentTime)));
        theCurrentColor.SetGreen(::dtol(theProperty->GetChannelValueAtTime(1, theCurrentTime)));
        theCurrentColor.SetBlue(::dtol(theProperty->GetChannelValueAtTime(2, theCurrentTime)));

        long thePreviousPixel = ::TimeToPos(thePreviousTime, m_TimeRatio);
        long theCurrentPixel = ::TimeToPos(theCurrentTime, m_TimeRatio);

        // Draw a gradient from the previous keyframe position to the current one.
        inRenderer->DrawGradient(
            CRct(thePreviousPixel, 0, theCurrentPixel - thePreviousPixel, GetSize().y),
            thePreviousColor, theCurrentColor);
        thePreviousTime = theCurrentTime;
        thePreviousColor = theCurrentColor;
        theLastPosition = theCurrentPixel;
    }
    // Fill in from the last keyframe to the end of the bar.
    inRenderer->DrawGradient(CRct(theLastPosition, 0, GetSize().x - theLastPosition, GetSize().y),
                             thePreviousColor, thePreviousColor);
}

//=============================================================================
/**
 * OnMouseOver event, handles the highlighting of the row.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
void CPropertyTimebarRow::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    m_PropertyRow->OnMouseOver();
}

//=============================================================================
/**
 * OnMouseOut event, handles the de-highlighting of this row.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
void CPropertyTimebarRow::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    m_PropertyRow->OnMouseOut();
}

//=============================================================================
/**
 * OnMouseRDown event, pops up a context menu
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the modifier key states (shift, alt, and ctrl).
 */
bool CPropertyTimebarRow::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // Only do it if a child has not handled the mouse down.
    bool theRetVal = CControl::OnMouseRDown(inPoint, inFlags);
    if (!theRetVal) {
        m_PropertyRow->Select();
        CKeyframeContextMenu theMenu(m_PropertyRow->GetProperty()->GetKeyframesManager(),
                                     m_PropertyRow->GetProperty());
        DoPopup(&theMenu, inPoint);
        theRetVal = true;
    }

    return theRetVal;
}

//=============================================================================
/**
 * OnMouseOut event, handles the de-highlighting of this row.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
void CPropertyTimebarRow::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    UICPROFILE(OnMouseMove);
    CControl::OnMouseMove(inPoint, inFlags);
}
//=============================================================================
/**
 * Set this control to being highlighted or not.
 * @param inIsHighlighted true if this is to be highlighted.
 */
void CPropertyTimebarRow::SetHighlighted(bool inIsHighlighted)
{
    if (inIsHighlighted)
        m_BackgroundColor = m_PropertyRow->GetTimebarHighlightBackgroundColor();
    else
        m_BackgroundColor = m_PropertyRow->GetTimebarBackgroundColor();

    Invalidate();
}

//=============================================================================
/**
 * Get the state row that this belongs to.
 */
CPropertyRow *CPropertyTimebarRow::GetPropertyRow()
{
    return m_PropertyRow;
}

//=============================================================================
/**
 * Sets teh time ratio
 *
 * @param inTimeRatio the new ratio
 */
void CPropertyTimebarRow::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;

    TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        (*thePos)->SetTimeRatio(inTimeRatio);
    }
    m_DetailedView.SetTimeRatio(inTimeRatio);

    Invalidate();
}

void CPropertyTimebarRow::SetDetailedView(bool inDetailedView)
{
    m_DetailedView.SetVisible(inDetailedView);
}

//=============================================================================
/**
 * Sets teh size of this control.
 * @param inSize size to set the row
 */
void CPropertyTimebarRow::SetSize(CPt inSize)
{
    CControl::SetSize(inSize);

    m_DetailedView.SetSize(inSize);
}

//=============================================================================
/**
 * called when keyframes need to be updated, this funciton has two loops:
 * the first loops through and deletes any keys no longer in the sskf list.  the
 * second adds any keys in the sskf list that are not already in the list
 *
 */
void CPropertyTimebarRow::RefreshKeyframes()
{
    m_Refreshing = true;
    UICPROFILE(RefreshKeyframes);

    // First Loop clears any keys that do not correlate to a supersetkey
    TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();
    while (thePos != m_Keyframes.end()) {
        CPropertyTimelineKeyframe *theTimelineKey = (*thePos);
        CPt theSize = theTimelineKey->GetSize();
        IKeyframe *theTempKey = nullptr;
        theTempKey = m_PropertyRow->GetProperty()->GetKeyframeByTime(theTimelineKey->GetTime());

        // If we find a key at this time, then the timeline key doesn't need to be deleted
        if (!theTempKey) {
            RemoveChild(theTimelineKey);
            delete theTimelineKey;
            thePos = m_Keyframes.erase(thePos);
        } else if (theTempKey->IsDynamic() != theTimelineKey->IsDynamic()) {
            theTimelineKey->SetDynamic(theTempKey->IsDynamic());
        } else {
            // Set the position
            theTimelineKey->SetPosition(
                ::TimeToPos(theTempKey->GetTime(), m_TimeRatio) - (theSize.x / 2), 0);
            ++thePos;
        }
    }

    // Second Loop adds teh remaining keys
    long theKeyframeCount = m_PropertyRow->GetProperty()->GetKeyframeCount();
    for (long theKey = 0; theKey < theKeyframeCount; ++theKey) {
        bool theFoundFlag = false;
        IKeyframe *theTempKey = m_PropertyRow->GetProperty()->GetKeyframeByIndex(theKey);
        TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();

        long theKeyframeTime = theTempKey->GetTime();
        // each key needs to be compared to all the keys in the sskf list to see if it has to be
        // added.
        for (; thePos != m_Keyframes.end() && !theFoundFlag; ++thePos) {
            CPropertyTimelineKeyframe *theCurrentKey = (*thePos);
            if (theCurrentKey->GetTime() == theKeyframeTime) {
                theFoundFlag = true;
            }
        }
        if (!theFoundFlag) {
            // If we don't have a timeline key, then we have to make a new one
            CPropertyTimelineKeyframe *thePropertyTimelineKey =
                new CPropertyTimelineKeyframe(this, m_TimeRatio);
            thePropertyTimelineKey->SetTime(theKeyframeTime);
            thePropertyTimelineKey->Select(theTempKey->IsSelected());
            thePropertyTimelineKey->SetDynamic(theTempKey->IsDynamic());
            thePropertyTimelineKey->SetSize(CPt(15, 16));
            CPt theSize = thePropertyTimelineKey->GetSize();
            long theXPosition = ::TimeToPos(theKeyframeTime, m_TimeRatio) - (theSize.x / 2);
            thePropertyTimelineKey->SetPosition(theXPosition, 0);
            AddChild(thePropertyTimelineKey);
            m_Keyframes.push_back(thePropertyTimelineKey);
        }
    }

    if (m_DetailedView.IsVisible())
        m_DetailedView.RefreshKeyframes();

    m_Refreshing = false;
}

void CPropertyTimebarRow::Invalidate(bool inInvalidate)
{
    if (!m_Refreshing) {
        CControl::Invalidate(inInvalidate);
    }
}

//=============================================================================
/**
 * Handler for when a child key is selected
 *
 * @param inTime time of the key
 */
void CPropertyTimebarRow::OnKeySelected(long inTime, bool inSelected)
{
    if (inTime >= 0) {
        m_PropertyRow->GetProperty()->SelectKeyframes(inSelected, inTime);
        Invalidate();
    }
}

//=============================================================================
/**
 * Deselects all keyframes
 */
void CPropertyTimebarRow::DeselectAllKeyframes()
{
    m_PropertyRow->GetProperty()->SelectKeyframes(false);
}

//=============================================================================
/**
 * called when a child changes and the keyframes need to be refreshed
 *
 * @param inDirtyFlag true if this object is now dirty
 *
 */
void CPropertyTimebarRow::SetDirty(bool inDirtyFlag)
{
    if (m_DirtyFlag == inDirtyFlag)
        return;

    m_DirtyFlag = inDirtyFlag;
    Invalidate();
}

//=============================================================================
/**
 * SelectKeysInRect: selects any keyframes inside the rect
 *
 * @param inRect is used to select keyframes
 * @param inModifierKeyDown indicates if the control modifier is down
 */
void CPropertyTimebarRow::SelectKeysInRect(CRct inRect, bool inModifierKeyDown)
{
    CMultiSelectAspect<TTimelineKeyframeList> theMultiSelectAspect(m_Keyframes,
                                                                   m_PropertyRow->GetProperty());
    theMultiSelectAspect.MultiSelect(inRect, inModifierKeyDown);
}

//=============================================================================
/**
 * CommitSelections: commits all the master keyframe selections by setting their
 *                   previous selection state to the current selection state.
 *					 This will prevent the keyframes in the current selection
 *from
 *					 switching states as we select other keyframes.
 *
 * @param NONE
 * @return NONE
 */

void CPropertyTimebarRow::CommitSelections()
{
    CMultiSelectAspect<TTimelineKeyframeList> theMultiSelectAspect(m_Keyframes,
                                                                   m_PropertyRow->GetProperty());
    theMultiSelectAspect.CommitSelections();
}

//=============================================================================
/**
 * true if there are selected keys on this object
 */
bool CPropertyTimebarRow::HasSelectedKeys()
{
    bool theRetVal = false;
    TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end() && !theRetVal; ++thePos) {
        if ((*thePos)->IsSelected()) {
            theRetVal = true;
        }
    }
    return theRetVal;
}

//=============================================================================
/**
 * selects all keys for this timebar row
 */
void CPropertyTimebarRow::SelectAllKeys()
{
    TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        m_PropertyRow->GetProperty()->SelectKeyframes(true, (*thePos)->GetTime());
        (*thePos)->Select(true);
        Invalidate();
    }
}

//=============================================================================
/**
 * Set this control as being visible or not.
 * If the control is not visible then it will not be drawn and will not
 * get mouse clicks.
 * @param inIsVisible true if this control is to be visible.
 */
void CPropertyTimebarRow::SetVisible(bool inIsVisible)
{
    if (inIsVisible != IsVisible()) {
        CControl::SetVisible(inIsVisible);
        SetDirty(true);
    }
}

bool CPropertyTimebarRow::HasKeyframe(long inTime) const
{
    return m_PropertyRow->GetProperty()->GetKeyframeByTime(inTime) != nullptr;
}

//=============================================================================
/**
 * @param inTime -1 for all keyframes
 */
void CPropertyTimebarRow::SelectKeysByTime(long inTime, bool inSelected)
{
    TTimelineKeyframeList::iterator thePos = m_Keyframes.begin();
    bool theFoundFlag = false;
    for (; thePos != m_Keyframes.end() && !theFoundFlag; ++thePos) {
        CPropertyTimelineKeyframe *theKey = (*thePos);
        if (inTime == -1 || theKey->GetTime() == inTime) {
            theKey->Select(inSelected);
            theFoundFlag = (inTime != -1);
        }
    }
    Invalidate();
}

void CPropertyTimebarRow::SetSnappingListProvider(ISnappingListProvider *inProvider)
{
    m_SnappingListProvider = inProvider;
}

ISnappingListProvider &CPropertyTimebarRow::GetSnappingListProvider() const
{
    // sk - If you hit this, it means the setup is incorrect. e.g. the parent row (which is most
    // probably a staterow) isn't pass 'down' this info.
    ASSERT(m_SnappingListProvider);
    return *m_SnappingListProvider;
}
