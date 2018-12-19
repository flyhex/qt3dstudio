/****************************************************************************
**
** Copyright (C) 1999-2012 NVIDIA Corporation.
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

#include "qtAuthoring-config.h"

#ifdef QT_3DSTUDIO_FBX

#include "Qt3DSCommonPrecompile.h"

#define FBXSDK_NEW_API 1

#include <tuple>
#include <memory>
#include <map>
#include "Qt3DSFBXSDK.h"
#include "Qt3DSImportFbxDomUtils.h"
#include "Qt3DSImportSceneGraphTranslation.h"
#include "foundation/Qt3DSVec3.h"
#include <unordered_set>
#include <unordered_map>

using namespace qt3dsimp;

namespace {

#define IOSREF FbxIOSettings::IOSettingsRef()

typedef std::unordered_set<FbxNode *> TNodeSet;

//==============================================================================
//	Loads FBX document and processes it.
//==============================================================================

class FbxDomWalker
{
public:
    FbxDomWalker(ISceneGraphTranslation *inTranslation);
    ~FbxDomWalker();

public:
    typedef std::vector<SFaceMaterialInfo> TFaceMaterialIndices;
    typedef std::vector<_SVertexWeightInfo> TPerVertexWeightInfo;
    typedef std::vector<_SJointInfo> TJointInfoList;
    typedef std::tuple<SVector3, SVector3, SVector2, SVector3, SVector3, SVector4, SVector4,
                       SVector2, SVector3>
        TVertexInfoTuple;
    typedef std::pair<std::map<TVertexInfoTuple, long>,
                      std::vector<std::pair<std::string, TLongsList>>>
        TFaceIndicies;

    typedef std::vector<TPerVertexWeightInfo> TVertexWeigthList;
    typedef std::vector<FbxAnimCurve *> TAnimCurveList;
    typedef std::multimap<const FbxNode *, long> TNodeToIndicesMap;
    typedef std::unordered_map<const FbxNode *, bool> TNodeIsAnimatedMap;
    typedef std::unordered_map<const FbxNode *, long> TJointNodeHierarchyMap;
    typedef std::unordered_set<const FbxNode *> TJointNodeRootSet;

public:
    bool LoadDocument(const std::string &inFilePath);
    void ProcessScene();
    void ProcessAnimationStacks();

protected:
    template <typename TNodeFilter>
    void FilterNodeHierarchy(TNodeFilter inFilter, TNodeSet &inSet);
    void ProcessNode(FbxNode *inFbxNode);
    void ProcessGroup(FbxNode *inFbxNode);
    void ProcessLight(FbxNode *inFbxNode);
    void ProcessCamera(FbxNode *inFbxNode);
    void ProcessMesh(FbxNode *inFbxNode);
    void ProcessNodeChildren(FbxNode *inFbxNode);
    long ProcessSkeletonNode(FbxNode *inFbxNode, TNodeSet &inNodeSet, bool inIsRoot);
    void ProcessTransform(FbxNode *inFbxNode, bool ignoreScale = false);
    void ReadVertex(const FbxVector4 *inFbxCtrlPoints, int inFbxCtrlPointIndex, float *outValue,
                    FbxAMatrix &geometricTransformation);
    void ReadVertex(const TPerVertexWeightInfo &inFbxWeights, int inFbxCtrlPointIndex,
                    float *outValue);
    void ReadNormal(const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex, float *outValue);
    void ReadColor(const FbxGeometryElementVertexColor *inFbxColors, int inFbxIndex,
                   float *outValue);
    void ReadTexCoord(const FbxGeometryElementUV *inFbxUVs, int inFbxIndex, float *outValue);
    void ReadTexTangent(const FbxGeometryElementTangent *inFbxTangents, int inFbxIndex,
                        float *outValue);
    void ReadTexTangent(std::vector<SVector3> &inTangentPoints,
                        const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex,
                        float *outValue);
    void ReadTexBinormal(const FbxGeometryElementBinormal *inFbxBinormals, int inFbxIndex,
                         float *outValue);
    void ReadTexBinormal(std::vector<SVector3> &inBinormalPoints,
                         const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex,
                         float *outValue);
    void ReadWeight(const TPerVertexWeightInfo &inFbxWeights, int inFbxIndex, float *outValue);
    void ReadBoneIndex(const TPerVertexWeightInfo &inFbxWeights, int inFbxIndex, float *outValue,
                       const TJointInfoList &inAbsoluteJoints);
    void ReadMaterial(const FbxSurfaceMaterial *inMaterial, const int startFace,
                      const int faceCount);
    void ReadDefaultMaterial(const FbxNode *pNode, const int startFace, const int faceCount);
    void ProcessTextures(const FbxSurfaceMaterial *inMaterial, const char *fbxMatType,
                         ETextureMapType sgMapType, SColorOrTexture &outColor);
    void ProcessTextureParameter(FbxTexture *inTexture, STextureParameters &outTextureParameters);
    void ProcessAnimLayer(FbxNode *inNode, FbxAnimLayer *inAnimLayer);
    void ProcessAnimCurve(FbxAnimCurve *inAnimCurve, const char *transformType,
                          const char *channelName);
    // skeletal animation related
    long GetJointNodeID(const FbxNode *inFbxNode);
    long GetOrCreateJointNodeID(const FbxNode *inFbxNode);
    int GetParentJointID(const FbxNode *inFbxNode);
    void AddJointNode(int jointID, int parentID, FbxAMatrix &invBindPose,
                      FbxAMatrix &localToGlobalBoneSpace, TJointInfoList &outJointInfoList);

    // mesh pre-processing
    void GenerateMeshTangents(const FbxMesh *inFbxMesh, std::vector<SVector3> &outTangents,
                              std::vector<SVector3> &outBinormals);

    void QueryMaterialInfo(FbxMesh *theFbxMesh, SFaceMaterialInfo *faceMatInfo);
    // mesh deformer related
    void GetVertexWeights(FbxMesh *theFbxMesh, TVertexWeigthList &outWeights,
                          TJointInfoList &outJointInfoList, TNodeSet &outBoneSet);

    // Animation related
    bool ApplyAnimation(const FbxNode *inNode);
    bool IsAnimated(const FbxNode *inNode);
    void GetIndicesFromNode(const FbxNode *inNode, TLongsList &outIndicies);
    bool CurveNodeIsAnimated(FbxAnimCurveNode *inAnimCurveNode);
    void FilterAnimCurve(FbxAnimCurve *inAnimCurve, std::vector<int> &outKeyIndexList);

protected:
    FbxManager *m_FbxManager;
    FbxScene *m_FbxScene;
    ISceneGraphTranslation *m_Translator;
    long m_AnimationTrackCount;
    TNodeToIndicesMap m_NodeToIndicies;
    TNodeIsAnimatedMap m_NodeIsAnimatedMap;
    TJointNodeHierarchyMap m_JointNodeHierarchyMap;
    TNodeSet m_importNodes;
    EAuthoringToolType m_AuthoringToolType;
};

FbxDomWalker::FbxDomWalker(ISceneGraphTranslation *inTranslation)
    : m_FbxManager(nullptr)
    , m_FbxScene(nullptr)
    , m_Translator(inTranslation)
    , m_AnimationTrackCount(0)
    , m_AuthoringToolType(EAuthoringToolType_Unknown)
{
}

FbxDomWalker::~FbxDomWalker()
{
    if (m_FbxScene != nullptr)
        m_FbxScene->Destroy();
    if (m_FbxManager != nullptr)
        m_FbxManager->Destroy();
}

/**
* @brief Load a FBX document
*
* @return load status
*/
bool FbxDomWalker::LoadDocument(const std::string &inFilePath)
{
    if (QFile::exists(QString::fromStdString(inFilePath))) {
        m_FbxManager = FbxManager::Create();

        FbxIOSettings *theIOSettings = FbxIOSettings::Create(m_FbxManager, IOSROOT);
        m_FbxManager->SetIOSettings(theIOSettings);

        m_FbxScene = FbxScene::Create(m_FbxManager, "");

        FbxImporter *theImporter = FbxImporter::Create(m_FbxManager, "");
        if (!theImporter->Initialize(inFilePath.c_str(), -1, m_FbxManager->GetIOSettings())) {
            theImporter->Destroy();
            return false;
        }

        int major = 0, minor = 0, revision = 0;
        theImporter->GetFileVersion(major, minor, revision);

        if (theImporter->IsFBX()) {
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
        }

        bool theStatus = theImporter->Import(m_FbxScene);
        if (theStatus) {
            // get authoring tool
            FbxDocumentInfo *pDocInfo = m_FbxScene->GetDocumentInfo();
            if (pDocInfo) {
                const char *appName = "3ds Max";
                FbxString appString;

                // this should give us the latest used app on this file
                FbxProperty lastSavedAppProp = pDocInfo->LastSaved_ApplicationName;
                if (lastSavedAppProp.IsValid()) {
                    FbxDataType lPropertyDataType = lastSavedAppProp.GetPropertyDataType();

                    if (lPropertyDataType.GetType() == eFbxString) {
                        appString = lastSavedAppProp.Get<FbxString>();
                        appName = appString.Buffer();
                    }
                }

                if (!strcmp(appName, "")) {
                    // otherwise try to get original app name
                    FbxProperty originAppProp = pDocInfo->Original_ApplicationName;
                    if (originAppProp.IsValid()) {
                        FbxDataType lPropertyDataType = originAppProp.GetPropertyDataType();

                        if (lPropertyDataType.GetType() == eFbxString) {
                            appString = originAppProp.Get<FbxString>();
                            appName = appString.Buffer();
                        }
                    }
                }

                if (!strcmp(appName, "3ds Max")) {
                    m_Translator->SetAuthoringTool(EAuthoringToolType_FBX_Max,
                                                   major * 1000 + minor * 100 + revision);
                    m_AuthoringToolType = EAuthoringToolType_FBX_Max;
                } else if (!strcmp(appName, "modo")) {
                    m_Translator->SetAuthoringTool(EAuthoringToolType_FBX_Modo,
                                                   major * 1000 + minor * 100 + revision);
                    m_AuthoringToolType = EAuthoringToolType_FBX_Modo;
                } else if (!strcmp(appName, "Maya")) {
                    m_Translator->SetAuthoringTool(EAuthoringToolType_FBX_Maya,
                                                   major * 1000 + minor * 100 + revision);
                    m_AuthoringToolType = EAuthoringToolType_FBX_Maya;
                } else if (strstr(appName, "Blender")) {
                    qWarning("Importing from Blender. Light and Camera rotations may be incorrect");
                    m_Translator->SetAuthoringTool(EAuthoringToolType_FBX_Blender,
                                                   major * 1000 + minor * 100 + revision);
                    m_AuthoringToolType = EAuthoringToolType_FBX_Blender;
                } else {
                    m_Translator->SetAuthoringTool(EAuthoringToolType_FBX_Max,
                                                   major * 1000 + minor * 100 + revision);
                    m_AuthoringToolType = EAuthoringToolType_FBX_Max;
                }
            } else {
                m_Translator->SetAuthoringTool(EAuthoringToolType_FBX_Max,
                                               major * 1000 + minor * 100 + revision);
                m_AuthoringToolType = EAuthoringToolType_FBX_Max;
            }
        }

        theImporter->Destroy();

        return theStatus;
    }

    return false;
}

inline void AddParentsToSet(FbxNode &inNode, TNodeSet &ioSet)
{
    FbxNode *theParent = inNode.GetParent();
    while (theParent && ioSet.find(theParent) == ioSet.end()) {
        ioSet.insert(theParent);
        theParent = theParent->GetParent();
    }
}

template <typename TNodeFilter>
void DoFilterNodeHierarchy(TNodeFilter inFilter, TNodeSet &ioSet, FbxNode &inNode)
{
    if (inFilter(inNode) && ioSet.find(&inNode) == ioSet.end()) {
        ioSet.insert(&inNode);
        AddParentsToSet(inNode, ioSet);
    }
    int childCount = inNode.GetChildCount();
    for (int idx = 0; idx < childCount; ++idx) {
        FbxNode *theNode = inNode.GetChild(idx);
        DoFilterNodeHierarchy(inFilter, ioSet, *theNode);
    }
}

template <typename TNodeFilter>
void FbxDomWalker::FilterNodeHierarchy(TNodeFilter inFilter, TNodeSet &inSet)
{
    if (m_FbxScene->GetRootNode())
        DoFilterNodeHierarchy(inFilter, inSet, *m_FbxScene->GetRootNode());
}

struct SMeshFilter
{
    // filter out valid meshes.  Anything else we don't care about.
    bool operator()(FbxNode &inFbxNode) const
    {
        if (inFbxNode.GetNodeAttribute() != nullptr) {
            return inFbxNode.GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh
                    && inFbxNode.GetMesh() && inFbxNode.GetMesh()->GetControlPointsCount() != 0;
        }

        return false;
    }
};

struct SLightCameraFilter
{
    // filter out valid lights and cameras.
    bool operator()(FbxNode &inFbxNode) const
    {
        if (inFbxNode.GetNodeAttribute() != nullptr) {
            return (inFbxNode.GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLight
                    && inFbxNode.GetLight())
                    || (inFbxNode.GetNodeAttribute()->GetAttributeType()
                        == FbxNodeAttribute::eCamera && inFbxNode.GetCamera());
        }

        return false;
    }
};
struct SNodeSetFilter
{
    const TNodeSet &m_NodeSet;
    SNodeSetFilter(const TNodeSet &inSet)
        : m_NodeSet(inSet)
    {
    }
    bool operator()(FbxNode &inNode) const { return m_NodeSet.find(&inNode) != m_NodeSet.end(); }
};

/**
* @brief Process a scene
*
* @return no return
*/
void FbxDomWalker::ProcessScene()
{
    if (m_FbxScene != nullptr) {
        // FbxAxisSystem::MayaYUp.ConvertScene( m_FbxScene );
        int sign;
        FbxAxisSystem SceneAxisSystem = m_FbxScene->GetGlobalSettings().GetAxisSystem();
        if (SceneAxisSystem.GetUpVector(sign) == FbxAxisSystem::eZAxis) {
            std::vector<INodeTransform *> theTransforms;

            NodeTransform *theRotX = new NodeTransform(ETransformType_Rotation4);
            (*theRotX)[0] = 1.0f;
            (*theRotX)[1] = (*theRotX)[2] = 0.0f;
            (*theRotX)[3] =
                -90.0f; // Apply a -90 degree rotation on the X-axis to 'right' the model
            theTransforms.push_back(theRotX);

            m_Translator->SetTransforms(theTransforms);
            delete theRotX;
        }

        // triangulate just in case
        FbxGeometryConverter theFbxGeometryConverter(m_FbxManager);
        bool bOk = theFbxGeometryConverter.Triangulate(m_FbxScene, true, true);
        if (!bOk) {
            QT3DS_ASSERT(false);
        }

        m_importNodes.clear();
        FilterNodeHierarchy(SMeshFilter(), m_importNodes);
        FilterNodeHierarchy(SLightCameraFilter(), m_importNodes);
        ProcessNode(m_FbxScene->GetRootNode());
    }
}

/**
* @brief Get/create the ID of a joint
*
* @param[in] inFbxNode	Pointer to node element
*
* @return joint id
*/
long FbxDomWalker::GetJointNodeID(const FbxNode *inFbxNode)
{
    TJointNodeHierarchyMap::iterator theIter;

    theIter = m_JointNodeHierarchyMap.find(inFbxNode);

    if (theIter == m_JointNodeHierarchyMap.end())
        theIter = m_JointNodeHierarchyMap
                      .insert(std::make_pair(inFbxNode, (long)m_JointNodeHierarchyMap.size()))
                      .first;

    return theIter->second;
}

/**
* @brief Get the parent ID of a joint
*
* @param[in] inFbxNode	Pointer to node element
*
* @return parent joint id
*/
int FbxDomWalker::GetParentJointID(const FbxNode *inFbxNode)
{
    long nodeIndex = -1;
    TJointNodeHierarchyMap::iterator theIter;
    const FbxNode *parent = inFbxNode->GetParent();

    while (parent) {
        theIter = m_JointNodeHierarchyMap.find(parent);

        if (theIter != m_JointNodeHierarchyMap.end()) {
            nodeIndex = theIter->second;
            break;
        }

        parent = parent->GetParent();
    }

    return nodeIndex;
}

/**
* @brief Process children nodes
*
* @param[in] inFbxNode	Pointer to node element
*
* @return no return
*/
void FbxDomWalker::ProcessNodeChildren(FbxNode *inFbxNode)
{
    for (int i = 0; i < inFbxNode->GetChildCount(); ++i) {
        if (m_importNodes.find(inFbxNode) != m_importNodes.end())
            ProcessNode(inFbxNode->GetChild(i));
    }
}

/**
* @brief Process a group
*
* @param[in] inFbxNode	Pointer to node element
*
* @return no return
*/
void FbxDomWalker::ProcessGroup(FbxNode *inFbxNode)
{
    std::string groupName = "Group_";
    groupName += inFbxNode->GetName();
    m_Translator->PushGroup(groupName.c_str());
    ProcessTransform(inFbxNode);
    ProcessNodeChildren(inFbxNode);
    m_Translator->PopGroup();
}

/**
* @brief Process a light
*
* @param[in] inFbxNode    Pointer to node element
*
* @return no return
*/
void FbxDomWalker::ProcessLight(FbxNode *inFbxNode)
{
    std::string lightName = inFbxNode->GetName();
    m_Translator->PushLight(lightName.c_str());
    ProcessTransform(inFbxNode, true);
    FbxLight *light = inFbxNode->GetLight();
    FbxDouble3 color = light->Color.Get();
    m_Translator->SetLightProperties(light->LightType.Get(), SFloat3(color[0], color[1], color[2]),
            light->Intensity.Get(), 0, 0, light->CastShadows.Get());
    ProcessNodeChildren(inFbxNode);
    m_Translator->PopLight();
}

/**
* @brief Process a camera
*
* @param[in] inFbxNode    Pointer to node element
*
* @return no return
*/
void FbxDomWalker::ProcessCamera(FbxNode *inFbxNode)
{
    std::string cameraName = inFbxNode->GetName();
    m_Translator->PushCamera(cameraName.c_str());
    ProcessTransform(inFbxNode, true);
    FbxCamera *camera = inFbxNode->GetCamera();
    m_Translator->SetCameraProperties(camera->GetNearPlane(), camera->GetFarPlane(),
                                      camera->ProjectionType.Get() == FbxCamera::eOrthogonal,
                                      camera->FieldOfView.Get());
    ProcessNodeChildren(inFbxNode);
    m_Translator->PopCamera();
}

/**
* @brief Process a skeleton node and the children
*
* @param[in] inFbxNode	Pointer to node element
* @param[in] inNodeSet	Pointer to a skeleton node set to process
* @param[in] inRoot		True if root skeleton node
*
* @return root id
*/
long FbxDomWalker::ProcessSkeletonNode(FbxNode *inFbxNode, TNodeSet &inNodeSet, bool inRoot)
{
    bool pushGroup = false;
    long groupId = -1;
    if (inFbxNode->GetNodeAttribute() != nullptr) {
        std::string groupName = "Skeleton_";
        groupName += inFbxNode->GetName();
        m_Translator->PushGroup(groupName.c_str());
        pushGroup = true;
        groupId = GetJointNodeID(inFbxNode);

        ProcessTransform(inFbxNode);
        if (inRoot)
            m_Translator->SetIgnoresParentTransform(true);

        inRoot = false;

        m_Translator->SetGroupSkeletonId(groupId);
    }

    for (int i = 0; i < inFbxNode->GetChildCount(); ++i) {
        FbxNode *childNode = inFbxNode->GetChild(i);
        if (inNodeSet.find(childNode) != inNodeSet.end()) {
            long subId = ProcessSkeletonNode(inFbxNode->GetChild(i), inNodeSet, inRoot);
            if (groupId == -1)
                groupId = subId;
        }
    }

    if (pushGroup)
        m_Translator->PopGroup();

    return groupId;
}

/**
* @brief Process a node and the children
*
* @param[in] inFbxNode					Pointer to node element
*
* @return no return
*/
void FbxDomWalker::ProcessNode(FbxNode *inFbxNode)
{
    if (inFbxNode->GetNodeAttribute() != nullptr && m_importNodes.find(inFbxNode)
            != m_importNodes.end()) {
        FbxNodeAttribute::EType theAttribType = inFbxNode->GetNodeAttribute()->GetAttributeType();
        switch (theAttribType) {
        case FbxNodeAttribute::eMesh: {
            FbxMesh *theFbxMesh = inFbxNode->GetMesh();
            if (theFbxMesh == nullptr || theFbxMesh->GetControlPointsCount() == 0)
                ProcessGroup(inFbxNode);
            else
                ProcessMesh(inFbxNode);
            break;
        }
        case FbxNodeAttribute::eLight: {
            ProcessLight(inFbxNode);
            break;
        }
        case FbxNodeAttribute::eCamera: {
            ProcessCamera(inFbxNode);
            break;
        }
        case FbxNodeAttribute::eSkeleton: {
            // Ignore skeleton for now; we add them as the first child of the model with a special
            // flag indicating they ignore parent transforms.
            break;
        }
        default: {
            ProcessGroup(inFbxNode);
            break;
        }
        }
    } else {
        ProcessNodeChildren(inFbxNode);
    }
}

/**
* @brief Get the geometric transform of a node
*
* @param[in] inFbxNode					Pointer to node element
*
* @return geometric transform matrix
*/
FbxAMatrix getGeometricTransform(const FbxNode *pNode)
{
    FbxAMatrix GeometryMat;

    GeometryMat.SetT(pNode->GetGeometricTranslation(FbxNode::eSourcePivot));
    GeometryMat.SetR(pNode->GetGeometricRotation(FbxNode::eSourcePivot));
    GeometryMat.SetS(pNode->GetGeometricScaling(FbxNode::eSourcePivot));

    return GeometryMat;
}

/**
* @brief Process a transform for a node and apply animation track
*
* @param[in] inFbxNode					Pointer to node element
*
* @returna no return
*/
void FbxDomWalker::ProcessTransform(FbxNode *inFbxNode, bool ignoreScale)
{
    std::vector<INodeTransform *> theTransforms;

    // first apply animation
    ApplyAnimation(inFbxNode);

    // add local node transforms
    // if animated the non-static transforms are overwritten by the animation
    {
        FbxAMatrix theTransformMatrix;

        theTransformMatrix = inFbxNode->EvaluateLocalTransform();

        // Lights and cameras should ignore scale
        if (ignoreScale)
            theTransformMatrix.SetS(FbxVector4(1.0, 1.0, 1.0, 1.0));
        // TODO: Do some rotation magic if m_AuthoringToolType == Blender?

        theTransforms.push_back(new NodeTransform(ETransformType_Matrix4x4));

        ReadFMatrix4(theTransformMatrix, *theTransforms.back());
    }

    m_Translator->SetTransforms(theTransforms);

    std::for_each(theTransforms.begin(), theTransforms.end(), NodeTransform::Delete);
}

/**
* @brief Add a joint node to the list
*
* @param[in] jointID					Joint ID
* @param[in] parentID					Parent joint ID
* @param[in] invBindPose				Bind pose matrix
* @param[in] localToGlobalBoneSpace		Global bone space matrix
* @param[in/out] outJointInfoList		List of joint nodes belonging to a mesh
*
* @returna no return
*/
void FbxDomWalker::AddJointNode(int jointID, int parentID, FbxAMatrix &invBindPose,
                                FbxAMatrix &localToGlobalBoneSpace,
                                TJointInfoList &outJointInfoList)
{
    float invMatrix[16];
    ReadFMatrix4T(invBindPose, invMatrix);
    float lTobMatrix[16];
    ReadFMatrix4T(localToGlobalBoneSpace, lTobMatrix);

    outJointInfoList.push_back(SJointInfo(jointID, parentID, invMatrix, lTobMatrix));
}

// Get the matrix of the given pose
FbxAMatrix GetPoseMatrix(FbxPose *pPose, int pNodeIndex)
{
    FbxAMatrix lPoseMatrix;
    FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

    memcpy((double *)lPoseMatrix, (double *)lMatrix, sizeof(lMatrix.mData));

    return lPoseMatrix;
}

// get the global position of a node
FbxAMatrix GetGlobalPosition(FbxNode *pNode, FbxPose *pPose)
{
    FbxAMatrix lGlobalPosition;
    bool lPositionFound = false;

    if (pPose) {
        int lNodeIndex = pPose->Find(pNode);

        if (lNodeIndex > -1) {
            // The bind pose is always a global matrix.
            // If we have a rest pose, we need to check if it is
            // stored in global or local space.
            if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex)) {
                lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
            } else {
                // We have a local matrix, we need to convert it to
                // a global space matrix.
                FbxAMatrix lParentGlobalPosition;

                if (pNode->GetParent()) {
                    lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pPose);
                }

                FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
                lGlobalPosition = lParentGlobalPosition * lLocalPosition;
            }

            lPositionFound = true;
        }
    }

    if (!lPositionFound) {
        // There is no pose entry for that node, get the current global position instead.

        // Ideally this would use parent global position and local position to compute the global
        // position.
        // Unfortunately the equation
        //    lGlobalPosition = pParentGlobalPosition * lLocalPosition
        // does not hold when inheritance type is other than "Parent" (RSrs).
        // To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
        lGlobalPosition = pNode->EvaluateGlobalTransform(0);
    }
    return lGlobalPosition;
}

// Compute the transform matrix that the cluster will transform the vertex at binding time.
void ComputeInitialBindPoseTransform(FbxAMatrix &pMeshGlobalPosition, FbxMesh *pMesh,
                                     FbxCluster *pCluster, FbxPose * /* pPose */,
                                     FbxAMatrix &pInverseBindMatrix,
                                     FbxAMatrix &pJointToGlobalMatrix)
{
    FbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();

    FbxAMatrix lReferenceGlobalInitPosition;
    FbxAMatrix lReferenceGlobalCurrentPosition;
    FbxAMatrix lAssociateGlobalInitPosition;
    FbxAMatrix lAssociateGlobalCurrentPosition;
    FbxAMatrix lClusterGlobalInitPosition;
    FbxAMatrix lClusterGlobalCurrentPosition;

    FbxAMatrix lReferenceGeometry;
    FbxAMatrix lAssociateGeometry;
    FbxAMatrix lClusterGeometry;

    FbxAMatrix lClusterRelativeInitPosition;
    FbxAMatrix lClusterRelativeCurrentPositionInverse;

    if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel()) {
        // not supported yet
        QT3DS_ASSERT(false);
    } else {
        pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
        lReferenceGlobalCurrentPosition = pMeshGlobalPosition;
        // Multiply lReferenceGlobalInitPosition by Geometric Transformation
        lReferenceGeometry = getGeometricTransform(pMesh->GetNode());
        lReferenceGlobalInitPosition *= lReferenceGeometry;

        // Get the link initial global position and the link current global position.
        pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);

        // Compute the initial position of the link relative to the reference.
        lClusterRelativeInitPosition =
            lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

        pInverseBindMatrix = lReferenceGlobalCurrentPosition.Inverse();
        pJointToGlobalMatrix = lClusterRelativeInitPosition;
    }
}

/**
* @brief Reads the weigths belonging to each vertex
*
* @param[in]  inNode			Pointer to node element
* @param[in]  inFbxMesh			Pointer to mesh element
* @param[out] outWeights		List of all weights for al vertices
* @param[out] outJointInfoList	List of joint (bones) info belonging to this mesh
* @param[out] outBoneSet		List of bones nodes belonging to this mesh
*
* @returna unique index
*/
void FbxDomWalker::GetVertexWeights(FbxMesh *inFbxMesh, TVertexWeigthList &outWeights,
                                    TJointInfoList &outJointInfoList, TNodeSet &outBoneSet)
{
    int theDeformerCount = inFbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    int theBlendShapeDeformerCount = inFbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
    int theVertexCacheDeformerCount = inFbxMesh->GetDeformerCount(FbxDeformer::eVertexCache);

    if (theBlendShapeDeformerCount && theVertexCacheDeformerCount) {
        QT3DS_ASSERT(false);
        // not supported
        return;
    }

    if (!theDeformerCount) {
        return;
    }

    FbxCluster::ELinkMode theClusterMode = FbxCluster::eNormalize;

    // a weight entry for each vertex
    outWeights.resize(inFbxMesh->GetControlPointsCount());

    // right now we do not handle multiple deformers per mesh
    QT3DS_ASSERT(theDeformerCount <= 1);

    for (int i = 0; i < theDeformerCount; i++) {
        FbxSkin *theSkin = FbxCast<FbxSkin>(inFbxMesh->GetDeformer(i));

        if (theSkin && theSkin->GetClusterCount() > 0) {
            std::vector<FbxVector4>
                theGlobaBindPose; // contains the transformed vertices to bone space

            FbxDeformer::EDeformerType theDeformerType = theSkin->GetDeformerType();
            // we only support skin deformer
            if (theDeformerType != FbxDeformer::eSkin) {
                QT3DS_ASSERT(false);
                return;
            }

            theClusterMode = theSkin->GetCluster(0)->GetLinkMode();

            theGlobaBindPose.resize(inFbxMesh->GetControlPointsCount());

            // same for all clusters
            // GetTransformMatrix is the matrix to convert vertices of the mesh (skin) to world
            // space
            // at initial binding
            FbxAMatrix thelocalSkinToWorldSpace;
            theSkin->GetCluster(0)->GetTransformMatrix(thelocalSkinToWorldSpace);
            thelocalSkinToWorldSpace *= getGeometricTransform(inFbxMesh->GetNode());

            // convert all vertices from mesh space to world space
            for (unsigned long vertexId = 0; vertexId < theGlobaBindPose.size(); vertexId++) {
                theGlobaBindPose[vertexId] =
                    thelocalSkinToWorldSpace.MultT(inFbxMesh->GetControlPoints()[vertexId]);
            }

            for (int clusterId = 0; clusterId < theSkin->GetClusterCount(); clusterId++) {
                const FbxCluster *theCluster = theSkin->GetCluster(clusterId);

                if (!theCluster->GetLink())
                    continue;

                // this call adds the link node to our global list if not found
                int jointIdx = GetJointNodeID(theCluster->GetLink());
                int parentJointIdx = GetParentJointID(theCluster->GetLink());

                // should always be a skleton right now
                QT3DS_ASSERT(theCluster->GetLink()->GetNodeAttribute()->GetAttributeType()
                          == FbxNodeAttribute::eSkeleton);

                // All the links must have the same link mode. and right now we don't support
                // additive mode
                if ((theCluster->GetLinkMode() != theClusterMode)
                    || (theClusterMode == FbxCluster::eAdditive)) {
                    QT3DS_ASSERT(false);
                    return;
                }

                // Add the link node to the bone list
                outBoneSet.insert(const_cast<FbxNode *>(theCluster->GetLink()));

                int *vertexIDs = theCluster->GetControlPointIndices();
                double *weights = theCluster->GetControlPointWeights();

                FbxAMatrix BoneBindposeMatrix;
                FbxAMatrix invBoneBindposeMatrix;
                // GetTransformLinkMatrix converts from cluster (bone) space to world space
                theCluster->GetTransformLinkMatrix(BoneBindposeMatrix);
                // The inverse converts from world space to cluster (bone) sapce
                invBoneBindposeMatrix = BoneBindposeMatrix.Inverse();
                // This Matrix convertes from local bone space to world space
                FbxAMatrix boneLocalToSkinWorldSpace =
                    invBoneBindposeMatrix * thelocalSkinToWorldSpace;
                FbxAMatrix worldSpaceToSkinSpace = thelocalSkinToWorldSpace.Inverse();

                // add a joint node to our internal list
                AddJointNode(jointIdx, parentJointIdx, boneLocalToSkinWorldSpace,
                             worldSpaceToSkinSpace, outJointInfoList);

                // get the actual weights
                for (int j = 0; j < theCluster->GetControlPointIndicesCount(); j++) {
                    const int vertexID = vertexIDs[j];
                    const float w = (float)weights[j];

                    // this can happen with smooting groups
                    if (vertexID >= inFbxMesh->GetControlPointsCount())
                        continue;
                    if (w == 0.0f)
                        continue;

                    // store away values and convert vertices to local bone space
                    outWeights[vertexID].push_back(_SVertexWeightInfo(
                        jointIdx, w, invBoneBindposeMatrix.MultT(theGlobaBindPose[vertexID])));
                }
            }
        }
    }

    // according to the cluster link mode we must adjust the weights
    for (unsigned long vertexID = 0; vertexID < outWeights.size(); vertexID++) {
        float weightSum = 0.0f;

        // we currently only support up to 4 weights per vertex
        QT3DS_ASSERT(outWeights[vertexID].size() < 4);

        // get total sum of the weigths for each vertex
        for (unsigned long i = 0; i < outWeights[vertexID].size(); i++) {
            weightSum += outWeights[vertexID][i].m_Weight;
        }

        if (weightSum != 0.0f) {
            switch (theClusterMode) {
            case FbxCluster::eNormalize: {
                float oneOverWeightSum = 1.0f / weightSum;
                // in this mode we need to normalize the weights
                for (unsigned long i = 0; i < outWeights[vertexID].size(); i++) {
                    outWeights[vertexID][i].m_Weight *= oneOverWeightSum;
                }
            } break;
            case FbxCluster::eTotalOne: {
                // in this mode we must sum up to one
                if (weightSum < 1.0f) {
                    outWeights[vertexID].push_back(_SVertexWeightInfo(
                        0, 1.0f - weightSum, inFbxMesh->GetControlPoints()[vertexID]));
                }
            } break;
            case FbxCluster::eAdditive:
                // how should we handle this mode?
                break;
            }
        }
    }
}

static int GetNormalIndex(const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex)
{
    int theFbxNormalIndex = 0;
    switch (inFbxNormals->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxNormalIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxNormalIndex = inFbxNormals->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    return theFbxNormalIndex;
}

/**
* @brief computer tangents and binormals from mesh input data like
*		 normals and uv's.
*		 If you are interested in how it works check
*		 "Math for 3D Game programming and CG"
*
* @param[in]	 inFbxMesh		pointer to fbx mesh
* @param[in/out] outTangents	output vectors of tangents
* @param[in/out] outBinormals	output vectors of binormals
*
* @returna unique index
*/
void FbxDomWalker::GenerateMeshTangents(const FbxMesh *inFbxMesh,
                                        std::vector<SVector3> &outTangents,
                                        std::vector<SVector3> &outBinormals)
{
    // check if we have everything we need

    // vertex data
    // int theFbxCtrlPointCount = inFbxMesh->GetControlPointsCount();
    const FbxVector4 *theFbxCtrlPoints = inFbxMesh->GetControlPoints();

    // normal data
    const FbxGeometryElementNormal *theFbxNormals = inFbxMesh->GetElementNormal();
    int theFbxNormalCount = 0;
    FbxGeometryElement::EMappingMode theFbxNormalsMappingMode = FbxGeometryElement::eNone;
    if (theFbxNormals) {
        theFbxNormalCount = theFbxNormals->GetDirectArray().GetCount();
        theFbxNormalsMappingMode = theFbxNormals->GetMappingMode();
    }
    // uv data
    const FbxGeometryElementUV *theFbxUVs = inFbxMesh->GetElementUV();
    int theFbxUVsCount = 0;
    FbxGeometryElement::EMappingMode theFbxUVsMappingMode = FbxGeometryElement::eNone;
    if (theFbxUVs) {
        theFbxUVsCount = theFbxUVs->GetDirectArray().GetCount();
        theFbxUVsMappingMode = theFbxUVs->GetMappingMode();
    }

    // we need normals and uv's
    if (theFbxNormalCount == 0 || theFbxUVsCount == 0) {
        return;
    }

    // allocate arrays
    SVector3 *tan1 = new SVector3[theFbxNormalCount * 2];
    SVector3 *tan2 = tan1 + theFbxNormalCount;
    SVector3 *normals = new SVector3[theFbxNormalCount];
    char *normalsPerVtx = new char[theFbxNormalCount];

    // failed to allocate menory
    if (!tan1 || !normals || !normalsPerVtx) {
        QT3DS_ASSERT(false);
        return;
    }

    memset(tan1, 0x00, theFbxNormalCount * sizeof(SVector3) * 2);
    memset(normals, 0x00, theFbxNormalCount * sizeof(SVector3));
    memset(normalsPerVtx, 0x00, theFbxNormalCount * sizeof(char));

    FbxAMatrix identidy;
    identidy.SetIdentity();

    int theVertexID = 0;
    SVector3 vx1, vx2, vx3;
    SVector3 n1, n2, n3;
    SVector2 uv1, uv2, uv3;

    int theFbxTriangleCount = inFbxMesh->GetPolygonCount();
    for (int i = 0; i < theFbxTriangleCount; i++) {
        QT3DS_ASSERT(inFbxMesh->GetPolygonSize(i) == 3);

        // read vertex 1 data
        int theFbxCtrlPointIndex1 = inFbxMesh->GetPolygonVertex(i, 0);
        ReadVertex(theFbxCtrlPoints, theFbxCtrlPointIndex1, (float *)vx1, identidy);
        ReadNormal(theFbxNormals, (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                       ? theVertexID
                       : theFbxCtrlPointIndex1,
                   (float *)n1);
        ReadTexCoord(theFbxUVs, (theFbxUVsMappingMode == FbxGeometryElement::eByPolygonVertex)
                         ? theVertexID
                         : theFbxCtrlPointIndex1,
                     (float *)uv1);
        theFbxCtrlPointIndex1 = GetNormalIndex(
            theFbxNormals, (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                ? theVertexID
                : theFbxCtrlPointIndex1);
        ++theVertexID;

        // read vertex 2 data
        int theFbxCtrlPointIndex2 = inFbxMesh->GetPolygonVertex(i, 1);
        ReadVertex(theFbxCtrlPoints, theFbxCtrlPointIndex2, (float *)vx2, identidy);
        ReadNormal(theFbxNormals, (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                       ? theVertexID
                       : theFbxCtrlPointIndex2,
                   (float *)n2);
        ReadTexCoord(theFbxUVs, (theFbxUVsMappingMode == FbxGeometryElement::eByPolygonVertex)
                         ? theVertexID
                         : theFbxCtrlPointIndex2,
                     (float *)uv2);
        theFbxCtrlPointIndex2 = GetNormalIndex(
            theFbxNormals, (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                ? theVertexID
                : theFbxCtrlPointIndex2);
        ++theVertexID;

        // read vertex 3 data
        int theFbxCtrlPointIndex3 = inFbxMesh->GetPolygonVertex(i, 2);
        ReadVertex(theFbxCtrlPoints, theFbxCtrlPointIndex3, (float *)vx3, identidy);
        ReadNormal(theFbxNormals, (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                       ? theVertexID
                       : theFbxCtrlPointIndex3,
                   (float *)n3);
        ReadTexCoord(theFbxUVs, (theFbxUVsMappingMode == FbxGeometryElement::eByPolygonVertex)
                         ? theVertexID
                         : theFbxCtrlPointIndex3,
                     (float *)uv3);
        theFbxCtrlPointIndex3 = GetNormalIndex(
            theFbxNormals, (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                ? theVertexID
                : theFbxCtrlPointIndex3);
        ++theVertexID;

        // compute triangle vectors
        float x1 = vx2[0] - vx1[0];
        float x2 = vx3[0] - vx1[0];
        float y1 = vx2[1] - vx1[1];
        float y2 = vx3[1] - vx1[1];
        float z1 = vx2[2] - vx1[2];
        float z2 = vx3[2] - vx1[2];

        float s1 = uv2[0] - uv1[0];
        float s2 = uv3[0] - uv1[0];
        float t1 = uv2[1] - uv1[1];
        float t2 = uv3[1] - uv1[1];

        float area = (s1 * t2 - s2 * t1);
        if (area == 0.0)
            area = 1.0;

        float r = 1.0f / area;

        float sx = ((t2 * x1 - t1 * x2) * r);
        float sy = ((t2 * y1 - t1 * y2) * r);
        float sz = ((t2 * z1 - t1 * z2) * r);

        float tx = ((s1 * x2 - s2 * x1) * r);
        float ty = ((s1 * y2 - s2 * y1) * r);
        float tz = ((s1 * z2 - s2 * z1) * r);

        tan1[theFbxCtrlPointIndex1][0] += sx;
        tan1[theFbxCtrlPointIndex1][1] += sy;
        tan1[theFbxCtrlPointIndex1][2] += sz;
        tan1[theFbxCtrlPointIndex2][0] += sx;
        tan1[theFbxCtrlPointIndex2][1] += sy;
        tan1[theFbxCtrlPointIndex2][2] += sz;
        tan1[theFbxCtrlPointIndex3][0] += sx;
        tan1[theFbxCtrlPointIndex3][1] += sy;
        tan1[theFbxCtrlPointIndex3][2] += sz;

        tan2[theFbxCtrlPointIndex1][0] += tx;
        tan2[theFbxCtrlPointIndex1][1] += ty;
        tan2[theFbxCtrlPointIndex1][2] += tz;
        tan2[theFbxCtrlPointIndex2][0] += tx;
        tan2[theFbxCtrlPointIndex2][1] += ty;
        tan2[theFbxCtrlPointIndex2][2] += tz;
        tan2[theFbxCtrlPointIndex3][0] += tx;
        tan2[theFbxCtrlPointIndex3][1] += ty;
        tan2[theFbxCtrlPointIndex3][2] += tz;

        normals[theFbxCtrlPointIndex1][0] += n1.m_Data[0];
        normals[theFbxCtrlPointIndex1][1] += n1.m_Data[1];
        normals[theFbxCtrlPointIndex1][2] += n1.m_Data[2];
        normals[theFbxCtrlPointIndex2][0] += n2.m_Data[0];
        normals[theFbxCtrlPointIndex2][1] += n2.m_Data[1];
        normals[theFbxCtrlPointIndex2][2] += n2.m_Data[2];
        normals[theFbxCtrlPointIndex3][0] += n3.m_Data[0];
        normals[theFbxCtrlPointIndex3][1] += n3.m_Data[1];
        normals[theFbxCtrlPointIndex3][2] += n3.m_Data[2];

        normalsPerVtx[theFbxCtrlPointIndex1] += 1;
        normalsPerVtx[theFbxCtrlPointIndex2] += 1;
        normalsPerVtx[theFbxCtrlPointIndex3] += 1;
    }

    outTangents.resize(theFbxNormalCount);
    outBinormals.resize(theFbxNormalCount);
    // compute actual tangents
    for (int i = 0; i < theFbxNormalCount; i++) {
        QT3DSVec3 n(normals[i][0], normals[i][1], normals[i][2]);
        const qt3ds::QT3DSVec3 t(tan1[i][0], tan1[i][1], tan1[i][2]);
        const qt3ds::QT3DSVec3 t2(tan2[i][0], tan2[i][1], tan2[i][2]);

        n = n / normalsPerVtx[i];

        // Gram-Schmidt orthogonalize
        qt3ds::QT3DSVec3 temp, t3, bi;
        temp = t - n * n.dot(t);
        t3 = temp;
        t3.normalize();
        outTangents[i][0] = t3.x;
        outTangents[i][1] = t3.y;
        outTangents[i][2] = t3.z;

        // Calculate bi tangent
        temp = n.cross(t);
        temp.normalize();
        float dp = temp.dot(t2);
        float sig = (dp < 0.0f) ? -1.0f : 1.0f;
        outBinormals[i][0] = temp.x * sig;
        outBinormals[i][1] = temp.y * sig;
        outBinormals[i][2] = temp.z * sig;
    }

    delete[] tan1;
    delete[] normals;
    delete[] normalsPerVtx;
}

/**
* @brief Returns a unique face index which is made up of a vertex, normal,
*		 texcoord index and weights tuple.
*		 The order of ioFaceIndicies.first eventually becomes the order which we will
 *		 fill the vertex, normal and texcoord arrays weights the for Model.
*
* @param[in/out]  ioFaceIndicies	List to add the element
* @param[in/out]  inFaceValues		The tuple which is the key
*
* @returna unique index
*/
long RetrieveFaceIndex(FbxDomWalker::TFaceIndicies &ioFaceIndicies,
                       const FbxDomWalker::TVertexInfoTuple &inFaceValues)
{
    std::pair<FbxDomWalker::TFaceIndicies::first_type::iterator, bool> theTupleEntry;

    theTupleEntry = ioFaceIndicies.first.insert(
        std::make_pair(inFaceValues, (long)(ioFaceIndicies.first.size())));

    return theTupleEntry.first->second;
}

/**
* @brief Processes a mesh entity and it childs.
*		 In addition it processes a skeleton belonging to a mesh
*
* @param[in]  inFbxNode		Pointer to node element
*
* @return no return.
*/
void FbxDomWalker::ProcessMesh(FbxNode *inFbxNode)
{
    FbxMesh *theFbxMesh = inFbxNode->GetMesh();
    if (theFbxMesh == nullptr) {
        QT3DS_ASSERT(false);
        return;
    }

    // push mesh onto stack
    m_Translator->PushModel(inFbxNode->GetName());
    ProcessTransform(inFbxNode);

    theFbxMesh = inFbxNode->GetMesh();

    // GetVertexWeights produces a joint info list.
    // theJointInfoList retrieves information about the per vertex weights,
    // and the bone index list for each vertex.
    // and a list of current bones for this mesh
    TNodeSet theBoneSet;
    TJointInfoList theJointInfoList;
    TVertexWeigthList theVertexWeights;
    GetVertexWeights(theFbxMesh, theVertexWeights, theJointInfoList, theBoneSet);

    TFaceIndicies theFaceIndices;
    TFaceMaterialIndices theFaceMaterialIndices;
    bool canUseDirectMode = true; // if we have a direct mapping we can read data in faster way

    // vertex data
    int theFbxCtrlPointCount = theFbxMesh->GetControlPointsCount();
    const FbxVector4 *theFbxCtrlPoints = theFbxMesh->GetControlPoints();
    // index data
    // int theFbxVertexIndexCount = theFbxMesh->GetPolygonVertexCount();
    // int* theFbxIndexArray = theFbxMesh->GetPolygonVertices();
    // normal data
    const FbxGeometryElementNormal *theFbxNormals = theFbxMesh->GetElementNormal();
    int theFbxNormalCount = 0;
    FbxGeometryElement::EMappingMode theFbxNormalsMappingMode = FbxGeometryElement::eNone;
    if (theFbxNormals) {
        theFbxNormalCount = theFbxNormals->GetDirectArray().GetCount();
        theFbxNormalsMappingMode = theFbxNormals->GetMappingMode();
        canUseDirectMode &=
            (theFbxNormalsMappingMode == FbxGeometryElement::eByControlPoint) ? true : false;
    }
    // uv data
    int theFbxUVsSetCount = theFbxMesh->GetElementUVCount(); // overall uv set count

    const FbxGeometryElementUV *theFbxUVs = theFbxMesh->GetElementUV();
    int theFbxUVsCount = 0;
    FbxGeometryElement::EMappingMode theFbxUVsMappingMode = FbxGeometryElement::eNone;
    if (theFbxUVs) {
        theFbxUVsCount = theFbxUVs->GetDirectArray().GetCount();
        theFbxUVsMappingMode = theFbxUVs->GetMappingMode();
        canUseDirectMode &=
            (theFbxUVsMappingMode == FbxGeometryElement::eByControlPoint) ? true : false;
    }
    // uv data second set
    const FbxGeometryElementUV *theFbxUV2s = theFbxMesh->GetElementUV(1);
    int theFbxUV2sCount = 0;
    FbxGeometryElement::EMappingMode theFbxUV2sMappingMode = FbxGeometryElement::eNone;
    if (theFbxUV2s && theFbxUVsSetCount > 1) {
        theFbxUV2sCount = theFbxUV2s->GetDirectArray().GetCount();
        theFbxUV2sMappingMode = theFbxUV2s->GetMappingMode();
        canUseDirectMode &=
            (theFbxUV2sMappingMode == FbxGeometryElement::eByControlPoint) ? true : false;
    }

    // tangent data
    FbxGeometryElementTangent *theFbxTangents = theFbxMesh->GetElementTangent();
    int theFbxTangentsCount = 0;
    FbxGeometryElement::EMappingMode theFbxTangentsMappingMode = FbxGeometryElement::eNone;
    if (theFbxTangents) {
        theFbxTangentsCount = theFbxTangents->GetDirectArray().GetCount();
        theFbxTangentsMappingMode = theFbxTangents->GetMappingMode();
        canUseDirectMode &=
            (theFbxTangentsMappingMode == FbxGeometryElement::eByControlPoint) ? true : false;
    }
    // binormal data
    FbxGeometryElementBinormal *theFbxBinormals = theFbxMesh->GetElementBinormal();
    int theFbxBinormalsCount = 0;
    FbxGeometryElement::EMappingMode theFbxBinormalsMappingMode = FbxGeometryElement::eNone;
    if (theFbxBinormals) {
        theFbxBinormalsCount = theFbxBinormals->GetDirectArray().GetCount();
        theFbxBinormalsMappingMode = theFbxBinormals->GetMappingMode();
        canUseDirectMode &=
            (theFbxBinormalsMappingMode == FbxGeometryElement::eByControlPoint) ? true : false;
    }
    // material data
    FbxGeometryElementMaterial *theFbxMaterials = theFbxMesh->GetElementMaterial();
    int theFbxMaterialsCount = 0;
    FbxGeometryElement::EMappingMode theFbxMaterialsMappingMode = FbxGeometryElement::eNone;
    if (theFbxMaterials) {
        theFbxMaterialsCount = theFbxMesh->GetElementMaterialCount();
        theFbxMaterialsMappingMode = theFbxMaterials->GetMappingMode();
    }
    // vertex color data
    const FbxGeometryElementVertexColor *theFbxColors = theFbxMesh->GetElementVertexColor();
    int theFbxColorsCount = 0;
    FbxGeometryElement::EMappingMode theFbxColorsMappingMode = FbxGeometryElement::eNone;
    if (theFbxColors) {
        theFbxColorsCount = theFbxColors->GetDirectArray().GetCount();
        theFbxColorsMappingMode = theFbxColors->GetMappingMode();
        canUseDirectMode &= theFbxColorsMappingMode == FbxGeometryElement::eByControlPoint;
    }

    // check if we need to generate tangents and binormals
    std::vector<SVector3> newTangents;
    std::vector<SVector3> newBinormals;
    if (!theFbxTangents && !theFbxBinormals) {
        GenerateMeshTangents(theFbxMesh, newTangents, newBinormals);
    }

    // polygon data
    int theFbxTriangleCount = theFbxMesh->GetPolygonCount();

    SFaceMaterialInfo thePerFaceMaterialnfo;
    thePerFaceMaterialnfo.m_StartFace = 0;
    QueryMaterialInfo(theFbxMesh, &thePerFaceMaterialnfo);

    TLongsList theMaterialFaceIndicies;
    theMaterialFaceIndicies.reserve(theFbxTriangleCount);

    // When we build a transform history we build everything into one matrix
    // but the geometric transform is not inherited therefore
    // we transform it on load
    FbxAMatrix geomTransform = getGeometricTransform(inFbxNode);

    // when we can go direct mode everything is easier
    canUseDirectMode = false; // currently disabled
    if (canUseDirectMode) {
        QT3DS_ASSERT(!theFbxNormals
                  || theFbxNormalsMappingMode == FbxGeometryElement::eByControlPoint);
        QT3DS_ASSERT(!theFbxUVs || theFbxUVsMappingMode == FbxGeometryElement::eByControlPoint);
        QT3DS_ASSERT(!theFbxTangents
                  || theFbxTangentsMappingMode == FbxGeometryElement::eByControlPoint);
        QT3DS_ASSERT(!theFbxBinormals
                  || theFbxBinormalsMappingMode == FbxGeometryElement::eByControlPoint);

        for (int i = 0; i < theFbxCtrlPointCount; ++i) {
            TVertexInfoTuple theFaceTupleValues;

            ReadVertex(theFbxCtrlPoints, i, get<0>(theFaceTupleValues), geomTransform);

            if (theFbxNormals)
                ReadNormal(theFbxNormals, i, get<1>(theFaceTupleValues));
            if (theFbxUVs)
                ReadTexCoord(theFbxUVs, i, get<2>(theFaceTupleValues));

            if (theFbxTangents)
                ReadTexTangent(theFbxTangents, i, get<3>(theFaceTupleValues));
            else if (newTangents.size() > 0)
                ReadTexTangent(newTangents, theFbxNormals, i, get<3>(theFaceTupleValues));

            if (theFbxBinormals)
                ReadTexBinormal(theFbxBinormals, i, get<4>(theFaceTupleValues));
            else if (newBinormals.size() > 0)
                ReadTexBinormal(newBinormals, theFbxNormals, i, get<4>(theFaceTupleValues));

            long theFaceIndex = RetrieveFaceIndex(theFaceIndices, theFaceTupleValues);
            theMaterialFaceIndicies.push_back(theFaceIndex);

            if (theFbxMaterialsCount) {
                SFaceMaterialInfo currentPerFaceMaterialnfo;
                currentPerFaceMaterialnfo.m_StartFace = i / 3;
                QueryMaterialInfo(theFbxMesh, &currentPerFaceMaterialnfo);

                if (!(thePerFaceMaterialnfo == currentPerFaceMaterialnfo)) {
                    // add per face material info
                    thePerFaceMaterialnfo.m_FaceCount = i / 3 - thePerFaceMaterialnfo.m_StartFace;
                    QT3DS_ASSERT(i % 3 == 0);
                    theFaceMaterialIndices.push_back(thePerFaceMaterialnfo);

                    thePerFaceMaterialnfo = currentPerFaceMaterialnfo;
                }
            }
        }
    } else {
        int theVertexID = 0;

        for (int i = 0; i < theFbxTriangleCount; ++i) {
            for (int j = 0; j < 3; ++j) {
                TVertexInfoTuple theFaceTupleValues;

                int theFbxCtrlPointIndex = theFbxMesh->GetPolygonVertex(i, j);
                QT3DS_ASSERT(theFbxMesh->GetPolygonSize(i) == 3);
                QT3DS_ASSERT(theFbxCtrlPointIndex < theFbxCtrlPointCount
                          && theFbxCtrlPointIndex != -1);

                ReadVertex(theFbxCtrlPoints, theFbxCtrlPointIndex, get<0>(theFaceTupleValues),
                           geomTransform);

                if (theFbxNormals) {
                    ReadNormal(theFbxNormals,
                               (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                                   ? theVertexID
                                   : theFbxCtrlPointIndex,
                               get<1>(theFaceTupleValues));
                }
                if (theFbxUVs) {
                    ReadTexCoord(theFbxUVs,
                                 (theFbxUVsMappingMode == FbxGeometryElement::eByPolygonVertex)
                                     ? theVertexID
                                     : theFbxCtrlPointIndex,
                                 get<2>(theFaceTupleValues));
                }
                if (theFbxTangents) {
                    ReadTexTangent(theFbxTangents, (theFbxTangentsMappingMode
                                                    == FbxGeometryElement::eByPolygonVertex)
                                       ? theVertexID
                                       : theFbxCtrlPointIndex,
                                   get<3>(theFaceTupleValues));
                } else if (newTangents.size() > 0) {
                    // we use the mapping as the normals
                    ReadTexTangent(
                        newTangents, theFbxNormals,
                        (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                            ? theVertexID
                            : theFbxCtrlPointIndex,
                        get<3>(theFaceTupleValues));
                }
                if (theFbxBinormals) {
                    ReadTexBinormal(theFbxBinormals, (theFbxBinormalsMappingMode
                                                      == FbxGeometryElement::eByPolygonVertex)
                                        ? theVertexID
                                        : theFbxCtrlPointIndex,
                                    get<4>(theFaceTupleValues));
                } else if (newBinormals.size() > 0) {
                    ReadTexBinormal(
                        newBinormals, theFbxNormals,
                        (theFbxNormalsMappingMode == FbxGeometryElement::eByPolygonVertex)
                            ? theVertexID
                            : theFbxCtrlPointIndex,
                        get<4>(theFaceTupleValues));
                }

                if (theVertexWeights.size() > 0) {
                    ReadWeight(theVertexWeights[theFbxCtrlPointIndex], theFbxCtrlPointIndex,
                               get<5>(theFaceTupleValues));
                    ReadBoneIndex(theVertexWeights[theFbxCtrlPointIndex], theFbxCtrlPointIndex,
                                  get<6>(theFaceTupleValues), theJointInfoList);
                }

                if (theFbxUV2s) {
                    ReadTexCoord(theFbxUV2s,
                                 (theFbxUV2sMappingMode == FbxGeometryElement::eByPolygonVertex)
                                     ? theVertexID
                                     : theFbxCtrlPointIndex,
                                 get<7>(theFaceTupleValues));
                }

                if (theFbxColors) {
                    ReadColor(theFbxColors,
                              (theFbxColorsMappingMode == FbxGeometryElement::eByPolygonVertex)
                                   ? theVertexID
                                   : theFbxCtrlPointIndex,
                              get<8>(theFaceTupleValues));
                }

                long theFaceIndex = RetrieveFaceIndex(theFaceIndices, theFaceTupleValues);
                theMaterialFaceIndicies.push_back(theFaceIndex);

                ++theVertexID;
            }

            if (theFbxMaterialsCount) {
                SFaceMaterialInfo currentPerFaceMaterialnfo;
                currentPerFaceMaterialnfo.m_StartFace = i;
                QueryMaterialInfo(theFbxMesh, &currentPerFaceMaterialnfo);

                if (!(thePerFaceMaterialnfo == currentPerFaceMaterialnfo)) {
                    // the material changed add per face material info
                    thePerFaceMaterialnfo.m_FaceCount = i - thePerFaceMaterialnfo.m_StartFace;
                    theFaceMaterialIndices.push_back(thePerFaceMaterialnfo);

                    thePerFaceMaterialnfo = currentPerFaceMaterialnfo;
                }
            }
        }
    }

    TFloatsList theVertices;
    TFloatsList theNormals;
    TFloatsList theTexCoords;
    TFloatsList theTexTangents;
    TFloatsList theTexBinormals;
    TFloatsList theWeights;
    TFloatsList theBoneIndex;
    TFloatsList theTexCoords2;
    TFloatsList theColors;

    // Prepare arrays for population
    long theNumberOfUniqueFacePoints = (long)theFaceIndices.first.size();
    if (theNumberOfUniqueFacePoints == 0)
        return;

    theVertices.resize(theNumberOfUniqueFacePoints * 3);
    if (theFbxNormals) {
        theNormals.resize(theNumberOfUniqueFacePoints * 3);
    }
    if (theFbxUVs) {
        theTexCoords.resize(theNumberOfUniqueFacePoints * 2);
    }
    if (theFbxTangents || newTangents.size() > 0) {
        theTexTangents.resize(theNumberOfUniqueFacePoints * 3);
    }
    if (theFbxBinormals || newBinormals.size() > 0) {
        theTexBinormals.resize(theNumberOfUniqueFacePoints * 3);
    }
    if (theVertexWeights.size() > 0) {
        theWeights.resize(theNumberOfUniqueFacePoints * 4);
        theBoneIndex.resize(theNumberOfUniqueFacePoints * 4);
    }
    if (theFbxUV2s) {
        theTexCoords2.resize(theNumberOfUniqueFacePoints * 2);
    }
    if (theFbxColors)
        theColors.resize(theNumberOfUniqueFacePoints * 3);

    // Populate vertex, normal and texcoord arrays so that face indicies can reference from them
    TFaceIndicies::first_type::const_iterator theIter = theFaceIndices.first.begin();
    TFaceIndicies::first_type::const_iterator theEnd = theFaceIndices.first.end();
    for (; theIter != theEnd; ++theIter) {
        long theFaceIndex = theIter->second;
        const TVertexInfoTuple &thePointTuple = theIter->first;

        WriteFloat3(theVertices, theFaceIndex, get<0>(thePointTuple));

        if (theFbxNormals)
            WriteFloat3(theNormals, theFaceIndex, get<1>(thePointTuple));
        if (theFbxUVs)
            WriteFloat2(theTexCoords, theFaceIndex, get<2>(thePointTuple));
        if (theFbxTangents || newTangents.size() > 0)
            WriteFloat3(theTexTangents, theFaceIndex, get<3>(thePointTuple));
        if (theFbxBinormals || newBinormals.size() > 0)
            WriteFloat3(theTexBinormals, theFaceIndex, get<4>(thePointTuple));
        if (theVertexWeights.size() > 0) {
            WriteFloat4(theWeights, theFaceIndex, get<5>(thePointTuple));
            WriteFloat4(theBoneIndex, theFaceIndex, get<6>(thePointTuple));
        }
        if (theFbxUV2s)
            WriteFloat2(theTexCoords2, theFaceIndex, get<7>(thePointTuple));
        if (theFbxColors)
            WriteFloat3(theColors, theFaceIndex, get<8>(thePointTuple));
    }

    m_Translator->SetGeometry(theVertices, theNormals, theTexCoords, theTexCoords2, theTexTangents,
                              theTexBinormals, theWeights, theBoneIndex, theColors,
                              theMaterialFaceIndicies);

    // Read material belonging to this mesh
    // At this point theFaceMaterialIndices contains the information which material is used for each
    // face
    if (theFbxMaterialsCount) {
        // add final per face material
        thePerFaceMaterialnfo.m_FaceCount = theFbxTriangleCount - thePerFaceMaterialnfo.m_StartFace;
        theFaceMaterialIndices.push_back(thePerFaceMaterialnfo);

        for (size_t i = 0; i < theFaceMaterialIndices.size(); i++) {
            SFaceMaterialInfo thePerFaceMaterialnfo = theFaceMaterialIndices.at(i);

            for (size_t elCount = 0; elCount < thePerFaceMaterialnfo.m_MatElementsIDs.size();
                 elCount++) {
                FbxGeometryElementMaterial *theMaterialElement = theFbxMesh->GetElementMaterial(
                    thePerFaceMaterialnfo.m_MatElementsIDs.at(elCount));
                FbxSurfaceMaterial *theMaterial = inFbxNode->GetMaterial(
                    theMaterialElement->GetIndexArray().GetAt(thePerFaceMaterialnfo.m_StartFace));
                if (theMaterial)
                    ReadMaterial(theMaterial, thePerFaceMaterialnfo.m_StartFace,
                                 thePerFaceMaterialnfo.m_FaceCount);
            }
        }
    } else if (theFbxNormals) {
        // if there is no material defined try to create a default one
        // add final per face material
        thePerFaceMaterialnfo.m_FaceCount = theFbxTriangleCount - thePerFaceMaterialnfo.m_StartFace;
        theFaceMaterialIndices.push_back(thePerFaceMaterialnfo);

        ReadDefaultMaterial(inFbxNode, thePerFaceMaterialnfo.m_StartFace,
                            thePerFaceMaterialnfo.m_FaceCount);
    }

    // stream out joint info if any
    if (theJointInfoList.size()) {
        TJointInfoList::iterator theIter;

        for (theIter = theJointInfoList.begin(); theIter != theJointInfoList.end(); theIter++) {
            m_Translator->SetJointNode(*theIter);
        }
    }

    // Here we process a skeleton hierarchy which belongs to a mesh.
    // Note this means we actually copy skeleton instance into a mesh
    // We do this that at render time we get the relationship mesh <-> skeleton
    if (theBoneSet.size()) {
        // This should be equal
        QT3DS_ASSERT(theBoneSet.size() == theJointInfoList.size());

        // Set the bones for this mesh.
        // In ProcessSkeletonNode we only process bones which belong to the current set
        TNodeSet skeletonSet;
        FilterNodeHierarchy(SNodeSetFilter(theBoneSet), skeletonSet);

        if (skeletonSet.size()) {
            // start with parent of root bone otherwise we miss our inserted animation node
            long skeletonRoot = ProcessSkeletonNode(m_FbxScene->GetRootNode(), skeletonSet, true);
            m_Translator->SetModelSkeletonRoot(skeletonRoot);
        }
    }

    ProcessNodeChildren(inFbxNode);
    m_Translator->PopModel();
}

/**
* @brief Read psoition value for a vertex.
*		 This version convertes the vertex with the local geometric transform
*
* @param[in]  inFbxCtrlPoints			Pointer to normal element
* @param[in]  inFbxCtrlPointIndex		Unused
* @param[out] outValue					Retrieves the position for this vertex.
* @param[out] geometricTransformation	Local transform matrix to convert with.
*
* @return no return.
*/
void FbxDomWalker::ReadVertex(const FbxVector4 *inFbxCtrlPoints, int inFbxCtrlPointIndex,
                              float *outValue, FbxAMatrix &geometricTransformation)
{
    const FbxVector4 &inCtrlPoint = inFbxCtrlPoints[inFbxCtrlPointIndex];
    FbxVector4 transformedCtrlPoint = geometricTransformation.MultT(inCtrlPoint);

    outValue[0] = (float)transformedCtrlPoint[0];
    outValue[1] = (float)transformedCtrlPoint[1];
    outValue[2] = (float)transformedCtrlPoint[2];
}

/**
* @brief Read psoition value for a vertex
*		 This version read the position from the inFbxWeights array.
*		 This position is in bone space
*
* @param[in]  inFbxWeights			Pointer to normal element
* @param[in]  inFbxCtrlPointIndex	Unused
* @param[out] outValue				Retrieves the position for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadVertex(const TPerVertexWeightInfo &inFbxWeights,
                              int /* inFbxCtrlPointIndex */, float *outValue)
{
    outValue[0] = inFbxWeights[0].m_Position[0];
    outValue[1] = inFbxWeights[0].m_Position[1];
    outValue[2] = inFbxWeights[0].m_Position[2];
}

/**
* @brief Read normal for a vertex
*
* @param[in]  inFbxNormals		Pointer to normal element
* @param[in]  inFbxIndex		Index to read from (direct or indirect)
* @param[out] outValue			Retrieves the normal for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadNormal(const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex,
                              float *outValue)
{
    int theFbxNormalIndex = 0;
    switch (inFbxNormals->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxNormalIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxNormalIndex = inFbxNormals->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    FbxVector4 theNormal = inFbxNormals->GetDirectArray().GetAt(theFbxNormalIndex);

    outValue[0] = (float)theNormal[0];
    outValue[1] = (float)theNormal[1];
    outValue[2] = (float)theNormal[2];
}

void FbxDomWalker::ReadColor(const FbxGeometryElementVertexColor *inFbxColors, int inFbxIndex,
                             float *outValue)
{
    int theFbxColorIndex = 0;
    switch (inFbxColors->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxColorIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxColorIndex = inFbxColors->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    FbxColor theColor = inFbxColors->GetDirectArray().GetAt(theFbxColorIndex);

    outValue[0] = (float)theColor.mRed;
    outValue[1] = (float)theColor.mGreen;
    outValue[2] = (float)theColor.mBlue;
}

/**
* @brief Read texture coords for a vertex
*
* @param[in]  inFbxUVs			Pointer to UV element
* @param[in]  inFbxIndex		Index to read from (direct or indirect)
* @param[out] outValue			Retrieves the texture coords for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadTexCoord(const FbxGeometryElementUV *inFbxUVs, int inFbxIndex,
                                float *outValue)
{
    int theFbxUVIndex = 0;
    switch (inFbxUVs->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxUVIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxUVIndex = inFbxUVs->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    const FbxVector2 uv(inFbxUVs->GetDirectArray().GetAt(theFbxUVIndex));
    outValue[0] = (float) uv[0];
    outValue[1] = (float) uv[1];
}

/**
* @brief Read tangents for a vertex
*
* @param[in]  inFbxTangents		Pointer to tangents element
* @param[in]  inFbxIndex		Index to read from (direct or indirect)
* @param[out] outValue			Retrieves the tangents for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadTexTangent(const FbxGeometryElementTangent *inFbxTangents, int inFbxIndex,
                                  float *outValue)
{
    int theFbxTangentIndex = 0;
    switch (inFbxTangents->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxTangentIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxTangentIndex = inFbxTangents->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    FbxVector4 theTexTangent = inFbxTangents->GetDirectArray().GetAt(theFbxTangentIndex);

    outValue[0] = (float)theTexTangent[0];
    outValue[1] = (float)theTexTangent[1];
    outValue[2] = (float)theTexTangent[2];
}

/**
* @brief Read tangents for a vertex
*
* @param[in]  inTangentPoints	Pointer to tangents array
* @param[in]  inFbxIndex		Index to read from (direct or indirect)
* @param[out] outValue			Retrieves the tangents for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadTexTangent(std::vector<SVector3> &inTangentPoints,
                                  const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex,
                                  float *outValue)
{
    int theFbxTangentIndex = 0;
    switch (inFbxNormals->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxTangentIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxTangentIndex = inFbxNormals->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    outValue[0] = inTangentPoints[theFbxTangentIndex][0];
    outValue[1] = inTangentPoints[theFbxTangentIndex][1];
    outValue[2] = inTangentPoints[theFbxTangentIndex][2];
}

/**
* @brief Read binormals for a vertex
*
* @param[in]  inFbxBinormals	Pointer to binormal element
* @param[in]  inFbxIndex		Index to read from (direct or indirect)
* @param[out] outValue			Retrieves the binormal for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadTexBinormal(const FbxGeometryElementBinormal *inFbxBinormals, int inFbxIndex,
                                   float *outValue)
{
    int theFbxBinormalIndex = 0;
    switch (inFbxBinormals->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxBinormalIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxBinormalIndex = inFbxBinormals->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    FbxVector4 theTexBinormal = inFbxBinormals->GetDirectArray().GetAt(theFbxBinormalIndex);

    outValue[0] = (float)theTexBinormal[0];
    outValue[1] = (float)theTexBinormal[1];
    outValue[2] = (float)theTexBinormal[2];
}

/**
* @brief Read binormals for a vertex
*
* @param[in]  inBinormalPoints	Pointer to binormal array
* @param[in]  inFbxIndex		Index to read from (direct or indirect)
* @param[out] outValue			Retrieves the binormal for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadTexBinormal(std::vector<SVector3> &inBinormalPoints,
                                   const FbxGeometryElementNormal *inFbxNormals, int inFbxIndex,
                                   float *outValue)
{
    int theFbxBinormalIndex = 0;
    switch (inFbxNormals->GetReferenceMode()) {
    case FbxGeometryElement::eDirect:
        theFbxBinormalIndex = inFbxIndex;
        break;
    case FbxGeometryElement::eIndexToDirect:
        theFbxBinormalIndex = inFbxNormals->GetIndexArray().GetAt(inFbxIndex);
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    outValue[0] = inBinormalPoints[theFbxBinormalIndex][0];
    outValue[1] = inBinormalPoints[theFbxBinormalIndex][1];
    outValue[2] = inBinormalPoints[theFbxBinormalIndex][2];
}

/**
* @brief Read weights for a vertex
*
* @param[in]  inFbxWeights		Pointer to per vertex weights, bone indices array
* @param[in]  inFbxIndex		Unused here
* @param[out] outValue			Retrieves the bone indices for this vertex.
*
* @return no return.
*/
void FbxDomWalker::ReadWeight(const FbxDomWalker::TPerVertexWeightInfo &inFbxWeights,
                              int /* inFbxIndex */, float *outValue)
{
    // clear since we might not use all values
    outValue[0] = outValue[1] = outValue[2] = outValue[3] = 0;
    for (unsigned long i = 0; i < inFbxWeights.size() && i < 4; i++) {
        outValue[i] = inFbxWeights[i].m_Weight;
    }
}

/**
* @brief Read bone indices for a vertex
*
* @param[in]  inFbxWeights		Pointer to per vertex weights, bone indices array
* @param[in]  inFbxIndex		Unused here
* @param[out] outValue			Retrieves the bone indices for this vertex.
* @param[in]  inAbsoluteJoints	Absolute joint ID's
*
* @return no return.
*/
void FbxDomWalker::ReadBoneIndex(const FbxDomWalker::TPerVertexWeightInfo &inFbxWeights,
                                 int /* inFbxIndex */, float *outValue,
                                 const TJointInfoList &inAbsoluteJoints)
{
    // clear since we might not use all values
    outValue[0] = outValue[1] = outValue[2] = outValue[3] = 0;

    for (size_t i = 0; i < inFbxWeights.size() && i < 4; i++) {
        size_t relativeIndex = 0;
        // At render time we build a bone array which start always at zero index for each mesh.
        // Therefore we need to remap the absolute bone index to a relative bone index
        for (size_t relJointIdx = 0, relJointEnd = inAbsoluteJoints.size();
             relJointIdx < relJointEnd; ++relJointIdx) {
            if (inAbsoluteJoints[relJointIdx].m_JointID == inFbxWeights[i].m_JointID) {
                relativeIndex = relJointIdx;
                break;
            }
        }
        outValue[i] = (float)relativeIndex;
    }
}

/**
* @brief Process textures parameters
*
* @param[in]  inTexture				Pointer to exture entry
* @param[out] outTextureParameters	Contains information about texture parameter
*
* @return no return.
*/
void FbxDomWalker::ProcessTextureParameter(FbxTexture *inTexture,
                                           STextureParameters &outTextureParameters)
{
    if (inTexture) {
        outTextureParameters.m_Flag = true;
        outTextureParameters.m_offsetU.SetValue(inTexture->GetTranslationU());
        outTextureParameters.m_offsetV.SetValue(inTexture->GetTranslationV());
        outTextureParameters.m_repeatU.SetValue(inTexture->GetScaleU());
        outTextureParameters.m_repeatV.SetValue(inTexture->GetScaleV());
#if 0
        // TODO: These parameters do not work correctly with current math in SetTexture, so they are
        // ignored for now. Created a task (QT3DS-2255) to implement support later, if deemed
        // necessary.
        outTextureParameters.m_rotateUV.SetValue(inTexture->GetRotationW());
        outTextureParameters.m_mirrorU.SetValue(inTexture->GetSwapUV());
        outTextureParameters.m_mirrorV.SetValue(inTexture->GetSwapUV());
#endif
        if (inTexture->GetWrapModeU() == FbxTexture::eRepeat) {
            outTextureParameters.m_wrapU.m_Value = 1;
            outTextureParameters.m_wrapU.m_Flag = true;
        }
        if (inTexture->GetWrapModeV() == FbxTexture::eRepeat) {
            outTextureParameters.m_wrapV.m_Value = 1;
            outTextureParameters.m_wrapV.m_Flag = true;
        }
    }
}

/**
* @brief Process textures of the import file
*
* @param[in]  inMaterial	Pointer to material entry
* @param[in]  fbxMatType	Name of the material type like ambient, diffuse,...
* @param[in]  sgMapType		Convered material type for ambient, diffuse,...
* @param[out] outColor		Contains information if material is color or texture
*
* @return no return.
*/
void FbxDomWalker::ProcessTextures(const FbxSurfaceMaterial *inMaterial, const char *fbxMatType,
                                   ETextureMapType sgMapType, SColorOrTexture &outColor)
{
    FbxProperty theProperty = inMaterial->FindProperty(fbxMatType);

    if (theProperty.IsValid()) {
        STextureParameters theTextureParameters;
        int textureCount = theProperty.GetSrcObjectCount<FbxTexture>();

        if (textureCount)
            outColor.SetTexture();

        for (int i = 0; i < textureCount; ++i) {
            FbxLayeredTexture *theLayeredTexture = theProperty.GetSrcObject<FbxLayeredTexture>(i);
            // do we have layered textures for this property?
            if (theLayeredTexture) {
                int layeredTextureCount = theLayeredTexture->GetSrcObjectCount<FbxLayeredTexture>();
                // texture blending
                for (int j = 0; j < layeredTextureCount; ++j) {
                    FbxTexture *theTexture = theLayeredTexture->GetSrcObject<FbxTexture>(j);
                    if (theTexture) {
                        ProcessTextureParameter(theTexture, theTextureParameters);
                        FbxFileTexture *theFileTexture = FbxCast<FbxFileTexture>(theLayeredTexture);
                        m_Translator->PushTexture(theFileTexture->GetRelativeFileName(),
                                                  theFileTexture->GetFileName(), sgMapType);
                        m_Translator->SetTexture(sgMapType, theTextureParameters);
                        m_Translator->PopTexture();
                    }
                }
            } else {
                FbxTexture *theTexture = theProperty.GetSrcObject<FbxTexture>(i);
                if (theTexture) {
                    ProcessTextureParameter(theTexture, theTextureParameters);
                    FbxFileTexture *theFileTexture = FbxCast<FbxFileTexture>(theTexture);
                    m_Translator->PushTexture(theFileTexture->GetRelativeFileName(),
                                              theFileTexture->GetFileName(), sgMapType);
                    m_Translator->SetTexture(sgMapType, theTextureParameters);
                    m_Translator->PopTexture();
                }
            }
        }
    }
}

/**
* @brief Query material information for a certain face
*
* @param[in] fbxMesh		Pointer to mesh entry
* @param[in/out] info		info contains the face to query.
*							At return info contains the material ID for this
*face
*
* @return no return.
*/
void FbxDomWalker::QueryMaterialInfo(FbxMesh *fbxMesh, SFaceMaterialInfo *info)
{
    for (int l = 0; l < fbxMesh->GetElementMaterialCount(); l++) {
        FbxGeometryElementMaterial *theMaterialElement = fbxMesh->GetElementMaterial(l);
        FbxSurfaceMaterial *theMaterial = nullptr;
        int theMatId = -1;
        theMaterial = fbxMesh->GetNode()->GetMaterial(
            theMaterialElement->GetIndexArray().GetAt(info->m_StartFace));
        theMatId = theMaterialElement->GetIndexArray().GetAt(info->m_StartFace);

        if (theMatId >= 0) {
            info->m_MatElementsIDs.push_back(l);
            info->m_MatIDs.push_back(theMatId);
        }
    }
}

/**
* @brief Read material and attach to mesh
*
* @param[in] inMaterial		Pointer to material entry
* @param[in] startFace		The start face index for this material
* @param[in] faceCount		The face count for this material
*
* @return no return.
*/
void FbxDomWalker::ReadMaterial(const FbxSurfaceMaterial *inMaterial, const int startFace,
                                const int faceCount)
{
    SMaterialParameters theNewMaterial;
    FbxDouble3 fbxColor;
    float attenuationFactor = 0;

    if (!inMaterial)
        return;

    QT3DS_ASSERT(inMaterial->GetClassId().Is(FbxSurfaceMaterial::ClassId)
              || inMaterial->GetClassId().Is(FbxSurfacePhong::ClassId)
              || inMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId));

    // init some base values
    theNewMaterial.m_Ambient.SetColor(0, 0, 0, 1);
    theNewMaterial.m_Diffuse.SetColor(1, 1, 1, 1);

    if (inMaterial->GetClassId().Is(FbxSurfacePhong::ClassId)) {
        // lambert is base for all materials
        // for phong we override the type and read the additional values later below
        FbxSurfaceLambert *lambertMaterial = (FbxSurfaceLambert *)inMaterial;
        theNewMaterial.m_TechniqueType = EMatCommonProfileTechnique_Lambert;

        fbxColor = lambertMaterial->Ambient;
        attenuationFactor = (float)lambertMaterial->AmbientFactor;
        theNewMaterial.m_Ambient.SetColor((float)fbxColor.mData[0] * attenuationFactor,
                                          (float)fbxColor.mData[1] * attenuationFactor,
                                          (float)fbxColor.mData[2] * attenuationFactor, 1);
        fbxColor = lambertMaterial->Diffuse;
        attenuationFactor = (float)lambertMaterial->DiffuseFactor;
        theNewMaterial.m_Diffuse.SetColor((float)fbxColor.mData[0] * attenuationFactor,
                                          (float)fbxColor.mData[1] * attenuationFactor,
                                          (float)fbxColor.mData[2] * attenuationFactor, 1);
        fbxColor = lambertMaterial->Emissive;
        attenuationFactor = (float)lambertMaterial->EmissiveFactor;
        theNewMaterial.m_Emission.SetColor((float)fbxColor.mData[0] * attenuationFactor,
                                           (float)fbxColor.mData[1] * attenuationFactor,
                                           (float)fbxColor.mData[2] * attenuationFactor, 1);
        fbxColor = lambertMaterial->TransparentColor;
        theNewMaterial.m_Transparent.SetColor((float)fbxColor.mData[0], (float)fbxColor.mData[1],
                                              (float)fbxColor.mData[2], 1);
        theNewMaterial.m_Transparency.SetValue((float)lambertMaterial->TransparencyFactor);
    }

    if (inMaterial->GetClassId().Is(FbxSurfacePhong::ClassId)) {
        FbxSurfacePhong *phongMaterial = (FbxSurfacePhong *)inMaterial;
        theNewMaterial.m_TechniqueType = EMatCommonProfileTechnique_Phong;

        fbxColor = phongMaterial->Specular;
        attenuationFactor = (float)phongMaterial->SpecularFactor;
        theNewMaterial.m_Specular.SetColor((float)fbxColor.mData[0] * attenuationFactor,
                                           (float)fbxColor.mData[1] * attenuationFactor,
                                           (float)fbxColor.mData[2] * attenuationFactor, 1);
        fbxColor = phongMaterial->Reflection;
        theNewMaterial.m_Reflective.SetColor((float)fbxColor.mData[0], (float)fbxColor.mData[1],
                                             (float)fbxColor.mData[2], 1);
        theNewMaterial.m_Reflectivity.SetValue((float)phongMaterial->ReflectionFactor);
        theNewMaterial.m_Shininess.SetValue((float)phongMaterial->Shininess);
    }

    m_Translator->PushMaterial(inMaterial->GetName());

    // handle textures if any
    ProcessTextures(inMaterial, FbxSurfaceMaterial::sAmbient, ETextureMapTypeDiffuse,
                    theNewMaterial.m_Ambient);
    ProcessTextures(inMaterial, FbxSurfaceMaterial::sDiffuse, ETextureMapTypeDiffuse,
                    theNewMaterial.m_Diffuse);
    ProcessTextures(inMaterial, FbxSurfaceMaterial::sEmissive, ETextureMapTypeEmissive,
                    theNewMaterial.m_Emission);
    ProcessTextures(inMaterial, FbxSurfaceMaterial::sTransparentColor, ETextureMapTypeOpacity,
                    theNewMaterial.m_Transparent);
    ProcessTextures(inMaterial, FbxSurfaceMaterial::sSpecular, ETextureMapTypeSpecular,
                    theNewMaterial.m_Specular);

    m_Translator->SetMaterial(theNewMaterial);
    m_Translator->PopMaterial(startFace, faceCount);
}

/**
* @brief Add a default material in the import file does not specifiy one
*
* @param[in] inFbxNode		Node to attach the material
* @param[in] startFace		The start face index for this material
* @param[in] faceCount		The face count for this material
*
* @return no return.
*/
void FbxDomWalker::ReadDefaultMaterial(const FbxNode *inFbxNode, const int startFace,
                                       const int faceCount)
{
    SMaterialParameters theNewMaterial;
    theNewMaterial.m_Diffuse.SetColor(0.8f, 0.8f, 0.8f, 1.0f);
    theNewMaterial.m_Transparency.SetValue(0.0); // opaque

    FbxProperty theProperty = inFbxNode->GetFirstProperty();

    // iterate throug the porperty list
    while (theProperty.IsValid()) {
        FbxDataType theFbxType = theProperty.GetPropertyDataType();

        if (FbxColor3DT == theFbxType) {
            FbxDouble3 theDouble3 = theProperty.Get<FbxDouble3>();

            theNewMaterial.m_Diffuse.SetColor((float)theDouble3[0], (float)theDouble3[1],
                                              (float)theDouble3[2], 1);
            break;
        } else if (FbxColor4DT == theFbxType) {
            FbxDouble4 theDouble4 = theProperty.Get<FbxDouble4>();

            theNewMaterial.m_Diffuse.SetColor((float)theDouble4[0], (float)theDouble4[1],
                                              (float)theDouble4[2], (float)theDouble4[3]);
            break;
        }

        theProperty = inFbxNode->GetNextProperty(theProperty);
    }

    std::string theMatName = inFbxNode->GetName();
    theMatName += "_Diffuse_Default";

    m_Translator->PushMaterial(theMatName.c_str());

    m_Translator->SetMaterial(theNewMaterial);
    m_Translator->PopMaterial(startFace, faceCount);
}

/**
* @brief Searches for the animation track indices of a node
*
* @param[in]  inNode		Node to find the animation tracks
* @param[out] outIndicies	List of animation track indices for this node
*
* @return no return.
*/
void FbxDomWalker::GetIndicesFromNode(const FbxNode *inNode, TLongsList &outIndicies)
{
    auto theResult = m_NodeToIndicies.equal_range(inNode);

    outIndicies.clear();

    for (TNodeToIndicesMap::const_iterator theIter = theResult.first; theIter != theResult.second;
         ++theIter) {
        outIndicies.push_back(theIter->second);
    }
}

/**
* @brief Apply animation tracks if node is animated
*
* @param[in] inNode	Node to animate
*
* @return true if animated.
*/
bool FbxDomWalker::ApplyAnimation(const FbxNode *inNode)
{
    TLongsList theAnimationTrackIndicies;
    GetIndicesFromNode(inNode, theAnimationTrackIndicies);
    bool theAnimated = false;

    for (TLongsList::const_iterator theIter = theAnimationTrackIndicies.begin();
         theIter != theAnimationTrackIndicies.end(); ++theIter) {
        theAnimated = true;
        m_Translator->ApplyAnimationTrack(*theIter);
    }

    return theAnimated;
}

/**
* @brief Check if the Fbx node is animated
*
* @param[in] inNode	Node to search for
*
* @return true if animated.
*/
bool FbxDomWalker::IsAnimated(const FbxNode *inNode)
{
    bool isAnimated = false;
    TNodeIsAnimatedMap::iterator theIter;

    theIter = m_NodeIsAnimatedMap.find(inNode);

    if (theIter != m_NodeIsAnimatedMap.end())
        isAnimated = true;

    return isAnimated;
}

/**
* @brief Check if to floats are alomst equal.
*		 We might need to figure a good epsilon
*
* @param[in] lhs	Left hand float
* @param[in] rhs	Right hand float
*
* @return true if almost equal.
*/
inline bool floatAlmostEquals(float lhs, float rhs)
{
    return fabs(lhs - rhs) < .01f;
}

/**
* @brief Check all attached curves of the node if they contains actual animation.
*
* @param[in] inAnimCurveNode	Pointer to animation curve node
*
* @return true or false.
*/
bool FbxDomWalker::CurveNodeIsAnimated(FbxAnimCurveNode *inAnimCurveNode)
{
    bool hasValidAnimation = false;

    for (unsigned int i = 0; i < inAnimCurveNode->GetChannelsCount() && !hasValidAnimation; i++) {
        FbxAnimCurve *theAnimCurve = inAnimCurveNode->GetCurve(i);

        if (theAnimCurve && theAnimCurve->KeyGetCount()) {
            int theKeyCount = theAnimCurve->KeyGetCount();
            // get start value
            FbxAnimCurveKey theKey = theAnimCurve->KeyGet(0);
            float lastValue = theKey.GetValue();

            // Run through all keyframes, compare key values per keyframe.
            for (int i = 1; i < theKeyCount; i++) {
                theKey = theAnimCurve->KeyGet(i);
                float curValue = theKey.GetValue();
                if (!floatAlmostEquals(lastValue, curValue)) {
                    hasValidAnimation = true;
                    break;
                }
                lastValue = curValue;
            }
        }
    }

    return hasValidAnimation;
}

/**
* @brief Filter the key entries of a single animation curve. This creates a flat animation curve
*
* @param[in] inAnimCurve			Pointer to animation curve
* @param[in/out] outKeyIndexList	This vector contains the filtered indices with different
* keys
*
* @return no return.
*/
void FbxDomWalker::FilterAnimCurve(FbxAnimCurve *inAnimCurve, std::vector<int> &outKeyIndexList)
{
    if (inAnimCurve && inAnimCurve->KeyGetCount()) {
        int theKeyCount = inAnimCurve->KeyGetCount();
        // get start value
        FbxAnimCurveKey theKey = inAnimCurve->KeyGet(0);
        float value = theKey.GetValue();

        outKeyIndexList.push_back(0);

        // Run through all keyframes, compare key values per keyframe.
        for (int i = 1; i < theKeyCount - 1; i++) {
            theKey = inAnimCurve->KeyGet(i);
            float curValue = theKey.GetValue();
            if (!floatAlmostEquals(value, curValue)) {
                outKeyIndexList.push_back(i);
                // this is our new value which we compare against
                value = curValue;
            }
        }

        // push last entry to the list
        outKeyIndexList.push_back(theKeyCount - 1);
    }
}

/**
* @brief Write out a single keyframe which we use internaly
*
* @param[in] FbxAnimCurveKey	Pointer to a curve key entry
* @param[in] transformType		Name of the transform type (R, S, T)
* @param[in] channelName		Name of the channel type (X, Y, Z)
* @param[in] inTranslator		Pointer to our scene graph translator
*
* @return no return.
*/
inline void WriteKeyframe(const FbxAnimCurveKey &inKey, const char *transformType,
                          const char *channelName, qt3dsimp::ISceneGraphTranslation *inTranslator)
{
    // Create a key
    SKeyframeParameters theKeyframeInfo((float)inKey.GetTime().GetSecondDouble(), inKey.GetValue(),
                                        0, 0, 0, 0);

    // Set information about the key
    inTranslator->CacheAnimationKey(transformType, channelName, theKeyframeInfo);
}

/**
* @brief Produce keyframes for an animation curve
*
* @param[in] FbxAnimCurveKey	Pointer to a curve key entry
* @param[in] transformType		Name of the transform type (R, S, T)
* @param[in] channelName		Name of the channel type (X, Y, Z)
*
* @return no return.
*/
void FbxDomWalker::ProcessAnimCurve(FbxAnimCurve *inAnimCurve, const char *transformType,
                                    const char *channelName)
{
    if (inAnimCurve && inAnimCurve->KeyGetCount()) {
        int theKeyCount = inAnimCurve->KeyGetCount();

        // Run through all keyframes, writing time and value.
        for (int i = 0; i < theKeyCount; i++) {
            FbxAnimCurveKey theKey = inAnimCurve->KeyGet(i);
            WriteKeyframe(theKey, transformType, channelName, m_Translator);
        }
    }
}

/**
* @brief Produce animation information regarding a node
*
* @param[in] inNode			Pointer to a Fbx node which contains animation
* @param[in] inAnimLayer	Pointer to a Fbx animation layer
*
* @return no return.
*/
void FbxDomWalker::ProcessAnimLayer(FbxNode *inNode, FbxAnimLayer *inAnimLayer)
{
    if (inNode && inAnimLayer) {
        bool isAnimated = false;
        // get animation curve nodes
        FbxAnimCurveNode *theTranslateCurveNode = inNode->LclTranslation.GetCurveNode(inAnimLayer);
        FbxAnimCurveNode *theRotateCurveNode = inNode->LclRotation.GetCurveNode(inAnimLayer);
        FbxAnimCurveNode *theScaleCurveNode = inNode->LclScaling.GetCurveNode(inAnimLayer);
        // color animation
        FbxAnimCurve *redAnimCurve = nullptr;
        FbxAnimCurve *greenAnimCurve = nullptr;
        FbxAnimCurve *blueAnimCurve = nullptr;

        FbxNodeAttribute *nodeAttribute = inNode->GetNodeAttribute();
        if (nodeAttribute) {
            redAnimCurve = nodeAttribute->Color.GetCurve(inAnimLayer, FBXSDK_CURVENODE_COLOR_RED);
            greenAnimCurve =
                nodeAttribute->Color.GetCurve(inAnimLayer, FBXSDK_CURVENODE_COLOR_GREEN);
            blueAnimCurve = nodeAttribute->Color.GetCurve(inAnimLayer, FBXSDK_CURVENODE_COLOR_BLUE);
        }

        if (theTranslateCurveNode && CurveNodeIsAnimated(theTranslateCurveNode)) {
            for (unsigned int i = 0; i < theTranslateCurveNode->GetChannelsCount(); i++) {
                isAnimated = true;

                // For each animated sub property, create a track
                m_NodeToIndicies.insert(std::make_pair(inNode, m_AnimationTrackCount++));
                m_Translator->CacheAnimationTrack();

                m_Translator->SetAnimationTrack(FBXSDK_CURVENODE_TRANSLATION,
                                                theTranslateCurveNode->GetChannelName(i));

                ProcessAnimCurve(theTranslateCurveNode->GetCurve(i), FBXSDK_CURVENODE_TRANSLATION,
                                 theTranslateCurveNode->GetChannelName(i));
            }
        }

        if (theRotateCurveNode && CurveNodeIsAnimated(theRotateCurveNode)) {
            for (unsigned int i = 0; i < theRotateCurveNode->GetChannelsCount(); i++) {
                isAnimated = true;

                // For each animated sub property, create a track
                m_NodeToIndicies.insert(std::make_pair(inNode, m_AnimationTrackCount++));
                m_Translator->CacheAnimationTrack();

                m_Translator->SetAnimationTrack(FBXSDK_CURVENODE_ROTATION,
                                                theRotateCurveNode->GetChannelName(i));

                ProcessAnimCurve(theRotateCurveNode->GetCurve(i), FBXSDK_CURVENODE_ROTATION,
                                 theRotateCurveNode->GetChannelName(i));
            }
        }

        if (theScaleCurveNode && CurveNodeIsAnimated(theScaleCurveNode)) {
            for (unsigned int i = 0; i < theScaleCurveNode->GetChannelsCount(); i++) {
                isAnimated = true;

                // For each animated sub property, create a track
                m_NodeToIndicies.insert(std::make_pair(inNode, m_AnimationTrackCount++));
                m_Translator->CacheAnimationTrack();

                m_Translator->SetAnimationTrack(FBXSDK_CURVENODE_SCALING,
                                                theScaleCurveNode->GetChannelName(i));

                ProcessAnimCurve(theScaleCurveNode->GetCurve(i), FBXSDK_CURVENODE_SCALING,
                                 theScaleCurveNode->GetChannelName(i));
            }
        }

        if (redAnimCurve) {
            isAnimated = true;
            m_NodeToIndicies.insert(std::make_pair(inNode, m_AnimationTrackCount++));
            m_Translator->CacheAnimationTrack();

            m_Translator->SetAnimationTrack(FBXSDK_CURVENODE_COLOR,
                                            theScaleCurveNode->GetChannelName(0));

            ProcessAnimCurve(redAnimCurve, FBXSDK_CURVENODE_COLOR, FBXSDK_CURVENODE_COLOR_RED);
        }
        if (greenAnimCurve) {
            isAnimated = true;
            m_NodeToIndicies.insert(std::make_pair(inNode, m_AnimationTrackCount++));
            m_Translator->CacheAnimationTrack();

            m_Translator->SetAnimationTrack(FBXSDK_CURVENODE_COLOR,
                                            theScaleCurveNode->GetChannelName(0));

            ProcessAnimCurve(greenAnimCurve, FBXSDK_CURVENODE_COLOR, FBXSDK_CURVENODE_COLOR_GREEN);
        }
        if (blueAnimCurve) {
            isAnimated = true;
            m_NodeToIndicies.insert(std::make_pair(inNode, m_AnimationTrackCount++));
            m_Translator->CacheAnimationTrack();

            m_Translator->SetAnimationTrack(FBXSDK_CURVENODE_COLOR,
                                            theScaleCurveNode->GetChannelName(0));

            ProcessAnimCurve(blueAnimCurve, FBXSDK_CURVENODE_COLOR, FBXSDK_CURVENODE_COLOR_BLUE);
        }

        // mark this node as animated
        if (isAnimated)
            m_NodeIsAnimatedMap.insert(std::make_pair(inNode, true));

        // process node children
        for (int i = 0; i < inNode->GetChildCount(); ++i) {
            ProcessAnimLayer(inNode->GetChild(i), inAnimLayer);
        }
    }
}

/**
* @brief Produce animations
*
*
* @return no return.
*/
void FbxDomWalker::ProcessAnimationStacks()
{
    if (m_FbxScene != nullptr) {
        // get animation stack count
        int numAnimStacks = m_FbxScene->GetSrcObjectCount<FbxAnimStack>();

        // iterate throug all animations stacks
        for (int i = 0; i < numAnimStacks; i++) {
            FbxAnimStack *theAnimStack = m_FbxScene->GetSrcObject<FbxAnimStack>(i);

            if (theAnimStack) {
                FbxNode *theRootNode = m_FbxScene->GetRootNode();
                // our internal animation system cannot handle pivots.
                // This means fbx can contain "pre-rotations" which must be applied
                // to the object before the aniamtion is applied. In addition this rotation
                // is inherited to children. This messes up our converion entirely.
                // Convert animation to not contain pivots and such.
                theRootNode->ResetPivotSetAndConvertAnimation();
                int numAnimLayers = theAnimStack->GetMemberCount<FbxAnimLayer>();

                // iterate throug all animations layers
                for (int n = 0; n < numAnimLayers; n++) {
                    FbxAnimLayer *theAnimLayer = theAnimStack->GetMember<FbxAnimLayer>(n);
                    for (int nodeCount = 0; nodeCount < theRootNode->GetChildCount(); nodeCount++) {
                        ProcessAnimLayer(theRootNode->GetChild(nodeCount), theAnimLayer);
                    }
                }
            }
        }
    }
}

} // End anonymous namespace

struct ScopedHelper
{
    ISceneGraphTranslation &m_Helper;
    ScopedHelper(ISceneGraphTranslation &h)
        : m_Helper(h)
    {
    }
    ~ScopedHelper() { m_Helper.Release(); }
};

bool CImportTranslation::ParseFbxFile(const std::string &fileName, Import &import, ISGTranslationLog &log)
{
    ISceneGraphTranslation &transHelper = ISceneGraphTranslation::CreateTranslation(import, log);
    ScopedHelper __scope(transHelper);
    FbxDomWalker theDomWalker(&transHelper);
    if (theDomWalker.LoadDocument(fileName)) {
        theDomWalker.ProcessAnimationStacks();
        theDomWalker.ProcessScene();
        return true;
    }
    return false;
}
#endif
