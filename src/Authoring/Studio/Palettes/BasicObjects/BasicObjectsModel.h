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

#ifndef BASICOBJECTSMODEL_H
#define BASICOBJECTSMODEL_H

#include <QAbstractListModel>

#include "IDragable.h"
#include "StudioObjectTypes.h"

class BasicObjectItem : public IDragable {

public:
    BasicObjectItem() {}
    BasicObjectItem(const QString &name, const QString &icon,
                    EStudioObjectType objectType, EPrimitiveType primitiveType)
        : m_name(name), m_icon(icon)
        , m_objectType(objectType), m_primitiveType(primitiveType)
    { }

    virtual ~BasicObjectItem();

    QString name() const { return m_name; }
    QString icon() const { return m_icon; }

    EStudioObjectType objectType() const { return m_objectType; }
    EPrimitiveType primitiveType() const { return m_primitiveType; }

    void setName(const QString &name) { m_name = name; }
    void setIcon(const QString &icon) { m_icon = icon; }
    void setObjectType(EStudioObjectType type) { m_objectType = type; }
    void setPrimitveType(EPrimitiveType type) { m_primitiveType = type; }

    long GetFlavor() const override {return EUIC_FLAVOR_BASIC_OBJECTS;}

private:
    QString m_name;
    QString m_icon;
    EStudioObjectType m_objectType;
    EPrimitiveType m_primitiveType;
};

class BasicObjectsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    BasicObjectsModel(QObject *parent = nullptr);

    enum Roles {
        NameRole = Qt::DisplayRole,
        IconRole = Qt::DecorationRole,
        ObjectTypeRole = Qt::UserRole + 1,
        PrimitiveTypeRole
    };

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

private:
    void initialize();

    QVector<BasicObjectItem> m_ObjectItems;

    const QString m_MimeType = QLatin1String("application/x-basic-object");
};

#endif // BASICOBJECTSMODEL_H
