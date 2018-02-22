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
#include "KeyframeManager.h"

#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtCore/qdebug.h>

RowManager::RowManager(TimelineGraphicsScene *scene, QGraphicsLinearLayout *layoutLabels,
                       QGraphicsLinearLayout *layoutTimeline)
    : m_scene(scene)
    , m_layoutLabels(layoutLabels)
    , m_layoutTimeline(layoutTimeline)
{

}

RowTree *RowManager::getOrCreatePropertyRow(PropertyType propType, RowTree *masterRow)
{
    if (masterRow->hasPropertyChildren()) {
        const auto childRows = masterRow->childRows();
        for (const auto child : childRows) {
            if (child->propertyType() == propType)
                return child;
        }
    }

    return createRow(RowType::Property, masterRow, 0, propType);
}

RowTree *RowManager::createRow(RowType rowType, RowTree *parentRow, const QString &label,
                               PropertyType propType)
{
    if (parentRow == nullptr && rowType != RowType::Scene) {
        qWarning() << __FUNCTION__ << "Invalid parent. Row must have a valid parent row."
                                      " No row added.";
    } else if (parentRow != nullptr && parentRow->rowType() == RowType::Property) {
        qWarning() << __FUNCTION__ << "Property row cannot have children. No row added.";
    } else {
        RowTree *rowLabel = nullptr;

        if (propType != PropertyType::None)
            rowLabel = new RowTree(m_scene->ruler(), propType);
        else
            rowLabel = new RowTree(m_scene->ruler(), rowType, label);

        if (parentRow != nullptr)
            parentRow->addChild(rowLabel);

        rowLabel->rowTimeline()->setStartTime(0);
        rowLabel->rowTimeline()->setEndTime(qMin(10.0, m_scene->ruler()->duration()));

        int index = getRowIndex(parentRow) + 1;
        if (index == 0)
            index = 1;

        m_layoutLabels->insertItem(index, rowLabel);
        m_layoutTimeline->insertItem(index, rowLabel->rowTimeline());

        return rowLabel;
    }

    return nullptr;
}

RowTree *RowManager::getRowAbove(RowTree *row)
{
    int rowIndex = getRowIndex(row);

    if (rowIndex > 1) {
        RowTree *rowAbove = static_cast<RowTree *>(m_layoutLabels->itemAt(rowIndex - 1));

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
        return static_cast<RowTree *>(m_layoutTimeline->itemAt(idx)->graphicsItem());

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
    if (row != nullptr && row != m_selectedRow && row->rowType() != RowType::Property) {
        if (m_selectedRow != nullptr)
            m_selectedRow->setState(InteractiveTimelineItem::Normal);

        row->setState(InteractiveTimelineItem::Selected);
        m_selectedRow = row;
    }
}

void RowManager::deleteRow(RowTree *row)
{
   if (row != nullptr && row->rowType() != RowType::Scene) {
       if (m_selectedRow == row)
           selectRow(getRowAbove(row));

       deleteRowRecursive(row);
   }
}

void RowManager::deleteRowRecursive(RowTree *row)
{
   if (!row->childRows().empty()) {
       for (auto child : row->childRows())
            deleteRowRecursive(child);
   }

   if (row == m_selectedRow)
       m_selectedRow = nullptr;

   if (row->parentRow() != nullptr)
       row->parentRow()->removeChild(row);

   m_scene->keyframeManager()->deleteKeyframes(row->rowTimeline());

   m_layoutTimeline->removeItem(row->rowTimeline());
   m_layoutLabels->removeItem(row);
   delete row->rowTimeline();
   delete row;
}

RowTree *RowManager::selectedRow() const
{
    return m_selectedRow;
}

int RowManager::getRowIndex(RowTree *row)
{
    if (row != nullptr) {
        for (int i = 1; i < m_layoutLabels->count(); ++i) {
          if (row == m_layoutLabels->itemAt(i)->graphicsItem())
             return i;
        }
    }

    return -1;
}

void RowManager::clampIndex(int &idx)
{
    if (idx < 1)
        idx = 1;
    else if (idx > m_layoutLabels->count() - 1)
        idx = m_layoutLabels->count() - 1;
}

// Index within rows indices bounds
bool RowManager::validIndex(int idx) const
{
    return idx > 0 && idx < m_layoutLabels->count();
}

// Adjust index to point to the correct row taking into consideration collaped rows
void RowManager::correctIndex(int &idx)
{
    if (!validIndex(idx)) {
        idx = -1;
        return;
    }

    // adjust for collapsed items (invisible)
    for (int i = 1; i <= idx; ++i) {
        if (!m_layoutTimeline->itemAt(i)->graphicsItem()->isVisible()) {
            if (++idx > m_layoutTimeline->count() - 1) {
                idx = -1;
                return;
            }
        }
    }
}
