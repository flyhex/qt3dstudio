/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
#include "StudioRendererImpl.h"
#include "StudioRendererTranslation.h"
#include "Qt3DSRenderEffectSystem.h"
#include "foundation/StrConvertUTF.h"
#include "Qt3DSFileTools.h"
#include "Qt3DSRenderUIPLoader.h"
#include "Qt3DSRenderWidgets.h"
#include "foundation/Qt3DSBounds3.h"
#include "Qt3DSRenderResourceManager.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "Qt3DSRenderCamera.h"
#include "foundation/Qt3DSPlane.h"
#include "Qt3DSRenderRotationHelper.h"
#include "Qt3DSRenderPluginGraphObject.h"
#include "Qt3DSRenderPlugin.h"
#include "StudioCoreSystem.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSRenderPluginPropertyValue.h"
#include "Qt3DSRenderEffectSystem.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderMaterialHelpers.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderReferencedMaterial.h"
#include "Qt3DSRenderPixelGraphicsTypes.h"
#include "Qt3DSRenderPixelGraphicsRenderer.h"
#include "Qt3DSRenderPathManager.h"

#include "PathWidget.h"
#include "Qt3DSRenderLightmaps.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderLight.h"

#pragma warning(disable : 4100) // unreferenced formal parameter

using namespace qt3ds::studio;
QT3DSU32 qt3ds::studio::g_GraphObjectTranslatorTag;
using qt3ds::render::SPGGraphObject;
using qt3ds::render::SPGRect;
using qt3ds::render::SPGVertLine;
using qt3ds::render::SPGHorzLine;
using qt3ds::render::NVRenderRectF;
using qt3ds::render::NVRenderRect;

namespace {
using namespace qt3dsdm;

struct STranslatorDataModelParser
{
    STranslation &m_Context;
    Qt3DSDMInstanceHandle m_InstanceHandle;
    STranslatorDataModelParser(STranslation &inContext, Qt3DSDMInstanceHandle inInstance)
        : m_Context(inContext)
        , m_InstanceHandle(inInstance)
    {
    }
    Qt3DSDMInstanceHandle GetInstanceHandle() { return m_InstanceHandle; }

    template <typename TDataType>
    inline Option<TDataType> GetPropertyValue(qt3dsdm::Qt3DSDMPropertyHandle inProperty)
    {
        Option<SValue> theValue =
            m_Context.m_Reader.GetRawInstancePropertyValue(GetInstanceHandle(), inProperty);
        if (theValue.hasValue())
            return qt3dsdm::get<TDataType>(*theValue);
        return Empty();
    }

    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, QT3DSF32 &outValue)
    {
        Option<float> theValue = GetPropertyValue<float>(inProperty);
        if (theValue.hasValue()) {
            outValue = theValue.getValue();
            return true;
        }
        return false;
    }

    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, QT3DSU32 &outValue)
    {
        auto theValue = GetPropertyValue<qt3ds::QT3DSI32>(inProperty);
        if (theValue.hasValue()) {
            outValue = NVMax(theValue.getValue(), 0);
            return true;
        }
        return false;
    }

    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, QT3DSI32 &outValue)
    {
        auto theValue = GetPropertyValue<qt3ds::QT3DSI32>(inProperty);
        if (theValue.hasValue()) {
            outValue = *theValue;
            return true;
        }
        return false;
    }

    bool ParseRadianProperty(Qt3DSDMPropertyHandle inProperty, QT3DSF32 &outValue)
    {
        if (ParseProperty(inProperty, outValue)) {
            TORAD(outValue);
            return true;
        }
        return false;
    }
    bool ParseRadianProperty(Qt3DSDMPropertyHandle inProperty, QT3DSVec3 &outValue)
    {
        if (ParseProperty(inProperty, outValue)) {
            TORAD(outValue.x);
            TORAD(outValue.y);
            TORAD(outValue.z);
            return true;
        }
        return false;
    }
    bool ParseOpacityProperty(Qt3DSDMPropertyHandle inProperty, QT3DSF32 &outValue)
    {
        if (ParseProperty(inProperty, outValue)) {
            outValue = (1.0f / 100.0f) * outValue;
            return true;
        }
        return false;
    }
    bool ParseRotationOrder(Qt3DSDMPropertyHandle inProperty, QT3DSU32 &outValue)
    {
        qt3ds::render::CRegisteredString temp;
        if (ParseProperty(inProperty, temp)) {
            outValue = qt3ds::render::MapRotationOrder(temp);
            return true;
        }
        return false;
    }
    bool ParseOrientation(Qt3DSDMPropertyHandle inProperty, qt3ds::render::NodeFlags &outValue)
    {
        qt3ds::render::CRegisteredString temp;
        if (ParseProperty(inProperty, temp)) {
            bool isLeftHanded = strcmp(temp.c_str(), "Left Handed") == 0;
            outValue.SetLeftHanded(isLeftHanded);
            return true;
        }
        return false;
    }

    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, bool &outValue)
    {
        Option<bool> theValue = GetPropertyValue<bool>(inProperty);
        if (theValue.hasValue()) {
            outValue = theValue.getValue();
            return true;
        }
        return false;
    }
    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, QT3DSVec2 &outValue)
    {
        Option<qt3dsdm::SFloat2> theValue = GetPropertyValue<qt3dsdm::SFloat2>(inProperty);
        if (theValue.hasValue()) {
            outValue = QT3DSVec2(theValue->m_Floats[0], theValue->m_Floats[1]);
            return true;
        }
        return false;
    }
    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, QT3DSVec3 &outValue)
    {
        Option<SFloat3> theValue = GetPropertyValue<SFloat3>(inProperty);
        if (theValue.hasValue()) {
            outValue = QT3DSVec3(theValue->m_Floats[0], theValue->m_Floats[1], theValue->m_Floats[2]);
            return true;
        }
        return false;
    }
    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, qt3ds::render::CRegisteredString &outValue)
    {
        Option<qt3dsdm::TDataStrPtr> theValue = GetPropertyValue<qt3dsdm::TDataStrPtr>(inProperty);
        if (theValue.hasValue() && *theValue) {
            qt3ds::render::IStringTable &theStrTable(m_Context.m_Context.GetStringTable());
            outValue = theStrTable.RegisterStr((*theValue)->GetData());
            return true;
        }
        return false;
    }

    bool ParseAndResolveSourcePath(qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                   qt3ds::render::CRegisteredString &outValue)
    {
        if (ParseProperty(inProperty, outValue)) {
            if (outValue.IsValid() && outValue.c_str()[0] != '#') {
                Q3DStudio::CFilePath theDirectory = m_Context.m_Doc.GetDocumentDirectory();
                Q3DStudio::CFilePath theResolvedPath =
                    Q3DStudio::CFilePath::CombineBaseAndRelative(theDirectory, outValue.c_str());
                outValue =
                    m_Context.m_Context.GetStringTable().RegisterStr(theResolvedPath.toCString());
            }
            return true;
        }
        return false;
    }

    template <typename TEnumType>
    bool ParseEnumProperty(qt3dsdm::Qt3DSDMPropertyHandle inProperty, TEnumType &ioValue)
    {
        qt3ds::render::CRegisteredString temp;
        if (ParseProperty(inProperty, temp)) {
            qt3ds::render::SEnumNameMap *theNameMap(qt3ds::render::SEnumParseMap<TEnumType>::GetMap());
            for (qt3ds::render::SEnumNameMap *theIter = theNameMap;
                 theIter->m_Name && *theIter->m_Name; ++theIter) {
                // hack to match advanced overlay types, whose name start with a '*'
                const char8_t *p = temp;
                if (*p == '*')
                    ++p;
                if (strcmp(p, theIter->m_Name) == 0) {
                    ioValue = (TEnumType)theIter->m_Enum;
                    return true;
                }
            }
        }
        return false;
    }

    bool ParseNodeFlagsProperty(qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                qt3ds::render::NodeFlags &outValue,
                                qt3ds::render::NodeFlagValues::Enum theFlag)
    {
        bool temp = false;
        if (ParseProperty(inProperty, temp)) {
            outValue.ClearOrSet(temp, theFlag);
            return true;
        }
        return false;
    }
    bool ParseNodeFlagsInverseProperty(qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                       qt3ds::render::NodeFlags &outValue,
                                       qt3ds::render::NodeFlagValues::Enum theFlag)
    {
        bool temp = false;
        if (ParseProperty(inProperty, temp)) {
            outValue.ClearOrSet(!temp, theFlag);
            return true;
        }
        return false;
    }
    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, qt3ds::render::SImage *&ioImage)
    {
        Option<SLong4> theData = GetPropertyValue<SLong4>(inProperty);
        if (theData.hasValue()) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(
                m_Context.m_Reader.GetInstanceForGuid(*theData));
            SGraphObjectTranslator *imageTranslator = m_Context.GetOrCreateTranslator(theInstance);
            if (imageTranslator
                && imageTranslator->GetGraphObject().m_Type
                    == qt3ds::render::GraphObjectTypes::Image) {
                SImage *theNewImage = static_cast<SImage *>(&imageTranslator->GetGraphObject());
                ioImage = theNewImage;
            } else
                ioImage = nullptr;
            return true;
        }
        return false;
    }

    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, qt3ds::render::SGraphObject *&ioObjRef)
    {
        Option<SObjectRefType> theData = GetPropertyValue<SObjectRefType>(inProperty);
        if (theData.hasValue()) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(
                m_Context.m_Reader.GetInstanceForObjectRef(m_InstanceHandle, *theData));
            SGraphObjectTranslator *theItemTranslator =
                m_Context.GetOrCreateTranslator(theInstance);
            if (theItemTranslator)
                ioObjRef = &theItemTranslator->GetGraphObject();
        }
        return true;
    }

    bool ParseProperty(Qt3DSDMPropertyHandle inProperty, qt3ds::render::SNode *&ioNodePtr)
    {
        Option<SObjectRefType> theData = GetPropertyValue<SObjectRefType>(inProperty);
        SNode *theNewNodePtr = nullptr;
        if (theData.hasValue()) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(
                m_Context.m_Reader.GetInstanceForObjectRef(m_InstanceHandle, *theData));
            SGraphObjectTranslator *theItemTranslator =
                m_Context.GetOrCreateTranslator(theInstance);
            if (theItemTranslator) {
                SGraphObject &theObject = theItemTranslator->GetGraphObject();
                if (GraphObjectTypes::IsNodeType(theObject.m_Type))
                    theNewNodePtr = &static_cast<SNode &>(theObject);
            }
        }
        ioNodePtr = theNewNodePtr;
        return true;
    }
};

// Define parse tables
#define Scene_ClearColor m_Scene.m_BackgroundColor
#define Scene_UseClearColor m_Scene.m_BgColorEnable
#define Node_Rotation m_Node.m_Rotation
#define Node_Position m_Node.m_Position
#define Node_Scale m_Node.m_Scale
#define Node_Pivot m_Node.m_Pivot
#define Node_LocalOpacity m_Node.m_Opacity
#define Node_RotationOrder m_Node.m_RotationOrder
#define Node_LeftHanded m_Node.m_Orientation
#define Layer_TemporalAAEnabled m_Layer.m_TemporalAA
#define Layer_LayerEnableDepthTest m_Layer.m_DisableDepthTest
#define Layer_LayerEnableDepthPrePass m_Layer.m_DisableDepthPrepass
#define Layer_ClearColor m_Layer.m_BackgroundColor
#define Layer_Background m_Layer.m_Background
#define Layer_BlendType m_Layer.m_BlendType
#define Layer_Size m_Layer.m_Size
#define Layer_Location m_Layer.m_Location
#define Layer_ProgressiveAAMode m_Layer.m_ProgressiveAA
#define Layer_MultisampleAAMode m_Layer.m_MultisampleAA
#define Layer_HorizontalFieldValues m_Layer.m_HorizontalFieldValues
#define Layer_Left m_Layer.m_Left
#define Layer_LeftUnits m_Layer.m_LeftUnits
#define Layer_Width m_Layer.m_Width
#define Layer_WidthUnits m_Layer.m_WidthUnits
#define Layer_Right m_Layer.m_Right
#define Layer_RightUnits m_Layer.m_RightUnits
#define Layer_VerticalFieldValues m_Layer.m_VerticalFieldValues
#define Layer_Top m_Layer.m_Top
#define Layer_TopUnits m_Layer.m_TopUnits
#define Layer_Height m_Layer.m_Height
#define Layer_HeightUnits m_Layer.m_HeightUnits
#define Layer_Bottom m_Layer.m_Bottom
#define Layer_BottomUnits m_Layer.m_BottomUnits
#define Layer_AoStrength m_Layer.m_AoStrength
#define Layer_AoDistance m_Layer.m_AoDistance
#define Layer_AoSoftness m_Layer.m_AoSoftness
#define Layer_AoBias m_Layer.m_AoBias
#define Layer_AoSamplerate m_Layer.m_AoSamplerate
#define Layer_AoDither m_Layer.m_AoDither
#define Layer_ShadowStrength m_Layer.m_ShadowStrength
#define Layer_ShadowDist m_Layer.m_ShadowDist
#define Layer_ShadowSoftness m_Layer.m_ShadowSoftness
#define Layer_ShadowBias m_Layer.m_ShadowBias
#define Layer_LightProbe m_Layer.m_LightProbe
#define Layer_ProbeBright m_Layer.m_ProbeBright
#define Layer_FastIbl m_Layer.m_FastIbl
#define Layer_ProbeHorizon m_Layer.m_ProbeHorizon
#define Layer_ProbeFov m_Layer.m_ProbeFov
#define Layer_LightProbe2 m_Layer.m_LightProbe2
#define Layer_Probe2Fade m_Layer.m_Probe2Fade
#define Layer_Probe2Window m_Layer.m_Probe2Window
#define Layer_Probe2Pos m_Layer.m_Probe2Pos
#define Layer_TexturePath m_Asset.m_SourcePath
#define Camera_ClipNear m_Camera.m_ClipNear
#define Camera_ClipFar m_Camera.m_ClipFar
#define Camera_FOV m_Camera.m_Fov
#define Camera_Orthographic m_Camera.m_Orthographic
#define Camera_ScaleMode m_Camera.m_ScaleMode
#define Camera_ScaleAnchor m_Camera.m_ScaleAnchor
#define Light_LightType m_Light.m_LightType
#define Light_Scope m_Light.m_Scope
#define Light_DiffuseColor m_Light.m_LightColor
#define Light_SpecularColor m_Light.m_SpecularColor
#define Light_AmbientColor m_Light.m_AmbientColor
#define Light_Brightness m_Light.m_Brightness
#define Light_LinearFade m_Light.m_LinearFade
#define Light_ExponentialFade m_Light.m_ExpFade
#define Light_AreaWidth m_Light.m_AreaWidth
#define Light_AreaHeight m_Light.m_AreaHeight
#define Light_CastShadow m_Light.m_CastShadow
#define Light_ShadowBias m_Light.m_ShadowBias
#define Light_ShadowFactor m_Light.m_ShadowFactor
#define Light_ShadowMapRes m_Light.m_ShadowMapRes
#define Light_ShadowMapFar m_Light.m_ShadowMapFar
#define Light_ShadowMapFov m_Light.m_ShadowMapFov
#define Light_ShadowFilter m_Light.m_ShadowFilter
#define Model_MeshPath m_Asset.m_SourcePath
#define Model_TessellationMode m_Model.m_Tessellation
#define Model_EdgeTess m_Model.m_EdgeTess
#define Model_InnerTess m_Model.m_InnerTess
#define Lightmaps_LightmapIndirect m_Lightmaps.m_LightmapIndirect
#define Lightmaps_LightmapRadiosity m_Lightmaps.m_LightmapRadiosity
#define Lightmaps_LightmapShadow m_Lightmaps.m_LightmapShadow
#define MaterialBase_IblProbe m_MaterialBase.m_IblProbe
#define Material_Lighting m_Material.m_ShaderLighting
#define Material_BlendMode m_Material.m_BlendMode
#define Material_DiffuseColor m_Material.m_DiffuseColor
#define Material_DiffuseMaps_0 m_Material.m_DiffuseMap1
#define Material_DiffuseMaps_1 m_Material.m_DiffuseMap2
#define Material_DiffuseMaps_2 m_Material.m_DiffuseMap3
#define Material_EmissivePower m_Material.m_EmissivePower
#define Material_EmissiveColor m_Material.m_EmissiveColor
#define Material_EmissiveMap m_Material.m_EmissiveMap
#define Material_EmissiveMap2 m_Material.m_EmissiveMap2
#define Material_SpecularReflection m_Material.m_SpecularReflection
#define Material_SpecularMap m_Material.m_SpecularMap
#define Material_SpecularModel m_Material.m_SpecularModel
#define Material_SpecularTint m_Material.m_SpecularTint
#define Material_IOR m_Material.m_IOR
#define Material_FresnelPower m_Material.m_FresnelPower
#define Material_SpecularAmount m_Material.m_SpecularAmount
#define Material_SpecularRoughness m_Material.m_SpecularRoughness
#define Material_RoughnessMap m_Material.m_RoughnessMap
#define Material_Opacity m_Material.m_Opacity
#define Material_OpacityMap m_Material.m_OpacityMap
#define Material_BumpMap m_Material.m_BumpMap
#define Material_BumpAmount m_Material.m_BumpAmount
#define Material_NormalMap m_Material.m_NormalMap
#define Material_DisplacementMap m_Material.m_DisplacementMap
#define Material_DisplaceAmount m_Material.m_DisplaceAmount
#define Material_TranslucencyMap m_Material.m_TranslucencyMap
#define Material_TranslucentFalloff m_Material.m_TranslucentFalloff
#define Material_DiffuseLightWrap m_Material.m_DiffuseLightWrap
#define Material_ReferencedMaterial m_ReferencedMaterial.m_ReferencedMaterial
#define Image_ImagePath m_Asset.m_SourcePath
#define Image_OffscreenRendererId m_Image.m_SubPresentation
#define Image_Scale_X m_Image.m_RepeatU
#define Image_Scale_Y m_Image.m_RepeatV
#define Image_Pivot_X m_Image.m_PivotU
#define Image_Pivot_Y m_Image.m_PivotV
#define Image_Rotation m_Image.m_RotationUV
#define Image_Position_X m_Image.m_PositionU
#define Image_Position_Y m_Image.m_PositionV
#define Image_MappingMode m_Image.m_TextureMapping
#define Image_HorizontalTilingMode m_Image.m_TilingU
#define Image_VerticalTilingMode m_Image.m_TilingV
#define Text_Text m_Text.m_TextString
#define Text_Font m_Text.m_Font
#define Text_FontSize m_Text.m_Size
#define Text_HorizontalAlignment m_Text.m_HorzAlign
#define Text_VerticalAlignment m_Text.m_VertAlign
#define Text_Leading m_Text.m_Leading
#define Text_Tracking m_Text.m_Tracking
#define Text_TextColor m_Text.m_TextColor
#define Text_EnableAcceleratedFont m_Text.m_EnableAcceleratedFont
#define Path_Width m_Path.m_Width
#define Path_LinearError m_Path.m_LinearError
#define Path_InnerTessAmount m_Path.m_InnerTessAmount
#define Path_EdgeTessAmount m_Path.m_EdgeTessAmount
#define Path_Opacity m_Path.m_Opacity
#define Path_BeginCapping m_Path.m_BeginCap
#define Path_BeginCapOffset m_Path.m_BeginCapOffset
#define Path_BeginCapOpacity m_Path.m_BeginCapOpacity
#define Path_BeginCapWidth m_Path.m_BeginCapWidth
#define Path_EndCapping m_Path.m_EndCap
#define Path_EndCapOffset m_Path.m_EndCapOffset
#define Path_EndCapOpacity m_Path.m_EndCapOpacity
#define Path_EndCapWidth m_Path.m_EndCapWidth
#define Path_PathType m_Path.m_PathType
#define Path_PaintStyle m_Path.m_PaintStyle
#define Path_PathBuffer m_Asset.m_SourcePath
#define SubPath_Closed m_SubPath.m_Closed

// Fill in implementations for the actual parse tables.
#define HANDLE_QT3DS_RENDER_PROPERTY(type, name, dirty)                                              \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_VEC3_PROPERTY(type, name, dirty)                                         \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_REAL_VEC2_PROPERTY(type, name, dirty)                                    \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_COLOR_PROPERTY(type, name, dirty)                                        \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_RADIAN_PROPERTY(type, name, dirty)                                       \
    theParser.ParseRadianProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_VEC3_RADIAN_PROPERTY(type, name, dirty)                                  \
    theParser.ParseRadianProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_OPACITY_PROPERTY(type, name, dirty)                                      \
    theParser.ParseOpacityProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_ROTATION_ORDER_PROPERTY(type, name, dirty)                                      \
    theParser.ParseRotationOrder(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_NODE_ORIENTATION_PROPERTY(type, name, dirty)                                    \
    theParser.ParseOrientation(inContext.m_ObjectDefinitions.type##_##name, theItem.m_Flags);
#define HANDLE_QT3DS_RENDER_DEPTH_TEST_PROPERTY(type, name, dirty)                                   \
    if (theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name))    \
        theItem.m_##name = !theItem.m_##name;
#define HANDLE_QT3DS_NODE_FLAGS_PROPERTY(type, name, dirty)                                          \
    theParser.ParseNodeFlagsProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_Flags, \
                                     qt3ds::render::NodeFlagValues::name);
#define HANDLE_QT3DS_NODE_FLAGS_INVERSE_PROPERTY(type, name, dirty)                                  \
    theParser.ParseNodeFlagsInverseProperty(inContext.m_ObjectDefinitions.type##_##name,           \
                                            theItem.m_Flags, qt3ds::render::NodeFlagValues::name);
#define HANDLE_QT3DS_RENDER_ENUM_PROPERTY(type, name, dirty)                                         \
    theParser.ParseEnumProperty(inContext.m_ObjectDefinitions.type##_##name, theItem.m_##name);
#define HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(type, name, dirty)                                   \
    theParser.ParseAndResolveSourcePath(inContext.m_ObjectDefinitions.type##_##name,               \
                                        theItem.m_##name);
#define HANDLE_QT3DS_RENDER_ARRAY_PROPERTY(type, name, index, dirty)                                 \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name##_##index,                 \
                            theItem.m_##name[index]);
#define HANDLE_QT3DS_RENDER_VEC2_PROPERTY(type, name, dirty)                                         \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name##_##X,                     \
                            theItem.m_##name.x);                                                   \
    theParser.ParseProperty(inContext.m_ObjectDefinitions.type##_##name##_##Y, theItem.m_##name.y);
#define HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(                                                     \
    type, name, dirty) // noop by intention already handled by HANDLE_QT3DS_RENDER_COLOR_PROPERTY
#define HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(                                                 \
    type, name, dirty) // noop by intention already handled by HANDLE_QT3DS_RENDER_VEC3_PROPERTY

struct SSceneTranslator : public SGraphObjectTranslator
{
    SSceneTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SScene)())
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SScene &theItem = static_cast<SScene &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_SCENE_PROPERTIES
        SLayer *theCurrentLayer = nullptr;
        theItem.m_FirstChild = nullptr;
        for (long idx = 0, end = inContext.m_AssetGraph.GetChildCount(GetInstanceHandle());
             idx < end; ++idx) {
            SGraphObjectTranslator::PushTranslation(inContext);
            qt3dsdm::Qt3DSDMInstanceHandle theLayer =
                inContext.m_AssetGraph.GetChild(GetInstanceHandle(), idx);
            SGraphObjectTranslator *theTranslator = inContext.GetOrCreateTranslator(theLayer);
            if (theTranslator && theTranslator->GetGraphObject().m_Type ==
                qt3ds::render::GraphObjectTypes::Layer) {
                SLayer *theLayerObj = static_cast<SLayer *>(&theTranslator->GetGraphObject());
                theLayerObj->m_NextSibling = nullptr;
                if (theItem.m_FirstChild == nullptr)
                    theItem.m_FirstChild = theLayerObj;
                else
                    theCurrentLayer->m_NextSibling = theLayerObj;
                theCurrentLayer = theLayerObj;
            }
        }
    }
    void ClearChildren() override
    {
        SScene &theItem = static_cast<SScene &>(GetGraphObject());
        SLayer *theLastChild = nullptr;

        for (SLayer *theChild = theItem.m_FirstChild; theChild;
             theChild = static_cast<SLayer *>(theChild->m_NextSibling)) {
            if (theLastChild)
                theLastChild->m_NextSibling = nullptr;
            theChild->m_Parent = nullptr;
            theLastChild = theChild;
        }
        theItem.m_FirstChild = nullptr;
    }
    void AppendChild(SGraphObject &inChild) override
    {
        if (inChild.m_Type != GraphObjectTypes::Layer) {
            QT3DS_ASSERT(false);
            return;
        }
        SScene &theItem = static_cast<SScene &>(GetGraphObject());
        SLayer &theLayer = static_cast<SLayer &>(inChild);
        theItem.AddChild(theLayer);
    }
    void SetActive(bool /*inActive*/) override
    {
        // How could we not be active?
    }
};

struct SNodeTranslator : public SGraphObjectTranslator
{
    SNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SNode)())
    {
        Initialize();
    }
    SNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, SNode &inNode)
        : SGraphObjectTranslator(inInstance, inNode)
    {
        Initialize();
    }
    void Initialize()
    {
        SNode &theNode = static_cast<SNode &>(GetGraphObject());
        // Ensure the global transform is valid because we use this before we render sometimes.
        theNode.m_GlobalTransform = QT3DSMat44::createIdentity();
    }
    static inline bool IsNodeType(qt3ds::render::GraphObjectTypes::Enum inType)
    {
        return qt3ds::render::GraphObjectTypes::IsNodeType(inType);
    }
    void PushTranslation(STranslation &inContext) override
    {
        SGraphObjectTranslator::PushTranslation(inContext);
        SNode &theItem = static_cast<SNode &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_NODE_PROPERTIES
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Node.m_BoneId,
                                theItem.m_SkeletonId);
        bool ignoresParent = false;
        if (theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Node.m_IgnoresParent,
                                    ignoresParent))
            theItem.m_Flags.SetIgnoreParentTransform(ignoresParent);
    }
    void AppendChild(SGraphObject &inChild) override
    {
        if (GraphObjectTypes::IsNodeType(inChild.m_Type) == false) {
            QT3DS_ASSERT(false);
            return;
        }

        SNode &theItem = static_cast<SNode &>(GetGraphObject());
        SNode &theChild = static_cast<SNode &>(inChild);
        theItem.AddChild(theChild);
        theItem.MarkDirty(qt3ds::render::NodeTransformDirtyFlag::TransformIsDirty);
        theChild.MarkDirty(qt3ds::render::NodeTransformDirtyFlag::TransformIsDirty);
    }
    void ClearChildren() override
    {
        SNode &theItem = static_cast<SNode &>(GetGraphObject());
        SNode *theLastChild = nullptr;
        for (SNode *theChild = theItem.m_FirstChild; theChild; theChild = theChild->m_NextSibling) {
            if (theLastChild)
                theLastChild->m_NextSibling = nullptr;
            theLastChild = theChild;
            theChild->m_Parent = nullptr;
        }
        theItem.m_FirstChild = nullptr;
    }

    void SetActive(bool inActive) override
    {
        SNode &theNode = static_cast<SNode &>(GetGraphObject());
        if (inActive != theNode.m_Flags.IsActive()) {
            theNode.m_Flags.SetActive(inActive);
            theNode.MarkDirty(qt3ds::render::NodeTransformDirtyFlag::TransformIsDirty);
        }
    }
};

struct SLayerTranslator : public SNodeTranslator
{
    SLayerTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SNodeTranslator(inInstance, *QT3DS_NEW(inAlloc, SLayer)())
    {
    }
    void PushTranslation(STranslation &inContext) override
    {
        SNodeTranslator::PushTranslation(inContext);
        SLayer &theItem = static_cast<SLayer &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_LAYER_PROPERTIES
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Layer.m_AoSamplerate,
                                theItem.m_AoSamplerate);
    }
    void AppendChild(SGraphObject &inChild) override
    {
        if (GraphObjectTypes::IsNodeType(inChild.m_Type)) {
            SNodeTranslator::AppendChild(inChild);
        } else if (inChild.m_Type == GraphObjectTypes::Effect) {
            SLayer &theItem = static_cast<SLayer &>(GetGraphObject());
            theItem.AddEffect(static_cast<SEffect &>(inChild));
        } else if (inChild.m_Type == GraphObjectTypes::RenderPlugin) {
            SLayer &theItem = static_cast<SLayer &>(GetGraphObject());
            theItem.m_RenderPlugin = &static_cast<qt3ds::render::SRenderPlugin &>(inChild);
        }
    }
    void ClearChildren() override
    {
        SNodeTranslator::ClearChildren();
        SLayer &theItem = static_cast<SLayer &>(GetGraphObject());
        SEffect *theLastChild = nullptr;
        for (SEffect *theChild = theItem.m_FirstEffect; theChild;
             theChild = theChild->m_NextEffect) {
            if (theLastChild)
                theLastChild->m_NextEffect = nullptr;
            theLastChild = theChild;
            theChild->m_Layer = nullptr;
        }
        theItem.m_FirstEffect = nullptr;
        theItem.m_RenderPlugin = nullptr;
        // Don't clear the light probe properties because those are added/removed as part of the
        // normal
        // property scan, they aren't added as generic children.
    }
};
struct SLightTranslator : public SNodeTranslator
{
    SLightTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SNodeTranslator(inInstance, *QT3DS_NEW(inAlloc, SLight)())
    {
    }
    void PushTranslation(STranslation &inContext) override
    {
        SNodeTranslator::PushTranslation(inContext);
        SLight &theItem = static_cast<SLight &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_LIGHT_PROPERTIES
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Light.m_ShadowMapRes,
                                theItem.m_ShadowMapRes);
    }
    void AppendChild(SGraphObject &inChild) override { SNodeTranslator::AppendChild(inChild); }
    void ClearChildren() override { SNodeTranslator::ClearChildren(); }
};
struct SCameraTranslator : public SNodeTranslator
{
    SCameraTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SNodeTranslator(inInstance, *QT3DS_NEW(inAlloc, SCamera)())
    {
    }
    void PushTranslation(STranslation &inContext) override
    {
        SNodeTranslator::PushTranslation(inContext);
        SCamera &theItem = static_cast<SCamera &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_CAMERA_PROPERTIES
    }
};
struct SModelTranslator : public SNodeTranslator
{
    SModelTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SNodeTranslator(inInstance, *QT3DS_NEW(inAlloc, SModel)())
    {
    }
    void PushTranslation(STranslation &inContext) override
    {
        SNodeTranslator::PushTranslation(inContext);
        SModel &theItem = static_cast<SModel &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_MODEL_PROPERTIES
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Model.m_PoseRoot,
                                theItem.m_SkeletonRoot);

        theItem.m_FirstMaterial = nullptr;
        for (long idx = 0, end = inContext.m_AssetGraph.GetChildCount(GetInstanceHandle());
             idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theItemHandle =
                inContext.m_AssetGraph.GetChild(GetInstanceHandle(), idx);
            SGraphObjectTranslator *theTranslator = inContext.GetOrCreateTranslator(theItemHandle);
            if (theTranslator && IsMaterial(theTranslator->GetGraphObject())) {
                SGraphObject *theMaterial = &theTranslator->GetGraphObject();
                SetNextMaterialSibling(*theMaterial, nullptr);
                theItem.AddMaterial(*theMaterial);
            }
        }
    }
    void AppendChild(SGraphObject &inChild) override
    {
        if (GraphObjectTypes::IsNodeType(inChild.m_Type)) {
            SNodeTranslator::AppendChild(inChild);
        } else if (IsMaterial(inChild)) {
            SModel &theItem = static_cast<SModel &>(GetGraphObject());
            theItem.AddMaterial(inChild);
        } else {
            QT3DS_ASSERT(false);
        }
    }
    void ClearChildren() override
    {
        SModel &theItem = static_cast<SModel &>(GetGraphObject());
        SNodeTranslator::ClearChildren();

        SGraphObject *theLastMaterial = nullptr;
        for (SGraphObject *theMaterial = theItem.m_FirstMaterial; theMaterial;
             theMaterial = qt3ds::render::GetNextMaterialSibling(theMaterial)) {
            if (theLastMaterial)
                qt3ds::render::SetNextMaterialSibling(*theLastMaterial, nullptr);
            theLastMaterial = theMaterial;
        }
        theItem.m_FirstMaterial = nullptr;
    }
};

static SFloat2 ToFloat2(const Option<SValue> &inValue)
{
    if (inValue.hasValue())
        return inValue->getData<SFloat2>();
    return SFloat2();
}

static float ToFloat(const Option<SValue> &inValue)
{
    if (inValue.hasValue())
        return inValue->getData<QT3DSF32>();
    return 0.0f;
}

struct SPathSubPathTranslator : public SGraphObjectTranslator
{
    eastl::vector<qt3ds::render::SPathAnchorPoint> m_PathBuffer;
    SPathSubPathTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SPathSubPath)())
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SPathSubPath &theItem = static_cast<SPathSubPath &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_PATH_SUBPATH_PROPERTIES
        m_PathBuffer.clear();
        Q3DStudio::IDocumentReader &theReader(inContext.m_Doc.GetDocumentReader());
        QT3DSU32 anchorCount = 0;
        for (QT3DSI32 idx = 0, end = inContext.m_AssetGraph.GetChildCount(GetInstanceHandle());
             idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theAnchor =
                inContext.m_AssetGraph.GetChild(GetInstanceHandle(), idx);
            if (theReader.GetObjectTypeName(theAnchor) == L"PathAnchorPoint")
                ++anchorCount;
        }
        QT3DSU32 anchorIdx = 0;
        for (QT3DSI32 idx = 0, end = inContext.m_AssetGraph.GetChildCount(GetInstanceHandle());
             idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theAnchor =
                inContext.m_AssetGraph.GetChild(GetInstanceHandle(), idx);
            if (theReader.GetObjectTypeName(theAnchor) == L"PathAnchorPoint") {
                SFloat2 theAnchorPos = ToFloat2(theReader.GetInstancePropertyValue(
                    theAnchor,
                    inContext.m_ObjectDefinitions.m_PathAnchorPoint.m_Position.m_Property));
                float theIncomingAngle = ToFloat(theReader.GetInstancePropertyValue(
                    theAnchor,
                    inContext.m_ObjectDefinitions.m_PathAnchorPoint.m_IncomingAngle.m_Property));
                float theIncomingDistance = ToFloat(theReader.GetInstancePropertyValue(
                    theAnchor,
                    inContext.m_ObjectDefinitions.m_PathAnchorPoint.m_IncomingDistance.m_Property));
                float theOutgoingDistance = ToFloat(theReader.GetInstancePropertyValue(
                    theAnchor,
                    inContext.m_ObjectDefinitions.m_PathAnchorPoint.m_OutgoingDistance.m_Property));
                qt3ds::render::SPathAnchorPoint thePoint(QT3DSVec2(theAnchorPos[0], theAnchorPos[1]),
                                                       theIncomingAngle, theIncomingAngle + 180.0f,
                                                       theIncomingDistance, theOutgoingDistance);
                m_PathBuffer.push_back(thePoint);
                ++anchorIdx;
            }
        }
        inContext.m_Context.GetPathManager().SetPathSubPathData(
            theItem,
            qt3ds::foundation::toConstDataRef(m_PathBuffer.begin(), (QT3DSU32)m_PathBuffer.size()));
    }

    void AppendChild(SGraphObject &) override {}

    void ClearChildren() override {}

    void SetActive(bool /*inActive*/) override {}
};

struct SPathTranslator : public SNodeTranslator
{
    SPathTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SNodeTranslator(inInstance, *QT3DS_NEW(inAlloc, SPath)())
    {
    }
    void AppendChild(SGraphObject &inChild) override
    {
        if (GraphObjectTypes::IsMaterialType(inChild.m_Type)) {
            SPath &theItem = static_cast<SPath &>(GetGraphObject());
            theItem.AddMaterial(&inChild);
            theItem.m_Flags.SetDirty(true);
        } else if (inChild.m_Type == GraphObjectTypes::PathSubPath) {
            SPath &theItem = static_cast<SPath &>(GetGraphObject());
            theItem.AddSubPath(static_cast<SPathSubPath &>(inChild));
            theItem.m_Flags.SetDirty(true);
        } else {
            SNodeTranslator::AppendChild(inChild);
        }
    }

    void ClearChildren() override
    {
        SNodeTranslator::ClearChildren();
        SPath &theItem = static_cast<SPath &>(GetGraphObject());
        theItem.ClearMaterials();
        theItem.ClearSubPaths();
    }

    void PushTranslation(STranslation &inContext) override
    {
        SNodeTranslator::PushTranslation(inContext);
        SPath &theItem = static_cast<SPath &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_PATH_PROPERTIES
    }
};

struct SDefaultMaterialTranslator : public SGraphObjectTranslator
{
    SDefaultMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                               qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SDefaultMaterial)())
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SGraphObjectTranslator::PushTranslation(inContext);
        SDefaultMaterial &theItem = static_cast<SDefaultMaterial &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_MATERIAL_PROPERTIES

        // qt3dsdm::Qt3DSDMInstanceHandle parent = inContext.m_AssetGraph.GetParent(
        // GetInstanceHandle() );
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapIndirect,
                                theItem.m_Lightmaps.m_LightmapIndirect);
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapRadiosity,
                                theItem.m_Lightmaps.m_LightmapRadiosity);
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapShadow,
                                theItem.m_Lightmaps.m_LightmapShadow);
    }

    void AppendChild(SGraphObject &) override {}
    void ClearChildren() override {}

    void SetActive(bool /*inActive*/) override {}
};

struct SImageTranslator : public SGraphObjectTranslator
{
    SImageTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SImage)())
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SGraphObjectTranslator::PushTranslation(inContext);
        SImage &theItem = static_cast<SImage &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_IMAGE_PROPERTIES

        theItem.m_Flags.SetDirty(true);
        theItem.m_Flags.SetTransformDirty(true);
    }
    void AppendChild(SGraphObject &child) override
    {
        SImage &theItem = static_cast<SImage &>(GetGraphObject());
        if (child.m_Type == GraphObjectTypes::RenderPlugin)
            theItem.m_RenderPlugin = &static_cast<qt3ds::render::SRenderPlugin &>(child);
    }
    void ClearChildren() override
    {
        SImage &theItem = static_cast<SImage &>(GetGraphObject());
        theItem.m_RenderPlugin = nullptr;
    }

    void SetActive(bool /*inActive*/) override {}
};

struct STextTranslator : public SNodeTranslator
{
    STextTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SNodeTranslator(inInstance, *QT3DS_NEW(inAlloc, SText)())
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SNodeTranslator::PushTranslation(inContext);
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        SText &theItem = static_cast<SText &>(GetGraphObject());
        ITERATE_QT3DS_RENDER_TEXT_PROPERTIES
        theItem.m_Flags.SetTextDirty(true);
    }
};

inline qt3ds::QT3DSVec2 ToRenderType(const qt3dsdm::SFloat2 &inType)
{
    return qt3ds::QT3DSVec2(inType.m_Floats[0], inType.m_Floats[1]);
}
inline qt3ds::QT3DSVec3 ToRenderType(const qt3dsdm::SFloat3 &inType)
{
    return qt3ds::QT3DSVec3(inType.m_Floats[0], inType.m_Floats[1], inType.m_Floats[2]);
}

struct SDynamicObjectTranslator : public SGraphObjectTranslator
{
    typedef eastl::vector<eastl::pair<QT3DSU32, int>> TIdxToPropertyMap;
    eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType> m_ConvertStr;
    TIdxToPropertyMap m_PropertyMap;

    SDynamicObjectTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &,
                             SDynamicObject &inObject)
        : SGraphObjectTranslator(inInstance, inObject)
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SDynamicObject &theItem = static_cast<SDynamicObject &>(GetGraphObject());
        IDynamicObjectSystem &theSystem = inContext.m_Context.GetDynamicObjectSystem();
        using namespace qt3ds::render::dynamic;
        using qt3ds::foundation::NVConstDataRef;
        NVConstDataRef<SPropertyDefinition> theProperties =
            theSystem.GetProperties(theItem.m_ClassName);
        if (m_PropertyMap.size() == 0) {
            for (QT3DSU32 idx = 0, end = theProperties.size(); idx < end; ++idx) {
                const SPropertyDefinition &theDefinition(theProperties[idx]);
                qt3ds::foundation::ConvertUTF(theDefinition.m_Name.c_str(), 0, m_ConvertStr);
                const wchar_t *thePropName =
                    reinterpret_cast<const wchar_t *>(m_ConvertStr.c_str());
                qt3dsdm::Qt3DSDMPropertyHandle theProperty =
                    inContext.m_Reader.FindProperty(GetInstanceHandle(), thePropName);
                if (theProperty.Valid())
                    m_PropertyMap.push_back(eastl::make_pair(idx, theProperty.GetHandleValue()));
            }
        }
        std::shared_ptr<IDataCore> theDataCore =
            inContext.m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetDataCore();
        for (TIdxToPropertyMap::iterator theIter = m_PropertyMap.begin(), end = m_PropertyMap.end();
             theIter != end; ++theIter) {
            const SPropertyDefinition &theDefinition(theProperties[theIter->first]);
            qt3dsdm::Qt3DSDMPropertyHandle theProperty = theIter->second;
            // Sometimes it is possible to have dirty properties that no longer exist, e.g.
            // when undoing standard material -> custom material change. We just ignore changes
            // to such properties.
            if (theDataCore->IsProperty(theProperty)) {
                Option<qt3dsdm::SValue> theValueOpt =
                    inContext.m_Reader.GetInstancePropertyValue(GetInstanceHandle(), theProperty);
                if (theValueOpt.hasValue()) {
                    qt3dsdm::SValue &theValue(*theValueOpt);
                    switch (qt3dsdm::GetValueType(theValue)) {
                    case qt3dsdm::DataModelDataType::Long:
                        if (theDefinition.m_DataType
                                == qt3ds::render::NVRenderShaderDataTypes::QT3DSI32) {
                            theItem.SetPropertyValue(theDefinition,
                                                     qt3dsdm::get<qt3ds::QT3DSI32>(theValue));
                        } else {
                            QT3DS_ASSERT(false);
                        }
                        break;
                    case qt3dsdm::DataModelDataType::Bool:
                        if (theDefinition.m_DataType
                                == qt3ds::render::NVRenderShaderDataTypes::QT3DSRenderBool) {
                            theItem.SetPropertyValue(theDefinition, qt3dsdm::get<bool>(theValue));
                        } else {
                            QT3DS_ASSERT(false);
                        }
                        break;
                    case qt3dsdm::DataModelDataType::Float:
                        if (theDefinition.m_DataType
                                == qt3ds::render::NVRenderShaderDataTypes::QT3DSF32) {
                            theItem.SetPropertyValue(theDefinition, qt3dsdm::get<float>(theValue));
                        } else {
                            QT3DS_ASSERT(false);
                        }
                        break;
                    case qt3dsdm::DataModelDataType::Float2:
                        if (theDefinition.m_DataType
                                == qt3ds::render::NVRenderShaderDataTypes::QT3DSVec2) {
                            theItem.SetPropertyValue(
                                theDefinition, ToRenderType(qt3dsdm::get<qt3dsdm::SFloat2>(theValue)));
                        } else {
                            QT3DS_ASSERT(false);
                        }
                        break;
                    case qt3dsdm::DataModelDataType::Float3:
                        if (theDefinition.m_DataType
                                == qt3ds::render::NVRenderShaderDataTypes::QT3DSVec3) {
                            theItem.SetPropertyValue(
                                theDefinition, ToRenderType(qt3dsdm::get<qt3dsdm::SFloat3>(theValue)));
                        } else {
                            QT3DS_ASSERT(false);
                        }
                        break;
                    // Could be either an enum or a texture.
                    case qt3dsdm::DataModelDataType::String: {
                        qt3dsdm::TDataStrPtr theData = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue);
                        if (theData) {
                            eastl::string theStr;
                            qt3ds::render::ConvertWideUTF(theData->GetData(), 0, theStr);
                            eastl::string theWorkspace;
                            theItem.SetPropertyValue(
                                theDefinition, theStr.c_str(),
                                inContext.m_Doc.GetDocumentDirectory().GetCharStar(), theWorkspace,
                                inContext.m_Context.GetStringTable());
                        }
                    } break;
                    default:
                        QT3DS_ASSERT(false);
                    }
                }
            }
        }
    }

    void AppendChild(SGraphObject &) override {}
    void ClearChildren() override {}
};

struct SEffectTranslator : public SDynamicObjectTranslator
{
    // TODO - move this map to inContext and have it looked up by name.
    IEffectSystem *m_EffectSystem;

    SEffectTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc,
                      SEffect &inEffect)
        : SDynamicObjectTranslator(inInstance, inAlloc, inEffect)
        , m_EffectSystem(nullptr)
    {
    }
    void PushTranslation(STranslation &inContext) override
    {
        m_EffectSystem = &inContext.m_Context.GetEffectSystem();
        SDynamicObjectTranslator::PushTranslation(inContext);
    }

    void SetActive(bool inActive) override
    {
        SEffect &theItem = static_cast<SEffect &>(GetGraphObject());
        if (m_EffectSystem)
            theItem.SetActive(inActive, *m_EffectSystem);
        else
            theItem.m_Flags.SetActive(inActive);
    }
};
struct SCustomMaterialTranslator : public SDynamicObjectTranslator
{
    ICustomMaterialSystem *m_MaterialSystem;

    SCustomMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                              qt3ds::NVAllocatorCallback &inAlloc, SCustomMaterial &inMaterial)
        : SDynamicObjectTranslator(inInstance, inAlloc, inMaterial)
        , m_MaterialSystem(nullptr)
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SDynamicObjectTranslator::PushTranslation(inContext);
        SCustomMaterial &theItem = static_cast<SCustomMaterial &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_CUSTOM_MATERIAL_PROPERTIES

        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapIndirect,
                                theItem.m_Lightmaps.m_LightmapIndirect);
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapRadiosity,
                                theItem.m_Lightmaps.m_LightmapRadiosity);
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapShadow,
                                theItem.m_Lightmaps.m_LightmapShadow);
    }

    void SetActive(bool inActive) override
    {
        if (m_MaterialSystem) {
            SCustomMaterial &theItem = static_cast<SCustomMaterial &>(GetGraphObject());
            if (inActive != theItem.m_Flags.IsActive()) {
                theItem.m_Flags.SetActive(inActive);
                m_MaterialSystem->OnMaterialActivationChange(theItem, inActive);
            }
        }
    }
};
struct SReferencedMaterialTranslator : public SGraphObjectTranslator
{
    SReferencedMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                  qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SReferencedMaterial)())
    {
    }
    void PushTranslation(STranslation &inContext) override
    {
        SGraphObjectTranslator::PushTranslation(inContext);
        SReferencedMaterial &theItem = static_cast<SReferencedMaterial &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        ITERATE_QT3DS_RENDER_REFERENCED_MATERIAL_PROPERTIES

        theItem.m_Dirty.SetDirty();
        if (theItem.m_ReferencedMaterial == &theItem) {
            qCCritical(qt3ds::INVALID_OPERATION,
                "Referenced material is referencing itself.");
        } else if (theItem.m_ReferencedMaterial
                   && theItem.m_ReferencedMaterial->m_Type == GraphObjectTypes::DefaultMaterial) {
            SDefaultMaterial *theDefaultItem =
                static_cast<SDefaultMaterial *>(theItem.m_ReferencedMaterial);
            theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapIndirect,
                                    theDefaultItem->m_Lightmaps.m_LightmapIndirect);
            theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapRadiosity,
                                    theDefaultItem->m_Lightmaps.m_LightmapRadiosity);
            theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapShadow,
                                    theDefaultItem->m_Lightmaps.m_LightmapShadow);
        } else if (theItem.m_ReferencedMaterial
                   && theItem.m_ReferencedMaterial->m_Type == GraphObjectTypes::CustomMaterial) {
            SCustomMaterial *theDefaultItem =
                static_cast<SCustomMaterial *>(theItem.m_ReferencedMaterial);
            theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapIndirect,
                                    theDefaultItem->m_Lightmaps.m_LightmapIndirect);
            theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapRadiosity,
                                    theDefaultItem->m_Lightmaps.m_LightmapRadiosity);
            theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Lightmaps.m_LightmapShadow,
                                    theDefaultItem->m_Lightmaps.m_LightmapShadow);
        }
    }

    void AppendChild(SGraphObject &) override {}

    void ClearChildren() override {}

    void SetActive(bool /*inActive*/) override {}
};
using qt3ds::render::SRenderPlugin;
using qt3ds::render::SRenderPropertyValueUpdate;
using qt3ds::render::IRenderPluginClass;
using qt3ds::render::SRenderPluginPropertyDeclaration;
struct SRenderPluginPropertyUpdateFactory
{
    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates, float value,
                    const SRenderPluginPropertyDeclaration &theDec, IRenderPluginClass &)
    {
        ioUpdates.push_back(SRenderPropertyValueUpdate(theDec.m_Name, value));
    }
    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates, qt3ds::QT3DSI32 value,
                    const SRenderPluginPropertyDeclaration &theDec, IRenderPluginClass &)
    {
        ioUpdates.push_back(SRenderPropertyValueUpdate(theDec.m_Name, value));
    }
    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates, bool value,
                    const SRenderPluginPropertyDeclaration &theDec, IRenderPluginClass &)
    {
        ioUpdates.push_back(SRenderPropertyValueUpdate(theDec.m_Name, value));
    }
    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates, qt3dsdm::TDataStrPtr value,
                    const SRenderPluginPropertyDeclaration &theDec, IRenderPluginClass &,
                    qt3ds::foundation::IStringTable &strTable)
    {
        if (value) {
            ioUpdates.push_back(
                SRenderPropertyValueUpdate(theDec.m_Name, strTable.RegisterStr(value->GetData())));
        }
    }
    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates, qt3dsdm::SStringRef value,
                    const SRenderPluginPropertyDeclaration &theDec, IRenderPluginClass &,
                    qt3ds::foundation::IStringTable &strTable)
    {
        ioUpdates.push_back(
            SRenderPropertyValueUpdate(theDec.m_Name, strTable.RegisterStr(value.m_Id)));
    }
    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates,
                    const qt3dsdm::SFloat2 &value, const SRenderPluginPropertyDeclaration &theDec,
                    IRenderPluginClass &inClass)
    {
        ioUpdates.push_back(SRenderPropertyValueUpdate(
            inClass.GetPropertyValueInfo(theDec.m_StartOffset).first, value.m_Floats[0]));
        ioUpdates.push_back(SRenderPropertyValueUpdate(
            inClass.GetPropertyValueInfo(theDec.m_StartOffset + 1).first, value.m_Floats[1]));
    }

    static void Add(eastl::vector<SRenderPropertyValueUpdate> &ioUpdates,
                    const qt3dsdm::SFloat3 &value, const SRenderPluginPropertyDeclaration &theDec,
                    IRenderPluginClass &inClass)
    {
        ioUpdates.push_back(SRenderPropertyValueUpdate(
            inClass.GetPropertyValueInfo(theDec.m_StartOffset).first, value.m_Floats[0]));
        ioUpdates.push_back(SRenderPropertyValueUpdate(
            inClass.GetPropertyValueInfo(theDec.m_StartOffset + 1).first, value.m_Floats[1]));
        ioUpdates.push_back(SRenderPropertyValueUpdate(
            inClass.GetPropertyValueInfo(theDec.m_StartOffset + 2).first, value.m_Floats[2]));
    }
};
struct SRenderPluginTranslator : public SGraphObjectTranslator
{
    eastl::vector<SRenderPropertyValueUpdate> m_PropertyUpdates;

    SRenderPluginTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                            qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SRenderPlugin)())
    {
    }

    void PushTranslation(STranslation &inContext) override
    {
        SRenderPlugin &theItem = static_cast<SRenderPlugin &>(GetGraphObject());
        // First, get the instance via resolving the source path.
        Qt3DSDMPropertyHandle sourcepath =
            inContext.m_Reader.FindProperty(GetInstanceHandle(), L"sourcepath");
        Option<SValue> theSourcePath =
            inContext.m_Reader.GetInstancePropertyValue(GetInstanceHandle(), sourcepath);
        qt3dsdm::TDataStrPtr theData = theSourcePath->getData<qt3dsdm::TDataStrPtr>();
        if (!theData)
            return;

        Q3DStudio::CFilePath theFullPath = inContext.m_Doc.GetResolvedPathToDoc(theData->GetData());
        qt3ds::foundation::IStringTable &theStrTable = inContext.m_Context.GetStringTable();
        theItem.m_PluginPath = theStrTable.RegisterStr(theFullPath.toCString());
        qt3ds::render::IRenderPluginInstance *theInstance =
            inContext.m_Context.GetRenderPluginManager().GetOrCreateRenderPluginInstance(
                theItem.m_PluginPath, &theItem);

        // Couldn't load the instance, so we can't render the instance.
        if (theInstance == nullptr)
            return;
        // Grab the instance's parent and get the properties that are specific to just that
        // instance.
        TInstanceHandleList derivationParents;
        std::shared_ptr<IDataCore> theDataCore =
            inContext.m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetDataCore();
        theDataCore->GetInstanceParents(GetInstanceHandle(), derivationParents);
        if (derivationParents.size() == 0)
            return;
        TPropertyHandleList theSpecificProperties;
        theDataCore->GetInstanceProperties(derivationParents[0], theSpecificProperties);
        eastl::string propStem;
        eastl::string propname;
        m_PropertyUpdates.clear();
        qt3ds::render::IRenderPluginClass &theClass = theInstance->GetPluginClass();
        using qt3ds::render::SRenderPluginPropertyDeclaration;
        if (theClass.GetRegisteredProperties().size() == 0) {
            for (size_t idx = 0, end = theSpecificProperties.size(); idx < end; ++idx) {
                Qt3DSDMPropertyDefinition theProperty =
                    theDataCore->GetProperty(theSpecificProperties[idx]);
                qt3dsdm::AdditionalMetaDataType::Value theMetaType =
                    inContext.m_StudioSystem.GetActionMetaData()->GetAdditionalMetaDataType(
                        GetInstanceHandle(), theSpecificProperties[idx]);
                CRegisteredString thePropName(theStrTable.RegisterStr(theProperty.m_Name.c_str()));
                switch (theProperty.m_Type) {
                case DataModelDataType::Float:
                    theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                        thePropName, qt3ds::render::SRenderPluginPropertyTypes::Float));
                    break;
                case DataModelDataType::Float2:
                    theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                        thePropName, qt3ds::render::SRenderPluginPropertyTypes::Vector2));
                    break;
                case DataModelDataType::Float3:
                    if (theMetaType != AdditionalMetaDataType::Color) {
                        theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                            thePropName, qt3ds::render::SRenderPluginPropertyTypes::Vector3));
                    } else {
                        theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                            thePropName, qt3ds::render::SRenderPluginPropertyTypes::Color));
                    }
                    break;
                case DataModelDataType::Long:
                    theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                        thePropName, qt3ds::render::SRenderPluginPropertyTypes::Long));
                    break;
                case DataModelDataType::String:
                case DataModelDataType::StringRef:
                    theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                        thePropName, qt3ds::render::SRenderPluginPropertyTypes::String));
                    break;
                case DataModelDataType::Bool:
                    theClass.RegisterProperty(SRenderPluginPropertyDeclaration(
                        thePropName, qt3ds::render::SRenderPluginPropertyTypes::Boolean));
                    break;
                default:
                    // Unsupported plugin property.
                    QT3DS_ASSERT(false);
                    break;
                }
            }
        }
        for (size_t idx = 0, end = theSpecificProperties.size(); idx < end; ++idx) {
            Option<SValue> thePropValueOpt = inContext.m_Reader.GetInstancePropertyValue(
                GetInstanceHandle(), theSpecificProperties[idx]);
            if (thePropValueOpt.hasValue()) {
                SValue &thePropValue = thePropValueOpt.getValue();
                Qt3DSDMPropertyDefinition theProperty =
                    theDataCore->GetProperty(theSpecificProperties[idx]);
                SRenderPluginPropertyDeclaration theDeclaration(theClass.GetPropertyDeclaration(
                    theStrTable.RegisterStr(theProperty.m_Name.c_str())));

                switch (thePropValue.getType()) {
                case DataModelDataType::None:
                    QT3DS_ASSERT(false);
                    break;
                case DataModelDataType::Float:
                    SRenderPluginPropertyUpdateFactory::Add(
                        m_PropertyUpdates, thePropValue.getData<float>(), theDeclaration, theClass);
                    break;
                case DataModelDataType::Float2:
                    SRenderPluginPropertyUpdateFactory::Add(m_PropertyUpdates,
                                                            thePropValue.getData<SFloat2>(),
                                                            theDeclaration, theClass);
                    break;
                case DataModelDataType::Float3:
                    SRenderPluginPropertyUpdateFactory::Add(m_PropertyUpdates,
                                                            thePropValue.getData<SFloat3>(),
                                                            theDeclaration, theClass);
                    break;
                case DataModelDataType::Long:
                    SRenderPluginPropertyUpdateFactory::Add(
                        m_PropertyUpdates, thePropValue.getData<qt3ds::QT3DSI32>(), theDeclaration, theClass);
                    break;
                case DataModelDataType::String:
                    SRenderPluginPropertyUpdateFactory::Add(m_PropertyUpdates,
                                                            thePropValue.getData<TDataStrPtr>(),
                                                            theDeclaration, theClass, theStrTable);
                    break;
                case DataModelDataType::Bool:
                    SRenderPluginPropertyUpdateFactory::Add(
                        m_PropertyUpdates, thePropValue.getData<bool>(), theDeclaration, theClass);
                    break;
                case DataModelDataType::StringRef:
                    SRenderPluginPropertyUpdateFactory::Add(m_PropertyUpdates,
                                                            thePropValue.getData<SStringRef>(),
                                                            theDeclaration, theClass, theStrTable);
                    break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }
        }
        theInstance->Update(NVConstDataRef<SRenderPropertyValueUpdate>(m_PropertyUpdates.data(),
                                                                       m_PropertyUpdates.size()));
    }
    void AppendChild(SGraphObject &) override {}
    void ClearChildren() override {}
    void SetActive(bool inActive) override
    {
        SRenderPlugin &theItem = static_cast<SRenderPlugin &>(GetGraphObject());
        theItem.m_Flags.SetActive(inActive);
    }
};

struct SAliasTranslator : public SGraphObjectTranslator
{
    SGraphObjectTranslator *m_ReferenceTree;
    Qt3DSDMInstanceHandle m_ReferencedInstance;
    SAliasTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3ds::NVAllocatorCallback &inAlloc)
        : SGraphObjectTranslator(inInstance, *QT3DS_NEW(inAlloc, SNode)())
        , m_ReferenceTree(nullptr)
    {
    }
    void RecurseAndCreateTranslators(STranslation &inContext,
                                     qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        for (QT3DSI32 idx = 0, end = inContext.m_AssetGraph.GetChildCount(inInstance); idx < end;
             ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theChild = inContext.m_AssetGraph.GetChild(inInstance, idx);
            inContext.GetOrCreateTranslator(theChild, m_InstanceHandle);
            RecurseAndCreateTranslators(inContext, theChild);
        }
    }
    void PushTranslation(STranslation &inContext) override
    {
        STranslatorDataModelParser theParser(inContext, GetInstanceHandle());
        Option<SObjectRefType> theData = theParser.GetPropertyValue<SObjectRefType>(
            inContext.m_ObjectDefinitions.m_Alias.m_ReferencedNode);
        m_ReferencedInstance = Qt3DSDMInstanceHandle();
        m_ReferenceTree = nullptr;
        ((SNode *)m_GraphObject)->m_Flags.SetDirty(true);
        if (theData.hasValue()) {
            m_ReferencedInstance =
                inContext.m_Reader.GetInstanceForObjectRef(GetInstanceHandle(), *theData);
            if (inContext.m_Reader.IsInstance(m_ReferencedInstance)) {
                m_ReferenceTree =
                    inContext.GetOrCreateTranslator(m_ReferencedInstance, m_InstanceHandle);
                if (m_ReferenceTree
                    && !GraphObjectTypes::IsNodeType(m_ReferenceTree->GetGraphObject().m_Type)) {
                    QT3DS_ASSERT(false);
                    m_ReferenceTree = nullptr;
                    m_ReferencedInstance = Qt3DSDMInstanceHandle();
                } else {
                    RecurseAndCreateTranslators(inContext, m_ReferencedInstance);
                }
            }
        }
    }

    void AfterRenderGraphIsBuilt(STranslation &inContext) override
    {
        SNode &theItem = static_cast<SNode &>(GetGraphObject());
        STranslatorDataModelParser theParser(inContext, m_InstanceHandle);
        ITERATE_QT3DS_RENDER_NODE_PROPERTIES
        theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Node.m_BoneId,
                                theItem.m_SkeletonId);
        bool ignoresParent = false;
        if (theParser.ParseProperty(inContext.m_ObjectDefinitions.m_Node.m_IgnoresParent,
                                    ignoresParent))
            theItem.m_Flags.SetIgnoreParentTransform(ignoresParent);
        theItem.m_Flags.SetDirty(true);
    }
    void AppendChild(SGraphObject &inObject) override
    {
        if (m_ReferenceTree)
            m_ReferenceTree->AppendChild(inObject);
    }
    void ClearChildren() override
    {
        if (m_ReferenceTree)
            m_ReferenceTree->ClearChildren();
    }
    void SetActive(bool inActive) override
    {
        SNode &theItem = static_cast<SNode &>(GetGraphObject());
        theItem.m_Flags.SetActive(inActive);
    }
    SGraphObject &GetGraphObject() override
    {
        if (m_ReferenceTree)
            return *m_ReferenceTree->m_GraphObject;
        return *m_GraphObject;
    }
    qt3dsdm::Qt3DSDMInstanceHandle GetSceneGraphInstanceHandle() override
    {
        if (m_ReferencedInstance.Valid())
            return m_ReferencedInstance;
        return m_InstanceHandle;
    }
    Qt3DSDMInstanceHandle GetInstanceHandle() override { return m_InstanceHandle; }

    SGraphObject &GetNonAliasedGraphObject() override { return *m_GraphObject; }
};
}

void SGraphObjectTranslator::PushTranslation(STranslation &inTranslatorContext)
{
    Q3DStudio::CString theId = inTranslatorContext.m_Reader.GetFileId(GetInstanceHandle());
    if (theId.size())
        GetGraphObject().m_Id =
            inTranslatorContext.m_Context.GetStringTable().RegisterStr(theId.c_str());
}

bool STranslation::IncludeNode(const SNode &inNode)
{
    SGraphObjectTranslator *theTranslator = inNode.m_UserData.DynamicCast<SGraphObjectTranslator>();
    if (theTranslator
        && m_Doc.GetDocumentReader().IsCurrentlyActive(theTranslator->GetInstanceHandle()))
        return true;
    return false;
}

void STranslation::ReleaseEffect(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (m_Reader.IsInstance(inInstance) == false)
        return;

    qt3dsdm::ComposerObjectTypes::Enum theType = m_ObjectDefinitions.GetType(inInstance);
    qt3dsdm::Qt3DSDMInstanceHandle theParentClass = m_Reader.GetFirstBaseClass(inInstance);

    if (theType == NULL && theParentClass.Valid())
        theType = m_ObjectDefinitions.GetType(theParentClass);

    if (theType == qt3dsdm::ComposerObjectTypes::Effect) {
        IEffectSystem &theSystem = m_Context.GetEffectSystem();
        if (theParentClass.Valid()) {
            Q3DStudio::CString theInstanceName = m_Reader.GetName(theParentClass);
            CRegisteredString theNameStr =
                m_Context.GetStringTable().RegisterStr(theInstanceName);

            if (theSystem.IsEffectRegistered(theNameStr)) {
                TInstanceToTranslatorMap::iterator theTranslatorList =
                    m_TranslatorMap.find(inInstance);
                if (theTranslatorList != m_TranslatorMap.end())
                   m_TranslatorMap.erase(theTranslatorList);
                theSystem.SetEffectRequiresCompilation(theNameStr, true);
            }
        }
    }
}

SGraphObjectTranslator *STranslation::CreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    SGraphObjectTranslator *theNewTranslator = nullptr;
    qt3dsdm::ComposerObjectTypes::Enum theType = m_ObjectDefinitions.GetType(inInstance);
    qt3dsdm::Qt3DSDMInstanceHandle theParentClass = m_Reader.GetFirstBaseClass(inInstance);
    if (theType == NULL && theParentClass.Valid())
        theType = m_ObjectDefinitions.GetType(theParentClass);

    // For the subset of possible instances, pick out the valid translators.
    switch (theType) {
    case qt3dsdm::ComposerObjectTypes::Group:
    case qt3dsdm::ComposerObjectTypes::Component:
    case qt3dsdm::ComposerObjectTypes::Node:
        theNewTranslator = QT3DS_NEW(m_Allocator, SNodeTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Scene:
        theNewTranslator = QT3DS_NEW(m_Allocator, SSceneTranslator)(inInstance, m_Allocator);
        m_Scene = static_cast<SScene *>(&theNewTranslator->GetGraphObject());
        m_Scene->m_Presentation = &m_Presentation;
        break;
    case qt3dsdm::ComposerObjectTypes::Layer:
        theNewTranslator = QT3DS_NEW(m_Allocator, SLayerTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Light:
        theNewTranslator = QT3DS_NEW(m_Allocator, SLightTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Camera:
        theNewTranslator = QT3DS_NEW(m_Allocator, SCameraTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Model:
        theNewTranslator = QT3DS_NEW(m_Allocator, SModelTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Image:
        theNewTranslator = QT3DS_NEW(m_Allocator, SImageTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Text:
        theNewTranslator = QT3DS_NEW(m_Allocator, STextTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Material:
        theNewTranslator = QT3DS_NEW(m_Allocator, SDefaultMaterialTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::ReferencedMaterial:
        theNewTranslator =
            QT3DS_NEW(m_Allocator, SReferencedMaterialTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Alias:
        theNewTranslator = QT3DS_NEW(m_Allocator, SAliasTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Path:
        theNewTranslator = QT3DS_NEW(m_Allocator, SPathTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::SubPath:
        theNewTranslator = QT3DS_NEW(m_Allocator, SPathSubPathTranslator)(inInstance, m_Allocator);
        break;
    case qt3dsdm::ComposerObjectTypes::Effect: {
        IEffectSystem &theSystem = m_Context.GetEffectSystem();
        if (theParentClass.Valid()) {
            Q3DStudio::CString theInstanceName = m_Reader.GetName(theParentClass);
            CRegisteredString theNameStr =
                m_Context.GetStringTable().RegisterStr(theInstanceName);

            if (theSystem.IsEffectRegistered(theNameStr)
                && theSystem.DoesEffectRequireCompilation(theNameStr)) {
                theSystem.UnregisterEffect(theNameStr);
            }

            if (!theSystem.IsEffectRegistered(theNameStr)) {
                // We assume the effect has already been registered and such.
                qt3dsdm::IMetaData &theMetaData(*m_StudioSystem.GetActionMetaData());
                Q3DStudio::CString theInstancePath = m_Reader.GetSourcePath(theParentClass);
                Option<qt3dsdm::SMetaDataEffect> theMetaEffect =
                    theMetaData.GetEffectBySourcePath(
                        m_Context.GetStringTable().GetNarrowStr(theInstancePath));
                if (theMetaEffect.hasValue()) {
                    qt3ds::render::IUIPLoader::CreateEffectClassFromMetaEffect(
                        theNameStr, m_Context.GetFoundation(), theSystem, theMetaEffect,
                        m_Context.GetStringTable());
                    theSystem.SetEffectRequiresCompilation(theNameStr, true);
                }
            }

            if (theSystem.IsEffectRegistered(theNameStr)) {
                theNewTranslator = QT3DS_NEW(m_Allocator, SEffectTranslator)(
                    inInstance, m_Allocator,
                    *theSystem.CreateEffectInstance(theNameStr, m_Allocator));
            }
        }
    } break;
    case qt3dsdm::ComposerObjectTypes::CustomMaterial: {
        ICustomMaterialSystem &theSystem = m_Context.GetCustomMaterialSystem();
        if (theParentClass.Valid()) {
            Q3DStudio::CString theInstanceName = m_Reader.GetName(theParentClass);
            CRegisteredString theNameStr =
                m_Context.GetStringTable().RegisterStr(theInstanceName);
            if (!theSystem.IsMaterialRegistered(theNameStr)) {
                // We assume the effect has already been registered and such.
                qt3dsdm::IMetaData &theMetaData(*m_StudioSystem.GetActionMetaData());
                Q3DStudio::CString theInstancePath = m_Reader.GetSourcePath(theParentClass);
                Option<qt3dsdm::SMetaDataCustomMaterial> theMaterialData =
                    theMetaData.GetMaterialBySourcePath(
                        m_Context.GetStringTable().GetNarrowStr(theInstancePath));
                if (theMaterialData.hasValue()) {
                    qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial(
                        theNameStr, m_Context.GetFoundation(), theSystem, theMaterialData,
                        m_Context.GetStringTable());
                }
            }
            if (theSystem.IsMaterialRegistered(theNameStr)) {
                theNewTranslator = QT3DS_NEW(m_Allocator, SCustomMaterialTranslator)(
                    inInstance, m_Allocator,
                    *theSystem.CreateCustomMaterial(theNameStr, m_Allocator));
                static_cast<SCustomMaterialTranslator *>(theNewTranslator)->m_MaterialSystem =
                    &theSystem;
            }
        }
    } break;
    case qt3dsdm::ComposerObjectTypes::RenderPlugin: {
        theNewTranslator = QT3DS_NEW(m_Allocator, SRenderPluginTranslator)(inInstance, m_Allocator);
    } break;
    }
    return theNewTranslator;
}

bool CompareTranslator(const STranslation::THandleTranslatorPair &first,
                       const STranslation::THandleTranslatorPair &second)
{
    return first.first == second.first;
}

struct STranslatorPredicate
{
    Qt3DSDMInstanceHandle m_Instance;
    STranslatorPredicate(Qt3DSDMInstanceHandle &ins)
        : m_Instance(ins)
    {
    }
    bool operator()(const STranslation::THandleTranslatorPair &first) const
    {
        return first.first == m_Instance;
    }
};

Option<STranslation::THandleTranslatorPair>
FindTranslator(STranslation::THandleTranslatorPairList &inList,
               Qt3DSDMInstanceHandle inInstance = Qt3DSDMInstanceHandle())
{
    STranslation::THandleTranslatorPairList::iterator iter =
        eastl::find_if(inList.begin(), inList.end(), STranslatorPredicate(inInstance));
    if (iter != inList.end())
        return *iter;
    return Empty();
}

SGraphObjectTranslator *STranslation::GetOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    return GetOrCreateTranslator(inInstance, Qt3DSDMInstanceHandle());
}

SGraphObjectTranslator *
STranslation::GetOrCreateTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                    qt3dsdm::Qt3DSDMInstanceHandle inAliasInstance)
{
    TInstanceToTranslatorMap::iterator theTranslatorList =
        m_TranslatorMap.insert(eastl::make_pair(inInstance, THandleTranslatorPairList())).first;
    THandleTranslatorPairList &theList = theTranslatorList->second;
    Option<STranslation::THandleTranslatorPair> theExistingTranslator =
        FindTranslator(theList, inAliasInstance);

    if (theExistingTranslator.hasValue()) {
        return theExistingTranslator->second;
    }
    if (m_Reader.IsInstance(inInstance) == false)
        return nullptr;

    SGraphObjectTranslator *theNewTranslator = CreateTranslator(inInstance);
    if (theNewTranslator != nullptr) {
        theNewTranslator->m_AliasInstanceHandle = inAliasInstance;
        m_DirtySet.insert(*theNewTranslator);
        theList.push_back(THandleTranslatorPair(inAliasInstance, theNewTranslator));
    }

    return theNewTranslator;
}

STranslation::THandleTranslatorPairList &
STranslation::GetTranslatorsForInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    return m_TranslatorMap.insert(eastl::make_pair(inInstance, THandleTranslatorPairList()))
        .first->second;
}

qt3dsdm::Qt3DSDMInstanceHandle STranslation::GetAnchorPoint(QT3DSU32 inAnchorIndex)
{
    SGraphObjectTranslator *thePathTranslator =
        static_cast<SGraphObjectTranslator *>(m_PathWidget->GetNode().m_UserData.m_UserData);
    if (thePathTranslator == nullptr)
        return qt3dsdm::Qt3DSDMInstanceHandle();
    qt3dsdm::Qt3DSDMInstanceHandle thePathHandle = thePathTranslator->GetInstanceHandle();
    QT3DSU32 theAnchorIndex = 0;
    for (QT3DSI32 idx = 0, end = m_AssetGraph.GetChildCount(thePathHandle); idx < end; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theChildInstance = m_AssetGraph.GetChild(thePathHandle, idx);
        if (m_Doc.GetDocumentReader().GetObjectTypeName(theChildInstance) == L"SubPath") {
            QT3DSI32 numAnchors = m_AssetGraph.GetChildCount(theChildInstance);
            QT3DSU32 endIndex = theAnchorIndex + (QT3DSU32)numAnchors;
            if (endIndex > inAnchorIndex) {
                return m_AssetGraph.GetChild(theChildInstance, inAnchorIndex - theAnchorIndex);
            } else
                theAnchorIndex = endIndex;
        }
    }
    return qt3dsdm::Qt3DSDMInstanceHandle();
}

qt3dsdm::Qt3DSDMInstanceHandle STranslation::GetAnchorPoint(SPathPick &inPick)
{
    return GetAnchorPoint(inPick.m_AnchorIndex);
}

namespace qt3ds {
namespace studio {
    struct SEditCameraLayerTranslator : public SLayerTranslator
    {
        SEditCameraLayerTranslator(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                   qt3ds::NVAllocatorCallback &inAlloc)
            : SLayerTranslator(inInstance, inAlloc)
        {
        }
        void PushTranslation(STranslation &) override
        {
            SLayer &theItem = static_cast<SLayer &>(GetGraphObject());
            theItem.m_Flags.SetActive(true);
        }
        void AppendChild(SGraphObject &inChild) override
        {
            if (GraphObjectTypes::IsNodeType(inChild.m_Type)) {
                SNodeTranslator::AppendChild(inChild);
            }
        }
    };
}
}

STranslation::STranslation(IStudioRenderer &inRenderer, IQt3DSRenderContext &inContext)
    : m_Renderer(inRenderer)
    , m_Context(inContext)
    , m_Doc(*g_StudioApp.GetCore()->GetDoc())
    , m_Reader(m_Doc.GetDocumentReader())
    , m_ObjectDefinitions(
          m_Doc.GetStudioSystem()->GetClientDataModelBridge()->GetObjectDefinitions())
    , m_StudioSystem(*m_Doc.GetStudioSystem())
    , m_FullSystem(*m_Doc.GetStudioSystem()->GetFullSystem())
    , m_AssetGraph(*m_Doc.GetAssetGraph())
    , m_Allocator(inContext.GetRenderContext().GetFoundation())
    , m_TranslatorMap(inContext.GetAllocator(), "STranslation::m_TranslatorMap")
    , m_DirtySet(inContext.GetAllocator(), "STranslation::m_DirtySet")
    , m_Scene(nullptr)
    , m_SignalConnections(inContext.GetAllocator(), "STranslation::m_SignalConnections")
    , m_ComponentSecondsDepth(0)
    , m_KeyRepeat(0)
    , m_EditCameraEnabled(false)
    , m_EditLightEnabled(false)
    , m_Viewport(0, 0)
    , m_EditCameraLayerTranslator(nullptr)
    , m_PixelBuffer(inContext.GetAllocator(), "STranslation::m_PixelBuffer")
    , m_editModeCamerasAndLights(inContext.GetAllocator(),
                                 "STranslation::m_editModeCamerasAndLights")
    , m_GuideAllocator(inContext.GetAllocator(), "STranslation::m_GuideAllocator")
{
    m_EditCamera.m_Flags.SetActive(true);
    m_EditLight.m_Flags.SetActive(true);
    qt3dsdm::Qt3DSDMInstanceHandle theScene = m_AssetGraph.GetRoot(0);
    m_GraphIterator.ClearResults();
    m_AssetGraph.GetDepthFirst(m_GraphIterator, theScene);
    for (; !m_GraphIterator.IsDone(); ++m_GraphIterator) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(m_GraphIterator.GetCurrent());
        GetOrCreateTranslator(theInstance);
    }
    qt3dsdm::IStudioFullSystemSignalProvider *theProvider = m_FullSystem.GetSignalProvider();
    m_SignalConnections.push_back(
        theProvider->ConnectInstanceCreated(std::bind(&STranslation::DoMarkDirty,
                                                      this, std::placeholders::_1)));
    m_SignalConnections.push_back(theProvider->ConnectInstanceDeleted(
        std::bind(&STranslation::ReleaseTranslation, this, std::placeholders::_1)));
    m_SignalConnections.push_back(
        theProvider->ConnectInstancePropertyValue(std::bind(&STranslation::DoMarkDirty,
                                                            this, std::placeholders::_1)));
    m_SignalConnections.push_back(m_AssetGraph.ConnectChildAdded(
        std::bind(&STranslation::MarkGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_SignalConnections.push_back(m_AssetGraph.ConnectChildMoved(
        std::bind(&STranslation::MarkGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_SignalConnections.push_back(m_AssetGraph.ConnectChildRemoved(
        std::bind(&STranslation::MarkGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_SignalConnections.push_back(theProvider->ConnectBeginComponentSeconds(
        std::bind(&STranslation::MarkBeginComponentSeconds, this, std::placeholders::_1)));
    m_SignalConnections.push_back(theProvider->ConnectComponentSeconds(
        std::bind(&STranslation::MarkComponentSeconds, this, std::placeholders::_1)));

    ::CColor color = CStudioPreferences::GetRulerBackgroundColor(); // Rectangles under tick marks
    m_rectColor = QT3DSVec4(color.GetRed() / 255.f,
                            color.GetGreen() / 255.f,
                            color.GetBlue() / 255.f,
                            1.f);
    color = CStudioPreferences::GetRulerTickColor(); // Tick marks
    m_lineColor = QT3DSVec4(color.GetRed() / 255.f,
                            color.GetGreen() / 255.f,
                            color.GetBlue() / 255.f,
                            1.f);
    color = CStudioPreferences::GetGuideColor();
    m_guideColor = QT3DSVec4(color.GetRed() / 255.f,
                             color.GetGreen() / 255.f,
                             color.GetBlue() / 255.f,
                             1.f);
    color = CStudioPreferences::GetGuideSelectedColor();
    m_selectedGuideColor = QT3DSVec4(color.GetRed() / 255.f,
                                     color.GetGreen() / 255.f,
                                     color.GetBlue() / 255.f,
                                     1.f);
    color = CStudioPreferences::GetGuideFillColor(); // Not sure what this is used for
    m_guideFillColor = QT3DSVec4(color.GetRed() / 255.f,
                                 color.GetGreen() / 255.f,
                                 color.GetBlue() / 255.f,
                                 1.f);
    color = CStudioPreferences::GetGuideFillSelectedColor(); // Not sure what this is used for
    m_selectedGuideFillColor = QT3DSVec4(color.GetRed() / 255.f,
                                         color.GetGreen() / 255.f,
                                         color.GetBlue() / 255.f,
                                         1.f);
}

void STranslation::BuildRenderGraph(SGraphObjectTranslator &inParent,
                                    Qt3DSDMInstanceHandle inAliasHandle)
{
    SGraphObjectTranslator &theParentTranslator(inParent);
    theParentTranslator.ClearChildren();
    if (m_EditCameraEnabled) {
        const auto objectType = theParentTranslator.GetGraphObject().m_Type;
        if (objectType == GraphObjectTypes::Layer) {
            theParentTranslator.AppendChild(m_EditCamera);
            if (m_EditLightEnabled) {
                m_EditLight.m_Parent = &m_EditCamera;
                m_EditCamera.m_FirstChild = &m_EditLight;
            } else {
                m_EditCamera.m_FirstChild = nullptr;
                m_EditLight.m_Parent = nullptr;
            }
        } else if (objectType == GraphObjectTypes::Light) {
            m_editModeCamerasAndLights.push_back(&inParent);
        } else if (objectType == GraphObjectTypes::Camera) {
            m_editModeCamerasAndLights.push_back(&inParent);
        }
    }

    // Alias handles propagate down the scene graph.
    if (inParent.GetInstanceHandle() != inParent.GetSceneGraphInstanceHandle())
        inAliasHandle = inParent.GetInstanceHandle();
    for (long idx = 0, end = m_AssetGraph.GetChildCount(inParent.GetSceneGraphInstanceHandle());
         idx < end; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theChild(
            m_AssetGraph.GetChild(inParent.GetSceneGraphInstanceHandle(), idx));
        SGraphObjectTranslator *theTranslator = GetOrCreateTranslator(theChild, inAliasHandle);
        if (theTranslator == nullptr)
            continue;

        // We we have edit cameras active, we only render the active layer and we remove any cameras
        // in the active layer.  Furthermore if our edit light is active, then we also remove any
        // active lights in the layer.
        if (m_EditCameraEnabled) {
            if (theTranslator->GetGraphObject().m_Type == GraphObjectTypes::Layer) {
                if (theChild == m_Doc.GetActiveLayer()) {
                    if (m_EditCameraLayerTranslator != nullptr
                            && m_EditCameraLayerTranslator->GetInstanceHandle() != theChild) {
                        QT3DS_FREE(m_Allocator, m_EditCameraLayerTranslator);
                        m_EditCameraLayerTranslator = nullptr;
                    }
                    if (!m_EditCameraLayerTranslator) {
                        m_EditCameraLayerTranslator =
                            QT3DS_NEW(m_Allocator, SEditCameraLayerTranslator)(theChild,
                                                                               m_Allocator);
                    }
                    theTranslator = m_EditCameraLayerTranslator;
                    theParentTranslator.AppendChild(theTranslator->GetGraphObject());
                    BuildRenderGraph(*m_EditCameraLayerTranslator);
                }
            } else {
                theParentTranslator.AppendChild(theTranslator->GetGraphObject());
                BuildRenderGraph(theChild, inAliasHandle);

                if (theTranslator->GetGraphObject().m_Type == GraphObjectTypes::Effect)
                    theTranslator->SetActive(false);
                else if (theTranslator->GetGraphObject().m_Type == GraphObjectTypes::Camera)
                    theTranslator->SetActive(false);
                else if (theTranslator->GetGraphObject().m_Type == GraphObjectTypes::Light
                         && m_EditLightEnabled == true)
                    theTranslator->SetActive(false);
                else
                    theTranslator->SetActive(m_Reader.IsCurrentlyActive(theChild));
            }
        } else // Else build the graph and it will be an exact copy of the asset graph.
        {
            theParentTranslator.AppendChild(theTranslator->GetGraphObject());
            if (m_Reader.IsCurrentlyActive(theChild)) {
                BuildRenderGraph(theChild, inAliasHandle);
                theTranslator->SetActive(true);
            } else {
                theTranslator->SetActive(false);
                DeactivateScan(*theTranslator, inAliasHandle);
            }
        }
    }
    if (GraphObjectTypes::Layer == theParentTranslator.GetGraphObject().m_Type)
        m_Context.GetRenderer().ChildrenUpdated(
            static_cast<SLayer &>(theParentTranslator.GetGraphObject()));

    // Allow certain nodes to override their children.
    theParentTranslator.AfterRenderGraphIsBuilt(*this);
}

void STranslation::DeactivateScan(SGraphObjectTranslator &inParent,
                                  Qt3DSDMInstanceHandle inAliasHandle)
{
    // Alias handles propagate down the scene graph.
    if (inParent.GetInstanceHandle() != inParent.GetSceneGraphInstanceHandle())
        inAliasHandle = inParent.GetInstanceHandle();
    for (long idx = 0, end = m_AssetGraph.GetChildCount(inParent.GetSceneGraphInstanceHandle());
         idx < end; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theChild(
            m_AssetGraph.GetChild(inParent.GetSceneGraphInstanceHandle(), idx));
        SGraphObjectTranslator *theTranslator = GetOrCreateTranslator(theChild, inAliasHandle);
        if (theTranslator == nullptr)
            continue;
        theTranslator->SetActive(false);
        DeactivateScan(*theTranslator, inAliasHandle);
    }
}

// We build the render graph every time we render.  This may seem wasteful
void STranslation::BuildRenderGraph(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                                    Qt3DSDMInstanceHandle inAliasHandle)
{
    SGraphObjectTranslator *theParentTranslator = GetOrCreateTranslator(inParent, inAliasHandle);
    if (theParentTranslator == nullptr)
        return;
    if (m_Reader.IsCurrentlyActive(inParent) == false) {
        theParentTranslator->SetActive(false);
        return;
    }
    BuildRenderGraph(*theParentTranslator, inAliasHandle);
}

void STranslation::ReleaseTranslation(Q3DStudio::TIdentifier inInstance)
{
    m_TranslatorMap.erase(inInstance);
}

void STranslation::MarkDirty(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    // Anchor points are not handled individually.
    if (m_Reader.GetObjectTypeName(inInstance) == L"PathAnchorPoint")
        inInstance = m_AssetGraph.GetParent(inInstance);
    GetOrCreateTranslator(inInstance);

    THandleTranslatorPairList &theTranslators = GetTranslatorsForInstance(inInstance);
    for (size_t idx = 0, end = theTranslators.size(); idx < end; ++idx) {
        m_DirtySet.insert(*theTranslators[(eastl::allocator::size_type)idx].second);
    }
    RequestRender();
}

void STranslation::PreRender()
{
    // Run through the entire asset graph and mark active or inactive if we have an
    // associated render representation.
    // If we cache all the components and some of their state then we don't have to do this
    // but for now it is more stable to run through the graph.
    // There is always one root, the scene.
    TIdentifier theRoot = m_AssetGraph.GetRoot(0);
    m_editModeCamerasAndLights.clear();
    ClearDirtySet();
    BuildRenderGraph(theRoot);
    m_Context.SetScaleMode(qt3ds::render::ScaleModes::ExactSize);
    m_Context.SetMatteColor(QT3DSVec4(.13f, .13f, .13f, 1.0f));
    QT3DSVec2 theViewportDims(GetViewportDimensions());
    // Ensure the camera points where it should
    if (m_EditCameraEnabled) {
        m_EditCameraInfo.ApplyToCamera(m_EditCamera, GetViewportDimensions());
        m_EditLight.MarkDirty(qt3ds::render::NodeTransformDirtyFlag::TransformIsDirty);
    }

    if (m_Scene) {
        CStudioProjectSettings *theSettings = m_Doc.GetCore()->GetStudioProjectSettings();
        CPt thePresSize = theSettings->GetPresentationSize();
        // The presentation sizes are used for when we have to render a layer offscreen.  If their
        // width and height
        // isn't set, then they use the presentation dimensions.
        m_Presentation.m_PresentationDimensions =
            QT3DSVec2((QT3DSF32)thePresSize.x, (QT3DSF32)thePresSize.y);
        QT3DSVec2 theViewportDims(GetViewportDimensions());
        m_Context.SetWindowDimensions(
            QSize((QT3DSU32)theViewportDims.x, (QT3DSU32)theViewportDims.y));
        m_Context.SetPresentationDimensions(
            QSize((QT3DSU32)m_Presentation.m_PresentationDimensions.x,
                                           (QT3DSU32)m_Presentation.m_PresentationDimensions.y));

        // set if we draw geometry in wireframe mode
        m_Context.SetWireframeMode(CStudioPreferences::IsWireframeModeOn());

        if (m_EditCameraEnabled) {
            m_Presentation.m_PresentationDimensions = theViewportDims;
            m_Context.SetPresentationDimensions(
                QSize((QT3DSU32)theViewportDims.x, (QT3DSU32)theViewportDims.y));
            ::CColor theEditCameraBackground = CStudioPreferences::GetEditViewBackgroundColor();
            m_Context.SetSceneColor(QT3DSVec4(theEditCameraBackground.GetRed() / 255.0f,
                                              theEditCameraBackground.GetGreen() / 255.0f,
                                              theEditCameraBackground.GetBlue() / 255.0f, 1.0f));
        } else {

            TIdentifier theRoot = m_AssetGraph.GetRoot(0);
            SGraphObjectTranslator *theSceneTranslator = GetOrCreateTranslator(theRoot);
            if (theSceneTranslator) {
                SScene &theScene = static_cast<SScene &>(theSceneTranslator->GetGraphObject());
                if (theScene.m_UseClearColor)
                    m_Context.SetSceneColor(QT3DSVec4(theScene.m_ClearColor, 1.0f));
                else
                    m_Context.SetSceneColor(QT3DSVec4(QT3DSVec3(0.0f), 1.0f));
            }
        }
    }
    if (m_EditCameraEnabled == false && g_StudioApp.IsAuthorZoom()) {
        if (m_Presentation.m_PresentationDimensions.x > theViewportDims.x
            || m_Presentation.m_PresentationDimensions.y > theViewportDims.y) {
            m_Context.SetScaleMode(qt3ds::render::ScaleModes::FitSelected);
        }
    }
}

static void CreatePixelRect(STranslation &inTranslation, QT3DSF32 left, QT3DSF32 right, QT3DSF32 bottom,
                            QT3DSF32 top, QT3DSVec4 color)
{
    SPGRect *theRect = QT3DS_NEW(inTranslation.m_GuideAllocator, SPGRect)();
    theRect->m_Left = left;
    theRect->m_Right = right;
    theRect->m_Top = top;
    theRect->m_Bottom = bottom;
    theRect->m_FillColor = color;
    inTranslation.m_GuideContainer.push_back(theRect);
}

static void CreatePixelVertLine(STranslation &inTranslation, QT3DSF32 inXPos, QT3DSF32 inBottom,
                                QT3DSF32 inTop, QT3DSVec4 color)
{
    SPGVertLine *theLine = QT3DS_NEW(inTranslation.m_GuideAllocator, SPGVertLine)();
    theLine->m_X = inXPos;
    theLine->m_Bottom = inBottom;
    theLine->m_Top = inTop;
    theLine->m_LineColor = color;
    inTranslation.m_GuideContainer.push_back(theLine);
}

static void CreatePixelHorzLine(STranslation &inTranslation, QT3DSF32 inYPos, QT3DSF32 inLeft,
                                QT3DSF32 inRight, QT3DSVec4 color)
{
    SPGHorzLine *theLine = QT3DS_NEW(inTranslation.m_GuideAllocator, SPGHorzLine)();
    theLine->m_Y = inYPos;
    theLine->m_Left = inLeft;
    theLine->m_Right = inRight;
    theLine->m_LineColor = color;
    inTranslation.m_GuideContainer.push_back(theLine);
}

static void CreateTopBottomTickMarks(STranslation &inTranslation, QT3DSF32 posX, QT3DSF32 innerBottom,
                                     QT3DSF32 innerTop, QT3DSF32 outerBottom, QT3DSF32 outerTop,
                                     QT3DSF32 lineHeight, QT3DSVec4 lineColor)
{
    CreatePixelVertLine(inTranslation, posX, innerBottom - lineHeight, innerBottom, lineColor);
    CreatePixelVertLine(inTranslation, posX, innerTop, innerTop + lineHeight, lineColor);
}

static void DrawTickMarksOnHorizontalRects(STranslation &inTranslation, QT3DSF32 innerLeft,
                                           QT3DSF32 innerRight, QT3DSF32 innerBottom, QT3DSF32 innerTop,
                                           QT3DSF32 outerBottom, QT3DSF32 outerTop, QT3DSVec4 lineColor)
{
    QT3DSF32 centerPosX = floor(innerLeft + (innerRight - innerLeft) / 2.0f + .5f);
    CreateTopBottomTickMarks(inTranslation, centerPosX, innerBottom, innerTop, outerBottom,
                             outerTop, 15, lineColor);
    for (QT3DSU32 incrementor = 10;
         (centerPosX + incrementor) < innerRight && (centerPosX - incrementor) > innerLeft;
         incrementor += 10) {
        QT3DSF32 rightEdge = centerPosX + incrementor;
        QT3DSF32 leftEdge = centerPosX - incrementor;
        QT3DSF32 lineHeight = 0;
        if (incrementor % 100 == 0)
            lineHeight = 11;
        else if (incrementor % 20)
            lineHeight = 4;
        else
            lineHeight = 2;

        if (rightEdge < innerRight)
            CreateTopBottomTickMarks(inTranslation, rightEdge, innerBottom, innerTop, outerBottom,
                                     outerTop, lineHeight, lineColor);
        if (leftEdge > innerLeft)
            CreateTopBottomTickMarks(inTranslation, leftEdge, innerBottom, innerTop, outerBottom,
                                     outerTop, lineHeight, lineColor);
    }
}

static void CreateLeftRightTickMarks(STranslation &inTranslation, QT3DSF32 inYPos, QT3DSF32 innerLeft,
                                     QT3DSF32 innerRight, QT3DSF32 outerLeft, QT3DSF32 outerRight,
                                     QT3DSF32 lineLength, QT3DSVec4 lineColor)
{
    CreatePixelHorzLine(inTranslation, inYPos, innerLeft - lineLength, innerLeft, lineColor);
    CreatePixelHorzLine(inTranslation, inYPos, innerRight, innerRight + lineLength, lineColor);
}

static void DrawTickMarksOnVerticalRects(STranslation &inTranslation, QT3DSF32 innerLeft,
                                         QT3DSF32 innerRight, QT3DSF32 innerBottom, QT3DSF32 innerTop,
                                         QT3DSF32 outerLeft, QT3DSF32 outerRight, QT3DSVec4 lineColor)
{
    QT3DSF32 centerPosY = floor(innerBottom + (innerTop - innerBottom) / 2.0f + .5f);
    CreateLeftRightTickMarks(inTranslation, centerPosY, innerLeft, innerRight, outerLeft,
                             outerRight, 15, lineColor);
    for (QT3DSU32 incrementor = 10;
         (centerPosY + incrementor) < innerTop && (centerPosY - incrementor) > innerBottom;
         incrementor += 10) {
        QT3DSF32 topEdge = centerPosY + incrementor;
        QT3DSF32 bottomEdge = centerPosY - incrementor;
        QT3DSF32 lineHeight = 0;
        if (incrementor % 100 == 0)
            lineHeight = 11;
        else if (incrementor % 20)
            lineHeight = 4;
        else
            lineHeight = 2;

        if (topEdge < innerTop)
            CreateLeftRightTickMarks(inTranslation, topEdge, innerLeft, innerRight, outerLeft,
                                     outerRight, lineHeight, lineColor);
        if (bottomEdge > innerBottom)
            CreateLeftRightTickMarks(inTranslation, bottomEdge, innerLeft, innerRight, outerLeft,
                                     outerRight, lineHeight, lineColor);
    }
}

class IGuideElementFactory
{
protected:
    virtual ~IGuideElementFactory() {}
public:
    virtual void CreateLine(QT3DSF32 inPos) = 0;
    virtual void CreateRect(QT3DSF32 inPosMin, QT3DSF32 inPosMax) = 0;
};

static void CreateGuide(IGuideElementFactory &inFactory, QT3DSF32 inPos, QT3DSF32 inWidth)
{
    QT3DSF32 halfWidth = inWidth / 2.0f;
    QT3DSF32 leftLine = floor(inPos + 1.0f - halfWidth);
    inFactory.CreateLine(leftLine);
    // Then we are done if not enough width
    if (inWidth < 2.0f)
        return;

    QT3DSF32 rightLine = leftLine + inWidth - 1;
    inFactory.CreateLine(rightLine);

    if (inWidth < 3.0f)
        return;
    QT3DSF32 rectStart = leftLine + 1;
    QT3DSF32 rectStop = rectStart + inWidth - 2.0f;
    inFactory.CreateRect(rectStart, rectStop);
}

struct SHorizontalGuideFactory : public IGuideElementFactory
{
    STranslation &m_Translation;
    QT3DSF32 m_Start;
    QT3DSF32 m_Stop;
    QT3DSVec4 m_LineColor;
    QT3DSVec4 m_FillColor;
    SHorizontalGuideFactory(STranslation &trans, QT3DSF32 start, QT3DSF32 stop, QT3DSVec4 lineColor,
                            QT3DSVec4 fillColor)
        : m_Translation(trans)
        , m_Start(start)
        , m_Stop(stop)
        , m_LineColor(lineColor)
        , m_FillColor(fillColor)
    {
    }
    void CreateLine(QT3DSF32 inPos) override
    {
        CreatePixelHorzLine(m_Translation, inPos, m_Start, m_Stop, m_LineColor);
    }

    void CreateRect(QT3DSF32 inPosMin, QT3DSF32 inPosMax) override
    {
        CreatePixelRect(m_Translation, m_Start, m_Stop, inPosMin, inPosMax, m_FillColor);
    }
};

struct SVerticalGuideFactory : public IGuideElementFactory
{
    STranslation &m_Translation;
    QT3DSF32 m_Start;
    QT3DSF32 m_Stop;
    QT3DSVec4 m_LineColor;
    QT3DSVec4 m_FillColor;
    SVerticalGuideFactory(STranslation &trans, QT3DSF32 start, QT3DSF32 stop, QT3DSVec4 lineColor,
                          QT3DSVec4 fillColor)
        : m_Translation(trans)
        , m_Start(start)
        , m_Stop(stop)
        , m_LineColor(lineColor)
        , m_FillColor(fillColor)
    {
    }
    void CreateLine(QT3DSF32 inPos) override
    {
        CreatePixelVertLine(m_Translation, inPos, m_Start, m_Stop, m_LineColor);
    }

    void CreateRect(QT3DSF32 inPosMin, QT3DSF32 inPosMax) override
    {
        CreatePixelRect(m_Translation, inPosMin, inPosMax, m_Start, m_Stop, m_FillColor);
    }
};

void STranslation::Render(int inWidgetId, bool inDrawGuides)
{
    // For now, we just render.
    // Next step will be to get the bounding boxes and such setup.
    // but we will want a custom renderer to do that.
    if (m_Scene) {
        // Note that begin frame is called before we allocate the bounding box and axis widgets so
        // that we can take advantage of the renderer's per-frame-allocator.
        m_Context.BeginFrame();
        // Render the bounding boxes and extra widgets.
        // This is called *before* the render because these sort of appendages need to be added
        // to the layer renderables.
        qt3dsdm::TInstanceHandleList theHandles = m_Doc.GetSelectedValue().GetSelectedInstances();

        // Don't show the bounding box or pivot for the component we are *in* the component
        SGraphObjectTranslator *theTranslator = nullptr;
        long theToolMode = g_StudioApp.GetToolMode();
        int theCameraToolMode = m_EditCameraEnabled ? (theToolMode & STUDIO_CAMERATOOL_MASK) : 0;
        bool shouldDisplayWidget = false;
        if (theCameraToolMode == 0) {
            switch (theToolMode) {
            default:
                break;
            case STUDIO_TOOLMODE_MOVE:
            case STUDIO_TOOLMODE_ROTATE:
            case STUDIO_TOOLMODE_SCALE:
                shouldDisplayWidget = true;
                break;
            };
        }

        bool selectedPath = false;

        for (size_t selectedIdx = 0, selectedEnd = theHandles.size(); selectedIdx < selectedEnd;
             ++selectedIdx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance = theHandles[selectedIdx];
            if (theInstance
                != m_Doc.GetDocumentReader().GetComponentForSlide(m_Doc.GetActiveSlide())) {
                if (m_Doc.GetDocumentReader().GetObjectTypeName(theInstance)
                    == L"PathAnchorPoint") {
                    theInstance = m_AssetGraph.GetParent(m_AssetGraph.GetParent(theInstance));
                    shouldDisplayWidget = false;
                }
                theTranslator = GetOrCreateTranslator(theInstance);
                // Get the tool mode right now.
                if (theTranslator) {
                    GraphObjectTypes::Enum theType(theTranslator->GetGraphObject().m_Type);
                    if (CStudioPreferences::IsBoundingBoxesOn()) {
                        switch (theType) {
                        case GraphObjectTypes::Node:
                            DrawGroupBoundingBoxes(*theTranslator);
                            break;
                        case GraphObjectTypes::Text:
                        case GraphObjectTypes::Model:
                        case GraphObjectTypes::Layer:
                        case GraphObjectTypes::Light:
                        case GraphObjectTypes::Path:
                            DrawNonGroupBoundingBoxes(*theTranslator);
                            break;
                        }
                    }
                    // Don't draw the axis if there is a widget.
                    if (CStudioPreferences::ShouldDisplayPivotPoint()
                        && shouldDisplayWidget == false) {
                        switch (theTranslator->GetGraphObject().m_Type) {
                        case GraphObjectTypes::Node:
                        case GraphObjectTypes::Text:
                        case GraphObjectTypes::Model:
                            DrawAxis(*theTranslator);
                            break;
                        }
                    }
                    if (theType == GraphObjectTypes::Path && selectedPath == false) {
                        selectedPath = true;
                        if (!m_PathWidget)
                            m_PathWidget = qt3ds::widgets::IPathWidget::CreatePathWidget(
                                m_Context.GetAllocator(), m_Context);
                        m_PathWidget->SetNode(
                            static_cast<SNode &>(theTranslator->GetGraphObject()));
                        m_Context.GetRenderer().AddRenderWidget(*m_PathWidget);
                    }
                }
            }
        }

        if (theHandles.size() > 1)
            theTranslator = nullptr;

        qt3ds::widgets::IStudioWidget *theNextWidget(nullptr);
        if (theTranslator && GraphObjectTypes::IsNodeType(theTranslator->GetGraphObject().m_Type)
            && theTranslator->GetGraphObject().m_Type != GraphObjectTypes::Layer) {

            qt3ds::render::SNode &theNode(
                static_cast<qt3ds::render::SNode &>(theTranslator->GetGraphObject()));
            SCamera *theRenderCamera = m_Context.GetRenderer().GetCameraForNode(theNode);
            bool isActiveCamera = theRenderCamera == (static_cast<SCamera *>(&theNode));
            if (shouldDisplayWidget && isActiveCamera == false
                    && theTranslator->GetGraphObject().m_Type != GraphObjectTypes::Camera) {
                switch (theToolMode) {
                default:
                    QT3DS_ASSERT(false);
                    break;
                case STUDIO_TOOLMODE_MOVE:
                    // Render translation widget
                    if (!m_TranslationWidget)
                        m_TranslationWidget = qt3ds::widgets::IStudioWidget::CreateTranslationWidget(
                            m_Context.GetAllocator());
                    theNextWidget = m_TranslationWidget.mPtr;
                    break;
                case STUDIO_TOOLMODE_ROTATE:
                    if (!m_RotationWidget)
                        m_RotationWidget = qt3ds::widgets::IStudioWidget::CreateRotationWidget(
                            m_Context.GetAllocator());
                    theNextWidget = m_RotationWidget.mPtr;
                    break;

                case STUDIO_TOOLMODE_SCALE:
                    if (!m_ScaleWidget)
                        m_ScaleWidget = qt3ds::widgets::IStudioWidget::CreateScaleWidget(
                            m_Context.GetAllocator());
                    theNextWidget = m_ScaleWidget.mPtr;
                    break;
                }
                if (theNextWidget) {
                    theNextWidget->SetNode(static_cast<SNode &>(theTranslator->GetGraphObject()));
                    m_Context.GetRenderer().AddRenderWidget(*theNextWidget);
                }
            }
        }
        if (m_LastRenderedWidget.mPtr && m_LastRenderedWidget.mPtr != theNextWidget)
            ResetWidgets();

        m_LastRenderedWidget = theNextWidget;
        if (m_LastRenderedWidget) {
            m_LastRenderedWidget->SetSubComponentId(inWidgetId);
            switch (g_StudioApp.GetManipulationMode()) {
            case StudioManipulationModes::Local:
                m_LastRenderedWidget->SetRenderWidgetMode(qt3ds::render::RenderWidgetModes::Local);
                break;
            case StudioManipulationModes::Global:
                m_LastRenderedWidget->SetRenderWidgetMode(qt3ds::render::RenderWidgetModes::Global);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }

        m_Scene->PrepareForRender(GetViewportDimensions(), m_Context);

        m_Context.RunRenderTasks();

        if (m_EditCameraEnabled) {
            if (m_GradientWidget == nullptr)
                m_GradientWidget = qt3ds::widgets::SGradientWidget
                                    ::CreateGradientWidget(m_Context.GetAllocator());
            // render gradient background
            if (m_EditCameraEnabled) {
                SNode *node = GetEditCameraLayer();
                m_GradientWidget->SetNode(*node);
                m_GradientWidget->Render(m_Context.GetRenderWidgetContext(),
                                         m_Context.GetRenderContext(),
                                         m_EditCameraInfo.IsOrthographic());
            } else {
                m_GradientWidget->Render(m_Context.GetRenderWidgetContext(),
                                         m_Context.GetRenderContext(), true);
            }
        }

        m_Scene->Render(GetViewportDimensions(), m_Context, SScene::DoNotClear);
        if (m_editModeCamerasAndLights.size() > 0) {
            if (!m_VisualAidWidget) {
                m_VisualAidWidget = qt3ds::widgets::SVisualAidWidget
                    ::CreateVisualAidWidget(m_Context.GetAllocator());
            }
            for (SGraphObjectTranslator *translator : m_editModeCamerasAndLights) {
                SGraphObject &object = translator->GetGraphObject();
                qt3dsdm::Qt3DSDMInstanceHandle handle = translator->GetInstanceHandle();
                m_VisualAidWidget->setSelected(false);

                for (int j = 0; j < theHandles.size(); ++j) {
                    if (handle == theHandles[j]) {
                        m_VisualAidWidget->setSelected(true);
                        break;
                    }
                }

                if (object.m_Type == GraphObjectTypes::Camera) {
                    QT3DSVec2 dim = QT3DSVec2(m_InnerRect.m_Right - m_InnerRect.m_Left,
                                              m_InnerRect.m_Top - m_InnerRect.m_Bottom);
                    NVRenderRectF theViewport(0, 0, dim.x, dim.y);
                    static_cast<SCamera *>(&object)->CalculateGlobalVariables(theViewport, dim);
                }
                m_VisualAidWidget->SetNode(static_cast<SNode *>(&object));
                m_VisualAidWidget->Render(m_Context.GetRenderWidgetContext(),
                                          m_Context.GetRenderContext());
            }
        }

        if (inDrawGuides && !m_EditCameraEnabled && !g_StudioApp.IsAuthorZoom()) {
            m_GuideContainer.clear();
            // Figure out the matte area.
            NVRenderRect theContextViewport = m_Context.GetContextViewport();
            NVRenderRect thePresentationViewport = m_Context.GetPresentationViewport();
            m_Context.GetRenderContext().SetViewport(theContextViewport);
            QT3DSI32 innerLeft = thePresentationViewport.m_X;
            QT3DSI32 innerRight = thePresentationViewport.m_X + thePresentationViewport.m_Width;
            QT3DSI32 innerBottom = thePresentationViewport.m_Y;
            QT3DSI32 innerTop = thePresentationViewport.m_Y + thePresentationViewport.m_Height;

            QT3DSI32 outerLeft = innerLeft - 16;
            QT3DSI32 outerRight = innerRight + 16;
            QT3DSI32 outerBottom = innerBottom - 16;
            QT3DSI32 outerTop = innerTop + 16;
            // Retain the rects for picking purposes.
            m_InnerRect = SRulerRect(innerLeft, innerTop, innerRight, innerBottom);
            m_OuterRect = SRulerRect(outerLeft, outerTop, outerRight, outerBottom);

            // Draw tick marks around the presentation
            CreatePixelRect(*this, (QT3DSF32)outerLeft, (QT3DSF32)innerLeft, (QT3DSF32)innerBottom,
                            (QT3DSF32)innerTop, m_rectColor);
            CreatePixelRect(*this, (QT3DSF32)innerRight, (QT3DSF32)outerRight,
                            (QT3DSF32)innerBottom, (QT3DSF32)innerTop, m_rectColor);
            CreatePixelRect(*this, (QT3DSF32)innerLeft, (QT3DSF32)innerRight, (QT3DSF32)outerBottom,
                            (QT3DSF32)innerBottom, m_rectColor);
            CreatePixelRect(*this, (QT3DSF32)innerLeft, (QT3DSF32)innerRight, (QT3DSF32)innerTop,
                            (QT3DSF32)outerTop, m_rectColor);
            DrawTickMarksOnHorizontalRects(*this, (QT3DSF32)innerLeft, (QT3DSF32)innerRight,
                                           (QT3DSF32)innerBottom, (QT3DSF32)innerTop,
                                           (QT3DSF32)outerBottom, (QT3DSF32)outerTop, m_lineColor);
            DrawTickMarksOnVerticalRects(*this, (QT3DSF32)innerLeft, (QT3DSF32)innerRight,
                                         (QT3DSF32)innerBottom, (QT3DSF32)innerTop,
                                         (QT3DSF32)outerLeft, (QT3DSF32)outerRight, m_lineColor);
            qt3dsdm::TGuideHandleList theGuides = m_Doc.GetDocumentReader().GetGuides();
            qt3dsdm::Qt3DSDMGuideHandle theSelectedGuide;
            Q3DStudio::SSelectedValue theSelection = m_Doc.GetSelectedValue();
            if (theSelection.getType() == Q3DStudio::SelectedValueTypes::Guide)
                theSelectedGuide = theSelection.getData<qt3dsdm::Qt3DSDMGuideHandle>();

            // Draw guides
            for (size_t guideIdx = 0, guideEnd = theGuides.size(); guideIdx < guideEnd;
                 ++guideIdx) {
                qt3dsdm::SGuideInfo theInfo =
                    m_Doc.GetDocumentReader().GetGuideInfo(theGuides[guideIdx]);
                bool isGuideSelected = theGuides[guideIdx] == theSelectedGuide;
                QT3DSVec4 theColor = isGuideSelected ? m_selectedGuideColor : m_guideColor;
                QT3DSVec4 theFillColor = isGuideSelected ? m_selectedGuideFillColor
                                                         : m_guideFillColor;
                switch (theInfo.m_Direction) {
                case qt3dsdm::GuideDirections::Horizontal: {
                    SHorizontalGuideFactory theFactory(*this, (QT3DSF32)innerLeft, (QT3DSF32)innerRight,
                                                       theColor, theFillColor);
                    CreateGuide(theFactory, (QT3DSF32)m_InnerRect.m_Bottom + theInfo.m_Position,
                                (QT3DSF32)theInfo.m_Width);
                } break;
                case qt3dsdm::GuideDirections::Vertical: {
                    SVerticalGuideFactory theFactory(*this, (QT3DSF32)innerBottom, (QT3DSF32)innerTop,
                                                     theColor, theFillColor);
                    CreateGuide(theFactory, (QT3DSF32)m_InnerRect.m_Left + theInfo.m_Position,
                                (QT3DSF32)theInfo.m_Width);
                } break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }
            m_Context.GetPixelGraphicsRenderer().Render(
                qt3ds::foundation::toDataRef(m_GuideContainer.data(), m_GuideContainer.size()));

            m_GuideContainer.clear();
            m_GuideAllocator.reset();
        }

        m_Context.EndFrame();

        if (m_ZoomRender.hasValue()) {
            RenderZoomRender(*m_ZoomRender);
            m_ZoomRender = Empty();
        }

        // Render the pick buffer, useful for debugging why a widget wasn't hit.
        /*
        if ( m_PickBuffer )
        {
            qt3ds::render::NVRenderContext& theRenderContext( m_Context.GetRenderContext() );
            qt3ds::render::STextureDetails thePickDetails = m_PickBuffer->GetTextureDetails();
            theRenderContext.SetViewport( qt3ds::render::NVRenderRect( 0, 0, thePickDetails.m_Width,
        thePickDetails.m_Height ) );
            qt3ds::render::SCamera theCamera;
            theCamera.MarkDirty( qt3ds::render::NodeTransformDirtyFlag::TransformIsDirty );
            theCamera.m_Flags.SetOrthographic( true );
            QT3DSVec2 theDimensions( (QT3DSF32)thePickDetails.m_Width, (QT3DSF32)thePickDetails.m_Height );
            theCamera.CalculateGlobalVariables( render::NVRenderRectF( 0, 0, theDimensions.x,
        theDimensions.y ), theDimensions );
            QT3DSMat44 theVP;
            theCamera.CalculateViewProjectionMatrix( theVP );
            theRenderContext.SetCullingEnabled( false );
            theRenderContext.SetBlendingEnabled( false );
            theRenderContext.SetDepthTestEnabled( false );
            theRenderContext.SetDepthWriteEnabled( false );
            m_Context.GetRenderer().RenderQuad( theDimensions, theVP, *m_PickBuffer );
        }*/
    }
}

void STranslation::ResetWidgets()
{
    if (m_ScaleWidget)
        m_ScaleWidget->SetAxisScale(QT3DSVec3(1, 1, 1));
    if (m_RotationWidget)
        m_RotationWidget->ClearRotationEdges();
    m_CumulativeRotation = 0.0f;
}

void STranslation::DoPrepareForDrag(SNode *inSelectedNode)
{
    if (inSelectedNode == nullptr)
        return;

    m_MouseDownNode = *inSelectedNode;
    m_MouseDownParentGlobalTransformInverse = Empty();
    m_MouseDownParentRotationInverse = Empty();
    m_MouseDownGlobalRotation = Empty();
    // Orphan this node manually since it is a straight copy
    m_MouseDownNode.m_Parent = nullptr;
    m_MouseDownNode.m_FirstChild = nullptr;
    m_MouseDownNode.m_NextSibling = nullptr;
    SCamera *theCamera = m_Context.GetRenderer().GetCameraForNode(*inSelectedNode);
    m_CumulativeRotation = 0.0f;
    if (theCamera == nullptr)
        return;
    m_MouseDownCamera = *theCamera;
    m_LastPathDragValue = Empty();
}

void STranslation::EndDrag()
{
    ResetWidgets();
}

bool STranslation::IsPathWidgetActive()
{
    qt3dsdm::TInstanceHandleList theHandles = m_Doc.GetSelectedValue().GetSelectedInstances();
    for (size_t selectedIdx = 0, selectedEnd = theHandles.size(); selectedIdx < selectedEnd;
         ++selectedIdx) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(theHandles[selectedIdx]);
        if (m_Doc.GetDocumentReader().GetObjectTypeName(theInstance) == L"PathAnchorPoint")
            theInstance = m_AssetGraph.GetParent(m_AssetGraph.GetParent(theInstance));
        SGraphObjectTranslator *theTranslator = GetOrCreateTranslator(theInstance);
        if (theTranslator && theTranslator->GetGraphObject().m_Type == GraphObjectTypes::Path)
            return true;
    }
    return false;
}

inline qt3ds::render::SLayer *GetLayerForNode(const qt3ds::render::SNode &inNode)
{
    SNode *theNode;
    // empty loop intentional
    for (theNode = const_cast<SNode *>(&inNode);
         theNode && theNode->m_Type != GraphObjectTypes::Layer; theNode = theNode->m_Parent) {
    }
    if (theNode && theNode->m_Type == GraphObjectTypes::Layer)
        return static_cast<SLayer *>(theNode);
    return nullptr;
}

void STranslation::RenderZoomRender(SZoomRender &inRender)
{
    SLayer *theLayer(inRender.m_Layer);
    CPt thePoint(inRender.m_Point);
    if (theLayer) {
        qt3ds::render::IQt3DSRenderer &theRenderer(m_Context.GetRenderer());
        Option<qt3ds::render::SLayerPickSetup> thePickSetup(
            theRenderer.GetLayerPickSetup(*theLayer, QT3DSVec2((QT3DSF32)thePoint.x, (QT3DSF32)thePoint.y),
                                          QSize(16, 16)));
        if (thePickSetup.hasValue()) {
            qt3ds::render::NVRenderContext &theRenderContext(m_Context.GetRenderContext());
            theRenderContext.SetViewport(qt3ds::render::NVRenderRect(0, 0, 100, 100));
            theRenderContext.SetScissorRect(qt3ds::render::NVRenderRect(0, 0, 100, 100));
            theRenderContext.SetDepthWriteEnabled(true);
            theRenderContext.SetScissorTestEnabled(true);
            theRenderContext.SetClearColor(QT3DSVec4(.2f, .2f, .2f, 0.0f));
            theRenderContext.Clear(qt3ds::render::NVRenderClearFlags(
                qt3ds::render::NVRenderClearValues::Color | qt3ds::render::NVRenderClearValues::Depth));
            theRenderer.RunLayerRender(*theLayer, thePickSetup->m_ViewProjection);
            theRenderContext.SetScissorTestEnabled(false);
        }
    }
}

void STranslation::DrawBoundingBox(SNode &inNode, QT3DSVec3 inColor)
{
    qt3ds::NVBounds3 theBounds = inNode.GetBounds(m_Context.GetBufferManager(),
                                               m_Context.GetPathManager(), true, this);
    qt3ds::render::IRenderWidget &theBBoxWidget = qt3ds::render::IRenderWidget::CreateBoundingBoxWidget(
        inNode, theBounds, inColor, m_Context.GetRenderer().GetPerFrameAllocator());
    m_Context.GetRenderer().AddRenderWidget(theBBoxWidget);
}

void STranslation::DrawAxis(SGraphObjectTranslator &inTranslator)
{
    if (GraphObjectTypes::IsNodeType(inTranslator.GetGraphObject().m_Type)) {
        qt3ds::render::IRenderWidget &theAxisWidget = qt3ds::render::IRenderWidget::CreateAxisWidget(
            static_cast<SNode &>(inTranslator.GetGraphObject()),
            m_Context.GetRenderer().GetPerFrameAllocator());
        m_Context.GetRenderer().AddRenderWidget(theAxisWidget);
    } else {
        QT3DS_ASSERT(false);
    }
}

Option<QT3DSU32> STranslation::PickWidget(CPt inMouseCoords, TranslationSelectMode::Enum,
                                       qt3ds::widgets::IStudioWidgetBase &inWidget)
{
    SNode &theNode = inWidget.GetNode();
    SGraphObjectTranslator *theWidgetTranslator =
        theNode.m_UserData.DynamicCast<SGraphObjectTranslator>();
    SLayer *theLayer = GetLayerForNode(theNode);
    if (theLayer && theWidgetTranslator) {
        Option<qt3ds::render::SLayerPickSetup> thePickSetup(
            m_Context.GetRenderer().GetLayerPickSetup(
                *theLayer, QT3DSVec2((QT3DSF32)inMouseCoords.x, (QT3DSF32)inMouseCoords.y),
                QSize(4, 4)));
        if (thePickSetup.hasValue()) {
            qt3ds::render::NVRenderContext &theContext(m_Context.GetRenderContext());
            qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderFrameBuffer *>
                __currentrt(theContext, &qt3ds::render::NVRenderContext::GetRenderTarget,
                            &qt3ds::render::NVRenderContext::SetRenderTarget);
            qt3ds::render::NVRenderFrameBuffer *theFBO =
                m_Context.GetResourceManager().AllocateFrameBuffer();
            const QT3DSU32 fboDims = 8;
            if (!m_PickBuffer) {
                m_PickBuffer = theContext.CreateTexture2D();
                m_PickBuffer->SetTextureData(qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8>(), 0, fboDims,
                                             fboDims,
                                             qt3ds::render::NVRenderTextureFormats::LuminanceAlpha8);
            }
            qt3ds::render::NVRenderRenderBuffer *theRenderBuffer =
                m_Context.GetResourceManager().AllocateRenderBuffer(
                    fboDims, fboDims, qt3ds::render::NVRenderRenderBufferFormats::Depth16);
            theFBO->Attach(qt3ds::render::NVRenderFrameBufferAttachments::Color0,
                           qt3ds::render::NVRenderTextureOrRenderBuffer(*m_PickBuffer));
            theFBO->Attach(qt3ds::render::NVRenderFrameBufferAttachments::Depth,
                           qt3ds::render::NVRenderTextureOrRenderBuffer(*theRenderBuffer));
            qt3ds::render::NVRenderRect theViewport(0, 0, fboDims, fboDims);
            theContext.SetViewport(theViewport);
            theContext.SetDepthWriteEnabled(true);
            theContext.SetDepthTestEnabled(true);
            theContext.SetScissorTestEnabled(false);
            theContext.SetBlendingEnabled(false);
            theContext.SetClearColor(QT3DSVec4(0, 0, 0, 0));
            theContext.Clear(qt3ds::render::NVRenderClearFlags(
                qt3ds::render::NVRenderClearValues::Color | qt3ds::render::NVRenderClearValues::Depth));
            inWidget.RenderPick(thePickSetup->m_ProjectionPreMultiply, theContext,
                                QSize(4, 4));
            // Now read the pixels back.
            m_PixelBuffer.resize(fboDims * fboDims * 3);
            theContext.ReadPixels(theViewport, qt3ds::render::NVRenderReadPixelFormats::RGB8,
                                  m_PixelBuffer);
            m_Context.GetResourceManager().Release(*theFBO);
            m_Context.GetResourceManager().Release(*theRenderBuffer);
            eastl::hash_map<QT3DSU32, QT3DSU32> tallies;
            QT3DSU32 numPixels = fboDims * fboDims;
            for (QT3DSU32 idx = 0; idx < numPixels; ++idx) {
                qt3ds::QT3DSU16 theChannelAmount =
                    m_PixelBuffer[idx * 3] + (m_PixelBuffer[idx * 3 + 1] << 8);
                if (theChannelAmount)
                    tallies.insert(eastl::make_pair(theChannelAmount, (QT3DSU32)0)).first->second += 1;
            }
            QT3DSU32 tallyMaxTally = 0;
            QT3DSU32 tallyMaxIdx = 0;
            for (eastl::hash_map<QT3DSU32, QT3DSU32>::iterator iter = tallies.begin(),
                                                         end = tallies.end();
                 iter != end; ++iter) {
                if (iter->second > tallyMaxTally) {
                    tallyMaxTally = iter->second;
                    tallyMaxIdx = iter->first;
                }
            }
            if (tallyMaxIdx > 0) {
                return tallyMaxIdx;
            }
        }
    }
    return Empty();
}

SStudioPickValue STranslation::Pick(CPt inMouseCoords, TranslationSelectMode::Enum inSelectMode)
{
    bool requestRender = false;

    if (m_Doc.GetDocumentReader().AreGuidesEditable()) {
        qt3dsdm::TGuideHandleList theGuides = m_Doc.GetDocumentReader().GetGuides();
        CPt renderSpacePt(inMouseCoords.x - (long)m_InnerRect.m_Left,
                          (long)GetViewportDimensions().y - inMouseCoords.y
                              - (long)m_InnerRect.m_Bottom);
        for (size_t guideIdx = 0, guideEnd = theGuides.size(); guideIdx < guideEnd; ++guideIdx) {
            qt3dsdm::SGuideInfo theGuideInfo =
                m_Doc.GetDocumentReader().GetGuideInfo(theGuides[guideIdx]);
            float width = (theGuideInfo.m_Width / 2.0f) + 2.0f;
            switch (theGuideInfo.m_Direction) {
            case qt3dsdm::GuideDirections::Horizontal:
                if (fabs((float)renderSpacePt.y - theGuideInfo.m_Position) <= width)
                    return theGuides[guideIdx];
                break;
            case qt3dsdm::GuideDirections::Vertical:
                if (fabs((float)renderSpacePt.x - theGuideInfo.m_Position) <= width)
                    return theGuides[guideIdx];
                break;
            }
        }
    }
    if (IsPathWidgetActive()) {
        Option<QT3DSU32> picked = PickWidget(inMouseCoords, inSelectMode, *m_PathWidget);
        if (picked.hasValue()) {
            RequestRender();
            DoPrepareForDrag(&m_PathWidget->GetNode());
            return m_PathWidget->PickIndexToPickValue(*picked);
        }
    }
    // Pick against the widget first if possible.
    if (m_LastRenderedWidget && (m_LastRenderedWidget->GetNode().m_Flags.IsActive()
                                 || m_LastRenderedWidget->GetNode().m_Type
                                 == GraphObjectTypes::Light
                                 || m_LastRenderedWidget->GetNode().m_Type
                                 == GraphObjectTypes::Camera)) {
        Option<QT3DSU32> picked = PickWidget(inMouseCoords, inSelectMode, *m_LastRenderedWidget);
        if (picked.hasValue()) {
            RequestRender();
            DoPrepareForDrag(&m_LastRenderedWidget->GetNode());
            return m_LastRenderedWidget->PickIndexToPickValue(*picked);
        }
    }
    // Pick against Lights and Cameras
    // This doesn't use the color picker or renderer pick
    float lastDist = 99999999999999.0f;
    int lastIndex = -1;
    for (size_t i = 0; i < m_editModeCamerasAndLights.size(); ++i) {
        const QT3DSVec2 mouseCoords((QT3DSF32)inMouseCoords.x, (QT3DSF32)inMouseCoords.y);
        float dist;
        SGraphObject &object = m_editModeCamerasAndLights[i]->GetGraphObject();
        m_VisualAidWidget->SetNode(static_cast<SNode *>(&object));
        if (m_VisualAidWidget->pick(m_Context.GetRenderer().GetRenderWidgetContext(),
                                    dist, GetViewportDimensions(), mouseCoords)) {
            if (dist < lastDist) {
                lastDist = dist;
                lastIndex = i;
            }
        }
    }

    if (m_Scene && m_Scene->m_FirstChild) {
        qt3ds::render::Qt3DSRenderPickResult thePickResult =
            m_Context.GetRenderer().Pick(*m_Scene->m_FirstChild, GetViewportDimensions(),
                                            QT3DSVec2((QT3DSF32)inMouseCoords.x, (QT3DSF32)inMouseCoords.y));
        if (thePickResult.m_HitObject) {
            const SGraphObject &theObject = *thePickResult.m_HitObject;

            // check hit distance to cameras and lights
            if (lastIndex != -1 && thePickResult.m_CameraDistanceSq > lastDist * lastDist)
                return m_editModeCamerasAndLights[lastIndex]->GetInstanceHandle();

            if (theObject.m_Type == GraphObjectTypes::Model
                || theObject.m_Type == GraphObjectTypes::Text
                || theObject.m_Type == GraphObjectTypes::Path) {
                const SNode &theTranslatorModel(static_cast<const SNode &>(theObject));
                SGraphObjectTranslator *theTranslator =
                    theTranslatorModel.m_UserData.DynamicCast<SGraphObjectTranslator>();
                const SNode *theModelPtr = &theTranslatorModel;
                if (theTranslator->GetPossiblyAliasedInstanceHandle()
                    != theTranslator->GetInstanceHandle()) {
                    theTranslator =
                        GetOrCreateTranslator(theTranslator->GetPossiblyAliasedInstanceHandle());
                    theModelPtr =
                        static_cast<const SNode *>(&theTranslator->GetNonAliasedGraphObject());
                }
                Qt3DSDMInstanceHandle theActiveComponent =
                    m_Reader.GetComponentForSlide(m_Doc.GetActiveSlide());
                if (inSelectMode == TranslationSelectMode::Group) {
                    // Bounce up the hierarchy till one of two conditions are met
                    // the parent is a layer or the our component is the active component
                    // but the parent's is not.
                    while (theTranslator && GraphObjectTypes::IsNodeType(
                                                theTranslator->GetGraphObject().m_Type)) {
                        SNode *myNode = static_cast<SNode *>(&theTranslator->GetGraphObject());
                        if (myNode->m_Parent == nullptr) {
                            theTranslator = nullptr;
                            break;
                        }
                        SNode *parentNode = myNode->m_Parent;
                        SGraphObjectTranslator *theParentTranslator =
                            parentNode->m_UserData.DynamicCast<SGraphObjectTranslator>();
                        Qt3DSDMInstanceHandle myComponent =
                            m_Reader.GetAssociatedComponent(theTranslator->GetInstanceHandle());
                        Qt3DSDMInstanceHandle myParentComponent = m_Reader.GetAssociatedComponent(
                            theParentTranslator->GetInstanceHandle());
                        if (parentNode->m_Type == GraphObjectTypes::Layer) {
                            if (myParentComponent != theActiveComponent)
                                theTranslator = nullptr;
                            break;
                        }
                        if (myComponent == theActiveComponent
                            && myParentComponent != theActiveComponent)
                            break;
                        theTranslator = theParentTranslator;
                    }
                } else {
                    // Bounce up until we get into the active component and then stop.
                    while (inSelectMode == TranslationSelectMode::Single && theTranslator
                           && GraphObjectTypes::IsNodeType(theTranslator->GetGraphObject().m_Type)
                           && m_Reader.GetAssociatedComponent(theTranslator->GetInstanceHandle())
                               != theActiveComponent) {
                        SNode *theNode = static_cast<SNode *>(&theTranslator->GetGraphObject());
                        theNode = theNode->m_Parent;
                        if (theNode && theNode->m_Type != GraphObjectTypes::Layer)
                            theTranslator =
                                theNode->m_UserData.DynamicCast<SGraphObjectTranslator>();
                        else
                            theTranslator = nullptr;
                    }
                }

                if (theTranslator) {
                    QT3DS_ASSERT(GraphObjectTypes::IsNodeType(theTranslator->GetGraphObject().m_Type));
                    DoPrepareForDrag(static_cast<SNode *>(&theTranslator->GetGraphObject()));
                    return theTranslator->GetInstanceHandle();
                }
            }
        }
        if (requestRender)
            RequestRender();
    }

    if (lastIndex != -1)
        return m_editModeCamerasAndLights[lastIndex]->GetInstanceHandle();

    return SStudioPickValue();
}

qt3ds::foundation::Option<qt3dsdm::SGuideInfo> STranslation::PickRulers(CPt inMouseCoords)
{
    CPt renderSpacePt(inMouseCoords.x, (long)GetViewportDimensions().y - inMouseCoords.y);
    // If mouse is inside outer rect but outside inner rect.
    if (m_OuterRect.Contains(renderSpacePt.x, renderSpacePt.y)
        && !m_InnerRect.Contains(renderSpacePt.x, renderSpacePt.y)) {
        std::shared_ptr<qt3dsdm::IGuideSystem> theGuideSystem =
            m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetGuideSystem();
        if (renderSpacePt.x >= m_InnerRect.m_Left && renderSpacePt.x <= m_InnerRect.m_Right) {
            return qt3dsdm::SGuideInfo((QT3DSF32)renderSpacePt.y - (QT3DSF32)m_InnerRect.m_Bottom,
                                     qt3dsdm::GuideDirections::Horizontal);
        } else if (renderSpacePt.y >= m_InnerRect.m_Bottom
                   && renderSpacePt.y <= m_InnerRect.m_Top) {
            return qt3dsdm::SGuideInfo((QT3DSF32)renderSpacePt.x - (QT3DSF32)m_InnerRect.m_Left,
                                     qt3dsdm::GuideDirections::Vertical);
        }
    }
    return qt3ds::foundation::Option<qt3dsdm::SGuideInfo>();
}

QT3DSVec3 STranslation::GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inInstance, CPt inPos)
{
    ClearDirtySet();
    SGraphObjectTranslator *theTranslator = GetOrCreateTranslator(inInstance);
    if (theTranslator == nullptr)
        return QT3DSVec3(0, 0, 0);
    if (GraphObjectTypes::IsNodeType(theTranslator->GetGraphObject().m_Type) == false)
        return QT3DSVec3(0, 0, 0);
    SNode *theNode = static_cast<SNode *>(&theTranslator->GetGraphObject());
    SCamera *theCamera = m_Context.GetRenderer().GetCameraForNode(*theNode);
    {
        // Get the node's parent
        Qt3DSDMInstanceHandle theParent = m_AssetGraph.GetParent(inInstance);
        SGraphObjectTranslator *theParentTranslator = GetOrCreateTranslator(theParent);
        if (theParentTranslator
            && GraphObjectTypes::IsNodeType(theParentTranslator->GetGraphObject().m_Type))
            theCamera = m_Context.GetRenderer().GetCameraForNode(
                *static_cast<SNode *>(&theParentTranslator->GetGraphObject()));
    }
    if (theCamera == nullptr)
        return QT3DSVec3(0, 0, 0);

    QT3DSVec3 theGlobalPos(theNode->GetGlobalPos());
    return m_Context.GetRenderer().UnprojectToPosition(*theCamera, theGlobalPos,
                                                          QT3DSVec2((QT3DSF32)inPos.x, (QT3DSF32)inPos.y));
}

static void CheckLockToAxis(QT3DSF32 &inXDistance, QT3DSF32 &inYDistance, bool inLockToAxis)
{
    if (inLockToAxis) {
        if (fabs(inXDistance) > fabs(inYDistance))
            inYDistance = 0;
        else
            inXDistance = 0;
    }
}

void STranslation::ApplyPositionalChange(QT3DSVec3 inDiff, SNode &inNode,
                                         CUpdateableDocumentEditor &inEditor)
{
    if (m_MouseDownParentGlobalTransformInverse.isEmpty()) {
        if (inNode.m_Parent)
            m_MouseDownParentGlobalTransformInverse =
                inNode.m_Parent->m_GlobalTransform.getInverse();
        else
            m_MouseDownParentGlobalTransformInverse = QT3DSMat44::createIdentity();
    }
    QT3DSMat44 theGlobalTransform = m_MouseDownNode.m_GlobalTransform;
    QT3DSMat44 theNewLocalTransform =
        m_MouseDownParentGlobalTransformInverse.getValue() * theGlobalTransform;
    QT3DSVec3 theOldPos = theNewLocalTransform.column3.getXYZ();
    theOldPos.z *= -1;

    theGlobalTransform.column3 += QT3DSVec4(inDiff, 0.0f);
    theNewLocalTransform = m_MouseDownParentGlobalTransformInverse.getValue() * theGlobalTransform;
    QT3DSVec3 thePos = theNewLocalTransform.column3.getXYZ();
    thePos.z *= -1;

    QT3DSVec3 theDiff = thePos - theOldPos;

    SetPosition(m_MouseDownNode.m_Position + theDiff, inEditor);
}

void STranslation::TranslateSelectedInstanceAlongCameraDirection(
    CPt inOriginalCoords, CPt inMouseCoords, CUpdateableDocumentEditor &inEditor)
{
    SNode *theNode = GetSelectedNode();
    if (theNode == nullptr)
        return;
    SCamera *theCamera = m_Context.GetRenderer().GetCameraForNode(*theNode);
    if (theCamera == nullptr)
        return;
    QT3DSF32 theYDistance = QT3DSF32(inMouseCoords.y - inOriginalCoords.y);
    if (fabs(theYDistance) == 0)
        return;

    QT3DSF32 theMouseMultiplier = 1.0f / 2.0f;
    QT3DSF32 theDistanceMultiplier = 1.0f + theYDistance * theMouseMultiplier;
    QT3DSVec3 theCameraDir = m_MouseDownCamera.GetDirection();

    QT3DSVec3 theDiff = theCameraDir * theDistanceMultiplier;
    ApplyPositionalChange(theDiff, *theNode, inEditor);
}

void STranslation::TranslateSelectedInstance(CPt inOriginalCoords, CPt inMouseCoords,
                                             CUpdateableDocumentEditor &inEditor, bool inLockToAxis)
{
    SNode *theNode = GetSelectedNode();
    if (theNode == nullptr)
        return;
    qt3ds::render::IQt3DSRenderer &theRenderer(m_Context.GetRenderer());

    QT3DSF32 theXDistance = QT3DSF32(inMouseCoords.x - inOriginalCoords.x);
    QT3DSF32 theYDistance = QT3DSF32(inMouseCoords.y - inOriginalCoords.y);
    if (fabs(theXDistance) == 0 && fabs(theYDistance) == 0)
        return;

    CheckLockToAxis(theXDistance, theYDistance, inLockToAxis);

    inMouseCoords.x = inOriginalCoords.x + (long)theXDistance;
    inMouseCoords.y = inOriginalCoords.y + (long)theYDistance;
    QT3DSVec3 theNodeGlobal = m_MouseDownNode.GetGlobalPos();
    QT3DSVec3 theOriginalPos = theRenderer.UnprojectToPosition(
        *theNode, theNodeGlobal, QT3DSVec2((QT3DSF32)inOriginalCoords.x, (QT3DSF32)inOriginalCoords.y));
    QT3DSVec3 theNewPos = theRenderer.UnprojectToPosition(
        *theNode, theNodeGlobal, QT3DSVec2((QT3DSF32)inMouseCoords.x, (QT3DSF32)inMouseCoords.y));

    QT3DSVec3 theDiff = theNewPos - theOriginalPos;
    ApplyPositionalChange(theDiff, *theNode, inEditor);
}

void STranslation::ScaleSelectedInstanceZ(CPt inOriginalCoords, CPt inMouseCoords,
                                          CUpdateableDocumentEditor &inEditor)
{
    SNode *theNode = GetSelectedNode();
    if (theNode == nullptr)
        return;

    // Scale scales uniformly and responds to mouse Y only.
    QT3DSF32 theYDistance = (QT3DSF32)inMouseCoords.y - (QT3DSF32)inOriginalCoords.y;
    if (fabs(theYDistance) == 0)
        return;

    QT3DSF32 theMouseMultiplier = 1.0f / 40.0f;
    QT3DSF32 theScaleMultiplier = 1.0f + theYDistance * theMouseMultiplier;

    SetScale(QT3DSVec3(m_MouseDownNode.m_Scale.x, m_MouseDownNode.m_Scale.y,
                    m_MouseDownNode.m_Scale.z * theScaleMultiplier),
             inEditor);
}

void STranslation::ScaleSelectedInstance(CPt inOriginalCoords, CPt inMouseCoords,
                                         CUpdateableDocumentEditor &inEditor)
{
    SNode *theNode = GetSelectedNode();
    if (theNode == nullptr)
        return;

    // Scale scales uniformly and responds to mouse Y only.
    QT3DSF32 theYDistance = (QT3DSF32)inMouseCoords.y - (QT3DSF32)inOriginalCoords.y;
    if (fabs(theYDistance) == 0)
        return;

    QT3DSF32 theMouseMultiplier = 1.0f / 40.0f;
    QT3DSF32 theScaleMultiplier = 1.0f + theYDistance * theMouseMultiplier;

    SetScale(m_MouseDownNode.m_Scale * theScaleMultiplier, inEditor);
}

void STranslation::CalculateNodeGlobalRotation(SNode &inNode)
{
    if (inNode.m_Parent)
        CalculateNodeGlobalRotation(*inNode.m_Parent);
    if (m_MouseDownParentRotationInverse.isEmpty()) {
        m_MouseDownParentRotationInverse = QT3DSMat33::createIdentity();
        m_MouseDownGlobalRotation = QT3DSMat33::createIdentity();
    }

    QT3DSMat44 localRotation;
    inNode.CalculateRotationMatrix(localRotation);
    if (inNode.m_Flags.IsLeftHanded())
        SNode::FlipCoordinateSystem(localRotation);
    QT3DSMat33 theRotation;
    SNode::GetMatrixUpper3x3(theRotation, localRotation);

    m_MouseDownParentRotationInverse = m_MouseDownGlobalRotation;
    m_MouseDownGlobalRotation = m_MouseDownGlobalRotation.getValue() * theRotation;
}

void STranslation::ApplyRotationToSelectedInstance(const QT3DSQuat &inFinalRotation, SNode &inNode,
                                                   CUpdateableDocumentEditor &inEditor,
                                                   bool inIsMouseRelative)
{
    if (m_MouseDownParentRotationInverse.isEmpty()) {
        CalculateNodeGlobalRotation(inNode);
        m_MouseDownParentRotationInverse = m_MouseDownParentRotationInverse->getInverse();
    }
    QT3DSMat33 theRotationMatrix(inFinalRotation);

    QT3DSMat33 theFinalGlobal = theRotationMatrix * m_MouseDownGlobalRotation.getValue();
    QT3DSMat33 theLocalGlobal = m_MouseDownParentRotationInverse.getValue() * theFinalGlobal;
    QT3DSVec3 theRotations = inNode.GetRotationVectorFromRotationMatrix(theLocalGlobal);
    theRotations = qt3ds::render::SRotationHelper::ToNearestAngle(inNode.m_Rotation, theRotations,
                                                                inNode.m_RotationOrder);
    SetRotation(theRotations, inEditor);
    // Trackball rotation is relative to the previous mouse position.
    // Rotation manipulator rotation is relative to only the original mouse down position
    // so inIsMouseRelative is false for rotations that are relative to the original mouse down
    // position.
    if (inIsMouseRelative)
        m_MouseDownGlobalRotation = theFinalGlobal;
}

void STranslation::RotateSelectedInstanceAboutCameraDirectionVector(
    CPt inPreviousMouseCoords, CPt inMouseCoords, CUpdateableDocumentEditor &inEditor)
{
    SNode *theNode = GetSelectedNode();
    if (theNode == nullptr)
        return;
    SCamera *theCamera = m_Context.GetRenderer().GetCameraForNode(*theNode);
    if (theCamera == nullptr)
        return;

    QT3DSVec3 theDirection = m_MouseDownCamera.GetDirection();
    QT3DSF32 theYDistance = (QT3DSF32)inMouseCoords.y - (QT3DSF32)inPreviousMouseCoords.y;
    QT3DSQuat theYRotation(-1.0f * theYDistance * g_RotationScaleFactor, theDirection);

    ApplyRotationToSelectedInstance(theYRotation, *theNode, inEditor);
}

// This method never feels right to me.  It is difficult to apply it to a single axis (of course for
// that you can use the inspector palette).
void STranslation::RotateSelectedInstance(CPt inOriginalCoords, CPt inPreviousCoords,
                                          CPt inMouseCoords, CUpdateableDocumentEditor &inEditor,
                                          bool inLockToAxis)
{
    SNode *theNode = GetSelectedNode();
    if (theNode == nullptr)
        return;
    SCamera *theCamera = m_Context.GetRenderer().GetCameraForNode(*theNode);
    if (theCamera == nullptr)
        return;
    // We want to do a similar translation to what we did below but we need to calculate the
    // parent's global rotation without scale included.

    QT3DSF32 theXDistance = (QT3DSF32)inMouseCoords.x - (QT3DSF32)inPreviousCoords.x;
    QT3DSF32 theYDistance = (QT3DSF32)inMouseCoords.y - (QT3DSF32)inPreviousCoords.y;
    bool xIsZero = fabs(theXDistance) < .001f;
    bool yIsZero = fabs(theYDistance) < .001f;
    if (xIsZero && yIsZero)
        return;

    if (inLockToAxis) {
        QT3DSF32 originalDistX = (QT3DSF32)inMouseCoords.x - (QT3DSF32)inOriginalCoords.x;
        QT3DSF32 originalDistY = (QT3DSF32)inMouseCoords.y - (QT3DSF32)inOriginalCoords.y;
        if (inLockToAxis) {
            if (fabs(originalDistX) > fabs(originalDistY))
                theYDistance = 0;
            else
                theXDistance = 0;
        }
    }

    QT3DSVec3 theXAxis = m_MouseDownCamera.m_GlobalTransform.column0.getXYZ();
    QT3DSVec3 theYAxis = m_MouseDownCamera.m_GlobalTransform.column1.getXYZ();

    QT3DSVec3 theFinalAxis = theXDistance * theYAxis + theYDistance * theXAxis;
    QT3DSF32 theTotalDistance = theFinalAxis.normalize();
    QT3DSQuat theRotation(theTotalDistance * g_RotationScaleFactor / 2.0f, theFinalAxis);

    ApplyRotationToSelectedInstance(theRotation, *theNode, inEditor);
}

inline void NiceAdd(QT3DSF32 &ioValue, QT3DSF32 inIncrement)
{
    QT3DSF32 temp = ioValue + inIncrement;
    // Round to nearest .5
    QT3DSF32 sign = temp >= 0 ? 1.0f : -1.0f;
    QT3DSU32 largerValue = (QT3DSU32)(fabs(temp * 10.0f));

    QT3DSU32 leftover = largerValue % 10;
    // Round down to zero
    largerValue -= leftover;
    if (leftover < 2)
        leftover = 0;
    else if (leftover > 7) {
        leftover = 0;
        largerValue += 10;
    } else
        leftover = 5;
    largerValue += leftover;

    ioValue = sign * (QT3DSF32)largerValue / 10.0f;
}

static inline QT3DSVec3 GetAxis(QT3DSU32 inIndex, QT3DSMat33 &inMatrix)
{
    QT3DSVec3 retval(0, 0, 0);
    switch (inIndex) {
    case 0:
        retval = inMatrix.column0;
        break;
    case 1:
        retval = inMatrix.column1;
        break;
    case 2:
        retval = inMatrix.column2;
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }
    retval.normalize();
    return retval;
}

inline Option<QT3DSF32> GetScaleAlongAxis(const QT3DSVec3 &inAxis, const QT3DSVec3 &inObjToOriginal,
                                       const QT3DSVec3 &inObjToCurrent)
{
    QT3DSF32 lhs = inAxis.dot(inObjToCurrent);
    QT3DSF32 rhs = inAxis.dot(inObjToOriginal);
    if (fabs(rhs) > .001f)
        return lhs / rhs;
    return Empty();
}

// Make a nice rotation from the incoming rotation
static inline QT3DSF32 MakeNiceRotation(QT3DSF32 inAngle)
{
    TODEG(inAngle);
    inAngle *= 10.0f;
    QT3DSF32 sign = inAngle > 0.0f ? 1.0f : -1.0f;
    // Attempt to ensure angle is prtty clean
    QT3DSU32 clampedAngle = (QT3DSU32)(fabs(inAngle) + .5f);
    QT3DSU32 leftover = clampedAngle % 10;
    clampedAngle -= leftover;
    if (leftover <= 2)
        leftover = 0;
    else if (leftover <= 7)
        leftover = 5;
    else
        leftover = 10;
    clampedAngle += leftover;
    QT3DSF32 retval = (QT3DSF32)clampedAngle;
    retval = (retval * sign) / 10.0f;
    TORAD(retval);
    return retval;
}

static inline QT3DSF32 ShortestAngleDifference(QT3DSF32 inCumulative, QT3DSF32 inNewTotal)
{
    QT3DSF32 diff = qt3ds::render::SRotationHelper::ToMinimalAngle(inNewTotal - inCumulative);
    return inCumulative + diff;
}

Option<SDragPreparationResult>
STranslation::PrepareWidgetDrag(qt3ds::widgets::StudioWidgetComponentIds::Enum inComponentId,
                                qt3ds::widgets::StudioWidgetTypes::Enum inWidgetId,
                                qt3ds::render::RenderWidgetModes::Enum inWidgetMode, SNode &inNode,
                                CPt inOriginalCoords, CPt inPreviousMouseCoords, CPt inMouseCoords)
{
    SDragPreparationResult retval;
    retval.m_ComponentId = inComponentId;
    retval.m_WidgetType = inWidgetId;
    retval.m_WidgetMode = inWidgetMode;
    qt3ds::render::IQt3DSRenderer &theRenderer(m_Context.GetRenderer());
    retval.m_Renderer = &theRenderer;
    retval.m_Node = &inNode;
    retval.m_Layer = GetLayerForNode(inNode);
    retval.m_Camera = theRenderer.GetCameraForNode(inNode);
    QSize theUnsignedDimensions(m_Context.GetWindowDimensions());
    QT3DSVec2 theWindowDimensions((QT3DSF32)theUnsignedDimensions.width(),
                               (QT3DSF32)theUnsignedDimensions.height());
    if (retval.m_Camera == nullptr || retval.m_Layer == nullptr)
        return Empty();

    SCamera &theCamera(*retval.m_Camera);
    SLayer &theLayer(*retval.m_Layer);
    QT3DSVec2 theLayerOriginalCoords = m_Context.GetRenderer().GetLayerMouseCoords(
        *retval.m_Layer, QT3DSVec2((QT3DSF32)inOriginalCoords.x, (QT3DSF32)inOriginalCoords.y),
        theWindowDimensions, true);

    QT3DSVec2 theLayerMouseCoords = m_Context.GetRenderer().GetLayerMouseCoords(
        *retval.m_Layer, QT3DSVec2((QT3DSF32)inMouseCoords.x, (QT3DSF32)inMouseCoords.y),
        theWindowDimensions, true);

    QT3DSVec2 thePreviousLayerMouseCoords = m_Context.GetRenderer().GetLayerMouseCoords(
        *retval.m_Layer, QT3DSVec2((QT3DSF32)inPreviousMouseCoords.x, (QT3DSF32)inPreviousMouseCoords.y),
        theWindowDimensions, true);
    QT3DSMat44 theGlobalTransform(QT3DSMat44::createIdentity());
    if (inWidgetMode == qt3ds::render::RenderWidgetModes::Local) {
        theGlobalTransform = m_MouseDownNode.m_GlobalTransform;
    }
    retval.m_GlobalTransform = theGlobalTransform;
    QT3DSMat33 theNormalMat(theGlobalTransform.column0.getXYZ(), theGlobalTransform.column1.getXYZ(),
                         theGlobalTransform.column2.getXYZ());
    theNormalMat = theNormalMat.getInverse().getTranspose();
    retval.m_NormalMatrix = theNormalMat;
    qt3ds::render::NVRenderRectF theLayerRect(theRenderer.GetLayerRect(theLayer));
    SRay theOriginalRay =
        theCamera.Unproject(theLayerOriginalCoords, theLayerRect, theWindowDimensions);
    SRay theCurrentRay =
        theCamera.Unproject(theLayerMouseCoords, theLayerRect, theWindowDimensions);
    SRay thePreviousRay =
        theCamera.Unproject(thePreviousLayerMouseCoords, theLayerRect, theWindowDimensions);
    retval.m_OriginalRay = theOriginalRay;
    retval.m_CurrentRay = theCurrentRay;
    retval.m_PreviousRay = thePreviousRay;
    QT3DSVec3 theAxis;
    QT3DSVec3 thePlaneNormal;
    bool isPlane = false;
    QT3DSVec3 globalPos = inNode.GetGlobalPivot();
    QT3DSVec3 camGlobalPos = theCamera.GetGlobalPos();
    QT3DSVec3 theCamDirection;
    retval.m_GlobalPos = globalPos;
    retval.m_CameraGlobalPos = camGlobalPos;
    if (theCamera.m_Flags.IsOrthographic())
        theCamDirection = theCamera.GetDirection();
    else {
        theCamDirection = globalPos - camGlobalPos;
        // The normal will be normalized below.
    }
    theCamDirection.normalize();
    retval.m_CameraDirection = theCamDirection;
    retval.m_AxisIndex = 0;
    switch (inComponentId) {
    default:
        QT3DS_ASSERT(false);
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::XAxis:
        theAxis = QT3DSVec3(1, 0, 0);
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::YAxis:
        theAxis = QT3DSVec3(0, 1, 0);
        retval.m_AxisIndex = 1;
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::ZAxis:
        theAxis = QT3DSVec3(0, 0, -1);
        retval.m_AxisIndex = 2;
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::XPlane:
        thePlaneNormal = QT3DSVec3(1, 0, 0);
        isPlane = true;
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::YPlane:
        thePlaneNormal = QT3DSVec3(0, 1, 0);
        isPlane = true;
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::ZPlane:
        thePlaneNormal = QT3DSVec3(0, 0, -1);
        isPlane = true;
        break;
    case qt3ds::widgets::StudioWidgetComponentIds::CameraPlane: {
        isPlane = true;
        thePlaneNormal = theCamDirection;
    } break;
    }
    retval.m_IsPlane = isPlane;
    if (inWidgetId == qt3ds::widgets::StudioWidgetTypes::Rotation) {
        if (isPlane == false) {
            theAxis = theNormalMat.transform(theAxis);
            theAxis.normalize();
            thePlaneNormal = theAxis;
            retval.m_Plane = qt3ds::NVPlane(thePlaneNormal, -1.0f * thePlaneNormal.dot(globalPos));
        } else {
            if (inComponentId != qt3ds::widgets::StudioWidgetComponentIds::CameraPlane) {
                thePlaneNormal = theNormalMat.transform(thePlaneNormal);
            }
            thePlaneNormal.normalize();
            retval.m_Plane = qt3ds::NVPlane(thePlaneNormal, -1.0f * (thePlaneNormal.dot(globalPos)));
        }
    } else {
        if (isPlane == false) {
            theAxis = theNormalMat.transform(theAxis);
            theAxis.normalize();
            QT3DSVec3 theCameraToObj = globalPos - camGlobalPos;
            QT3DSVec3 theTemp = theAxis.cross(theOriginalRay.m_Direction);
            // When the axis is parallel to the camera, we can't drag meaningfully
            if (theTemp.magnitudeSquared() < .05f) {
                // Attempt to find a better axis by moving the object back towards the camera.
                QT3DSF32 theSign = theCameraToObj.dot(theCamDirection) > 0.0 ? -1.0f : 1.0f;
                QT3DSF32 theDistance = theCameraToObj.dot(theCamDirection);
                QT3DSVec3 thePoint = globalPos + (theDistance * theSign) * theAxis;
                // Check if we actually moved to right direction
                QT3DSVec3 theNewCameraToObj = thePoint - camGlobalPos;
                QT3DSF32 theNewDistance = theNewCameraToObj.dot(theCamDirection);
                if (theNewDistance > theDistance)
                    thePoint = globalPos - (theDistance * theSign) * theAxis;

                QT3DSVec3 theNewDir = thePoint - camGlobalPos;
                theNewDir.normalize();
                theTemp = theAxis.cross(theNewDir);
                // Attempt again to find a better cross
                if (theTemp.magnitudeSquared() < .05f)
                    return Empty();
            }
            thePlaneNormal = theTemp.cross(theAxis);
            thePlaneNormal.normalize();
            retval.m_Plane = qt3ds::NVPlane(thePlaneNormal, -1.0f * thePlaneNormal.dot(globalPos));
        } else {
            thePlaneNormal = theNormalMat.transform(thePlaneNormal);
            thePlaneNormal.normalize();
            retval.m_Plane = qt3ds::NVPlane(thePlaneNormal, -1.0f * (thePlaneNormal.dot(globalPos)));
        }
    }
    retval.m_Axis = theAxis;
    retval.m_OriginalPlaneCoords = theOriginalRay.Intersect(retval.m_Plane);
    retval.m_CurrentPlaneCoords = theCurrentRay.Intersect(retval.m_Plane);
    retval.m_PreviousPlaneCoords = thePreviousRay.Intersect(retval.m_Plane);
    return retval;
}

void STranslation::PerformWidgetDrag(int inWidgetSubComponent, CPt inOriginalCoords,
                                     CPt inPreviousMouseCoords, CPt inMouseCoords,
                                     CUpdateableDocumentEditor &inEditor)
{
    if (inWidgetSubComponent == 0 || m_LastRenderedWidget == nullptr) {
        QT3DS_ASSERT(false);
        return;
    }
    Option<SDragPreparationResult> thePrepResult(PrepareWidgetDrag(
        static_cast<qt3ds::widgets::StudioWidgetComponentIds::Enum>(inWidgetSubComponent),
        m_LastRenderedWidget->GetWidgetType(), m_LastRenderedWidget->GetRenderWidgetMode(),
        m_LastRenderedWidget->GetNode(), inOriginalCoords, inPreviousMouseCoords, inMouseCoords));
    if (!thePrepResult.hasValue())
        return;

    Option<QT3DSVec3> theOriginalPlaneCoords(thePrepResult->m_OriginalPlaneCoords);
    Option<QT3DSVec3> theCurrentPlaneCoords(thePrepResult->m_CurrentPlaneCoords);
    QT3DSVec3 globalPos(thePrepResult->m_GlobalPos);
    bool isPlane(thePrepResult->m_IsPlane);
    QT3DSVec3 theAxis(thePrepResult->m_Axis);
    QT3DSU32 axisIndex(thePrepResult->m_AxisIndex);
    SNode *theNode(thePrepResult->m_Node);
    SRay theCurrentRay(thePrepResult->m_CurrentRay);
    SRay theOriginalRay(thePrepResult->m_OriginalRay);
    QT3DSVec3 thePlaneNormal(thePrepResult->m_Plane.n);
    QT3DSVec3 theCamDirection(thePrepResult->m_CameraDirection);
    QT3DSVec3 camGlobalPos(thePrepResult->m_CameraGlobalPos);

    switch (m_LastRenderedWidget->GetWidgetType()) {
    default:
        QT3DS_ASSERT(false);
        return;
    case qt3ds::widgets::StudioWidgetTypes::Scale: {
        if (theOriginalPlaneCoords.hasValue() && theCurrentPlaneCoords.hasValue()) {
            QT3DSVec3 objToOriginal = globalPos - *theOriginalPlaneCoords;
            QT3DSVec3 objToCurrent = globalPos - *theCurrentPlaneCoords;
            QT3DSVec3 theScaleMultiplier(1, 1, 1);
            if (!isPlane) {
                // Ensure that we only have a scale vector in the direction of the axis.
                objToOriginal = theAxis * (theAxis.dot(objToOriginal));
                objToCurrent = theAxis * (theAxis.dot(objToCurrent));
                QT3DSF32 objToOriginalDot = theAxis.dot(objToOriginal);
                if (fabs(objToOriginalDot) > .001f)
                    theScaleMultiplier[axisIndex] =
                        theAxis.dot(objToCurrent) / theAxis.dot(objToOriginal);
                else
                    theScaleMultiplier[axisIndex] = 0.0f;
            }

            QT3DSMat33 theNodeAxisMatrix(theNode->m_GlobalTransform.column0.getXYZ(),
                                      theNode->m_GlobalTransform.column1.getXYZ(),
                                      theNode->m_GlobalTransform.column2.getXYZ());
            theNodeAxisMatrix = theNodeAxisMatrix.getInverse().getTranspose();
            QT3DSVec3 &theLocalXAxis(theNodeAxisMatrix.column0);
            QT3DSVec3 &theLocalYAxis(theNodeAxisMatrix.column1);
            QT3DSVec3 &theLocalZAxis(theNodeAxisMatrix.column2);
            theLocalXAxis.normalize();
            theLocalYAxis.normalize();
            theLocalZAxis.normalize();

            Option<QT3DSF32> theXScale = GetScaleAlongAxis(theLocalXAxis, objToOriginal, objToCurrent);
            Option<QT3DSF32> theYScale = GetScaleAlongAxis(theLocalYAxis, objToOriginal, objToCurrent);
            Option<QT3DSF32> theZScale = GetScaleAlongAxis(theLocalZAxis, objToOriginal, objToCurrent);
            QT3DSVec3 theScale = m_MouseDownNode.m_Scale;
            if (theXScale.isEmpty() && theYScale.isEmpty() && theZScale.isEmpty()) {
                theScale = QT3DSVec3(0, 0, 0);
            } else {
                if (theXScale.hasValue())
                    theScale.x *= *theXScale;
                if (theYScale.hasValue())
                    theScale.y *= *theYScale;
                if (theZScale.hasValue())
                    theScale.z *= *theZScale;
            }
            m_LastRenderedWidget->SetAxisScale(theScaleMultiplier);
            SetScale(theScale, inEditor);
        }
    } break;
    case qt3ds::widgets::StudioWidgetTypes::Rotation: {
        QT3DSF32 theIntersectionCosine = theOriginalRay.m_Direction.dot(thePlaneNormal);
        QT3DSVec3 objToPrevious;
        QT3DSVec3 objToCurrent;
        /*
        long theModifiers = CHotKeys::GetCurrentKeyModifiers();
        if ( theModifiers & CHotKeys::MODIFIER_SHIFT )
        {
            DebugBreak();
        }
        */
        if (!theOriginalPlaneCoords.hasValue() || !theCurrentPlaneCoords.hasValue())
                return;
        if (fabs(theIntersectionCosine) > .08f) {
            objToPrevious = globalPos - *theOriginalPlaneCoords;
            objToCurrent = globalPos - *theCurrentPlaneCoords;
            objToPrevious.normalize();
            QT3DSF32 lineLen = objToCurrent.normalize();
            QT3DSF32 cosAngle = objToPrevious.dot(objToCurrent);
            QT3DSVec3 theCrossProd = objToPrevious.cross(objToCurrent);
            QT3DSF32 theCrossPlaneDot = theCrossProd.dot(thePlaneNormal);
            QT3DSF32 angleSign = theCrossPlaneDot >= 0.0f ? 1.0f : -1.0f;
            QT3DSF32 angleRad = acos(cosAngle) * angleSign;
            angleRad = MakeNiceRotation(angleRad);
            QT3DSQuat theRotation(angleRad, thePlaneNormal);
            m_CumulativeRotation = ShortestAngleDifference(m_CumulativeRotation, angleRad);
            m_LastRenderedWidget->SetRotationEdges(-1.0f * objToPrevious, thePlaneNormal,
                                                   m_CumulativeRotation, lineLen);
            ApplyRotationToSelectedInstance(theRotation, *theNode, inEditor, false);
        }
        // In this case we are viewing the plane of rotation pretty much dead on, so we need to
        // assume the camera and the object are both in the plane of rotation. In this case we
        // *sort* of need to do trackball rotation but force it to one plane of rotation.
        else {
            // Setup a plane 600 units away from the camera and have the gadget run from there.

            // This keeps rotation consistent.
            QT3DSVec3 thePlaneSpot = theCamDirection * -600.0f + camGlobalPos;
            qt3ds::NVPlane theCameraPlaneAtObject(theCamDirection,
                                               -1.0f * (theCamDirection.dot(thePlaneSpot)));
            theCurrentPlaneCoords = theCurrentRay.Intersect(theCameraPlaneAtObject);
            theOriginalPlaneCoords = theOriginalRay.Intersect(theCameraPlaneAtObject);
            QT3DSVec3 theChangeVector = *theOriginalPlaneCoords - *theCurrentPlaneCoords;
            // Remove any component of the change vector that doesn't lie in the plane.
            theChangeVector =
                theChangeVector - theChangeVector.dot(thePlaneNormal) * thePlaneNormal;
            QT3DSF32 theDistance = theChangeVector.normalize();
            // We want about 90* per 200 units in imaginary 600-units-from-camera space.
            QT3DSF32 theScaleFactor = 1.0f / 200.0f;
            if (thePrepResult->m_Camera->m_Flags.IsOrthographic())
                theScaleFactor = 1.0f / 100.0f;

            QT3DSF32 theDeg = 90.0f * theDistance * theScaleFactor;
            // Check the sign of the angle.
            QT3DSVec3 theCurrentIsectDir = camGlobalPos - *theCurrentPlaneCoords;
            QT3DSVec3 thePreviousIsectDir = camGlobalPos - *theOriginalPlaneCoords;
            QT3DSVec3 theCrossProd = theCurrentIsectDir.cross(thePreviousIsectDir);
            QT3DSF32 theAngleSign = theCrossProd.dot(thePlaneNormal) > 0.0f ? 1.0f : -1.0f;
            theDeg *= theAngleSign;
            QT3DSF32 theRad(theDeg);
            TORAD(theRad);
            theRad = MakeNiceRotation(theRad);
            QT3DSQuat theRotation(theRad, thePlaneNormal);
            ApplyRotationToSelectedInstance(theRotation, *theNode, inEditor, false);
        }
    } break;
    case qt3ds::widgets::StudioWidgetTypes::Translation: {
        if (theOriginalPlaneCoords.hasValue() && theCurrentPlaneCoords.hasValue()) {
            QT3DSVec3 theDiff = *theCurrentPlaneCoords - *theOriginalPlaneCoords;
            if (isPlane) {
                ApplyPositionalChange(theDiff, *theNode, inEditor);
            } else {
                QT3DSVec3 theMovement = theAxis * theAxis.dot(theDiff);
                ApplyPositionalChange(theMovement, *theNode, inEditor);
            }
        }
    } break;
    }
}

static float RoundToNearest(float inValue, float inMin, float inMax, float inRound)
{
    float half = (inMin + inMax) / 2.0f;
    inValue -= half;
    inValue = inRound * floor(inValue / inRound + .5f);
    inValue += half;
    inValue -= inMin;
    return inValue;
}

void STranslation::PerformGuideDrag(Qt3DSDMGuideHandle inGuide, CPt inPoint,
                                    CUpdateableDocumentEditor &inEditor)
{
    qt3dsdm::SGuideInfo theInfo = m_Doc.GetDocumentReader().GetGuideInfo(inGuide);
    CPt renderSpacePt(inPoint.x, (long)GetViewportDimensions().y - inPoint.y);
    switch (theInfo.m_Direction) {
    case qt3dsdm::GuideDirections::Horizontal:
        theInfo.m_Position = RoundToNearest((float)renderSpacePt.y, (float)m_InnerRect.m_Bottom,
                                            (float)m_InnerRect.m_Top, 10.0f);
        break;
    case qt3dsdm::GuideDirections::Vertical:
        theInfo.m_Position = RoundToNearest((float)renderSpacePt.x, (float)m_InnerRect.m_Left,
                                            (float)m_InnerRect.m_Right, 10.0f);
        break;
    default:
        QT3DS_ASSERT(FALSE);
        break;
        break;
    }
    inEditor.EnsureEditor(L"Drag Guide", __FILE__, __LINE__).UpdateGuide(inGuide, theInfo);
    inEditor.FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle());
}

void STranslation::CheckGuideInPresentationRect(Qt3DSDMGuideHandle inGuide,
                                                CUpdateableDocumentEditor &inEditor)
{
    qt3dsdm::SGuideInfo theInfo = m_Doc.GetDocumentReader().GetGuideInfo(inGuide);
    bool inPresentation = false;
    QT3DSF32 presHeight = (QT3DSF32)m_InnerRect.m_Top - (QT3DSF32)m_InnerRect.m_Bottom;
    QT3DSF32 presWidth = (QT3DSF32)m_InnerRect.m_Right - (QT3DSF32)m_InnerRect.m_Left;
    switch (theInfo.m_Direction) {
    case qt3dsdm::GuideDirections::Horizontal:
        inPresentation = 0.0f <= theInfo.m_Position && presHeight >= theInfo.m_Position;
        break;
    case qt3dsdm::GuideDirections::Vertical:
        inPresentation = 0.0f <= theInfo.m_Position && presWidth >= theInfo.m_Position;
        break;
    }
    if (!inPresentation)
        inEditor.EnsureEditor(L"Delete Guide", __FILE__, __LINE__).DeleteGuide(inGuide);
}

namespace {

QT3DSF32 degToRad(const QT3DSF32 a)
{
    return (QT3DSF32)0.01745329251994329547 * a;
}

/**
\brief Converts radians to degrees.
*/
QT3DSF32 radToDeg(const QT3DSF32 a)
{
    return (QT3DSF32)57.29577951308232286465 * a;
}
}

void STranslation::PerformPathDrag(qt3ds::studio::SPathPick &inPathPick, CPt inOriginalCoords,
                                   CPt inPreviousMouseCoords, CPt inMouseCoords,
                                   CUpdateableDocumentEditor &inEditor)
{
    Option<SDragPreparationResult> thePrepResult(PrepareWidgetDrag(
        qt3ds::widgets::StudioWidgetComponentIds::ZPlane,
        qt3ds::widgets::StudioWidgetTypes::Translation, qt3ds::render::RenderWidgetModes::Local,
        m_PathWidget->GetNode(), inOriginalCoords, inPreviousMouseCoords, inMouseCoords));
    if (!thePrepResult.hasValue())
        return;

    Option<QT3DSVec3> theOriginalPlaneCoords(thePrepResult->m_OriginalPlaneCoords);
    Option<QT3DSVec3> theCurrentPlaneCoords(thePrepResult->m_CurrentPlaneCoords);
    Option<QT3DSVec3> thePreviousPlaneCoords(thePrepResult->m_PreviousPlaneCoords);
    if (theOriginalPlaneCoords.hasValue() && theCurrentPlaneCoords.hasValue()) {
        QT3DSVec3 theGlobalDiff = *theCurrentPlaneCoords - *theOriginalPlaneCoords;
        QT3DSMat44 theGlobalInverse = thePrepResult->m_GlobalTransform.getInverse();
        QT3DSVec3 theCurrentPos = theGlobalInverse.transform(*theCurrentPlaneCoords);
        QT3DSVec3 theOldPos = theGlobalInverse.transform(*theOriginalPlaneCoords);
        QT3DSVec3 theDiff = theCurrentPos - theOldPos;
        // Now find the anchor point; nontrivial.
        SPathTranslator *theTranslator =
            reinterpret_cast<SPathTranslator *>(thePrepResult->m_Node->m_UserData.m_UserData);
        qt3dsdm::Qt3DSDMInstanceHandle thePathHandle = theTranslator->GetInstanceHandle();
        qt3dsdm::Qt3DSDMInstanceHandle theAnchorHandle = GetAnchorPoint(inPathPick);

        if (theAnchorHandle.Valid()) {
            qt3dsdm::Qt3DSDMPropertyHandle thePosProperty =
                m_ObjectDefinitions.m_PathAnchorPoint.m_Position.m_Property;
            qt3dsdm::Qt3DSDMPropertyHandle theAngleProperty =
                m_ObjectDefinitions.m_PathAnchorPoint.m_IncomingAngle.m_Property;
            qt3dsdm::Qt3DSDMPropertyHandle theIncomingDistanceProperty =
                m_ObjectDefinitions.m_PathAnchorPoint.m_IncomingDistance.m_Property;
            qt3dsdm::Qt3DSDMPropertyHandle theOutgoingDistanceProperty =
                m_ObjectDefinitions.m_PathAnchorPoint.m_OutgoingDistance.m_Property;

            IDocumentReader &theReader(m_Doc.GetDocumentReader());
            SFloat2 anchorPos =
                *theReader.GetTypedInstancePropertyValue<SFloat2>(theAnchorHandle, thePosProperty);
            QT3DSVec2 anchorPosVec = QT3DSVec2(anchorPos[0], anchorPos[1]);
            if (m_LastPathDragValue.hasValue() == false) {
                SPathAnchorDragInitialValue initialValue;
                initialValue.m_Position = anchorPosVec;
                initialValue.m_IncomingAngle = theReader.GetTypedInstancePropertyValue<float>(
                    theAnchorHandle, theAngleProperty);
                initialValue.m_IncomingDistance = theReader.GetTypedInstancePropertyValue<float>(
                    theAnchorHandle, theIncomingDistanceProperty);
                initialValue.m_OutgoingDistance = theReader.GetTypedInstancePropertyValue<float>(
                    theAnchorHandle, theOutgoingDistanceProperty);
                m_LastPathDragValue = initialValue;
            }
            SPathAnchorDragInitialValue &lastValue(*m_LastPathDragValue);
            QT3DSVec2 theCurrentValue;
            switch (inPathPick.m_Property) {
            case SPathPick::Anchor:
                theCurrentValue = lastValue.m_Position;
                break;
            case SPathPick::IncomingControl:
                theCurrentValue = qt3ds::render::IPathManagerCore::GetControlPointFromAngleDistance(
                    lastValue.m_Position, lastValue.m_IncomingAngle, lastValue.m_IncomingDistance);
                break;
            case SPathPick::OutgoingControl:
                theCurrentValue = qt3ds::render::IPathManagerCore::GetControlPointFromAngleDistance(
                    lastValue.m_Position, lastValue.m_IncomingAngle + 180.0f,
                    lastValue.m_OutgoingDistance);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
            theCurrentValue[0] += theDiff.x;
            theCurrentValue[1] += theDiff.y;
            Q3DStudio::IDocumentEditor &theEditor =
                inEditor.EnsureEditor(L"Anchor Point Drag", __FILE__, __LINE__);
            switch (inPathPick.m_Property) {
            case SPathPick::Anchor:
                theEditor.SetInstancePropertyValue(theAnchorHandle, thePosProperty,
                                                   SFloat2(theCurrentValue.x, theCurrentValue.y));
                break;
            case SPathPick::IncomingControl: {
                QT3DSVec2 angleDistance =
                    qt3ds::render::IPathManagerCore::GetAngleDistanceFromControlPoint(
                        anchorPosVec, theCurrentValue);
                float angleDiff = angleDistance.x - lastValue.m_IncomingAngle;
                float minimalDiff =
                    radToDeg(qt3ds::render::SRotationHelper::ToMinimalAngle(degToRad(angleDiff)));
                float newAngle = lastValue.m_IncomingAngle + minimalDiff;
                theEditor.SetInstancePropertyValue(theAnchorHandle, theAngleProperty, newAngle);
                theEditor.SetInstancePropertyValue(theAnchorHandle, theIncomingDistanceProperty,
                                                   angleDistance.y);
            } break;
            case SPathPick::OutgoingControl: {
                QT3DSVec2 angleDistance =
                    qt3ds::render::IPathManagerCore::GetAngleDistanceFromControlPoint(
                        anchorPosVec, theCurrentValue);
                angleDistance.x += 180.0f;
                float angleDiff = angleDistance.x - lastValue.m_IncomingAngle;
                float minimalDiff =
                    radToDeg(qt3ds::render::SRotationHelper::ToMinimalAngle(degToRad(angleDiff)));
                float newAngle = lastValue.m_IncomingAngle + minimalDiff;
                theEditor.SetInstancePropertyValue(theAnchorHandle, theAngleProperty, newAngle);
                theEditor.SetInstancePropertyValue(theAnchorHandle, theOutgoingDistanceProperty,
                                                   angleDistance.y);
            } break;
            }

            inEditor.FireImmediateRefresh(m_AssetGraph.GetParent(theAnchorHandle));
        }
    }
}

SNode *STranslation::GetEditCameraLayer()
{
    if (m_EditCameraLayerTranslator)
        return static_cast<SNode *>(&m_EditCameraLayerTranslator->GetGraphObject());
    return nullptr;
}

PickTargetAreas::Enum STranslation::GetPickArea(CPt inPoint)
{
    qt3ds::render::NVRenderRectF displayViewport = m_Context.GetDisplayViewport();
    QT3DSVec2 thePickPoint((QT3DSF32)inPoint.x,
                        m_Context.GetWindowDimensions().height() - (QT3DSF32)inPoint.y);
    QT3DSF32 left = displayViewport.m_X;
    QT3DSF32 right = displayViewport.m_X + displayViewport.m_Width;
    QT3DSF32 top = displayViewport.m_Y + displayViewport.m_Height;
    QT3DSF32 bottom = displayViewport.m_Y;
    if (thePickPoint.x < left || thePickPoint.x > right || thePickPoint.y < bottom
        || thePickPoint.y > top)
        return PickTargetAreas::Matte;
    return PickTargetAreas::Presentation;
}
