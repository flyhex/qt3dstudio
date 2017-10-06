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

#ifndef INCLUDED_KEYFRAMES_MANAGER_H
#define INCLUDED_KEYFRAMES_MANAGER_H 1

#pragma once

#include "ITimelineKeyframesManager.h"
#include "OffsetKeyframesCommandHelper.h"

class CTimelineTranslationManager;
class CUICDMTimelineKeyframe;
class CUICDMTimelineItemBinding;
class CPasteKeyframeCommandHelper;

//=============================================================================
/**
 * Abstraction layer to the class that manages both selected keyframes.
 */
//=============================================================================
class CKeyframesManager : public ITimelineKeyframesManager
{
public:
    CKeyframesManager(CTimelineTranslationManager *inTransMgr);
    virtual ~CKeyframesManager();

    // IKeyframesManager
    bool HasSelectedKeyframes(bool inOnlyDynamic = false) override;
    bool HasDynamicKeyframes() override;
    bool CanPerformKeyframeCopy() override;
    bool CanPerformKeyframePaste() override;
    void CopyKeyframes() override;
    bool RemoveKeyframes(bool inPerformCopy) override;
    void PasteKeyframes() override;
    void SetKeyframeInterpolation() override;
    void DeselectAllKeyframes() override;
    void SelectAllKeyframes() override;
    void SetChangedKeyframes() override;
    // ITimelineKeyframesManager
    void SetKeyframeTime(long inTime) override;
    void SetKeyframesDynamic(bool inDynamic) override;
    bool CanMakeSelectedKeyframesDynamic() override;
    long OffsetSelectedKeyframes(long inOffset) override;
    void CommitChangedKeyframes() override;
    void RollbackChangedKeyframes() override;

    void SetKeyframeSelected(CUICDMTimelineKeyframe *inKeyframe, bool inSelected,
                             CUICDMTimelineItemBinding *inOwningInstance = nullptr);

protected:
    void SetKeyframeDynamic(CUICDMTimelineKeyframe *inKeyframe, bool inDynamic);
    void CopySelectedKeyframes();

public:
    struct SKeyframeInstancePair
    {
        CUICDMTimelineKeyframe *m_Keyframe;
        CUICDMTimelineItemBinding *m_Instance;

        SKeyframeInstancePair(CUICDMTimelineKeyframe *inKeyframe,
                              CUICDMTimelineItemBinding *inInstance)
        {
            m_Keyframe = inKeyframe;
            m_Instance = inInstance;
        }
    };

protected:
    typedef std::vector<SKeyframeInstancePair> TSelectedKeyframeList; ///< handle multiple keyframes
                                                                      ///manipulation, e.g.
                                                                      ///offsetting by dragging

    CTimelineTranslationManager *m_TransMgr;
    TSelectedKeyframeList m_SelectedKeyframes;
    COffsetKeyframesCommandHelper m_OffsetKeyframeCommandHelper; // so that we can commit on mouseup
    CPasteKeyframeCommandHelper *m_PasteKeyframeCommandHelper;
    std::set<CUICDMTimelineItemBinding *> m_InstanceSet;
    std::vector<UICDM::CUICDMInstanceHandle> m_InstanceList;
};

#endif // INCLUDED_IKEYFRAMES_MANAGER_H
