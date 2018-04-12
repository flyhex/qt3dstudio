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

#ifndef ROWTIMELINECONTEXTMENU_H
#define ROWTIMELINECONTEXTMENU_H

#include <QtWidgets/qmenu.h>
#include <QtWidgets/qaction.h>
#include <QtWidgets/qgraphicssceneevent.h>

class RowTree;
class KeyframeManager;

class RowTimelineContextMenu : public QMenu
{
    Q_OBJECT
public:
    explicit RowTimelineContextMenu(RowTree *inRowTree,
                                    KeyframeManager *inKeyframeManager,
                                    QGraphicsSceneContextMenuEvent *inEvent,
                                    QWidget *parent = nullptr);
    virtual ~RowTimelineContextMenu();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void initialize();
    void insertKeyframe();
    void cutSelectedKeyframes();
    void copySelectedKeyframes();
    void pasteKeyframes();
    void deleteSelectedKeyframes();
    void deleteRowKeyframes();

    RowTree *m_rowTree = nullptr;
    KeyframeManager *m_keyframeManager = nullptr;
    QGraphicsSceneContextMenuEvent *m_menuEvent = nullptr;
    QAction *m_insertKeyframeAction = nullptr;
    QAction *m_cutSelectedKeyframesAction = nullptr;
    QAction *m_copySelectedKeyframesAction = nullptr;
    QAction *m_pasteKeyframesAction = nullptr;
    QAction *m_deleteSelectedKeyframesAction = nullptr;
    QAction *m_deleteRowKeyframesAction = nullptr;
};

#endif // ROWTIMELINECONTEXTMENU_H
