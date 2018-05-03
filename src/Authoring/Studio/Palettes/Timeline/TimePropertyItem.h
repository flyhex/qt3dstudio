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

#ifndef TIMEPROPERTYITEM_H
#define TIMEPROPERTYITEM_H

#include <QQuickPaintedItem>

class CTimelineRow;
class ITimelineItemProperty;

class TimePropertyItem : public QQuickPaintedItem
{
    Q_PROPERTY(double timeRatio READ timeRatio WRITE setTimeRatio NOTIFY timeRatioChanged)
    Q_PROPERTY(CTimelineRow *timelineRow READ timelineRow WRITE setTimelineRow
               NOTIFY timelineRowChanged)

    Q_OBJECT
public:
    explicit TimePropertyItem(QQuickItem *parent = nullptr);
    ~TimePropertyItem() override;

    void paint(QPainter *painter) override;

    double timeRatio() const;
    void setTimeRatio(double timeRatio);

    CTimelineRow *timelineRow() const;
    void setTimelineRow(CTimelineRow *property);

Q_SIGNALS:
    void timeRatioChanged(double timeRatio);
    void timelineRowChanged();

private:
    void drawDetailedChannel(QPainter *painter, long channelIndex, const QColor &color);

    double m_timeRatio = 0.0;
    float m_MaxVal = 0.0f;
    float m_MinVal = 0.0f;
    long m_MinY = 0;
    long m_MaxY = 0;
    CTimelineRow *m_timelineRow = nullptr;
    ITimelineItemProperty *m_property = nullptr;
};

#endif // TIMEPROPERTYITEM_H
