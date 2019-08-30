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

using namespace TimelineConstants;

Ruler::Ruler(TimelineItem *parent) : TimelineItem(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void Ruler::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)

    painter->translate(RULER_EDGE_OFFSET, RULER_BASE_Y);

    // draw inactive base line
    int startX = qMax(m_viewportX - int(RULER_EDGE_OFFSET), 0);
    int endX = m_viewportX + widget->width();
    painter->setPen(CStudioPreferences::timelineRulerColorDisabled());
    painter->drawLine(startX, 0, endX, 0);

    // draw active base line
    int durEndX = qMin(endX, int(timeToDistance(m_duration)));
    if (durEndX > startX) {
        painter->setPen(CStudioPreferences::timelineRulerColor());
        painter->drawLine(startX, 0, durEndX, 0);
    }

    // draw ruler steps and text
    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);

    bool useDisabledColor = false;
    int startSecond = int(distanceToTime(startX) / 1000);
    int endSecond   = int(distanceToTime(endX)   / 1000) + 1;

    static const int LABEL_W = 40;
    static const int LABEL_H = 10;
    static const int MIN_SEC_STEP_DIVISION       = 20;
    static const int MIN_SEC_STEP_LABEL          = 60;
    static const int MIN_HALF_SEC_STEP_DIVISION  = 10;
    static const int MIN_HALF_SEC_STEP_LABEL     = 70;
    static const int MIN_ONE_TENTH_STEP_DIVISION = 4;
    static const int MIN_ONE_TENTH_STEP_LABEL    = 30;

    QRectF labelRect(0, 0, LABEL_W, LABEL_H);
    double secStep      = timeToDistance(1000);
    double halfSecStep  = timeToDistance(500);
    double oneTenthStep = timeToDistance(100);

    int skipperSecLabel = qMax(int(MIN_SEC_STEP_LABEL / secStep), 1);
    if (skipperSecLabel > 5)
        skipperSecLabel = lroundf(skipperSecLabel / 5.f) * 5;

    int skipperSecDivision = qMax(int(MIN_SEC_STEP_DIVISION / secStep), 1);

    // keep skipperSecLabel and skipperSecDivision in sync
    if (skipperSecLabel > 5 && skipperSecDivision % 5)
        skipperSecDivision = lroundf(skipperSecLabel / 10.f) * 5;

    bool drawHalfSecDiv    = halfSecStep  > MIN_HALF_SEC_STEP_DIVISION;
    bool drawHalfSecLabel  = halfSecStep  > MIN_HALF_SEC_STEP_LABEL;
    bool drawOneTenthDiv   = oneTenthStep > MIN_ONE_TENTH_STEP_DIVISION;
    bool drawOneTenthLabel = oneTenthStep > MIN_ONE_TENTH_STEP_LABEL;

    for (int i = startSecond; i <= endSecond; ++i) { // draw seconds
        if (i % skipperSecDivision)
            continue;

        int x = int(timeToDistance(i * 1000));
        if (!useDisabledColor && x > durEndX) {
            painter->setPen(CStudioPreferences::timelineRulerColorDisabled());
            useDisabledColor = true;
        }

        // draw second division
        painter->drawLine(x, -RULER_DIV_H1, x, 0);

        if (i % skipperSecLabel == 0) { // draw label if not skipped
            labelRect.moveTo(x - LABEL_W / 2, -LABEL_H - 7);
            painter->drawText(labelRect, Qt::AlignCenter, formatTime(i));
        }

        // draw half seconds
        if (!drawHalfSecDiv || i == endSecond)
            continue;

        x = int(timeToDistance(i * 1000 + 500));
        if (!useDisabledColor && x > durEndX) {
            painter->setPen(CStudioPreferences::timelineRulerColorDisabled());
            useDisabledColor = true;
        }
        painter->drawLine(x, -RULER_DIV_H2, x, 0);

        if (drawHalfSecLabel) {
            labelRect.moveTo(x - LABEL_W / 2, -LABEL_H - 7);
            painter->drawText(labelRect, Qt::AlignCenter, formatTime(i + .5));
        }

        // draw one tenth
        if (!drawOneTenthDiv)
            continue;

        for (int j = 1; j <= 9; ++j) {
            if (j != 5) {
                x = int(timeToDistance(i * 1000 + j * 100));
                if (!useDisabledColor && x > durEndX) {
                    painter->setPen(CStudioPreferences::timelineRulerColorDisabled());
                    useDisabledColor = true;
                }
                painter->drawLine(x, -RULER_DIV_H3, x, 0);

                if (drawOneTenthLabel) {
                    labelRect.moveTo(x - LABEL_W / 2, -LABEL_H - 7);
                    painter->drawText(labelRect, Qt::AlignCenter, formatTime(i + .1 * j));
                }
            }
        }
    }
}

void Ruler::setTimelineScale(int scl)
{
    // scl is in the range 0..100, project it to an exponential value
    bool isScaleDown = scl < 50;
    const double normVal = abs(scl - 50) / 10.; // normalize to range 5..0..5 from scale down to up
    double scaleVal = pow(normVal, 1.3);

    // For scaling down normalize to range -1..0. 5^1.3 is max scaleVal but 5.1 is used to make
    // sure scaleVal doesn't reach -1 in which case m_timeScale would become 0.
    if (isScaleDown)
        scaleVal /= -pow(5.1, 1.3);

    m_timeScale = 1 + scaleVal;
    update();
}

// convert distance values to time (milliseconds)
long Ruler::distanceToTime(double distance) const
{
    return long(distance / (RULER_MILLI_W * m_timeScale));
}

// convert time (milliseconds) values to distance
double Ruler::timeToDistance(long time) const
{
    return time * RULER_MILLI_W * m_timeScale;
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

// x position at the left edge of the timeline (0 when no scrolling)
int Ruler::viewportX() const
{
    return m_viewportX;
}

// Returns ruler time in m:s.t or s.t format
const QString Ruler::formatTime(double seconds)
{
    if (seconds < 60)
        return QString::number(seconds);

    int mins = int(seconds) / 60;
    double secs = seconds - mins * 60;

    const QChar fillChar = tr("0").at(0);
    return QStringLiteral("%1:%2").arg(mins).arg(QString::number(secs).rightJustified(2, fillChar));
}

int Ruler::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRuler;
}
