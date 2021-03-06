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

#ifndef ROWMANAGER_H
#define ROWMANAGER_H

#include "RowTypes.h"
#include "StudioObjectTypes.h"
#include <QtCore/qstring.h>

class TimelineGraphicsScene;
class RowTree;
class RowTimeline;
class RowTimelinePropertyGraph;
class ITimelineItemBinding;
class Qt3DSDMTimelineItemBinding;

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)

class RowManager
{
public:
    RowManager(TimelineGraphicsScene *scene, QGraphicsLinearLayout *layoutLabels,
               QGraphicsLinearLayout *layoutTimeline);
    ~RowManager();

    void selectRow(RowTree *row, bool multiSelect = false);
    void setRowSelection(RowTree *row, bool selected);
    void deleteRow(RowTree *row);
    void finalizeRowDeletions();
    void clearSelection();
    void updateFiltering(RowTree *rowTree = nullptr);
    void recreateRowsFromBinding(ITimelineItemBinding *rootBinding);
    void updateRulerDuration(bool updateMaxDuration = true);
    bool isSingleSelected() const;
    RowTree *createRowFromBinding(ITimelineItemBinding *binding, RowTree *parentRow = nullptr,
                                  int index = -1);
    RowTree *getOrCreatePropertyRow(RowTree *masterRow, const QString &propType, int index = -1);
    RowTree *createRow(EStudioObjectType rowType, RowTree *parentRow = nullptr,
                       const QString &label = QString(), const QString &propType = QString(),
                       int index = -1);
    RowTree *getRowAtPos(const QPointF &scenePos) const;
    RowTree *selectedRow() const;
    bool isComponentRoot() const;
    bool isRowSelected(RowTree *row) const;
    QVector<RowTree *> selectedRows() const;
    void ensureRowExpandedAndVisible(RowTree *row, bool forceChildUpdate) const;
    QVector<RowTimelinePropertyGraph *> getExpandedPropertyGraphs() const;

private:
    void deleteRowRecursive(RowTree *row, bool deferChildRows);
    void updateRowFilterRecursive(RowTree *row);
    void createRowsFromBindingRecursive(ITimelineItemBinding *binding,
                                        RowTree *parentRow = nullptr);
    void removeAllRows();

    QVector<RowTree *> m_selectedRows;
    TimelineGraphicsScene *m_scene;
    QGraphicsLinearLayout *m_layoutTree;
    QGraphicsLinearLayout *m_layoutTimeline;
    QVector<RowTree *> m_deletedRows;
};

#endif // ROWMANAGER_H
