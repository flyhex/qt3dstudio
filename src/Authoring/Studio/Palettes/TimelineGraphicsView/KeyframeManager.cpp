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

#include "KeyframeManager.h"
#include "RowTree.h"
#include "RowTimeline.h"
#include "Keyframe.h"
#include "RowTypes.h"
#include "TimelineConstants.h"
#include "Ruler.h"
#include "PlayHead.h"
#include "RowManager.h"
#include "TimelineGraphicsScene.h"
#include "StudioObjectTypes.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "StudioClipboard.h"
#include "CmdDataModelRemoveKeyframe.h"
#include "CmdDataModelInsertKeyframe.h"
#include "CmdDataModelChangeKeyframe.h"
#include "Qt3DSDMAnimation.h"
#include "ClientDataModelBridge.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/OffsetKeyframesCommandHelper.h"
#include "Bindings/Qt3DSDMTimelineKeyframe.h"
#include "StudioPreferences.h"
#include "Qt3DSDMAnimation.h"
#include "Dialogs.h"
#include "TimeEditDlg.h"
#include "Bindings/PasteKeyframesCommandHelper.h"

#include <qglobal.h>
#include <QtCore/qhash.h>
#include <QtCore/qdebug.h>

using namespace qt3dsdm;

KeyframeManager::KeyframeManager(TimelineGraphicsScene *scene)
    : m_scene(scene)
    , m_pasteKeyframeCommandHelper(nullptr)
{
}

KeyframeManager::~KeyframeManager()
{
    delete m_pasteKeyframeCommandHelper;
}

QList<Keyframe *> KeyframeManager::insertKeyframe(RowTimeline *row, double time,
                                                  bool selectInsertedKeyframes)
{
    QList<Keyframe *> addedKeyframes;
    QList<RowTimeline *> propRows;
    if (!row->rowTree()->isProperty()) {
        const auto childRows = row->rowTree()->childRows();
        for (const auto r : childRows) {
            if (r->isProperty())
                propRows.append(r->rowTimeline());
        }
    } else {
        propRows.append(row);
    }

    if (!propRows.empty()) {
        Keyframe *keyframe = nullptr;
        for (const auto &r : qAsConst(propRows)) {
            keyframe = new Keyframe(time, r);
            r->insertKeyframe(keyframe);
            r->parentRow()->insertKeyframe(keyframe);
            addedKeyframes.append(keyframe);
        }

        if (selectInsertedKeyframes && !addedKeyframes.empty()) {
            deselectAllKeyframes();
            selectKeyframes(addedKeyframes);
        }
    }

    return addedKeyframes;
}

void KeyframeManager::selectKeyframe(Keyframe *keyframe)
{
    if (!m_selectedKeyframes.contains(keyframe)) {
        m_selectedKeyframes.append(keyframe);

        if (!m_selectedKeyframesMasterRows.contains(keyframe->rowMaster))
            m_selectedKeyframesMasterRows.append(keyframe->rowMaster);

        keyframe->binding->SetSelected(true);
        keyframe->rowMaster->putSelectedKeyframesOnTop();
        keyframe->rowMaster->updateKeyframes();
    }
}

void KeyframeManager::selectConnectedKeyframes(Keyframe *keyframe)
{
    // Select all keyframes of same master row at same time
    const auto keyframes = keyframe->rowMaster->keyframes();
    for (const auto k : keyframes) {
        if (k->time == keyframe->time)
            selectKeyframe(k);
    }
}

void KeyframeManager::selectKeyframes(const QList<Keyframe *> &keyframes)
{
    for (const auto keyframe : keyframes) {
        if (!m_selectedKeyframes.contains(keyframe)) {
            m_selectedKeyframes.append(keyframe);

            if (!m_selectedKeyframesMasterRows.contains(keyframe->rowMaster))
                m_selectedKeyframesMasterRows.append(keyframe->rowMaster);
        }
    }

    for (auto keyframe : qAsConst(m_selectedKeyframes))
        keyframe->binding->SetSelected(true);

    for (auto row : qAsConst(m_selectedKeyframesMasterRows)) {
        row->putSelectedKeyframesOnTop();
        row->updateKeyframes();
    }
}

QList<Keyframe *> KeyframeManager::selectedKeyframes() const
{
    return m_selectedKeyframes;
}

// update bindings after selected keyframes are moved
void KeyframeManager::commitMoveSelectedKeyframes()
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    COffsetKeyframesCommandHelper h(*theDoc);
    for (Keyframe *keyframe : qAsConst(m_selectedKeyframes))
        keyframe->binding->UpdateKeyframesTime(&h, keyframe->time * 1000);
}

void KeyframeManager::selectKeyframesInRect(const QRectF &rect)
{
    deselectAllKeyframes();

    RowTree *row = m_scene->rowManager()->getRowAtPos(QPointF(0, rect.top()));
    while (row && row->y() < rect.bottom()) {
        if (!row->locked()) {
            const auto keyframes = row->rowTimeline()->getKeyframesInRange(rect);
            for (auto keyframe : keyframes) {
                if (!m_selectedKeyframes.contains(keyframe)) {
                    m_selectedKeyframes.append(keyframe);

                    if (!m_selectedKeyframesMasterRows.contains(keyframe->rowMaster))
                        m_selectedKeyframesMasterRows.append(keyframe->rowMaster);
                }
            }
        }
        row = m_scene->rowManager()->getRowAtPos(QPointF(0, row->y() + row->size().height()));
    }

    for (auto keyframe : qAsConst(m_selectedKeyframes))
        keyframe->binding->SetSelected(true);

    for (auto row : qAsConst(m_selectedKeyframesMasterRows)) {
        row->putSelectedKeyframesOnTop();
        row->updateKeyframes();
    }
}

void KeyframeManager::deselectKeyframe(Keyframe *keyframe)
{
    if (m_selectedKeyframes.contains(keyframe)) {
        m_selectedKeyframes.removeAll(keyframe);
        keyframe->rowMaster->updateKeyframes();
        m_selectedKeyframesMasterRows.removeAll(keyframe->rowMaster);

        keyframe->binding->SetSelected(false);
        keyframe->rowMaster->putSelectedKeyframesOnTop();
    }
}

void KeyframeManager::deselectConnectedKeyframes(Keyframe *keyframe)
{
    // Deselect all keyframes of same master row at same time
    const auto keyframes = keyframe->rowMaster->keyframes();
    for (const auto k : keyframes) {
        if (k->time == keyframe->time)
            deselectKeyframe(k);
    }
}

void KeyframeManager::deselectAllKeyframes()
{
    for (auto keyframe : qAsConst(m_selectedKeyframes))
        keyframe->binding->SetSelected(false);

    for (auto row : qAsConst(m_selectedKeyframesMasterRows))
        row->updateKeyframes();

    m_selectedKeyframes.clear();
    m_selectedKeyframesMasterRows.clear();
}

void KeyframeManager::deselectRowKeyframes(RowTree *row)
{
    const QList<Keyframe *> keyframes = row->rowTimeline()->keyframes();
    for (const auto keyframe : keyframes) {
        if (row->isProperty())
            deselectKeyframe(keyframe);
        else
            deselectConnectedKeyframes(keyframe);
    }
}

bool KeyframeManager::deleteSelectedKeyframes()
{
    if (!m_selectedKeyframes.empty()) {
        CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
        CCmdDataModelRemoveKeyframe *cmd = new CCmdDataModelRemoveKeyframe(theDoc);
        for (auto keyframe : qAsConst(m_selectedKeyframes)) {
            cmd->addKeyframeHandles(keyframe->binding);

            keyframe->rowMaster->removeKeyframe(keyframe);
            keyframe->rowProperty->removeKeyframe(keyframe);

            delete keyframe;
        }

        for (auto row : qAsConst(m_selectedKeyframesMasterRows))
            row->updateKeyframes();

        m_selectedKeyframes.clear();
        m_selectedKeyframesMasterRows.clear();

        g_StudioApp.GetCore()->ExecuteCommand(cmd);
        return true;
    }

    return false;
}

// delete all keyframes on a row
void KeyframeManager::deleteKeyframes(RowTimeline *row, bool repaint)
{
    const auto keyframes = row->keyframes();
    for (auto keyframe : keyframes) {
        keyframe->rowMaster->removeKeyframe(keyframe);
        keyframe->rowProperty->removeKeyframe(keyframe);

        if (m_selectedKeyframes.contains(keyframe))
            m_selectedKeyframes.removeAll(keyframe);

        delete keyframe;
    }

    if (m_selectedKeyframesMasterRows.contains(row))
        m_selectedKeyframesMasterRows.removeAll(row);

    if (repaint)
        row->updateKeyframes();
}

void KeyframeManager::copySelectedKeyframes()
{
    if (!m_selectedKeyframes.empty() && m_selectedKeyframesMasterRows.count() == 1) {
        // Keyframe copying doesn't use clipboard, so clear it so that next time we paste
        // it will paste the keyframes rather than the last object we copied
        CStudioClipboard::ClearClipboard();

        if (m_pasteKeyframeCommandHelper)
            m_pasteKeyframeCommandHelper->Clear(); // clear out previously copied data
        else
            m_pasteKeyframeCommandHelper = new CPasteKeyframeCommandHelper();

        // calc min copied frames time
        double minTime = 999999.0; // in seconds (~277.78 hrs)
        for (auto keyframe : qAsConst(m_selectedKeyframes)) {
            if (keyframe->time < minTime)
                minTime = keyframe->time;
        }

        qt3dsdm::IAnimationCore *animationCore = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
                                                 ->GetAnimationCore();

        for (auto keyframe : qAsConst(m_selectedKeyframes)) {
            Qt3DSDMTimelineKeyframe *kf = keyframe->binding;
            Qt3DSDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
            kf->GetKeyframeHandles(theKeyframeHandles);
            qt3dsdm::SGetOrSetKeyframeInfo info[3];
            size_t infoCount = 0;
            if (!theKeyframeHandles.empty()) {
                switch (theKeyframeHandles.size()) {
                case 1:
                    info[0] = setKeyframeInfo(theKeyframeHandles[0], *animationCore);
                    infoCount = 1;
                    break;
                case 3:
                    info[0] = setKeyframeInfo(theKeyframeHandles[0], *animationCore);
                    info[1] = setKeyframeInfo(theKeyframeHandles[1], *animationCore);
                    info[2] = setKeyframeInfo(theKeyframeHandles[2], *animationCore);
                    infoCount = 3;
                    break;
                default:
                    break;
                }

                double dt = Qt3DSDMTimelineKeyframe::GetTimeInSecs(kf->GetTime()) - minTime;
                qt3dsdm::Qt3DSDMAnimationHandle animation =
                    animationCore->GetAnimationForKeyframe(theKeyframeHandles[0]);
                m_pasteKeyframeCommandHelper->AddKeyframeData(
                    animationCore->GetAnimationInfo(animation).m_Property, dt, info, infoCount);
            }
        }
    }
}

qt3dsdm::SGetOrSetKeyframeInfo KeyframeManager::setKeyframeInfo(
        qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe, qt3dsdm::IAnimationCore &inCore)
{
    qt3dsdm::TKeyframe theKeyframeData = inCore.GetKeyframeData(inKeyframe);
    qt3dsdm::SEaseInEaseOutKeyframe keyframe =
            qt3dsdm::get<qt3dsdm::SEaseInEaseOutKeyframe>(theKeyframeData);
    bool isDynamic = false;
    if (inCore.IsFirstKeyframe(inKeyframe)) {
        isDynamic = inCore.GetAnimationInfo(inCore.GetAnimationForKeyframe(inKeyframe))
                .m_DynamicFirstKeyframe;
    }

    return qt3dsdm::SGetOrSetKeyframeInfo(keyframe.m_KeyframeValue, keyframe.m_EaseIn,
                                          keyframe.m_EaseOut, isDynamic);
}

void KeyframeManager::pasteKeyframes()
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    if (m_pasteKeyframeCommandHelper && m_pasteKeyframeCommandHelper->HasCopiedKeyframes()) {
        qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = theDoc->GetSelectedInstance();
        if (theSelectedInstance.Valid()) {
            long theCurrentViewTimeInMilliseconds = theDoc->GetCurrentViewTime();
            CCmdDataModelInsertKeyframe *theInsertKeyframesCommand =
                m_pasteKeyframeCommandHelper->GetCommand(theDoc, theCurrentViewTimeInMilliseconds,
                                                         theSelectedInstance);
            if (theInsertKeyframesCommand)
                g_StudioApp.GetCore()->ExecuteCommand(theInsertKeyframesCommand);
        }
    }
}

void KeyframeManager::moveSelectedKeyframes(double dx)
{
    double dt = m_scene->ruler()->distanceToTime(dx);

    if (dt < 0) { // check min limit
        double minTime = 999999; // seconds (~277.78 hrs)
        for (auto keyframe : qAsConst(m_selectedKeyframes)) {
            if (keyframe->time < minTime)
                minTime = keyframe->time;
        }

        if (minTime + dt < 0)
            dt = -minTime;
    }

    for (auto keyframe : qAsConst(m_selectedKeyframes))
        keyframe->time += dt;

    for (auto row : qAsConst(m_selectedKeyframesMasterRows))
        row->updateKeyframes();
}

// selected keyframes belong to only one master row
bool KeyframeManager::oneMasterRowSelected() const
{
    return m_selectedKeyframesMasterRows.count() == 1;
}

bool KeyframeManager::hasSelectedKeyframes() const
{
    return !m_selectedKeyframes.empty();
}

bool KeyframeManager::hasCopiedKeyframes() const
{
    return m_pasteKeyframeCommandHelper &&
           m_pasteKeyframeCommandHelper->HasCopiedKeyframes();
}

bool KeyframeManager::hasDynamicKeyframes(RowTree *row) const
{
    const QList<Keyframe *> keyframes = row->rowTimeline()->keyframes();
    for (const auto keyframe : keyframes) {
        if (keyframe->binding->IsDynamic())
            return true;
    }
    return false;
}

// IKeyframesManager interface to connect Doc and KeyframeManager
// Mahmoud_TODO: rewrite a better interface for the new timeline
// ITimelineKeyframesManager interface
void KeyframeManager::SetKeyframeTime(long inTime)
{
    CTimeEditDlg theTimeEditDlg;
    theTimeEditDlg.setKeyframesManager(this);
    theTimeEditDlg.showDialog(inTime, g_StudioApp.GetCore()->GetDoc(), ASSETKEYFRAME);
}

void KeyframeManager::SetKeyframesDynamic(bool inDynamic)
{
    if (!hasSelectedKeyframes())
        return;

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    IAnimationCore *animationCore = doc->GetStudioSystem()->GetAnimationCore();
    CCmdDataModelChangeDynamicKeyframe *cmd = nullptr;

    for (int i = 0; i < m_selectedKeyframes.size(); ++i) {
        Qt3DSDMTimelineKeyframe *timelineKeyframe = m_selectedKeyframes[i]->binding;
        Qt3DSDMTimelineKeyframe::TKeyframeHandleList keyframeHandles;
        timelineKeyframe->GetKeyframeHandles(keyframeHandles);

        for (size_t keyIndex = 0; keyIndex < keyframeHandles.size(); ++keyIndex) {
            qt3dsdm::Qt3DSDMAnimationHandle animation(
                animationCore->GetAnimationForKeyframe(keyframeHandles.at(keyIndex)));
            if (!cmd)
                cmd = new CCmdDataModelChangeDynamicKeyframe(doc, animation, inDynamic);
            else
                cmd->AddHandle(animation);
        }
    }

    if (cmd)
        g_StudioApp.GetCore()->ExecuteCommand(cmd);
}

long KeyframeManager::OffsetSelectedKeyframes(long inOffset)
{
    double dx = m_scene->ruler()->timeToDistance(inOffset / 1000.0);
    moveSelectedKeyframes(dx);
    return 0;
}

bool KeyframeManager::CanMakeSelectedKeyframesDynamic()
{
    // Mahmoud_TODO: implement if needed
    return false;
}

void KeyframeManager::CommitChangedKeyframes()
{
    commitMoveSelectedKeyframes();
}

void KeyframeManager::RollbackChangedKeyframes()
{
    for (Keyframe *keyframe : qAsConst(m_selectedKeyframes))
        keyframe->time = keyframe->binding->GetTime() / 1000.0;

    for (auto row : qAsConst(m_selectedKeyframesMasterRows))
        row->updateKeyframes();
}

// IKeyframesManager interface
bool KeyframeManager::HasSelectedKeyframes(bool inOnlyDynamic)
{
    return hasSelectedKeyframes();
}

bool KeyframeManager::HasDynamicKeyframes()
{
    return false; // Mahmoud_TODO: implement
}

bool KeyframeManager::CanPerformKeyframeCopy()
{
    return !m_selectedKeyframes.empty() && m_selectedKeyframesMasterRows.count() == 1;
}

bool KeyframeManager::CanPerformKeyframePaste()
{
    if (m_pasteKeyframeCommandHelper && m_pasteKeyframeCommandHelper->HasCopiedKeyframes()) {
        qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance =
            g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance();
        if (theSelectedInstance.Valid())
            return true;
    }

    return false;
}

void KeyframeManager::CopyKeyframes()
{
    copySelectedKeyframes();
}

bool KeyframeManager::RemoveKeyframes(bool inPerformCopy)
{
    Q_UNUSED(inPerformCopy)

    return deleteSelectedKeyframes();
}

void KeyframeManager::PasteKeyframes()
{
    pasteKeyframes();
}

void KeyframeManager::SetKeyframeInterpolation()
{
    if (!hasSelectedKeyframes())
        return;

    float theEaseIn = 0;
    float theEaseOut = 0;
    if (CStudioPreferences::GetInterpolation())
        theEaseIn = theEaseOut = 100;

    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    IAnimationCore *theAnimationCore = theDoc->GetStudioSystem()->GetAnimationCore();

    if (!m_selectedKeyframes.empty()) {
        Qt3DSDMTimelineKeyframe *theTimelineKeyframe = m_selectedKeyframes.front()->binding;
        Qt3DSDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
        theTimelineKeyframe->GetKeyframeHandles(theKeyframeHandles);
        TKeyframe theKeyframeData = theAnimationCore->GetKeyframeData(theKeyframeHandles[0]);
        GetEaseInOutValues(theKeyframeData, theEaseIn, theEaseOut);
    }

    if (g_StudioApp.GetDialogs()->PromptForKeyframeInterpolation(theEaseIn, theEaseOut)) {
        // Note: Having "editor" variable here is important as its destructor
        // creates proper transaction
        Q3DStudio::ScopedDocumentEditor editor(*theDoc, QObject::tr("Set Keyframe Interpolation"),
                                        __FILE__, __LINE__);
        for (Keyframe *keyframe : qAsConst(m_selectedKeyframes)) {
            Qt3DSDMTimelineKeyframe *theTimelineKeyframe = keyframe->binding;
            Qt3DSDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
            theTimelineKeyframe->GetKeyframeHandles(theKeyframeHandles);
            for (size_t i = 0; i < theKeyframeHandles.size(); ++i) {
                TKeyframe theKeyframeData =
                    theAnimationCore->GetKeyframeData(theKeyframeHandles[i]);
                SetEaseInOutValues(theKeyframeData, theEaseIn, theEaseOut);
                theAnimationCore->SetKeyframeData(theKeyframeHandles[i], theKeyframeData);
            }
        }
    }
}

void KeyframeManager::SelectAllKeyframes()
{
    // Mahmoud_TODO: implement if needed
}

void KeyframeManager::DeselectAllKeyframes()
{
    deselectAllKeyframes();
}

void KeyframeManager::SetChangedKeyframes()
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMInstanceHandle selectedInstance = theDoc->GetSelectedInstance();
    if (selectedInstance.Valid()) {
        using namespace Q3DStudio;
        Q3DStudio::ScopedDocumentEditor editor(*theDoc, QObject::tr("Set Changed Keyframes"),
                                               __FILE__, __LINE__);
        CStudioSystem *theStudioSystem = theDoc->GetStudioSystem();
        // Get all animated properties.
        TPropertyHandleList properties;
        theStudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(selectedInstance,
                                                                             properties);
        for (size_t i = 0; i < properties.size(); ++i) {
            if (theStudioSystem->GetAnimationSystem()->IsPropertyAnimated(
                        selectedInstance, properties[i])) {
                editor->KeyframeProperty(selectedInstance, properties[i], true);
            }
        }
    }
}
