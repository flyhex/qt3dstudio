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

#ifndef ROWMOVER_H
#define ROWMOVER_H

#include "TimelineConstants.h"

#include <QtWidgets/qgraphicsitem.h>

class RowTree;

class RowMover : public QGraphicsRectItem
{
public:
    RowMover();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    void start(RowTree *row);
    void end();
    void resetInsertionParent(RowTree *newTarget = nullptr);
    void updateState(int index, int depth, int rawIndex);

    RowTree *insertionParent() const;
    RowTree *sourceRow() const;

    int targetIndex() const;
    bool isActive();
    bool isValidMove(int index, RowTree *rowAtIndex);

private:
    RowTree *m_insertionParent = nullptr; // insertion parent
    RowTree *m_sourceRow = nullptr;       // dragged row
    int m_targetIndex = -1;               // insertion index
    bool m_active = false;
};

#endif // ROWMOVER_H
