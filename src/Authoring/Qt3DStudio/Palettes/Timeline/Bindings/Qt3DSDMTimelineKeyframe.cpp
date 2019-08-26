/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSDMTimelineKeyframe.h"
#include "Qt3DSDMAnimation.h"
#include "CmdDataModelChangeKeyframe.h"
#include "CmdBatch.h"
#include "Qt3DSDMStudioSystem.h"

#include "Doc.h"
#include "StudioApp.h"
#include "Core.h"

using namespace qt3dsdm;

// TODO: figure out if we can just use IDoc instead of CDoc
Qt3DSDMTimelineKeyframe::Qt3DSDMTimelineKeyframe(IDoc *inDoc)
    : m_Doc(static_cast<CDoc *>(inDoc))
{
}

Qt3DSDMTimelineKeyframe::~Qt3DSDMTimelineKeyframe()
{
}

bool Qt3DSDMTimelineKeyframe::IsSelected() const
{
    return m_Selected;
}

long Qt3DSDMTimelineKeyframe::GetTime() const
{
    if (!m_KeyframeHandles.empty()) {
        IAnimationCore *animaCore = m_Doc->GetStudioSystem()->GetAnimationCore();
        Qt3DSDMKeyframeHandle kfHandle = *m_KeyframeHandles.begin();
        if (animaCore->KeyframeValid(kfHandle))
            return getKeyframeTime(animaCore->GetKeyframeData(kfHandle));
    }

    return -1; // keyframe was deleted, and data cannot be retrieved.
}

void Qt3DSDMTimelineKeyframe::SetTime(const long inNewTime)
{
    CCmd *theCmd = nullptr;
    if (m_KeyframeHandles.size() == 1) {
        theCmd = new CCmdDataModelSetKeyframeTime(m_Doc, m_KeyframeHandles.front(), inNewTime);
    } else { // more than 1 channel
        CCmdBatch *theBatch = new CCmdBatch(m_Doc);
        TKeyframeHandleList::iterator theIter = m_KeyframeHandles.begin();
        for (; theIter != m_KeyframeHandles.end(); ++theIter)
            theBatch->AddCommand(new CCmdDataModelSetKeyframeTime(m_Doc, *theIter, inNewTime));
        theCmd = theBatch;
    }
    if (theCmd)
        m_Doc->GetCore()->ExecuteCommand(theCmd);
}

inline Qt3DSDMAnimationHandle GetAnimationHandle(qt3dsdm::IAnimationCore *inAnimationCore,
                                                const TKeyframeHandleList &inKeyframes)
{
    if (!inKeyframes.empty())
        return inAnimationCore->GetAnimationForKeyframe(inKeyframes[0]);
    return 0;
}

void Qt3DSDMTimelineKeyframe::SetDynamic(bool inIsDynamic)
{
    if (!m_KeyframeHandles.empty()) {
        Qt3DSDMAnimationHandle theAnimation =
            GetAnimationHandle(m_Doc->GetStudioSystem()->GetAnimationCore(), m_KeyframeHandles);
        if (theAnimation.Valid())
            m_Doc->GetCore()->ExecuteCommand(
                new CCmdDataModelChangeDynamicKeyframe(m_Doc, theAnimation, inIsDynamic));
    }
}

Keyframe *Qt3DSDMTimelineKeyframe::getUI()
{
    return m_ui;
}

void Qt3DSDMTimelineKeyframe::setUI(Keyframe *kfUI)
{
    m_ui = kfUI;
}

// Only the first key of a track can be dynamic.
bool Qt3DSDMTimelineKeyframe::IsDynamic() const
{
    qt3dsdm::IAnimationCore *theAnimationCore = m_Doc->GetStudioSystem()->GetAnimationCore();
    Qt3DSDMAnimationHandle theAnimation = GetAnimationHandle(theAnimationCore, m_KeyframeHandles);
    if (theAnimation.Valid()) {
        SAnimationInfo theInfo = theAnimationCore->GetAnimationInfo(theAnimation);
        if (theInfo.m_DynamicFirstKeyframe) {
            TKeyframeHandleList theKeyframes;
            theAnimationCore->GetKeyframes(theAnimation, theKeyframes);
            if (!theKeyframes.empty()) // only true if track is dynamic and this is the first
                                       // keyframe. Might have to optimize because this is so
                                       // clunky.
                return (theKeyframes[0] == m_KeyframeHandles[0]);
        }
    }
    return false;
}

void Qt3DSDMTimelineKeyframe::AddKeyframeHandle(qt3dsdm::Qt3DSDMKeyframeHandle inHandle)
{
    m_KeyframeHandles.push_back(inHandle);
}

bool Qt3DSDMTimelineKeyframe::HasKeyframeHandle(qt3dsdm::Qt3DSDMKeyframeHandle inHandle) const
{
    TKeyframeHandleList::const_iterator theIter = m_KeyframeHandles.begin();
    for (; theIter != m_KeyframeHandles.end(); ++theIter) {
        if (*theIter == inHandle)
            return true;
    }
    return false;
}

void Qt3DSDMTimelineKeyframe::SetSelected(bool inSelected)
{
    m_Selected = inSelected;
}

void Qt3DSDMTimelineKeyframe::GetKeyframeHandles(TKeyframeHandleList &outList) const
{
    outList = m_KeyframeHandles;
}
