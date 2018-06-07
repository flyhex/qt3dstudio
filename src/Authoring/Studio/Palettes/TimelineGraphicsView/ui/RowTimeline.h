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
#include "RowTypes.h"
#include "Bindings/Qt3DSDMTimelineItemProperty.h"

class RowTree;
class RowTimelinePropertyGraph;
struct Keyframe;

class RowTimeline : public InteractiveTimelineItem
{
    Q_OBJECT

public:
    explicit RowTimeline();
    ~RowTimeline();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void setState(State state) override;
    void setRowTree(RowTree *rowTree);
    void updatePosition();
    void startDurationMove(double clickX);
    void moveDurationBy(double dx);
    void moveDurationTo(double newX);
    void setStartTime(double startTime);
    void setEndTime(double endTime);
    void setStartX(double startX);
    void setEndX(double endX);
    void setBarColor(const QColor &color);
    void putSelectedKeyframesOnTop();
    void updateKeyframes();
    void insertKeyframe(Keyframe *keyframe);
    void removeKeyframe(Keyframe *keyframe);
    void updateKeyframesFromBinding(qt3dsdm::Qt3DSDMPropertyHandle propHandle);
    void updateDurationFromBinding();
    TimelineControlType getClickedControl(const QPointF &scenePos) const;
    double getStartX() const;
    double getEndX() const;
    double getStartTime() const;
    double getEndTime() const;
    double getDurationMoveTime() const; // the time a row duration has moved (to commit to binding)
    double getDurationMoveOffsetX() const;
    QColor barColor() const;
    int type() const;
    RowTimeline *parentRow() const;
    RowTree *rowTree() const;
    Keyframe *getClickedKeyframe(const QPointF &scenePos);
    QList<Keyframe *> getKeyframesInRange(const QRectF &rect) const;
    QList<Keyframe *> keyframes() const;
    void showToolTip(const QPointF &pos);

protected:
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void updateChildrenStartRecursive(RowTree *rowTree, double oldStartX);
    void updateChildrenEndRecursive(RowTree *rowTree, double oldEndX);
    void updateChildrenMinStartXRecursive(RowTree *rowTree);
    void updateChildrenMaxEndXRecursive(RowTree *rowTree);
    void drawColorPropertyGradient(QPainter *painter, int width);
    bool isColorProperty() const;
    double timeToX(double time) const;
    double xToTime(double xPos) const;

    RowTree *m_rowTree;
    RowTimelinePropertyGraph *m_propertyGraph = nullptr;
    double m_startTime = 0;
    double m_startDurationMoveStartTime = 0;
    double m_startDurationMoveOffsetX = 0;
    double m_endTime = 0;
    double m_startX = 0;
    double m_endX = 0;
    double m_minStartX = 0;
    double m_maxEndX = 0;
    bool m_isProperty = false; // used in the destructor
    QList<Keyframe *> m_keyframes;
    QColor m_barColor;

    friend class RowTree;
};

#endif // ROWTIMELINE_H
