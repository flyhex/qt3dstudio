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
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include <memory>
#include "Qt3DSDMDataCore.h"
#include "SimpleDataCore.h"
#include "DataCoreProducer.h"
#include "Qt3DSDMSlideCore.h"

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

#define QT3DS_PROPNAME_Typed "Typed"
#define QT3DS_PROPNAME_Guided "Guided"
#define QT3DS_PROPNAME_Named "Named"
#define QT3DS_PROPNAME_SlideOwner "SlideOwner"
#define QT3DS_PROPNAME_Slide "Slide"
#define QT3DS_PROPNAME_Action "Action"
#define QT3DS_PROPNAME_Asset "Asset"
#define QT3DS_PROPNAME_Scene "Scene"
#define QT3DS_PROPNAME_Image "Image"
#define QT3DS_PROPNAME_Material "Material"
#define QT3DS_PROPNAME_Behavior "Behavior"
#define QT3DS_PROPNAME_Node "Node"
#define QT3DS_PROPNAME_Layer "Layer"
#define QT3DS_PROPNAME_Group "Group"
#define QT3DS_PROPNAME_Model "Model"
#define QT3DS_PROPNAME_Light "Light"
#define QT3DS_PROPNAME_Camera "Camera"
#define QT3DS_PROPNAME_Component "Component"
#define QT3DS_PROPNAME_Text "Text"
#define QT3DS_PROPNAME_Effect "Effect"
#define QT3DS_PROPNAME_RenderPlugin "RenderPlugin"
#define QT3DS_PROPNAME_MaterialBase "MaterialBase"
#define QT3DS_PROPNAME_CustomMaterial "CustomMaterial"
#define QT3DS_PROPNAME_ReferencedMaterial "ReferencedMaterial"
#define QT3DS_PROPNAME_Alias "Alias"
#define QT3DS_PROPNAME_Lightmaps "Lightmaps"

#define QT3DS_PROPNAME_type "type"
#define QT3DS_PROPNAME_id "id"
#define QT3DS_PROPNAME_name "name"
#define QT3DS_PROPNAME_componentid "componentid"
#define QT3DS_PROPNAME_playmode "playmode"
#define QT3DS_PROPNAME_playthroughto "playthroughto"
#define QT3DS_PROPNAME_initialplaystate "initialplaystate"
#define QT3DS_PROPNAME_actioneyeball "actioneyeball"
#define QT3DS_PROPNAME_sourcepath "sourcepath"
#define QT3DS_PROPNAME_importid "importid"
#define QT3DS_PROPNAME_starttime "starttime"
#define QT3DS_PROPNAME_endtime "endtime"
#define QT3DS_PROPNAME_eyeball "eyeball"
#define QT3DS_PROPNAME_shy "shy"
#define QT3DS_PROPNAME_locked "locked"
#define QT3DS_PROPNAME_timebarcolor "timebarcolor"
#define QT3DS_PROPNAME_timebartext "timebartext"
#define QT3DS_PROPNAME_bgcolorenable "bgcolorenable"
#define QT3DS_PROPNAME_background "background"
#define QT3DS_PROPNAME_backgroundcolor "backgroundcolor"
#define QT3DS_PROPNAME_blendtype "blendtype"
#define QT3DS_PROPNAME_scaleu "scaleu"
#define QT3DS_PROPNAME_scalev "scalev"
#define QT3DS_PROPNAME_mappingmode "mappingmode"
#define QT3DS_PROPNAME_tilingmodehorz "tilingmodehorz"
#define QT3DS_PROPNAME_tilingmodevert "tilingmodevert"
#define QT3DS_PROPNAME_rotationuv "rotationuv"
#define QT3DS_PROPNAME_positionu "positionu"
#define QT3DS_PROPNAME_positionv "positionv"
#define QT3DS_PROPNAME_pivotu "pivotu"
#define QT3DS_PROPNAME_pivotv "pivotv"
#define QT3DS_PROPNAME_subpresentation "subpresentation"
#define QT3DS_PROPNAME_iblprobe "iblprobe"
#define QT3DS_PROPNAME_shaderlighting "shaderlighting"
#define QT3DS_PROPNAME_blendmode "blendmode"
#define QT3DS_PROPNAME_vertexcolors "vertexcolors"
#define QT3DS_PROPNAME_diffuse "diffuse"
#define QT3DS_PROPNAME_diffusemap "diffusemap"
#define QT3DS_PROPNAME_diffusemap2 "diffusemap2"
#define QT3DS_PROPNAME_diffusemap3 "diffusemap3"
#define QT3DS_PROPNAME_specularreflection "specularreflection"
#define QT3DS_PROPNAME_specularmap "specularmap"
#define QT3DS_PROPNAME_specularmodel "specularmodel"
#define QT3DS_PROPNAME_speculartint "speculartint"
#define QT3DS_PROPNAME_ior "ior"
#define QT3DS_PROPNAME_specularamount "specularamount"
#define QT3DS_PROPNAME_specularroughness "specularroughness"
#define QT3DS_PROPNAME_roughnessmap "roughnessmap"
#define QT3DS_PROPNAME_opacitymap "opacitymap"
#define QT3DS_PROPNAME_emissivepower "emissivepower"
#define QT3DS_PROPNAME_emissivecolor "emissivecolor"
#define QT3DS_PROPNAME_emissivemap "emissivemap"
#define QT3DS_PROPNAME_emissivemap2 "emissivemap2"
#define QT3DS_PROPNAME_normalmap "normalmap"
#define QT3DS_PROPNAME_normalstrength "normalstrength"
#define QT3DS_PROPNAME_position "position"
#define QT3DS_PROPNAME_rotation "rotation"
#define QT3DS_PROPNAME_scale "scale"
#define QT3DS_PROPNAME_pivot "pivot"
#define QT3DS_PROPNAME_opacity "opacity"
#define QT3DS_PROPNAME_rotationorder "rotationorder"
#define QT3DS_PROPNAME_orientation "orientation"
#define QT3DS_PROPNAME_progressiveaa "progressiveaa"
#define QT3DS_PROPNAME_multisampleaa "multisampleaa"
#define QT3DS_PROPNAME_disabledepthtest "disabledepthtest"
#define QT3DS_PROPNAME_disabledepthprepass "disabledepthprepass"
#define QT3DS_PROPNAME_layerwidth "layerwidth"
#define QT3DS_PROPNAME_layerheight "layerheight"
#define QT3DS_PROPNAME_lighttype "lighttype"
#define QT3DS_PROPNAME_lightdiffuse "lightdiffuse"
#define QT3DS_PROPNAME_lightspecular "lightspecular"
#define QT3DS_PROPNAME_lightambient "lightambient"
#define QT3DS_PROPNAME_brightness "brightness"
#define QT3DS_PROPNAME_linearfade "linearfade"
#define QT3DS_PROPNAME_expfade "expfade"
#define QT3DS_PROPNAME_areawidth "areawidth"
#define QT3DS_PROPNAME_areaheight "areaheight"
#define QT3DS_PROPNAME_castshadow "castshadow"
#define QT3DS_PROPNAME_shdwbias "shdwbias"
#define QT3DS_PROPNAME_shdwfactor "shdwfactor"
#define QT3DS_PROPNAME_shdwmapres "shdwmapres"
#define QT3DS_PROPNAME_shdwmapfar "shdwmapfar"
#define QT3DS_PROPNAME_shdwmapfov "shdwmapfov"
#define QT3DS_PROPNAME_shdwfilter "shdwfilter"
#define QT3DS_PROPNAME_orthographic "orthographic"
#define QT3DS_PROPNAME_fov "fov"
#define QT3DS_PROPNAME_fovhorizontal "fovhorizontal"
#define QT3DS_PROPNAME_clipnear "clipnear"
#define QT3DS_PROPNAME_clipfar "clipfar"
#define QT3DS_PROPNAME_lookatlock "lookatlock"
#define QT3DS_PROPNAME_lookatpoint "lookatpoint"
#define QT3DS_PROPNAME_textstring "textstring"
#define QT3DS_PROPNAME_textcolor "textcolor"
#define QT3DS_PROPNAME_font "font"
#define QT3DS_PROPNAME_size "size"
#define QT3DS_PROPNAME_horzalign "horzalign"
#define QT3DS_PROPNAME_vertalign "vertalign"
#define QT3DS_PROPNAME_leading "leading"
#define QT3DS_PROPNAME_tracking "tracking"
#define QT3DS_PROPNAME_dropshadow "dropshadow"
#define QT3DS_PROPNAME_dropshadowstrength "dropshadowstrength"
#define QT3DS_PROPNAME_dropshadowoffset "dropshadowoffset" // To be removed in 2.x (when UIP version is next updated)
#define QT3DS_PROPNAME_dropshadowoffsetx "dropshadowoffsetx"
#define QT3DS_PROPNAME_dropshadowoffsety "dropshadowoffsety"
#define QT3DS_PROPNAME_dropshadowhorzalign "dropshadowhorzalign" // To be removed in 2.x (when UIP version is next updated)
#define QT3DS_PROPNAME_dropshadowvertalign "dropshadowvertalign" // To be removed in 2.x (when UIP version is next updated)
#define QT3DS_PROPNAME_wordwrap "wordwrap"
#define QT3DS_PROPNAME_boundingbox "boundingbox"
#define QT3DS_PROPNAME_elide "elide"
#define QT3DS_PROPNAME_enableacceleratedfont "enableacceleratedfont"
#define QT3DS_PROPNAME_importfile "importfile"
#define QT3DS_PROPNAME_fileid "fileid"
#define QT3DS_PROPNAME_size "size"
#define QT3DS_PROPNAME_location "location"
#define QT3DS_PROPNAME_boneid "boneid"
#define QT3DS_PROPNAME_poseroot "poseroot"
#define QT3DS_PROPNAME_ignoresparent "ignoresparent"
#define QT3DS_PROPNAME_tessellation "tessellation"
#define QT3DS_PROPNAME_edgetess "edgetess"
#define QT3DS_PROPNAME_innertess "innertess"
#define QT3DS_PROPNAME_scalemode "scalemode"
#define QT3DS_PROPNAME_scaleanchor "scaleanchor"
#define QT3DS_PROPNAME_horzfields "horzfields"
#define QT3DS_PROPNAME_left "left"
#define QT3DS_PROPNAME_leftunits "leftunits"
#define QT3DS_PROPNAME_width "width"
#define QT3DS_PROPNAME_widthunits "widthunits"
#define QT3DS_PROPNAME_right "right"
#define QT3DS_PROPNAME_rightunits "rightunits"
#define QT3DS_PROPNAME_vertfields "vertfields"
#define QT3DS_PROPNAME_top "top"
#define QT3DS_PROPNAME_topunits "topunits"
#define QT3DS_PROPNAME_height "height"
#define QT3DS_PROPNAME_heightunits "heightunits"
#define QT3DS_PROPNAME_bottom "bottom"
#define QT3DS_PROPNAME_bottomunits "bottomunits"
#define QT3DS_PROPNAME_aostrength "aostrength"
#define QT3DS_PROPNAME_aodistance "aodistance"
#define QT3DS_PROPNAME_aosoftness "aosoftness"
#define QT3DS_PROPNAME_aobias "aobias"
#define QT3DS_PROPNAME_aosamplerate "aosamplerate"
#define QT3DS_PROPNAME_aodither "aodither"
#define QT3DS_PROPNAME_shadowstrength "shadowstrength"
#define QT3DS_PROPNAME_shadowdist "shadowdist"
#define QT3DS_PROPNAME_shadowsoftness "shadowsoftness"
#define QT3DS_PROPNAME_shadowbias "shadowbias"
#define QT3DS_PROPNAME_source "source"
#define QT3DS_PROPNAME_referencedmaterial "referencedmaterial"
#define QT3DS_PROPNAME_lightprobe "lightprobe"
#define QT3DS_PROPNAME_probebright "probebright"
#define QT3DS_PROPNAME_fastibl "fastibl"
#define QT3DS_PROPNAME_probehorizon "probehorizon"
#define QT3DS_PROPNAME_probefov "probefov"
#define QT3DS_PROPNAME_lightprobe2 "lightprobe2"
#define QT3DS_PROPNAME_probe2fade "probe2fade"
#define QT3DS_PROPNAME_probe2window "probe2window"
#define QT3DS_PROPNAME_probe2pos "probe2pos"
#define QT3DS_PROPNAME_bumpmap "bumpmap"
#define QT3DS_PROPNAME_bumpamount "bumpamount"
#define QT3DS_PROPNAME_normalmap "normalmap"
#define QT3DS_PROPNAME_displacementmap "displacementmap"
#define QT3DS_PROPNAME_displaceamount "displaceamount"
#define QT3DS_PROPNAME_translucencymap "translucencymap"
#define QT3DS_PROPNAME_translucentfalloff "translucentfalloff"
#define QT3DS_PROPNAME_diffuselightwrap "diffuselightwrap"
#define QT3DS_PROPNAME_fresnelPower "fresnelPower"
#define QT3DS_PROPNAME_referencednode "referencednode"
#define QT3DS_PROPNAME_temporalaa "temporalaa"
#define QT3DS_PROPNAME_scope "scope"
#define QT3DS_PROPNAME_Path "Path"
#define QT3DS_PROPNAME_pathdata "pathdata"
#define QT3DS_PROPNAME_PathAnchorPoint "PathAnchorPoint"
#define QT3DS_PROPNAME_incomingangle "incomingangle"
#define QT3DS_PROPNAME_incomingdistance "incomingdistance"
#define QT3DS_PROPNAME_outgoingdistance "outgoingdistance"
#define QT3DS_PROPNAME_linearerror "linearerror"
#define QT3DS_PROPNAME_edgetessamount "edgetessamount"
#define QT3DS_PROPNAME_innertessamount "innertessamount"
#define QT3DS_PROPNAME_begincap "begincap"
#define QT3DS_PROPNAME_begincapoffset "begincapoffset"
#define QT3DS_PROPNAME_begincapopacity "begincapopacity"
#define QT3DS_PROPNAME_begincapwidth "begincapwidth"
#define QT3DS_PROPNAME_endcap "endcap"
#define QT3DS_PROPNAME_endcapoffset "endcapoffset"
#define QT3DS_PROPNAME_endcapopacity "endcapopacity"
#define QT3DS_PROPNAME_endcapwidth "endcapwidth"
#define QT3DS_PROPNAME_pathtype "pathtype"
#define QT3DS_PROPNAME_closed "closed"
#define QT3DS_PROPNAME_paintstyle "paintstyle"
#define QT3DS_PROPNAME_SubPath "SubPath"
#define QT3DS_PROPNAME_lightmapindirect "lightmapindirect"
#define QT3DS_PROPNAME_lightmapradiosity "lightmapradiosity"
#define QT3DS_PROPNAME_lightmapshadow "lightmapshadow"
#define QT3DS_PROPNAME_controlledproperty "controlledproperty"

QString ComposerObjectTypes::Convert(ComposerObjectTypes::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case name:                                                                                     \
        return QStringLiteral(QT3DS_PROPNAME_##name);
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return QStringLiteral("Unknown");
}

ComposerObjectTypes::Enum ComposerObjectTypes::Convert(const QString &inType)
{

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (inType == QLatin1String(QT3DS_PROPNAME_##name))                                                      \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    QT3DS_ASSERT(false);
    return ComposerObjectTypes::Unknown;
}

QString ComposerPropertyNames::Convert(ComposerPropertyNames::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    case name:                                                                                     \
        return QStringLiteral(QT3DS_PROPNAME_##name);
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
    return QStringLiteral("Unknown");
}

ComposerPropertyNames::Enum ComposerPropertyNames::Convert(const QString &inType)
{
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    if (inType == QLatin1String(QT3DS_PROPNAME_##name))                                                      \
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
        IDataCore &inCore, Qt3DSDMInstanceHandle inInstance)                                        \
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
                                                           Qt3DSDMInstanceHandle inInstance,
                                                           ComposerObjectTypes::Enum inObjectType)
{
    inMetaData.SetInstanceAsCanonical(inInstance, ComposerObjectTypes::Convert(inObjectType));
}

void ComposerTypeDefinitionsHelper::SetInstancePropertyValue(IDataCore &inDataCore,
                                                             Qt3DSDMInstanceHandle inInstance,
                                                             Qt3DSDMPropertyHandle inProperty,
                                                             const QString &inPropValue)
{
    inDataCore.SetInstancePropertyValue(inInstance, inProperty, make_shared<CDataStr>(inPropValue));
}

void ComposerTypeDefinitionsHelper::DeriveInstance(IDataCore &inDataCore,
                                                   Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMInstanceHandle inParent)
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
    , m_MaterialBase(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Lightmaps(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_MaterialBase)
    , m_Material(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_CustomMaterial(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_ReferencedMaterial(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_Behavior(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Effect(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Node(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Layer(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Group(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Model(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Light(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Camera(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Component(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node, m_SlideOwner)
    , m_Text(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_RenderPlugin(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Alias(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Path(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_PathAnchorPoint(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_SubPath(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
{
}

bool SComposerObjectDefinitions::IsA(Qt3DSDMInstanceHandle inInstance,
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
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return false;
}

// Could easily return None, meaning we can't identify the object type.
// Checks the type of the first derivation parent, so this won't ever return
// SlideOwner, for instance.
ComposerObjectTypes::Enum SComposerObjectDefinitions::GetType(Qt3DSDMInstanceHandle inInstance)
{
    if (m_DataCore.IsInstance(inInstance) == false)
        return ComposerObjectTypes::Unknown;

    Qt3DSDMInstanceHandle theTargetInstance = inInstance;
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

Qt3DSDMInstanceHandle
SComposerObjectDefinitions::GetInstanceForType(ComposerObjectTypes::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case ComposerObjectTypes::name:                                                                \
        return m_##name.m_Instance;
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return 0;
}
