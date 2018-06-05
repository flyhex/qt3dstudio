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
#include "TimelineConstants.h"
#include "RowTypes.h"
#include "StudioObjectTypes.h"
#include "RowTreeLabelItem.h"

#include <QtCore/qpropertyanimation.h>
#include <QtCore/qparallelanimationgroup.h>

class RowTimeline;
class Ruler;
class ITimelineItemBinding;
class TimelineGraphicsScene;
class ITimelineItemProperty;

class RowTree : public InteractiveTimelineItem
{
    Q_OBJECT

public:
    enum class ExpandState {
        Unknown,
        Collapsed,
        Expanded,
        HiddenCollapsed,
        HiddenExpanded
    };

    enum class DnDState {
        None,
        Source, // the row being dragged while DnD-ing
        Parent, // parent of the insertion point
        Any     // accept any state (default value in setDnDState() method)
    };

    explicit RowTree(TimelineGraphicsScene *timelineScene,
                     EStudioObjectType rowType = OBJTYPE_UNKNOWN, const QString &label = {});
    // property row constructor
    explicit RowTree(TimelineGraphicsScene *timelineScene, const QString &propType);
    ~RowTree();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void setState(State state) override;
    void setTimelineRow(RowTimeline *rowTimeline);
    void setParent(RowTree *parent);
    void addChild(RowTree *child);
    void addChildAt(RowTree *child, int index);
    void removeChild(RowTree *child);
    void setDnDState(DnDState state, DnDState onlyIfState = DnDState::Any, bool recursive = false);
    void setTreeWidth(double w);
    void setBinding(ITimelineItemBinding *binding);
    void setPropBinding(ITimelineItemProperty *binding); // for property rows
    void selectLabel();
    void togglePropertyExpanded();
    void setPropertyExpanded(bool expand);
    ITimelineItemProperty *propBinding();
    TreeControlType getClickedControl(const QPointF &scenePos);
    bool shy() const;
    bool visible() const;
    bool locked() const;
    bool expanded() const;
    bool expandHidden() const;
    bool isDecendentOf(RowTree *row) const;
    bool isContainer() const;
    bool isProperty() const;
    bool isPropertyOrMaterial() const;
    bool isComponent() const;
    bool isMaster() const;
    bool hasPropertyChildren() const;
    bool empty() const; // has zero child rows (and zero properties)
    bool selected() const;
    bool draggable() const;
    bool hasDurationBar() const;
    bool propertyExpanded() const;
    int depth() const;
    int type() const;
    int index() const;
    int indexInLayout() const;
    int treeWidth() const;
    EStudioObjectType rowType() const;
    QString propertyType() const;
    RowTree *getChildAt(int index) const;
    RowTree *parentRow() const;
    RowTree *getPropertyRow(const QString &type) const;
    QList<RowTree *> childRows() const;
    QList<RowTree *> childProps() const;
    RowTimeline *rowTimeline() const;
    QString label() const;
    void toggleShy();
    void toggleVisible();
    void toggleLocked();
    void updateFromBinding();
    void updateLabel();
    void setRowVisible(bool visible);
    void setDnDHover(bool val);
    DnDState getDnDState() const;

    ITimelineItemBinding *getBinding() const;
    void updateExpandStatus(ExpandState state, bool animate = true);
    void updateArrowVisibility();
    void updateFilter();

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void initialize();
    void initializeAnimations();
    void animateExpand(ExpandState state);
    void updateDepthRecursive();
    void updateLockRecursive(bool state);
    void updateLabelPosition();
    void updateIndices(bool isInsertion, int startIndex, int startIndexInLayout, bool isProperty);
    bool hasActionButtons() const;
    bool hasComponentAncestor() const;
    int removeChildFromLayout(RowTree *child) const;
    int getCountDecendentsRecursive() const;
    int addToLayout(int indexInLayout);
    int getLastChildIndex(bool isProperty) const;

    RowTree *m_parentRow = nullptr;
    RowTimeline *m_rowTimeline = nullptr;
    int m_depth = 1;
    int m_index = 0;
    int m_indexInLayout = 1;
    bool m_shy = false;
    bool m_visible = true;
    bool m_locked = false;
    bool m_isProperty = false;
    bool m_isPropertyExpanded = false;
    bool m_master = false;
    bool m_filtered = false;
    bool m_arrowVisible = false;
    bool m_dndHover = false;
    DnDState m_dndState = DnDState::None;
    ExpandState m_expandState = ExpandState::HiddenCollapsed;
    TimelineGraphicsScene *m_scene;
    RowTreeLabelItem m_labelItem;
    EStudioObjectType m_rowType = OBJTYPE_UNKNOWN;
    QString m_propertyType; // for property rows
    QString m_label;
    QList<RowTree *> m_childRows;
    QList<RowTree *> m_childProps;
    ITimelineItemBinding *m_binding = nullptr;
    ITimelineItemProperty *m_PropBinding = nullptr; // for property rows

    QRect m_rectArrow;
    QRect m_rectShy;
    QRect m_rectVisible;
    QRect m_rectLocked;
    QRect m_rectType;

    QParallelAnimationGroup m_expandAnimation;
    QPropertyAnimation *m_expandHeightAnimation;
    QPropertyAnimation *m_expandTimelineHeightAnimation;
    QPropertyAnimation *m_expandOpacityAnimation;
    QPropertyAnimation *m_expandTimelineOpacityAnimation;

    friend class RowTimeline;
    friend class RowTimelinePropertyGraph;
};

#endif // ROWTREE_H
