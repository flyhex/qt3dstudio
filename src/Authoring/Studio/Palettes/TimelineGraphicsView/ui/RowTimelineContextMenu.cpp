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

#include "RowTimelineContextMenu.h"
#include "RowTree.h"
#include "Keyframe.h"
#include "KeyframeManager.h"
#include "MainFrm.h"
#include "StudioApp.h"
#include "TimelineControl.h"
#include "Bindings/ITimelineItemBinding.h"

RowTimelineContextMenu::RowTimelineContextMenu(RowTree *inRowTree,
                                               KeyframeManager *inKeyframeManager,
                                               QGraphicsSceneContextMenuEvent *inEvent,
                                               TimelineControl *timelineControl,
                                               QWidget *parent)
    : QMenu(parent)
    , m_rowTree(inRowTree)
    , m_keyframeManager(inKeyframeManager)
    , m_menuEvent(inEvent)
    , m_timelineControl(timelineControl)
{
    initialize();
}

RowTimelineContextMenu::~RowTimelineContextMenu()
{
}

void RowTimelineContextMenu::initialize()
{
    m_insertKeyframeAction = new QAction(tr("Insert Keyframe"), this);
    m_insertKeyframeAction->setShortcut(Qt::Key_S);
    m_insertKeyframeAction->setShortcutVisibleInContextMenu(true);
    connect(m_insertKeyframeAction, &QAction::triggered, this,
            &RowTimelineContextMenu::insertKeyframe);
    addAction(m_insertKeyframeAction);

    m_cutSelectedKeyframesAction = new QAction(tr("Cut Selected Keyframe"), this);
    m_cutSelectedKeyframesAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_X));
    m_cutSelectedKeyframesAction->setShortcutVisibleInContextMenu(true);
    connect(m_cutSelectedKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::cutSelectedKeyframes);
    addAction(m_cutSelectedKeyframesAction);

    m_copySelectedKeyframesAction = new QAction(tr("Copy Selected Keyframe"), this);
    m_copySelectedKeyframesAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_C));
    m_copySelectedKeyframesAction->setShortcutVisibleInContextMenu(true);
    connect(m_copySelectedKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::copySelectedKeyframes);
    addAction(m_copySelectedKeyframesAction);

    m_pasteKeyframesAction = new QAction(tr("Paste Keyframes"), this);
    m_pasteKeyframesAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_V));
    m_pasteKeyframesAction->setShortcutVisibleInContextMenu(true);
    connect(m_pasteKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::pasteKeyframes);
    addAction(m_pasteKeyframesAction);

    m_deleteSelectedKeyframesAction = new QAction(tr("Delete Selected Keyframe"), this);
    m_deleteSelectedKeyframesAction->setShortcut(Qt::Key_Delete);
    m_deleteSelectedKeyframesAction->setShortcutVisibleInContextMenu(true);
    connect(m_deleteSelectedKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::deleteSelectedKeyframes);
    addAction(m_deleteSelectedKeyframesAction);

    m_deleteRowKeyframesAction = new QAction(tr("Delete All Channel Keyframes"), this);
    m_deleteRowKeyframesAction->setShortcut(
                QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_K));
    m_deleteRowKeyframesAction->setShortcutVisibleInContextMenu(true);
    connect(m_deleteRowKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::deleteRowKeyframes);
    addAction(m_deleteRowKeyframesAction);

    m_keyframe = m_rowTree->rowTimeline()->getClickedKeyframe(m_menuEvent->scenePos());
    bool ctrlPressed = m_menuEvent->modifiers() & Qt::ControlModifier;
    if (m_keyframe) {
        if (!m_keyframe->selected() && !ctrlPressed)
            m_keyframeManager->deselectAllKeyframes();

        m_keyframeManager->selectKeyframe(m_keyframe);
    } else {
        m_keyframeManager->deselectAllKeyframes();
    }

    if (m_rowTree->rowTimeline()->keyframes().size()) {
        m_hasDynamicKeyframes = m_keyframeManager->hasDynamicKeyframes(m_rowTree);
        QString label;
        if (m_hasDynamicKeyframes)
            label = tr("Make Animations Static");
        else
            label = tr("Make Animations Dynamic");

        m_dynamicKeyframesAction = new QAction(label, this);
        connect(m_dynamicKeyframesAction, &QAction::triggered, this,
                &RowTimelineContextMenu::toggleDynamicKeyframes);
        addAction(m_dynamicKeyframesAction);
    }

    addSeparator();

    if (m_keyframe) {
        m_setInterpolationAction = new QAction(tr("Set Interpolation..."), this);
        m_setInterpolationAction->setShortcut(Qt::Key_I);
        m_setInterpolationAction->setShortcutVisibleInContextMenu(true);
        connect(m_setInterpolationAction, &QAction::triggered, this,
                &RowTimelineContextMenu::setInterpolation);
        addAction(m_setInterpolationAction);

        m_setKeyframeTimeAction = new QAction(tr("Set Keyframe Time..."), this);
        connect(m_setKeyframeTimeAction, &QAction::triggered, this,
                &RowTimelineContextMenu::setKeyframeTime);
        addAction(m_setKeyframeTimeAction);
    } else {
        m_setTimeBarColorAction = new QAction(tr("Change Time Bar Color..."), this);
        connect(m_setTimeBarColorAction, &QAction::triggered, this,
                &RowTimelineContextMenu::changeTimeBarColor);
        addAction(m_setTimeBarColorAction);

        m_setTimeBarTimeAction = new QAction(tr("Set Time Bar Time..."), this);
        m_setTimeBarTimeAction->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_T));
        m_setTimeBarTimeAction->setShortcutVisibleInContextMenu(true);
        connect(m_setTimeBarTimeAction, &QAction::triggered, this,
                &RowTimelineContextMenu::setTimeBarTime);
        addAction(m_setTimeBarTimeAction);
    }
}

void RowTimelineContextMenu::showEvent(QShowEvent *event)
{
    bool propRow = m_rowTree->isProperty();
    bool hasPropRows = m_rowTree->hasPropertyChildren();

    m_insertKeyframeAction->setEnabled(!m_keyframe && (propRow || hasPropRows));
    m_cutSelectedKeyframesAction->setEnabled(m_keyframeManager->oneMasterRowSelected());
    m_copySelectedKeyframesAction->setEnabled(m_keyframeManager->oneMasterRowSelected());
    m_pasteKeyframesAction->setEnabled(m_keyframeManager->hasCopiedKeyframes());
    m_deleteSelectedKeyframesAction->setEnabled(m_keyframeManager->hasSelectedKeyframes());
    m_deleteRowKeyframesAction->setEnabled(!m_rowTree->rowTimeline()->keyframes().empty());
    if (!m_keyframe) {
        m_setTimeBarColorAction->setEnabled(m_rowTree->hasDurationBar());
        m_setTimeBarTimeAction->setEnabled(m_rowTree->hasDurationBar());
    }

    QMenu::showEvent(event);
}

void RowTimelineContextMenu::insertKeyframe()
{
    RowTree *destinationRowTree = nullptr;
    if (m_rowTree->isProperty()) {
        // When inserting into a property, insert actually into
        // its parent rowtree
        destinationRowTree = m_rowTree->parentRow();
    } else {
        destinationRowTree = m_rowTree;
    }

    destinationRowTree->getBinding()->InsertKeyframe();
}

void RowTimelineContextMenu::cutSelectedKeyframes()
{
    m_keyframeManager->copySelectedKeyframes();
    m_keyframeManager->deleteSelectedKeyframes();
}

void RowTimelineContextMenu::copySelectedKeyframes()
{
    m_keyframeManager->copySelectedKeyframes();
}

void RowTimelineContextMenu::pasteKeyframes()
{
    m_keyframeManager->pasteKeyframes();
}

void RowTimelineContextMenu::deleteSelectedKeyframes()
{
    m_keyframeManager->deleteSelectedKeyframes();
}

void RowTimelineContextMenu::deleteRowKeyframes()
{
    RowTree *destinationRowTree = nullptr;
    if (m_rowTree->isProperty()) {
        // Can't delete nicely just from property, so get the actual object row
        destinationRowTree = m_rowTree->parentRow();
    } else {
        destinationRowTree = m_rowTree;
    }
    destinationRowTree->getBinding()->DeleteAllChannelKeyframes();
}

void RowTimelineContextMenu::setInterpolation()
{
    m_keyframeManager->SetKeyframeInterpolation();
}

void RowTimelineContextMenu::setKeyframeTime()
{
    m_keyframeManager->SetKeyframeTime(m_keyframe->time * 1000.0);
}

void RowTimelineContextMenu::changeTimeBarColor()
{
    g_StudioApp.m_pMainWnd->OnTimelineSetTimeBarColor();
}

void RowTimelineContextMenu::setTimeBarTime()
{
    if (m_timelineControl) {
        m_timelineControl->setRowTimeline(m_rowTree->rowTimeline());
        m_timelineControl->showDurationEditDialog();
    }
}

void RowTimelineContextMenu::toggleDynamicKeyframes()
{
    QList<Keyframe *> selectedKeyframes = m_keyframeManager->selectedKeyframes();

    if (selectedKeyframes.isEmpty()) {
        // If property row is clicked, only make that property's first keyframe dynamic.
        // Otherwise make all properties' first keyframes dynamic
        // Note that it doesn't matter which keyframe we make dynamic, as the dynamic keyframe will
        // automatically change to the first one in time order.
        QList<Keyframe *> keyframes;
        if (m_rowTree->isProperty()) {
            keyframes.append(m_rowTree->rowTimeline()->keyframes().first());
        } else {
            const auto childProps = m_rowTree->childProps();
            for (const auto prop : childProps)
                keyframes.append(prop->rowTimeline()->keyframes().first());
        }
        m_keyframeManager->selectKeyframes(keyframes);
    }

    m_keyframeManager->SetKeyframesDynamic(!m_hasDynamicKeyframes);

    if (selectedKeyframes.isEmpty())
        m_keyframeManager->deselectAllKeyframes();
}
