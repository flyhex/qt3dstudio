/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "VClockPolicy.h"

template <typename TTimePolicy = CVClockPolicy>
class CVClock
{
public:
    CVClock();
    virtual ~CVClock();

    // Function for Setting the Policy
    void SetUpperBound(long inUpperBound) { m_TimePolicy.SetUpperBound(inUpperBound); }
    long GetUpperBound() { return m_TimePolicy.GetUpperBound(); }
    void SetLowerBound(long inLowerBound) { m_TimePolicy.SetLowerBound(inLowerBound); }
    long GetLowerBound() { return m_TimePolicy.GetLowerBound(); }
    void SetClockType(long inClockType) { m_TimePolicy.SetClockType(inClockType); }
    long GetClockType() { return m_TimePolicy.GetClockType(); }
    void SetLooping(bool inLooping) { m_TimePolicy.SetLooping(inLooping); }
    bool GetLooping() { return m_TimePolicy.GetLooping(); }

    long UpdateClock(long inAbsoluteTime);
    long GetTime();
    void SetTime(long inTime, long inAbsoluteTime);

protected:
    bool AdjustTime(long &inVirtualTime, long inAdjustedTime, bool &outPonged);

private:
    TTimePolicy m_TimePolicy;

    long m_VirtualTime;
    long m_Marker; // The difference between the absolute time and the virtual time
};

//==============================================================================
/**
 *	Constructor.
 *
 *	Set the Field Values to a reasonable value.
 *
 */
template <typename TTimePolicy>
CVClock<TTimePolicy>::CVClock()
    : m_VirtualTime(0)
    , m_Marker(0)
{
}

//==============================================================================
/**
 *	Destructor.
 */
template <typename TTimePolicy>
CVClock<TTimePolicy>::~CVClock()
{
}

//==============================================================================
/**
 *	This will calculate the Virtual time based on the Parents virtual time.
 *
 *	This function will also adjust the time based on clock Rate.
 *	and wether or not the Object is paused or not.
 *	Also the Clock Policy will adjust the time based on the Bounds.
 *	And context type.
 *
 *	@param	inAbsoluteTime The parents Virtual time.
 *	@param	outPonged only valid for roundtrip clock policy (refer to
 *			CVClockPolicy::HandleRoundTrip), always false for normal clock policy
 *
 *	@return	The Adjusted Time.
 */
template <typename TTimePolicy>
bool CVClock<TTimePolicy>::AdjustTime(long &outVirtualTime, long inAdjustedTime, bool &outPonged)
{
    bool theLooped = false;

    // return of true implies that we looped and we need to reset out clock.
    theLooped = m_TimePolicy.AdjustTime(outVirtualTime, outPonged);

    if (theLooped) {
        m_Marker = inAdjustedTime - outVirtualTime;
    }

    return theLooped;
}

//==============================================================================
/**
 *	This will calculate the Virtual time based on the Parents virtual time.
 *	@param	inAbsoluteTime the Virtual time of the parent.
 *	@return
 */
template <typename TTimePolicy>
long CVClock<TTimePolicy>::UpdateClock(long inAbsoluteTime)
{
    // We have to calculate a new Virtual Time.
    // Send the Virtual time off to the TimePolicy so it can process it.
    m_VirtualTime = inAbsoluteTime - m_Marker;

    if (m_VirtualTime < 0) {
        m_VirtualTime = 0;
        m_Marker = inAbsoluteTime;
    }

    // If AdjustTime returns true if we looped and we need to reset out clock.
    bool thePonged;
    AdjustTime(m_VirtualTime, inAbsoluteTime, thePonged);

    return m_VirtualTime;
}

//==============================================================================
/**
 *	Gets the Adjusted time of the Clock.
 *
 *	@return	the Virtual time.
 */
template <typename TTimePolicy>
long CVClock<TTimePolicy>::GetTime()
{
    return m_VirtualTime;
}

template <typename TTimePolicy>
void CVClock<TTimePolicy>::SetTime(long inTime, long inAbsoluteTime)
{
    m_VirtualTime = inTime;
    m_Marker = inAbsoluteTime - m_VirtualTime;
}