/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "RuntimePrefix.h"

//==============================================================================
//    Includes
//==============================================================================
#include "Qt3DSAttributeHashes.h"

//==============================================================================
//    Namespace
//==============================================================================
namespace Q3DStudio {

// !!!!! AUTOGENERATED CODE - DO NOT MODIFY MANUALLY !!!!!

// Run the AttributeHashes project to regenerate this file from Attributehashes.txt list



/// Function providing reverse hash lookup
const char *GetAttributeString(const EAttribute inAttribute)
{
    switch (inAttribute) {
        case ATTRIBUTE_NAME: return "name";
        case ATTRIBUTE_TYPE: return "type";
        case ATTRIBUTE_OPACITY: return "opacity";
        case ATTRIBUTE_STARTTIME: return "starttime";
        case ATTRIBUTE_ENDTIME: return "endtime";
        case ATTRIBUTE_SOURCEPATH: return "sourcepath";
        case ATTRIBUTE_IMPORTID: return "importid";
        case ATTRIBUTE_EYEBALL: return "eyeball";
        case ATTRIBUTE_POSITION: return "position";
        case ATTRIBUTE_POSITION_X: return "position.x";
        case ATTRIBUTE_POSITION_Y: return "position.y";
        case ATTRIBUTE_POSITION_Z: return "position.z";
        case ATTRIBUTE_ROTATION: return "rotation";
        case ATTRIBUTE_ROTATION_X: return "rotation.x";
        case ATTRIBUTE_ROTATION_Y: return "rotation.y";
        case ATTRIBUTE_ROTATION_Z: return "rotation.z";
        case ATTRIBUTE_SCALE: return "scale";
        case ATTRIBUTE_SCALE_X: return "scale.x";
        case ATTRIBUTE_SCALE_Y: return "scale.y";
        case ATTRIBUTE_SCALE_Z: return "scale.z";
        case ATTRIBUTE_PIVOT: return "pivot";
        case ATTRIBUTE_PIVOT_X: return "pivot.x";
        case ATTRIBUTE_PIVOT_Y: return "pivot.y";
        case ATTRIBUTE_PIVOT_Z: return "pivot.z";
        case ATTRIBUTE_ROTATIONORDER: return "rotationorder";
        case ATTRIBUTE_ORIENTATION: return "orientation";
        case ATTRIBUTE_TESSELLATION: return "tessellation";
        case ATTRIBUTE_EDGETESS: return "edgetess";
        case ATTRIBUTE_INNERTESS: return "innertess";
        case ATTRIBUTE_ORTHOGRAPHIC: return "orthographic";
        case ATTRIBUTE_CLIPNEAR: return "clipnear";
        case ATTRIBUTE_CLIPFAR: return "clipfar";
        case ATTRIBUTE_FOV: return "fov";
        case ATTRIBUTE_SCALEMODE: return "scalemode";
        case ATTRIBUTE_SCALEANCHOR: return "scaleanchor";
        case ATTRIBUTE_BRIGHTNESS: return "brightness";
        case ATTRIBUTE_LINEARFADE: return "linearfade";
        case ATTRIBUTE_EXPFADE: return "expfade";
        case ATTRIBUTE_LIGHTTYPE: return "lighttype";
        case ATTRIBUTE_SCOPE: return "scope";
        case ATTRIBUTE_LIGHTDIFFUSE: return "lightdiffuse";
        case ATTRIBUTE_LIGHTDIFFUSE_R: return "lightdiffuse.r";
        case ATTRIBUTE_LIGHTDIFFUSE_G: return "lightdiffuse.g";
        case ATTRIBUTE_LIGHTDIFFUSE_B: return "lightdiffuse.b";
        case ATTRIBUTE_LIGHTAMBIENT_R: return "lightambient.r";
        case ATTRIBUTE_LIGHTAMBIENT: return "lightambient";
        case ATTRIBUTE_LIGHTAMBIENT_G: return "lightambient.g";
        case ATTRIBUTE_LIGHTAMBIENT_B: return "lightambient.b";
        case ATTRIBUTE_LIGHTSPECULAR: return "lightspecular";
        case ATTRIBUTE_LIGHTSPECULAR_R: return "lightspecular.r";
        case ATTRIBUTE_LIGHTSPECULAR_G: return "lightspecular.g";
        case ATTRIBUTE_LIGHTSPECULAR_B: return "lightspecular.b";
        case ATTRIBUTE_AREAWIDTH: return "areawidth";
        case ATTRIBUTE_AREAHEIGHT: return "areaheight";
        case ATTRIBUTE_CASTSHADOW: return "castshadow";
        case ATTRIBUTE_SHDWBIAS: return "shdwbias";
        case ATTRIBUTE_SHDWFACTOR: return "shdwfactor";
        case ATTRIBUTE_SHDWMAPRES: return "shdwmapres";
        case ATTRIBUTE_SHDWMAPFAR: return "shdwmapfar";
        case ATTRIBUTE_SHDWMAPFOV: return "shdwmapfov";
        case ATTRIBUTE_SHDWFILTER: return "shdwfilter";
        case ATTRIBUTE_LIGHTMAPINDIRECT: return "lightmapindirect";
        case ATTRIBUTE_LIGHTMAPRADIOSITY: return "lightmapradiosity";
        case ATTRIBUTE_LIGHTMAPSHADOW: return "lightmapshadow";
        case ATTRIBUTE_IBLPROBE: return "iblprobe";
        case ATTRIBUTE_SHADERLIGHTING: return "shaderlighting";
        case ATTRIBUTE_EMISSIVEPOWER: return "emissivepower";
        case ATTRIBUTE_EMISSIVECOLOR: return "emissivecolor";
        case ATTRIBUTE_EMISSIVECOLOR_R: return "emissivecolor.r";
        case ATTRIBUTE_EMISSIVECOLOR_G: return "emissivecolor.g";
        case ATTRIBUTE_EMISSIVECOLOR_B: return "emissivecolor.b";
        case ATTRIBUTE_DIFFUSE: return "diffuse";
        case ATTRIBUTE_DIFFUSE_R: return "diffuse.r";
        case ATTRIBUTE_DIFFUSE_G: return "diffuse.g";
        case ATTRIBUTE_DIFFUSE_B: return "diffuse.b";
        case ATTRIBUTE_SPECULARMAP: return "specularmap";
        case ATTRIBUTE_SPECULARMODEL: return "specularmodel";
        case ATTRIBUTE_SPECULARTINT: return "speculartint";
        case ATTRIBUTE_SPECULARTINT_R: return "speculartint.r";
        case ATTRIBUTE_SPECULARTINT_G: return "speculartint.g";
        case ATTRIBUTE_SPECULARTINT_B: return "speculartint.b";
        case ATTRIBUTE_IOR: return "ior";
        case ATTRIBUTE_FRESNELPOWER: return "fresnelPower";
        case ATTRIBUTE_SPECULARAMOUNT: return "specularamount";
        case ATTRIBUTE_SPECULARROUGHNESS: return "specularroughness";
        case ATTRIBUTE_BLENDMODE: return "blendmode";
        case ATTRIBUTE_CULLING: return "culling";
        case ATTRIBUTE_ZBUFFERWRITE: return "zbufferwrite";
        case ATTRIBUTE_DIFFUSEMAP: return "diffusemap";
        case ATTRIBUTE_DIFFUSEMAP2: return "diffusemap2";
        case ATTRIBUTE_DIFFUSEMAP3: return "diffusemap3";
        case ATTRIBUTE_SPECULARREFLECTION: return "specularreflection";
        case ATTRIBUTE_OPACITYMAP: return "opacitymap";
        case ATTRIBUTE_EMISSIVEMAP: return "emissivemap";
        case ATTRIBUTE_EMISSIVEMAP2: return "emissivemap2";
        case ATTRIBUTE_BUMPMAP: return "bumpmap";
        case ATTRIBUTE_BUMPAMOUNT: return "bumpamount";
        case ATTRIBUTE_NORMALMAP: return "normalmap";
        case ATTRIBUTE_DISPLACEMENTMAP: return "displacementmap";
        case ATTRIBUTE_DISPLACEAMOUNT: return "displaceamount";
        case ATTRIBUTE_TRANSLUCENCYMAP: return "translucencymap";
        case ATTRIBUTE_TRANSLUCENTFALLOFF: return "translucentfalloff";
        case ATTRIBUTE_DIFFUSELIGHTWRAP: return "diffuselightwrap";
        case ATTRIBUTE_REFERENCEDMATERIAL: return "referencedmaterial";
        case ATTRIBUTE_ROTATIONUV: return "rotationuv";
        case ATTRIBUTE_POSITIONU: return "positionu";
        case ATTRIBUTE_POSITIONV: return "positionv";
        case ATTRIBUTE_SCALEU: return "scaleu";
        case ATTRIBUTE_SCALEV: return "scalev";
        case ATTRIBUTE_PIVOTU: return "pivotu";
        case ATTRIBUTE_PIVOTV: return "pivotv";
        case ATTRIBUTE_TILINGMODEHORZ: return "tilingmodehorz";
        case ATTRIBUTE_TILINGMODEVERT: return "tilingmodevert";
        case ATTRIBUTE_MAPPINGTYPE: return "mappingtype";
        case ATTRIBUTE_MAPPINGMODE: return "mappingmode";
        case ATTRIBUTE_SUBPRESENTATION: return "subpresentation";
        case ATTRIBUTE_URI: return "uri";
        case ATTRIBUTE_TRANSPARENT: return "transparent";
        case ATTRIBUTE_PROGRESSIVEAA: return "progressiveaa";
        case ATTRIBUTE_MULTISAMPLEAA: return "multisampleaa";
        case ATTRIBUTE_TEMPORALAA: return "temporalaa";
        case ATTRIBUTE_BLENDTYPE: return "blendtype";
        case ATTRIBUTE_HORZFIELDS: return "horzfields";
        case ATTRIBUTE_LEFT: return "left";
        case ATTRIBUTE_LEFTUNITS: return "leftunits";
        case ATTRIBUTE_WIDTH: return "width";
        case ATTRIBUTE_WIDTHUNITS: return "widthunits";
        case ATTRIBUTE_RIGHT: return "right";
        case ATTRIBUTE_RIGHTUNITS: return "rightunits";
        case ATTRIBUTE_VERTFIELDS: return "vertfields";
        case ATTRIBUTE_TOP: return "top";
        case ATTRIBUTE_TOPUNITS: return "topunits";
        case ATTRIBUTE_HEIGHT: return "height";
        case ATTRIBUTE_HEIGHTUNITS: return "heightunits";
        case ATTRIBUTE_BOTTOM: return "bottom";
        case ATTRIBUTE_BOTTOMUNITS: return "bottomunits";
        case ATTRIBUTE_AOSTRENGTH: return "aostrength";
        case ATTRIBUTE_AODISTANCE: return "aodistance";
        case ATTRIBUTE_AOSOFTNESS: return "aosoftness";
        case ATTRIBUTE_AOBIAS: return "aobias";
        case ATTRIBUTE_AOSAMPLERATE: return "aosamplerate";
        case ATTRIBUTE_AODITHER: return "aodither";
        case ATTRIBUTE_SHADOWSTRENGTH: return "shadowstrength";
        case ATTRIBUTE_SHADOWDIST: return "shadowdist";
        case ATTRIBUTE_SHADOWSOFTNESS: return "shadowsoftness";
        case ATTRIBUTE_SHADOWBIAS: return "shadowbias";
        case ATTRIBUTE_LIGHTPROBE: return "lightprobe";
        case ATTRIBUTE_PROBEBRIGHT: return "probebright";
        case ATTRIBUTE_FASTIBL: return "fastibl";
        case ATTRIBUTE_PROBEHORIZON: return "probehorizon";
        case ATTRIBUTE_PROBEFOV: return "probefov";
        case ATTRIBUTE_LIGHTPROBE2: return "lightprobe2";
        case ATTRIBUTE_PROBE2FADE: return "probe2fade";
        case ATTRIBUTE_PROBE2WINDOW: return "probe2window";
        case ATTRIBUTE_PROBE2POS: return "probe2pos";
        case ATTRIBUTE_DISABLEDEPTHTEST: return "disabledepthtest";
        case ATTRIBUTE_DISABLEDEPTHPREPASS: return "disabledepthprepass";
        case ATTRIBUTE_TEXTCOLOR: return "textcolor";
        case ATTRIBUTE_TEXTCOLOR_R: return "textcolor.r";
        case ATTRIBUTE_TEXTCOLOR_G: return "textcolor.g";
        case ATTRIBUTE_TEXTCOLOR_B: return "textcolor.b";
        case ATTRIBUTE_SIZE: return "size";
        case ATTRIBUTE_FONT: return "font";
        case ATTRIBUTE_TRACKING: return "tracking";
        case ATTRIBUTE_LEADING: return "leading";
        case ATTRIBUTE_RENDERSTYLE: return "renderstyle";
        case ATTRIBUTE_TEXTSTRING: return "textstring";
        case ATTRIBUTE_BACKCOLOR_R: return "backcolor.r";
        case ATTRIBUTE_BACKCOLOR_G: return "backcolor.g";
        case ATTRIBUTE_BACKCOLOR_B: return "backcolor.b";
        case ATTRIBUTE_TEXTTYPE: return "texttype";
        case ATTRIBUTE_USEBACKCOLOR: return "usebackcolor";
        case ATTRIBUTE_WORDWRAP: return "wordwrap";
        case ATTRIBUTE_HORZSCROLL: return "horzscroll";
        case ATTRIBUTE_HORZALIGN: return "horzalign";
        case ATTRIBUTE_VERTSCROLL: return "vertscroll";
        case ATTRIBUTE_VERTALIGN: return "vertalign";
        case ATTRIBUTE_BOXHEIGHT: return "boxheight";
        case ATTRIBUTE_BOXWIDTH: return "boxwidth";
        case ATTRIBUTE_REMOTESTRINGSOURCE: return "remotestringsource";
        case ATTRIBUTE_CACHEDTEXTSTRING: return "cachedtextstring";
        case ATTRIBUTE_ENABLEACCELERATEDFONT: return "enableacceleratedfont";
        case ATTRIBUTE_BEHAVIORSCRIPTS: return "BehaviorScripts";
        case ATTRIBUTE_UICCUSTOMOBJTYPE: return "UICCustomObjType";
        case ATTRIBUTE_BGCOLORENABLE: return "bgcolorenable";
        case ATTRIBUTE_BACKGROUND: return "background";
        case ATTRIBUTE_BACKGROUNDCOLOR_R: return "backgroundcolor.r";
        case ATTRIBUTE_BACKGROUNDCOLOR_G: return "backgroundcolor.g";
        case ATTRIBUTE_BACKGROUNDCOLOR_B: return "backgroundcolor.b";
        case ATTRIBUTE_PATHTYPE: return "pathtype";
        case ATTRIBUTE_LINEARERROR: return "linearerror";
        case ATTRIBUTE_EDGETESSAMOUNT: return "edgetessamount";
        case ATTRIBUTE_INNERTESSAMOUNT: return "innertessamount";
        case ATTRIBUTE_BEGINCAP: return "begincap";
        case ATTRIBUTE_BEGINCAPOFFSET: return "begincapoffset";
        case ATTRIBUTE_BEGINCAPOPACITY: return "begincapopacity";
        case ATTRIBUTE_BEGINCAPWIDTH: return "begincapwidth";
        case ATTRIBUTE_ENDCAP: return "endcap";
        case ATTRIBUTE_ENDCAPOFFSET: return "endcapoffset";
        case ATTRIBUTE_ENDCAPOPACITY: return "endcapopacity";
        case ATTRIBUTE_ENDCAPWIDTH: return "endcapwidth";
        case ATTRIBUTE_PAINTSTYLE: return "paintstyle";
        case ATTRIBUTE_CLOSED: return "closed";
        case ATTRIBUTE_INCOMINGANGLE: return "incomingangle";
        case ATTRIBUTE_INCOMINGDISTANCE: return "incomingdistance";
        case ATTRIBUTE_OUTGOINGDISTANCE: return "outgoingdistance";
        case ATTRIBUTE_PARTICLETYPE: return "particletype";
        case ATTRIBUTE_MAXPARTICLES: return "maxparticles";
        case ATTRIBUTE_PARTICLESIZE: return "particlesize";
        case ATTRIBUTE_LIFETIME: return "lifetime";
        case ATTRIBUTE_CONTROLLEDPROPERTY: return "controlledproperty";
        case ATTRIBUTE_QT_IO: return "qt.io";
        default: {
            static char s_UnknownHash[16];
            sprintf(s_UnknownHash, "(0x%08X)", inAttribute);
            return s_UnknownHash;
        }
    }
}

} // namespace Q3DStudio

