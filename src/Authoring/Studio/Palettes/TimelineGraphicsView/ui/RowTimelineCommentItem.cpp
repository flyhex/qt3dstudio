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

#include "RowTimelineCommentItem.h"
#include "TimelineConstants.h"
#include "TimelineItem.h"
#include "RowTree.h"
#include "StudioPreferences.h"

#include <QtWidgets/qstyleoption.h>
#include <QtGui/qevent.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextoption.h>
#include <QtGui/qtextdocument.h>

static const int MAX_COMMENT_SIZE = 2000; // Should be enough

RowTimelineCommentItem::RowTimelineCommentItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent)
    , m_acceptOnFocusOut(true)
{
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setTextWidth(MAX_COMMENT_SIZE);
    setDefaultTextColor(CStudioPreferences::textColor());
    setVisible(false);
}

QString RowTimelineCommentItem::label() const
{
    return m_label;
}

void RowTimelineCommentItem::setLabel(const QString &label)
{
    setPlainText(label);
    if (m_label != label) {
        m_label = label;
        emit labelChanged(m_label);
    }
}

RowTree *RowTimelineCommentItem::parentRow() const
{
    return m_rowTree;
}

void RowTimelineCommentItem::setParentRow(RowTree *row)
{
    m_rowTree = row;
}

int RowTimelineCommentItem::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TimelineItem::TypeRowTimelineCommentItem;
}

void RowTimelineCommentItem::paint(QPainter *painter,
                                   const QStyleOptionGraphicsItem *option,
                                   QWidget *widget)
{
    // prevents flickering when the row is just inserted to the layout
    if (m_rowTree && !m_rowTree->y())
        return;

    // Paint background
    QRectF r = boundingRect();
    r.adjust(-TimelineConstants::RULER_EDGE_OFFSET,
             TimelineConstants::ROW_TEXT_OFFSET_Y, 0,
             TimelineConstants::ROW_TEXT_OFFSET_Y);
    painter->fillRect(r, CStudioPreferences::timelineRowCommentBgColor());

    // Remove the HasFocus style state, to prevent the dotted line from being drawn.
    QStyleOptionGraphicsItem *style = const_cast<QStyleOptionGraphicsItem *>(option);
    style->state &= ~QStyle::State_HasFocus;

    QGraphicsTextItem::paint(painter, option, widget);
}

void RowTimelineCommentItem::focusOutEvent(QFocusEvent *event)
{
    if (m_acceptOnFocusOut)
        validateLabel();
    else
        setPlainText(m_label);

    // Remove possible selection
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
    QGraphicsTextItem::focusOutEvent(event);
    // Next time default to accepting
    m_acceptOnFocusOut = true;
}

void RowTimelineCommentItem::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        m_acceptOnFocusOut = true;
        clearFocus();
        event->accept();
        return;
    } else if (key == Qt::Key_Escape) {
        m_acceptOnFocusOut = false;
        clearFocus();
        event->accept();
        return;
    }

    QGraphicsTextItem::keyPressEvent(event);
}

QRectF RowTimelineCommentItem::boundingRect() const
{
    return QRectF(0, 0,  parentItem()->boundingRect().width(),
                  TimelineConstants::ROW_H);
}

void RowTimelineCommentItem::validateLabel()
{
    QString text = toPlainText().trimmed();
    text = text.left(MAX_COMMENT_SIZE);
    setLabel(text);
}
