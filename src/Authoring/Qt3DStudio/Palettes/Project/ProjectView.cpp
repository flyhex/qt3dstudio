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
#include "ProjectView.h"
#include "ProjectFileSystemModel.h"
#include "Core.h"
#include "Dispatch.h"
#include "Doc.h"
#include "Literals.h"
#include "StudioUtils.h"
#include "ImportUtils.h"
#include "StudioApp.h"
#include "StudioClipboard.h"
#include "StudioPreferences.h"
#include "Qt3DSImport.h"
#include "Dialogs.h"
#include "IDocumentEditor.h"
#include "ProjectContextMenu.h"
#include "EditPresentationIdDlg.h"

#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtGui/qdrag.h>
#include <QtGui/qdesktopservices.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick/qquickitem.h>

ProjectView::ProjectView(const QSize &preferredSize, QWidget *parent) : QQuickWidget(parent)
  , m_ProjectModel(new ProjectFileSystemModel(this))
  , m_preferredSize(preferredSize)
{
    const QString theApplicationPath = Qt3DSFile::GetApplicationDirectory();

    m_defaultBehaviorDir = theApplicationPath + QStringLiteral("/Content/Behavior Library");
    m_defaultEffectDir = theApplicationPath + QStringLiteral("/Content/Effect Library");
    m_defaultFontDir = theApplicationPath + QStringLiteral("/Content/Font Library");
    m_defaultImageDir = theApplicationPath + QStringLiteral("/Content/Maps Library");
    m_defaultMaterialDir = theApplicationPath + QStringLiteral("/Content/Material Library");
    m_defaultModelDir = theApplicationPath + QStringLiteral("/Content/Models Library");
    m_defaultPresentationDir = theApplicationPath + QStringLiteral("/Content/Presentations");
    m_defaultQmlStreamDir = theApplicationPath + QStringLiteral("/Content/Qml Streams");

    m_BehaviorDir = m_defaultBehaviorDir;
    m_EffectDir = m_defaultEffectDir;
    m_FontDir = m_defaultFontDir;
    m_ImageDir = m_defaultImageDir;
    m_MaterialDir = m_defaultMaterialDir;
    m_ModelDir = m_defaultModelDir;
    m_presentationDir = m_defaultPresentationDir;
    m_qmlStreamDir = m_defaultQmlStreamDir;

    m_assetImportDir = theApplicationPath + QStringLiteral("/Content");

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &ProjectView::initialize);

    auto dispatch = g_StudioApp.GetCore()->GetDispatch();
    dispatch->AddPresentationChangeListener(this);
    dispatch->AddDataModelListener(this);
    dispatch->AddFileOpenListener(this);
}

ProjectView::~ProjectView()
{
}

QAbstractItemModel *ProjectView::projectModel() const
{
    return m_ProjectModel;
}

QSize ProjectView::sizeHint() const
{
    return m_preferredSize;
}

void ProjectView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_resDir"), StudioUtils::resourceImageUrl());
    rootContext()->setContextProperty(QStringLiteral("_parentView"), this);

    engine()->addImportPath(StudioUtils::qmlImportPath());
    setSource(QUrl(QStringLiteral("qrc:/Palettes/Project/ProjectView.qml")));
}

void ProjectView::effectAction(int row)
{
    m_EffectDir = m_defaultEffectDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_EffectDir, Q3DStudio::DocumentEditorFileType::Effect);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::fontAction(int row)
{
    m_FontDir = m_defaultFontDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_FontDir, Q3DStudio::DocumentEditorFileType::Font);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::imageAction(int row)
{
    m_ImageDir = m_defaultImageDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_ImageDir, Q3DStudio::DocumentEditorFileType::Image);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::materialAction(int row)
{
    m_MaterialDir = m_defaultMaterialDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_MaterialDir, Q3DStudio::DocumentEditorFileType::Material);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::modelAction(int row)
{
    m_ModelDir = m_defaultModelDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_ModelDir, Q3DStudio::DocumentEditorFileType::DAE);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::presentationAction(int row)
{
    m_presentationDir = m_defaultPresentationDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_presentationDir, Q3DStudio::DocumentEditorFileType::Presentation);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::behaviorAction(int row)
{
    m_BehaviorDir = m_defaultBehaviorDir;
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_BehaviorDir, Q3DStudio::DocumentEditorFileType::Behavior);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::assetImportAction(int row)
{
    QList<QUrl> urls = g_StudioApp.GetDialogs()->SelectAssets(
                m_assetImportDir, Q3DStudio::DocumentEditorFileType::Unknown);
    m_ProjectModel->importUrls(urls, row);
}

void ProjectView::assetImportInContext(int row)
{
    // If the context is a default directory, select the correct directory
    Q3DStudio::DocumentEditorFileType::Enum assetType = m_ProjectModel->assetTypeForRow(row);
    QString *assetDir = &m_assetImportDir;
    switch (assetType) {
    case Q3DStudio::DocumentEditorFileType::Effect:
        assetDir = &m_EffectDir;
        break;
    case Q3DStudio::DocumentEditorFileType::Font:
        assetDir = &m_FontDir;
        break;
    case Q3DStudio::DocumentEditorFileType::Image:
        assetDir = &m_ImageDir;
        break;
    case Q3DStudio::DocumentEditorFileType::Material:
        assetDir = &m_MaterialDir;
        break;
    case Q3DStudio::DocumentEditorFileType::DAE:
        assetDir = &m_ModelDir;
        break;
    case Q3DStudio::DocumentEditorFileType::Behavior:
        assetDir = &m_BehaviorDir;
        break;
    case Q3DStudio::DocumentEditorFileType::Presentation:
        assetDir = &m_presentationDir;
        break;
    default:
        break;
    }

    QList<QUrl> urls;
    urls = g_StudioApp.GetDialogs()->SelectAssets(*assetDir, assetType);
    m_ProjectModel->importUrls(urls, row, false);
}

void ProjectView::OnNewPresentation()
{
    rebuild();
}

void ProjectView::OnClosingPresentation()
{
    // Clear project view when closing presentation
    m_ProjectModel->clearModelData();
}

void ProjectView::OnOpenDocument(const QString &inFilename, bool inSucceeded)
{
    Q_UNUSED(inFilename)
    Q_UNUSED(inSucceeded)
}

void ProjectView::OnSaveDocument(const QString &inFilename, bool inSucceeded, bool inSaveCopy)
{
    Q_UNUSED(inFilename)
    Q_UNUSED(inSucceeded)
    Q_UNUSED(inSaveCopy)
    m_ProjectModel->asyncUpdateReferences();
}

void ProjectView::OnDocumentPathChanged(const QString &inNewPath)
{
    Q_UNUSED(inNewPath)
}

void ProjectView::OnBeginDataModelNotifications()
{
}

void ProjectView::OnEndDataModelNotifications()
{
    m_ProjectModel->asyncUpdateReferences();
}

void ProjectView::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    Q_UNUSED(inInstance);
}

void ProjectView::OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                                     long inInstanceCount)
{
    Q_UNUSED(inInstance);
    Q_UNUSED(inInstanceCount);
}

void ProjectView::mousePressEvent(QMouseEvent *event)
{
    g_StudioApp.setLastActiveView(this);
    QQuickWidget::mousePressEvent(event);
}

void ProjectView::keyReleaseEvent(QKeyEvent *event)
{
    QKeyEvent *ke = static_cast<QKeyEvent *>(event);
    if (m_selected >= 0) {
        auto key = ke->key();
        if (key == Qt::Key_Delete)
            deleteFile(m_selected);
        else if (key == Qt::Key_Right)
            m_ProjectModel->expand(m_selected);
        else if (key == Qt::Key_Left && m_ProjectModel->isCollapsable(m_selected))
            m_ProjectModel->collapse(m_selected);
    } else {
        QQuickWidget::keyReleaseEvent(event);
    }
}

void ProjectView::startDrag(QQuickItem *item, int row)
{
    item->grabMouse(); // Grab to make sure we can ungrab after the drag
    const auto index = m_ProjectModel->index(row);

    QDrag drag(this);
    drag.setMimeData(m_ProjectModel->mimeData({index}));
    drag.setPixmap(QPixmap(QQmlFile::urlToLocalFileOrQrc(index.data(Qt::DecorationRole).toUrl())));
    Qt::DropAction action = Qt::CopyAction;
    // prevent DnD the currently open presentation and presentations with empty id
    if (isCurrentPresentation(row) || ((isPresentation(row) || isQmlStream(row))
            && presentationId(row).isEmpty())) {
        action = Qt::IgnoreAction;
    }
    drag.exec(action);

    // Ungrab to trigger mouse release on the originating item
    QTimer::singleShot(0, item, &QQuickItem::ungrabMouse);
}

bool ProjectView::isCurrentPresentation(int row) const
{
    return m_ProjectModel->isCurrentPresentation(m_ProjectModel->filePath(row));
}

void ProjectView::editPresentationId(int row, bool qmlStream)
{
    QString relativePresPath = QDir(g_StudioApp.GetCore()->getProjectFile().getProjectPath())
                               .relativeFilePath(m_ProjectModel->filePath(row));

    EditPresentationIdDlg dlg(relativePresPath,
                              qmlStream ? EditPresentationIdDlg::EditQmlStreamId
                                        : EditPresentationIdDlg::EditPresentationId, this);
    dlg.exec();
}

void ProjectView::showSpecificShaderError(const int row)
{
    m_ProjectModel->showItemError(m_ProjectModel->index(row));
}

void ProjectView::renamePresentation(int row, bool qmlStream)
{
    QString relativePresPath = QDir(g_StudioApp.GetCore()->getProjectFile().getProjectPath())
                               .relativeFilePath(m_ProjectModel->filePath(row));

    EditPresentationIdDlg dlg(relativePresPath,
                              qmlStream ? EditPresentationIdDlg::EditQmlStreamName
                                        : EditPresentationIdDlg::EditPresentationName, this);
    dlg.exec();
}

void ProjectView::showContainingFolder(int row) const
{
    if (row == -1)
        return;
    const auto path = m_ProjectModel->filePath(row);
#if defined(Q_OS_WIN)
    QString param = QStringLiteral("explorer ");
    if (!QFileInfo(path).isDir())
        param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(path).replace(QLatin1String(" "), QLatin1String("\ "));
    QProcess::startDetached(param);
#elif defined(Q_OS_MACOS)
    QProcess::startDetached("/usr/bin/osascript", {"-e",
        QStringLiteral("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(path)});
    QProcess::startDetached("/usr/bin/osascript", {"-e",
        QStringLiteral("tell application \"Finder\" to activate")});
#else
    // we cannot select a file here, because no file browser really supports it...
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
}

void ProjectView::copyPath(int row) const
{
    if (row == -1)
        return;

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const QString path = m_ProjectModel->filePath(row);
    const QString relativePath = doc->GetRelativePathToDoc(path);
    CStudioClipboard::CopyTextToClipboard(relativePath);
}

void ProjectView::copyFullPath(int row) const
{
    if (row == -1)
        return;
    const auto path = m_ProjectModel->filePath(row);
    CStudioClipboard::CopyTextToClipboard(path);
}

bool ProjectView::isPresentation(int row) const
{
    return m_ProjectModel->filePath(row).endsWith(QLatin1String(".uip"));
}

bool ProjectView::isQmlStream(int row) const
{
    return g_StudioApp.isQmlStream(m_ProjectModel->filePath(row));
}

bool ProjectView::isMaterialFolder(int row) const
{
    return m_ProjectModel->filePath(row).endsWith(QLatin1String("/materials"));
}

bool ProjectView::isInMaterialFolder(int row) const
{
    return g_StudioApp.GetCore()->getProjectFile().getRelativeFilePathTo(
                m_ProjectModel->filePath(row)).startsWith(QLatin1String("materials/"));
}

bool ProjectView::isMaterialData(int row) const
{
    return m_ProjectModel->filePath(row).endsWith(QLatin1String(".materialdef"));
}

bool ProjectView::isInitialPresentation(int row) const
{
    return m_ProjectModel->isInitialPresentation(m_ProjectModel->filePath(row));
}

bool ProjectView::isFolder(int row) const
{
    return QFileInfo(m_ProjectModel->filePath(row)).isDir();
}

bool ProjectView::isReferenced(int row) const
{
    const auto index = m_ProjectModel->index(row);
    return index.data(ProjectFileSystemModel::IsReferencedRole).toBool()
            || index.data(ProjectFileSystemModel::IsProjectReferencedRole).toBool();
}

QString ProjectView::presentationId(int row) const
{
    return m_ProjectModel->presentationId(m_ProjectModel->filePath(row));
}

void ProjectView::setInitialPresentation(int row)
{
    QString setId = presentationId(row);

    // If presentation id is empty, it means .uip is not part of the project. It shouldn't be
    // possible to set initial presentation in that case.
    Q_ASSERT(!setId.isEmpty());

    g_StudioApp.GetCore()->getProjectFile().setInitialPresentation(setId);
    m_ProjectModel->updateRoles({Qt::DecorationRole});
}

bool ProjectView::isRefreshable(int row) const
{
    return m_ProjectModel->isRefreshable(row);
}

void ProjectView::showContextMenu(int x, int y, int index)
{
    ProjectContextMenu contextMenu(this, index);
    contextMenu.exec(mapToGlobal({x, y}));
}

bool ProjectView::toolTipsEnabled()
{
    return CStudioPreferences::isTooltipsOn();
}

void ProjectView::openFile(int row)
{
    if (row == -1)
        return;

    QFileInfo fi(m_ProjectModel->filePath(row));
    if (fi.isDir() || isCurrentPresentation(row))
        return;

    QString filePath = QDir::cleanPath(fi.absoluteFilePath());
    QTimer::singleShot(0, [filePath, row, this]() {
        // .uip files should be opened in this studio instance
        if (filePath.endsWith(QLatin1String(".uip"), Qt::CaseInsensitive)) {
            if (g_StudioApp.PerformSavePrompt())
                g_StudioApp.OnLoadDocument(filePath);
        } else if (filePath.endsWith(QLatin1String(".materialdef"), Qt::CaseInsensitive)) {
            editMaterial(row);
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    });
}

void ProjectView::setSelected(int row)
{
    m_selected = row;
}

void ProjectView::refreshImport(int row) const
{
    if (row == -1)
        return;
    using namespace Q3DStudio;
    const auto path = m_ProjectModel->filePath(row);
    qt3dsimp::ImportPtrOrError importPtr = qt3dsimp::Import::Load(path.toStdWString().c_str());
    if (importPtr.m_Value) {
        const auto destDir = importPtr.m_Value->GetDestDir();
        const auto srcFile = importPtr.m_Value->GetSrcFile();
        const QString fullSrcPath(QDir(destDir).filePath(srcFile));
        const QFileInfo oldFile(fullSrcPath);
        const QFileInfo newFile(g_StudioApp.GetDialogs()->ConfirmRefreshModelFile(fullSrcPath));
        if (newFile.exists() && newFile.isFile()) {
            // We don't want to create undo point of "Refresh Import", undoing this sort of
            // thing is supposed to be done in the DCC tool. However, we do want to do the
            // refresh inside a transaction to make sure signaling works correctly.
            SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), QObject::tr("Refresh Import"))
                    ->RefreshImport(oldFile.canonicalFilePath(), newFile.canonicalFilePath(), path);
            // Clear undo/redo stack to ensure stack is not corrupted
            // after refresh (stack may contain additions/deletions with chosen import,
            // in which case undoing/redoing those actions can result in crashes or wrong meshes)
            g_StudioApp.GetCore()->GetCmdStack()->Clear();
            g_StudioApp.GetCore()->GetDoc()->SetModifiedFlag(true);
        }
    }
}

void ProjectView::addMaterial(int row) const
{
    if (row == -1)
        return;

    QString path = m_ProjectModel->filePath(row);
    QFileInfo info(path);
    if (info.isFile())
        path = info.dir().path();
    path += QLatin1String("/Material");
    QString extension = QLatin1String(".materialdef");

    QFile file(path + extension);
    int i = 1;
    while (file.exists())
        file.setFileName(path + QString::number(i++) + extension);

    file.open(QIODevice::WriteOnly);
    file.write("<MaterialData version=\"1.0\">\n</MaterialData>");

    m_ProjectModel->selectFile(file.fileName()); // select the added material
}

// show material properties in the inspector
void ProjectView::editMaterial(int row) const
{
    QString path = m_ProjectModel->filePath(row);
    QFileInfo fi(path);

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    bool isDocModified = doc->isModified();
    { // Scope for the ScopedDocumentEditor
        Q3DStudio::ScopedDocumentEditor sceneEditor(
                    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QString()));
        const auto material = sceneEditor->getOrCreateMaterial(path);
        QString name;
        QMap<QString, QString> values;
        QMap<QString, QMap<QString, QString>> textureValues;
        sceneEditor->getMaterialInfo(fi.absoluteFilePath(), name, values, textureValues);
        sceneEditor->setMaterialValues(fi.absoluteFilePath(), values, textureValues);
        if (material.Valid())
            doc->SelectDataModelObject(material);
    }
    // Several aspects of the editor are not updated correctly if the data core is changed without
    // a transaction. The above scope completes the transaction for creating a new material. Next
    // the added undo has to be popped from the stack and the modified flag has to be restored
    // TODO: Find a way to update the editor fully without a transaction
    doc->SetModifiedFlag(isDocModified);
    g_StudioApp.GetCore()->GetCmdStack()->RemoveLastUndo();
}

void ProjectView::duplicateMaterial(int row)
{
    QString pathSrc = m_ProjectModel->filePath(row);
    QString pathDest = pathSrc;
    QRegExp rgxCopy(QLatin1String("Copy\\d*"));

    do {
        int copyIdx = rgxCopy.lastIndexIn(pathDest);
        if (copyIdx != -1) { // src mat. name ends in Copy (+ a number >= 0)
            QString copy = rgxCopy.cap();
            int n = copy.length();
            if (n > 4) { // name ends with a number, increment it
                QString oldNumber = copy.mid(4);
                QString newNumber = QString::number(oldNumber.toInt() + 1);
                copy.replace(oldNumber, newNumber);
            } else { // name ends with Copy, make it Copy2
                copy += '2';
            }

            pathDest.replace(copyIdx, n, copy);
       } else { // name doesn't end with Copy, add it
            pathDest.replace(QLatin1String(".materialdef"), QLatin1String(" Copy.materialdef"));
       }
   } while (QFile::exists(pathDest));

    m_ProjectModel->selectFile(pathDest); // select the duplicated material
    QFile::copy(pathSrc, pathDest);
}

void ProjectView::duplicatePresentation(int row) const
{
    g_StudioApp.duplicatePresentation(m_ProjectModel->filePath(row));
}

void ProjectView::deleteFile(int row) const
{
    if (isReferenced(row)) {
        // Execution should never get here, as menu option is disabled, but since reference cache
        // updates are asynchronous, it is possible to have situation where menu item is enabled
        // but deletion is no longer valid when selected.
        qWarning() << __FUNCTION__ << "Tried to delete referenced file";
        return;
    }

    const QString &filePath = m_ProjectModel->filePath(row);

    if (isPresentation(row) || isQmlStream(row)) {
        // When deleting renderables, project file assets needs to be updated
        g_StudioApp.GetCore()->getProjectFile().deletePresentationFile(filePath);
    } else {
        QFile file(filePath);
        file.remove();
    }
}

void ProjectView::rebuild()
{
    m_selected = -1;
    m_ProjectModel->setRootPath(g_StudioApp.GetCore()->getProjectFile().getProjectPath());
}
