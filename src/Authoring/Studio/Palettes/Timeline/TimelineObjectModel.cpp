/****************************************************************************
**
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
#include "TimelineObjectModel.h"

#include "ColorControl.h"
#include "IKeyframe.h"
#include "SlideRow.h"
#include "StateRow.h"
#include "PropertyRow.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineItem.h"
#include "Bindings/ITimelineTimebar.h"

#include "StudioUtils.h"

#include <QDebug>

TimelineObjectModel::~TimelineObjectModel()
{
   m_rows.clear();
   m_timelineItemBinding = nullptr;
}

QHash<int, QByteArray> TimelineObjectModel::roleNames() const
{
    auto names = ObjectListModel::roleNames();
    names.insert(SelectedRole, "selected");
    names.insert(SelectedColorRole, "selectedColor");
    names.insert(ItemColorRole, "itemColor");
    names.insert(TimeInfoRole, "timeInfo");
    names.insert(KeyframesRole, "keyframes");

    return names;
}

QVariant TimelineObjectModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    auto timelineRow = const_cast<TimelineObjectModel*>(this)->timelineRowForIndex(index);
    auto timelineItemBinding = timelineRow->GetTimelineItemBinding();

    switch (role) {
    case TimelineRowRole: {
        return QVariant::fromValue(timelineRow.data());
    }
    case SelectedRole: {
        return timelineRow->IsSelected();
    }
    case ItemColorRole: {
        auto timebar = timelineItemBinding->GetTimelineItem()->GetTimebar();

        auto color = timebar->GetTimebarColor().getQColor();
        return color.name();
    }
    case SelectedColorRole: {
        auto timebar = timelineItemBinding->GetTimelineItem()->GetTimebar();

        auto color = CColorControl::CalculateSelectedColor(timebar->GetTimebarColor()).getQColor();
        return color.name();
    }
    case TimeInfoRole: {
        auto timebar = timelineItemBinding->GetTimelineItem()->GetTimebar();

        const auto startTime = timebar->GetStartTime();
        const auto endTime = timebar->GetEndTime();
        const auto timeRatio = timelineRow->GetTimeRatio();

        TimebarTimeInfo timeInfo;
        timeInfo.m_lifeStart = ::TimeToPos(startTime, timeRatio);
        timeInfo.m_lifeEnd = ::TimeToPos(endTime, timeRatio);
        timeInfo.m_startPosition = ::TimeToPos(timelineRow->GetActiveStart(), timeRatio);
        timeInfo.m_endPosition = ::TimeToPos(timelineRow->GetActiveEnd(), timeRatio);

        return QVariant::fromValue(timeInfo);
    }
    case KeyframesRole: {
        QVariantList keyframes;

        const auto timeRatio = timelineRow->GetTimeRatio();

        long keyframeCount = timelineItemBinding->GetKeyframeCount();
        for (long i = 0; i < keyframeCount; ++i) {
            auto key = timelineItemBinding->GetKeyframeByIndex(i);

            KeyframeInfo keyInfo;
            keyInfo.m_time = key->GetTime();
            keyInfo.m_selected = key->IsSelected();
            keyInfo.m_dynamic = key->IsDynamic();
            keyInfo.m_position = ::TimeToPos(keyInfo.m_time, timeRatio);

            keyframes.append(QVariant::fromValue(keyInfo));
        }

        return keyframes;
    }

    default: ;
    }

    return ObjectListModel::data(index, role);
}

void TimelineObjectModel::setTimelineItemBinding(ITimelineItemBinding *inTimelineItem)
{
    Q_ASSERT(inTimelineItem);
    m_timelineItemBinding = inTimelineItem;
}

QSharedPointer<CTimelineRow> TimelineObjectModel::timelineRowForIndex(const QModelIndex &index)
{
    if (!index.parent().isValid()) {
        if (!m_slideRow) {
            m_slideRow.reset(new CSlideRow(nullptr));
            Q_ASSERT(m_timelineItemBinding);
            m_slideRow->Initialize(m_timelineItemBinding);
            m_rows[m_baseHandle.GetHandleValue()] = m_slideRow;
        }
        return m_slideRow;
    } else {
        const auto handle = handleForIndex(index).GetHandleValue();

        if (!m_rows.contains(handle)) {
            auto parentRow = dynamic_cast<CBaseStateRow*>(timelineRowForIndex(index.parent()).data());
            Q_ASSERT(parentRow);
            // TODO:
            // the second "true" argument assumes it is loaded, avoids calling LoadChildren
            // this is temporary here, as long as the old timeline code is still used
            auto stateRow = new CStateRow(parentRow, true);
            auto parentBinding = parentRow->GetTimelineItemBinding();
            auto itemBinding = parentBinding->GetChild(index.row());
            stateRow->Initialize(itemBinding);
            parentRow->AddStateRow(stateRow, nullptr);
            itemBinding->SetParent(parentBinding); // KDAB_TODO do we really need it?
            m_rows[handle].reset(stateRow);

            connect(stateRow, &CTimelineRow::selectedChanged, this, [this, index] {
                emit dataChanged(index, index, {SelectedRole});
            });

            connect(stateRow, &CTimelineRow::timeRatioChanged, this, [this, index] {
                emit dataChanged(index, index, {TimeInfoRole});
            });

            connect(stateRow, &CTimelineRow::dirtyChanged, this, [this, index] {
                emit dataChanged(index, index, {});
            });
        }

        return m_rows[handle];
    }

    Q_ASSERT(0);
    return nullptr;
}
