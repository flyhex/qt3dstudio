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
#include "UICDMPrefix.h"
#include "UICDMComposerTypeDefinitions.h"
#include <memory>
#include "UICDMDataCore.h"
#include "SimpleDataCore.h"
#include "DataCoreProducer.h"
#include "UICDMSlideCore.h"

#include <QtGlobal>

using namespace qt3dsdm;
using std::make_shared;
using std::shared_ptr;

#ifdef _WIN32
#pragma warning(disable : 4100)
#endif

// SComposerTypePropertyDefinition constructors

namespace {

template <typename TDataType>
struct DataConstructor
{
    template <typename TInputType>
    TDataType Construct(const TInputType &inInput)
    {
        return TDataType(inInput);
    }
};

template <>
struct DataConstructor<SLong4>
{
    SLong4 Construct(int inValue)
    {
        unsigned long theValue((unsigned long)inValue);
        return SLong4(theValue, theValue, theValue, theValue);
    }
};

template <>
struct DataConstructor<TDataStrPtr>
{
    TDataStrPtr Construct(const wchar_t *inData)
    {
        if (IsTrivial(inData))
            return make_shared<CDataStr>();
        return make_shared<CDataStr>(inData);
    }
};
template <>
struct DataConstructor<SStringOrInt>
{
    SStringOrInt Construct(const wchar_t *inData)
    {
        if (IsTrivial(inData))
            return make_shared<CDataStr>();
        return make_shared<CDataStr>(inData);
    }
};
template <>
struct DataConstructor<SObjectRefType>
{
    SObjectRefType Construct(const wchar_t *) { return SObjectRefType(); }
};
}

#define UIC_WCHAR_T_Typed L"Typed"
#define UIC_WCHAR_T_Guided L"Guided"
#define UIC_WCHAR_T_Named L"Named"
#define UIC_WCHAR_T_SlideOwner L"SlideOwner"
#define UIC_WCHAR_T_Slide L"Slide"
#define UIC_WCHAR_T_Action L"Action"
#define UIC_WCHAR_T_Asset L"Asset"
#define UIC_WCHAR_T_Scene L"Scene"
#define UIC_WCHAR_T_Image L"Image"
#define UIC_WCHAR_T_Material L"Material"
#define UIC_WCHAR_T_Behavior L"Behavior"
#define UIC_WCHAR_T_Node L"Node"
#define UIC_WCHAR_T_Layer L"Layer"
#define UIC_WCHAR_T_Group L"Group"
#define UIC_WCHAR_T_Model L"Model"
#define UIC_WCHAR_T_Light L"Light"
#define UIC_WCHAR_T_Camera L"Camera"
#define UIC_WCHAR_T_Component L"Component"
#define UIC_WCHAR_T_Text L"Text"
#define UIC_WCHAR_T_Effect L"Effect"
#define UIC_WCHAR_T_RenderPlugin L"RenderPlugin"
#define UIC_WCHAR_T_MaterialBase L"MaterialBase"
#define UIC_WCHAR_T_CustomMaterial L"CustomMaterial"
#define UIC_WCHAR_T_ReferencedMaterial L"ReferencedMaterial"
#define UIC_WCHAR_T_Alias L"Alias"
#define UIC_WCHAR_T_Lightmaps L"Lightmaps"

#define UIC_WCHAR_T_type L"type"
#define UIC_WCHAR_T_id L"id"
#define UIC_WCHAR_T_name L"name"
#define UIC_WCHAR_T_componentid L"componentid"
#define UIC_WCHAR_T_playmode L"playmode"
#define UIC_WCHAR_T_playthroughto L"playthroughto"
#define UIC_WCHAR_T_initialplaystate L"initialplaystate"
#define UIC_WCHAR_T_actioneyeball L"actioneyeball"
#define UIC_WCHAR_T_sourcepath L"sourcepath"
#define UIC_WCHAR_T_importid L"importid"
#define UIC_WCHAR_T_starttime L"starttime"
#define UIC_WCHAR_T_endtime L"endtime"
#define UIC_WCHAR_T_eyeball L"eyeball"
#define UIC_WCHAR_T_shy L"shy"
#define UIC_WCHAR_T_locked L"locked"
#define UIC_WCHAR_T_timebarcolor L"timebarcolor"
#define UIC_WCHAR_T_timebartext L"timebartext"
#define UIC_WCHAR_T_bgcolorenable L"bgcolorenable"
#define UIC_WCHAR_T_background L"background"
#define UIC_WCHAR_T_backgroundcolor L"backgroundcolor"
#define UIC_WCHAR_T_blendtype L"blendtype"
#define UIC_WCHAR_T_scaleu L"scaleu"
#define UIC_WCHAR_T_scalev L"scalev"
#define UIC_WCHAR_T_mappingmode L"mappingmode"
#define UIC_WCHAR_T_tilingmodehorz L"tilingmodehorz"
#define UIC_WCHAR_T_tilingmodevert L"tilingmodevert"
#define UIC_WCHAR_T_rotationuv L"rotationuv"
#define UIC_WCHAR_T_positionu L"positionu"
#define UIC_WCHAR_T_positionv L"positionv"
#define UIC_WCHAR_T_pivotu L"pivotu"
#define UIC_WCHAR_T_pivotv L"pivotv"
#define UIC_WCHAR_T_subpresentation L"subpresentation"
#define UIC_WCHAR_T_iblprobe L"iblprobe"
#define UIC_WCHAR_T_shaderlighting L"shaderlighting"
#define UIC_WCHAR_T_blendmode L"blendmode"
#define UIC_WCHAR_T_diffuse L"diffuse"
#define UIC_WCHAR_T_diffusemap L"diffusemap"
#define UIC_WCHAR_T_diffusemap2 L"diffusemap2"
#define UIC_WCHAR_T_diffusemap3 L"diffusemap3"
#define UIC_WCHAR_T_specularreflection L"specularreflection"
#define UIC_WCHAR_T_specularmap L"specularmap"
#define UIC_WCHAR_T_specularmodel L"specularmodel"
#define UIC_WCHAR_T_speculartint L"speculartint"
#define UIC_WCHAR_T_ior L"ior"
#define UIC_WCHAR_T_specularamount L"specularamount"
#define UIC_WCHAR_T_specularroughness L"specularroughness"
#define UIC_WCHAR_T_opacitymap L"opacitymap"
#define UIC_WCHAR_T_emissivepower L"emissivepower"
#define UIC_WCHAR_T_emissivecolor L"emissivecolor"
#define UIC_WCHAR_T_emissivemap L"emissivemap"
#define UIC_WCHAR_T_emissivemap2 L"emissivemap2"
#define UIC_WCHAR_T_normalmap L"normalmap"
#define UIC_WCHAR_T_normalstrength L"normalstrength"
#define UIC_WCHAR_T_position L"position"
#define UIC_WCHAR_T_rotation L"rotation"
#define UIC_WCHAR_T_scale L"scale"
#define UIC_WCHAR_T_pivot L"pivot"
#define UIC_WCHAR_T_opacity L"opacity"
#define UIC_WCHAR_T_rotationorder L"rotationorder"
#define UIC_WCHAR_T_orientation L"orientation"
#define UIC_WCHAR_T_progressiveaa L"progressiveaa"
#define UIC_WCHAR_T_multisampleaa L"multisampleaa"
#define UIC_WCHAR_T_disabledepthtest L"disabledepthtest"
#define UIC_WCHAR_T_disabledepthprepass L"disabledepthprepass"
#define UIC_WCHAR_T_layerwidth L"layerwidth"
#define UIC_WCHAR_T_layerheight L"layerheight"
#define UIC_WCHAR_T_lighttype L"lighttype"
#define UIC_WCHAR_T_lightdiffuse L"lightdiffuse"
#define UIC_WCHAR_T_lightspecular L"lightspecular"
#define UIC_WCHAR_T_lightambient L"lightambient"
#define UIC_WCHAR_T_brightness L"brightness"
#define UIC_WCHAR_T_linearfade L"linearfade"
#define UIC_WCHAR_T_expfade L"expfade"
#define UIC_WCHAR_T_areawidth L"areawidth"
#define UIC_WCHAR_T_areaheight L"areaheight"
#define UIC_WCHAR_T_castshadow L"castshadow"
#define UIC_WCHAR_T_shdwbias L"shdwbias"
#define UIC_WCHAR_T_shdwfactor L"shdwfactor"
#define UIC_WCHAR_T_shdwmapres L"shdwmapres"
#define UIC_WCHAR_T_shdwmapfar L"shdwmapfar"
#define UIC_WCHAR_T_shdwmapfov L"shdwmapfov"
#define UIC_WCHAR_T_shdwfilter L"shdwfilter"
#define UIC_WCHAR_T_orthographic L"orthographic"
#define UIC_WCHAR_T_fov L"fov"
#define UIC_WCHAR_T_clipnear L"clipnear"
#define UIC_WCHAR_T_clipfar L"clipfar"
#define UIC_WCHAR_T_lookatlock L"lookatlock"
#define UIC_WCHAR_T_lookatpoint L"lookatpoint"
#define UIC_WCHAR_T_textstring L"textstring"
#define UIC_WCHAR_T_textcolor L"textcolor"
#define UIC_WCHAR_T_font L"font"
#define UIC_WCHAR_T_size L"size"
#define UIC_WCHAR_T_horzalign L"horzalign"
#define UIC_WCHAR_T_vertalign L"vertalign"
#define UIC_WCHAR_T_leading L"leading"
#define UIC_WCHAR_T_tracking L"tracking"
#define UIC_WCHAR_T_enableacceleratedfont L"enableacceleratedfont"
#define UIC_WCHAR_T_importfile L"importfile"
#define UIC_WCHAR_T_fileid L"fileid"
#define UIC_WCHAR_T_size L"size"
#define UIC_WCHAR_T_location L"location"
#define UIC_WCHAR_T_boneid L"boneid"
#define UIC_WCHAR_T_poseroot L"poseroot"
#define UIC_WCHAR_T_ignoresparent L"ignoresparent"
#define UIC_WCHAR_T_tessellation L"tessellation"
#define UIC_WCHAR_T_edgetess L"edgetess"
#define UIC_WCHAR_T_innertess L"innertess"
#define UIC_WCHAR_T_scalemode L"scalemode"
#define UIC_WCHAR_T_scaleanchor L"scaleanchor"
#define UIC_WCHAR_T_horzfields L"horzfields"
#define UIC_WCHAR_T_left L"left"
#define UIC_WCHAR_T_leftunits L"leftunits"
#define UIC_WCHAR_T_width L"width"
#define UIC_WCHAR_T_widthunits L"widthunits"
#define UIC_WCHAR_T_right L"right"
#define UIC_WCHAR_T_rightunits L"rightunits"
#define UIC_WCHAR_T_vertfields L"vertfields"
#define UIC_WCHAR_T_top L"top"
#define UIC_WCHAR_T_topunits L"topunits"
#define UIC_WCHAR_T_height L"height"
#define UIC_WCHAR_T_heightunits L"heightunits"
#define UIC_WCHAR_T_bottom L"bottom"
#define UIC_WCHAR_T_bottomunits L"bottomunits"
#define UIC_WCHAR_T_aostrength L"aostrength"
#define UIC_WCHAR_T_aodistance L"aodistance"
#define UIC_WCHAR_T_aosoftness L"aosoftness"
#define UIC_WCHAR_T_aobias L"aobias"
#define UIC_WCHAR_T_aosamplerate L"aosamplerate"
#define UIC_WCHAR_T_aodither L"aodither"
#define UIC_WCHAR_T_shadowstrength L"shadowstrength"
#define UIC_WCHAR_T_shadowdist L"shadowdist"
#define UIC_WCHAR_T_shadowsoftness L"shadowsoftness"
#define UIC_WCHAR_T_shadowbias L"shadowbias"
#define UIC_WCHAR_T_source L"source"
#define UIC_WCHAR_T_referencedmaterial L"referencedmaterial"
#define UIC_WCHAR_T_lightprobe L"lightprobe"
#define UIC_WCHAR_T_probebright L"probebright"
#define UIC_WCHAR_T_fastibl L"fastibl"
#define UIC_WCHAR_T_probehorizon L"probehorizon"
#define UIC_WCHAR_T_probefov L"probefov"
#define UIC_WCHAR_T_lightprobe2 L"lightprobe2"
#define UIC_WCHAR_T_probe2fade L"probe2fade"
#define UIC_WCHAR_T_probe2window L"probe2window"
#define UIC_WCHAR_T_probe2pos L"probe2pos"
#define UIC_WCHAR_T_bumpmap L"bumpmap"
#define UIC_WCHAR_T_bumpamount L"bumpamount"
#define UIC_WCHAR_T_normalmap L"normalmap"
#define UIC_WCHAR_T_displacementmap L"displacementmap"
#define UIC_WCHAR_T_displaceamount L"displaceamount"
#define UIC_WCHAR_T_translucencymap L"translucencymap"
#define UIC_WCHAR_T_translucentfalloff L"translucentfalloff"
#define UIC_WCHAR_T_diffuselightwrap L"diffuselightwrap"
#define UIC_WCHAR_T_fresnelPower L"fresnelPower"
#define UIC_WCHAR_T_referencednode L"referencednode"
#define UIC_WCHAR_T_temporalaa L"temporalaa"
#define UIC_WCHAR_T_scope L"scope"
#define UIC_WCHAR_T_Path L"Path"
#define UIC_WCHAR_T_pathdata L"pathdata"
#define UIC_WCHAR_T_PathAnchorPoint L"PathAnchorPoint"
#define UIC_WCHAR_T_incomingangle L"incomingangle"
#define UIC_WCHAR_T_incomingdistance L"incomingdistance"
#define UIC_WCHAR_T_outgoingdistance L"outgoingdistance"
#define UIC_WCHAR_T_linearerror L"linearerror"
#define UIC_WCHAR_T_edgetessamount L"edgetessamount"
#define UIC_WCHAR_T_innertessamount L"innertessamount"
#define UIC_WCHAR_T_begincap L"begincap"
#define UIC_WCHAR_T_begincapoffset L"begincapoffset"
#define UIC_WCHAR_T_begincapopacity L"begincapopacity"
#define UIC_WCHAR_T_begincapwidth L"begincapwidth"
#define UIC_WCHAR_T_endcap L"endcap"
#define UIC_WCHAR_T_endcapoffset L"endcapoffset"
#define UIC_WCHAR_T_endcapopacity L"endcapopacity"
#define UIC_WCHAR_T_endcapwidth L"endcapwidth"
#define UIC_WCHAR_T_pathtype L"pathtype"
#define UIC_WCHAR_T_closed L"closed"
#define UIC_WCHAR_T_paintstyle L"paintstyle"
#define UIC_WCHAR_T_SubPath L"SubPath"
#define UIC_WCHAR_T_lightmapindirect L"lightmapindirect"
#define UIC_WCHAR_T_lightmapradiosity L"lightmapradiosity"
#define UIC_WCHAR_T_lightmapshadow L"lightmapshadow"

const wchar_t *ComposerObjectTypes::Convert(ComposerObjectTypes::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case name:                                                                                     \
        return UIC_WCHAR_T_##name;
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    }
    QT3DS_ASSERT(false);
    return L"Unknown";
}

ComposerObjectTypes::Enum ComposerObjectTypes::Convert(const wchar_t *inType)
{

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (AreEqual(UIC_WCHAR_T_##name, inType))                                                      \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    QT3DS_ASSERT(false);
    return ComposerObjectTypes::Unknown;
}

ComposerObjectTypes::Enum ComposerObjectTypes::Convert(const char8_t *inType)
{

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (AreEqual(#name, inType))                                                                   \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    QT3DS_ASSERT(false);
    return ComposerObjectTypes::Unknown;
}

const wchar_t *ComposerPropertyNames::Convert(ComposerPropertyNames::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    case name:                                                                                     \
        return UIC_WCHAR_T_##name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return L"Unknown";
}

ComposerPropertyNames::Enum ComposerPropertyNames::Convert(const wchar_t *inType)
{
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    if (AreEqual(inType, UIC_WCHAR_T_##name))                                                      \
        return name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    QT3DS_ASSERT(false);
    return ComposerPropertyNames::Unknown;
}

const char8_t *ComposerPropertyNames::ConvertNarrow(ComposerPropertyNames::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    case name:                                                                                     \
        return #name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return "Unknown";
}

ComposerPropertyNames::Enum ComposerPropertyNames::Convert(const char8_t *inType)
{
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    if (AreEqual(inType, #name))                                                                   \
        return name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    QT3DS_ASSERT(false);
    return ComposerPropertyNames::Unknown;
}

#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    , memberName(inCore, inInstance, DataConstructor<dtype>().Construct(defaultValue))

#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    , memberName(inCore, inInstance)

#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)                  \
    HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    SComposerTypePropertyDefinition<ComposerObjectTypes::name>::SComposerTypePropertyDefinition(   \
        IDataCore &inCore, CUICDMInstanceHandle inInstance)                                        \
        : reserved(false) propmacro                                                  \
    { \
        Q_UNUSED(inCore);\
        Q_UNUSED(inInstance);\
    }
ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE

void ComposerTypeDefinitionsHelper::SetInstanceAsCanonical(IMetaData &inMetaData,
                                                           CUICDMInstanceHandle inInstance,
                                                           ComposerObjectTypes::Enum inObjectType)
{
    inMetaData.SetInstanceAsCanonical(inInstance, ComposerObjectTypes::Convert(inObjectType));
}

void ComposerTypeDefinitionsHelper::SetInstancePropertyValue(IDataCore &inDataCore,
                                                             CUICDMInstanceHandle inInstance,
                                                             CUICDMPropertyHandle inProperty,
                                                             const wchar_t *inPropValue)
{
    inDataCore.SetInstancePropertyValue(inInstance, inProperty, make_shared<CDataStr>(inPropValue));
}

void ComposerTypeDefinitionsHelper::DeriveInstance(IDataCore &inDataCore,
                                                   CUICDMInstanceHandle inInstance,
                                                   CUICDMInstanceHandle inParent)
{
    inDataCore.DeriveInstance(inInstance, inParent);
}

// Container object for all of the object definitions
SComposerObjectDefinitions::SComposerObjectDefinitions(
    IDataCore &inCore,
    IMetaData &inMetaData /*, ISlideCore& inSlideCore, IPropertySystem& inPropertySystem*/)
    : m_DataCore(inCore)
    , m_MetaData(inMetaData)
    //, m_SlideCore( inSlideCore )
    //, m_PropertySystem( inPropertySystem )
    , m_Typed(inCore, inMetaData, inCore.CreateInstance())
    , m_Guided(inCore, inMetaData, inCore.CreateInstance())
    , m_Named(inCore, inMetaData, inCore.CreateInstance())
    , m_SlideOwner(inCore, inMetaData, inCore.CreateInstance())
    , m_Slide(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Named)
    , m_Action(inCore, inMetaData, inCore.CreateInstance(), m_Typed)
    , m_Asset(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Guided, m_Named)
    , m_Scene(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset, m_SlideOwner)
    , m_Image(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Lightmaps(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_MaterialBase(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_Material(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_MaterialBase)
    , m_CustomMaterial(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_MaterialBase)
    , m_ReferencedMaterial(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_MaterialBase)
    , m_Behavior(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Node(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Layer(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Model(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Group(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Light(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Camera(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Component(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node, m_SlideOwner)
    , m_Text(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Effect(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_RenderPlugin(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Alias(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Path(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_PathAnchorPoint(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_SubPath(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
{
}

bool SComposerObjectDefinitions::IsA(CUICDMInstanceHandle inInstance,
                                     ComposerObjectTypes::Enum inType)
{
    if (m_DataCore.IsInstance(inInstance) == false)
        return false;

    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case ComposerObjectTypes::name:                                                                \
        return m_DataCore.IsInstanceOrDerivedFrom(inInstance, m_##name.m_Instance);
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    }
    QT3DS_ASSERT(false);
    return false;
}

// Could easily return None, meaning we can't identify the object type.
// Checks the type of the first derivation parent, so this won't ever return
// SlideOwner, for instance.
ComposerObjectTypes::Enum SComposerObjectDefinitions::GetType(CUICDMInstanceHandle inInstance)
{
    if (m_DataCore.IsInstance(inInstance) == false)
        return ComposerObjectTypes::Unknown;

    CUICDMInstanceHandle theTargetInstance = inInstance;
    TInstanceHandleList theHandleList;
    m_DataCore.GetInstanceParents(inInstance, theHandleList);
    if (theHandleList.empty() == false)
        theTargetInstance = theHandleList[0];

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (m_##name.m_Instance == theTargetInstance)                                                  \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    return ComposerObjectTypes::Unknown;
}

CUICDMInstanceHandle
SComposerObjectDefinitions::GetInstanceForType(ComposerObjectTypes::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case ComposerObjectTypes::name:                                                                \
        return m_##name.m_Instance;
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    }
    QT3DS_ASSERT(false);
    return 0;
}
