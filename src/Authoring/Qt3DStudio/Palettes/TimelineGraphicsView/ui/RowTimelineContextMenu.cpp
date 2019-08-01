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
#include "Core.h"
#include "Doc.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMStudioSystem.h"
#include "SlideSystem.h"
#include "StudioPreferences.h"
#include "TimelineControl.h"
#include "Bindings/ITimelineItemBinding.h"
#include "TimelineGraphicsScene.h"
#include "TimelineToolbar.h"

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
        m_animType = addMenu(tr("Animation type"));
        QAction *actionLinear = m_animType->addAction(tr("Linear"));
        QAction *actionEase = m_animType->addAction(tr("Ease In/Out"));
        QAction *actionBezier = m_animType->addAction(tr("Bezier"));
        actionLinear->setData(qt3dsdm::EAnimationTypeLinear);
        actionLinear->setCheckable(true);
        actionEase->setData(qt3dsdm::EAnimationTypeEaseInOut);
        actionEase->setCheckable(true);
        actionBezier->setData(qt3dsdm::EAnimationTypeBezier);
        actionBezier->setCheckable(true);
        connect(m_animType, &QMenu::triggered, this, &RowTimelineContextMenu::onAnimTypeChange);
        if (m_rowTree->isProperty()) {
            qt3dsdm::EAnimationType animaType = m_rowTree->propBinding()->animationType();
            if (animaType == qt3dsdm::EAnimationTypeLinear)
                actionLinear->setChecked(true);
            else if (animaType == qt3dsdm::EAnimationTypeEaseInOut)
                actionEase->setChecked(true);
            else if (animaType == qt3dsdm::EAnimationTypeBezier)
                actionBezier->setChecked(true);
        }
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

        QAction *showRowTextsAction
                = m_rowTree->m_scene->widgetTimeline()->toolbar()->actionShowRowTexts();
        showRowTextsAction->setShortcutVisibleInContextMenu(true);
        addAction(showRowTextsAction);
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
    if (m_keyframe) {
        RowTree *row = propRow ? m_rowTree : m_keyframe->rowProperty->rowTree();
        qt3dsdm::EAnimationType animaType = row->propBinding()->animationType();
        m_setInterpolationAction->setEnabled(animaType == qt3dsdm::EAnimationTypeEaseInOut);
    } else {
        m_animType->setEnabled(propRow);
        m_setTimeBarColorAction->setEnabled(m_rowTree->hasDurationBar());
        m_setTimeBarTimeAction->setEnabled(m_rowTree->hasDurationBar());
    }

    QMenu::showEvent(event);
}

void RowTimelineContextMenu::onAnimTypeChange(QAction *action)
{
    if (!action->isChecked())
        return;

    using namespace qt3dsdm;

    // m_rowTree in this method is guaranteed to be a property row

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    IAnimationCore *animCore = doc->GetAnimationCore();
    ISlideSystem *slideSys = doc->GetStudioSystem()->GetSlideSystem();

    Q3DStudio::ScopedDocumentEditor editor(*doc, tr("Set Animation Type"), __FILE__, __LINE__);

    EAnimationType animType = EAnimationType(action->data().toInt());
    Qt3DSDMInstanceHandle instance = m_rowTree->parentRow()->instance();
    Qt3DSDMPropertyHandle property = m_rowTree->propBinding()->getPropertyHandle();
    Qt3DSDMSlideHandle slide = slideSys->GetAssociatedSlide(instance);
    TCharStr propType = doc->GetStudioSystem()->GetPropertySystem()->GetName(property);

    std::vector<qt3dsdm::Qt3DSDMAnimationHandle> animHandles
            = m_rowTree->propBinding()->animationHandles();
    for (size_t i = 0; i < animHandles.size(); ++i) {
        qt3dsdm::TKeyframeHandleList keyframeHandles;
        animCore->GetKeyframes(animHandles[i], keyframeHandles);
        if (animType == EAnimationTypeLinear) {
            QVector<SLinearKeyframe> keyframes;
            for (Qt3DSDMKeyframeHandle kfHandle : keyframeHandles) {
                TKeyframe kfData = animCore->GetKeyframeData(kfHandle);
                keyframes.append(SLinearKeyframe(KeyframeTime(kfData),
                                                 KeyframeValueValue(kfData)));
            }
            long numFloatsPerKeyframe = sizeof(SLinearKeyframe) / sizeof(float);
            long numValues = long(keyframeHandles.size()) * numFloatsPerKeyframe;
            editor->CreateOrSetAnimation(slide, instance, propType.wide_str(), long(i),
                                         animType,
                                         reinterpret_cast<float *>(keyframes.begin()),
                                         numValues);
        } else if (animType == EAnimationTypeEaseInOut) {
            float easeIn = CStudioPreferences::GetInterpolation() ? 100 : 0;
            float easeOut = CStudioPreferences::GetInterpolation() ? 100 : 0;
            QVector<SEaseInEaseOutKeyframe> keyframes;
            for (Qt3DSDMKeyframeHandle kfHandle : keyframeHandles) {
                TKeyframe kfData = animCore->GetKeyframeData(kfHandle);
                keyframes.append(SEaseInEaseOutKeyframe(KeyframeTime(kfData),
                                                        KeyframeValueValue(kfData),
                                                        easeIn, easeOut));
            }
            long numFloatsPerKeyframe = sizeof(SEaseInEaseOutKeyframe) / sizeof(float);
            long numValues = long(keyframeHandles.size()) * numFloatsPerKeyframe;
            editor->CreateOrSetAnimation(slide, instance, propType.wide_str(), long(i),
                                         animType,
                                         reinterpret_cast<float *>(keyframes.begin()),
                                         numValues);
        } else if (animType == EAnimationTypeBezier) {
            QVector<SBezierKeyframe> keyframes;
            for (Qt3DSDMKeyframeHandle kfHandle : keyframeHandles) {
                TKeyframe kfData = animCore->GetKeyframeData(kfHandle);
                float kfTime = KeyframeTime(kfData);
                float kfValue = KeyframeValueValue(kfData);
                keyframes.append(SBezierKeyframe(kfTime, kfValue,
                                                 kfTime - .5f, kfValue,
                                                 kfTime + .5f, kfValue));
            }
            long numFloatsPerKeyframe = sizeof(SBezierKeyframe) / sizeof(float);
            long numValues = long(keyframeHandles.size()) * numFloatsPerKeyframe;
            editor->CreateOrSetAnimation(slide, instance, propType.wide_str(), long(i),
                                         animType,
                                         reinterpret_cast<float *>(keyframes.begin()),
                                         numValues);
        }
    }
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
    m_keyframeManager->SetKeyframeTime(m_keyframe->time);
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
