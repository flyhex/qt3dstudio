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

#include "TimelineGraphicsScene.h"
#include "TimelineItem.h"
#include "TreeHeader.h"
#include "Ruler.h"
#include "PlayHead.h"
#include "RowTree.h"
#include "RowMover.h"
#include "RowTimeline.h"
#include "TimelineConstants.h"
#include "TimelineToolbar.h"
#include "SelectionRect.h"
#include "RowManager.h"
#include "KeyframeManager.h"
#include "Keyframe.h"
#include "IDocumentEditor.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "ResourceCache.h"
#include "DurationEditDlg.h"
#include "TimelineControl.h"
#include "RowTreeContextMenu.h"
#include "RowTimelineContextMenu.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicswidget.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qaction.h>
#include <QtGui/qevent.h>
#include <QtCore/qtimer.h>
#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtWidgets/qaction.h>

TimelineGraphicsScene::TimelineGraphicsScene(TimelineWidget *timelineWidget)
    : QGraphicsScene(timelineWidget)
    , m_layoutRoot(new QGraphicsLinearLayout)
    , m_layoutTree(new QGraphicsLinearLayout(Qt::Vertical))
    , m_layoutTimeline(new QGraphicsLinearLayout(Qt::Vertical))
    , m_ruler(new Ruler)
    , m_playHead(new PlayHead(m_ruler))
    , m_selectionRect(new SelectionRect(m_ruler))
    , m_rowMover(new RowMover)
    , m_widgetTimeline(timelineWidget)
    , m_widgetRoot(new QGraphicsWidget)
    , m_rowManager(new RowManager(this, m_layoutTree, m_layoutTimeline))
    , m_keyframeManager(new KeyframeManager(this))
    , m_timelineControl(new TimelineControl)
    , m_currentCursor(-1)
{
    addItem(m_playHead);
    addItem(m_selectionRect);
    addItem(m_rowMover);
    addItem(m_widgetRoot);

    m_rowMover->setVisible(false);

    m_layoutRoot->setSpacing(0);
    m_layoutRoot->setContentsMargins(0, 0, 0, 0);
    m_layoutRoot->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_widgetRoot->setLayout(m_layoutRoot);

    m_layoutTree->setSpacing(0);
    m_layoutTree->setContentsMargins(0, 0, 0, 0);
    m_layoutTree->setMinimumWidth(TimelineConstants::TREE_BOUND_W);
    m_layoutTree->setMaximumWidth(TimelineConstants::TREE_BOUND_W);
    m_layoutTree->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_layoutTimeline->setSpacing(0);
    m_layoutTimeline->setContentsMargins(0, 0, 0, 0);
    m_layoutTimeline->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    m_layoutRoot->addItem(m_layoutTree);
    m_layoutRoot->addItem(m_layoutTimeline);

    m_treeHeader = new TreeHeader;

    m_layoutTree->addItem(m_treeHeader);
    m_layoutTimeline->addItem(m_ruler);

    QTimer::singleShot(0, this, [this]() {
        m_playHead->setPosition(0);
        m_widgetTimeline->viewTreeContent()->horizontalScrollBar()->setValue(0);
    });
}

void TimelineGraphicsScene::setTimelineScale(int scl)
{
    m_ruler->setTimelineScale(scl);
    m_playHead->updatePosition();

    double timelineWidth = TimelineConstants::RULER_EDGE_OFFSET * 2
                           + m_ruler->duration() * TimelineConstants::RULER_SEC_W * scl;
    m_layoutTimeline->setMinimumWidth(timelineWidth);
    m_layoutTimeline->setMaximumWidth(timelineWidth);

    for (int i = 1; i < m_layoutTimeline->count(); i++)
        static_cast<RowTimeline *>(m_layoutTimeline->itemAt(i)->graphicsItem())->updatePosition();
}

void TimelineGraphicsScene::addNewLayer()
{
    // TODO: get the update from the data model
//    RowTree *newLayer = m_rowManager->createRow(OBJTYPE_LAYER, m_sceneRow);
//    m_rowManager->selectRow(newLayer);

    // scroll to top
    m_widgetTimeline->viewTimelineContent()->verticalScrollBar()->setValue(0);
}

// TODO: test function, to be removed
void debugPrintRows(RowTree *row)
{
    qDebug().noquote().nospace() << "|" << QString("-").repeated(row->depth()) << row->label();

    if (!row->empty()) {
        for (auto child : row->childRows())
            debugPrintRows(child);
    }
}

void TimelineGraphicsScene::deleteSelectedRow() {
    m_rowManager->deleteRow(m_rowManager->selectedRow());
}

// Mahmoud_TODO: debug func, remove
void printAsset(Q3DStudio::TIdentifier asset, QString padding = " ")
{
    TAssetGraphPtr assetGraph = g_StudioApp.GetCore()->GetDoc()->GetAssetGraph();
    padding = padding.append("-");

    for (int i = 0; i < assetGraph.get()->GetChildCount(asset); i++) {
        qDebug().noquote().nospace()
                << "\x1b[42m \x1b[1m" << __FUNCTION__
                << padding
                << assetGraph.get()->GetChild(asset, i)
                << "\x1b[m";

        printAsset(assetGraph.get()->GetChild(asset, i), padding);
    }
}

void TimelineGraphicsScene::commitMoveRows()
{
    int sourceIndex = m_rowMover->sourceIndex();
    int targetIndex = m_rowMover->targetIndex();
    int rowSrcDepth = m_rowMover->sourceRow()->depth();

    // same place, abort
    if ((sourceIndex == targetIndex || sourceIndex == targetIndex + 1)
        && m_rowMover->sourceRow()->parentRow() == m_rowMover->insertionParent()) {
        return;
    }

    RowTree *rowInsertion =
            static_cast<RowTree *>(m_layoutTree->itemAt(targetIndex)->graphicsItem());

    // gather the rows to be moved
    QList<RowTree *> rowsToMove { m_rowMover->sourceRow() };
    for (int i = sourceIndex + 1; i < m_layoutTree->count();) {
        RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());

        if (row_i->depth() <= rowSrcDepth)
            break;

        m_layoutTree->removeAt(i);
        m_layoutTimeline->removeAt(i);
        rowsToMove.append(row_i);
    }

    if (m_rowMover->movingDown())
        targetIndex -= rowsToMove.count();

    if (rowInsertion->isProperty() || rowInsertion->rowType() == OBJTYPE_MATERIAL)
        rowInsertion = rowInsertion->parentRow();

    // handle for the row we will insertion after
    qt3dsdm::Qt3DSDMInstanceHandle handleInsertion = static_cast<Qt3DSDMTimelineItemBinding *>
            (rowInsertion->getBinding())->GetInstance();

    // handle for the moving row
    qt3dsdm::Qt3DSDMInstanceHandle handleSource = static_cast<Qt3DSDMTimelineItemBinding *>
            (m_rowMover->sourceRow()->getBinding())->GetInstance();

    // handle for the parent of the insertion row
    qt3dsdm::Qt3DSDMInstanceHandle handleParent = static_cast<Qt3DSDMTimelineItemBinding *>
            (m_rowMover->insertionParent()->getBinding())->GetInstance();

    // commit the row move to the binding
    bool firstChildInParent = m_rowMover->sourceRow()->parentRow() == m_rowMover->insertionParent()
            && rowInsertion == m_rowMover->insertionParent();
    int index = m_rowManager->getChildIndex(m_rowMover->insertionParent(), rowInsertion) + 1;
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), QObject::tr("Reorder Rows"))
        ->ReorderRows(handleSource, handleParent, index, firstChildInParent);

    // commit the row move to the UI
    m_rowMover->insertionParent()->addChild(m_rowMover->sourceRow());

    for (RowTree *child : qAsConst(rowsToMove)) {
        ++targetIndex;
        m_layoutTree->insertItem(targetIndex, child);
        m_layoutTimeline->insertItem(targetIndex, child->rowTimeline());
    }
}

void TimelineGraphicsScene::getLastChildRow(RowTree *row, int index, RowTree *outLastChild,
                                            RowTree *outNextSibling, int &outLastChildIndex) const
{
    if (row != nullptr && m_layoutTree->count() > 1) {
        RowTree *row_i_prev = nullptr;
        for (int i = index + 1; i < m_layoutTree->count(); ++i) {
            RowTree *row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());

            if (row_i->depth() <= row->depth()) {
                outLastChild = row_i_prev;
                outNextSibling =  row_i;
                outLastChildIndex =  i-1;
                return;
            }

            row_i_prev = row_i;
        }

        // row (and all its descendants) are at the bottom of the list
        outLastChild = static_cast<RowTree *>
                (m_layoutTree->itemAt(m_layoutTree->count() - 1)->graphicsItem());
        outNextSibling = nullptr;
        outLastChildIndex = m_layoutTree->count() - 1;
        return;
    }

    outLastChild = nullptr;
    outNextSibling = nullptr;
    outLastChildIndex = -1;
}

// TODO: not used, remove
bool TimelineGraphicsScene::lastRowInAParent(RowTree *rowAtIndex, int index)
{
    int depth = nextRowDepth(index);

    return depth == -1 || depth < rowAtIndex->depth();
}

// not used except in lastRowInAParent()
int TimelineGraphicsScene::nextRowDepth(int index) {
    if (index < m_layoutTree->count() - 1)
        index ++;

    return static_cast<RowTree *>(m_layoutTree->itemAt(index)->graphicsItem())->depth();
}

bool TimelineGraphicsScene::validLayerMove(RowTree *rowAtIndex, RowTree *nextRowAtIndex)
{
    // we don't care about non-layers in this method
    if (m_rowMover->sourceRow()->rowType() != OBJTYPE_LAYER)
        return true;

    if (rowAtIndex->rowType() == OBJTYPE_SCENE)
        return true;

    if (rowAtIndex->rowType() == OBJTYPE_LAYER)
       return rowAtIndex->empty() || !rowAtIndex->expanded();

    if (nextRowAtIndex == nullptr || (nextRowAtIndex->depth() <= rowAtIndex->depth()
                                      && nextRowAtIndex->depth() == 2))
        return true;

    return false;
}

void TimelineGraphicsScene::updateTreeWidth(double treeWidth)
{
    m_treeWidth = treeWidth;

    m_treeHeader->setWidth(treeWidth);

    RowTree *row_i;
    for (int i = 1; i < m_layoutTree->count(); ++i) {
        row_i = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
        row_i->setTreeWidth(treeWidth);
    }
}

void TimelineGraphicsScene::setMouseCursor(CMouseCursor::Qt3DSMouseCursor cursor)
{
    if (m_currentCursor != cursor) {
        if (m_currentCursor != -1)
            qApp->changeOverrideCursor(CResourceCache::GetInstance()->GetCursor(cursor));
        else
            qApp->setOverrideCursor(CResourceCache::GetInstance()->GetCursor(cursor));
        m_currentCursor = cursor;
    }
}

void TimelineGraphicsScene::resetMouseCursor()
{
    if (m_currentCursor != -1) {
        // Restoring back to no-override state seems to not change the cursor automatically
        // to the default cursor, so let's do that manually before restoring the cursor
        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();
        m_currentCursor = -1;
    }
}

void TimelineGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_pressPos = event->scenePos();
        QGraphicsItem *item = itemAt(m_pressPos, QTransform());
        if (item != nullptr) {
            item = getItemBelowType(TimelineItem::TypePlayHead, item, m_pressPos);

            if (item->type() == TimelineItem::TypeRuler) {
                m_rulerPressed = true;
                double time = m_ruler->distanceToTime(event->scenePos().x()
                                                      - m_ruler->durationStartX()) * 1000;
                g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(time);
            } else if (item->type() == TimelineItem::TypeTreeHeader) {
                if (m_treeHeader->handleButtonsClick(m_pressPos) != TreeControlType::None)
                    m_rowManager->updateFiltering();
            } else if (item->type() == TimelineItem::TypeRowTree
                       || item->type() == TimelineItem::TypeRowTreeLabelItem) {
                item = getItemBelowType(TimelineItem::TypeRowTreeLabelItem, item, m_pressPos);
                RowTree *rowTree = static_cast<RowTree *>(item);
                if (!rowTree->isProperty()) {
                    m_clickedTreeControlType = rowTree->getClickedControl(m_pressPos);
                    if (m_clickedTreeControlType != TreeControlType::None) {
                        m_rowManager->updateFiltering(rowTree);
                    } else if (!rowTree->locked() && rowTree->rowType() != OBJTYPE_MATERIAL
                               && rowTree->rowType() != OBJTYPE_IMAGE) { // dragging rows to reorder
                        int index = event->scenePos().y() / TimelineConstants::ROW_H;
                        m_rowManager->correctIndex(index);

                        if (rowTree->rowType() != OBJTYPE_SCENE && !rowTree->isProperty())
                            m_rowMover->start(rowTree, index);
                    }
                }
            } else if (item->type() == TimelineItem::TypeRowTimeline) {
                m_editedTimelineRow = static_cast<RowTimeline *>(item);
                Keyframe *keyframe = m_editedTimelineRow->getClickedKeyframe(m_pressPos);
                if (keyframe != nullptr) {  // pressed a keyframe
                    const bool ctrlKeyDown = event->modifiers() & Qt::ControlModifier;

                    if (ctrlKeyDown && keyframe->selected()) {
                        if (m_editedTimelineRow->rowTree()->isProperty())
                            m_keyframeManager->deselectKeyframe(keyframe);
                        else
                            m_keyframeManager->deselectConnectedKeyframes(keyframe);
                    } else {
                        if (!ctrlKeyDown && !keyframe->selected())
                            m_keyframeManager->deselectAllKeyframes();

                        if (m_editedTimelineRow->rowTree()->isProperty())
                            m_keyframeManager->selectKeyframe(keyframe);
                        else
                            m_keyframeManager->selectConnectedKeyframes(keyframe);

                        m_keyframePressed = true;
                    }
                } else {
                    m_keyframeManager->deselectAllKeyframes();
                    m_clickedTimelineControlType =
                            m_editedTimelineRow->getClickedControl(m_pressPos);

                    // clicked an empty spot on a timeline row, start selection rect.
                    if (m_clickedTimelineControlType == TimelineControlType::None)
                        m_selectionRect->start(m_pressPos);
                }
            }
        } else {
            m_keyframeManager->deselectAllKeyframes();

            if (m_pressPos.x() > m_ruler->x() && m_pressPos.y() > TimelineConstants::ROW_H)
                m_selectionRect->start(m_pressPos);
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void TimelineGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_editedTimelineRow)
        updateHoverStatus(event->scenePos());

    if (qAbs(event->scenePos().x() - m_pressPos.x()) > 10
            || qAbs(event->scenePos().y() - m_pressPos.y()) > 10)
        m_dragging = true;

    if (m_rulerPressed) {
        long time = m_ruler->distanceToTime(event->scenePos().x() - m_ruler->durationStartX())
                    * 1000;
        if (time >= 0)
            g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(time);
    } else if (m_dragging) {
        if (m_clickedTimelineControlType == TimelineControlType::StartHandle) {
            // resizing layer timline duration from left
            m_editedTimelineRow->setStartX(event->scenePos().x() - m_ruler->pos().x());
        } else if (m_clickedTimelineControlType == TimelineControlType::EndHandle) {
            // resizing layer timline duration from right
            m_editedTimelineRow->setEndX(event->scenePos().x() - m_ruler->pos().x());
        } else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
            // moving layer timeline duration
            double dx = event->scenePos().x() - m_pressPos.x();
            m_editedTimelineRow->moveDurationBy(dx);
            if (dx > 0)
                rowManager()->updateRulerDuration();
            m_pressPos = event->scenePos();
        } else if (m_selectionRect->isActive()) {
            // resizing keyframe selection rect
            m_selectionRect->updateSize(event->scenePos());
            m_keyframeManager->selectKeyframesInRect(m_selectionRect->rect());
        } else if (m_rowMover->isActive()) {
            // moving rows vertically (reorder/reparent)
            int indexRaw = qRound(event->scenePos().y() / TimelineConstants::ROW_H) - 1;
            int index = indexRaw;
            m_rowManager->correctIndex(index);
            bool valid = index != -1;

            RowTree *rowAtIndex;
            RowTree *nextRowAtIndex;
            RowTree *lastChildAtIndex = nullptr; // so far not used

            if (valid) { // valid row index
                rowAtIndex = static_cast<RowTree *>(m_layoutTree->itemAt(index)->graphicsItem());
                nextRowAtIndex = index > m_layoutTree->count() - 2 ? nullptr :
                             static_cast<RowTree *>(m_layoutTree->itemAt(index + 1)->graphicsItem());

                if (!rowAtIndex->expanded())
                    getLastChildRow(rowAtIndex, index, lastChildAtIndex, nextRowAtIndex, index);

                        // not moving an ancestor into a decendent
                valid = !rowAtIndex->isDecendentOf(m_rowMover->sourceRow())

                        // not inserting next to property or material rows
                        && !(nextRowAtIndex != nullptr && (nextRowAtIndex->isProperty()
                                                           || nextRowAtIndex->rowType()
                                                              == OBJTYPE_MATERIAL))

                        // not inserting as a first child of self
                        && !(rowAtIndex == m_rowMover->sourceRow() && !rowAtIndex->empty())

                        // not inserting non-layer under the scene
                        && !(m_rowMover->sourceRow()->rowType() != OBJTYPE_LAYER
                             && rowAtIndex->rowType() == OBJTYPE_SCENE)

                        // Layer cases
                        && validLayerMove(rowAtIndex, nextRowAtIndex);
            }

            if (valid) {
                // calc insertion depth
                int depth = rowAtIndex->depth();
                int depthNextRow = nextRowAtIndex != nullptr ? nextRowAtIndex->depth() : 3;
                int depthBasedOnX = (event->scenePos().x() - 20) / 15;

                if (rowAtIndex->isContainer() && rowAtIndex->expanded()
                        && rowAtIndex != m_rowMover->sourceRow()) {
                    depth++; // Container: allow insertion as a child
                } else if (rowAtIndex->isProperty()) {
                    depth--;  // Property: prevent insertion as a sibling
                }

                depthBasedOnX = qMax(depthBasedOnX, depthNextRow);
                depth = qBound(3, depth, depthBasedOnX);

                if (m_rowMover->sourceRow()->rowType() == OBJTYPE_LAYER)
                    depth = 2;

                // calc insertion parent
                RowTree *insertParent = rowAtIndex;
                for (int i = rowAtIndex->depth(); i >= depth; --i)
                    insertParent = insertParent->parentRow();

                m_rowMover->resetInsertionParent(insertParent);
                m_rowMover->updateState(index, depth, indexRaw);
            }

            if (!valid) {
                m_rowMover->setVisible(false);
                m_rowMover->resetInsertionParent();
            }
        } else if (m_keyframePressed) { // moving selected keyframes
            QPointF scenePos = event->scenePos();

            if (scenePos.x() < m_ruler->durationStartX())
                scenePos.setX(m_ruler->durationStartX());

            m_keyframeManager->moveSelectedKeyframes(scenePos.x() - m_pressPos.x());

            m_pressPos = scenePos;
        }
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void TimelineGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());

        if (item != nullptr && !m_dragging) {
            item = getItemBelowType(TimelineItem::TypeRowTreeLabelItem, item, m_pressPos);
            item = getItemBelowType(TimelineItem::TypePlayHead, item, m_pressPos);
            // select pressed row
            RowTree *rowTree = nullptr;
            if (item->type() == TimelineItem::TypeRowTree)
                rowTree = static_cast<RowTree *>(item);
            else if (item->type() == TimelineItem::TypeRowTimeline)
                rowTree = static_cast<RowTimeline *>(item)->rowTree();

            if (rowTree != nullptr && m_clickedTreeControlType == TreeControlType::None
                    && !rowTree->locked()) {
                m_rowManager->selectRow(rowTree);
            }

        } else if (m_rowMover->isActive()) { // moving rows (reorder/reparent)
            if (m_rowMover->insertionParent() != nullptr) // valid row move, commit it
                commitMoveRows();
        } else if (m_keyframePressed) {
            // update keyframe movement (time) to binding
            m_keyframeManager->commitMoveSelectedKeyframes();
        } else if (m_clickedTimelineControlType == TimelineControlType::StartHandle
                   || m_clickedTimelineControlType == TimelineControlType::EndHandle
                   || m_clickedTimelineControlType == TimelineControlType::Duration) {
            // update duration values to the binding
            m_editedTimelineRow->commitDurationMove();
        }

        if (m_clickedTimelineControlType == TimelineControlType::Duration
                || m_clickedTimelineControlType == TimelineControlType::EndHandle) {
            // Update ruler duration if needed
            rowManager()->updateRulerDuration();
        }

        // reset mouse press params
        m_selectionRect->end();
        m_rowMover->end();
        m_dragging = false;
        m_rulerPressed = false;
        m_keyframePressed = false;
        m_clickedTimelineControlType = TimelineControlType::None;
        m_editedTimelineRow = nullptr;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void TimelineGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    const QPointF scenePos = event->scenePos();
    QGraphicsItem *item = itemAt(scenePos, QTransform());
    if (item != nullptr) {
        item = getItemBelowType(TimelineItem::TypePlayHead, item, scenePos);
        if (item->type() == TimelineItem::TypeRowTreeLabelItem) {
            RowTreeLabelItem *treeLabelItem = static_cast<RowTreeLabelItem *>(item);
            if (!treeLabelItem->parentRow()->isProperty()) {
                // Tree labels text can be edited with double-click
                treeLabelItem->setEnabled(true);
                treeLabelItem->setFocus();
            }
        } else if (item->type() == TimelineItem::TypeRowTimeline) {
            RowTimeline *timelineItem = static_cast<RowTimeline *>(item);
            long theStartTime = timelineItem->getStartTime() * 1000;
            long theEndTime = timelineItem->getEndTime() * 1000;
            m_timelineControl->setRowTimeline(timelineItem);
            CDurationEditDlg theDurationEditDlg;
            theDurationEditDlg.showDialog(theStartTime, theEndTime,
                                          g_StudioApp.GetCore()->GetDoc(), m_timelineControl);
        }
    }

    QGraphicsScene::mouseDoubleClickEvent(event);
}

void TimelineGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    bool ctrl = keyEvent->modifiers() & Qt::ControlModifier;

    if (keyEvent->key() == Qt::Key_Delete) {
        m_keyframeManager->deleteSelectedKeyframes();
    } else if (keyEvent->key() == Qt::Key_C && ctrl) { // Ctrl+C
        m_keyframeManager->copySelectedKeyframes();
    } else if (keyEvent->key() == Qt::Key_X && ctrl) { // Ctrl+X
        if (m_keyframeManager->oneMasterRowSelected()) { // must be from a single master row
            m_keyframeManager->copySelectedKeyframes();
            m_keyframeManager->deleteSelectedKeyframes();
        }
    } else if (keyEvent->key() == Qt::Key_V && ctrl) { // Ctrl+V
        if (m_rowManager->selectedRow() != nullptr)
            m_keyframeManager->pasteKeyframes(m_rowManager->selectedRow()->rowTimeline());
    }

    QGraphicsScene::keyPressEvent(keyEvent);
}

void TimelineGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

void TimelineGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    int index = event->scenePos().y() / TimelineConstants::ROW_H;
    RowTree *row = m_rowManager->rowAt(index);

    if (row == nullptr)
        return;

    if (event->scenePos().x() > TimelineConstants::TREE_BOUND_W) { // timeline context menu
        RowTimelineContextMenu timelineContextMenu(row, m_keyframeManager, event);
        timelineContextMenu.exec(event->screenPos());
    } else { // tree context menu
        if (!row->isProperty()) {
            RowTreeContextMenu treeContextMenu(row);
            treeContextMenu.exec(event->screenPos());
        }
    }
}

bool TimelineGraphicsScene::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Delete) {
            QGraphicsScene::keyPressEvent(static_cast<QKeyEvent *>(event));
            event->accept();
            return true;
        }
        Q_FALLTHROUGH();

    default:
        return QGraphicsScene::event(event);
    }
}

void TimelineGraphicsScene::updateHoverStatus(const QPointF &scenePos)
{
    QGraphicsItem *item = itemAt(scenePos, QTransform());
    if (item != nullptr) {
        item = getItemBelowType(TimelineItem::TypePlayHead, item, scenePos);
        if (item->type() == TimelineItem::TypeRowTimeline) {
            RowTimeline *timelineItem = static_cast<RowTimeline *>(item);
            TimelineControlType controlType =
                    timelineItem->getClickedControl(scenePos);
            if (controlType == TimelineControlType::StartHandle
                    || controlType == TimelineControlType::EndHandle) {
                setMouseCursor(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);
            } else {
                resetMouseCursor();
            }
        }
    }
}

// Return next item below [type] item, or item itself
// Used at least for skipping PlayHead and RowTreeLabelItem
QGraphicsItem *TimelineGraphicsScene::getItemBelowType(TimelineItem::ItemType type,
                                                       QGraphicsItem *item,
                                                       const QPointF &scenePos)
{
    if (item->type() == type) {
        const QList<QGraphicsItem *> hoverItems = items(scenePos);
        if (hoverItems.size() > 1)
            return hoverItems.at(1);
    }
    return item;
}

// Getters
Ruler                 *TimelineGraphicsScene::ruler()           const { return m_ruler;           }
PlayHead              *TimelineGraphicsScene::playHead()        const { return m_playHead;        }
TreeHeader            *TimelineGraphicsScene::treeHeader()      const { return m_treeHeader;      }
RowMover              *TimelineGraphicsScene::rowMover()        const { return m_rowMover;        }
RowManager            *TimelineGraphicsScene::rowManager()      const { return m_rowManager;      }
QGraphicsWidget       *TimelineGraphicsScene::widgetRoot()      const { return m_widgetRoot;      }
KeyframeManager       *TimelineGraphicsScene::keyframeManager() const { return m_keyframeManager; }
QGraphicsLinearLayout *TimelineGraphicsScene::layoutTree()      const { return m_layoutTree;      }
