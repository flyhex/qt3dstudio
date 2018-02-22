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

Ruler::Ruler(TimelineItem *parent) : TimelineItem(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void Ruler::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    double xStep = TimelineConstants::RULER_SEC_W / TimelineConstants::RULER_SEC_DIV * m_timeScale;
    double totalSegments = m_duration * TimelineConstants::RULER_SEC_DIV;

    painter->save();
    painter->setPen(QColor(TimelineConstants::RULER_COLOR));

    painter->drawLine(QPointF(0, TimelineConstants::RULER_BASE_Y),
                      QPointF(10000, TimelineConstants::RULER_BASE_Y));

    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);

    for (int i = 0; i <= totalSegments; i++) {
        int h = (i % TimelineConstants::RULER_SEC_DIV == 0
              || i % TimelineConstants::RULER_SEC_DIV == TimelineConstants::RULER_SEC_DIV * .5)
                ? TimelineConstants::RULER_DIV_H1 : TimelineConstants::RULER_DIV_H2;

        painter->drawLine(QPointF(TimelineConstants::RULER_EDGE_OFFSET + xStep * i,
                                  TimelineConstants::RULER_BASE_Y - h),
                          QPointF(TimelineConstants::RULER_EDGE_OFFSET + xStep * i,
                                  TimelineConstants::RULER_BASE_Y));

        if (i % TimelineConstants::RULER_SEC_DIV == 0) {
            painter->drawText(QRectF(TimelineConstants::RULER_EDGE_OFFSET
                                     + xStep * i - 10, 2, 20, 10), Qt::AlignCenter,
                              tr("%1s").arg(i / TimelineConstants::RULER_SEC_DIV));
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

// convert distance values to time
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

int Ruler::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRuler;
}
