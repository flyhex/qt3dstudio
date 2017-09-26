/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "stdafx.h"
#include "ClientDataModelBridge.h"
#include "Doc.h"
#include "StandardExtensions.h"
#include <fstream>
#include "Qt3DSDMSlideCore.h"
#include "Qt3DSDMSlideGraphCore.h"

#include "StudioFullSystem.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMActionCore.h"
#include "GraphUtils.h"
#include "StudioCoreSystem.h"

#include "Core.h"
#include "RelativePathTools.h"
#include "FormattedInputStream.h"
#include "FormattedOutputStream.h"
#include "Dispatch.h"
#include "Qt3DSFileTools.h"

using namespace qt3dsdm;
using namespace std;

inline SLong4 GuidToLong4(Q3DStudio::CId inGuid)
{
    Q3DStudio::TGUIDPacked thePacked(inGuid);
    SLong4 theGuid(thePacked.Data1, thePacked.Data2, thePacked.Data3, thePacked.Data4);
    return theGuid;
}

CClientDataModelBridge::CClientDataModelBridge(
    qt3dsdm::IDataCore *inDataCore, qt3dsdm::ISlideCore *inSlideCore,
    qt3dsdm::ISlideGraphCore *inSlideGraphCore, qt3dsdm::IAnimationCore *inAnimationCore,
    std::shared_ptr<qt3dsdm::IMetaData> inNewMetaData,
    std::shared_ptr<qt3dsdm::SComposerObjectDefinitions> inDefinitions, CDoc *inDoc)
    : m_DataCore(inDataCore)
    , m_SlideCore(inSlideCore)
    , m_SlideGraphCore(inSlideGraphCore)
    , m_AnimationCore(inAnimationCore)
    , m_NewMetaData(inNewMetaData)
    , m_ObjectDefinitions(inDefinitions)
    , m_Doc(inDoc)
    , m_DefaultMaterial(inDefinitions->m_Material)
    , m_SceneImage(inDefinitions->m_Image)
    , m_Node(inDefinitions->m_Node)
    , m_Layer(inDefinitions->m_Layer)
    , m_Model(inDefinitions->m_Model)
    , m_Light(inDefinitions->m_Light)
    , m_Camera(inDefinitions->m_Camera)
    , m_Text(inDefinitions->m_Text)
    , m_Group(inDefinitions->m_Group)
    , m_Component(inDefinitions->m_Component)
    , m_Behavior(inDefinitions->m_Behavior)
    , m_Scene(inDefinitions->m_Scene)
    , m_SlideItem(inDefinitions->m_Slide)
    , m_ActionItem(inDefinitions->m_Action)
    , m_SceneAsset(inDefinitions->m_Asset)
    , m_Effect(inDefinitions->m_Effect)
    , m_RenderPlugin(inDefinitions->m_RenderPlugin)
    , m_MaterialBase(inDefinitions->m_MaterialBase)
    , m_CustomMaterial(inDefinitions->m_CustomMaterial)
    , m_Alias(inDefinitions->m_Alias)
    , m_Path(inDefinitions->m_Path)
    , m_Lightmaps(inDefinitions->m_Lightmaps)
    , m_DataInput(inDefinitions->m_DataInput)
    , m_CacheEnabled(false)
{
}

CClientDataModelBridge::~CClientDataModelBridge()
{
}

Qt3DSDMSlideHandle CClientDataModelBridge::CreateNonMasterSlide(Qt3DSDMSlideHandle inMasterSlide,
                                                               Q3DStudio::CId inGuid,
                                                               const Q3DStudio::CString &inName)
{
    Qt3DSDMInstanceHandle theInstance = m_DataCore->CreateInstance();
    m_DataCore->DeriveInstance(theInstance, m_SlideItem.m_Instance);
    m_DataCore->SetInstancePropertyValue(theInstance, m_SlideItem.m_ComponentId,
                                         GuidToLong4(inGuid));
    SetName(theInstance, inName);
    Qt3DSDMSlideHandle theSlide = m_SlideCore->CreateSlide(theInstance);
    m_SlideCore->DeriveSlide(theSlide, inMasterSlide);
    return theSlide;
}

// Get the Scene or Component Asset of inAsset by querying the Parent
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetSceneOrComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (IsSceneInstance(inInstance) || IsComponentInstance(inInstance))
        return inInstance;

    if (!IsImageInstance(inInstance)) {
        return GetSceneOrComponentInstance(GetParentInstance(inInstance));
    } else {
        // Special case for Image because Image is a property of Material. Find which Material uses
        // this image.
        // This portion of code may never get executed though, but just in case...
        qt3dsdm::Qt3DSDMInstanceHandle theParentInstance;
        qt3dsdm::Qt3DSDMPropertyHandle theProperty;
        if (!GetMaterialFromImageInstance(inInstance, theParentInstance, theProperty)) {
            GetLayerFromImageProbeInstance(inInstance, theParentInstance, theProperty);
        }
        GetSceneOrComponentInstance(theParentInstance);
    }

    // Something is really wrong if we reach here.... Probably the Graph is not setup correctly
    ASSERT(0);
    return 0;
}

Qt3DSDMSlideGraphHandle
CClientDataModelBridge::GetOrCreateGraph(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    qt3dsdm::Qt3DSDMInstanceHandle theSceneComponentInstance = GetSceneOrComponentInstance(inInstance);
    Q3DStudio::CId theGuid = GetGUID(theSceneComponentInstance);
    Qt3DSDMInstanceHandle existing = GetInstanceByGUIDDerivedFrom(
        GuidToLong4(theGuid), m_SlideItem.m_Instance, m_SlideItem.m_ComponentId);
    if (existing.Valid()) {
        // There is an implicit assumption here that the slide graph is one level deep, i.e. only
        // the
        // root has children.
        Qt3DSDMSlideHandle theSlide = m_SlideCore->GetSlideByInstance(existing);
        if (m_SlideCore->GetParentSlide(theSlide).Valid())
            theSlide = m_SlideCore->GetParentSlide(theSlide);
        return m_SlideGraphCore->GetSlideGraph(theSlide);
    }
    ClearCache();
    Qt3DSDMInstanceHandle rootInstance = m_DataCore->CreateInstance();
    m_DataCore->DeriveInstance(rootInstance, m_SlideItem.m_Instance);
    m_DataCore->SetInstancePropertyValue(rootInstance, m_SlideItem.m_ComponentId,
                                         GuidToLong4(theGuid));
    SetName(rootInstance, Q3DStudio::CString::fromQString(QObject::tr("Master Slide")));
    Qt3DSDMSlideHandle masterSlide = m_SlideCore->CreateSlide(rootInstance);
    Qt3DSDMSlideGraphHandle retval(m_SlideGraphCore->CreateSlideGraph(masterSlide));
    Qt3DSDMSlideHandle theSlide1Handle =
        CreateNonMasterSlide(masterSlide, theGuid, Q3DStudio::CString::fromQString(QObject::tr("Slide1")));

    // always activate slide 1 on create
    m_SlideGraphCore->SetGraphActiveSlide(retval, theSlide1Handle);

    return retval;
}

Qt3DSDMSlideHandle
CClientDataModelBridge::GetOrCreateGraphRoot(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    return m_SlideGraphCore->GetGraphRoot(GetOrCreateGraph(inInstance));
}

Qt3DSDMSlideHandle GetSlideByIndex(Qt3DSDMSlideGraphHandle inGraph, int inIndex,
                                  ISlideCore &inSlideCore, ISlideGraphCore &inSlideGraphCore)
{
    Qt3DSDMSlideHandle theRoot = inSlideGraphCore.GetGraphRoot(inGraph);
    if (inIndex == 0)
        return theRoot;
    --inIndex;
    TSlideHandleList theSlides;
    inSlideCore.GetChildSlides(theRoot, theSlides);
    return theSlides.at(inIndex);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::CreateAssetInstance(Q3DStudio::CId &inId,
                                                                 EStudioObjectType inObjectType)
{
    Qt3DSDMInstanceHandle existing = GetInstanceByGUID(inId);
    if (existing.Valid())
        return existing;

    ClearCache();
    Qt3DSDMInstanceHandle theNewInstance = m_DataCore->CreateInstance();
    switch (inObjectType) {
    case OBJTYPE_MATERIAL:
        m_DataCore->DeriveInstance(theNewInstance, m_DefaultMaterial.m_Instance);
        break;
    case OBJTYPE_MODEL:
        m_DataCore->DeriveInstance(theNewInstance, m_Model.m_Instance);
        break;
    case OBJTYPE_TEXT:
        m_DataCore->DeriveInstance(theNewInstance, m_Text.m_Instance);
        break;
    case OBJTYPE_GROUP:
        m_DataCore->DeriveInstance(theNewInstance, m_Group.m_Instance);
        break;
    case OBJTYPE_COMPONENT:
        m_DataCore->DeriveInstance(theNewInstance, m_Component.m_Instance);
        break;
    case OBJTYPE_IMAGE:
        m_DataCore->DeriveInstance(theNewInstance, m_SceneImage.m_Instance);
        break;
    case OBJTYPE_LIGHT:
        m_DataCore->DeriveInstance(theNewInstance, m_Light.m_Instance);
        break;
    case OBJTYPE_CAMERA:
        m_DataCore->DeriveInstance(theNewInstance, m_Camera.m_Instance);
        break;
    case OBJTYPE_LAYER:
        m_DataCore->DeriveInstance(theNewInstance, m_Layer.m_Instance);
        break;
    case OBJTYPE_BEHAVIOR:
        m_DataCore->DeriveInstance(theNewInstance, m_Behavior.m_Instance);
        break;
    case OBJTYPE_SCENE:
        m_DataCore->DeriveInstance(theNewInstance, m_Scene.m_Instance);
        break;
    case OBJTYPE_EFFECT:
        m_DataCore->DeriveInstance(theNewInstance, m_Effect.m_Instance);
        break;
    case OBJTYPE_ALIAS:
        m_DataCore->DeriveInstance(theNewInstance, m_Alias.m_Instance);
        break;
    case OBJTYPE_LIGHTMAPS:
        m_DataCore->DeriveInstance(theNewInstance, m_Lightmaps.m_Instance);
        break;
    case OBJTYPE_DATAINPUT:
        m_DataCore->DeriveInstance(theNewInstance, m_DataInput.m_Instance);
        break;
    }

    m_DataCore->SetInstancePropertyValue(theNewInstance, GetObjectDefinitions().m_Guided.m_GuidProp,
                                         GuidToLong4(inId));

#if _DEBUG
    if (inObjectType == OBJTYPE_MATERIAL)
        ASSERT(GetMaterialInstanceByGUID(inId).Valid());
    else if (inObjectType == OBJTYPE_MODEL)
        ASSERT(GetModelInstanceByGUID(inId).Valid());
    else
        ASSERT(GetInstanceByGUID(inId).Valid());
#endif

    return theNewInstance;
}

qt3dsdm::Qt3DSDMInstanceHandle CClientDataModelBridge::GetSlideInstance()
{
    return m_SlideItem.m_Instance;
}

qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetSlideComponentIdProperty()
{
    return m_SlideItem.m_ComponentId;
}

qt3dsdm::Qt3DSDMInstanceHandle CClientDataModelBridge::GetActionInstance()
{
    return m_ActionItem.m_Instance;
}

qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetActionEyeball()
{
    return m_ActionItem.m_ActionEyeball;
}

qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetImportId()
{
    return m_SceneAsset.m_ImportId;
}
qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetNameProperty()
{
    return GetObjectDefinitions().m_Named.m_NameProp;
}
qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetIdProperty()
{
    return GetObjectDefinitions().m_Guided.m_GuidProp;
}
qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetTypeProperty()
{
    return GetObjectDefinitions().m_Typed.m_TypeProp;
}
qt3dsdm::Qt3DSDMPropertyHandle CClientDataModelBridge::GetSourcePathProperty()
{
    return m_SceneAsset.m_SourcePath;
}

bool CClientDataModelBridge::IsInternalProperty(const TCharStr &inPropertyName) const
{
    return (inPropertyName == L"name" || inPropertyName == L"importid" || inPropertyName == L"type"
            || inPropertyName == L"id" || inPropertyName == L"componentid"
            || inPropertyName == L"rotationorder" || inPropertyName == L"orientation"
            || inPropertyName == L"starttime" || inPropertyName == L"endtime"
            || inPropertyName == L"eyeball" || inPropertyName == L"shy"
            || inPropertyName == L"locked" || inPropertyName == L"timebarcolor"
            || inPropertyName == L"timebartext");
}

// Find the owning component. Note: inInstanceHandle may or may not have a client representation,
// e.g. Images do not, so we can't use CAsset::GetControllingComponent.
// Returns NULL if can't find one or its in the scene.
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetOwningComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle)
{
    int theSlideIndex;
    return GetOwningComponentInstance(inInstanceHandle, theSlideIndex);
}

// Find the owning component. Note: inInstanceHandle may or may not have a client representation,
// e.g. Images do not, so we can't use CAsset::GetControllingComponent.
// Returns NULL if can't find one or its in the scene.
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetOwningComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle,
                                                   int &outSlideIndex)
{
    if (!inInstanceHandle.Valid())
        return {};

    // get the slide this instance is in
    Qt3DSDMSlideHandle theSlideHandle =
        m_SlideGraphCore->GetAssociatedGraph(inInstanceHandle).second;
    if (!theSlideHandle.Valid())
        return {};

    return GetOwningComponentInstance(theSlideHandle, outSlideIndex);
}

// Find the owning component of the Slide Handle
// Returns NULL if can't find one.
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetOwningComponentInstance(qt3dsdm::Qt3DSDMSlideHandle inSlideHandle,
                                                   int &outSlideIndex)
{
    SLong4 theComponentGuid = GetComponentGuid(inSlideHandle);
    if (!GuidValid(theComponentGuid))
        return {};
    Q3DStudio::CId theId(theComponentGuid.m_Longs[0], theComponentGuid.m_Longs[1],
                         theComponentGuid.m_Longs[2], theComponentGuid.m_Longs[3]);

    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    outSlideIndex = theSlideSystem->GetSlideIndex(inSlideHandle);
    return GetInstanceByGUID(theComponentGuid);
}

// Find the owning component of the Slide Handle
// Returns NULL if can't find one.
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetOwningComponentInstance(qt3dsdm::Qt3DSDMSlideHandle inSlideHandle)
{
    int theSlideIndex;
    return GetOwningComponentInstance(inSlideHandle, theSlideIndex);
}

// Find the component Guid of the Slide Handle
SLong4 CClientDataModelBridge::GetComponentGuid(qt3dsdm::Qt3DSDMSlideHandle inSlideHandle)
{
    qt3dsdm::SLong4 theComponentGuid;

    // get the master slide (because only master knows which component instance)
    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    Qt3DSDMSlideHandle theMasterSlideHandle = theSlideSystem->GetMasterSlide(inSlideHandle);
    if (!theMasterSlideHandle.Valid())
        return theComponentGuid;

    Qt3DSDMInstanceHandle theOwningComponentInstance =
        theSlideSystem->GetSlideInstance(theMasterSlideHandle);
    if (!theOwningComponentInstance.Valid())
        return theComponentGuid;

    SValue theValue;
    if (m_DataCore->GetInstancePropertyValue(theOwningComponentInstance, m_SlideItem.m_ComponentId,
                                             theValue))
        theComponentGuid = theValue.getData<SLong4>();

    return theComponentGuid;
}

//==============================================================================
/**
 *	Helper method to check whether this asset is active.
 *	An asset is active if it meets the following criteria
 *	1. It's eyeball is on
 *	2. the current time falls within it's timebar
 */
bool CClientDataModelBridge::IsActive(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle,
                                      long inCurrentTime)
{
    // Check the eyeball
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    qt3dsdm::SValue theValue;
    thePropertySystem->GetInstancePropertyValue(inInstanceHandle, GetSceneAsset().m_Eyeball,
                                                theValue);
    if (!qt3dsdm::get<bool>(theValue))
        return false;

    // check current time is within the timebar
    thePropertySystem->GetInstancePropertyValue(inInstanceHandle, GetSceneAsset().m_StartTime,
                                                theValue);
    long theStartTime = qt3dsdm::get<qt3ds::QT3DSI32>(theValue);
    thePropertySystem->GetInstancePropertyValue(inInstanceHandle, GetSceneAsset().m_EndTime,
                                                theValue);
    long theEndTime = qt3dsdm::get<qt3ds::QT3DSI32>(theValue);
    if (inCurrentTime < theStartTime || inCurrentTime > theEndTime)
        return false;

    // has to be active if all the above succeed
    return true;
}

//==============================================================================
/**
 *	Get the active slide index of this component (or scene)
 *	@param	inAsset		the controlling component (component or scene)
 */
qt3dsdm::Qt3DSDMSlideHandle
CClientDataModelBridge::GetComponentActiveSlide(qt3dsdm::Qt3DSDMInstanceHandle inComponent)
{
    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    Q3DStudio::CId theId = GetGUID(inComponent);
    Qt3DSDMSlideHandle theMasterSlide =
        theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theId));
    return theSlideSystem->GetActiveSlide(theMasterSlide);
}

qt3dsdm::Qt3DSDMSlideHandle
CClientDataModelBridge::GetComponentSlide(qt3dsdm::Qt3DSDMInstanceHandle inComponent, long inIndex)
{
    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    Q3DStudio::CId theId = GetGUID(inComponent);
    Qt3DSDMSlideHandle theMasterSlide =
        theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theId));
    return theSlideSystem->GetSlideByIndex(theMasterSlide, inIndex);
}

//
void CClientDataModelBridge::SetName(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle,
                                     const Q3DStudio::CString &inName)
{
    TDataStrPtr theName(new CDataStr(inName));
    Qt3DSDMSlideHandle theAssociatedSlide =
        m_Doc->GetStudioSystem()->GetFullSystem()->GetSlideSystem()->GetAssociatedSlide(
            inInstanceHandle);
    if (theAssociatedSlide.Valid())
        m_Doc->GetStudioSystem()->GetFullSystem()->GetSlideCore()->ForceSetInstancePropertyValue(
            theAssociatedSlide, inInstanceHandle, GetNameProperty(), theName);
    else {
        qt3dsdm::TDataCorePtr theDataCore =
            m_Doc->GetStudioSystem()->GetFullSystem()->GetCoreSystem()->GetDataCore();
        theDataCore->SetInstancePropertyValue(inInstanceHandle, GetNameProperty(), theName);
    }
}

Q3DStudio::CString CClientDataModelBridge::GetName(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle)
{
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    TDataStrPtr theString;
    if (m_Doc->GetStudioSystem()->IsInstance(inInstanceHandle)) {
        SValue theValue;
        if (thePropertySystem->GetInstancePropertyValue(inInstanceHandle, GetNameProperty(),
                                                        theValue)
            && GetValueType(theValue) == DataModelDataType::String)
            theString = qt3dsdm::get<TDataStrPtr>(theValue);
    }
    return (theString) ? Q3DStudio::CString(theString->GetData()) : "";
}

bool CClientDataModelBridge::IsActiveComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    using namespace qt3dsdm;
    using namespace Q3DStudio;
    CDoc &theDoc(*m_Doc);
    Qt3DSDMInstanceHandle theInstance = inInstance;
    Qt3DSDMSlideHandle theSlide = theDoc.GetActiveSlide();
    SLong4 theGuid = GetComponentGuid(theSlide);
    CId theActiveComponentId(theGuid.m_Longs[0], theGuid.m_Longs[1], theGuid.m_Longs[2],
                             theGuid.m_Longs[3]);
    CId theInstanceId = GetGUID(theInstance);
    return theActiveComponentId == theInstanceId;
}

// Helper for getting the type as a wstring... returns "" if it can't figure things out
std::wstring GetInstanceType(IPropertySystem *inPropertySystem, Qt3DSDMInstanceHandle inInstance)
{
    std::wstring theReturn(L"");
    try {
        Qt3DSDMPropertyHandle theProperty =
            inPropertySystem->GetAggregateInstancePropertyByName(inInstance, L"type");
        SValue theTypeValue;
        if (theProperty
            && inPropertySystem->GetInstancePropertyValue(inInstance, theProperty, theTypeValue)) {
            theReturn.assign(qt3dsdm::get<TDataStrPtr>(theTypeValue)->GetData());
        }
    } catch (...) {
        theReturn.assign(L"");
    }

    return theReturn;
}

// Find which material that uses this image instance
bool CClientDataModelBridge::GetMaterialFromImageInstance(
    qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::Qt3DSDMInstanceHandle &outMaterialInstance,
    qt3dsdm::Qt3DSDMPropertyHandle &outProperty)
{
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    SLong4 theDeletedImageLong4 =
        GetNamedInstancePropertyValue<SLong4>(thePropertySystem, inInstance, L"id");

    TInstanceHandleList theInstances;
    m_DataCore->GetInstancesDerivedFrom(
        theInstances, m_DefaultMaterial.m_Instance); // Get all default material instances
    size_t theInstanceCount = theInstances.size();
    for (size_t theInstanceIndex = 0; theInstanceIndex < theInstanceCount; ++theInstanceIndex) {
        Qt3DSDMInstanceHandle theInstance = theInstances[theInstanceIndex];
        std::wstring theWideTypeString(GetInstanceType(thePropertySystem, theInstance));
        if (theWideTypeString == L"Material") {
            TPropertyHandleList theProperties;
            thePropertySystem->GetAggregateInstanceProperties(theInstance, theProperties);
            size_t thePropertyCount = theProperties.size();
            for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount;
                 ++thePropertyIndex) {
                Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];
                AdditionalMetaDataType::Value theAdditionalMetaDataType =
                    thePropertySystem->GetAdditionalMetaDataType(theInstance, theProperty);

                if (theAdditionalMetaDataType == AdditionalMetaDataType::Image) {
                    SLong4 theLong4PropertyValue = GetSpecificInstancePropertyValue<SLong4>(
                        thePropertySystem, theInstance, theProperty);
                    if (theDeletedImageLong4 == theLong4PropertyValue) {
                        outMaterialInstance = theInstance;
                        outProperty = theProperty;
                        return true;
                    }
                }
            }
        }
    }

    m_DataCore->GetInstancesDerivedFrom(
        theInstances, m_CustomMaterial.m_Instance); // Get all custom material instances
    theInstanceCount = theInstances.size();
    for (size_t theInstanceIndex = 0; theInstanceIndex < theInstanceCount; ++theInstanceIndex) {
        Qt3DSDMInstanceHandle theInstance = theInstances[theInstanceIndex];
        std::wstring theWideTypeString(GetInstanceType(thePropertySystem, theInstance));
        if (theWideTypeString == L"CustomMaterial") {
            TPropertyHandleList theProperties;
            thePropertySystem->GetAggregateInstanceProperties(theInstance, theProperties);
            size_t thePropertyCount = theProperties.size();
            for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount;
                 ++thePropertyIndex) {
                Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];
                AdditionalMetaDataType::Value theAdditionalMetaDataType =
                    thePropertySystem->GetAdditionalMetaDataType(theInstance, theProperty);

                if (theAdditionalMetaDataType == AdditionalMetaDataType::Image) {
                    SLong4 theLong4PropertyValue = GetSpecificInstancePropertyValue<SLong4>(
                        thePropertySystem, theInstance, theProperty);
                    if (theDeletedImageLong4 == theLong4PropertyValue) {
                        outMaterialInstance = theInstance;
                        outProperty = theProperty;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool CClientDataModelBridge::GetLayerFromImageProbeInstance(
    qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::Qt3DSDMInstanceHandle &outLayerInstance,
    qt3dsdm::Qt3DSDMPropertyHandle &outProperty)
{
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    SLong4 theDeletedImageLong4 =
        GetNamedInstancePropertyValue<SLong4>(thePropertySystem, inInstance, L"id");

    TInstanceHandleList theInstances;
    m_DataCore->GetInstancesDerivedFrom(
        theInstances, this->m_Layer.m_Instance); // Get all default Layer instances
    size_t theInstanceCount = theInstances.size();

    for (size_t theInstanceIndex = 0; theInstanceIndex < theInstanceCount; ++theInstanceIndex) {
        Qt3DSDMInstanceHandle theInstance = theInstances[theInstanceIndex];
        std::wstring theWideTypeString(GetInstanceType(thePropertySystem, theInstance));
        if (theWideTypeString == L"Layer") {
            // Layer should have only one image property, which is the light probe, but this is a
            // little more
            // generic should anyone ever add more in the future.
            TPropertyHandleList theProperties;
            thePropertySystem->GetAggregateInstanceProperties(theInstance, theProperties);
            size_t thePropertyCount = theProperties.size();
            for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount;
                 ++thePropertyIndex) {
                Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];
                AdditionalMetaDataType::Value theAdditionalMetaDataType =
                    thePropertySystem->GetAdditionalMetaDataType(theInstance, theProperty);

                if (theAdditionalMetaDataType == AdditionalMetaDataType::Image) {
                    SLong4 theLong4PropertyValue = GetSpecificInstancePropertyValue<SLong4>(
                        thePropertySystem, theInstance, theProperty);
                    if (theDeletedImageLong4 == theLong4PropertyValue) {
                        outLayerInstance = theInstance;
                        outProperty = theProperty;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// enable caching between BeginRender() to EndRender()
// assumption: instances (CSimpleDataCore.m_Objects) remain constant between BeginRender to
// EndRender
void CClientDataModelBridge::BeginRender()
{
    ClearCache();
}

void CClientDataModelBridge::EndRender()
{
    ClearCache();
}

void CClientDataModelBridge::ClearCache()
{
    m_CacheImageInstances.clear();
    m_CacheMaterialInstances.clear();
    m_CacheModelInstances.clear();
}

// requirement: inInstance must be derived from instance that has Guid property!
bool CClientDataModelBridge::DerivedGuidMatches(IDataCore &inDataCore,
                                                Qt3DSDMInstanceHandle inInstance,
                                                Qt3DSDMPropertyHandle inProperty, SLong4 inGuid)
{
    SValue theLong4;
    if (inDataCore.GetInstancePropertyValue(inInstance, inProperty, theLong4)
        && Equals(inGuid, theLong4.toOldSkool())) {
        return true;
    }
    return false;
}

qt3dsdm::Qt3DSDMInstanceHandle CClientDataModelBridge::GetInstanceByGUID(const Q3DStudio::CId &inId)
{
    return GetInstanceByGUID(GuidToLong4(inId));
}

void CClientDataModelBridge::ClearInstanceGUIDCache(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                    qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    if (inProperty == GetObjectDefinitions().m_Guided.m_GuidProp) {
        TInstanceToGUIDHash::iterator theEntry(m_CachedInstanceToGUIDHash.find(inInstance));
        if (theEntry != m_CachedInstanceToGUIDHash.end()) {
            m_CachedGUIDToInstancesHash.erase(theEntry->second);
            m_CachedInstanceToGUIDHash.erase(theEntry);
        }
    }
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetInstanceByGUID(SLong4 inLong4)
{
    if (inLong4 == SLong4(0, 0, 0, 0))
        return 0;
    if (m_InstanceCachePropertyChangedConnection == NULL && m_Doc->GetStudioSystem()
        && m_Doc->GetStudioSystem()->GetFullSystem()) {
        IStudioFullSystemSignalProvider *theProvider(
            m_Doc->GetStudioSystem()->GetFullSystem()->GetSignalProvider());
        m_InstanceCachePropertyChangedConnection = theProvider->ConnectInstancePropertyValue(
            std::bind(&CClientDataModelBridge::ClearInstanceGUIDCache, this, std::placeholders::_1,
                      std::placeholders::_2));
        m_InstanceCacheInstanceDeletedConnection = theProvider->ConnectInstanceDeleted(
            std::bind(&CClientDataModelBridge::ClearInstanceGUIDCache, this, std::placeholders::_1,
                        GetObjectDefinitions().m_Guided.m_GuidProp));
    }
    TGUIDInstanceHash::iterator theEntry(m_CachedGUIDToInstancesHash.find(inLong4));
    if (theEntry != m_CachedGUIDToInstancesHash.end()) {
        if (m_DataCore->IsInstance(theEntry->second))
            return theEntry->second;
        else {
            m_CachedInstanceToGUIDHash.erase(theEntry->second);
            m_CachedGUIDToInstancesHash.erase(theEntry);
        }
    }
    Qt3DSDMInstanceHandle retval = MaybeCacheGetInstanceByGUIDDerivedFrom(
        inLong4, m_CachedGuidInstances, GetObjectDefinitions().m_Guided.m_Instance,
        GetObjectDefinitions().m_Guided.m_GuidProp);
    m_CachedGUIDToInstancesHash.insert(std::make_pair(inLong4, retval));
    m_CachedInstanceToGUIDHash.insert(std::make_pair(retval, inLong4));
    return retval;
}

qt3dsdm::SLong4 CClientDataModelBridge::GetInstanceGUID(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    std::pair<TInstanceToGUIDHash::iterator, bool> theEntry(
        m_CachedInstanceToGUIDHash.insert(std::make_pair(inInstance, SLong4())));
    if (theEntry.second) {
        Q3DStudio::CId theInstanceGuid;
        theInstanceGuid.Generate();

        const Q3DStudio::TGUIDPacked thePackedInstanceGuid(theInstanceGuid);
        SLong4 theInstanceLong4;
        theInstanceLong4.m_Longs[0] = thePackedInstanceGuid.Data1;
        theInstanceLong4.m_Longs[1] = thePackedInstanceGuid.Data2;
        theInstanceLong4.m_Longs[2] = thePackedInstanceGuid.Data3;
        theInstanceLong4.m_Longs[3] = thePackedInstanceGuid.Data4;
        m_DataCore->SetInstancePropertyValue(inInstance, GetObjectDefinitions().m_Guided.m_GuidProp,
                                             SValue(theInstanceLong4));
        m_CachedGUIDToInstancesHash.insert(std::make_pair(theInstanceLong4, inInstance));
        m_CachedInstanceToGUIDHash.insert(std::make_pair(inInstance, theInstanceLong4));
        return theInstanceLong4;
    }
    return theEntry.first->second;
}

void CClientDataModelBridge::SetInstanceGUID(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                             qt3dsdm::SLong4 inGuid)
{
    m_DataCore->SetInstancePropertyValue(inInstance, GetObjectDefinitions().m_Guided.m_GuidProp,
                                         SValue(inGuid));
    m_CachedGUIDToInstancesHash.clear();
    m_CachedInstanceToGUIDHash.clear();
    erase_if(m_CachedGuidInstances,
             std::bind(std::equal_to<qt3dsdm::Qt3DSDMInstanceHandle>(), inInstance,
                       std::placeholders::_1));
    erase_if(m_CacheImageInstances,
             std::bind(std::equal_to<qt3dsdm::Qt3DSDMInstanceHandle>(), inInstance,
                       std::placeholders::_1));
    erase_if(m_CacheMaterialInstances,
             std::bind(std::equal_to<qt3dsdm::Qt3DSDMInstanceHandle>(), inInstance,
                       std::placeholders::_1));
    erase_if(m_CacheModelInstances,
             std::bind(std::equal_to<qt3dsdm::Qt3DSDMInstanceHandle>(), inInstance,
                       std::placeholders::_1));
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetImageInstanceByGUID(const Q3DStudio::CId &inId)
{
    return GetImageInstanceByGUID(GuidToLong4(inId));
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetImageInstanceByGUID(SLong4 inLong4)
{
    if (inLong4 == SLong4(0, 0, 0, 0)) // invalid GUID
        return 0;
    return GetInstanceByGUID(inLong4);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetMaterialInstanceByGUID(const Q3DStudio::CId &inId)
{
    return GetMaterialInstanceByGUID(GuidToLong4(inId));
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetMaterialInstanceByGUID(SLong4 inLong4)
{
    return MaybeCacheGetInstanceByGUIDDerivedFrom(inLong4, m_CacheMaterialInstances,
                                                  m_DefaultMaterial.m_Instance,
                                                  GetObjectDefinitions().m_Guided.m_GuidProp);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetModelInstanceByGUID(const Q3DStudio::CId &inId)
{
    return GetModelInstanceByGUID(GuidToLong4(inId));
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetModelInstanceByGUID(SLong4 inLong4)
{
    return MaybeCacheGetInstanceByGUIDDerivedFrom(inLong4, m_CacheModelInstances,
                                                  m_Model.m_Instance,
                                                  GetObjectDefinitions().m_Guided.m_GuidProp);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetComponentInstanceByGUID(const Q3DStudio::CId &inId)
{
    return GetComponentInstanceByGUID(GuidToLong4(inId));
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetComponentInstanceByGUID(SLong4 inLong4)
{
    return GetInstanceByGUIDDerivedFrom(inLong4, m_SlideItem.m_Instance, m_SlideItem.m_ComponentId);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::MaybeCacheGetInstanceByGUIDDerivedFrom(
    SLong4 inLong4, TInstanceHandleList &ioCacheInstances, Qt3DSDMInstanceHandle inParentHandle,
    Qt3DSDMPropertyHandle inProperty)
{
    if (m_CacheEnabled) {
        if (ioCacheInstances.empty())
            m_DataCore->GetInstancesDerivedFrom(ioCacheInstances, inParentHandle); // build cache
        return GetInstanceByGUIDDerivedFrom(inLong4, ioCacheInstances, inProperty);
    } else
        return GetInstanceByGUIDDerivedFrom(inLong4, inParentHandle, inProperty);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetInstanceByGUIDDerivedFrom(
    SLong4 inLong4, Qt3DSDMInstanceHandle inParentHandle, Qt3DSDMPropertyHandle inProperty)
{
    // Run through all the instances derived from parent, and if a guid matches return that
    // instance.
    TInstanceHandleList instances;
    m_DataCore->GetInstancesDerivedFrom(instances, inParentHandle);
    return GetInstanceByGUIDDerivedFrom(inLong4, instances, inProperty);
}

Qt3DSDMInstanceHandle CClientDataModelBridge::GetInstanceByGUIDDerivedFrom(
    SLong4 inLong4, const TInstanceHandleList &instances, Qt3DSDMPropertyHandle inProperty)
{
    // Run through all the instances derived from parent, and if a guid matches return that
    // instance.
    TInstanceHandleList::const_iterator existing =
        find_if(instances.begin(), instances.end(),
                std::bind(CClientDataModelBridge::DerivedGuidMatches, std::ref(*m_DataCore),
                          std::placeholders::_1, inProperty, inLong4));
    if (existing != instances.end())
        return *existing;
    return 0;
}

qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetInstance(qt3dsdm::Qt3DSDMInstanceHandle inRoot,
                                    const qt3dsdm::SObjectRefType &inValue)
{
    using namespace qt3dsdm;
    switch (inValue.GetReferenceType()) {
    case ObjectReferenceType::Absolute:
        return GetInstanceByGUID(get<SLong4>(inValue.m_Value));
    case ObjectReferenceType::Relative: {
        Q3DStudio::CString thePath(qt3dsdm::get<qt3dsdm::TDataStrPtr>(inValue.m_Value)->GetData());
        IObjectReferenceHelper *theObjRefHelper = m_Doc->GetDataModelObjectReferenceHelper();
        if (theObjRefHelper) {
            bool theFullResolvedFlag;
            CRelativePathTools::EPathType thePathType;
            return CRelativePathTools::FindAssetInstanceByObjectPath(
                m_Doc, inRoot, thePath, thePathType, theFullResolvedFlag, theObjRefHelper);
        }
    } break;
    }
    return 0;
}
// Get the instance handle from the info stored in inValue
// inValue can be either of type long and TDataStrPtr only
// if it's long, it's the instance handle stored as long
// if it's long4, it's the GUID
// if it's a string, it's a path and so we need to decode the path to find the asset
qt3dsdm::Qt3DSDMInstanceHandle CClientDataModelBridge::GetInstance(qt3dsdm::Qt3DSDMInstanceHandle inRoot,
                                                                const qt3dsdm::SValue &inValue)
{
    using namespace qt3dsdm;
    return GetInstance(inRoot, ConvertToObjectRef(inValue));
}

std::pair<qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::SLong4>
CClientDataModelBridge::CreateImageInstance(qt3dsdm::Qt3DSDMInstanceHandle inSourceInstance,
                                            Qt3DSDMPropertyHandle inSlot,
                                            Qt3DSDMSlideHandle inSlide)
{
    Qt3DSDMInstanceHandle theNewInstance =
        m_Doc->GetStudioSystem()
            ->GetPropertySystem()
            ->CreateInstance(); // this should call CDataCoreProducer::CreateInstance(). If there is
                                // Consumer, the action can be undo
    m_DataCore->DeriveInstance(theNewInstance, m_SceneImage.m_Instance);

    // Set non-per-slide information *before* associating with slide

    SValue theName;
    m_DataCore->GetInstancePropertyValue(inSourceInstance, GetNameProperty(), theName);
    m_DataCore->SetInstancePropertyValue(theNewInstance, GetNameProperty(), theName);

    Q3DStudio::CId theInstanceGuid;
    theInstanceGuid.Generate();

    const Q3DStudio::TGUIDPacked thePackedInstanceGuid(theInstanceGuid);
    SLong4 theInstanceLong4;
    theInstanceLong4.m_Longs[0] = thePackedInstanceGuid.Data1;
    theInstanceLong4.m_Longs[1] = thePackedInstanceGuid.Data2;
    theInstanceLong4.m_Longs[2] = thePackedInstanceGuid.Data3;
    theInstanceLong4.m_Longs[3] = thePackedInstanceGuid.Data4;
    m_DataCore->SetInstancePropertyValue(theNewInstance, GetObjectDefinitions().m_Guided.m_GuidProp,
                                         SValue(theInstanceLong4));

    // Library objects does not need to be associated with any slide.
    if (inSlide.Valid())
        m_Doc->GetStudioSystem()->GetSlideSystem()->AssociateInstanceWithSlide(inSlide,
                                                                               theNewInstance);

    if (m_DefaultMaterial.m_SpecularReflection.m_Property
        == inSlot) { // Special case by spec, new "Specular Reflection" slot images hard set to
                     // mapping mode "Environment"
        m_DataCore->SetInstancePropertyValue(theNewInstance, m_SceneImage.m_TextureMapping,
                                             TDataStrPtr(new CDataStr(L"Environmental Mapping")));
    } else if (m_Layer.m_LightProbe.m_Property == inSlot
               || m_Layer.m_LightProbe2.m_Property == inSlot) {
        m_DataCore->SetInstancePropertyValue(theNewInstance, m_SceneImage.m_TextureMapping,
                                             TDataStrPtr(new CDataStr(L"Light Probe")));
    } else if (m_MaterialBase.m_IblProbe.m_Property == inSlot) {
        m_DataCore->SetInstancePropertyValue(theNewInstance, m_SceneImage.m_TextureMapping,
                                             TDataStrPtr(new CDataStr(L"IBL Override")));
    }

    return std::make_pair(theNewInstance, theInstanceLong4);
}

Q3DStudio::CId CClientDataModelBridge::GetId(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                             qt3dsdm::Qt3DSDMPropertyHandle inProperty) const
{
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    SValue theSourceValue;
    if (m_DataCore->IsInstance(inInstance)
        && thePropertySystem->HasAggregateInstanceProperty(
               inInstance, inProperty) // check if the property exists before querying the value
        && thePropertySystem->GetInstancePropertyValue(
               inInstance, inProperty,
               theSourceValue)) // this function may throw error if the property doesn't exist
    {
        SLong4 theLong4 = qt3dsdm::get<SLong4>(theSourceValue);
        return Q3DStudio::CId(theLong4.m_Longs[0], theLong4.m_Longs[1], theLong4.m_Longs[2],
                              theLong4.m_Longs[3]);
    }
    return Q3DStudio::CId();
}

// Check whether this instance is inside the current active component
bool CClientDataModelBridge::IsInActiveComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    qt3dsdm::Qt3DSDMInstanceHandle theActiveRoot = m_Doc->GetActiveRootInstance();
    return (theActiveRoot == GetOwningComponentInstance(inInstance));
}

// Check whether inInstance is inside inComponentInstance
bool CClientDataModelBridge::IsInComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                           qt3dsdm::Qt3DSDMInstanceHandle inComponentInstance)
{
    return (GetOwningComponentInstance(inInstance) == inComponentInstance);
}

//==============================================================================
/**
 * Get the component that this instance is a member of.
 * The component is the independent group or the Scene. inIsFirstCall is used
 * to ignore the first independent group in order to get it's parent.
 * @param inIsFirstCall true if the child is to be ignored as being a component.
 * @return the parent component, or this if the parent is NULL.
 */
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetParentComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                           bool inIsFirstCall /*= true*/)
{
    if (!inIsFirstCall && (IsSceneInstance(inInstance) || IsComponentInstance(inInstance)))
        return inInstance;

    qt3dsdm::Qt3DSDMInstanceHandle theParentInstance = GetParentInstance(inInstance);
    if (theParentInstance.Valid())
        return GetParentComponent(theParentInstance, false);
    else
        return 0;
}

//=============================================================================
/**
 *	Get a unique, non-conflicting name for a child.
 *	This will stip off all trailing numbers, find the base name then find a
 *	non-used index from the base name.
 *	If nothing already exists at inDesiredName then no index will be added.
 *	@param inDesiredName the desired base name for the object.
 *	@return a unique name that no other child has.
 */
Q3DStudio::CString
CClientDataModelBridge::GetUniqueChildName(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                                           qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                           Q3DStudio::CString inDesiredName)
{
    // fyi, if we get abc123, theBaseName gets abc and theBaseIndex gets 123

    Q3DStudio::CString theBaseName = inDesiredName;
    Q3DStudio::CString theBaseIndex;

    // Strip off all trailing numbers to get the base name
    if (theBaseName.Length() > 0) {
        wchar_t theLastChar = static_cast<const wchar_t *>(theBaseName)[theBaseName.Length() - 1];
        while (theBaseName.Length() > 0 && theLastChar >= '0' && theLastChar <= '9') {
            // get a wchar_t* so we can index
            const wchar_t *theBasePtr = theBaseName;
            // find where the desired character is
            long theDesiredOffset = theBaseName.Length() - 1;
            // make a CString of the desired character and concatenate with the existing base index
            theBaseIndex = Q3DStudio::CString(theBasePtr + theDesiredOffset, 1) + theBaseIndex;

            // take out that last character
            theBaseName = theBaseName.Extract(0, theBaseName.Length() - 1);

            // get the new last char
            if (theBaseName.Length() > 0)
                theLastChar = static_cast<const wchar_t *>(theBaseName)[theBaseName.Length() - 1];
        }
    }

    qt3dsdm::Qt3DSDMInstanceHandle theExistingChild = 0;
    // If there is a base name then use it
    if (theBaseName.Length() > 0)
        theExistingChild = GetChildByName(inParent, inDesiredName, inInstance);
    else // there is no base name, just set it to a random setting so it'll fall into the while loop
        theExistingChild = inParent;

    Q3DStudio::CString theUniqueName = inDesiredName;

    if (theExistingChild != 0 && theExistingChild != inInstance) {
        long theIndex;
        if (theBaseIndex.Length() != 0)
            theIndex = atoi(theBaseIndex.GetCharStar());
        else
            theIndex = 2;

        // If the name is in use then increment the index until one is found.
        while (theExistingChild != 0 && theExistingChild != inInstance) {
            theUniqueName.Format(_LSTR("%ls%d"), static_cast<const wchar_t *>(theBaseName), theIndex);
            ++theIndex;
            theExistingChild = GetChildByName(inParent, theUniqueName, inInstance);
        }
    }

    return theUniqueName;
}

bool CClientDataModelBridge::CheckNameUnique(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                             Q3DStudio::CString inDesiredName)
{
    qt3dsdm::Qt3DSDMInstanceHandle theExistingChild = 0;
    if (inDesiredName.Length() > 0) {
        theExistingChild = GetChildByName(GetParentInstance(inInstance), inDesiredName,
                                          qt3dsdm::Qt3DSDMInstanceHandle());
    }

    return ((int)theExistingChild == 0 || theExistingChild == inInstance);
}

//=============================================================================
/**
 *	Get SourcePath value for this instance
 */
Q3DStudio::CString
CClientDataModelBridge::GetSourcePath(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    if (inInstance.Valid()) {
        qt3dsdm::SValue theValue;
        IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
        thePropertySystem->GetInstancePropertyValue(inInstance, m_SceneAsset.m_SourcePath,
                                                    theValue);
        return qt3dsdm::get<TDataStrPtr>(theValue)->GetData();
    } else
        return L"";
}

//=============================================================================
/**
 *	Get all instances that are derived from ItemBase Instance.
 */
TInstanceHandleList CClientDataModelBridge::GetItemBaseInstances() const
{
    TInstanceHandleList theInstances;
    m_DataCore->GetInstancesDerivedFrom(theInstances, m_SceneAsset.m_Instance);
    return theInstances;
}

inline void AddSourcePathToList(std::set<Q3DStudio::CString> &ioSourcePathList,
                                const SValue &inValue)
{
    Q3DStudio::CFilePath theSourcePath = qt3dsdm::get<TDataStrPtr>(inValue)->GetData();
    if (!theSourcePath.filePath().isEmpty())
        ioSourcePathList.insert(theSourcePath.toCString());
}

//=============================================================================
/**
 *	Get list of values from all instances derived from inParentInstance
 */
std::vector<SValue> CClientDataModelBridge::GetValueList(Qt3DSDMInstanceHandle inParentInstance,
                                                         Qt3DSDMPropertyHandle inProperty,
                                                         IValueFilter *inFilter) const
{
    std::vector<SValue> theValueList;
    TInstanceHandleList theInstances;
    m_DataCore->GetInstancesDerivedFrom(theInstances, inParentInstance);

    // Iterate through each instance derived from inParentInstance and get the inProperty property
    // value.
    for (TInstanceHandleList::const_iterator theIter = theInstances.begin();
         theIter != theInstances.end(); ++theIter) {
        // Skip the parent instance.
        if (*theIter == inParentInstance)
            continue;

        GetValueListFromAllSlides(*theIter, inProperty, theValueList, inFilter);
    }

    return theValueList;
}

//=============================================================================
/**
 *	Get list of values from all slides
 */
void CClientDataModelBridge::GetValueListFromAllSlides(Qt3DSDMInstanceHandle inInstance,
                                                       Qt3DSDMPropertyHandle inProperty,
                                                       std::vector<SValue> &outValueList,
                                                       IValueFilter *inFilter) const
{
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    ISlideCore *theSlideCore = m_Doc->GetStudioSystem()->GetSlideCore();

    // Check if the instance is in master slide and if the property is unlinked.
    // This will determine how we should query the value.
    qt3dsdm::Qt3DSDMSlideHandle theSlide = theSlideSystem->GetAssociatedSlide(inInstance);
    if (theSlide.Valid() && theSlideSystem->IsMasterSlide(theSlide)
        && !theSlideSystem->IsPropertyLinked(inInstance, inProperty)) {
        // If the instance is in master slide and the property is unlinked, we need to query the
        // value from each slides
        size_t theSlideCount =
            theSlideSystem->GetSlideCount(theSlideSystem->GetAssociatedSlide(inInstance));
        for (size_t theSlideIndex = 0; theSlideIndex < theSlideCount; ++theSlideIndex) {
            qt3dsdm::SValue theValue;
            Qt3DSDMSlideHandle theSpecificSlide =
                theSlideSystem->GetSlideByIndex(theSlide, theSlideIndex);
            if (theSlideCore->GetSpecificInstancePropertyValue(theSpecificSlide, inInstance,
                                                               inProperty, theValue)
                && (inFilter == NULL || inFilter->KeepValue(inInstance, inProperty, theValue)))
                outValueList.push_back(theValue);
        }
    } else {
        // Else, we can get the property value
        qt3dsdm::SValue theValue;
        if (thePropertySystem->GetInstancePropertyValue(inInstance, inProperty, theValue)
            && (inFilter == NULL
                || inFilter->KeepValue(inInstance, inProperty, theValue.toOldSkool())))
            outValueList.push_back(theValue.toOldSkool());
    }
}

// Temporary hack to keep the layer's sourcepath property from setting off the file-not-found
// warnings.  This will go away when we get to the point where we can do better.
struct SValueListFilter : public IValueFilter
{
    const CClientDataModelBridge &m_Bridge;
    SValueListFilter(const CClientDataModelBridge &bridge)
        : m_Bridge(bridge)
    {
    }
    bool KeepValue(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle /*inProperty*/,
                           const qt3dsdm::SValue & /*inValue*/) override
    {
        return !m_Bridge.IsLayerInstance(inInstance);
    }
};

//=============================================================================
/**
 *	Get SourcePath list from all instances
 */
std::set<Q3DStudio::CString> CClientDataModelBridge::GetSourcePathList() const
{
    // Get the source path property list
    SValueListFilter theFilter(*this);
    std::vector<SValue> theValueList =
        GetValueList(m_SceneAsset.m_Instance, m_SceneAsset.m_SourcePath, &theFilter);

    // Translate from SValue to Q3DStudio::CString and also remove the identifier
    std::set<Q3DStudio::CString> theSourcePathList;
    for (std::vector<SValue>::iterator theIter = theValueList.begin();
         theIter != theValueList.end(); ++theIter)
        AddSourcePathToList(theSourcePathList, *theIter);

    return theSourcePathList;
}

inline void AddStringToList(std::set<Q3DStudio::CString> &ioStringList, const SValue &inValue)
{
    Q3DStudio::CString theString = qt3dsdm::get<TDataStrPtr>(inValue)->GetData();
    if (theString != L"")
        ioStringList.insert(theString);
}

//=============================================================================
/**
 *	Get Font file list from all Text instances
 */
std::set<Q3DStudio::CString> CClientDataModelBridge::GetFontFileList() const
{
    // Get the font name property list
    std::vector<SValue> theValueList = GetValueList(m_Text.m_Instance, m_Text.m_Font);
    std::set<Q3DStudio::CString> theFontNameList;
    for (std::vector<SValue>::iterator theIter = theValueList.begin();
         theIter != theValueList.end(); ++theIter)
        AddStringToList(theFontNameList, *theIter);

    // early return
    if (theFontNameList.empty())
        return theFontNameList;

    // Translate the font name to font file
    std::set<Q3DStudio::CString> theFontFileList;
    std::vector<std::pair<Q3DStudio::CString, Q3DStudio::CString>> theFontNameFileList;
    m_Doc->GetProjectFonts(theFontNameFileList);
    for (std::set<Q3DStudio::CString>::iterator theFontNameIter = theFontNameList.begin();
         theFontNameIter != theFontNameList.end(); ++theFontNameIter) {
        // Given the font name, try to get the font file from the list of fonts registered in
        // Studio.
        // If the font is not found, it means that we are using missing font file.
        // Create some non-existing path to inform user that this font is missing.
        bool theFontFound = false;
        Q3DStudio::CString theFontFile;
        for (size_t idx = 0, end = theFontNameFileList.size(); idx < end; ++idx) {
            if (theFontNameFileList[idx].first == *theFontNameIter) {
                theFontFound = true;
                theFontFile = theFontNameFileList[idx].second;
                break;
            }
        }
        if (!theFontFound) {
            theFontFile = L"fonts\\File with font name [";
            theFontFile.append(*theFontNameIter);
            theFontFile.append(" ]");
        }
        theFontFileList.insert(Q3DStudio::CFilePath(theFontFile).toCString());
    }
    return theFontFileList;
}

static void GetDynamicObjecTextures(IDataCore &inDataCore, IPropertySystem &inPropertySystem,
                                    Qt3DSDMInstanceHandle inBaseInstance,
                                    std::vector<SValue> &outValues,
                                    const CClientDataModelBridge &inBridge)
{
    std::vector<SValue> &theValueList(outValues);
    // Get all effect instances
    TInstanceHandleList theDerivedInstances;
    inDataCore.GetInstancesDerivedFrom(theDerivedInstances, inBaseInstance);

    // Iterate each effect instance and check if it has Texture property
    IPropertySystem *thePropertySystem = &inPropertySystem;
    for (TInstanceHandleList::const_iterator theIter = theDerivedInstances.begin();
         theIter != theDerivedInstances.end(); ++theIter) {
        Qt3DSDMInstanceHandle theInstance = *theIter;

        // Skip the parent instance
        // This works because when we import a custom material, we create a new class that is
        // derived from the
        // custom material base instance.  We then add all properties and defaults to this new
        // class.  Finally
        // we derive all scene level instances from this new class.  Thus the new class's parent is
        // the base instance
        // and the new class is the parent of the scene level instance.
        TInstanceHandleList theParentList;
        inDataCore.GetInstanceParents(theInstance, theParentList);
        if (std::find(theParentList.begin(), theParentList.end(), inBaseInstance)
            != theParentList.end())
            continue;

        // Iterate through all property and check if it is Texture property
        TPropertyHandleList theProperties;
        thePropertySystem->GetAggregateInstanceProperties(theInstance, theProperties);
        size_t thePropertyCount = theProperties.size();
        for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount; ++thePropertyIndex) {
            Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];
            qt3dsdm::AdditionalMetaDataType::Value theAdditionalMetaDataType =
                thePropertySystem->GetAdditionalMetaDataType(theInstance, theProperty);
            if (theAdditionalMetaDataType == AdditionalMetaDataType::Texture) {
                // Get the value list of texture property
                inBridge.GetValueListFromAllSlides(theInstance, theProperty, theValueList);
            }
        }
    }
}

//=============================================================================
/**
 *	Get texture list from all effect instances
 */
std::set<Q3DStudio::CString> CClientDataModelBridge::GetDynamicObjectTextureList() const
{
    std::vector<SValue> theValueList;

    // Get all effect instances
    TInstanceHandleList theEffectInstances;
    m_DataCore->GetInstancesDerivedFrom(theEffectInstances, m_Effect.m_Instance);
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    GetDynamicObjecTextures(*m_DataCore, *thePropertySystem, m_Effect.m_Instance, theValueList,
                            *this);
    GetDynamicObjecTextures(*m_DataCore, *thePropertySystem, m_CustomMaterial.m_Instance,
                            theValueList, *this);

    // Translate from SValue to Q3DStudio::CString and also remove the identifier
    std::set<Q3DStudio::CString> theSourcePathList;
    for (std::vector<SValue>::iterator theIter = theValueList.begin();
         theIter != theValueList.end(); ++theIter)
        AddSourcePathToList(theSourcePathList, *theIter);

    return theSourcePathList;
}

bool CClientDataModelBridge::IsLockedAtAll(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    qt3dsdm::SValue theValue;
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    if (thePropertySystem->HasAggregateInstanceProperty(inInstance, GetSceneAsset().m_Locked)) {
        thePropertySystem->GetInstancePropertyValue(inInstance, GetSceneAsset().m_Locked, theValue);
        return qt3dsdm::get<bool>(theValue);
    }
    return false;
}

bool CClientDataModelBridge::IsDuplicateable(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (!inInstance.Valid())
        return false;

    EStudioObjectType theObjectType = GetObjectType(inInstance);
    if (theObjectType == OBJTYPE_SCENE || theObjectType == OBJTYPE_MATERIAL
        || theObjectType == OBJTYPE_IMAGE)
        return false;
    // If we are delving inside component and selecting the component itself (the component is root
    // in timeline palette)
    else if (theObjectType == OBJTYPE_COMPONENT && IsActiveComponent(inInstance))
        return false;
    else
        return !IsLockedAtAll(inInstance);
}

bool CClientDataModelBridge::IsMultiSelectable(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (!m_DataCore->IsInstance(inInstance))
        return false;
    EStudioObjectType theObjectType = GetObjectType(inInstance);

    bool isPotentiallySelectable = theObjectType == OBJTYPE_LIGHT || theObjectType == OBJTYPE_CAMERA
        || theObjectType == OBJTYPE_MODEL || theObjectType == OBJTYPE_GROUP
        || theObjectType == OBJTYPE_COMPONENT || theObjectType == OBJTYPE_TEXT
        || theObjectType == OBJTYPE_ALIAS || theObjectType == OBJTYPE_PATH;

    if (!isPotentiallySelectable)
        return false;

    // If we are delving inside component and selecting the component itself (the component is root
    // in timeline palette)
    if (theObjectType == OBJTYPE_COMPONENT && IsActiveComponent(inInstance))
        return false;

    return !IsLockedAtAll(inInstance);
}

bool CClientDataModelBridge::CanDelete(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (!inInstance.Valid())
        return false;

    switch (GetObjectType(inInstance)) {
    case OBJTYPE_MODEL:
    case OBJTYPE_TEXT:
    case OBJTYPE_GROUP:
    case OBJTYPE_CAMERA:
    case OBJTYPE_LIGHT:
    case OBJTYPE_IMAGE:
    case OBJTYPE_ALIAS:
    case OBJTYPE_PATH:
    case OBJTYPE_PATHANCHORPOINT:
    case OBJTYPE_SUBPATH:
    case OBJTYPE_EFFECT:
    case OBJTYPE_DATAINPUT:
        return !IsLockedAtAll(inInstance);
        break;
    case OBJTYPE_COMPONENT:
        return !IsLockedAtAll(inInstance) && !IsActiveComponent(inInstance);
        break;
    case OBJTYPE_LAYER:
        // We could not delete a layer if
        // 1. if the deleted layer is in master slide, and there is only 1 master layer
        // otherwise it means it is not the last layer, and can be deleted.
        if (IsMaster(inInstance)) {
            qt3dsdm::ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
            qt3dsdm::TInstanceHandleList theInstanceList;
            qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
                theSlideSystem->GetAssociatedSlide(inInstance);
            theSlideSystem->GetAssociatedInstances(theMasterSlide, theInstanceList);
            long theMasterLayerCount = 0;
            for (qt3dsdm::TInstanceHandleList::const_iterator theIter = theInstanceList.begin();
                 theIter != theInstanceList.end(); ++theIter) {
                if (IsLayerInstance(*theIter))
                    ++theMasterLayerCount;
            }
            if (theMasterLayerCount == 1)
                return false;
        }

        return !IsLockedAtAll(inInstance);
        break;
    case OBJTYPE_BEHAVIOR:
        return true;
        break;
    case OBJTYPE_MATERIAL:
    case OBJTYPE_LIGHTMAPS:
    case OBJTYPE_SCENE:
        return false;
        break;
    default:
        return false;
        break;
    }
}

bool CClientDataModelBridge::IsMaster(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (IsSceneInstance(inInstance))
        return true;
    else {
        ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theSlideHandle = theSlideSystem->GetAssociatedSlide(inInstance);
        return theSlideSystem->IsMasterSlide(theSlideHandle);
    }
}

//=============================================================================
/**
 *	Get the layer that this instance lies in.
 */
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetResidingLayer(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (!inInstance.Valid())
        return 0;

    if (IsLayerInstance(inInstance))
        return inInstance;
    else {
        qt3dsdm::Qt3DSDMInstanceHandle theParent = GetParentInstance(inInstance);
        if (theParent.Valid())
            return GetResidingLayer(theParent);
        else
            return 0;
    }
}

//=============================================================================
/**
 *	Get a child object by it's name in the active Slide.
 *	This is meant to only be used by GetUniqueChildName, names are not Unique
 *	so this should be used with extreme care.
 *	@param inName the name of the child object to fetch.
 *	@return the child that was found.
 */
qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetChildByName(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                                       Q3DStudio::CString inName,
                                       qt3dsdm::Qt3DSDMInstanceHandle skipInstance)
{
    Q3DStudio::CGraphIterator theChildren;
    GetAssetChildren(m_Doc, inParent, theChildren);

    for (; !theChildren.IsDone(); ++theChildren) {
        qt3dsdm::Qt3DSDMInstanceHandle theChildInstance = theChildren.GetCurrent();
        if (GetName(theChildInstance) == inName && skipInstance != theChildInstance)
            return theChildInstance;
    }
    return 0;
}

Q3DStudio::CId CClientDataModelBridge::GetGUID(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return GetId(inInstance, GetObjectDefinitions().m_Guided.m_GuidProp);
}

qt3dsdm::Qt3DSDMInstanceHandle
CClientDataModelBridge::GetParentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (IsImageInstance(inInstance)) {
        qt3dsdm::Qt3DSDMInstanceHandle theParentInstance;
        qt3dsdm::Qt3DSDMPropertyHandle theProperty;
        if (!GetMaterialFromImageInstance(inInstance, theParentInstance, theProperty))
            this->GetLayerFromImageProbeInstance(inInstance, theParentInstance, theProperty);
        return theParentInstance;
    } else {
        Q3DStudio::TGraphPtr theGraph = m_Doc->GetAssetGraph();
        if (theGraph->IsExist(inInstance))
            return theGraph->GetParent(inInstance);
        else
            return 0;
    }
}

EStudioObjectType CClientDataModelBridge::GetObjectType(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    SValue theTypeValue;
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    thePropertySystem->GetInstancePropertyValue(inInstance, GetTypeProperty(), theTypeValue);
    std::wstring theType(qt3dsdm::get<TDataStrPtr>(theTypeValue)->GetData());

    if (theType == L"Behavior")
        return OBJTYPE_BEHAVIOR;
    else if (theType == L"Camera")
        return OBJTYPE_CAMERA;
    else if (theType == L"Group")
        return OBJTYPE_GROUP;
    else if (theType == L"Component")
        return OBJTYPE_COMPONENT;
    else if (theType == L"Image" || theType == L"LibraryImage")
        return OBJTYPE_IMAGE;
    else if (theType == L"Layer")
        return OBJTYPE_LAYER;
    else if (theType == L"Light")
        return OBJTYPE_LIGHT;
    else if (theType == L"Material")
        return OBJTYPE_MATERIAL;
    else if (theType == L"Model")
        return OBJTYPE_MODEL;
    else if (theType == L"Alias")
        return OBJTYPE_ALIAS;
    else if (theType == L"Scene")
        return OBJTYPE_SCENE;
    else if (theType == L"Slide")
        return OBJTYPE_SLIDE;
    else if (theType == L"Text")
        return OBJTYPE_TEXT;
    else if (theType == L"Effect")
        return OBJTYPE_EFFECT;
    else if (theType == L"RenderPlugin")
        return OBJTYPE_RENDERPLUGIN;
    else if (theType == L"CustomMaterial")
        return OBJTYPE_CUSTOMMATERIAL;
    else if (theType == L"ReferencedMaterial")
        return OBJTYPE_REFERENCEDMATERIAL;
    else if (theType == L"Path")
        return OBJTYPE_PATH;
    else if (theType == L"PathAnchorPoint")
        return OBJTYPE_PATHANCHORPOINT;
    else if (theType == L"SubPath")
        return OBJTYPE_SUBPATH;
    else if (theType == L"DataInput")
        return OBJTYPE_DATAINPUT;
    else if (theType == L"Lightmaps")
        return OBJTYPE_LIGHTMAPS;
    else {
        ASSERT(0);
        return OBJTYPE_UNKNOWN;
    }
}

bool CClientDataModelBridge::IsBehaviorInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Behavior.m_Instance);
}

bool CClientDataModelBridge::IsCameraInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Camera.m_Instance);
}

bool CClientDataModelBridge::IsGroupInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Group.m_Instance);
}

bool CClientDataModelBridge::IsActionInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_ActionItem.m_Instance);
}

bool CClientDataModelBridge::IsComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Component.m_Instance);
}

bool CClientDataModelBridge::IsLayerInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Layer.m_Instance);
}

bool CClientDataModelBridge::IsLightInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Light.m_Instance);
}
bool CClientDataModelBridge::IsModelInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Model.m_Instance);
}

bool CClientDataModelBridge::IsMaterialBaseInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_MaterialBase.m_Instance);
}

bool CClientDataModelBridge::IsMaterialInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_DefaultMaterial.m_Instance);
}

bool CClientDataModelBridge::IsImageInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_SceneImage.m_Instance);
}

bool CClientDataModelBridge::IsSceneInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Scene.m_Instance);
}

bool CClientDataModelBridge::IsTextInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Text.m_Instance);
}

bool CClientDataModelBridge::IsEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Effect.m_Instance);
}

bool CClientDataModelBridge::IsRenderPluginInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_RenderPlugin.m_Instance);
}

bool CClientDataModelBridge::IsCustomMaterialInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_CustomMaterial.m_Instance);
}

bool CClientDataModelBridge::IsDataInputInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_DataInput.m_Instance);
}

bool CClientDataModelBridge::IsReferencedMaterialInstance(
    qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(
        inInstance, m_ObjectDefinitions->m_ReferencedMaterial.m_Instance);
}

bool CClientDataModelBridge::IsLightmapsInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Lightmaps.m_Instance);
}

bool CClientDataModelBridge::IsNodeType(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, m_Node.m_Instance);
}
bool CClientDataModelBridge::IsSceneGraphInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const
{
    if (m_DataCore->IsInstance(inInstance) == false)
        return false;
    return IsNodeType(inInstance) || IsSceneInstance(inInstance) || IsImageInstance(inInstance)
        || IsMaterialInstance(inInstance) || IsCustomMaterialInstance(inInstance)
        || IsReferencedMaterialInstance(inInstance) || IsRenderPluginInstance(inInstance)
        || IsEffectInstance(inInstance) || IsBehaviorInstance(inInstance)
        || IsDataInputInstance(inInstance);
}

bool SActionInvalidProperty::operator()(qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    using namespace qt3dsdm;
    AdditionalMetaDataType::Value theType =
        m_PropertySystem.GetAdditionalMetaDataType(m_Instance, inProperty);
    if (theType == AdditionalMetaDataType::Image || theType == AdditionalMetaDataType::Texture
        || theType == AdditionalMetaDataType::ObjectRef || theType == AdditionalMetaDataType::Mesh
        || theType == AdditionalMetaDataType::Import)
        return true;
    return false;
}

// Get all actions that used this instance as either the trigger or target object
void CClientDataModelBridge::GetReferencedActions(Qt3DSDMInstanceHandle inReferencedInstance,
                                                  long inReferencedMode,
                                                  TActionHandleList &outActions)
{
    IActionCore *theActionCore = m_Doc->GetStudioSystem()->GetActionCore();
    TActionHandleList theActions;
    theActionCore->GetActions(theActions);
    bool theRefViaOwner = ((inReferencedMode & REFERENCED_AS_OWNER) != 0);
    bool theRefViaTrigger = ((inReferencedMode & REFERENCED_AS_TRIGGER) != 0);
    bool theRefViaTarget = ((inReferencedMode & REFERENCED_AS_TARGET) != 0);

    for (TActionHandleList::iterator theIter = theActions.begin(); theIter != theActions.end();
         ++theIter) {
        SActionInfo theActionInfo = theActionCore->GetActionInfo(*theIter);

        if (!inReferencedInstance.Valid()
            || (theRefViaOwner && inReferencedInstance == theActionInfo.m_Owner)
            || (theRefViaTarget
                && inReferencedInstance
                    == GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject))
            || (theRefViaTrigger
                && inReferencedInstance
                    == GetInstance(theActionInfo.m_Owner, theActionInfo.m_TriggerObject)))
            outActions.push_back(*theIter);
    }
}

void CClientDataModelBridge::UpdateHandlerArgumentValue(qt3dsdm::HandlerArgumentType::Value inArgType,
                                                        qt3dsdm::Qt3DSDMInstanceHandle inTargetObject,
                                                        qt3dsdm::SValue inOrigValue,
                                                        qt3dsdm::SValue inNewValue)
{
    IActionCore *theActionCore = m_Doc->GetStudioSystem()->GetActionCore();
    TActionHandleList theActions;
    GetReferencedActions(inTargetObject, qt3dsdm::REFERENCED_AS_TARGET, theActions);

    for (TActionHandleList::iterator theAction = theActions.begin(); theAction != theActions.end();
         ++theAction) {
        THandlerArgHandleList theArgList;
        theActionCore->GetHandlerArguments(*theAction, theArgList);

        for (THandlerArgHandleList::iterator theArg = theArgList.begin();
             theArg != theArgList.end(); ++theArg) {
            SHandlerArgumentInfo theInfo(theActionCore->GetHandlerArgumentInfo(*theArg));
            if (theInfo.m_ArgType == inArgType && theInfo.m_Value == inOrigValue) {
                theActionCore->SetHandlerArgumentValue(*theArg, inNewValue);
            }
        }
    }
}

std::wstring CClientDataModelBridge::GetDefaultHandler(Qt3DSDMInstanceHandle inInstance,
                                                       std::wstring inOldHandler)
{
    IMetaData *theMetaData = m_Doc->GetStudioSystem()->GetActionMetaData();

    // We try to maintain old handler whenever possible.
    // This is to fix bug 6569: Maintain handler option under action palette when changing target
    // object
    if (inOldHandler != L""
        && theMetaData->FindHandlerByName(inInstance, inOldHandler.c_str()).Valid())
        return inOldHandler;

    // This is to fix bug 5106: Default action for components should be Go To Slide
    std::wstring theHandlerName;
    if (m_DataCore->IsInstanceOrDerivedFrom(inInstance,
                                            m_ObjectDefinitions->m_SlideOwner.m_Instance)) {
        theHandlerName = L"Go to Slide";
        // Verify that Go to Slide is valid, just in case MetaData.xml is changed
        if (theMetaData->FindHandlerByName(inInstance, theHandlerName.c_str()).Valid())
            return theHandlerName;
    }

    // Default to the first handler found
    THandlerHandleList theHandlerList;
    GetHandlers(inInstance, theHandlerList);
    if (theHandlerList.size() > 0)
        theHandlerName = theMetaData->GetHandlerInfo(theHandlerList[0])->m_Name.wide_str();
    else
        theHandlerName = L""; // set to unknown handler
    return theHandlerName;
}

std::wstring CClientDataModelBridge::GetDefaultEvent(Qt3DSDMInstanceHandle inInstance,
                                                     std::wstring inOldEvent)
{
    IMetaData *theMetaData = m_Doc->GetStudioSystem()->GetActionMetaData();

    // We try to maintain old event whenever possible.
    if (inOldEvent != L"" && theMetaData->FindEvent(inInstance, inOldEvent.c_str()).Valid())
        return inOldEvent;

    // Default to the first event found
    std::wstring theEventName;
    TEventHandleList theEventList;
    GetEvents(inInstance, theEventList);
    if (theEventList.size() > 0)
        theEventName = theMetaData->GetEventInfo(theEventList[0])->m_Name.wide_str();
    else
        theEventName = L""; // set to unknown event
    return theEventName;
}

// TODO : We should change the argument of Property to hold on to a long instead of
// the string.
// The long represents the property id which is what we really need. using the name, we need to do
// all those conversion

void CClientDataModelBridge::ResetHandlerArguments(Qt3DSDMActionHandle inAction,
                                                   Qt3DSDMHandlerHandle inHandler)
{
    // Remove old args
    IActionCore *theActionCore = m_Doc->GetStudioSystem()->GetActionCore();
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    THandlerArgHandleList theOldArgs;
    theActionCore->GetHandlerArguments(inAction, theOldArgs);
    for (THandlerArgHandleList::iterator theArg = theOldArgs.begin(); theArg != theOldArgs.end();
         ++theArg) {
        theActionCore->RemoveHandlerArgument(*theArg);
    }

    if (inHandler.Valid()) // there could be no valid handler for this type of object
    {
        IMetaData &theNewMetaData(*m_Doc->GetStudioSystem()->GetActionMetaData());
        typedef vector<SMetaDataHandlerArgumentInfo> THandlerArgMetaDataList;

        THandlerArgMetaDataList theNewArgs;
        theNewMetaData.GetHandlerArguments(inHandler, theNewArgs);

        for (THandlerArgMetaDataList::const_iterator theIter = theNewArgs.begin();
             theIter != theNewArgs.end(); ++theIter) {
            const SMetaDataHandlerArgumentInfo &theArgMetaData(*theIter);
            Qt3DSDMHandlerArgHandle theArgument = theActionCore->AddHandlerArgument(
                inAction, theArgMetaData.m_Name, theArgMetaData.m_ArgType,
                theArgMetaData.GetDataType());
            SValue theValue = theArgMetaData.m_DefaultValue;
            switch (theArgMetaData.m_ArgType) {
            case HandlerArgumentType::Event:
                theValue = 0; // TODO: Hardcode for now. Should query event meta data list.
                break;
            case HandlerArgumentType::Property: {
                SActionInfo theActionInfo = theActionCore->GetActionInfo(inAction);
                Qt3DSDMInstanceHandle theTargetObject =
                    GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject);
                qt3dsdm::TPropertyHandleList theProperties;
                m_DataCore->GetAggregateInstanceProperties(theTargetObject, theProperties);
                erase_if(theProperties,
                         SActionInvalidProperty(*thePropertySystem, theTargetObject));
                if (theProperties.size() > 0) {
                    theValue = qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(
                        thePropertySystem->GetName(theProperties[0]).wide_str()));
                }
            } break;
            case HandlerArgumentType::Slide: {
                std::list<Q3DStudio::CString> theSlideNames;
                GetSlideNamesOfAction(inAction, theSlideNames);
                if (theSlideNames.size() > 0)
                    theValue = TDataStrPtr(new CDataStr(*theSlideNames.begin()));
            } break;
            }
            theActionCore->SetHandlerArgumentValue(theArgument, theValue.toOldSkool());
        }

        // now find out all those Dependent argument and set it to the type of the
        // property
        // I am assuming there is 1 and only 1 for Dependent and
        // Property which is true for all our use cases
        const SActionInfo &theActionInfo = theActionCore->GetActionInfo(inAction);
        DataModelDataType::Value theDataType = DataModelDataType::None;
        Qt3DSDMHandlerArgHandle theDependentArg = 0;
        Qt3DSDMInstanceHandle theDependentInstance;
        Qt3DSDMPropertyHandle theDependentProperty;
        for (THandlerArgHandleList::const_iterator theIterator =
                 theActionInfo.m_HandlerArgs.begin();
             theIterator != theActionInfo.m_HandlerArgs.end(); ++theIterator) {
            const SHandlerArgumentInfo &theArgument =
                theActionCore->GetHandlerArgumentInfo(*theIterator);
            if (theArgument.m_ArgType == HandlerArgumentType::Property) {
                SActionInfo theActionInfo = theActionCore->GetActionInfo(inAction);
                theDependentInstance =
                    GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject);
                TCharStr thePropertyName =
                    qt3dsdm::get<qt3dsdm::TDataStrPtr>(theArgument.m_Value)->GetData();
                theDependentProperty =
                    GetAggregateInstancePropertyByName(theDependentInstance, thePropertyName);
                theDataType = thePropertySystem->GetDataType(theDependentProperty);
            } else if (theArgument.m_ArgType == HandlerArgumentType::Dependent) {
                theDependentArg = *theIterator;
            }
        }

        if (theDataType != DataModelDataType::None && theDependentArg != 0) {
            // found
            SetArgTypeDependentDefaultValue(theDependentArg, theDataType, theDependentInstance,
                                            theDependentProperty);
        }
    }
}
// Resolve the path
void CClientDataModelBridge::ResetHandlerArguments(qt3dsdm::Qt3DSDMActionHandle inAction,
                                                   const std::wstring &inHandler)
{
    IActionCore &theActionCore = *m_Doc->GetStudioSystem()->GetActionCore();
    const SActionInfo &theInfo(theActionCore.GetActionInfo(inAction));
    ResetHandlerArguments(inAction,
                          ResolveHandler(theInfo.m_Owner, theInfo.m_TargetObject, inHandler));
}

qt3dsdm::Qt3DSDMEventHandle
CClientDataModelBridge::ResolveEvent(qt3dsdm::Qt3DSDMInstanceHandle inResolveRoot,
                                     const qt3dsdm::SObjectRefType &inResolution,
                                     const std::wstring &inEventName)
{
    Qt3DSDMInstanceHandle theInstance = GetInstance(inResolveRoot, inResolution);
    if (theInstance.Valid() == false)
        return 0;
    IMetaData &theMetaData(*m_Doc->GetStudioSystem()->GetActionMetaData());
    return theMetaData.FindEvent(theInstance, inEventName.c_str());
}

qt3dsdm::Qt3DSDMHandlerHandle
CClientDataModelBridge::ResolveHandler(qt3dsdm::Qt3DSDMInstanceHandle inResolveRoot,
                                       const qt3dsdm::SObjectRefType &inResolution,
                                       const std::wstring &inHandlerName)
{
    Qt3DSDMInstanceHandle theInstance = GetInstance(inResolveRoot, inResolution);
    if (theInstance.Valid() == false)
        return 0;
    IMetaData &theMetaData(*m_Doc->GetStudioSystem()->GetActionMetaData());
    return theMetaData.FindHandlerByName(theInstance, inHandlerName.c_str());
}

qt3dsdm::Qt3DSDMEventHandle CClientDataModelBridge::ResolveEvent(const qt3dsdm::SActionInfo &inInfo)
{
    return ResolveEvent(inInfo.m_Owner, inInfo.m_TriggerObject, inInfo.m_Event);
}

qt3dsdm::Qt3DSDMHandlerHandle CClientDataModelBridge::ResolveHandler(const qt3dsdm::SActionInfo &inInfo)
{
    return ResolveHandler(inInfo.m_Owner, inInfo.m_TargetObject, inInfo.m_Handler);
}

void CClientDataModelBridge::SetHandlerArgumentValue(Qt3DSDMHandlerArgHandle inHandlerArgument,
                                                     const SValue &inValue)
{
    IActionCore *theActionCore = m_Doc->GetStudioSystem()->GetActionCore();
    IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
    theActionCore->SetHandlerArgumentValue(inHandlerArgument, inValue);

    const SHandlerArgumentInfo &theHandlerArgument =
        theActionCore->GetHandlerArgumentInfo(inHandlerArgument);
    if (theHandlerArgument.m_ArgType == HandlerArgumentType::Property) {
        // for property type, we will go find the dependent type and update it to the value type of
        // this property
        const SActionInfo &theActionInfo =
            theActionCore->GetActionInfo(theHandlerArgument.m_Action);
        Qt3DSDMInstanceHandle theInstance =
            GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject);
        TCharStr thePropertyName =
            qt3dsdm::get<qt3dsdm::TDataStrPtr>(theHandlerArgument.m_Value)->GetData();
        Qt3DSDMPropertyHandle thePropertyHandle =
            GetAggregateInstancePropertyByName(theInstance, thePropertyName);
        DataModelDataType::Value theDataType = thePropertySystem->GetDataType(thePropertyHandle);

        // now find all the dependent argument, assuming only 1 exist and set it to the data type
        for (THandlerArgHandleList::const_iterator theIterator =
                 theActionInfo.m_HandlerArgs.begin();
             theIterator != theActionInfo.m_HandlerArgs.end(); ++theIterator) {
            const SHandlerArgumentInfo &theArgument =
                theActionCore->GetHandlerArgumentInfo(*theIterator);
            if (theArgument.m_ArgType == HandlerArgumentType::Dependent) {
                SetArgTypeDependentDefaultValue(*theIterator, theDataType, theInstance,
                                                thePropertyHandle);
                break;
            }
        }
    }
}

void CClientDataModelBridge::GetActionDependentProperty(Qt3DSDMActionHandle inAction,
                                                        Qt3DSDMInstanceHandle &outInstance,
                                                        Qt3DSDMPropertyHandle &outProperty)
{
    IActionCore *theActionCore = m_Doc->GetStudioSystem()->GetActionCore();
    const SActionInfo &theActionInfo = theActionCore->GetActionInfo(inAction);
    outInstance = GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject);
    for (THandlerArgHandleList::const_iterator theIterator = theActionInfo.m_HandlerArgs.begin();
         theIterator != theActionInfo.m_HandlerArgs.end(); ++theIterator) {
        const SHandlerArgumentInfo &theArgument =
            theActionCore->GetHandlerArgumentInfo(*theIterator);
        if (theArgument.m_ArgType == HandlerArgumentType::Property) {
            TCharStr thePropertyName =
                qt3dsdm::get<qt3dsdm::TDataStrPtr>(theArgument.m_Value)->GetData();
            outProperty = GetAggregateInstancePropertyByName(outInstance, thePropertyName);
            break;
        }
    }
}

void CClientDataModelBridge::GetSlideNamesOfAction(qt3dsdm::Qt3DSDMActionHandle inAction,
                                                   std::list<Q3DStudio::CString> &outSlideNames)
{
    SActionInfo theActionInfo = m_Doc->GetStudioSystem()->GetActionCore()->GetActionInfo(inAction);
    Qt3DSDMInstanceHandle theTargetInstance =
        GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject);
    Q3DStudio::CId theTargetId = GetGUID(theTargetInstance);

    ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    Qt3DSDMSlideHandle theMasterSlide =
        theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theTargetId));
    size_t theSlideCount = theSlideSystem->GetSlideCount(theMasterSlide);

    for (size_t theSlideIndex = 1; theSlideIndex < theSlideCount;
         ++theSlideIndex) // Index 0 refers to Master Slide, so start from index 1
    {
        Qt3DSDMSlideHandle theSlideHandle =
            theSlideSystem->GetSlideByIndex(theMasterSlide, theSlideIndex);
        Qt3DSDMInstanceHandle theInstanceHandle = theSlideSystem->GetSlideInstance(theSlideHandle);
        outSlideNames.push_back(GetName(theInstanceHandle));
    }
}

void CClientDataModelBridge::SetArgTypeDependentDefaultValue(
    Qt3DSDMHandlerArgHandle inHandlerArgument, DataModelDataType::Value inDataType,
    Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty)
{
    SValue theValue;
    SetDefault(inDataType, theValue);
    // Special case for string, check if there is a stringlist for this property
    if (inDataType == DataModelDataType::String) {
        IPropertySystem *thePropertySystem = m_Doc->GetStudioSystem()->GetPropertySystem();
        TMetaDataData theData =
            thePropertySystem->GetAdditionalMetaDataData(inInstance, inProperty);
        if (theData.getType() == AdditionalMetaDataType::StringList) {
            qt3dsdm::TMetaDataStringList theStringList =
                qt3dsdm::get<qt3dsdm::TMetaDataStringList>(theData);
            if (theStringList.size() > 0)
                theValue = TDataStrPtr(new CDataStr(theStringList[0].wide_str()));
        }
    }
    m_Doc->GetStudioSystem()->GetActionCore()->SetHandlerArgumentValue(inHandlerArgument, theValue);
}

void CClientDataModelBridge::GetEvents(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                       qt3dsdm::TEventHandleList &outEvents)
{
    if (m_Doc->GetStudioSystem()->IsInstance(inInstance))
        m_Doc->GetStudioSystem()->GetActionMetaData()->GetEvents(inInstance, outEvents);
}

qt3dsdm::SEventInfo CClientDataModelBridge::GetEventInfo(qt3dsdm::Qt3DSDMEventHandle inEvent)
{
    if (inEvent.Valid() == false)
        return SEventInfo();
    return m_Doc->GetStudioSystem()->GetActionMetaData()->GetEventInfo(inEvent);
}

void CClientDataModelBridge::GetHandlers(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                         qt3dsdm::THandlerHandleList &outHandles)
{
    if (m_Doc->GetStudioSystem()->IsInstance(inInstance))
        m_Doc->GetStudioSystem()->GetActionMetaData()->GetHandlers(inInstance, outHandles);
}

qt3dsdm::SHandlerInfo CClientDataModelBridge::GetHandlerInfo(qt3dsdm::Qt3DSDMHandlerHandle inHandler)
{
    if (inHandler.Valid() == false)
        return SHandlerInfo();
    return m_Doc->GetStudioSystem()->GetActionMetaData()->GetHandlerInfo(inHandler);
}

Qt3DSDMPropertyHandle
CClientDataModelBridge::GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                                           const TCharStr &inPropertyName)
{
    return m_DataCore->GetAggregateInstancePropertyByName(inInstance, inPropertyName);
}
