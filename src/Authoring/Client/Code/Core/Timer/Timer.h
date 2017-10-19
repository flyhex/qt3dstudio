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
#ifndef __TIMER_H_
#define __TIMER_H_

//==============================================================================
//	Includes
//==============================================================================

//==============================================================================
//	Forwards
//==============================================================================
#include "Qt3DSTime.h"
#include "HiResTimer.h"

//==============================================================================
//	Constants
//==============================================================================

//==============================================================================
//	Class
//==============================================================================

//==============================================================================
/**
 *	@class	CTimer
 *	@brief	This timer class controls the elapsed time in a presentation.
 */
class CTimer
{
    //==============================================================================
    //	Enumerations
    //==============================================================================

    //==============================================================================
    //	Fields
    //==============================================================================

protected:
    long m_CurrentTime; ///< The current time
    long m_Offset;
    bool m_Pause;
    CHiResTimer m_Timer;

    //==============================================================================
    //	Methods
    //==============================================================================

    // Construction
public:
    CTimer();
    virtual ~CTimer();

public:
    long GetTime();
    void Play();
    void Reset();
    void Pause();
    void Resume();
    void Stop();

    TLarge GetHiResTime();
};

#endif //__TIMER_H_
