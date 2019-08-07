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

    const QPointF edgeOffset(RULER_EDGE_OFFSET, 0);
    double timelineScale = m_rowTimeline->rowTree()->m_scene->ruler()->timelineScale();

    // draw graph base line (graph_Y)
    painter->setPen(QPen(CStudioPreferences::studioColor3()));
    painter->drawLine(edgeOffset.x(), m_graphY, rect.right(), m_graphY);

    // draw channel curves and control points (for bezier)
    auto animHandles = m_propBinding->animationHandles();
    bool isBezier = m_rowTimeline->rowTree()->propBinding()->animationType()
                    == EAnimationTypeBezier;
    for (size_t channelIndex = 0; channelIndex < animHandles.size(); ++channelIndex) {
        if (m_rowTimeline->rowTree()->channelActive(channelIndex)) {
            // draw channel curve
            // Mahmoud_TODO: this block will draw the channel curves without sampling which is more
            // efficient, but it cannot be used till the bezier equation is fixed (check:QT3DS-3777)
            //-------------------------------draw using cubicTo ------------------------------------
//            QPainterPath path2;
//            Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
//            m_animCore->GetKeyframes(animHandles[channelIndex], keyframeHandles);
//            if (!keyframeHandles.empty()) {
//                TKeyframe kf0 = m_animCore->GetKeyframeData(keyframeHandles[0]);
//                path2.moveTo(getKeyframePosition(GetKeyframeSeconds(kf0), KeyframeValueValue(kf0))
//                             + edgeOffset);

//                for (int i = 1; i < keyframeHandles.size(); ++i) {
//                    TKeyframe kf1 = m_animCore->GetKeyframeData(keyframeHandles[i]);
//                    float kf0InTime = -1, kf0InValue, kf0OutTime, kf0OutValue;
//                    float kf1InTime = -1, kf1InValue, kf1OutTime, kf1OutValue;
//                    getBezierValues(kf0, kf0InTime, kf0InValue, kf0OutTime, kf0OutValue);
//                    getBezierValues(kf1, kf1InTime, kf1InValue, kf1OutTime, kf1OutValue);
//                    if (kf0InTime != -1) {
//                        path2.cubicTo(getKeyframePosition(kf0OutTime, kf0OutValue) + edgeOffset,
//                                      getKeyframePosition(kf1InTime, kf1InValue) + edgeOffset,
//                                      getKeyframePosition(GetKeyframeSeconds(kf1),
//                                                          KeyframeValueValue(kf1))
//                                      + edgeOffset);
//                    } else {
//                        path2.lineTo(getKeyframePosition(GetKeyframeSeconds(kf1),
//                                                         KeyframeValueValue(kf1))
//                                     + edgeOffset);
//                    }
//                    // Mahmoud_TODO: handle ease in/out curves as well
//                    kf0 = kf1;
//                }
//                painter->setPen(QPen(QColor("#ff0000"), 1));
//                painter->drawPath(path2);
//            }
            //-------------------------------draw using cubicTo ------------------------------------

            QPainterPath path;
            int start_i = qMax(rect.x(), edgeOffset.x());
            for (int i = start_i; i < rect.right(); i += 5) { // 5 = sampling step in pixels
                // Value time in ms
                long time = (i - edgeOffset.x()) / (RULER_MILLI_W * timelineScale);
                qreal value = m_propBinding->GetChannelValueAtTime(channelIndex, time);
                qreal yPos = m_graphY - value * m_valScale;
                if (i == start_i)
                    path.moveTo(i, yPos);
                else
                    path.lineTo(i, yPos);
            }
            QColor channelColor;
            if (channelIndex == 0)
                channelColor = CStudioPreferences::GetXAxisColor();
            else if (channelIndex == 1)
                channelColor = CStudioPreferences::GetYAxisColor();
            else if (channelIndex == 2)
                channelColor = CStudioPreferences::GetZAxisColor();
            painter->setPen(QPen(channelColor, 2));
            painter->drawPath(path);

            // draw bezier control points
            if (isBezier) {
                static const QPixmap pixBezierHandle("://images/breadcrumb_component_button.png");
                static const QPixmap pixBezierHandlePressed("://images/breadcrumb_component_grey"
                                                            "_button.png");

                Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
                m_animCore->GetKeyframes(animHandles[channelIndex], keyframeHandles);

                size_t kfHandlesSize = keyframeHandles.size();
                for (size_t i = 0; i < kfHandlesSize; ++i) {
                    SBezierKeyframe kf = get<SBezierKeyframe>(m_animCore->GetKeyframeData(
                                                                  keyframeHandles[i]));

                    QPointF centerPos = getBezierControlPosition(kf) + edgeOffset;
                    const QPointF PIX_HALF_W = QPointF(8.0, 8.0);

                    // draw vertical keyframe separator line
                    painter->setPen(QPen(CStudioPreferences::studioColor2(), 1));
                    painter->drawLine(centerPos.x(), rect.y(), centerPos.x(), rect.height());

                    // draw tangent-in part
                    painter->setPen(CStudioPreferences::getBezierControlColor());
                    if (i > 0) {
                        QPointF cInPos = getBezierControlPosition(kf, BezierControlType::In)
                                         + edgeOffset;
                        painter->drawLine(cInPos, centerPos);
                        painter->drawPixmap(cInPos - PIX_HALF_W, pixBezierHandle);
                    }

                    // draw tangent-out part
                    if (i < kfHandlesSize - 1) {
                        QPointF cOutPos = getBezierControlPosition(kf, BezierControlType::Out)
                                          + edgeOffset;
                        painter->drawLine(cOutPos, centerPos);
                        painter->drawPixmap(cOutPos - PIX_HALF_W, pixBezierHandle);
                    }

                    // draw center point
                    painter->setPen(QPen(CStudioPreferences::getBezierControlColor(), 3));
                    painter->drawPoint(centerPos);
                }
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
 * @return the pressed handle type, or None if no handle is pressed
 */
TimelineControlType RowTimelinePropertyGraph::getClickedBezierControl(const QPointF &pos)
{
    if (m_rowTimeline->rowTree()->propBinding()->animationType() != EAnimationTypeBezier)
        return TimelineControlType::None;

    m_currKeyframeData.first = 0; // reset the data

    auto animHandles = m_propBinding->animationHandles();
    for (size_t chIndex = 0; chIndex < animHandles.size(); ++chIndex) {
        if (m_rowTimeline->rowTree()->channelActive(chIndex)) {
            Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
            m_animCore->GetKeyframes(animHandles[chIndex], keyframeHandles);
            const double CONTROL_RADIUS = 8;
            for (auto kfHandle : keyframeHandles) {
                SBezierKeyframe kf = get<SBezierKeyframe>(m_animCore->GetKeyframeData(kfHandle));

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
    float time = 0; // seconds
    float value = 0;
    if (type == BezierControlType::None) {
        time = kf.m_KeyframeSeconds;
        value = kf.m_KeyframeValue;
    } else if (type == BezierControlType::In) {
        time = kf.m_InTangentTime;
        value = kf.m_InTangentValue;
    } else if (type == BezierControlType::Out) {
        time = kf.m_OutTangentTime;
        value = kf.m_OutTangentValue;
    }

    return getKeyframePosition(time, value);
}

// time is in seconds
QPointF RowTimelinePropertyGraph::getKeyframePosition(float time, float value) const
{
    return QPointF(m_rowTimeline->rowTree()->m_scene->ruler()->timeToDistance(time * 1000),
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
    float time = m_rowTimeline->rowTree()->m_scene->ruler()->distanceToTime(p.x()) / 1000.f; // secs
    float value = (m_graphY - p.y()) / m_valScale;

    SBezierKeyframe kf = get<SBezierKeyframe>(m_currKeyframeData.second);
    bool isBezierIn = controlType == TimelineControlType::BezierInHandle;

    // prevent handles from moving to the other side of the keyframe
    if ((isBezierIn && time > kf.m_KeyframeSeconds)
        || (!isBezierIn && time < kf.m_KeyframeSeconds)) {
        time = kf.m_KeyframeSeconds;
    }

    // prevent handles from going beyond prev. and next keyframes
    Qt3DSDMAnimationHandle anim = m_animCore->GetAnimationForKeyframe(m_currKeyframeData.first);
    Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
    m_animCore->GetKeyframes(anim, keyframeHandles);
    for (size_t i = 0; i < keyframeHandles.size(); ++i) {
        if (keyframeHandles[i] == m_currKeyframeData.first) {
            float currKfTime = KeyframeTime(m_animCore->GetKeyframeData(keyframeHandles[i]));
            float prevKfTime = i > 0
                    ? KeyframeTime(m_animCore->GetKeyframeData(keyframeHandles[i - 1])) : -FLT_MAX;
            float nextKfTime = i < keyframeHandles.size() - 1
                    ? KeyframeTime(m_animCore->GetKeyframeData(keyframeHandles[i + 1])) : FLT_MAX;
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

    float &currHandleTime = isBezierIn ? kf.m_InTangentTime : kf.m_OutTangentTime;
    float &currHandleValue = isBezierIn ? kf.m_InTangentValue : kf.m_OutTangentValue;
    float &otherHandleTime = isBezierIn ? kf.m_OutTangentTime : kf.m_InTangentTime;
    float &otherHandleValue = isBezierIn ? kf.m_OutTangentValue : kf.m_InTangentValue;

    currHandleTime = time;
    currHandleValue = value;

    if (!CHotKeys::isCtrlDown()) {
        otherHandleTime = kf.m_KeyframeSeconds + (kf.m_KeyframeSeconds - time);
        otherHandleValue = kf.m_KeyframeValue + (kf.m_KeyframeValue - currHandleValue);
    }

    m_animCore->SetKeyframeData(m_currKeyframeData.first, kf);
}

// adjust graph scale and y so that all keyframe and control points are visible
void RowTimelinePropertyGraph::fitGraph()
{
    const qreal MARGIN_Y = 10; // margin at top & bottom of graph
    m_graphY = m_rowTimeline->size().height() - MARGIN_Y;
    m_graphH = m_rowTimeline->size().height() - MARGIN_Y * 2;

    // get min/max keyframes values in the active channels
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    const auto animHandles = m_propBinding->animationHandles();
    for (int i = 0; i < animHandles.size(); ++i) {
        if (m_rowTimeline->rowTree()->channelActive(i)) {
            Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
            m_animCore->GetKeyframes(animHandles[i], keyframeHandles);
            for (auto kfHandle : keyframeHandles) {
                TKeyframe keyframeData = m_animCore->GetKeyframeData(kfHandle);
                float value = KeyframeValueValue(keyframeData);
                if (value < minVal)
                    minVal = value;
                if (value > maxVal)
                    maxVal = value;

                // for bezier keyframes compare tangents in/out also
                if (keyframeData.getType() == qt3dsdm::EAnimationTypeBezier) {
                    float timeIn, valueIn, timeOut, valueOut;
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
    }

    m_valScale = m_graphH / (maxVal - minVal);
    m_graphY += minVal * m_valScale;

    m_rowTimeline->update();
}

void RowTimelinePropertyGraph::adjustScale(bool isIncrement)
{
    float pitch = m_valScale * .3f;
    m_valScale += isIncrement ? pitch : -pitch;

    if (m_valScale > 10.f)
        m_valScale = 10.f;
    else if (m_valScale < .01f)
        m_valScale = .01f;

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
