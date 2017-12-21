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

#include "ClientDataModelBridge.h"
#include "ColorControl.h"
#include "Core.h"
#include "Doc.h"
#include "IKeyframe.h"
#include "SlideRow.h"
#include "StateRow.h"
#include "PropertyRow.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineItem.h"
#include "Bindings/ITimelineTimebar.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMStudioSystem.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"

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

    if (!timelineRow) {
        return {};
    }

    auto propertyRow = dynamic_cast<CPropertyRow *>(timelineRow);
    if (propertyRow) {
        return dataForProperty(propertyRow, index, role);
    }

    auto timelineItemBinding = timelineRow->GetTimelineItemBinding();
    switch (role) {
    case TimelineRowRole: {
        return QVariant::fromValue(timelineRow);
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
            appendKey(keyframes, key, timeRatio);
        }


        return keyframes;
    }

    default: ;
    }

    return ObjectListModel::data(index, role);
}

QVariant TimelineObjectModel::dataForProperty(CPropertyRow *propertyRow, const QModelIndex &index, int role) const
{
    auto timelineItemProperty = propertyRow->GetProperty();
    if (!timelineItemProperty)
        return {};

    switch (role) {
    case TimelineRowRole: {
        return QVariant::fromValue(propertyRow);
    }
    case SelectedRole: {
        return propertyRow->IsSelected();
    }
    case ItemColorRole: {
        return propertyRow->GetTimebarBackgroundColor().getQColor().name();
    }
    case SelectedColorRole: {
        return propertyRow->GetTimebarHighlightBackgroundColor().getQColor().name();
    }
    case TimeInfoRole: {
        return data(index.parent(), role);
    }
    case KeyframesRole: {
        QVariantList keyframes;
        const auto timeRatio = propertyRow->GetTimeRatio();

        auto timelineItemProperty = propertyRow->GetProperty();
        long keyframeCount = timelineItemProperty->GetKeyframeCount();
        for (long i = 0; i < keyframeCount; ++i) {
            auto key = timelineItemProperty->GetKeyframeByIndex(i);
            appendKey(keyframes, key, timeRatio);
        }

        return keyframes;
    }

    // roles from ObjectListModel
    case NameRole: {
        return timelineItemProperty->GetName().toQString();
    }

    case IconRole: {
        return resourceImageUrl() + "Objects-Property-Normal.png";
    }
    case TextColorRole: {
        auto textColor = CStudioPreferences::GetNormalColor();
        if (timelineItemProperty->IsMaster())
            textColor = CStudioPreferences::GetMasterColor();

        return textColor.getQColor().name();
    }

    case PathReferenceRole:
    case AbsolutePathRole:
        return {};

    default: ;
    }

    return ObjectListModel::data(index, role);
}

QModelIndex TimelineObjectModel::parent(const QModelIndex &index) const
{
    const auto handle = handleForIndex(index);
    auto it = m_properties.constBegin();
    while (it != m_properties.constEnd()) {
        if (it->contains(handle)) {
            return indexForHandle(it.key());
        }
        ++it;
    }

    return ObjectListModel::parent(index);
}

void TimelineObjectModel::setTimelineItemBinding(ITimelineItemBinding *inTimelineItem)
{
    Q_ASSERT(inTimelineItem);
    m_timelineItemBinding = inTimelineItem;
}

CTimelineRow* TimelineObjectModel::timelineRowForIndex(const QModelIndex &index)
{
    if (!index.parent().isValid()) {
        if (!m_slideRow) {
            m_slideRow.reset(new CSlideRow(nullptr));
            Q_ASSERT(m_timelineItemBinding);
            m_slideRow->Initialize(m_timelineItemBinding);
            m_rows[m_baseHandle.GetHandleValue()] = m_slideRow.data();
        }
        return m_slideRow.data();
    } else {
        const auto handle = handleForIndex(index).GetHandleValue();

        if (!m_rows.contains(handle)) {
            int propertyCount = m_properties.value(handleForIndex(index.parent()), {}).count();

            auto parentRow = dynamic_cast<CBaseStateRow*>(timelineRowForIndex(index.parent()));
            Q_ASSERT(parentRow);
            auto parentBinding = parentRow->GetTimelineItemBinding();

            if (propertyCount > 0 && index.row() < propertyCount &&
                dynamic_cast<Qt3DSDMTimelineItemBinding *>(parentBinding)) {
                auto binding = static_cast<Qt3DSDMTimelineItemBinding *>(parentBinding);

                auto propertyRow = binding->AddPropertyRow(handle);

                connect(propertyRow, &CTimelineRow::timeRatioChanged, this, [this, index] {
                    emit dataChanged(index, index, {TimeInfoRole, KeyframesRole});
                });

                m_rows[handle] = propertyRow;

                return propertyRow;
            } else {
                // TODO:
                // the second "true" argument assumes it is loaded, avoids calling LoadChildren
                // this is temporary here, as long as the old timeline code is still used
                auto stateRow = new CStateRow(parentRow, true);
                auto itemBinding = parentBinding->GetChild(index.row() - propertyCount);
                itemBinding->setCreateUIRow(false);
                stateRow->Initialize(itemBinding);
                itemBinding->SetParent(parentBinding); // KDAB_TODO do we really need it?
                m_rows[handle] = stateRow;

                connect(stateRow, &CTimelineRow::selectedChanged, this, [this, index] {
                    emit dataChanged(index, index, {SelectedRole});
                });

                connect(stateRow, &CTimelineRow::timeRatioChanged, this, [this, index] {
                    emit dataChanged(index, index, {TimeInfoRole, KeyframesRole});
                });

                connect(stateRow, &CTimelineRow::dirtyChanged, this, [this, index] {
                    emit dataChanged(index, index, {});
                });

                return stateRow;
            }
        }

        return m_rows[handle];
    }

    Q_ASSERT(0);
    return nullptr;
}

void TimelineObjectModel::addProperty(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                      qt3dsdm::Qt3DSDMPropertyHandle property)
{
    auto parentIndex = indexForHandle(parentInstance);
    auto propertyCount = m_properties.value(parentInstance.GetHandleValue(), {}).count();

    // Now we rely on rowCount calling childrenList, thus getting the new rows automatically
    beginInsertRows(parentIndex, propertyCount, propertyCount);
    endInsertRows();
}

void TimelineObjectModel::removeProperty(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                         qt3dsdm::Qt3DSDMPropertyHandle property)
{
    auto parentIndex = indexForHandle(parentInstance);
    auto properties = m_properties.value(parentInstance.GetHandleValue(), {});
    auto propertyIndex = properties.indexOf(property);

    // Now we rely on rowCount calling childrenList, thus getting the new rows automatically
    beginRemoveRows(parentIndex, propertyIndex, propertyIndex);
    endRemoveRows();
}

qt3dsdm::TInstanceHandleList TimelineObjectModel::childrenList(const qt3dsdm::Qt3DSDMSlideHandle &slideHandle, const qt3dsdm::Qt3DSDMInstanceHandle &handle) const
{
     auto studioSystem = m_core->GetDoc()->GetStudioSystem();
     auto propertySystem = studioSystem->GetPropertySystem();
     qt3dsdm::SValue typeValue;
     propertySystem->GetInstancePropertyValue(handle, studioSystem->GetClientDataModelBridge()
                                              ->GetTypeProperty(), typeValue);
     qt3dsdm::DataModelDataType::Value valueType(qt3dsdm::GetValueType(typeValue));
     if (valueType == qt3dsdm::DataModelDataType::None)
         return {};

    auto children = ObjectListModel::childrenList(slideHandle, handle);

    qt3dsdm::TPropertyHandleList properties;
    propertySystem->GetAggregateInstanceProperties(handle, properties);
    auto it = children.begin();
    QVector<qt3dsdm::Qt3DSDMInstanceHandle> animatedProperties;
    for (const auto &propertyHandle: properties) {
        if (studioSystem->GetAnimationSystem()->IsPropertyAnimated(
                    handle, propertyHandle)) {
            animatedProperties.append(propertyHandle);
            it = children.insert(it, propertyHandle);
        }
    }
    m_properties[handle] = animatedProperties;

    return children;
}

void TimelineObjectModel::appendKey(QVariantList &keyframes, IKeyframe *key, double timeRatio) const
{
    KeyframeInfo keyInfo;
    keyInfo.m_time = key->GetTime();
    keyInfo.m_selected = key->IsSelected();
    keyInfo.m_dynamic = key->IsDynamic();
    keyInfo.m_position = ::TimeToPos(keyInfo.m_time, timeRatio);

    keyframes.append(QVariant::fromValue(keyInfo));
}
