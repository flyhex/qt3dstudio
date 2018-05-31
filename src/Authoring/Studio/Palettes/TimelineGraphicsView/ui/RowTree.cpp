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

#include "RowTree.h"
#include "RowTimeline.h"
#include "RowManager.h"
#include "TimelineConstants.h"
#include "StudioObjectTypes.h"
#include "TimelineGraphicsScene.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Qt3DSString.h"
#include "TreeHeader.h"

#include <QtGui/qpainter.h>
#include "QtGui/qtextcursor.h"
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicssceneevent.h>

// object row constructor
RowTree::RowTree(TimelineGraphicsScene *timelineScene, EStudioObjectType rowType,
                 const QString &label)
    : InteractiveTimelineItem()
    , m_rowTimeline(new RowTimeline())
{
    m_scene = timelineScene;
    m_rowType = rowType;
    m_label = label;

    initialize();
}

RowTree::~RowTree()
{
    delete m_rowTimeline; // this will also delete the keyframes
    m_rowTimeline = nullptr;
}

ITimelineItemBinding *RowTree::getBinding() const
{
    return m_binding;
}

// property row constructor
RowTree::RowTree(TimelineGraphicsScene *timelineScene, const QString &propType)
    : InteractiveTimelineItem()
    , m_rowTimeline(new RowTimeline())
{
    m_scene = timelineScene;
    m_label = propType;
    m_propertyType = propType;

    m_isProperty = true;
    m_rowTimeline->m_isProperty = true;

    initialize();
}

void RowTree::initialize()
{
    setTimelineRow(m_rowTimeline);
    m_rowTimeline->setRowTree(this);

    initializeAnimations();

    m_labelItem.setParentItem(this);
    m_labelItem.setParentRow(this);
    m_labelItem.setLabel(m_label);
    updateLabelPosition();

    // Default all rows to collapsed
    setRowVisible(false);
    m_expandState = ExpandState::HiddenCollapsed;

    connect(&m_labelItem, &RowTreeLabelItem::labelChanged, this,
            [this](const QString &label) {
        // Update label on timeline and on model
        m_label = label;
        // TODO: Get rid of CString APIs
        auto clabel = Q3DStudio::CString::fromQString(m_label);
        m_binding->GetTimelineItem()->SetName(clabel);
    });
}

void RowTree::initializeAnimations()
{
    // Init left side expand animations
    m_expandHeightAnimation = new QPropertyAnimation(this, "maximumSize");
    m_expandHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_expandAnimation.addAnimation(m_expandHeightAnimation);
    m_expandOpacityAnimation = new QPropertyAnimation(this, "opacity");
    m_expandOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION / 3);
    m_expandAnimation.addAnimation(m_expandOpacityAnimation);

    // Init right side expand animations
    m_expandTimelineHeightAnimation = new QPropertyAnimation(m_rowTimeline, "maximumSize");
    m_expandTimelineHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_expandAnimation.addAnimation(m_expandTimelineHeightAnimation);
    m_expandTimelineOpacityAnimation = new QPropertyAnimation(m_rowTimeline, "opacity");
    m_expandTimelineOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION / 3);
    m_expandAnimation.addAnimation(m_expandTimelineOpacityAnimation);

    connect(&m_expandAnimation, &QAbstractAnimation::stateChanged,
            [this](const QAbstractAnimation::State newState) {
        if (m_rowTimeline) {
            if (newState == QAbstractAnimation::Running) {
                setVisible(true);
                m_rowTimeline->setVisible(true);
            } else if (newState == QAbstractAnimation::Stopped) {
                if (this->maximumHeight() == 0) {
                    setVisible(false);
                    m_rowTimeline->setVisible(false);
                }
            }
        }
    });
}

void RowTree::animateExpand(ExpandState state)
{
    int endHeight = 0; // hidden states
    float endOpacity = 0;
    if (state == ExpandState::Expanded) {
        endHeight = m_isPropertyExpanded ? TimelineConstants::ROW_H_EXPANDED
                                         : TimelineConstants::ROW_H;
        endOpacity = 1;
    } else if (state == ExpandState::Collapsed) {
        endHeight = TimelineConstants::ROW_H;
        endOpacity = 1;
    }
    m_expandHeightAnimation->setEndValue(QSizeF(size().width(), endHeight));
    m_expandTimelineHeightAnimation->setEndValue(QSizeF(m_rowTimeline->size().width(),
                                                        endHeight));
    m_expandOpacityAnimation->setEndValue(endOpacity);
    m_expandTimelineOpacityAnimation->setEndValue(endOpacity);

    m_expandAnimation.start();
}

void RowTree::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!y()) // prevents flickering when the row is just inserted to the layout
        return;

    static const int ICON_SIZE = 16;
    const int offset = 5 + m_depth * TimelineConstants::ROW_DEPTH_STEP;
    const int iconY = (TimelineConstants::ROW_H / 2) - (ICON_SIZE / 2);

    // update button bounds rects
    m_rectArrow  .setRect(offset, iconY, ICON_SIZE, ICON_SIZE);
    m_rectType   .setRect(offset + ICON_SIZE, iconY, ICON_SIZE, ICON_SIZE);
    m_rectShy    .setRect(treeWidth() - 16 * 3.3, iconY, ICON_SIZE, ICON_SIZE);
    m_rectVisible.setRect(treeWidth() - 16 * 2.2, iconY, ICON_SIZE, ICON_SIZE);
    m_rectLocked .setRect(treeWidth() - 16 * 1.1, iconY, ICON_SIZE, ICON_SIZE);

    // Background
    QColor bgColor;
    if (m_dndState == DnDState::Source)
        bgColor = TimelineConstants::ROW_COLOR_DND_SRC;
    else if (m_isProperty)
        bgColor = TimelineConstants::ROW_COLOR_NORMAL_PROP;
    else if (m_dndHover)
        bgColor = TimelineConstants::ROW_COLOR_DND_TGT;
    else if (m_state == Selected)
        bgColor = TimelineConstants::ROW_COLOR_SELECTED;
    else if (m_state == Hovered && !m_locked)
        bgColor = TimelineConstants::ROW_COLOR_OVER;
    else
        bgColor = TimelineConstants::ROW_COLOR_NORMAL;

    painter->fillRect(QRect(0, 0, size().width(), size().height() - 1), bgColor);

    // left divider
    painter->setPen(TimelineConstants::WIDGET_BG_COLOR);
    painter->drawLine(18, 0, 18, size().height() - 1);

    // expand/collapse arrow
    static const QPixmap pixArrow = QPixmap(":/images/arrow.png");
    static const QPixmap pixArrowDown = QPixmap(":/images/arrow_down.png");
    if (m_arrowVisible)
        painter->drawPixmap(m_rectArrow, expanded() ? pixArrowDown : pixArrow);

    // Row type icon
    static const QPixmap pixSceneNormal     = QPixmap(":/images/Objects-Scene-Normal.png");
    static const QPixmap pixLayerNormal     = QPixmap(":/images/Objects-Layer-Normal.png");
    static const QPixmap pixObjectNormal    = QPixmap(":/images/Objects-Model-Normal.png");
    static const QPixmap pixLightNormal     = QPixmap(":/images/Objects-Light-Normal.png");
    static const QPixmap pixCameraNormal    = QPixmap(":/images/Objects-Camera-Normal.png");
    static const QPixmap pixTextNormal      = QPixmap(":/images/Objects-Text-Normal.png");
    static const QPixmap pixAliasNormal     = QPixmap(":/images/Objects-Alias-Normal.png");
    static const QPixmap pixGroupNormal     = QPixmap(":/images/Objects-Group-Normal.png");
    static const QPixmap pixComponentNormal = QPixmap(":/images/Objects-Component-Normal.png");
    static const QPixmap pixMaterialNormal  = QPixmap(":/images/Objects-Material-Normal.png");
    static const QPixmap pixPropertyNormal  = QPixmap(":/images/Objects-Property-Normal.png");
    static const QPixmap pixImageNormal     = QPixmap(":/images/Objects-Image-Normal.png");
    static const QPixmap pixBehaviorNormal  = QPixmap(":/images/Objects-Behavior-Normal.png");
    static const QPixmap pixEffectNormal    = QPixmap(":/images/Objects-Effect-Normal.png");

    static const QPixmap pixSceneDisabled     = QPixmap(":/images/Objects-Scene-Disabled.png");
    static const QPixmap pixLayerDisabled     = QPixmap(":/images/Objects-Layer-Disabled.png");
    static const QPixmap pixObjectDisabled    = QPixmap(":/images/Objects-Model-Disabled.png");
    static const QPixmap pixLightDisabled     = QPixmap(":/images/Objects-Light-Disabled.png");
    static const QPixmap pixCameraDisabled    = QPixmap(":/images/Objects-Camera-Disabled.png");
    static const QPixmap pixTextDisabled      = QPixmap(":/images/Objects-Text-Disabled.png");
    static const QPixmap pixAliasDisabled     = QPixmap(":/images/Objects-Alias-Disabled.png");
    static const QPixmap pixGroupDisabled     = QPixmap(":/images/Objects-Group-Disabled.png");
    static const QPixmap pixComponentDisabled = QPixmap(":/images/Objects-Component-Disabled.png");
    static const QPixmap pixMaterialDisabled  = QPixmap(":/images/Objects-Material-Disabled.png");
    static const QPixmap pixPropertyDisabled  = QPixmap(":/images/Objects-Property-Disabled.png");
    static const QPixmap pixImageDisabled     = QPixmap(":/images/Objects-Image-Disabled.png");
    static const QPixmap pixBehaviorDisabled  = QPixmap(":/images/Objects-Behavior-Disabled.png");
    static const QPixmap pixEffectDisabled    = QPixmap(":/images/Objects-Effect-Disabled.png");

    QPixmap pixRowType;
    switch (m_rowType) {
    case OBJTYPE_SCENE:
        pixRowType = m_locked ? pixSceneDisabled : pixSceneNormal;
        break;
    case OBJTYPE_LAYER:
        pixRowType = m_locked ? pixLayerDisabled : pixLayerNormal;
        break;
    case OBJTYPE_MODEL:
        pixRowType = m_locked ? pixObjectDisabled : pixObjectNormal;
        break;
    case OBJTYPE_LIGHT:
        pixRowType = m_locked ? pixLightDisabled : pixLightNormal;
        break;
    case OBJTYPE_CAMERA:
        pixRowType = m_locked ? pixCameraDisabled : pixCameraNormal;
        break;
    case OBJTYPE_TEXT:
        pixRowType = m_locked ? pixTextDisabled : pixTextNormal;
        break;
    case OBJTYPE_ALIAS:
        pixRowType = m_locked ? pixAliasDisabled : pixAliasNormal;
        break;
    case OBJTYPE_GROUP:
        pixRowType = m_locked ? pixGroupDisabled : pixGroupNormal;
        break;
    case OBJTYPE_COMPONENT:
        pixRowType = m_locked ? pixComponentDisabled : pixComponentNormal;
        break;
    case OBJTYPE_MATERIAL:
        pixRowType = m_locked ? pixMaterialDisabled : pixMaterialNormal;
        break;
    case OBJTYPE_IMAGE:
        pixRowType = m_locked ? pixImageDisabled : pixImageNormal;
        break;
    case OBJTYPE_BEHAVIOR:
        pixRowType = m_locked ? pixBehaviorDisabled : pixBehaviorNormal;
        break;
    case OBJTYPE_EFFECT:
        pixRowType = m_locked ? pixEffectDisabled : pixEffectNormal;
        break;
    default:
        break;
    }

    if (m_isProperty)
        pixRowType = m_locked ? pixPropertyDisabled : pixPropertyNormal;

    painter->drawPixmap(m_rectType, pixRowType);

    // Shy, eye, lock separator
    painter->fillRect(QRect(treeWidth() - TimelineConstants::TREE_ICONS_W,
                            0, 1, size().height()),
                      TimelineConstants::WIDGET_BG_COLOR);

    // Shy, eye, lock
    static const QPixmap pixEmpty = QPixmap(":/images/Toggle-Empty.png");
    static const QPixmap pixShy = QPixmap(":/images/Toggle-Shy.png");
    static const QPixmap pixHide = QPixmap(":/images/Toggle-HideShow.png");
    static const QPixmap pixLock = QPixmap(":/images/Toggle-Lock.png");
    if (hasActionButtons()) {
        painter->drawPixmap(m_rectShy    , m_shy     ? pixShy  : pixEmpty);
        painter->drawPixmap(m_rectVisible, m_visible ? pixHide : pixEmpty);
        painter->drawPixmap(m_rectLocked , m_locked  ? pixLock : pixEmpty);
    }

    // Candidate parent of a dragged row
    if (m_dndState == DnDState::Parent) {
        painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 1));
        painter->drawRect(QRect(1, 1, treeWidth() - 2, size().height() - 3));
    }

    // Action indicators
    static const QPixmap pixMasterAction = QPixmap(":/images/Action-MasterAction.png");
    static const QPixmap pixAction = QPixmap(":/images/Action-Action.png");
    static const QPixmap pixChildMasterAction = QPixmap(":/images/Action-ChildMasterAction.png");
    static const QPixmap pixChildAction = QPixmap(":/images/Action-ChildAction.png");
    static const QPixmap pixCompMasterAction = QPixmap(":/images/Action-ComponentMasterAction.png");
    static const QPixmap pixCompAction = QPixmap(":/images/Action-ComponentAction.png");

    if (!isProperty()) {
        // Don't access binding when we are in inconsistent state
        // TODO: Refactor so we don't need to access binding during paint
        if (m_scene->widgetTimeline()->isFullReconstructPending())
            return;

        Qt3DSDMTimelineItemBinding *itemBinding =
                static_cast<Qt3DSDMTimelineItemBinding *>(m_binding);
        if (itemBinding->HasAction(true)) // has master action
            painter->drawPixmap(0, 0, pixMasterAction);
        else if (itemBinding->HasAction(false)) // has action
            painter->drawPixmap(0, 0, pixAction);

        if (!expanded()) {
            if (itemBinding->ChildrenHasAction(true)) // children have master action
                painter->drawPixmap(0, 0, pixChildMasterAction);
            else if (itemBinding->ChildrenHasAction(false)) // children have action
                painter->drawPixmap(0, 0, pixChildAction);
        }

        if (itemBinding->ComponentHasAction(true)) // component has master action
            painter->drawPixmap(0, 0, pixCompMasterAction);
        else if (itemBinding->ComponentHasAction(false)) // component has action
            painter->drawPixmap(0, 0, pixCompAction);
    }
}

int RowTree::treeWidth() const
{
    return m_scene->treeWidth();
}

void RowTree::setBinding(ITimelineItemBinding *binding)
{
    m_binding = binding;

    // Restore the expansion state of rows
    m_expandState = m_scene->expandMap().value(
                static_cast<Qt3DSDMTimelineItemBinding *>(binding)->GetInstance(),
                ExpandState::Unknown);

    if (m_expandState == ExpandState::Unknown) {
        // Everything but scene/component is initially collapsed and hidden
        if (m_rowType == OBJTYPE_SCENE || m_rowType == OBJTYPE_COMPONENT)
            m_expandState = ExpandState::Expanded;
        else
            m_expandState = ExpandState::HiddenCollapsed;
    }

    // Make sure all children of visible expanded parents are shown, and vice versa
    if (parentRow()) {
        if (parentRow()->expanded()) {
            if (m_expandState == ExpandState::HiddenCollapsed)
                m_expandState = ExpandState::Collapsed;
            else if (m_expandState == ExpandState::HiddenExpanded)
                m_expandState = ExpandState::Expanded;
        } else {
            if (m_expandState == ExpandState::Collapsed)
                m_expandState = ExpandState::HiddenCollapsed;
            else if (m_expandState == ExpandState::Expanded)
                m_expandState = ExpandState::HiddenExpanded;
        }
    }

    setRowVisible(m_expandState == ExpandState::Collapsed
            || m_expandState == ExpandState::Expanded);

    updateFromBinding();
}

ITimelineItemProperty *RowTree::propBinding()
{
    return m_PropBinding;
}

void RowTree::setPropBinding(ITimelineItemProperty *binding)
{
    m_PropBinding = binding;

    if (parentRow()->expanded())
        setRowVisible(true);

    // Update label color
    m_labelItem.setMaster(m_PropBinding->IsMaster());
}

void RowTree::setState(State state)
{
    m_state = state;
    m_rowTimeline->m_state = state;

    update();
    m_rowTimeline->update();
}

void RowTree::setTimelineRow(RowTimeline *rowTimeline)
{
    m_rowTimeline = rowTimeline;
}

void RowTree::selectLabel()
{
    m_labelItem.setEnabled(true);
    m_labelItem.setFocus();
    // Select all text
    QTextCursor cursor = m_labelItem.textCursor();
    cursor.select(QTextCursor::Document);
    m_labelItem.setTextCursor(cursor);
}

RowTree *RowTree::parentRow() const
{
    return m_parentRow;
}

int RowTree::depth() const
{
    return m_depth;
}

EStudioObjectType RowTree::rowType() const
{
   return m_rowType;
}

QString RowTree::propertyType() const
{
    return m_propertyType;
}

int RowTree::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRowTree;
}

int RowTree::index() const
{
    // first child in a parent has index 0
    return m_index;
}

int RowTree::indexInLayout() const
{
    // first child (scene) at index 1, tree header at index 0 (invisible rows are also counted)
    return m_indexInLayout;
}

void RowTree::addChild(RowTree *child)
{
    int index = getLastChildIndex(child->isProperty()) + 1;
    addChildAt(child, index);
}

int RowTree::getLastChildIndex(bool isProperty) const
{
    int index = -1;
    if (isProperty && !m_childProps.empty())
        index = m_childProps.last()->index();
    else if (!isProperty && !m_childRows.empty())
        index = m_childRows.last()->index();

    return index;
}

void RowTree::updateArrowVisibility()
{
    bool oldVisibility = m_arrowVisible;
    if (m_childRows.empty() && m_childProps.empty()) {
        m_arrowVisible = false;
    } else {
        if (m_childProps.empty()) {
            m_arrowVisible = false;
            for (RowTree *row : qAsConst(m_childRows)) {
                if (!row->m_filtered) {
                    m_arrowVisible = true;
                    break;
                }
            }
        } else {
            m_arrowVisible = true;
        }
    }
    if (oldVisibility != m_arrowVisible)
        update();
}

void RowTree::updateFilter()
{
    bool parentOk = !m_parentRow || m_parentRow->isVisible();
    bool shyOk     = !m_shy      || !m_scene->treeHeader()->filterShy();
    bool visibleOk = m_visible   || !m_scene->treeHeader()->filterHidden();
    bool lockOk    = !m_locked   || !m_scene->treeHeader()->filterLocked();
    bool expandOk  = !expandHidden();

    m_filtered = !(shyOk && visibleOk && lockOk);
    setVisible(parentOk && shyOk && visibleOk && lockOk && expandOk);
    m_rowTimeline->setVisible(isVisible());
}

int RowTree::getCountDecendentsRecursive() const
{
    int num = m_childProps.count();

    for (auto child : qAsConst(m_childRows)) {
       num++;
       num += child->getCountDecendentsRecursive();
    }

    return num;
}

void RowTree::addChildAt(RowTree *child, int index)
{
    // Mahmoud_TODO: improvement: implement moving the child (instead of remove/add) if it is added
    //               under the same parent.

    int maxIndex = getLastChildIndex(child->isProperty()) + 1;

    if (index > maxIndex)
        index = maxIndex;

    if (child->parentRow() == this && index == child->m_index) // same place
        return;

    if (child->parentRow())
        child->parentRow()->removeChild(child);

    child->m_index = index;

    QList<RowTree *> &childRows = child->isProperty() ? m_childProps : m_childRows;
    int updateIndexInLayout = child->m_indexInLayout;
    child->m_indexInLayout = m_indexInLayout + index + 1;

    if (!child->isProperty()) {
        child->m_indexInLayout += m_childProps.count();

        if (!m_childRows.empty()) {
            for (int i = 0; i < index; ++i)
                child->m_indexInLayout += m_childRows.at(i)->getCountDecendentsRecursive();
        }
    }

    if (!childRows.contains(child))
        childRows.insert(index, child);

    child->m_parentRow = this;
    child->updateDepthRecursive();
    if (!child->isProperty()) {
        m_rowTimeline->updateChildrenMinStartXRecursive(this);
        m_rowTimeline->updateChildrenMaxEndXRecursive(this);
    }

    // update the layout
    child->addToLayout(child->m_indexInLayout);

    // update indices
    updateIndexInLayout = std::min(updateIndexInLayout, child->m_indexInLayout);
    updateIndices(true, child->m_index + 1, updateIndexInLayout, child->isProperty());
    updateArrowVisibility();
}

int RowTree::addToLayout(int indexInLayout)
{
    m_scene->layoutTree()->insertItem(indexInLayout, this);
    m_scene->layoutTimeline()->insertItem(indexInLayout, rowTimeline());

    indexInLayout++;

    for (auto p : qAsConst(m_childProps))
        indexInLayout = p->addToLayout(indexInLayout);

    for (auto c : qAsConst(m_childRows))
        indexInLayout = c->addToLayout(indexInLayout);

    return indexInLayout;
}

RowTree *RowTree::getChildAt(int index) const
{
    if (index < 0 || index > m_childRows.count() - 1)
        return nullptr;

    return m_childRows.at(index);
}

// TODO: so far not used, delete if end up not used
void RowTree::moveChild(int from, int to)
{
    m_childRows.move(from, to);
}

// this does not destroy the row, just remove it from the layout and parenting hierarchy
void RowTree::removeChild(RowTree *child)
{
    if (m_childProps.contains(child) || m_childRows.contains(child)) { // child exists
        removeChildFromLayout(child);

        // detach from parent
        if (child->isProperty())
            m_childProps.removeAll(child);
        else
            m_childRows.removeAll(child);

        child->m_depth = -1;
        child->m_parentRow = nullptr;

        updateIndices(false, child->m_index, child->m_indexInLayout, child->isProperty());
        updateArrowVisibility();
    }
}

int RowTree::removeChildFromLayout(RowTree *child) const
{
    int numRemoved = 0;
    int deleteIndex = child->m_indexInLayout;
    for (;;) {
       RowTree *row_i = static_cast<RowTree *>(m_scene->layoutTree()->itemAt(deleteIndex)
                                               ->graphicsItem());
       if (row_i->depth() <= child->depth() && numRemoved > 0)
           break;

       m_scene->layoutTree()->removeItem(row_i);
       m_scene->layoutTimeline()->removeItem(row_i->rowTimeline());
       numRemoved++;

       if (m_scene->layoutTree()->count() == deleteIndex) // reached end of the list
           break;
    }

    return numRemoved;
}

bool RowTree::draggable() const
{
    return !m_locked && !isProperty()
           && m_rowType != OBJTYPE_IMAGE
           && m_rowType != OBJTYPE_SCENE
           && m_rowType != OBJTYPE_MATERIAL;
}

void RowTree::updateDepthRecursive()
{
    if (m_parentRow) {
        m_depth = m_parentRow->m_depth + 1;
        updateLabelPosition();

        for (auto p : qAsConst(m_childProps))
            p->updateDepthRecursive();

        for (auto r : qAsConst(m_childRows))
            r->updateDepthRecursive();
    }
}

// update this parent's children indices after a child row is inserted or removed
void RowTree::updateIndices(bool isInsertion, int index, int indexInLayout, bool isProperty)
{
    // update index
    if (isProperty && index < m_childProps.count()) {
        for (int i = index; i < m_childProps.count(); i++)
            m_childProps.at(i)->m_index += isInsertion ? 1 : -1;
    } else if (!isProperty && index < m_childRows.count()) {
        for (int i = index; i < m_childRows.count(); i++)
            m_childRows.at(i)->m_index += isInsertion ? 1 : -1;
    }

    // update indexInLayout
    for (int i = indexInLayout; i < m_scene->layoutTree()->count(); ++i) {
        RowTree *row_i = static_cast<RowTree *>(m_scene->layoutTree()->itemAt(i)->graphicsItem());
        row_i->m_indexInLayout = i;
    }
}

void RowTree::updateFromBinding()
{
    // update view (shy, visible, locked)
    m_shy = m_binding->GetTimelineItem()->IsShy();
    m_visible = m_binding->GetTimelineItem()->IsVisible();
    m_locked = m_binding->GetTimelineItem()->IsLocked();

    // Update label locking & color
    Qt3DSDMTimelineItemBinding *itemBinding =
            static_cast<Qt3DSDMTimelineItemBinding *>(m_binding);
    m_labelItem.setLocked(m_locked);
    m_master = itemBinding->IsMaster();
    m_labelItem.setMaster(m_master);
}

void RowTree::updateLabel()
{
    if (m_binding)
        m_labelItem.setLabel(m_binding->GetTimelineItem()->GetName().toQString());
}

void RowTree::setRowVisible(bool visible)
{
    if (visible) {
        setMaximumHeight(TimelineConstants::ROW_H);
        setOpacity(1.0);
        setVisible(true);
        m_rowTimeline->setMaximumHeight(TimelineConstants::ROW_H);
        m_rowTimeline->setOpacity(1.0);
        m_rowTimeline->setVisible(true);
    } else {
        setMaximumHeight(0.0);
        setOpacity(0.0);
        setVisible(false);
        m_rowTimeline->setMaximumHeight(0.0);
        m_rowTimeline->setOpacity(0.0);
        m_rowTimeline->setVisible(false);
    }
}

bool RowTree::hasPropertyChildren() const
{
    return !m_childProps.empty();
}

void RowTree::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF p = event->pos();
    if (m_rectType.contains(p.x(), p.y()) && !m_locked)
        if (m_binding)
            m_binding->OpenAssociatedEditor();
}

// handle clicked control and return its type
TreeControlType RowTree::getClickedControl(const QPointF &scenePos)
{
    QPointF p = mapFromScene(scenePos.x(), scenePos.y());
    if (m_arrowVisible && m_rectArrow.contains(p.x(), p.y()) && !m_locked) {
        updateExpandStatus(m_expandState == ExpandState::Expanded ? ExpandState::Collapsed
                                                                  : ExpandState::Expanded, false);
        update();
        return TreeControlType::Arrow;
    }

    if (hasActionButtons()) {
        if (m_rectShy.contains(p.x(), p.y())) {
            toggleShy();
            return TreeControlType::Shy;
        } else if (m_rectVisible.contains(p.x(), p.y())) {
            toggleVisible();
            return TreeControlType::Hide;
        } else if (m_rectLocked.contains(p.x(), p.y())) {
            toggleLocked();
            return TreeControlType::Lock;
        }
    }

    return TreeControlType::None;
}

void RowTree::updateExpandStatus(ExpandState state, bool animate)
{
    if (m_expandState == state)
        return;

    m_expandState = state;

    if (m_scene->widgetTimeline()->isFullReconstructPending())
        return;

    // Store the expanded state of items so we can restore it on slide change
    if (m_binding) {
        m_scene->expandMap().insert(
                    static_cast<Qt3DSDMTimelineItemBinding *>(m_binding)->GetInstance(),
                    m_expandState);
    }

    if (animate)
        animateExpand(m_expandState);

    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows)) {
            if (state == ExpandState::Expanded) {
                if (child->m_expandState == ExpandState::HiddenExpanded)
                    child->updateExpandStatus(ExpandState::Expanded);
                else if (child->m_expandState == ExpandState::HiddenCollapsed)
                    child->updateExpandStatus(ExpandState::Collapsed);
            } else {
                if (child->m_expandState == ExpandState::Expanded)
                    child->updateExpandStatus(ExpandState::HiddenExpanded);
                else if (child->m_expandState == ExpandState::Collapsed)
                    child->updateExpandStatus(ExpandState::HiddenCollapsed);
            }
        }
    }

    if (!m_childProps.empty()) {
        for (auto child : qAsConst(m_childProps)) {
            // Properties can never be collapsed
            if (state == ExpandState::Expanded)
                child->updateExpandStatus(ExpandState::Expanded);
            else
                child->updateExpandStatus(ExpandState::HiddenExpanded);
        }
    }

    updateFilter();
}

void RowTree::updateLockRecursive(bool state)
{
    m_locked = state;
    m_labelItem.setLocked(m_locked);
    update();

    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows))
            child->updateLockRecursive(m_locked);
    }
}

void RowTree::updateLabelPosition()
{
    int offset = 5 + m_depth * TimelineConstants::ROW_DEPTH_STEP + 30;
    m_labelItem.setPos(offset, -1);
}

bool RowTree::expanded() const
{
    if (m_isProperty)
        return false;
    else
        return m_expandState == ExpandState::Expanded;
}

bool RowTree::expandHidden() const
{
    return m_expandState == ExpandState::HiddenExpanded
            || m_expandState == ExpandState::HiddenCollapsed;
}

bool RowTree::isDecendentOf(RowTree *row) const
{
    RowTree *parentRow = m_parentRow;

    while (parentRow) {
        if (parentRow == row)
            return true;

        parentRow = parentRow->parentRow();
    }

    return false;
}

void RowTree::setDnDHover(bool val)
{
    m_dndHover = val;
    update();
}

void RowTree::setDnDState(DnDState state, DnDState onlyIfState, bool recursive)
{
    if (m_dndState == onlyIfState || onlyIfState == DnDState::Any) {
        m_dndState = state;
        update();

        if (recursive) { // used by source rows to highlights all of their descendants
            for (auto child : qAsConst(m_childProps))
                child->setDnDState(state, onlyIfState, true);

            for (auto child : qAsConst(m_childRows))
                child->setDnDState(state, onlyIfState, true);
        }
    }
}

RowTree::DnDState RowTree::getDnDState() const
{
    return m_dndState;
}

bool RowTree::isContainer() const
{
    if (isComponent() && m_indexInLayout == 1) // root element inside a component
        return true;

    return !m_isProperty
            && m_rowType != OBJTYPE_ALIAS
            && m_rowType != OBJTYPE_MATERIAL
            && m_rowType != OBJTYPE_IMAGE
            && m_rowType != OBJTYPE_TEXT
            && m_rowType != OBJTYPE_COMPONENT
            && m_rowType != OBJTYPE_BEHAVIOR
            && m_rowType != OBJTYPE_EFFECT;
}

bool RowTree::isProperty() const
{
    return m_isProperty;
}

RowTree *RowTree::getPropertyRow(const QString &type) const
{
    for (RowTree *prop : qAsConst(m_childProps)) {
        if (prop->label() == type)
            return prop;
    }

    return nullptr;
}


bool RowTree::isPropertyOrMaterial() const
{
    return m_isProperty || m_rowType == OBJTYPE_MATERIAL || m_rowType == OBJTYPE_IMAGE;
}

bool RowTree::isComponent() const
{
    return m_rowType == OBJTYPE_COMPONENT;
}

bool RowTree::isMaster() const
{
    return m_master;
}

bool RowTree::empty() const
{
    return m_childRows.empty() && m_childProps.empty();
}

bool RowTree::selected() const
{
    return m_state == Selected;
}

QList<RowTree *> RowTree::childRows() const
{
    return m_childRows;
}

QList<RowTree *> RowTree::childProps() const
{
    return m_childProps;
}

RowTimeline *RowTree::rowTimeline() const
{
    return m_rowTimeline;
}

QString RowTree::label() const
{
    return m_label;
}

void RowTree::toggleShy()
{
    if (hasActionButtons()) {
        m_shy = !m_shy;
        update();
        m_binding->GetTimelineItem()->SetShy(m_shy);
    }
}

void RowTree::toggleVisible()
{
    if (hasActionButtons()) {
        m_visible = !m_visible;
        update();
        m_binding->GetTimelineItem()->SetVisible(m_visible);
    }
}

void RowTree::toggleLocked()
{
    if (hasActionButtons()) {
        updateLockRecursive(!m_locked);
        m_binding->GetTimelineItem()->SetLocked(m_locked);
        if (m_locked && selected())
            m_scene->rowManager()->clearSelection();
    }
}

bool RowTree::shy() const
{
    return m_shy;
}

bool RowTree::visible() const
{
    return m_visible;
}

bool RowTree::locked() const
{
    return m_locked;
}

// Returns true for items with shy/visible/lock buttons
bool RowTree::hasActionButtons() const
{
    return (!m_isProperty
            && m_rowType != OBJTYPE_SCENE
            && m_rowType != OBJTYPE_MATERIAL
            && m_rowType != OBJTYPE_IMAGE);
}

bool RowTree::hasComponentAncestor() const
{
    RowTree *parentRow = m_parentRow;
    while (parentRow) {
        if (parentRow->rowType() == OBJTYPE_COMPONENT)
            return true;
        parentRow = parentRow->parentRow();
    }
    return false;
}

// Returns true for items with duration bar
bool RowTree::hasDurationBar() const
{
    return hasActionButtons(); // Same at least now
}

bool RowTree::propertyExpanded() const
{
    return m_isPropertyExpanded;
}

void RowTree::togglePropertyExpanded()
{
    setPropertyExpanded(!m_isPropertyExpanded);
}

void RowTree::setPropertyExpanded(bool expand)
{
    m_isPropertyExpanded = expand;
    if (m_isPropertyExpanded)
        animateExpand(ExpandState::Expanded);
    else
        animateExpand(ExpandState::Collapsed);
}
