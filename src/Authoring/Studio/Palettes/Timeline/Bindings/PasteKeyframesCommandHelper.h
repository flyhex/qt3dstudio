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
#define INCLUDED_PASTE_KEYFRAME_COMMAND_HELPER_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "CmdDataModelInsertKeyframe.h"
#include "UICDMPropertyDefinition.h"
#include "UICDMDataCore.h"

// This caches all copied keyframes' time and data, for a paste action.
// This has to deal with the actual data and not keyframe handles, because a prior Cut can
// invalidate those handles.
class CPasteKeyframeCommandHelper
{
protected:
    typedef std::vector<CCmdDataModelInsertKeyframe::STimeKeyframeData> TCopiedKeyframeList;
    TCopiedKeyframeList m_CopiedKeyframeList;

public: // Construction
    CPasteKeyframeCommandHelper() {}
    ~CPasteKeyframeCommandHelper() {}

    // inTime should be relative to the earliest keyframe time in this list
    void AddKeyframeData(qt3dsdm::CUICDMPropertyHandle inProperty, float inKeyframeTime,
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
    CCmdDataModelInsertKeyframe *GetCommand(CDoc *inDoc, long inTimeOffsetInMilliseconds,
                                            qt3dsdm::CUICDMInstanceHandle inTargetInstance)
    {
        using namespace qt3dsdm;

        CCmdDataModelInsertKeyframe *theInsertKeyframesCommand = nullptr;
        TCopiedKeyframeList::iterator theIter = m_CopiedKeyframeList.begin();
        qt3dsdm::IPropertySystem *thePropertySystem = inDoc->GetStudioSystem()->GetPropertySystem();
        CClientDataModelBridge *theBridge = inDoc->GetStudioSystem()->GetClientDataModelBridge();

        for (; theIter != m_CopiedKeyframeList.end(); ++theIter) {
            TCharStr thePropertyName = thePropertySystem->GetName(theIter->m_Property);
            DataModelDataType::Value thePropertyType =
                thePropertySystem->GetDataType(theIter->m_Property);
            CUICDMPropertyHandle theTargetPropertyHandle =
                theBridge->GetAggregateInstancePropertyByName(inTargetInstance, thePropertyName);
            if (theTargetPropertyHandle.Valid()) // property exists on target
            {
                // sanity check for type match
                DataModelDataType::Value theTargetPropertyType =
                    thePropertySystem->GetDataType(theTargetPropertyHandle);
                if (theTargetPropertyType == thePropertyType) {
                    // 2. Offset keyframe time by current view time
                    double milliseconds = theIter->m_KeyframeTime * 1000.0;
                    double theTimeInMilliseconds = milliseconds + inTimeOffsetInMilliseconds;
                    float theTimeInSeconds = static_cast<float>(theTimeInMilliseconds / 1000.0);

                    if (!theInsertKeyframesCommand)
                        theInsertKeyframesCommand = new CCmdDataModelInsertKeyframe(
                            inDoc, inTargetInstance, theTargetPropertyHandle, theTimeInSeconds,
                            theIter->m_Infos, theIter->m_ValidInfoCount);
                    else
                        theInsertKeyframesCommand->AddKeyframeData(
                            theTargetPropertyHandle, theTimeInSeconds, theIter->m_Infos,
                            theIter->m_ValidInfoCount);
                }
            }
        }
        return theInsertKeyframesCommand;
    }

    void Clear() { m_CopiedKeyframeList.clear(); }
};

#endif
