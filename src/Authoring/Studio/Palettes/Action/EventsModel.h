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

#ifndef EVENTSMODEL_H
#define EVENTSMODEL_H

#include <QAbstractListModel>

#include "UICDMHandles.h"

/** Model for both action events and action handlers */
class EventsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit EventsModel(QObject *parent = nullptr);

    void setEventList(const UICDM::TEventHandleList &eventList);
    void setHandlerList(const UICDM::THandlerHandleList &handlerList);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    enum Roles {
        NameRole = Qt::DisplayRole,
        DescriptionRole = Qt::UserRole + 1,
        IconRole,
        HighlightedIconRole,
        ExpandedRole,
        ParentExpandedRole,
        IsCategoryRole
    };

    QHash<int, QByteArray> roleNames() const override;

    UICDM::CDataModelHandle handleForRow(int row) const;

private:
    struct EventInfo {
        UICDM::CDataModelHandle handle;
        QString name;
        QString description;

        bool isValid() const { return handle.Valid(); }
    };

    struct CategoryInfo {
        QString name;
        QString icon;
        QString description;
        QString highlightIcon;
        bool expanded =  true;

        bool isValid() const { return !name.isEmpty(); }
    };

    CategoryInfo categoryForRow(int row) const;
    int categoryRowForRow(int row) const;
    EventInfo eventForRow(int row) const;

    QHash<QString, QVector<EventInfo> > m_events;
    QVector<CategoryInfo> m_categories;
    int m_rowCount = 0;
};

#endif // EVENTSMODEL_H
