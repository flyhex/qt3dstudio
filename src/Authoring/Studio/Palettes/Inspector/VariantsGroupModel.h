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

#ifndef VARIANTSGROUPMODEL_H
#define VARIANTSGROUPMODEL_H

#include <QtCore/qabstractitemmodel.h>

class VariantsTagModel;

class VariantsGroupModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool variantsEmpty MEMBER m_variantsEmpty NOTIFY varaintsEmptyChanged)

public:
Q_SIGNALS:
    void varaintsEmptyChanged();

public:
    explicit VariantsGroupModel(QObject *parent = nullptr);

    enum Roles {
        GroupTitleRole = Qt::UserRole + 1,
        GroupColorRole,
        TagRole
    };

    struct TagGroupData
    {
        QString m_title;
        QString m_color;
        VariantsTagModel *m_tagsModel = nullptr;
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = GroupTitleRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void refresh();

    Q_INVOKABLE void setTagState(const QString &group, const QString &tag, bool selected);
    Q_INVOKABLE void addNewTag(const QString &group);
    Q_INVOKABLE void addNewGroup();
    Q_INVOKABLE void importVariants();
    Q_INVOKABLE void exportVariants();


protected:
      QHash<int, QByteArray> roleNames() const override;

private:
      QVector<TagGroupData> m_data;
      int m_instance = 0; // selected layer instance
      int m_property = 0; // variant tags property handler
      bool m_variantsEmpty = true; // no groups (nor tags)
};

#endif // VARIANTSGROUPMODEL_H
