/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "TimelineControl.h"
#include "TimelineGraphicsScene.h"
#include "RowManager.h"
#include "RowTree.h"
#include "Bindings/ITimelineItemBinding.h"
#include "StudioApp.h"
#include "Dialogs.h"

TimelineControl::TimelineControl(TimelineGraphicsScene *scene)
    : m_scene(scene)
{
}

void TimelineControl::setRowTimeline(RowTimeline *rowTimeline)
{
    m_rowTimeline = rowTimeline;
    m_timebar = m_rowTimeline->rowTree()->getBinding()->GetTimelineItem()->GetTimebar();
    m_startTime = m_rowTimeline->getStartTime();
    m_endTime = m_rowTimeline->getEndTime();
    m_rowTimeline->updateBoundChildren(true);
    m_rowTimeline->updateBoundChildren(false);
}

void TimelineControl::showDurationEditDialog()
{
    g_StudioApp.GetDialogs()->asyncDisplayDurationEditDialog(m_startTime, m_endTime, this);
}

void TimelineControl::ChangeStartTime(long inTime)
{
    m_rowTimeline->setStartTime(inTime);
}

void TimelineControl::ChangeEndTime(long inTime)
{
    m_rowTimeline->setEndTime(inTime);
    m_scene->rowManager()->updateRulerDuration();
}

void TimelineControl::Commit()
{
    m_timebar->ChangeTime(m_rowTimeline->getStartTime(), true);
    m_timebar->ChangeTime(m_rowTimeline->getEndTime(), false);
    m_timebar->CommitTimeChange();
}

void TimelineControl::Rollback()
{
    m_rowTimeline->setStartTime(m_startTime);
    m_rowTimeline->setEndTime(m_endTime);
    m_scene->rowManager()->updateRulerDuration();
}
