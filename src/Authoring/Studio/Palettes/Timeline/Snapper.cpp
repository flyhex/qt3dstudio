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

#include "Snapper.h"
#include "StudioPreferences.h"
#include "Control.h"
#include "CoreUtils.h"
#include "StudioUtils.h"
#include "HotKeys.h"

//=============================================================================
/**
 * Create a new snapper object.
 * @param inTimeRatio the default time ratio.
 */
CSnapper::CSnapper(double inTimeRatio)
    : m_theStartTime(0)
    , m_theEndTime(0)
    , m_TimeRatio(inTimeRatio)
    , m_IsSnappingKeyframes(false)
    , m_IsSnappingSelectedKeyframes(true)
    , m_Offset(0)
    , m_StartHeight(0)
    , m_EndHeight(0)
    , m_PeriodicInterval(LONG_MAX)
    , m_InitialOffset(0)
    , m_ObjectTimeOffset(0)
    , m_TimeOffset(0)
    , m_KeyFrameClicked(false)
    , m_Source(nullptr)
{
    SetSnappingDistance(CStudioPreferences::GetSnapRange());
}

CSnapper::~CSnapper()
{
}
//=============================================================================
/**
 * Set if keyframe is clicked
 *  @param inKeyFrameClicked toggles snapping to end handles at 1 pixel range,
 *   if true.
 */
void CSnapper::SetKeyFrameClicked(bool inKeyFrameClicked)
{
    m_KeyFrameClicked = inKeyFrameClicked;
}
//=============================================================================
/**
 * Clear all the snapping points from this.
 * This effectively erases this object.
 */
void CSnapper::Clear()
{
    m_PeriodicInterval = LONG_MAX;
    m_Times.clear();
    m_KeyFrameClicked = false;
    SetSnappingDistance(CStudioPreferences::GetSnapRange());
}

//=============================================================================
/**
 * Add a snapping point at the specified time.
 * @param inTime the time to add the point at.
 */
void CSnapper::AddTime(long inTime)
{
    m_Times.insert(inTime);
}

//=============================================================================
/**
 * Add a snapping point at the specified pixel location.
 * @param inPosition the position of the snapping point to add.
 */
void CSnapper::AddPixelLocation(long inPosition)
{
    AddTime(::PosToTime(inPosition, m_TimeRatio));
}

//=============================================================================
/**
 * Set whether or not keyframes should be added as snapping points.
 * @param true if keyframes should be added as snapping points.
 */
void CSnapper::SetSnappingKeyframes(bool inIsSnappingKeyframes)
{
    m_IsSnappingKeyframes = inIsSnappingKeyframes;
}

//=============================================================================
/**
 * Checks whether or not keyframes are being snapped to.
 * @return true if keyframes should be snapped to.
 */
bool CSnapper::IsSnappingKeyframes()
{
    return m_IsSnappingKeyframes;
}

//=============================================================================
/**
 * Sets whether or not selected keyframes are being snapped to.
 * This is used to ignore all selected keyframes when keyframes are being
 * dragged.
 * @param inIsSnappingSelectedKeyframes true if selected keys should be snapped.
 */
void CSnapper::SetSnappingSelectedKeyframes(bool inIsSnappingSelectedKeyframes)
{
    m_IsSnappingSelectedKeyframes = inIsSnappingSelectedKeyframes;
}

//=============================================================================
/**
 * Checks whether or not selected keyframes are being snapped to.
 * This is used to ignore all selected keyframes when keyframes are being
 * dragged.
 * @return true if selected keyframes should be snapping points.
 */
bool CSnapper::IsSnappingSelectedKeyframes()
{
    return m_IsSnappingKeyframes && m_IsSnappingSelectedKeyframes;
}

//=============================================================================
/**
 * Set the current time ratio.
 * This will effect all time based positions as well as the intervals. It is
 * suggested that this is called on an empty snapper when possible for speed.
 * @param inTimeRatio the new time ratio.
 */
void CSnapper::SetTimeRatio(double inTimeRatio)
{
    m_SnapDistance = ::dtol(m_SnapDistance * m_TimeRatio / inTimeRatio);
    m_TimeRatio = inTimeRatio;
}

//=============================================================================
/**
 * Set the visible area of this snapper.
 * This is used to limit snapping to visible areas only. The visibility limit
 * is only on vertical because the user can scroll horizontally and expose
 * previously non visible objects. The heights are relative to the initial
 * offset and
 * @param inStartHeight the minimum height for visibility.
 * @param inEndHeight the maximum height for visibility.
 */
void CSnapper::SetVisibleArea(long inStartHeight, long inEndHeight)
{
    m_StartHeight = inStartHeight;
    m_EndHeight = inEndHeight;
}

//=============================================================================
/**
 * Push an offset into this for calculating later visibilities.
 * This is accumulated with previous offsets.
 * @param inHeight the amount to modify the offset by.
 */
void CSnapper::PushOffset(long inHeight)
{
    m_Offsets.push_back(inHeight);
    m_Offset += inHeight;
}

//=============================================================================
/**
 * Remove an offset that was pushed on.
 * This will update the current offset to be what it was before the push.
 */
void CSnapper::PopOffset()
{
    m_Offset -= m_Offsets.back();
    m_Offsets.pop_back();
}

//=============================================================================
/**
 * Checks to see if an object at inPosition of height inHeight is visible.
 * This uses the current offset with the visible area to check visibility.
 * @param inPosition the position of the object to be checked.
 * @param inHeight the height of the object to be checked.
 */
bool CSnapper::IsVisible(long inPosition, long inHeight)
{
    return (inPosition + m_Offset < m_EndHeight
            && inPosition + m_Offset + inHeight > m_StartHeight);
}

//=============================================================================
/**
 * Add a periodic interval to the snapping points.
 * This will make snapping points at every multiple of inInterval. The interval
 * starts at time 0.
 * @param inInterval time in millis for the periodic points.
 */
void CSnapper::SetPeriodicInterval(long inInterval)
{
    m_PeriodicInterval = inInterval;
}

//=============================================================================
/**
 * Interpret the given position into a snapped/nonsnapped position.
 * This will use the inFlags to determine whether or not this should be snapping
 * and uses inPosition to figure out the closest snapping position. If the
 * closest position is not within the tolerances then inPosition will be
 * returned.
 * @param inPosition to position to check for snapping.
 * @param inFlags the mouse state flags, to determine whether or not snapping.
 */
bool CSnapper::InterpretTimeEx(long &ioTime, long inFlags)
{
    // Only snap if shift key is down.
    if (inFlags & CHotKeys::MODIFIER_SHIFT)
        return GetSnapTime(ioTime, true);
    else
        return GetSnapTime(ioTime, false);
}

//=============================================================================
/**
 * Interpret the given position into a snapped/nonsnapped position.
 * This will use the inFlags to determine whether or not this should be snapping
 * and uses inPosition to figure out the closest snapping position. If the
 * closest position is not within the tolerances then inPosition will be
 * returned.
 * @param inPosition to position to check for snapping.
 * @param inFlags the mouse state flags, to determine whether or not snapping.
 */
long CSnapper::InterpretTime(long inTime, long inFlags)
{
    if (inFlags & CHotKeys::MODIFIER_SHIFT) {
        GetSnapTime(inTime, true);
        return inTime;
    }
    GetSnapTime(inTime, false);
    return inTime;
}

//=============================================================================
/**
 * Set the maximum distance that snapping will occur at.
 * This sets the maximum tolerances for a position to be away from a snapping
 * point and still get snapped to it.
 * @param inSnapDistance the snap distance, in pixels.
 */
void CSnapper::SetSnappingDistance(long inSnapDistance)
{
    m_SnapDistance = ::dtol(inSnapDistance / m_TimeRatio);
}

//=============================================================================
/**
 * Helper method to find the closer of two values to a third.
 * If both values are the same distance then the first value will be returned.
 * @param inFirst the first value to check the distance to inBase.
 * @param inSecond the second value to check the distance to inBase.
 * @param inBase the value the others are being compared to.
 * @return the value, either first or second, that is closest to inBase.
 */
long GetClosestValue(long inFirst, long inSecond, long inBase)
{
    return (::labs(inFirst - inBase) <= ::labs(inSecond - inBase)) ? inFirst : inSecond;
}

//=============================================================================
/**
 * Given the current time, it is adjusted if necessary to snap.
 * @param ioTime the current time on input; on output the adjusted time
 * @param inShiftKeyDown true if the shift key was down, otherwise false
 * @return true if a snap occurred, otherwise false
 */
bool CSnapper::GetSnapTime(long &ioTime, bool inShiftKeyDown)
{
    bool theReturnValue = false;

    if (inShiftKeyDown) // If user hits the shift key (i.e. snapping is toggled on)
    {
        long thePreviousTime = 0;
        long theNextTime = 0;

        // Go through all the snapping positions finding the positions on either
        // side of ioPosition. Bsically just loop through until a snap position
        // is larger than in position and use that with the previous value to get
        // the closest snapping location.
        TTimeList::iterator thePos = m_Times.begin();
        for (; thePos != m_Times.end(); ++thePos) {
            thePreviousTime = theNextTime;
            theNextTime = (*thePos);

            // Don't need to go any further because we've hit the first point larget than
            // ioPosition.
            if (theNextTime >= ioTime) {
                break;
            }
        }

        // Use the last snap position less than ioPosition and the first snap position greater than
        // ioPosition
        // to find the closest of the two.
        long theClosestTime = GetClosestValue(thePreviousTime, theNextTime, ioTime);
        long theClosestInterval = GetClosestPeriodicInterval(ioTime);

        // Get the closest snapping position between the periodic interval and the position
        theClosestTime = GetClosestValue(theClosestTime, theClosestInterval, ioTime);

        // If the closest position is within tolerances then use it, otherwise return the original
        // value.
        if (::labs(theClosestTime - ioTime) <= m_SnapDistance) {
            ioTime = theClosestTime;
            theReturnValue = true;
        }
    } else // If user does not hit the shift key (i.e. snapping is toggled off)
    {
        // Snap to end handles at 1 pixel range if the current object dragged
        // is a keyframe
        if (m_KeyFrameClicked) {
            // Returns if the startTime or the endTime of a Time Bar is closer to the dragged
            // keyframe
            long theClosestTime = GetClosestValue(m_theStartTime, m_theEndTime, ioTime);

            // Set snapping range to 1 pixel and converts it to time.
            // The snapping range of 1 pixel applies for keyframes that are dragged really
            // close to the end handles.
            long thePixel = 1;
            long theTime = ::dtol(thePixel / m_TimeRatio);

            // Determines if the closest time is within 1 pixel range
            // If so returns the closest time, which is the snapping time, and true indicating
            // that snapping occurs.
            if (::labs(theClosestTime - ioTime) <= theTime) {
                ioTime = theClosestTime;
                theReturnValue = true;
            }
        }
    }

    return theReturnValue;
}
//=============================================================================
/**
 * Get the closest periodic interval to inPosition.
 * Since it is too expensive to store every possible periodic interval in the
 * snapping list, this just dynamically figures out the closest periodic
 * interval.
 */
void CSnapper::SetStartEndTime(long theStartTime, long theEndTime)
{
    m_theStartTime = theStartTime;
    m_theEndTime = theEndTime;
}
//=============================================================================
/**
 * Get the closest periodic interval to inPosition.
 * Since it is too expensive to store every possible periodic interval in the
 * snapping list, this just dynamically figures out the closest periodic
 * interval.
 */
long CSnapper::GetClosestPeriodicInterval(long inTime)
{
    long theIntervalLow = inTime / m_PeriodicInterval * m_PeriodicInterval;
    long theIntervalHigh = (inTime / m_PeriodicInterval + 1) * m_PeriodicInterval;

    return GetClosestValue(theIntervalLow, theIntervalHigh, inTime);
}

//=============================================================================
/**
 * Used to pass off snapping logic to this for objects being dragged.
 * This does all the work of an object when the object itself is being dragged
 * and it's position is being modified by the drag, but it needs to be snapped
 * as well. It is not necessary to call this unless ProcessDrag is being
 * used.
 * @param inClickPosition the mouse click location, usually just inPoint.x.
 * @param inCenterOffset the center of the object where it should be snapped to.
 */
void CSnapper::BeginDrag(long inClickLocation, long inCenterTimeOffset)
{
    m_InitialOffset = inClickLocation;
    m_ObjectTimeOffset = inCenterTimeOffset;
}

//=============================================================================
/**
 * Process an object's drag event and figure out where the object should be.
 * This does all the work of figuring out where the object should be snapped
 * to. It is not necessary to call this, but it may make implementing snapping
 * much easier. BeginDrag needs to be called on OnMouseDown to use this.
 * @param inPosition the position of the mouse, GetPosition( ).x + inPoint.x.
 * @param inFlags the mouse state flags, used to determine snapping state.
 */
long CSnapper::ProcessDrag(long inTime, long inOffset, long inFlags)
{
    long theModPos = inTime + m_TimeOffset + ::dtol((inOffset - m_InitialOffset) / m_TimeRatio);
    InterpretTimeEx(theModPos, inFlags);
    theModPos -= m_TimeOffset;

    return theModPos;
}

//=============================================================================
/**
 * Set the source for this Snapper object.
 * This is so that objects may choose to no add themselves to snapping lists
 * that they originated. This is only meant for comparison with the 'this' ptr.
 * @param inSource the source object for this snapping list.
 */
void CSnapper::SetSource(void *inSource)
{
    m_Source = inSource;
}

//=============================================================================
/**
 * Get the source for this snapper object.
 * @return the source of the snapping.
 */
void *CSnapper::GetSource()
{
    return m_Source;
}

void CSnapper::SetTimeOffset(long inTimeOffset)
{
    m_TimeOffset = inTimeOffset;
}
