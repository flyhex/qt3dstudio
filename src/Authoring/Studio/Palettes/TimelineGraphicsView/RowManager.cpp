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

RowTree *RowManager::createRowFromBinding(ITimelineItemBinding *binding, RowTree *parentRow)
{
    RowTree *newRow = createRow(binding->GetTimelineItem()->GetObjectType(), parentRow,
                                binding->GetTimelineItem()->GetName().toQString());

    // connect the new row and its binding
    binding->setRowTree(newRow);
    newRow->setBinding(binding);

    // set row start/end time
    ITimelineTimebar *timebar = binding->GetTimelineItem()->GetTimebar();
    newRow->rowTimeline()->setStartTime(timebar->GetStartTime() * .001);
    newRow->rowTimeline()->setEndTime(timebar->GetEndTime() * .001);

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
    for (int i = 0; i < binding->GetChildrenCount(); i++)
        createRowsFromBindingRecursive(binding->GetChild(i), newRow);
}

RowTree *RowManager::getOrCreatePropertyRow(RowTree *masterRow, const QString &propType)
{
    if (masterRow->hasPropertyChildren()) {
        const auto childRows = masterRow->childRows();
        for (const auto child : childRows) {
            if (child->propertyType() == propType)
                return child;
        }
    }

    return createRow(OBJTYPE_UNKNOWN, masterRow, 0, propType);
}

RowTree *RowManager::createRow(EStudioObjectType rowType, RowTree *parentRow, const QString &label,
                               const QString &propType)
{
    if (parentRow != nullptr && parentRow->isProperty()) {
        qWarning() << __FUNCTION__ << "Property row cannot have children. No row added.";
    } else {
        // If the row doesnt have a parent, insert it under the scene (first row is the tree header)
        if (parentRow == nullptr && rowType != OBJTYPE_SCENE && m_layoutTree->count() > 1)
            parentRow = static_cast<RowTree *>(m_layoutTree->itemAt(1));

        RowTree *rowTree = nullptr;

        if (!propType.isEmpty()) // property row
            rowTree = new RowTree(m_scene, propType);
        else
            rowTree = new RowTree(m_scene, rowType, label);

        if (parentRow != nullptr)
            parentRow->addChild(rowTree);

        int index = getLastChildIndex(parentRow) + 1;
        if (index < 1)
            index = 1;

        m_layoutTree->insertItem(index, rowTree);
        m_layoutTimeline->insertItem(index, rowTree->rowTimeline());

        return rowTree;
    }

    return nullptr;
}

// Mahmoud_TODO: optimize this method or use another approach
void RowManager::syncRowPositionWithBinding(RowTree *row,
                                            Qt3DSDMTimelineItemBinding *parentBinding)
{
    int count = parentBinding->GetChildrenCount();
    int bindingIndex = -1;
    for (int i = 0; i < count; i++) {
        if (parentBinding->GetChild(i) == row->getBinding()) {
            bindingIndex = i;
            break;
        }
    }

    if (bindingIndex != -1) {
        int parentIndex = getRowIndex(parentBinding->getRowTree());
        int rowIndex = parentIndex + 1;

        for (int i = 0; i < bindingIndex; i++) {
            RowTree *childRow = static_cast<RowTree *>(m_layoutTree->itemAt(rowIndex));
            rowIndex = getLastChildIndex(childRow, rowIndex) + 1;
        }

        m_layoutTree->insertItem(rowIndex, row);
        m_layoutTimeline->insertItem(rowIndex, row->rowTimeline());
    }
}

void RowManager::reorderPropertiesFromBinding(Qt3DSDMTimelineItemBinding *binding)
{
     int index = getRowIndex(binding->getRowTree()) + 1;
     if (index > 1) {
         for (int i = binding->GetPropertyCount() - 1; i >= 0; i--) {
             RowTree *rowToMove = binding->GetProperty(i)->getRowTree();
             m_layoutTree->insertItem(index, rowToMove);
             m_layoutTimeline->insertItem(index, rowToMove->rowTimeline());
         }
     }
}

RowTree *RowManager::getRowAbove(RowTree *row)
{
    int rowIndex = getRowIndex(row);

    if (rowIndex > 1) {
        RowTree *rowAbove = static_cast<RowTree *>(m_layoutTree->itemAt(rowIndex - 1));

        if (rowAbove != nullptr) {
            while (rowAbove != nullptr && rowAbove->depth() > row->depth())
                rowAbove = rowAbove->parentRow();

            return rowAbove;
        }
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

void RowManager::selectRow(RowTree *row)
{
    if (row == nullptr)
        return;

    if (row->isProperty())
        row = row->parentRow();

    if (row != m_selectedRow) {
        clearSelection();
        row->setState(InteractiveTimelineItem::Selected);
        m_selectedRow = row;

        Qt3DSDMTimelineItemBinding *binding =
                static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding());
        g_StudioApp.GetCore()->GetDoc()->SelectDataModelObject(binding->GetInstance());
    }
}

void RowManager::clearSelection()
{
    if (m_selectedRow != nullptr) {
        m_selectedRow->setState(InteractiveTimelineItem::Normal);
        m_selectedRow = nullptr;
    }
}

void RowManager::updateRulerDuration()
{
    double duration = 0;
    double maxDuration = 0; // for setting correct size for the view so scrollbars appear correctly
    for (int i = 1; i < m_layoutTree->count(); ++i) {
        RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
        double dur_i = row_i->rowTimeline()->getEndTime();

        if (row_i->rowType() == OBJTYPE_LAYER && dur_i > duration)
            duration = dur_i;

        if (dur_i > maxDuration)
            maxDuration = dur_i;
    }

    m_scene->ruler()->setDuration(duration, maxDuration);
}

void RowManager::updateFiltering(RowTree *row)
{
    if (row == nullptr) { // update all rows
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
    bool parentOk = row->parentRow() == nullptr || row->parentRow()->isVisible();
    bool shyOk     = !row->shy()    || !m_scene->treeHeader()->filterShy();
    bool visibleOk = row->visible() || !m_scene->treeHeader()->filterHidden();
    bool lockOk    = !row->locked() || !m_scene->treeHeader()->filterLocked();

    row->setVisible(parentOk && shyOk && visibleOk && lockOk);
    row->rowTimeline()->setVisible(row->isVisible());
}

void RowManager::deleteRow(RowTree *row)
{
   if (row != nullptr && row->rowType() != OBJTYPE_SCENE) {
       if (m_selectedRow == row)
           selectRow(getRowAbove(row));

       deleteRowRecursive(row);
   }
}

void RowManager::deleteRowRecursive(RowTree *row)
{
   if (!row->childRows().empty()) {
       const auto childRows = row->childRows();
       for (auto child : childRows)
            deleteRowRecursive(child);
   }

   if (row == m_selectedRow)
       m_selectedRow = nullptr;

   if (row->parentRow() != nullptr)
       row->parentRow()->removeChild(row);

   m_scene->keyframeManager()->deleteKeyframes(row->rowTimeline());

   m_layoutTimeline->removeItem(row->rowTimeline());
   m_layoutTree->removeItem(row);
   delete row;
}

RowTree *RowManager::selectedRow() const
{
    return m_selectedRow;
}

int RowManager::getRowIndex(RowTree *row)
{
    if (row != nullptr) {
        for (int i = 1; i < m_layoutTree->count(); ++i) {
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

bool RowManager::hasProperties(RowTree *row)
{
    if (row != nullptr && !row->empty()) {
        int index = getRowIndex(row);
        if (index != -1 && index < m_layoutTree->count() - 1) {
            RowTree *nextRow = static_cast<RowTree *>(m_layoutTree->itemAt(index + 1)
                                                      ->graphicsItem());
            return nextRow->isProperty();
        }
    }

    return false;
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
