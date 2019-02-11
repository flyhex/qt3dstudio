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

#include "VariantsTagModel.h"

VariantsTagModel::VariantsTagModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

void VariantsTagModel::init(const QVector<std::pair<QString, bool> > &data)
{
    m_data = data;
}

void VariantsTagModel::updateTagState(const QString &tag, bool selected)
{
    for (auto &t : m_data) {
        if (t.first == tag) {
            t.second = selected;
            break;
        }
    }
}

// return the tags in a formatted string to be saved to the property
QString VariantsTagModel::serialize(const QString &groupName) const
{
    QString ret;
    bool skipFirst = false;
    for (auto &t : qAsConst(m_data)) {
        if (t.second) {
            if (skipFirst)
                ret.append(QLatin1Char(','));

            ret.append(groupName + QLatin1Char(':') + t.first);

            skipFirst = true;
        }
    }

    return ret;
}

int VariantsTagModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return m_data.size();
}

QVariant VariantsTagModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == TagRole)
        return m_data.at(index.row()).first;
    else if (role == SelectedRole)
        return m_data.at(index.row()).second;

    return QVariant();
}

QHash<int, QByteArray> VariantsTagModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(TagRole, "tag");
    names.insert(SelectedRole, "selected");
    return names;
}

Qt::ItemFlags VariantsTagModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable; // FIXME: Implement me!
}
