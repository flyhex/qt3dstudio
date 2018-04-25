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
#include "CmdDataModelRemoveKeyframe.h"
#include "CmdDataModelInsertKeyframe.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/OffsetKeyframesCommandHelper.h"
#include "Bindings/Qt3DSDMTimelineKeyframe.h"
#include "StudioPreferences.h"
#include "Qt3DSDMAnimation.h"
#include "Dialogs.h"

#include <qglobal.h>
#include <QtCore/qhash.h>
#include <QtCore/qdebug.h>

using namespace qt3dsdm;

// Mahmmoud_TODO: This function is copied from old timeline code. It should be removed after the
// new timeline is done (during cleanup of old timeline)
// legacy stuff that we have to support for animation tracks in the old data model to work
inline void PostExecuteCommand(IDoc *inDoc)
{
    CDoc *theDoc = dynamic_cast<CDoc *>(inDoc);
    theDoc->GetCore()->CommitCurrentCommand();
}

KeyframeManager::KeyframeManager(TimelineGraphicsScene *scene) : m_scene(scene)
{
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

        keyframe->rowMaster->putSelectedKeyframesOnTop();
        keyframe->rowMaster->updateKeyframes();

        keyframe->binding->SetSelected(true);
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

    int idx1 = (rect.top() + 4) / TimelineConstants::ROW_H;
    int idx2 = (rect.bottom() - 4) / TimelineConstants::ROW_H;

    m_scene->rowManager()->clampIndex(idx1);
    m_scene->rowManager()->clampIndex(idx2);

    RowTimeline *rowTimeline;
    for (int i = idx1; i <= idx2; ++i) {
        rowTimeline = m_scene->rowManager()->rowTimelineAt(i);

        if (rowTimeline != nullptr) {
            const auto keyframes = rowTimeline->getKeyframesInRange(rect.left(), rect.right());
            for (auto keyframe : keyframes) {
                if (!m_selectedKeyframes.contains(keyframe)) {
                    m_selectedKeyframes.append(keyframe);

                    if (!m_selectedKeyframesMasterRows.contains(keyframe->rowMaster))
                        m_selectedKeyframesMasterRows.append(keyframe->rowMaster);
                }
            }
        }
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
        PostExecuteCommand(theDoc);
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
    if (!m_selectedKeyframes.empty()
        && m_selectedKeyframesMasterRows.count() == 1) {
        // delete old copies
        for (auto keyframe : qAsConst(m_copiedKeyframes))
            delete keyframe;

        m_copiedKeyframes.clear();

        Keyframe *copyKeyframe;
        for (auto keyframe : qAsConst(m_selectedKeyframes)) {
            copyKeyframe = new Keyframe(*keyframe);
            copyKeyframe->rowMaster = nullptr;
            copyKeyframe->rowProperty = nullptr;
            m_copiedKeyframes.append(copyKeyframe);
        }
    }
}

void KeyframeManager::pasteKeyframes(RowTimeline *row)
{
    if (row == nullptr)
        return;

    if (row->rowTree()->isProperty())
        row = row->parentRow();

    if (!m_copiedKeyframes.empty()) {
        // filter copied keyframes to the row supported properties
        const QList<Keyframe *> filteredKeyframes = filterKeyframesForRow(row, m_copiedKeyframes);

        // calc min/max copied frames time
        double minTime = 999999; // seconds (~277.78 hrs)
        double maxTime = 0;
        for (auto keyframe : filteredKeyframes) {
            if (keyframe->time < minTime)
                minTime = keyframe->time;

            if (keyframe->time > maxTime)
                maxTime = keyframe->time;
        }

        double dt = m_scene->playHead()->time() - minTime;

        if (maxTime + dt > m_scene->ruler()->duration())
            dt = m_scene->ruler()->duration() - maxTime;

        RowTree *propRow;
        QList<Keyframe *> addedKeyframes;
        for (auto keyframe : filteredKeyframes) {
            propRow = m_scene->rowManager()->getOrCreatePropertyRow(row->rowTree(),
                                                                    keyframe->propertyType);
            addedKeyframes.append(insertKeyframe(propRow->rowTimeline(), keyframe->time + dt,
                                                 false));
        }

        if (!addedKeyframes.empty()) {
            deselectAllKeyframes();
            selectKeyframes(addedKeyframes);
        }
    }
}

QList<Keyframe *> KeyframeManager::filterKeyframesForRow(RowTimeline *row,
                                                         const QList<Keyframe *> &keyframes)
{
    QList<Keyframe *> result;

    for (auto keyframe : keyframes) {
        if (SUPPORTED_ROW_PROPS[row->rowTree()->rowType()].contains(keyframe->propertyType))
            result.append(keyframe);
    }

    return result;
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
    return !m_copiedKeyframes.empty();
}

// Mahmoud_TODO: rewrite a better interface for the new timeline
// ITimelineKeyframesManager interface
void KeyframeManager::SetKeyframeTime(long inTime)
{
    // Mahmoud_TODO: implement if needed
}

void KeyframeManager::SetKeyframesDynamic(bool inDynamic)
{
    // Mahmoud_TODO: implement if needed
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
    return true; // Mahmoud_TODO: implement
}

bool KeyframeManager::CanPerformKeyframeCopy()
{
    return true; // Mahmoud_TODO: implement
}

bool KeyframeManager::CanPerformKeyframePaste()
{
    return true; // Mahmoud_TODO: implement
}

void KeyframeManager::CopyKeyframes()
{
    copySelectedKeyframes();
}

bool KeyframeManager::RemoveKeyframes(bool inPerformCopy)
{
    return deleteSelectedKeyframes();
}

void KeyframeManager::PasteKeyframes()
{
    // Mahmoud_TODO: implement
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
        Q3DStudio::ScopedDocumentEditor editor(*theDoc, L"Set Keyframe Interpolation",
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
    // Mahmoud_TODO: implement if needed
}

void KeyframeManager::SetChangedKeyframes()
{
    // Mahmoud_TODO: implement if needed
}

const QHash<int, QList<QString>> KeyframeManager::SUPPORTED_ROW_PROPS = {
    { OBJTYPE_LAYER, {
        "Left",
        "Width",
        "Top",
        "Height",
        "Ambient Occulusion",
        "AO Distance",
        "AO Softness",
        "AO Threshold",
        "AO Sampling Rate",
        "IBL Brightness",
        "IBL Horizon Cutoff",
        "IBL FOV Angle" }
    },
    { OBJTYPE_CAMERA, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "FieldOfView",
        "ClippingStart",
        "ClippingEnd" }
    },
    { OBJTYPE_LIGHT, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "LightColor",
        "SpecularColor",
        "AmbientColor",
        "Brightness",
        "ShadowDarkness",
        "ShadowSoftness",
        "ShadowDepthBias",
        "ShadowFarClip",
        "ShadowFOV" }
    },
    { OBJTYPE_MODEL, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "Opacity",
        "EdgeTessellation",
        "InnerTessellation" }
    },
    { OBJTYPE_TEXT, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "Opacity",
        "TextColor",
        "Leading",
        "Tracking" }
    },
    { OBJTYPE_ALIAS, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "Opacity" }
    },
    { OBJTYPE_GROUP, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "Opacity" }
    },
    { OBJTYPE_COMPONENT, {
        "Position",
        "Rotation",
        "Scale",
        "Pivot",
        "Opacity" }
    }
};
