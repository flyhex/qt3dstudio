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

#include "RowManager.h"
#include "RowTree.h"
#include "TimelineGraphicsScene.h"
#include "Ruler.h"
#include "TreeHeader.h"
#include "KeyframeManager.h"
#include "Keyframe.h"
#include "StudioObjectTypes.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"
#include "Bindings/Qt3DSDMTimelineKeyframe.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "StudioObjectTypes.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentEditor.h"

#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

RowManager::RowManager(TimelineGraphicsScene *scene, QGraphicsLinearLayout *layoutLabels,
                       QGraphicsLinearLayout *layoutTimeline)
    : m_scene(scene)
    , m_layoutTree(layoutLabels)
    , m_layoutTimeline(layoutTimeline)
{

}

RowManager::~RowManager()
{
    finalizeRowDeletions();
}

void RowManager::recreateRowsFromBinding(ITimelineItemBinding *rootBinding)
{
    removeAllRows();
    createRowsFromBindingRecursive(rootBinding);
}

void RowManager::removeAllRows()
{
    m_scene->keyframeManager()->deselectAllKeyframes();
    clearSelection();

    // delete rows
    for (int i = m_layoutTree->count() - 1; i >= 1; --i) {
        RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
        m_layoutTree->removeAt(i);
        m_layoutTimeline->removeAt(i);
        delete row_i; // this will also delete the timeline row
    }
}

RowTree *RowManager::createRowFromBinding(ITimelineItemBinding *binding, RowTree *parentRow,
                                          int index)
{
    RowTree *newRow = createRow(binding->GetTimelineItem()->GetObjectType(), parentRow,
                                binding->GetTimelineItem()->GetName().toQString(),
                                QString(), index);

    // connect the new row and its binding
    binding->setRowTree(newRow);
    newRow->setBinding(binding);

    // hide if material container
    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    if (bridge->isMaterialContainer(newRow->instance())) {
        newRow->setVisible(false);
        newRow->rowTimeline()->setVisible(false);
    }

    // set row start/end time & color
    ITimelineTimebar *timebar = binding->GetTimelineItem()->GetTimebar();
    RowTimeline *rowTimeline = newRow->rowTimeline();
    rowTimeline->clearBoundChildren();
    rowTimeline->setStartTime(timebar->GetStartTime());
    rowTimeline->setEndTime(timebar->GetEndTime());
    rowTimeline->setBarColor(timebar->GetTimebarColor());

    // create property rows
    for (int i = 0; i < binding->GetPropertyCount(); i++) {
        ITimelineItemProperty *prop_i = binding->GetProperty(i);
        RowTree *propRow = getOrCreatePropertyRow(newRow, prop_i->GetName().toQString());

        // connect the property row and its binding
        prop_i->setRowTree(propRow);
        propRow->setPropBinding(prop_i);

        // add keyframes
        for (int j = 0; j < prop_i->GetKeyframeCount(); j++) {
            Qt3DSDMTimelineKeyframe *kf
                    = static_cast<Qt3DSDMTimelineKeyframe *>(prop_i->GetKeyframeByIndex(j));

            QList<Keyframe *> addedKeyframes
                    = m_scene->keyframeManager()->insertKeyframe(propRow->rowTimeline(),
                    kf->GetTime(), false);

            Keyframe *kfUI = addedKeyframes.at(0);
            kf->setUI(kfUI);
            kfUI->binding = kf;
            kfUI->dynamic = kf->IsDynamic();
        }
    }

    updateRulerDuration();

    return newRow;
}

void RowManager::createRowsFromBindingRecursive(ITimelineItemBinding *binding, RowTree *parentRow)
{
    auto instance = static_cast<Qt3DSDMTimelineItemBinding *>(binding)->GetInstance();

    RowTree *newRow = createRowFromBinding(binding, parentRow);
    // create child rows recursively
    const QList<ITimelineItemBinding *> children = binding->GetChildren();
    for (auto child : children)
        createRowsFromBindingRecursive(child, newRow);
}

RowTree *RowManager::getOrCreatePropertyRow(RowTree *masterRow, const QString &propType, int index)
{
    RowTree *propertyRow = masterRow->getPropertyRow(propType);
    if (!propertyRow)
        propertyRow = createRow(OBJTYPE_UNKNOWN, masterRow, {}, propType, index);

    propertyRow->updateLock(masterRow->locked());

    return propertyRow;
}

RowTree *RowManager::createRow(EStudioObjectType rowType, RowTree *parentRow, const QString &label,
                               const QString &propType, int index)
{
    if (parentRow && parentRow->isProperty()) {
        qWarning() << __FUNCTION__ << "Property row cannot have children. No row added.";
    } else {
        // If the row doesnt have a parent, insert it under the scene (first row is the tree header)
        if (!parentRow && rowType != OBJTYPE_SCENE && m_layoutTree->count() > 1)
            parentRow = static_cast<RowTree *>(m_layoutTree->itemAt(1));

        RowTree *rowTree = nullptr;

        if (!propType.isEmpty()) // property row
            rowTree = new RowTree(m_scene, propType);
        else
            rowTree = new RowTree(m_scene, rowType, label);

        if (parentRow) {
            if (index != -1)
                parentRow->addChildAt(rowTree, index);
            else
                parentRow->addChild(rowTree);
        } else {
            // root element, no parent
            m_layoutTree->insertItem(1, rowTree);
            m_layoutTimeline->insertItem(1, rowTree->rowTimeline());
        }

        return rowTree;
    }

    return nullptr;
}

RowTree *RowManager::getRowAtPos(const QPointF &scenePos) const
{
    const QList<QGraphicsItem *> items = m_scene->items(scenePos);

    for (auto item : items) {
        if (item->type() == TimelineItem::TypeRowTree)
            return static_cast<RowTree *>(item);
    }

    return nullptr;
}

// Call this to select/unselect row, affecting bindings
void RowManager::selectRow(RowTree *row, bool multiSelect)
{
    if (!row) {
        g_StudioApp.GetCore()->GetDoc()->DeselectAllItems();
        return;
    }

    if (row->locked())
        return;

    if (multiSelect && row->propertyExpanded())
        return;

    if (row->isProperty())
        row = row->parentRow();

    if (multiSelect && m_selectedRows.size() > 0) {
        // Do not allow singular object types into multiselection
        if ((row->objectType() | m_selectedRows[0]->objectType()) & OBJTYPE_IS_SINGULAR)
            return;
    }

    Qt3DSDMTimelineItemBinding *binding
            = static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding());
    if (binding)
        binding->SetSelected(multiSelect);
}

QVector<RowTimelinePropertyGraph *> RowManager::getExpandedPropertyGraphs() const
{
    QVector<RowTimelinePropertyGraph *> graphs;

    // the earliest possible property row index in the layout is 4
    for (int i = 4; i < m_layoutTree->count(); ++i) {
        RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
        if (row_i->propertyExpanded())
            graphs.append(row_i->rowTimeline()->propertyGraph());
    }

    return graphs;
}

// Call this to update row selection UI status
void RowManager::setRowSelection(RowTree *row, bool selected)
{
    if (!row)
        return;

    if (selected) {
        if (!m_selectedRows.contains(row))
            m_selectedRows.append(row);
        row->setState(InteractiveTimelineItem::Selected);
        // Expand parents if not expanded
        QPointer<RowTree> pRow = row->parentRow();
        if (!pRow.isNull()) {
            QTimer::singleShot(0, [this, pRow]() {
                if (!pRow.isNull())
                    ensureRowExpandedAndVisible(pRow, false);
            });
        }
    } else {
        m_selectedRows.removeAll(row);
        row->setState(InteractiveTimelineItem::Normal);
    }
}

// Call this to clear all selections UI status
void RowManager::clearSelection()
{
    for (auto row : qAsConst(m_selectedRows))
        row->setState(InteractiveTimelineItem::Normal);
    m_selectedRows.clear();
}

// Updates duration of ruler
// When you don't want to update max duration (so width of timeline, scrollbar)
// set updateMaxDuration to false.
void RowManager::updateRulerDuration(bool updateMaxDuration)
{
    long duration = 0;
    long maxDuration = 0; // for setting correct size for the view so scrollbars appear correctly
    if (m_layoutTree->count() > 1) {
        auto rootRow = static_cast<RowTree *>(m_layoutTree->itemAt(1)->graphicsItem());
        bool insideComponent = rootRow->objectType() == OBJTYPE_COMPONENT;
        for (int i = 2; i < m_layoutTree->count(); ++i) {
            RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
            long dur_i = row_i->rowTimeline()->getEndTime();

            if ((insideComponent || row_i->objectType() == OBJTYPE_LAYER) && dur_i > duration)
                duration = dur_i;

            if (dur_i > maxDuration)
                maxDuration = dur_i;
        }
        rootRow->rowTimeline()->setEndTime(duration);
    }

    m_scene->ruler()->setDuration(duration);

    if (updateMaxDuration)
        m_scene->ruler()->setMaxDuration(maxDuration);
}

void RowManager::updateFiltering(RowTree *row)
{
    if (!row) // update all rows
        row = static_cast<RowTree *>(m_layoutTree->itemAt(1));
    updateRowFilterRecursive(row);
}

void RowManager::updateRowFilterRecursive(RowTree *row)
{
    row->updateFilter();

    if (!row->empty()) {
        const auto childRows = row->childRows();
        for (auto child : childRows)
            updateRowFilterRecursive(child);
        row->updateArrowVisibility();
    }
}

void RowManager::deleteRow(RowTree *row)
{
   if (row && row->objectType() != OBJTYPE_SCENE) {
       if (row->parentRow())
           row->parentRow()->removeChild(row);

       deleteRowRecursive(row, true);
   }
}

void RowManager::finalizeRowDeletions()
{
    for (auto row : qAsConst(m_deletedRows)) {
        // If the row has been reparented, no need to delete it
        if (!row->parentRow())
            deleteRowRecursive(row, false);
    }
    m_deletedRows.clear();
}

void RowManager::deleteRowRecursive(RowTree *row, bool deferChildRows)
{
    if (!row->childProps().empty()) {
        const auto childProps = row->childProps();
        for (auto child : childProps)
            deleteRowRecursive(child, false);
    }

    if (!row->childRows().empty()) {
        const auto childRows = row->childRows();
        for (auto child : childRows) {
            if (deferChildRows) {
                // Let's not delete child rows just yet, there may be a pending move for them.
                // This happens when the same transaction contains parent deletion and child row
                // move, such as ungrouping items.
                child->setParentRow(nullptr);
                m_deletedRows.append(child);
            } else {
                deleteRowRecursive(child, false);
            }
        }
    }

    m_selectedRows.removeAll(row);
    m_deletedRows.removeAll(row); // Row actually deleted, remove it from pending deletes

    m_scene->keyframeManager()->deleteKeyframes(row->rowTimeline(), false);

    if (row->getBinding())
        static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding())->setRowTree(nullptr);

    delete row;
}

RowTree *RowManager::selectedRow() const
{
    if (m_selectedRows.size() == 1)
        return m_selectedRows.first();
    return nullptr;
}

bool RowManager::isComponentRoot() const
{
    if (m_layoutTree->count() > 1) {
        RowTree *root = static_cast<RowTree *>(m_layoutTree->itemAt(1)->graphicsItem());
        return root->objectType() == OBJTYPE_COMPONENT;
    }
    return false;
}

bool RowManager::isRowSelected(RowTree *row) const
{
    return m_selectedRows.contains(row);
}

QVector<RowTree *> RowManager::selectedRows() const
{
    return m_selectedRows;
}

void RowManager::ensureRowExpandedAndVisible(RowTree *row, bool forceChildUpdate) const
{
    RowTree *parentRow = row;
    while (parentRow) {
        parentRow->updateExpandStatus(parentRow->expandHidden()
                                    ? RowTree::ExpandState::HiddenExpanded
                                    : RowTree::ExpandState::Expanded, false,
                                    forceChildUpdate);
        parentRow = parentRow->parentRow();
    }
}

bool RowManager::isSingleSelected() const
{
    return m_selectedRows.size() == 1;
}
