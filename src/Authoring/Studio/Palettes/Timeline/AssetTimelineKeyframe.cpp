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
#include "AssetTimelineKeyframe.h"
#include "StateTimebarlessRow.h"
#include "Renderer.h"
#include "MasterP.h"
#include "StateRow.h"
#include "KeyframeContextMenu.h"
#include "HotKeys.h"
#include "ResourceCache.h"
#include "ITimelineControl.h"
#include "Bindings/ITimelineItemBinding.h"
#include "StudioUtils.h"
#include "TimeEditDlg.h"

CAssetTimelineKeyframe::CAssetTimelineKeyframe(CStateTimebarlessRow *inParentRow,
                                               double inTimeRatio)
    : m_Selected(false)
    , m_ParentRow(inParentRow)
    , m_IsMouseDown(false)
    , m_IsDragging(false)
    , m_TimeRatio(inTimeRatio)
{
    CResourceCache *theCache = CResourceCache::GetInstance();
    m_Icon = theCache->GetBitmap("Keyframe-Master-Normal.png");
    m_DisabledIcon = theCache->GetBitmap("Keyframe-Master-Disabled.png");
    m_SelectedIcon = theCache->GetBitmap("Keyframe-Master-Selected.png");
    m_DynamicIcon = theCache->GetBitmap("Keyframe-MasterDynamic-Normal.png");
    m_DynamicSelectedIcon = theCache->GetBitmap("Keyframe-MasterDynamic-Selected.png");

    m_RightIcon = theCache->GetBitmap("Keyframe-MasterRight-Normal.png");
    m_RightDisabledIcon = theCache->GetBitmap("Keyframe-MasterRight-disabled.png");
    m_RightSelectedIcon = theCache->GetBitmap("Keyframe-MasterRight-Selected.png");
    m_RightDynamicIcon = theCache->GetBitmap("Keyframe-MasterRightDynamic-Normal.png");
    m_RightDynamicSelectedIcon = theCache->GetBitmap("Keyframe-MasterRightDynamic-Selected.png");

    m_LeftIcon = theCache->GetBitmap("Keyframe-MasterLeft-Normal.png");
    m_LeftDisabledIcon = theCache->GetBitmap("Keyframe-MasterLeft-disabled.png");
    m_LeftSelectedIcon = theCache->GetBitmap("Keyframe-MasterLeft-Selected.png");
    m_LeftDynamicIcon = theCache->GetBitmap("Keyframe-MasterLeftDynamic-Normal.png");
    m_LeftDynamicSelectedIcon = theCache->GetBitmap("Keyframe-MasterLeftDynamic-Selected.png");

    m_RectOverHandled = false;
    m_PreviousSelectState = false;
}

CAssetTimelineKeyframe::~CAssetTimelineKeyframe()
{
}

//=============================================================================
/**
 * SetRectOverHandled: Sets if mouse rectangle has been handled
 * param@ inState indicates if the rectangle over has been handled.
 * return@ NONE
 */

void CAssetTimelineKeyframe::SetRectOverHandled(bool inState)
{
    m_RectOverHandled = inState;
}

//=============================================================================
/**
 * GetRectOverHandled: GetRectOverHandled
 * param@ NONE
 * return@ m_RectOverHandled, which indicates if the rectangle over has been handled
 */
bool CAssetTimelineKeyframe::GetRectOverHandled()
{
    return m_RectOverHandled;
}

//=============================================================================
/**
 * SetPreviousSelectState: Sets if the current keyframe was previously selected
 * param@ inState is used to set m_PreviousSelectState.
 * return@ NONE
 */
void CAssetTimelineKeyframe::SetPreviousSelectState(bool inState)
{
    m_PreviousSelectState = inState;
}

//=============================================================================
/**
 * GetPreviousSelectState: Returns the keyframe's previous select state
 * param@ NONE
 * return@ m_PreviousSelectState that stores the select state for the keyframe
 */
bool CAssetTimelineKeyframe::GetPreviousSelectState()
{
    return m_PreviousSelectState;
}

//=============================================================================
/**
* Updates the ToolTip and moves it to the correct place on screen.
* @param inPoint the point that the tooltip is supposed to be placed.
*/
void CAssetTimelineKeyframe::RefreshToolTip(CPt inPoint)
{
    Q3DStudio::CString theCommentText;
    CStateRow *theStateRow = m_ParentRow->GetStateRow();
    CRct theTimelineBounds(theStateRow->GetTopControl()->GetBounds());

    // format label
    theCommentText = " " + ::FormatTimeString(GetTime());

    inPoint.y = GetPosition().y - GetSize().y;
    inPoint.x = GetSize().x / 2;
    ShowMoveableWindow(inPoint, theCommentText, theTimelineBounds);
}

//=============================================================================
/**
 * Gets the correct image and draws
 */
void CAssetTimelineKeyframe::Draw(CRenderer *inRenderer)
{
    inRenderer->DrawBitmap(CPt(0, 0), GetImage());
}

//=============================================================================
/**
 * Gets the name of the current bitmap depending on the state of the button,
 * postion of the mouse, etc.  Returns name of the image for the up state by
 * default.
 * @return name of the image representing current state of the button
 */
QPixmap CAssetTimelineKeyframe::GetImage() const
{
    QPixmap theImage = m_Icon;
    long theStartTime = m_ParentRow->GetStateRow()->GetStartTime();
    long theEndTime = m_ParentRow->GetStateRow()->GetEndTime();

    if (theStartTime == m_Time) {
        theImage = m_LeftIcon;
        if (!IsEnabled())
            theImage = m_LeftDisabledIcon;
        else if (m_IsDynamic) {
            if (m_Selected)
                return m_LeftDynamicSelectedIcon;
            else
                return m_LeftDynamicIcon;
        } else if (m_Selected)
            theImage = m_LeftSelectedIcon;
    } else if (theEndTime == m_Time) {
        theImage = m_RightIcon;
        if (!IsEnabled())
            theImage = m_RightDisabledIcon;
        else if (m_IsDynamic) {
            if (m_Selected)
                return m_RightDynamicSelectedIcon;
            else
                return m_RightDynamicIcon;
        } else if (m_Selected)
            theImage = m_RightSelectedIcon;
    } else {
        if (!IsEnabled())
            theImage = m_DisabledIcon;
        else if (m_IsDynamic) {
            if (m_Selected)
                return m_DynamicSelectedIcon;
            else
                return m_DynamicIcon;
        } else if (m_Selected)
            theImage = m_SelectedIcon;
    }
    return theImage;
}

//=============================================================================
/**
 * @return true if the mouse is over the keyframe
 * @param inPoint the point where the mouse is
 */
bool CAssetTimelineKeyframe::HitTest(const CPt &inPoint) const
{
    bool theRetVal = false;
    // If not over the control then don't bother with specific checks
    if (CControl::HitTest(inPoint)) {
        // If the key is at the beginning or end of the timebar then calculate the test differently
        long theStartTime = m_ParentRow->GetStateRow()->GetStartTime();
        long theEndTime = m_ParentRow->GetStateRow()->GetEndTime();
        CPt thePoint = inPoint - GetPosition();
        if (theStartTime == m_Time)
            theRetVal = (thePoint.x > 7);
        else if (theEndTime == m_Time)
            theRetVal = (thePoint.x < 9);
        else {
            if (m_Selected)
                theRetVal = (thePoint.x > 1 && thePoint.x < 15);
            else
                theRetVal = (thePoint.x > 3 && thePoint.x < 13);
        }
    }
    return theRetVal;
}

//=============================================================================
/**
 * Handler for left mouse down events.
 * @param inPoint the point where the mouse is
 * @param inFlags indicates modifier keys that were down at time of the event
 */
bool CAssetTimelineKeyframe::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // Store the mouse down location in screen coordinates so that we can check the dragging buffer
    // in OnMouseMove
    m_MouseDownLoc = inPoint;

    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        bool theClearPreviouslySelectedKeys = false;
        bool theSelectedFlag = false;
        // If the control key is down then we change state, otherwise
        if (!((CHotKeys::MODIFIER_CONTROL & inFlags) == CHotKeys::MODIFIER_CONTROL)) {
            theClearPreviouslySelectedKeys = !m_Selected; // clear if not multi-selecting
            theSelectedFlag = true;
        } else {
            theSelectedFlag = !m_Selected;
        }
        m_ParentRow->OnKeySelected(m_Time, theSelectedFlag, theClearPreviouslySelectedKeys);
        m_Selected = theSelectedFlag; // set this after OnKeySelected, because the function may
                                      // clear out all previously selected keys including this
        // TODO : sk - 1-1 mapping of seemingly useless calls in
        // CPropertyTimelineKeyframe::OnMouseDown, see my comments there.
        //	m_StudioDoc->UpdateClientScene( true );
        //	m_ParentRow->GetStateRow( )->GetState( )->FireAnimatedPropertiesChanged( );

        m_IsMouseDown = true;
        CStateRow *theStateRow = m_ParentRow->GetStateRow();
        long theStartTime = theStateRow->GetStartTime();
        long theEndTime = theStateRow->GetEndTime();
        m_Snapper.SetStartEndTime(theStartTime, theEndTime);
        m_Snapper.SetSource(this);
        m_Snapper.SetKeyFrameClicked(true);
        m_Snapper.SetSnappingSelectedKeyframes(false);

        theStateRow->GetTimebar()->GetSnappingListProvider().PopulateSnappingList(&m_Snapper);
        m_Snapper.BeginDrag(inPoint.x);

        // display the time range tooltip
        RefreshToolTip(inPoint);
    }
    return true;
}

//=============================================================================
/**
 * Handler for right mouse down events.
 * @param inPoint the point where the mouse is
 * @param inFlags indicates modifier keys that were down at time of the event
 */
bool CAssetTimelineKeyframe::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseRDown(inPoint, inFlags)) {
        if (!m_Selected) {
            m_Selected = true;
            m_ParentRow->OnKeySelected(m_Time, m_Selected, true);
        }
        ITimelineItemProperty *iProperty = nullptr;
        if (GetTimelineItemBinding()->GetPropertyCount() > 0) {
            iProperty = GetTimelineItemBinding()->GetProperty(0);
        }
        CKeyframeContextMenu theMenu(GetTimelineItemBinding()->GetKeyframesManager(), iProperty);
        theMenu.SetTime(GetTime());
        DoPopup(&theMenu, inPoint);
    }

    return true;
}

//=============================================================================
/**
 * called when this key is selected
 *
 * @param inState the state this key is selected to
 */
void CAssetTimelineKeyframe::Select(bool inState)
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
void CAssetTimelineKeyframe::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
    m_IsMouseDown = false;
    m_IsDragging = false;

    GetTimelineItemBinding()->CommitChangedKeyframes();

    HideMoveableWindow();
    Invalidate();
}

//=============================================================================
/**
 * handler for the onMouse Move event.  Offsets selected keys.
 * Displays the StudioToolTip for the keyframe, showing the time it is at.
 *
 * @param inFlags the state of things when the mouse was moved
 * @param inPoint the point where the mouse is
 */
void CAssetTimelineKeyframe::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);
    UICPROFILE(OnMouseMove);
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

        // If we are now dragging, procceed as normal
        if (m_IsDragging) {
            long theNewTime = m_Snapper.ProcessDrag(m_Time, inPoint.x, inFlags);
            long theDiffTime = theNewTime - m_Time;

            if (theDiffTime != 0) {
                // theDiffTime can get updated if its invalid.
                theDiffTime = GetTimelineItemBinding()->OffsetSelectedKeyframes(theDiffTime);
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
 * Sets the time ratio
 *
 * @param inTimeRatio the new ratio
 */
void CAssetTimelineKeyframe::SetTimeRatio(double inTimeRatio)
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
bool CAssetTimelineKeyframe::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);

    GetTimelineItemBinding()->OnEditKeyframeTime(m_Time, ASSETKEYFRAME);
    m_IsMouseDown = false;
    m_IsDragging = false;
    return true;
}

//=============================================================================
/**
 * @return  true if selected
 */
bool CAssetTimelineKeyframe::IsSelected()
{
    return m_Selected;
}

void CAssetTimelineKeyframe::SetSize(CPt inSize)
{
    CControl::SetSize(inSize);
}

//=============================================================================
/**
 *
 */
ITimelineItemBinding *CAssetTimelineKeyframe::GetTimelineItemBinding() const
{
    return m_ParentRow->GetStateRow()->GetTimelineItemBinding();
}
