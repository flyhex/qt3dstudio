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

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMTimeline.h"

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
namespace qt3dsdm {
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
    typedef std::map<qt3dsdm::Qt3DSDMInstanceHandle, CUICDMTimelineItemBinding *>
        TInstanceHandleBindingMap;

    // Store expanded state
    typedef std::map<qt3dsdm::Qt3DSDMInstanceHandle, bool> TInstanceHandleExpandedMap; // UICDM support

protected: // Properties
    // UICDM support
    TInstanceHandleBindingMap m_InstanceHandleBindingMap;

    CKeyframesManager *m_KeyframesManager;
    IBreadCrumbProvider *m_BreadCrumbProvider;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>>
        m_Connections; /// connections to the UICDM

    TInstanceHandleExpandedMap m_InstanceHandleExpandedMap;

public:
    CTimelineTranslationManager();
    ~CTimelineTranslationManager();

public:
    ITimelineItemBinding *GetOrCreate(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void CreateNewPropertyRow(ITimelineItemProperty *inTimelineItemPropertyBinding,
                              CBaseStateRow *inParentRow, CPropertyRow *inNextRow);
    void RemovePropertyRow(ITimelineItemProperty *inTimelineItemPropertyBinding);

    void Clear();
    void Unregister(ITimelineItemBinding *inTimelineItem);

    CKeyframesManager *GetKeyframesManager() const;
    IBreadCrumbProvider *GetBreadCrumbProvider() const;
    CBaseStateRow *GetSelectedRow() const;
    long GetCurrentViewTime() const;
    CUICDMTimelineItemBinding *GetBinding(qt3dsdm::Qt3DSDMInstanceHandle inHandle) const;
    CUICDMTimelineItemBinding *GetSelectedBinding() const;

    void ClearKeyframeSelection();
    void OnNewPresentation();
    void OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);

    qt3dsdm::CStudioSystem *GetStudioSystem() const;

    // UICDM callback
    void OnAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void OnAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void OnPropertyLinked(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                          qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void OnPropertyUnlinked(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void RefreshKeyframe(qt3dsdm::CUICDMAnimationHandle inAnimation,
                         qt3dsdm::CUICDMKeyframeHandle inKeyframe,
                         ETimelineKeyframeTransaction inTransaction);
    void OnKeyframeInserted(qt3dsdm::CUICDMAnimationHandle inAnimation,
                            qt3dsdm::CUICDMKeyframeHandle inKeyframe);
    void OnKeyframeDeleted(qt3dsdm::CUICDMAnimationHandle inAnimation,
                           qt3dsdm::CUICDMKeyframeHandle inKeyframe);
    void OnKeyframeUpdated(qt3dsdm::CUICDMKeyframeHandle inKeyframe);
    void OnPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void OnDynamicKeyframeChanged(qt3dsdm::CUICDMAnimationHandle inAnimation, bool inDynamic);

    void OnAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void OnAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void OnChildAdded(int inParent, int inChild, long inIndex);
    void OnChildRemoved(int inParent, int inChild, long inIndex);
    void OnChildMoved(int inParent, int inChild, long inOldIndex, long inNewIndex);

    void OnActionEvent(qt3dsdm::CUICDMActionHandle inAction, qt3dsdm::CUICDMSlideHandle inSlide,
                       qt3dsdm::Qt3DSDMInstanceHandle inOwner);

    // Helper function to iterate over all bindings
    void ClearBindingsKeyframeSelection();
    CDoc *GetDoc() const;

    // Store expanded state
    bool IsExpanded(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    void SetExpanded(qt3dsdm::Qt3DSDMInstanceHandle inInstance, bool inExpanded);

protected:
    void SetSelected(Q3DStudio::SSelectedValue inSelectable, bool inSelected);
    ITimelineItemBinding *EnsureLoaded(qt3dsdm::Qt3DSDMInstanceHandle inHandle);
};

#endif // INCLUDED_TIMELINE_TRANSLATIONMANAGER_H