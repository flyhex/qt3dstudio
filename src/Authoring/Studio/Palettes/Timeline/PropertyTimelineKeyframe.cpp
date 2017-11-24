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
#include "Dialogs.h"

//==============================================================================
//	Includes
//==============================================================================
#include "PropertyTimelineKeyframe.h"
#include "PropertyTimebarRow.h"
#include "Renderer.h"
#include "PropertyRow.h"
#include "PropertyRowUI.h"
#include "KeyframeContextMenu.h"
#include "HotKeys.h"
#include "ResourceCache.h"
#include "TimelineControl.h"
#include "Bindings/ITimelineItemProperty.h"
#include "StudioUtils.h"
#include "TimeEditDlg.h"

CPropertyTimelineKeyframe::CPropertyTimelineKeyframe(CPropertyTimebarRow *inParentRow,
                                                     double inTimeRatio)
    : m_Selected(false)
    , m_IsMouseDown(false)
    , m_IsDragging(false)
{
    m_ParentRow = inParentRow;
    m_TimeRatio = inTimeRatio;
    CResourceCache *theCache = CResourceCache::GetInstance();
    m_Icon = theCache->GetBitmap("Keyframe-Property-Normal.png");
    m_DisabledIcon = theCache->GetBitmap("Keyframe-Property-Disabled.png");
    m_SelectedIcon = theCache->GetBitmap("Keyframe-Property-Selected.png");
    m_DynamicIcon = theCache->GetBitmap("Keyframe-PropertyDynamic-Normal.png");
    m_DynamicSelectedIcon = theCache->GetBitmap("Keyframe-PropertyDynamic-Selected.png");
    m_RectOverHandled = false;
    m_PreviousSelectState = false;
}

CPropertyTimelineKeyframe::~CPropertyTimelineKeyframe()
{
}

//=============================================================================
/**
 * SetRectOverHandled: Sets if mouse rectangle has been handled
 * param@ inState indicates if the rectangle over has been handled.
 * return@ NONE
 */
void CPropertyTimelineKeyframe::SetRectOverHandled(bool inState)
{
    m_RectOverHandled = inState;
}

//=============================================================================
/**
 * GetRectOverHandled: GetRectOverHandled
 * param@ NONE
 * return@ m_RectOverHandled, which indicates if the rectangle over has been handled
 */
bool CPropertyTimelineKeyframe::GetRectOverHandled()
{
    return m_RectOverHandled;
}

//=============================================================================
/**
 * SetPreviousSelectState: Sets if the current keyframe was previously selected
 * param@ inState is used to set m_PreviousSelectState.
 * return@ NONE
 */
void CPropertyTimelineKeyframe::SetPreviousSelectState(bool inState)
{
    m_PreviousSelectState = inState;
}

//=============================================================================
/**
 * GetPreviousSelectState: Returns the keyframe's previous select state
 * param@ NONE
 * return@ m_PreviousSelectState that stores the select state for the keyframe
 */
bool CPropertyTimelineKeyframe::GetPreviousSelectState()
{
    return m_PreviousSelectState;
}

//=============================================================================
/**
 * Gets teh correct image and draws
 */
void CPropertyTimelineKeyframe::Draw(CRenderer *inRenderer)
{
    if (m_Selected) {
        inRenderer->DrawBitmap(CPt(0, 0), GetImage());
    } else {
        inRenderer->DrawBitmap(CPt(2, 0), GetImage());
    }
}

//=============================================================================
/**
 * Gets the current bitmap depending on the state of the button, postion of the
 * mouse, etc.  Returns the image for the up state by default.
 */
QPixmap CPropertyTimelineKeyframe::GetImage()
{
    if (!IsEnabled())
        return m_DisabledIcon;
    else if (m_IsDynamic) {
        if (m_Selected)
            return m_DynamicSelectedIcon;
        else
            return m_DynamicIcon;
    } else if (m_Selected)
        return m_SelectedIcon;
    else
        return m_Icon;
}

//=============================================================================
/**
 * Handler for left-click events.  Allows keyframe dragging.  Children are
 * allowed to handle this event before the parent does.
 * @param inPoint the point where the mouse is
 * @param inFlags the state of modifier keys when the event occurred
 * @return true
 */
bool CPropertyTimelineKeyframe::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // Store the mouse down location in screen coordinates so that we can check the dragging buffer
    // in OnMouseMove
    m_MouseDownLoc = inPoint;

    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        // If the control key is down then we change state, otherwise
        if (!((CHotKeys::MODIFIER_CONTROL & inFlags) == CHotKeys::MODIFIER_CONTROL)) {
            if (!m_Selected)
                GetProperty()->ClearKeySelection();

            m_Selected = true;
        } else {
            m_Selected = !m_Selected;
        }
        m_ParentRow->OnKeySelected(m_Time, m_Selected);
        // TODO : sk - Remove this when we are clear on the fact that UpdateClientScene &
        // FireChangeEvent do nothing useful here.
        //				This call makes no sense. However, if you run into some wierd
        //behavior related to mouse down events, this might be a clue.
        //				I am leaving this just in case this speeds up debugging for anyone
        //else looking at this.
        //				CAssetTimelineKeyframe::OnMouseDown does similar things.
        //	m_Doc->UpdateClientScene( true );
        //	m_ParentRow->GetPropertyRow()->GetProperty()->FireChangeEvent( false );
        m_IsMouseDown = true;

        m_Snapper.Clear();
        m_Snapper.SetSource(this);
        m_Snapper.SetSnappingSelectedKeyframes(false);
        m_ParentRow->GetSnappingListProvider().PopulateSnappingList(&m_Snapper);
        m_Snapper.BeginDrag(inPoint.x);

        // display the time range tooltip
        RefreshToolTip(inPoint);
    }

    return true;
}

//=============================================================================
/**
 * Handler for right-click events.  Pops up a context menu.  Children are
 * allowed to handle this event before the parent does.
 * @param inPoint the point where the mouse is
 * @param inFlags the state of modifier keys when the event occurred
 * @return true
 */
bool CPropertyTimelineKeyframe::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseRDown(inPoint, inFlags)) {
        if (!m_Selected) {
            GetProperty()->ClearKeySelection();
            m_Selected = true;
            m_ParentRow->OnKeySelected(m_Time, m_Selected);
        }

        CKeyframeContextMenu theMenu(GetProperty()->GetKeyframesManager(), GetProperty());
        theMenu.SetTime(GetTime());
        DoPopup(&theMenu, inPoint);
    }

    return true;
}

//=============================================================================
/**
 * called when this key is selected
 * @param inState the state this key is selected to
 */
void CPropertyTimelineKeyframe::Select(bool inState)
{
    if (m_Selected != inState) {
        m_Selected = inState;
        Invalidate();
    }
}

//=============================================================================
/**
 * handler for the mouse up event
 * @param inFlags the state of things when the mouse button was released
 * @param inPoint the point where the mouse is
 */
void CPropertyTimelineKeyframe::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);

    // If this key is selected, then we need to make sure that it moved all the way to the
    // mouse up location
    if (m_Selected && m_IsMouseDown && m_IsDragging) {
        long theNewTime = m_Snapper.ProcessDrag(m_Time, inPoint.x, inFlags);
        long theDiffTime = theNewTime - m_Time;

        // theDiffTime can get updated if its invalid.
        theDiffTime = GetProperty()->OffsetSelectedKeyframes(theDiffTime);
        // Set this key's time so it won't be recalced in Refresh keyframes in the row
        SetTime(m_Time + theDiffTime);
    }

    m_IsMouseDown = false;
    m_IsDragging = false;

    GetProperty()->CommitChangedKeyframes();
    HideMoveableWindow();
    Invalidate();
}

//=============================================================================
/**
 * handler for the onMouse Move event.  Offsets selected keys.
 *
 * @param inFlags the state of things when the mouse was moved
 * @param inPoint the point where the mouse is
 */
void CPropertyTimelineKeyframe::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    // If the mouse is down and this is slected, then offst the keys
    if (m_IsMouseDown && m_Selected) {
        // If we are not yet dragging the keyframe
        if (!m_IsDragging) {
            long theDiff = ::abs(inPoint.x) - m_MouseDownLoc.x;
            // Figure out if the mouse has moved far enough to start the drag, and readjust the drag
            // postion on the snapper
            m_IsDragging = (::abs(theDiff) > DRAGBUFFER);
            if (m_IsDragging && (::abs(theDiff) - DRAGBUFFER) > 2) {
                m_Snapper.BeginDrag(m_MouseDownLoc.x);
            } else
                m_Snapper.BeginDrag(inPoint.x);
        }

        // If we are already dragging the keyframe, procceed as normal
        if (m_IsDragging) {
            long theNewTime = m_Snapper.ProcessDrag(m_Time, inPoint.x, inFlags);
            long theDiffTime = theNewTime - m_Time;

            if (theDiffTime != 0) {
                // theDiffTime can get updated if its invalid.
                theDiffTime = GetProperty()->OffsetSelectedKeyframes(theDiffTime);
                // Set this key's time so it won't be recalced in Refresh keyframes in the row
                SetTime(m_Time + theDiffTime);
                Invalidate();
            }
        }

        // display the time range tooltip
        RefreshToolTip(inPoint);
    }
}

//=============================================================================
/**
 * Sets teh time ratio
 *
 * @param inTimeRatio the new ratio
 */
void CPropertyTimelineKeyframe::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
    CPt theSize = GetSize();
    SetPosition(::TimeToPos(GetTime(), m_TimeRatio) - (theSize.x / 2), 0);
}

//=============================================================================
/**
 * Pass the double click notification on to the row and have it process it.
 * The row will do object-specific actions on doubleclicks.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 * @return true stating that the event was processed.
 */
bool CPropertyTimelineKeyframe::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);

    GetProperty()->OnEditKeyframeTime(m_Time, ASSETKEYFRAME);
    return true;
}

//=============================================================================
/**
 * @return  true if selected
 */
bool CPropertyTimelineKeyframe::IsSelected()
{
    return m_Selected;
}

//=============================================================================
/**
* Updates the ToolTip and moves it to the correct place on screen.
* @param inPoint the point that the tooltip is supposed to be placed.
*/
void CPropertyTimelineKeyframe::RefreshToolTip(CPt inPoint)
{
    CRct theTimelineBounds(m_ParentRow->GetPropertyRowUI()->GetTopControl()->GetBounds());

    // format label
    Q3DStudio::CString theCommentText = " " + ::FormatTimeString(GetTime());

    inPoint.y = GetPosition().y - GetSize().y;
    inPoint.x = GetSize().x / 2;
    ShowMoveableWindow(inPoint, theCommentText, theTimelineBounds);
}

//=============================================================================
/**
* Helper function to retrieve the ITimelineItemProperty
*/
ITimelineItemProperty *CPropertyTimelineKeyframe::GetProperty() const
{
    auto propertyRow = static_cast<CPropertyRow *>(m_ParentRow->GetPropertyRowUI()->GetTimelineRow());
    return propertyRow->GetProperty();
}
