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

#include "Ruler.h"
#include "TimelineConstants.h"

#include <QtGui/qpainter.h>
#include <QtWidgets/qwidget.h>
#include <QtCore/qdebug.h>

Ruler::Ruler(TimelineItem *parent) : TimelineItem(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void Ruler::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)

    double xStep = TimelineConstants::RULER_SEC_W / TimelineConstants::RULER_SEC_DIV * m_timeScale;
    double totalSegmentsWidth = TimelineConstants::RULER_EDGE_OFFSET
            + m_duration * xStep * TimelineConstants::RULER_SEC_DIV;

    // Ruler painted width to be at least widget width
    double minRulerWidth = widget->width();
    double rowXMax = std::max(minRulerWidth, totalSegmentsWidth);

    painter->save();
    painter->setPen(QColor(TimelineConstants::RULER_COLOR_DISABLED));
    painter->drawLine(TimelineConstants::RULER_EDGE_OFFSET,
                      TimelineConstants::RULER_BASE_Y,
                      rowXMax + TimelineConstants::RULER_EDGE_OFFSET,
                      TimelineConstants::RULER_BASE_Y);
    painter->setPen(QColor(TimelineConstants::RULER_COLOR));
    painter->drawLine(TimelineConstants::RULER_EDGE_OFFSET,
                      TimelineConstants::RULER_BASE_Y,
                      totalSegmentsWidth,
                      TimelineConstants::RULER_BASE_Y);

    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);

    const int margin = 50;
    const int secDiv = TimelineConstants::RULER_SEC_DIV;
    double rowX = 0;
    bool useDisabledColor = false;
    for (int i = 0; rowX < rowXMax; i++) {
        rowX = TimelineConstants::RULER_EDGE_OFFSET + xStep * i;

        // Optimization to skip painting outside the visible area
        if (rowX < (m_viewportX - margin) || rowX > (m_viewportX + minRulerWidth + margin))
            continue;

        const int h = i % secDiv == 0 ? TimelineConstants::RULER_DIV_H1 :
                i % secDiv == secDiv * 0.5 ? TimelineConstants::RULER_DIV_H2 :
                TimelineConstants::RULER_DIV_H3;

        if (!useDisabledColor && rowX > totalSegmentsWidth) {
            painter->setPen(QColor(TimelineConstants::RULER_COLOR_DISABLED));
            useDisabledColor = true;
        }
        painter->drawLine(QPointF(rowX, TimelineConstants::RULER_BASE_Y - h),
                          QPointF(rowX, TimelineConstants::RULER_BASE_Y - 1));

        // See if label should be shown at this tick at this zoom level
        bool drawTimestamp = false;
        if ((i % (secDiv * 4) == 0)
                || (i % (secDiv * 2) == 0 && m_timeScale >= TimelineConstants::RULER_TICK_SCALE1)
                || (i % secDiv == 0 && m_timeScale >= TimelineConstants::RULER_TICK_SCALE2)
                || (i % secDiv == secDiv * 0.5
                    && m_timeScale >= TimelineConstants::RULER_TICK_SCALE3)
                || (m_timeScale >= TimelineConstants::RULER_TICK_SCALE4)) {
            drawTimestamp = true;
        }

        if (drawTimestamp) {
            QRectF timestampPos = QRectF(TimelineConstants::RULER_EDGE_OFFSET
                                         + xStep * i - TimelineConstants::RULER_LABEL_W / 2,
                                         1, TimelineConstants::RULER_LABEL_W,
                                         TimelineConstants::RULER_LABEL_H);
            painter->drawText(timestampPos, Qt::AlignCenter,
                              timestampString(i * 1000 / TimelineConstants::RULER_SEC_DIV));
        }

    }

    painter->restore();
}

void Ruler::setTimelineScale(double scl)
{
    m_timeScale = scl;
    update();
}

// convert distance values to time
double Ruler::distanceToTime(double distance) const
{
    return distance / (TimelineConstants::RULER_SEC_W * m_timeScale);
}

// convert time values to distance
double Ruler::timeToDistance(double time) const
{
    return time * TimelineConstants::RULER_SEC_W * m_timeScale;
}

// x position of ruler value 0
double Ruler::durationStartX() const
{
    return x() + TimelineConstants::RULER_EDGE_OFFSET;
}

// x position of ruler max value (duration)
double Ruler::durationEndX() const
{
    return durationStartX() + timeToDistance(m_duration);
}

double Ruler::timelineScale() const
{
    return m_timeScale;
}

double Ruler::duration() const
{
    return m_duration;
}

double Ruler::maxDuration() const
{
    return m_maxDuration;
}

void Ruler::setDuration(double duration, double maxDuration)
{
    if (m_duration != duration || m_maxDuration != maxDuration) {
        m_duration = duration;
        m_maxDuration = maxDuration;
        update();

        emit durationChanged(duration);
    }
}

void Ruler::setViewportX(int viewportX)
{
    if (m_viewportX != viewportX) {
        m_viewportX = viewportX;
        update();
    }
}

// Returns timestamp in mm:ss.ttt or ss.ttt format
const QString Ruler::timestampString(int timeMs)
{
    int ms = timeMs % 1000;
    int s = timeMs % 60000 / 1000;
    int m = timeMs % 3600000 / 60000;
    QString msString = QString::number(ms).rightJustified(3, '0');
    QString sString = QString::number(s).rightJustified(2, '0');
    if (timeMs == 0)
        return tr("0");
    else if (m == 0)
        return tr("%1.%2").arg(sString).arg(msString);
    else
        return tr("%1:%2.%3").arg(m).arg(sString).arg(msString);
}

int Ruler::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRuler;
}
