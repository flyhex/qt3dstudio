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

#ifndef ROWTIMELINEPROPERTYGRAPH_H
#define ROWTIMELINEPROPERTYGRAPH_H

#include "Qt3DSDMAnimation.h"
#include "RowTypes.h"
#include "TimelineConstants.h"
#include "Bindings/Qt3DSDMTimelineKeyframe.h"

#include <QtCore/qobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qset.h>

QT_FORWARD_DECLARE_CLASS(QPainter);

class RowTimeline;
class ITimelineItemProperty;
namespace qt3dsdm {
class IAnimationCore;
class Qt3DSDMAnimationHandle;
}

class RowTimelinePropertyGraph : public QObject
{
    Q_OBJECT

public:
    explicit RowTimelinePropertyGraph(QObject *parent = nullptr);
    TimelineControlType getClickedBezierControl(const QPointF &pos, bool isHover = false);
    void paintGraphs(QPainter *painter, const QRectF &rect);
    void updateBezierControlValue(TimelineControlType controlType, const QPointF &scenePos);
    void adjustScale(bool isIncrement);
    void startPan();
    void pan(qreal dy);
    void fitGraph();
    void commitBezierEdit();
    void setExpandHeight(int h);
    void selectBezierKeyframesInRange(const QRectF &rect);
    void deselectAllBezierKeyframes();
    void updateChannelFiltering(const QVector<bool> &activeChannels);

private:
    enum class BezierControlType {None, In, Out};

    QPointF getBezierControlPosition(const qt3dsdm::SBezierKeyframe &kf,
                                     BezierControlType type = BezierControlType::None) const;
    QPointF getKeyframePosition(float time, float value) const;
    void checkValScaleLimits();

    std::pair<qt3dsdm::Qt3DSDMKeyframeHandle, qt3dsdm::TKeyframe> m_currKeyframeData;
    RowTimeline *m_rowTimeline = nullptr;
    ITimelineItemProperty *m_propBinding = nullptr;
    qt3dsdm::IAnimationCore *m_animCore = nullptr;
    float m_valScale = .5f;
    qreal m_graphY = 0;
    qreal m_graphYPanInit = 0; // value of graph_y when panning starts
    qreal m_graphH = 0;
    int m_expandHeight = TimelineConstants::ROW_GRAPH_H; // height when expanded
    qt3dsdm::Qt3DSDMKeyframeHandle m_hoveredBezierKeyframe;
    QSet<qt3dsdm::Qt3DSDMKeyframeHandle> m_selectedBezierKeyframes;
    QVector<qt3dsdm::Qt3DSDMAnimationHandle> m_activeChannels; // active channels anim. handles
    QVector<int> m_activeChannelsIndex;
    QTimer m_fitCurveTimer;
};

#endif // ROWTIMELINEPROPERTYGRAPH_H
