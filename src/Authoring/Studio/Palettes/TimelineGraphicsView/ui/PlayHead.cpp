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

#include "PlayHead.h"
#include "Ruler.h"
#include "TimelineConstants.h"

#include <QtGui/qpainter.h>
#include <QtGui/qcursor.h>

PlayHead::PlayHead(Ruler *ruler)
    : QGraphicsRectItem()
    , m_ruler(ruler)
{
    setZValue(99);
    setRect(-TimelineConstants::PLAYHEAD_W * .5, 0, TimelineConstants::PLAYHEAD_W, 0);
}

void PlayHead::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    static const QPixmap pixHead = QPixmap(":/images/PlaybackHead.png");

    painter->drawPixmap(-TimelineConstants::PLAYHEAD_W * .5, 0, pixHead);
    painter->setPen(QColor(TimelineConstants::PLAYHEAD_LINE_COLOR));
    painter->drawLine(0, 0, 0, 3000);
}

void PlayHead::setHeight(int height)
{
    setRect(
        rect().x(),
        rect().y(),
        rect().width(),
        height);
}

void PlayHead::setTime(double time)
{
    if (time < 0)
        time = 0;
    else if (time > m_ruler->duration())
        time = m_ruler->duration();

    m_time = time;
    updatePosition();
}

void PlayHead::setPosition(double posX)
{
    posX = qBound(TimelineConstants::RULER_EDGE_OFFSET, posX, m_ruler->duration()
                  * TimelineConstants::RULER_SEC_W * m_ruler->timelineScale()
                  + TimelineConstants::RULER_EDGE_OFFSET);

    setX(m_ruler->x() + posX);
    m_time = (posX - TimelineConstants::RULER_EDGE_OFFSET)
            / (TimelineConstants::RULER_SEC_W * m_ruler->timelineScale());
}

void PlayHead::updatePosition()
{
    setX(m_ruler->x() + TimelineConstants::RULER_EDGE_OFFSET
         + m_time * TimelineConstants::RULER_SEC_W * m_ruler->timelineScale());
}

double PlayHead::time() const
{
    return m_time;
}

int PlayHead::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TimelineItem::TypePlayHead;
}
