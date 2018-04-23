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
#include "qtAuthoring-config.h"
#include <QtCore/qset.h>

#include "stdafx.h"
#include "ProjectFileSystemModel.h"
#include "StudioUtils.h"
#include "StudioApp.h"
#include "ClientDataModelBridge.h"
#include "Core.h"
#include "Doc.h"
#include "Qt3DSFileTools.h"
#include "ImportUtils.h"
#include "Dialogs.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSImportTranslation.h"
#include "IDocumentEditor.h"
#include "IDragable.h"

ProjectFileSystemModel::ProjectFileSystemModel(QObject *parent) : QAbstractListModel(parent)
    , m_model(new QFileSystemModel(this))
{
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &ProjectFileSystemModel::modelRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ProjectFileSystemModel::modelRowsRemoved);
    connect(m_model, &QAbstractItemModel::layoutChanged, this, &ProjectFileSystemModel::modelLayoutChanged);
}

QHash<int, QByteArray> ProjectFileSystemModel::roleNames() const
{
    auto modelRoleNames = m_model->roleNames();
    modelRoleNames.insert(IsExpandableRole, "_isExpandable");
    modelRoleNames.insert(IsDraggableRole, "_isDraggable");
    modelRoleNames.insert(IsReferencedRole, "_isReferenced");
    modelRoleNames.insert(DepthRole, "_depth");
    modelRoleNames.insert(ExpandedRole, "_expanded");
    return modelRoleNames;
}

int ProjectFileSystemModel::rowCount(const QModelIndex &) const
{
    return m_items.count();
}

QVariant ProjectFileSystemModel::data(const QModelIndex &index, int role) const
{
    const auto &item = m_items.at(index.row());

    switch (role) {
    case Qt::DecorationRole: {
        QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
        return resourceImageUrl() + getIconName(path);
    }

    case IsExpandableRole: {
        if (item.index == m_rootIndex) {
            return false;
        } else {
            return hasVisibleChildren(item.index);
        }
    }

    case IsDraggableRole:
        return QFileInfo(item.index.data(QFileSystemModel::FilePathRole).toString()).isFile();

    case IsReferencedRole: {
        const QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
        return m_references.contains(path);
    }

    case DepthRole:
        return item.depth;

    case ExpandedRole:
        return item.expanded;

    default:
        return m_model->data(item.index, role);
    }
}

QMimeData *ProjectFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    const auto path = filePath(indexes.first().row()); // can only drag one item
    Qt3DSFile dragFile(Q3DStudio::CString::fromQString(path));
    return CDropSourceFactory::Create(QT3DS_FLAVOR_ASSET_UICFILE,
                                      reinterpret_cast<void *>(&dragFile),
                                      sizeof(dragFile));
}

QString ProjectFileSystemModel::filePath(int row) const
{
    if (row < 0 || row >= m_items.size())
        return QString();
    const auto &item = m_items.at(row);
    return item.index.data(QFileSystemModel::FilePathRole).toString();
}

bool ProjectFileSystemModel::isRefreshable(int row) const
{
    const QString path = filePath(row);
    if (path.endsWith(QStringLiteral(".import")))
        return m_references.contains(path);
    return false;
}

void ProjectFileSystemModel::updateReferences(bool emitDataChanged)
{
    m_references.clear();
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    const auto sourcePathList = bridge->GetSourcePathList();
    const auto fontFileList = bridge->GetFontFileList();
    const auto effectTextureList = bridge->GetDynamicObjectTextureList();

    auto addFileReferences = [this, doc](const Q3DStudio::CString &str) {
        auto path = doc->GetResolvedPathToDoc(str).toQString();
        path = QDir::cleanPath(path);
        m_references.append(path);
        QString rootPath = QDir::cleanPath(doc->GetDocumentDirectory().toQString());
        QString parentPath = QFileInfo(path).path();
        do {
            if (!m_references.contains(parentPath))
                m_references.append(parentPath);
            path = parentPath;
            parentPath = QFileInfo(path).path();
        } while (rootPath != path && parentPath != path);
     };

    std::for_each(sourcePathList.begin(), sourcePathList.end(), addFileReferences);
    std::for_each(fontFileList.begin(), fontFileList.end(), addFileReferences);
    std::for_each(effectTextureList.begin(), effectTextureList.end(), addFileReferences);

    if (emitDataChanged)
        Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0), {IsReferencedRole});
}

Q3DStudio::DocumentEditorFileType::Enum ProjectFileSystemModel::assetTypeForRow(int row)
{
    if (row <= 0 || row >= m_items.size())
        return Q3DStudio::DocumentEditorFileType::Unknown;

    const QString rootPath = m_items[0].index.data(QFileSystemModel::FilePathRole).toString();
    QString path = m_items[row].index.data(QFileSystemModel::FilePathRole).toString();
    QFileInfo fi(path);
    if (!fi.isDir())
        path = fi.absolutePath();
    path = path.mid(rootPath.length() + 1);
    const int slash = path.indexOf(QStringLiteral("/"));
    if (slash >= 0)
        path = path.left(slash);
    if (path == QStringLiteral("effects"))
        return Q3DStudio::DocumentEditorFileType::Effect;
    else if (path == QStringLiteral("fonts"))
        return Q3DStudio::DocumentEditorFileType::Font;
    else if (path == QStringLiteral("maps"))
        return Q3DStudio::DocumentEditorFileType::Image;
    else if (path == QStringLiteral("materials"))
        return Q3DStudio::DocumentEditorFileType::Material;
    else if (path == QStringLiteral("models"))
        return Q3DStudio::DocumentEditorFileType::DAE;
    else if (path == QStringLiteral("scripts"))
        return Q3DStudio::DocumentEditorFileType::Behavior;

    return Q3DStudio::DocumentEditorFileType::Unknown;
}

void ProjectFileSystemModel::setRootPath(const QString &path)
{
    setRootIndex(m_model->setRootPath(path));
}

void ProjectFileSystemModel::setRootIndex(const QModelIndex &rootIndex)
{
    if (rootIndex == m_rootIndex)
        return;

    clearModelData();
    updateReferences(false);

    m_rootIndex = rootIndex;

    beginInsertRows({}, 0, 0);
    m_items.append({ m_rootIndex, 0, true, nullptr, 0 });
    endInsertRows();

    showModelTopLevelItems();
}

void ProjectFileSystemModel::clearModelData()
{
    beginResetModel();
    m_defaultDirToAbsPathMap.clear();
    m_items.clear();
    endResetModel();
}

void ProjectFileSystemModel::showModelTopLevelItems()
{
    int rowCount = m_model->rowCount(m_rootIndex);

    if (rowCount == 0) {
        if (m_model->hasChildren(m_rootIndex) && m_model->canFetchMore(m_rootIndex))
            m_model->fetchMore(m_rootIndex);
    } else {
        showModelChildItems(m_rootIndex, 0, rowCount - 1);
    }
}

void ProjectFileSystemModel::showModelChildItems(const QModelIndex &parentIndex, int start, int end)
{
    const int parentRow = modelIndexRow(parentIndex);
    if (parentRow == -1)
        return;

    Q_ASSERT(isVisible(parentIndex));

    QVector<QModelIndex> rowsToInsert;
    for (int i = start; i <= end; ++i) {
        const auto &childIndex = m_model->index(i, 0, parentIndex);
        if (isVisible(childIndex))
            rowsToInsert.append(childIndex);
    }

    const int insertCount = rowsToInsert.count();
    if (insertCount == 0)
        return;

    auto parent = &m_items[parentRow];

    const int depth = parent->depth + 1;
    const int startRow = parentRow + parent->childCount + 1;

    beginInsertRows({}, startRow, startRow + insertCount - 1);

    for (auto it = rowsToInsert.rbegin(); it != rowsToInsert.rend(); ++it)
        m_items.insert(startRow, { *it, depth, false, parent, 0 });

    for (; parent != nullptr; parent = parent->parent)
        parent->childCount += insertCount;

    endInsertRows();

    // also fetch children so we're notified when files are added or removed in immediate subdirs
    for (const auto &childIndex : rowsToInsert) {
        if (m_model->hasChildren(childIndex) && m_model->canFetchMore(childIndex))
            m_model->fetchMore(childIndex);
    }
}

void ProjectFileSystemModel::expand(int row)
{
    Q_ASSERT(row >= 0 && row < m_items.size());

    auto &item = m_items[row];
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

bool ProjectFileSystemModel::hasValidUrlsForDropping(const QList<QUrl> &urls) const
{
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            const QString path = url.toLocalFile();
            const QFileInfo fileInfo(path);
            if (fileInfo.isFile()) {
                const QString extension = fileInfo.suffix();
                return extension.compare(QLatin1String(CDialogs::GetDAEFileExtension()),
                                         Qt::CaseInsensitive) == 0
#ifdef QT_3DSTUDIO_FBX
                       || extension.compare(QLatin1String(CDialogs::GetFbxFileExtension()),
                                            Qt::CaseInsensitive) == 0
#endif
                       || getIconType(path) != OBJTYPE_UNKNOWN;
            }
        }
    }

    return false;
}

void ProjectFileSystemModel::importUrls(const QList<QUrl> &urls, int row, bool autoSort)
{
    if (row < 0 || row >= m_items.size())
        row = 0; // Import to root folder row not specified

    // If importing via buttons or doing in-context import to project root,
    // sort imported items to default folders according to their type
    const bool sortToDefaults = autoSort || row == 0;
    if (sortToDefaults)
        updateDefaultDirMap();

    const TreeItem &item = m_items.at(row);
    QString targetPath = item.index.data(QFileSystemModel::FilePathRole).toString();
    QFileInfo fi(targetPath);
    if (!fi.isDir())
        targetPath = fi.absolutePath();
    const QDir targetDir(targetPath);

    QStringList expandPaths;

    for (const auto &url : urls) {
        QString sortedPath = targetPath;
        QDir sortedDir = targetDir;

        if (sortToDefaults) {
            const QString defaultDir = m_defaultDirToAbsPathMap.value(
                        g_StudioApp.GetDialogs()->defaultDirForUrl(url));
            if (!defaultDir.isEmpty()) {
                sortedPath = defaultDir;
                sortedDir.setPath(sortedPath);
            }
        }

        if (sortedDir.exists()) {
            expandPaths << sortedPath;
            importUrl(sortedPath, url);
        }
    }

    for (const QString &expandPath : qAsConst(expandPaths)) {
        int expandRow = rowForPath(expandPath);
        if (expandRow >= 0 && !m_items[expandRow].expanded)
            expand(expandRow);
    }
}

void ProjectFileSystemModel::importUrl(const QDir &targetDir, const QUrl &url) const
{
    using namespace Q3DStudio;
    using namespace qt3dsimp;
    // Drag and Drop - From Explorer window to Project Palette
    // For all valid Project File Types:
    // - This performs a file copy from the source Explorer location to the selected Project Palette
    // Folder
    // - The destination copy must NOT be read-only even if the source is read-only
    // For DAE, it will import the file.

    if (!url.isLocalFile())
        return;

    const QString sourceFile = url.toLocalFile();

    const QFileInfo fileInfo(sourceFile);
    if (!fileInfo.isFile())
        return;

    const auto doc = g_StudioApp.GetCore()->GetDoc();

    const QString extension = fileInfo.suffix();
    const QString fileStem = fileInfo.baseName();
    const QString outputFileName = QStringLiteral("%1.%2").arg(fileStem).arg(CDialogs::GetImportFileExtension());

    if (extension.compare(QLatin1String(CDialogs::GetDAEFileExtension()), Qt::CaseInsensitive) == 0) {
        SColladaTranslator translator(sourceFile);
        const QDir outputDir = SFileTools::FindUniqueDestDirectory(targetDir, fileStem);
        const QString fullOutputFile = outputDir.filePath(outputFileName);
        const SImportResult importResult =
            CPerformImport::TranslateToImportFile(translator, CFilePath(fullOutputFile));
        bool forceError = QFileInfo(fullOutputFile).isFile() == false;
        IDocumentEditor::DisplayImportErrors(
                sourceFile, importResult.m_Error, doc->GetImportFailedHandler(),
                translator.m_TranslationLog, forceError);
#ifdef QT_3DSTUDIO_FBX
    } else if (extension.compare(QLatin1String(CDialogs::GetFbxFileExtension()), Qt::CaseInsensitive) == 0) {
        SFbxTranslator translator(sourceFile);
        const QDir outputDir = SFileTools::FindUniqueDestDirectory(targetDir, fileStem);
        const QString fullOutputFile = outputDir.filePath(outputFileName);
        const SImportResult importResult =
            CPerformImport::TranslateToImportFile(translator, CFilePath(fullOutputFile));
        bool forceError = QFileInfo(fullOutputFile).isFile() == false;
        IDocumentEditor::DisplayImportErrors(
                sourceFile, importResult.m_Error, doc->GetImportFailedHandler(),
                translator.m_TranslationLog, forceError);
#endif
    } else {
        // Copy the file to target directory
        // FindAndCopyDestFile will make sure the file name is unique and make sure it is
        // not read only.
        bool copyResult = SFileTools::FindAndCopyDestFile(targetDir, sourceFile);
        ASSERT(copyResult);

        // For effect and custom material files, automatically copy related resources
        if (CDialogs::IsEffectFileExtension(extension.toLatin1().data())
                || CDialogs::IsMaterialFileExtension(extension.toLatin1().data())) {
            std::vector<Q3DStudio::CString> effectFileSourcePaths;
            doc->GetDocumentReader().ParseSourcePathsOutOfEffectFile(
                        Q3DStudio::CFilePath::GetAbsolutePath(CFilePath(sourceFile)),
                        effectFileSourcePaths);

            const QDir fileDir = QFileInfo(sourceFile).dir();
            const QDir documentDir(doc->GetDocumentDirectory().toQString());

            for (const auto &effectFile : effectFileSourcePaths) {
                const QString sourcePath = fileDir.filePath(effectFile.toQString());
                const QString resultPath = documentDir.filePath(effectFile.toQString());

                const QFileInfo resultFileInfo(resultPath);
                if (!resultFileInfo.exists()) {
                    resultFileInfo.dir().mkpath(".");
                    QFile::copy(sourcePath, resultPath);
                }
            }
        }
    }
}

int ProjectFileSystemModel::rowForPath(const QString &path) const
{
    for (int i = m_items.size() - 1; i >= 0 ; --i) {
        const QString itemPath = m_items[i].index.data(QFileSystemModel::FilePathRole).toString();
        if (path == itemPath)
            return i;
    }
    return -1;
}

void ProjectFileSystemModel::collapse(int row)
{
    Q_ASSERT(row >= 0 && row < m_items.size());

    auto &item = m_items[row];
    Q_ASSERT(item.expanded == true);

    const int childCount = item.childCount;

    if (childCount > 0) {
        beginRemoveRows({}, row + 1, row + childCount);

        m_items.erase(std::begin(m_items) + row + 1, std::begin(m_items) + row + 1 + childCount);

        for (auto parent = &item; parent != nullptr; parent = parent->parent)
            parent->childCount -= childCount;

        endRemoveRows();
    }

    item.expanded = false;
    Q_EMIT dataChanged(index(row), index(row));
}

int ProjectFileSystemModel::modelIndexRow(const QModelIndex &modelIndex) const
{
    auto it = std::find_if(
                std::begin(m_items),
                std::end(m_items),
                [&modelIndex](const TreeItem &item)
                {
                    return item.index == modelIndex;
                });

    return it != std::end(m_items) ? std::distance(std::begin(m_items), it) : -1;
}

bool ProjectFileSystemModel::isExpanded(const QModelIndex &modelIndex) const
{
    if (modelIndex == m_rootIndex)
        return true;
    const int row = modelIndexRow(modelIndex);
    return row != -1 && m_items.at(row).expanded;
}

EStudioObjectType ProjectFileSystemModel::getIconType(const QString &path) const
{
    Q3DStudio::CFilePath filePath(Q3DStudio::CString::fromQString(path));
    return Q3DStudio::ImportUtils::GetObjectFileTypeForFile(filePath).m_IconType;
}

QString ProjectFileSystemModel::getIconName(const QString &path) const
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

bool ProjectFileSystemModel::hasVisibleChildren(const QModelIndex &modelIndex) const
{
    const QDir dir(modelIndex.data(QFileSystemModel::FilePathRole).toString());
    if (!dir.exists() || dir.isEmpty())
        return false;

    const auto fileInfoList = dir.entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot);
    for (const auto &fileInfo : fileInfoList) {
        if (fileInfo.isDir() || getIconType(fileInfo.filePath()) != OBJTYPE_UNKNOWN)
            return true;
    }

    return false;
}

bool ProjectFileSystemModel::isVisible(const QModelIndex &modelIndex) const
{
    bool result = false;

    if (modelIndex == m_rootIndex) {
        result = true;
    } else {
        QString path = modelIndex.data(QFileSystemModel::FilePathRole).toString();
        QFileInfo fileInfo(path);
        if (fileInfo.isFile()) {
            result = getIconType(path) != OBJTYPE_UNKNOWN;
        } else {
            result = true;
        }
    }

    return result;
}

void ProjectFileSystemModel::modelRowsInserted(const QModelIndex &parent, int start, int end)
{
    if (!m_rootIndex.isValid())
        return;

    if (isExpanded(parent)) {
        showModelChildItems(parent, start, end);
    } else {
        if (hasVisibleChildren(parent)) {
            // show expand arrow
            const int row = modelIndexRow(parent);
            Q_EMIT dataChanged(index(row), index(row));
        }
    }
}

void ProjectFileSystemModel::modelRowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (!m_rootIndex.isValid())
        return;

    if (isExpanded(parent)) {
        for (int i = start; i <= end; ++i) {
            const int row = modelIndexRow(m_model->index(i, 0, parent));

            if (row != -1) {
                const auto &item = m_items.at(row);

                beginRemoveRows({}, row, row + item.childCount);

                for (auto parent = item.parent; parent != nullptr; parent = parent->parent)
                    parent->childCount -= 1 + item.childCount;

                m_items.erase(std::begin(m_items) + row,
                              std::begin(m_items) + row + item.childCount + 1);

                endRemoveRows();
            }
        }
    }

    if (!hasVisibleChildren(parent)) {
        // collapse the now empty folder
        const int row = modelIndexRow(parent);
        if (m_items[row].expanded)
            collapse(row);
        else
            Q_EMIT dataChanged(index(row), index(row));
    }
}

void ProjectFileSystemModel::modelLayoutChanged()
{
    if (!m_rootIndex.isValid())
        return;

    QSet<QPersistentModelIndex> expandedItems;
    for (const auto &item : m_items) {
        if (item.expanded)
            expandedItems.insert(item.index);
    }

    const std::function<int(const QModelIndex &, TreeItem *)> insertChildren = [this, &expandedItems, &insertChildren](const QModelIndex &parentIndex, TreeItem *parent)
    {
        Q_ASSERT(isVisible(parentIndex));

        const int rowCount = m_model->rowCount(parentIndex);
        const int depth = parent->depth + 1;

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

    m_items.erase(std::begin(m_items) + 1, std::end(m_items));
    m_items.reserve(itemCount);
    insertChildren(m_rootIndex, &m_items.first());

    Q_ASSERT(m_items.count() == itemCount);

    Q_EMIT dataChanged(index(0), index(itemCount - 1));
}

void ProjectFileSystemModel::updateDefaultDirMap()
{
    if (m_defaultDirToAbsPathMap.isEmpty()) {
        m_defaultDirToAbsPathMap.insert(QStringLiteral("effects"), QString());
        m_defaultDirToAbsPathMap.insert(QStringLiteral("fonts"), QString());
        m_defaultDirToAbsPathMap.insert(QStringLiteral("maps"), QString());
        m_defaultDirToAbsPathMap.insert(QStringLiteral("materials"), QString());
        m_defaultDirToAbsPathMap.insert(QStringLiteral("models"), QString());
        m_defaultDirToAbsPathMap.insert(QStringLiteral("scripts"), QString());
    }

    const QString rootPath = m_items[0].index.data(QFileSystemModel::FilePathRole).toString();
    const QStringList keys = m_defaultDirToAbsPathMap.keys();
    for (const QString &key : keys) {
        QString currentValue = m_defaultDirToAbsPathMap[key];
        if (currentValue.isEmpty()) {
            const QString defaultPath = rootPath + QStringLiteral("/") + key;
            const QFileInfo fi(defaultPath);
            if (fi.exists() && fi.isDir())
                m_defaultDirToAbsPathMap.insert(key, defaultPath);
        } else {
            const QFileInfo fi(currentValue);
            if (!fi.exists())
                m_defaultDirToAbsPathMap.insert(key, QString());
        }
    }
}
