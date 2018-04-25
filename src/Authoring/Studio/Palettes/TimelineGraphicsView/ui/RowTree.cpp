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
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Qt3DSString.h"

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
    m_labelItem.setRowTypeLabel(m_rowType);

    initialize();
}

RowTree::~RowTree()
{
    delete m_rowTimeline; // this will also delete the keyframes
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
        if (newState == QAbstractAnimation::Running) {
            setVisible(true);
            m_rowTimeline->setVisible(true);
        } else if (newState == QAbstractAnimation::Stopped) {
            if (this->maximumHeight() == 0) {
                setVisible(false);
                m_rowTimeline->setVisible(false);
            }
        }
    });

}

void RowTree::animateExpand(ExpandState state)
{
    int endHeight = 0; // ExpandState::Hidden
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
    const int offset = 5 + m_depth * 15;
    const int iconSize = 16;
    const int iconY = (TimelineConstants::ROW_H / 2) - (iconSize / 2);

    // update button bounds rects
    m_rectArrow  .setRect(offset, iconY, iconSize, iconSize);
    m_rectType   .setRect(offset + iconSize, iconY, iconSize, iconSize);
    m_rectShy    .setRect(m_treeWidth - 16 * 3.3, iconY, iconSize, iconSize);
    m_rectVisible.setRect(m_treeWidth - 16 * 2.2, iconY, iconSize, iconSize);
    m_rectLocked .setRect(m_treeWidth - 16 * 1.1, iconY, iconSize, iconSize);

    // Background
    QColor bgColor;
    if (m_moveSource)
        bgColor = TimelineConstants::ROW_COLOR_MOVE_SRC;
    else if (m_isProperty)
        bgColor = TimelineConstants::ROW_COLOR_NORMAL_PROP;
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
    if (!m_childRows.empty())
        painter->drawPixmap(m_rectArrow, m_expanded ? pixArrowDown : pixArrow);

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
    painter->fillRect(QRect(m_treeWidth - TimelineConstants::TREE_ICONS_W,
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
    if (m_moveTarget) {
        painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 1));
        painter->drawRect(QRect(1, 1, m_treeWidth - 2, size().height() - 3));
    }
}

int RowTree::treeWidth() const
{
    return m_treeWidth;
}

void RowTree::setTreeWidth(double w)
{
    m_treeWidth = w;
    update();
}

void RowTree::setBinding(ITimelineItemBinding *binding)
{
    m_binding = binding;

    // update view (shy, visible, locked)
    m_shy = m_binding->GetTimelineItem()->IsShy();
    m_visible = m_binding->GetTimelineItem()->IsVisible();
    m_locked = m_binding->GetTimelineItem()->IsLocked();

    // Update label locking & color
    Qt3DSDMTimelineItemBinding *itemBinding =
            static_cast<Qt3DSDMTimelineItemBinding *>(binding);
    m_labelItem.setLocked(m_locked);
    m_labelItem.setMaster(itemBinding->IsMaster());
}

ITimelineItemProperty *RowTree::propBinding()
{
    return m_PropBinding;
}

void RowTree::setPropBinding(ITimelineItemProperty *binding)
{
    m_PropBinding = binding;
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

void RowTree::addChild(RowTree *child)
{
    if (child->parentRow() != this) {
        if (child->parentRow() != nullptr)
            child->parentRow()->removeChild(child);

        if (!m_childRows.contains(child)) {
            m_childRows.prepend(child);
            child->m_depth = m_depth + 1;
            child->m_parentRow = this;

            child->updateDepthRecursive();
            m_rowTimeline->updateChildrenMinStartXRecursive(this);
            m_rowTimeline->updateChildrenMaxEndXRecursive(this);
        }
    }
}

void RowTree::updateDepthRecursive()
{
    m_depth = m_parentRow->m_depth + 1;

    if (!empty()) {
        for (auto child : qAsConst(m_childRows))
            child->updateDepthRecursive();
    }
    updateLabelPosition();
}

// TODO: so far not used, delete if end up not used
void RowTree::moveChild(int from, int to)
{
    m_childRows.move(from, to);
}

void RowTree::removeChild(RowTree *child)
{
    if (m_childRows.contains(child)) {
        m_childRows.removeAll(child);
        child->m_depth = -1;
        child->m_parentRow = nullptr;

        if (m_childRows.empty())
            m_expanded = true;
        child->updateLabelPosition();
    }
}

bool RowTree::hasPropertyChildren()
{
    return m_scene->rowManager()->hasProperties(this);
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
    if (!m_childRows.empty() && m_rectArrow.contains(p.x(), p.y()) && !m_locked) {
        m_expanded = !m_expanded;
        updateExpandStatus(m_expanded, true);
        update();
        return TreeControlType::Arrow;
    }

    if (hasActionButtons()) {
        if (m_rectShy.contains(p.x(), p.y())) {
            m_shy = !m_shy;
            update();

            m_binding->GetTimelineItem()->SetShy(m_shy);

            return TreeControlType::Shy;
        } else if (m_rectVisible.contains(p.x(), p.y())) {
            m_visible = !m_visible;
            update();

            m_binding->GetTimelineItem()->SetVisible(m_visible);


            return TreeControlType::Hide;
        } else if (m_rectLocked.contains(p.x(), p.y())) {
            updateLockRecursive(!m_locked);

            m_binding->GetTimelineItem()->SetLocked(m_locked);

            if (m_locked && selected())
                m_scene->rowManager()->clearSelection();

            return TreeControlType::Lock;
        }
    }

    return TreeControlType::None;
}

void RowTree::updateExpandStatus(bool expand, bool childrenOnly)
{
    if (!childrenOnly)
        animateExpand(expand ? ExpandState::Expanded : ExpandState::Hidden);

    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows))
            child->updateExpandStatus(expand && child->parentRow()->m_expanded);
    }
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
    int offset = 5 + m_depth * 15 + 30;
    m_labelItem.setPos(offset, -1);
}

bool RowTree::expanded() const
{
    return m_expanded;
}

bool RowTree::isDecendentOf(RowTree *row) const
{
    RowTree *parentRow = m_parentRow;

    while (parentRow != nullptr) {
        if (parentRow == row)
            return true;

        parentRow = parentRow->parentRow();
    }

    return false;
}

void RowTree::setMoveSourceRecursive(bool value)
{
    m_moveSource = value;
    update();
    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows))
            child->setMoveSourceRecursive(value);
    }
}

bool RowTree::isContainer() const
{
    return  m_rowType == OBJTYPE_SCENE
         || m_rowType == OBJTYPE_LAYER
         || m_rowType == OBJTYPE_GROUP
         || m_rowType == OBJTYPE_COMPONENT;
}

bool RowTree::isProperty() const
{
    return m_isProperty;
}

bool RowTree::empty() const
{
    return m_childRows.empty();
}

bool RowTree::selected() const
{
    return m_state == Selected;
}

void RowTree::setMoveTarget(bool value)
{
    m_moveTarget = value;
    update();
}

QList<RowTree *> RowTree::childRows() const
{
    return m_childRows;
}

RowTimeline *RowTree::rowTimeline() const
{
    return m_rowTimeline;
}

QString RowTree::label() const
{
    return m_label;
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

bool RowTree::hasComponentAncestor()
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
