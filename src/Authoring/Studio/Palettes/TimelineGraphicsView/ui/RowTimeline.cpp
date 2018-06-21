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
#include "RowTimelinePropertyGraph.h"
#include "RowTree.h"
#include "RowManager.h"
#include "Ruler.h"
#include "TimelineConstants.h"
#include "Keyframe.h"
#include "KeyframeManager.h"
#include "TimelineGraphicsScene.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"
#include "Bindings/Qt3DSDMTimelineItemProperty.h"

#include <QtGui/qpainter.h>
#include <QtGui/qbrush.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qlabel.h>
#include <QtCore/qdatetime.h>

RowTimeline::RowTimeline()
    : InteractiveTimelineItem()
{
    // 999999: theoretically big enough row width (~ 4.6 hrs of presentation length)
    setMinimumWidth(999999);
    setMaximumWidth(999999);
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
    Q_UNUSED(option)

    if (!y()) // prevents flickering when the row is just inserted to the layout
        return;

    const int currentHeight = size().height() - 1;

    if (isColorProperty() && !m_keyframes.empty()) {
        drawColorPropertyGradient(painter, widget->width());
    } else {
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
        painter->fillRect(0, 0, size().width(), currentHeight, bgColor);
    }

    // Duration
    if (m_rowTree->hasDurationBar()) {
        painter->save();

        // fully outside ancestors' limits, draw fully hashed
        if (m_minStartX > m_endX || m_maxEndX < m_startX) {
            painter->setBrush(QBrush(QColor(TimelineConstants::ROW_COLOR_DURATION_OFF1),
                                     Qt::BDiagPattern));
            painter->setPen(Qt::NoPen);
            painter->fillRect(QRect(m_startX, 0, m_endX - m_startX, currentHeight),
                              QColor(TimelineConstants::ROW_COLOR_DURATION_OFF2));
            painter->drawRect(QRect(m_startX, 0, m_endX - m_startX, currentHeight));

            painter->setPen(QPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE), 3));
            painter->drawLine(m_startX, 0, m_startX, currentHeight);
            painter->drawLine(m_endX, 0, m_endX, currentHeight);
        } else {
            // draw main duration part
            double x = qMax(m_startX, m_minStartX);
            double w = qMin(m_endX, m_maxEndX) - x;

            painter->setPen(Qt::NoPen);
            painter->fillRect(QRect(x, 0, w, currentHeight), m_barColor);

            if ( m_state == Selected) {
                // draw selection overlay on bar
                int marginY = 3;
                painter->fillRect(QRect(x, marginY, w, currentHeight - marginY * 2),
                                  QColor(TimelineConstants::ROW_COLOR_DURATION_SELECTED));
            }

            // draw hashed part before
            painter->setBrush(QBrush(QColor(TimelineConstants::ROW_COLOR_DURATION_OFF1),
                                     Qt::BDiagPattern));
            if (m_startX < m_minStartX) {
                painter->setPen(Qt::NoPen);
                painter->fillRect(QRect(m_startX, 0, m_minStartX - m_startX, currentHeight),
                                  QColor(TimelineConstants::ROW_COLOR_DURATION_OFF2));
                painter->drawRect(QRect(m_startX, 0, m_minStartX - m_startX, currentHeight));
                painter->setPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE));
                painter->drawLine(m_minStartX, 0, m_minStartX, currentHeight);
            }

            // draw hashed part after
            if (m_endX > m_maxEndX) {
                painter->setPen(Qt::NoPen);
                painter->fillRect(QRect(m_maxEndX, 0, m_endX - m_maxEndX, currentHeight),
                                  QColor(TimelineConstants::ROW_COLOR_DURATION_OFF2));
                painter->drawRect(QRect(m_maxEndX, 0, m_endX - m_maxEndX, currentHeight));
                painter->setPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE));
                painter->drawLine(m_maxEndX, 0, m_maxEndX, currentHeight);
            }

            painter->setPen(QPen(QColor(TimelineConstants::ROW_COLOR_DURATION_EDGE), 2));
            painter->drawLine(m_startX, 0, m_startX, currentHeight);
            painter->drawLine(m_endX, 0, m_endX, currentHeight);
        }

        painter->restore();
    }

    if (m_propertyGraph) {
        // Property graph
        QRectF graphRect(TimelineConstants::RULER_EDGE_OFFSET, 0,
                         widget->width(), currentHeight);
        m_propertyGraph->paintGraphs(painter, graphRect);
    }

    // Keyframes
    const qreal keyFrameH = 16.0;
    const qreal keyFrameHalfH = keyFrameH / 2.0;
    const qreal keyFrameY = (qMin(currentHeight, TimelineConstants::ROW_H) / 2.0) - keyFrameHalfH;
    const qreal hiddenKeyFrameY = keyFrameY + (keyFrameH * 2.0 / 3.0) + 2.0;

    // Hidden descendant keyframe indicators
    if (!m_rowTree->expanded()) {
        static const QPixmap pixKeyframeHidden = QPixmap(":/images/keyframe-hidden-normal.png");
        QVector<double> childKeyframeTimes;
        collectChildKeyframeTimes(childKeyframeTimes);

        const qreal oldOpacity = painter->opacity();
        painter->setOpacity(0.75);
        for (const auto time : qAsConst(childKeyframeTimes)) {
            const qreal xCoord = timeToX(time) - 2.5;
            painter->drawPixmap(QPointF(xCoord, hiddenKeyFrameY), pixKeyframeHidden);
        }
        painter->setOpacity(oldOpacity);
    }

    if (m_rowTree->hasPropertyChildren()) { // master keyframes
        static const QPixmap pixKeyframeMasterNormal
                = QPixmap(":/images/Keyframe-Master-Normal.png");
        static const QPixmap pixKeyframeMasterSelected
                = QPixmap(":/images/Keyframe-Master-Selected.png");
        static const QPixmap pixKeyframeMasterDynamicNormal
                = QPixmap(":/images/Keyframe-MasterDynamic-Normal.png");
        static const QPixmap pixKeyframeMasterDynamicSelected
                = QPixmap(":/images/Keyframe-MasterDynamic-Selected.png");

        for (auto keyframe : qAsConst(m_keyframes)) {
            QPixmap pixmap;
            if (keyframe->selected()) {
                if (keyframe->dynamic)
                    pixmap = pixKeyframeMasterDynamicSelected;
                else
                    pixmap = pixKeyframeMasterSelected;
            } else {
                if (keyframe->dynamic)
                    pixmap = pixKeyframeMasterDynamicNormal;
                else
                    pixmap = pixKeyframeMasterNormal;
            }
            painter->drawPixmap(QPointF(timeToX(keyframe->time) - 8.5, keyFrameY), pixmap);
        }
    } else if (m_rowTree->isProperty()) {
        static const QPixmap pixKeyframePropertyNormal
                = QPixmap(":/images/Keyframe-Property-Normal.png");
        static const QPixmap pixKeyframePropertySelected
                = QPixmap(":/images/Keyframe-Property-Selected.png");
        static const QPixmap pixKeyframePropertyDynamicNormal
                = QPixmap(":/images/Keyframe-PropertyDynamic-Normal.png");
        static const QPixmap pixKeyframePropertyDynamicSelected
                = QPixmap(":/images/Keyframe-PropertyDynamic-Selected.png");

        for (auto keyframe : qAsConst(m_keyframes)) {
            QPixmap pixmap;
            if (keyframe->selected()) {
                if (keyframe->dynamic)
                    pixmap = pixKeyframePropertyDynamicSelected;
                else
                    pixmap = pixKeyframePropertySelected;
            } else {
                if (keyframe->dynamic)
                    pixmap = pixKeyframePropertyDynamicNormal;
                else
                    pixmap = pixKeyframePropertyNormal;
            }
            painter->drawPixmap(QPointF(timeToX(keyframe->time) - (keyframe->selected() ? 7.5 : 5.5),
                                keyFrameY), pixmap);
        }
    }
}

bool RowTimeline::isColorProperty() const
{
    ITimelineItemProperty *propBinding = m_rowTree->propBinding();
    if (propBinding) {
        qt3dsdm::TDataTypePair type = propBinding->GetType();
        if (m_rowTree->isProperty()
                && type.first == qt3dsdm::DataModelDataType::Float3
                && type.second == qt3dsdm::AdditionalMetaDataType::Color) {
            return true;
        }
    }
    return false;
}

void RowTimeline::drawColorPropertyGradient(QPainter *painter, int width)
{
    // Gradient scaled width, or at least widget width
    double minWidth = width;
    double timelineScale = m_rowTree->m_scene->ruler()->timelineScale();
    double scaledWidth = width * (timelineScale / 2);
    width = qMax(minWidth, scaledWidth);

    ITimelineItemProperty *propBinding = m_rowTree->propBinding();
    QLinearGradient bgGradient(0, 0, width, 0);

    for (auto keyframe : qAsConst(m_keyframes)) {
        double xPos = timeToX(keyframe->time);
        double gradPos = xPos / width;
        gradPos = qBound(0.0, gradPos, 1.0);
        long timeMs = keyframe->time * 1000;
        QColor currentColor;
        // Get the color at the specified time.
        currentColor.setRed(propBinding->GetChannelValueAtTime(0, timeMs));
        currentColor.setGreen(propBinding->GetChannelValueAtTime(1, timeMs));
        currentColor.setBlue(propBinding->GetChannelValueAtTime(2, timeMs));
        bgGradient.setColorAt(gradPos, currentColor);
    }
    painter->fillRect(TimelineConstants::RULER_EDGE_OFFSET, 0,
                      width, size().height() - 1, bgGradient);
}

Keyframe *RowTimeline::getClickedKeyframe(const QPointF &scenePos)
{
    QPointF p = mapFromScene(scenePos.x(), scenePos.y());
    double x;

    QList<Keyframe *> keyframes;
    if (m_rowTree->hasPropertyChildren()) {
        const auto childProps = m_rowTree->childProps();
        for (auto child : childProps)
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

QList<Keyframe *> RowTimeline::getKeyframesInRange(const QRectF &rect) const
{
    double x;
    QRectF localRect = mapFromScene(rect).boundingRect();

    QList<Keyframe *> result;

    static const int KF_CENTER_Y = 10;
    for (auto keyframe : qAsConst(m_keyframes)) {
        x = timeToX(keyframe->time);

        if (localRect.left() < x && localRect.right() > x
                && localRect.top() < KF_CENTER_Y && localRect.bottom() > KF_CENTER_Y) {
            result.append(keyframe);
        }
    }

    return result;
}

void RowTimeline::updateDurationFromBinding()
{
    if (m_rowTree->isProperty()) // this method works for main rows only
        return;

    ITimelineTimebar *timebar = m_rowTree->m_binding->GetTimelineItem()->GetTimebar();
    setStartTime(timebar->GetStartTime() * .001);
    setEndTime(timebar->GetEndTime() * .001);
}

void RowTimeline::updateKeyframesFromBinding(qt3dsdm::Qt3DSDMPropertyHandle propHandle)
{
    if (m_rowTree->isProperty()) // this method works for main rows only
        return;

    // find the UI property row from handle
    RowTree *propRow = nullptr;
    const auto childProps = m_rowTree->childProps();
    for (auto child : childProps) {
        qt3dsdm::Qt3DSDMPropertyHandle propertyHandle =
            static_cast<Qt3DSDMTimelineItemProperty *>(child->m_PropBinding)
            ->getPropertyHandle();
        if (propertyHandle == propHandle) {
            propRow = child;
            break;
        }
    }

    if (propRow) {
        m_rowTree->m_scene->keyframeManager()->deleteKeyframes(propRow->rowTimeline(), false);

        for (int i = 0; i < propRow->m_PropBinding->GetKeyframeCount(); i++) {
            Qt3DSDMTimelineKeyframe *kf = static_cast<Qt3DSDMTimelineKeyframe *>
                    (propRow->m_PropBinding->GetKeyframeByIndex(i));

            Keyframe *kfUI = new Keyframe(static_cast<double>(kf->GetTime() * .001),
                                          propRow->rowTimeline());
            kfUI->binding = kf;
            kfUI->dynamic = kf->IsDynamic();
            kf->setUI(kfUI);
            propRow->rowTimeline()->insertKeyframe(kfUI);
            propRow->parentRow()->rowTimeline()->insertKeyframe(kfUI);
            if (kf->IsSelected())
                m_rowTree->m_scene->keyframeManager()->selectKeyframe(kfUI);
        }

        if (isVisible()) {
            propRow->rowTimeline()->update();
            update();
        } else {
            // Find the first visible parent and update that to show hidden keyframes
            RowTree *updateRow = m_rowTree->parentRow();
            while (updateRow && !updateRow->isVisible())
                updateRow = updateRow->parentRow();
            if (updateRow)
                updateRow->rowTimeline()->update();
        }
    }
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
    if (!m_keyframes.empty()) {
        std::partition(m_keyframes.begin(), m_keyframes.end(), [](Keyframe *kf) {
            return !kf->selected();
        });
    }

    if (m_rowTree->hasPropertyChildren()) { // has property rows
        const auto childProps = m_rowTree->childProps();
        for (auto child : childProps) {
            std::partition(child->rowTimeline()->m_keyframes.begin(),
                           child->rowTimeline()->m_keyframes.end(), [](Keyframe *kf) {
                return !kf->selected();
            });
        }
    }
}

void RowTimeline::updateKeyframes()
{
    update();

    if (m_rowTree->hasPropertyChildren()) { // master keyframes
        const auto childProps = m_rowTree->childProps();
        for (const auto child : childProps)
            child->rowTimeline()->update();
    }
}

TimelineControlType RowTimeline::getClickedControl(const QPointF &scenePos) const
{
    if (!m_rowTree->hasDurationBar())
        return TimelineControlType::None;

    QPointF p = mapFromScene(scenePos.x(), scenePos.y());
    const int halfHandle = TimelineConstants::DURATION_HANDLE_W * .5;
    // Never choose start handle if end time is zero, as you cannot adjust it in that case
    bool startHandle = p.x() > m_startX - halfHandle && p.x() < m_startX + halfHandle
            && !qFuzzyIsNull(m_endTime);
    bool endHandle = p.x() > m_endX - halfHandle && p.x() < m_endX + halfHandle;
    if (startHandle && endHandle) {
        // If handles overlap, choose the handle based on the side of the click relative to start
        startHandle = p.x() < m_startX;
        endHandle = !startHandle;
    }
    if (startHandle)
        return TimelineControlType::StartHandle;
    else if (endHandle)
        return TimelineControlType::EndHandle;
    else if (p.x() > m_startX && p.x() < m_endX && !rowTree()->locked())
        return TimelineControlType::Duration;

    return TimelineControlType::None;
}

void RowTimeline::startDurationMove(double clickX)
{
    // clickX is in ruler coordinate space
    m_startDurationMoveStartTime = m_startTime;
    m_startDurationMoveOffsetX = clickX - m_startX;
}

// move the duration area (start/end x)
void RowTimeline::moveDurationBy(double dx)
{
    if (m_startX + dx < TimelineConstants::RULER_EDGE_OFFSET)
        dx = TimelineConstants::RULER_EDGE_OFFSET - m_startX;

    m_startX += dx;
    m_endX += dx;

    if (!m_rowTree->parentRow() || m_rowTree->rowType() == OBJTYPE_LAYER
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = m_startX;
        m_maxEndX = m_endX;
    }

    m_startTime = xToTime(m_startX);
    m_endTime = xToTime(m_endX);

    // move keyframes with the row
    if (!m_rowTree->isProperty()) { // make sure we don't move the keyframes twice
        for (Keyframe *keyframe : qAsConst(m_keyframes))
            keyframe->time += rowTree()->m_scene->ruler()->distanceToTime(dx);
    }

    update();

    if (!m_rowTree->empty()) {
        updateChildrenMinStartXRecursive(m_rowTree);
        updateChildrenMaxEndXRecursive(m_rowTree);

        for (RowTree *child : qAsConst(m_rowTree->m_childRows))
            child->m_rowTimeline->moveDurationBy(dx);
    }
}

void RowTimeline::moveDurationTo(double newX)
{
    if (newX < TimelineConstants::RULER_EDGE_OFFSET)
        newX = TimelineConstants::RULER_EDGE_OFFSET;

    double dx = newX - m_startX;
    double durationX = m_endX - m_startX;

    m_startX = newX;
    m_endX = m_startX + durationX;

    if (!m_rowTree->parentRow() || m_rowTree->rowType() == OBJTYPE_LAYER
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = m_startX;
        m_maxEndX = m_endX;
    }

    m_startTime = xToTime(m_startX);
    m_endTime = xToTime(m_endX);

    // move keyframes with the row
    if (!m_rowTree->isProperty()) { // make sure we don't move the keyframes twice
        for (Keyframe *keyframe : qAsConst(m_keyframes))
            keyframe->time += rowTree()->m_scene->ruler()->distanceToTime(dx);
    }

    update();

    if (!m_rowTree->empty()) {
        updateChildrenMinStartXRecursive(m_rowTree);
        updateChildrenMaxEndXRecursive(m_rowTree);

        for (RowTree *child : qAsConst(m_rowTree->m_childRows))
            child->m_rowTimeline->moveDurationBy(dx);
    }
}

double RowTimeline::getDurationMoveTime() const
{
    return m_startTime - m_startDurationMoveStartTime;
}

double RowTimeline::getDurationMoveOffsetX() const
{
    return m_startDurationMoveOffsetX;
}

double RowTimeline::getDuration() const
{
    return m_endTime - m_startTime;
}

// convert time (seconds) values to x
double RowTimeline::timeToX(double time) const
{
    return TimelineConstants::RULER_EDGE_OFFSET + time * TimelineConstants::RULER_SEC_W
           * rowTree()->m_scene->ruler()->timelineScale();
}

// convert x values to time (seconds)
double RowTimeline::xToTime(double xPos) const
{
    return (xPos - TimelineConstants::RULER_EDGE_OFFSET)
            / (TimelineConstants::RULER_SEC_W * rowTree()->m_scene->ruler()->timelineScale());
}

void RowTimeline::collectChildKeyframeTimes(QVector<double> &childKeyframeTimes)
{
    const auto childRows = m_rowTree->childRows();
    for (const auto row : childRows) {
        row->rowTimeline()->collectChildKeyframeTimes(childKeyframeTimes);
        const auto keyframes = row->rowTimeline()->keyframes();
        for (const auto kf : keyframes)
            childKeyframeTimes.append(kf->time);
    }
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
    else if (startX > m_endX)
        startX = m_endX;

    double oldStartX = m_startX;
    m_startX = startX;
    m_startTime = xToTime(startX);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = 0;
    }

    updateChildrenStartRecursive(m_rowTree, oldStartX);
    updateChildrenMinStartXRecursive(m_rowTree);
    update();
}

// Set the position of the end of the row duration
void RowTimeline::setEndX(double endX)
{
    if (endX < m_startX)
        endX = m_startX;

    double oldEndX = m_endX;
    m_endX = endX;
    m_endTime = xToTime(endX);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
        || m_rowTree->hasComponentAncestor()) {
        m_maxEndX = 999999;
    }

    updateChildrenEndRecursive(m_rowTree, oldEndX);
    updateChildrenMaxEndXRecursive(m_rowTree);
    update();
}

QColor RowTimeline::barColor() const
{
    return m_barColor;
}

void RowTimeline::setBarColor(const QColor &color)
{
    m_barColor = color;
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
        bool isComponentChild = m_rowTree->rowType() == OBJTYPE_COMPONENT
                || m_rowTree->hasComponentAncestor();
        for (auto child : childRows) {
            if (isComponentChild) {
                child->rowTimeline()->m_minStartX = 0;
            } else {
                child->rowTimeline()->m_minStartX = qMax(rowTree->rowTimeline()->m_startX,
                                                         rowTree->rowTimeline()->m_minStartX);
            }
            child->rowTimeline()->update();

            updateChildrenMinStartXRecursive(child);
        }
    }
}

void RowTimeline::updateChildrenMaxEndXRecursive(RowTree *rowTree)
{
    if (m_rowTree->rowType() != OBJTYPE_SCENE && !rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        bool isComponentChild = m_rowTree->rowType() == OBJTYPE_COMPONENT
                || m_rowTree->hasComponentAncestor();
        for (auto child : childRows) {
            if (isComponentChild) {
                child->rowTimeline()->m_maxEndX = 999999;
            } else {
                child->rowTimeline()->m_maxEndX = qMin(rowTree->rowTimeline()->m_endX,
                                                       rowTree->rowTimeline()->m_maxEndX);
            }
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

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = 0;
    }

    updateChildrenStartRecursive(m_rowTree, oldStartX);
    updateChildrenMinStartXRecursive(m_rowTree);
    update();
}

void RowTimeline::setEndTime(double endTime)
{
    double oldEndX = m_endX;
    m_endTime = endTime;
    m_endX = timeToX(endTime);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
            || m_rowTree->hasComponentAncestor()) {
        m_maxEndX = 999999;
    }

    updateChildrenEndRecursive(m_rowTree, oldEndX);
    updateChildrenMaxEndXRecursive(m_rowTree);
    update();
}

double RowTimeline::getStartX() const
{
    return m_startX;
}

double RowTimeline::getEndX() const
{
    return m_endX;
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
    if (m_rowTree->isProperty()) {
        if (m_propertyGraph)
            delete m_propertyGraph;
        m_propertyGraph = new RowTimelinePropertyGraph(this);
    }
}

RowTree *RowTimeline::rowTree() const
{
    return m_rowTree;
}

QList<Keyframe *> RowTimeline::keyframes() const
{
    return m_keyframes;
}

QString RowTimeline::formatTime(double seconds) const
{
    static const QString timeTemplate = tr("%1:%2.%3");
    static const QChar fillChar = tr("0").at(0);

    long mins = seconds / 60;
    long secs = seconds - mins * 60;
    long millis = qRound((seconds - (int)seconds) * 1000);

    return timeTemplate.arg(mins).arg(secs, 2, 10, fillChar).arg(millis, 3, 10, fillChar);
}

void RowTimeline::showToolTip(const QPointF &pos)
{
    QLabel *tooltip = m_rowTree->m_scene->timebarTooltip();

    tooltip->setText(formatTime(m_startTime) + " - " + formatTime(m_endTime)
                     + " (" + formatTime(m_endTime - m_startTime) + ")");

    tooltip->adjustSize();

    QPoint newPos = pos.toPoint() + QPoint(-tooltip->width() / 2,
                     -tooltip->height() - TimelineConstants::TIMEBAR_TOOLTIP_OFFSET_V);

    // Confine the tooltip to the current screen area to avoid artifacts from different pixel ratios
    static const int MARGIN = 5;
    const QRect screenGeometry = QApplication::desktop()->screenGeometry(
                m_rowTree->m_scene->widgetTimeline());
    int xMin = screenGeometry.x() + MARGIN;
    int xMax = screenGeometry.x() + screenGeometry.width() - tooltip->width() - MARGIN;
    if (newPos.x() < xMin)
        newPos.setX(xMin);
    else if (newPos.x() > xMax)
        newPos.setX(xMax);

    tooltip->move(newPos);
    tooltip->raise();
    tooltip->show();
}

RowTimeline *RowTimeline::parentRow() const
{
    if (!m_rowTree->m_parentRow)
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
