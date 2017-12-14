/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "ComponentContextMenu.h"
#include "TimelineControl.h"
#include "StudioUtils.h"
#include "StudioClipboard.h"
#include "Dialogs.h"
#include "BaseTimelineTreeControl.h"
#include "Bindings/ITimelineItemBinding.h"
#include "RelativePathTools.h"

CComponentContextMenu::CComponentContextMenu(CBaseTimelineTreeControl *inTreeControl,
                                             ITimelineItemBinding *inTimelineItemBinding,
                                             QWidget *parent)
    : QMenu(parent)
    , m_TreeControl(inTreeControl)
    , m_TimelineItemBinding(inTimelineItemBinding)
{
    Initialize();
}

void CComponentContextMenu::Initialize()
{
    m_renameAction = new QAction(tr("Rename Object"), this);
    connect(m_renameAction, &QAction::triggered, this, &CComponentContextMenu::RenameObject);
    addAction(m_renameAction);

    m_duplicateAction = new QAction(tr("Duplicate Object"), this);
    connect(m_duplicateAction, &QAction::triggered, this, &CComponentContextMenu::DuplicateObject);
    addAction(m_duplicateAction);

    m_deleteAction = new QAction(tr("Delete Object"), this);
    connect(m_deleteAction, &QAction::triggered, this, &CComponentContextMenu::DeleteObject);
    addAction(m_deleteAction);

    addSeparator();

    m_copyAction = new QAction(tr("Copy"), this);
    connect(m_copyAction, &QAction::triggered, this, &CComponentContextMenu::CopyObject);
    addAction(m_copyAction);

    m_pasteAction = new QAction(tr("Paste"), this);
    connect(m_pasteAction, &QAction::triggered, this, &CComponentContextMenu::PasteObject);
    addAction(m_pasteAction);

    m_cutAction = new QAction(tr("Cut"), this);
    connect(m_cutAction, &QAction::triggered, this, &CComponentContextMenu::CutObject);
    addAction(m_cutAction);
    addSeparator();

    m_makeAction = new QAction(tr("Make Component"), this);
    connect(m_makeAction, &QAction::triggered, this, &CComponentContextMenu::MakeComponent);
    addAction(m_makeAction);

    if (CanInspectComponent()) {
        m_inspectAction = new QAction(tr("Edit Component"), this);
        connect(m_inspectAction, &QAction::triggered,
                this, &CComponentContextMenu::InspectComponent);
        addAction(m_inspectAction);
    }

    if (m_TimelineItemBinding->IsExternalizeable()) {
        addSeparator();
        m_externalizeAction = new QAction(tr("Externalize Buffer"), this);
        connect(m_externalizeAction, &QAction::triggered,
                this, &CComponentContextMenu::Externalize);
        addAction(m_externalizeAction);
    } else if (m_TimelineItemBinding->IsInternalizeable()) {
        addSeparator();
        m_internalizeAction = new QAction(tr("Internalize Buffer"), this);
        connect(m_internalizeAction, &QAction::triggered,
                this, &CComponentContextMenu::Internalize);
        addAction(m_internalizeAction);
    }

    addSeparator();

    m_copyPathAction = new QAction(tr("Copy Object Path"), this);
    connect(m_copyPathAction, &QAction::triggered, this, &CComponentContextMenu::CopyObjectPath);
    addAction(m_copyPathAction);
}

void CComponentContextMenu::showEvent(QShowEvent *event)
{
    m_renameAction->setEnabled(CanRenameObject());
    m_duplicateAction->setEnabled(CanDuplicateObject());
    m_deleteAction->setEnabled(CanDeleteObject());

    m_cutAction->setEnabled(CanCutObject());
    m_copyAction->setEnabled(CanCopyObject());
    m_pasteAction->setEnabled(CanPasteObject());

    m_makeAction->setEnabled(CanMakeComponent());

    QMenu::showEvent(event);
}


CComponentContextMenu::~CComponentContextMenu()
{
}

//=============================================================================
/**
 * Checks to see if the object can be renamed.
 * @return true if the object can be renamed.
 */
bool CComponentContextMenu::CanRenameObject()
{
    return m_TimelineItemBinding->IsValidTransaction(ITimelineItemBinding::EUserTransaction_Rename);
}

//=============================================================================
/**
 * Rename the object.
 */
void CComponentContextMenu::RenameObject()
{
    m_TreeControl->DoRename();
}

//=============================================================================
/**
 * Checks to see if the object can be duplicated.
 * @return true if the object can be duplicated.
 */
bool CComponentContextMenu::CanDuplicateObject()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Duplicate);
}

//=============================================================================
/**
 * Duplicate the object.
 */
void CComponentContextMenu::DuplicateObject()
{
    m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Duplicate);
}

//=============================================================================
/**
 * Checks to see if the object can be deleted.
 * @return true if the object can be deleted.
 */
bool CComponentContextMenu::CanDeleteObject()
{
    return m_TimelineItemBinding->IsValidTransaction(ITimelineItemBinding::EUserTransaction_Delete);
}

//=============================================================================
/**
 * Deletes the object from the scene graph.
 */
void CComponentContextMenu::DeleteObject()
{
    m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Delete);
}

//=============================================================================
/**
 * Checks to see if the State is a component and can be inspected.
 * @return true is the state is a component and can be inspected.
 */
bool CComponentContextMenu::CanInspectComponent()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_EditComponent);
}

//=============================================================================
/**
 * Inspect the State (Component).
 * This will make the component the top level item of the timelineview.
 */
void CComponentContextMenu::InspectComponent()
{
    m_TimelineItemBinding->OpenAssociatedEditor();
}

//=============================================================================
/**
 * Checks to see if the object can be wrapped in a component.
 * @return true if the object is allowed to be wrapped in a component.
 */
bool CComponentContextMenu::CanMakeComponent()
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_MakeComponent);
}

//=============================================================================
/**
 *	Wraps the specified asset hierarchy under a component.
 */
void CComponentContextMenu::MakeComponent()
{
    m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_MakeComponent);
}

//=============================================================================
/**
 * Get the full Scripting path of the object and copy it to the clipboard.
 * This will figure out the proper way to address the object via scripting
 * and put that path into the clipboard.
 */
void CComponentContextMenu::CopyObjectPath()
{
    CStudioClipboard::CopyTextToClipboard(m_TimelineItemBinding->GetObjectPath().toQString());
}

//=============================================================================
/**
 * Checks to see if the object can be copied
 * @return true if the object can be copied
 */
bool CComponentContextMenu::CanCopyObject()
{
    return m_TimelineItemBinding->IsValidTransaction(ITimelineItemBinding::EUserTransaction_Copy);
}

//=============================================================================
/**
 * Copy the object.
 */
void CComponentContextMenu::CopyObject()
{
    m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Copy);
}

bool CComponentContextMenu::CanCutObject()
{
    return m_TimelineItemBinding->IsValidTransaction(ITimelineItemBinding::EUserTransaction_Cut);
}

void CComponentContextMenu::CutObject()
{
    m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Cut);
}

//=============================================================================
/**
 * Checks to see if the object can be pasted
 * @return true if the object can be pasted
 */
bool CComponentContextMenu::CanPasteObject()
{
    return m_TimelineItemBinding->IsValidTransaction(ITimelineItemBinding::EUserTransaction_Paste);
}

//=============================================================================
/**
 * Paste the object.
 */
void CComponentContextMenu::PasteObject()
{
    m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Paste);
}

ITimelineItem *CComponentContextMenu::GetTimelineItem() const
{
    return m_TimelineItemBinding->GetTimelineItem();
}

void CComponentContextMenu::Externalize()
{
    m_TimelineItemBinding->Externalize();
}

void CComponentContextMenu::Internalize()
{
    m_TimelineItemBinding->Internalize();
}
