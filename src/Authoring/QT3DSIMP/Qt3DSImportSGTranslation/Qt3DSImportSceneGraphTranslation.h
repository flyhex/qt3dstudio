/****************************************************************************
**
** Copyright (C) 1999-2009 NVIDIA Corporation.
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
// Includes
//==============================================================================
#pragma once

#include <vector>
#include "Qt3DSImportTranslation.h"
#include "Qt3DSImportTranslationCommon.h"

namespace qt3dsimp {

typedef enum _ETransformType {
    ETransformType_Matrix4x4,
    ETransformType_Rotation3, // rotation defined with 3 angles
    ETransformType_Rotation4, // rotation defined with an axis and angle
    ETransformType_Translate3,
    ETransformType_Scale3,
    ETransformType_Unknown
} ETransformType;

class INodeTransform
{
public:
    virtual ~INodeTransform() {}
    virtual operator float *() const = 0;
    virtual ETransformType GetType() const = 0;
};

class NodeTransform : public INodeTransform
{
public:
    NodeTransform(ETransformType inType)
    {
        m_Data = NULL;
        switch (inType) {
        case ETransformType_Matrix4x4:
            m_Data = new float[16];
            break;
        case ETransformType_Rotation3:
            m_Data = new float[3];
            break;
        case ETransformType_Rotation4:
            m_Data = new float[4];
            break;
        case ETransformType_Translate3:
            m_Data = new float[3];
            break;
        case ETransformType_Scale3:
            m_Data = new float[3];
            break;
        default:
            break;
        }

        m_Type = inType;
    }

    ~NodeTransform() { delete[] m_Data; }

    operator float *() const override { return m_Data; }

    ETransformType GetType() const override { return m_Type; }

    static void Delete(INodeTransform *inTransform) { delete inTransform; }

protected:
    float *m_Data;
    ETransformType m_Type;
};

typedef std::vector<INodeTransform *> TTransformList;

typedef enum _ETextureMapType {
    ETextureMapTypeDiffuse,
    ETextureMapTypeOpacity,
    ETextureMapTypeSpecular,
    ETextureMapTypeRoughness,
    ETextureMapTypeEmissive
} ETextureMapType;

typedef enum _EMatCommonProfileTechnique {
    EMatCommonProfileTechnique_Blinn,
    EMatCommonProfileTechnique_Constant,
    EMatCommonProfileTechnique_Lambert,
    EMatCommonProfileTechnique_Phong,
    EMatCommonProfileTechnique_Count
} EMatCommonProfileTechnique;

typedef enum _EMatOpaqueType // refer to domFx_opaque_enum
{ EMatOpaqueType_A_ONE, // When a transparent opaque attribute is set to A_ONE, it means the
                        // transparency information will be taken from the alpha channel of the
                        // color, texture, or parameter supplying the value. The value of 1.0 is
                        // opaque in this mode.
  EMatOpaqueType_RGB_ZERO, // When a transparent opaque attribute is set to RGB_ZERO, it means the
                           // transparency information will be taken from the red, green, and blue
                           // channels of the color, texture, or parameter supplying the value. Each
                           // channel is modulated independently. The value of 0.0 is opaque in this
                           // mode.
} EMatOpaqueType;

typedef struct _SFloatFlag
{
    float m_Value; // the value of this struct
    bool m_Flag; // to indicate if this value presents in SceneGraph
    _SFloatFlag()
        : m_Value(0.0f)
        , m_Flag(false)
    {
    }
    void SetValue(float inValue)
    {
        m_Value = inValue;
        m_Flag = true;
    }
} SFloatFlag;

typedef struct _SLongFlag
{
    long m_Value; // the value of this struct
    bool m_Flag; // to indicate if this value presents in SceneGraph
    _SLongFlag()
        : m_Value(0)
        , m_Flag(false)
    {
    }
    void SetValue(long inValue)
    {
        m_Value = inValue;
        m_Flag = true;
    }
} SLongFlag;

typedef struct _SColorOrTexture
{
    typedef enum _Type {
        None, // This information doesn't present in SceneGraph or of Param Type
        Color, // Color type
        Texture, // Texture type
    } Type;

    Type m_Type;
    float m_Color[4]; // the color, if present
    _SColorOrTexture()
        : m_Type(None)
    {
        m_Color[0] = m_Color[1] = m_Color[2] = m_Color[3] = 0.0f;
    }
    void SetColor(const float inColor[])
    {
        m_Color[0] = inColor[0];
        m_Color[1] = inColor[1];
        m_Color[2] = inColor[2];
        m_Color[3] = inColor[3];
        m_Type = Color;
    }
    void SetColor(float c0, float c1, float c2, float c3)
    {
        m_Color[0] = c0;
        m_Color[1] = c1;
        m_Color[2] = c2;
        m_Color[3] = c3;
        m_Type = Color;
    }
    void SetTexture() { m_Type = Texture; }
} SColorOrTexture;

//==============================================================================
// To store information regarding texture parameters which normally stored under <extra> tag
//==============================================================================
typedef struct _STextureParameters
{
    SFloatFlag m_coverageU;
    SFloatFlag m_coverageV;
    SFloatFlag m_repeatU;
    SFloatFlag m_repeatV;
    SFloatFlag m_offsetU;
    SFloatFlag m_offsetV;
    SFloatFlag m_rotateUV;
    SLongFlag m_wrapU;
    SLongFlag m_wrapV;
    SLongFlag m_mirrorU;
    SLongFlag m_mirrorV;
    bool m_Flag; // to indicate if dae contains texture parameters information
    _STextureParameters()
        : m_Flag(false)
    {
    }
} STextureParameters;

//==============================================================================
// To store information regarding material parameters which normally stored under <extra> tag
//==============================================================================
typedef struct _SMaterialExtraParameters
{
    SFloatFlag m_SpecLevel;
    SFloatFlag m_EmissionLevel;
    bool m_Flag; // to indicate if dae contains profile_COMMON technique extra information
    _SMaterialExtraParameters()
        : m_Flag(false)
    {
    }
} SMaterialExtraParameters;

//==============================================================================
// To store information regarding material parameters which stored under profile_COMMON technique
//==============================================================================
typedef struct _SMaterialParameters
{
    EMatCommonProfileTechnique m_TechniqueType;
    SColorOrTexture m_Emission;
    SColorOrTexture m_Ambient;
    SColorOrTexture m_Diffuse;
    SColorOrTexture m_Specular;
    SFloatFlag m_Shininess;
    SColorOrTexture m_Reflective;
    SFloatFlag m_Reflectivity;
    SColorOrTexture m_Transparent;
    SLongFlag m_TransparentOpaqueType;
    SFloatFlag m_Transparency;
    SFloatFlag m_Index_of_refraction;
    SMaterialExtraParameters m_Extra;

    _SMaterialParameters()
        : m_TechniqueType(EMatCommonProfileTechnique_Count)
    {
    }
    _SMaterialParameters(EMatCommonProfileTechnique inTechniqueType)
        : m_TechniqueType(inTechniqueType)
    {
    }
} SMaterialParameters;

//==============================================================================
// To store information regarding animation keyframes
//==============================================================================
typedef struct _SKeyframeParameters
{
    float m_KeyframeTime;
    float m_Value;
    float m_INTANGENTX;
    float m_INTANGENTY;
    float m_OUTTANGENTX;
    float m_OUTTANGENTY;

    _SKeyframeParameters(float inKeyframeTime, float inValue, float inINTANGENTX,
                         float inINTANGENTY, float inOUTTANGENTX, float inOUTTANGENTY)
        : m_KeyframeTime(inKeyframeTime)
        , m_Value(inValue)
        , m_INTANGENTX(inINTANGENTX)
        , m_INTANGENTY(inINTANGENTY)
        , m_OUTTANGENTX(inOUTTANGENTX)
        , m_OUTTANGENTY(inOUTTANGENTY)
    {
    }

} SKeyframeParameters;

//==============================================================================
// To store information regarding skelatal animations
//==============================================================================
typedef struct _SJointInfo
{
    int m_JointID;
    int m_ParentID;
    float m_invBindPose[16];
    float m_localToGlobalBoneSpace[16];

    _SJointInfo(int jointIndex, int parentID, float *invBindPose, float *localToGlobal)
        : m_JointID(jointIndex)
        , m_ParentID(parentID)
    {
        ::memcpy(m_invBindPose, invBindPose, sizeof(float) * 16);
        ::memcpy(m_localToGlobalBoneSpace, localToGlobal, sizeof(float) * 16);
    }

} SJointInfo;

//==============================================================================
// Templatized container for floats
//==============================================================================
template <size_t N>
struct _SVector
{
    float m_Data[N];

    bool operator<(const struct _SVector<N> &inVector) const
    {
        return ::memcmp(m_Data, inVector.m_Data, sizeof(m_Data)) < 0;
    }

    operator float *() { return m_Data; }

    operator const float *() const { return m_Data; }
};

typedef struct _SVector<4> SVector4;
typedef struct _SVector<3> SVector3;
typedef struct _SVector<2> SVector2;

class Import;

typedef enum _EAuthoringToolType {
    EAuthoringToolType_Unknown = 0,
    EAuthoringToolType_NextGen_Max,
    EAuthoringToolType_NextGen_Maya,
    EAuthoringToolType_FCollada_Max,
    EAuthoringToolType_FCollada_Maya,
    EAuthoringToolType_SketchUp,
    EAuthoringToolType_Cinema4D,
    EAuthoringToolType_StudioCORE,
    EAuthoringToolType_FBX_Max,
    EAuthoringToolType_FBX_Modo,
    EAuthoringToolType_FBX_Maya,
} EAuthoringToolType;

//==============================================================================
// Function interfaces for a translation class to allow much easier (and typed)
// data writing to the base import library.
//==============================================================================
class ISceneGraphTranslation
{
protected:
    virtual ~ISceneGraphTranslation() {}

public:
    virtual void Release() = 0;

    virtual void SetAuthoringTool(EAuthoringToolType inAuthoringToolType,
                                  long inAuthoringToolVersion) = 0;

    virtual void PushGroup(const char *inName) = 0;
    virtual void SetGroupSkeletonId(long inId) = 0;
    virtual void SetIgnoresParentTransform(bool inValue) = 0;
    virtual void PopGroup() = 0;
    virtual void PushModel(const char *inName) = 0;
    virtual void SetModelSkeletonRoot(long inId) = 0;
    virtual void PopModel() = 0;
    virtual void PushMaterial(const char *inName) = 0;
    virtual void PopMaterial(long inStartFaceIndex, long inNumberOfFaces) = 0;
    virtual void PushTexture(const char *inName, const char *inSourcePath, long inMapType) = 0;
    virtual void PopTexture() = 0;

    // Translation failed, mark top object as invalid.
    virtual void MarkInvalid() = 0;

    virtual void SetTransforms(const TTransformList &inTransforms) = 0;
    virtual void SetGeometry(TFloatsList &ioVertices, TFloatsList &ioNormals,
                             TFloatsList &ioTexCoords, TFloatsList &ioTexCoords2,
                             TFloatsList &ioTexTangents, TFloatsList &ioTexBinormals,
                             TFloatsList &ioWeights, TFloatsList &ioBoneIndex,
                             TFloatsList &ioColors, TLongsList &ioFaceIndicies) = 0;
    virtual void SetMaterial(const SMaterialParameters &inMaterialParameters) = 0;
    virtual void SetTexture(long inMapType, const STextureParameters &inTextureParameters) = 0;
    virtual void SetJointNode(SJointInfo &inJointInfo) = 0;

    virtual long CacheAnimationTrack() = 0;
    virtual void ApplyAnimationTrack(long inAnimationTrackIndex) = 0;
    virtual void SetAnimationTrack(const char *inBasePropertyName,
                                   const char *inSubPropertyName) = 0;
    virtual void CacheAnimationKey(const char *inBaseProperty, const char *inSubPropertyName,
                                   const SKeyframeParameters &inParameters) = 0;

    virtual void LogWarning(ESceneGraphWarningCode inWarningCode, const char *inAssociatedName) = 0;

    static ISceneGraphTranslation &CreateTranslation(Import &import, ISGTranslationLog &log);
};
}
