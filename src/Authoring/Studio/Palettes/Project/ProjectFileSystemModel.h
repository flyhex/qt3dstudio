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
#ifndef TREEVIEWADAPTOR_H
#define TREEVIEWADAPTOR_H

#include "StudioObjectTypes.h"
#include "DocumentEditorEnumerations.h"

#include <QtWidgets/qfilesystemmodel.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>

QT_FORWARD_DECLARE_CLASS(QFileSystemModel)

class ProjectFileSystemModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ProjectFileSystemModel(QObject *parent = nullptr);

    enum {
        IsExpandableRole = QFileSystemModel::FilePermissions + 1,
        IsDraggableRole,
        IsReferencedRole,
        DepthRole,
        ExpandedRole,
    };

    void setRootPath(const QString &path);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    QString filePath(int row) const;
    bool isRefreshable(int row) const;

    void updateReferences(bool emitDataChanged);
    Q3DStudio::DocumentEditorFileType::Enum assetTypeForRow(int row);

    Q_INVOKABLE void expand(int row);
    Q_INVOKABLE void collapse(int row);

    Q_INVOKABLE void importUrls(const QList<QUrl> &urls, int row, bool autoSort = true);
    Q_INVOKABLE bool hasValidUrlsForDropping(const QList<QUrl> &urls) const;

Q_SIGNALS:
    void modelChanged(QAbstractItemModel *model);

private:
    void setRootIndex(const QModelIndex &rootIndex);
    void clearModelData();
    void showModelTopLevelItems();
    void showModelChildItems(const QModelIndex &parentItem, int start, int end);
    int modelIndexRow(const QModelIndex &modelIndex) const;
    bool isExpanded(const QModelIndex &modelIndex) const;
    QString getIconName(const QString &path) const;
    EStudioObjectType getIconType(const QString &path) const;
    bool isVisible(const QModelIndex& modelIndex) const;
    bool hasVisibleChildren(const QModelIndex &modelIndex) const;
    void importUrl(const QDir &targetDir, const QUrl &url) const;
    int rowForPath(const QString &path) const;

    void modelRowsInserted(const QModelIndex &parent, int start, int end);
    void modelRowsRemoved(const QModelIndex &parent, int start, int end);
    void modelRowsMoved(const QModelIndex &parent, int start, int end);
    void modelLayoutChanged();

    void updateDefaultDirMap();

    struct TreeItem {
        QPersistentModelIndex index;
        int depth;
        bool expanded;
        TreeItem *parent;
        int childCount;
    };

    QFileSystemModel *m_model = nullptr;
    QPersistentModelIndex m_rootIndex;
    QList<TreeItem> m_items;
    QStringList m_references;
    QHash<QString, QString> m_defaultDirToAbsPathMap;
};

#endif // TREEVIEWADAPTOR_H
