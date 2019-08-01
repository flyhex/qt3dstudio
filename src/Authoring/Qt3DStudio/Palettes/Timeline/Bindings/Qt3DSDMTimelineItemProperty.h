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

#ifndef QT3DSDM_TIMELINE_ITEM_PROPERTY_H
#define QT3DSDM_TIMELINE_ITEM_PROPERTY_H 1

#pragma once

#include "ITimelineItemProperty.h"
#include "Qt3DSDMTimelineKeyframe.h"
#include "Qt3DSDMTimeline.h"
#include "Qt3DSDMPropertyDefinition.h"
#include "Qt3DSDMAnimation.h"

class RowTree;
class CTimelineTranslationManager;
class CCmdDataModelSetKeyframeValue;
class Qt3DSDMTimelineItemBinding;

//=============================================================================
/**
 * A data model item's property.
 * Typically only animated properties show up in the Timeline.
 */
//=============================================================================
class Qt3DSDMTimelineItemProperty : public ITimelineItemProperty
{
public:
    Qt3DSDMTimelineItemProperty(CTimelineTranslationManager *inTransMgr,
                                qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
                                qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    virtual ~Qt3DSDMTimelineItemProperty();

    // ITimelineProperty
    Q3DStudio::CString GetName() const override;
    bool IsMaster() const override;
    qt3dsdm::TDataTypePair GetType() const override;
    float GetMaximumValue() const override;
    float GetMinimumValue() const override;
    void SetSelected() override;
    void DeleteAllKeys() override;
    IKeyframe *GetKeyframeByTime(long inTime) const override;
    IKeyframe *GetKeyframeByIndex(long inIndex) const override;
    long GetKeyframeCount() const override;
    size_t GetChannelCount() const override;
    float GetChannelValueAtTime(long inChannelIndex, long inTime) override;
    void SetChannelValueAtTime(long inChannelIndex, long inTime, float inValue) override;
    bool IsDynamicAnimation() override;
    void setRowTree(RowTree *rowTree) override;
    RowTree *getRowTree() const override;
    qt3dsdm::Qt3DSDMPropertyHandle getPropertyHandle() const override;
    std::vector<qt3dsdm::Qt3DSDMAnimationHandle> animationHandles() const override;

    bool RefreshKeyframe(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                         ETimelineKeyframeTransaction inTransaction);
    IKeyframe *GetKeyframeByHandle(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe);

    void RefreshKeyFrames(void);
    qt3dsdm::EAnimationType animationType() const override;

protected:
    using TKeyframeList = std::vector<Qt3DSDMTimelineKeyframe *>;

    void InitializeCachedVariables(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool CreateKeyframeIfNonExistent(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                                     qt3dsdm::Qt3DSDMAnimationHandle inOwningAnimation);
    void OnPropertyLinkStatusChanged(qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                     qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                     qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void CreateKeyframes();
    void ReleaseKeyframes();

    qt3dsdm::Qt3DSDMInstanceHandle m_InstanceHandle;
    qt3dsdm::Qt3DSDMPropertyHandle m_PropertyHandle;
    CTimelineTranslationManager *m_TransMgr;
    std::vector<qt3dsdm::Qt3DSDMAnimationHandle> m_AnimationHandles;
    TKeyframeList m_Keyframes;
    CCmdDataModelSetKeyframeValue
        *m_SetKeyframeValueCommand; // for merging modifying keyframe values via graph
    qt3dsdm::TDataTypePair m_Type;
    Q3DStudio::CString m_Name;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_Signals;

private:
    RowTree *m_rowTree = nullptr;
};

#endif // QT3DSDM_TIMELINE_ITEM_PROPERTY_H
