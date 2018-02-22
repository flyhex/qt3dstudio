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
#include "TimelineConstants.h"

#include <QtGui/qpainter.h>
#include <QtWidgets/qgraphicssceneevent.h>

RowTree::RowTree(Ruler *ruler, RowType rowType, const QString &label)
    : InteractiveTimelineItem()
    , m_rowTimeline(new RowTimeline(ruler))
{
    m_rowType = rowType;
    m_label = label;

    setTimelineRow(m_rowTimeline);
    m_rowTimeline->setRowTree(this);
}

RowTree::RowTree(Ruler *ruler, PropertyType propType)
    : InteractiveTimelineItem()
    , m_rowTimeline(new RowTimeline(ruler))
{
    m_rowType = RowType::Property;
    m_propertyType = propType;
    updatePropertyLabel();

    setTimelineRow(m_rowTimeline);
    m_rowTimeline->setRowTree(this);
}

void RowTree::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    int offset = 5 + m_depth * 15;

    // update button bounds rects
    m_rectArrow  .setRect(offset, size().height() * .5 - 8, 16, 16);
    m_rectShy    .setRect(size().width() - 16 * 3.3, size().height() * .5 - 8, 16, 16);
    m_rectVisible.setRect(size().width() - 16 * 2.2, size().height() * .5 - 8, 16, 16);
    m_rectLocked .setRect(size().width() - 16 * 1.1, size().height() * .5 - 8, 16, 16);

    // Background
    QColor bgColor;
    if (m_moveSource)
        bgColor = TimelineConstants::ROW_COLOR_MOVE_SRC;
    else if (m_state == Selected)
        bgColor = TimelineConstants::ROW_COLOR_SELECTED;
    else if (m_state == Hovered)
        bgColor = TimelineConstants::ROW_COLOR_OVER;
    else if (m_rowType == RowType::Property)
        bgColor = TimelineConstants::ROW_COLOR_NORMAL_PROP;
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
    static const QPixmap pixScene = QPixmap(":/images/Objects-Scene-Normal.png");
    static const QPixmap pixLayer = QPixmap(":/images/Asset-Layer-Normal.png");
    static const QPixmap pixObject = QPixmap(":/images/Asset-Cube-Normal.png");
    static const QPixmap pixLight = QPixmap(":/images/Asset-Light-Normal.png");
    static const QPixmap pixCamera = QPixmap(":/images/Asset-Camera-Normal.png");
    static const QPixmap pixText = QPixmap(":/images/Asset-Text-Normal.png");
    static const QPixmap pixAlias = QPixmap(":/images/Asset-Alias-Normal.png");
    static const QPixmap pixGroup = QPixmap(":/images/Asset-Group-Normal.png");
    static const QPixmap pixComponent = QPixmap(":/images/Asset-Component-Normal.png");
    static const QPixmap pixProperty = QPixmap(":/images/Objects-Property-Normal.png");

    QPixmap pixRowType;
    QString rowLabel;
    switch (m_rowType) {
    case RowType::Scene:
        pixRowType = pixScene;
        rowLabel = tr("Scene");
        break;
    case RowType::Layer:
        pixRowType = pixLayer;
        rowLabel = tr("Layer");
        break;
    case RowType::Object:
        pixRowType = pixObject;
        rowLabel = tr("Object");
        break;
    case RowType::Light:
        pixRowType = pixLight;
        rowLabel = tr("Light");
        break;
    case RowType::Camera:
        pixRowType = pixCamera;
        rowLabel = tr("Camera");
        break;
    case RowType::Text:
        pixRowType = pixText;
        rowLabel = tr("Text");
        break;
    case RowType::Alias:
        pixRowType = pixAlias;
        rowLabel = tr("Alias");
        break;
    case RowType::Group:
        pixRowType = pixGroup;
        rowLabel = tr("Group");
        break;
    case RowType::Component:
        pixRowType = pixComponent;
        rowLabel = tr("Component");
        break;
    case RowType::Property:
        pixRowType = pixProperty;
        rowLabel = tr("Property");
        break;
    default:
        pixRowType = pixLayer;
        rowLabel = tr("Layer");
    }

    if (m_label == 0)
        m_label = rowLabel;

    y = (size().height() - 16) * .5;
    painter->drawPixmap(offset + 15, y, 16, 16, pixRowType);

    // Label
    painter->setPen(QColor(TimelineConstants::ROW_TEXT_COLOR));
    painter->drawText(offset + 35, size().height() * .5 + 4, tr("%1").arg(m_label));

    // Shy, eye, lock BG (to hide the label when overlapping)
    painter->fillRect(QRect(size().width() - 53, 0, 53, size().height() - 1), bgColor);

    // Shy, eye, lock
    static const QPixmap pixEmpty = QPixmap(":/images/Toggle-Empty.png");
    static const QPixmap pixShy = QPixmap(":/images/Toggle-Shy.png");
    static const QPixmap pixHide = QPixmap(":/images/Toggle-HideShow.png");
    static const QPixmap pixLock = QPixmap(":/images/Toggle-Lock.png");
    if (m_rowType != RowType::Property) {
        painter->drawPixmap(m_rectShy    , m_shy     ? pixShy  : pixEmpty);
        painter->drawPixmap(m_rectVisible, m_visible ? pixHide : pixEmpty);
        painter->drawPixmap(m_rectLocked , m_locked  ? pixLock : pixEmpty);
    }

    // Candidate parent of a dragged row
    if (m_moveTarget) {
        painter->setPen(QPen(QColor(TimelineConstants::ROW_MOVER_COLOR), 1));
        painter->drawRect(QRect(1, 1, size().width()-2, size().height() - 3));
    }
}

void RowTree::updatePropertyLabel()
{
    switch (m_propertyType) {
    case PropertyType::Position:
        m_label = tr("Position");
        break;
    case PropertyType::Rotation:
        m_label = tr("Rotation");
        break;
    case PropertyType::Scale:
        m_label = tr("Scale");
        break;
    case PropertyType::Pivot:
        m_label = tr("Pivot");
        break;
    case PropertyType::Opacity:
        m_label = tr("Opacity");
        break;
    case PropertyType::EdgeTessellation:
        m_label = tr("Edge Tessellation");
        break;
    case PropertyType::InnerTessellation:
        m_label = tr("Inner Tessellation");
        break;
    case PropertyType::TextColor:
        m_label = tr("Text Color");
        break;
    case PropertyType::Leading:
        m_label = tr("Leading");
        break;
    case PropertyType::Tracking:
        m_label = tr("Tracking");
        break;
    case PropertyType::LightColor:
        m_label = tr("Light Color");
        break;
    case PropertyType::SpecularColor:
        m_label = tr("Specular Color");
        break;
    case PropertyType::AmbientColor:
        m_label = tr("Ambient Color");
        break;
    case PropertyType::Brightness:
        m_label = tr("Brightness");
        break;
    case PropertyType::ShadowDarkness:
        m_label = tr("Shadow Darkness");
        break;
    case PropertyType::ShadowSoftness:
        m_label = tr("Shadow Softness");
        break;
    case PropertyType::ShadowDepthBias:
        m_label = tr("Shadow Depth Bias");
        break;
    case PropertyType::FieldOfView:
        m_label = tr("Field Of View");
        break;
    case PropertyType::ClippingStart:
        m_label = tr("Clipping Start");
        break;
    case PropertyType::ClippingEnd:
        m_label = tr("Clipping End");
        break;
    case PropertyType::Left:
        m_label = tr("Left");
        break;
    case PropertyType::Top:
        m_label = tr("Top");
        break;
    case PropertyType::Width:
        m_label = tr("Width");
        break;
    case PropertyType::Height:
        m_label = tr("Height");
        break;
    case PropertyType::AO:
        m_label = tr("Ambient Occlusion");
        break;
    case PropertyType::AODistance:
        m_label = tr("AO Distance");
        break;
    case PropertyType::AOSoftness:
        m_label = tr("AO Softness");
        break;
    case PropertyType::AOThreshold:
        m_label = tr("AO Threshold");
        break;
    case PropertyType::AOSamplingRate:
        m_label = tr("AO Sampling Rate");
        break;
    case PropertyType::IBLBrightness:
        m_label = tr("IBL Brightness");
        break;
    case PropertyType::IBLHorizonCutoff:
        m_label = tr("IBL Horizon Cutoff");
        break;
    case PropertyType::IBLFOVAngle:
        m_label = tr("IBL FOV Angle");
        break;
    case PropertyType::ProbeCrossfade:
        m_label = tr("Probe Crossfade");
        break;
    default:
        m_label = tr("Unnamed Property");
    }
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

RowTree *RowTree::parentRow() const
{
    return m_parentRow;
}

int RowTree::depth() const
{
    return m_depth;
}

RowType RowTree::rowType() const
{
   return m_rowType;
}

PropertyType RowTree::propertyType() const
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
    }
}

bool RowTree::hasPropertyChildren()
{
    return !m_childRows.empty() && m_childRows.first()->rowType() == RowType::Property;
}

bool RowTree::handleButtonsClick(QGraphicsSceneMouseEvent *event)
{
    if (rowType() == RowType::Property)
        return false;

    QPointF p = mapFromScene(event->scenePos().x(), event->scenePos().y());

    if (m_rectArrow.contains(p.x(), p.y())) {
        m_expanded = !m_expanded;
        updateExpandStatus(m_expanded, true);
        update();
        return true;
    } else if (m_rectShy.contains(p.x(), p.y())) {
        m_shy = !m_shy;
        update();
        return true;
    } else if (m_rectVisible.contains(p.x(), p.y())) {
        m_visible = !m_visible;
        update();
        return true;
    } else if (m_rectLocked.contains(p.x(), p.y())) {
        m_locked = !m_locked;
        update();
        return true;
    }

    return false;
}

void RowTree::updateExpandStatus(bool expand, bool childrenOnly)
{
    if (!childrenOnly) {
        setVisible(expand);
        m_rowTimeline->setVisible(expand);
    }

    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows))
            child->updateExpandStatus(expand && child->parentRow()->m_expanded);
    }
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
    return  m_rowType == RowType::Scene
         || m_rowType == RowType::Layer
         || m_rowType == RowType::Group
         || m_rowType == RowType::Component;
}

bool RowTree::empty() const {
    return m_childRows.empty();
}

void RowTree::setMoveTarget(bool value) {
    m_moveTarget = value;
}

QList<RowTree *> RowTree::childRows() const {
    return m_childRows;
}

RowTimeline *RowTree::rowTimeline() const {
    return m_rowTimeline;
}

QString RowTree::label() const {
    return m_label;
}
