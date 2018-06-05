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

#ifndef TREEHEADER_H
#define TREEHEADER_H

#include "TimelineItem.h"
#include "TimelineConstants.h"
#include "RowTypes.h"
#include "TimelineGraphicsScene.h"

class RowTimeline;

class TreeHeader : public TimelineItem
{
    Q_OBJECT

public:
    explicit TreeHeader(TimelineGraphicsScene *timelineScene, TimelineItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                             QWidget *widget = nullptr) override;
    TreeControlType handleButtonsClick(const QPointF &scenePos);
    bool filterShy() const;
    bool filterHidden() const;
    bool filterLocked() const;
    int type() const;

    void toggleFilterShy();
    void toggleFilterHidden();
    void toggleFilterLocked();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    TimelineGraphicsScene *m_scene;
    bool m_shy = false;
    bool m_visible = false;
    bool m_lock = false;
    int m_hoveredItem = -1;
    QRect m_rectShy;
    QRect m_rectVisible;
    QRect m_rectLock;
};

#endif // TREEHEADER_H
