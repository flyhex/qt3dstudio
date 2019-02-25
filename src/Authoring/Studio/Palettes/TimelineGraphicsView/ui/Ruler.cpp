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
#include "StudioPreferences.h"

#include <QtGui/qpainter.h>
#include <QtWidgets/qwidget.h>

Ruler::Ruler(TimelineItem *parent) : TimelineItem(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void Ruler::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)

    double xStep = TimelineConstants::RULER_SEC_W / TimelineConstants::RULER_SEC_DIV * m_timeScale;
    double activeSegmentsWidth = TimelineConstants::RULER_EDGE_OFFSET
            + m_duration / 1000.0 * xStep * TimelineConstants::RULER_SEC_DIV;
    double totalSegmentsWidth = TimelineConstants::RULER_EDGE_OFFSET
            + m_maxDuration / 1000.0 * xStep * TimelineConstants::RULER_SEC_DIV;

    // Ruler painted width to be at least widget width
    double minRulerWidth = widget->width();
    double rowXMax = std::max(minRulerWidth, totalSegmentsWidth);

    painter->save();
    painter->setPen(CStudioPreferences::timelineRulerColorDisabled());
    painter->drawLine(TimelineConstants::RULER_EDGE_OFFSET,
                      TimelineConstants::RULER_BASE_Y,
                      rowXMax + TimelineConstants::RULER_EDGE_OFFSET,
                      TimelineConstants::RULER_BASE_Y);
    painter->setPen(CStudioPreferences::timelineRulerColor());
    painter->drawLine(TimelineConstants::RULER_EDGE_OFFSET,
                      TimelineConstants::RULER_BASE_Y,
                      activeSegmentsWidth,
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

        const int h = i % secDiv == 0 ? TimelineConstants::RULER_DIV_H1
         : i % secDiv == secDiv * 0.5 ? TimelineConstants::RULER_DIV_H2
                                      : TimelineConstants::RULER_DIV_H3;

        if (!useDisabledColor && rowX > activeSegmentsWidth) {
            painter->setPen(CStudioPreferences::timelineRulerColorDisabled());
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

// convert distance values to time (milliseconds)
long Ruler::distanceToTime(double distance) const
{
    return distance / (TimelineConstants::RULER_MILLI_W * m_timeScale);
}

// convert time (milliseconds) values to distance
double Ruler::timeToDistance(long time) const
{
    return time * TimelineConstants::RULER_MILLI_W * m_timeScale;
}

double Ruler::timelineScale() const
{
    return m_timeScale;
}

// Returns end of right-most layer/component row.
// Active color of ruler is used up to this point.
// Slide plays up to this point.
long Ruler::duration() const
{
    return m_duration;
}

// Returns end of right-most row.
// Ruler steps & labels are drawn up to this point.
// Timeline scrollbar allows scrolling up to this point.
long Ruler::maxDuration() const
{
    return m_maxDuration;
}

void Ruler::setDuration(long duration)
{
    if (m_duration != duration) {
        m_duration = duration;
        update();
        emit durationChanged(m_duration);
    }
}

void Ruler::setMaxDuration(long maxDuration)
{
    if (m_maxDuration != maxDuration) {
        m_maxDuration = maxDuration;
        update();
        emit maxDurationChanged(m_maxDuration);
    }
}

void Ruler::setViewportX(int viewportX)
{
    if (m_viewportX != viewportX) {
        m_viewportX = viewportX;
        emit viewportXChanged(m_viewportX);
        update();
    }
}

int Ruler::viewportX() const
{
    return m_viewportX;
}

// Returns timestamp in mm:ss.ttt or ss.ttt format
const QString Ruler::timestampString(int timeMs)
{
    static const QString zeroString = tr("0");
    static const QChar fillChar = tr("0").at(0);
    static const QString noMinutesTemplate = tr("%1.%2");
    static const QString minutesTemplate = tr("%1:%2.%3");

    int ms = timeMs % 1000;
    int s = timeMs % 60000 / 1000;
    int m = timeMs % 3600000 / 60000;
    const QString msString = QString::number(ms).rightJustified(3, fillChar);
    const QString sString = QString::number(s);

    if (timeMs == 0) {
        return zeroString;
    } else if (m == 0) {
        if (s < 10)
            return noMinutesTemplate.arg(sString).arg(msString);
        else
            return noMinutesTemplate.arg(sString.rightJustified(2, fillChar)).arg(msString);
    } else {
        return minutesTemplate.arg(m).arg(sString.rightJustified(2, fillChar)).arg(msString);
    }
}

int Ruler::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRuler;
}
