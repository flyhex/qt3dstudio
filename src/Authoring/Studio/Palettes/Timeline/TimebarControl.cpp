/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "TimebarControl.h"
#include "StateTimebarRow.h"
#include "StateRow.h"
#include "Renderer.h"
#include "ColorControl.h"
#include "StudioPreferences.h"
#include "Views.h"
#include "TimelineControl.h"
#include "TimelineTimelineLayout.h"
#include "ResourceCache.h"
#include "HotKeys.h"
#include "Preferences.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "CoreUtils.h"
#include "StudioUtils.h"
#include "MasterP.h"

const float SCALING_FACTOR = 0.50;

//=============================================================================
/**
 * Create a timebar control on the specified state timebar row.
 * Attaches a new ToolTip to it that displays the time range this timebar control
 * encompasses.
 * @param inRow the row on which this timebar is attached.
 */
CTimebarControl::CTimebarControl(CStateTimebarRow *inRow,
                                 ITimelineItemBinding *inTimelineItemBinding)
    : m_IsSelected(false)
    , m_IsMouseDown(false)
    , m_MaybeDragStart(false)
    , m_LeftLeftTip(this, true)
    , m_LeftTip(this, true)
    , m_RightTip(this, false)
    , m_RightRightTip(this, false)
    , m_SnappingListProvider(nullptr)
{
    m_TimebarRow = inRow;
    m_TimelineItemBinding = inTimelineItemBinding;
    // Start/End times
    ITimelineTimebar *theTimelineTimebar = GetTimebar();
    m_StartTime = theTimelineTimebar->GetStartTime();
    m_EndTime = theTimelineTimebar->GetEndTime();
    bool theShowHandleBars = theTimelineTimebar->ShowHandleBars();
    m_LeftLeftTip.ShowHandles(theShowHandleBars);
    m_RightRightTip.ShowHandles(theShowHandleBars);

    m_LeftLeftTip.SetSize(CStudioPreferences::GetTimebarTipSize(),
                          CStudioPreferences::GetRowSize());
    m_LeftTip.SetPosition(CPt(0, 0));
    m_LeftTip.SetSize(CStudioPreferences::GetTimebarInnerTipSize(),
                      CStudioPreferences::GetRowSize());
    m_LeftTip.SetPosition(CPt(m_LeftLeftTip.GetSize().x, 0));

    m_RightTip.SetSize(CStudioPreferences::GetTimebarInnerTipSize(),
                       CStudioPreferences::GetRowSize());
    m_RightRightTip.SetSize(CStudioPreferences::GetTimebarTipSize(),
                            CStudioPreferences::GetRowSize());

    m_EditControl = new CCommentEdit(theTimelineTimebar);
    m_EditControl->SetPosition(CStudioPreferences::GetTimebarTipSize() * 2, 1);
    m_EditControl->SetSize(CPt(100, 15));
    m_EditControl->SetFillBackground(false);
    AddChild(m_EditControl);
    AddChild(&m_LeftTip);
    AddChild(&m_RightTip);
    AddChild(&m_RightRightTip);
    AddChild(&m_LeftLeftTip);
}

//=============================================================================
/**
 * Destructor
 */
CTimebarControl::~CTimebarControl()
{
    delete m_EditControl;
}

//=============================================================================
/**
 * Draw this timebar control to the renderer.
 * @param inRenderer the renderer to draw to.
 */
void CTimebarControl::Draw(CRenderer *inRenderer)
{
    CStateRow *theRow = m_TimebarRow->GetStateRow();
    CRct theRect(GetSize());

    ::CColor theNormalColor = GetTimebar()->GetTimebarColor();
    ::CColor theSelectedColor = CColorControl::CalculateSelectedColor(theNormalColor);

    ::CColor theBorderColor = CStudioPreferences::GetTimeBarBorderColor();
    ::CColor theDarkExtendedColor = CStudioPreferences::GetExtendedObjectDarkColor();
    ::CColor theLightExtendedColor = CStudioPreferences::GetExtendedObjectLightColor();

    long theTipOffset = CStudioPreferences::GetTimebarTipSize();

    if (!IsEnabled()) {
        theNormalColor = CStudioPreferences::GetLockedTimebarColor();
        theBorderColor = CStudioPreferences::GetLockedBorderColor();
        theDarkExtendedColor = CStudioPreferences::GetExtendedLockedDarkColor();
        theLightExtendedColor = CStudioPreferences::GetExtendedLockedLightColor();
    }

    // Calculate the start/end/activestart
    long theObjectLifeStart = ::TimeToPos(m_StartTime, m_TimeRatio);
    long theStartPos = ::TimeToPos(theRow->GetActiveStart(), m_TimeRatio) - theObjectLifeStart;
    long theEndPos = ::TimeToPos(theRow->GetActiveEnd(), m_TimeRatio) - theObjectLifeStart;
    long theObjectLifeEnd = ::TimeToPos(m_EndTime, m_TimeRatio) - theObjectLifeStart;

    CRct theGradientRct(theStartPos + theTipOffset, 0, theEndPos - theStartPos, theRect.size.y - 1);

    if (theEndPos > theStartPos) {
        inRenderer->DrawGradientBitmap(theGradientRct, theNormalColor, 0, SCALING_FACTOR);
        // Calculate the gradient rect a bit differently depending on selection
        if (m_IsSelected) {
            CRct theSelectedRct(CPt(theGradientRct.position.x, theGradientRct.position.y + 3),
                                CPt(theGradientRct.size.x, theGradientRct.size.y - 7));
            inRenderer->FillSolidRect(theSelectedRct, theSelectedColor);
        }
    }

    inRenderer->PushPen(theBorderColor);
    // Check to see if we need some hashes at the end
    if (theObjectLifeEnd > theEndPos) {
        long theUpdatedStartTime = theEndPos;
        if (theStartPos > theUpdatedStartTime)
            theUpdatedStartTime = theStartPos;
        else {
            inRenderer->MoveTo(theEndPos + theTipOffset, 0);
            inRenderer->LineTo(theEndPos + theTipOffset, theRect.size.y - 1);
        }
        CRct theClippingRect(CPt(theUpdatedStartTime + theTipOffset + 1, 0),
                             CPt(theObjectLifeEnd - theUpdatedStartTime - 1, theRect.size.y - 1));
        inRenderer->PushClippingRect(theClippingRect);

        // Draw the hashed background
        DrawHashedBackgroundX(inRenderer, theDarkExtendedColor, theLightExtendedColor,
                              theClippingRect);
        inRenderer->PopClippingRect();
    }

    // Check to see if we need some hashes at the beginning
    if (theStartPos > 0) {
        long theUpdatedEndTime = theStartPos;
        if (theObjectLifeEnd < theUpdatedEndTime)
            theUpdatedEndTime = theObjectLifeEnd;
        else {
            inRenderer->MoveTo(theStartPos + theTipOffset, 0);
            inRenderer->LineTo(theStartPos + theTipOffset, theRect.size.y - 1);
        }
        CRct theClippingRect(CPt(theTipOffset, 0), CPt(theUpdatedEndTime, theRect.size.y - 1));
        inRenderer->PushClippingRect(theClippingRect);

        // Draw the hashed background
        DrawHashedBackgroundX(inRenderer, theDarkExtendedColor, theLightExtendedColor,
                              theClippingRect);
        inRenderer->PopClippingRect();
    }

    // Draw the border stuff
    inRenderer->MoveTo(CPt(theTipOffset, 0));
    inRenderer->LineTo(CPt(theTipOffset, theRect.size.y - 1));
    inRenderer->MoveTo(CPt(theObjectLifeEnd + theTipOffset, 0));
    inRenderer->LineTo(CPt(theObjectLifeEnd + theTipOffset, theRect.size.y - 1));

    inRenderer->PopPen();
    // Setting the position with the active time
    m_EditControl->SetPosition(CStudioPreferences::GetTimebarTipSize() * 2, 1);
}

//=============================================================================
/**
 * Draws a hashed background in a given clipping rect
 *
 * @param inStartX the x position to start from
 * @param inSizeY the y size the you want the lines to range from
 * @param inEndX one after the last place where lines can be drawn from
 * @inRenderer the renderer to draw to
 * @param inFirstColor the first hash color
 * @param inSecondColor the second hash color
 * @para inRect the clipping rect
 */
void CTimebarControl::DrawHashedBackgroundX(CRenderer *inRenderer, ::CColor inFirstColor,
                                            ::CColor inSecondColor, CRct inRect)
{
    inRenderer->FillSolidRect(inRect, inFirstColor);
    if (m_IsSelected) {
        CRct theSelectedRct(CPt(inRect.position.x, inRect.position.y + 4),
                            CPt(inRect.size.x, inRect.size.y - 8));
        inRenderer->FillSolidRect(
            theSelectedRct, CColorControl::CalculateSelectedColor(GetTimebar()->GetTimebarColor()));
    }

    inRenderer->FillHashed(inRect, inSecondColor);
}

//=============================================================================
/**
 * Set the current time ratio.
 * The time ratio controls the length of this control and is the ratio of
 * pixels to milliseconds.
 * @param inTimeRatio the new time ratio.
 */
void CTimebarControl::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;

    Refresh();
}

//=============================================================================
/**
 * Set the size of this control.
 * @param inSize the new size of this control.
 */
void CTimebarControl::SetSize(CPt inSize)
{
    CControl::SetSize(CPt(inSize.x, inSize.y));

    CStateRow *theRow = m_TimebarRow->GetStateRow();
    long theTipSize =
        CStudioPreferences::GetTimebarTipSize() + CStudioPreferences::GetTimebarInnerTipSize();
    long theCommentSize = CStudioPreferences::GetDefaultCommentSize();
    if (inSize.x < theCommentSize)
        theCommentSize = inSize.x;

    // Recalculate the comment size depending on where the timebar is and how large it is
    long theDiff = ::dtol((theRow->GetActiveEnd() - theRow->GetActiveStart()) * m_TimeRatio);
    if (theDiff < theCommentSize) {
        theCommentSize = theDiff - theTipSize;
        if (theCommentSize < 0)
            theCommentSize = 0;
    }

    m_EditControl->SetSize(CPt(theCommentSize, 15));

    // Set the two right tips depending on where the right side is
    m_RightTip.SetPosition(CPt(inSize.x - theTipSize, 0));
    m_RightRightTip.SetPosition(CPt(inSize.x - m_RightRightTip.GetSize().x + 1, 0));
}

//=============================================================================
/**
 * Set whether this control is selected or not.
 * If this is selected then it will modify how this control looks.
 * @param inIsSelected true if this control is to be selected.
 */
void CTimebarControl::SetSelected(bool inIsSelected)
{
    if (inIsSelected != m_IsSelected) {
        m_IsSelected = inIsSelected;
        m_EditControl->SetSelected(m_IsSelected);
        Invalidate();
    }
}

void CTimebarControl::RefreshMetaData()
{
    m_EditControl->RefreshMetaData();
}

//=============================================================================
/**
 * Request for this control to refresh it's properties.
 * This checks the size of the asset and adjusts it's size the the asset's
 * length. Called when the time ratio or properties have changed.
 * If the time has changed then Refresh( long, long ) must be called with the
 * new times.
 */
void CTimebarControl::Refresh()
{
    Refresh(m_StartTime, m_EndTime);
}

//=============================================================================
/**
 * Request for this control to refresh it's properties.
 * This updates all the properties of this control and resize it as necessary.
 * Called when the time changes on the asset, the time ratio changes or any
 * properties that this displays change.
 * @param inStartTime the asset's start time.
 * @param inEndTime the asset's end time.
 */
void CTimebarControl::Refresh(long inStartTime, long inEndTime)
{
    m_StartTime = inStartTime;
    m_EndTime = inEndTime;

    long thePosition = ::TimeToPos(inStartTime, m_TimeRatio);
    long theSize = ::dtol((inEndTime - inStartTime) * m_TimeRatio);

    SetPosition(thePosition - CStudioPreferences::GetTimebarTipSize(), GetPosition().y);

    SetSize(CPt(theSize + 2 * CStudioPreferences::GetTimebarTipSize(), GetMinimumSize().y));
    if (IsInvalidated())
        m_TimebarRow->Invalidate();
}

//=============================================================================
/**
 *	Get the interface to the timebar item in the data model
 */
ITimelineTimebar *CTimebarControl::GetTimebar()
{
    return m_TimelineItemBinding->GetTimelineItem()->GetTimebar();
}

//=============================================================================
/**
* Updates the ToolTip and moves it to the correct place on screen.
* @param inPoint the point that the tooltip is supposed to be placed.
*/
void CTimebarControl::RefreshToolTip(CPt inPoint)
{
    Q3DStudio::CString theCommentText;
    CStateRow *theRow = m_TimebarRow->GetStateRow();

    CRct theTimelineBounds(GetTopControlBounds());
    // format label as: startTime - endTime (timeDifference)
    theCommentText = " " + FormatTimeString(theRow->GetStartTime()) + " - "
        + FormatTimeString(theRow->GetEndTime()) + " ("
        + FormatTimeString(theRow->GetEndTime() - theRow->GetStartTime()) + ")";
    inPoint.y = GetPosition().y - GetSize().y;
    ShowMoveableWindow(inPoint, theCommentText, theTimelineBounds);
}

//=============================================================================
/**
  * OnMouseDoubleClick: Pop up a dialog box for the editing of the timebar start
  *                     and end time.
  */
bool CTimebarControl::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDoubleClick(inPoint, inFlags)
        && !m_TimelineItemBinding->IsLockedEnabled()) {
        CDurationEditDlg theDurationEditDlg;
        theDurationEditDlg.showDialog(m_StartTime, m_EndTime, g_StudioApp.GetCore()->GetDoc(),
                                      this);
    }

    return true;
}

//=============================================================================
/**
 * Allows this timebar control to add any times it wishes to the snapper list
 * @param inSnapper the Snapper that is handling the snapping functions for this timebar
 */
void CTimebarControl::PopulateSnappingList(CSnapper *inSnapper)
{
    Q_UNUSED(inSnapper);
}

//=============================================================================
/**
 * Start drag handler, puts this control into drag mode.
 * @param inPoint the point where the mouse was clicked.
 * @param inFlags the mouse state.
 */
bool CTimebarControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_IsMouseDown = true;
        m_MaybeDragStart = true;
        m_MouseDownLoc = inPoint;

        OnBeginDrag();

        m_TimebarRow->GetStateRow()->Select(SBaseStateRowSelectionKeyState());

        m_Snapper.Clear();
        m_Snapper.SetSource(this);

        GetSnappingListProvider().PopulateSnappingList(&m_Snapper);
        m_Snapper.BeginDrag(inPoint.x);

        if (HasFocus(m_EditControl) && !m_EditControl->HitTest(inPoint)) {
            m_EditControl->OnLoseFocus();
        }

        // display the time range tooltip
        RefreshToolTip(inPoint);
    }
    return true;
}

//=============================================================================
/**
 * Puts up the context menu.
 * @param inPoint the point where the mouse was clicked.
 * @param inFlags the mouse state.
 */
bool CTimebarControl::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseRDown(inPoint, inFlags)) {
        if (m_IsMouseDown) {
            m_IsMouseDown = false;
            CommitTimeChange();
            HideMoveableWindow();
        }
        // only right-clicking ON the timebar will show the timebar (text and color) properties'
        // options
        ShowContextMenu(inPoint, true);
    }

    return true;
}

//=============================================================================
/**
 * Notification that the drag has finished.
 * @param inPoint the point where the mouse was let go.
 * @param inFlags the state of the mouse.
 */
void CTimebarControl::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // try to prevent stuck mousetips on exceptions
    try {
        CControl::OnMouseUp(inPoint, inFlags);
        CommitTimeChange();
    } catch (...) {
    }
    m_IsMouseDown = false;
    m_MaybeDragStart = false;
    HideMoveableWindow();
}

//=============================================================================
/**
 * Handler for the mouse move messages.
 * If the mouse is down then this will drag the control and offset the timebar.
 * @param inPoint the current location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CTimebarControl::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    bool theCommentEditMode = m_EditControl->GetEditMode();

    // If we are in edit Comment mode or locked, then we do not drag the timebar.
    if (!theCommentEditMode && m_IsMouseDown && !m_TimelineItemBinding->IsLockedEnabled()) {
        QT3DS_PROFILE(OnMouseMove);

        if (m_MaybeDragStart) {
            // Dragging in the first 5 pixels will be ignored to avoid unconsciously accidental
            // moves
            CPt theDragDistance = inPoint - m_MouseDownLoc;
            if (theDragDistance.x * theDragDistance.x + theDragDistance.y * theDragDistance.y <= 25)
                return;

            m_MaybeDragStart = false;
        }

        long theNewTime = m_Snapper.ProcessDrag(m_StartTime, inPoint.x, inFlags);
        if (theNewTime < 0)
            theNewTime = 0;
        long theDiffTime = theNewTime - m_StartTime;

        if (theDiffTime) {
            GetTimebar()->OffsetTime(theDiffTime);
            // display the time range tooltip
            RefreshToolTip(inPoint);
        }
    }
}

//=============================================================================
/**
 * Call from the left TimebarTab to resize the control.
 * @param inTime the time to set the start time to.
 */
void CTimebarControl::ResizeTimebarLeftTo(long inTime)
{
    // TOOD: sk - Figure out what this does
    // if ( inTime != 0 )
    {
        // The whole idea is to not do anything additional once times passes 0 (negatively)
        // unless it is valid that time is negative on the timebar
        if (inTime < 0 && m_StartTime > 0)
            inTime = -m_StartTime; // so that it decrements to 0

        if (m_StartTime > 0 || (m_StartTime == 0 && inTime > 0))
            GetTimebar()->ChangeTime(inTime, true);
    }
}

//=============================================================================
/**
 * Call from the right TimebarTab to resize the control.
 * @param inTime the time to set the start time to.
 */
void CTimebarControl::ResizeTimebarRightTo(long inTime)
{
    GetTimebar()->ChangeTime(inTime, false);
}

//=============================================================================
/**
 * Sets the Actual string of text
 * @param inText the text to set the comment text to
 */
void CTimebarControl::SetText(const Q3DStudio::CString &inText)
{
    m_EditControl->SetData(inText);
}

//=============================================================================
/**
 * Sets the Text Color on the edit control
 * @param inColor the color
 */
void CTimebarControl::SetTextColor(::CColor inColor)
{
    m_EditControl->SetTextColor(inColor);
}

long CTimebarControl::GetStartTime()
{
    return m_StartTime;
}

long CTimebarControl::GetEndTime()
{
    return m_EndTime;
}

//=============================================================================
/**
 * Sets whether or not this control is enabled.
 * If the control is not enabled then it is still drawn and still intercepts
 * mouse clicks, but it will not actually process them.
 * @param inIsEnabled true if this control is to be enabled.
 */
void CTimebarControl::SetEnabled(bool inIsEnabled)
{
    CControl::SetEnabled(inIsEnabled);
}

//=============================================================================
/**
 * COMMENT!!!!!!!!!!!!!!!!!!!!!!
 */
void CTimebarControl::OnLoseFocus()
{
    if (m_IsMouseDown) {
        m_IsMouseDown = false;
        CommitTimeChange();
        HideMoveableWindow();
    }
    CControl::OnLoseFocus();
}

//=============================================================================
/**
 * Setup prior to dragging.
 */
void CTimebarControl::OnBeginDrag()
{
    GetTimebar()->OnBeginDrag();
}

void CTimebarControl::ChangeStartTime(long inTime)
{
    ResizeTimebarLeftTo(inTime);
}

void CTimebarControl::ChangeEndTime(long inTime)
{
    ResizeTimebarRightTo(inTime);
}

void CTimebarControl::Commit()
{
    GetTimebar()->CommitTimeChange();
}
void CTimebarControl::Rollback()
{
    GetTimebar()->RollbackTimeChange();
}

void CTimebarControl::ShowContextMenu(CPt inPoint, bool inShowTimebarPropertiesOptions)
{
    CTimebarKeyframeContextMenu theMenu(this, m_TimelineItemBinding->GetKeyframesManager(),
                                        inShowTimebarPropertiesOptions);
    DoPopup(&theMenu, inPoint);
}

void CTimebarControl::CommitTimeChange()
{
    GetTimebar()->CommitTimeChange();
}

//=============================================================================
/**
 *	The binding is a keyframes holder
 */
ITimelineItemKeyframesHolder *CTimebarControl::GetKeyframesHolder()
{
    return m_TimelineItemBinding;
}

//=============================================================================
/**
 * Start editing the timebar comment
 */
void CTimebarControl::OnEditTimeComment()
{
    GrabFocus(m_EditControl);
    m_EditControl->DoChangeComment();
}

//=============================================================================
/**
 * Need to invalidate all timebars to redraw
 */
void CTimebarControl::OnToggleTimebarHandles()
{
    Invalidate();
}

void CTimebarControl::SetTimebarTime()
{
    GetTimebar()->SetTimebarTime(this);
}

::CColor CTimebarControl::GetTimebarColor()
{
    return GetTimebar()->GetTimebarColor();
}

void CTimebarControl::SetTimebarColor(const ::CColor &inColor)
{
    GetTimebar()->SetTimebarColor(inColor);
}

void CTimebarControl::SetSnappingListProvider(ISnappingListProvider *inProvider)
{
    m_SnappingListProvider = inProvider;
}

ISnappingListProvider &CTimebarControl::GetSnappingListProvider() const
{
    // sk - If you hit this, it means the setup order is incorrect. e.g. loading children is done
    // depth first, ie your child's children is loaded before parent, doesn't work that way.
    ASSERT(m_SnappingListProvider);
    return *m_SnappingListProvider;
}

CRct CTimebarControl::GetTopControlBounds() const
{
    return m_TimebarRow->GetStateRow()->GetTopControl()->GetBounds();
}
