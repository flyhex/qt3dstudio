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
#include "Ruler.h"
#include "TimelineGraphicsScene.h"
#include "Bindings/ITimelineItemProperty.h"

RowTimelinePropertyGraph::RowTimelinePropertyGraph(QObject *parent)
    : QObject(parent)
{
    m_rowTimeline = static_cast<RowTimeline *>(parent);
}

void RowTimelinePropertyGraph::paintGraphs(QPainter *painter, const QRectF &rect)
{
    m_rect = rect;
    m_propBinding = m_rowTimeline->rowTree()->propBinding();

    // Animate alpha 0..255 while expanding
    int alpha = 255 * (m_rect.height() - TimelineConstants::ROW_H)
            / (TimelineConstants::ROW_H_EXPANDED - TimelineConstants::ROW_H);
    alpha = std::max(0, alpha);

    if (alpha == 0)
        return;

    // Available line colors
    QColor colors[6] = { QColor(255, 0, 0, alpha), QColor(0, 255, 0, alpha),
                         QColor(0, 0, 255, alpha), QColor(255, 255, 0, alpha),
                         QColor(255, 0, 255, alpha), QColor(0, 255, 255, alpha) };

    long channelCount = m_propBinding->GetChannelCount();

    // Don't want to overflow the color array
    if (channelCount <= 6) {
        // For each channel graph it.
        for (long i = 0; i < channelCount; ++i)
            paintSingleChannel(painter, i, colors[i]);
    }
}

void RowTimelinePropertyGraph::paintSingleChannel(QPainter *painter, long inChannelIndex,
                                                  const QColor &inColor)
{
    float maxVal = m_propBinding->GetMaximumValue();
    float minVal = m_propBinding->GetMinimumValue();

    double timelineScale = m_rowTimeline->rowTree()->m_scene->ruler()->timelineScale();

    // Step in pixels
    int interval = 5;
    // Margin at top & bottom of graph
    float marginY = 10;
    float graphY = m_rect.y() + marginY;
    float graphHeight = m_rect.height() - marginY * 2;

    QPainterPath path;
    for (int i = 0; i < m_rect.width(); i += interval) {
        // Value time in ms
        long time = 1000 * (i / (TimelineConstants::RULER_SEC_W * timelineScale));
        float value = m_propBinding->GetChannelValueAtTime(inChannelIndex, time);
        float yPos = graphY + (1.0 - (value - minVal) / (maxVal - minVal)) * graphHeight;

        if (i == 0)
            path.moveTo(m_rect.x() + i, yPos);
        else
            path.lineTo(m_rect.x() + i, yPos);
    }

    painter->setPen(QPen(inColor, 2));
    painter->drawPath(path);
}
