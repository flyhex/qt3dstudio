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
#include <QtCore/qset.h>

#include "Qt3DSCommonPrecompile.h"

#include "ChooserModelBase.h"
#include "Core.h"
#include "Dispatch.h"
#include "Doc.h"
#include "StudioUtils.h"
#include "Qt3DSFileTools.h"
#include "ImportUtils.h"
#include "StudioApp.h"

ChooserModelBase::ChooserModelBase(QObject *parent) : QAbstractListModel(parent)
    , m_model(new QFileSystemModel(this))
{
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &ChooserModelBase::modelRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ChooserModelBase::modelRowsRemoved);
    connect(m_model, &QAbstractItemModel::layoutChanged, this, &ChooserModelBase::modelLayoutChanged);

    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);

    rebuild();
}

ChooserModelBase::~ChooserModelBase()
{
    g_StudioApp.GetCore()->GetDispatch()->RemovePresentationChangeListener(this);
}

QHash<int, QByteArray> ChooserModelBase::roleNames() const
{
    auto modelRoleNames = m_model->roleNames();
    modelRoleNames.insert(IsExpandableRole, "isExpandable");
    modelRoleNames.insert(DepthRole, "depth");
    modelRoleNames.insert(ExpandedRole, "expanded");
    modelRoleNames.insert(IsSelectableRole, "isSelectable");
    modelRoleNames.insert(IsCurrentFile, "isCurrentFile");
    return modelRoleNames;
}

int ChooserModelBase::rowCount(const QModelIndex &) const
{
    return getFixedItems().count() + m_items.count();
}

QVariant ChooserModelBase::data(const QModelIndex &index, int role) const
{
    const int row = index.row();

    const auto fixedItems = getFixedItems();
    const int fixedItemCount = fixedItems.count();

    if (row < fixedItemCount) {
        const auto &item = fixedItems.at(row);

        switch (role) {
        case Qt::DecorationRole:
            if (!item.iconSource.isEmpty()) {
                return StudioUtils::resourceImageUrl() + item.iconSource;
            } else {
                return StudioUtils::resourceImageUrl()
                        + CStudioObjectTypes::GetNormalIconName(item.iconType);
            }

        case IsExpandableRole:
            return false;

        case DepthRole:
            return 0;

        case ExpandedRole:
            return false;

        case IsSelectableRole:
            return true;

        case IsCurrentFile:
            return item.name == m_currentFile;

        default:
            return item.name;
        }
    } else {
        const auto &item = m_items.at(row - fixedItemCount);

        switch (role) {
        case Qt::DecorationRole: {
            QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
            return StudioUtils::resourceImageUrl() + getIconName(path);
        }

        case IsExpandableRole: {
            QFileInfo fileInfo(item.index.data(QFileSystemModel::FilePathRole).toString());
            return fileInfo.isDir();
        }

        case DepthRole:
            return item.depth;

        case ExpandedRole:
            return item.expanded;

        case IsSelectableRole: {
            QFileInfo fileInfo(item.index.data(QFileSystemModel::FilePathRole).toString());
            return fileInfo.isFile();
        }

        case IsCurrentFile: {
            QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
            return path == m_currentFile;
        }

        case QFileSystemModel::FileNameRole: {
            QString displayName = specialDisplayName(item);
            if (displayName.isEmpty())
                displayName = m_model->data(item.index, QFileSystemModel::FileNameRole).toString();
            return displayName;
        }

        default:
            return m_model->data(item.index, role).toString();
        }
    }
}

void ChooserModelBase::setCurrentFile(const QString &path)
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const QDir documentDir(doc->GetDocumentDirectory().toQString());
    const QString fullPath = QDir::cleanPath(documentDir.filePath(path));

    if (fullPath != m_currentFile) {
        const auto fixedItems = getFixedItems();

        const auto fileRow = [this, &fixedItems](const QString &path)
        {
            const int fixedItemCount = fixedItems.count();

            for (int i = 0; i < fixedItemCount; ++i) {
                const auto &item = fixedItems.at(i);

                if (item.name == path)
                    return i;
            }

            const int itemCount = m_items.count();

            for (int i = 0; i < itemCount; ++i) {
                const auto &item = m_items.at(i);

                if (item.index.data(QFileSystemModel::FilePathRole).toString() == path)
                    return fixedItemCount + i;
            }

            return -1;
        };

        int previousRow = fileRow(m_currentFile);
        int currentRow = fileRow(fullPath);

        m_currentFile = fullPath;

        if (previousRow != -1)
            Q_EMIT dataChanged(index(previousRow), index(previousRow));

        if (currentRow != -1)
            Q_EMIT dataChanged(index(currentRow), index(currentRow));
    }

    // expand parent folder if current file is hidden
    auto matched = m_model->match(m_rootIndex, QFileSystemModel::FilePathRole, path, 1,
                                  Qt::MatchExactly|Qt::MatchRecursive);
    if (!matched.isEmpty()) {
        auto modelIndex = matched.first();
        if (modelIndexRow(modelIndex) == -1)
            expand(m_model->parent(modelIndex));
    }
}

void ChooserModelBase::expand(const QModelIndex &modelIndex)
{
    if (modelIndex == m_rootIndex)
        return;

    int row = modelIndexRow(modelIndex);
    if (row == -1) {
        QModelIndex parentIndex = m_model->parent(modelIndex);
        expand(parentIndex);

        row = modelIndexRow(modelIndex);
        Q_ASSERT(row != -1);
    }

    if (!m_items.at(row).expanded)
        expand(row + getFixedItems().count());
}

void ChooserModelBase::setRootPath(const QString &path)
{
    setRootIndex(m_model->setRootPath(path));
}

void ChooserModelBase::setRootIndex(const QModelIndex &rootIndex)
{
    if (rootIndex != m_rootIndex) {
        clearModelData();
        m_rootIndex = rootIndex;
        showModelTopLevelItems();
    }
}

void ChooserModelBase::clearModelData()
{
    if (!m_items.isEmpty()) {
        const auto fixedItemCount = getFixedItems().count();
        beginRemoveRows({}, fixedItemCount, fixedItemCount + m_items.count() - 1);
        m_items.clear();
        endRemoveRows();
    }
}

void ChooserModelBase::showModelTopLevelItems()
{
    int rowCount = m_model->rowCount(m_rootIndex);

    if (rowCount == 0) {
        if (m_model->hasChildren(m_rootIndex) && m_model->canFetchMore(m_rootIndex))
            m_model->fetchMore(m_rootIndex);
    } else {
        showModelChildItems(m_rootIndex, 0, rowCount - 1);

        for (int i = 0; i < rowCount; ++i) {
            const auto &childIndex = m_model->index(i, 0, m_rootIndex);
            if (m_model->hasChildren(childIndex) && m_model->canFetchMore(childIndex))
                m_model->fetchMore(childIndex);
        }
    }
}

void ChooserModelBase::showModelChildItems(const QModelIndex &parentIndex, int start, int end)
{
    QVector<QModelIndex> rowsToInsert;
    for (int i = start; i <= end; ++i) {
        const auto &childIndex = m_model->index(i, 0, parentIndex);
        if (isVisible(childIndex))
            rowsToInsert.append(childIndex);
    }

    const int insertCount = rowsToInsert.count();

    if (insertCount != 0) {
        TreeItem *parent;
        int depth, startRow;

        if (parentIndex == m_rootIndex) {
            parent = nullptr;
            depth = 0;
            startRow = 0;
        } else {
            const int parentRow = modelIndexRow(parentIndex);
            Q_ASSERT(parentRow != -1 && isVisible(parentIndex));
            parent = &m_items[parentRow];
            depth = parent->depth + 1;
            startRow = parentRow + parent->childCount + 1;
        }

        const int fixedItemCount = getFixedItems().count();
        beginInsertRows({}, startRow + fixedItemCount, startRow + fixedItemCount + insertCount - 1);

        for (auto it = rowsToInsert.rbegin(); it != rowsToInsert.rend(); ++it)
            m_items.insert(startRow, { *it, depth, false, parent, 0 });

        for (; parent != nullptr; parent = parent->parent)
            parent->childCount += insertCount;

        endInsertRows();
    }
}

void ChooserModelBase::expand(int row)
{
    const int fixedItemCount = getFixedItems().count();
    Q_ASSERT(row >= fixedItemCount && row < fixedItemCount + m_items.count());

    auto &item = m_items[row - fixedItemCount];
    Q_ASSERT(item.expanded == false);

    const auto &modelIndex = item.index;

    const int rowCount = m_model->rowCount(modelIndex);
    if (rowCount == 0) {
        if (m_model->hasChildren(modelIndex) && m_model->canFetchMore(modelIndex))
            m_model->fetchMore(modelIndex);
    } else {
        showModelChildItems(modelIndex, 0, rowCount - 1);
    }

    item.expanded = true;
    Q_EMIT dataChanged(index(row), index(row));
}

void ChooserModelBase::collapse(int row)
{
    const int fixedItemCount = getFixedItems().count();
    Q_ASSERT(row >= fixedItemCount && row < fixedItemCount + m_items.count());

    auto &item = m_items[row - fixedItemCount];
    Q_ASSERT(item.expanded == true);

    const int childCount = item.childCount;

    if (childCount > 0) {
        beginRemoveRows({}, row + 1, row + childCount);

        auto first = std::begin(m_items) + row - fixedItemCount + 1;
        m_items.erase(first, first + childCount);

        for (auto parent = &item; parent != nullptr; parent = parent->parent)
            parent->childCount -= childCount;

        endRemoveRows();
    }

    item.expanded = false;
    Q_EMIT dataChanged(index(row), index(row));
}

void ChooserModelBase::OnNewPresentation()
{
    rebuild();
}

int ChooserModelBase::modelIndexRow(const QModelIndex &modelIndex) const
{
    auto it = std::find_if(std::begin(m_items), std::end(m_items),
                [&modelIndex](const TreeItem &item)
                {
                    return item.index == modelIndex;
                });

    return it != std::end(m_items) ? std::distance(std::begin(m_items), it) : -1;
}

bool ChooserModelBase::isExpanded(const QModelIndex &modelIndex) const
{
    if (modelIndex == m_rootIndex) {
        return true;
    } else {
        const int row = modelIndexRow(modelIndex);
        return row != -1 && m_items.at(row).expanded;
    }
}

EStudioObjectType ChooserModelBase::getIconType(const QString &path) const
{
    return Q3DStudio::ImportUtils::GetObjectFileTypeForFile(path).m_IconType;
}

QString ChooserModelBase::specialDisplayName(const ChooserModelBase::TreeItem &item) const
{
    Q_UNUSED(item)
    return {};
}

QString ChooserModelBase::getIconName(const QString &path) const
{
    QString iconName;

    QFileInfo fileInfo(path);
    if (fileInfo.isFile()) {
        EStudioObjectType type = getIconType(path);
        if (type != OBJTYPE_UNKNOWN)
            iconName = CStudioObjectTypes::GetNormalIconName(type);
        else
            iconName = QStringLiteral("Objects-Layer-Normal.png");
    } else {
        iconName = QStringLiteral("Objects-Folder-Normal.png");
    }

    return iconName;
}

bool ChooserModelBase::isVisible(const QModelIndex &modelIndex) const
{
    QString path = modelIndex.data(QFileSystemModel::FilePathRole).toString();
    QFileInfo fileInfo(path);

    if (fileInfo.isFile()) {
        return isVisible(path);
    } else {
        return hasVisibleChildren(modelIndex);
    }
}

bool ChooserModelBase::hasVisibleChildren(const QModelIndex &modelIndex) const
{
    int rowCount = m_model->rowCount(modelIndex);

    for (int i = 0; i < rowCount; ++i) {
        const auto &childIndex = m_model->index(i, 0, modelIndex);

        if (m_model->hasChildren(childIndex)) {
            if (hasVisibleChildren(childIndex))
                return true;
        } else {
            QString path = childIndex.data(QFileSystemModel::FilePathRole).toString();
            QFileInfo fileInfo(path);
            if (fileInfo.isFile() && isVisible(path))
                return true;
        }
    }

    return false;
}

void ChooserModelBase::modelRowsInserted(const QModelIndex &parentIndex, int start, int end)
{
    if (!m_rootIndex.isValid())
        return;

    if (isExpanded(parentIndex)) {
        showModelChildItems(parentIndex, start, end);
    } else {
        if (modelIndexRow(parentIndex) == -1) {
            // parent wasn't inserted in model yet, check if any of the new rows is visible

            bool visible = false;

            for (int i = start; i <= end; ++i) {
                const auto &childIndex = m_model->index(i, 0, parentIndex);
                QString path = childIndex.data(QFileSystemModel::FilePathRole).toString();
                QFileInfo fileInfo(path);
                if (fileInfo.isFile() && isVisible(path)) {
                    visible = true;
                    break;
                }
            }

            // if any of the new rows is visible, insert parent folder index into model

            if (visible) {
                QModelIndex index = parentIndex, parent = m_model->parent(parentIndex);

                while (parent != m_rootIndex && modelIndexRow(parent) == -1) {
                    index = parent;
                    parent = m_model->parent(parent);
                }

                if (isExpanded(parent) && modelIndexRow(index) == -1) {
                    const int row = index.row();
                    showModelChildItems(parent, row, row);
                }
            }
        }

        // if one of the new rows is the current file expand parent folder

        bool containsCurrent = false;

        for (int i = start; i <= end; ++i) {
            const auto &childIndex = m_model->index(i, 0, parentIndex);
            if (childIndex.data(QFileSystemModel::FilePathRole).toString() == m_currentFile) {
                containsCurrent = true;
                break;
            }
        }

        if (containsCurrent)
            expand(parentIndex);
    }

    // fetch children so we're notified when files are added or removed

    for (int i = start; i <= end; ++i) {
        const auto &childIndex = m_model->index(i, 0, parentIndex);
        if (m_model->hasChildren(childIndex) && m_model->canFetchMore(childIndex))
            m_model->fetchMore(childIndex);
    }
}

void ChooserModelBase::modelRowsRemoved(const QModelIndex &parentIndex, int start, int end)
{
    if (!m_rootIndex.isValid())
        return;

    if (isExpanded(parentIndex)) {
        const auto fixedItems = getFixedItems();

        const auto removeRow = [this, &fixedItems](int row)
        {
            const auto &item = m_items.at(row);

            const int fixedItemCount = fixedItems.count();
            beginRemoveRows({}, row + fixedItemCount, row + fixedItemCount + item.childCount);

            for (auto parent = item.parent; parent != nullptr; parent = parent->parent)
                parent->childCount -= 1 + item.childCount;

            m_items.erase(std::begin(m_items) + row, std::begin(m_items) + row + item.childCount + 1);

            endRemoveRows();
        };

        // remove rows

        for (int i = start; i <= end; ++i) {
            const int row = modelIndexRow(m_model->index(i, 0, parentIndex));
            if (row != -1)
                removeRow(row);
        }

        // also remove folder row if there are no more visible children

        QModelIndex index = parentIndex;

        while (index != m_rootIndex && !hasVisibleChildren(index)) {
            const int row = modelIndexRow(index);
            Q_ASSERT(row != -1);
            removeRow(row);
            index = m_model->parent(index);
        }
    }
}

void ChooserModelBase::modelLayoutChanged()
{
    if (!m_rootIndex.isValid())
        return;

    QSet<QPersistentModelIndex> expandedItems;
    for (const auto &item : m_items) {
        if (item.expanded)
            expandedItems.insert(item.index);
    }

    const std::function<int(const QModelIndex &, TreeItem *)> insertChildren =
        [this, &expandedItems, &insertChildren](const QModelIndex &parentIndex, TreeItem *parent)
        {
            Q_ASSERT(parentIndex == m_rootIndex || isVisible(parentIndex));

            const int rowCount = m_model->rowCount(parentIndex);
            const int depth = parent == nullptr ? 0 : parent->depth + 1;

            int childCount = 0;

            for (int i = 0; i < rowCount; ++i) {
                const auto &childIndex = m_model->index(i, 0, parentIndex);
                if (isVisible(childIndex)) {
                    const bool expanded = expandedItems.contains(childIndex);
                    m_items.append({ childIndex, depth, expanded, parent, 0 });
                    auto &item = m_items.last();
                    if (expanded) {
                        item.childCount = insertChildren(childIndex, &item);
                        childCount += item.childCount;
                    }
                    ++childCount;
                }
            }

            return childCount;
        };

    const int itemCount = m_items.count();

    m_items.clear();
    m_items.reserve(itemCount);

    insertChildren(m_rootIndex, nullptr);
    Q_ASSERT(m_items.count() == itemCount);

    const int fixedItemCount = getFixedItems().count();
    Q_EMIT dataChanged(index(fixedItemCount), index(fixedItemCount + itemCount - 1));
}

void ChooserModelBase::rebuild()
{
    setRootPath(g_StudioApp.GetCore()->getProjectFile().getProjectPath());
}
