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

#ifndef ROWTIMELINE_H
#define ROWTIMELINE_H

#include "InteractiveTimelineItem.h"

class RowTree;
class Ruler;
struct Keyframe;

class RowTimeline : public InteractiveTimelineItem
{
    Q_OBJECT

public:
    enum ControlType {
        TypeNone,
        TypeKeyFrame,
        TypeDuration,
        TypeStartHandle,
        TypeEndHandle
    };

    explicit RowTimeline(Ruler *ruler);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void setState(State state) override;
    void setRowTree(RowTree *rowTree);
    void updatePosition();
    void moveDurationBy(double dx);
    void setStartTime(double startTime);
    void setEndTime(double endTime);
    void setStartX(double startX);
    void setEndX(double endX);
    void putSelectedKeyframesOnTop();
    void updateKeyframes();
    void insertKeyframe(Keyframe *keyframe);
    void removeKeyframe(Keyframe *keyframe);
    int getClickedControl(const QPointF &scenePos);
    double getStartTime() const;
    double getEndTime() const;
    int type() const;
    RowTimeline *parentRow() const;
    RowTree *rowTree() const;
    Keyframe *getClickedKeyframe(const QPointF &scenePos);
    QList<Keyframe *> getKeyframesInRange(const double left, const double right);
    QList<Keyframe *> keyframes() const;

private:
    void updateChildrenMinStartXRecursive(RowTree *rowTree);
    void updateChildrenMaxEndXRecursive(RowTree *rowTree);
    double timeToX(double time);
    double xToTime(double xPos);

    RowTree *m_rowTree;
    Ruler *m_ruler;
    double m_startTime;
    double m_endTime;
    double m_startX;
    double m_endX;
    double m_minStartX;
    double m_maxEndX;
    QList<Keyframe *> m_keyframes;

    friend class RowTree;
};

#endif // ROWTIMELINE_H
