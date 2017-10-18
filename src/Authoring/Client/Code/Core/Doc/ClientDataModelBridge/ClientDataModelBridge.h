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
#pragma once
#ifndef CLIENTDATAMODELBRIDGEH
#define CLIENTDATAMODELBRIDGEH

#include "UICDMHandles.h"
#include "UICDMDataTypes.h"
#include "UICId.h"
#include "UICDMActionInfo.h"
#include "StudioObjectTypes.h"
#include "Graph.h"
#include "Pt.h"
#include "UICDMMetaData.h"
#include "UICDMComposerTypeDefinitions.h"

class CDoc;

namespace qt3dsdm {
class IDataCore;
class ISlideGraphCore;
class ISlideCore;
class IAnimationCore;
struct SUICDMPropertyDefinition;
class IPropertySystem;
class IInstancePropertyCore;
class ISignalConnection;
class IMetaData;
}

struct SLong4Hasher
{
    std::size_t operator()(const qt3dsdm::SLong4 &inObject) const
    {
        return std::hash<unsigned long>()(inObject.m_Longs[0])
            ^ std::hash<unsigned long>()(inObject.m_Longs[1])
            ^ std::hash<unsigned long>()(inObject.m_Longs[2])
            ^ std::hash<unsigned long>()(inObject.m_Longs[3]);
    }
};

typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Material>
    SDataModelDefaultMaterial;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Image> SDataModelSceneImage;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Node> SDataModelNode;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Layer> SDataModelLayer;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Model> SDataModelModel;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Light> SDataModelLight;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Camera> SDataModelCamera;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Text> SDataModelText;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Group> SDataModelGroup;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Component> SDataModelComponent;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Behavior> SDataModelBehavior;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Scene> SDataModelScene;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Slide> SDataModelSlide;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Action> SDataModelAction;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Asset> SDataModelSceneAsset;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Effect> SDataModelEffect;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::RenderPlugin>
    SDataModelRenderPlugin;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::MaterialBase>
    SDataModelMaterialBase;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::CustomMaterial>
    SDataModelCustomMaterial;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Lightmaps> SDataModelLightmaps;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Alias> SDataModelAlias;
typedef qt3dsdm::SComposerObjectDefinition<qt3dsdm::ComposerObjectTypes::Path> SDataModelPath;

struct IValueFilter
{
    virtual ~IValueFilter() {}
    virtual bool KeepValue(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                           const qt3dsdm::SValue &inValue) = 0;
};

struct SActionInvalidProperty
{
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    qt3dsdm::IPropertySystem &m_PropertySystem;
    SActionInvalidProperty(qt3dsdm::IPropertySystem &ps, qt3dsdm::Qt3DSDMInstanceHandle inInstance)
        : m_Instance(inInstance)
        , m_PropertySystem(ps)
    {
    }
    bool operator()(qt3dsdm::Qt3DSDMPropertyHandle inProperty);
};

class CClientDataModelBridge
{
    Q_DISABLE_COPY(CClientDataModelBridge)

    qt3dsdm::IDataCore *m_DataCore;
    qt3dsdm::ISlideCore *m_SlideCore;
    qt3dsdm::ISlideGraphCore *m_SlideGraphCore;
    qt3dsdm::IAnimationCore *m_AnimationCore;
    std::shared_ptr<qt3dsdm::IMetaData> m_NewMetaData;
    std::shared_ptr<qt3dsdm::SComposerObjectDefinitions> m_ObjectDefinitions;
    CDoc *m_Doc;

    SDataModelDefaultMaterial &m_DefaultMaterial;
    SDataModelSceneImage &m_SceneImage;
    SDataModelNode &m_Node;
    SDataModelLayer &m_Layer;
    SDataModelModel &m_Model;
    SDataModelLight &m_Light;
    SDataModelCamera &m_Camera;
    SDataModelText &m_Text;
    SDataModelGroup &m_Group;
    SDataModelComponent &m_Component;
    SDataModelBehavior &m_Behavior;
    SDataModelScene &m_Scene;
    SDataModelSlide &m_SlideItem;
    SDataModelAction &m_ActionItem;
    SDataModelSceneAsset &m_SceneAsset;
    SDataModelEffect &m_Effect;
    SDataModelRenderPlugin &m_RenderPlugin;
    SDataModelMaterialBase &m_MaterialBase;
    SDataModelCustomMaterial &m_CustomMaterial;
    SDataModelAlias &m_Alias;
    SDataModelPath &m_Path;
    SDataModelLightmaps &m_Lightmaps;

    // cache to increase performance
    bool m_CacheEnabled;

    typedef std::unordered_map<qt3dsdm::SLong4, qt3dsdm::Qt3DSDMInstanceHandle, SLong4Hasher>
        TGUIDInstanceHash;
    typedef std::unordered_map<int, qt3dsdm::SLong4> TInstanceToGUIDHash;

    TGUIDInstanceHash m_CachedGUIDToInstancesHash;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_InstanceCachePropertyChangedConnection;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_InstanceCacheInstanceDeletedConnection;
    TInstanceToGUIDHash m_CachedInstanceToGUIDHash;

    qt3dsdm::TInstanceHandleList m_CachedGuidInstances;
    qt3dsdm::TInstanceHandleList m_CacheImageInstances;
    qt3dsdm::TInstanceHandleList m_CacheMaterialInstances;
    qt3dsdm::TInstanceHandleList m_CacheModelInstances;

public:
    CClientDataModelBridge(qt3dsdm::IDataCore *inDataCore, qt3dsdm::ISlideCore *inSlideCore,
                           qt3dsdm::ISlideGraphCore *inSlideGraphCore,
                           qt3dsdm::IAnimationCore *inAnimationCore,
                           std::shared_ptr<qt3dsdm::IMetaData> inNewMetaData,
                           std::shared_ptr<qt3dsdm::SComposerObjectDefinitions> inDefinitions,
                           CDoc *inDoc);
    virtual ~CClientDataModelBridge();

    virtual qt3dsdm::Qt3DSDMInstanceHandle CreateAssetInstance(Q3DStudio::CId &inId,
                                                            EStudioObjectType inObjectType);
    virtual qt3dsdm::CUICDMSlideGraphHandle GetOrCreateGraph(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    virtual qt3dsdm::CUICDMSlideHandle GetOrCreateGraphRoot(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetSlideInstance();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetSlideComponentIdProperty();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetNameProperty();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetIdProperty();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetTypeProperty();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetSourcePathProperty();
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetActionInstance();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetActionEyeball();
    virtual qt3dsdm::Qt3DSDMPropertyHandle GetImportId();

    virtual qt3dsdm::SComposerObjectDefinitions &GetObjectDefinitions()
    {
        return *m_ObjectDefinitions;
    }
    virtual const qt3dsdm::SComposerObjectDefinitions &GetObjectDefinitions() const
    {
        return *m_ObjectDefinitions;
    }

    virtual bool IsInternalProperty(const qt3dsdm::TCharStr &inPropertyName) const;

    virtual qt3dsdm::Qt3DSDMInstanceHandle
    GetOwningComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle);
    virtual qt3dsdm::Qt3DSDMInstanceHandle
    GetOwningComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle, int &outSlideIndex);
    virtual qt3dsdm::Qt3DSDMInstanceHandle
    GetOwningComponentInstance(qt3dsdm::CUICDMSlideHandle inSlideHandle, int &outSlideIndex);
    virtual qt3dsdm::Qt3DSDMInstanceHandle
    GetOwningComponentInstance(qt3dsdm::CUICDMSlideHandle inSlideHandle);
    virtual qt3dsdm::SLong4 GetComponentGuid(qt3dsdm::CUICDMSlideHandle inSlideHandle);

    virtual bool IsActive(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle, long inCurrentTime);

    virtual qt3dsdm::CUICDMSlideHandle
    GetComponentActiveSlide(qt3dsdm::Qt3DSDMInstanceHandle inComponent);
    virtual qt3dsdm::CUICDMSlideHandle GetComponentSlide(qt3dsdm::Qt3DSDMInstanceHandle inComponent,
                                                       long inIndex);

    const SDataModelDefaultMaterial &GetDefaultMaterial() const { return m_DefaultMaterial; }
    const SDataModelSceneImage &GetSceneImage() const { return m_SceneImage; }
    const SDataModelNode &GetNode() const { return m_Node; }
    const SDataModelLayer &GetLayer() const { return m_Layer; }
    const SDataModelModel &GetModel() const { return m_Model; }
    const SDataModelLight &GetLight() const { return m_Light; }
    const SDataModelCamera &GetCamera() const { return m_Camera; }
    const SDataModelText &GetText() const { return m_Text; }
    const SDataModelGroup &GetGroup() const { return m_Group; }
    const SDataModelComponent &GetComponent() const { return m_Component; }
    const SDataModelBehavior &GetBehavior() const { return m_Behavior; }
    const SDataModelScene &GetScene() const { return m_Scene; }
    const SDataModelSceneAsset &GetSceneAsset() const { return m_SceneAsset; }
    const SDataModelSlide &GetSlide() const { return m_SlideItem; }
    const SDataModelEffect &GetEffect() const { return m_Effect; }
    const SDataModelAlias &GetAlias() const { return m_Alias; }
    const SDataModelPath &GetPath() const { return m_Path; }
    const SDataModelLightmaps &GetLightmaps() const { return m_Lightmaps; }

    // Is this the instance that owns the document's currently active slide?
    bool IsActiveComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

public: // Operations which likely don't belong on this class
    virtual bool GetMaterialFromImageInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                              qt3dsdm::Qt3DSDMInstanceHandle &outMaterialInstance,
                                              qt3dsdm::Qt3DSDMPropertyHandle &outProperty);
    virtual bool GetLayerFromImageProbeInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                qt3dsdm::Qt3DSDMInstanceHandle &outLayerInstance,
                                                qt3dsdm::Qt3DSDMPropertyHandle &outProperty);

public: // Bridging to Actions. These needs to be here as UICDM has no hierarchy info and we need to
        // resolve the path to idenitfy the object referenced
    // We should really reconsider to have the hierachcy store outside of UICDM.
    void GetReferencedActions(qt3dsdm::Qt3DSDMInstanceHandle inReferencedInstance,
                              long inReferencedMode, qt3dsdm::TActionHandleList &outActions);
    void UpdateHandlerArgumentValue(qt3dsdm::HandlerArgumentType::Value inArgType,
                                    qt3dsdm::Qt3DSDMInstanceHandle inTargetObject,
                                    qt3dsdm::SValue inOrigValue, qt3dsdm::SValue inNewValue);
    std::wstring GetDefaultHandler(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                   std::wstring inOldHandler = L"");
    std::wstring GetDefaultEvent(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                 std::wstring inOldEvent = L"");
    void ResetHandlerArguments(qt3dsdm::CUICDMActionHandle inAction,
                               qt3dsdm::CUICDMHandlerHandle inHandler);
    // Resolve the path
    void ResetHandlerArguments(qt3dsdm::CUICDMActionHandle inAction, const std::wstring &inHandler);
    qt3dsdm::CUICDMEventHandle ResolveEvent(qt3dsdm::Qt3DSDMInstanceHandle inResolveRoot,
                                          const qt3dsdm::SObjectRefType &inResolution,
                                          const std::wstring &inEventName);
    qt3dsdm::CUICDMHandlerHandle ResolveHandler(qt3dsdm::Qt3DSDMInstanceHandle inResolveRoot,
                                              const qt3dsdm::SObjectRefType &inResolution,
                                              const std::wstring &inHandlerName);
    qt3dsdm::CUICDMEventHandle ResolveEvent(const qt3dsdm::SActionInfo &inInfo);
    qt3dsdm::CUICDMHandlerHandle ResolveHandler(const qt3dsdm::SActionInfo &inInfo);
    void SetHandlerArgumentValue(qt3dsdm::CUICDMHandlerArgHandle inHandlerArgument,
                                 const qt3dsdm::SValue &inValue);
    void GetActionDependentProperty(qt3dsdm::CUICDMActionHandle inAction,
                                    qt3dsdm::Qt3DSDMInstanceHandle &outInstance,
                                    qt3dsdm::Qt3DSDMPropertyHandle &outProperty);
    void GetSlideNamesOfAction(qt3dsdm::CUICDMActionHandle inAction,
                               std::list<Q3DStudio::CString> &outSlideNames);

protected:
    void SetArgTypeDependentDefaultValue(qt3dsdm::CUICDMHandlerArgHandle inHandlerArgument,
                                         qt3dsdm::DataModelDataType::Value inDataType,
                                         qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                         qt3dsdm::Qt3DSDMPropertyHandle inProperty);

public: // TODO: We should really consider having CStudioCoreSystem or CStudioFullSystem manages the
        // MetaData, so that we can query directly from within
    void GetEvents(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::TEventHandleList &outEvents);
    qt3dsdm::SEventInfo GetEventInfo(qt3dsdm::CUICDMEventHandle inEvent);

    void GetHandlers(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::THandlerHandleList &outHandles);
    qt3dsdm::SHandlerInfo GetHandlerInfo(qt3dsdm::CUICDMHandlerHandle inHandler);

    qt3dsdm::Qt3DSDMPropertyHandle
    GetAggregateInstancePropertyByName(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                       const qt3dsdm::TCharStr &inStr);

private:
    qt3dsdm::Qt3DSDMInstanceHandle
    GetInstanceByGUIDDerivedFrom(qt3dsdm::SLong4 inLong4, qt3dsdm::Qt3DSDMInstanceHandle inParentHandle,
                                 qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    qt3dsdm::Qt3DSDMInstanceHandle
    GetInstanceByGUIDDerivedFrom(qt3dsdm::SLong4 inLong4, const qt3dsdm::TInstanceHandleList &instances,
                                 qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    qt3dsdm::Qt3DSDMInstanceHandle MaybeCacheGetInstanceByGUIDDerivedFrom(
        qt3dsdm::SLong4 inLong4, qt3dsdm::TInstanceHandleList &ioCacheInstances,
        qt3dsdm::Qt3DSDMInstanceHandle inParentHandle, qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    static bool DerivedGuidMatches(qt3dsdm::IDataCore &inDataCore,
                                   qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                   qt3dsdm::Qt3DSDMPropertyHandle inProperty, qt3dsdm::SLong4 inGuid);
    void ClearCache();
    qt3dsdm::Qt3DSDMInstanceHandle GetSceneOrComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    qt3dsdm::CUICDMSlideHandle CreateNonMasterSlide(qt3dsdm::CUICDMSlideHandle inMasterSlide,
                                                  Q3DStudio::CId inGuid,
                                                  const Q3DStudio::CString &inName);

public: // helpers
    void BeginRender(); // enable cache to increase performance
    void EndRender(); // disable cache
    qt3dsdm::Qt3DSDMInstanceHandle GetInstanceByGUID(const Q3DStudio::CId &inId);
    qt3dsdm::Qt3DSDMInstanceHandle GetInstanceByGUID(qt3dsdm::SLong4 inLong4);
    // GUIDS are auto-generated, but is sometimes necessary to hard-set a specific guid
    qt3dsdm::SLong4 GetInstanceGUID(qt3dsdm::Qt3DSDMInstanceHandle);
    void SetInstanceGUID(qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::SLong4 inGuid);
    void ClearInstanceGUIDCache(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    qt3dsdm::Qt3DSDMInstanceHandle GetImageInstanceByGUID(const Q3DStudio::CId &inId);
    qt3dsdm::Qt3DSDMInstanceHandle GetImageInstanceByGUID(qt3dsdm::SLong4 inLong4);
    qt3dsdm::Qt3DSDMInstanceHandle GetMaterialInstanceByGUID(const Q3DStudio::CId &inId);
    qt3dsdm::Qt3DSDMInstanceHandle GetMaterialInstanceByGUID(qt3dsdm::SLong4 inLong4);
    qt3dsdm::Qt3DSDMInstanceHandle GetModelInstanceByGUID(const Q3DStudio::CId &inId);
    qt3dsdm::Qt3DSDMInstanceHandle GetModelInstanceByGUID(qt3dsdm::SLong4 inLong4);
    qt3dsdm::Qt3DSDMInstanceHandle GetComponentInstanceByGUID(const Q3DStudio::CId &inId);
    qt3dsdm::Qt3DSDMInstanceHandle GetComponentInstanceByGUID(qt3dsdm::SLong4 inLong4);
    qt3dsdm::Qt3DSDMInstanceHandle GetInstance(qt3dsdm::Qt3DSDMInstanceHandle inRoot,
                                            const qt3dsdm::SValue &inValue);
    qt3dsdm::Qt3DSDMInstanceHandle GetInstance(qt3dsdm::Qt3DSDMInstanceHandle inRoot,
                                            const qt3dsdm::SObjectRefType &inValue);
    qt3dsdm::TInstanceHandleList GetItemBaseInstances() const;

    std::pair<qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::SLong4>
    CreateImageInstance(qt3dsdm::Qt3DSDMInstanceHandle inSourceInstance,
                        qt3dsdm::Qt3DSDMPropertyHandle inSlot, qt3dsdm::CUICDMSlideHandle inUICDMSlide);

    void SetName(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle, const Q3DStudio::CString &inName);
    Q3DStudio::CString GetName(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle);

    // Convenience functions to get GUID property value from instance handle
private:
    Q3DStudio::CId GetId(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                         qt3dsdm::Qt3DSDMPropertyHandle inProperty) const;

    // Helper for old methods in CAsset
public:
    bool IsInActiveComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool IsInComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                       qt3dsdm::Qt3DSDMInstanceHandle inComponentInstance);
    qt3dsdm::Qt3DSDMInstanceHandle GetParentComponent(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                   bool inIsFirstCall = true);
    Q3DStudio::CString GetUniqueChildName(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                                          qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                          Q3DStudio::CString inDesiredName);
    bool CheckNameUnique(qt3dsdm::Qt3DSDMInstanceHandle inInstance, Q3DStudio::CString inDesiredName);
    Q3DStudio::CString GetSourcePath(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    std::set<Q3DStudio::CString> GetSourcePathList() const;
    std::set<Q3DStudio::CString> GetFontFileList() const;
    std::set<Q3DStudio::CString> GetDynamicObjectTextureList() const;
    bool IsLockedAtAll(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool IsDuplicateable(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool IsMultiSelectable(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool CanDelete(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool IsMaster(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    qt3dsdm::Qt3DSDMInstanceHandle GetResidingLayer(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void GetValueListFromAllSlides(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                   qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                   std::vector<qt3dsdm::SValue> &outValueList,
                                   IValueFilter *inFilter = NULL) const;

protected:
    qt3dsdm::Qt3DSDMInstanceHandle GetChildByName(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                                               Q3DStudio::CString inChildName);
    std::vector<qt3dsdm::SValue>
    GetValueList(qt3dsdm::Qt3DSDMInstanceHandle inParentInstance,
                                         qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                         IValueFilter *inFilter = NULL) const;

public:
    Q3DStudio::CId GetGUID(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;

    qt3dsdm::Qt3DSDMInstanceHandle GetParentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

    // TODO: EStudioObjectType and EASSETTYPE can't co-exist, one must go. Think EStudioObjectType
    // should win since things are better classified
    EStudioObjectType GetObjectType(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    bool IsBehaviorInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsCameraInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsGroupInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsActionInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsComponentInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsLayerInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsLightInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsModelInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsMaterialBaseInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsMaterialInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsReferencedMaterialInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsImageInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsSceneInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsTextInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsRenderPluginInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsCustomMaterialInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsLightmapsInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    bool IsNodeType(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
    // Returns true if this instance would be in the scene graph
    bool IsSceneGraphInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) const;
};

inline qt3dsdm::SLong4 GuidtoSLong4(const Q3DStudio::CId &inId)
{
    Q3DStudio::TGUIDPacked thePacked(inId);
    return qt3dsdm::SLong4(thePacked.Data1, thePacked.Data2, thePacked.Data3, thePacked.Data4);
}

inline Q3DStudio::CId Long4ToGuid(const qt3dsdm::SLong4 &inLong4)
{
    return Q3DStudio::CId(inLong4.m_Longs[0], inLong4.m_Longs[1], inLong4.m_Longs[2],
                          inLong4.m_Longs[3]);
}

inline bool GuidValid(const qt3dsdm::SLong4 &inLong4)
{
    return (inLong4.m_Longs[0] && inLong4.m_Longs[1] && inLong4.m_Longs[2] && inLong4.m_Longs[3]);
}

#endif
