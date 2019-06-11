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

#include "BasicObjectsModel.h"
#include "DropSource.h"
#include "Literals.h"
#include "StudioUtils.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QMimeData>

BasicObjectsModel::BasicObjectsModel(QObject *parent) : QAbstractListModel(parent)
{
    initialize();
}

void BasicObjectsModel::initialize()
{
    m_ObjectItems = InitializeObjectModel();
}

const QVector<BasicObjectItem> BasicObjectsModel::InitializeObjectModel()
{
    return {
        {tr("Rectangle"), "Asset-Rectangle-Normal.png"_L1, OBJTYPE_MODEL, PRIMITIVETYPE_RECT},
        {tr("Sphere"), "Asset-Sphere-Normal.png"_L1, OBJTYPE_MODEL, PRIMITIVETYPE_SPHERE},
        {tr("Cube"), "Asset-Cube-Normal.png"_L1, OBJTYPE_MODEL, PRIMITIVETYPE_BOX},
        {tr("Cylinder"), "Asset-Cylinder-Normal.png"_L1, OBJTYPE_MODEL, PRIMITIVETYPE_CYLINDER},
        {tr("Cone"), "Asset-Cone-Normal.png"_L1, OBJTYPE_MODEL, PRIMITIVETYPE_CONE},
        {tr("Component"), "Asset-Component-Normal.png"_L1, OBJTYPE_COMPONENT, PRIMITIVETYPE_UNKNOWN},
        {tr("Group"), "Asset-Group-Normal.png"_L1, OBJTYPE_GROUP, PRIMITIVETYPE_UNKNOWN},
        {tr("Text"), "Asset-Text-Normal.png"_L1, OBJTYPE_TEXT, PRIMITIVETYPE_UNKNOWN},
        {tr("Camera"), "Asset-Camera-Normal.png"_L1, OBJTYPE_CAMERA, PRIMITIVETYPE_UNKNOWN},
        {tr("Light"), "Asset-Light-Normal.png"_L1, OBJTYPE_LIGHT, PRIMITIVETYPE_UNKNOWN},
        // Hide alias until it will be replaced with prefabs
        //{tr("Alias"), "Asset-Alias-Normal.png"_L1, OBJTYPE_ALIAS, PRIMITIVETYPE_UNKNOWN},
    };
}

// Returns meshes part of basic objects
const QVector<BasicObjectItem> BasicObjectsModel::BasicMeshesModel()
{
    return InitializeObjectModel().mid(0, 5);
}

QVariant BasicObjectsModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(),index.parent()))
        return {};

    const auto row = index.row();

    switch (role) {
    case NameRole: return m_ObjectItems.at(row).name();
    case IconRole: return StudioUtils::resourceImageUrl() +
                m_ObjectItems.at(row).icon();
    case ObjectTypeRole: return m_ObjectItems.at(row).objectType();
    case PrimitiveTypeRole: return m_ObjectItems.at(row).primitiveType();
    }

    return {};
}

int BasicObjectsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_ObjectItems.count();
}

QHash<int, QByteArray> BasicObjectsModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(NameRole, "name");
    names.insert(IconRole, "icon");

    return names;
}

Qt::ItemFlags BasicObjectsModel::flags(const QModelIndex &index) const {
    if (index.isValid())
        return Qt::ItemIsDragEnabled;

    return QAbstractListModel::flags(index);
}

QStringList BasicObjectsModel::mimeTypes() const
{
    return { m_MimeType };
}

QMimeData *BasicObjectsModel::mimeData(const QModelIndexList &indexes) const
{

    const auto row = indexes.first().row(); // we support only one item for D&D
    auto object = m_ObjectItems.at(row);

    auto *data = CDropSourceFactory::Create(object.GetFlavor(), &object);
    return data;
}

BasicObjectItem::~BasicObjectItem()
{
}
