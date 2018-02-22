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
#include "Separator.h"
#include "PlayHead.h"
#include "RowTree.h"
#include "RowMover.h"
#include "RowTimeline.h"
#include "Separator.h"
#include "TimelineConstants.h"
#include "TimelineToolbar.h"
#include "SelectionRect.h"
#include "RowManager.h"
#include "KeyframeManager.h"
#include "Keyframe.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicswidget.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qmenu.h>
#include <QtGui/qevent.h>
#include <QtCore/qtimer.h>
#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtWidgets/qaction.h>

TimelineGraphicsScene::TimelineGraphicsScene(QGraphicsView *viewTimelineContent,
                                             TimelineWidget *parent)
    : QGraphicsScene (parent)
    , m_layoutRoot(new QGraphicsLinearLayout)
    , m_layoutLabels(new QGraphicsLinearLayout(Qt::Vertical))
    , m_layoutTimeline(new QGraphicsLinearLayout(Qt::Vertical))
    , m_separator(new Separator)
    , m_ruler(new Ruler)
    , m_playHead(new PlayHead(m_ruler))
    , m_selectionRect(new SelectionRect(m_ruler))
    , m_rowMover(new RowMover)
    , m_widget(parent)
    , m_viewTimelineContent(viewTimelineContent)
    , m_widgetRoot(new QGraphicsWidget)
    , m_rowManager(new RowManager(this, m_layoutLabels, m_layoutTimeline))
    , m_keyframeManager(new KeyframeManager(this))
{
    addItem(m_playHead);
    addItem(m_selectionRect);
    addItem(m_rowMover);

    m_rowMover->setVisible(false);
    m_rowMover->setRect(0, 0, TimelineConstants::LABELS_MIN_W, TimelineConstants::ROW_H-1);

    // TODO: remove
//    m_widgetRoot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    addItem(m_widgetRoot);

    m_layoutRoot->setSpacing(0);
    m_layoutRoot->setContentsMargins(0, 0, 0, 0);
    m_layoutRoot->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_widgetRoot->setLayout(m_layoutRoot);

    m_layoutLabels->setSpacing(0);
    m_layoutLabels->setContentsMargins(0, 0, 0, 0);
    m_layoutLabels->setMinimumWidth(TimelineConstants::LABELS_DEFAULT_W);
    m_layoutLabels->setMaximumWidth(TimelineConstants::LABELS_DEFAULT_W);
    m_layoutLabels->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    setTimelineScale(m_ruler->timelineScale()); // refresh timeline width
    m_layoutTimeline->setSpacing(0);
    m_layoutTimeline->setContentsMargins(0, 0, 0, 0);
    m_layoutTimeline->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    m_layoutRoot->addItem(m_layoutLabels);
    m_layoutRoot->addItem(m_separator);
    m_layoutRoot->addItem(m_layoutTimeline);

    m_layoutLabels->addItem(new TreeHeader);
    m_layoutTimeline->addItem(m_ruler);

    QTimer::singleShot(0, this, [this]() {
        m_playHead->setPosition(0);
    });

    connect(m_ruler, &Ruler::rulerClicked, [this](const double &posX) {
        m_playHead->setPosition(posX);
        m_widget->toolbar()->setTime(m_playHead->time());
    });

    m_sceneRow = m_rowManager->createRow(RowType::Scene);

//    add some test rows
    // TODO: remove after connecting the view to the app data model
    RowTree *layer1 = m_rowManager->createRow(RowType::Layer, m_sceneRow, tr("layer 1"));
    RowTree *layer2 = m_rowManager->createRow(RowType::Layer, m_sceneRow, tr("layer 2"));

    RowTree *obj1  = m_rowManager->createRow(RowType::Object, layer1, tr("Cone"));
    RowTree *group1= m_rowManager->createRow(RowType::Group, layer1, tr("group 1"));
    RowTree *cam   = m_rowManager->createRow(RowType::Camera, layer1, tr("cam 1"));
    RowTree *light = m_rowManager->createRow(RowType::Light, layer2);
    RowTree *obj2  = m_rowManager->createRow(RowType::Object, layer1, tr("Cube"));

    RowTree *alias = m_rowManager->createRow(RowType::Alias, layer2);
    RowTree *comp  = m_rowManager->createRow(RowType::Component, layer2);
    RowTree *group2= m_rowManager->createRow(RowType::Group, comp, tr("group 2"));
    RowTree *obj3  = m_rowManager->createRow(RowType::Object, comp, tr("Sphere"));
    RowTree *cam2  = m_rowManager->createRow(RowType::Camera, group2, tr("cam 2"));
    RowTree *obj4  = m_rowManager->createRow(RowType::Object, group2, tr("Cylinder"));
    RowTree *text  = m_rowManager->createRow(RowType::Text, group2);

    // properties
    RowTree *prop1 = m_rowManager->getOrCreatePropertyRow(PropertyType::Scale, cam);
    RowTree *prop2 = m_rowManager->getOrCreatePropertyRow(PropertyType::Opacity, obj2);
    RowTree *prop3 = m_rowManager->getOrCreatePropertyRow(PropertyType::Position, obj3);
    RowTree *prop4 = m_rowManager->getOrCreatePropertyRow(PropertyType::Rotation, obj3);
    RowTree *prop5 = m_rowManager->getOrCreatePropertyRow(PropertyType::Rotation, obj3);

    // keyframes
    m_keyframeManager->insertKeyframe(prop1->rowTimeline(), 5, 7);
    m_keyframeManager->insertKeyframe(prop1->rowTimeline(), 3, 52);
    m_keyframeManager->insertKeyframe(prop2->rowTimeline(), 12, 34);
    m_keyframeManager->insertKeyframe(prop3->rowTimeline(), 4.5, 6);
    m_keyframeManager->deselectAllKeyframes();
}

void TimelineGraphicsScene::setTimelineScale(int scl)
{
    m_ruler->setTimelineScale(scl);
    m_playHead->updatePosition();

    m_layoutTimeline->setMinimumWidth(TimelineConstants::RULER_EDGE_OFFSET * 2
                                      + m_ruler->duration() * TimelineConstants::RULER_SEC_W * scl);
    m_layoutTimeline->setMaximumWidth(TimelineConstants::RULER_EDGE_OFFSET * 2
                                      + m_ruler->duration() * TimelineConstants::RULER_SEC_W * scl);

    for (int i = 1; i < m_layoutTimeline->count(); i++)
        static_cast<RowTimeline *>(m_layoutTimeline->itemAt(i)->graphicsItem())->updatePosition();
}

void TimelineGraphicsScene::addNewLayer()
{
    RowTree *newLayer = m_rowManager->createRow(RowType::Layer, m_sceneRow);

    m_rowManager->selectRow(newLayer);

    // scroll to top
    m_viewTimelineContent->verticalScrollBar()->setValue(0);
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
    // TODO: test code, to be removed
//    MainRowLabel *row_i = nullptr;
//    for (int i = 1; i < m_layoutLabels->count(); ++i)
//    {
//        row_i = static_cast<MainRowLabel *>(m_layoutLabels->itemAt(i)->graphicsItem());
//        qDebug().noquote().nospace() << "|" << QString("-").repeated(row_i->depth()) << row_i->label();
//    }
//    qDebug() << "------------------------------";

    debugPrintRows(m_sceneRow);

//    m_rowManager->deleteRow(m_rowManager->selectedRow());
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

        // TODO: remove
//    qDebug() << "sourceIndex=" << sourceIndex << ", targetIndex=" << targetIndex << ", rowSrcDepth=" << rowSrcDepth;

    // gather the rows to be moved
    RowTree *row_i = nullptr;
    QList<RowTree *> itemsToMove { m_rowMover->sourceRow() };
    for (int i = sourceIndex + 1; i < m_layoutLabels->count();) {
        row_i = static_cast<RowTree *>(m_layoutLabels->itemAt(i)->graphicsItem());

        // TODO: remove
//      qDebug() << "i=" << i << ", row_i->depth()=" << row_i->depth();

        if (row_i->depth() <= rowSrcDepth)
            break;

        m_layoutLabels->removeAt(i);
        m_layoutTimeline->removeAt(i);
        itemsToMove.append(row_i);
    }

    m_rowMover->insertionParent()->addChild(m_rowMover->sourceRow());

    // TODO: remove
//  qDebug() << "itemsToMove.count()=" << itemsToMove.count();

    // commit the move
    if (m_rowMover->movingDown())
        targetIndex -= itemsToMove.count();

    for (auto child : qAsConst(itemsToMove)) {
        ++targetIndex;
        m_layoutLabels->insertItem(targetIndex, child);
        m_layoutTimeline->insertItem(targetIndex, child->rowTimeline());
    }
}

void TimelineGraphicsScene::getLastChildRow(RowTree *row, int index, RowTree *outLastChild,
                                            RowTree *outNextSibling, int &outLastChildIndex) const
{
    if (row != nullptr) {
        RowTree *row_i = nullptr;
        RowTree *row_i_prev = nullptr;
        for (int i = index + 1; i < m_layoutLabels->count() - 1; ++i) {
            row_i = static_cast<RowTree *>(m_layoutLabels->itemAt(i)->graphicsItem());

            if (row_i->depth() <= row->depth()) {
                outLastChild = row_i_prev;
                outNextSibling =  row_i;
                outLastChildIndex =  i-1;
                return;
            }

            row_i_prev = row_i;
        }
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
    if (index < m_layoutLabels->count() - 1)
        index ++;

    return static_cast<RowTree *>(m_layoutLabels->itemAt(index)->graphicsItem())->depth();
}

bool TimelineGraphicsScene::validLayerMove(RowTree *rowAtIndex, RowTree *nextRowAtIndex)
{
    // we don't care about non-layers in this method
    if (m_rowMover->sourceRow()->rowType() != RowType::Layer)
        return true;

    if (rowAtIndex->rowType() == RowType::Scene)
        return true;

    if (rowAtIndex->rowType() == RowType::Layer)
       return rowAtIndex->empty() || !rowAtIndex->expanded();

    if (nextRowAtIndex == nullptr || (nextRowAtIndex->depth() <= rowAtIndex->depth()
                                     && nextRowAtIndex->depth() == 2))
        return true;

    return false;
}

void TimelineGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_pressPos = event->scenePos();
        QGraphicsItem *item = itemAt(m_pressPos, QTransform());
        if (item != nullptr) {
            // select next item below playhead
            if (item->type() == TimelineItem::TypePlayHead)
                item = items(m_pressPos).at(1);

            if (item->type() == TimelineItem::TypeSeparator) {
                m_separatorPressed = true;
            } else if (item->type() == TimelineItem::TypeRuler) {
                m_rulerPressed = true;
            } else if (item->type() == TimelineItem::TypeRowTree) {
                RowTree *rowLabel = static_cast<RowTree *>(item);

                if (rowLabel->rowType() != RowType::Property) {
                    if (!rowLabel->handleButtonsClick(event)) {
                        // dragging layers to reorder
                        int index = event->scenePos().y() / TimelineConstants::ROW_H;
                        m_rowManager->correctIndex(index);

                        if (rowLabel->rowType() != RowType::Scene
                                && rowLabel->rowType() != RowType::Property) {
                            m_rowMover->start(rowLabel, index);
                        }
                    }
                }
            } else if (item->type() == TimelineItem::TypeRowTimeline) {
                m_editedTimelineRow = static_cast<RowTimeline *>(item);
                Keyframe *keyframe = m_editedTimelineRow->getClickedKeyframe(m_pressPos);
                if (keyframe != nullptr) {  // pressed a keyframe
                    const bool ctrlKeyDown = event->modifiers() & Qt::ControlModifier;

                    if (ctrlKeyDown && keyframe->selected) {
                        m_keyframeManager->deselectKeyframe(keyframe);
                    } else {
                        if (!ctrlKeyDown && !keyframe->selected)
                            m_keyframeManager->deselectAllKeyframes();

                        m_keyframeManager->selectKeyframe(keyframe);
                        m_keyframePressed = true;
                    }
                } else {
                    m_keyframeManager->deselectAllKeyframes();
                    m_clickedTimelineControlType =
                            m_editedTimelineRow->getClickedControl(m_pressPos);

                    // clicked an empty spot on a timeline row, start selection rect.
                    if (m_editedTimelineRow->getClickedControl(m_pressPos) == RowTimeline::TypeNone)
                        m_selectionRect->start(m_pressPos);
                }
            }
        } else {
            if (m_pressPos.x() > m_separator->x() && m_pressPos.y() > TimelineConstants::ROW_H)
                m_selectionRect->start(m_pressPos);
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void TimelineGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (qAbs(event->scenePos().x() - m_pressPos.x()) > 10
            || qAbs(event->scenePos().y() - m_pressPos.y()) > 10)
        m_dragging = true;

    if (m_rulerPressed) {
        m_playHead->setPosition(event->scenePos().x() - m_ruler->pos().x());
        m_widget->toolbar()->setTime(m_playHead->time());
    } else if (m_dragging) {
        if (m_separatorPressed) { // resizing labels part
            double x = event->scenePos().x() - m_separator->size().width() * .5;
            x = qBound(TimelineConstants::LABELS_MIN_W, x, TimelineConstants::LABELS_MAX_W);
            m_layoutLabels->setMinimumWidth(x);
            m_layoutLabels->setMaximumWidth(x);
            m_rowMover->setRect(0, -5, x, 10);

            m_playHead->updatePosition();
        } else if (m_clickedTimelineControlType == RowTimeline::TypeStartHandle) {
            // resizing layer timline duration from left
            m_editedTimelineRow->setStartX(event->scenePos().x() - m_ruler->pos().x());
        } else if (m_clickedTimelineControlType == RowTimeline::TypeEndHandle) {
            // resizing layer timline duration from right
            m_editedTimelineRow->setEndX(event->scenePos().x() - m_ruler->pos().x());
        } else if (m_clickedTimelineControlType == RowTimeline::TypeDuration) {
            // moving layer timline duration
            m_editedTimelineRow->moveDurationBy(event->scenePos().x() - m_pressPos.x());
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
            RowTree *lastChildAtIndex; // so far not used

            if (valid) { // valid row index
                rowAtIndex = static_cast<RowTree *>(m_layoutLabels->itemAt(index)->graphicsItem());
                nextRowAtIndex = index > m_layoutLabels->count() - 2 ? nullptr :
                             static_cast<RowTree *>(m_layoutLabels->itemAt(index + 1)->graphicsItem());

                if (!rowAtIndex->expanded())
                    getLastChildRow(rowAtIndex, index, lastChildAtIndex, nextRowAtIndex, index);

                        // not moving an ancestor into a decendent
                valid = !rowAtIndex->isDecendentOf(m_rowMover->sourceRow())

                        // not inserting next to property rows
                         && !(nextRowAtIndex != nullptr
                                && nextRowAtIndex->rowType() == RowType::Property)

                        // not  inserting as a first child of self
                        && !(rowAtIndex == m_rowMover->sourceRow() && !rowAtIndex->empty())

                        // not  inserting non-layer under the scene
                        && !(m_rowMover->sourceRow()->rowType() != RowType::Layer
                                && rowAtIndex->rowType() == RowType::Scene)

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
                } else if (rowAtIndex->rowType() == RowType::Property) {
                    depth--;  // Property: prevent insertion as a sibling
                }

                depthBasedOnX = qMax(depthBasedOnX, depthNextRow);
                depth = qBound(3, depth, depthBasedOnX);

                if (m_rowMover->sourceRow()->rowType() == RowType::Layer)
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
            else if (scenePos.x() > m_ruler->durationEndX())
                scenePos.setX(m_ruler->durationEndX());

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

        if (item != nullptr && !m_dragging && (item->type() == TimelineItem::TypeRowTree
                                            || item->type() == TimelineItem::TypeRowTimeline)) {
            // select row
            if (item->type() == TimelineItem::TypeRowTree)
                m_rowManager->selectRow( static_cast<RowTree *>(item) );
            else if (item->type() == TimelineItem::TypeRowTimeline)
                m_rowManager->selectRow( static_cast<RowTimeline *>(item)->rowTree() );
        } else if (m_rowMover->isActive()) { // moving rows (reorder/reparent)
            if (m_rowMover->insertionParent() != nullptr) // valid row move, commit it
                commitMoveRows();
        }

        // reset mouse drag params
        m_selectionRect->end();
        m_rowMover->end();
        m_separatorPressed = false;
        m_rulerPressed = false;
        m_dragging = false;
        m_clickedTimelineControlType = RowTimeline::TypeNone;
        m_keyframePressed = false;
        m_editedTimelineRow = nullptr;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void TimelineGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
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
    QMenu contextMenu;

    int index = event->scenePos().y() / TimelineConstants::ROW_H;

    RowTimeline *row = rowManager()->rowTimelineAt(index);

    if (row != nullptr) { // timeline context menu
        Keyframe *keyframe = row->getClickedKeyframe(event->scenePos());
        bool propRow = row->rowTree()->rowType() == RowType::Property;
        bool hasPropRows = row->rowTree()->hasPropertyChildren();
        bool ctrlPressed = event->modifiers() & Qt::ControlModifier;

        //TODO: remove
//      qDebug() << "index=" << index;
//      qDebug() << "keyframe=" << (keyframe != nullptr);
//      qDebug() << "propRow=" << propRow;
//      qDebug() << "hasPropRows=" << hasPropRows;
//      qDebug() << "---------------------";

        if (keyframe) {
            if (!keyframe->selected && !ctrlPressed)
                m_keyframeManager->deselectAllKeyframes();

            m_keyframeManager->selectKeyframe(keyframe);
        } else {
            m_keyframeManager->deselectAllKeyframes();
        }

        auto actionInsertKeyframe = contextMenu.addAction(QObject::tr("Insert Keyframe"));
        auto actionCutSelectedKeyframes =
                contextMenu.addAction(QObject::tr("Cut Selected Keyframe"));
        auto actionCopySelectedKeyframes =
                contextMenu.addAction(QObject::tr("Copy Selected Keyframe"));
        auto actionPasteKeyframes = contextMenu.addAction(QObject::tr("Paste Keyframes"));
        auto actionDeleteSelectedKeyframes =
                contextMenu.addAction(QObject::tr("Delete Selected Keyframe"));
        auto actionDeleteRowKeyframes =
                contextMenu.addAction(QObject::tr(propRow ? "Delete All Property Keyframes"
                                                          : "Delete All Channel Keyframes"));

        actionInsertKeyframe         ->setEnabled(!keyframe && (propRow || hasPropRows));
        actionCutSelectedKeyframes   ->setEnabled(m_keyframeManager->oneMasterRowSelected());
        actionCopySelectedKeyframes  ->setEnabled(m_keyframeManager->oneMasterRowSelected());
        actionPasteKeyframes         ->setEnabled(m_keyframeManager->hasCopiedKeyframes());
        actionDeleteSelectedKeyframes->setEnabled(m_keyframeManager->hasSelectedKeyframes());
        actionDeleteRowKeyframes     ->setEnabled(!row->keyframes().empty());

        // connections
        connect(actionInsertKeyframe, &QAction::triggered, this, [=]() {
            m_keyframeManager->insertKeyframe(row, m_playHead->time(), 0);
        });

        connect(actionCutSelectedKeyframes, &QAction::triggered, this, [=]() {
            m_keyframeManager->copySelectedKeyframes();
            m_keyframeManager->deleteSelectedKeyframes();
        });

        connect(actionCopySelectedKeyframes, &QAction::triggered, this, [=]() {
            m_keyframeManager->copySelectedKeyframes();
        });

        connect(actionPasteKeyframes, &QAction::triggered, this, [=]() {
            m_keyframeManager->pasteKeyframes(row);
        });

        connect(actionDeleteSelectedKeyframes, &QAction::triggered, this, [=]() {
            m_keyframeManager->deleteSelectedKeyframes();
        });

        connect(actionDeleteRowKeyframes, &QAction::triggered, this, [=]() {
            m_keyframeManager->deleteKeyframes(row);
        });
    }

    contextMenu.exec(event->screenPos());
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

Ruler           *TimelineGraphicsScene::ruler()           const { return m_ruler;           }
PlayHead        *TimelineGraphicsScene::playHead()        const { return m_playHead;        }
Separator       *TimelineGraphicsScene::separator()       const { return m_separator;       }
RowManager      *TimelineGraphicsScene::rowManager()      const { return m_rowManager;      }
QGraphicsWidget *TimelineGraphicsScene::widgetRoot()      const { return m_widgetRoot;      }
KeyframeManager *TimelineGraphicsScene::keyframeManager() const { return m_keyframeManager; }
