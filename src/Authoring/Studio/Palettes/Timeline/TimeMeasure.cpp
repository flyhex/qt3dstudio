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

#include "TimeMeasure.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"
#include "TimelineTimelineLayout.h"
#include "Snapper.h"
#include "Qt3DSDMSignals.h"

const long AUTO_TICK_AMNT = 60;
using namespace Q3DStudio;

//=============================================================================
/**
 * Create a new time measure.
 * @param inLayout the layout this is representing, used for modifying time.
 * @param inTimeRatio the current time ratio.
 * @param inIsTransparent true if the background of this control should not be drawn.
 */
CTimeMeasure::CTimeMeasure(CTimelineTimelineLayout *inLayout, double inTimeRatio,
                           bool inFillBackground /*= true */)
    : CBaseMeasure(inTimeRatio, inFillBackground)
    , m_ScrollDir(0)
    , m_TimePerPixel(0)
    , m_IsMouseDown(false)
    , m_TimelineLayout(inLayout)
{
    SetTimeRatio(inTimeRatio);
    SetName("TimeMeasure");

    m_EdgeMargin = 2;
    // the large tickmark is shorter than the medium to leave room for the text
    m_LargeHashOffset = 5;
}

CTimeMeasure::~CTimeMeasure()
{
}

//=============================================================================
/**
 * Set the amount of time that is represented for each pixel.
 * @param inTimePerPixel the amount of time represented for each pixel.
 */
void CTimeMeasure::SetTimeRatio(double inTimeRatio)
{
    m_Ratio = inTimeRatio;

    double theTimePerPixel = (double)(1 / inTimeRatio);

    // Only go through this if it has actually changed
    if (theTimePerPixel != m_TimePerPixel) {
        m_TimePerPixel = theTimePerPixel;

        // Go through the possible hash settings and find the one that best suits the
        // time per pixel.
        double theMillisPerLargeHash = theTimePerPixel * 50;
        if (theMillisPerLargeHash <= 100) // 100ms
            theMillisPerLargeHash = 100;
        else if (theMillisPerLargeHash <= 200) // 200ms
            theMillisPerLargeHash = 200;
        else if (theMillisPerLargeHash <= 500) // .5s
            theMillisPerLargeHash = 500;
        else if (theMillisPerLargeHash <= 1000) // 1s
            theMillisPerLargeHash = 1000;
        else if (theMillisPerLargeHash <= 2000) // 2s
            theMillisPerLargeHash = 2000;
        else if (theMillisPerLargeHash <= 5000) // 5s
            theMillisPerLargeHash = 5000;
        else if (theMillisPerLargeHash <= 10000) // 10s
            theMillisPerLargeHash = 10000;
        else if (theMillisPerLargeHash <= 20000) // 20s
            theMillisPerLargeHash = 20000;
        else if (theMillisPerLargeHash <= 30000) // 30s
            theMillisPerLargeHash = 30000;
        else if (theMillisPerLargeHash <= 60000) // 1m
            theMillisPerLargeHash = 60000;
        else if (theMillisPerLargeHash <= 120000) // 2m
            theMillisPerLargeHash = 120000;
        else if (theMillisPerLargeHash <= 300000) // 5m
            theMillisPerLargeHash = 300000;
        else if (theMillisPerLargeHash <= 600000) // 10m
            theMillisPerLargeHash = 600000;
        else if (theMillisPerLargeHash <= 1200000) // 20m
            theMillisPerLargeHash = 1200000;
        else if (theMillisPerLargeHash <= 1800000) // 30m
            theMillisPerLargeHash = 1800000;
        else if (theMillisPerLargeHash <= 3600000) // 1h
            theMillisPerLargeHash = 3600000;
        else
            theMillisPerLargeHash = 7200000; // 2h

        // Set the distances between the hashes
        m_LargeHashInterval = theMillisPerLargeHash;
        m_MediumHashInterval = theMillisPerLargeHash / 2;
        m_SmallHashInterval = theMillisPerLargeHash / 10;

        // update to StudioPreferences so that the ',' '.' and '<' '>' keys would respond
        // accordingly
        CStudioPreferences::SetTimeAdvanceAmount(static_cast<long>(m_SmallHashInterval));
        CStudioPreferences::SetBigTimeAdvanceAmount(static_cast<long>(m_MediumHashInterval));

        Invalidate();
    }
}
//=============================================================================
/**
 * Get the time formatted as a string.
 * This will figure out the best way to display the time and return it as a
 * string.
 * @param inTime the time to display in milliseconds.
 * @return the time formatted in a string.
 */
Q3DStudio::CString CTimeMeasure::FormatTime(long inTime)
{
    long theHours = inTime / 3600000;
    long theMinutes = inTime % 3600000 / 60000;
    long theSeconds = inTime % 60000 / 1000;
    long theMillis = inTime % 1000;

    bool theHoursOnlyFlag = theHours != 0 && theMinutes == 0 && theSeconds == 0 && theMillis == 0;
    bool theMinutesOnlyFlag =
        !theHoursOnlyFlag && theMinutes != 0 && theSeconds == 0 && theMillis == 0;
    bool theSecondsOnlyFlag = !theMinutesOnlyFlag && theMillis == 0;

    Q3DStudio::CString theTime;
    // If only hours are being displayed then format it as hours.
    if (theHoursOnlyFlag) {
        theTime.Format(_UIC("%dh"), theHours);
    }
    // If only minutes are being displayed then format it as minutes.
    else if (theMinutesOnlyFlag) {
        theTime.Format(_UIC("%dm"), theMinutes);
    }
    // If only seconds are being displayed then format as seconds
    else if (theSecondsOnlyFlag) {
        theTime.Format(_UIC("%ds"), theSeconds);
    }
    // If the intervals are correct then this should only be tenths of seconds, so do that.
    else {
        theTime.Format(_UIC("0.%ds"), theMillis / 100);
    }

    return theTime;
}

//=============================================================================
/**
 * Set the amount of time that this time measure is offset by.
 * @param inTimeOffset the offset time in milliseconds.
 */
void CTimeMeasure::SetTimeOffset(long inTimeOffset)
{
    if (inTimeOffset != m_Offset) {
        m_Offset = inTimeOffset;

        Invalidate();
    }
}

//=============================================================================
/**
 * Notification that the left mouse button was clicked.
 * This tells the timeline to move the playhead to the current loc.
 * @param inPoint the location where the mouse was clicked.
 * @param inFlags the state of the mouse.
 */
bool CTimeMeasure::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_IsMouseDown = true;

        m_TimelineLayout->OnTimeMeasureMouseDown(inPoint, inFlags);
    }

    return true;
}

//=============================================================================
/**
 * Notification that the mouse is moving over this control.
 * If the mouse was clicked on this control this will drag the playhead.
 * @param inPoint the location where the mouse was clicked.
 * @param inFlags the state of the mouse.
 */
void CTimeMeasure::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();

    // subtract out the button width since the playhead is never allowed into that area on the right
    // side
    // of the timeline and use it for the initial autoscrolling place
    if (inPoint.x > 0 && inPoint.x <= GetSize().x - CStudioPreferences::GetDefaultButtonWidth()) {
        CControl::OnMouseMove(inPoint, inFlags);
        if (m_IsMouseDown)
            m_TimelineLayout->OnTimeMeasureMouseDown(inPoint, inFlags);
        m_ScrollDir = 0;
    } else if (m_IsMouseDown) {
        if (inPoint.x < 0)
            m_ScrollDir = -1;
        else if (inPoint.x > GetSize().x - CStudioPreferences::GetDefaultButtonWidth())
            m_ScrollDir = 1;
        m_TimerConnection = ITickTock::GetInstance().AddTimer(
            150, true, std::bind(&CTimeMeasure::OnTimer, this), "CTimeMeasure::OnMouseMove");
        OnTimer();
    }
}

//=============================================================================
/**
 * Call back for the timer that was set in on mouse move
 */
void CTimeMeasure::OnTimer()
{
    CPt theOffset;
    if (m_ScrollDir > 0)
        theOffset.x =
            GetSize().x - 2 * CStudioPreferences::GetDefaultButtonWidth() + AUTO_TICK_AMNT;
    else if (m_ScrollDir < 0)
        theOffset.x = -AUTO_TICK_AMNT;
    m_TimelineLayout->OnTimeMeasureMouseDown(theOffset, 0);
    ;
}

//=============================================================================
/**
 * Notification that the mouse was unclicked.
 * This stops dragging of the playhead if it was dragging it.
 * @param inPoint the location where the mouse was unclicked.
 * @param inFlags the state of the mouse.
 */
void CTimeMeasure::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();
    m_IsMouseDown = false;
}

//=============================================================================
/**
 * Notification that the mouse was unclicked.
 * This stops dragging of the playhead if it was dragging it.
 * @param inPoint the location where the mouse was unclicked.
 * @param inFlags the state of the mouse.
 */
void CTimeMeasure::OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();
    m_IsMouseDown = false;
}

//=============================================================================
/**
 * Add the tick marks to the snapping list.
 * This uses the user preference for the tick marks and adds them.
 */
void CTimeMeasure::PopulateSnappingList(CSnapper *inSnapper)
{
    // Only if this is supposed to snap to time markers.
    if (CStudioPreferences::IsTimelineSnappingGridActive()) {
        // Check the resolution to snap to
        ESnapGridResolution theResolution = CStudioPreferences::GetTimelineSnappingGridResolution();
        double thePeriodicInterval;
        if (theResolution == SNAPGRID_TICKMARKS) {
            thePeriodicInterval = m_SmallHashInterval;
        } else if (theResolution == SNAPGRID_HALFSECONDS) {
            thePeriodicInterval = m_MediumHashInterval;
        } else {
            thePeriodicInterval = m_LargeHashInterval;
        }

        // Set a periodic interval for snapping
        inSnapper->SetPeriodicInterval(::dtol(thePeriodicInterval));
    }
}

void CTimeMeasure::OnLoseFocus()
{
    m_TimerConnection = std::shared_ptr<qt3dsdm::ISignalConnection>();
    m_IsMouseDown = false;
}

//=============================================================================
/**
 * Draw the time at the specified position.
 * @param inRenderer the renderer to draw to.
 * @param inPosition the position to draw the time to, the time will be centered here.
 * @param inTime the time to draw.
 */
void CTimeMeasure::DrawMeasureText(CRenderer *inRenderer, long inPosition, long inMeasure)
{
    Q3DStudio::CString theTimeFormat(FormatTime(inMeasure));
    // Offset the position by half the text size to center it over the hash.
    const auto textSize = inRenderer->GetTextSize(theTimeFormat.toQString());
    inPosition -= ::dtol(textSize.width() / 2);

    inRenderer->DrawText((float)inPosition, -3, theTimeFormat.toQString(),
                         QRect(0, 0, GetSize().x, GetSize().y),
                         CStudioPreferences::GetRulerTickColor().getQColor());
}

//=============================================================================
/**
 * Calculate the position of a time value on the time measure
 */
long CTimeMeasure::CalculatePos(double inNewValue)
{
    return ::TimeToPos(inNewValue, m_Ratio);
}
