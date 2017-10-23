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
#ifndef INCLUDED_MASTER_PROF_H
#define INCLUDED_MASTER_PROF_H 1

#ifdef PERFORM_PROFILE

#include "MethProf.h"

/**
To use it just add to the start of the section you are looking to profile:
        QT3DS_PROFILE( MethodName );

QT3DS_PROFILE is defined in MasterP.h

The profiler uses a stack variable to start and stop, so it will automatically stop when it gets
destructed.
To profile sub sections of a method you can do:
{
        QT3DS_PROFILE( MethodName_Subsection );
        CodeToBeProfiled( );
}

The naming convention is to use MethodName when profiling a method and MethodName_Subsection when
doing a certain
area of a method. This way it keeps clear the location of the profiling code.

Note- the profiling does not give very good statistics on recursive functions.

In playback enable the Profiling window, then use F7 to display the profiling statistics and F8 to
reset them. F8
is useful if you only want timing of playback with no loading, or if you only want statistics of
certain sections.
To see the profiling window make sure you have the debug menu turned on in your preferences
(shift-prefs from the
web player).

The symbols in the lines printed out have the following info:
Cc- Call Count: the number of times the profiling section was hit.
Tt- Total Time: the total amount of time spent in the profiling section.
Mt- Max Time: the maximum time spent in a single pass of the profiling section.
At- Avg Time: the average time spent in a single section.

Average time is really good for profiling specific methods and seeing what differences code changes
make.

Total Time is good for seeing how much time a specific method takes up of larger loops, such as
taking the total
time of player::update and comparing it to the total time of GLRenderUICMB::Render to see the
percentage of the
entire update cycle that rendering models is taking up.

Max Time is good for tracking down periodic hitches in the system where something may have a decent
average time,
but periodically take a long time.

Call Count is useful for checking that things get called the proper number of times, such as for
loading a
presentation a method should only be invoked once, but it may be called multiple times.
*/

class CMasterProf
{
public:
    CMasterProf();
    virtual ~CMasterProf();

    static CMasterProf *GetInstance();

    void AddMethProf(CMethProf *inMethProf);

    void Clear();

    long GetProfilerCount();
    CMethProf *GetProfiler(long inIndex);

protected:
    std::vector<CMethProf *> m_Profs;
};

class CProfiler
{
public:
    CProfiler(CMethProf *inMethProf)
    {
        m_MethProf = inMethProf;
        m_MethProf->Start();
    }

    virtual ~CProfiler() { m_MethProf->Stop(); }

protected:
    CMethProf *m_MethProf;
};

#endif // #ifdef PERFORM_PROFILE

// Super ninja code:
//
// Make a static variable within the function using inName to name it. Pass a reference to the
// static variable to the stack based start/stop CProfiler. The CProfiler will then use the
// static variable for keeping the statistics.
//
// Every time the section is entered the same static variable is used but a new Profiler is created.
// It's important to have the unique naming as well, this will allow multiple profilers per
// function.

#ifdef PERFORM_PROFILE
#define QT3DS_PROFILE(inName)                                                                         \
    static CMethProf the##inName##StaticProf(Q3DStudio::CString(__FUNCTION__)                      \
                                             + Q3DStudio::CString("::")                            \
                                             + Q3DStudio::CString(#inName));                       \
    CProfiler the##inName##Prof(&the##inName##StaticProf)

#else
#define QT3DS_PROFILE(inName)
#endif

#endif // INCLUDED_MASTER_PROF_H
