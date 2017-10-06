/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "TimelineKeyframe.h"
#include "Renderer.h"

CTimelineKeyframe::CTimelineKeyframe()
    : m_Time(0)
    , m_IsDynamic(false)
{
}

CTimelineKeyframe::~CTimelineKeyframe()
{
}

void CTimelineKeyframe::SetTime(long inTime)
{
    m_Time = inTime;
}

long CTimelineKeyframe::GetTime()
{
    return m_Time;
}

//=============================================================================
/**
 * called when this key is made dynamic
 * @param inIsDynamic true if the keyframe is dynamic
 */
void CTimelineKeyframe::SetDynamic(bool inIsDynamic)
{
    if (m_IsDynamic != inIsDynamic) {
        m_IsDynamic = inIsDynamic;
        // Invalidate( );
    }
}

//=============================================================================
/**
 * @return  true if dynamic
 */
bool CTimelineKeyframe::IsDynamic()
{
    return m_IsDynamic;
}
