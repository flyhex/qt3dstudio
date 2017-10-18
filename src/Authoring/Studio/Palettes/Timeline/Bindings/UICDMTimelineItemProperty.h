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

#ifndef INCLUDED_UICDMTIMELINE_ITEM_PROPERTY_H
#define INCLUDED_UICDMTIMELINE_ITEM_PROPERTY_H 1

#pragma once

#include "ITimelineItemProperty.h"
#include "UICDMTimelineKeyframe.h"
#include "UICDMTimeline.h"
#include "UICDMPropertyDefinition.h"

class CTimelineTranslationManager;
class CCmdDataModelSetKeyframeValue;
class CUICDMTimelineItemBinding;

//=============================================================================
/**
 * A data model item's property.
 * Typically only animated properties show up in the Timeline.
 */
//=============================================================================
class CUICDMTimelineItemProperty : public ITimelineItemProperty
{
public:
    CUICDMTimelineItemProperty(CTimelineTranslationManager *inTransMgr,
                               qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
                               qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    virtual ~CUICDMTimelineItemProperty();

    // ITimelineProperty
    Q3DStudio::CString GetName() const override;
    bool IsMaster() const override;
    qt3dsdm::TDataTypePair GetType() const override;
    float GetMaximumValue() const override;
    float GetMinimumValue() const override;
    void SetSelected() override;
    void ClearKeySelection() override;
    void DeleteAllKeys() override;
    ITimelineKeyframesManager *GetKeyframesManager() const override;
    IKeyframe *GetKeyframeByTime(long inTime) const override;
    IKeyframe *GetKeyframeByIndex(long inIndex) const override;
    long GetKeyframeCount() const override;
    long GetChannelCount() const override;
    float GetChannelValueAtTime(long inChannelIndex, long inTime) override;
    void SetChannelValueAtTime(long inChannelIndex, long inTime, float inValue) override;
    long OffsetSelectedKeyframes(long inOffset) override;
    void CommitChangedKeyframes() override;
    void OnEditKeyframeTime(long inCurrentTime, long inObjectAssociation) override;
    bool IsDynamicAnimation() override;
    // IKeyframeSelector
    void SelectKeyframes(bool inSelected, long inTime = -1) override;

    void Bind(CPropertyRow *inRow) override;
    void Release() override;
    CPropertyRow *GetRow() override;

    bool RefreshKeyframe(qt3dsdm::CUICDMKeyframeHandle inKeyframe,
                         ETimelineKeyframeTransaction inTransaction);
    IKeyframe *GetKeyframeByHandle(qt3dsdm::CUICDMKeyframeHandle inKeyframe);
    void DoSelectKeyframes(bool inSelected, long inTime, bool inParentTriggered,
                           CUICDMTimelineItemBinding *inParent);

    void RefreshKeyFrames(void);

protected:
    void InitializeCachedVariables(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool CreateKeyframeIfNonExistent(qt3dsdm::CUICDMKeyframeHandle inKeyframe,
                                     qt3dsdm::CUICDMAnimationHandle inOwningAnimation);
    void OnPropertyLinkStatusChanged(qt3dsdm::CUICDMSlideHandle inSlide,
                                     qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                     qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void CreateKeyframes();
    void ReleaseKeyframes();

protected:
    typedef std::vector<CUICDMTimelineKeyframe *> TKeyframeList;

    CPropertyRow *m_Row;
    qt3dsdm::Qt3DSDMInstanceHandle m_InstanceHandle;
    qt3dsdm::Qt3DSDMPropertyHandle m_PropertyHandle;
    CTimelineTranslationManager *m_TransMgr;
    std::vector<qt3dsdm::CUICDMAnimationHandle> m_AnimationHandles;
    TKeyframeList m_Keyframes;
    CCmdDataModelSetKeyframeValue
        *m_SetKeyframeValueCommand; // for merging modifying keyframe values via graph
    qt3dsdm::TDataTypePair m_Type;
    Q3DStudio::CString m_Name;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_Signals;
};

#endif // INCLUDED_UICDMTIMELINE_ITEM_PROPERTY_H
