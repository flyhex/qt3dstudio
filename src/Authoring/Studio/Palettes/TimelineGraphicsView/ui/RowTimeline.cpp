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

#include "RowTimeline.h"
#include "RowTree.h"
#include "RowManager.h"
#include "Ruler.h"
#include "TimelineConstants.h"
#include "Keyframe.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"

#include <QtGui/qpainter.h>
#include <QtWidgets/qgraphicssceneevent.h>

RowTimeline::RowTimeline()
    : InteractiveTimelineItem()
{
}

RowTimeline::~RowTimeline()
{
    // remove keyframes
    if (!m_keyframes.empty()) {
        if (m_isProperty) // non-property rows use the same keyframes from property rows.
            qDeleteAll(m_keyframes);

        m_keyframes.clear();
    }
}

void RowTimeline::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Background
    QColor bgColor;
    if (m_rowTree->isProperty())
        bgColor = TimelineConstants::ROW_COLOR_NORMAL_PROP;
    else if (m_state == Selected)
        bgColor = TimelineConstants::ROW_COLOR_SELECTED;
    else if (m_state == Hovered && !m_rowTree->m_locked)
        bgColor = TimelineConstants::ROW_COLOR_OVER;
    else
        bgColor = TimelineConstants::ROW_COLOR_NORMAL;
    painter->fillRect(0, 0, widget->width() + size().width(), size().height() - 1, bgColor);

    // Duration
    if (m_rowTree->hasDurationBar()) {
        painter->save();

        // fully outside ancestors' limits, draw fully hashed
        if (m_minStartX > m_endX || m_maxEndX < m_startX) {
            painter->setBrush(QBrush(QColor(TimelineConstants::ROW_COLOR_DURATION_OFF1),
                                     Qt::BDiagPattern));
            painter->setPen(Qt::NoPen);
            painter->fillRect(QRect(m_startX, 0, m_endX - m_startX, size().height() - 1),
                              QColor(TimelineConstants::ROW_COLOR_DURATION_OFF2));
            painter->drawRect(QRect(m_startX, 0, m_endX - m_startX, size().height() - 1));

            painter->setPen(QPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE), 3));
            painter->drawLine(m_startX, 0, m_startX, size().height() - 1);
            painter->drawLine(m_endX, 0, m_endX, size().height() - 1);
        } else {
            // draw main duration part
            double x = std::max(m_startX, m_minStartX);
            double w = std::min(m_endX, m_maxEndX) - x;
            painter->setPen(Qt::NoPen);
            painter->fillRect(QRect(x, 0, w, size().height() - 1), m_state == Selected
                              ? TimelineConstants::ROW_COLOR_DURATION_SELECTED
                              : TimelineConstants::ROW_COLOR_DURATION);

            // draw hashed part before
            painter->setBrush(QBrush(QColor(TimelineConstants::ROW_COLOR_DURATION_OFF1),
                                     Qt::BDiagPattern));
            if (m_startX < m_minStartX) {
                painter->setPen(Qt::NoPen);
                painter->fillRect(QRect(m_startX, 0, m_minStartX - m_startX, size().height() - 1),
                                  QColor(TimelineConstants::ROW_COLOR_DURATION_OFF2));
                painter->drawRect(QRect(m_startX, 0, m_minStartX - m_startX, size().height() - 1));
                painter->setPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE));
                painter->drawLine(m_minStartX, 0, m_minStartX, size().height() - 1);
            }

            // draw hashed part after
            if (m_endX > m_maxEndX) {
                painter->setPen(Qt::NoPen);
                painter->fillRect(QRect(m_maxEndX, 0, m_endX - m_maxEndX, size().height() - 1),
                                  QColor(TimelineConstants::ROW_COLOR_DURATION_OFF2));
                painter->drawRect(QRect(m_maxEndX, 0, m_endX - m_maxEndX, size().height() - 1));
                painter->setPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE));
                painter->drawLine(m_maxEndX, 0, m_maxEndX, size().height() - 1);
            }

            painter->setPen(QPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE), 2));
            painter->drawLine(m_startX, 0, m_startX, size().height() - 1);
            painter->drawLine(m_endX, 0, m_endX, size().height() - 1);
        }

        painter->restore();
    }

    // Keyframes
    int keyFrameY = 2 - (TimelineConstants::ROW_H - size().height())/2;
    if (m_rowTree->hasPropertyChildren()) { // master keyframes
        static const QPixmap pixKeyframeMasterNormal
                = QPixmap(":/images/Keyframe-Master-Normal.png");
        static const QPixmap pixKeyframeMasterSelected
                = QPixmap(":/images/Keyframe-Master-Selected.png");

        for (auto keyframe : m_keyframes) {
            painter->drawPixmap(timeToX(keyframe->time) - 8.5, keyFrameY, keyframe->selected
                                ? pixKeyframeMasterSelected : pixKeyframeMasterNormal);
        }
    } else if (m_rowTree->isProperty()) {
        static const QPixmap pixKeyframePropertyNormal
                = QPixmap(":/images/Keyframe-Property-Normal.png");
        static const QPixmap pixKeyframePropertySelected
                = QPixmap(":/images/Keyframe-Property-Selected.png");

        for (auto keyframe : m_keyframes) {
            painter->drawPixmap(timeToX(keyframe->time) - (keyframe->selected ? 7.5 : 5.5), keyFrameY,
                                keyframe->selected ? pixKeyframePropertySelected
                                                   : pixKeyframePropertyNormal);
        }
    }
}

Keyframe *RowTimeline::getClickedKeyframe(const QPointF &scenePos)
{
    QPointF p = mapFromScene(scenePos.x(), scenePos.y());
    double x;

    QList<Keyframe *> keyframes;
    if (m_rowTree->hasPropertyChildren()) {
        const auto childRows = m_rowTree->childRows();
        for (auto child : childRows)
            keyframes.append(child->rowTimeline()->m_keyframes);
    } else {
        keyframes = m_keyframes;
    }

    for (const auto keyframe : qAsConst(keyframes)) {
        x = timeToX(keyframe->time);

        if (p.x() > x - 5 && p.x() < x + 5 && p.y() > 3 && p.y() < 16)
            return keyframe;
    }

    return nullptr;
}

QList<Keyframe *> RowTimeline::getKeyframesInRange(const double left, const double right)
{
    double x;
    double x1 = mapFromScene(left, 0).x();
    double x2 = mapFromScene(right, 0).x();

    QList<Keyframe *> result;
    QList<Keyframe *> keyframes;

    if (m_rowTree->hasPropertyChildren()) {
        const auto childRows = m_rowTree->childRows();
        for (auto child : childRows) {
            if (child->isProperty())
                keyframes.append(child->rowTimeline()->m_keyframes);
        }
    } else {
        keyframes = m_keyframes;
    }

    for (auto keyframe : qAsConst(keyframes)) {
        x = timeToX(keyframe->time);

        if (x1 < x && x2 > x)
            result.append(keyframe);
    }

    return result;
}

void RowTimeline::insertKeyframe(Keyframe *keyframe)
{
    if (!m_keyframes.contains(keyframe))
        m_keyframes.append(keyframe);
}

void RowTimeline::removeKeyframe(Keyframe *keyframe)
{
    m_keyframes.removeAll(keyframe);
}

void RowTimeline::putSelectedKeyframesOnTop()
{
    if (!m_keyframes.empty())
        std::partition(m_keyframes.begin(), m_keyframes.end(), [](Keyframe *kf) {
            return !kf->selected;
        });

    if (m_rowTree->hasPropertyChildren()) { // has property rows
        const auto childRows = m_rowTree->childRows();
        for (auto child : childRows) {
            std::partition(child->rowTimeline()->m_keyframes.begin(),
                           child->rowTimeline()->m_keyframes.end(), [](Keyframe *kf) {
                return !kf->selected;
            });
        }
    }
}

void RowTimeline::updateKeyframes()
{
    update();

    if (m_rowTree->hasPropertyChildren()) { // master keyframes
        const auto childRows = m_rowTree->childRows();
        for (const auto child : childRows)
            child->rowTimeline()->update();
    }
}

TimelineControlType RowTimeline::getClickedControl(const QPointF &scenePos) const
{
    if (!m_rowTree->hasDurationBar())
        return TimelineControlType::None;

    QPointF p = mapFromScene(scenePos.x(), scenePos.y());
    if (p.x() > m_startX - TimelineConstants::DURATION_HANDLE_W * .5
            && p.x() < m_startX + TimelineConstants::DURATION_HANDLE_W * .5) {
        return TimelineControlType::StartHandle;
    } else if (p.x() > m_endX - TimelineConstants::DURATION_HANDLE_W * .5
               && p.x() < m_endX + TimelineConstants::DURATION_HANDLE_W * .5) {
        return TimelineControlType::EndHandle;
    } else if (p.x() > m_startX && p.x() < m_endX && !rowTree()->locked()) {
        return TimelineControlType::Duration;
    }

    return TimelineControlType::None;
}

// move the duration area (start/end x)
void RowTimeline::moveDurationBy(double dx)
{
    double dur = m_endX - m_startX;

    m_startX += dx;
    m_endX += dx;

    if (m_startX < TimelineConstants::RULER_EDGE_OFFSET) {
        m_startX = TimelineConstants::RULER_EDGE_OFFSET;
        m_endX = m_startX + dur;
    }

    if (m_rowTree->parentRow() == nullptr) {
        m_minStartX = m_startX;
        m_maxEndX = m_endX;
    }

    m_startTime = xToTime(m_startX);
    m_endTime = xToTime(m_endX);

    updateChildrenMinStartXRecursive(m_rowTree);
    updateChildrenMaxEndXRecursive(m_rowTree);

    update();

    if (!m_rowTree->empty()) {
        for (RowTree *child : qAsConst(m_rowTree->m_childRows))
            child->m_rowTimeline->moveDurationBy(dx);
    }
}

void RowTimeline::commitDurationMove()
{
    m_rowTree->m_binding->GetTimelineItem()->GetTimebar()->ChangeTime(m_startTime * 1000, true);
    m_rowTree->m_binding->GetTimelineItem()->GetTimebar()->ChangeTime(m_endTime * 1000, false);
    m_rowTree->m_binding->GetTimelineItem()->GetTimebar()->CommitTimeChange();
    if (!m_rowTree->empty()) {
        for (RowTree *child : qAsConst(m_rowTree->m_childRows)) {
            if (!child->isProperty())
                child->m_rowTimeline->commitDurationMove();
        }
    }
}

// convert time values to x
double RowTimeline::timeToX(double time)
{
    return TimelineConstants::RULER_EDGE_OFFSET + time * TimelineConstants::RULER_SEC_W
           * rowTree()->m_scene->ruler()->timelineScale();
}

// convert x values to time
double RowTimeline::xToTime(double xPos)
{
    return (xPos - TimelineConstants::RULER_EDGE_OFFSET)
           / (TimelineConstants::RULER_SEC_W * rowTree()->m_scene->ruler()->timelineScale());
}

// called after timeline scale is changed to update duration star/end positions
void RowTimeline::updatePosition()
{
    setStartX(timeToX(m_startTime));
    setEndX(timeToX(m_endTime));
}

// Set the position of the start of the row duration
void RowTimeline::setStartX(double startX)
{
    if (startX < TimelineConstants::RULER_EDGE_OFFSET)
        startX = TimelineConstants::RULER_EDGE_OFFSET;
    else if (startX > m_endX - 1)
        startX = m_endX - 1;

    double oldStartX = m_startX;
    m_startX = startX;
    m_startTime = xToTime(startX);

    if (m_rowTree->parentRow() == nullptr || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE)
        m_minStartX = 0;

    updateChildrenStartRecursive(m_rowTree, oldStartX);
    updateChildrenMinStartXRecursive(m_rowTree);
    update();
}

// Set the position of the end of the row duration
void RowTimeline::setEndX(double endX)
{
    if (endX < m_startX + 1)
        endX = m_startX + 1;
    else if (endX > m_endX)
        rowTree()->m_scene->rowManager()->updateRulerDuration();

    double oldEndX = m_endX;
    m_endX = endX;
    m_endTime = xToTime(endX);

    if (m_rowTree->parentRow() == nullptr || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE)
        m_maxEndX = 999999;

    updateChildrenEndRecursive(m_rowTree, oldEndX);
    updateChildrenMaxEndXRecursive(m_rowTree);
    update();
}

void RowTimeline::updateChildrenStartRecursive(RowTree *rowTree, double oldStartX)
{
    // Update all bound childred
    // Rows are considered to be bound when their times match
    if (!rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        for (auto child : childRows) {
            RowTimeline *rowTimeline = child->rowTimeline();
            if (rowTimeline->m_startX == oldStartX) {
                rowTimeline->m_startX = m_startX;
                rowTimeline->m_startTime = m_startTime;
                rowTimeline->updateChildrenStartRecursive(child, oldStartX);
                rowTimeline->update();
            }
        }
    }
}

void RowTimeline::updateChildrenEndRecursive(RowTree *rowTree, double oldEndX)
{
    // Update all bound childred
    // Rows are considered to be bound when their times match
    if (!rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        for (auto child : childRows) {
            RowTimeline *rowTimeline = child->rowTimeline();
            if (rowTimeline->m_endX == oldEndX) {
                rowTimeline->m_endX = m_endX;
                rowTimeline->m_endTime = m_endTime;
                rowTimeline->updateChildrenEndRecursive(child, oldEndX);
                rowTimeline->update();
            }
        }
    }
}

void RowTimeline::updateChildrenMinStartXRecursive(RowTree *rowTree)
{
    if (m_rowTree->rowType() != OBJTYPE_SCENE && !rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        for (auto child : childRows) {
            child->rowTimeline()->m_minStartX = std::max(rowTree->rowTimeline()->m_startX,
                                                         rowTree->rowTimeline()->m_minStartX);
            child->rowTimeline()->update();

            updateChildrenMinStartXRecursive(child);
        }
    }
}

void RowTimeline::updateChildrenMaxEndXRecursive(RowTree *rowTree)
{
    if (m_rowTree->rowType() != OBJTYPE_SCENE && !rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        for (auto child : childRows) {
            child->rowTimeline()->m_maxEndX = std::min(rowTree->rowTimeline()->m_endX,
                                                       rowTree->rowTimeline()->m_maxEndX);
            child->rowTimeline()->update();

            updateChildrenMaxEndXRecursive(child);
        }
    }
}

void RowTimeline::setStartTime(double startTime)
{
    double oldStartX = m_startX;
    m_startTime = startTime;
    m_startX = timeToX(startTime);

    if (m_rowTree->parentRow() == nullptr || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE)
        m_minStartX = 0;

    updateChildrenStartRecursive(m_rowTree, oldStartX);
    updateChildrenMinStartXRecursive(m_rowTree);
    update();
}

void RowTimeline::setEndTime(double endTime)
{
    double oldEndX = m_endX;
    m_endTime = endTime;
    m_endX = timeToX(endTime);

    if (m_rowTree->parentRow() == nullptr || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE)
        m_maxEndX = 999999;

    rowTree()->m_scene->rowManager()->updateRulerDuration();
    updateChildrenEndRecursive(m_rowTree, oldEndX);
    updateChildrenMaxEndXRecursive(m_rowTree);
    update();
}

double RowTimeline::getStartTime() const
{
    return m_startTime;
}

double RowTimeline::getEndTime() const
{
    return m_endTime;
}

void RowTimeline::setState(State state)
{
    m_state = state;
    m_rowTree->m_state = state;

    update();
    m_rowTree->update();
}

void RowTimeline::setRowTree(RowTree *rowTree)
{
    m_rowTree = rowTree;
}

RowTree *RowTimeline::rowTree() const
{
    return m_rowTree;
}

QList<Keyframe *> RowTimeline::keyframes() const
{
    return m_keyframes;
}

RowTimeline *RowTimeline::parentRow() const
{
    if (m_rowTree->m_parentRow == nullptr)
        return nullptr;

    return m_rowTree->m_parentRow->rowTimeline();
}

void RowTimeline::hoverLeaveEvent(QGraphicsSceneHoverEvent  *event)
{
    InteractiveTimelineItem::hoverLeaveEvent(event);
    // Make sure mouse cursor is reseted when moving away from timeline row
    m_rowTree->m_scene->resetMouseCursor();
}

int RowTimeline::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRowTimeline;
}
