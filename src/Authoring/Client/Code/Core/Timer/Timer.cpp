/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#include "Timer.h"

CTimer::CTimer()
    : m_CurrentTime(0)
    , m_Offset(0)
    , m_Pause(true)
{
}

CTimer::~CTimer()
{
}

//==============================================================================
/**
 *		Changes the timer state from Pause to "Run" or "Playing"
 *
 *		This recalculates the time offset from the last pause.
 *		Turns off manual mode.
 */
void CTimer::Play()
{
    m_Pause = false;
    m_Timer.Start();
}

//==============================================================================
/**
 *		Resets the timer offset
 *
 *		Turns off manual mode
 */
void CTimer::Reset()
{
    m_CurrentTime = 0;
    m_Offset = 0;

    m_Timer.Reset();
    m_Timer.Start();
}

//==============================================================================
/**
 *		Pauses the timer
 */
void CTimer::Pause()
{
    if (!m_Pause) {
        // Grab the currentTime before we leave this function, only for the first time
        m_CurrentTime = GetTime();
        m_Pause = true;
    }
}

//==============================================================================
/**
 *		Unpauses the timer
 */
void CTimer::Resume()
{
    if (m_Pause) {
        m_Offset = m_CurrentTime - m_Timer.ElapsedMilliSecs();
        m_Pause = false;
    }
}

//==============================================================================
/**
 *		Returns the current timer time in milliseconds
 */
long CTimer::GetTime()
{
    if (m_Pause)
        return m_CurrentTime;
    else
        return m_Timer.ElapsedMilliSecs() + m_Offset;
}

//==============================================================================
/**
 *		Returns the current timer time in microseconds
 */
TLarge CTimer::GetHiResTime()
{
    if (m_Pause)
        return m_CurrentTime * 1000;
    else
        return m_Timer.ElapsedMicroSecs() + (m_Offset * 1000);
}

//==============================================================================
/**
 *		Stops the hi-res timer
 */
void CTimer::Stop()
{
    m_Timer.Stop();
}
