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
#include "TimelineControl.h"
#include "RowTreeContextMenu.h"
#include "RowTimelineContextMenu.h"
#include "StudioPreferences.h"
#include "TimeEditDlg.h"
#include "StudioClipboard.h"

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
    , m_rowMover(new RowMover(this))
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

    m_treeHeader = new TreeHeader(this);

    m_layoutTree->addItem(m_treeHeader);
    m_layoutTimeline->addItem(m_ruler);

    QTimer::singleShot(0, this, [this]() {
        m_playHead->setPosition(0);
        m_widgetTimeline->viewTreeContent()->horizontalScrollBar()->setValue(0);
    });

    QAction *action = new QAction(this);
    action->setShortcut(Qt::Key_S);
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleInsertKeyframe);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_K));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this,
            &TimelineGraphicsScene::handleDeleteChannelKeyframes);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_T));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleSetTimeBarTime);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_G));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleMakeComponent);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_G));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleEditComponent);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleCopyObjectPath);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_H));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleShySelected);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_H));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, this, &TimelineGraphicsScene::handleLockSelected);
    timelineWidget->addAction(action);

    // TODO: Shortcuts for these to be determined
//    action = new QAction(this);
//    action->setShortcut(QKeySequence(TBD));
//    action->setShortcutContext(Qt::ApplicationShortcut);
//    connect(action, &QAction::triggered, m_treeHeader, &TreeHeader::toggleFilterShy);
//    timelineWidget->addAction(action);

//    action = new QAction(this);
//    action->setShortcut(QKeySequence(TBD));
//    action->setShortcutContext(Qt::ApplicationShortcut);
//    connect(action, &QAction::triggered, m_treeHeader, &TreeHeader::toggleFilterVisible);
//    timelineWidget->addAction(action);

//    action = new QAction(this);
//    action->setShortcut(QKeySequence(TBD));
//    action->setShortcutContext(Qt::ApplicationShortcut);
//    connect(action, &QAction::triggered, m_treeHeader, &TreeHeader::toggleFilterLock);
//    timelineWidget->addAction(action);
}

void TimelineGraphicsScene::setTimelineScale(int scl)
{
    m_ruler->setTimelineScale(scl);
    m_playHead->updatePosition();
    updateTimelineLayoutWidth();

    for (int i = 1; i < m_layoutTimeline->count(); i++)
        static_cast<RowTimeline *>(m_layoutTimeline->itemAt(i)->graphicsItem())->updatePosition();
}

void TimelineGraphicsScene::updateTimelineLayoutWidth()
{
    double timelineWidth = TimelineConstants::RULER_EDGE_OFFSET * 2
                           + m_ruler->maxDuration() * TimelineConstants::RULER_SEC_W
                           * m_ruler->timelineScale();

    m_layoutTimeline->setMinimumWidth(timelineWidth);
    m_layoutTimeline->setMaximumWidth(timelineWidth);
    // Mahmoud_TODO: could be requested by UX. else will be removed
//    m_widgetTimeline->viewTimelineContent()->horizontalScrollBar()->setValue(
//               m_widgetTimeline->viewTimelineContent()->horizontalScrollBar()->maximum());

//    if (m_editedTimelineRow)
//        m_widgetTimeline->viewTimelineContent()->ensureVisible(m_editedTimelineRow);
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
    // same place, abort
    if ((m_rowMover->sourceRow()->index() == m_rowMover->targetIndex())
        && m_rowMover->sourceRow()->parentRow() == m_rowMover->insertionParent()) {
        return;
    }

    // handle for the moving row
    qt3dsdm::Qt3DSDMInstanceHandle handleSource = static_cast<Qt3DSDMTimelineItemBinding *>
            (m_rowMover->sourceRow()->getBinding())->GetInstance();

    // handle for the parent of the insertion row
    auto parentBinding = static_cast<Qt3DSDMTimelineItemBinding *>
            (m_rowMover->insertionParent()->getBinding());
    qt3dsdm::Qt3DSDMInstanceHandle handleParent = parentBinding->GetInstance();

    // commit the row move to the binding
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), QObject::tr("Reorder Rows"))
        ->ReorderRows(handleSource, handleParent,
                      parentBinding->convertIndex(m_rowMover->targetIndex(), false));

    if (!m_rowMover->insertionParent()->expanded())
        m_rowMover->insertionParent()->updateExpandStatus(RowTree::ExpandState::Expanded, false);

    // updating the UI happens in TimelineWidget.onChildAdded()
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

void TimelineGraphicsScene::updateTreeWidth(double treeWidth)
{
    if (m_treeWidth != treeWidth) {
        m_treeWidth = treeWidth;
        update();
    }
}

double TimelineGraphicsScene::treeWidth() const
{
    return m_treeWidth;
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
        if (item) {
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
                    if (m_clickedTreeControlType != TreeControlType::None)
                        m_rowManager->updateFiltering(rowTree);
                    else if (rowTree->draggable())
                        m_rowMover->start(rowTree);
                }
            } else if (item->type() == TimelineItem::TypeRowTimeline) {
                m_editedTimelineRow = static_cast<RowTimeline *>(item);
                Keyframe *keyframe = m_editedTimelineRow->getClickedKeyframe(m_pressPos);
                if (keyframe) {  // pressed a keyframe
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

                        m_pressPosInKeyframe = (m_pressPos.x() - m_ruler->x())
                                - (TimelineConstants::RULER_EDGE_OFFSET
                                   + m_ruler->timeToDistance(keyframe->time));
                        m_keyframePressed = true;
                    }
                } else {
                    m_keyframeManager->deselectAllKeyframes();
                    m_clickedTimelineControlType =
                            m_editedTimelineRow->getClickedControl(m_pressPos);

                    // clicked an empty spot on a timeline row, start selection rect.
                    if (m_clickedTimelineControlType == TimelineControlType::None)
                        m_selectionRect->start(m_pressPos);
                    else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
                        m_editedTimelineRow->startDurationMove(
                            // click position in ruler space
                            m_pressPos.x() - m_ruler->x());
                    }
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

    bool shift = event->modifiers() & Qt::ShiftModifier;
    if (m_rulerPressed) {
        double distance = event->scenePos().x() - m_ruler->x();
        if (shift)
            snap(distance, false);
        distance -= TimelineConstants::RULER_EDGE_OFFSET;
        long time = m_ruler->distanceToTime(distance) * 1000;
        if (time < 0)
            time = 0;
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(time);
    } else if (m_dragging) {
        if (m_clickedTimelineControlType == TimelineControlType::StartHandle) {
            // resizing layer timline duration from left
            double distance = event->scenePos().x() - m_ruler->pos().x();
            if (shift)
                snap(distance);
            m_editedTimelineRow->setStartX(distance);
        } else if (m_clickedTimelineControlType == TimelineControlType::EndHandle) {
            // resizing layer timline duration from right
            double distance = event->scenePos().x() - m_ruler->pos().x();
            if (shift)
                snap(distance);
            m_editedTimelineRow->setEndX(distance);
            rowManager()->updateRulerDuration();
        } else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
            // moving layer timeline duration
            double newX = event->scenePos().x() - m_editedTimelineRow->getDurationMoveOffsetX()
                    - m_ruler->x();
            if (shift)
                snap(newX);
            m_editedTimelineRow->moveDurationTo(newX);
            rowManager()->updateRulerDuration();
        } else if (m_selectionRect->isActive()) {
            // resizing keyframe selection rect
            m_selectionRect->updateSize(event->scenePos());
            m_keyframeManager->selectKeyframesInRect(m_selectionRect->rect());
        } else if (m_rowMover->isActive() && m_rowManager->isSingleSelected()) {
            // moving rows vertically (reorder/reparent)
            // collapse all properties so correctIndex() counts correctly
            m_rowManager->collapseAllPropertyRows();
            m_rowMover->updateTargetRow(event->scenePos());
        } else if (m_keyframePressed) { // moving selected keyframes
            double newX = event->scenePos().x() - m_ruler->x() - m_pressPosInKeyframe;

            if (newX < TimelineConstants::RULER_EDGE_OFFSET)
                newX = TimelineConstants::RULER_EDGE_OFFSET;
            if (shift)
                snap(newX);
            newX += m_ruler->x() + m_pressPosInKeyframe;
            double dx = newX - m_pressPos.x();
            m_keyframeManager->moveSelectedKeyframes(dx);

            m_pressPos.setX(newX);
        }
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void TimelineGraphicsScene::updateSnapSteps() {
    m_snapSteps.clear();
    // i = 1 is always the scene row
    for (int i = 2; i < m_layoutTimeline->count(); i++) {
        RowTree *rowTree = static_cast<RowTree *>
                (m_layoutTree->itemAt(i)->graphicsItem());
        if (rowTree->hasDurationBar() && rowTree->expanded()) {
            if (!m_snapSteps.contains(rowTree->rowTimeline()->getStartX()))
                m_snapSteps.push_back(rowTree->rowTimeline()->getStartX());

            if (!m_snapSteps.contains(rowTree->rowTimeline()->getEndX()))
                m_snapSteps.push_back(rowTree->rowTimeline()->getEndX());

            // add keyframes times
            if (rowTree->hasPropertyChildren()) {
                const QList<Keyframe *> keyframes = rowTree->rowTimeline()->keyframes();
                for (Keyframe *k : keyframes) {
                    double kX = m_ruler->timeToDistance(k->time)
                            + TimelineConstants::RULER_EDGE_OFFSET;
                    if (!m_snapSteps.contains(kX))
                        m_snapSteps.push_back(kX);
                }
            }
        }
    }
}

TExpandMap &TimelineGraphicsScene::expandMap()
{
    return m_expandMap;
}

void TimelineGraphicsScene::snap(double &value, bool snapToPlayHead)
{
    // snap to play head
    if (snapToPlayHead) {
        double playHeadX = m_playHead->x() - m_ruler->x();
        if (abs(value - playHeadX) < CStudioPreferences::GetSnapRange()) {
            value = playHeadX;
            return;
        }
    }

    // duration edges snap
    for (double v : qAsConst(m_snapSteps)) {
        if (abs(value - v) < CStudioPreferences::GetSnapRange()) {
            value = v;
            return;
        }
    }

    // time steps snap
    if (CStudioPreferences::IsTimelineSnappingGridActive()) {
        double snapStep = TimelineConstants::RULER_SEC_W * m_ruler->timelineScale();
        if (CStudioPreferences::GetTimelineSnappingGridResolution() == SNAPGRID_HALFSECONDS)
            snapStep *= .5;
        else if (CStudioPreferences::GetTimelineSnappingGridResolution() == SNAPGRID_TICKMARKS)
            snapStep *= .1;

        double snapValue = TimelineConstants::RULER_EDGE_OFFSET
            + round((value - TimelineConstants::RULER_EDGE_OFFSET) / snapStep) * snapStep;
        if (abs(value - snapValue) < CStudioPreferences::GetSnapRange())
            value = snapValue;
    }
}

void TimelineGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    const bool ctrlKeyDown = event->modifiers() & Qt::ControlModifier;
    if (event->button() == Qt::LeftButton) {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());

        if (item && !m_dragging) {
            item = getItemBelowType(TimelineItem::TypeRowTreeLabelItem, item, m_pressPos);
            item = getItemBelowType(TimelineItem::TypePlayHead, item, m_pressPos);
            // select pressed row
            RowTree *rowTree = nullptr;
            if (item->type() == TimelineItem::TypeRowTree)
                rowTree = static_cast<RowTree *>(item);
            else if (item->type() == TimelineItem::TypeRowTimeline)
                rowTree = static_cast<RowTimeline *>(item)->rowTree();

            if (rowTree && m_clickedTreeControlType == TreeControlType::None
                    && !rowTree->locked()) {
                m_rowManager->selectRow(rowTree, ctrlKeyDown);
            }

        } else if (m_rowMover->isActive()) { // moving rows (reorder/reparent)
            if (m_rowMover->insertionParent()) // valid row move, commit it
                commitMoveRows();
        } else if (m_keyframePressed) {
            // update keyframe movement (time) to binding
            m_keyframeManager->commitMoveSelectedKeyframes();
            updateSnapSteps();
        } else if (m_clickedTimelineControlType == TimelineControlType::StartHandle) {
            ITimelineTimebar *timebar = m_editedTimelineRow->rowTree()->getBinding()
                    ->GetTimelineItem()->GetTimebar();
            timebar->ChangeTime(m_editedTimelineRow->getStartTime() * 1000, true);
            timebar->CommitTimeChange();
            updateSnapSteps();
        } else if (m_clickedTimelineControlType == TimelineControlType::EndHandle) {
            ITimelineTimebar *timebar = m_editedTimelineRow->rowTree()->getBinding()
                    ->GetTimelineItem()->GetTimebar();
            timebar->ChangeTime(m_editedTimelineRow->getEndTime() * 1000, false);
            timebar->CommitTimeChange();
            updateSnapSteps();
            if (m_playHead->time() > ruler()->duration())
                g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(ruler()->duration() * 1000);
        } else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
            ITimelineTimebar *timebar = m_editedTimelineRow->rowTree()->getBinding()
                    ->GetTimelineItem()->GetTimebar();
            timebar->OffsetTime(m_editedTimelineRow->getDurationMoveTime() * 1000);
            timebar->CommitTimeChange();
            updateSnapSteps();
            if (m_playHead->time() > ruler()->duration())
                g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(ruler()->duration() * 1000);
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
    if (item) {
        QGraphicsItem *itemBelowPlayhead =
                getItemBelowType(TimelineItem::TypePlayHead, item, scenePos);
        if (item->type() == TimelineItem::TypeRuler
                || itemBelowPlayhead->type() == TimelineItem::TypeRuler) {
            CDoc *doc = g_StudioApp.GetCore()->GetDoc();
            CTimeEditDlg timeEditDlg;
            timeEditDlg.showDialog(doc->GetCurrentViewTime(), doc, PLAYHEAD);
            return;
        }

        item = itemBelowPlayhead;
        if (item->type() == TimelineItem::TypeRowTree) {
            RowTree *treeItem = static_cast<RowTree *>(item);
            if (treeItem->isProperty())
                treeItem->togglePropertyExpanded();
        } else if (item->type() == TimelineItem::TypeRowTreeLabelItem) {
            RowTreeLabelItem *treeLabelItem = static_cast<RowTreeLabelItem *>(item);
            if (treeLabelItem->parentRow()->isProperty()) {
                treeLabelItem->parentRow()->togglePropertyExpanded();
            } else {
                // Tree labels text can be edited with double-click
                treeLabelItem->setEnabled(true);
                treeLabelItem->setFocus();
            }
        } else if (item->type() == TimelineItem::TypeRowTimeline) {
            RowTimeline *rowTimeline = static_cast<RowTimeline *>(item);
            Keyframe *clickedKeyframe = rowTimeline->getClickedKeyframe(scenePos);
            if (clickedKeyframe) {
                CDoc *doc = g_StudioApp.GetCore()->GetDoc();
                CTimeEditDlg timeEditDlg;
                timeEditDlg.setKeyframesManager(m_keyframeManager);
                timeEditDlg.showDialog(clickedKeyframe->time * 1000, doc, ASSETKEYFRAME);
            } else {
                handleSetTimeBarTime();
            }
        }
    }

    QGraphicsScene::mouseDoubleClickEvent(event);
}

void TimelineGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    // Eat left/right arrow keys on tree side unless some item (e.g. label) has focus
    if ((keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right)
            && (qApp->focusObject() == m_widgetTimeline->viewTreeContent() && !focusItem())) {
        keyEvent->accept();
        return;
    }
    if (keyEvent->key() == Qt::Key_Delete)
        g_StudioApp.DeleteSelectedObject(); // Despite the name, this deletes objects and keyframes

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

    // Internally some things like make component depend on the correct row being selected,
    // so make sure it is.
    m_rowManager->selectRow(row);
    if (event->scenePos().x() > TimelineConstants::TREE_BOUND_W) { // timeline context menu
        RowTimelineContextMenu timelineContextMenu(row, m_keyframeManager, event,
                                                   m_timelineControl);
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
    if (item) {
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

void TimelineGraphicsScene::handleInsertKeyframe()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow)
        selectedRow->getBinding()->InsertKeyframe();
}

void TimelineGraphicsScene::handleDeleteChannelKeyframes()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow)
        selectedRow->getBinding()->DeleteAllChannelKeyframes();
}

void TimelineGraphicsScene::handleSetTimeBarTime()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow && selectedRow->hasDurationBar()) {
        m_timelineControl->setRowTimeline(selectedRow->rowTimeline());
        m_timelineControl->showDurationEditDialog();
    }
}

void TimelineGraphicsScene::handleMakeComponent()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow) {
        selectedRow->getBinding()->PerformTransaction(
                    ITimelineItemBinding::EUserTransaction_MakeComponent);
    }
}

void TimelineGraphicsScene::handleCopyObjectPath()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow) {
        CStudioClipboard::CopyTextToClipboard(
                    selectedRow->getBinding()->GetObjectPath().toQString());
    }
}

void TimelineGraphicsScene::handleEditComponent()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow && selectedRow->getBinding()->IsValidTransaction(
                ITimelineItemBinding::EUserTransaction_EditComponent)) {
        selectedRow->getBinding()->OpenAssociatedEditor();
    }
}

void TimelineGraphicsScene::handleShySelected()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow)
        selectedRow->toggleShy();
}

void TimelineGraphicsScene::handleLockSelected()
{
    RowTree *selectedRow = m_rowManager->selectedRow();
    if (selectedRow)
        selectedRow->toggleLocked();
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
QGraphicsLinearLayout *TimelineGraphicsScene::layoutTimeline()  const { return m_layoutTimeline;  }
TimelineWidget        *TimelineGraphicsScene::widgetTimeline()  const { return m_widgetTimeline;  }