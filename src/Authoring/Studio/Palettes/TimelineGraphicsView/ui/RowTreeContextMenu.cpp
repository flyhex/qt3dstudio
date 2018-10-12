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
#include "StudioApp.h"
#include "Doc.h"
#include "Core.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "ChooseImagePropertyDlg.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"

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
    // add sub-presentations submenu
    if (m_RowTree->rowType() & (OBJTYPE_LAYER | OBJTYPE_MATERIAL | OBJTYPE_IMAGE)) {
        m_subpMenu = addMenu(tr("Set sub-presentation"));
        connect(m_subpMenu, &QMenu::triggered, this, &RowTreeContextMenu::addSubPresentation);

        m_subpMenu->addAction(tr("[None]"));
        for (auto sp : qAsConst(g_StudioApp.m_subpresentations))
            m_subpMenu->addAction(sp.m_id);

        addSeparator();
    }

    m_renameAction = new QAction(tr("Rename Object"), this);
    connect(m_renameAction, &QAction::triggered, this, &RowTreeContextMenu::renameObject);
    addAction(m_renameAction);

    m_duplicateAction = new QAction(tr("Duplicate Object"), this);
    m_duplicateAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_D));
    m_duplicateAction->setShortcutVisibleInContextMenu(true);
    connect(m_duplicateAction, &QAction::triggered,
            this, &RowTreeContextMenu::duplicateObject);
    addAction(m_duplicateAction);

    m_deleteAction = new QAction(tr("Delete Object"), this);
    m_deleteAction->setShortcut(Qt::Key_Delete);
    m_deleteAction->setShortcutVisibleInContextMenu(true);
    connect(m_deleteAction, &QAction::triggered, this, &RowTreeContextMenu::deleteObject);
    addAction(m_deleteAction);

    m_groupAction = new QAction(tr("Group Objects"), this);
    m_groupAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_G));
    m_groupAction->setShortcutVisibleInContextMenu(true);
    connect(m_groupAction, &QAction::triggered, this, &RowTreeContextMenu::groupObjects);
    addAction(m_groupAction);

    addSeparator();

    m_addLayerAction = new QAction(tr("Add Layer"), this);
    m_addLayerAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_L));
    m_addLayerAction->setShortcutVisibleInContextMenu(true);
    connect(m_addLayerAction, &QAction::triggered, this, &RowTreeContextMenu::addLayer);
    addAction(m_addLayerAction);

    addSeparator();

    m_copyAction = new QAction(tr("Copy"), this);
    m_copyAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_C));
    m_copyAction->setShortcutVisibleInContextMenu(true);
    connect(m_copyAction, &QAction::triggered, this, &RowTreeContextMenu::copyObject);
    addAction(m_copyAction);

    m_pasteAction = new QAction(tr("Paste"), this);
    m_pasteAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_V));
    m_pasteAction->setShortcutVisibleInContextMenu(true);
    connect(m_pasteAction, &QAction::triggered, this, &RowTreeContextMenu::pasteObject);
    addAction(m_pasteAction);

    m_cutAction = new QAction(tr("Cut"), this);
    m_cutAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_X));
    m_cutAction->setShortcutVisibleInContextMenu(true);
    connect(m_cutAction, &QAction::triggered, this, &RowTreeContextMenu::cutObject);
    addAction(m_cutAction);
    addSeparator();

    m_makeAction = new QAction(tr("Make Component"), this);
    m_makeAction->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_G));
    m_makeAction->setShortcutVisibleInContextMenu(true);
    connect(m_makeAction, &QAction::triggered, this, &RowTreeContextMenu::makeComponent);
    addAction(m_makeAction);

    if (canInspectComponent()) {
        m_inspectAction = new QAction(tr("Edit Component"), this);
        m_inspectAction->setShortcut(QKeySequence(
                                      Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_G));
        m_inspectAction->setShortcutVisibleInContextMenu(true);
        connect(m_inspectAction, &QAction::triggered, this, &RowTreeContextMenu::inspectComponent);
        addAction(m_inspectAction);
    }

    addSeparator();

    m_copyPathAction = new QAction(tr("Copy Object Path"), this);
    m_copyPathAction->setShortcut(QKeySequence(
                                      Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C));
    m_copyPathAction->setShortcutVisibleInContextMenu(true);
    connect(m_copyPathAction, &QAction::triggered, this, &RowTreeContextMenu::copyObjectPath);
    addAction(m_copyPathAction);
}

void RowTreeContextMenu::showEvent(QShowEvent *event)
{
    if (m_subpMenu)
        m_subpMenu->setEnabled(canAddSubPresentation());
    m_renameAction->setEnabled(canRenameObject());
    m_duplicateAction->setEnabled(canDuplicateObject());
    m_deleteAction->setEnabled(canDeleteObject());
    m_canGroupObjects = canGroupObjects();
    m_canUngroupObjects = canUngroupObjects();
    m_groupAction->setEnabled(m_canUngroupObjects || m_canGroupObjects);
    if (m_canUngroupObjects)
        m_groupAction->setText(tr("Ungroup Objects"));

    m_cutAction->setEnabled(canCutObject());
    m_copyAction->setEnabled(canCopyObject());
    m_pasteAction->setEnabled(canPasteObject());

    m_makeAction->setEnabled(canMakeComponent());

    m_addLayerAction->setEnabled(canAddLayer());

    QMenu::showEvent(event);
}

bool RowTreeContextMenu::canAddSubPresentation() const
{
    return !g_StudioApp.m_subpresentations.empty();
}

bool RowTreeContextMenu::canRenameObject() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Rename);
}

void RowTreeContextMenu::addSubPresentation(QAction *action)
{
    CDoc &doc(*g_StudioApp.GetCore()->GetDoc());
    qt3dsdm::Qt3DSDMInstanceHandle instance =
            static_cast<Qt3DSDMTimelineItemBinding *>(m_TimelineItemBinding)->GetInstance();
    Q3DStudio::CString presentationId;
    if (action->text() != tr("[None]"))
        presentationId = Q3DStudio::CString::fromQString(action->text());

    if (m_RowTree->rowType() == OBJTYPE_LAYER) {
        qt3dsdm::Qt3DSDMPropertyHandle propHandle = doc.GetPropertySystem()
                ->GetAggregateInstancePropertyByName(instance, L"sourcepath");
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(doc, tr("Set layer sub-presentation"))
                ->SetInstancePropertyValueAsRenderable(instance, propHandle, presentationId);
    } else if (m_RowTree->rowType() == OBJTYPE_MATERIAL) {
        auto &bridge(*doc.GetStudioSystem()->GetClientDataModelBridge());
        // if this is a ref material, update the material it references

        qt3dsdm::Qt3DSDMInstanceHandle refInstance = 0;
        if (bridge.GetObjectType(instance) == OBJTYPE_REFERENCEDMATERIAL) {
            auto optValue = doc.getSceneEditor()->GetInstancePropertyValue(instance,
                            bridge.GetObjectDefinitions().m_ReferencedMaterial
                            .m_ReferencedMaterial.m_Property);
            if (optValue.hasValue())
                refInstance = bridge.GetInstance(doc.GetSceneInstance(), optValue.getValue());
        }

        ChooseImagePropertyDlg dlg(refInstance ? refInstance : instance, refInstance != 0);
        if (dlg.exec() == QDialog::Accepted) {
            qt3dsdm::Qt3DSDMPropertyHandle propHandle = dlg.getSelectedPropertyHandle();
            if (dlg.detachMaterial()) {
                Q3DStudio::ScopedDocumentEditor editor(Q3DStudio::SCOPED_DOCUMENT_EDITOR(doc,
                                                            tr("Set material sub-presentation")));
                editor->BeginAggregateOperation();
                editor->SetMaterialType(instance, "Standard Material");
                editor->setInstanceImagePropertyValue(instance, propHandle, presentationId);
                editor->EndAggregateOperation();
            } else {
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(doc, tr("Set material sub-presentation"))
                ->setInstanceImagePropertyValue(refInstance ? refInstance : instance, propHandle,
                                                presentationId);
            }
        }
    } else if (m_RowTree->rowType() == OBJTYPE_IMAGE) {
        qt3dsdm::Qt3DSDMPropertyHandle propHandle = doc.GetPropertySystem()
                ->GetAggregateInstancePropertyByName(instance, L"subpresentation");
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(doc, tr("Set image sub-presentation"))
                ->SetInstancePropertyValueAsRenderable(instance, propHandle, presentationId);
    }
}

void RowTreeContextMenu::renameObject()
{
    m_RowTree->selectLabel();
}

bool RowTreeContextMenu::canDuplicateObject() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Duplicate);
}

void RowTreeContextMenu::duplicateObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Duplicate);
}

bool RowTreeContextMenu::canDeleteObject() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Delete);
}

bool RowTreeContextMenu::canGroupObjects() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Group);
}

bool RowTreeContextMenu::canUngroupObjects() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Ungroup);
}

void RowTreeContextMenu::deleteObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Delete);
}

void RowTreeContextMenu::groupObjects()
{
    if (m_canGroupObjects)
        m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Group);
    else if (m_canUngroupObjects)
        m_TimelineItemBinding->PerformTransaction(ITimelineItemBinding::EUserTransaction_Ungroup);
}

bool RowTreeContextMenu::canInspectComponent() const
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
bool RowTreeContextMenu::canMakeComponent() const
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

bool RowTreeContextMenu::canCopyObject() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Copy);
}

void RowTreeContextMenu::copyObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Copy);
}

bool RowTreeContextMenu::canCutObject() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Cut);
}

void RowTreeContextMenu::cutObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Cut);
}

bool RowTreeContextMenu::canAddLayer() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_AddLayer);
}
void RowTreeContextMenu::addLayer()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_AddLayer);
}

bool RowTreeContextMenu::canPasteObject() const
{
    return m_TimelineItemBinding->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_Paste);
}

void RowTreeContextMenu::pasteObject()
{
    m_TimelineItemBinding->PerformTransaction(
                ITimelineItemBinding::EUserTransaction_Paste);
}
