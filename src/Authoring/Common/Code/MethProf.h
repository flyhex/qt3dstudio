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

#ifndef INCLUDED_METH_PROF_H
#define INCLUDED_METH_PROF_H 1

#pragma once

#ifdef PERFORM_PROFILE

#include "HiResTimer.h"
#include "UICString.h"
#include <vector>

class CMethProf
{
public:
    CMethProf(const Q3DStudio::CString &inName = "");
    virtual ~CMethProf();

    void Clear();

    void Start();
    void Stop();

    unsigned long GetCount();
    unsigned long GetTotalMillis();
    float GetAverageMillis();
    float GetMaxMillis();
    Q3DStudio::CString GetDescription();
    Q3DStudio::CString GetName();

protected:
    TLarge m_TotalTime;
    TLarge m_MaxTime;
    CHiResTimer m_Timer;

    unsigned long m_CallCount;
    long m_StackCount;

    Q3DStudio::CString m_Name;
};

#endif // #ifdef PERFORM_PROFILE

#endif // INCLUDED_METH_PROF_H
