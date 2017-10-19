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

#ifndef OBJECTLISTMODEL_H
#define OBJECTLISTMODEL_H

#include <QAbstractItemModel>
#include <QAbstractListModel>

#include "Qt3DSDMHandles.h"

class IObjectReferenceHelper;
class CCore;

class ObjectListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ObjectListModel(CCore *core, const qt3dsdm::Qt3DSDMInstanceHandle &baseHandle, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    enum Roles {
        NameRole = Qt::DisplayRole,
        AbsolutePathRole = Qt::UserRole + 1,
        PathReferenceRole,
        IconRole,
        TextColorRole,
        HandleRole,
        LastRole = HandleRole
    };

    QHash<int, QByteArray> roleNames() const override;

    qt3dsdm::Qt3DSDMInstanceHandle baseHandle() const {return m_baseHandle;}

private:
    qt3dsdm::Qt3DSDMInstanceHandle handleForIndex(const QModelIndex &index) const;

    qt3dsdm::TInstanceHandleList childrenList(const qt3dsdm::Qt3DSDMSlideHandle &slideHandle,
                                                    const qt3dsdm::Qt3DSDMInstanceHandle &handle) const;

    QString nameForHandle(const qt3dsdm::Qt3DSDMInstanceHandle &handle) const;

    QModelIndex indexForHandle(const qt3dsdm::Qt3DSDMInstanceHandle &handle, const QModelIndex &startIndex = {}) const;

    CCore *m_core;
    qt3dsdm::Qt3DSDMSlideHandle m_slideHandle;
    qt3dsdm::Qt3DSDMInstanceHandle m_baseHandle;
    IObjectReferenceHelper *m_objRefHelper;
};

class FlatObjectListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    FlatObjectListModel(ObjectListModel *sourceModel, QObject *parent = nullptr);

    enum Roles {
        DepthRole = ObjectListModel::LastRole + 1,
        ExpandedRole,
        ParentExpandedRole,
        HasChildrenRole
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &data, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void setSourceModel(ObjectListModel *sourceModel);
    ObjectListModel *sourceModel() const {return m_sourceModel;}

private:
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    struct SourceInfo {
        bool expanded = false;
        int depth = 0;
        QPersistentModelIndex index;
    };

    QVector<SourceInfo> collectSourceIndexes(const QModelIndex &sourceIndex, int depth) const;

    QVector<SourceInfo> m_sourceInfo;
    ObjectListModel *m_sourceModel;
};


#endif // OBJECTLISTMODEL_H
