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
#ifndef PROJECTVIEW_H
#define PROJECTVIEW_H

#include "DispatchListeners.h"
#include "Qt3DSFile.h"
#include "EditPresentationIdDlg.h"

#include <QQuickWidget>
#include <QModelIndex>

class ProjectFileSystemModel;
QT_FORWARD_DECLARE_CLASS(QQuickItem)

class ProjectView : public QQuickWidget,
                    public CPresentationChangeListener,
                    public IDataModelListener,
                    public CFileOpenListener


{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *projectModel READ projectModel NOTIFY projectChanged FINAL)

public:
    explicit ProjectView(const QSize &preferredSize, QWidget *parent = nullptr);
    ~ProjectView();

    QSize sizeHint() const override;

    QAbstractItemModel *projectModel() const;

    Q_INVOKABLE void effectAction(int row);
    Q_INVOKABLE void fontAction(int row);
    Q_INVOKABLE void imageAction(int row);
    Q_INVOKABLE void materialAction(int row);
    Q_INVOKABLE void modelAction(int row);
    Q_INVOKABLE void presentationAction(int row);
    Q_INVOKABLE void behaviorAction(int row);
    Q_INVOKABLE void assetImportAction(int row);
    void assetImportInContext(int row);

    Q_INVOKABLE void startDrag(QQuickItem *item, int row);

    Q_INVOKABLE void showContainingFolder(int row) const;
    Q_INVOKABLE void copyPath(int row) const;
    Q_INVOKABLE void copyFullPath(int row) const;
    Q_INVOKABLE void refreshImport(int row) const;

    Q_INVOKABLE bool isGroup(int row) const;
    Q_INVOKABLE bool isRefreshable(int row) const;
    Q_INVOKABLE void showContextMenu(int x, int y, int index);
    Q_INVOKABLE bool toolTipsEnabled();
    Q_INVOKABLE void openFile(int row);

    bool isPresentation(int row) const;
    bool isQmlStream(int row) const;
    bool isCurrentPresentation(int row) const;
    void openPresentation(int row);
    void editPresentationId(int row);

    // CPresentationChangeListener
    void OnNewPresentation() override;
    // CFileOpenListener
    void OnOpenDocument(const Qt3DSFile &inFilename, bool inSucceeded) override;
    void OnSaveDocument(const Qt3DSFile &inFilename, bool inSucceeded, bool inSaveCopy) override;
    void OnDocumentPathChanged(const Qt3DSFile &inNewPath) override;
    // IDataModelListener
    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    // These are used during drag operations or during operations which
    // require immediate user feedback.  So they are unimplemented, effectively,
    // we ignore them.
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                                    long inInstanceCount) override;

Q_SIGNALS:
    void projectChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void initialize();
    void rebuild();

    ProjectFileSystemModel *m_ProjectModel = nullptr;
    QColor m_BaseColor = QColor::fromRgb(75, 75, 75);
    QString m_defaultBehaviorDir;
    QString m_defaultEffectDir;
    QString m_defaultFontDir;
    QString m_defaultImageDir;
    QString m_defaultMaterialDir;
    QString m_defaultModelDir;
    QString m_defaultPresentationDir;
    QString m_defaultQmlStreamDir;
    QString m_BehaviorDir;
    QString m_EffectDir;
    QString m_FontDir;
    QString m_ImageDir;
    QString m_MaterialDir;
    QString m_ModelDir;
    QString m_presentationDir;
    QString m_qmlStreamDir;
    QString m_assetImportDir;
    QSize m_preferredSize;
};

#endif // PROJECTVIEW_H
