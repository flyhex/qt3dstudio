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

#ifndef TIMEMEASUREITEM_H
#define TIMEMEASUREITEM_H

#include <QQuickPaintedItem>

class TimeMeasureItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(double timeRatio READ timeRatio WRITE setTimeRatio NOTIFY timeRatioChanged)

public:
    explicit TimeMeasureItem(QQuickItem *parent = nullptr);
    ~TimeMeasureItem() override;

    void paint(QPainter *painter) override;

    double timeRatio() const;
    void setTimeRatio(double timeRatio);

Q_SIGNALS:
    void timeRatioChanged(double timeRatio);

private:
    void DrawMeasureText(QPainter *painter, long inPosition, long inMeasure) const;
    QString FormatTime(long inTime) const;

    double m_SmallHashInterval = 0.0;
    double m_MediumHashInterval = 0.0;
    double m_LargeHashInterval = 0.0;
    double m_Ratio = 0.0;
    long m_Offset = 0;
    double m_TimePerPixel = 0.0;
};

#endif // TIMEMEASUREITEM_H
