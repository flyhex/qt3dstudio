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

#include "RowMover.h"
#include "RowTree.h"
#include "RowManager.h"
#include "TimelineGraphicsScene.h"
#include "TimelineConstants.h"

#include <QtGui/qpainter.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicslinearlayout.h>

RowMover::RowMover(TimelineGraphicsScene *scene)
    : TimelineItem()
    , m_scene(scene)
{
    setZValue(99);
    setGeometry(0, 0, TimelineConstants::TREE_MAX_W, 10);
}

void RowMover::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();

    painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 1));
    painter->drawLine(0, 0, TimelineConstants::TREE_BOUND_W, 0);

    painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 4));
    painter->drawLine(0, 2, 5, 2);

    painter->restore();
}

RowTree *RowMover::insertionParent() const
{
    return m_insertionParent;
}

void RowMover::resetInsertionParent(RowTree *newTarget)
{
    if (m_insertionParent) {
        m_insertionParent->setMoveTarget(false);
        m_insertionParent = nullptr;
    }

    if (newTarget) {
        m_insertionParent = newTarget;
        m_insertionParent->setMoveTarget(true);
    }
}

int RowMover::targetIndex() const
{
    return m_targetIndex;
}

RowTree *RowMover::sourceRow() const
{
    return m_sourceRow;
}

bool RowMover::isActive()
{
    return m_active;
}

void RowMover::start(RowTree *row)
{
    m_active = true;

    if (row) {
        m_sourceRow = row;
        m_sourceRow->setMoveSourceRecursive(true);
        qApp->setOverrideCursor(Qt::ClosedHandCursor);
    }
}

void RowMover::end(bool force)
{
    if (m_active || force) {
        m_active = false;
        if (m_sourceRow) {
            m_sourceRow->setMoveSourceRecursive(false);
            m_sourceRow = nullptr;
        }

        setVisible(false);
        resetInsertionParent();

        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();
    }
}

void RowMover::updateState(int index, int depth, double y)
{
    m_targetIndex = index;

    setPos(25 + depth * TimelineConstants::ROW_DEPTH_STEP, y);
    setVisible(true);
}

RowTree *RowMover::getRowAtPos(const QPointF &scenePos)
{
    QList<QGraphicsItem *> items = m_scene->items(scenePos);

    int index = 0;
    while (index < items.size()) {
        QGraphicsItem *item = items.at(index++);
        if (item->type() == TimelineItem::TypeRowTree)
            return static_cast<RowTree *>(item);
    }

    return nullptr;
}

void RowMover::updateTargetRow(const QPointF &scenePos)
{
    // row will be inserted just below rowInsert1 and just above rowInsert2 (if it exists)
    RowTree *rowInsert1 = getRowAtPos(scenePos + QPointF(0, TimelineConstants::ROW_H * -.5));
    RowTree *rowInsert2 = getRowAtPos(scenePos + QPointF(0, TimelineConstants::ROW_H * .5));

    bool valid = rowInsert1

            // not moving an ancestor into a decendent
            && (!m_sourceRow || !rowInsert1->isDecendentOf(m_sourceRow))

            // not inserting as a first child of self
            && (!m_sourceRow || !(rowInsert1 == m_sourceRow && !rowInsert1->empty()))

            // not inserting non-layer at root level
            && !((!m_sourceRow || m_sourceRow->rowType() != OBJTYPE_LAYER)
                 && rowInsert1->rowType() == OBJTYPE_SCENE)

            // Layer cases
            && validLayerMove(rowInsert1, rowInsert2);

    if (valid) {
        // if dragging over a property or a parent of a property, move to the first row
        // after the property
        if (rowInsert1->isProperty()) {
            rowInsert1 = rowInsert1->parentRow()->childProps().last();
            rowInsert2 = getRowAtPos(QPointF(0, rowInsert1->y() + TimelineConstants::ROW_H));
        } else if (rowInsert1->hasPropertyChildren() && rowInsert1->expanded()) {
            rowInsert1 = rowInsert1->childProps().last();
            rowInsert2 = getRowAtPos(QPointF(0, rowInsert1->y() + TimelineConstants::ROW_H));
        }

        // calc insertion depth
        int depth;
        if (m_sourceRow && m_sourceRow->rowType() == OBJTYPE_LAYER) {
            depth = 2; // layers can only be moved on depth 2
        } else if (m_sourceRow && m_sourceRow->rowType() == OBJTYPE_MATERIAL) {
            depth = m_sourceRow->depth(); // materials cannot change parent
            if (rowInsert2 && depth < rowInsert2->depth())
                valid = false;
        } else {
            int depthMin = rowInsert2 ? rowInsert2->depth() : 3;
            int depthMax = rowInsert1->depth();

            if (rowInsert1->isContainer() && rowInsert1 != m_sourceRow) {
                depthMax++; // Container: allow insertion as a child
            } else if (rowInsert1->isPropertyOrMaterial()
                       && !rowInsert1->parentRow()->isContainer()) {
                depthMax--; // non-container with properties and/or a material
            }

            static const int LEFT_MARGIN = 20;
            depth = (scenePos.x() - LEFT_MARGIN) / TimelineConstants::ROW_DEPTH_STEP;
            depth = qBound(depthMin, depth, depthMax);
        }

        // calc insertion parent
        RowTree *insertParent = rowInsert1;
        for (int i = rowInsert1->depth(); i >= depth; --i)
            insertParent = insertParent->parentRow();

        resetInsertionParent(insertParent);

        if (m_sourceRow && m_sourceRow->rowType() == OBJTYPE_MATERIAL
                && m_sourceRow->parentRow() != m_insertionParent) {
            valid = false; // not moving a material row outside its parent
        }

        if (m_sourceRow && m_sourceRow->isMaster() && !m_insertionParent->isMaster())
            valid = false; // don't insert master slide object into non-master slide object

        if (valid) {
            // calc insertion index
            int index = rowInsert1->index() + 1;
            if (rowInsert1->isProperty() && depth == rowInsert1->depth()) {
                index = 0;
            } else if (rowInsert1 == insertParent) {
                if (insertParent->expanded() || insertParent->childRows().empty())
                    index = 0;
                else
                    index = insertParent->childRows().last()->index() + 1;
            } else if (depth < rowInsert1->depth()) {
                RowTree *row = rowInsert1;
                for (int i = depth; i < rowInsert1->depth(); ++i)
                    row = row->parentRow();
                index = row->index() + 1;
            }

            if (m_sourceRow && m_sourceRow->parentRow() == insertParent
                    && index > m_sourceRow->index()) { // moving down under the same parent
                index--; // m_sourceRow is removed from layout, shift index up by 1
            }

            updateState(index, depth, rowInsert1->y() + TimelineConstants::ROW_H);
        }
    }

    if (!valid) {
        setVisible(false);
        resetInsertionParent();
    }
}

bool RowMover::validLayerMove(RowTree *rowAtIndex, RowTree *nextRowAtIndex)
{
    // we don't care about non-layers in this method
    if (!m_sourceRow || m_sourceRow->rowType() != OBJTYPE_LAYER)
        return true;

    if (rowAtIndex->rowType() == OBJTYPE_SCENE)
        return true;

    if (rowAtIndex->rowType() == OBJTYPE_LAYER)
       return rowAtIndex->empty() || !rowAtIndex->expanded();

    if (!nextRowAtIndex || (nextRowAtIndex->depth() <= rowAtIndex->depth()
                            && nextRowAtIndex->depth() == 2)) {
        return true;
    }

    return false;
}

int RowMover::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRowMover;
}
