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

#include "TreeHeader.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"

#include <QtGui/qpainter.h>


TreeHeader::TreeHeader(TimelineGraphicsScene *timelineScene, TimelineItem *parent)
    : TimelineItem(parent)
    , m_scene(timelineScene)
{
    setAcceptHoverEvents(true);
}

void TreeHeader::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    bool hiResIcons = StudioUtils::devicePixelRatio(widget->window()->windowHandle()) > 1.0;

    double treeWidth = m_scene->treeWidth() - m_scene->getScrollbarOffsets().x();
    m_rectShy    .setRect(treeWidth - 16 * 3.3, size().height() * .5 - 8, 16, 16);
    m_rectVisible.setRect(treeWidth - 16 * 2.2, size().height() * .5 - 8, 16, 16);
    m_rectLock   .setRect(treeWidth - 16 * 1.1, size().height() * .5 - 8, 16, 16);

    static const QPixmap pixShy = QPixmap(":/images/Toggle-Shy.png");
    static const QPixmap pixShy2x = QPixmap(":/images/Toggle-Shy@2x.png");
    static const QPixmap pixVisible = QPixmap(":/images/Toggle-HideShow.png");
    static const QPixmap pixVisible2x = QPixmap(":/images/Toggle-HideShow@2x.png");
    static const QPixmap pixLock = QPixmap(":/images/Toggle-Lock.png");
    static const QPixmap pixLock2x = QPixmap(":/images/Toggle-Lock@2x.png");

    const QColor selectedColor = CStudioPreferences::timelineFilterButtonSelectedColor();
    const QColor hoveredColor = CStudioPreferences::timelineFilterButtonHoveredColor();

    if (m_shy)
        painter->fillRect(m_rectShy, selectedColor);

    if (m_visible)
        painter->fillRect(m_rectVisible, selectedColor);

    if (m_lock)
        painter->fillRect(m_rectLock, selectedColor);

    // Paint hovering as semi-transparent overlay
    if (m_hoveredItem == TreeControlType::Shy)
        painter->fillRect(m_rectShy, hoveredColor);
    else if (m_hoveredItem == TreeControlType::Hide)
        painter->fillRect(m_rectVisible, hoveredColor);
    else if (m_hoveredItem == TreeControlType::Lock)
        painter->fillRect(m_rectLock, hoveredColor);

    painter->drawPixmap(m_rectShy     , hiResIcons ? pixShy2x     : pixShy);
    painter->drawPixmap(m_rectVisible , hiResIcons ? pixVisible2x : pixVisible);
    painter->drawPixmap(m_rectLock    , hiResIcons ? pixLock2x    : pixLock);
}

TreeControlType TreeHeader::handleButtonsClick(const QPointF &scenePos)
{
    QPointF p = mapFromScene(scenePos.x(), scenePos.y());

    if (m_rectShy.contains(p.x(), p.y())) {
        toggleFilterShy();
        return TreeControlType::Shy;
    } else if (m_rectVisible.contains(p.x(), p.y())) {
        toggleFilterHidden();
        return TreeControlType::Hide;
    } else if (m_rectLock.contains(p.x(), p.y())) {
        toggleFilterLocked();
        return TreeControlType::Lock;
    }

    return TreeControlType::None;
}

bool TreeHeader::filterShy() const
{
    return m_shy;
}

bool TreeHeader::filterHidden() const
{
    return m_visible;
}

bool TreeHeader::filterLocked() const
{
    return m_lock;
}

int TreeHeader::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeTreeHeader;
}

void TreeHeader::toggleFilterShy()
{
    m_shy = !m_shy;
    update();
}

void TreeHeader::toggleFilterHidden()
{
    m_visible = !m_visible;
    update();
}

void TreeHeader::toggleFilterLocked()
{
    m_lock = !m_lock;
    update();
}

void TreeHeader::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QPointF p = event->scenePos();
    TreeControlType hoveredItem = TreeControlType::None;
    if (m_rectShy.contains(p.x(), p.y())) {
        QString action = m_shy ? tr("Show") : tr("Hide");
        setToolTip(tr("%1 shy objects").arg(action));
        hoveredItem = TreeControlType::Shy;
    } else if (m_rectVisible.contains(p.x(), p.y())) {
        QString action = m_visible ? tr("Show") : tr("Hide");
        setToolTip(tr("%1 inactive objects").arg(action));
        hoveredItem = TreeControlType::Hide;
    } else if (m_rectLock.contains(p.x(), p.y())) {
        QString action = m_lock ? tr("Show") : tr("Hide");
        setToolTip(tr("%1 locked objects").arg(action));
        hoveredItem = TreeControlType::Lock;
    } else {
        setToolTip("");
    }

    if (m_hoveredItem != hoveredItem) {
        // Update hover status only if it has changed
        m_hoveredItem = hoveredItem;
        update();
    }
}

void TreeHeader::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hoveredItem = TreeControlType::None;
    update();
}
