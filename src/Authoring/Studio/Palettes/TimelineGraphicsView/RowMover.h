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
#include "TimelineItem.h"
#include "DocumentEditorEnumerations.h"

class RowTree;
class TimelineGraphicsScene;

class RowMover : public TimelineItem
{
public:
    RowMover(TimelineGraphicsScene *m_scene);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    void start(RowTree *row = nullptr);
    void end(bool force = false);
    void updateTargetRow(const QPointF &scenePos);
    bool isActive();
    bool validLayerMove(RowTree *rowAtIndex, RowTree *nextRowAtIndex);
    RowTree *insertionTarget() const;
    RowTree *insertionParent() const;
    RowTree *sourceRow() const;
    int type() const;
    Q3DStudio::DocumentEditorInsertType::Enum insertionType() const;

private:
    void updateState(int depth, double y);
    void resetInsertionParent(RowTree *newParent = nullptr);
    RowTree *getRowAtPos(const QPointF &scenePos);

    TimelineGraphicsScene *m_scene = nullptr;
    RowTree *m_insertionTarget = nullptr; // insertion target
    RowTree *m_insertionParent = nullptr; // insertion parent
    RowTree *m_sourceRow = nullptr;       // dragged row
    bool m_active = false;
    Q3DStudio::DocumentEditorInsertType::Enum m_insertType =
            Q3DStudio::DocumentEditorInsertType::Unknown;
};

#endif // ROWMOVER_H
