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

//=============================================================================
// Prefix
//=============================================================================
#include "stdafx.h"

//=============================================================================
// Includes
//=============================================================================
#include "TimelineTimelineLayout.h"
#include "TimeMeasure.h"
#include "ScalableScroller.h"
#include "StudioUtils.h"
#include "TimelineRow.h"
#include "TimelineControl.h"
#include "StateRow.h"
#include "Snapper.h"
#include "Bindings/TimelineTranslationManager.h"
#include "ControlData.h"
#include "HotKeys.h"
#include "foundation/Qt3DSLogging.h"

//=============================================================================
// Defines
//=============================================================================
// For Win the modifier key for keyframe multi selection is the control key.
#define MODIFIER_KEY CHotKeys::MODIFIER_CONTROL

//=============================================================================
// Class constants
//=============================================================================
const double DEFAULT_TIME_RATIO = .05;
const double CTimelineTimelineLayout::SCALING_PERCENTAGE_INC = 1.1;
const double CTimelineTimelineLayout::SCALING_PERCENTAGE_DEC = 0.9;
const double CTimelineTimelineLayout::MAX_ZOOM_OUT = 7e-005;

//=============================================================================
/**
 * Constructor
 */
CTimelineTimelineLayout::CTimelineTimelineLayout(CTimelineControl *inTimelineControl, IDoc *inDoc)
    : m_Playhead(this, inDoc)
    , m_IsLayoutChanged(false)
    , m_IsMouseDown(false)
{
    m_ControlData->SetMouseWheelEnabled(true);
    m_TimelineControl = inTimelineControl;

    m_TimeRatio = DEFAULT_TIME_RATIO + .01;
    m_TimeMeasure = new CTimeMeasure(this, m_TimeRatio);
    m_Scroller = new CScalableScroller();

    m_Scroller->SetVerticalScrollMode(CScroller::ALWAYS);
    m_Scroller->SetHorizontalScrollMode(CScroller::ALWAYS);
    m_Scroller->AddScrollListener(this);
    m_BoundingRect = new CAreaBoundingRect();
    m_BoundingRect->SetName("TimelineAreaBoundingRect");
    m_BoundingRect->SetVisible(false);
    m_BoundingRect->SetAlpha(128);

    AddChild(m_TimeMeasure);
    AddChild(m_Scroller);
    AddChild(&m_Playhead);
    AddChild(m_BoundingRect);

    // Blank control filling in the bottom of the timeline, under the rows
    CBlankControl *theTimelineBlankControl = new CBlankControl();
    m_TimebarList = new CFlowLayout(theTimelineBlankControl);

    m_Scroller->AddChild(m_TimebarList);
    m_Scroller->SetScalingListener(this);
    m_TimebarList->SetName("TimelineTimelineLayoutList");

    // Initializing flags for keyframe multi select to work.
    m_CommitKeyframeSelection = false;
}

//=============================================================================
/**
 * Destructor
 */
CTimelineTimelineLayout::~CTimelineTimelineLayout()
{
    delete m_TimeMeasure;
    delete m_Scroller;
    delete m_TimebarList;
    delete m_BoundingRect;
}

//=============================================================================
/**
 * Clear all the StateRows out of the top-level list.
 * This is used when the current presentation is being cleared out.
 */
void CTimelineTimelineLayout::ClearRows()
{
    TTimelineRowList::iterator thePos = m_Rows.begin();
    for (; thePos != m_Rows.end(); ++thePos) {
        CTimelineRow *theRow = (*thePos);
        m_TimebarList->RemoveChild(theRow->GetTimebarControl());
    }

    m_Rows.clear();
}

//=============================================================================
/**
 * Set the size of this control.
 * Overrrides CControl::SetSize so that this can redo the layout of all inner
 * controls.
 * @param inSize the new size of this control.
 */
void CTimelineTimelineLayout::SetSize(CPt inSize)
{
    CControl::SetSize(inSize);

    RecalcLayout();
}

//=============================================================================
/**
 * Recalculate the positioning of all the child components.
 */
void CTimelineTimelineLayout::RecalcLayout()
{
    CPt mySize = GetSize();
    // Put the time measure on top taking 21 pixels high.
    m_TimeMeasure->SetSize(CPt(mySize.x, 21));
    m_TimeMeasure->SetPosition(CPt(0, 0));

    // Make the scroller take up the rest of the space.
    m_Scroller->SetSize(CPt(mySize.x, mySize.y - 42));
    m_Scroller->SetPosition(CPt(0, 21));

    // Make it the full length of the view, minus the bottom scroll bar.
    m_Playhead.SetSize(CPt(13, GetSize().y - m_Scroller->GetHorizontalBar()->GetSize().y - 21));

    long theMinTime = -(m_Playhead.GetCenterOffset());

    if (!m_Rows.empty()) {
        long theLatestTime = 0;
        TTimelineRowList::iterator thePos = m_Rows.begin();
        for (; thePos != m_Rows.end(); ++thePos) {
            CTimelineRow *theRow = (*thePos);
            long theRowLatestTime = theRow->GetLatestEndTime();
            if (theRowLatestTime > theLatestTime)
                theLatestTime = theRowLatestTime;
        }

        long theMinWidth = ::TimeToPos(theLatestTime, m_TimeRatio) + END_BUFFER_SIZE;
        long theMinHeight = m_TimebarList->GetMinimumSize().y;

        CPt theVisSize = m_Scroller->GetVisibleSize();

        if (theMinHeight < theVisSize.y)
            theMinHeight = theVisSize.y;
        if (theMinWidth < theVisSize.x)
            theMinWidth = theVisSize.x;

        m_TimebarList->ResetMinMaxPref();
        m_TimebarList->SetAbsoluteSize(CPt(theMinWidth, theMinHeight));
    }

    // Set up the limits.
    m_Playhead.SetMinMaxPosition(theMinTime, mySize.x - m_Scroller->GetVerticalBar()->GetSize().x);

    // Set playhead to time 0.
    SetTime(m_TimelineControl->GetTranslationManager()->GetCurrentViewTime(), true);

    // Reset! so that this isn't unnecessarily run
    m_IsLayoutChanged = false;
}

//=============================================================================
/**
 * Add a timeline row to this object.
 * This will add the row as a top level object.
 * @param inRow the row to be added.
 */
void CTimelineTimelineLayout::AddRow(CTimelineRow *inRow)
{
    m_Rows.push_back(inRow);

    m_TimebarList->AddChild(inRow->GetTimebarControl());

    inRow->SetTimeRatio(m_TimeRatio);
    // For keyframe/timebar snapping.
    inRow->SetSnappingListProvider(this);
}

//=============================================================================
/**
 * Call from the ScalableScroller that it is scaling the right side of the timebar.
 * @param inLength the length that the thumb wants to be.
 * @param inTotalLength the maximum length that the thumb can be.
 * @param inOffset the offset of the thumb position.
 */
void CTimelineTimelineLayout::OnScalingRight(long inLength, long inTotalLength, long inOffset)
{
    double theViewSize = m_Scroller->GetVisibleSize().x;
    double theClientSize = m_Scroller->GetContaineeSize().x;
    double theLength = inLength;
    double theTotalLength = inTotalLength;

    double theTimeRatio =
        (theViewSize * theTotalLength) / (theClientSize * theLength) * m_TimeRatio;

    // This means the bar was dragged to the far end, just prevent it for getting wacky.
    if (theTimeRatio > 0) {
        // This will set the time ratio, but will cap it at 1 or MAX_ZOOM_OUT so if the Time ratio
        // less than max, don't need to move the timeline
        SetTimeRatio(theTimeRatio);
        if (theTimeRatio < 1) {
            double theMaxVisPos = m_Scroller->GetMaxVisiblePosition().x;
            long theVisiblePosition = ::dtol(theMaxVisPos * inOffset / (inTotalLength - inLength));
            m_Scroller->SetVisiblePosition(
                CPt(theVisiblePosition, m_Scroller->GetVisiblePosition().y));
        }
    }
}

//=============================================================================
/**
 * Under construction.
 */
void CTimelineTimelineLayout::OnScalingLeft(long inLength, long inTotalLength, long inOffset)
{
    // Hey- look at that, doesn't matter which side you're scaling.
    // Hey, nice comment especially the function header
    OnScalingRight(inLength, inTotalLength, inOffset);
}

void CTimelineTimelineLayout::OnScalingReset()
{
    SetTimeRatio(DEFAULT_TIME_RATIO);
}

//=============================================================================
/**
 * Set the TimeRatio to be used.
 * This will propagate the time ratio down to all the child items.
 * @param inTimeRatio the time ratio to be set.
 */
void CTimelineTimelineLayout::SetTimeRatio(double inTimeRatio)
{
    if (inTimeRatio != m_TimeRatio) {
        if (inTimeRatio > 1)
            inTimeRatio = 1;
        //		if ( inTimeRatio < MAX_ZOOM_OUT )
        //			inTimeRatio = MAX_ZOOM_OUT;

        m_TimeRatio = inTimeRatio;
        m_TimeMeasure->SetTimeRatio(inTimeRatio);

        TTimelineRowList::iterator thePos = m_Rows.begin();
        for (; thePos != m_Rows.end(); ++thePos) {
            CTimelineRow *theRow = (*thePos);
            theRow->SetTimeRatio(inTimeRatio);
        }

        RecalcLayout();

        // store the timeline ratio
        SetTimelineRatio(m_TimelineControl->GetActiveSlide(), m_TimeRatio);
        qCInfo(qt3ds::TRACE_INFO) << "Set time ratio: " << inTimeRatio;
    }
}

//==============================================================================
/**
 * When timeline layout has changed. RecalcLayout should be called to adjust the scrollbars if
 * a asset is expanded/collapsed in the timeline.
 */
void CTimelineTimelineLayout::OnTimelineLayoutChanged()
{
    RecalcLayout();

    // In addition, this has to be 'marked' for if SetScrollerPositionY is called due to
    // new assets being added, RecalcLayout has to be called again.
    m_IsLayoutChanged = true;
}

/**
 *	Deletes the time zoom ratio for a particular slide.
 *	@param inContext the time context of that slide to delete
 */
void CTimelineTimelineLayout::DeleteTimelineRatio(qt3dsdm::Qt3DSDMSlideHandle inSlide)
{
    m_TimelineRatio.erase(inSlide);
}

/**
 *	Clear all entries
 */
void CTimelineTimelineLayout::ClearAllTimeRatios()
{
    m_TimelineRatio.clear();
}

/**
 *	Retrieves the time zoom ratio for a particular slide
 *	@param inContext the time context of that slide to retrieve zoom ratio
 *  @return the zoom ratio, or -1 if it's not found
 */
double CTimelineTimelineLayout::GetTimelineRatio(qt3dsdm::Qt3DSDMSlideHandle inSlide)
{
    TSlideRatioMap::iterator theResult = m_TimelineRatio.find(inSlide);
    if (theResult != m_TimelineRatio.end())
        return theResult->second;
    else
        return -1;
}

/**
 *	Sets the time zoom ratio for a particular slide
 *	@param inContext the time context of that slide
 *  @param inRatio the zoom factor
 */
void CTimelineTimelineLayout::SetTimelineRatio(qt3dsdm::Qt3DSDMSlideHandle inSlide, double inRatio)
{
    m_TimelineRatio[inSlide] = inRatio;
}

//=============================================================================
/**
 * For testing purposes.
 */
long CTimelineTimelineLayout::GetMaximumTimebarTime()
{
    return 30000;
}

//=============================================================================
/**
 * Call from the TimelineView to notifiy this that some of its objects got filtered.
 * This was used for redoing the layout but is no longer necessary.
 */
void CTimelineTimelineLayout::Filter()
{
}

//=============================================================================
/**
 * Notification from the CScroller that it is scrolling.
 * This will update the other views with the verticall scrolling and update
 * the TimeMeasure with the horizontal scroll amount.
 * @param inScrollAmount the amount that was scrolled by.
 */
void CTimelineTimelineLayout::OnScroll(CScroller *inSource, CPt inScrollAmount)
{
    Q_UNUSED(inSource);

    m_TimelineControl->SetScrollPositionY(m_Scroller, m_Scroller->GetVisiblePosition().y);

    long theTimeOffset = GetViewTimeOffset();
    m_TimeMeasure->SetTimeOffset(theTimeOffset);

    long thePlayheadPos =
        ::TimeToPos(m_TimelineControl->GetTranslationManager()->GetCurrentViewTime()
                        - theTimeOffset,
                    m_TimeRatio)
        - m_Playhead.GetCenterOffset();

    m_Playhead.SetPosition(CPt(thePlayheadPos, 0));

    m_DragBeginPoint += inScrollAmount;
}

void CTimelineTimelineLayout::SetScrollPositionY(CScroller *inSource, long inPositionY,
                                                 bool inAbsolute)
{
    Q_UNUSED(inSource);

    CPt theVisPos = m_Scroller->GetVisiblePosition();

    if (!inAbsolute) {
        CPt theMaxSize = m_Scroller->GetMaxVisiblePosition();

        CRct theVisibleRect(CPt(theVisPos.x, theMaxSize.y - theVisPos.y),
                            m_Scroller->GetVisibleSize());
        CPt thePoint(theVisPos.x, inPositionY);
        if (!theVisibleRect.IsInRect(thePoint))
            m_Scroller->SetVisiblePosition(CPt(theVisPos.x, inPositionY));
    } else {
        // For new assets added, RecalcLayout needs be called here if there was a layout changed
        // because
        // m_TimebarList->GetMinimumSize( ).y is only updated at this point, otherwise the tree and
        // layout will
        // go out of sync.
        if (m_IsLayoutChanged)
            RecalcLayout();

        m_Scroller->SetVisiblePosition(CPt(theVisPos.x, inPositionY));
    }
}

//=============================================================================
/**
 * Get the scroller control this is using.
 * Meant for testing purposes.
 * @return the scroller this is using.
 */
CScalableScroller *CTimelineTimelineLayout::GetScroller()
{
    return m_Scroller;
}

//=============================================================================
/**
 * Get the playhead control this is using.
 * Meant for testing purposes.
 * @return the playhead this is using.
 */
CPlayhead *CTimelineTimelineLayout::GetPlayhead()
{
    return &m_Playhead;
}

//=============================================================================
/**
 * Scroll the contents of the timeline horizontally.
 * This is used mainly by the playhead to scroll the view when it gets to the
 * edge.
 * @param inAmount the amount to scroll the view by.
 * @return the amount actually scrolled, limited by min/max values.
 */
long CTimelineTimelineLayout::ScrollLayout(long inAmount)
{
    // Log the current position for returning
    CPt thePosition = m_Scroller->GetVisiblePosition();

    m_Scroller->SetVisiblePosition(CPt(thePosition.x + inAmount, thePosition.y));

    // Return how much was actually scrolled, let the scroller handle min/max scroll amounts.
    return m_Scroller->GetVisiblePosition().x - thePosition.x;
}

//=============================================================================
/**
 * Recalculate what the time is based on the location of the playhead.
 * This will call SetTime on the TimelineView with the new time.
 * @param inUpdateClient true if the client time should be updated.
 */
void CTimelineTimelineLayout::RecalcTime(bool inUpdateClient, long inFlags)
{
    long theOffset = m_Playhead.GetPosition().x + m_Playhead.GetCenterOffset()
        + m_Scroller->GetVisiblePosition().x;

    long theTime = ::PosToTime(theOffset, m_TimeRatio);
    m_Snapper.InterpretTimeEx(theTime, inFlags);

    // Update the time
    m_Playhead.UpdateTime(theTime, inUpdateClient);
}

//=============================================================================
/**
 * Call from the timeline view that the time is changing.
 * @param inNewTime the new time.
 * @param inIsSecondary lame flag to prevent infinite recursion.
 */
void CTimelineTimelineLayout::SetTime(long inNewTime, bool inIsSecondary)
{
    long theOffset = ::TimeToPos(inNewTime, m_TimeRatio);
    theOffset -= m_Scroller->GetVisiblePosition().x + m_Playhead.GetCenterOffset();

    long theViewSize = m_Scroller->GetVisibleSize().x;

    if (!inIsSecondary) {
        if (theOffset < -m_Playhead.GetCenterOffset()) {
            long thePos = ::TimeToPos(inNewTime, m_TimeRatio) - m_Playhead.GetCenterOffset();
            m_Scroller->SetVisiblePosition(CPt(thePos, m_Scroller->GetVisiblePosition().y));
        } else if (theOffset > (theViewSize - (m_Playhead.GetCenterOffset() + 20))) {
            long thePos = ::TimeToPos(inNewTime, m_TimeRatio) + 20;
            thePos -= theViewSize;
            m_Scroller->SetVisiblePosition(CPt(thePos, m_Scroller->GetVisiblePosition().y));
        }
        SetTime(inNewTime, true);
    } else {
        m_Playhead.SetPosition(CPt(theOffset, m_Playhead.GetPosition().y));
    }
}

//=============================================================================
/**
 * Notification that the TimeMeasure was clicked on.
 * This is used to reposition the playhead wherever the mouse was clicked.
 * @param inPoint the location of the mouse local to the time measure.
 */
void CTimelineTimelineLayout::OnTimeMeasureMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inFlags);

    m_Snapper.Clear();
    m_Snapper.SetSource(&m_Playhead);
    PopulateSnappingList(&m_Snapper);
    m_Snapper.SetSnappingKeyframes(true);

    m_Playhead.SetPosition(
        CPt(inPoint.x - m_Playhead.GetCenterOffset(), m_Playhead.GetPosition().y));
    RecalcTime(true, inFlags);
}

//=============================================================================
/**
 * Handles left-clicks.  Starts a drag operation if a child does not handle the
 * message.
 * @param inPoint location of the mouse when event occurred
 * @param inFlags state of modifier keys when event occurred
 */
bool CTimelineTimelineLayout::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_BoundingRect->SetSize(CPt(0, 0));
        m_BoundingRect->SetVisible(true);

        // Do not deselect all keyframes as the user intends to select more keyframes,
        // when the modifier key is pressed.
        if (!(inFlags & MODIFIER_KEY))
            m_TimelineControl->GetTranslationManager()->ClearKeyframeSelection();

        m_IsMouseDown = true;
        m_DragBeginPoint = inPoint;
    }
    return true;
}

void CTimelineTimelineLayout::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    if (m_IsMouseDown) {
        CPt theSize;
        CRct theRect;

        // Tests if the user has pressed the modifier key, while moving the mouse.
        bool theModifierKeyDown;
        if (inFlags & MODIFIER_KEY)
            theModifierKeyDown = true;
        else
            theModifierKeyDown = false;

        // Calculate the rect for the bounding box
        theSize = CPt(inPoint.x - m_DragBeginPoint.x, inPoint.y - m_DragBeginPoint.y);
        theRect = CRct(m_DragBeginPoint, theSize);
        theRect.Normalize();
        m_BoundingRect->SetPosition(theRect.position);
        m_BoundingRect->SetSize(theRect.size);
        theRect.Offset(-m_Scroller->GetPosition());
        theRect.Offset(m_Scroller->GetVisiblePosition());

        // Select all keys inside the rect

        TTimelineRowList::iterator thePos = m_Rows.begin();

        for (; thePos != m_Rows.end(); ++thePos) {
            CStateRow *theRow = reinterpret_cast<CStateRow *>(*thePos);
            theRow->SelectKeysInRect(theRect, theModifierKeyDown, m_CommitKeyframeSelection);
        }
        m_CommitKeyframeSelection = false;
    }
}

void CTimelineTimelineLayout::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // try to prevent stuck mousetips on exceptions
    try {
        CControl::OnMouseUp(inPoint, inFlags);
        m_BoundingRect->SetVisible(false);
    } catch (...) {
    }

    m_IsMouseDown = false;

    // Commits the key frame selection. This finalises the keyframes selection
    // in the rect. When the mouse is down again, we would be able to append
    // the commited keyframes with the new batch of keyframes.
    m_CommitKeyframeSelection = true;
}

void CTimelineTimelineLayout::PopulateSnappingList(CSnapper *inSnappingList)
{
    CRct theArea(m_Scroller->GetVisibleSize());
    theArea.Offset(-m_Scroller->GetPosition());
    theArea.Offset(m_Scroller->GetVisiblePosition());

    inSnappingList->SetVisibleArea(theArea.position.y, theArea.size.y);

    inSnappingList->SetTimeRatio(m_TimeRatio);
    if (inSnappingList->GetSource() != &m_Playhead)
        inSnappingList->AddTime(m_Playhead.GetCurrentTime());

    m_TimeMeasure->PopulateSnappingList(inSnappingList);

    TTimelineRowList::iterator theRowIter = m_Rows.begin();
    for (; theRowIter != m_Rows.end(); ++theRowIter) {
        (*theRowIter)->PopulateSnappingList(inSnappingList);
    }
}

long CTimelineTimelineLayout::GetViewTimeOffset()
{
    return ::dtol(m_Scroller->GetVisiblePosition().x / m_TimeRatio);
}

CTimeMeasure *CTimelineTimelineLayout::GetTimeMeasure()
{
    return m_TimeMeasure;
}

//=============================================================================
/**
 * Register all the events for hotkeys that are active for the entire application.
 * Hotkeys for the entire application are ones that are not view specific in
 * scope.
 * @param inShortcutHandler the global shortcut handler.
 */
void CTimelineTimelineLayout::RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler)
{
    inShortcutHandler->RegisterKeyDownEvent(new CDynHotKeyConsumer<CTimelineTimelineLayout>(
                                                this, &CTimelineTimelineLayout::OnScalingZoomIn),
                                            0, Qt::Key_Plus);
    inShortcutHandler->RegisterKeyDownEvent(new CDynHotKeyConsumer<CTimelineTimelineLayout>(
                                                this, &CTimelineTimelineLayout::OnScalingZoomOut),
                                            0, Qt::Key_Minus);
    inShortcutHandler->RegisterKeyDownEvent(new CDynHotKeyConsumer<CTimelineTimelineLayout>(
                                                this, &CTimelineTimelineLayout::OnScalingZoomIn),
                                            Qt::KeypadModifier, Qt::Key_Plus);
    inShortcutHandler->RegisterKeyDownEvent(new CDynHotKeyConsumer<CTimelineTimelineLayout>(
                                                this, &CTimelineTimelineLayout::OnScalingZoomOut),
                                            Qt::KeypadModifier, Qt::Key_Minus);
}

//=============================================================================
/**
 * Call from the Hotkey that it is zooming in the timebar.
 */

void CTimelineTimelineLayout::OnScalingZoomIn()
{
    double theTimeRatio = m_TimeRatio * SCALING_PERCENTAGE_INC;

    SetTimeRatio(theTimeRatio);
    CenterToPlayhead();
}

//=============================================================================
/**
 * Call from the Hotkey that it is zooming out of the timebar.
 */

void CTimelineTimelineLayout::OnScalingZoomOut()
{
    double theTimeRatio = m_TimeRatio * SCALING_PERCENTAGE_DEC;

    SetTimeRatio(theTimeRatio);
    CenterToPlayhead();
}

void CTimelineTimelineLayout::CenterToPlayhead()
{
    long theTime = m_Playhead.GetCurrentTime();
    long thePos = ::TimeToPos(theTime, m_TimeRatio);
    long theNewPosX = thePos - (m_Scroller->GetSize().x / 2);

    m_Scroller->SetVisiblePosition(CPt(theNewPosX, m_Scroller->GetVisiblePosition().y));
}

//==============================================================================
/**
 * Handle mouse wheel messages to allow zooming
 */
bool CTimelineTimelineLayout::OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;
    if (inFlags & CHotKeys::MODIFIER_CONTROL) {
        if (inAmount > 0)
            OnScalingZoomIn();
        else
            OnScalingZoomOut();
        theRetVal = true;
    } else
        theRetVal = CControl::OnMouseWheel(inPoint, inAmount, inFlags);
    return theRetVal;
}
