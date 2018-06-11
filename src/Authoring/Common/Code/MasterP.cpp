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
#include "Qt3DSCommonPrecompile.h"

#ifdef PERFORM_PROFILE

#include "MasterP.h"

using namespace Q3DStudio;

//==============================================================================
/**
 * Constructor, this shouldn't be necessary since this has a static GetInstance.
 * Use GetInstance instead to use all the same one.
 */
CMasterProf::CMasterProf()
{
}

CMasterProf::~CMasterProf()
{
}

//==============================================================================
/**
 * Get the One profiler.
 * This is the instance that all method profilers register themselves on.
 */
CMasterProf *CMasterProf::GetInstance()
{
    static CMasterProf theProf;
    return &theProf;
}

//==============================================================================
/**
 * Add a meth prof to the list of available profilers.
 * This should be auto-called by the CMethProf class.
 */
void CMasterProf::AddMethProf(CMethProf *inMethProf)
{
    m_Profs.push_back(inMethProf);
}

//==============================================================================
/**
 * Reset all the statistics.
 * This will clear all call counts and call times for all the method profilers
 * in the system.
 */
void CMasterProf::Clear()
{
    // delete all the meth profs in here.
    std::vector<CMethProf *>::iterator thePos = m_Profs.begin();
    for (; thePos != m_Profs.end(); ++thePos) {
        (*thePos)->Clear();
    }
}

//==============================================================================
/**
 * Get the number of profilers in the system, used for iterating through the profilers.
 */
long CMasterProf::GetProfilerCount()
{
    return static_cast<long>(m_Profs.size());
}

//==============================================================================
/**
 * Get a profiler at inIndex.
 * @param inIndex must be less than GetProfilerCount.
 */
CMethProf *CMasterProf::GetProfiler(long inIndex)
{
    return m_Profs.at(inIndex);
}

#endif // #ifdef PERFORM_PROFILE
