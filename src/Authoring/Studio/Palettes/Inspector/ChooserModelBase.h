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
#ifndef CHOOSERMODELBASE_H
#define CHOOSERMODELBASE_H

#include "DispatchListeners.h"
#include "StudioObjectTypes.h"

#include <QFileSystemModel>
#include <QAbstractListModel>
#include <QList>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QFileSystemModel)

class ChooserModelBase : public QAbstractListModel, public CPresentationChangeListener
{
    Q_OBJECT

public:
    explicit ChooserModelBase(QObject *parent = nullptr);
    ~ChooserModelBase();

    enum {
        IsExpandableRole = QFileSystemModel::FilePermissions + 1,
        DepthRole,
        ExpandedRole,
        IsSelectableRole,
        IsCurrentFile
    };

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void expand(int row);
    Q_INVOKABLE void collapse(int row);

    void setCurrentFile(const QString &path);

    // CPresentationChangeListener
    void OnNewPresentation() override;

Q_SIGNALS:
    void modelChanged(QAbstractItemModel *model);

protected:
    EStudioObjectType getIconType(const QString &path) const;

    virtual bool isVisible(const QString &path) const = 0;

    struct FixedItem
    {
        EStudioObjectType iconType;
        QString iconSource;
        QString name;
    };

    virtual const QVector<FixedItem> getFixedItems() const = 0;

private:
    void setRootPath(const QString &path);
    void setRootIndex(const QModelIndex &rootIndex);
    void clearModelData();
    void showModelTopLevelItems();
    void showModelChildItems(const QModelIndex &parentItem, int start, int end);
    int modelIndexRow(const QModelIndex &modelIndex) const;
    bool isExpanded(const QModelIndex &modelIndex) const;
    QString getIconName(const QString &path) const;
    bool isVisible(const QModelIndex &modelIndex) const;
    bool hasVisibleChildren(const QModelIndex &modelIndex) const;
    void expand(const QModelIndex &modelIndex);

    void modelRowsInserted(const QModelIndex &parent, int start, int end);
    void modelRowsRemoved(const QModelIndex &parent, int start, int end);
    void modelRowsMoved(const QModelIndex &parent, int start, int end);
    void modelLayoutChanged();

    void rebuild();

    struct TreeItem {
        QPersistentModelIndex index;
        int depth;
        bool expanded;
        TreeItem *parent;
        int childCount;
    };

    QFileSystemModel *m_model;
    QPersistentModelIndex m_rootIndex;
    QList<TreeItem> m_items;
    QString m_currentFile;
};

#endif // CHOOSERMODELBASE_H
