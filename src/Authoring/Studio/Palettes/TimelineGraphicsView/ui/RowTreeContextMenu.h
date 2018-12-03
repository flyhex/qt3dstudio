/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef ROWTREECONTEXTMENU_H
#define ROWTREECONTEXTMENU_H

#include <QtWidgets/qmenu.h>
#include <QtWidgets/qaction.h>

class ITimelineItemBinding;
class RowTree;

class RowTreeContextMenu : public QMenu
{
    Q_OBJECT
public:
    RowTreeContextMenu(RowTree *inRowTree,
                       QWidget *parent = nullptr);
    virtual ~RowTreeContextMenu();

protected:
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:
    void addSubPresentation(QAction *action);
    void renameObject();
    void duplicateObject();
    void deleteObject();
    void groupObjects();
    void inspectComponent();
    void makeComponent();
    void makeAnimatable();
    void copyObject();
    void copyObjectPath();
    void pasteObject();
    void cutObject();
    void addLayer();

private:
    void initialize();

    bool canAddSubPresentation() const;
    bool canRenameObject() const;
    bool canDuplicateObject() const;
    bool canDeleteObject() const;
    bool canGroupObjects() const;
    bool canUngroupObjects() const;
    bool canInspectComponent() const;
    bool canMakeComponent() const;
    bool canMakeAnimatable() const;
    bool canCopyObject() const;
    bool canPasteObject() const;
    bool canCutObject() const;
    bool canAddLayer() const;

    RowTree *m_RowTree;
    ITimelineItemBinding *m_TimelineItemBinding;
    QMenu *m_subpMenu = nullptr; // sub-presentation submenu
    QAction *m_renameAction = nullptr;
    QAction *m_duplicateAction = nullptr;
    QAction *m_deleteAction = nullptr;
    QAction *m_groupAction = nullptr;
    QAction *m_addLayerAction = nullptr;
    QAction *m_inspectAction = nullptr;
    QAction *m_makeAction = nullptr;
    QAction *m_animAction = nullptr;
    QAction *m_copyPathAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_pasteAction = nullptr;
    bool m_canGroupObjects = false;
    bool m_canUngroupObjects = false;
};
#endif // ROWTREECONTEXTMENU_H
