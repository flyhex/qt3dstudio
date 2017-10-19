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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "TimelineTranslationManager.h"
#include "SlideTimelineItemBinding.h"
#include "GroupTimelineItemBinding.h"
#include "BehaviorTimelineItemBinding.h"
#include "MaterialTimelineItemBinding.h"
#include "ImageTimelineItemBinding.h"
#include "PathAnchorPointTimelineItemBinding.h"
#include "PathTimelineItemBinding.h"
#include "LayerTimelineItemBinding.h"
#include "KeyframesManager.h"
#include "TimelineBreadCrumbProvider.h"
#include "BaseStateRow.h"
#include "PropertyRow.h"
#include "IDoc.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMSignals.h"

// Link to Data model
#include "ClientDataModelBridge.h"
#include "Qt3DSDMDataCore.h"
#include "Doc.h" //Because we need to access Client Data Model Bridge
#include "StudioApp.h"
#include "Core.h"

using namespace qt3dsdm;

CTimelineTranslationManager::CTimelineTranslationManager()
{
    m_KeyframesManager = new CKeyframesManager(this);
    m_BreadCrumbProvider = new CTimelineBreadCrumbProvider(g_StudioApp.GetCore()->GetDoc());
}

CTimelineTranslationManager::~CTimelineTranslationManager()
{
    // clean up all bindings
    Clear();
    m_Connections.clear();
    delete m_KeyframesManager;
    delete m_BreadCrumbProvider;
}

ITimelineItemBinding *CTimelineTranslationManager::GetOrCreate(Qt3DSDMInstanceHandle inInstance)
{
    ITimelineItemBinding *theBinding = GetBinding(inInstance);
    if (!theBinding) {
        Qt3DSDMTimelineItemBinding *theReturn = nullptr;
        qt3dsdm::IPropertySystem *thePropertySystem = GetStudioSystem()->GetPropertySystem();
        Qt3DSDMPropertyHandle theTypeProperty =
            thePropertySystem->GetAggregateInstancePropertyByName(inInstance, L"type");

        SValue theTypeValue;
        thePropertySystem->GetInstancePropertyValue(inInstance, theTypeProperty, theTypeValue);

        std::wstring theWideTypeString(qt3dsdm::get<TDataStrPtr>(theTypeValue)->GetData());

        if (theWideTypeString == L"Material" || theWideTypeString == L"CustomMaterial"
            || theWideTypeString == L"ReferencedMaterial")
            theReturn = new CMaterialTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Image")
            theReturn = new CImageTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Group" || theWideTypeString == L"Component")
            theReturn = new CGroupTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Behavior")
            theReturn = new CBehaviorTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Slide")
            theReturn = new CSlideTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"PathAnchorPoint")
            theReturn = new CPathAnchorPointTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Path")
            theReturn = new CPathTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Layer")
            theReturn = new CLayerTimelineItemBinding(this, inInstance);
        else if (theWideTypeString == L"Model" || theWideTypeString == L"Text"
                 || theWideTypeString == L"Camera" || theWideTypeString == L"Effect"
                 || theWideTypeString == L"Light" || theWideTypeString == L"RenderPlugin"
                 || theWideTypeString == L"Alias" || theWideTypeString == L"SubPath")
            theReturn = new Qt3DSDMTimelineItemBinding(this, inInstance);
        else {
            // Add support for additional UICDM types here.
            ASSERT(0);
        }

        m_InstanceHandleBindingMap.insert(
            std::make_pair(theReturn->GetInstanceHandle(), theReturn));
        m_InstanceHandleExpandedMap.insert(std::make_pair(theReturn->GetInstanceHandle(), false));
        theBinding = theReturn;
    }

    return theBinding;
}

//==============================================================================
/**
 * Create a new CPropertyRow that maps to this ITimelineItemProperty.
 * The caller is assumed to have ensured that this is only called once per property binding.
 */
void CTimelineTranslationManager::CreateNewPropertyRow(
    ITimelineItemProperty *inTimelineItemPropertyBinding, CBaseStateRow *inParentRow,
    CPropertyRow *inNextRow)
{
    if (!inParentRow || !inTimelineItemPropertyBinding)
        return;

    CPropertyRow *theNewRow = new CPropertyRow(inTimelineItemPropertyBinding);
    inParentRow->AddPropertyRow(theNewRow, inNextRow);
    inTimelineItemPropertyBinding->Bind(theNewRow);
}

//==============================================================================
/**
 * Does the reverse of CreateNewPropertyRow, when a property has been de-animated.
 */
void CTimelineTranslationManager::RemovePropertyRow(
    ITimelineItemProperty *inTimelineItemPropertyBinding)
{
    CPropertyRow *theRow = nullptr;
    if (!inTimelineItemPropertyBinding
        || (theRow = inTimelineItemPropertyBinding->GetRow()) == nullptr)
        return;

    CBaseStateRow *theParentRow = theRow->GetParentRow();
    if (theParentRow) {
        inTimelineItemPropertyBinding->Release();
        theParentRow->RemovePropertyRow(theRow); // this implicitly delete the row
    }
}

//==============================================================================
/**
 * Clear all bindings, typically when a presentation is closed.
 */
void CTimelineTranslationManager::Clear()
{
    // clean up all bindings
    m_InstanceHandleBindingMap.clear();
}

//==============================================================================
/**
 * Called when the associated UI is no longer valid.
 */
void CTimelineTranslationManager::Unregister(ITimelineItemBinding *inTimelineItem)
{
    // UICDM
    bool theDeselectItem = false;
    TInstanceHandleBindingMap::iterator theInstanceIter = m_InstanceHandleBindingMap.begin();
    for (; theInstanceIter != m_InstanceHandleBindingMap.end(); ++theInstanceIter) {
        if (theInstanceIter->second == inTimelineItem) {
            // If this is the currently selected object and make sure that is cleared.
            qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance =
                g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance();
            if (theSelectedInstance.Valid() && theSelectedInstance == theInstanceIter->first)
                theDeselectItem = true;

            m_InstanceHandleBindingMap.erase(theInstanceIter);
            break;
        }
    }

    delete inTimelineItem;
}

CKeyframesManager *CTimelineTranslationManager::GetKeyframesManager() const
{
    return m_KeyframesManager;
}

IBreadCrumbProvider *CTimelineTranslationManager::GetBreadCrumbProvider() const
{
    return m_BreadCrumbProvider;
}

CBaseStateRow *CTimelineTranslationManager::GetSelectedRow() const
{
    ITimelineItemBinding *theBinding = GetSelectedBinding();
    if (theBinding)
        return theBinding->GetRow();
    return nullptr;
}

long CTimelineTranslationManager::GetCurrentViewTime() const
{
    return g_StudioApp.GetCore()->GetDoc()->GetCurrentViewTime();
}

//==============================================================================
/**
 * @return the Binding object that corresponds to this instance.
 */
Qt3DSDMTimelineItemBinding *
CTimelineTranslationManager::GetBinding(Qt3DSDMInstanceHandle inHandle) const
{
    TInstanceHandleBindingMap::const_iterator theIter = m_InstanceHandleBindingMap.find(inHandle);
    if (theIter != m_InstanceHandleBindingMap.end())
        return theIter->second;
    return nullptr;
}

Qt3DSDMTimelineItemBinding *CTimelineTranslationManager::GetSelectedBinding() const
{
    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance =
        g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance();
    if (theSelectedInstance.Valid()) {
        Qt3DSDMTimelineItemBinding *theBinding = GetBinding(theSelectedInstance);
        return theBinding;
    }
    return nullptr;
}

//==============================================================================
/**
 * Triggered from individual binding classes, to clear all keyframe selection
 */
void CTimelineTranslationManager::ClearKeyframeSelection()
{
    m_KeyframesManager->DeselectAllKeyframes();
}

//==============================================================================
/**
 * Set up callbacks for animation changes
 */
void CTimelineTranslationManager::OnNewPresentation()
{
    m_Connections.clear();
    m_InstanceHandleExpandedMap.clear();

    IStudioFullSystemSignalProvider *theSignalProvider =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();
    m_Connections.push_back(theSignalProvider->ConnectAnimationCreated(
        std::bind(&CTimelineTranslationManager::OnAnimationCreated, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectAnimationDeleted(
        std::bind(&CTimelineTranslationManager::OnAnimationDeleted, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectPropertyLinked(
        std::bind(&CTimelineTranslationManager::OnPropertyLinked, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectPropertyUnlinked(
        std::bind(&CTimelineTranslationManager::OnPropertyUnlinked, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectKeyframeInserted(
        std::bind(&CTimelineTranslationManager::OnKeyframeInserted, this,
             std::placeholders::_1, std::placeholders::_2)));
    m_Connections.push_back(theSignalProvider->ConnectKeyframeErased(
        std::bind(&CTimelineTranslationManager::OnKeyframeDeleted, this,
             std::placeholders::_1, std::placeholders::_2)));
    m_Connections.push_back(theSignalProvider->ConnectKeyframeUpdated(
        std::bind(&CTimelineTranslationManager::OnKeyframeUpdated, this, std::placeholders::_1)));
    m_Connections.push_back(theSignalProvider->ConnectInstancePropertyValue(
        std::bind(&CTimelineTranslationManager::OnPropertyChanged, this,
             std::placeholders::_1, std::placeholders::_2)));
    m_Connections.push_back(theSignalProvider->ConnectFirstKeyframeDynamicSet(
        std::bind(&CTimelineTranslationManager::OnDynamicKeyframeChanged, this,
             std::placeholders::_1, std::placeholders::_2)));

    m_Connections.push_back(theSignalProvider->ConnectInstanceCreated(
        std::bind(&CTimelineTranslationManager::OnAssetCreated, this, std::placeholders::_1)));
    m_Connections.push_back(theSignalProvider->ConnectInstanceDeleted(
        std::bind(&CTimelineTranslationManager::OnAssetDeleted, this, std::placeholders::_1)));

    m_Connections.push_back(theSignalProvider->ConnectActionCreated(
        std::bind(&CTimelineTranslationManager::OnActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectActionDeleted(
        std::bind(&CTimelineTranslationManager::OnActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

    Q3DStudio::CGraph &theGraph(*g_StudioApp.GetCore()->GetDoc()->GetAssetGraph());
    m_Connections.push_back(theGraph.ConnectChildAdded(
        std::bind(&CTimelineTranslationManager::OnChildAdded, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theGraph.ConnectChildRemoved(
        std::bind(&CTimelineTranslationManager::OnChildRemoved, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theGraph.ConnectChildMoved(
        std::bind(&CTimelineTranslationManager::OnChildMoved, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
}

//==============================================================================
/**
 * Selection events on the old data model was triggered via signals on the actual objects.
 * For the new data model, it would be via this OnSelectionChange event.
 */
void CTimelineTranslationManager::OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    // Deselect all items
    TInstanceHandleBindingMap::const_iterator theIter = m_InstanceHandleBindingMap.begin();
    for (; theIter != m_InstanceHandleBindingMap.end(); ++theIter) {
        ITimelineItemBinding *theBinding = theIter->second;
        CBaseStateRow *theRow = theBinding->GetRow();
        if (theRow)
            theRow->OnSelected(false);
    }

    // Select new
    if (inNewSelectable)
        SetSelected(inNewSelectable, true);
}

CDoc *CTimelineTranslationManager::GetDoc() const
{
    return dynamic_cast<CDoc *>(g_StudioApp.GetCore()->GetDoc());
}

CStudioSystem *CTimelineTranslationManager::GetStudioSystem() const
{
    // TODO: figure if we can just deal with IDoc instead of CDoc
    return g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
}

void CTimelineTranslationManager::OnAnimationCreated(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty)
{
    Qt3DSDMTimelineItemBinding *theTimelineBinding =
        dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(inInstance));
    if (theTimelineBinding)
        theTimelineBinding->AddPropertyRow(inProperty);
}

void CTimelineTranslationManager::OnAnimationDeleted(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty)
{
    Qt3DSDMTimelineItemBinding *theTimelineBinding =
        dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(inInstance));
    if (theTimelineBinding)
        theTimelineBinding->RemovePropertyRow(inProperty);
}

void CTimelineTranslationManager::OnPropertyLinked(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty)
{
    Qt3DSDMTimelineItemBinding *theTimelineBinding =
        dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(inInstance));
    if (theTimelineBinding)
        theTimelineBinding->OnPropertyLinked(inProperty);
}

void CTimelineTranslationManager::OnPropertyUnlinked(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty)
{
    OnPropertyLinked(inInstance, inProperty);
}

void CTimelineTranslationManager::RefreshKeyframe(Qt3DSDMAnimationHandle inAnimation,
                                                  Qt3DSDMKeyframeHandle inKeyframe,
                                                  ETimelineKeyframeTransaction inTransaction)
{
    Qt3DSDMTimelineItemBinding *theTimelineBinding = nullptr;
    if (GetStudioSystem()->GetAnimationCore()->AnimationValid(inAnimation)) {
        SAnimationInfo theAnimationInfo =
            GetStudioSystem()->GetAnimationCore()->GetAnimationInfo(inAnimation);
        theTimelineBinding =
            dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(theAnimationInfo.m_Instance));

        if (theTimelineBinding)
            theTimelineBinding->RefreshPropertyKeyframe(theAnimationInfo.m_Property, inKeyframe,
                                                        inTransaction);
    }
    // else, animation has been nuked, ignore this event, we'll get a AnimationDelete
}

void CTimelineTranslationManager::OnKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                                     Qt3DSDMKeyframeHandle inKeyframe)
{
    RefreshKeyframe(inAnimation, inKeyframe, ETimelineKeyframeTransaction_Add);
}

void CTimelineTranslationManager::OnKeyframeDeleted(Qt3DSDMAnimationHandle inAnimation,
                                                    Qt3DSDMKeyframeHandle inKeyframe)
{
    RefreshKeyframe(inAnimation, inKeyframe, ETimelineKeyframeTransaction_Delete);
}

void CTimelineTranslationManager::OnKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe)
{
    IAnimationCore *theAnimationCore = GetStudioSystem()->GetAnimationCore();
    if (theAnimationCore->KeyframeValid(inKeyframe)) {
        Qt3DSDMAnimationHandle theAnimationHandle =
            theAnimationCore->GetAnimationForKeyframe(inKeyframe);
        RefreshKeyframe(theAnimationHandle, inKeyframe, ETimelineKeyframeTransaction_Update);
    }
    // else, keyframe has been nuked, ignore this event, we'll get a KeyframeDeleted
}

void CTimelineTranslationManager::OnPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                    qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    Qt3DSDMTimelineItemBinding *theTimelineBinding =
        dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(inInstance));
    if (theTimelineBinding)
        theTimelineBinding->OnPropertyChanged(inProperty);
}

void CTimelineTranslationManager::OnDynamicKeyframeChanged(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                                                           bool inDynamic)
{
    Q_UNUSED(inDynamic);

    Qt3DSDMTimelineItemBinding *theTimelineBinding = nullptr;
    if (GetStudioSystem()->GetAnimationCore()->AnimationValid(inAnimation)) {
        SAnimationInfo theAnimationInfo =
            GetStudioSystem()->GetAnimationCore()->GetAnimationInfo(inAnimation);
        theTimelineBinding =
            dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(theAnimationInfo.m_Instance));
        if (theTimelineBinding)
            theTimelineBinding->RefreshPropertyKeyframe(
                theAnimationInfo.m_Property, 0, ETimelineKeyframeTransaction_DynamicChanged);
    }
}

void CTimelineTranslationManager::OnAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    CClientDataModelBridge *theDataModelBridge = GetStudioSystem()->GetClientDataModelBridge();

    if (theDataModelBridge->IsSceneGraphInstance(inInstance))
        EnsureLoaded(inInstance);
}

void CTimelineTranslationManager::OnAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    // You can't assume the instance is valid.  Someone may have deleted a large number of items
    // from the model and then decided to send notifications after the fact.
    // if the created asset is library asset, do nothing
    // start to add the scene asset to the timeline
    Qt3DSDMTimelineItemBinding *theItemBinding =
        dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(inInstance));
    if (theItemBinding) {
        Qt3DSDMTimelineItemBinding *theParentBinding =
            dynamic_cast<Qt3DSDMTimelineItemBinding *>(theItemBinding->GetParent());
        if (theParentBinding)
            theParentBinding->OnDeleteChild(inInstance);
    }
}

void CTimelineTranslationManager::OnChildAdded(int /*inParent*/, int inChild, long /*inIndex*/)
{
    OnAssetCreated(inChild);
}
void CTimelineTranslationManager::OnChildRemoved(int /*inParent*/, int inChild, long /*inIndex*/)
{
    OnAssetDeleted(inChild);
}
void CTimelineTranslationManager::OnChildMoved(int /*inParent*/, int inChild, long /*inOldIndex*/,
                                               long /*inNewIndex*/)
{
    OnAssetDeleted(inChild);
    OnAssetCreated(inChild);
}

//==============================================================================
/**
 * Callback method whenever an action is either created or removed.
 * Basically, it tells the owner of the action to update its timeline control to
 * update the icon that shows action association status
 */
void CTimelineTranslationManager::OnActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction,
                                                qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                                qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    Q_UNUSED(inAction);

    // the slide that action is added to is the current slide or
    // is added to the master slide of the current slide
    Qt3DSDMTimelineItemBinding *theTimelineBinding =
        dynamic_cast<Qt3DSDMTimelineItemBinding *>(GetBinding(inOwner));
    if (theTimelineBinding)
        theTimelineBinding->UpdateActionStatus();
}

//==============================================================================
/**
 * Helper functions to go through ALL binding and clear any keyframes selection.
 */
void CTimelineTranslationManager::ClearBindingsKeyframeSelection()
{
    // UICDM bindings handle their own selections
    TInstanceHandleBindingMap::const_iterator theIter = m_InstanceHandleBindingMap.begin();
    for (; theIter != m_InstanceHandleBindingMap.end(); ++theIter)
        theIter->second->DoSelectKeyframes(false, -1, true);
}

//==============================================================================
/**
 * Helper function to find the binding that corresponds to inSelectable and set its selection state
 */
void CTimelineTranslationManager::SetSelected(Q3DStudio::SSelectedValue inSelectable,
                                              bool inSelected)
{
    qt3dsdm::TInstanceHandleList theInstances = inSelectable.GetSelectedInstances();
    for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
        Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
        if (GetStudioSystem()->IsInstance(theInstance)) {
            ITimelineItemBinding *theBinding = EnsureLoaded(theInstance);
            if (theBinding) {
                CBaseStateRow *theRow = theBinding->GetRow();
                if (theRow)
                    theRow->OnSelected(inSelected);
            }
        }
    }
}

ITimelineItemBinding *CTimelineTranslationManager::EnsureLoaded(Qt3DSDMInstanceHandle inHandle)
{
    ITimelineItemBinding *theBinding = GetBinding(inHandle);
    bool rowLoaded = theBinding != nullptr && theBinding->GetRow() != nullptr;
    if (rowLoaded == false) {
        // tell my parent to load me
        CClientDataModelBridge *theDataModelBridge = GetStudioSystem()->GetClientDataModelBridge();
        Qt3DSDMInstanceHandle theParent = theDataModelBridge->GetParentInstance(inHandle);
        if (theParent.Valid()) {
            ITimelineItemBinding *theParentBinding = EnsureLoaded(theParent);
            if (theParentBinding)
                theParentBinding->GetRow()->LoadChildren();

            // The LoadChildren has an optimzation such that if it's already loaded, it won't
            // recreate again
            // So, if we still can't get the binding after LoadChildren, it very likely means that
            // this is newly added
            // so call OnAddChild to let it just add this.
            theBinding = GetBinding(inHandle);
            bool rowLoaded = theBinding != nullptr && theBinding->GetRow() != nullptr;
            if (theParentBinding && rowLoaded == false) {
                // start to add the scene asset to the timeline
                Qt3DSDMTimelineItemBinding *theUICDMBinding =
                    dynamic_cast<Qt3DSDMTimelineItemBinding *>(theParentBinding);
                theUICDMBinding->OnAddChild(inHandle);
                theBinding = GetBinding(inHandle);
            }
        }
    }
    return theBinding;
}

//==============================================================================
/**
 * remember the expanded state for the current presentation
 */
bool CTimelineTranslationManager::IsExpanded(Qt3DSDMInstanceHandle inInstance) const
{
    TInstanceHandleExpandedMap::const_iterator theIter =
        m_InstanceHandleExpandedMap.find(inInstance);
    if (theIter != m_InstanceHandleExpandedMap.end()) {
        return theIter->second;
    }
    return false;
}

//==============================================================================
/**
 * remember the expanded state for the current presentation
 */
void CTimelineTranslationManager::SetExpanded(Qt3DSDMInstanceHandle inInstance, bool inExpanded)
{
    TInstanceHandleExpandedMap::iterator theIter = m_InstanceHandleExpandedMap.find(inInstance);
    if (theIter != m_InstanceHandleExpandedMap.end()) {
        theIter->second = inExpanded;
    }
}
