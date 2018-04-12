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
#include "Bindings/ITimelineItemBinding.h"

RowTimelineContextMenu::RowTimelineContextMenu(RowTree *inRowTree,
                                               KeyframeManager *inKeyframeManager,
                                               QGraphicsSceneContextMenuEvent *inEvent,
                                               QWidget *parent)
    : QMenu(parent)
    , m_rowTree(inRowTree)
    , m_keyframeManager(inKeyframeManager)
    , m_menuEvent(inEvent)
{
    initialize();
}

RowTimelineContextMenu::~RowTimelineContextMenu()
{
}

void RowTimelineContextMenu::initialize()
{
    m_insertKeyframeAction = new QAction(tr("Insert Keyframe"), this);
    connect(m_insertKeyframeAction, &QAction::triggered, this,
            &RowTimelineContextMenu::insertKeyframe);
    addAction(m_insertKeyframeAction);

    m_cutSelectedKeyframesAction = new QAction(tr("Cut Selected Keyframe"), this);
    connect(m_cutSelectedKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::cutSelectedKeyframes);
    addAction(m_cutSelectedKeyframesAction);

    m_copySelectedKeyframesAction = new QAction(tr("Copy Selected Keyframe"), this);
    connect(m_copySelectedKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::copySelectedKeyframes);
    addAction(m_copySelectedKeyframesAction);

    m_pasteKeyframesAction = new QAction(tr("Paste Keyframes"), this);
    connect(m_pasteKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::pasteKeyframes);
    addAction(m_pasteKeyframesAction);

    m_deleteSelectedKeyframesAction = new QAction(tr("Delete Selected Keyframe"), this);
    connect(m_deleteSelectedKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::deleteSelectedKeyframes);
    addAction(m_deleteSelectedKeyframesAction);

    m_deleteRowKeyframesAction = new QAction(
                m_rowTree->isProperty() ? tr("Delete All Property Keyframes")
                                        : tr("Delete All Channel Keyframes"), this);

    connect(m_deleteRowKeyframesAction, &QAction::triggered, this,
            &RowTimelineContextMenu::deleteRowKeyframes);
    addAction(m_deleteRowKeyframesAction);
}

void RowTimelineContextMenu::showEvent(QShowEvent *event)
{
    Keyframe *keyframe = m_rowTree->rowTimeline()->getClickedKeyframe(m_menuEvent->scenePos());
    bool ctrlPressed = m_menuEvent->modifiers() & Qt::ControlModifier;
    if (keyframe) {
        if (!keyframe->selected && !ctrlPressed)
            m_keyframeManager->deselectAllKeyframes();

        m_keyframeManager->selectKeyframe(keyframe);
    } else {
        m_keyframeManager->deselectAllKeyframes();
    }

    bool propRow = m_rowTree->isProperty();
    bool hasPropRows = m_rowTree->hasPropertyChildren();

    m_insertKeyframeAction->setEnabled(!keyframe && (propRow || hasPropRows));
    m_cutSelectedKeyframesAction->setEnabled(m_keyframeManager->oneMasterRowSelected());
    m_copySelectedKeyframesAction->setEnabled(m_keyframeManager->oneMasterRowSelected());
    m_pasteKeyframesAction->setEnabled(m_keyframeManager->hasCopiedKeyframes());
    m_deleteSelectedKeyframesAction->setEnabled(m_keyframeManager->hasSelectedKeyframes());
    m_deleteRowKeyframesAction->setEnabled(!m_rowTree->rowTimeline()->keyframes().empty());

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

    // update and wire the UI from binding
    for (long i = 0; i < destinationRowTree->getBinding()->GetPropertyCount(); ++i) {
        ITimelineItemProperty *prop_i = destinationRowTree->getBinding()->GetProperty(i);
        for (long j = 0; j < prop_i->GetKeyframeCount(); ++j) {
            Qt3DSDMTimelineKeyframe *kf =
                    static_cast<Qt3DSDMTimelineKeyframe *>(prop_i->GetKeyframeByIndex(j));

            if (kf->getUI() == nullptr) { // newly added keyframe
                Keyframe *kfUI = m_keyframeManager->insertKeyframe(prop_i->getRowTree()
                                 ->rowTimeline(), static_cast<double>(kf->GetTime()) * .001,
                                 false).at(0);
                // wire the keyframe UI and binding
                kf->setUI(kfUI);
                kfUI->binding = kf;
            }
        }
    }
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
    m_keyframeManager->pasteKeyframes(m_rowTree->rowTimeline());
}

void RowTimelineContextMenu::deleteSelectedKeyframes()
{
    m_keyframeManager->deleteSelectedKeyframes();
}

void RowTimelineContextMenu::deleteRowKeyframes()
{
    m_keyframeManager->deleteKeyframes(m_rowTree->rowTimeline());
}
