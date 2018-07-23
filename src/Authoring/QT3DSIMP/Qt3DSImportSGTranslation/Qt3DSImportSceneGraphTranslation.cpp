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

#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSImportSceneGraphTranslation.h"
#include "Qt3DSImportImpl.h"
#include "Qt3DSImportMesh.h"
#include "Matrix.h"
#include "Rotation3.h"
#include "EulerAngles.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "Qt3DSImportComposerTypes.h"
#include "CommonConstants.h"

#include <QtCore/qstring.h>

using namespace qt3dsimp;

namespace {

struct SubsetEntry
{
    long m_FaceStartIndex;
    long m_NumFaces;
    SubsetEntry(long off = 0, long numFace = 0)
        : m_FaceStartIndex(off)
        , m_NumFaces(numFace)
    {
    }
};

struct SGKeyframe
{
    QT3DSF32 m_Seconds;
    QT3DSF32 m_Value;
    QT3DSF32 m_EaseIn;
    QT3DSF32 m_EaseOut;
    SGKeyframe(QT3DSF32 sec, QT3DSF32 val, QT3DSF32 ei = 100.0f, QT3DSF32 eo = 100.0f)
        : m_Seconds(sec)
        , m_Value(val)
        , m_EaseIn(ei)
        , m_EaseOut(eo)
    {
    }
    SGKeyframe()
        : m_Seconds(0)
        , m_Value(0)
        , m_EaseIn(0)
        , m_EaseOut(0)
    {
    }
};

struct SGAnimationApplier
{
    virtual ~SGAnimationApplier() {}
    virtual void Apply(Import &inImport, const wchar_t *inInstanceId, QT3DSU32 subPropIndex,
                       EAnimationType inAnimType, NVConstDataRef<float> inData) = 0;
};

template <typename TDataType>
struct SSpecificSGAnimationApplier : public SGAnimationApplier
{
    SImportPropertyDefinition<TDataType> m_Property;
    SSpecificSGAnimationApplier(const SImportPropertyDefinition<TDataType> &inProp)
        : m_Property(inProp)
    {
    }
    void Apply(Import &inImport, const wchar_t *inInstanceId, QT3DSU32 subPropIndex,
                       EAnimationType inAnimType, NVConstDataRef<float> inData) override
    {
        inImport.AddAnimation(inInstanceId, m_Property, subPropIndex, inAnimType, inData);
    }
};

struct SGAnimation
{
    std::function<void(Import &, QT3DSU32, ImportArray<SGKeyframe> &)> m_AnimationApplier;
    ImportArray<SGKeyframe> m_Keyframes;
    QT3DSU32 m_SubPropIndex;

    std::shared_ptr<SGAnimationApplier> m_Applier;

    template <typename TDataType>
    void SetProperty(const SImportPropertyDefinition<TDataType> &inProperty)
    {
        m_Applier = std::make_shared<SSpecificSGAnimationApplier<TDataType>>(inProperty);
    }
    template <typename TDataType>
    void Handle(ComposerPropertyNames::Enum inName)
    {
        SetProperty(SImportPropertyDefinition<TDataType>(inName));
    }

    SGAnimation()
        : m_SubPropIndex(0)
    {
    }
    void AddKey(QT3DSF32 sec, QT3DSF32 val, QT3DSF32 ei = 100.0f, QT3DSF32 eo = 100.0f)
    {
        (void)ei, (void)eo;
        m_Keyframes.push_back(SGKeyframe(sec, val));
    }
};

static inline qt3dsdm::SValue ToColor(const float p[])
{
    return qt3dsdm::SFloat3(p[0], p[1], p[2]);
}
static inline qt3dsdm::SValue ToColor(const float p[], float inMult)
{
    return qt3dsdm::SFloat3(p[0] * inMult, p[1] * inMult, p[2] * inMult);
}
template <typename TDataType>
static inline TDataType GetMin(TDataType a, TDataType b)
{
    return qMin(a, b);
}
template <typename TDataType>
static inline TDataType GetMax(TDataType a, TDataType b)
{
    return qMax(a, b);
}
static void RGBtoHSV(float r, float g, float b, float *h, float *s, float *v)
{
    float min, max, delta;

    min = GetMin(GetMin(r, g), b);
    max = GetMax(GetMax(r, g), b);
    *v = max; // v
    delta = max - min;
    if (max != 0)
        *s = delta / max; // s
    else {
        // r = g = b = 0		// s = 0, v is undefined
        *s = 0;
        *h = -1;
        return;
    }
    if (r == max)
        *h = (g - b) / delta; // between yellow & magenta
    else if (g == max)
        *h = 2 + (b - r) / delta; // between cyan & yellow
    else
        *h = 4 + (r - g) / delta; // between magenta & cyan
    *h *= 60; // degrees
    if (*h < 0)
        *h += 360;
}

//==============================================================================
/**
 *	Swizzle the vertex value to match Studio format
 */
void SwizzleVertex(TFloatsList &ioValues)
{
    long theSize = (long)ioValues.size();
    for (long i = 0; i < theSize; i += 3) {
        // Negate z component
        ioValues[i + 2] = -ioValues[i + 2];
    }
}

//==============================================================================
/**
 *	Negate the vertex value
 */
void NegateVertex(TFloatsList &ioValues)
{
    long theSize = (long)ioValues.size();
    for (long i = 0; i < theSize; i += 3) {
        // Negate xyz component
        ioValues[i] = -ioValues[i];
        ioValues[i + 1] = -ioValues[i + 1];
        ioValues[i + 2] = -ioValues[i + 2];
    }
}

//==============================================================================
/**
 *	Swizzle the TexCoord value to match Studio format
 */
void SwizzleTexCoord(TFloatsList &ioValues)
{
    long theSize = (long)ioValues.size();
    for (long i = 0; i < theSize; i += 2) {
        // Flip y axis
        ioValues[i + 1] = 1 - ioValues[i + 1];
    }
}

//==============================================================================
/**
 *	Swizzle the Face Indices to match Studio format
 */
void SwizzleFaceIndices(TLongsList &ioValues)
{
    long theSize = (long)ioValues.size();
    long theTemp;
    for (long i = 0; i < theSize; i += 3) {
        // Swap x & z value
        theTemp = ioValues[i];
        ioValues[i] = ioValues[i + 2];
        ioValues[i + 2] = theTemp;
    }
}

//==============================================================================
/**
 *	Convert an axis angle rotation vector into a matrix representation.
 *	This code is modified from FCollada's FMMatrix44::AxisRotationMatrix
 */
void matrixFromAxisAngle(float inDegreeAngle, float inXAxis, float inYAxis, float inZAxis,
                         float inMatrix[4][4])
{
    // If axis or angle is zero, we're going to get a bad matrix, so bail.
    if (fabsf(inDegreeAngle) < 0.001f
        || (fabsf(inXAxis) < 0.001f && fabsf(inYAxis) < 0.001f && fabsf(inZAxis) < 0.001f)) {
        return;
    }

    inDegreeAngle *= (float)QT3DS_DEGREES_TO_RADIANS;

    Q3DStudio::CVector3 theAxis(inXAxis, inYAxis, inZAxis);
    if (theAxis.LengthSquared() != 1.0f)
        theAxis.Normalize();

    // Formulae inspired from
    // http://www.mines.edu/~gmurray/ArbitraryAxisRotation/ArbitraryAxisRotation.html
    float xSq = theAxis.x * theAxis.x;
    float ySq = theAxis.y * theAxis.y;
    float zSq = theAxis.z * theAxis.z;
    float cT = cosf(inDegreeAngle);
    float sT = sinf(inDegreeAngle);

    inMatrix[0][0] = xSq + (ySq + zSq) * cT;
    inMatrix[0][1] = theAxis.x * theAxis.y * (1.0f - cT) + theAxis.z * sT;
    inMatrix[0][2] = theAxis.x * theAxis.z * (1.0f - cT) - theAxis.y * sT;
    inMatrix[0][3] = 0;
    inMatrix[1][0] = theAxis.x * theAxis.y * (1.0f - cT) - theAxis.z * sT;
    inMatrix[1][1] = ySq + (xSq + zSq) * cT;
    inMatrix[1][2] = theAxis.y * theAxis.z * (1.0f - cT) + theAxis.x * sT;
    inMatrix[1][3] = 0;
    inMatrix[2][0] = theAxis.x * theAxis.z * (1.0f - cT) + theAxis.y * sT;
    inMatrix[2][1] = theAxis.y * theAxis.z * (1.0f - cT) - theAxis.x * sT;
    inMatrix[2][2] = zSq + (xSq + ySq) * cT;
    inMatrix[2][3] = 0;
    inMatrix[3][2] = inMatrix[3][1] = inMatrix[3][0] = 0;
    inMatrix[3][3] = 1;
}

#define FLT_TOLERANCE 0.0001f

template <class T>
T Sign(const T &val)
{
    return (val >= T(0)) ? T(1) : T(-1);
}

//==============================================================================
/**
 *	Get a 2x2 determinant
 */
static float det2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

//==============================================================================
/**
 *	Get the 3x3 determinant
 */
static float det3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2,
                    float c3)
{
    return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

//==============================================================================
/**
 *	Decomposes the scale portion of a matrix.
 *	Modified from FCollada's FMMatrix44::Decompose
 */
void DecomposeScale(Q3DStudio::CVector3 &outScale, float inMatrix[4][4])
{
    outScale.x = sqrtf(inMatrix[0][0] * inMatrix[0][0] + inMatrix[0][1] * inMatrix[0][1]
                       + inMatrix[0][2] * inMatrix[0][2]);
    outScale.y = sqrtf(inMatrix[1][0] * inMatrix[1][0] + inMatrix[1][1] * inMatrix[1][1]
                       + inMatrix[1][2] * inMatrix[1][2]);
    outScale.z = sqrtf(inMatrix[2][0] * inMatrix[2][0] + inMatrix[2][1] * inMatrix[2][1]
                       + inMatrix[2][2] * inMatrix[2][2]);

    float isInverted =
        Sign(det3x3(inMatrix[0][0], inMatrix[0][1], inMatrix[0][2], inMatrix[1][0], inMatrix[1][1],
                    inMatrix[1][2], inMatrix[2][0], inMatrix[2][1], inMatrix[2][2]));

    if (isInverted < 0.0f) {
        outScale.x = -outScale.x;
        outScale.y = -outScale.y;
        outScale.z = -outScale.z;
    }
}

static SFloat3 ConvertToSValue(const Q3DStudio::CVector3 &data)
{
    return SFloat3(data.x, data.y, data.z);
}
//==============================================================================
/**
*	Translate SubPropertyName to index.
*/
long GetSubPropertyIndex(const char *inSubPropertyName)
{
    if (qstricmp(inSubPropertyName, "x") == 0 || qstricmp(inSubPropertyName, "r") == 0)
        return 0;
    else if (qstricmp(inSubPropertyName, "y") == 0 || qstricmp(inSubPropertyName, "g") == 0)
        return 1;
    else if (qstricmp(inSubPropertyName, "z") == 0 || qstricmp(inSubPropertyName, "b") == 0)
        return 2;
    else if (qstricmp(inSubPropertyName, "a") == 0)
        return 3;
    return 0;
}

bool Replace(std::string &str, const char *find, const char *replace)
{
    std::string::size_type pos = str.find(find);
    if (pos != std::string::npos) {
        size_t len = strlen(find);
        str = str.replace(pos, len, replace);
        return true;
    }
    return false;
}

class SGTrans : public ISceneGraphTranslation, public SImportComposerTypes
{
    // uses relationship, not owns.
    Import &m_Import;
    ImportArray<TIMPHandle> m_InstanceStack;
    ImportHashMap<TIMPHandle, MeshBuilder *> m_Helpers;
    ImportHashSet<TIMPHandle> m_DuplicateMaterials;
    ImportArray<SGAnimation> m_Animations;
    ImportHashSet<TIMPHandle> m_Invalids;
    ISGTranslationLog &m_Log;
    MemoryBuffer<RawAllocator> m_TempBuffer;
    EAuthoringToolType m_AuthoringToolType;
    long m_AuthoringToolVersion;
    bool m_isDuplicatedMaterial;

public:
    SGTrans(Import &imp, ISGTranslationLog &log)
        : m_Import(imp)
        , m_Log(log)
        , m_AuthoringToolType(EAuthoringToolType_Unknown)
        , m_AuthoringToolVersion(0)
        , m_isDuplicatedMaterial(false)
    {
        TIMPHandle root =
            imp.CreateInstance(CImportTranslation::GetRootNodeId(), ComposerObjectTypes::Group);
        m_InstanceStack.push_back(root);
        m_Animations.reserve(100);
    }
    virtual ~SGTrans()
    {
        for (ImportHashMap<TIMPHandle, MeshBuilder *>::iterator iter = m_Helpers.begin(),
                                                                end = m_Helpers.end();
             iter != end; ++iter)
            iter->second->Release();
    }

    template <typename TDataType>
    TDataType *TempAlloc(QT3DSU32 size)
    {
        m_TempBuffer.reserve(size * sizeof(TDataType));
        return (TDataType *)m_TempBuffer.begin();
    }

    TCharPtr ToImport(const char *data)
    {
        if (data == NULL || *data == 0)
            return L"";

        // Convert from multi byte to wide character string
        QString s = QString::fromLocal8Bit(data);
        wchar_t *dataPtr(TempAlloc<wchar_t>(s.length() + 1));
        const long theWideBufferSize = s.toWCharArray(dataPtr);
        dataPtr[theWideBufferSize] = 0;
        return dataPtr;
    }

    TCharPtr ToImport(const wchar_t *data) { return data; }

    template <typename TNameType>
    void InternalPushObject(const TNameType *inName, ComposerObjectTypes::Enum inType)
    {
        if (m_InstanceStack.size() == 0) {
            MarkInvalid();
            return;
        }
        TCharPtr name(ToImport(inName));

        if (name == NULL || *name == 0)
            name = ComposerObjectTypes::Convert(inType);

        Q3DStudio::CFilePath objectName = Q3DStudio::CFilePath(name);
        Q3DStudio::CString normName = objectName.toCString();
        name = normName;

        TCharPtr nameStem = name;
        if (m_Import.FindInstanceById(name).hasValue()) {
            wchar_t nameBuf[1024];
            int idx = 1;
            do {
                swprintf(nameBuf, 1024, L"%ls_%04d", nameStem, idx);
                ++idx;
                name = nameBuf;
            } while (m_Import.FindAnyInstanceById(name)
                         .hasValue()); // don't use any instance which was already used valid or not
        }
        TIMPHandle inst = m_Import.CreateInstance(name, inType);
        m_Import.AddChild(m_InstanceStack.back(), inst);
        m_Import.SetInstancePropertyValue(inst, m_Asset.m_NameProp, nameStem);
        m_InstanceStack.push_back(inst);
    }

    void PushObject(const char *inName, ComposerObjectTypes::Enum inType)
    {
        InternalPushObject(inName, inType);
    }

    TIMPHandle PopObject()
    {
        TIMPHandle back = m_InstanceStack.back();
        m_InstanceStack.pop_back();
        if (m_Invalids.contains(back))
            return 0;
        return back;
    }

    void Release() override { delete this; }

    //==============================================================================
    /**
     *	Set Authoring Tool
     */
    void SetAuthoringTool(EAuthoringToolType inAuthoringToolType,
                                  long inAuthoringToolVersion) override
    {
        m_AuthoringToolType = inAuthoringToolType;
        m_AuthoringToolVersion = inAuthoringToolVersion;
    }

    virtual bool isFbxFormatType(EAuthoringToolType inAuthoringToolType)
    {
        return ((inAuthoringToolType == EAuthoringToolType_FBX_Max)
                || (inAuthoringToolType == EAuthoringToolType_FBX_Modo)
                || (inAuthoringToolType == EAuthoringToolType_FBX_Maya));
    }

    void PushGroup(const char *inName) override { PushObject(inName, ComposerObjectTypes::Group); }
    void SetGroupSkeletonId(long inId) override
    {
        TIMPHandle model = m_InstanceStack.back();
        m_Import.SetInstancePropertyValue(model, m_Node.m_BoneId, inId);
    }

    void SetIgnoresParentTransform(bool inValue) override
    {
        TIMPHandle model = m_InstanceStack.back();
        m_Import.SetInstancePropertyValue(model, m_Node.m_IgnoresParent, inValue);
    }

    void PopGroup() override { PopObject(); }
    void PushModel(const char *inName) override { PushObject(inName, ComposerObjectTypes::Model); }
    void SetModelSkeletonRoot(long inName) override
    {
        TIMPHandle model = m_InstanceStack.back();
        m_Import.SetInstancePropertyValue(model, m_Model.m_PoseRoot, inName);
    }
    void PopModel() override
    {
        TIMPHandle model = PopObject();
        if (model) {
            TCharPtr modelId = m_Import.GetInstanceByHandle(model)->m_Id;
            ImportHashMap<TIMPHandle, MeshBuilder *>::const_iterator entry = m_Helpers.find(model);
            if (entry != m_Helpers.end()) {
                entry->second->ConnectSubMeshes();
                entry->second->OptimizeMesh();
                Mesh &mesh = entry->second->GetMesh();
                CharPtrOrError meshData = m_Import.AddMesh(mesh, modelId);
                if (meshData.m_Value && *meshData.m_Value)
                    m_Import.SetInstancePropertyValue(model, m_Asset.m_SourcePath,
                                                      meshData.m_Value);
                if (meshData.m_Error)
                    m_Log.OnWarning(ESceneGraphWarningCode_LockedDestFile, meshData.m_Value);
            }
        }
    }
    void PushMaterial(const char *inName) override
    {
        // mark as not duplicated material
        m_isDuplicatedMaterial = false;
        // find if there exists already an instance of this material
        TCharPtr name(ToImport(inName));
        Option<InstanceDesc> inst = m_Import.FindInstanceById(name);

        if (inst.hasValue()) {
            if (m_DuplicateMaterials.contains(inst.getValue().m_Handle)) {
                m_isDuplicatedMaterial = true;
            }
        }

        PushObject(inName, ComposerObjectTypes::Material);

        // add to list if unique for this mesh instance and PushObject succeeded
        if (!m_isDuplicatedMaterial && !m_Invalids.contains(m_InstanceStack.back())) {
            // always add the original instance if exists because this is what we looking for
            if (inst.hasValue())
                m_DuplicateMaterials.insert(inst.getValue().m_Handle);
            else
                m_DuplicateMaterials.insert(m_InstanceStack.back());
        }
    }
    void PopMaterial(long inStartFaceIndex, long inNumberOfFaces) override
    {
        TIMPHandle material = m_InstanceStack.back();

        if (material) {
            // this is a duplicated material mark it invalid
            // this means it is not written to the import file
            if (m_isDuplicatedMaterial) {
                MarkInvalid();
            }

            PopObject();

            const wchar_t *theNameBuf = L"";
            Option<SValue> theName(
                m_Import.GetInstancePropertyValue(material, m_Asset.m_NameProp.GetName()));
            if (theName.hasValue()) {
                theNameBuf = qt3dsdm::get<TDataStrPtr>(*theName)->GetData();

                GetOrCreateMeshBuilder()->AddMeshSubset(
                    theNameBuf, inNumberOfFaces * 3,
                    inStartFaceIndex * 3, 0);
            }
        }
    }
    void PushTexture(const char * /*inName*/, const char *inSourcePath, long inMapType) override
    {
        TCharPtr source = m_Import.RegisterStr(ToImport(inSourcePath));
        // Ignore paths such as:
        //  /d:/something
        if (source[2] == ':') {
            if (source[0] == '\\' || source[0] == '/')
                ++source;
        }
        CharPtrOrError result = m_Import.AddImage(source);
        TCharPtr dest = result.m_Value;
        TCharPtr imgPath = NULL;
        wchar_t pathBuf[1024];
        if (result.m_Error) {
            ESceneGraphWarningCode code = ESceneGraphWarningCode_Generic;
            const wchar_t *errorStr = source;
            if (result.m_ErrorData.m_Error == ImportErrorCodes::SourceFileNotReadable)
                code = ESceneGraphWarningCode_MissingSourceFile;
            else if (result.m_ErrorData.m_Error == ImportErrorCodes::ResourceNotWriteable) {
                code = ESceneGraphWarningCode_LockedDestFile;
                errorStr = m_Import.RegisterStr(dest);
            }
            m_Log.OnWarning(code, errorStr);
        }
        if (result.m_Value != NULL && *result.m_Value)
            imgPath = result.m_Value;
        else {
            swprintf(pathBuf, 1024, L"%hs/missing.png",
                     m_Import.GetImageDir().toStdString().c_str());
            imgPath = pathBuf;
        }
        // We want to name the texture after the material id concatenated with the map type.
        // This ensures that we carry texture settings across for materials because the material
        // id doesn't change.
        TIMPHandle theMaterial = m_InstanceStack.back();
        std::wstring theId(m_Import.GetInstanceByHandle(theMaterial)->m_Id);
        theId.append(L"_");
        switch (inMapType) {
        case ETextureMapTypeDiffuse:
            theId.append(L"diffusemap");
            break;
        case ETextureMapTypeOpacity:
            theId.append(L"opacitymap");
            break;
        case ETextureMapTypeSpecular:
            theId.append(L"specularmap");
            break;
        case ETextureMapTypeEmissive:
            theId.append(L"emissivemap");
            break;
        case ETextureMapTypeRoughness:
            theId.append(L"roughnessmap");
            break;
        }
        InternalPushObject(theId.c_str(), ComposerObjectTypes::Image);
        TIMPHandle topObj = m_InstanceStack.back();
        m_Import.SetInstancePropertyValue(topObj, m_Image.m_SourcePath, imgPath);
    }

    void PopTexture() override { PopObject(); }

    void MarkInvalid() override
    {
        m_Invalids.insert(m_InstanceStack.back());
        m_Import.MarkInstanceInvalid(m_InstanceStack.back());
    }

    template <typename TPropertyType, typename TDataType>
    void SetInstancePropertyValue(TIMPHandle hdl,
                                  const SImportPropertyDefinition<TPropertyType> &name,
                                  const TDataType &value)
    {
        m_Import.SetInstancePropertyValue(hdl, name, value);
    }

    void SetMaterial(const SMaterialParameters &inMaterialParameters) override
    {
        TIMPHandle theMaterial = m_InstanceStack.back();

        if (inMaterialParameters.m_Diffuse.m_Type == SColorOrTexture::Texture)
            // Set diffuse color to 1.0 if there is diffuse texture because I think diffuse color is
            // multiplied with diffuse texture to get the final color
            SetInstancePropertyValue(theMaterial, m_Material.m_DiffuseColor,
                                     qt3dsdm::SFloat3(1.0f, 1.0f, 1.0f));
        else
            SetInstancePropertyValue(theMaterial, m_Material.m_DiffuseColor,
                                     qt3dsdm::SFloat3(inMaterialParameters.m_Diffuse.m_Color[0],
                                                    inMaterialParameters.m_Diffuse.m_Color[1],
                                                    inMaterialParameters.m_Diffuse.m_Color[2]));

        if (inMaterialParameters.m_Extra.m_EmissionLevel.m_Flag) {
            SetInstancePropertyValue(theMaterial, m_Material.m_EmissivePower,
                                     inMaterialParameters.m_Extra.m_EmissionLevel.m_Value * 100);
        } else {
            float theHSV[3];
            RGBtoHSV(inMaterialParameters.m_Emission.m_Color[0],
                     inMaterialParameters.m_Emission.m_Color[1],
                     inMaterialParameters.m_Emission.m_Color[2], &theHSV[0], &theHSV[1],
                     &theHSV[2]);
            SetInstancePropertyValue(theMaterial, m_Material.m_EmissivePower, theHSV[2] * 100);
        }

        if (inMaterialParameters.m_Shininess.m_Flag) {
            float theShininess = inMaterialParameters.m_Shininess.m_Value;
            if (inMaterialParameters.m_TechniqueType == EMatCommonProfileTechnique_Blinn) {
                // From Collada spec 1.4
                // To maximize application compatibility, it is suggested that developers use the
                // Blinn-Torrance-Sparrow for
                // <shininess> range of 0 to 1. For <shininess> greater than 1.0, the COLLADA author
                // was probably
                // using an application that followed the Blinn-Phong lighting model, so it is
                // recommended to support both
                // Blinn equations according to whichever range the shininess resides in.

                // We don't support Blinn-Torance-Sparrow, so the workaround is to multiply by 100
                // if it's within the range 0 to 1
                if (theShininess <= 1.0f)
                    theShininess *= 100.0f;
            }
            SetInstancePropertyValue(theMaterial, m_Material.m_SpecularRoughness, theShininess);
        }

        bool theSpecularEnable = false;
        if (theSpecularEnable)
            SetInstancePropertyValue(theMaterial, m_Material.m_SpecularAmount, 1.0f);
        else
            SetInstancePropertyValue(theMaterial, m_Material.m_SpecularAmount, 0.0f);

        // Opacity
        float theTransparency = 1.0f;
        if (inMaterialParameters.m_Transparency.m_Flag) {
            // This is true for MAX and MODO but not for MAYA
            if ((m_AuthoringToolType == EAuthoringToolType_FBX_Max)
                || (m_AuthoringToolType == EAuthoringToolType_FBX_Modo)) {
                // on FBX 0 is fully opaque and 1 is fully transparent
                // we treat it vice versa
                theTransparency = 1.0f - inMaterialParameters.m_Transparency.m_Value;
            } else {
                theTransparency = inMaterialParameters.m_Transparency.m_Value;
            }
        }
        float theTransparent = 1.0f;
        float theOpacity = theTransparency * theTransparent;
        if (inMaterialParameters.m_TransparentOpaqueType.m_Flag
            && inMaterialParameters.m_Transparent.m_Type == SColorOrTexture::Color) {
            if (inMaterialParameters.m_TransparentOpaqueType.m_Value == EMatOpaqueType_A_ONE) {
                theTransparent = inMaterialParameters.m_Transparent.m_Color[3];
                theOpacity = theTransparency * theTransparent;
            } else // EMatOpaqueType_RGB_ZERO
            {
                theTransparent = (inMaterialParameters.m_Transparent.m_Color[0]
                                  + inMaterialParameters.m_Transparent.m_Color[1]
                                  + inMaterialParameters.m_Transparent.m_Color[2])
                    / 3.0f;
                theOpacity = 1.0f - (theTransparency * theTransparent);
            }
        }
        SetInstancePropertyValue(theMaterial, m_Material.m_Opacity, theOpacity * 100.0f);

        SetInstancePropertyValue(theMaterial, m_Material.m_BlendMode, L"Normal");
    }
    void SetTexture(long inMapType, const STextureParameters &inTextureParameters) override
    {
        QT3DS_ASSERT(m_InstanceStack.size() > 2);

        TIMPHandle theImageInstance = m_InstanceStack.back(); // This refers to the texture
        m_InstanceStack.pop_back();
        m_InstanceStack.push_back(theImageInstance);

        // Create Image Instance at specified map property
        SImportPropertyDefinition<SLong4> *theProperty = &m_Material.m_DiffuseMap1;
        switch (inMapType) {
        case ETextureMapTypeDiffuse:
            theProperty = &m_Material.m_DiffuseMap1;
            break;
        case ETextureMapTypeOpacity:
            theProperty = &m_Material.m_OpacityMap;
            break;
        case ETextureMapTypeSpecular:
            theProperty = &m_Material.m_SpecularReflection;
            break;
        case ETextureMapTypeEmissive:
            theProperty = &m_Material.m_EmissiveMap;
            break;
        }
        if (m_InstanceStack.size() > 1) {
            // Set the map property.
            Option<InstanceDesc> theImage(m_Import.GetInstanceByHandle(theImageInstance));
            Option<InstanceDesc> theMaterial(
                m_Import.GetInstanceByHandle(m_InstanceStack[m_InstanceStack.size() - 2]));
            QT3DS_ASSERT(theImage.hasValue() && theMaterial.hasValue());
            if (theImage.hasValue() && theMaterial.hasValue()) {
                QT3DS_ASSERT(theMaterial->m_Type == ComposerObjectTypes::Material);
                m_Import.SetInstancePropertyValue(theMaterial->m_Handle, *theProperty,
                                                  theImage->m_Id);
            }
        }

        // Set the Image Instance properties
        if (inTextureParameters.m_Flag) {
            // Ok, this portion requires some explanation.
            // It seems that Coverage in Maya affects the Repeat in some ways.
            // The relation seems to be IDE Repeat = Maya Repeat / Maya Coverage for Coverage >= 1.0
            // However, if Coverage is less than 1.0, this doesn't seems to hold anymore....
            float theCoverageU = 1.0f;
            float theCoverageV = 1.0f;
            if (inTextureParameters.m_coverageU.m_Flag) {
                theCoverageU = inTextureParameters.m_coverageU.m_Value;
            }

            if (inTextureParameters.m_coverageV.m_Flag) {
                theCoverageV = inTextureParameters.m_coverageV.m_Value;
            }

            float theScaleU = 1.0f;
            if (inTextureParameters.m_repeatU.m_Flag) {
                theScaleU = inTextureParameters.m_repeatU.m_Value;
                SetInstancePropertyValue(theImageInstance, m_Image.m_RepeatU,
                                         theScaleU / theCoverageU);
            }

            float theScaleV = 1.0f;
            if (inTextureParameters.m_repeatV.m_Flag) {
                theScaleV = inTextureParameters.m_repeatV.m_Value;
                SetInstancePropertyValue(theImageInstance, m_Image.m_RepeatV,
                                         theScaleV / theCoverageV);
            }

            // From AMXMayaExporter: pivot values are ignored and defaults to 0.5
            const float theDefaultPivot = 0.5;
            // It seems that Coverage is affecting Pivot & Position

            SetInstancePropertyValue(theImageInstance, m_Image.m_PivotU,
                                     theDefaultPivot / theCoverageU);
            SetInstancePropertyValue(theImageInstance, m_Image.m_PivotV,
                                     theDefaultPivot / theCoverageV);

            // From AMXMayaExporter: do offset with pivotU and scaleU
            // maya does rotateUV about pivot(0.5,0.5) while position/scaleUV is done via (0,0)
            // then we gotta translate to Studio's way of UV pivot...
            if (inTextureParameters.m_offsetU.m_Flag) {
                SetInstancePropertyValue(
                    theImageInstance, m_Image.m_PositionU,
                    (theDefaultPivot - 1 + inTextureParameters.m_offsetU.m_Value * theCoverageU)
                        / theScaleU);
            }
            // From AMXMayaExporter: do offset with pivotV and scaleV...
            // Same explanation as for position U
            if (inTextureParameters.m_offsetV.m_Flag) {
                SetInstancePropertyValue(
                    theImageInstance, m_Image.m_PositionV,
                    (theDefaultPivot - 1 + inTextureParameters.m_offsetV.m_Value * theCoverageV)
                        / theScaleV);
            }

            if (inTextureParameters.m_rotateUV.m_Flag) {
                SetInstancePropertyValue(theImageInstance, m_Image.m_RotationUV,
                                         inTextureParameters.m_rotateUV.m_Value);
            }

            // tiling mode U
            std::wstring theHorizontalTilingMode = L"Tiled"; // tiled
            if (inTextureParameters.m_wrapU.m_Flag) {
                if (inTextureParameters.m_wrapU.m_Value == 0)
                    theHorizontalTilingMode = L"No Tiling"; // not tiled
            }
            if (inTextureParameters.m_mirrorU.m_Flag) {
                if (inTextureParameters.m_mirrorU.m_Value == 1)
                    theHorizontalTilingMode = L"Mirrored"; // mirrored
            }

            SetInstancePropertyValue(theImageInstance, m_Image.m_TilingU,
                                     theHorizontalTilingMode.c_str());

            // tiling mode V
            std::wstring theVerticalTilingMode = L"Tiled"; // tiled
            if (inTextureParameters.m_wrapV.m_Flag) {
                if (inTextureParameters.m_wrapV.m_Value == 0)
                    theVerticalTilingMode = L"No Tiling"; // not tiled
            }
            if (inTextureParameters.m_mirrorV.m_Flag) {
                if (inTextureParameters.m_mirrorV.m_Value == 1)
                    theVerticalTilingMode = L"Mirrored"; // mirrored
            }

            SetInstancePropertyValue(theImageInstance, m_Image.m_TilingV,
                                     theVerticalTilingMode.c_str());
        }
    }

    void SetTransforms(const TTransformList &inTransforms) override
    {
        TIMPHandle theNode = m_InstanceStack.back();

        Q3DStudio::CMatrix theTransformStack;
        theTransformStack.Identity();

        TTransformList::const_iterator theIter = inTransforms.begin();
        TTransformList::const_iterator theEnd = inTransforms.end();
        for (; theIter != theEnd; ++theIter) {
            const INodeTransform *theTransform = *theIter;

            Q3DStudio::CMatrix theMatrix;

            switch ((*theIter)->GetType()) {
            case ETransformType_Matrix4x4: {
                Q3DStudio::CMatrix::TArray &theArray = theMatrix.GetArray();

                for (long theRow = 0; theRow < 4; ++theRow)
                    for (long theColumn = 0; theColumn < 4; ++theColumn)
                        theArray[theColumn][theRow] = (*theTransform)[(theRow * 4) + theColumn];
            } break;
            case ETransformType_Rotation3: {
                // this a rotation is defined by 3 angles
                Q3DStudio::CRotation3 rotation;
                rotation.SetAngles((*theTransform)[0], (*theTransform)[1], (*theTransform)[2]);
                theMatrix.Rotate(rotation);
            } break;
            case ETransformType_Rotation4: {
                // EulerAngleConverter::CEulerAngleConverter theConverter;
                // EulerAngleConverter::EulerAngles theAxisAngle ={ ( *theTransform )[0], (
                // *theTransform )[1], ( *theTransform )[2], ( *theTransform )[3] *
                // (float)Q3DStudio::QT3DS_DEGREES_TO_RADIANS };
                // theConverter.Eul_ToHMatrix( theAxisAngle, theMatrix.GetArray( ) );
                matrixFromAxisAngle((*theTransform)[3], (*theTransform)[0], (*theTransform)[1],
                                    (*theTransform)[2], theMatrix.GetArray());
            } break;
            case ETransformType_Translate3: {
                theMatrix.SetTranslate(Q3DStudio::CVector3((*theTransform)[0], (*theTransform)[1],
                                                           (*theTransform)[2]));
            } break;
            case ETransformType_Scale3: {
                theMatrix.SetScale(Q3DStudio::CVector3((*theTransform)[0], (*theTransform)[1],
                                                       (*theTransform)[2]));
            } break;
            default:
                break;
            }

            theTransformStack = theMatrix * theTransformStack;
        }

        // Get out the rotation
        EulerAngleConverter::CEulerAngleConverter theConverter;
        EulerAngleConverter::EulerAngles theEulerAngles =
            theConverter.Eul_FromHMatrix(theTransformStack.GetArray(), EulOrdXYZr);
        Q3DStudio::CRotation3 theRotation;
        theRotation.SetRadians(-theEulerAngles.x, -theEulerAngles.y, -theEulerAngles.z);

        Q3DStudio::CVector3 theTranslation = theTransformStack.GetTranslation();
        theTranslation.z *= -1;

        Q3DStudio::CVector3 theScale;
        DecomposeScale(theScale, theTransformStack.GetArray());

        SFloat3 theRotationAngles;
        theRotation.GetAngles(theRotationAngles[0], theRotationAngles[1], theRotationAngles[2]);

        SetInstancePropertyValue(theNode, m_Node.m_Rotation, theRotationAngles);
        SetInstancePropertyValue(theNode, m_Node.m_Position, ConvertToSValue(theTranslation));
        SetInstancePropertyValue(theNode, m_Node.m_Scale, ConvertToSValue(theScale));

        SetInstancePropertyValue(theNode, m_Node.m_Orientation, L"Right Handed");
        SetInstancePropertyValue(theNode, m_Node.m_RotationOrder, L"XYZr");
    }

    MeshBuilder *GetOrCreateMeshBuilder()
    {
        ImportHashMap<TIMPHandle, MeshBuilder *>::iterator entry =
            m_Helpers.find(m_InstanceStack.back());
        if (entry != m_Helpers.end())
            return entry->second;
        MeshBuilder *newHelper = &MeshBuilder::CreateMeshBuilder();
        m_Helpers.insert(eastl::make_pair(m_InstanceStack.back(), newHelper));
        return newHelper;
    }

    template <typename TDataType>
    static NVConstDataRef<QT3DSU8> toRef(const std::vector<TDataType> &data)
    {
        const TDataType *dataPtr = data.size() ? &data[0] : NULL;
        return toU8ConstDataRef(dataPtr, (QT3DSU32)data.size());
    }

    static inline MeshBuilderVBufEntry ToEntry(const TFloatsList &data, const char *name,
                                               QT3DSU32 numComponents)
    {
        return MeshBuilderVBufEntry(name, toRef(data), NVRenderComponentTypes::QT3DSF32,
                                    numComponents);
    }

    std::vector<QT3DSU16> m_ShortIndicies;
    std::vector<QT3DSU32> m_UIntIndicies;

    // We used to swizzle vertexes, which was intended to go from right to left handed coordinates.
    // But the mesh that we save out gets loaded in its native format by the runtime.
    // Thus we shouldn't here, but swizzle it when it is loaded into studio.
    void SetGeometry(TFloatsList &ioVertices, TFloatsList &ioNormals,
                             TFloatsList &ioTexCoords, TFloatsList &ioTexCoords2,
                             TFloatsList &ioTexTangents, TFloatsList &ioTexBinormals,
                             TFloatsList &ioWeights, TFloatsList &ioBoneIndex,
                             TFloatsList &ioColors, TLongsList &ioFaceIndicies) override
    {
        // we clear our duplicate materials for each new geometry
        m_DuplicateMaterials.clear();

        MeshBuilder *theModel = GetOrCreateMeshBuilder();
        ////////////////////////////////////////// To Fix Bug 4043
        /////////////////////////////////////////////
        // TexTangents exported from ColladaMaya 3.05C or ColladaMax 3.05C are inverted!
        // But ColladaMaya 3.02 & NextGen 0.9.5 are not inverted
        if (m_AuthoringToolType == EAuthoringToolType_FCollada_Max
            || m_AuthoringToolType == EAuthoringToolType_FCollada_Maya) {
            if (m_AuthoringToolVersion == 305) // TODO: Test with version 303 and 304
            {
                NegateVertex(ioTexTangents);
            }
        }
        ///////////////////////////////////////////// End fix
        //////////////////////////////////////////////////

        bool valid = true;
        NVRenderComponentTypes::Enum theIndexCompType = NVRenderComponentTypes::QT3DSU16;

        // check if we exceed 65k vertex count
        // ioVertices is a float array of xyz per vertex
        if (ioVertices.size() / 3 > QT3DS_MAX_U16) {
            theIndexCompType = NVRenderComponentTypes::QT3DSU32;
        }

        QT3DSU32 theIndexCompSize = NVRenderComponentTypes::getSizeofType(theIndexCompType);

        MeshBuilderVBufEntry entries[] = {
            ToEntry(ioVertices, Mesh::GetPositionAttrName(), 3),
            ToEntry(ioNormals, Mesh::GetNormalAttrName(), 3),
            ToEntry(ioTexCoords, Mesh::GetUVAttrName(), 2),
            ToEntry(ioTexCoords2, Mesh::GetUV2AttrName(), 2),
            ToEntry(ioTexTangents, Mesh::GetTexTanAttrName(), 3),
            ToEntry(ioTexBinormals, Mesh::GetTexBinormalAttrName(), 3),
            ToEntry(ioWeights, Mesh::GetWeightAttrName(), 4),
            ToEntry(ioBoneIndex, Mesh::GetBoneIndexAttrName(), 4),
            ToEntry(ioColors, Mesh::GetColorAttrName(), 3),
        };
        valid = valid && theModel->SetVertexBuffer(toDataRef(entries, 9));

        if (!valid) {
            valid = false;
        } else if (theIndexCompSize == 2) {
            m_ShortIndicies.resize(ioFaceIndicies.size());
            for (size_t idx = 0, end = ioFaceIndicies.size(); idx < end; ++idx)
                m_ShortIndicies[idx] = static_cast<QT3DSU16>(ioFaceIndicies[idx]);
            if (m_ShortIndicies.size())
                theModel->SetIndexBuffer(
                    NVDataRef<QT3DSU8>(reinterpret_cast<QT3DSU8 *>(&m_ShortIndicies[0]),
                                    (QT3DSU32)m_ShortIndicies.size() * theIndexCompSize),
                    theIndexCompType);
            else
                valid = false;
        } else if (theIndexCompSize == 4) {
            m_UIntIndicies.resize(ioFaceIndicies.size());
            for (size_t idx = 0, end = ioFaceIndicies.size(); idx < end; ++idx)
                m_UIntIndicies[idx] = static_cast<QT3DSU32>(ioFaceIndicies[idx]);
            if (m_UIntIndicies.size())
                theModel->SetIndexBuffer(
                    NVDataRef<QT3DSU8>(reinterpret_cast<QT3DSU8 *>(&m_UIntIndicies[0]),
                                    (QT3DSU32)m_UIntIndicies.size() * theIndexCompSize),
                    theIndexCompType);
            else
                valid = false;
        }

        if (!valid) {
            MarkInvalid();
            theModel->SetIndexBuffer(NVDataRef<QT3DSU8>(), theIndexCompType);
        }
    }

    void SetJointNode(SJointInfo &inJointInfo) override
    {
        MeshBuilder *theModel = GetOrCreateMeshBuilder();
        theModel->AddJoint(inJointInfo.m_JointID, inJointInfo.m_ParentID, inJointInfo.m_invBindPose,
                           inJointInfo.m_localToGlobalBoneSpace);
    }

    long CacheAnimationTrack() override
    {
        m_Animations.push_back(SGAnimation());
        return (QT3DSI32)m_Animations.size() - 1;
    }
    void ApplyAnimationTrack(long inAnimationTrackIndex) override
    {
        if (inAnimationTrackIndex >= (long)m_Animations.size() || m_InstanceStack.size() == 0) {
            QT3DS_ASSERT(false);
            return;
        }
        SGAnimation &anim = m_Animations[inAnimationTrackIndex];
        if (!anim.m_Applier) {
            QT3DS_ASSERT(false);
            return;
        }
        TIMPHandle instance(m_InstanceStack.back());
        InstanceDesc theDesc(m_Import.GetInstanceByHandle(instance));
        TCharPtr instId = theDesc.m_Id;
        anim.m_Applier->Apply(
            m_Import, instId, anim.m_SubPropIndex, EAnimationTypeEaseInOut,
            toConstDataRef((float *)anim.m_Keyframes.begin(), (QT3DSU32)anim.m_Keyframes.size() * 4));
    }

    void CacheAnimationKey(const char *inBaseProperty, const char *inSubPropertyName,
                                   const SKeyframeParameters &inParameters) override
    {
        // Hardcode to use SEaseInEaseOutKeyframe. It seems that Studio needs this in order to
        // animate properly.
        // (I tried using SBezierKeyframe, but Studio doesn't Animate properly).
        SGKeyframe theKeyframe(inParameters.m_KeyframeTime, inParameters.m_Value);

        // Negate if we are animating position.z
        if (isFbxFormatType(m_AuthoringToolType)) {
            if (qstricmp(inBaseProperty, "T") == 0 && qstricmp(inSubPropertyName, "Z") == 0)
                theKeyframe.m_Value = -theKeyframe.m_Value;
        } else {
            if (qstricmp(inBaseProperty, "translate") == 0
                && qstricmp(inSubPropertyName, "z") == 0)
                theKeyframe.m_Value = -theKeyframe.m_Value;
        }

        m_Animations.back().m_Keyframes.push_back(theKeyframe);
    }

    void SetAnimationTrack(const char *inBasePropertyName, const char *inSubPropertyName) override
    {
        if (m_Animations.size() == 0) {
            QT3DS_ASSERT(false);
            return;
        }

        SGAnimation &theTrack = m_Animations.back();

        // Extract if there is any SubPropertyName merged into inBasePropertyName.
        std::string theBasePropertyName(inBasePropertyName);
        std::string theSubPropertyName(inSubPropertyName);
        std::string::size_type thePos = theBasePropertyName.rfind('.');
        if (thePos != std::string::npos) {
            theSubPropertyName = theBasePropertyName.substr(thePos + 1);
            theBasePropertyName = theBasePropertyName.substr(0, thePos);
        }

        std::string theStudioPropertyName(theBasePropertyName.c_str());

        if (isFbxFormatType(m_AuthoringToolType)) {
            // Translate FBX element names into actual property names for Studio
            (Replace(theStudioPropertyName, "T", "position") != 0)
                || (Replace(theStudioPropertyName, "R", "rotation") != 0)
                || (Replace(theStudioPropertyName, "S", "scale") != 0);
        } else {
            // Translate COLLADA element names into actual property names for Studio
            (Replace(theStudioPropertyName, "translate", "position") != 0)
                || (Replace(theStudioPropertyName, "rotate", "rotation") != 0)
                || (Replace(theStudioPropertyName, "transparency", "opacity") != 0);
        }

        long idx = GetSubPropertyIndex(theSubPropertyName.c_str());
        theTrack.m_SubPropIndex = idx;
        Q3DStudio::CString theName(theStudioPropertyName.c_str());
        ComposerPropertyNames::Enum thePropertyName(ComposerPropertyNames::Convert(theName));
        ImportVisitPropertyType(thePropertyName, theTrack);
    }

    void LogWarning(ESceneGraphWarningCode inWarningCode, const char *inAssociatedName) override
    {
        m_Log.OnWarning(inWarningCode, m_Import.RegisterStr(ToImport(inAssociatedName)));
    }
};
}

ISceneGraphTranslation &ISceneGraphTranslation::CreateTranslation(Import &import,
                                                                  ISGTranslationLog &log)
{
    return *new SGTrans(import, log);
}
