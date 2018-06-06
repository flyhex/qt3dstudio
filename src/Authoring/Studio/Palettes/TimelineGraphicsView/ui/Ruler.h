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

#ifndef RULER_H
#define RULER_H

#include "TimelineItem.h"

class Ruler : public TimelineItem
{
    Q_OBJECT

signals:
    void rulerClicked(const double &pos);

public:
    explicit Ruler(TimelineItem *parent = nullptr);

    void setTimelineScale(double scl);
    double distanceToTime(double distance) const;
    double timeToDistance(double time) const;
    double durationStartX() const;
    double durationEndX() const;
    double timelineScale() const;
    double duration() const;
    double maxDuration() const;
    void setDuration(double duration);
    void setMaxDuration(double maxDuration);
    void setViewportX(int viewportX);
    int type() const;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

signals:
    void maxDurationChanged(double maxDuration);
    void durationChanged(double duration);

private:
    const QString timestampString(int timeMs);
    double m_timeScale = 2;
    double m_duration = 0; // in seconds
    double m_maxDuration = 0; // in seconds
    int m_viewportX = 0;
};

#endif // RULER_H
