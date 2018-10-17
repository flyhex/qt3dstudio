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

#include "DataInputSelectModel.h"
#include "StudioUtils.h"
#include "StudioObjectTypes.h"

const int DataInputSelectModel::TypeRole = Qt::UserRole + 1;

DataInputSelectModel::DataInputSelectModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

DataInputSelectModel::~DataInputSelectModel()
{
}

QString DataInputSelectModel::getActiveIconPath() const
{
    return StudioUtils::resourceImageUrl()
            + CStudioObjectTypes::GetNormalIconName(OBJTYPE_DATAINPUT);
}

QString DataInputSelectModel::getInactiveIconPath() const
{
    return StudioUtils::resourceImageUrl()
            + CStudioObjectTypes::GetDisabledIconName(OBJTYPE_DATAINPUT);
}

QVariant DataInputSelectModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_diTypePairList.size() || index.row() < 0)
        return QVariant();

    QPair<QString, QString> pair = m_diTypePairList.at(index.row());
    if (role == Qt::DisplayRole)
        return QVariant::fromValue(QString(pair.first));
    else if (role == TypeRole)
        return QVariant::fromValue(QString(pair.second));

    return QVariant();
}

void DataInputSelectModel::setData(const QVector<QPair<QString, QString>> &data)
{
    // need explicit begin/end in order to make model content update
    // in UI also when datainput chooser widget parent is (QWidget-based)
    // timeline toolbar or slideview
    beginResetModel();
    m_diTypePairList = data;
    endResetModel();
}

void DataInputSelectModel::clear()
{
    beginResetModel();
    m_diTypePairList.clear();
    endResetModel();
}

QHash<int, QByteArray> DataInputSelectModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles.insert(TypeRole, QByteArray("datatype"));
    return roles;
}
