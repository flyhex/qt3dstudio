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
#include <QtCore/qpointer.h>
#include "RowTimelineCommentItem.h"

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
    void updateBoundChildren(bool start);
    void clearBoundChildren();
    void moveDurationBy(double dx);
    void moveDurationTo(double newX);
    void setStartTime(long startTime);
    void setEndTime(long endTime);
    void setStartX(double startX);
    void setEndX(double endX);
    void setBarColor(const QColor &color);
    void setControllerText(const QString &controller);
    void putSelectedKeyframesOnTop();
    void updateKeyframes();
    void insertKeyframe(Keyframe *keyframe);
    void removeKeyframe(Keyframe *keyframe);
    void updateKeyframesFromBinding(const QList<int> &properties);
    void updateDurationFromBinding();
    TimelineControlType getClickedControl(const QPointF &scenePos, bool isHover = false) const;
    double getStartX() const;
    double getEndX() const;
    long getStartTime() const;
    long getEndTime() const;
    long getDurationMoveTime() const; // the time a row duration has moved (to commit to binding)
    double getDurationMoveOffsetX() const;
    long getDuration() const;
    QColor barColor() const;
    int type() const override;
    RowTimeline *parentRow() const;
    RowTree *rowTree() const;
    Keyframe *getClickedKeyframe(const QPointF &scenePos);
    QList<Keyframe *> getKeyframesInRange(const QRectF &rect) const;
    QList<Keyframe *> keyframes() const;
    void showToolTip(const QPointF &pos);
    void toggleColorGradient();
    bool isColorProperty() const { return m_isColorProperty; }
    RowTimelinePropertyGraph *propertyGraph() const { return m_propertyGraph; }

protected:
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void initialize();
    void updateChildrenStartRecursive();
    void updateChildrenEndRecursive();
    void updateChildrenMinStartXRecursive(RowTree *rowTree);
    void updateChildrenMaxEndXRecursive(RowTree *rowTree);
    void updateCommentItem();
    void updateCommentItemPos();
    void drawColorPropertyGradient(QPainter *painter, const QRect &rect);
    QString formatTime(long millis) const;
    void collectChildKeyframeTimes(QVector<long> &childKeyframeTimes);

    RowTree *m_rowTree;
    RowTimelinePropertyGraph *m_propertyGraph = nullptr;
    RowTimelineCommentItem *m_commentItem = nullptr;
    long m_startTime = 0;
    long m_startDurationMoveStartTime = 0;
    double m_startDurationMoveOffsetX = 0;
    long m_endTime = 0;
    double m_startX = 0;
    double m_endX = 0;
    double m_minStartX = 0;
    double m_maxEndX = 0;
    bool m_isProperty = false; // used in the destructor
    bool m_isColorProperty = false;
    bool m_drawColorGradient = true;
    QString m_controllerDataInput;
    QList<Keyframe *> m_keyframes;
    QColor m_barColor;
    QVector<QPointer<RowTimeline>> m_boundChildrenStart;
    QVector<QPointer<RowTimeline>> m_boundChildrenEnd;

    friend class RowTree;
};

#endif // ROWTIMELINE_H
