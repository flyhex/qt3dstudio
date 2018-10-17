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
#include <QtCore/qtimer.h>

#include "PresentationFile.h"
#include "Qt3DSCommonPrecompile.h"
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
#include "Qt3DSMessageBox.h"
#include "IDocumentEditor.h"
#include "IDragable.h"
#include <QtQml/qqmlapplicationengine.h>
#include "IObjectReferenceHelper.h"

ProjectFileSystemModel::ProjectFileSystemModel(QObject *parent) : QAbstractListModel(parent)
    , m_model(new QFileSystemModel(this))
{
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &ProjectFileSystemModel::modelRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ProjectFileSystemModel::modelRowsRemoved);
    connect(m_model, &QAbstractItemModel::layoutChanged, this, &ProjectFileSystemModel::modelLayoutChanged);
    connect(&g_StudioApp.GetCore()->getProjectFile(), &ProjectFile::presentationIdChanged,
            this, &ProjectFileSystemModel::handlePresentationIdChange);
    connect(&g_StudioApp.GetCore()->getProjectFile(), &ProjectFile::assetNameChanged,
            this, &ProjectFileSystemModel::asyncUpdateReferences);
}

QHash<int, QByteArray> ProjectFileSystemModel::roleNames() const
{
    auto modelRoleNames = m_model->roleNames();
    modelRoleNames.insert(IsExpandableRole, "_isExpandable");
    modelRoleNames.insert(IsDraggableRole, "_isDraggable");
    modelRoleNames.insert(IsReferencedRole, "_isReferenced");
    modelRoleNames.insert(DepthRole, "_depth");
    modelRoleNames.insert(ExpandedRole, "_expanded");
    modelRoleNames.insert(FileIdRole, "_fileId");
    modelRoleNames.insert(ExtraIconRole, "_extraIcon");
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
        return StudioUtils::resourceImageUrl() + getIconName(path);
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

    case FileIdRole: {
        const QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
        EStudioObjectType iconType = getIconType(path);
        if (iconType == OBJTYPE_PRESENTATION || iconType == OBJTYPE_QML_STREAM)
            return presentationId(path);
        else
            return {};
    }

    case ExtraIconRole: {
        const QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
        EStudioObjectType iconType = getIconType(path);
        if (iconType == OBJTYPE_PRESENTATION || iconType == OBJTYPE_QML_STREAM) {
            if (presentationId(path).isEmpty())
                return QStringLiteral("warning.png");
            else
                return {};
        } else {
            return {};
        }
    }

    default:
        return m_model->data(item.index, role);
    }
}

QMimeData *ProjectFileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    const QString path = filePath(indexes.first().row()); // can only drag one item
    return CDropSourceFactory::Create(QT3DS_FLAVOR_ASSET_UICFILE, path);
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
    // Import needs to be refreshable even if it is not referenced, as user may drag just individual
    // meshes into the scene, and not the whole import.
    return path.endsWith(QLatin1String(".import"));
}

void ProjectFileSystemModel::updateReferences()
{
    m_references.clear();
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    const auto sourcePathList = bridge->GetSourcePathList();
    const auto fontFileList = bridge->GetFontFileList();
    const auto effectTextureList = bridge->GetDynamicObjectTextureList();
    auto renderableList = bridge->getRenderableList();
    auto subpresentationRecord = g_StudioApp.m_subpresentations;

    const QDir projectDir(doc->GetCore()->getProjectFile().getProjectPath());
    const QString projectPath = QDir::cleanPath(projectDir.absolutePath());
    const QString projectPathSlash = projectPath + QLatin1Char('/');

    // Add current presentation to renderables list
    renderableList.insert(doc->getPresentationId());
    subpresentationRecord.push_back(
                SubPresentationRecord({}, doc->getPresentationId(),
                                      projectDir.relativeFilePath(doc->GetDocumentPath())));

    auto addReferencesPresentation = [this, doc, &projectPath](const Q3DStudio::CString &str) {
        addPathsToReferences(projectPath, doc->GetResolvedPathToDoc(str).toQString());
    };
    auto addReferencesProject = [this, doc, &projectPath](const Q3DStudio::CString &str) {
        addPathsToReferences(
                    projectPath,
                    doc->GetCore()->getProjectFile().getAbsoluteFilePathTo(str.toQString()));
    };
    auto addReferencesRenderable = [this, &projectPath, &projectPathSlash, &subpresentationRecord]
            (const QString &id) {
        for (SubPresentationRecord r : qAsConst(subpresentationRecord)) {
            if (r.m_id == id)
                addPathsToReferences(projectPath, projectPathSlash + r.m_argsOrSrc);
        }
    };

    std::for_each(sourcePathList.begin(), sourcePathList.end(), addReferencesPresentation);
    std::for_each(fontFileList.begin(), fontFileList.end(), addReferencesPresentation);
    std::for_each(effectTextureList.begin(), effectTextureList.end(), addReferencesProject);
    std::for_each(effectTextureList.begin(), effectTextureList.end(), addReferencesPresentation);
    std::for_each(renderableList.begin(), renderableList.end(), addReferencesRenderable);

    m_references.insert(projectPath);

    updateRoles({IsReferencedRole, Qt::DecorationRole});
}

/**
 * Checks if file is already imported and if not, adds it to outImportedFiles
 *
 * @param importFile The new imported file to check
 * @param outImportedFiles List of already imported files
 * @return true if importFile was added
 */
bool ProjectFileSystemModel::addUniqueImportFile(const QString &importFile,
                                                 QStringList &outImportedFiles) const
{
    const QString cleanPath = QFileInfo(importFile).canonicalFilePath();
    if (outImportedFiles.contains(cleanPath)) {
        return false;
    } else {
        outImportedFiles.append(cleanPath);
        return true;
    }
}

/**
 * Copy a file with option to override an existing file or skip the override.
 *
 * @param srcFile The source file to copy.
 * @param targetFile The destination file path.
 * @param outImportedFiles list of absolute source paths of the dependent assets that are imported
 *                         in the same import context.
 * @param outOverrideChoice The copy skip/override choice used in this import context.
 */
void ProjectFileSystemModel::overridableCopyFile(const QString &srcFile, const QString &targetFile,
                                                 QStringList &outImportedFiles,
                                                 int &outOverrideChoice) const
{
    QFileInfo srcFileInfo(srcFile);
    if (srcFileInfo.exists() && addUniqueImportFile(srcFile, outImportedFiles)) {
        QFileInfo targetFileInfo(targetFile);
        if (!targetFileInfo.dir().exists())
            targetFileInfo.dir().mkpath(QStringLiteral("."));

        if (targetFileInfo.exists()) { // asset exists, show override / skip box
            if (outOverrideChoice == QMessageBox::YesToAll) {
                QFile::remove(targetFile);
            } else if (outOverrideChoice == QMessageBox::NoToAll) {
                // QFile::copy() does not override files
            } else {
                QString pathFromRoot = QDir(g_StudioApp.GetCore()->getProjectFile()
                                            .getProjectPath())
                                            .relativeFilePath(targetFile);
                outOverrideChoice = g_StudioApp.GetDialogs()
                        ->displayOverrideAssetBox(pathFromRoot);
                if (outOverrideChoice & (QMessageBox::Yes | QMessageBox::YesToAll))
                    QFile::remove(targetFile);
            }
        }
        QFile::copy(srcFile, targetFile);
    }
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
    const int slash = path.indexOf(QLatin1String("/"));
    if (slash >= 0)
        path = path.left(slash);
    if (path == QLatin1String("effects"))
        return Q3DStudio::DocumentEditorFileType::Effect;
    else if (path == QLatin1String("fonts"))
        return Q3DStudio::DocumentEditorFileType::Font;
    else if (path == QLatin1String("maps"))
        return Q3DStudio::DocumentEditorFileType::Image;
    else if (path == QLatin1String("materials"))
        return Q3DStudio::DocumentEditorFileType::Material;
    else if (path == QLatin1String("models"))
        return Q3DStudio::DocumentEditorFileType::DAE;
    else if (path == QLatin1String("scripts"))
        return Q3DStudio::DocumentEditorFileType::Behavior;
    else if (path == QLatin1String("presentations"))
        return Q3DStudio::DocumentEditorFileType::Presentation;
    else if (path == QLatin1String("qml streams"))
        return Q3DStudio::DocumentEditorFileType::QmlStream;

    return Q3DStudio::DocumentEditorFileType::Unknown;
}

void ProjectFileSystemModel::setRootPath(const QString &path)
{
    setRootIndex(m_model->setRootPath(path));

    // Open the presentations folder by default
    connect(this, &ProjectFileSystemModel::dataChanged,
            this, &ProjectFileSystemModel::asyncExpandPresentations);
}

void ProjectFileSystemModel::setRootIndex(const QModelIndex &rootIndex)
{
    if (rootIndex == m_rootIndex)
        return;

    clearModelData();

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
    if (row < 0 || row > m_items.size() - 1 || m_items[row].expanded)
        return;

    auto &item = m_items[row];
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

void ProjectFileSystemModel::showInfo(int row)
{
    if (row < 0 || row >= m_items.size())
        row = 0;

    const TreeItem &item = m_items.at(row);
    QString path = item.index.data(QFileSystemModel::FilePathRole).toString();

    QFileInfo fi(path);

    if (fi.suffix() == QLatin1String("matdata")) {
        const auto sceneEditor = g_StudioApp.GetCore()->GetDoc()->getSceneEditor();
        const auto material = sceneEditor
                ->getOrCreateMaterial(Q3DStudio::CString::fromQString(fi.completeBaseName()));
        QString name;
        QMap<QString, QString> values;
        QMap<QString, QMap<QString, QString>> textureValues;
        sceneEditor->getMaterialInfo(fi.absoluteFilePath(), name, values, textureValues);
        sceneEditor->setMaterialValues(name, values, textureValues);
        if (material.Valid())
            g_StudioApp.GetCore()->GetDoc()->SelectDataModelObject(material);
    }
}

void ProjectFileSystemModel::duplicate(int row)
{
    if (row < 0 || row >= m_items.size())
        row = 0;

    const TreeItem &item = m_items.at(row);
    QString path = item.index.data(QFileSystemModel::FilePathRole).toString();

    QFileInfo srcFile(path);
    const QString destPathStart = srcFile.dir().absolutePath() + QLatin1Char('/')
            + srcFile.completeBaseName() + QStringLiteral(" Copy");
    const QString destPathEnd = QStringLiteral(".") + srcFile.suffix();
    QString destPath = destPathStart + destPathEnd;

    int i = 0;
    while (QFile::exists(destPath)) {
        i++;
        destPath = destPathStart + QString::number(i) + destPathEnd;
    }

    QFile::copy(path, destPath);
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
    QHash<QString, QString> presentationNodes; // <relative path to presentation, presentation id>
    // List of all files that have been copied by this import. Used to avoid duplicate imports
    // due to some of the imported files also being assets used by other imported files.
    QStringList importedFiles;
    int overrideChoice = QMessageBox::NoButton;

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
            importUrl(sortedDir, url, presentationNodes, importedFiles, overrideChoice);
            expandPaths << sortedDir.path();
        }
    }

    // Batch update all imported presentation nodes
    g_StudioApp.GetCore()->getProjectFile().addPresentationNodes(presentationNodes);

    for (const QString &expandPath : qAsConst(expandPaths)) {
        int expandRow = rowForPath(expandPath);
        if (expandRow >= 0 && !m_items[expandRow].expanded)
            expand(expandRow);
    }
}

/**
 * Imports a single asset and the assets it depends on.
 *
 * @param targetDir Target path where the asset is imported to
 * @param url Source url where the asset is imported from
 * @param outPresentationNodes Map where presentation node information is stored for later
 *                             registration. The key is relative path to presentation. The value
 *                             is presentation id.
 * @param outImportedFiles List of absolute source paths of the dependent assets that are imported
 *                         in the same import context.
 * @param outOverrideChoice The copy skip/override choice used in this import context.
 */
void ProjectFileSystemModel::importUrl(QDir &targetDir, const QUrl &url,
                                       QHash<QString, QString> &outPresentationNodes,
                                       QStringList &outImportedFiles,
                                       int &outOverrideChoice) const
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

    // Skip importing if the file has already been imported
    if (!addUniqueImportFile(sourceFile, outImportedFiles))
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
        QQmlApplicationEngine qmlEngine;
        QObject *qmlRoot = nullptr;
        if (extension == QLatin1String("qml")) {
            qmlEngine.load(sourceFile);
            if (qmlEngine.rootObjects().size() > 0) {
                const char *rootClassName = qmlEngine.rootObjects().at(0)
                                            ->metaObject()->superClass()->className();
                // the assumption here is that any qml that is not a behavior is a qml stream
                if (strcmp(rootClassName, "Q3DStudio::Q3DSQmlBehavior") != 0) { // not a behavior
                    qmlRoot = qmlEngine.rootObjects().at(0);
                    // put the qml in the correct folder
                    if (targetDir.path().endsWith(QLatin1String("/scripts"))) {
                        const QString path(QStringLiteral("../qml streams"));
                        targetDir.mkpath(path); // create the folder if doesn't exist
                        targetDir.cd(path);
                    }
                }
            } else {
                // Invalid qml file, block import
                g_StudioApp.GetDialogs()->DisplayKnownErrorDialog(
                            tr("Failed to parse '%1'\nAborting import.").arg(sourceFile));
                return;
            }
        }
        // Copy the file to target directory
        // FindAndCopyDestFile will make sure the file name is unique and make sure it is
        // not read only.
        QString destPath; // final file path (after copying and renaming)
        bool copyResult = SFileTools::FindAndCopyDestFile(targetDir, sourceFile, destPath);
        Q_ASSERT(copyResult);

        if (CDialogs::isPresentationFileExtension(extension.toLatin1().data())) {
            // Don't override id with empty if it is already added, which can happen when
            // multi-importing both presentation and its subpresentation
            const QString presPath
                    = doc->GetCore()->getProjectFile().getRelativeFilePathTo(destPath);
            if (!outPresentationNodes.contains(presPath))
                outPresentationNodes.insert(presPath, {});
            importPresentationAssets(fileInfo, QFileInfo(destPath), outPresentationNodes,
                                     outImportedFiles, outOverrideChoice);
        } else if (qmlRoot) { // importing a qml stream
            const QString presPath
                    = doc->GetCore()->getProjectFile().getRelativeFilePathTo(destPath);
            if (!outPresentationNodes.contains(presPath))
                outPresentationNodes.insert(presPath, {});
            importQmlAssets(qmlRoot, fileInfo.dir(), targetDir, outImportedFiles,
                            outOverrideChoice);
        }

        // For effect and custom material files, automatically copy related resources
        if (CDialogs::IsEffectFileExtension(extension.toLatin1().data())
                || CDialogs::IsMaterialFileExtension(extension.toLatin1().data())) {
            QHash<QString, QString> effectFileSourcePaths;
            QString absSrcPath = fileInfo.absoluteFilePath();
            QString projectPath
                    = QFileInfo(PresentationFile::findProjectFile(absSrcPath)).absolutePath();
            g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                .ParseSourcePathsOutOfEffectFile(absSrcPath, effectFileSourcePaths, projectPath);

            QHashIterator<QString, QString> pathIter(effectFileSourcePaths);
            while (pathIter.hasNext()) {
                pathIter.next();
                overridableCopyFile(pathIter.value(),
                                    QDir(g_StudioApp.GetCore()->getProjectFile().getProjectPath())
                                    .absoluteFilePath(pathIter.key()),
                                    outImportedFiles, outOverrideChoice);
            }
        }
    }
}

/**
 * Import all assets used in a uip file, this includes materials and effects (and their assets),
 * images, fonts, subpresentations (and their recursive assets), models and scripts. Assets are
 * imported in the same relative structure in the imported-from folder in order not to break the
 * assets paths.
 *
 * @param uipSrc source file path where the uip is imported from
 * @param uipTarget target path where the uip is imported to
 * @param outPresentationNodes map where presentation node information is stored for later
 *                             registration. The key is relative path to presentation. The value
 *                             is presentation id.
 * @param overrideChoice tracks user choice (yes to all / no to all) to maintain the value through
 *                       recursive calls
 * @param outImportedFiles list of absolute source paths of the dependent assets that are imported
 *                         in the same import context.
 * @param outOverrideChoice The copy skip/override choice used in this import context.
 */
void ProjectFileSystemModel::importPresentationAssets(
        const QFileInfo &uipSrc, const QFileInfo &uipTarget,
        QHash<QString, QString> &outPresentationNodes, QStringList &outImportedFiles,
        int &outOverrideChoice) const
{
    QHash<QString, QString> importPathMap;
    QString projPathSrc; // project absolute path for the source uip
    PresentationFile::getSourcePaths(uipSrc, uipTarget, importPathMap, projPathSrc,
                                     outPresentationNodes);

    QHashIterator<QString, QString> pathIter(importPathMap);
    while (pathIter.hasNext()) {
        pathIter.next();
        QString srcAssetPath = pathIter.value();
        const QString path = pathIter.key();
        QString targetAssetPath;
        if (path.startsWith(QLatin1String("./"))) { // path from project root
            if (srcAssetPath.isEmpty())
                srcAssetPath = QDir(projPathSrc).absoluteFilePath(path);
            targetAssetPath = QDir(g_StudioApp.GetCore()->getProjectFile().getProjectPath())
                                .absoluteFilePath(path);
        } else { // relative path
            if (srcAssetPath.isEmpty())
                srcAssetPath = uipSrc.dir().absoluteFilePath(path);
            targetAssetPath = uipTarget.dir().absoluteFilePath(path);
        }

        overridableCopyFile(srcAssetPath, targetAssetPath, outImportedFiles, outOverrideChoice);

        // recursively load any uip asset's assets
        if (path.endsWith(QLatin1String(".uip"))) {
            importPresentationAssets(QFileInfo(srcAssetPath), QFileInfo(targetAssetPath),
                                     outPresentationNodes, outImportedFiles, outOverrideChoice);
        }
    }
}

/**
 * Import all assets specified in "source" properties in a qml file.
 *
 * @param qmlNode The qml node to checkfor assets. Recursively checks all child nodes, too.
 * @param srcDir target dir where the assets are imported to
 * @param outImportedFiles list of absolute source paths of the dependent assets that are imported
 *                         in the same import context.
 * @param outOverrideChoice The copy skip/override choice used in this import context.
 */
void ProjectFileSystemModel::importQmlAssets(const QObject *qmlNode, const QDir &srcDir,
                                             const QDir &targetDir,
                                             QStringList &outImportedFiles,
                                             int &outOverrideChoice) const
{
    QString assetSrc = qmlNode->property("source").toString(); // absolute file path

    if (!assetSrc.isEmpty()) {
        // remove file:///
        if (assetSrc.startsWith(QLatin1String("file:///")))
            assetSrc = assetSrc.mid(8);
        else if (assetSrc.startsWith(QLatin1String("file://")))
            assetSrc = assetSrc.mid(7);

        overridableCopyFile(srcDir.absoluteFilePath(assetSrc),
                            targetDir.absoluteFilePath(srcDir.relativeFilePath(assetSrc)),
                            outImportedFiles, outOverrideChoice);
    }

    // recursively load child nodes
    const QObjectList qmlNodeChildren = qmlNode->children();
    for (int i = 0; i < qmlNodeChildren.count(); ++i) {
        importQmlAssets(qmlNodeChildren.at(i), srcDir, targetDir, outImportedFiles,
                        outOverrideChoice);
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

void ProjectFileSystemModel::updateRoles(const QVector<int> &roles, int startRow, int endRow)
{
    Q_EMIT dataChanged(index(startRow, 0),
                       index(endRow < 0 ? rowCount() - 1 : endRow, 0), roles);
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
    return Q3DStudio::ImportUtils::GetObjectFileTypeForFile(path).m_IconType;
}

QString ProjectFileSystemModel::getIconName(const QString &path) const
{
    QString iconName;

    bool referenced = m_references.contains(path);

    QFileInfo fileInfo(path);
    if (fileInfo.isFile()) {
        EStudioObjectType type = getIconType(path);

        if (type == OBJTYPE_PRESENTATION) {
            const bool isCurrent = isCurrentPresentation(path);
            const bool isInitial = isInitialPresentation(path);
            if (isInitial) {
                iconName = isCurrent ? QStringLiteral("initial_used.png")
                                     : QStringLiteral("initial_notUsed.png");
            } else if (isCurrent) {
                iconName = QStringLiteral("presentation_edit.png");
            }
        }

        if (iconName.isEmpty()) {
            if (type != OBJTYPE_UNKNOWN) {
                iconName = referenced ? CStudioObjectTypes::GetNormalIconName(type)
                                      : CStudioObjectTypes::GetDisabledIconName(type);
            } else {
                iconName = referenced ? QStringLiteral("Objects-Layer-Normal.png")
                                      : QStringLiteral("Objects-Layer-Disabled.png");
            }
        }
    } else {
        iconName = referenced ? QStringLiteral("Objects-Folder-Normal.png")
                              : QStringLiteral("Objects-Folder-Disabled.png");
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
    QString path = modelIndex.data(QFileSystemModel::FilePathRole).toString();

    if (modelIndex == m_rootIndex || QFileInfo(path).isDir())
        return true;

    if (path.endsWith(QLatin1String("_autosave.uip"))
        || path.endsWith(QLatin1String("_@preview@.uip"))
        || path.endsWith(QLatin1String(".uia"))) {
        return false;
    }

    return getIconType(path) != OBJTYPE_UNKNOWN;
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
        m_defaultDirToAbsPathMap.insert(QStringLiteral("presentations"), QString());
        m_defaultDirToAbsPathMap.insert(QStringLiteral("qml streams"), QString());
    }

    const QString rootPath = m_items[0].index.data(QFileSystemModel::FilePathRole).toString();
    const QStringList keys = m_defaultDirToAbsPathMap.keys();
    for (const QString &key : keys) {
        QString currentValue = m_defaultDirToAbsPathMap[key];
        if (currentValue.isEmpty()) {
            const QString defaultPath = rootPath + QLatin1Char('/') + key;
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

void ProjectFileSystemModel::addPathsToReferences(const QString &projectPath,
                                                  const QString &origPath)
{
    m_references.insert(origPath);
    QString path = origPath;
    QString parentPath = QFileInfo(path).path();
    do {
        m_references.insert(path);
        path = parentPath;
        parentPath = QFileInfo(path).path();
    } while (path != projectPath && parentPath != path);
}

void ProjectFileSystemModel::handlePresentationIdChange(const QString &path, const QString &id)
{
    int row = rowForPath(QDir::cleanPath(
                             QDir(g_StudioApp.GetCore()->GetDoc()->GetCore()->getProjectFile()
                                  .getProjectPath()).absoluteFilePath(path)));
    updateRoles({FileIdRole, ExtraIconRole}, row, row);
}

void ProjectFileSystemModel::asyncExpandPresentations()
{
    disconnect(this, &ProjectFileSystemModel::dataChanged,
               this, &ProjectFileSystemModel::asyncExpandPresentations);

    // expand presentation folder by default (if it exists).
    QTimer::singleShot(0, [this]() {
        QString path = g_StudioApp.GetCore()->getProjectFile().getProjectPath()
                       + QStringLiteral("/presentations");
        expand(rowForPath(path));
    });
}

void ProjectFileSystemModel::asyncUpdateReferences()
{
    QTimer::singleShot(0, this, &ProjectFileSystemModel::updateReferences);
}

bool ProjectFileSystemModel::isCurrentPresentation(const QString &path) const
{
    return path == g_StudioApp.GetCore()->GetDoc()->GetDocumentPath();
}

bool ProjectFileSystemModel::isInitialPresentation(const QString &path) const
{
    QString checkId = presentationId(path);

    return !checkId.isEmpty()
            && checkId == g_StudioApp.GetCore()->getProjectFile().initialPresentation();
}

QString ProjectFileSystemModel::presentationId(const QString &path) const
{
    QString presId;
    if (isCurrentPresentation(path))
        presId = g_StudioApp.GetCore()->GetDoc()->getPresentationId();
    else
        presId = g_StudioApp.getRenderableId(QFileInfo(path).absoluteFilePath());

    return presId;
}
