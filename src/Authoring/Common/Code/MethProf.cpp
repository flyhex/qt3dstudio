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
#include "stdafx.h"

#ifdef PERFORM_PROFILE

#include "MethProf.h"
#include "MasterP.h"
#include "Qt3DSString.h"

using namespace Q3DStudio;

//==============================================================================
/**
 * Create a profiler with inName as the identifying name.
 * This will auto register itself on CMasterProf.
 */
CMethProf::CMethProf(const Q3DStudio::CString &inName)
{
    m_Name = inName;

    this->Clear();

    CMasterProf::GetInstance()->AddMethProf(this);
}

CMethProf::~CMethProf()
{
}

//==============================================================================
/**
 * Clear the statistics for this profiler.
 * This will cause all the timings to be reset.
 */
void CMethProf::Clear()
{
    m_TotalTime = 0;
    m_MaxTime = 0;

    m_CallCount = 0;
    m_StackCount = 0;
}

//==============================================================================
/**
 * Start the profiler running.
 * This will increment the call count and start the timer running.
 */
void CMethProf::Start()
{
    if (m_StackCount == 0) {
        m_Timer.Start();
    }
    ++m_StackCount;
}

//==============================================================================
/**
 * Stop the profiler from running.
 * This will log the amount of time taken from when Start was first called.
 */
void CMethProf::Stop()
{
    --m_StackCount;
    ++m_CallCount;

    if (m_StackCount == 0) {
        m_Timer.Stop();
        TLarge theElapsed = m_Timer.ElapsedMicroSecs();

        if (theElapsed > m_MaxTime)
            m_MaxTime = theElapsed;

        m_TotalTime += theElapsed;
    }
}

//==============================================================================
/**
 * Get the number of times that this profiler was started.
 */
unsigned long CMethProf::GetCount()
{
    return m_CallCount;
}

//==============================================================================
/**
 * Get the total amount of time in milliseconds that this profiler was running.
 */
unsigned long CMethProf::GetTotalMillis()
{
    return (unsigned long)(m_TotalTime / 1000);
}

//==============================================================================
/**
 * Get the average time in milliseconds that this profiler was running.
 */
float CMethProf::GetAverageMillis()
{
    if (m_CallCount > 0) {
        float theTotal = (float)GetTotalMillis();
        return theTotal / m_CallCount;
    }
    return 0;
}

//==============================================================================
/**
 * Get the maximum time in milliseconds that this profiler was running.
 */
float CMethProf::GetMaxMillis()
{
    return ((float)m_MaxTime) / 1000.0f;
}

//==============================================================================
/**
 * Get a formatted string description of this profiler.
 */
Q3DStudio::CString CMethProf::GetDescription()
{
    Q3DStudio::CString theString;
    theString.Format(_UIC("%ls      cc: %d      tt: %d      mt: %8.2f      at: %8.2f"),
                     (const wchar_t *)m_Name, m_CallCount, GetTotalMillis(), GetMaxMillis(),
                     GetAverageMillis());
    return theString;
}

//==============================================================================
/**
 * Get the name of this profiler.
 */
Q3DStudio::CString CMethProf::GetName()
{
    return m_Name;
}

#endif // #ifdef PERFORM_PROFILE
