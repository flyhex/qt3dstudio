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
#ifndef INCLUDED_QT3DSDM_TIMELINEITEM_BINDING_H
#define INCLUDED_QT3DSDM_TIMELINEITEM_BINDING_H 1

#pragma once

#include "ITimelineItemBinding.h"
#include "ITimelineItem.h"

// Data model
#include "Qt3DSDMHandles.h"
#include "IDragable.h"
#include "Qt3DSDMAssetTimelineKeyframe.h"
#include "OffsetKeyframesCommandHelper.h"
#include "Qt3DSDMTimeline.h"
#include "Qt3DSDMSignals.h"
#include "DispatchListeners.h"

//==============================================================================
//	Classes
//==============================================================================
class CTimelineTranslationManager;
class CBaseStateRow;
class Qt3DSDMTimelineItemProperty;
class CCmdDataModelSetKeyframeTime;
class RowTree;

namespace qt3dsdm {
class CStudioSystem;
}

//=============================================================================
/**
 * Binding to generic DataModel object
 */
class Qt3DSDMTimelineItemBinding : public ITimelineItemBinding,
        public ITimelineItem,
        public IDragable,
        public IDataModelListener

{
protected: // Typedef
    typedef std::map<qt3dsdm::Qt3DSDMPropertyHandle, Qt3DSDMTimelineItemProperty *> TPropertyBindingMap;
    typedef std::vector<Qt3DSDMAssetTimelineKeyframe> TAssetKeyframeList;

protected:
    CBaseStateRow *m_Row; // TODO: remove after finishing the new timeline
    RowTree *m_rowTree = nullptr;
    CTimelineTranslationManager *m_TransMgr;
    qt3dsdm::Qt3DSDMInstanceHandle m_DataHandle;
    ITimelineItemBinding *m_Parent;
    ITimelineTimebar *m_TimelineTimebar;
    TPropertyBindingMap m_PropertyBindingMap;
    TAssetKeyframeList m_Keyframes; /// Sorted (by time) list of keyframes
    qt3dsdm::CStudioSystem *m_StudioSystem;

    qt3dsdm::TSignalConnectionPtr m_StartTimeConnection;
    qt3dsdm::TSignalConnectionPtr m_EndTimeConnection;

public:
    Qt3DSDMTimelineItemBinding(CTimelineTranslationManager *inMgr,
                               qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);
    Qt3DSDMTimelineItemBinding(CTimelineTranslationManager *inMgr);
    virtual ~Qt3DSDMTimelineItemBinding();

protected:
    bool GetBoolean(qt3dsdm::Qt3DSDMPropertyHandle inProperty) const;
    void SetBoolean(qt3dsdm::Qt3DSDMPropertyHandle inProperty, bool inValue,
                    const QString &inNiceText) const;
    void SetInstanceHandle(qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);

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
    CBaseStateRow *GetRow() override; // Mahmoud_TODO: remove after finishing the new timeline
    RowTree *getRowTree() const override;
    void setRowTree(RowTree *row) override;
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
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetInstanceHandle() const;

    // IDragable
    long GetFlavor() const override;

    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                            long inInstanceCount) override;
    void RefreshStateRow(bool inRefreshChildren = false);

    virtual void AddPropertyRow(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
                                bool inAppend = false);
    virtual void RemovePropertyRow(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle);
    virtual void RefreshPropertyKeyframe(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
                                         qt3dsdm::Qt3DSDMKeyframeHandle,
                                         ETimelineKeyframeTransaction inTransaction);
    virtual void OnPropertyChanged(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle);
    virtual void OnPropertyLinked(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle);

    virtual void UIRefreshPropertyKeyframe(long inOffset);
    // Keyframe manipulation
    virtual bool HasDynamicKeyframes(long inTime);
    virtual void SetDynamicKeyframes(long inTime, bool inDynamic);
    virtual void DoSelectKeyframes(bool inSelected, long inTime, bool inUpdateUI);
    virtual void OnPropertySelection(long inTime);

    virtual void OnAddChild(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    virtual void OnDeleteChild(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

    void UpdateActionStatus();

    Q3DStudio::CId GetGuid() const;

    // Bridge between asset & DataModel. Ideally we should be fully DataModel
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetInstance() const;

    int getAnimatedPropertyIndex(int propertyHandle) const;

    ITimelineItemProperty *GetOrCreatePropertyBinding(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle);
    ITimelineItemProperty *GetPropertyBinding(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle);
protected:
    virtual ITimelineTimebar *CreateTimelineTimebar();
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

#endif // INCLUDED_QT3DSDM_TIMELINEITEM_BINDING_H
