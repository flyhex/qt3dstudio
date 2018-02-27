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
#include "TimelineConstants.h"

#include <QtGui/qpainter.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgraphicsitem.h>

RowMover::RowMover() : QGraphicsRectItem()
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

int RowMover::sourceIndex() const
{
    return m_sourceIndex;
}

int RowMover::targetIndex() const
{
    return m_targetIndex;
}

bool RowMover::movingDown() const
{
    return m_targetIndex >= m_sourceIndex;
}

RowTree *RowMover::sourceRow() const
{
    return m_sourceRow;
}

bool RowMover::isActive()
{
    return m_active;
}

void RowMover::start(RowTree *row, int index)
{
    m_sourceRow = row;
    m_sourceIndex = index;
    m_active = true;

    m_sourceRow->setMoveSourceRecursive(true);

    qApp->setOverrideCursor(Qt::ClosedHandCursor);
}

void RowMover::end()
{
    if (m_active) {
        m_sourceRow->setMoveSourceRecursive(false);

        m_active = false;
        m_sourceRow = nullptr;
        m_sourceIndex = -1;

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

// TODO: not used, probably delete
bool RowMover::isValidMove(int index, RowTree *rowAtIndex)
{
    return
        // not the same row
        //index != m_currentIndex &&

        // not moving an ancestor into a decendent
        !rowAtIndex->isDecendentOf(m_sourceRow)

        // not at the top of an expanded object with property children
         && (rowAtIndex->childRows().empty() || !rowAtIndex->childRows().first()->isProperty()
             || !rowAtIndex->expanded());
}
