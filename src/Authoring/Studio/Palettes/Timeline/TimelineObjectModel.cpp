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
    names.insert(TimelineRowRole, "timelineRow");
    names.insert(SelectedRole, "selected");
    names.insert(SelectedColorRole, "selectedColor");
    names.insert(ItemColorRole, "itemColor");
    names.insert(TimeInfoRole, "timeInfo");
    names.insert(KeyframesRole, "keyframes");
    names.insert(ShyRowRole, "shy");
    names.insert(VisibleRowRole, "rowVisible");
    names.insert(LockedRowRole, "locked");
    names.insert(IsPropertyRole, "isProperty");
    names.insert(PropertyExpandedRole, "propertyExpanded");
    names.insert(VisibleRole, "visible");
    names.insert(HasActionRole, "hasAction");
    names.insert(HasMasterActionRole, "hasMasterAction");
    names.insert(HasChildActionRole, "hasChildAction");
    names.insert(HasMasterChildActionRole, "hasMasterChildAction");
    names.insert(HasComponentActionRole, "hasComponentAction");
    names.insert(HasMasterComponentActionRole, "hasMasterComponentAction");

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
    auto timelineItem = timelineItemBinding->GetTimelineItem();

    switch (role) {
    case TimelineRowRole: {
        return QVariant::fromValue(timelineRow);
    }
    case SelectedRole: {
        return timelineRow->IsSelected();
    }
    case ItemColorRole: {
        auto timebar = timelineItem->GetTimebar();

        auto color = timebar->GetTimebarColor().getQColor();
        return color.name();
    }
    case SelectedColorRole: {
        auto timebar = timelineItem->GetTimebar();

        auto color = CColorControl::CalculateSelectedColor(timebar->GetTimebarColor()).getQColor();
        return color.name();
    }
    case TimeInfoRole: {
        auto timebar = timelineItem->GetTimebar();

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

    case ShyRowRole: {
        return timelineItem->IsShy();
    }

    case VisibleRowRole: {
        return timelineItem->IsVisible();
    }

    case LockedRowRole: {
        return timelineItem->IsLocked();
    }

    case VisibleRole: {
        return isVisible(index);
    }

    case IsPropertyRole: {
        return false;
    }
    case PropertyExpandedRole: {
        return false;
    }
    case HasActionRole: {
        return timelineItem->HasAction(false);
    }

    case HasMasterActionRole:{
        return timelineItem->HasAction(true);
    }

    case HasChildActionRole: {
        return timelineItem->ChildrenHasAction(false);
    }

    case HasMasterChildActionRole: {
        return timelineItem->ChildrenHasAction(true);
    }

    case HasComponentActionRole:  {
        return timelineItem->ComponentHasAction(false);
    }

    case HasMasterComponentActionRole:  {
        return timelineItem->ComponentHasAction(true);
    }

    default: ;
    }

    return ObjectListModel::data(index, role);
}

bool TimelineObjectModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return false;

    auto timelineRow = const_cast<TimelineObjectModel*>(this)->timelineRowForIndex(index);

    if (!timelineRow)
        return false;

    auto propertyRow = dynamic_cast<CPropertyRow *>(timelineRow);
    if (propertyRow) {
        switch (role) {
        case PropertyExpandedRole:
            propertyRow->setExpanded(value.toBool());
            emit dataChanged(index, index, {role});
            return true;
        }
        return false;
    }

    auto timelineItemBinding = timelineRow->GetTimelineItemBinding();
    auto timelineItem = timelineItemBinding->GetTimelineItem();

    QVariant result;
    switch (role) {
    case ShyRowRole:
        timelineItem->SetShy(value.toBool());
        break;
    case VisibleRowRole:
        timelineItem->SetVisible(value.toBool());
        break;
    case LockedRowRole:
        timelineItem->SetLocked(value.toBool());
        break;
    }
    if (!result.isValid())
        return ObjectListModel::setData(index, value, role);

    emit dataChanged(index, index, {role});

    return true;
}

QVariant TimelineObjectModel::dataForProperty(CPropertyRow *propertyRow, const QModelIndex &index, int role) const
{
    auto timelineItemProperty = propertyRow->GetProperty();
    if (!timelineItemProperty)
        return {};

    switch (role) {
    case PropertyExpandedRole: {
        return propertyRow->expanded();
    }
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


    case IsPropertyRole: {
        return true;
    }

    case VisibleRole: {
        return isVisible(index);
    }

    case HasActionRole:
    case HasMasterActionRole:
    case HasChildActionRole:
    case HasMasterChildActionRole:
    case HasComponentActionRole:
    case HasMasterComponentActionRole:
        return false;

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

                CPropertyRow *propertyRow = nullptr;

                // check if the property row was already created
                // KDAB_TODO can we remove this once we get rid of the legacy timeline view?
                if (auto propertyBinding = binding->GetPropertyBinding(handle))
                    propertyRow = propertyBinding->GetRow();

                if (propertyRow == nullptr)
                    propertyRow = binding->AddPropertyRow(handle);

                Q_ASSERT(propertyRow);

                connect(propertyRow, &CTimelineRow::timeRatioChanged, this, [this, index] {
                    emit dataChanged(index, index, {TimeInfoRole, KeyframesRole});
                });

                connect(propertyRow, &CTimelineRow::dirtyChanged, this, [this, index] {
                    emit dataChanged(index, index, {});
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
                    // update item and its children
                    auto nextIndex = this->index(index.row() + 1, 0, index.parent());
                    if (!nextIndex.isValid())
                        nextIndex = index;
                    emit dataChanged(index, nextIndex, {});
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

    // addProperty and removeProperty are called multiple times for properties with multiple
    // components, e.g. Position, so check if the property was already inserted in m_properties
    if (m_properties.value(parentInstance.GetHandleValue(), {}).contains(property))
        return;

    auto children = childrenList(m_slideHandle, parentInstance);
    auto it = std::find(children.begin(), children.end(), property);
    Q_ASSERT(it != children.end());
    int propertyIndex = std::distance(children.begin(), it);

    // Now we rely on rowCount calling childrenList, thus getting the new rows automatically
    beginInsertRows(parentIndex, propertyIndex, propertyIndex);
    endInsertRows();
}

void TimelineObjectModel::removeProperty(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                         qt3dsdm::Qt3DSDMPropertyHandle property)
{
    auto parentIndex = indexForHandle(parentInstance);

    auto parentHandle = parentInstance.GetHandleValue();
    if (!m_properties.contains(parentHandle))
            return;

    auto& properties = m_properties[parentHandle];

    auto propertyIndex = properties.indexOf(property);
    if (propertyIndex == -1)
        return;

    properties.remove(propertyIndex);

    // Now we rely on rowCount calling childrenList, thus getting the new rows automatically
    beginRemoveRows(parentIndex, propertyIndex, propertyIndex);
    endRemoveRows();
}

void TimelineObjectModel::setHideShy(bool enabled)
{
    m_hideShy = enabled;
    Q_EMIT roleUpdated(VisibleRole);
}

void TimelineObjectModel::setHideHidden(bool enabled)
{
    m_hideHidden = enabled;
    Q_EMIT roleUpdated(VisibleRole);
}

void TimelineObjectModel::setHideLocked(bool enabled)
{
    m_hideLocked = enabled;
    Q_EMIT roleUpdated(VisibleRole);
}

void TimelineObjectModel::updateActions()
{
    Q_EMIT rolesUpdated({HasActionRole, HasChildActionRole, HasComponentActionRole,
                         HasMasterActionRole, HasMasterChildActionRole,
                         HasMasterComponentActionRole});
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

    qt3dsdm::TPropertyHandleList properties;
    propertySystem->GetAggregateInstanceProperties(handle, properties);
    QVector<qt3dsdm::Qt3DSDMInstanceHandle> animatedProperties;
    for (const auto &propertyHandle: properties) {
        if (studioSystem->GetAnimationSystem()->IsPropertyAnimated(
                    handle, propertyHandle)) {
            animatedProperties.append(propertyHandle);
        }
    }
    m_properties[handle] = animatedProperties;

    qt3dsdm::TInstanceHandleList children(animatedProperties.begin(), animatedProperties.end());
    auto objectChildren = ObjectListModel::childrenList(slideHandle, handle);
    children.insert(children.end(), objectChildren.begin(), objectChildren.end());

    return children;
}

void TimelineObjectModel::appendKey(QVariantList &keyframes, IKeyframe *key, double timeRatio) const
{
    try {
        KeyframeInfo keyInfo;
        keyInfo.m_time = key->GetTime();
        keyInfo.m_selected = key->IsSelected();
        keyInfo.m_dynamic = key->IsDynamic();
        keyInfo.m_position = ::TimeToPos(keyInfo.m_time, timeRatio);

        keyframes.append(QVariant::fromValue(keyInfo));
    } catch (qt3dsdm::AnimationKeyframeNotFound) {
        // happens when the keyframe was removed and added again. This is because
        // CStudioAnimationSystem::Deanimate does not erase the keyframes, just
        // marks as being deleted for later reuse. If the UI requests an update
        // while a keyframe has no associated animation, this exception is triggered
    }
}

bool TimelineObjectModel::isVisible(const QModelIndex &index) const
{
    auto idx = index;
    while (idx.isValid()) {
        auto row = const_cast<TimelineObjectModel*>(this)->timelineRowForIndex(idx);

        if (!row)
            return false;

        auto binding = row->GetTimelineItemBinding();
        if (binding) {
            auto item = binding->GetTimelineItem();

            auto hidden = (m_hideShy && item->IsShy()) ||
                    (m_hideHidden && !item->IsVisible()) ||
                    (m_hideLocked && item->IsLocked());

            if (hidden)
                return false;
        }
        idx = idx.parent();
    }

    return true;
}
