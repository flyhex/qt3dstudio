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

#include "TimePropertyItem.h"
#include "CoreUtils.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"
#include "TimelineRow.h"
#include "PropertyRow.h"
#include "Bindings/ITimelineItemProperty.h"

#include <QPainter>

TimePropertyItem::TimePropertyItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

TimePropertyItem::~TimePropertyItem()
{
}

void TimePropertyItem::paint(QPainter *painter)
{
    if (!m_property)
        return;

    static const QVector<QColor> channelColors{
        QColor(255, 0, 0),
        QColor(0, 255, 0),
        QColor(0, 0, 255),
        QColor(255, 255, 0),
        QColor(255, 0, 255),
        QColor(0, 255, 255)
    };

    m_MaxVal = m_property->GetMaximumValue();
    m_MinVal = m_property->GetMinimumValue();

    m_MinY = 10;
    m_MaxY = height() - m_MinY;

    painter->save();
    painter->fillRect(0, 0, width(), height(), CStudioPreferences::backgroundColor());
    long channelCount = m_property->GetChannelCount();
    // Don't want to overflow the color array
    if (channelCount <= 6) {
        // For each channel graph it.
        for (long i = 0; i < channelCount; ++i)
            drawDetailedChannel(painter, i, channelColors[i]);
    }
    painter->restore();
}

double TimePropertyItem::timeRatio() const
{
    return m_timeRatio;
}

void TimePropertyItem::setTimeRatio(double timeRatio)
{
    if (qFuzzyCompare(m_timeRatio, timeRatio))
        return;

    m_timeRatio = timeRatio;

    update();
    Q_EMIT timeRatioChanged(m_timeRatio);
}

CTimelineRow *TimePropertyItem::timelineRow() const
{
    return m_timelineRow;
}

void TimePropertyItem::setTimelineRow(CTimelineRow *timelineRow)
{
    if (timelineRow != m_timelineRow) {
        m_timelineRow = timelineRow;
        auto propertyRow = dynamic_cast<CPropertyRow*>(timelineRow);
        if (propertyRow) {
            m_property = propertyRow->GetProperty();
        } else {
            m_timelineRow = nullptr;
            m_property = nullptr;
        }
        Q_EMIT timelineRowChanged();
    }
    // intentionally request and update, as this might indicate the parent changes, eg.
    // keyframes were moved around
    update();
}

void TimePropertyItem::drawDetailedChannel(QPainter *painter, long channelIndex, const QColor &color)
{
    painter->setPen(color);


    float theValue = m_property->GetChannelValueAtTime(channelIndex, 0);
    long theYPos = (long)((1.0 - (theValue - m_MinVal) / (m_MaxVal - m_MinVal)) * (m_MaxY - m_MinY)
                          + m_MinY + .5);

    auto pos = QPoint(0, theYPos);

    long theInterval = 5;
    long theSize = width() + theInterval;

    for (long thePixel = 0; thePixel < theSize; thePixel += theInterval) {
        long theTime = ::PosToTime(thePixel, m_timeRatio); //(long)( thePixel / m_TimeRatio + .5 );
        theValue = m_property->GetChannelValueAtTime(channelIndex, theTime);
        theYPos = (long)((1.0 - (theValue - m_MinVal) / (m_MaxVal - m_MinVal)) * (m_MaxY - m_MinY)
                         + m_MinY + .5);

        auto newPos = QPoint(thePixel, theYPos);
        painter->drawLine(pos, newPos);
        pos = newPos;
    }
}

