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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_UICDM_TIMELINEITEM_BINDING_H
#define INCLUDED_UICDM_TIMELINEITEM_BINDING_H 1

#pragma once

#include "ITimelineItemBinding.h"
#include "ITimelineItem.h"

// Data model
#include "UICDMHandles.h"
#include "IDragable.h"
#include "UICDMAssetTimelineKeyframe.h"
#include "OffsetKeyframesCommandHelper.h"
#include "UICDMTimeline.h"
#include "UICDMSignals.h"
#include "DispatchListeners.h"

//==============================================================================
//	Classes
//==============================================================================
class CTimelineTranslationManager;
class CBaseStateRow;
class CUICDMTimelineItemProperty;
class CCmdDataModelSetKeyframeTime;

namespace UICDM {
class CStudioSystem;
}

//=============================================================================
/**
 * Binding to generic UICDM object
 */
class CUICDMTimelineItemBinding : public ITimelineItemBinding,
                                  public ITimelineItem,
                                  public IDragable,
                                  public IDataModelListener

{
protected: // Typedef
    typedef std::map<UICDM::CUICDMPropertyHandle, CUICDMTimelineItemProperty *> TPropertyBindingMap;
    typedef std::vector<CUICDMAssetTimelineKeyframe> TAssetKeyframeList;

protected:
    CBaseStateRow *m_Row;
    CTimelineTranslationManager *m_TransMgr;
    UICDM::CUICDMInstanceHandle m_DataHandle;
    ITimelineItemBinding *m_Parent;
    ITimelineTimebar *m_TimelineTimebar;
    TPropertyBindingMap m_PropertyBindingMap;
    TAssetKeyframeList m_Keyframes; /// Sorted (by time) list of keyframes
    UICDM::CStudioSystem *m_StudioSystem;

    UICDM::TSignalConnectionPtr m_StartTimeConnection;
    UICDM::TSignalConnectionPtr m_EndTimeConnection;

public:
    CUICDMTimelineItemBinding(CTimelineTranslationManager *inMgr,
                              UICDM::CUICDMInstanceHandle inDataHandle);
    CUICDMTimelineItemBinding(CTimelineTranslationManager *inMgr);
    virtual ~CUICDMTimelineItemBinding();

protected:
    bool UICDMGetBoolean(UICDM::CUICDMPropertyHandle inProperty) const;
    void UICDMSetBoolean(UICDM::CUICDMPropertyHandle inProperty, bool inValue,
                         const QString &inNiceText) const;
    void SetInstanceHandle(UICDM::CUICDMInstanceHandle inDataHandle);

public:
    // ITimelineItem
    EStudioObjectType GetObjectType() const override;
    bool IsMaster() const override;
    bool IsShy() const override;
    void SetShy(bool) override;
    bool IsLocked() const override;
    void SetLocked(bool) override;
    bool IsVisible() const override;
    void SetVisible(bool) override;
    bool IsExpanded() const override;
    void SetExpanded(bool inExpanded) override;
    bool HasAction(bool inMaster) override;
    bool ChildrenHasAction(bool inMaster) override;
    bool ComponentHasAction(bool inMaster) override;
    ITimelineTimebar *GetTimebar() override;

    // INamable
    Q3DStudio::CString GetName() const override;
    void SetName(const Q3DStudio::CString &inName) override;

    // ITimelineItemBinding
    ITimelineItem *GetTimelineItem() override;
    CBaseStateRow *GetRow() override;
    void SetSelected(bool inMultiSelect) override;
    void OnCollapsed() override;
    void ClearKeySelection() override;
    bool OpenAssociatedEditor() override;
    void DoStartDrag(CControlWindowListener *inWndListener) override;
    void SetDropTarget(CDropTarget *inTarget) override;
    // Hierarchy
    long GetChildrenCount() override;
    ITimelineItemBinding *GetChild(long inIndex) override;
    ITimelineItemBinding *GetParent() override;
    void SetParent(ITimelineItemBinding *parent) override;
    // Properties
    long GetPropertyCount() override;
    ITimelineItemProperty *GetProperty(long inIndex) override;
    // Eye/Lock toggles
    bool ShowToggleControls() const override;
    bool IsLockedEnabled() const override;
    bool IsVisibleEnabled() const override;
    // Init/Cleanup
    void Bind(CBaseStateRow *inRow) override;
    void Release() override;
    // ContextMenu
    bool IsValidTransaction(EUserTransaction inTransaction) override;
    void PerformTransaction(EUserTransaction inTransaction) override;
    Q3DStudio::CString GetObjectPath() override;
    // Selected keyframes
    ITimelineKeyframesManager *GetKeyframesManager() const override;
    // Properties
    void RemoveProperty(ITimelineItemProperty *inProperty) override;
    void LoadProperties() override;

    // ITimelineItemKeyframesHolder
    void InsertKeyframe() override;
    void DeleteAllChannelKeyframes() override;
    long GetKeyframeCount() const override;
    IKeyframe *GetKeyframeByTime(long inTime) const override;
    IKeyframe *GetKeyframeByIndex(long inIndex) const override;
    long OffsetSelectedKeyframes(long inOffset) override;
    void CommitChangedKeyframes() override;
    void OnEditKeyframeTime(long inCurrentTime, long inObjectAssociation) override;

    // IKeyframeSelector
    void SelectKeyframes(bool inSelected, long inTime = -1) override;

    // IUICDMSelectable
    virtual UICDM::CUICDMInstanceHandle GetInstanceHandle() const;

    // IDragable
    long GetFlavor() const override;

    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(UICDM::CUICDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(UICDM::CUICDMInstanceHandle *inInstance,
                                                    long inInstanceCount) override;
    void RefreshStateRow(bool inRefreshChildren = false);

    virtual void AddPropertyRow(UICDM::CUICDMPropertyHandle inPropertyHandle,
                                bool inAppend = false);
    virtual void RemovePropertyRow(UICDM::CUICDMPropertyHandle inPropertyHandle);
    virtual void RefreshPropertyKeyframe(UICDM::CUICDMPropertyHandle inPropertyHandle,
                                         UICDM::CUICDMKeyframeHandle,
                                         ETimelineKeyframeTransaction inTransaction);
    virtual void OnPropertyChanged(UICDM::CUICDMPropertyHandle inPropertyHandle);
    virtual void OnPropertyLinked(UICDM::CUICDMPropertyHandle inPropertyHandle);

    virtual void UIRefreshPropertyKeyframe(long inOffset);
    // Keyframe manipulation
    virtual bool HasDynamicKeyframes(long inTime);
    virtual void SetDynamicKeyframes(long inTime, bool inDynamic);
    virtual void DoSelectKeyframes(bool inSelected, long inTime, bool inUpdateUI);
    virtual void OnPropertySelection(long inTime);

    virtual void OnAddChild(UICDM::CUICDMInstanceHandle inInstance);
    virtual void OnDeleteChild(UICDM::CUICDMInstanceHandle inInstance);

    void UpdateActionStatus();

    Q3DStudio::CId GetGuid() const;

    // Bridge between asset & UICDM. Ideally we should be fully UICDM
    virtual UICDM::CUICDMInstanceHandle GetInstance() const;

protected:
    virtual ITimelineTimebar *CreateTimelineTimebar();
    ITimelineItemProperty *GetPropertyBinding(UICDM::CUICDMPropertyHandle inPropertyHandle);
    ITimelineItemProperty *GetOrCreatePropertyBinding(UICDM::CUICDMPropertyHandle inPropertyHandle);
    void RemoveAllPropertyBindings();
    void AddKeyframes(ITimelineItemProperty *inPropertyBinding);
    bool
    DeleteAssetKeyframesWhereApplicable(ITimelineItemProperty *inTriggerPropertyBinding = nullptr);
    void UpdateKeyframe(IKeyframe *inKeyframe, ETimelineKeyframeTransaction inTransaction);

    // For iterating through children
    virtual bool AmITimeParent() const { return false; }

    // subclasses can call this method to open referenced files
    virtual bool OpenSourcePathFile();
};

#endif // INCLUDED_UICDM_TIMELINEITEM_BINDING_H
