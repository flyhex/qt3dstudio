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
#include "StudioPreferences.h"

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

    m_autoExpandTimer.setSingleShot(true);
    connect(&m_autoExpandTimer, &QTimer::timeout, [this]() {
        if (m_rowAutoExpand) {
            m_rowAutoExpand->updateExpandStatus(RowTree::ExpandState::Expanded, true);
            // Update RowMover after the expansion. The +50 below is just a small margin to ensure
            // correct row heights before updateTargetRowLater is called.
            QTimer::singleShot(TimelineConstants::EXPAND_ANIMATION_DURATION + 50, [this]() {
                if (updateTargetRowLater)
                    updateTargetRowLater();
            });
        }
    });
}

void RowMover::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    static const QPolygon polygon({QPoint(0, 0), QPoint(0, 3), QPoint(7, 3), QPoint(7, 1),
                                   QPoint(TimelineConstants::TREE_BOUND_W, 1),
                                   QPoint(TimelineConstants::TREE_BOUND_W, 0)});
    painter->setPen(QPen(CStudioPreferences::timelineRowMoverColor(), 1));
    painter->setBrush(CStudioPreferences::timelineRowMoverColor());
    painter->drawConvexPolygon(polygon);
}

RowTree *RowMover::insertionTarget() const
{
    return m_insertionTarget;
}

RowTree *RowMover::insertionParent() const
{
    return m_insertionParent;
}

QVector<RowTree *> RowMover::sourceRows() const
{
    return m_sourceRows;
}

void RowMover::resetInsertionParent(RowTree *newParent)
{
    if (m_insertionParent) {
        m_insertionParent->setDnDState(RowTree::DnDState::None, RowTree::DnDState::Parent);
        m_insertionParent = nullptr;
    }

    if (newParent) {
        m_insertionParent = newParent;
        m_insertionParent->setDnDState(RowTree::DnDState::Parent, RowTree::DnDState::None);
    } else {
        m_insertionTarget = nullptr;
    }
}

bool RowMover::isActive()
{
    return m_active;
}

void RowMover::start(const QVector<RowTree *> &rows)
{
    m_sourceRows.clear();
    if (!rows.isEmpty()) {
        // Remove rows that have an ancestor included in the selection or ones that are of
        // invalid type for moving
        for (auto candidateRow : rows) {
            bool omit = !candidateRow->draggable();
            if (!omit) {
                for (auto checkRow : rows) {
                    if (candidateRow->isDecendentOf(checkRow))
                        omit = true;
                }
                if (!omit)
                    m_sourceRows.append(candidateRow);
            }
        }
        if (!m_sourceRows.isEmpty()) {
            m_active = true;
            for (auto row : qAsConst(m_sourceRows))
                row->setDnDState(RowTree::DnDState::Source, RowTree::DnDState::None, true);
            qApp->setOverrideCursor(Qt::ClosedHandCursor);
        }
    }
}

void RowMover::end(bool force)
{
    if (m_active || force) {
        m_active = false;
        for (auto row : qAsConst(m_sourceRows))
            row->setDnDState(RowTree::DnDState::None, RowTree::DnDState::Any, true);
        m_sourceRows.clear();

        if (m_insertionTarget)
            m_insertionTarget->setDnDState(RowTree::DnDState::None);

        setVisible(false);
        resetInsertionParent();
        updateTargetRowLater = {};
        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();

        m_autoExpandTimer.stop();
    }
}

void RowMover::updateState(int depth, double y)
{
    setPos(24 + depth * TimelineConstants::ROW_DEPTH_STEP, y);
    setVisible(true);
}

bool RowMover::isNextSiblingRow(RowTree *rowMain, RowTree *rowSibling) const
{
    // order matters, rowSibling is below rowMain
    return rowMain->parentRow() == rowSibling->parentRow()
            && rowSibling->index() - rowMain->index() == 1;
}

bool RowMover::sourceRowsHasMaster() const
{
    for (auto sourceRow : qAsConst(m_sourceRows)) {
        if (sourceRow->isMaster() && sourceRow->rowType() != OBJTYPE_LAYER)
            return true;
    }

    return false;
}

bool RowMover::isSourceRowsDescendant(RowTree *row) const
{
    if (row) {
        for (auto sourceRow : qAsConst(m_sourceRows)) {
            if (row == sourceRow || row->isDecendentOf(sourceRow))
                return true;
        }
    }

    return false;
}

// rowType parameter is used to highlight the target row when RowMover is not active,
// i.e. when dragging from project or basic objects palettes
void RowMover::updateTargetRow(const QPointF &scenePos, EStudioObjectType rowType)
{
    // DnD a presentation / Qml stream from the project panel (to set it as a subpresentation)
    if (rowType == OBJTYPE_PRESENTATION || rowType == OBJTYPE_QML_STREAM) {
        if (m_insertionTarget)
            m_insertionTarget->setDnDState(RowTree::DnDState::None, RowTree::DnDState::SP_TARGET);

        RowTree *rowAtMouse = m_scene->rowManager()->getRowAtPos(scenePos);
        if (rowAtMouse) {
            // m_insertionTarget will go through CFileDropSource::ValidateTarget() which will
            // filter out invalid drop rows
            m_insertionTarget = rowAtMouse;
            m_insertType = Q3DStudio::DocumentEditorInsertType::LastChild;

            if (rowAtMouse->rowType() == OBJTYPE_LAYER || rowAtMouse->rowType() == OBJTYPE_MATERIAL
                || rowAtMouse->rowType() == OBJTYPE_IMAGE) {
                m_insertionTarget->setDnDState(RowTree::DnDState::SP_TARGET);
            }
            m_rowAutoExpand = rowAtMouse;
            m_autoExpandTimer.start(TimelineConstants::AUTO_EXPAND_TIME);
        } else {
            m_rowAutoExpand = nullptr;
            m_autoExpandTimer.stop();
        }
        return;
    }

    EStudioObjectType theRowType = rowType;
    if (theRowType == OBJTYPE_UNKNOWN && m_sourceRows.size() == 1)
        theRowType = m_sourceRows[0]->rowType();

    // row will be inserted just below rowInsert1 and just above rowInsert2 (if it exists)
    RowTree *rowInsert1 = m_scene->rowManager()
            ->getRowAtPos(scenePos + QPointF(0, TimelineConstants::ROW_H * -.5));
    RowTree *rowInsert2 = m_scene->rowManager()
            ->getRowAtPos(scenePos + QPointF(0, TimelineConstants::ROW_H * .5));

    bool valid = rowInsert1 && theRowType != OBJTYPE_MATERIAL
            && theRowType != OBJTYPE_CUSTOMMATERIAL;

    if (valid) {
        // if dragging over a property or a parent of a property, move to the first row
        // after the property
        if (rowInsert1->isProperty()) {
            rowInsert1 = rowInsert1->parentRow()->childProps().last();
            rowInsert2 = m_scene->rowManager()
                    ->getRowAtPos(QPointF(0, rowInsert1->y() + TimelineConstants::ROW_H));
        } else if (rowInsert1->hasPropertyChildren() && rowInsert1->expanded()) {
            rowInsert1 = rowInsert1->childProps().last();
            rowInsert2 = m_scene->rowManager()
                    ->getRowAtPos(QPointF(0, rowInsert1->y() + TimelineConstants::ROW_H));
        }

        // calc insertion depth
        bool inAComponent = static_cast<RowTree *>(m_scene->layoutTree()->itemAt(1))->isComponent();
        int depth;
        int depthMin = 2;
        if (rowInsert2)
            depthMin = rowInsert2->depth();
        else if (theRowType != OBJTYPE_LAYER && !inAComponent)
            depthMin = 3;

        int depthMax = rowInsert1->depth();
        bool srcHasMaster = sourceRowsHasMaster();
        if (!rowInsert1->locked() && rowInsert1->isContainer() && !m_sourceRows.contains(rowInsert1)
            // prevent insertion a master row under a non-master unless under a component root
            && (!(srcHasMaster && !rowInsert1->isMaster()) || rowInsert1->isComponent())) {
            depthMax++; // Container: allow insertion as a child
        } else if (rowInsert1->isPropertyOrMaterial() && !rowInsert1->parentRow()->isContainer()) {
            depthMax--; // non-container with properties and/or a material
        } else if (srcHasMaster) {
            RowTree *row = rowInsert1->parentRow();
            while (row && !row->isMaster() && !row->isComponent()) {
                depthMax--;
                row = row->parentRow();
            }
        }

        if (theRowType == OBJTYPE_LAYER) {
            depth = 2; // layers can only be moved on depth 2
        } else if (theRowType == OBJTYPE_EFFECT) {
            depth = 3; // effects can only be moved on depth 3 (layer direct child)
        } else {
            static const int LEFT_MARGIN = 20;
            depth = (scenePos.x() - LEFT_MARGIN) / TimelineConstants::ROW_DEPTH_STEP;
            depth = qBound(depthMin, depth, depthMax);
        }
        // calc insertion parent
        RowTree *insertParent = rowInsert1;
        for (int i = rowInsert1->depth(); i >= depth; --i)
            insertParent = insertParent->parentRow();

        resetInsertionParent(insertParent);

        if (depth < depthMin || depth > depthMax
                || (theRowType != OBJTYPE_UNKNOWN
                    && !CStudioObjectTypes::AcceptableParent(theRowType,
                                                             m_insertionParent->rowType()))
                || m_insertionParent->locked()) {
            valid = false;
        }

        for (auto sourceRow : qAsConst(m_sourceRows)) {
            if (m_insertionParent == sourceRow
                || m_insertionParent->isDecendentOf(sourceRow)
                || !CStudioObjectTypes::AcceptableParent(sourceRow->rowType(),
                                                         m_insertionParent->rowType())) {
                // prevent insertion under itself, or under unacceptable parent
                valid = false;
                break;
            }
        }

        // calc insertion target and type
        if (rowInsert1 == m_insertionParent) {
            if (m_insertionParent->expanded() && !m_insertionParent->childRows().empty()) {
                m_insertionTarget = m_insertionParent->childRows().at(0);
                m_insertType = Q3DStudio::DocumentEditorInsertType::PreviousSibling;
            } else {
                m_insertionTarget = m_insertionParent;
                m_insertType = Q3DStudio::DocumentEditorInsertType::LastChild;
            }
        } else if (rowInsert1->isProperty() && depth == rowInsert1->depth()) {
            if (m_insertionParent->childRows().isEmpty()) {
                m_insertionTarget = m_insertionParent;
                m_insertType = Q3DStudio::DocumentEditorInsertType::LastChild;
            } else {
                m_insertionTarget = m_insertionParent->childRows().at(0);
                m_insertType = Q3DStudio::DocumentEditorInsertType::PreviousSibling;
            }
        } else {
            m_insertionTarget = rowInsert1;
            m_insertType = Q3DStudio::DocumentEditorInsertType::NextSibling;
            if (depth < rowInsert1->depth()) {
                for (int i = depth; i < rowInsert1->depth(); ++i)
                    m_insertionTarget = m_insertionTarget->parentRow();
            }
        }
        // Don't allow single move right next to moving row at same depth
        if (m_sourceRows.size() == 1
                && (m_insertionTarget == m_sourceRows[0]
                    || ((m_insertType == Q3DStudio::DocumentEditorInsertType::NextSibling
                         && isNextSiblingRow(m_insertionTarget, m_sourceRows[0]))
                        || (m_insertType == Q3DStudio::DocumentEditorInsertType::PreviousSibling
                            && isNextSiblingRow(m_sourceRows[0], m_insertionTarget))))) {
            valid = false;
        }
        if (valid) {
            updateState(depth, rowInsert1->y() + rowInsert1->size().height());

            // auto expand
            if (!rowInsert1->locked() && !rowInsert1->expanded() && rowInsert1->isContainer()
                    && !rowInsert1->empty() && !isSourceRowsDescendant(rowInsert1)
                    && depth == rowInsert1->depth() + 1) {
                updateTargetRowLater = std::bind(&RowMover::updateTargetRow, this,
                                                 scenePos, rowType);
                m_rowAutoExpand = rowInsert1;
                m_autoExpandTimer.start(TimelineConstants::AUTO_EXPAND_TIME);
            } else {
                m_rowAutoExpand = nullptr;
                m_autoExpandTimer.stop();
            }
        }
    }

    if (!valid) {
        m_rowAutoExpand = nullptr;
        m_autoExpandTimer.stop();
        setVisible(false);
        resetInsertionParent();
    }
}

int RowMover::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRowMover;
}

Q3DStudio::DocumentEditorInsertType::Enum RowMover::insertionType() const
{
    return m_insertType;
}
