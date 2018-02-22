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

#ifndef ROWTREE_H
#define ROWTREE_H

#include "InteractiveTimelineItem.h"
#include "RowTypes.h"

class RowTimeline;
class Ruler;

class RowTree : public InteractiveTimelineItem
{
    Q_OBJECT

public:
    explicit RowTree(Ruler *ruler, RowType rowType = RowType::Layer, const QString &label = {});
    explicit RowTree(Ruler *ruler, PropertyType propType); // property row constructor

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void setState(State state) override;
    void setTimelineRow(RowTimeline *rowTimeline);
    void setParent(RowTree *parent);
    void addChild(RowTree *child);
    void moveChild(int from, int to);   // NOT USED
    void removeChild(RowTree *child);
    void setMoveSourceRecursive(bool value);
    void setMoveTarget(bool value);

    bool handleButtonsClick(QGraphicsSceneMouseEvent *event);
    bool hasPropertyChildren();
    bool shy() const;
    bool visible() const;
    bool locked() const;
    bool expanded() const;
    bool isDecendentOf(RowTree *row) const;
    bool isContainer() const;
    bool empty() const;

    int depth() const;
    int type() const;
    RowType rowType() const;
    PropertyType propertyType() const;

    RowTree *parentRow() const;
    QList<RowTree *> childRows() const;
    RowTimeline *rowTimeline() const;
    QString label() const;

private:
    void updateExpandStatus(bool expand, bool childrenOnly = false);
    void updateDepthRecursive();
    void updatePropertyLabel();

    RowTree *m_parentRow = nullptr;
    RowTimeline *m_rowTimeline = nullptr;
    int m_depth = 1;
    bool m_shy = false;
    bool m_visible = true;
    bool m_locked = false;
    bool m_expanded = true;
    bool m_moveSource = false;
    bool m_moveTarget = false;
    RowType m_rowType = RowType::Layer;
    PropertyType m_propertyType = PropertyType::None; // for property rows
    QString m_label = 0;
    QList<RowTree *> m_childRows;

    QRect m_rectArrow;
    QRect m_rectShy;
    QRect m_rectVisible;
    QRect m_rectLocked;

    friend class RowTimeline;
};

#endif // ROWTREE_H
