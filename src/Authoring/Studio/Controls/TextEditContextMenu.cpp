/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#include "Strings.h"

//==============================================================================
//	Includes
//==============================================================================
#include "TextEditContextMenu.h"
#include "TextEdit.h"
#include "StudioClipboard.h"

//==============================================================================
/**
 * Constructor
 */

CTextEditContextMenu::CTextEditContextMenu(CTextEdit *inEditControl, QWidget *parent) : QMenu(parent)
{
    m_TextEditControl = inEditControl;

    m_cutAction = new QAction(tr("Cut"), this);
    connect(m_cutAction, &QAction::triggered, this, &CTextEditContextMenu::CutText);
    addAction(m_cutAction);

    m_copyAction = new QAction(tr("Copy"), this);
    connect(m_copyAction, &QAction::triggered, this, &CTextEditContextMenu::CopyText);
    addAction(m_copyAction);

    m_pasteAction = new QAction(tr("Paste"), this);
    connect(m_pasteAction, &QAction::triggered, this, &CTextEditContextMenu::PasteText);
    addAction(m_pasteAction);
}

//==============================================================================
/**
 * Destructor
 */

CTextEditContextMenu::~CTextEditContextMenu()
{
}

//==============================================================================
/**
 * returns true if 'cut' can be applied to the control
 */
bool CTextEditContextMenu::CanCutText()
{
    return true;
}

//==============================================================================
/**
 * Returns true if you can copy the text in the control
 */
bool CTextEditContextMenu::CanCopyText()
{
    return true;
}

//==============================================================================
/**
 * Returns true if you may paste text from the clipboard into the control
 */
bool CTextEditContextMenu::CanPasteText()
{
    return CStudioClipboard::CanPasteText();
}

//==============================================================================
/**
 * Calls the controls cuttext function
 */
void CTextEditContextMenu::CutText()
{
    m_TextEditControl->CutText();
}

//==============================================================================
/**
 * Calls the controls copy text function
 */
void CTextEditContextMenu::CopyText()
{
    m_TextEditControl->CopyText();
}

//==============================================================================
/**
 * Calls the controls paste text function
 */
void CTextEditContextMenu::PasteText()
{
    m_TextEditControl->PasteText();
}

void CTextEditContextMenu::showEvent(QShowEvent *event)
{
    m_cutAction->setEnabled(CanCutText());
    m_copyAction->setEnabled(CanCopyText());
    m_pasteAction->setEnabled(CanPasteText());

    QMenu::showEvent(event);
}
