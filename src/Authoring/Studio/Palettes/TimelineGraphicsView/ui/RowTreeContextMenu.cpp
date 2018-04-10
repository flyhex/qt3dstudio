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

#include "RowTreeContextMenu.h"
#include "RowTree.h"
#include "StudioClipboard.h"
#include "Bindings/ITimelineItemBinding.h"

RowTreeContextMenu::RowTreeContextMenu(RowTree *inRowTree, QWidget *parent)
    : QMenu(parent)
    , m_RowTree(inRowTree)
    , m_TimelineItemBinding(inRowTree->getBinding())
{
    initialize();
}

RowTreeContextMenu::~RowTreeContextMenu()
{
}

void RowTreeContextMenu::initialize()
{
    m_renameAction = new QAction(tr("Rename Object"), this);
    connect(m_renameAction, &QAction::triggered, this, &RowTreeContextMenu::renameObject);
    addAction(m_renameAction);

    m_duplicateAction = new QAction(tr("Duplicate Object"), this);
    connect(m_duplicateAction, &QAction::triggered,
            this, &RowTreeContextMenu::duplicateObject);
    addAction(m_duplicateAction);

    m_deleteAction = new QAction(tr("Delete Object"), this);
    connect(m_deleteAction, &QAction::triggered, this, &RowTreeContextMenu::deleteObject);
    addAction(m_deleteAction);

    addSeparator();

    m_copyAction = new QAction(tr("Copy"), this);
    connect(m_copyAction, &QAction::triggered, this, &RowTreeContextMenu::copyObject);
    addAction(m_copyAction);

    m_pasteAction = new QAction(tr("Paste"), this);
    connect(m_pasteAction, &QAction::triggered, this, &RowTreeContextMenu::pasteObject);
    addAction(m_pasteAction);

    m_cutAction = new QAction(tr("Cut"), this);
    connect(m_cutAction, &QAction::triggered, this, &RowTreeContextMenu::cutObject);
    addAction(m_cutAction);
    addSeparator();

    m_makeAction = new QAction(tr("Make Component"), this);
    connect(m_makeAction, &QAction::triggered, this, &RowTreeContextMenu::makeComponent);
    addAction(m_makeAction);

    if (canInspectComponent()) {
        m_inspectAction = new QAction(tr("Edit Component"), this);
        connect(m_inspectAction, &QAction::triggered,
                this, &RowTreeContextMenu::inspectComponent);
        addAction(m_inspectAction);
    }

    addSeparator();

    m_copyPathAction = new QAction(tr("Copy Object Path"), this);
    connect(m_copyPathAction, &QAction::triggered,
            this, &RowTreeContextMenu::copyObjectPath);
    addAction(m_copyPathAction);
}

void RowTreeContextMenu::showEvent(QShowEvent *event)
{
    m_renameAction->setEnabled(canRenameObject());
    m_duplicateAction->setEnabled(canDuplicateObject());
    m_deleteAction->setEnabled(canDeleteObject());

    m_cutAction->setEnabled(canCutObject());
    m_copyAction->setEnabled(canCopyObject());
    m_pasteAction->setEnabled(canPasteObject());

    m_makeAction->setEnabled(canMakeComponent());

    QMenu::showEvent(event);
}

bool RowTreeContextMenu::canRenameObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Rename);
}

void RowTreeContextMenu::renameObject()
{
    m_RowTree->selectLabel();
}

bool RowTreeContextMenu::canDuplicateObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Duplicate);
}

void RowTreeContextMenu::duplicateObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Duplicate);
}

bool RowTreeContextMenu::canDeleteObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Delete);
}

void RowTreeContextMenu::deleteObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Delete);
}

bool RowTreeContextMenu::canInspectComponent()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_EditComponent);
}

/**
 * Inspect the State (Component).
 * This will make the component the top level item of the timelineview.
 */
void RowTreeContextMenu::inspectComponent()
{
    m_TimelineItemBinding->OpenAssociatedEditor();
}

/**
 * Checks to see if the object can be wrapped in a component.
 * @return true if the object is allowed to be wrapped in a component.
 */
bool RowTreeContextMenu::canMakeComponent()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_MakeComponent);
}

/**
 * Wraps the specified asset hierarchy under a component.
 */
void RowTreeContextMenu::makeComponent()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_MakeComponent);
}

/**
 * Get the full Scripting path of the object and copy it to the clipboard.
 * This will figure out the proper way to address the object via scripting
 * and put that path into the clipboard.
 */
void RowTreeContextMenu::copyObjectPath()
{
    CStudioClipboard::CopyTextToClipboard(
                m_TimelineItemBinding->GetObjectPath().toQString());
}

bool RowTreeContextMenu::canCopyObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Copy);
}

void RowTreeContextMenu::copyObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Copy);
}

bool RowTreeContextMenu::canCutObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Cut);
}

void RowTreeContextMenu::cutObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Cut);
}

bool RowTreeContextMenu::canPasteObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Paste);
}

void RowTreeContextMenu::pasteObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Paste);
}
