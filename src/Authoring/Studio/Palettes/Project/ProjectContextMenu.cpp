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

#include "ProjectContextMenu.h"
#include "ProjectView.h"

ProjectContextMenu::ProjectContextMenu(ProjectView *parent, int index)
    : QMenu(parent)
    , m_view(parent)
    , m_index(index)
{
    QAction *action = nullptr;
    if (m_view->isPresentation(m_index)) {
        const bool currentPresentation = m_view->isCurrentPresentation(m_index);

        action = new QAction(tr("Open Presentation"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleOpenPresentation);
        action->setEnabled(!currentPresentation);
        addAction(action);

        action = new QAction(tr("Rename Presentation"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleRenamePresentation);
        addAction(action);

        action = new QAction(tr("Edit Presentation Id"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleEditPresentationId);
        addAction(action);

        static const QIcon iconInitial = QIcon(QStringLiteral(":/images/initial_notUsed.png"));

        if (m_view->isInitialPresentation(m_index)) {
            action = new QAction(iconInitial, tr("Initial Presentation"));
            // This action does nothing, it's merely informative, so let's disable it
            action->setEnabled(false);
        } else {
            action = new QAction(tr("Set as Initial Presentation"));
            if (m_view->presentationId(m_index).isEmpty()) {
                action->setEnabled(false);
            } else {
                connect(action, &QAction::triggered,
                        this, &ProjectContextMenu::handleInitialPresentation);
            }
        }
        addAction(action);

        addSeparator();
    } else if (m_view->isQmlStream(m_index)) {
        action = new QAction(tr("Rename Qml Stream"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleRenameQmlStream);
        addAction(action);

        action = new QAction(tr("Edit Qml Stream Id"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleEditQmlStreamId);
        addAction(action);

        addSeparator();
    }

    if (m_view->isMaterialData(m_index)) {
        action = new QAction(tr("Edit Material"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleEditMaterial);
        addAction(action);

        action = new QAction(tr("Duplicate"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleDuplicate);
        addAction(action);

        addSeparator();
    }

    action = new QAction(tr("Show Containing Folder"));
    connect(action, &QAction::triggered, this, &ProjectContextMenu::handleShowContainingFolder);
    addAction(action);

    addSeparator();

    action = new QAction(tr("Copy Path"));
    connect(action, &QAction::triggered, this, &ProjectContextMenu::handleCopyPath);
    addAction(action);

    action = new QAction(tr("Copy Full Path"));
    connect(action, &QAction::triggered, this, &ProjectContextMenu::handleCopyFullPath);
    addAction(action);

    addSeparator();

    action = new QAction(tr("Import Assets..."));
    connect(action, &QAction::triggered, this, &ProjectContextMenu::handleImportAssets);
    addAction(action);

    if (m_view->isMaterialFolder(m_index)) {
        addSeparator();
        action = new QAction(tr("Add Material"));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleAddMaterial);
        addAction(action);
    }

    if (m_view->isRefreshable(m_index)) {
        addSeparator();
        action = new QAction(tr("Refresh Import..."));
        connect(action, &QAction::triggered, this, &ProjectContextMenu::handleRefreshImport);
        addAction(action);
    }
}

ProjectContextMenu::~ProjectContextMenu()
{
}

void ProjectContextMenu::handleOpenPresentation()
{
    m_view->openPresentation(m_index);
}

void ProjectContextMenu::handleEditPresentationId()
{
    m_view->editPresentationId(m_index, false);
}

void ProjectContextMenu::handleEditQmlStreamId()
{
    m_view->editPresentationId(m_index, true);
}

void ProjectContextMenu::handleShowContainingFolder()
{
    m_view->showContainingFolder(m_index);
}

void ProjectContextMenu::handleCopyPath()
{
    m_view->copyPath(m_index);
}

void ProjectContextMenu::handleCopyFullPath()
{
    m_view->copyFullPath(m_index);
}

void ProjectContextMenu::handleRefreshImport()
{
    m_view->refreshImport(m_index);
}

void ProjectContextMenu::handleImportAssets()
{
    m_view->assetImportInContext(m_index);
}

void ProjectContextMenu::handleAddMaterial()
{
    m_view->addMaterial(m_index);
}

void ProjectContextMenu::handleEditMaterial()
{
    m_view->editMaterial(m_index);
}

void ProjectContextMenu::handleDuplicate()
{
    m_view->duplicate(m_index);
}

void ProjectContextMenu::handleInitialPresentation()
{
    m_view->setInitialPresentation(m_index);
}

void ProjectContextMenu::handleRenamePresentation()
{
    m_view->renamePresentation(m_index, false);
}

void ProjectContextMenu::handleRenameQmlStream()
{
    m_view->renamePresentation(m_index, true);
}
