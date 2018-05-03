/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "TimeMeasureItem.h"
#include "CoreUtils.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"

#include <QPainter>

TimeMeasureItem::TimeMeasureItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

TimeMeasureItem::~TimeMeasureItem()
{
}

void TimeMeasureItem::paint(QPainter *painter)
{
    const double edgeMargin = 2;
    const double largeHashOffset = 5;
    const double mediumHashOffset = 6;
    const double smallHashOffset = 3;

    QPen pen(CStudioPreferences::GetRulerTickColor());
    painter->setPen(pen);

    long theLength = width();
    long theHeight = height() - edgeMargin;

    double theTotalMeasure = theLength / m_Ratio;

    long theNumLargeHashes = (long)(theTotalMeasure / m_LargeHashInterval) + 1;

    long theOffset = m_Offset - (m_Offset % ::dtol(m_LargeHashInterval));

    for (long i = 0; i < theNumLargeHashes + 1; ++i) {
        double theMeasure = m_LargeHashInterval * i + theOffset;

        long thePos = ::TimeToPos(theMeasure - m_Offset, m_Ratio);

        if (thePos > 0)
            painter->drawLine(thePos, theHeight, thePos, theHeight - largeHashOffset);

        DrawMeasureText(painter, thePos, long(theMeasure));

        // sanity check
        if (m_MediumHashInterval > 0) {
            thePos = ::TimeToPos(theMeasure - m_Offset + m_MediumHashInterval, m_Ratio);
            if (thePos > 0)
                painter->drawLine(thePos, theHeight, thePos, theHeight - mediumHashOffset);

            for (double theSmallInterval = 0; theSmallInterval < m_LargeHashInterval;
                 theSmallInterval += m_SmallHashInterval) {
                thePos = ::TimeToPos(theMeasure - m_Offset + theSmallInterval, m_Ratio);

                if (thePos > 0)
                    painter->drawLine(thePos, theHeight, thePos, theHeight - smallHashOffset);
            }
        } // if medium is valid
    }

    // Draw the top outline
    painter->drawLine(0, theHeight, theLength, theHeight);
}

void TimeMeasureItem::setTimeRatio(double inTimeRatio)
{
    if (qFuzzyCompare(m_Ratio, inTimeRatio))
        return;

    m_Ratio = inTimeRatio;

    double theTimePerPixel = (double)(1 / inTimeRatio);

    // Only go through this if it has actually changed
    if (theTimePerPixel != m_TimePerPixel) {
        m_TimePerPixel = theTimePerPixel;

        // Go through the possible hash settings and find the one that best suits the
        // time per pixel.
        double theMillisPerLargeHash = theTimePerPixel * 50;
        if (theMillisPerLargeHash <= 100) // 100ms
            theMillisPerLargeHash = 100;
        else if (theMillisPerLargeHash <= 200) // 200ms
            theMillisPerLargeHash = 200;
        else if (theMillisPerLargeHash <= 500) // .5s
            theMillisPerLargeHash = 500;
        else if (theMillisPerLargeHash <= 1000) // 1s
            theMillisPerLargeHash = 1000;
        else if (theMillisPerLargeHash <= 2000) // 2s
            theMillisPerLargeHash = 2000;
        else if (theMillisPerLargeHash <= 5000) // 5s
            theMillisPerLargeHash = 5000;
        else if (theMillisPerLargeHash <= 10000) // 10s
            theMillisPerLargeHash = 10000;
        else if (theMillisPerLargeHash <= 20000) // 20s
            theMillisPerLargeHash = 20000;
        else if (theMillisPerLargeHash <= 30000) // 30s
            theMillisPerLargeHash = 30000;
        else if (theMillisPerLargeHash <= 60000) // 1m
            theMillisPerLargeHash = 60000;
        else if (theMillisPerLargeHash <= 120000) // 2m
            theMillisPerLargeHash = 120000;
        else if (theMillisPerLargeHash <= 300000) // 5m
            theMillisPerLargeHash = 300000;
        else if (theMillisPerLargeHash <= 600000) // 10m
            theMillisPerLargeHash = 600000;
        else if (theMillisPerLargeHash <= 1200000) // 20m
            theMillisPerLargeHash = 1200000;
        else if (theMillisPerLargeHash <= 1800000) // 30m
            theMillisPerLargeHash = 1800000;
        else if (theMillisPerLargeHash <= 3600000) // 1h
            theMillisPerLargeHash = 3600000;
        else
            theMillisPerLargeHash = 7200000; // 2h

        // Set the distances between the hashes
        m_LargeHashInterval = theMillisPerLargeHash;
        m_MediumHashInterval = theMillisPerLargeHash / 2;
        m_SmallHashInterval = theMillisPerLargeHash / 10;

        update();
    }

    emit timeRatioChanged(inTimeRatio);
}

double TimeMeasureItem::timeRatio() const
{
    return m_Ratio;
}

void TimeMeasureItem::DrawMeasureText(QPainter *painter, long inPosition, long inMeasure) const
{
    QString theTimeFormat(FormatTime(inMeasure));

    // Offset the position by half the text size to center it over the hash.
    QFontMetrics fm = painter->fontMetrics();
    const auto textSize = fm.size(Qt::TextSingleLine, theTimeFormat);
    inPosition -= ::dtol(textSize.width() / 2);

    QRectF rect(0, 0, width(), height());
    rect.translate(inPosition, -3);

    painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, theTimeFormat);
}

QString TimeMeasureItem::FormatTime(long inTime) const
{
    long theHours = inTime / 3600000;
    long theMinutes = inTime % 3600000 / 60000;
    long theSeconds = inTime % 60000 / 1000;
    long theMillis = inTime % 1000;

    bool theHoursOnlyFlag = theHours != 0 && theMinutes == 0 && theSeconds == 0 && theMillis == 0;
    bool theMinutesOnlyFlag =
        !theHoursOnlyFlag && theMinutes != 0 && theSeconds == 0 && theMillis == 0;
    bool theSecondsOnlyFlag = !theMinutesOnlyFlag && theMillis == 0;

    QString theTime;
    // If only hours are being displayed then format it as hours.
    if (theHoursOnlyFlag) {
        theTime = tr("%1h").arg(theHours);
    }
    // If only minutes are being displayed then format it as minutes.
    else if (theMinutesOnlyFlag) {
        theTime = tr("%1m").arg(theMinutes);
    }
    // If only seconds are being displayed then format as seconds
    else if (theSecondsOnlyFlag) {
        theTime = tr("%1s").arg(theSeconds);
    }
    // If the intervals are correct then this should only be tenths of seconds, so do that.
    else {
        theTime = tr("0.%1s").arg(theMillis / 100);
    }

    return theTime;
}
