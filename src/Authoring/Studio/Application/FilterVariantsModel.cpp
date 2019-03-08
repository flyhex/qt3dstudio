/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "FilterVariantsModel.h"
#include "VariantsTagModel.h"
#include "StudioApp.h"
#include "Views.h"
#include "MainFrm.h"
#include "Core.h"
#include "VariantTagDialog.h"

FilterVariantsModel::FilterVariantsModel(VariantsFilterT &variantsFilter, QObject *parent)
    : QAbstractListModel(parent)
    , m_variantsFilter(variantsFilter)
{

}

void FilterVariantsModel::refresh()
{
    beginResetModel();

    // delete tag models
    for (auto &g : qAsConst(m_data))
        delete g.m_tagsModel;

    m_data.clear();

    // build the variants data model
    const auto variantsDef = g_StudioApp.GetCore()->getProjectFile().variantsDef();
    const auto keys = variantsDef.keys();
    for (auto &group : keys) {
        VariantsGroupData g;
        g.m_title = group;
        g.m_color = variantsDef[group].m_color;
        QVector<std::pair<QString, bool> > tags;
        for (int i = 0; i < variantsDef[group].m_tags.length(); ++i) {
            QString tag = variantsDef[group].m_tags[i];
            bool state = m_variantsFilter.contains(group) && m_variantsFilter[group].contains(tag);
            tags.append({tag, state});
        }
        g.m_tagsModel = new VariantsTagModel(tags);
        m_data.push_back(g);
    }

    endResetModel();
}

int FilterVariantsModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return m_data.size();
}

QVariant FilterVariantsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == GroupTitleRole)
        return m_data.at(index.row()).m_title;
    else if (role == GroupColorRole)
        return m_data.at(index.row()).m_color;
    else if (role == TagsRole)
        return QVariant::fromValue(m_data.at(index.row()).m_tagsModel);

    return QVariant();
}

void FilterVariantsModel::setTagState(const QString &group, const QString &tag, bool selected)
{
    if (selected)
        m_variantsFilter[group].insert(tag);
    else
        m_variantsFilter[group].remove(tag);

    bool filtered = false;
    for (auto &g : qAsConst(m_variantsFilter)) {
        if (!g.isEmpty()) {
            filtered = true;
            break;
        }
    }

    g_StudioApp.GetViews()->getMainFrame()->updateActionPreviewVariantsState(filtered);
}

void FilterVariantsModel::toggleGroupState(const QString &group)
{
    const auto variantsDef = g_StudioApp.GetCore()->getProjectFile().variantsDef();

    if (m_variantsFilter[group].size() < variantsDef[group].m_tags.size()) {
        // not all tags selected, select all
        const auto tags = variantsDef[group].m_tags;
        for (auto &t : tags)
            m_variantsFilter[group].insert(t);
    } else {
        // all tags selected, select none
        m_variantsFilter[group].clear();
    }
    refresh();

    bool filtered = false;
    for (auto &g : qAsConst(m_variantsFilter)) {
        if (!g.isEmpty()) {
            filtered = true;
            break;
        }
    }

    g_StudioApp.GetViews()->getMainFrame()->updateActionPreviewVariantsState(filtered);
}

void FilterVariantsModel::clearAll()
{
    m_variantsFilter.clear();
    refresh();
    g_StudioApp.GetViews()->getMainFrame()->updateActionPreviewVariantsState(false);
}

QHash<int, QByteArray> FilterVariantsModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(GroupTitleRole, "group");
    names.insert(GroupColorRole, "color");
    names.insert(TagsRole, "tags");
    return names;
}

Qt::ItemFlags FilterVariantsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable;
}
