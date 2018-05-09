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

#ifndef TIMELINEGRAPHICSSCENE_H
#define TIMELINEGRAPHICSSCENE_H

#include "RowTree.h"
#include "TimelineWidget.h"
#include "RowTimeline.h"
#include "RowTypes.h"
#include "TimelineConstants.h"
#include "MouseCursor.h"

#include <QtWidgets/qgraphicsscene.h>
#include <QtCore/qlist.h>
#include <QtCore/qhash.h>

class Ruler;
class PlayHead;
class TreeHeader;
class SelectionRect;
class RowMover;
class RowManager;
class KeyframeManager;
class TimelineControl;
class IKeyframesManager;
struct Keyframe;

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)

typedef QHash<qt3dsdm::Qt3DSDMInstanceHandle, RowTree::ExpandState> TExpandMap;

class TimelineGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit TimelineGraphicsScene(TimelineWidget *timelineWidget);

    void setTimelineScale(int scale);
    void updateTimelineLayoutWidth();
    Ruler *ruler() const;
    PlayHead *playHead() const;
    RowManager *rowManager() const;
    RowMover *rowMover() const;
    QGraphicsWidget *widgetRoot() const;
    KeyframeManager *keyframeManager() const;
    QGraphicsLinearLayout *layoutTree() const;
    QGraphicsLinearLayout *layoutTimeline() const;
    TreeHeader *treeHeader() const;
    double treeWidth() const;
    TimelineWidget *widgetTimeline() const;
    void updateTreeWidth(double x);
    void setMouseCursor(CMouseCursor::Qt3DSMouseCursor cursor);
    void resetMouseCursor();
    void updateSnapSteps();
    TExpandMap &expandMap();

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    void commitMoveRows();
    void updateHoverStatus(const QPointF &scenePos);
    void snap(double &value, bool snapToPlayHead = true);
    int nextRowDepth(int index);
    bool lastRowInAParent(RowTree *rowAtIndex, int index);
    QGraphicsItem *getItemBelowType(TimelineItem::ItemType type,
                                    QGraphicsItem *item,
                                    const QPointF &scenePos);
    void handleInsertKeyframe();
    void handleDeleteChannelKeyframes();
    void handleSetTimeBarTime();
    void handleMakeComponent();
    void handleCopyObjectPath();
    void handleEditComponent();
    void handleShySelected();
    void handleLockSelected();

    QGraphicsLinearLayout *m_layoutRoot;
    QGraphicsLinearLayout *m_layoutTree;
    QGraphicsLinearLayout *m_layoutTimeline;

    TreeHeader *m_treeHeader;
    Ruler *m_ruler;
    PlayHead *m_playHead;
    TimelineWidget *m_widgetTimeline;
    QGraphicsWidget *m_widgetRoot;
    RowMover *m_rowMover = nullptr;
    RowTimeline *m_editedTimelineRow = nullptr;
    SelectionRect *m_selectionRect;
    RowManager *m_rowManager = nullptr;
    KeyframeManager *m_keyframeManager = nullptr;
    QPointF m_pressPos;
    QList<double> m_snapSteps;
    CMouseCursor::Qt3DSMouseCursor m_currentCursor;
    TimelineControl *m_timelineControl = nullptr;

    bool m_rulerPressed = false;
    bool m_keyframePressed = false;
    bool m_dragging = false;
    TimelineControlType m_clickedTimelineControlType = TimelineControlType::None;
    TreeControlType m_clickedTreeControlType = TreeControlType::None;
    double m_pressPosInKeyframe;
    double m_treeWidth = TimelineConstants::TREE_DEFAULT_W;
    TExpandMap m_expandMap;
};

#endif // TIMELINEGRAPHICSSCENE_H
