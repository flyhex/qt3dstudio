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

#include "RowTimelinePropertyGraph.h"
#include "RowTimeline.h"
#include "RowTree.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMAnimation.h"
#include "Keyframe.h"
#include "Ruler.h"
#include "TimelineGraphicsScene.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "Bindings/ITimelineItemProperty.h"

using namespace qt3dsdm;
using namespace TimelineConstants;

RowTimelinePropertyGraph::RowTimelinePropertyGraph(QObject *parent)
    : QObject(parent)
    , m_rowTimeline(static_cast<RowTimeline *>(parent))
    , m_animCore(g_StudioApp.GetCore()->GetDoc()->GetAnimationCore())
{
    m_fitCurveTimer.setInterval(10);

    // smooth animation for when curve height change
    connect(&m_fitCurveTimer, &QTimer::timeout, [this]() {
        fitGraph();
        if (m_rowTimeline->size().height() == m_expandHeight)
            m_fitCurveTimer.stop();
    });
}

void RowTimelinePropertyGraph::paintGraphs(QPainter *painter, const QRectF &rect)
{
    m_propBinding = m_rowTimeline->rowTree()->propBinding();

    if (rect.height() < ROW_H) // rect height = row height - 1
        return;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setClipRect(rect);

    static const QPointF edgeOffset(RULER_EDGE_OFFSET, 0);
    double timelineScale = m_rowTimeline->rowTree()->m_scene->ruler()->timelineScale();

    // draw graph base line (graph_Y)
    painter->setPen(QPen(CStudioPreferences::studioColor3()));
    painter->drawLine(edgeOffset.x(), m_graphY, rect.right(), m_graphY);

    // draw value ruler
    static const int STEP_MIN = 20;
    static const int STEP_MAX = 40;
    static const QVector<qreal> RULER_VALS {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};

    int rulerValIdx = 5; // start at val 100
    qreal step = RULER_VALS[rulerValIdx] * m_valScale;

    while (step < STEP_MIN && rulerValIdx < RULER_VALS.size() - 1)
        step = RULER_VALS[++rulerValIdx] * m_valScale;
    while (step > STEP_MAX && rulerValIdx > 0)
        step = RULER_VALS[--rulerValIdx] * m_valScale;

    qreal dy = rect.bottom() - m_graphY;
    qreal d_start = dy > 0 ? std::fmod(dy, step) : step - std::fmod(-dy, step); // start delta
    qreal start_y = rect.bottom() - d_start;
    painter->setFont(QFont(CStudioPreferences::fontFaceName(), 7));
    for (qreal i = start_y; i > rect.y(); i -= step) {
        // draw ruler line
        painter->setPen(QPen(CStudioPreferences::studioColor2(), 1));
        painter->drawLine(rect.x(), rect.y() + i, rect.right(), rect.y() + i);

        // draw ruler value text
        painter->setPen(QPen(CStudioPreferences::studioColor3(), 1));
        qreal val_i = (m_graphY - i) / m_valScale;
        painter->drawText(rect.x() + edgeOffset.x() + 8, i - 2, QString::number(qRound(val_i)));
    }

    // draw vertical keyframe separator lines
    painter->setPen(QPen(CStudioPreferences::studioColor2(), 1));
    Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
    m_animCore->GetKeyframes(m_activeChannels[0], keyframeHandles);
    for (size_t i = 0; i < keyframeHandles.size(); ++i) {
        TKeyframe kf = m_animCore->GetKeyframeData(keyframeHandles[i]);
        QPointF centerPos = getKeyframePosition(getKeyframeTime(kf), getKeyframeValue(kf))
                            + edgeOffset;
        painter->drawLine(centerPos.x(), rect.y(), centerPos.x(), rect.height());
    }

    // draw channel curves
    painter->setPen(CStudioPreferences::studioColor3()); // default to locked color
    for (size_t i = 0; i < m_activeChannels.size(); ++i) {
        QPainterPath path;
        int start_j = qMax(rect.x(), edgeOffset.x());
        for (int j = start_j; j < rect.right(); j += 5) { // 5 = sampling step in pixels
            long time = (j - edgeOffset.x()) / (RULER_MILLI_W * timelineScale); // millis
            float value = m_propBinding->GetChannelValueAtTime(m_activeChannelsIndex[i], time);
            adjustColorProperty(value);

            qreal yPos = m_graphY - value * m_valScale;

            if (j == start_j)
                path.moveTo(j, yPos);
            else
                path.lineTo(j, yPos);
        }

        static const QColor chColors[4] {
            CStudioPreferences::xAxisColor(),
            CStudioPreferences::yAxisColor(),
            CStudioPreferences::zAxisColor(),
            CStudioPreferences::wAxisColor()
        };

        if (!m_rowTimeline->rowTree()->locked())
            painter->setPen(chColors[m_activeChannelsIndex[i]]);
        painter->drawPath(path);
    }

    // draw bezier keyframes and their control points
    if (m_rowTimeline->rowTree()->propBinding()->animationType() == EAnimationTypeBezier) {
        for (size_t i = 0; i < m_activeChannels.size(); ++i) {
            // draw bezier control points
            static const QPixmap pixBezierHandle("://images/breadcrumb_component_button.png");

            Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
            m_animCore->GetKeyframes(m_activeChannels[i], keyframeHandles);

            for (auto &kfHandle : keyframeHandles) {
                SBezierKeyframe kf = get<SBezierKeyframe>(m_animCore->GetKeyframeData(kfHandle));

                QPointF centerPos = getBezierControlPosition(kf) + edgeOffset;
                const QPointF PIX_HALF_W = QPointF(8.0, 8.0);

                bool kfSelected = m_selectedBezierKeyframes.contains(kfHandle)
                                  && !m_rowTimeline->rowTree()->locked();
                if (kfSelected) {
                    // draw tangent-in part
                    painter->setPen(CStudioPreferences::bezierControlColor());
                    if (kfHandle != *keyframeHandles.begin()) {
                        QPointF cInPos = getBezierControlPosition(kf, BezierControlType::In)
                                         + edgeOffset;
                        painter->drawLine(cInPos, centerPos);
                        painter->drawPixmap(cInPos - PIX_HALF_W, pixBezierHandle);
                    }

                    // draw tangent-out part
                    if (kfHandle != *(keyframeHandles.end() - 1)) {
                        QPointF cOutPos = getBezierControlPosition(kf, BezierControlType::Out)
                                          + edgeOffset;
                        painter->drawLine(cOutPos, centerPos);
                        painter->drawPixmap(cOutPos - PIX_HALF_W, pixBezierHandle);
                    }
                } else if (kfHandle == m_hoveredBezierKeyframe) {
                    // a bezier keyframe hovered
                    painter->setPen(QPen(CStudioPreferences::bezierControlColor()));
                    painter->drawRect(centerPos.x() - 4, centerPos.y() - 4, 8, 8);
                }

                // draw center point
                painter->setPen(QPen(m_rowTimeline->rowTree()->locked()
                                     ? CStudioPreferences::studioColor3()
                                     : CStudioPreferences::bezierControlColor(), kfSelected
                                                                                    ? 6 : 3));
                painter->drawPoint(centerPos);
            }
        }
    }
}

/**
 * This method is called when the user mouse-presses the property graph. If the press is on a
 * bezier keyframe handle, it returns the handle type and saves the list of keyframe channels
 * handles to be precessed while the user drags the handle (in updateBezierControlValue())
 *
 * @param pos press position in local coordinate system
 * @param isHover when true this is a hover, else a click
 * @return the pressed handle type, or None if no handle is pressed
 */
TimelineControlType RowTimelinePropertyGraph::getClickedBezierControl(const QPointF &pos,
                                                                      bool isHover)
{
    if (m_rowTimeline->rowTree()->propBinding()->animationType() != EAnimationTypeBezier)
        return TimelineControlType::None;

    bool aKeyframeWasHovered = m_hoveredBezierKeyframe.Valid();

    // reset data
    m_hoveredBezierKeyframe = 0;
    m_currKeyframeData.first = 0;

    if (aKeyframeWasHovered)
        m_rowTimeline->update();

    for (size_t i = 0; i < m_activeChannels.size(); ++i) {
        Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
        m_animCore->GetKeyframes(m_activeChannels[i], keyframeHandles);
        const double CONTROL_RADIUS = 8;
        for (auto kfHandle : keyframeHandles) {
            SBezierKeyframe kf = get<SBezierKeyframe>(m_animCore->GetKeyframeData(kfHandle));

            QPointF kfPos = getBezierControlPosition(kf);
            if (QLineF(kfPos, pos).length() < CONTROL_RADIUS) { // over a bezier keyframe
                m_currKeyframeData.first = kfHandle;
                m_currKeyframeData.second = m_animCore->GetKeyframeData(kfHandle);
                if (isHover) {
                    m_hoveredBezierKeyframe = kfHandle;
                } else {
                    if (!CHotKeys::isCtrlDown())
                        m_selectedBezierKeyframes.clear();

                    m_selectedBezierKeyframes.insert(kfHandle);
                }
                m_rowTimeline->update();

                return TimelineControlType::BezierKeyframe;
            }

            if (m_selectedBezierKeyframes.contains(kfHandle)) {
                QPointF cInPos = getBezierControlPosition(kf, BezierControlType::In);
                QPointF cOutPos = getBezierControlPosition(kf, BezierControlType::Out);
                bool clickedInHandle = QLineF(cInPos, pos).length() < CONTROL_RADIUS;
                bool clickedOutHandle = QLineF(cOutPos, pos).length() < CONTROL_RADIUS;
                if (clickedInHandle || clickedOutHandle) {
                    m_currKeyframeData.first = kfHandle;
                    m_currKeyframeData.second = m_animCore->GetKeyframeData(kfHandle);

                    return clickedInHandle ? TimelineControlType::BezierInHandle
                                           : TimelineControlType::BezierOutHandle;
                }
            }
        }
    }

    return TimelineControlType::None;
}

QPointF RowTimelinePropertyGraph::getBezierControlPosition(const SBezierKeyframe &kf,
                                                           BezierControlType type) const
{
    long time = 0;
    float value = 0;
    if (type == BezierControlType::None) {
        time = kf.m_time;
        value = kf.m_value;
    } else if (type == BezierControlType::In) {
        time = kf.m_InTangentTime;
        value = kf.m_InTangentValue;
    } else if (type == BezierControlType::Out) {
        time = kf.m_OutTangentTime;
        value = kf.m_OutTangentValue;
    }

    return getKeyframePosition(time, value);
}

QPointF RowTimelinePropertyGraph::getKeyframePosition(long time, float value) const
{
    adjustColorProperty(value);

    return QPointF(m_rowTimeline->rowTree()->m_scene->ruler()->timeToDistance(time),
                   m_graphY - value * m_valScale);
}

/**
 * This method is called when the user drags a bezier handle. It updates the bezier keyframe
 * tangent value based on the current position of the handle.
 *
 * @param controlType which handle is being dragged? (BezierInHandle or BezierOutHandle)
 * @param scenePos handle position in timeline scene coordinates
 */
void RowTimelinePropertyGraph::updateBezierControlValue(TimelineControlType controlType,
                                                        const QPointF &scenePos)
{
    QPointF p = m_rowTimeline->mapFromScene(scenePos.x() - RULER_EDGE_OFFSET, scenePos.y());

    // time and value at current mouse position
    long time = m_rowTimeline->rowTree()->m_scene->ruler()->distanceToTime(p.x());
    float value = (m_graphY - p.y()) / m_valScale;
    adjustColorProperty(value, false);

    SBezierKeyframe kf = get<SBezierKeyframe>(m_currKeyframeData.second);
    bool isBezierIn = controlType == TimelineControlType::BezierInHandle;

    // prevent handles from moving to the other side of the keyframe
    if ((isBezierIn && time > kf.m_time)
        || (!isBezierIn && time < kf.m_time)) {
        time = kf.m_time;
    }

    // prevent handles from going beyond prev. and next keyframes
    Qt3DSDMAnimationHandle anim = m_animCore->GetAnimationForKeyframe(m_currKeyframeData.first);
    Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
    m_animCore->GetKeyframes(anim, keyframeHandles);
    for (size_t i = 0; i < keyframeHandles.size(); ++i) {
        if (keyframeHandles[i] == m_currKeyframeData.first) {
            long currKfTime = getKeyframeTime(m_animCore->GetKeyframeData(keyframeHandles[i]));
            long prevKfTime = i > 0
                    ? getKeyframeTime(m_animCore->GetKeyframeData(keyframeHandles[i - 1]))
                    : LONG_MIN / 2;
            long nextKfTime = i < keyframeHandles.size() - 1
                    ? getKeyframeTime(m_animCore->GetKeyframeData(keyframeHandles[i + 1]))
                    : LONG_MAX / 2;

            if (isBezierIn) {
                if (time < prevKfTime)
                    time = prevKfTime;
                if (!CHotKeys::isCtrlDown() && time < currKfTime * 2 - nextKfTime)
                    time = currKfTime * 2 - nextKfTime;
            } else { // bezier out
                if (time > nextKfTime)
                    time = nextKfTime;
                if (!CHotKeys::isCtrlDown() && time > currKfTime * 2 - prevKfTime)
                    time = currKfTime * 2 - prevKfTime;
            }
            break;
        }
    }

    long &currHandleTime = isBezierIn ? kf.m_InTangentTime : kf.m_OutTangentTime;
    float &currHandleValue = isBezierIn ? kf.m_InTangentValue : kf.m_OutTangentValue;
    long &otherHandleTime = isBezierIn ? kf.m_OutTangentTime : kf.m_InTangentTime;
    float &otherHandleValue = isBezierIn ? kf.m_OutTangentValue : kf.m_InTangentValue;

    currHandleTime = time;
    currHandleValue = value;

    if (!CHotKeys::isCtrlDown()) {
        otherHandleTime = kf.m_time + (kf.m_time - time);
        otherHandleValue = kf.m_value + (kf.m_value - currHandleValue);
    }

    m_animCore->SetKeyframeData(m_currKeyframeData.first, kf);
}

void RowTimelinePropertyGraph::updateChannelFiltering(const QVector<bool> &activeChannels)
{
    m_activeChannels.clear();
    m_activeChannelsIndex.clear();

    const auto animHandles = m_propBinding->animationHandles();
    for (int i = 0; i < activeChannels.size(); ++i) {
        if (activeChannels[i]) {
            m_activeChannels.append(animHandles[i]);
            m_activeChannelsIndex.append(i);
        }
    }

    fitGraph();
}

// adjust graph scale and y so that all keyframe and control points are visible
void RowTimelinePropertyGraph::fitGraph()
{
    // get min/max keyframes values in the active channels
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    for (int i = 0; i < m_activeChannels.size(); ++i) {
        Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
        m_animCore->GetKeyframes(m_activeChannels[i], keyframeHandles);
        for (auto kfHandle : keyframeHandles) {
            TKeyframe keyframeData = m_animCore->GetKeyframeData(kfHandle);
            float value = getKeyframeValue(keyframeData);
            if (value < minVal)
                minVal = value;
            if (value > maxVal)
                maxVal = value;

            // for bezier keyframes compare tangents in/out also
            if (keyframeData.getType() == qt3dsdm::EAnimationTypeBezier) {
                long timeIn, timeOut;
                float valueIn, valueOut;
                getBezierValues(keyframeData, timeIn, valueIn, timeOut, valueOut);

                if (!m_animCore->IsFirstKeyframe(kfHandle)) { // check tangent-in value
                    if (valueIn < minVal)
                        minVal = valueIn;
                    if (valueIn > maxVal)
                        maxVal = valueIn;
                }

                if (!m_animCore->IsLastKeyframe(kfHandle)) { // check tangent-out value
                    if (valueOut < minVal)
                        minVal = valueOut;
                    if (valueOut > maxVal)
                        maxVal = valueOut;
                }
            }
        }
    }

    adjustColorProperty(maxVal);
    adjustColorProperty(minVal);

    const float marginT = 20.f;
    const float marginB = 10.f;
    const float graphH = float(m_rowTimeline->size().height()) - (marginT + marginB);
    m_valScale = graphH / (maxVal - minVal);
    checkValScaleLimits();

    m_graphY = marginT + maxVal * m_valScale;

    m_rowTimeline->update();
}

// show color properties values in the range 0-255
void RowTimelinePropertyGraph::adjustColorProperty(float &val, bool scaleUp) const
{
    if (m_rowTimeline->isColorProperty())
        scaleUp ? val *= 255.f : val /= 255.f;
}

void RowTimelinePropertyGraph::checkValScaleLimits()
{
    // m_valScale can be NaN if maxVal and minVal are same (i.e. horizontal line curve)
    if (isnan(m_valScale) || m_valScale > 10.f)
        m_valScale = 10.f;
    else if (m_valScale < .01f)
        m_valScale = .01f;
}

void RowTimelinePropertyGraph::selectBezierKeyframesInRange(const QRectF &rect)
{
    QRectF localRect = m_rowTimeline->mapFromScene(rect).boundingRect();
    localRect.translate(-RULER_EDGE_OFFSET, 0);

    for (int i = 0; i < m_activeChannels.size(); ++i) {
        Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
        m_animCore->GetKeyframes(m_activeChannels[i], keyframeHandles);
        for (auto kfHandle : keyframeHandles) {
            SBezierKeyframe kf = get<SBezierKeyframe>(m_animCore->GetKeyframeData(kfHandle));

            QPointF kfPosition = getKeyframePosition(getKeyframeTime(kf), getKeyframeValue(kf));
            if (localRect.contains(kfPosition))
                m_selectedBezierKeyframes.insert(kfHandle);
            else
                m_selectedBezierKeyframes.remove(kfHandle);
        }
    }
}

void RowTimelinePropertyGraph::deselectAllBezierKeyframes()
{
    if (!m_currKeyframeData.first.Valid()) // not currently editing a bezier control
        m_selectedBezierKeyframes.clear();
}

void RowTimelinePropertyGraph::adjustScale(const QPointF &scenePos, bool isIncrement)
{
    QPointF p = m_rowTimeline->mapFromScene(scenePos.x() - RULER_EDGE_OFFSET, scenePos.y());

    float oldScale = m_valScale;
    float pitch = m_valScale * .3f;
    m_valScale += isIncrement ? pitch : -pitch;
    checkValScaleLimits();

    float d1 = m_graphY - p.y();           // dY before scale
    float d2 = d1 * m_valScale / oldScale; // dY after scale

    m_graphY += d2 - d1;

    m_rowTimeline->update();
}

void RowTimelinePropertyGraph::startPan()
{
    m_graphYPanInit = m_graphY;
}

void RowTimelinePropertyGraph::pan(qreal dy)
{
    m_graphY = m_graphYPanInit + dy;
    m_rowTimeline->update();
}

void RowTimelinePropertyGraph::commitBezierEdit()
{
    // reset the moved keyframe and commit the change, so the undo/redo works correctly
    // Mahmoud_TODO: improve this (using Updatable editor?)
    TKeyframe movedKeyframe = m_animCore->GetKeyframeData(m_currKeyframeData.first);
    m_animCore->SetKeyframeData(m_currKeyframeData.first, m_currKeyframeData.second);

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), tr("Edit Bezier curve"))
            ->setBezierKeyframeValue(m_currKeyframeData.first, movedKeyframe);
}

void RowTimelinePropertyGraph::setExpandHeight(int h)
{
    m_expandHeight = h;
    m_fitCurveTimer.start();
}
