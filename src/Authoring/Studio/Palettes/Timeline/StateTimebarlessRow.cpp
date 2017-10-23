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
#include "StateTimebarlessRow.h"
#include "IKeyframe.h"
#include "Renderer.h"
#include "StateRow.h"
#include "MasterP.h"
#include "KeyframeContextMenu.h"
#include "Snapper.h"
#include "MultiSelectAspect.h"
#include "PropertyTimebarRow.h"
#include "PropertyRow.h"
#include "AssetTimelineKeyframe.h"
#include "Bindings/ITimelineItemBinding.h"
#include "StudioUtils.h"

//=============================================================================
/**
 * Creates a new CStateTimebarRow for the StateRow.
 * @param inStateRow the State Row that this is on.
 */
CStateTimebarlessRow::CStateTimebarlessRow(CStateRow *inStateRow)
    : m_StateRow(inStateRow)
    , m_Selected(false)
    , m_Refreshing(false)
{
    m_BackgroundColor = m_StateRow->GetTimebarBackgroundColor(m_StateRow->GetObjectType());
}

CStateTimebarlessRow::~CStateTimebarlessRow()
{
    TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        CAssetTimelineKeyframe *theDeletedKey = (*thePos);
        RemoveChild(theDeletedKey);
        delete theDeletedKey;
    }
}

void CStateTimebarlessRow::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation)
{
    CControl::OnDraw(inRenderer, inDirtyRect, inIgnoreValidation);
}

//=============================================================================
/**
 * Draws the this row background.
 * @param inRenderer the renderer to draw to.
 */
void CStateTimebarlessRow::Draw(CRenderer *inRenderer)
{
    QT3DS_PROFILE(Draw);

    if (m_DirtyFlag) {
        RefreshKeyframes();
        m_DirtyFlag = false;
    }

    CBaseTimebarlessRow::Draw(inRenderer);
}

//=============================================================================
/**
 * OnMouseRDown event, handles context menus for this object.
 * @param inPoint the location of the mouse over this control.
 * @param inFlags the mouse state flags.
 */
bool CStateTimebarlessRow::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_StateRow->Select(SBaseStateRowSelectionKeyState()); // ensure this is selected, but doesn't
                                                          // affect any key selections, because this
                                                          // can be triggered from a key being
                                                          // selected
    return CControl::OnMouseRDown(inPoint, inFlags);
}

//=============================================================================
/**
 * Set the amount of time that is being represented per pixel.
 * @param inTimerPerPixel the amound of time being represented per pixel.
 */
void CStateTimebarlessRow::SetTimeRatio(double inTimeRatio)
{
    CBaseTimebarlessRow::SetTimeRatio(inTimeRatio);

    TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        (*thePos)->SetTimeRatio(inTimeRatio);
    }
}

//=============================================================================
/**
 * Get the state row that this belongs to.
 */
CStateRow *CStateTimebarlessRow::GetStateRow()
{
    return m_StateRow;
}

//=============================================================================
/**
 * Handler for when a child key is selected
 *
 * @param inTime time of the key
 */
void CStateTimebarlessRow::OnKeySelected(long inTime, bool inSelected,
                                         bool inClearPreviouslySelectedKeys)
{
    ITimelineItemBinding *theTimelineItemBinding = m_StateRow->GetTimelineItemBinding();
    if (inSelected)
        theTimelineItemBinding->SetSelected(false);

    if (inClearPreviouslySelectedKeys)
        theTimelineItemBinding->ClearKeySelection();

    theTimelineItemBinding->SelectKeyframes(inSelected, inTime);
    RefreshKeyframes();
    Invalidate();
}

CBaseStateRow *CStateTimebarlessRow::GetBaseStateRow() const
{
    return m_StateRow;
}

//=============================================================================
/**
 * Checks the data binding, instead of the property rows since they may not be created
 * (delayed-loading) if this is not expanded.
 */
bool CStateTimebarlessRow::PropertiesHaveKeyframe(long inTime)
{
    bool theResult = false;

    ITimelineItemBinding *theTimelineItemBinding = m_StateRow->GetTimelineItemBinding();
    long theNumProps = theTimelineItemBinding->GetPropertyCount();
    for (long theIndex = 0; theIndex < theNumProps; ++theIndex) {
        ITimelineItemProperty *theProp = theTimelineItemBinding->GetProperty(theIndex);
        if (theProp && theProp->GetKeyframeByTime(inTime)) {
            theResult = true;
            break;
        }
    }
    return theResult;
}

//=============================================================================
/**
 * called when keyframes need to be updated, this funciton has two loops:
 * the first loops through and deletes any keys no longer in the sskf list.  the
 * second adds any keys in the sskf list that are not already in the list
 *
 */
void CStateTimebarlessRow::RefreshKeyframes()
{
    QT3DS_PROFILE(RefreshKeyframes);

    m_Refreshing = true;

    ITimelineItemBinding *theTimelineItemBinding = m_StateRow->GetTimelineItemBinding();
    long theKeyframeCount = theTimelineItemBinding->GetKeyframeCount();
    TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();

    // First Loop clears any keys that do not correlate to a supersetkey
    while (thePos != m_Keyframes.end()) {
        CAssetTimelineKeyframe *theTimelineKey = (*thePos);
        IKeyframe *theTempKey = nullptr;
        theTempKey = theTimelineItemBinding->GetKeyframeByTime(theTimelineKey->GetTime());

        // If we find a key at this time, then the timeline key doesn't need to be deleted
        if (!theTempKey || !PropertiesHaveKeyframe(theTimelineKey->GetTime())) {
            RemoveChild(theTimelineKey);
            delete theTimelineKey;
            thePos = m_Keyframes.erase(thePos);
        } else if (theTempKey->IsDynamic() != theTimelineKey->IsDynamic()) {
            theTimelineKey->SetDynamic(theTempKey->IsDynamic());
        } else {
            // Set the position
            theTimelineKey->SetPosition(::TimeToPos(theTempKey->GetTime(), m_TimeRatio)
                                            - (theTimelineKey->GetSize().x / 2),
                                        0);
            ++thePos;
        }
    }

    // Second Loop adds the remaining keys
    for (long theKey = 0; theKey < theKeyframeCount; ++theKey) {
        bool theFoundFlag = false;
        IKeyframe *theTempKey = theTimelineItemBinding->GetKeyframeByIndex(theKey);
        TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();

        // each key needs to be compared to all the keys in the sskf list to see if it has to be
        // added.
        for (; thePos != m_Keyframes.end() && !theFoundFlag; ++thePos) {
            CAssetTimelineKeyframe *theCurrentKey = (*thePos);
            if (theCurrentKey->GetTime() == theTempKey->GetTime()) {
                theFoundFlag = true;
            }
        }
        if (!theFoundFlag && PropertiesHaveKeyframe(theTempKey->GetTime())) {
            // If we don't have a timeline key, then we have to make a new one
            CAssetTimelineKeyframe *theAssetTimelineKey =
                new CAssetTimelineKeyframe(this, m_TimeRatio);
            theAssetTimelineKey->SetTime(theTempKey->GetTime());
            theAssetTimelineKey->Select(theTempKey->IsSelected());
            theAssetTimelineKey->SetDynamic(theTempKey->IsDynamic());
            theAssetTimelineKey->SetSize(CPt(17, 16));
            theAssetTimelineKey->SetPosition(::TimeToPos(theTempKey->GetTime(), m_TimeRatio)
                                                 - (theAssetTimelineKey->GetSize().x / 2),
                                             0);
            AddChild(theAssetTimelineKey);
            m_Keyframes.push_back(theAssetTimelineKey);
        }
    }
    m_Refreshing = false;
}

void CStateTimebarlessRow::Invalidate(bool inInvalidate)
{
    if (!m_Refreshing) {
        CControl::Invalidate(inInvalidate);
    }
}

//=============================================================================
/**
 * called when a list has a member change selection
 * @param inTime -1 to affect all keyframes.
 *
 */
void CStateTimebarlessRow::SelectKeysByTime(long inTime, bool inSelected)
{
    TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();
    bool theFoundFlag = false;
    for (; thePos != m_Keyframes.end() && !theFoundFlag; ++thePos) {
        CAssetTimelineKeyframe *theKey = (*thePos);
        if (inTime == -1 || theKey->GetTime() == inTime) {
            theKey->Select(inSelected);
            theFoundFlag = (inTime != -1);
        }
    }
    Invalidate();
}

//=============================================================================
/**
 * SelectKeysInRect: selects any keyframes inside the rect
 *
 * @param inRect the Rect to select the keyframes in
 * @return NONE.
 */
void CStateTimebarlessRow::SelectKeysInRect(CRct inRect, bool inModifierKeyDown)
{
    CMultiSelectAspect<TTimelineAssetKeyframeList> theMultiSelectAspect(
        m_Keyframes, m_StateRow->GetTimelineItemBinding());
    theMultiSelectAspect.MultiSelect(inRect, inModifierKeyDown);
}

//=============================================================================
/**
 * CommitSelections: commits all the master keyframe selections by setting their
 *                   previous selection state to the current selection state.
 *					 This will prevent the current keyframe states from
 *changing.
 *
 * @param NONE
 * @return NONE
 */

void CStateTimebarlessRow::CommitSelections()
{
    CMultiSelectAspect<TTimelineAssetKeyframeList> theMultiSelectAspect(
        m_Keyframes, m_StateRow->GetTimelineItemBinding());
    theMultiSelectAspect.CommitSelections();
}

//=============================================================================
/**
 * true if there are selected keys on this object
 */
bool CStateTimebarlessRow::HasSelectedKeys()
{
    bool theRetVal = false;
    TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();
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
void CStateTimebarlessRow::SelectAllKeys()
{
    m_StateRow->GetTimelineItemBinding()->SelectKeyframes(true, -1);

    TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos)
        (*thePos)->Select(true);

    Invalidate();
}

//=============================================================================
/**
 * Populates the snapping list with any snapping points that may be on this.
 * This will add the timebar ends, master keyframes to the snapping list, and
 * time labels to the snapping list.
 * @param inSnapper the snapper to add the points to.
 */
void CStateTimebarlessRow::PopulateSnappingList(CSnapper *inSnapper)
{
    // Only add points if this is not the object originating the snapping.
    if (inSnapper->GetSource() != this) {
        // Add Keyframes
        TTimelineAssetKeyframeList::iterator thePos = m_Keyframes.begin();
        for (; thePos != m_Keyframes.end(); ++thePos) {
            if (inSnapper->IsSnappingSelectedKeyframes())
                inSnapper->AddTime((*thePos)->GetTime());
            else if (!(*thePos)->IsSelected())
                inSnapper->AddTime((*thePos)->GetTime());
        }
    }
}
