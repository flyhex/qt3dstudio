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

#include "Separator.h"
#include "TimelineConstants.h"

#include <QtGui/qpainter.h>
#include <QtGui/qcursor.h>
#include <QtWidgets/qapplication.h>

Separator::Separator(TimelineItem *parent) : TimelineItem(parent)
{
    setMinimumWidth(TimelineConstants::SEPARATOR_W);
    setMaximumWidth(TimelineConstants::SEPARATOR_W);
    setMaximumHeight(10000);
    // TODO: remove
//    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAcceptHoverEvents(true);
}

void Separator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // TODO: remove
//    painter->fillRect(0, 0, size().width(), size().height(), QColor("#666666"));
}

void Separator::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    qApp->setOverrideCursor(Qt::SplitHCursor);
}

void Separator::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    qApp->changeOverrideCursor(Qt::ArrowCursor);
    qApp->restoreOverrideCursor();
}

int Separator::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeSeparator;
}
