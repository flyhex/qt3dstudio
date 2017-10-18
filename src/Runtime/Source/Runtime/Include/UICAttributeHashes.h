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

#pragma once

//==============================================================================
//    Namespace
//==============================================================================
namespace Q3DStudio {

// !!!!! AUTOGENERATED CODE - DO NOT MODIFY MANUALLY !!!!!

// Run the AttributeHashes project to regenerate this file from Attributehashes.txt list

/// Key for the CElement attribute-value pair
enum EAttribute {
    ATTRIBUTE_NAME =                 0x02B79D95, // name
    ATTRIBUTE_TYPE =                 0x005F9806, // type
    ATTRIBUTE_OPACITY =              0x0191C315, // opacity
    ATTRIBUTE_STARTTIME =            0x010A57B1, // starttime
    ATTRIBUTE_ENDTIME =              0x003BF5F8, // endtime
    ATTRIBUTE_SOURCEPATH =           0x0009EA60, // sourcepath
    ATTRIBUTE_IMPORTID =             0x008F7900, // importid
    ATTRIBUTE_EYEBALL =              0x02F454F0, // eyeball
    ATTRIBUTE_POSITION =             0x00E9B7D7, // position
    ATTRIBUTE_POSITION_X =           0x027C230D, // position.x
    ATTRIBUTE_POSITION_Y =           0x027D234C, // position.y
    ATTRIBUTE_POSITION_Z =           0x027E238B, // position.z
    ATTRIBUTE_ROTATION =             0x03E51862, // rotation
    ATTRIBUTE_ROTATION_X =           0x0239EE18, // rotation.x
    ATTRIBUTE_ROTATION_Y =           0x023AEE57, // rotation.y
    ATTRIBUTE_ROTATION_Z =           0x023BEE96, // rotation.z
    ATTRIBUTE_SCALE =                0x01012856, // scale
    ATTRIBUTE_SCALE_X =              0x0065440C, // scale.x
    ATTRIBUTE_SCALE_Y =              0x0066444B, // scale.y
    ATTRIBUTE_SCALE_Z =              0x0067448A, // scale.z
    ATTRIBUTE_PIVOT =                0x009E907E, // pivot
    ATTRIBUTE_PIVOT_X =              0x03811834, // pivot.x
    ATTRIBUTE_PIVOT_Y =              0x03821873, // pivot.y
    ATTRIBUTE_PIVOT_Z =              0x038318B2, // pivot.z
    ATTRIBUTE_ROTATIONORDER =        0x03CE5F70, // rotationorder
    ATTRIBUTE_ORIENTATION =          0x001A90B0, // orientation
    ATTRIBUTE_TESSELLATION =         0x0335861F, // tessellation
    ATTRIBUTE_EDGETESS =             0x023933D2, // edgetess
    ATTRIBUTE_INNERTESS =            0x01529259, // innertess
    ATTRIBUTE_ORTHOGRAPHIC =         0x0244BB70, // orthographic
    ATTRIBUTE_CLIPNEAR =             0x0068FF28, // clipnear
    ATTRIBUTE_CLIPFAR =              0x037EF699, // clipfar
    ATTRIBUTE_FOV =                  0x00D60213, // fov
    ATTRIBUTE_SCALEMODE =            0x01FD2FD3, // scalemode
    ATTRIBUTE_SCALEANCHOR =          0x02CFCF41, // scaleanchor
    ATTRIBUTE_BRIGHTNESS =           0x0230D3AF, // brightness
    ATTRIBUTE_LINEARFADE =           0x0104E9FF, // linearfade
    ATTRIBUTE_EXPFADE =              0x006B9267, // expfade
    ATTRIBUTE_LIGHTTYPE =            0x0033F1D0, // lighttype
    ATTRIBUTE_SCOPE =                0x0258D0CC, // scope
    ATTRIBUTE_LIGHTDIFFUSE =         0x01246FD4, // lightdiffuse
    ATTRIBUTE_LIGHTDIFFUSE_R =       0x035AAB10, // lightdiffuse.r
    ATTRIBUTE_LIGHTDIFFUSE_G =       0x034FA85B, // lightdiffuse.g
    ATTRIBUTE_LIGHTDIFFUSE_B =       0x034AA720, // lightdiffuse.b
    ATTRIBUTE_LIGHTAMBIENT_R =       0x0179AD1A, // lightambient.r
    ATTRIBUTE_LIGHTAMBIENT =         0x00DA56DE, // lightambient
    ATTRIBUTE_LIGHTAMBIENT_G =       0x016EAA65, // lightambient.g
    ATTRIBUTE_LIGHTAMBIENT_B =       0x0169A92A, // lightambient.b
    ATTRIBUTE_LIGHTSPECULAR =        0x03E39A07, // lightspecular
    ATTRIBUTE_LIGHTSPECULAR_R =      0x0241EBC3, // lightspecular.r
    ATTRIBUTE_LIGHTSPECULAR_G =      0x0236E90E, // lightspecular.g
    ATTRIBUTE_LIGHTSPECULAR_B =      0x0231E7D3, // lightspecular.b
    ATTRIBUTE_AREAWIDTH =            0x005A8BE7, // areawidth
    ATTRIBUTE_AREAHEIGHT =           0x00334D2C, // areaheight
    ATTRIBUTE_CASTSHADOW =           0x0335FD81, // castshadow
    ATTRIBUTE_SHDWBIAS =             0x0125E79F, // shdwbias
    ATTRIBUTE_SHDWFACTOR =           0x01B11BE9, // shdwfactor
    ATTRIBUTE_SHDWMAPRES =           0x01E53834, // shdwmapres
    ATTRIBUTE_SHDWMAPFAR =           0x019A30FD, // shdwmapfar
    ATTRIBUTE_SHDWMAPFOV =           0x00830B07, // shdwmapfov
    ATTRIBUTE_SHDWFILTER =           0x0176E1E0, // shdwfilter
    ATTRIBUTE_LIGHTMAPINDIRECT =     0x004F1D6C, // lightmapindirect
    ATTRIBUTE_LIGHTMAPRADIOSITY =    0x00AC7C50, // lightmapradiosity
    ATTRIBUTE_LIGHTMAPSHADOW =       0x00191F3A, // lightmapshadow
    ATTRIBUTE_IBLPROBE =             0x0039FD03, // iblprobe
    ATTRIBUTE_SHADERLIGHTING =       0x0068A84F, // shaderlighting
    ATTRIBUTE_EMISSIVEPOWER =        0x03D6F9F2, // emissivepower
    ATTRIBUTE_EMISSIVECOLOR =        0x00B7AC94, // emissivecolor
    ATTRIBUTE_EMISSIVECOLOR_R =      0x039B87D0, // emissivecolor.r
    ATTRIBUTE_EMISSIVECOLOR_G =      0x0390851B, // emissivecolor.g
    ATTRIBUTE_EMISSIVECOLOR_B =      0x038B83E0, // emissivecolor.b
    ATTRIBUTE_DIFFUSE =              0x0105521E, // diffuse
    ATTRIBUTE_DIFFUSE_R =            0x015B085A, // diffuse.r
    ATTRIBUTE_DIFFUSE_G =            0x015005A5, // diffuse.g
    ATTRIBUTE_DIFFUSE_B =            0x014B046A, // diffuse.b
    ATTRIBUTE_SPECULARMAP =          0x034CD047, // specularmap
    ATTRIBUTE_SPECULARMODEL =        0x039EBE5A, // specularmodel
    ATTRIBUTE_SPECULARTINT =         0x03535E02, // speculartint
    ATTRIBUTE_SPECULARTINT_R =       0x0399623E, // speculartint.r
    ATTRIBUTE_SPECULARTINT_G =       0x038E5F89, // speculartint.g
    ATTRIBUTE_SPECULARTINT_B =       0x03895E4E, // speculartint.b
    ATTRIBUTE_IOR =                  0x00667354, // ior
    ATTRIBUTE_FRESNELPOWER =         0x022178B6, // fresnelPower
    ATTRIBUTE_SPECULARAMOUNT =       0x01144425, // specularamount
    ATTRIBUTE_SPECULARROUGHNESS =    0x03925653, // specularroughness
    ATTRIBUTE_BLENDMODE =            0x01923A6C, // blendmode
    ATTRIBUTE_CULLING =              0x03C539F0, // culling
    ATTRIBUTE_ZBUFFERWRITE =         0x03E19B3B, // zbufferwrite
    ATTRIBUTE_DIFFUSEMAP =           0x00FF8126, // diffusemap
    ATTRIBUTE_DIFFUSEMAP2 =          0x0038D4A8, // diffusemap2
    ATTRIBUTE_DIFFUSEMAP3 =          0x0039D4E7, // diffusemap3
    ATTRIBUTE_SPECULARREFLECTION =   0x006B4C12, // specularreflection
    ATTRIBUTE_OPACITYMAP =           0x00DA796F, // opacitymap
    ATTRIBUTE_EMISSIVEMAP =          0x00F6427B, // emissivemap
    ATTRIBUTE_EMISSIVEMAP2 =         0x03476893, // emissivemap2
    ATTRIBUTE_BUMPMAP =              0x024EE11A, // bumpmap
    ATTRIBUTE_BUMPAMOUNT =           0x01BC4192, // bumpamount
    ATTRIBUTE_NORMALMAP =            0x03BD578B, // normalmap
    ATTRIBUTE_DISPLACEMENTMAP =      0x01BCD1FB, // displacementmap
    ATTRIBUTE_DISPLACEAMOUNT =       0x01EC1EAF, // displaceamount
    ATTRIBUTE_TRANSLUCENCYMAP =      0x01D8F015, // translucencymap
    ATTRIBUTE_TRANSLUCENTFALLOFF =   0x0097E985, // translucentfalloff
    ATTRIBUTE_DIFFUSELIGHTWRAP =     0x038F6522, // diffuselightwrap
    ATTRIBUTE_REFERENCEDMATERIAL =   0x035FDA80, // referencedmaterial
    ATTRIBUTE_ROTATIONUV =           0x012E3A61, // rotationuv
    ATTRIBUTE_POSITIONU =            0x01D05AB4, // positionu
    ATTRIBUTE_POSITIONV =            0x01D15AF3, // positionv
    ATTRIBUTE_SCALEU =               0x001409F5, // scaleu
    ATTRIBUTE_SCALEV =               0x00150A34, // scalev
    ATTRIBUTE_PIVOTU =               0x03F8ABCD, // pivotu
    ATTRIBUTE_PIVOTV =               0x03F9AC0C, // pivotv
    ATTRIBUTE_TILINGMODEHORZ =       0x02562203, // tilingmodehorz
    ATTRIBUTE_TILINGMODEVERT =       0x03F92B21, // tilingmodevert
    ATTRIBUTE_MAPPINGTYPE =          0x02CA9058, // mappingtype
    ATTRIBUTE_MAPPINGMODE =          0x002715CF, // mappingmode
    ATTRIBUTE_SUBPRESENTATION =      0x03CA7426, // subpresentation
    ATTRIBUTE_URI =                  0x00296894, // uri
    ATTRIBUTE_TRANSPARENT =          0x0316BA2E, // transparent
    ATTRIBUTE_PROGRESSIVEAA =        0x019F1955, // progressiveaa
    ATTRIBUTE_MULTISAMPLEAA =        0x013D29FD, // multisampleaa
    ATTRIBUTE_TEMPORALAA =           0x00212AFE, // temporalaa
    ATTRIBUTE_BLENDTYPE =            0x0035B4F5, // blendtype
    ATTRIBUTE_HORZFIELDS =           0x02B8A818, // horzfields
    ATTRIBUTE_LEFT =                 0x0196B9B9, // left
    ATTRIBUTE_LEFTUNITS =            0x02F9D2D8, // leftunits
    ATTRIBUTE_WIDTH =                0x00C4D65A, // width
    ATTRIBUTE_WIDTHUNITS =           0x01D7DF77, // widthunits
    ATTRIBUTE_RIGHT =                0x039EAB44, // right
    ATTRIBUTE_RIGHTUNITS =           0x0357EF0D, // rightunits
    ATTRIBUTE_VERTFIELDS =           0x03462436, // vertfields
    ATTRIBUTE_TOP =                  0x002F6B0B, // top
    ATTRIBUTE_TOPUNITS =             0x03D58806, // topunits
    ATTRIBUTE_HEIGHT =               0x00CE9F79, // height
    ATTRIBUTE_HEIGHTUNITS =          0x00C91D18, // heightunits
    ATTRIBUTE_BOTTOM =               0x00F4EE75, // bottom
    ATTRIBUTE_BOTTOMUNITS =          0x0174091C, // bottomunits
    ATTRIBUTE_AOSTRENGTH =           0x010F7ED1, // aostrength
    ATTRIBUTE_AODISTANCE =           0x01DC349D, // aodistance
    ATTRIBUTE_AOSOFTNESS =           0x02CCDC71, // aosoftness
    ATTRIBUTE_AOBIAS =               0x01818219, // aobias
    ATTRIBUTE_AOSAMPLERATE =         0x0039B568, // aosamplerate
    ATTRIBUTE_AODITHER =             0x0274316C, // aodither
    ATTRIBUTE_SHADOWSTRENGTH =       0x0039ED5F, // shadowstrength
    ATTRIBUTE_SHADOWDIST =           0x038213FA, // shadowdist
    ATTRIBUTE_SHADOWSOFTNESS =       0x01F74AFF, // shadowsoftness
    ATTRIBUTE_SHADOWBIAS =           0x02CB3EA7, // shadowbias
    ATTRIBUTE_LIGHTPROBE =           0x02D47DC6, // lightprobe
    ATTRIBUTE_PROBEBRIGHT =          0x029DC5B6, // probebright
    ATTRIBUTE_FASTIBL =              0x02559509, // fastibl
    ATTRIBUTE_PROBEHORIZON =         0x014DAAF5, // probehorizon
    ATTRIBUTE_PROBEFOV =             0x03D66903, // probefov
    ATTRIBUTE_LIGHTPROBE2 =          0x00430008, // lightprobe2
    ATTRIBUTE_PROBE2FADE =           0x02ED0742, // probe2fade
    ATTRIBUTE_PROBE2WINDOW =         0x016B224E, // probe2window
    ATTRIBUTE_PROBE2POS =            0x024B0C0E, // probe2pos
    ATTRIBUTE_DISABLEDEPTHTEST =     0x000B8353, // disabledepthtest
    ATTRIBUTE_DISABLEDEPTHPREPASS =  0x02AE1EA7, // disabledepthprepass
    ATTRIBUTE_TEXTCOLOR =            0x02D9114A, // textcolor
    ATTRIBUTE_TEXTCOLOR_R =          0x00E9F186, // textcolor.r
    ATTRIBUTE_TEXTCOLOR_G =          0x00DEEED1, // textcolor.g
    ATTRIBUTE_TEXTCOLOR_B =          0x00D9ED96, // textcolor.b
    ATTRIBUTE_SIZE =                 0x00F2C81F, // size
    ATTRIBUTE_FONT =                 0x03412331, // font
    ATTRIBUTE_TRACKING =             0x02A25049, // tracking
    ATTRIBUTE_LEADING =              0x016A6BDA, // leading
    ATTRIBUTE_RENDERSTYLE =          0x03567B85, // renderstyle
    ATTRIBUTE_TEXTSTRING =           0x01124062, // textstring
    ATTRIBUTE_BACKCOLOR_R =          0x0290CCE0, // backcolor.r
    ATTRIBUTE_BACKCOLOR_G =          0x0285CA2B, // backcolor.g
    ATTRIBUTE_BACKCOLOR_B =          0x0280C8F0, // backcolor.b
    ATTRIBUTE_TEXTTYPE =             0x0240ADD9, // texttype
    ATTRIBUTE_USEBACKCOLOR =         0x0243BACB, // usebackcolor
    ATTRIBUTE_WORDWRAP =             0x0134B04C, // wordwrap
    ATTRIBUTE_HORZSCROLL =           0x005B3CC4, // horzscroll
    ATTRIBUTE_HORZALIGN =            0x00BA002A, // horzalign
    ATTRIBUTE_VERTSCROLL =           0x00E8B8E2, // vertscroll
    ATTRIBUTE_VERTALIGN =            0x03759C8C, // vertalign
    ATTRIBUTE_BOXHEIGHT =            0x0079AF8E, // boxheight
    ATTRIBUTE_BOXWIDTH =             0x016B7105, // boxwidth
    ATTRIBUTE_REMOTESTRINGSOURCE =   0x025DFEEE, // remotestringsource
    ATTRIBUTE_CACHEDTEXTSTRING =     0x0095DBA0, // cachedtextstring
    ATTRIBUTE_ENABLEACCELERATEDFONT = 0x0053A92D, // enableacceleratedfont
    ATTRIBUTE_BEHAVIORSCRIPTS =      0x01DF916A, // BehaviorScripts
    ATTRIBUTE_UICCUSTOMOBJTYPE =     0x029F1BCF, // UICCustomObjType
    ATTRIBUTE_BGCOLORENABLE =        0x0021EE1F, // bgcolorenable
    ATTRIBUTE_BACKGROUND =           0x006AA932, // background
    ATTRIBUTE_BACKGROUNDCOLOR_R =    0x02AF0767, // backgroundcolor.r
    ATTRIBUTE_BACKGROUNDCOLOR_G =    0x02A404B2, // backgroundcolor.g
    ATTRIBUTE_BACKGROUNDCOLOR_B =    0x029F0377, // backgroundcolor.b
    ATTRIBUTE_PATHTYPE =             0x02D2A5E1, // pathtype
    ATTRIBUTE_LINEARERROR =          0x0378A51D, // linearerror
    ATTRIBUTE_EDGETESSAMOUNT =       0x02577E3A, // edgetessamount
    ATTRIBUTE_INNERTESSAMOUNT =      0x0027A241, // innertessamount
    ATTRIBUTE_BEGINCAP =             0x03373D37, // begincap
    ATTRIBUTE_BEGINCAPOFFSET =       0x01FEFE64, // begincapoffset
    ATTRIBUTE_BEGINCAPOPACITY =      0x02C2761E, // begincapopacity
    ATTRIBUTE_BEGINCAPWIDTH =        0x0102BDE3, // begincapwidth
    ATTRIBUTE_ENDCAP =               0x00ADB3A9, // endcap
    ATTRIBUTE_ENDCAPOFFSET =         0x0382A9D6, // endcapoffset
    ATTRIBUTE_ENDCAPOPACITY =        0x019BA72C, // endcapopacity
    ATTRIBUTE_ENDCAPWIDTH =          0x03A315F1, // endcapwidth
    ATTRIBUTE_PAINTSTYLE =           0x03ADEC8D, // paintstyle
    ATTRIBUTE_CLOSED =               0x01807034, // closed
    ATTRIBUTE_INCOMINGANGLE =        0x03890AB3, // incomingangle
    ATTRIBUTE_INCOMINGDISTANCE =     0x005EB2A5, // incomingdistance
    ATTRIBUTE_OUTGOINGDISTANCE =     0x017C597F, // outgoingdistance
    ATTRIBUTE_PARTICLETYPE =         0x01C01260, // particletype
    ATTRIBUTE_MAXPARTICLES =         0x00BE66B7, // maxparticles
    ATTRIBUTE_PARTICLESIZE =         0x02534279, // particlesize
    ATTRIBUTE_LIFETIME =             0x0033D297, // lifetime
    ATTRIBUTE_QT_IO =                0x010EF2CF, // qt.io
}; // enum EAttribute

#define AK_STRING_QT_IO "qt.io"

/// Function providing reverse hash lookup
const char *GetAttributeString(const EAttribute inAttribute);

} // namespace Q3DStudio

