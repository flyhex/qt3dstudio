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

#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtGui/qdrag.h>
#include <QtGui/qdesktopservices.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>

ProjectView::ProjectView(const QSize &preferredSize, QWidget *parent) : QQuickWidget(parent)
  , m_ProjectModel(new ProjectFileSystemModel(this))
  , m_preferredSize(preferredSize)
{
    const QString theApplicationPath =
            Qt3DSFile::GetApplicationDirectory().GetPath().toQString();

    m_BehaviorDir = Qt3DSFile(
                Q3DStudio::CString::fromQString(theApplicationPath
                                                + QLatin1String("/Content/Behavior Library")));
    m_EffectDir = Qt3DSFile(
                Q3DStudio::CString::fromQString(theApplicationPath
                                                + QLatin1String("/Content/Effect Library")));
    m_FontDir = Qt3DSFile(
                Q3DStudio::CString::fromQString(theApplicationPath
                                                + QLatin1String("/Content/Font Library")));
    m_ImageDir = Qt3DSFile(
                Q3DStudio::CString::fromQString(theApplicationPath
                                                + QLatin1String("/Content/Maps Library")));
    m_MaterialDir = Qt3DSFile(
                Q3DStudio::CString::fromQString(theApplicationPath
                                                + QLatin1String("/Content/Material Library")));
    m_ModelDir = Qt3DSFile(
                Q3DStudio::CString::fromQString(theApplicationPath
                                                + QLatin1String("/Content/Models Library")));

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &ProjectView::initialize);

    auto dispatch = g_StudioApp.GetCore()->GetDispatch();
    dispatch->AddPresentationChangeListener(this);
    dispatch->AddDataModelListener(this);
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
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());
    rootContext()->setContextProperty("_projectView"_L1, this);

    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Project/ProjectView.qml"_L1));
}

void ProjectView::effectAction()
{
    m_EffectDir.Execute();
}

void ProjectView::fontAction()
{
    m_FontDir.Execute();
}

void ProjectView::imageAction()
{
    m_ImageDir.Execute();
}

void ProjectView::materialAction()
{
    m_MaterialDir.Execute();
}

void ProjectView::modelAction()
{
    m_ModelDir.Execute();
}

void ProjectView::behaviorAction()
{
    m_BehaviorDir.Execute();
}

void ProjectView::OnNewPresentation()
{
    rebuild();
}

void ProjectView::OnBeginDataModelNotifications()
{
}

void ProjectView::OnEndDataModelNotifications()
{
    m_ProjectModel->updateReferences(true);
}

void ProjectView::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    Q_UNUSED(inInstance);
}

void ProjectView::OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    Q_UNUSED(inInstance);
    Q_UNUSED(inInstanceCount);
}

void ProjectView::startDrag(QQuickItem *item, int row)
{
    const auto index = m_ProjectModel->index(row);

    QDrag drag(this);
    drag.setMimeData(m_ProjectModel->mimeData({index}));
    drag.exec(Qt::CopyAction);
    QTimer::singleShot(0, item, &QQuickItem::ungrabMouse);
}

void ProjectView::showInExplorer(int row) const
{
    if (row == -1)
        return;
    const auto path = m_ProjectModel->filePath(row);
#if defined(Q_OS_WIN)
    QString param = QStringLiteral("explorer ");
    if (!QFileInfo(path).isDir())
        param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(path).replace(QStringLiteral(" "), QStringLiteral("\ "));
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
    const auto path = m_ProjectModel->filePath(row);
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto relativePath = doc->GetRelativePathToDoc(
                Q3DStudio::CFilePath(Q3DStudio::CString::fromQString(path)));
    CStudioClipboard::CopyTextToClipboard(relativePath.toQString());
}

void ProjectView::copyFullPath(int row) const
{
    if (row == -1)
        return;
    const auto path = m_ProjectModel->filePath(row);
    CStudioClipboard::CopyTextToClipboard(path);
}

bool ProjectView::isGroup(int row) const
{
    if (row == -1)
        return false;
    Q3DStudio::CFilePath path(Q3DStudio::CString::fromQString(m_ProjectModel->filePath(row)));
    return Q3DStudio::ImportUtils::GetObjectFileTypeForFile(path).m_ObjectType == OBJTYPE_GROUP;
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
        if (newFile.exists() && newFile.isFile()){
            const auto doc = g_StudioApp.GetCore()->GetDoc();
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, tr("Refresh Import..."))
                    ->RefreshImport(oldFile.canonicalFilePath(), newFile.canonicalFilePath());
        }
    }
}

void ProjectView::rebuild()
{
    const auto theDoc = g_StudioApp.GetCore()->GetDoc();
    const Q3DStudio::CFilePath thePath(theDoc->GetDocumentPath().GetAbsolutePath());
    const Q3DStudio::CFilePath theRootDirPath = thePath.GetDirectory();

    m_ProjectModel->setRootPath(theRootDirPath.toQString());
}
