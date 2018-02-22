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

class TimelineGraphicsScene;
class RowTree;
class RowTimeline;

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)

class RowManager
{
public:
    RowManager(TimelineGraphicsScene *scene, QGraphicsLinearLayout *layoutLabels,
               QGraphicsLinearLayout *layoutTimeline);

    void clampIndex(int &idx);
    void correctIndex(int &idx);
    void selectRow(RowTree *row);
    void deleteRow(RowTree *row);
    RowTree *getOrCreatePropertyRow(PropertyType propType, RowTree *masterRow);
    RowTree *createRow(RowType rowType, RowTree *parentRow = nullptr, const QString &label = {},
                       PropertyType propType = PropertyType::None);
    RowTree *rowAt(int idx);
    RowTree *getRowAbove(RowTree *row);
    RowTimeline *rowTimelineAt(int idx);

    RowTree *selectedRow() const;

private:
    int getRowIndex(RowTree *row);
    bool validIndex(int idx) const;
    void deleteRowRecursive(RowTree *row);

    RowTree *m_selectedRow = nullptr;
    TimelineGraphicsScene *m_scene;
    QGraphicsLinearLayout *m_layoutLabels;
    QGraphicsLinearLayout *m_layoutTimeline;
};

#endif // ROWMANAGER_H
