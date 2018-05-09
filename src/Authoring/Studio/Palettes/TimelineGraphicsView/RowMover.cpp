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
    : QGraphicsRectItem()
    , m_scene(scene)
{
    setZValue(99);
    setRect(0, -5, TimelineConstants::TREE_MAX_W, 10);
}

void RowMover::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();

    painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 1));
    painter->drawLine(0, -1, TimelineConstants::TREE_BOUND_W, -1);

    painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 4));
    painter->drawLine(0, -2, 5, -2);

    painter->restore();
}

RowTree *RowMover::insertionParent() const
{
    return m_insertionParent;
}

void RowMover::resetInsertionParent(RowTree *newTarget)
{
    if (m_insertionParent != nullptr) {
        m_insertionParent->setMoveTarget(false);
        m_insertionParent = nullptr;
    }

    if (newTarget != nullptr) {
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

void RowMover::updateState(int index, int depth, int rawIndex)
{
    m_targetIndex = index;

    setPos(25 + depth * 15, (rawIndex + 1) * TimelineConstants::ROW_H);
    setVisible(true);
}

void RowMover::updateTargetRow(int mouseX, int mouseY)
{
    int indexRaw = qRound((float)mouseY / TimelineConstants::ROW_H) - 1;
    int indexInLayout = indexRaw;
    m_scene->rowManager()->correctIndex(indexInLayout);
    bool valid = indexInLayout != -1;

    RowTree *rowAtIndex;
    RowTree *nextRowAtIndex;

    if (valid) { // valid row index
        rowAtIndex = static_cast<RowTree *>(m_scene->layoutTree()->itemAt(indexInLayout)
                                            ->graphicsItem());
        nextRowAtIndex = indexInLayout > m_scene->layoutTree()->count() - 2 ? nullptr :
                     static_cast<RowTree *>(m_scene->layoutTree()->itemAt(indexInLayout + 1)
                                            ->graphicsItem());

        if (!rowAtIndex->expanded())
            nextRowAtIndex = m_scene->rowManager()->getNextSiblingRow(rowAtIndex);

                // not moving an ancestor into a decendent
        valid = (!m_sourceRow || !rowAtIndex->isDecendentOf(m_sourceRow))

                // not inserting as a first child of self
                && (!m_sourceRow || !(rowAtIndex == m_sourceRow && !rowAtIndex->empty()))

                // not inserting non-layer at root level
                && !((!m_sourceRow || m_sourceRow->rowType() != OBJTYPE_LAYER)
                     && rowAtIndex->rowType() == OBJTYPE_SCENE)

                // Layer cases
                && validLayerMove(rowAtIndex, nextRowAtIndex);
    }

    if (valid) {
        // if dragging over a property or a parent of a property, move to the first row
        // after the property
        if (rowAtIndex->isProperty())
            indexRaw = rowAtIndex->parentRow()->childProps().last()->indexInLayout();
        else if (rowAtIndex->hasPropertyChildren() && rowAtIndex->expanded())
            indexRaw = rowAtIndex->childProps().last()->indexInLayout();

        // calc insertion depth
        int depth;
        if (m_sourceRow && m_sourceRow->rowType() == OBJTYPE_LAYER) {
            depth = 2; // layers can only be moved on depth 2
        } else {
            int depthMin = nextRowAtIndex ? nextRowAtIndex->depth() : 3;
            int depthMax = rowAtIndex->depth();

            if (rowAtIndex->isContainer() && rowAtIndex != m_sourceRow) {
                depthMax++; // Container: allow insertion as a child
            } else if (rowAtIndex->isPropertyOrMaterial()
                      && !rowAtIndex->parentRow()->isContainer()) {
                 depthMax--; // non-container with properties and/or a material
            }

            static const int LEFT_MARGIN = 20;
            static const int STEP = 15;
            depth = (mouseX - LEFT_MARGIN) / STEP;
            depth = qBound(depthMin, depth, depthMax);
        }

        // calc insertion parent
        RowTree *insertParent = rowAtIndex;
        for (int i = rowAtIndex->depth(); i >= depth; --i)
            insertParent = insertParent->parentRow();
        resetInsertionParent(insertParent);

        if (m_sourceRow && m_sourceRow->rowType() == OBJTYPE_MATERIAL
                && m_sourceRow->parentRow() != m_insertionParent) {
            valid = false; // not moving a material row outside its parent
        }

        if (valid) {
            // calc insertion index
            int index = rowAtIndex->index() + 1;
            if (rowAtIndex->isProperty() && depth == rowAtIndex->depth()) {
                index = 0;
            } else if (rowAtIndex == insertParent) {
                if (insertParent->expanded() || insertParent->childRows().empty())
                    index = 0;
                else
                    index = insertParent->childRows().last()->index() + 1;
            } else if (depth < rowAtIndex->depth()) {
                RowTree *row = rowAtIndex;
                for (int i = depth; i < rowAtIndex->depth(); ++i)
                    row = row->parentRow();
                index = row->index() + 1;
            }

            if (m_sourceRow && m_sourceRow->parentRow() == insertParent
                    && index > m_sourceRow->index()) { // moving down under the same parent
                index--; // m_sourceRow is removed from layout, shift index up by 1
            }

            updateState(index, depth, indexRaw);
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
