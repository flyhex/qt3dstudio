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

namespace UICDM {
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
    std::size_t operator()(const UICDM::SLong4 &inObject) const
    {
        return std::hash<unsigned long>()(inObject.m_Longs[0])
            ^ std::hash<unsigned long>()(inObject.m_Longs[1])
            ^ std::hash<unsigned long>()(inObject.m_Longs[2])
            ^ std::hash<unsigned long>()(inObject.m_Longs[3]);
    }
};

typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Material>
    SDataModelDefaultMaterial;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Image> SDataModelSceneImage;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Node> SDataModelNode;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Layer> SDataModelLayer;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Model> SDataModelModel;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Light> SDataModelLight;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Camera> SDataModelCamera;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Text> SDataModelText;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Group> SDataModelGroup;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Component> SDataModelComponent;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Behavior> SDataModelBehavior;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Scene> SDataModelScene;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Slide> SDataModelSlide;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Action> SDataModelAction;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Asset> SDataModelSceneAsset;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Effect> SDataModelEffect;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::RenderPlugin>
    SDataModelRenderPlugin;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::MaterialBase>
    SDataModelMaterialBase;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::CustomMaterial>
    SDataModelCustomMaterial;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Lightmaps> SDataModelLightmaps;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Alias> SDataModelAlias;
typedef UICDM::SComposerObjectDefinition<UICDM::ComposerObjectTypes::Path> SDataModelPath;

struct IValueFilter
{
    virtual ~IValueFilter() {}
    virtual bool KeepValue(UICDM::CUICDMInstanceHandle inInstance,
                           UICDM::CUICDMPropertyHandle inProperty,
                           const UICDM::SValue &inValue) = 0;
};

struct SActionInvalidProperty
{
    UICDM::CUICDMInstanceHandle m_Instance;
    UICDM::IPropertySystem &m_PropertySystem;
    SActionInvalidProperty(UICDM::IPropertySystem &ps, UICDM::CUICDMInstanceHandle inInstance)
        : m_Instance(inInstance)
        , m_PropertySystem(ps)
    {
    }
    bool operator()(UICDM::CUICDMPropertyHandle inProperty);
};

class CClientDataModelBridge
{
    Q_DISABLE_COPY(CClientDataModelBridge)

    UICDM::IDataCore *m_DataCore;
    UICDM::ISlideCore *m_SlideCore;
    UICDM::ISlideGraphCore *m_SlideGraphCore;
    UICDM::IAnimationCore *m_AnimationCore;
    std::shared_ptr<UICDM::IMetaData> m_NewMetaData;
    std::shared_ptr<UICDM::SComposerObjectDefinitions> m_ObjectDefinitions;
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

    typedef std::unordered_map<UICDM::SLong4, UICDM::CUICDMInstanceHandle, SLong4Hasher>
        TGUIDInstanceHash;
    typedef std::unordered_map<int, UICDM::SLong4> TInstanceToGUIDHash;

    TGUIDInstanceHash m_CachedGUIDToInstancesHash;
    std::shared_ptr<UICDM::ISignalConnection> m_InstanceCachePropertyChangedConnection;
    std::shared_ptr<UICDM::ISignalConnection> m_InstanceCacheInstanceDeletedConnection;
    TInstanceToGUIDHash m_CachedInstanceToGUIDHash;

    UICDM::TInstanceHandleList m_CachedGuidInstances;
    UICDM::TInstanceHandleList m_CacheImageInstances;
    UICDM::TInstanceHandleList m_CacheMaterialInstances;
    UICDM::TInstanceHandleList m_CacheModelInstances;

public:
    CClientDataModelBridge(UICDM::IDataCore *inDataCore, UICDM::ISlideCore *inSlideCore,
                           UICDM::ISlideGraphCore *inSlideGraphCore,
                           UICDM::IAnimationCore *inAnimationCore,
                           std::shared_ptr<UICDM::IMetaData> inNewMetaData,
                           std::shared_ptr<UICDM::SComposerObjectDefinitions> inDefinitions,
                           CDoc *inDoc);
    virtual ~CClientDataModelBridge();

    virtual UICDM::CUICDMInstanceHandle CreateAssetInstance(Q3DStudio::CId &inId,
                                                            EStudioObjectType inObjectType);
    virtual UICDM::CUICDMSlideGraphHandle GetOrCreateGraph(UICDM::CUICDMInstanceHandle inInstance);
    virtual UICDM::CUICDMSlideHandle GetOrCreateGraphRoot(UICDM::CUICDMInstanceHandle inInstance);
    virtual UICDM::CUICDMInstanceHandle GetSlideInstance();
    virtual UICDM::CUICDMPropertyHandle GetSlideComponentIdProperty();
    virtual UICDM::CUICDMPropertyHandle GetNameProperty();
    virtual UICDM::CUICDMPropertyHandle GetIdProperty();
    virtual UICDM::CUICDMPropertyHandle GetTypeProperty();
    virtual UICDM::CUICDMPropertyHandle GetSourcePathProperty();
    virtual UICDM::CUICDMInstanceHandle GetActionInstance();
    virtual UICDM::CUICDMPropertyHandle GetActionEyeball();
    virtual UICDM::CUICDMPropertyHandle GetImportId();

    virtual UICDM::SComposerObjectDefinitions &GetObjectDefinitions()
    {
        return *m_ObjectDefinitions;
    }
    virtual const UICDM::SComposerObjectDefinitions &GetObjectDefinitions() const
    {
        return *m_ObjectDefinitions;
    }

    virtual bool IsInternalProperty(const UICDM::TCharStr &inPropertyName) const;

    virtual UICDM::CUICDMInstanceHandle
    GetOwningComponentInstance(UICDM::CUICDMInstanceHandle inInstanceHandle);
    virtual UICDM::CUICDMInstanceHandle
    GetOwningComponentInstance(UICDM::CUICDMInstanceHandle inInstanceHandle, int &outSlideIndex);
    virtual UICDM::CUICDMInstanceHandle
    GetOwningComponentInstance(UICDM::CUICDMSlideHandle inSlideHandle, int &outSlideIndex);
    virtual UICDM::CUICDMInstanceHandle
    GetOwningComponentInstance(UICDM::CUICDMSlideHandle inSlideHandle);
    virtual UICDM::SLong4 GetComponentGuid(UICDM::CUICDMSlideHandle inSlideHandle);

    virtual bool IsActive(UICDM::CUICDMInstanceHandle inInstanceHandle, long inCurrentTime);

    virtual UICDM::CUICDMSlideHandle
    GetComponentActiveSlide(UICDM::CUICDMInstanceHandle inComponent);
    virtual UICDM::CUICDMSlideHandle GetComponentSlide(UICDM::CUICDMInstanceHandle inComponent,
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
    bool IsActiveComponent(UICDM::CUICDMInstanceHandle inInstance);

public: // Operations which likely don't belong on this class
    virtual bool GetMaterialFromImageInstance(UICDM::CUICDMInstanceHandle inInstance,
                                              UICDM::CUICDMInstanceHandle &outMaterialInstance,
                                              UICDM::CUICDMPropertyHandle &outProperty);
    virtual bool GetLayerFromImageProbeInstance(UICDM::CUICDMInstanceHandle inInstance,
                                                UICDM::CUICDMInstanceHandle &outLayerInstance,
                                                UICDM::CUICDMPropertyHandle &outProperty);

public: // Bridging to Actions. These needs to be here as UICDM has no hierarchy info and we need to
        // resolve the path to idenitfy the object referenced
    // We should really reconsider to have the hierachcy store outside of UICDM.
    void GetReferencedActions(UICDM::CUICDMInstanceHandle inReferencedInstance,
                              long inReferencedMode, UICDM::TActionHandleList &outActions);
    void UpdateHandlerArgumentValue(UICDM::HandlerArgumentType::Value inArgType,
                                    UICDM::CUICDMInstanceHandle inTargetObject,
                                    UICDM::SValue inOrigValue, UICDM::SValue inNewValue);
    std::wstring GetDefaultHandler(UICDM::CUICDMInstanceHandle inInstance,
                                   std::wstring inOldHandler = L"");
    std::wstring GetDefaultEvent(UICDM::CUICDMInstanceHandle inInstance,
                                 std::wstring inOldEvent = L"");
    void ResetHandlerArguments(UICDM::CUICDMActionHandle inAction,
                               UICDM::CUICDMHandlerHandle inHandler);
    // Resolve the path
    void ResetHandlerArguments(UICDM::CUICDMActionHandle inAction, const std::wstring &inHandler);
    UICDM::CUICDMEventHandle ResolveEvent(UICDM::CUICDMInstanceHandle inResolveRoot,
                                          const UICDM::SObjectRefType &inResolution,
                                          const std::wstring &inEventName);
    UICDM::CUICDMHandlerHandle ResolveHandler(UICDM::CUICDMInstanceHandle inResolveRoot,
                                              const UICDM::SObjectRefType &inResolution,
                                              const std::wstring &inHandlerName);
    UICDM::CUICDMEventHandle ResolveEvent(const UICDM::SActionInfo &inInfo);
    UICDM::CUICDMHandlerHandle ResolveHandler(const UICDM::SActionInfo &inInfo);
    void SetHandlerArgumentValue(UICDM::CUICDMHandlerArgHandle inHandlerArgument,
                                 const UICDM::SValue &inValue);
    void GetActionDependentProperty(UICDM::CUICDMActionHandle inAction,
                                    UICDM::CUICDMInstanceHandle &outInstance,
                                    UICDM::CUICDMPropertyHandle &outProperty);
    void GetSlideNamesOfAction(UICDM::CUICDMActionHandle inAction,
                               std::list<Q3DStudio::CString> &outSlideNames);

protected:
    void SetArgTypeDependentDefaultValue(UICDM::CUICDMHandlerArgHandle inHandlerArgument,
                                         UICDM::DataModelDataType::Value inDataType,
                                         UICDM::CUICDMInstanceHandle inInstance,
                                         UICDM::CUICDMPropertyHandle inProperty);

public: // TODO: We should really consider having CStudioCoreSystem or CStudioFullSystem manages the
        // MetaData, so that we can query directly from within
    void GetEvents(UICDM::CUICDMInstanceHandle inInstance, UICDM::TEventHandleList &outEvents);
    UICDM::SEventInfo GetEventInfo(UICDM::CUICDMEventHandle inEvent);

    void GetHandlers(UICDM::CUICDMInstanceHandle inInstance, UICDM::THandlerHandleList &outHandles);
    UICDM::SHandlerInfo GetHandlerInfo(UICDM::CUICDMHandlerHandle inHandler);

    UICDM::CUICDMPropertyHandle
    GetAggregateInstancePropertyByName(UICDM::CUICDMInstanceHandle inInstance,
                                       const UICDM::TCharStr &inStr);

private:
    UICDM::CUICDMInstanceHandle
    GetInstanceByGUIDDerivedFrom(UICDM::SLong4 inLong4, UICDM::CUICDMInstanceHandle inParentHandle,
                                 UICDM::CUICDMPropertyHandle inProperty);
    UICDM::CUICDMInstanceHandle
    GetInstanceByGUIDDerivedFrom(UICDM::SLong4 inLong4, const UICDM::TInstanceHandleList &instances,
                                 UICDM::CUICDMPropertyHandle inProperty);
    UICDM::CUICDMInstanceHandle MaybeCacheGetInstanceByGUIDDerivedFrom(
        UICDM::SLong4 inLong4, UICDM::TInstanceHandleList &ioCacheInstances,
        UICDM::CUICDMInstanceHandle inParentHandle, UICDM::CUICDMPropertyHandle inProperty);
    static bool DerivedGuidMatches(UICDM::IDataCore &inDataCore,
                                   UICDM::CUICDMInstanceHandle inInstance,
                                   UICDM::CUICDMPropertyHandle inProperty, UICDM::SLong4 inGuid);
    void ClearCache();
    UICDM::CUICDMInstanceHandle GetSceneOrComponentInstance(UICDM::CUICDMInstanceHandle inInstance);
    UICDM::CUICDMSlideHandle CreateNonMasterSlide(UICDM::CUICDMSlideHandle inMasterSlide,
                                                  Q3DStudio::CId inGuid,
                                                  const Q3DStudio::CString &inName);

public: // helpers
    void BeginRender(); // enable cache to increase performance
    void EndRender(); // disable cache
    UICDM::CUICDMInstanceHandle GetInstanceByGUID(const Q3DStudio::CId &inId);
    UICDM::CUICDMInstanceHandle GetInstanceByGUID(UICDM::SLong4 inLong4);
    // GUIDS are auto-generated, but is sometimes necessary to hard-set a specific guid
    UICDM::SLong4 GetInstanceGUID(UICDM::CUICDMInstanceHandle);
    void SetInstanceGUID(UICDM::CUICDMInstanceHandle, UICDM::SLong4 inGuid);
    void ClearInstanceGUIDCache(UICDM::CUICDMInstanceHandle inInstance,
                                UICDM::CUICDMPropertyHandle inProperty);
    UICDM::CUICDMInstanceHandle GetImageInstanceByGUID(const Q3DStudio::CId &inId);
    UICDM::CUICDMInstanceHandle GetImageInstanceByGUID(UICDM::SLong4 inLong4);
    UICDM::CUICDMInstanceHandle GetMaterialInstanceByGUID(const Q3DStudio::CId &inId);
    UICDM::CUICDMInstanceHandle GetMaterialInstanceByGUID(UICDM::SLong4 inLong4);
    UICDM::CUICDMInstanceHandle GetModelInstanceByGUID(const Q3DStudio::CId &inId);
    UICDM::CUICDMInstanceHandle GetModelInstanceByGUID(UICDM::SLong4 inLong4);
    UICDM::CUICDMInstanceHandle GetComponentInstanceByGUID(const Q3DStudio::CId &inId);
    UICDM::CUICDMInstanceHandle GetComponentInstanceByGUID(UICDM::SLong4 inLong4);
    UICDM::CUICDMInstanceHandle GetInstance(UICDM::CUICDMInstanceHandle inRoot,
                                            const UICDM::SValue &inValue);
    UICDM::CUICDMInstanceHandle GetInstance(UICDM::CUICDMInstanceHandle inRoot,
                                            const UICDM::SObjectRefType &inValue);
    UICDM::TInstanceHandleList GetItemBaseInstances() const;

    std::pair<UICDM::CUICDMInstanceHandle, UICDM::SLong4>
    CreateImageInstance(UICDM::CUICDMInstanceHandle inSourceInstance,
                        UICDM::CUICDMPropertyHandle inSlot, UICDM::CUICDMSlideHandle inUICDMSlide);

    void SetName(UICDM::CUICDMInstanceHandle inInstanceHandle, const Q3DStudio::CString &inName);
    Q3DStudio::CString GetName(UICDM::CUICDMInstanceHandle inInstanceHandle);

    // Convenience functions to get GUID property value from instance handle
private:
    Q3DStudio::CId GetId(UICDM::CUICDMInstanceHandle inInstance,
                         UICDM::CUICDMPropertyHandle inProperty) const;

    // Helper for old methods in CAsset
public:
    bool IsInActiveComponent(UICDM::CUICDMInstanceHandle inInstance);
    bool IsInComponent(UICDM::CUICDMInstanceHandle inInstance,
                       UICDM::CUICDMInstanceHandle inComponentInstance);
    UICDM::CUICDMInstanceHandle GetParentComponent(UICDM::CUICDMInstanceHandle inInstance,
                                                   bool inIsFirstCall = true);
    Q3DStudio::CString GetUniqueChildName(UICDM::CUICDMInstanceHandle inParent,
                                          UICDM::CUICDMInstanceHandle inInstance,
                                          Q3DStudio::CString inDesiredName);
    bool CheckNameUnique(UICDM::CUICDMInstanceHandle inInstance, Q3DStudio::CString inDesiredName);
    Q3DStudio::CString GetSourcePath(UICDM::CUICDMInstanceHandle inInstance) const;
    std::set<Q3DStudio::CString> GetSourcePathList() const;
    std::set<Q3DStudio::CString> GetFontFileList() const;
    std::set<Q3DStudio::CString> GetDynamicObjectTextureList() const;
    bool IsLockedAtAll(UICDM::CUICDMInstanceHandle inInstance);
    bool IsDuplicateable(UICDM::CUICDMInstanceHandle inInstance);
    bool IsMultiSelectable(UICDM::CUICDMInstanceHandle inInstance);
    bool CanDelete(UICDM::CUICDMInstanceHandle inInstance);
    bool IsMaster(UICDM::CUICDMInstanceHandle inInstance);
    UICDM::CUICDMInstanceHandle GetResidingLayer(UICDM::CUICDMInstanceHandle inInstance);
    void GetValueListFromAllSlides(UICDM::CUICDMInstanceHandle inInstance,
                                   UICDM::CUICDMPropertyHandle inProperty,
                                   std::vector<UICDM::SValue> &outValueList,
                                   IValueFilter *inFilter = NULL) const;

protected:
    UICDM::CUICDMInstanceHandle GetChildByName(UICDM::CUICDMInstanceHandle inParent,
                                               Q3DStudio::CString inChildName);
    std::vector<UICDM::SValue>
    GetValueList(UICDM::CUICDMInstanceHandle inParentInstance,
                                         UICDM::CUICDMPropertyHandle inProperty,
                                         IValueFilter *inFilter = NULL) const;

public:
    Q3DStudio::CId GetGUID(UICDM::CUICDMInstanceHandle inInstance) const;

    UICDM::CUICDMInstanceHandle GetParentInstance(UICDM::CUICDMInstanceHandle inInstance);

    // TODO: EStudioObjectType and EASSETTYPE can't co-exist, one must go. Think EStudioObjectType
    // should win since things are better classified
    EStudioObjectType GetObjectType(UICDM::CUICDMInstanceHandle inInstance);
    bool IsBehaviorInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsCameraInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsGroupInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsActionInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsComponentInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsLayerInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsLightInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsModelInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsMaterialBaseInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsMaterialInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsReferencedMaterialInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsImageInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsSceneInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsTextInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsEffectInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsRenderPluginInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsCustomMaterialInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsLightmapsInstance(UICDM::CUICDMInstanceHandle inInstance) const;
    bool IsNodeType(UICDM::CUICDMInstanceHandle inInstance) const;
    // Returns true if this instance would be in the scene graph
    bool IsSceneGraphInstance(UICDM::CUICDMInstanceHandle inInstance) const;
};

inline UICDM::SLong4 GuidtoSLong4(const Q3DStudio::CId &inId)
{
    Q3DStudio::TGUIDPacked thePacked(inId);
    return UICDM::SLong4(thePacked.Data1, thePacked.Data2, thePacked.Data3, thePacked.Data4);
}

inline Q3DStudio::CId Long4ToGuid(const UICDM::SLong4 &inLong4)
{
    return Q3DStudio::CId(inLong4.m_Longs[0], inLong4.m_Longs[1], inLong4.m_Longs[2],
                          inLong4.m_Longs[3]);
}

inline bool GuidValid(const UICDM::SLong4 &inLong4)
{
    return (inLong4.m_Longs[0] && inLong4.m_Longs[1] && inLong4.m_Longs[2] && inLong4.m_Longs[3]);
}

#endif
