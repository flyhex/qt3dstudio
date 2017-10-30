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

#include "SlideContextMenu.h"
#include "SlideView.h"

SlideContextMenu::SlideContextMenu(SlideView *parent, int row, int rowCount, bool master)
    : QMenu(parent)
    , m_view(parent)
    , m_row(row)
    , m_rowCount(rowCount)
{
    QAction *action = new QAction(tr("New Slide"));
    action->setEnabled(!master);
    connect(action, &QAction::triggered, this, &SlideContextMenu::handleAddNewSlide);
    addAction(action);

    action = new QAction(tr("Delete Slide"));
    action->setEnabled(!master && m_row != -1 && m_rowCount > 1);
    connect(action, &QAction::triggered, this, &SlideContextMenu::handleRemoveSlide);
    addAction(action);

    action = new QAction(tr("Duplicate Slide"));
    action->setEnabled(!master && m_row != -1);
    connect(action, &QAction::triggered, this, &SlideContextMenu::handleDuplicateSlide);
    addAction(action);
}

SlideContextMenu::~SlideContextMenu()
{
}

void SlideContextMenu::handleAddNewSlide()
{
    m_view->addNewSlide(m_row == -1 ? m_rowCount : m_row + 1);
}

void SlideContextMenu::handleRemoveSlide()
{
    m_view->removeSlide(m_row);
}

void SlideContextMenu::handleDuplicateSlide()
{
    m_view->duplicateSlide(m_row);
}
