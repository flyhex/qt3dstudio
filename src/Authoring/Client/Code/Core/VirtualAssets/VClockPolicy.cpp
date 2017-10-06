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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"
//==============================================================================
//	Includes
//==============================================================================
#include "VClockPolicy.h"
#include "ClientErrorIDs.h"

UIC_DEFINE_THISFILE;

CVClockPolicy::CVClockPolicy()
{
    m_UpperBound = LONG_MAX;
    m_LowerBound = 0;
    m_Looping = false;
    m_Type = POLICY_NORMAL;
}

CVClockPolicy::CVClockPolicy(const CVClockPolicy &inSource)
{
    m_UpperBound = inSource.m_UpperBound;
    m_LowerBound = inSource.m_LowerBound;
    m_Looping = inSource.m_Looping;
    m_Type = inSource.m_Type;
}

long CVClockPolicy::ReverseTime(long inVirtualTime)
{
    return m_UpperBound - inVirtualTime;
}
//==============================================================================
/**
 *	This will handle the Time calculation for a forward moving Timer.
 *
 *	The incomming time will be adjusted based on the Range. Also the Time will
 *	get the Last value if we are not "Looping" otherwise loop to the start time.
 *
 *	@param	inVirtualTime the time from the Clock. it is a time relative to the last
 *"Pause/Reset"
 *
 *	@return	the Adjusted Time.
 */
long CVClockPolicy::HandleNormal(long inVirtualTime, long inRange, bool &outLooped)
{
    long theLeftValue = 0;

    outLooped = false;

    if (inVirtualTime <= m_UpperBound) {
        if (inVirtualTime >= m_LowerBound)
            theLeftValue = inVirtualTime;
        else
            theLeftValue = m_LowerBound;
    } else {
        if (m_Looping) {
            long theAdjustedTime = inVirtualTime % m_UpperBound;
            theLeftValue = ((theAdjustedTime) % inRange) + m_LowerBound;
            outLooped = true;
        } else {
            theLeftValue = m_UpperBound;
        }
    }

    return theLeftValue;
}

//==============================================================================
/**
*	This will determine the next clock value based on a policy that runs up and back.
*
*	@param	inVirtualTime the time from the Clock.
*	@param	inRange the time from the Clock.
*	@param	outPonged set to true when adjusted time reaches m_UpperBound .
*			( i.e when clock reaches the 'pong' end )
*
*	@return	the Adjusted time.
*/
long CVClockPolicy::HandleRoundTrip(long inVirtualTime, long inRange, bool &outLooped,
                                    bool &outPonged)
{
    Q_UNUSED(inRange);
    long theLeftValue = 0;

    outLooped = false;
    outPonged = false;

    // This is the up direction.
    if (inVirtualTime <= m_UpperBound) {
        theLeftValue = inVirtualTime;
        outPonged = (theLeftValue == m_UpperBound);
    }
    // This is the Downward direction.
    else {
        // TODO : This codes doesn't seem correct. It should have uses the modulus similar to
        // HandleNormal
        // See the newly added failing (but commented out test case at VClock.cxx in RoundTripTest
        theLeftValue = 2 * m_UpperBound - inVirtualTime;
        if (theLeftValue < m_LowerBound) {
            if (m_Looping) {
                theLeftValue = 2 * m_LowerBound - theLeftValue;
                outLooped = true;
            } else {
                theLeftValue = m_LowerBound;
            }
        }
    }
    return theLeftValue;
}

bool CVClockPolicy::AdjustTime(long &ioVirtualTime, bool &outPonged)
{
    long theLeftValue = 0;
    long theRange = m_UpperBound - m_LowerBound;
    bool theLooped = false;

    outPonged = false; // initialize this
    if (theRange > 0) {
        switch (m_Type) {
        case POLICY_NORMAL:
            // Note: The Range should be one more than the Diff, since we are inclusive.
            theLeftValue = this->HandleNormal(ioVirtualTime, theRange, theLooped);
            outPonged = false;
            break;
        case POLICY_ROUNDTRIP:
            theLeftValue = this->HandleRoundTrip(ioVirtualTime, theRange, theLooped, outPonged);
            break;
        default:
            break;
        }
    } else {
        theLeftValue = m_LowerBound;
        theRange = 100;
    }

    ioVirtualTime = theLeftValue;

    return theLooped;
}
