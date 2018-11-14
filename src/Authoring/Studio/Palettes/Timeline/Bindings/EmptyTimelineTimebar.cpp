/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#include "EmptyTimelineTimebar.h"
#include "StudioPreferences.h"

CEmptyTimelineTimebar::CEmptyTimelineTimebar()
{
}

CEmptyTimelineTimebar::~CEmptyTimelineTimebar()
{
}

long CEmptyTimelineTimebar::GetStartTime() const
{
    return 0;
}

long CEmptyTimelineTimebar::GetEndTime() const
{
    return 0;
}

long CEmptyTimelineTimebar::GetDuration() const
{
    return 0;
}

bool CEmptyTimelineTimebar::ShowHandleBars() const
{ // makes no sense to show handle bars, when this does not have start/end times.
    return false;
}

void CEmptyTimelineTimebar::OnBeginDrag()
{
}

void CEmptyTimelineTimebar::OffsetTime(long inDiff)
{
    Q_UNUSED(inDiff);
}

void CEmptyTimelineTimebar::ChangeTime(long inTime, bool inSetStart)
{
    Q_UNUSED(inTime);
    Q_UNUSED(inSetStart);
}

void CEmptyTimelineTimebar::CommitTimeChange()
{
}

void CEmptyTimelineTimebar::RollbackTimeChange()
{
}

QColor CEmptyTimelineTimebar::GetTimebarColor()
{
    return CStudioPreferences::GetObjectTimebarColor();
}

Q3DStudio::CString CEmptyTimelineTimebar::GetTimebarComment()
{
    return "";
}

void CEmptyTimelineTimebar::SetTimebarComment(const Q3DStudio::CString &inComment)
{
    Q_UNUSED(inComment);
}

void CEmptyTimelineTimebar::SetTimebarTime(ITimeChangeCallback *inCallback /*= nullptr*/)
{
    Q_UNUSED(inCallback);
}
