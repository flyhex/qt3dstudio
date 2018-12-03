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
#include "StudioObjectTypes.h"
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>

class RowTree;
class TimelineGraphicsScene;

class RowMover : public TimelineItem
{
public:
    RowMover(TimelineGraphicsScene *m_scene);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    void start(const QVector<RowTree *> &rows);
    void end(bool force = false);
    void updateTargetRow(const QPointF &scenePos, EStudioObjectType rowType = OBJTYPE_UNKNOWN,
                         Q3DStudio::DocumentEditorFileType::Enum fileType
                                                    = Q3DStudio::DocumentEditorFileType::Unknown);
    bool isActive() const;
    RowTree *insertionTarget() const;
    RowTree *insertionParent() const;
    QVector<RowTree *> sourceRows() const;
    void removeSourceRow(RowTree *row);
    bool shouldDeleteAfterMove() const;
    int type() const;
    Q3DStudio::DocumentEditorInsertType::Enum insertionType() const;

private:
    void updateState(int depth, double y);
    void resetInsertionParent(RowTree *newParent = nullptr);
    bool isSourceRowsDescendant(RowTree *row) const;
    bool sourceRowsHasMaster() const;
    bool isNextSiblingRow(RowTree *r1, RowTree *r2) const;

    TimelineGraphicsScene *m_scene = nullptr;
    QPointer<RowTree> m_insertionTarget; // insertion target
    RowTree *m_insertionParent = nullptr; // insertion parent
    RowTree *m_rowAutoExpand = nullptr;
    QVector<RowTree *> m_sourceRows;      // dragged rows
    bool m_active = false;
    bool m_deleteAfterMove = false;
    Q3DStudio::DocumentEditorInsertType::Enum m_insertType =
            Q3DStudio::DocumentEditorInsertType::Unknown;
    QTimer m_autoExpandTimer;
    std::function<void()> updateTargetRowLater = {};
};

#endif // ROWMOVER_H
