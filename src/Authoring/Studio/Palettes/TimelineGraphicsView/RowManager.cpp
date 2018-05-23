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
    RowTree *row_i;
    for (int i = m_layoutTree->count() - 1; i >= 1; --i) {
        row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
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

    // set row start/end time & color
    ITimelineTimebar *timebar = binding->GetTimelineItem()->GetTimebar();
    newRow->rowTimeline()->setStartTime(timebar->GetStartTime() * .001);
    newRow->rowTimeline()->setEndTime(timebar->GetEndTime() * .001);
    newRow->rowTimeline()->setBarColor(timebar->GetTimebarColor());

    // create property rows
    for (int i = 0; i < binding->GetPropertyCount(); i++) {
        ITimelineItemProperty *prop_i = binding->GetProperty(i);
        RowTree *propRow = getOrCreatePropertyRow(newRow, prop_i->GetName().toQString());

        // connect the property row and its binding
        prop_i->setRowTree(propRow);
        propRow->setPropBinding(prop_i);

        // add keyframes
        for (int j = 0; j < prop_i->GetKeyframeCount(); j++) {
            Qt3DSDMTimelineKeyframe *kf =
                    static_cast<Qt3DSDMTimelineKeyframe *>(prop_i->GetKeyframeByIndex(j));

            QList<Keyframe *> addedKeyframes =
                    m_scene->keyframeManager()->insertKeyframe(propRow->rowTimeline(),
                    static_cast<double>(kf->GetTime()) * .001, false);

            Keyframe *kfUI = addedKeyframes.at(0);
            kf->setUI(kfUI);
            kfUI->binding = kf;
        }
    }

    updateRulerDuration();

    return newRow;
}

void RowManager::createRowsFromBindingRecursive(ITimelineItemBinding *binding, RowTree *parentRow)
{
    RowTree *newRow = createRowFromBinding(binding, parentRow);
    // create child rows recursively
    const QList<ITimelineItemBinding *> children = binding->GetChildren();
    for (auto child : children)
        createRowsFromBindingRecursive(child, newRow);
}

RowTree *RowManager::getOrCreatePropertyRow(RowTree *masterRow, const QString &propType, int index)
{
    RowTree *propertyRow = masterRow->getPropertyRow(propType);
    if (propertyRow)
        return propertyRow;

    return createRow(OBJTYPE_UNKNOWN, masterRow, 0, propType, index);
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

RowTree *RowManager::rowAt(int idx)
{
    correctIndex(idx);

    if (idx != -1)
        return static_cast<RowTree *>(m_layoutTree->itemAt(idx)->graphicsItem());

    return nullptr;
}

RowTimeline *RowManager::rowTimelineAt(int idx)
{
    correctIndex(idx);

    if (idx != -1)
        return static_cast<RowTimeline *>(m_layoutTimeline->itemAt(idx)->graphicsItem());

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

    if (row->isProperty())
        row = row->parentRow();

    if (multiSelect && m_selectedRows.size() > 0) {
        // Do not allow certain object types into multiselection
        const EStudioObjectType rowType = row->rowType();
        const int singularType = OBJTYPE_SCENE | OBJTYPE_MATERIAL | OBJTYPE_LAYER
                | OBJTYPE_BEHAVIOR | OBJTYPE_EFFECT;
        if (singularType & rowType || singularType & m_selectedRows[0]->rowType())
            return;
    }

    Qt3DSDMTimelineItemBinding *binding =
            static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding());
    if (binding)
        binding->SetSelected(multiSelect);
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
            QTimer::singleShot(0, [pRow]() {
                if (!pRow.isNull()) {
                    RowTree *parentRow = pRow.data();
                    while (parentRow) {
                        parentRow->updateExpandStatus(RowTree::ExpandState::Expanded, false);
                        parentRow = parentRow->parentRow();
                    }
                }
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

void RowManager::updateRulerDuration()
{
    double duration = 0;
    double maxDuration = 0; // for setting correct size for the view so scrollbars appear correctly
    if (m_layoutTree->count() > 1) {
        auto rootRow = static_cast<RowTree *>(m_layoutTree->itemAt(1)->graphicsItem());
        bool isComponent = rootRow->rowType() == OBJTYPE_COMPONENT;
        for (int i = 1; i < m_layoutTree->count(); ++i) {
            RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
            double dur_i = row_i->rowTimeline()->getEndTime();

            if ((isComponent || row_i->rowType() == OBJTYPE_LAYER) && dur_i > duration)
                duration = dur_i;

            if (dur_i > maxDuration)
                maxDuration = dur_i;
        }
    }

    m_scene->ruler()->setDuration(duration, maxDuration);
}

void RowManager::updateFiltering(RowTree *row)
{
    if (!row) { // update all rows
        RowTree *row_i;
        for (int i = 1; i < m_layoutTree->count(); ++i) {
            row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
            updateRowFilter(row_i);
        }
    } else {
        updateRowFilterRecursive(row);
    }
}

void RowManager::updateRowFilterRecursive(RowTree *row)
{
    updateRowFilter(row);

    if (!row->empty()) {
        const auto childRows = row->childRows();
        for (auto child : childRows)
            updateRowFilterRecursive(child);
    }
}

void RowManager::updateRowFilter(RowTree *row)
{
    bool parentOk = !row->parentRow()|| row->parentRow()->isVisible();
    bool shyOk     = !row->shy()    || !m_scene->treeHeader()->filterShy();
    bool visibleOk = row->visible() || !m_scene->treeHeader()->filterHidden();
    bool lockOk    = !row->locked() || !m_scene->treeHeader()->filterLocked();
    bool expandOk  = !row->expandHidden();

    row->setVisible(parentOk && shyOk && visibleOk && lockOk && expandOk);
    row->rowTimeline()->setVisible(row->isVisible());
}

void RowManager::deleteRow(RowTree *row)
{
   if (row && row->rowType() != OBJTYPE_SCENE) {
       if (row->parentRow())
           row->parentRow()->removeChild(row);

       deleteRowRecursive(row);
   }
}

void RowManager::deleteRowRecursive(RowTree *row)
{
   if (!row->childProps().empty()) {
       const auto childProps = row->childProps();
       for (auto child : childProps)
            deleteRowRecursive(child);
   }

   if (!row->childRows().empty()) {
       const auto childRows = row->childRows();
       for (auto child : childRows)
            deleteRowRecursive(child);
   }

   m_selectedRows.removeAll(row);

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

bool RowManager::isRowSelected(RowTree *row) const
{
    return m_selectedRows.contains(row);
}

QVector<RowTree *> RowManager::selectedRows() const
{
    return m_selectedRows;
}

int RowManager::getRowIndex(RowTree *row, int startAt)
{
    if (row) {
        for (int i = startAt; i < m_layoutTree->count(); ++i) {
          if (row == m_layoutTree->itemAt(i)->graphicsItem())
             return i;
        }
    }

    return -1;
}

// Return the index of a direct child (not grand children) of a parent
int RowManager::getChildIndex(RowTree *parentRow, RowTree *childRow)
{
    int index = getRowIndex(parentRow);

    if (index != -1 && index < m_layoutTimeline->count() - 1) {
        // make sure it is a direct child, not a grand child.
        while (childRow->depth() > parentRow->depth() + 1)
            childRow = childRow->parentRow();

        int childIndex = -1;
        for (int i = index + 1; i < m_layoutTree->count(); ++i) {
            RowTree *childRow_i =
                    static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());

            if (childRow_i->depth() == parentRow->depth() + 1)
                childIndex++;
            else if (childRow_i->depth() <= parentRow->depth())
                break;

            if (childRow_i == childRow)
                return childIndex;
        }
    }

    return -1;
}

bool RowManager::isFirstChild(RowTree *parentRow, RowTree *childRow)
{
    int index = getRowIndex(parentRow);

    if (index != -1 && index < m_layoutTimeline->count() - 1) {
        RowTree *firstChild =
                static_cast<RowTree *>(m_layoutTree->itemAt(index + 1)->graphicsItem());

        if (firstChild == childRow && childRow->depth() == parentRow->depth() + 1)
            return true;
    }

    return false;
}

bool RowManager::isSingleSelected() const
{
    return m_selectedRows.size() == 1;
}

int RowManager::getLastChildIndex(RowTree *row, int index)
{
    if (index == -1)
        index = getRowIndex(row);

    if (index != -1) {
        for (int i = index + 1; i < m_layoutTree->count(); ++i) {
            if (static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem())->depth()
                <= row->depth()) {
                return i - 1;
            }
        }

        return m_layoutTree->count() - 1; // last row
    }

    return -1;
}

void RowManager::clampIndex(int &idx)
{
    if (idx < 1)
        idx = 1;
    else if (idx > m_layoutTree->count() - 1)
        idx = m_layoutTree->count() - 1;
}

// Index within rows indices bounds
bool RowManager::validIndex(int idx) const
{
    return idx > 0 && idx < m_layoutTree->count();
}

// Adjust index to point to the correct row taking into consideration collaped rows
void RowManager::correctIndex(int &idx)
{
    if (!validIndex(idx)) {
        idx = -1;
        return;
    }

    // adjust for collapsed and filtered items (invisible)
    for (int i = 1; i <= idx; ++i) {
        if (!m_layoutTimeline->itemAt(i)->graphicsItem()->isVisible()) {
            if (++idx > m_layoutTimeline->count() - 1) {
                idx = -1;
                return;
            }
        }
    }
}

void RowManager::collapseAllPropertyRows()
{
    for (int i = 0; i < m_layoutTree->count(); ++i) {
        RowTree *row = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
        if (row->isProperty() && row->propertyExpanded())
            row->setPropertyExpanded(false);
    }
}
