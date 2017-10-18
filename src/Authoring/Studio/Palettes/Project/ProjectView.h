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
#include "UICFile.h"

#include <QQuickWidget>
#include <QModelIndex>

class ProjectFileSystemModel;
QT_FORWARD_DECLARE_CLASS(QQuickItem)

class ProjectView : public QQuickWidget,
                    public CPresentationChangeListener,
                    public IDataModelListener

{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *projectModel READ projectModel NOTIFY projectChanged FINAL)

public:
    explicit ProjectView(QWidget *parent = nullptr);
    ~ProjectView();

    QSize sizeHint() const override;

    QAbstractItemModel *projectModel() const;

    Q_INVOKABLE void effectAction();
    Q_INVOKABLE void fontAction();
    Q_INVOKABLE void imageAction();
    Q_INVOKABLE void materialAction();
    Q_INVOKABLE void modelAction();
    Q_INVOKABLE void behaviorAction();

    Q_INVOKABLE void startDrag(QQuickItem *item, int row);

    Q_INVOKABLE void showInExplorer(int row) const;
    Q_INVOKABLE void copyPath(int row) const;
    Q_INVOKABLE void copyFullPath(int row) const;
    Q_INVOKABLE void refreshImport(int row) const;

    Q_INVOKABLE bool isGroup(int row) const;
    Q_INVOKABLE void showContextMenu(int x, int y, int index);

    // CPresentationChangeListener
    void OnNewPresentation() override;
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

private:
    void initialize();
    void rebuild();

    ProjectFileSystemModel *m_ProjectModel = nullptr;
    QColor m_BaseColor = QColor::fromRgb(75, 75, 75);
    CUICFile m_BehaviorDir{""};
    CUICFile m_EffectDir{""};
    CUICFile m_FontDir{""};
    CUICFile m_ImageDir{""};
    CUICFile m_MaterialDir{""};
    CUICFile m_ModelDir{""};
};

#endif // PROJECTVIEW_H
