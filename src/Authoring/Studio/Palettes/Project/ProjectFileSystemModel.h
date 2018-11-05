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
#include "Qt3DSFileTools.h"
#include "Dispatch.h"

#include <QtWidgets/qfilesystemmodel.h>
#include <QtWidgets/qmessagebox.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtQml/qqmlapplicationengine.h>

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
        IsProjectReferencedRole, // Means some other presentation in the project uses this file
        DepthRole,
        ExpandedRole,
        FileIdRole,
        ExtraIconRole
    };

    void setRootPath(const QString &path);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    QString filePath(int row) const;
    bool isRefreshable(int row) const;
    bool isCurrentPresentation(const QString &path) const;
    bool isInitialPresentation(const QString &path) const;
    QString presentationId(const QString &path) const;

    Q3DStudio::DocumentEditorFileType::Enum assetTypeForRow(int row);
    int rowForPath(const QString &path) const;
    void updateRoles(const QVector<int> &roles, int startRow = 0, int endRow = -1);

    Q_INVOKABLE void expand(int row);
    Q_INVOKABLE void collapse(int row);

    Q_INVOKABLE void importUrls(const QList<QUrl> &urls, int row, bool autoSort = true);
    Q_INVOKABLE bool hasValidUrlsForDropping(const QList<QUrl> &urls) const;
    Q_INVOKABLE void showInfo(int row);
    Q_INVOKABLE void duplicate(int row);

    void asyncUpdateReferences();
    void onFilesChanged(const Q3DStudio::TFileModificationList &inFileModificationList);

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
    void importUrl(QDir &targetDir, const QUrl &url,
                   QHash<QString, QString> &outPresentationNodes,
                   QStringList &outImportedFiles, int &outOverrideChoice) const;
    void importPresentationAssets(const QFileInfo &uipSrc, const QFileInfo &uipTarget,
                                  QHash<QString, QString> &outPresentationNodes,
                                  QStringList &outImportedFiles,
                                  int &outOverrideChoice) const;

    void modelRowsInserted(const QModelIndex &parent, int start, int end);
    void modelRowsRemoved(const QModelIndex &parent, int start, int end);
    void modelRowsMoved(const QModelIndex &parent, int start, int end);
    void modelLayoutChanged();
    void importQmlAssets(const QObject *qmlNode, const QDir &srcDir, const QDir &targetDir,
                         QStringList &outImportedFiles, int &outOverrideChoice) const;
    void updateDefaultDirMap();
    void addPathsToReferences(QSet<QString> &references, const QString &projectPath,
                              const QString &origPath);
    void handlePresentationIdChange(const QString &path, const QString &id);
    void asyncExpandPresentations();
    void updateReferences();
    bool addUniqueImportFile(const QString &importFile, QStringList &outImportedFiles) const;
    void overridableCopyFile(const QString &srcFile, const QString &targetFile,
                             QStringList &outImportedFiles, int &outOverrideChoice) const;
    void updateProjectReferences();
    void getQmlAssets(const QObject *qmlNode, QSet<QString> &outAssetPaths) const;
    QObject *getQmlStreamRootNode(QQmlApplicationEngine &qmlEngine, const QString &filePath,
                                  bool &outIsQmlStream) const;

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
    QSet<QString> m_references;
    QHash<QString, QString> m_defaultDirToAbsPathMap;

    // Cache of assets referred by other presentation files and qml streams in the project
    // Key: Absolute presentation file path
    // Value: Set of absolute asset file paths referred by the presentation file
    QHash<QString, QSet<QString>> m_presentationReferences;

    // Compilation of all m_presentationReferences sets and their parent paths
    QSet<QString> m_projectReferences;

    // Key: uip that needs update
    // Value: if true, uip was modified or created. If false, it was removed.
    QHash<QString, bool> m_projectReferencesUpdateMap;
    QTimer m_projectReferencesUpdateTimer;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_directoryConnection;
};

#endif // TREEVIEWADAPTOR_H
