/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_PASTE_KEYFRAME_COMMAND_HELPER_H
#define INCLUDED_PASTE_KEYFRAME_COMMAND_HELPER_H

#include "CmdDataModelInsertKeyframe.h"
#include "Qt3DSDMPropertyDefinition.h"
#include "Qt3DSDMDataCore.h"

// This caches all copied keyframes' time and data, for a paste action.
// This has to deal with the actual data and not keyframe handles, because a prior Cut can
// invalidate those handles.
class CPasteKeyframeCommandHelper
{
protected:
    typedef std::vector<CCmdDataModelInsertKeyframe::STimeKeyframeData> TCopiedKeyframeList;
    TCopiedKeyframeList m_CopiedKeyframeList;

public:
    CPasteKeyframeCommandHelper() {}
    ~CPasteKeyframeCommandHelper() {}

    // inTime should be relative to the earliest keyframe time in this list
    void AddKeyframeData(qt3dsdm::Qt3DSDMPropertyHandle inProperty, long inKeyframeTime,
                         qt3dsdm::SGetOrSetKeyframeInfo *inInfos, size_t inInfoCount)
    {
        m_CopiedKeyframeList.push_back(CCmdDataModelInsertKeyframe::STimeKeyframeData(
            inProperty, inKeyframeTime, inInfos, inInfoCount));
    }

    bool HasCopiedKeyframes() const { return !m_CopiedKeyframeList.empty(); }

    // Triggered by a "Paste Keyframe" action
    // Note: The logic is based on what the Animation Manager in the old system used to do.
    // 1. The condition for paste to occur is that the property name matches.
    // The old data model has a limitation that if the destination property is a linked property,
    // the source has to come from the same instance, most likely a easy way out than to deal with
    // with having to 'sync' all linked animation tracks.
    // but that is not an issue in the new data model.
    //
    // 2. The first pasted keyframe is at current view time and the rest are offset accordingly.
    CCmdDataModelInsertKeyframe *GetCommand(CDoc *doc, long timeOffset,
                                            qt3dsdm::Qt3DSDMInstanceHandle targetInstance)
    {
        using namespace qt3dsdm;

        CCmdDataModelInsertKeyframe *insertKeyframesCmd = nullptr;
        qt3dsdm::IPropertySystem *propSys = doc->GetStudioSystem()->GetPropertySystem();
        CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();

        for (auto &kfData : m_CopiedKeyframeList) {
             // check property exists on target
            if (bridge->hasAggregateInstanceProperty(targetInstance, kfData.m_Property)) {
                if (!insertKeyframesCmd)
                    insertKeyframesCmd = new CCmdDataModelInsertKeyframe(doc, targetInstance);

                // Offset keyframe time by current view time
                long time = kfData.m_KeyframeTime + timeOffset;
                insertKeyframesCmd->AddKeyframeData(kfData.m_Property, time, kfData.m_Infos,
                                                    kfData.m_ValidInfoCount);
            }
        }

        return insertKeyframesCmd;
    }

    void Clear() { m_CopiedKeyframeList.clear(); }
};

#endif
