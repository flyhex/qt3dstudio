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
#include "RowTimelinePropertyGraph.h"
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
#include "TimeEnums.h"
#include "StudioClipboard.h"
#include "Dialogs.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicswidget.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qaction.h>
#include <QtGui/qevent.h>
#include <QtCore/qtimer.h>
#include <QtCore/qglobal.h>
#include <QtWidgets/qaction.h>

static const QPointF invalidPoint(-999999.0, -999999.0);

TimelineGraphicsScene::TimelineGraphicsScene(TimelineWidget *timelineWidget)
    : QGraphicsScene(timelineWidget)
    , m_layoutRoot(new QGraphicsLinearLayout)
    , m_layoutTree(new QGraphicsLinearLayout(Qt::Vertical))
    , m_layoutTimeline(new QGraphicsLinearLayout(Qt::Vertical))
    , m_ruler(new Ruler)
    , m_playHead(new PlayHead(m_ruler))
    , m_widgetTimeline(timelineWidget)
    , m_widgetRoot(new QGraphicsWidget)
    , m_rowMover(new RowMover(this))
    , m_selectionRect(new SelectionRect())
    , m_rowManager(new RowManager(this, m_layoutTree, m_layoutTimeline))
    , m_keyframeManager(new KeyframeManager(this))
    , m_pressPos(invalidPoint)
    , m_pressScreenPos(invalidPoint)
    , m_timelineControl(new TimelineControl(this))
{
    addItem(m_playHead);
    addItem(m_selectionRect);
    addItem(m_rowMover);
    addItem(m_widgetRoot);

    m_timebarToolTip = new QLabel(m_widgetTimeline);
    m_timebarToolTip->setObjectName(QStringLiteral("timebarToolTip"));
    m_timebarToolTip->setWindowModality(Qt::NonModal);
    m_timebarToolTip->setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
    m_timebarToolTip->setContentsMargins(2, 2, 2, 2);

    m_variantsToolTip = new QLabel(m_widgetTimeline);
    m_variantsToolTip->setObjectName(QStringLiteral("variantsToolTip"));
    m_variantsToolTip->setWindowModality(Qt::NonModal);
    m_variantsToolTip->setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip
                                      | Qt::WindowTransparentForInput);
    m_variantsToolTip->setContentsMargins(2, 2, 2, 2);

    connect(qApp, &QApplication::focusChanged,
            this, &TimelineGraphicsScene::handleApplicationFocusLoss);

    m_rowMover->setVisible(false);

    m_autoScrollTimelineTimer.setInterval(10); // 10 ms

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

    // auto scrolling (when DnD is active and hovering on top or bottom of the tree list)
    connect(&m_autoScrollTimer, &QTimer::timeout, [this]() {
        QScrollBar *scrollbar = m_widgetTimeline->viewTreeContent()->verticalScrollBar();
        if (m_autoScrollUpOn)
            scrollbar->setValue(scrollbar->value() - TimelineConstants::AUTO_SCROLL_DELTA);
        else if (m_autoScrollDownOn)
            scrollbar->setValue(scrollbar->value() + TimelineConstants::AUTO_SCROLL_DELTA);
    });

    connect(&m_autoScrollTimelineTimer, &QTimer::timeout, [this]() {
        if (!qApp->focusWindow() && !g_StudioApp.isOnProgress()) {
            resetMousePressParams();
            return;
        }
        QGraphicsView *timelineContent = m_widgetTimeline->viewTimelineContent();
        const QPoint scrollBarOffsets(getScrollbarOffsets());
        const QRect contentRect = timelineContent->contentsRect();
        const double right = timelineContent->width() - scrollBarOffsets.x();
        QPoint p = m_widgetTimeline->mapFromGlobal(QCursor::pos())
                - QPoint(m_widgetTimeline->viewTreeContent()->width()
                         + TimelineConstants::SPLITTER_W, 0);

        // Limit the maximum scroll speed
        if (p.x() < 0) {
            p.setX(qMax(-TimelineConstants::TIMELINE_SCROLL_MAX_DELTA,
                        p.x() / TimelineConstants::TIMELINE_SCROLL_DIVISOR));
        } else if (p.x() > right) {
            p.setX(qMin(right + TimelineConstants::TIMELINE_SCROLL_MAX_DELTA,
                        right + 1 + ((p.x() - right)
                                     / TimelineConstants::TIMELINE_SCROLL_DIVISOR)));
        }

        if (m_selectionRect->isActive()) {
            p -= QPoint(0, m_widgetTimeline->navigationBar()->height() + TimelineConstants::ROW_H);
            const double bottom = timelineContent->contentsRect().height() - scrollBarOffsets.y();
            if (m_lastAutoScrollX != p.x() || p.x() <= 0 || p.x() >= right
                    || m_lastAutoScrollY != p.y() || p.y() <= 0 || p.y() >= bottom) {
                m_lastAutoScrollX = p.x();
                m_lastAutoScrollY = p.y();

                if (p.y() < 0) {
                    p.setY(qMax(-TimelineConstants::TIMELINE_SCROLL_MAX_DELTA,
                                p.y() / TimelineConstants::TIMELINE_SCROLL_DIVISOR));
                } else if (p.y() > bottom) {
                    p.setY(qMin(bottom + TimelineConstants::TIMELINE_SCROLL_MAX_DELTA,
                                bottom + 1 + ((p.y() - bottom)
                                              / TimelineConstants::TIMELINE_SCROLL_DIVISOR)));
                }

                // Resize keyframe selection rect
                const QPointF scenePoint = timelineContent->mapToScene(p);
                timelineContent->ensureVisible(scenePoint.x(), scenePoint.y(), 0, 0, 0, 0);
                QRectF visibleScene(
                            timelineContent->mapToScene(contentRect.topLeft()),
                            timelineContent->mapToScene(contentRect.bottomRight()
                                                        - scrollBarOffsets));
                m_selectionRect->updateSize(scenePoint, visibleScene);
                m_keyframeManager->selectKeyframesInRect(m_selectionRect->rect());
            }
        } else if (m_lastAutoScrollX != p.x() || p.x() <= 0 || p.x() >= right) {
            m_lastAutoScrollX = p.x();

            bool shift = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
            double scroll = timelineContent->horizontalScrollBar()->value();
            if (scroll != 0)
                scroll -= TimelineConstants::TREE_BOUND_W;

            double distance = p.x() + scroll - TimelineConstants::RULER_EDGE_OFFSET;
            if (m_clickedTimelineControlType == TimelineControlType::Duration
                    && !m_editedTimelineRow.isNull()) {
                distance -= m_editedTimelineRow->getDurationMoveOffsetX();
            }

            if (shift)
                snap(distance, !m_rulerPressed);

            if (m_rulerPressed) {
                long time = m_ruler->distanceToTime(distance);
                if (time < 0)
                    time = 0;
                g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(time);
            } else {
                if (m_editedTimelineRow.isNull()) {
                    resetMousePressParams();
                    return;
                }

                if (m_dragging) {
                    if (m_clickedTimelineControlType == TimelineControlType::DurationStartHandle) {
                        double visiblePtX = distance > 0 ? m_editedTimelineRow->getStartX() : 0;
                        if (distance > m_editedTimelineRow->getEndX())
                            visiblePtX += TimelineConstants::RULER_EDGE_OFFSET;

                        m_editedTimelineRow->setStartX(distance);
                        m_editedTimelineRow->showToolTip(QCursor::pos());
                        timelineContent->ensureVisible(TimelineConstants::TREE_BOUND_W
                                                       + TimelineConstants::RULER_EDGE_OFFSET
                                                       + visiblePtX,
                                                       m_editedTimelineRow->y(), 0, 0, 0, 0);
                    } else if (m_clickedTimelineControlType
                               == TimelineControlType::DurationEndHandle) {
                        long time = m_ruler->distanceToTime(distance);
                        double edgeMargin = 0;
                        if (time > TimelineConstants::MAX_SLIDE_TIME) {
                            distance = m_ruler->timeToDistance(TimelineConstants::MAX_SLIDE_TIME);
                            edgeMargin = TimelineConstants::RULER_EDGE_OFFSET;
                        } else if (time < m_editedTimelineRow->getStartTime()) {
                            edgeMargin = -TimelineConstants::RULER_EDGE_OFFSET;
                        }
                        m_editedTimelineRow->setEndX(distance);
                        m_editedTimelineRow->showToolTip(QCursor::pos());
                        rowManager()->updateRulerDuration(p.x() > right);
                        timelineContent->ensureVisible(
                                    TimelineConstants::TREE_BOUND_W
                                    + TimelineConstants::RULER_EDGE_OFFSET
                                    + m_editedTimelineRow->getEndX() + edgeMargin,
                                    m_editedTimelineRow->y(), 0, 0, 0, 0);
                    } else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
                        long time = m_ruler->distanceToTime(distance)
                                    + m_editedTimelineRow->getDuration(); // milliseconds
                        double visiblePtX = distance
                                            + m_editedTimelineRow->getDurationMoveOffsetX();
                        if (time > TimelineConstants::MAX_SLIDE_TIME) {
                            distance = m_ruler->timeToDistance(TimelineConstants::MAX_SLIDE_TIME
                                                               - m_editedTimelineRow->getDuration());
                            visiblePtX = m_editedTimelineRow->getEndX()
                                         + TimelineConstants::RULER_EDGE_OFFSET;
                        }

                        m_editedTimelineRow->moveDurationTo(distance);
                        m_editedTimelineRow->showToolTip(QCursor::pos());
                        rowManager()->updateRulerDuration(p.x() > right);
                        timelineContent->ensureVisible(
                                    TimelineConstants::TREE_BOUND_W
                                    + TimelineConstants::RULER_EDGE_OFFSET + visiblePtX,
                                    m_editedTimelineRow->y(), 0, 0, 0, 0);
                    }
                }
            }
        }
    });

    connect(&m_autoScrollTriggerTimer, &QTimer::timeout, [this]() {
        m_autoScrollTimer.start(TimelineConstants::AUTO_SCROLL_PERIOD);
    });

    QTimer::singleShot(0, this, [this]() {
        m_playHead->setTime(0);
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
    connect(action, &QAction::triggered, &g_StudioApp, &CStudioApp::toggleShy);
    timelineWidget->addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_H));
    action->setShortcutContext(Qt::ApplicationShortcut);
    connect(action, &QAction::triggered, &g_StudioApp, &CStudioApp::toggleLocked);
    timelineWidget->addAction(action);
}

TimelineGraphicsScene::~TimelineGraphicsScene()
{
    disconnect(qApp, &QApplication::focusChanged,
               this, &TimelineGraphicsScene::handleApplicationFocusLoss);
    delete m_dataInputSelector;
    delete m_keyframeManager;
}

void TimelineGraphicsScene::setTimelineScale(int scl)
{
    m_ruler->setTimelineScale(scl);
    m_playHead->updatePosition();
    updateTimelineLayoutWidth();

    for (int i = 1; i < m_layoutTimeline->count(); i++)
        static_cast<RowTimeline *>(m_layoutTimeline->itemAt(i)->graphicsItem())->updatePosition();
}

void TimelineGraphicsScene::setControllerText(const QString &controller)
{
    // check that we have scene/container root item at index 1
    if (m_layoutTimeline->count() < 2)
        return;

    RowTimeline *rt = static_cast<RowTimeline *>(m_layoutTimeline->itemAt(1)->graphicsItem());
    rt->setControllerText(controller);
}

void TimelineGraphicsScene::updateTimelineLayoutWidth()
{
    double timelineWidth = TimelineConstants::RULER_EDGE_OFFSET * 2
                           + m_ruler->timeToDistance(m_ruler->maxDuration());

    m_layoutTimeline->setMinimumWidth(timelineWidth);
    m_layoutTimeline->setMaximumWidth(timelineWidth);
}

void TimelineGraphicsScene::updateController()
{
    setControllerText(m_widgetTimeline->toolbar()->getCurrentController());
}

void TimelineGraphicsScene::commitMoveRows()
{
    if (!m_rowMover->insertionTarget()
            || m_rowMover->sourceRows().contains(m_rowMover->insertionTarget())) {
        return;
    }

    // handles for the moving rows
    qt3dsdm::TInstanceHandleList sourceHandles;
    const auto sourceRows = m_rowMover->sourceRows();
    for (auto sourceRow : sourceRows) {
        qt3dsdm::Qt3DSDMInstanceHandle handleSource = static_cast<Qt3DSDMTimelineItemBinding *>
                (sourceRow->getBinding())->GetInstance();
        sourceHandles.push_back(handleSource);
    }
    qt3dsdm::Qt3DSDMInstanceHandle handleTarget = static_cast<Qt3DSDMTimelineItemBinding *>
            (m_rowMover->insertionTarget()->getBinding())->GetInstance();

    if (!m_rowMover->insertionParent()->expanded())
        m_rowMover->insertionParent()->updateExpandStatus(RowTree::ExpandState::Expanded, false);

    // Remove sourcerows for items that will be deleted as result of RearrangeObjects,
    // f.ex objects that will be moved to a component; otherwise we try to update
    // timeline rows that no longer have valid scene objects linked to them.
    // Note that we remove all sourcerows that are being dragged currently, because they
    // all share the same drop target anyway.
    if (m_rowMover->shouldDeleteAfterMove()) {
        for (auto sourceRow : sourceRows)
            m_rowMover->removeSourceRow(sourceRow);
    }

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(),
                                      QObject::tr("Move Rows"))
            ->RearrangeObjects(sourceHandles, handleTarget, m_rowMover->insertionType());

    // updating the UI happens in TimelineWidget.onChildAdded()
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
    g_StudioApp.setLastActiveView(m_widgetTimeline);

    if ((event->modifiers() & Qt::AltModifier) && !m_dragging
            && m_timelineAltModifierMode == TimelineAltModifierMode::None) {
        if (event->button() == Qt::RightButton) {
            // Start scaling
            m_timelineAltModifierMode = TimelineAltModifierMode::ScaleTimeline;
            m_pressScreenPos = event->screenPos();
            event->accept();
            return;
        } else if (event->button() == Qt::MiddleButton) {
            // Start panning
            m_timelineAltModifierMode = TimelineAltModifierMode::PanTimeline;
            m_pressPos = event->scenePos();
            event->accept();
            return;
        }  else if (event->button() == Qt::LeftButton) {
            // Start property graph panning
            m_pressPos = event->scenePos();
            QGraphicsItem *item = getItemAt(m_pressPos);
            if (item && item->type() == TimelineItem::TypeRowTimeline) {
                RowTimeline *rowTimeline = static_cast<RowTimeline *>(item);
                if (rowTimeline->propertyGraph()) {
                    m_timelineAltModifierMode = TimelineAltModifierMode::PanPropertyGraph;
                    m_panProperyGraph = rowTimeline->propertyGraph();
                    m_panProperyGraph->startPan();
                    event->accept();
                    return;
                }
            }
        }
    }

    // Ignore non-left presses if dragging
    if (event->button() != Qt::LeftButton && (m_dragging || m_startRowMoverOnNextDrag)) {
        event->accept();
        return;
    }

    if (m_widgetTimeline->blockMousePress())
        return;

    if (!m_widgetTimeline->isFullReconstructPending() && event->button() == Qt::LeftButton) {
        resetMousePressParams();
        m_pressPos = event->scenePos();

        QGraphicsItem *item = getItemAt(m_pressPos);

        const bool ctrlKeyDown = event->modifiers() & Qt::ControlModifier;
        if (item) {
            if (item->type() == TimelineItem::TypeRuler) {
                m_rulerPressed = true;
                m_autoScrollTimelineTimer.start();
            } else if (item->type() == TimelineItem::TypeTreeHeader) {
                if (m_treeHeader->handleButtonsClick(m_pressPos) != TreeControlType::None) {
                    m_rowManager->updateFiltering();
                    updateSnapSteps();
                }
            } else if (item->type() == TimelineItem::TypeRowTree) {
                RowTree *rowTree = static_cast<RowTree *>(item);
                m_clickedTreeControlType = rowTree->getClickedControl(m_pressPos);
                if (m_clickedTreeControlType == TreeControlType::Shy
                        || m_clickedTreeControlType == TreeControlType::Hide
                        || m_clickedTreeControlType == TreeControlType::Lock) {
                    m_rowManager->updateFiltering(rowTree);
                    updateSnapSteps();
                } else if (m_clickedTreeControlType == TreeControlType::None) {
                    // Prepare to change selection to single selection at release if a multiselected
                    // row is clicked without ctrl.
                    if (!ctrlKeyDown && m_rowManager->isRowSelected(rowTree)
                            && !m_rowManager->isSingleSelected()) {
                        m_releaseSelectRow = rowTree;
                    }
                    m_rowManager->selectRow(rowTree, ctrlKeyDown);
                    if (rowTree->draggable())
                        m_startRowMoverOnNextDrag = true;
                } else if (m_clickedTreeControlType == TreeControlType::Arrow) {
                    updateSnapSteps();
                }
            } else if (item->type() == TimelineItem::TypeRowTimeline) {
                m_editedTimelineRow = static_cast<RowTimeline *>(item);
                Keyframe *keyframe = m_editedTimelineRow->getClickedKeyframe(m_pressPos);
                if (keyframe) {  // pressed a keyframe
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
                        m_pressedKeyframe = keyframe;
                    }
                } else {
                    m_keyframeManager->deselectAllKeyframes();
                    m_clickedTimelineControlType
                            = m_editedTimelineRow->getClickedControl(m_pressPos);

                    // clicked an empty spot on a timeline row, start selection rect.
                    if (m_clickedTimelineControlType == TimelineControlType::None) {
                        m_selectionRect->start(m_pressPos);
                    } else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
                        if (!ctrlKeyDown
                                && m_rowManager->isRowSelected(m_editedTimelineRow->rowTree())
                                && !m_rowManager->isSingleSelected()) {
                            m_releaseSelectRow = m_editedTimelineRow->rowTree();
                        }

                        m_rowManager->selectRow(m_editedTimelineRow->rowTree(), ctrlKeyDown);
                        // click position in ruler space
                        m_editedTimelineRow->startDurationMove(m_pressPos.x() - m_ruler->x());
                    } else if (m_clickedTimelineControlType
                               == TimelineControlType::DurationStartHandle
                               || m_clickedTimelineControlType
                                  == TimelineControlType::DurationEndHandle) {
                        m_editedTimelineRow->updateBoundChildren(
                                    m_clickedTimelineControlType
                                    == TimelineControlType::DurationStartHandle);
                    }
                    m_autoScrollTimelineTimer.start();
                }
            }
        } else {
            m_keyframeManager->deselectAllKeyframes();

            if (m_pressPos.x() > m_ruler->x() && m_pressPos.y() > TimelineConstants::ROW_H) {
                m_selectionRect->start(m_pressPos);
                m_autoScrollTimelineTimer.start();
            }
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void TimelineGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_timelineAltModifierMode == TimelineAltModifierMode::ScaleTimeline) {
        int deltaX = event->screenPos().x() - m_pressScreenPos.x();
        int deltaY = event->screenPos().y() - m_pressScreenPos.y();
        // Zooming in when moving down/right.
        int delta = -deltaX - deltaY;
        const int threshold = 20;
        if (delta < -threshold) {
            m_widgetTimeline->toolbar()->onZoomInButtonClicked();
            m_pressScreenPos = event->screenPos();
        } else if (delta > threshold) {
            m_widgetTimeline->toolbar()->onZoomOutButtonClicked();
            m_pressScreenPos = event->screenPos();
        }
    } else if (m_timelineAltModifierMode == TimelineAltModifierMode::PanTimeline) {
        int deltaX = event->scenePos().x() - m_pressPos.x();
        QScrollBar *scrollbar = m_widgetTimeline->viewTimelineContent()->horizontalScrollBar();
        scrollbar->setValue(scrollbar->value() - deltaX);
    } else if (m_timelineAltModifierMode == TimelineAltModifierMode::PanPropertyGraph) {
        qreal deltaY = event->scenePos().y() - m_pressPos.y();
        m_panProperyGraph->pan(deltaY);
    }

    if (m_editedTimelineRow.isNull())
        updateHoverStatus(event->scenePos());

    if (!m_dragging && m_timelineAltModifierMode == TimelineAltModifierMode::None
        && m_pressPos != invalidPoint && (event->scenePos() - m_pressPos).manhattanLength() > 10) {
        m_dragging = true;
    }

    bool shift = event->modifiers() & Qt::ShiftModifier;
    if (m_dragging) {
        if (m_clickedTimelineControlType == TimelineControlType::BezierInHandle
            || m_clickedTimelineControlType == TimelineControlType::BezierOutHandle) {
            if (m_editedTimelineRow && m_editedTimelineRow->propertyGraph()) {
                m_editedTimelineRow->propertyGraph()->updateBezierControlValue(
                            m_clickedTimelineControlType, event->scenePos());
            }
        } else if (m_startRowMoverOnNextDrag || m_rowMover->isActive()) {
            // moving rows vertically (reorder/reparent)
            if (m_startRowMoverOnNextDrag) {
                m_startRowMoverOnNextDrag = false;
                m_rowMover->start(m_rowManager->selectedRows());
            }
            if (m_rowMover->isActive()) {
                m_rowMover->updateTargetRow(event->scenePos());
                updateAutoScrolling(event->scenePos().y());
            }
        } else if (m_pressedKeyframe) { // moving selected keyframes
            double newX = event->scenePos().x() - m_ruler->x()
                          - TimelineConstants::RULER_EDGE_OFFSET - m_pressPosInKeyframe;

            if (newX < 0)
                newX = 0;
            if (shift)
                snap(newX);

            m_keyframeManager->moveSelectedKeyframes(ruler()->distanceToTime(newX));

            m_pressPos.setX(newX);
        }
    }

    QGraphicsScene::mouseMoveEvent(event);
}

// auto scroll when the mouse is at the top or bottom of the tree list
void TimelineGraphicsScene::updateAutoScrolling(double scenePosY)
{
    QScrollBar *scrollbar = m_widgetTimeline->viewTreeContent()->verticalScrollBar();
    double mouseY = scenePosY - scrollbar->value();
    int bottomY = m_widgetTimeline->height() - m_widgetTimeline->toolbar()->height()
            - TimelineConstants::ROW_H;
    if (mouseY > 0 && mouseY < TimelineConstants::ROW_H) {
        if (!m_autoScrollUpOn) {
            m_autoScrollTriggerTimer.start(TimelineConstants::AUTO_SCROLL_TRIGGER);
            m_autoScrollUpOn = true;
        }
    } else if (m_autoScrollUpOn) {
        m_autoScrollTimer.stop();
        m_autoScrollTriggerTimer.stop();
        m_autoScrollUpOn = false;
    }

    if (mouseY > bottomY - TimelineConstants::ROW_H - TimelineConstants::TOOLBAR_MARGIN
            && mouseY < bottomY) {
        if (!m_autoScrollDownOn) {
            m_autoScrollTriggerTimer.start(TimelineConstants::AUTO_SCROLL_TRIGGER);
            m_autoScrollDownOn = true;
        }
    } else if (m_autoScrollDownOn) {
        m_autoScrollTimer.stop();
        m_autoScrollTriggerTimer.stop();
        m_autoScrollDownOn = false;
    }
}

void TimelineGraphicsScene::stopAutoScroll() {
    m_autoScrollTimer.stop();
    m_autoScrollTriggerTimer.stop();
    m_autoScrollUpOn = false;
    m_autoScrollDownOn = false;
}

void TimelineGraphicsScene::updateSnapSteps()
{
    m_snapSteps.clear();
    // i = 1 is always the scene row (or component root)
    for (int i = 2; i < m_layoutTimeline->count(); i++) {
        RowTree *rowTree = static_cast<RowTree *>(m_layoutTree->itemAt(i)->graphicsItem());
        if (rowTree->isVisible()) {
            if (rowTree->hasDurationBar()) {
                double startX = rowTree->rowTimeline()->getStartX();
                if (!m_snapSteps.contains(startX))
                    m_snapSteps.push_back(startX);

                double endX = rowTree->rowTimeline()->getEndX();
                if (!m_snapSteps.contains(endX))
                    m_snapSteps.push_back(endX);
            }

            // add keyframes times
            if (rowTree->hasPropertyChildren()) {
                const QList<Keyframe *> keyframes = rowTree->rowTimeline()->keyframes();
                for (Keyframe *k : keyframes) {
                    double kX = m_ruler->timeToDistance(k->time);
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

void TimelineGraphicsScene::resetMousePressParams()
{
    m_autoScrollTimelineTimer.stop();
    m_selectionRect->end();
    m_rowMover->end();
    m_dragging = false;
    m_startRowMoverOnNextDrag = false;
    m_rulerPressed = false;
    m_pressedKeyframe = nullptr;
    m_panProperyGraph = nullptr;
    m_timelineAltModifierMode = TimelineAltModifierMode::None;
    m_clickedTimelineControlType = TimelineControlType::None;
    m_editedTimelineRow.clear();
    m_releaseSelectRow.clear();
    m_autoScrollTimer.stop();
    m_autoScrollTriggerTimer.stop();
    m_timebarToolTip->hide();
    m_pressPos = invalidPoint;
    m_pressScreenPos = invalidPoint;
    m_lastAutoScrollX = -1.0;
    m_lastAutoScrollY = -1.0;
}

void TimelineGraphicsScene::resetPressedKeyframe()
{
    m_pressedKeyframe = nullptr;
}

QLabel *TimelineGraphicsScene::timebarTooltip()
{
    return m_timebarToolTip;
}

void TimelineGraphicsScene::snap(double &value, bool snapToPlayHead)
{
    // snap to play head
    if (snapToPlayHead) {
        double playHeadX = m_playHead->x() - TimelineConstants::TREE_BOUND_W
                                           - TimelineConstants::RULER_EDGE_OFFSET;
        if (abs(value - playHeadX) < CStudioPreferences::snapRange()) {
            value = playHeadX;
            return;
        }
    }

    // duration edges snap
    for (double v : qAsConst(m_snapSteps)) {
        if (abs(value - v) < CStudioPreferences::snapRange()) {
            value = v;
            return;
        }
    }

    // time steps snap
    if (CStudioPreferences::isTimelineSnappingGridActive()) {
        double snapStep = TimelineConstants::RULER_SEC_W * m_ruler->timelineScale();
        if (CStudioPreferences::timelineSnappingGridResolution() == SNAPGRID_HALFSECONDS)
            snapStep *= .5;
        else if (CStudioPreferences::timelineSnappingGridResolution() == SNAPGRID_TICKMARKS)
            snapStep *= .1;

        double snapValue = round(value / snapStep) * snapStep;
        if (abs(value - snapValue) < CStudioPreferences::snapRange())
            value = snapValue;
    }
}

void TimelineGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_dragging) {
            if (m_rowMover->isActive()) { // moving rows (reorder/reparent)
                commitMoveRows();
            } else if (m_pressedKeyframe) {
                m_keyframeManager->commitMoveSelectedKeyframes();
            } else if (m_clickedTimelineControlType == TimelineControlType::DurationStartHandle) {
                if (!m_editedTimelineRow.isNull()) {
                    ITimelineTimebar *timebar = m_editedTimelineRow->rowTree()->getBinding()
                            ->GetTimelineItem()->GetTimebar();
                    timebar->ChangeTime(m_editedTimelineRow->getStartTime(), true);
                    timebar->CommitTimeChange();
                }
            } else if (m_clickedTimelineControlType == TimelineControlType::DurationEndHandle) {
                if (!m_editedTimelineRow.isNull()) {
                    ITimelineTimebar *timebar = m_editedTimelineRow->rowTree()->getBinding()
                                                ->GetTimelineItem()->GetTimebar();
                    timebar->ChangeTime(m_editedTimelineRow->getEndTime(), false);
                    timebar->CommitTimeChange();
                    if (m_playHead->time() > ruler()->duration())
                        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(ruler()->duration());
                }
            } else if (m_clickedTimelineControlType == TimelineControlType::Duration) {
                if (!m_editedTimelineRow.isNull()) {
                    ITimelineTimebar *timebar = m_editedTimelineRow->rowTree()->getBinding()
                            ->GetTimelineItem()->GetTimebar();
                    timebar->OffsetTime(m_editedTimelineRow->getDurationMoveTime());
                    timebar->CommitTimeChange();
                    if (m_playHead->time() > ruler()->duration())
                        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(ruler()->duration());
                }
            } else if (m_clickedTimelineControlType == TimelineControlType::BezierInHandle
                       || m_clickedTimelineControlType == TimelineControlType::BezierOutHandle) {
                if (m_editedTimelineRow->propertyGraph())
                    m_editedTimelineRow->propertyGraph()->commitBezierEdit();
            }
        } else if (!m_rulerPressed && (!m_releaseSelectRow.isNull() || !itemAt(event->scenePos(),
                                                                               QTransform()))) {
            m_rowManager->selectRow(nullptr);
            if (!m_releaseSelectRow.isNull())
                m_rowManager->selectRow(m_releaseSelectRow);
        }
    }

    if (m_timelineAltModifierMode == TimelineAltModifierMode::ScaleTimeline)
        updateSnapSteps();

    resetMousePressParams();

    QGraphicsScene::mouseReleaseEvent(event);
}

void TimelineGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPointF scenePos = event->scenePos();
        QGraphicsItem *item = getItemAt(scenePos);

        if (item) {
            CDoc *doc = g_StudioApp.GetCore()->GetDoc();
            if (item->type() == TimelineItem::TypeRuler) {
                g_StudioApp.GetDialogs()->asyncDisplayTimeEditDialog(doc->GetCurrentViewTime(),
                                                                     doc, PLAYHEAD,
                                                                     m_keyframeManager);
            } else {
                if (item->type() == TimelineItem::TypeRowTree) {
                    RowTree *rowTree = static_cast<RowTree *>(item);
                    if (rowTree->isProperty()) { // toggle property graph
                        rowTree->togglePropertyExpanded(scenePos);
                    } else {
                        // check label edit
                        QGraphicsItem *topItem = itemAt(scenePos, {});
                        if (topItem->type() == TimelineItem::TypeRowTreeLabel) {
                            RowTreeLabel *rowTreeLabel = static_cast<RowTreeLabel *>(topItem);
                            const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
                            EStudioObjectType rowObjType = rowTreeLabel->rowTree()->objectType();
                            int instance = rowTreeLabel->rowTree()->instance();
                            if (!rowTreeLabel->isLocked() && !bridge->isBasicMaterial(instance)
                                && rowObjType & ~(OBJTYPE_SCENE | OBJTYPE_IMAGE)) {
                                rowTreeLabel->setEnabled(true);
                                rowTreeLabel->setFocus();
                            }
                        }
                    }
                } else if (item->type() == TimelineItem::TypeRowTimeline) {
                    RowTimeline *rowTimeline = static_cast<RowTimeline *>(item);
                    Keyframe *clickedKeyframe = rowTimeline->getClickedKeyframe(scenePos);
                    if (clickedKeyframe) {
                        m_pressedKeyframe = clickedKeyframe;
                        g_StudioApp.GetDialogs()->asyncDisplayTimeEditDialog(
                                    clickedKeyframe->time, g_StudioApp.GetCore()->GetDoc(),
                                    ASSETKEYFRAME, m_keyframeManager);
                    } else {
                        if (!rowTimeline->rowTree()->locked())
                            handleSetTimeBarTime();
                    }
                }
            }
        }
    }

    QGraphicsScene::mouseDoubleClickEvent(event);
}

void TimelineGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
    // adjust property graph scale
    if (wheelEvent->modifiers() & Qt::AltModifier) {
        const QPointF pos = wheelEvent->scenePos();
        QGraphicsItem *item = getItemAt(pos);
        if (item && item->type() == TimelineItem::TypeRowTimeline) {
            RowTimeline *rowTimeline = static_cast<RowTimeline *>(item);
            if (rowTimeline->propertyGraph())
                rowTimeline->propertyGraph()->adjustScale(pos, wheelEvent->delta() > 0);
        }
    }

    // Make sure drag states update on wheel scrolls done during drag
    m_lastAutoScrollX = -1.0;
    m_lastAutoScrollY = -1.0;
    QGraphicsScene::wheelEvent(wheelEvent);
}

void TimelineGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    // Eat left/right arrow keys on tree side unless some item (e.g. label) has focus
    if ((keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right)
            && (qApp->focusObject() == m_widgetTimeline->viewTreeContent() && !focusItem())) {
        keyEvent->accept();
        return;
    } else if (keyEvent->key() == Qt::Key_Escape && m_rowMover->isActive()) {
        m_rowMover->end();
    } else if (keyEvent->key() == Qt::Key_Delete && !m_rowMover->isActive()
               && !focusItem()) {
        g_StudioApp.DeleteSelectedObject(); // Despite the name, this deletes objects and keyframes
    }
    // Make sure drag states update on keyboard scrolls done during drag
    if (keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right
            || keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
        m_lastAutoScrollX = -1.0;
        m_lastAutoScrollY = -1.0;
    }

    QGraphicsScene::keyPressEvent(keyEvent);
}

void TimelineGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

void TimelineGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    // No context menu if user is pressing ALT (so panning/zooming timeline)
    bool alt = event->modifiers() & Qt::AltModifier;
    RowTree *row = m_rowManager->getRowAtPos(QPointF(0, event->scenePos().y()));
    if (!row || m_widgetTimeline->isFullReconstructPending() || m_dragging
            || m_startRowMoverOnNextDrag || row->locked() || alt) {
        return;
    }

    resetMousePressParams(); // Make sure our mouse handling doesn't get confused by context menu

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
    bool variantsAreaHovered = false;
    QGraphicsItem *item = getItemAt(scenePos);
    if (item) {
        // update timeline row cursor
        if (item->type() == TimelineItem::TypeRowTimeline) {
            RowTimeline *rowTimeline = static_cast<RowTimeline *>(item);
            TimelineControlType controlType = rowTimeline->getClickedControl(scenePos, true);
            if (controlType == TimelineControlType::DurationStartHandle
                    || controlType == TimelineControlType::DurationEndHandle) {
                setMouseCursor(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);
            } else {
                resetMouseCursor();
            }
        } else if (!m_dragging && item->type() == TimelineItem::TypeRowTree) {
            // update tree row variants tooltip
            RowTree *rowTree = static_cast<RowTree *>(item);
            if (!rowTree->isProperty()) {
                int left = rowTree->clipX();
                int right = (int)rowTree->treeWidth() - TimelineConstants::TREE_ICONS_W;
                variantsAreaHovered = scenePos.x() > left && scenePos.x() < right;
                if (variantsAreaHovered && rowTree != m_variantsRowTree) {
                    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
                    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
                    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
                    auto property = bridge->getVariantsProperty(rowTree->instance());

                    using namespace qt3dsdm;
                    SValue sValue;
                    if (propertySystem->GetInstancePropertyValue(rowTree->instance(), property,
                                                                 sValue)) {
                        QString propVal = get<TDataStrPtr>(sValue)->toQString();
                        if (!propVal.isEmpty()) {
                            // parse propVal into variantsHash (group => tags)
                            const QStringList tagPairs = propVal.split(QLatin1Char(','));
                            QHash<QString, QStringList> variantsHash;
                            QStringList variantsHashKeys; // maintain traverse order
                            for (auto &tagPair : tagPairs) {
                                const QStringList pair = tagPair.split(QLatin1Char(':'));
                                variantsHash[pair[0]].append(pair[1]);
                                if (!variantsHashKeys.contains(pair[0]))
                                    variantsHashKeys.append(pair[0]);
                            }

                            // parse variantsHash into tooltipStr
                            const auto variantsDef
                                    = g_StudioApp.GetCore()->getProjectFile().variantsDef();
                            QString templ = QStringLiteral("<font color='%1'>%2</font>");
                            QString tooltipStr("<table>");
                            for (auto &g : qAsConst(variantsHashKeys)) {
                                tooltipStr.append("<tr><td>");
                                tooltipStr.append(templ.arg(variantsDef[g].m_color).arg(g + ": "));
                                tooltipStr.append("</td><td>");
                                for (auto &t : qAsConst(variantsHash[g]))
                                    tooltipStr.append(t + ", ");
                                tooltipStr.chop(2);
                                tooltipStr.append("</td></tr>");
                            }
                            tooltipStr.append("</table>");

                            int ttY = int(rowTree->y())
                                      + widgetTimeline()->navigationBar()->height();

                            m_variantsToolTip->setText(tooltipStr);
                            m_variantsToolTip->adjustSize();
                            m_variantsToolTip->move(m_widgetTimeline->mapToGlobal({right, ttY}));
                            m_variantsToolTip->raise();
                            m_variantsToolTip->show();
                            m_variantsRowTree = rowTree;
                        }
                    }
                }
            }
        }
    }

    if (m_variantsRowTree && !variantsAreaHovered) {
        m_variantsToolTip->hide();
        m_variantsRowTree = nullptr;
    }
}

// This method is similar to itemAt() but if it finds a playhead or tree label items, it returns
// what is below them
QGraphicsItem *TimelineGraphicsScene::getItemAt(const QPointF &scenePos) const
{
    const QList<QGraphicsItem *> hoverItems = items(scenePos);

    if (!hoverItems.empty()) {
        QGraphicsItem *item = hoverItems.at(0);

        int typeMask = TimelineItem::TypePlayHead | TimelineItem::TypeRowTreeLabel;
        if (item->type() & typeMask && hoverItems.size() > 1)
            item = hoverItems.at(1);

        return item;
    }

    return nullptr;
}

QPoint TimelineGraphicsScene::getScrollbarOffsets() const
{
    QGraphicsView *timelineContent = m_widgetTimeline->viewTimelineContent();
    return QPoint(timelineContent->verticalScrollBar()->isVisible()
            ? timelineContent->verticalScrollBar()->width() : 0,
            timelineContent->horizontalScrollBar()->isVisible()
            ? timelineContent->horizontalScrollBar()->height() : 0);
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

void TimelineGraphicsScene::handleApplicationFocusLoss()
{
    // Hide the timebar and variants tooltips if application loses focus
    if (!QApplication::focusWidget()) {
        m_timebarToolTip->hide();
        m_variantsToolTip->hide();
    }
}

void TimelineGraphicsScene::handleShowDISelector(const QString &propertyname,
                                                 qt3dsdm::Qt3DSDMInstanceHandle inInst,
                                                 const QPoint &pos)
{
    auto doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMPropertyHandle propHandle = doc->GetPropertySystem()
            ->GetAggregateInstancePropertyByName(inInst, propertyname.toStdWString().c_str());

    QVector<EDataType> allowedTypes = CDataInputDlg::getAcceptedTypes(
                doc->GetPropertySystem()->GetDataType(propHandle));

    // Instantiate selector in TimelineGraphicsScene instead of the originating context menu,
    // as context menu gets destructed when a selection is made.
    if (!m_dataInputSelector)
        m_dataInputSelector = new DataInputSelectView(allowedTypes, widgetTimeline());

    QVector<QPair<QString, int>> dataInputList;
    for (auto &it : qAsConst(g_StudioApp.m_dataInputDialogItems))
        dataInputList.append({it->name, it->type});
    // needs to be set just in case we are reusing an existing datainput selector instance
    m_dataInputSelector->setMatchingTypes(allowedTypes);
    m_dataInputSelector->setTypeFilter(DataInputTypeFilter::MatchingTypes);
    m_dataInputSelector->setData(dataInputList, m_dataInputSelector->getNoneString(),
                                 propHandle, inInst);
    m_dataInputSelector->setCurrentController(doc->GetCurrentController(inInst, propHandle));

    connect(m_dataInputSelector, &DataInputSelectView::dataInputChanged,
            [&](int handle, int instance, const QString &controllerName) {
        bool controlled = controllerName != m_dataInputSelector->getNoneString();
        g_StudioApp.GetCore()->GetDoc()
                ->SetInstancePropertyControlled(instance, Q3DStudio::CString(), handle,
                                                Q3DStudio::CString::fromQString(controllerName),
                                                controlled);
    });

    CDialogs::showWidgetBrowser(widgetTimeline(), m_dataInputSelector, pos);
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
Keyframe              *TimelineGraphicsScene::pressedKeyframe() const { return m_pressedKeyframe; }
