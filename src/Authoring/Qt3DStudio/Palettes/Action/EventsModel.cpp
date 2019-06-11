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

#include "EventsModel.h"

#include "ClientDataModelBridge.h"
#include "Core.h"
#include "Doc.h"
#include "StudioUtils.h"
#include "StudioApp.h"
#include "Qt3DSDMStudioSystem.h"

EventsModel::EventsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void EventsModel::setEventList(const qt3dsdm::TEventHandleList &eventList)
{
    beginResetModel();

    m_rowCount = 0;
    m_events.clear();
    m_categories.clear();

    auto studioSystem = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    auto theBridge = studioSystem->GetClientDataModelBridge();
    auto thePos = eventList.begin();
    for (; thePos != eventList.end(); ++thePos) {
        qt3dsdm::SEventInfo theEvent = theBridge->GetEventInfo(*thePos);

        CategoryInfo category;
        category.name = QString::fromWCharArray(theEvent.m_Category.wide_str());
        if (!m_events.contains(category.name)) {
          qt3dsdm::SCategoryInfo theCategoryMetaData = studioSystem->GetActionMetaData()
                  ->GetEventCategory(theEvent.m_Category);
          category.icon = QString::fromWCharArray(theCategoryMetaData.m_Icon.wide_str());
          category.highlightIcon = QString::fromWCharArray(theCategoryMetaData.m_HighlightIcon.wide_str());
          category.description = QString::fromWCharArray(theCategoryMetaData.m_Description.wide_str());
          m_categories.append(category);
          m_rowCount++;
        }

        EventInfo eventInfo;
        // Use the formal name to display, but if the formal name is not set, use the name instead
        eventInfo.name = QString::fromWCharArray(theEvent.m_FormalName.wide_str());
        if (eventInfo.name.isEmpty())
            eventInfo.name = QString::fromWCharArray(theEvent.m_Name.wide_str());
        eventInfo.handle = *thePos;

        eventInfo.description = QString::fromWCharArray(theEvent.m_Description.wide_str());
        m_events[category.name].append(eventInfo);
        m_rowCount++;

        //KDAB_TODO set the selection to the current event
    }

    endResetModel();
}

void EventsModel::setHandlerList(const qt3dsdm::THandlerHandleList &handlerList)
{
    beginResetModel();
    m_rowCount = 0;
    m_events.clear();
    m_categories.clear();

    auto studioSystem = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    auto theBridge = studioSystem->GetClientDataModelBridge();
    auto thePos = handlerList.begin();
    for (; thePos != handlerList.end(); ++thePos) {
        qt3dsdm::SHandlerInfo handlerInfo = theBridge->GetHandlerInfo(*thePos);

        CategoryInfo category;
        category.name = QString::fromWCharArray(handlerInfo.m_Category.wide_str());
        if (!m_events.contains(category.name)) {
          qt3dsdm::SCategoryInfo theCategoryMetaData = studioSystem->GetActionMetaData()
                  ->GetHandlerCategory(handlerInfo.m_Category);
          category.icon = QString::fromWCharArray(theCategoryMetaData.m_Icon.wide_str());
          category.highlightIcon = QString::fromWCharArray(theCategoryMetaData.m_HighlightIcon.wide_str());
          category.description = QString::fromWCharArray(theCategoryMetaData.m_Description.wide_str());
          m_categories.append(category);
          m_rowCount++;
        }

        EventInfo eventInfo;
        // Use the formal name to display, but if the formal name is not set, use the name instead
        eventInfo.name = QString::fromWCharArray(handlerInfo.m_FormalName.wide_str());
        if (eventInfo.name.isEmpty())
            eventInfo.name = QString::fromWCharArray(handlerInfo.m_Name.wide_str());
        eventInfo.handle = *thePos;

        eventInfo.description = QString::fromWCharArray(handlerInfo.m_Description.wide_str());
        m_events[category.name].append(eventInfo);
        m_rowCount++;

        //KDAB_TODO set the selection to the current event
    }

    endResetModel();
}

int EventsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_rowCount;
}

QVariant EventsModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    const auto row = index.row();
    auto category = categoryForRow(row);

    bool isCategory = category.isValid();
    EventInfo event;
    if (!isCategory)
        event = eventForRow(row);

    switch (role) {
    case NameRole:
        return isCategory ? category.name : event.name;
    case DescriptionRole:
        return isCategory ? category.description: event.description;
    case IconRole:
        return isCategory ? StudioUtils::resourceImageUrl() + category.icon : QString();
    case HighlightedIconRole:
        return isCategory ? StudioUtils::resourceImageUrl() + category.highlightIcon : QString();
    case ExpandedRole:
        return isCategory ? category.expanded : false;
    case ParentExpandedRole: {
        if (isCategory)
            return true;
        for (int i = row - 1; i >= 0; i--) {
            auto parentCategory = categoryForRow(i);
            if (parentCategory.isValid())
                return parentCategory.expanded;
        }
        return false;
    }
    case IsCategoryRole:
        return isCategory;
    }

    return QVariant();
}

bool EventsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == ExpandedRole) {
        int catRow = categoryRowForRow(index.row());
        if (catRow != -1) {
            auto category = &m_categories[catRow];
            category->expanded = value.toBool();
            Q_EMIT dataChanged(this->index(0, 0), this->index(rowCount() - 1, 0), {});
            return true;
        }
    }
    return false;
}

QHash<int, QByteArray> EventsModel::roleNames() const
{
    auto names = QAbstractItemModel::roleNames();
    names.insert(NameRole, "name");
    names.insert(DescriptionRole, "description");
    names.insert(IconRole, "icon");
    names.insert(HighlightedIconRole, "highlightedIcon");
    names.insert(IsCategoryRole, "isCategory");
    names.insert(ExpandedRole, "expanded");
    names.insert(ParentExpandedRole, "parentExpanded");

    return names;
}

qt3dsdm::CDataModelHandle EventsModel::handleForRow(int row) const
{
    if (row < 0 || row >= m_rowCount)
        return {};

    auto event = eventForRow(row);
    if (event.isValid())
        return event.handle;

    return {};
}

int EventsModel::rowForEventName(const QString &event) const
{
    int i = 0;
    for (const auto &category: m_categories) {
        i++;
        const auto events = m_events[category.name];
        for (int j = 0; j < events.size(); j++, i++) {
            if (events[j].name == event)
                return i;
        }
    }
    return i;
}

EventsModel::CategoryInfo EventsModel::categoryForRow(int row) const
{
    int i = 0;
    for (const auto &category: m_categories) {
        if (i == row)
            return category;
        i += m_events[category.name].size();
        i++;
    }

    return {};
}

int EventsModel::categoryRowForRow(int row) const
{
    int i = 0;
    int catRow = 0;
    for (const auto &category: m_categories) {
        if (i == row)
            return catRow;
        i += m_events[category.name].size();
        i++;
        catRow++;
    }

    return -1;
}

EventsModel::EventInfo EventsModel::eventForRow(int row) const
{
    if (row == 0) // first line is not an event, but a category
        return {};

    int i = 0;
    for (const auto &category: m_categories) {
        i++;
        const auto events = m_events[category.name];
        const int index = (row - i);
        if (row < i + events.size() &&  (index >= 0) ) {
            return events[index];
        }
        i += events.size();
    }

    return {};
}
