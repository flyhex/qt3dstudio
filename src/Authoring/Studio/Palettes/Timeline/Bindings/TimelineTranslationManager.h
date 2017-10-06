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
#ifndef INCLUDED_TIMELINE_TRANSLATIONMANAGER_H
#define INCLUDED_TIMELINE_TRANSLATIONMANAGER_H 1

#pragma once

#include "UICDMHandles.h"
#include "UICDMTimeline.h"

#include "Doc.h"

//==============================================================================
//	Classes
//==============================================================================
class ITimelineItemBinding;
class ITimelineItemProperty;
class CUICDMTimelineItemBinding;
class CBaseStateRow;
class CPropertyRow;
class CKeyframesManager;
class IBreadCrumbProvider;

// Link to data model
class CAsset;
class IUICDMSelectable;
class CClientDataModelBridge;

// UICDM
namespace UICDM {
class CStudioSystem;
class ISignalConnection;
}

class CDoc;

//=============================================================================
/**
 * There is a TranslationManager per presentation (project)
 */
class CTimelineTranslationManager
{
protected: // Typedefs
    // UICDM support
    typedef std::map<UICDM::CUICDMInstanceHandle, CUICDMTimelineItemBinding *>
        TInstanceHandleBindingMap;

    // Store expanded state
    typedef std::map<UICDM::CUICDMInstanceHandle, bool> TInstanceHandleExpandedMap; // UICDM support

protected: // Properties
    // UICDM support
    TInstanceHandleBindingMap m_InstanceHandleBindingMap;

    CKeyframesManager *m_KeyframesManager;
    IBreadCrumbProvider *m_BreadCrumbProvider;
    std::vector<std::shared_ptr<UICDM::ISignalConnection>>
        m_Connections; /// connections to the UICDM

    TInstanceHandleExpandedMap m_InstanceHandleExpandedMap;

public:
    CTimelineTranslationManager();
    ~CTimelineTranslationManager();

public:
    ITimelineItemBinding *GetOrCreate(UICDM::CUICDMInstanceHandle inInstance);
    void CreateNewPropertyRow(ITimelineItemProperty *inTimelineItemPropertyBinding,
                              CBaseStateRow *inParentRow, CPropertyRow *inNextRow);
    void RemovePropertyRow(ITimelineItemProperty *inTimelineItemPropertyBinding);

    void Clear();
    void Unregister(ITimelineItemBinding *inTimelineItem);

    CKeyframesManager *GetKeyframesManager() const;
    IBreadCrumbProvider *GetBreadCrumbProvider() const;
    CBaseStateRow *GetSelectedRow() const;
    long GetCurrentViewTime() const;
    CUICDMTimelineItemBinding *GetBinding(UICDM::CUICDMInstanceHandle inHandle) const;
    CUICDMTimelineItemBinding *GetSelectedBinding() const;

    void ClearKeyframeSelection();
    void OnNewPresentation();
    void OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);

    UICDM::CStudioSystem *GetStudioSystem() const;

    // UICDM callback
    void OnAnimationCreated(UICDM::CUICDMInstanceHandle inInstance,
                            UICDM::CUICDMPropertyHandle inProperty);
    void OnAnimationDeleted(UICDM::CUICDMInstanceHandle inInstance,
                            UICDM::CUICDMPropertyHandle inProperty);
    void OnPropertyLinked(UICDM::CUICDMInstanceHandle inInstance,
                          UICDM::CUICDMPropertyHandle inProperty);
    void OnPropertyUnlinked(UICDM::CUICDMInstanceHandle inInstance,
                            UICDM::CUICDMPropertyHandle inProperty);
    void RefreshKeyframe(UICDM::CUICDMAnimationHandle inAnimation,
                         UICDM::CUICDMKeyframeHandle inKeyframe,
                         ETimelineKeyframeTransaction inTransaction);
    void OnKeyframeInserted(UICDM::CUICDMAnimationHandle inAnimation,
                            UICDM::CUICDMKeyframeHandle inKeyframe);
    void OnKeyframeDeleted(UICDM::CUICDMAnimationHandle inAnimation,
                           UICDM::CUICDMKeyframeHandle inKeyframe);
    void OnKeyframeUpdated(UICDM::CUICDMKeyframeHandle inKeyframe);
    void OnPropertyChanged(UICDM::CUICDMInstanceHandle inInstance,
                           UICDM::CUICDMPropertyHandle inProperty);
    void OnDynamicKeyframeChanged(UICDM::CUICDMAnimationHandle inAnimation, bool inDynamic);

    void OnAssetCreated(UICDM::CUICDMInstanceHandle inInstance);
    void OnAssetDeleted(UICDM::CUICDMInstanceHandle inInstance);
    void OnChildAdded(int inParent, int inChild, long inIndex);
    void OnChildRemoved(int inParent, int inChild, long inIndex);
    void OnChildMoved(int inParent, int inChild, long inOldIndex, long inNewIndex);

    void OnActionEvent(UICDM::CUICDMActionHandle inAction, UICDM::CUICDMSlideHandle inSlide,
                       UICDM::CUICDMInstanceHandle inOwner);

    // Helper function to iterate over all bindings
    void ClearBindingsKeyframeSelection();
    CDoc *GetDoc() const;

    // Store expanded state
    bool IsExpanded(UICDM::CUICDMInstanceHandle inInstance) const;
    void SetExpanded(UICDM::CUICDMInstanceHandle inInstance, bool inExpanded);

protected:
    void SetSelected(Q3DStudio::SSelectedValue inSelectable, bool inSelected);
    ITimelineItemBinding *EnsureLoaded(UICDM::CUICDMInstanceHandle inHandle);
};

#endif // INCLUDED_TIMELINE_TRANSLATIONMANAGER_H