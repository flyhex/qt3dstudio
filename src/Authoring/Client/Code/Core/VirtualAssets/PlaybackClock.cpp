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

#include "PlaybackClock.h"
#include "Doc.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMDataCore.h"

//==============================================================================
//	Static Member Initialization
//==============================================================================

using namespace qt3dsdm;
using namespace std;

//====================================================================
/**
 *
 */
CPlaybackClock::CPlaybackClock(CDoc *inDoc)
    : m_Doc(inDoc)
    , m_LastAbsoluteTime(0)
{
}

CPlaybackClock::~CPlaybackClock()
{
}

// Called when the document is setting the time.
// We need to ensure that
void CPlaybackClock::OnTimeChange(long inNewTime)
{
    m_LastAbsoluteTime = m_ContextTimer.GetTime();
    m_VirtualTime = inNewTime;
}

//====================================================================
/**
 * The core of the updating cycle, this will do all the attachments,
 * detachments and events necessary for this time.
 */
void CPlaybackClock::UpdateTime()
{
    long lastAbsTime = m_LastAbsoluteTime;
    m_LastAbsoluteTime = m_ContextTimer.GetTime();
    m_VirtualTime += m_LastAbsoluteTime - lastAbsTime;

    long theCurrentTime = 0;
    switch (m_Policy) {
    case PlaybackClockPolicy::Normal:
        theCurrentTime = m_VirtualTime;
        if (m_Looping) {
            theCurrentTime = theCurrentTime % m_UpperBound;
            m_VirtualTime = theCurrentTime;
        }
        break;
    case PlaybackClockPolicy::RoundTrip: {
        long numCycles = m_VirtualTime / m_UpperBound;
        long cycleTime = m_VirtualTime % m_UpperBound;
        if (m_Looping == false && numCycles > 1) {
            theCurrentTime = 0;
        } else {
            if (numCycles % 2)
                theCurrentTime = m_UpperBound - cycleTime;
            else
                theCurrentTime = cycleTime;
        }
    } break;
    }

    // Code to play through to another slides
    if (theCurrentTime >= m_UpperBound)
        OnReachedUpperBound();
    else
        m_Doc->DoNotifyTimeChanged(theCurrentTime);
}

void CPlaybackClock::OnReachedUpperBound()
{
    qt3dsdm::Qt3DSDMSlideHandle theActiveSlide(m_Doc->GetActiveSlide());
    // clock has passed the end, check whether needs to switch slide
    qt3dsdm::Qt3DSDMInstanceHandle theInstanceHandle =
        GetSlideSystem()->GetSlideInstance(theActiveSlide);

    // Get the play through state
    qt3dsdm::SValue theValue;
    GetPropertySystem()->GetInstancePropertyValue(
        theInstanceHandle, GetClientDataModelBridge()->GetSlide().m_PlayMode, theValue);
    Q3DStudio::CString thePlayMode = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue)->GetData();

    if (thePlayMode == L"Play Through To...") {
        // figure out which slide to activate
        GetPropertySystem()->GetInstancePropertyValue(
            theInstanceHandle, GetClientDataModelBridge()->GetSlide().m_PlaythroughTo, theValue);
        SStringOrInt thePlaythroughTo = qt3dsdm::get<SStringOrInt>(theValue);

        qt3dsdm::ISlideSystem *theSlideSystem = GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle thePlaythroughToSlide;

        if (thePlaythroughTo.GetType() == SStringOrIntTypes::String) {
            Q3DStudio::CString theValue(get<TDataStrPtr>(thePlaythroughTo.m_Value)->GetData());
            if (theValue == L"Next") {
                size_t theNextSlideIndex = theSlideSystem->GetSlideIndex(theActiveSlide) + 1;
                qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
                    theSlideSystem->GetMasterSlide(theActiveSlide);
                size_t theSlideCount = theSlideSystem->GetSlideCount(theMasterSlide);
                if (theNextSlideIndex < theSlideCount) {
                    // Activate the next slide
                    thePlaythroughToSlide =
                        theSlideSystem->GetSlideByIndex(theMasterSlide, theNextSlideIndex);
                }
            } else if (theValue == L"Previous") {
                int thePrevSlideIndex = theSlideSystem->GetSlideIndex(theActiveSlide) - 1;
                qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
                    theSlideSystem->GetMasterSlide(theActiveSlide);
                if (thePrevSlideIndex > 0) // Index 0 refers to MasterSlide, so we check > 0
                {
                    // Activate the prev slide
                    thePlaythroughToSlide =
                        theSlideSystem->GetSlideByIndex(theMasterSlide, thePrevSlideIndex);
                }
            }
        } else {
            // Extract the slide handle from the string
            thePlaythroughToSlide = get<long>(thePlaythroughTo.m_Value);
        }

        if (thePlaythroughToSlide.Valid()) {
            m_Doc->NotifyActiveSlideChanged(thePlaythroughToSlide, true, true);
            UpdateClockProperties();
            StartPlayback();
        }
    } else if (thePlayMode == L"Stop at end") {
        if (!m_Doc->isPlayBackPreviewOn())
            m_Doc->SetPlayMode(PLAYMODE_STOP);
        m_Doc->DoNotifyTimeChanged(m_UpperBound);
    }
}

//====================================================================
/**
 * Called when user clicks Play
 */
void CPlaybackClock::StartPlayback()
{
    UpdateClockProperties();
    m_ContextTimer.Resume();
}

//====================================================================
/**
 * Called when user clicks Stop
 */
void CPlaybackClock::StopPlayback()
{
    m_ContextTimer.Pause();
}

//====================================================================
/**
 *	This method will reset all local time calculations.
 *	So the next time read will be min time.
 */
void CPlaybackClock::Reset()
{
    m_ContextTimer.Reset();
    m_ContextTimer.Pause();

    m_LastAbsoluteTime = m_ContextTimer.GetTime();
    m_VirtualTime = m_Doc->GetCurrentClientTime();
}

//====================================================================
/**
 *	Sets the Clock properties based on the active slide
 */
void CPlaybackClock::UpdateClockProperties()
{
    qt3dsdm::Qt3DSDMInstanceHandle theInstanceHandle =
        GetSlideSystem()->GetSlideInstance(m_Doc->GetActiveSlide());
    ASSERT(theInstanceHandle.Valid());

    qt3dsdm::SValue theValue;
    GetPropertySystem()->GetInstancePropertyValue(
        theInstanceHandle, GetClientDataModelBridge()->GetSlide().m_InitialPlayState, theValue);
    Q3DStudio::CString theInitialPlayState = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue)->GetData();

    GetPropertySystem()->GetInstancePropertyValue(
        theInstanceHandle, GetClientDataModelBridge()->GetSlide().m_PlayMode, theValue);
    Q3DStudio::CString thePlayMode = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue)->GetData();

    m_Policy = PlaybackClockPolicy::Normal;
    m_Looping = false;

    if (thePlayMode == L"Looping") // Loop
    {
        m_Looping = true;
    } else if (thePlayMode == L"PingPong") // Ping Pong.
    {
        m_Policy = PlaybackClockPolicy::RoundTrip;
        m_Looping = true;
    } else if (thePlayMode == L"Ping") // Ping Pong Once.
    {
        m_Policy = PlaybackClockPolicy::RoundTrip;
        m_Looping = false;
    }

    m_UpperBound = m_Doc->GetLatestEndTime();
    m_VirtualTime = m_Doc->GetCurrentClientTime();
}

qt3dsdm::IPropertySystem *CPlaybackClock::GetPropertySystem() const
{
    return m_Doc->GetStudioSystem()->GetPropertySystem();
}
qt3dsdm::ISlideSystem *CPlaybackClock::GetSlideSystem() const
{
    return m_Doc->GetStudioSystem()->GetSlideSystem();
}
CClientDataModelBridge *CPlaybackClock::GetClientDataModelBridge() const
{
    return m_Doc->GetStudioSystem()->GetClientDataModelBridge();
}
