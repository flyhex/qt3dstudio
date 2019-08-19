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

#include "TreeHeaderView.h"

TreeHeaderView::TreeHeaderView(QWidget *parent)
    : QGraphicsView(parent)
{
}

/**
 * Overridden to ignore scrolling after initial show related scrolling has been finished
 *
 * When RowTreeLabel (QGraphicsTextItem) gets focus for text editing, it forces views to scroll
 * themselves so that editable text item is always visible. We don't want tree header view to move.
 * @see QGraphicsTextItemPrivate::textControl() and _q_ensureVisible()
 */
void TreeHeaderView::scrollContentsBy(int dx, int dy)
{
    if (m_allowScrolling)
        QGraphicsView::scrollContentsBy(dx, dy);
}
