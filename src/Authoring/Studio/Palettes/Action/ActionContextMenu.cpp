/****************************************************************************
**
** Copyright (C) 2005 NVIDIA Corporation.
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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "ActionContextMenu.h"
#include "StudioClipboard.h"

//==============================================================================
/**
 *	Constructor
 *
 *	@param	inActionControl	Owner Action Control (commonly known as the Action Palette)
 *that will handle all cut/copy/paste/delete of Actions.
 */
CActionContextMenu::CActionContextMenu(QWidget *parent, bool hasActiveAction)
    : QMenu(parent)
{

    QAction *action = new QAction(tr("Copy Action"));
    action->setEnabled(hasActiveAction);
    connect(action, &QAction::triggered, this, &CActionContextMenu::copyAction);
    addAction(action);

    action = new QAction(tr("Paste Action"));
    action->setEnabled(CStudioClipboard::CanPasteAction());
    connect(action, &QAction::triggered, this, &CActionContextMenu::pasteAction);
    addAction(action);

    action = new QAction(tr("Cut Action"));
    action->setEnabled(hasActiveAction);
    connect(action, &QAction::triggered, this, &CActionContextMenu::cutAction);
    addAction(action);

    action = new QAction(tr("Delete Action"));
    action->setEnabled(hasActiveAction);
    connect(action, &QAction::triggered, this, &CActionContextMenu::deleteAction);
    addAction(action);
}

//==============================================================================
/**
 * Destructor
 */
CActionContextMenu::~CActionContextMenu()
{
}
