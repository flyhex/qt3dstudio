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
#ifndef INCLUDED_COMPONENT_CONTEXT_MENU_H
#define INCLUDED_COMPONENT_CONTEXT_MENU_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include <QtWidgets/qmenu.h>

//==============================================================================
//	Forwards
//==============================================================================
class CBaseTimelineTreeControl;
class ITimelineItem;
class ITimelineItemBinding;

class CComponentContextMenu : public QMenu
{
    Q_OBJECT
public:
    CComponentContextMenu(CBaseTimelineTreeControl *inTreeControl,
                          ITimelineItemBinding *inTimelineItemBinding,
                          QWidget *parent = nullptr);
    virtual ~CComponentContextMenu();

protected Q_SLOTS:
    void RenameObject();
    void DuplicateObject();
    void DeleteObject();
    void InspectComponent();
    void MakeComponent();
    void CopyObjectPath();
    void CopyObject();
    void PasteObject();
    void CutObject();
    void Externalize();
    void Internalize();

protected:
    void showEvent(QShowEvent *event) override;

    bool CanRenameObject();
    bool CanDuplicateObject();
    bool CanDeleteObject();
    bool CanInspectComponent();
    bool CanMakeComponent();
    bool CanCopyObject();
    bool CanPasteObject();
    bool CanCutObject();
    void Import();
    void RefreshImport();
    bool CanImport();
    bool CanRefreshImport();
    bool CanExportComponent();

    void Initialize();

    ITimelineItem *GetTimelineItem() const;

    CBaseTimelineTreeControl *m_TreeControl;
    ITimelineItemBinding *m_TimelineItemBinding;
    QAction *m_renameAction;
    QAction *m_duplicateAction;
    QAction *m_deleteAction;
    QAction *m_inspectAction;
    QAction *m_makeAction;
    QAction *m_copyPathAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_externalizeAction;
    QAction *m_internalizeAction;
};
#endif // INCLDUED_STATE_CONTEXT_MENU_H
