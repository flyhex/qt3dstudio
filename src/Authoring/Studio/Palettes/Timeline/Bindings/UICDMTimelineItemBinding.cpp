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
#include "UICDMTimelineItemBinding.h"
#include "TimelineTranslationManager.h"
#include "TimeEditDlg.h"
#include "EmptyTimelineTimebar.h"
#include "UICDMTimelineTimebar.h"
#include "BaseStateRow.h"
#include "BaseTimebarlessRow.h"
#include "PropertyTimebarRow.h"
#include "PropertyRow.h"
#include "KeyframesManager.h"
#include "StudioApp.h"
#include "Core.h"
#include "Dialogs.h"
#include "GraphUtils.h"
#include "UICDMDataCore.h"
#include "Strings.h"
#include "StringLoader.h"

// Data model specific
#include "IDoc.h"
#include "ClientDataModelBridge.h"
#include "Dispatch.h"
#include "DropSource.h"
#include "UICDMTimelineItemProperty.h"
#include "UICDMSlides.h"
#include "UICDMStudioSystem.h"
#include "UICDMSlideGraphCore.h"
#include "UICDMActionCore.h"
#include "UICDMAnimation.h"
#include "CmdDataModelChangeKeyframe.h"
#include "RelativePathTools.h"
#include "IDocumentEditor.h"
#include "UICFileTools.h"
#include "ImportUtils.h"

#include <QMessageBox>

using namespace qt3dsdm;

CUICDMTimelineItemBinding::CUICDMTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                                     CUICDMInstanceHandle inDataHandle)
    : m_Row(nullptr)
    , m_TransMgr(inMgr)
    , m_DataHandle(inDataHandle)
    , m_Parent(nullptr)
    , m_TimelineTimebar(nullptr)

{
    m_StudioSystem = m_TransMgr->GetStudioSystem();
    m_TransMgr->GetDoc()->GetCore()->GetDispatch()->AddDataModelListener(this);
}

CUICDMTimelineItemBinding::CUICDMTimelineItemBinding(CTimelineTranslationManager *inMgr)
    : m_Row(nullptr)
    , m_TransMgr(inMgr)
    , m_DataHandle(0)
    , m_Parent(nullptr)
    , m_TimelineTimebar(nullptr)
{
    m_StudioSystem = m_TransMgr->GetStudioSystem();
    m_TransMgr->GetDoc()->GetCore()->GetDispatch()->AddDataModelListener(this);
}

CUICDMTimelineItemBinding::~CUICDMTimelineItemBinding()
{
    RemoveAllPropertyBindings();
    delete m_TimelineTimebar;
    m_TransMgr->GetDoc()->GetCore()->GetDispatch()->RemoveDataModelListener(this);
}

// helpers
bool CUICDMTimelineItemBinding::UICDMGetBoolean(qt3dsdm::CUICDMPropertyHandle inProperty) const
{
    qt3dsdm::IPropertySystem *thePropertySystem = m_StudioSystem->GetPropertySystem();
    SValue theValue;
    thePropertySystem->GetInstancePropertyValue(m_DataHandle, inProperty, theValue);
    return qt3dsdm::get<bool>(theValue);
}

void CUICDMTimelineItemBinding::UICDMSetBoolean(qt3dsdm::CUICDMPropertyHandle inProperty,
                                                bool inValue, const QString &inNiceText) const
{
    CDoc *theDoc = dynamic_cast<CDoc *>(g_StudioApp.GetCore()->GetDoc());
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*theDoc, inNiceText)
        ->SetInstancePropertyValue(m_DataHandle, inProperty, inValue);
}

void CUICDMTimelineItemBinding::SetInstanceHandle(qt3dsdm::CUICDMInstanceHandle inDataHandle)
{
    m_DataHandle = inDataHandle;
}

EStudioObjectType CUICDMTimelineItemBinding::GetObjectType() const
{
    return m_StudioSystem->GetClientDataModelBridge()->GetObjectType(m_DataHandle);
}

bool CUICDMTimelineItemBinding::IsMaster() const
{
    CDoc *theDoc = dynamic_cast<CDoc *>(g_StudioApp.GetCore()->GetDoc());
    Q3DStudio::IDocumentReader &theReader(theDoc->GetDocumentReader());
    if (GetObjectType() == OBJTYPE_IMAGE) {
        CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
        CUICDMInstanceHandle theParent;
        CUICDMPropertyHandle theProperty;
        bool isPropertyLinked;

        theBridge->GetMaterialFromImageInstance(GetInstance(), theParent, theProperty);
        isPropertyLinked = theReader.IsPropertyLinked(theParent, theProperty);

        // Also check light probe
        if (!isPropertyLinked) {
            theBridge->GetLayerFromImageProbeInstance(GetInstance(), theParent, theProperty);
            isPropertyLinked = theReader.IsPropertyLinked(theParent, theProperty);
        }

        return isPropertyLinked;
    }
    CUICDMInstanceHandle theQueryHandle(m_DataHandle);
    if (GetObjectType() == OBJTYPE_PATHANCHORPOINT)
        theQueryHandle = theReader.GetParent(m_DataHandle);

    // logic: you can't unlink name, so if name is linked then, this is master.
    CUICDMPropertyHandle theNamePropHandle =
        m_StudioSystem->GetPropertySystem()->GetAggregateInstancePropertyByName(theQueryHandle,
                                                                                L"name");
    return theReader.IsPropertyLinked(theQueryHandle, theNamePropHandle);
}

bool CUICDMTimelineItemBinding::IsShy() const
{
    return UICDMGetBoolean(m_StudioSystem->GetClientDataModelBridge()->GetSceneAsset().m_Shy);
}
void CUICDMTimelineItemBinding::SetShy(bool inShy)
{
    UICDMSetBoolean(m_StudioSystem->GetClientDataModelBridge()->GetSceneAsset().m_Shy, inShy,
                    QObject::tr("Shy Toggle"));
}
bool CUICDMTimelineItemBinding::IsLocked() const
{
    return UICDMGetBoolean(m_StudioSystem->GetClientDataModelBridge()->GetSceneAsset().m_Locked);
}

void ToggleChildrenLock(Q3DStudio::ScopedDocumentEditor &scopedDocEditor,
                        CUICDMTimelineItemBinding *inTimelineItemBinding,
                        SDataModelSceneAsset inSceneAsset, bool inLocked)
{
    scopedDocEditor->SetInstancePropertyValue(inTimelineItemBinding->GetInstanceHandle(),
                                              inSceneAsset.m_Locked, inLocked);
    long childrenCount = inTimelineItemBinding->GetChildrenCount();
    if (childrenCount == 0)
        return;
    for (long i = 0; i < childrenCount; ++i) {
        CUICDMTimelineItemBinding *child =
            static_cast<CUICDMTimelineItemBinding *>(inTimelineItemBinding->GetChild(i));
        ToggleChildrenLock(scopedDocEditor, child, inSceneAsset, inLocked);
    }
}

void CUICDMTimelineItemBinding::SetLocked(bool inLocked)
{
    CDoc *theDoc = dynamic_cast<CDoc *>(g_StudioApp.GetCore()->GetDoc());
    Q3DStudio::ScopedDocumentEditor scopedDocEditor(*theDoc, L"SetLock", __FILE__, __LINE__);

    SDataModelSceneAsset sceneAsset = m_StudioSystem->GetClientDataModelBridge()->GetSceneAsset();
    ToggleChildrenLock(scopedDocEditor, this, sceneAsset, inLocked);

    if (inLocked)
        g_StudioApp.GetCore()->GetDoc()->NotifySelectionChanged();
}

bool CUICDMTimelineItemBinding::IsVisible() const
{
    return UICDMGetBoolean(m_StudioSystem->GetClientDataModelBridge()->GetSceneAsset().m_Eyeball);
}

void CUICDMTimelineItemBinding::SetVisible(bool inVisible)
{
    UICDMSetBoolean(m_StudioSystem->GetClientDataModelBridge()->GetSceneAsset().m_Eyeball,
                    inVisible, QObject::tr("Visibility Toggle"));
}

// remember the expanded state for the current presentation
bool CUICDMTimelineItemBinding::IsExpanded() const
{
    return m_TransMgr->IsExpanded(m_DataHandle);
}
// remember the expanded state for the current presentation
void CUICDMTimelineItemBinding::SetExpanded(bool inExpanded)
{
    m_TransMgr->SetExpanded(m_DataHandle, inExpanded);
}

bool CUICDMTimelineItemBinding::HasAction(bool inMaster)
{
    TActionHandleList theActions;
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();

    CUICDMSlideHandle theSlide = theDoc->GetActiveSlide();
    qt3dsdm::ISlideCore &theSlideCore(*m_StudioSystem->GetSlideCore());
    if (theSlideCore.IsSlide(theSlide)) {
        if (inMaster)
            theSlide =
                m_StudioSystem->GetSlideSystem()->GetMasterSlide(theSlide); // use the master slide

        m_StudioSystem->GetActionCore()->GetActions(theSlide, m_DataHandle, theActions);
    }
    return theActions.size() > 0;
}

bool CUICDMTimelineItemBinding::ChildrenHasAction(bool inMaster)
{
    // Get all the instances in this slidegraph
    // check whehter it's an action instance and is in the slide of interst
    // check also it's owner is a descendent of the viewed instances
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    IActionCore *theActionCore(m_StudioSystem->GetActionCore());
    CClientDataModelBridge *theBridge(m_StudioSystem->GetClientDataModelBridge());

    CUICDMSlideHandle theSlide = theDoc->GetActiveSlide();
    qt3dsdm::ISlideCore &theSlideCore(*m_StudioSystem->GetSlideCore());
    if (theSlideCore.IsSlide(theSlide)) {
        if (inMaster)
            theSlide =
                m_StudioSystem->GetSlideSystem()->GetMasterSlide(theSlide); // use the master slide

        TSlideInstancePairList theGraphInstances;
        m_StudioSystem->GetSlideSystem()->GetAssociatedInstances(theSlide, theGraphInstances);

        qt3dsdm::CUICDMInstanceHandle theObservedInstance = GetInstance();
        if (theObservedInstance.Valid()) {
            for (TSlideInstancePairList::const_iterator theIter = theGraphInstances.begin();
                 theIter != theGraphInstances.end(); ++theIter) {
                if (theIter->first == theSlide && theBridge->IsActionInstance(theIter->second)) {
                    CUICDMActionHandle theAction =
                        theActionCore->GetActionByInstance(theIter->second);
                    SActionInfo theActionInfo = theActionCore->GetActionInfo(theAction);
                    CUICDMInstanceHandle theAcionOwner = theActionInfo.m_Owner;
                    if (theAcionOwner.Valid()
                        && IsAscendant(theAcionOwner, theObservedInstance, theDoc->GetAssetGraph()))
                        return true;
                }
            }
        }
    }

    return false;
}

bool CUICDMTimelineItemBinding::ComponentHasAction(bool inMaster)
{
    // Get all the instances in this component slidegraph
    // check whether the instance is an action instance
    // if inMaster is true, we only interest with those that are in the master slide, else we want
    // those that are not in the master slide
    CClientDataModelBridge *theBridge(m_StudioSystem->GetClientDataModelBridge());
    if (!theBridge->IsComponentInstance(m_DataHandle))
        return false;

    Q3DStudio::CId theAssetId = theBridge->GetGUID(m_DataHandle);
    CUICDMSlideHandle theMasterSlide =
        m_StudioSystem->GetSlideSystem()->GetMasterSlideByComponentGuid(GuidtoSLong4(theAssetId));

    TSlideInstancePairList theGraphInstances;
    m_StudioSystem->GetSlideSystem()->GetAssociatedInstances(theMasterSlide, theGraphInstances);

    for (TSlideInstancePairList::const_iterator theIter = theGraphInstances.begin();
         theIter != theGraphInstances.end(); ++theIter) {
        if (((inMaster && theIter->first == theMasterSlide)
             || (!inMaster && theIter->first != theMasterSlide))
            && theBridge->IsActionInstance(theIter->second))
            return true;
    }
    return false;
}

ITimelineTimebar *CUICDMTimelineItemBinding::GetTimebar()
{
    if (!m_TimelineTimebar)
        m_TimelineTimebar = CreateTimelineTimebar();
    return m_TimelineTimebar;
}

Q3DStudio::CString CUICDMTimelineItemBinding::GetName() const
{
    if (m_StudioSystem->IsInstance(m_DataHandle) == false)
        return L"";
    CUICDMPropertyHandle theNamePropHandle =
        m_StudioSystem->GetPropertySystem()->GetAggregateInstancePropertyByName(m_DataHandle,
                                                                                L"name");
    SValue theNameValue;
    m_StudioSystem->GetPropertySystem()->GetInstancePropertyValue(m_DataHandle, theNamePropHandle,
                                                                  theNameValue);
    TDataStrPtr theName = qt3dsdm::get<TDataStrPtr>(theNameValue);

    return (theName) ? Q3DStudio::CString(theName->GetData()) : "";
}

void CUICDMTimelineItemBinding::SetName(const Q3DStudio::CString &inName)
{
    // Display warning dialog if user tried to enter an empty string
    if (inName.IsEmpty()) {
        Q3DStudio::CString theTitle(::LoadResourceString(IDS_ERROR_OBJECT_RENAME_TITLE));
        Q3DStudio::CString theString(::LoadResourceString(IDS_ERROR_OBJECT_RENAME_EMPTY_STRING));
        g_StudioApp.GetDialogs()->DisplayMessageBox(theTitle, theString, CUICMessageBox::ICON_ERROR,
                                                    false);

        return;
    }

    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    if (!theBridge->CheckNameUnique(m_DataHandle, inName)) {
        Q3DStudio::CString theTitle(::LoadResourceString(IDS_ERROR_OBJECT_RENAME_TITLE));
        Q3DStudio::CString theString(
            ::LoadResourceString(IDS_ERROR_OBJECT_RENAME_DUPLICATED_STRING));
        int theUserChoice = g_StudioApp.GetDialogs()->DisplayChoiceBox(
            theTitle, theString, CUICMessageBox::ICON_WARNING);
        if (theUserChoice == QMessageBox::Yes) {
            // Set with the unique name
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(*m_TransMgr->GetDoc(), QObject::tr("Set Name"))
                ->SetName(m_DataHandle, inName, true);
            return;
        }
    }
    // Set the name no matter it's unique or not
    CUICDMPropertyHandle theNamePropHandle =
        m_StudioSystem->GetPropertySystem()->GetAggregateInstancePropertyByName(m_DataHandle,
                                                                                L"name");
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*m_TransMgr->GetDoc(), QObject::tr("Set Name"))
        ->SetInstancePropertyValue(m_DataHandle, theNamePropHandle,
                                   std::make_shared<CDataStr>(inName));
}

ITimelineItem *CUICDMTimelineItemBinding::GetTimelineItem()
{
    return this;
}

CBaseStateRow *CUICDMTimelineItemBinding::GetRow()
{
    return m_Row;
}

void CUICDMTimelineItemBinding::SetSelected(bool inMultiSelect)
{
    if (!inMultiSelect)
        g_StudioApp.GetCore()->GetDoc()->SelectUICDMObject(m_DataHandle);
    else
        g_StudioApp.GetCore()->GetDoc()->ToggleUICDMObjectToSelection(m_DataHandle);
}

void CUICDMTimelineItemBinding::OnCollapsed()
{
    // Preserves legacy behavior where collapsing a tree will select that root, if any of its
    // descendant was selected
    // TODO: This won't work for Image (because Image is Material's property, not child)
    qt3dsdm::CUICDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        CDoc *theDoc = m_TransMgr->GetDoc();
        qt3dsdm::CUICDMInstanceHandle theSelectedInstance = theDoc->GetSelectedInstance();
        if (theSelectedInstance.Valid()
            && IsAscendant(theSelectedInstance, theInstance, theDoc->GetAssetGraph()))
            SetSelected(false);
    }
}

void CUICDMTimelineItemBinding::ClearKeySelection()
{
    m_TransMgr->ClearKeyframeSelection();
}

bool CUICDMTimelineItemBinding::OpenAssociatedEditor()
{
    return false; // nothing to do by default
}

void CUICDMTimelineItemBinding::DoStartDrag(CControlWindowListener *inWndListener)
{
    inWndListener->DoStartDrag(this);
}

inline qt3dsdm::CUICDMInstanceHandle CUICDMTimelineItemBinding::GetInstance() const
{
    return m_DataHandle;
}

void CUICDMTimelineItemBinding::SetDropTarget(CDropTarget *inTarget)
{
    qt3dsdm::CUICDMInstanceHandle theInstance = GetInstance();
    inTarget->SetInstance(theInstance);
}

long CUICDMTimelineItemBinding::GetChildrenCount()
{
    qt3dsdm::CUICDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        Q3DStudio::CGraphIterator theChildren;
        CUICDMSlideHandle theActiveSlide = m_TransMgr->GetDoc()->GetActiveSlide();
        GetAssetChildrenInTimeParent(theInstance, m_TransMgr->GetDoc(), AmITimeParent(),
                                     theChildren, theActiveSlide);
        return (long)theChildren.GetCount();
    }
    return 0;
}

ITimelineItemBinding *CUICDMTimelineItemBinding::GetChild(long inIndex)
{
    qt3dsdm::CUICDMInstanceHandle theInstance = GetInstance();
    if (theInstance.Valid()) {
        Q3DStudio::CGraphIterator theChildren;
        CUICDMSlideHandle theActiveSlide = m_TransMgr->GetDoc()->GetActiveSlide();
        GetAssetChildrenInTimeParent(theInstance, m_TransMgr->GetDoc(), AmITimeParent(),
                                     theChildren, theActiveSlide);
        theChildren += inIndex;

        qt3dsdm::CUICDMInstanceHandle theChildInstance = theChildren.GetCurrent();
        if (theChildInstance.Valid())
            return m_TransMgr->GetOrCreate(theChildInstance);
    }
    return nullptr;
}

ITimelineItemBinding *CUICDMTimelineItemBinding::GetParent()
{
    return m_Parent;
}
void CUICDMTimelineItemBinding::SetParent(ITimelineItemBinding *parent)
{
    if (parent != m_Parent) {
        ASSERT(parent == nullptr || m_Parent == nullptr);
        m_Parent = parent;
    }
}

long CUICDMTimelineItemBinding::GetPropertyCount()
{
    long theCount = 0;
    if (m_StudioSystem->IsInstance(m_DataHandle)) {
        TPropertyHandleList theProperties;
        m_StudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(m_DataHandle,
                                                                            theProperties);
        for (size_t thePropertyIndex = 0; thePropertyIndex < theProperties.size();
             ++thePropertyIndex) {
            if (m_StudioSystem->GetAnimationSystem()->IsPropertyAnimated(
                    m_DataHandle, theProperties[thePropertyIndex]))
                ++theCount;
        }
    }
    return theCount;
}

ITimelineItemProperty *CUICDMTimelineItemBinding::GetProperty(long inIndex)
{
    TPropertyHandleList theProperties;
    m_StudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(m_DataHandle,
                                                                        theProperties);
    long theIndex = -1;
    size_t thePropertyIndex = 0;
    for (; thePropertyIndex < theProperties.size(); ++thePropertyIndex) {
        if (m_StudioSystem->GetAnimationSystem()->IsPropertyAnimated(
                m_DataHandle, theProperties[thePropertyIndex])) {
            ++theIndex;
            if (theIndex == inIndex)
                break;
        }
    }
    ASSERT(thePropertyIndex < theProperties.size()); // no reason why this would be out of range!!
    return GetOrCreatePropertyBinding(theProperties[thePropertyIndex]);
}

bool CUICDMTimelineItemBinding::ShowToggleControls() const
{
    return true;
}
bool CUICDMTimelineItemBinding::IsLockedEnabled() const
{
    return IsLocked();
}
bool CUICDMTimelineItemBinding::IsVisibleEnabled() const
{
    // You can only toggle visible if you aren't on the master slide.
    return m_StudioSystem->GetSlideSystem()->GetSlideIndex(m_TransMgr->GetDoc()->GetActiveSlide())
        != 0;
}

void CUICDMTimelineItemBinding::Bind(CBaseStateRow *inRow)
{
    ASSERT(!m_Row);
    m_Row = inRow;

    // Because children(properties included) may only be loaded later, check if there are any
    // keyframes without having to have the UI created.
    TPropertyHandleList theProperties;
    m_StudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(m_DataHandle,
                                                                        theProperties);
    for (size_t thePropertyIndex = 0; thePropertyIndex < theProperties.size(); ++thePropertyIndex) {
        if (m_StudioSystem->GetAnimationSystem()->IsPropertyAnimated(
                m_DataHandle, theProperties[thePropertyIndex]))
            AddKeyframes(GetOrCreatePropertyBinding(theProperties[thePropertyIndex]));
    }

    // Set selection status
    CUICDMInstanceHandle theSelectedInstance = m_TransMgr->GetDoc()->GetSelectedInstance();
    m_Row->OnSelected(m_DataHandle == theSelectedInstance);
}

void CUICDMTimelineItemBinding::Release()
{
    m_Row = nullptr;
    RemoveAllPropertyBindings();
    m_TransMgr->Unregister(this);
}

bool CUICDMTimelineItemBinding::IsValidTransaction(EUserTransaction inTransaction)
{
    qt3dsdm::CUICDMInstanceHandle theInstance = GetInstance();
    switch (inTransaction) {
    case EUserTransaction_Rename:
        return (GetObjectType() != OBJTYPE_SCENE && GetObjectType() != OBJTYPE_IMAGE);

    case EUserTransaction_Duplicate:
        if (theInstance.Valid())
            return m_StudioSystem->GetClientDataModelBridge()->IsDuplicateable(theInstance);
        break;

    case EUserTransaction_Cut:
        return g_StudioApp.CanCut();

    case EUserTransaction_Copy:
        return g_StudioApp.CanCopy();

    case EUserTransaction_Paste:
        return m_TransMgr->GetDoc()->CanPasteObject();

    case EUserTransaction_Delete:
        if (theInstance.Valid())
            return m_StudioSystem->GetClientDataModelBridge()->CanDelete(theInstance);
        break;

    case EUserTransaction_MakeComponent: {
        bool theCanMakeFlag = false;
        if (theInstance.Valid()) {
            CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
            EStudioObjectType theObjectType = theBridge->GetObjectType(theInstance);

            if (!IsLocked()) {
                // Any assets that are attached to the Scene directly must not be wrapped in a
                // component.
                // This may include behavior assets which may be directly attached to the Scene.
                // This is because by principal, components cannot exist on the Scene directly.
                qt3dsdm::CUICDMInstanceHandle theParentInstance =
                    theBridge->GetParentInstance(theInstance);
                if (theObjectType != OBJTYPE_LAYER && theObjectType != OBJTYPE_SCENE
                    && theObjectType != OBJTYPE_MATERIAL && theObjectType != OBJTYPE_IMAGE
                    && theObjectType != OBJTYPE_EFFECT
                    && (theParentInstance.Valid()
                        && theBridge->GetObjectType(theParentInstance)
                            != OBJTYPE_SCENE)) // This checks if the object is
                                               // AttachedToSceneDirectly
                {
                    theCanMakeFlag = true;
                }
            }
        }
        return theCanMakeFlag;
    }

    case EUserTransaction_EditComponent:
        return (GetObjectType() == OBJTYPE_COMPONENT);

    default: // not handled
        break;
    }

    return false;
}

using namespace Q3DStudio;

inline void DoCut(CDoc &inDoc, const qt3dsdm::TInstanceHandleList &inInstances)
{
    inDoc.DeselectAllKeyframes();
    inDoc.CutObject(inInstances);
}

inline void DoDelete(CDoc &inDoc, const qt3dsdm::TInstanceHandleList &inInstances)
{
    inDoc.DeselectAllKeyframes();
    inDoc.DeleteObject(inInstances);
}

inline void DoMakeComponent(CDoc &inDoc, const qt3dsdm::TInstanceHandleList &inInstances)
{
    SCOPED_DOCUMENT_EDITOR(inDoc, QObject::tr("Make Component"))->MakeComponent(inInstances);
}

void CUICDMTimelineItemBinding::PerformTransaction(EUserTransaction inTransaction)
{
    CDoc *theDoc = m_TransMgr->GetDoc();
    qt3dsdm::TInstanceHandleList theInstances = theDoc->GetSelectedValue().GetSelectedInstances();
    if (theInstances.empty())
        return;
    CDispatch &theDispatch(*theDoc->GetCore()->GetDispatch());

    // Transactions that could result in *this* object being deleted need to be executed
    // via postmessage, not in this context because it could result in the currently
    // active timeline row being deleted while in its own mouse handler.
    switch (inTransaction) {
    case EUserTransaction_Duplicate: {
        theDoc->DeselectAllKeyframes();
        SCOPED_DOCUMENT_EDITOR(*theDoc, QObject::tr("Duplicate Object"))->DuplicateInstances(theInstances);
    } break;
    case EUserTransaction_Cut: {
        theDispatch.FireOnAsynchronousCommand(
            std::bind(DoCut, std::ref(*theDoc), theInstances));
    } break;
    case EUserTransaction_Copy: {
        theDoc->DeselectAllKeyframes();
        theDoc->CopyObject(theInstances);
    } break;
    case EUserTransaction_Paste: {
        theDoc->DeselectAllKeyframes();
        theDoc->PasteObject(GetInstance());
    } break;
    case EUserTransaction_Delete: {
        theDispatch.FireOnAsynchronousCommand(
            std::bind(DoDelete, std::ref(*theDoc), theInstances));
    } break;
    case EUserTransaction_MakeComponent: {
        theDispatch.FireOnAsynchronousCommand(
            std::bind(DoMakeComponent, std::ref(*theDoc), theInstances));
    }
    default: // not handled
        break;
    }
}

Q3DStudio::CString CUICDMTimelineItemBinding::GetObjectPath()
{
    CDoc *theDoc = m_TransMgr->GetDoc();
    // Because we are getting absolute path, the base id doesn't matter.
    return CRelativePathTools::BuildAbsoluteReferenceString(m_DataHandle, theDoc);
}

ITimelineKeyframesManager *CUICDMTimelineItemBinding::GetKeyframesManager() const
{
    return m_TransMgr->GetKeyframesManager();
}

void CUICDMTimelineItemBinding::RemoveProperty(ITimelineItemProperty *inProperty)
{
    Q_UNUSED(inProperty);
    // TODO: This function has no use in UICDM world. This is replaced by RemovePropertyRow(
    // CUICDMPropertyHandle inPropertyHandle ).
    // Decide if this function should be removed from ITimelineItemBinding.
}

void CUICDMTimelineItemBinding::LoadProperties()
{
    TPropertyHandleList theProperties;
    m_StudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(m_DataHandle,
                                                                        theProperties);
    for (size_t thePropertyIndex = 0; thePropertyIndex < theProperties.size(); ++thePropertyIndex) {
        if (m_StudioSystem->GetAnimationSystem()->IsPropertyAnimated(
                m_DataHandle, theProperties[thePropertyIndex]))
            AddPropertyRow(theProperties[thePropertyIndex], true);
    }
}

void CUICDMTimelineItemBinding::InsertKeyframe()
{
    if (m_PropertyBindingMap.empty())
        return;

    TPropertyBindingMap::const_iterator theIter = m_PropertyBindingMap.begin();
    ScopedDocumentEditor editor(*g_StudioApp.GetCore()->GetDoc(), L"Insert Keyframe", __FILE__,
                                __LINE__);
    for (; theIter != m_PropertyBindingMap.end(); ++theIter)
        editor->KeyframeProperty(m_DataHandle, theIter->first, false);
}

void CUICDMTimelineItemBinding::DeleteAllChannelKeyframes()
{
    if (m_PropertyBindingMap.empty())
        return;

    CDoc *theDoc = m_TransMgr->GetDoc();
    Q3DStudio::ScopedDocumentEditor editor(*theDoc, L"Delete Channel Keyframes", __FILE__,
                                           __LINE__);
    for (TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.begin(),
                                       theEnd = m_PropertyBindingMap.end();
         theIter != theEnd; ++theIter)
        theIter->second->DeleteAllKeys();
}

long CUICDMTimelineItemBinding::GetKeyframeCount() const
{
    // This list is updated when properties are loaded and when keyframes are added & deleted.
    return (long)m_Keyframes.size();
}

IKeyframe *CUICDMTimelineItemBinding::GetKeyframeByTime(long inTime) const
{
    TAssetKeyframeList::const_iterator theIter = m_Keyframes.begin();
    for (; theIter != m_Keyframes.end(); ++theIter) {
        if ((*theIter).GetTime() == inTime)
            return const_cast<CUICDMAssetTimelineKeyframe *>(&(*theIter));
    }
    return nullptr;
}

IKeyframe *CUICDMTimelineItemBinding::GetKeyframeByIndex(long inIndex) const
{
    if (inIndex >= 0 && inIndex < (long)m_Keyframes.size())
        return const_cast<CUICDMAssetTimelineKeyframe *>(&m_Keyframes[inIndex]);

    ASSERT(0); // should not happen
    return nullptr;
}

long CUICDMTimelineItemBinding::OffsetSelectedKeyframes(long inOffset)
{
    return m_TransMgr->GetKeyframesManager()->OffsetSelectedKeyframes(inOffset);
}

void CUICDMTimelineItemBinding::CommitChangedKeyframes()
{
    m_TransMgr->GetKeyframesManager()->CommitChangedKeyframes();
}

void CUICDMTimelineItemBinding::OnEditKeyframeTime(long inCurrentTime, long inObjectAssociation)
{
    CTimeEditDlg theTimeEditDlg;
    theTimeEditDlg.SetKeyframesManager(m_TransMgr->GetKeyframesManager());
    theTimeEditDlg.ShowDialog(inCurrentTime, 0, g_StudioApp.GetCore()->GetDoc(),
                              inObjectAssociation);
}

void CUICDMTimelineItemBinding::SelectKeyframes(bool inSelected, long inTime /*= -1 */)
{
    // Callback from UI, hence skip the UI update
    DoSelectKeyframes(inSelected, inTime, false);
}

CUICDMInstanceHandle CUICDMTimelineItemBinding::GetInstanceHandle() const
{
    return m_DataHandle;
}

long CUICDMTimelineItemBinding::GetFlavor() const
{
    return QT3DS_FLAVOR_ASSET_TL;
}

void CUICDMTimelineItemBinding::OnBeginDataModelNotifications()
{
}
void CUICDMTimelineItemBinding::OnEndDataModelNotifications()
{
    RefreshStateRow();
}
void CUICDMTimelineItemBinding::OnImmediateRefreshInstanceSingle(
    qt3dsdm::CUICDMInstanceHandle inInstance)
{
    if (inInstance == m_DataHandle)
        RefreshStateRow(true);
}
void CUICDMTimelineItemBinding::OnImmediateRefreshInstanceMultiple(
    qt3dsdm::CUICDMInstanceHandle *inInstance, long inInstanceCount)
{
    for (long idx = 0; idx < inInstanceCount; ++idx)
        if (inInstance[idx] == m_DataHandle) {
            RefreshStateRow();
            break;
        }
}

void CUICDMTimelineItemBinding::RefreshStateRow(bool inRefreshChildren)
{
    CStateRow *theRow = dynamic_cast<CStateRow *>(m_Row);
    if (theRow) {
        theRow->OnTimeChange();
        theRow->ClearDirty();
        if (inRefreshChildren) {
            long theChildrenCount = GetChildrenCount();
            for (long theIndex = 0; theIndex < theChildrenCount; ++theIndex) {
                ITimelineItemBinding *theChild = GetChild(theIndex);
                CUICDMTimelineItemBinding *theBinding =
                    dynamic_cast<CUICDMTimelineItemBinding *>(theChild);
                if (theBinding)
                    theBinding->RefreshStateRow(inRefreshChildren);
            }
        }
    }
}

ITimelineTimebar *CUICDMTimelineItemBinding::CreateTimelineTimebar()
{
    return new CUICDMTimelineTimebar(m_TransMgr, m_DataHandle);
}

ITimelineItemProperty *
CUICDMTimelineItemBinding::GetPropertyBinding(CUICDMPropertyHandle inPropertyHandle)
{
    TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.find(inPropertyHandle);
    // check if it already exists
    if (theIter != m_PropertyBindingMap.end())
        return theIter->second;
    return nullptr;
}

ITimelineItemProperty *
CUICDMTimelineItemBinding::GetOrCreatePropertyBinding(CUICDMPropertyHandle inPropertyHandle)
{
    ITimelineItemProperty *theProperty = GetPropertyBinding(inPropertyHandle);
    // check if it already exists
    if (theProperty)
        return theProperty;

    // Create
    CUICDMTimelineItemProperty *theTimelineProperty =
        new CUICDMTimelineItemProperty(m_TransMgr, inPropertyHandle, m_DataHandle);
    m_PropertyBindingMap.insert(std::make_pair(inPropertyHandle, theTimelineProperty));

    return theTimelineProperty;
}

//=============================================================================
/**
 * Add a new property row for this property.
 * @param inAppend true to skip the check to find where to insert. ( true if this is a
 * loading/initializing step, where the call is already done in order )
 */
void CUICDMTimelineItemBinding::AddPropertyRow(CUICDMPropertyHandle inPropertyHandle,
                                               bool inAppend /*= false */)
{
    ITimelineItemProperty *theTimelineProperty = GetPropertyBinding(inPropertyHandle);
    if (theTimelineProperty && theTimelineProperty->GetRow()) // if created, bail
        return;

    if (!theTimelineProperty)
        theTimelineProperty = GetOrCreatePropertyBinding(inPropertyHandle);

    // Find the row to insert this new property, if any, this preserves the order the property rows
    // is displayed in the timeline.
    ITimelineItemProperty *theNextProperty = nullptr;
    if (!inAppend) {
        TPropertyHandleList theProperties;
        m_StudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(m_DataHandle,
                                                                            theProperties);
        size_t thePropertyIndex = 0;
        size_t thePropertyCount = theProperties.size();
        for (; thePropertyIndex < thePropertyCount; ++thePropertyIndex) {
            if (theProperties[thePropertyIndex] == inPropertyHandle) {
                ++thePropertyIndex;
                break;
            }
        }
        // Not all properties are displayed, so another loop to search for the first one that maps
        // to a existing propertyrow
        for (; thePropertyIndex < thePropertyCount; ++thePropertyIndex) {
            TPropertyBindingMap::iterator theNextPropIter =
                m_PropertyBindingMap.find(theProperties[thePropertyIndex]);
            if (theNextPropIter != m_PropertyBindingMap.end()) {
                theNextProperty = theNextPropIter->second;
                break;
            }
        }
    }
    // Create a new property row
    m_TransMgr->CreateNewPropertyRow(theTimelineProperty, m_Row,
                                     theNextProperty ? theNextProperty->GetRow() : nullptr);

    // Update keyframes
    AddKeyframes(theTimelineProperty);
}

void CUICDMTimelineItemBinding::RemovePropertyRow(CUICDMPropertyHandle inPropertyHandle)
{
    TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.find(inPropertyHandle);
    if (theIter != m_PropertyBindingMap.end()) {
        ITimelineItemProperty *thePropertyBinding = theIter->second;

        bool theUpdateUI = DeleteAssetKeyframesWhereApplicable(thePropertyBinding);

        m_TransMgr->RemovePropertyRow(thePropertyBinding);
        m_PropertyBindingMap.erase(theIter);

        // UI must update
        if (m_Row && theUpdateUI) {
            m_Row->OnChildVisibilityChanged();
            m_Row->GetTimebar()->SetDirty(true);
        }
    }
}

// called when a keyframe is inserted, deleted or updated in the data model
void CUICDMTimelineItemBinding::RefreshPropertyKeyframe(
    qt3dsdm::CUICDMPropertyHandle inPropertyHandle, qt3dsdm::CUICDMKeyframeHandle inKeyframe,
    ETimelineKeyframeTransaction inTransaction)
{
    TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.find(inPropertyHandle);
    if (theIter != m_PropertyBindingMap.end()) {
        CUICDMTimelineItemProperty *theProperty = theIter->second;
        if (theProperty) {
            if (theProperty->RefreshKeyframe(inKeyframe, inTransaction)) {
                // Update asset keyframes
                UpdateKeyframe(theProperty->GetKeyframeByHandle(inKeyframe), inTransaction);
                if (m_Row) // UI update
                    m_Row->GetTimebar()->SetDirty(true);
            }
        }
    }
}

// called when the keyframes are updated in the UI and data model hasn't committed the change, ie no
// event callback from UICDM
void CUICDMTimelineItemBinding::UIRefreshPropertyKeyframe(long inOffset)
{
    if (!m_Row)
        return;

    // TODO: figure out a better way to sync m_Keyframes
    TAssetKeyframeList::iterator theKeyIter = m_Keyframes.begin();
    for (; theKeyIter != m_Keyframes.end(); ++theKeyIter) {
        if (theKeyIter->IsSelected())
            theKeyIter->UpdateTime(theKeyIter->GetTime() + inOffset);
    }
    // If a asset keyframe was "shared" by several properties' keyframes
    // we need to 'break' this sharing and create for the remaining unmoved keyframes.
    TPropertyBindingMap::const_iterator theIter = m_PropertyBindingMap.begin();
    for (; theIter != m_PropertyBindingMap.end(); ++theIter) {
        (*theIter).second->RefreshKeyFrames();

        for (long i = 0; i < theIter->second->GetKeyframeCount(); ++i) {
            IKeyframe *theKeyframe = theIter->second->GetKeyframeByIndex(i);
            UpdateKeyframe(theKeyframe, ETimelineKeyframeTransaction_Add);

            // Unfortunately, this is the way we can propagate UI updates to ALL selected keyframes
            if (theKeyframe->IsSelected()) {
                CPropertyRow *thePropertyRow = theIter->second->GetRow();
                if (thePropertyRow)
                    thePropertyRow->GetTimebar()->SetDirty(true);
            }
        }
    }
    m_Row->GetTimebar()->SetDirty(true);
}

void CUICDMTimelineItemBinding::OnPropertyChanged(CUICDMPropertyHandle inPropertyHandle)
{ // Refresh UI
    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    if (m_Row && (inPropertyHandle == theBridge->GetNameProperty()
                  || inPropertyHandle == theBridge->GetSceneAsset().m_Eyeball
                  || inPropertyHandle == theBridge->GetSceneAsset().m_Locked
                  || inPropertyHandle == theBridge->GetSceneAsset().m_Shy
                  || inPropertyHandle == theBridge->GetSceneAsset().m_StartTime
                  || inPropertyHandle == theBridge->GetSceneAsset().m_EndTime))
        m_Row->OnDirty();
}

void CUICDMTimelineItemBinding::OnPropertyLinked(CUICDMPropertyHandle inPropertyHandle)
{
    if (m_StudioSystem->GetAnimationSystem()->IsPropertyAnimated(m_DataHandle, inPropertyHandle)) {
        // Refresh property row by delete and recreate
        RemovePropertyRow(inPropertyHandle);
        AddPropertyRow(inPropertyHandle);
    }
}

bool CUICDMTimelineItemBinding::HasDynamicKeyframes(long inTime)
{
    if (inTime == -1) {
        if (GetPropertyCount() == 0)
            return false;

        for (long i = 0; i < GetPropertyCount(); ++i) {
            ITimelineItemProperty *theTimelineItemProperty = GetProperty(i);
            if (!theTimelineItemProperty->IsDynamicAnimation())
                return false;
        }
        return true;
    } else {
        TPropertyBindingMap::const_iterator theIter = m_PropertyBindingMap.begin();
        for (; theIter != m_PropertyBindingMap.end(); ++theIter) {
            IKeyframe *theKeyframe = theIter->second->GetKeyframeByTime(inTime);
            if (theKeyframe && theKeyframe->IsDynamic())
                return true;
        }
    }
    return false;
}

void CUICDMTimelineItemBinding::SetDynamicKeyframes(long inTime, bool inDynamic)
{
    TPropertyBindingMap::const_iterator theIter = m_PropertyBindingMap.begin();
    for (; theIter != m_PropertyBindingMap.end(); ++theIter) {
        IKeyframe *theKeyframe = theIter->second->GetKeyframeByTime(inTime);
        if (theKeyframe)
            theKeyframe->SetDynamic(inDynamic); // TODO: we want this in 1 batch command
    }
}

// Update UI on the selection state of all keyframes on this row and all its properties' keyframes.
void CUICDMTimelineItemBinding::DoSelectKeyframes(bool inSelected, long inTime, bool inUpdateUI)
{
    if (inTime == -1) // all keyframes
    {
        TAssetKeyframeList::iterator theKeyIter = m_Keyframes.begin();
        for (; theKeyIter != m_Keyframes.end(); ++theKeyIter)
            theKeyIter->SetSelected(inSelected);
    } else {
        CUICDMAssetTimelineKeyframe *theKeyframe =
            dynamic_cast<CUICDMAssetTimelineKeyframe *>(GetKeyframeByTime(inTime));
        if (theKeyframe)
            theKeyframe->SetSelected(inSelected);
    }
    if (inUpdateUI && m_Row)
        m_Row->GetTimebar()->SelectKeysByTime(-1, inSelected);

    // legacy feature: all properties with keyframes at inTime or all if inTime is -1 are selected
    // as well.
    TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.begin();
    for (; theIter != m_PropertyBindingMap.end(); ++theIter)
        theIter->second->DoSelectKeyframes(inSelected, inTime, true, this);
}

// When selecting by mouse-drag, if all properties are selected, select the asset keyframe. And if
// one gets de-selected, de-select. Legacy feature.
// Note that if only 1 property has a keyframe at time t, the asset keyframe gets selected
// automatically when that keyframe is selected. Its odd to me but
// that's how it has always behaved.
void CUICDMTimelineItemBinding::OnPropertySelection(long inTime)
{
    IKeyframe *theAssetKeyframe = GetKeyframeByTime(inTime);
    if (theAssetKeyframe) {
        bool theAllSelectedFlag = true;
        TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.begin();
        for (; theIter != m_PropertyBindingMap.end(); ++theIter) {
            IKeyframe *theKeyframe = theIter->second->GetKeyframeByTime(inTime);
            if (theKeyframe && !theKeyframe->IsSelected()) // done, i.e selection remain unchanged.
            {
                theAllSelectedFlag = false;
                break;
            }
        }
        if (theAssetKeyframe->IsSelected() != theAllSelectedFlag) {
            dynamic_cast<CUICDMAssetTimelineKeyframe *>(theAssetKeyframe)
                ->SetSelected(theAllSelectedFlag);
            // Update UI
            if (m_Row)
                m_Row->GetTimebar()->SelectKeysByTime(inTime, theAllSelectedFlag);
        }
    }
}

Q3DStudio::CId CUICDMTimelineItemBinding::GetGuid() const
{
    CClientDataModelBridge *theClientBridge = m_StudioSystem->GetClientDataModelBridge();
    qt3dsdm::IPropertySystem *thePropertySystem = m_StudioSystem->GetPropertySystem();
    SValue theValue;
    if (thePropertySystem->GetInstancePropertyValue(m_DataHandle, theClientBridge->GetIdProperty(),
                                                    theValue)) {
        SLong4 theLong4 = qt3dsdm::get<SLong4>(theValue);
        return Q3DStudio::CId(theLong4.m_Longs[0], theLong4.m_Longs[1], theLong4.m_Longs[2],
                              theLong4.m_Longs[3]);
    }
    return Q3DStudio::CId();
}

// Delete asset keyframes at time t if no property keyframes exist at time t
//@param inSkipPropertyBinding property that to skip, e.g. in cases where property is deleted
//@return true if there are asset keyframes deleted.
bool CUICDMTimelineItemBinding::DeleteAssetKeyframesWhereApplicable(
    ITimelineItemProperty *inSkipPropertyBinding /*= nullptr */)
{
    // iterate through m_Keyframes because we cannot obtain time information from the Animation
    // keyframes anymore, since they are deleted.
    std::vector<long> theDeleteIndicesList;
    for (size_t theIndex = 0; theIndex < m_Keyframes.size(); ++theIndex) {
        TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.begin();
        for (; theIter != m_PropertyBindingMap.end(); ++theIter) {
            if ((!inSkipPropertyBinding || theIter->second != inSkipPropertyBinding)
                && theIter->second->GetKeyframeByTime(m_Keyframes[theIndex].GetTime())) // done!
                break;
        }
        if (theIter == m_PropertyBindingMap.end())
            theDeleteIndicesList.push_back((long)theIndex);
    }
    // start with the last item, so that the indices remain valid.
    for (long i = (long)theDeleteIndicesList.size() - 1; i >= 0; --i) {
        TAssetKeyframeList::iterator theKeyIter = m_Keyframes.begin();
        std::advance(theKeyIter, theDeleteIndicesList[i]);
        m_Keyframes.erase(theKeyIter);
    }

    return !theDeleteIndicesList.empty();
}

void CUICDMTimelineItemBinding::RemoveAllPropertyBindings()
{
    TPropertyBindingMap::iterator theIter = m_PropertyBindingMap.begin();
    for (; theIter != m_PropertyBindingMap.end(); ++theIter)
        delete theIter->second;
    m_PropertyBindingMap.clear();
}

void CUICDMTimelineItemBinding::AddKeyframes(ITimelineItemProperty *inPropertyBinding)
{
    for (long i = 0; i < inPropertyBinding->GetKeyframeCount(); ++i)
        UpdateKeyframe(inPropertyBinding->GetKeyframeByIndex(i), ETimelineKeyframeTransaction_Add);
}

// Update the asset keyframes based on the properties' keyframes.
void CUICDMTimelineItemBinding::UpdateKeyframe(IKeyframe *inKeyframe,
                                               ETimelineKeyframeTransaction inTransaction)
{
    bool theDoAddFlag = (inTransaction == ETimelineKeyframeTransaction_Add);
    bool theDoDeleteFlag = (inTransaction == ETimelineKeyframeTransaction_Delete);

    // For update, if there isn't already a asset keyframe at the associated time, create one
    if (inTransaction == ETimelineKeyframeTransaction_Update) {
        theDoAddFlag = inKeyframe && !GetKeyframeByTime(inKeyframe->GetTime());
        theDoDeleteFlag = true; // plus, since we don't keep track of indiviual property keyframes
                                // here, iterate and make sure list is correct.
    }

    if (theDoDeleteFlag)
        DeleteAssetKeyframesWhereApplicable();

    // Add when a new keyframe is added or MAYBE when a keyframe is moved
    if (theDoAddFlag && inKeyframe) {
        long theKeyframeTime = inKeyframe->GetTime();
        if (theKeyframeTime >= 0) {
            bool theAppend = true;
            // insert this in the order that it should be. and we trust the
            TAssetKeyframeList::iterator theIter = m_Keyframes.begin();
            for (; theIter != m_Keyframes.end(); ++theIter) {
                long theTime = (*theIter).GetTime();
                if (theTime == theKeyframeTime) {
                    theAppend = false;
                    break; // already exists, we are done. Because we only need 1 to represent ALL
                           // properties
                }
            }
            if (theAppend)
                m_Keyframes.push_back(CUICDMAssetTimelineKeyframe(this, theKeyframeTime));
        }
    }
    if (m_Row && (theDoAddFlag
                  || inTransaction == ETimelineKeyframeTransaction_DynamicChanged)) // dynamic =>
                                                                                    // only UI needs
                                                                                    // to refresh
        m_Row->GetTimebar()->SetDirty(true);
}

void CUICDMTimelineItemBinding::OnAddChild(CUICDMInstanceHandle inInstance)
{
    CDoc *theDoc = m_TransMgr->GetDoc();
    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    ISlideSystem *theSlideSystem = m_StudioSystem->GetSlideSystem();

    qt3dsdm::CUICDMSlideHandle theSlide = theSlideSystem->GetAssociatedSlide(inInstance);
    if (theBridge->IsInActiveComponent(inInstance)
        && (theSlideSystem->IsMasterSlide(theSlide) || theSlide == theDoc->GetActiveSlide())) {
        // Only add if the asset is in the current active component, and it's a master asset or in
        // the current slide
        ITimelineItemBinding *theNextItem = nullptr;
        qt3dsdm::CUICDMInstanceHandle theParentInstance = GetInstance();
        // Figure out where to insert this row, if applicable.
        // CAsset has a list of children, and not necessarily all are active in this slide (e.g.
        // non-master children)
        Q3DStudio::TIdentifier theNextChild = 0;
        if (theParentInstance.Valid()) {
            // Get the next prioritized child in the same slide
            Q3DStudio::CGraphIterator theChildren;
            GetAssetChildrenInSlide(theDoc, theParentInstance, theDoc->GetActiveSlide(),
                                    theChildren);
            theNextChild = GetSibling(inInstance, true, theChildren);
        }

        if (theNextChild != 0)
            theNextItem = m_TransMgr->GetOrCreate(theNextChild);

        m_Row->AddChildRow(m_TransMgr->GetOrCreate(inInstance), theNextItem);
    }
}

void CUICDMTimelineItemBinding::OnDeleteChild(CUICDMInstanceHandle inInstance)
{
    ITimelineItemBinding *theChild = m_TransMgr->GetOrCreate(inInstance);
    if (theChild) {
        m_Row->RemoveChildRow(theChild);
    }
}

void CUICDMTimelineItemBinding::UpdateActionStatus()
{
    if (m_Row)
        m_Row->UpdateActionStatus();
}

//=============================================================================
/**
 *	Open the associated item as though it was double-clicked in explorer
 *	Respective subclasses (for example Image and Behavior) can call this function
 */
bool CUICDMTimelineItemBinding::OpenSourcePathFile()
{
    // Get source path property value
    CClientDataModelBridge *theClientBridge = m_StudioSystem->GetClientDataModelBridge();
    qt3dsdm::IPropertySystem *thePropertySystem = m_StudioSystem->GetPropertySystem();
    SValue theValue;
    if (thePropertySystem->GetInstancePropertyValue(
            m_DataHandle, theClientBridge->GetSourcePathProperty(), theValue)) {
        // Open the respective file
        Q3DStudio::CFilePath theSourcePath(qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue)->GetData());
        CUICFile theFile(m_TransMgr->GetDoc()->GetResolvedPathToDoc(theSourcePath));
        theFile.Execute();
        return true;
    }
    return false;
}
