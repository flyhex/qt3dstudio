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
#include <QtCore/qstring.h>
#include <StudioObjectTypes.h>

class TimelineGraphicsScene;
class RowTree;
class RowTimeline;
class ITimelineItemBinding;
class Qt3DSDMTimelineItemBinding;

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)

class RowManager
{
public:
    RowManager(TimelineGraphicsScene *scene, QGraphicsLinearLayout *layoutLabels,
               QGraphicsLinearLayout *layoutTimeline);

    void recreateRowsFromBinding(ITimelineItemBinding *rootBinding);
    void clampIndex(int &idx);
    void correctIndex(int &idx);
    void selectRow(RowTree *row);
    void deleteRow(RowTree *row);
    void clearSelection();
    void updateFiltering(RowTree *rowTree = nullptr);
    void reorderPropertiesFromBinding(Qt3DSDMTimelineItemBinding *binding);
    bool hasProperties(RowTree *row);
    RowTree *createRowFromBinding(ITimelineItemBinding *binding, RowTree *parentRow = nullptr);
    RowTree *getOrCreatePropertyRow(RowTree *masterRow, const QString &propType);
    RowTree *createRow(EStudioObjectType rowType, RowTree *parentRow = nullptr,
                       const QString &label = QString(), const QString &propType = QString());
    RowTree *rowAt(int idx);
    RowTree *getRowAbove(RowTree *row);
    RowTree *selectedRow() const;
    RowTimeline *rowTimelineAt(int idx);

private:
    int getRowIndex(RowTree *row);
    int getLastChildIndex(RowTree *row);
    bool validIndex(int idx) const;
    void deleteRowRecursive(RowTree *row);
    void updateRowFilter(RowTree *row);
    void updateRowFilterRecursive(RowTree *row);
    void createRowsFromBindingRecursive(ITimelineItemBinding *binding,
                                        RowTree *parentRow = nullptr);
    void removeAllRows();

    RowTree *m_selectedRow = nullptr;
    TimelineGraphicsScene *m_scene;
    QGraphicsLinearLayout *m_layoutTree;
    QGraphicsLinearLayout *m_layoutTimeline;
};

#endif // ROWMANAGER_H
