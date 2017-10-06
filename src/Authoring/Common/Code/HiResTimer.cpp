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
#include "stdafx.h"

#include "HiResTimer.h"

//=============================================================================
/**
 * Construct a new HiRes Timer, the timer starts out stopped.
 */
CHiResTimer::CHiResTimer()
    : m_IsRunning(false)
{
}

CHiResTimer::~CHiResTimer()
{
}

//=============================================================================
/**
 * Start the timer running, this will store the amount of time until Stop is called.
 */
void CHiResTimer::Start()
{
    if (!m_IsRunning) {
        m_StartTime.start();
        m_IsRunning = true;
    }
}

//=============================================================================
/**
 * Stop the timer from running, this will store the elapsed time from when
 * Start was called for later access.
 */
void CHiResTimer::Stop()
{
    if (m_IsRunning) {
        m_elapsed = m_StartTime.nsecsElapsed();
        m_IsRunning = false;
    }
}

//=============================================================================
/**
 * Get the number of usecs elapsed since start was called.
 * If the timer is running then this will return the time from when start was
 * called. If the timer is stopped then this will return the amount of time
 * between the start and last stop.
 */
TLarge CHiResTimer::ElapsedMicroSecs()
{
    if (m_IsRunning) {
        return m_StartTime.nsecsElapsed() / 1000;
    }
    return m_elapsed / 1000;
}

//=============================================================================
/**
 * Get the number of msecs elapsed since start was called.
 * If the timer is running then this will return the time from when start was
 * called. If the timer is stopped then this will return the amount of time
 * between the start and last stop.
 */
long CHiResTimer::ElapsedMilliSecs()
{
    if (m_IsRunning) {
        return m_StartTime.nsecsElapsed() / 1000000;
    }
    return m_elapsed / 1000000;
}

void CHiResTimer::Reset()
{
    m_StartTime.restart();
}
