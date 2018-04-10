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
    m_expandHeightAnimation->setEndValue(QSizeF(size().width(), TimelineConstants::ROW_H));
    m_expandAnimation.addAnimation(m_expandHeightAnimation);
    auto *expandOpacityAnimation = new QPropertyAnimation(this, "opacity");
    expandOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    expandOpacityAnimation->setEndValue(1);
    m_expandAnimation.addAnimation(expandOpacityAnimation);

    // Init right side expand animations
    m_expandTimelineHeightAnimation = new QPropertyAnimation(m_rowTimeline, "maximumSize");
    m_expandTimelineHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_expandTimelineHeightAnimation->setEndValue(QSizeF(m_rowTimeline->size().width(),
                                                        TimelineConstants::ROW_H));
    m_expandAnimation.addAnimation(m_expandTimelineHeightAnimation);
    auto *expandTimelineOpacityAnimation = new QPropertyAnimation(m_rowTimeline, "opacity");
    expandTimelineOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    expandTimelineOpacityAnimation->setEndValue(1);
    m_expandAnimation.addAnimation(expandTimelineOpacityAnimation);

    // Init left side collapse animations
    m_collapseHeightAnimation = new QPropertyAnimation(this, "maximumSize");
    m_collapseHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_collapseHeightAnimation->setEndValue(QSizeF(size().width(), 0));
    m_collapseAnimation.addAnimation(m_collapseHeightAnimation);
    auto *collapseOpacityAnimation = new QPropertyAnimation(this, "opacity");
    collapseOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION/2);
    collapseOpacityAnimation->setEndValue(0);
    m_collapseAnimation.addAnimation(collapseOpacityAnimation);

    // Init right side collapse animations
    m_collapseTimelineHeightAnimation = new QPropertyAnimation(m_rowTimeline, "maximumSize");
    m_collapseTimelineHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_collapseTimelineHeightAnimation->setEndValue(QSizeF(m_rowTimeline->size().width(), 0));
    m_collapseAnimation.addAnimation(m_collapseTimelineHeightAnimation);
    auto *collapseTimelineOpacityAnimation = new QPropertyAnimation(m_rowTimeline, "opacity");
    collapseTimelineOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    collapseTimelineOpacityAnimation->setEndValue(0);
    m_collapseAnimation.addAnimation(collapseTimelineOpacityAnimation);

    // Show when expanding starts
    connect(&m_expandAnimation, &QAbstractAnimation::stateChanged,
            [this](const QAbstractAnimation::State newState) {
        if (newState == QAbstractAnimation::Running) {
            setVisible(true);
            m_rowTimeline->setVisible(true);
        }
    });

    // Hide when collapsing ends
    connect(&m_collapseAnimation, &QAbstractAnimation::stateChanged,
            [this](const QAbstractAnimation::State newState) {
        if (newState == QAbstractAnimation::Stopped) {
            setVisible(false);
            m_rowTimeline->setVisible(false);
        }
    });

}

void RowTree::animateExpand(bool expand)
{
    if (expand) {
        // Update these as widths may have changed
        m_expandHeightAnimation->setEndValue(QSizeF(size().width(), TimelineConstants::ROW_H));
        m_expandTimelineHeightAnimation->setEndValue(QSizeF(m_rowTimeline->size().width(),
                                                            TimelineConstants::ROW_H));
        m_expandAnimation.start();
    } else {
        m_collapseHeightAnimation->setEndValue(QSizeF(size().width(), 0));
        m_collapseTimelineHeightAnimation->setEndValue(QSizeF(m_rowTimeline->size().width(), 0));
        m_collapseAnimation.start();
    }
}

void RowTree::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    int offset = 5 + m_depth * 15;

    // update button bounds rects
    m_rectArrow  .setRect(offset, size().height() * .5 - 8, 16, 16);
    m_rectShy    .setRect(m_treeWidth - 16 * 3.3, size().height() * .5 - 8, 16, 16);
    m_rectVisible.setRect(m_treeWidth - 16 * 2.2, size().height() * .5 - 8, 16, 16);
    m_rectLocked .setRect(m_treeWidth - 16 * 1.1, size().height() * .5 - 8, 16, 16);

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
    double y = (size().height() - pixArrow.height()) * .5;
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
    default:
        break;
    }

    if (m_isProperty)
        pixRowType = m_locked ? pixPropertyDisabled : pixPropertyNormal;

    y = (size().height() - 16) * .5;

    painter->drawPixmap(offset + 15, y, 16, 16, pixRowType);

    // Shy, eye, lock BG (to hide the label when overlapping)
    painter->fillRect(QRect(m_treeWidth - 53, 0, 53, size().height() - 1), bgColor);

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
    m_labelItem.setLocked(m_locked);
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
        animateExpand(expand);

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
    m_labelItem.setPos(offset, -3);
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

// Returns true for items with duration bar
bool RowTree::hasDurationBar() const
{
    return hasActionButtons(); // Same at least now
}
