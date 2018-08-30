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
#include "dae.h"
#include "dom/domCOLLADA.h"
#include "dom/domConstants.h"
#include "dom/domCommon_float_or_param_type.h"
#include "dom/domCommon_transparent_type.h"
#include "Qt3DSMath.h"
#include "Qt3DSImportColladaDOMUtils.h"
#include "Qt3DSImportColladaTransformUtils.h"
#include "Qt3DSImportColladaAnimationUtils.h"
#include "foundation/Qt3DSVec3.h"
#include <memory>
#include <map>
#include <functional>
#include "dom/domElements.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domCommon_color_or_texture_type.h"
#include "Qt3DSImportTranslationCommon.h"
#include "Qt3DSImportSceneGraphTranslation.h"
#include "Qt3DSImportTranslation.h"
#include "Dialogs.h"
#include "StudioApp.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qfileinfo.h>

using namespace qt3dsimp;

namespace {
//==============================================================================
//	Loads Collada document and processes it.
//  It starts 'walking' begining with the visual_scene, and calls various
//  function objects to process each encountered element hierachically.
//  A stack concept is used here to denote the active element being processed.
//  This flows nicely with the XML structure of the Collada file.
//==============================================================================
class ColladaDOMWalker
{
public:
    ColladaDOMWalker(ISceneGraphTranslation *inTranslation);
    ~ColladaDOMWalker();

public:
    typedef std::tuple<SVector3, SVector3, SVector2, SVector3, SVector3, SVector2, SVector4>
        TVertexInfoTuple;
    typedef std::vector<std::pair<std::string, daeURI>> TURIList;
    typedef std::pair<std::map<TVertexInfoTuple, long>,
                      std::vector<std::pair<std::string, TLongsList>>>
        TFaceIndicies;

    typedef std::multimap<const daeElement *, long> TElementToIndicesMap;

public:
    bool LoadDocument(const std::string &inFilePath);
    void ProcessScene();
    void ProcessLibraryAnimations();

protected:
    void SetFColladaAuthoringTool(const char *inName);
    long GetFColladaVersion(std::string theAuthoringToolLowerCase);

    void ProcessVisualScene(domVisual_scene *inVisualScene);
    void ProcessNode(const domNodeRef inNode);
    bool ProcessInstanceGeometry(const domNodeRef inNode,
                                 domInstance_geometryRef inInstanceGeometryRef);
    void ProcessGeometry(const domGeometry *inGeometry, TFaceIndicies &ioIndicies);
    void ProcessTriangle(const domTrianglesRef inTrianglesRef, TFaceIndicies &ioFaceIndicies,
                         bool &outHasNormals, bool &outHasTexCoords, bool &outHasTexCoords2,
                         bool &outHasTexTangents, bool &outHasTexBinormals, bool &outHasColors);
    void GenerateMeshTangents(const domTrianglesRef inTrianglesRef,
                              const SSourceArrayInfo &inVertexArrayInfo,
                              const SSourceArrayInfo &inNormalArrayInfo,
                              const SSourceArrayInfo &inTexCoordArrayInfo,
                              const SSourceArrayInfo &inTexCoord2ArrayInfo,
                              std::vector<SVector3> &outTangents,
                              std::vector<SVector3> &outBinormals);
    void ProcessTransform(const domNodeRef inNode);
    void ProcessBindMaterial(const domBind_materialRef inBindMaterialRef,
                             const TFaceIndicies &inFaceIndicies);
    void ProcessDefaultMaterial(const TFaceIndicies &inFaceIndicies);
    void ProcessInstanceMaterial(const domInstance_materialRef inInstanceMaterialRef);
    void ProcessMaterial(const domMaterial *inMaterial);
    void ProcessInstanceEffect(const domInstance_effectRef inEffectRef);
    void ProcessEffect(const domEffect *inEffect);
    void ProcessProfile(const domFx_profile_abstractRef &inProfile);
    void ProcessProfileCommon(const domProfile_COMMON *inProfileCommon);
    void ProcessProfileCommonTechnique(const domProfile_COMMON::domTechniqueRef inTechniqueRef);
    void ProcessProfileCommonTechniqueExtra(const domProfile_COMMON::domTechniqueRef inTechniqueRef,
                                            SMaterialExtraParameters &outMaterialExtraParameters);
    void
    ProcessTexture(const domCommon_color_or_texture_type_complexType::domTextureRef inTextureRef,
                   long inMapType);
    void ProcessTextureParameters(const domExtraRef inTextureExtraRef,
                                  STextureParameters &outTextureParameters);

    void ProcessAnimations(const domLibrary_animationsRef inLibraryAnimation);
    void ProcessAnimation(const domAnimationRef inAnimation);
    void ProcessChannel(const domChannelRef inChannel);
    void ProcessSampler(const domSampler *inSamplerRef, daeElement *inSourceElement,
                        const char *inBaseProperty, const TStringList &inIdentifiers);

protected:
    void GetColorOrTextureParamInfo(
        const domCommon_color_or_texture_type_complexType *inColorOrTexturePtr,
        SColorOrTexture &outColor, long inMapType);
    daeElement *FindElementByID(const char *inElementID);
    void TrackObjectIndex(const daeElement *inElement, long inIndex);
    void GetIndicesFromElement(const daeElement *inElement, TLongsList &outIndicies);
    bool ApplyAnimation(const daeElement *inContainerElement);

protected:
    std::function<void(const char *)> PushGroup;
    std::function<void()> PopGroup;
    std::function<void(const char *)> PushModel;
    std::function<void()> PopModel;
    std::function<void(const char *)> PushMaterial;
    std::function<void(long, long)> PopMaterial;
    std::function<void(const char *, const char *, long)> PushTexture;
    std::function<void()> PopTexture;
    std::function<void(daeElement *inElement)> CacheAnimationTrack;
    std::function<void(long)> ApplyAnimationTrack;

protected:
    std::function<void()> MarkInvalid;

protected:
    std::function<void(EAuthoringToolType, long)> SetAuthoringTool;
    std::function<void(const TTransformList &inTransforms)> SetTransforms;
    std::function<void(const SMaterialParameters &inMaterialParameters)> SetMaterial;
    std::function<void(long inMapType, const STextureParameters &inTextureParameters)> SetTexture;

    std::function<void(const char *, const char *)> SetAnimationTrack;
    std::function<void(const char *, const char *, const SKeyframeParameters &)>
        CacheAnimationKey;

    static void dummy() {}

protected:
    std::function<void(ESceneGraphWarningCode, const char *)> LogWarning;

protected:
    std::auto_ptr<DAE> m_DAE;
    domCOLLADA *m_ColladaRoot;
    ISceneGraphTranslation *m_Translator;
    const char *m_DAEFilename;
    TElementToIndicesMap m_ElementToIndicies;
};

void FindTexturesViaNewParam(daeElement *inElementPtr, ColladaDOMWalker::TURIList &outTexturePaths)
{
    domCommon_newparam_type *theNewParam = daeSafeCast<domCommon_newparam_type>(inElementPtr);

    if (theNewParam) {
        const domFx_sampler2D_commonRef theSampler2DRef = theNewParam->getSampler2D();

        if (theSampler2DRef) // Found a <sampler2D>, follow through to get to <surface>
        {
            const domFx_sampler2D_common_complexType::domSourceRef theSourceRef =
                theSampler2DRef->getSource();
            if (theSourceRef) {
                const xsNCName theSourceSid = theSourceRef->getValue();

                daeElement *theSurfaceNewParamPtr = NULL;
                if (RecursiveFindElementBySid(theSourceSid, theSurfaceNewParamPtr,
                                              theNewParam->getParent(), -1, domEffect::ID())) {
                    FindTexturesViaNewParam(theSurfaceNewParamPtr, outTexturePaths);
                }
            }
        } else {
            const domFx_surface_commonRef theSurfaceRef = theNewParam->getSurface();
            if (theSurfaceRef) // Found a <surface>, get the referenced texture from
                               // <library_images>
            {
                const domFx_surface_init_commonRef theSurfaceInitCommonRef =
                    theSurfaceRef->getFx_surface_init_common(); // look for <init_XXX>s
                if (theSurfaceInitCommonRef) {
                    const domFx_surface_init_from_common_Array &theInitFromArray =
                        theSurfaceInitCommonRef->getInit_from_array(); // look for <init_from>s
                    long theTextureCount = (long)theInitFromArray.getCount();
                    for (long theIndex = 0; theIndex < theTextureCount; ++theIndex) {
                        const xsIDREF &theTextureRef = theInitFromArray[theIndex]->getValue();
                        domImage *theImage = daeSafeCast<domImage>(theTextureRef.getElement());
                        if (theImage) {
                            const domImage::domInit_fromRef theInitFromRef =
                                theImage->getInit_from();
                            if (theInitFromRef)
                                outTexturePaths.push_back(
                                    std::make_pair(theImage->getID(), theInitFromRef->getValue()));
                        }
                    }
                }
            }
        }
    }
}

//==============================================================================
/**
 *	Returns a unique face index which is made up of a vertex, normal and
 *	texcoord index tuple.
 *	The order of ioFaceIndicies.first eventually becomes the order which we will
 *	fill the vertex, normal and texcoord arrays for Model.
 */
long RetrieveFaceIndex(ColladaDOMWalker::TFaceIndicies &ioFaceIndicies,
                       const ColladaDOMWalker::TVertexInfoTuple &inFaceValues)
{
    std::pair<ColladaDOMWalker::TFaceIndicies::first_type::iterator, bool> theTupleEntry;

    theTupleEntry = ioFaceIndicies.first.insert(
        std::make_pair(inFaceValues, (long)(ioFaceIndicies.first.size())));

    return theTupleEntry.first->second;
}

//==============================================================================
/**
 *	CTOR
 *	Sets up the ISceneGraphTranslation class and binds function objects to
 *	the actual functions within that class.
 */
ColladaDOMWalker::ColladaDOMWalker(ISceneGraphTranslation *inTranslation)
    : m_ColladaRoot(NULL)
    , m_Translator(inTranslation)
{
    PushGroup = std::bind(&ISceneGraphTranslation::PushGroup, m_Translator, std::placeholders::_1);
    PopGroup = std::bind(&ISceneGraphTranslation::PopGroup, m_Translator);
    PushModel = std::bind(&ISceneGraphTranslation::PushModel, m_Translator, std::placeholders::_1);
    PopModel = std::bind(&ISceneGraphTranslation::PopModel, m_Translator);

    PushMaterial = std::bind(&ISceneGraphTranslation::PushMaterial, m_Translator,
                             std::placeholders::_1);
    PopMaterial = std::bind(&ISceneGraphTranslation::PopMaterial, m_Translator,
                            std::placeholders::_1, std::placeholders::_2);
    PushTexture = std::bind(&ISceneGraphTranslation::PushTexture, m_Translator,
                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    PopTexture = std::bind(&ISceneGraphTranslation::PopTexture, m_Translator);

    CacheAnimationTrack =
        std::bind(&ColladaDOMWalker::TrackObjectIndex, this, std::placeholders::_1,
                    std::bind(&ISceneGraphTranslation::CacheAnimationTrack, m_Translator));
    ApplyAnimationTrack =
        std::bind(&ISceneGraphTranslation::ApplyAnimationTrack, m_Translator,
                  std::placeholders::_1);

    MarkInvalid = std::bind(&ISceneGraphTranslation::MarkInvalid, m_Translator);

    SetAuthoringTool = std::bind(&ISceneGraphTranslation::SetAuthoringTool, m_Translator,
                                 std::placeholders::_1, std::placeholders::_2);

    SetTransforms = std::bind(&ISceneGraphTranslation::SetTransforms, m_Translator,
                              std::placeholders::_1);

    SetMaterial = std::bind(&ISceneGraphTranslation::SetMaterial, m_Translator,
                            std::placeholders::_1);
    SetTexture = std::bind(&ISceneGraphTranslation::SetTexture, m_Translator,
                           std::placeholders::_1, std::placeholders::_2);

    SetAnimationTrack =
        std::bind(&ISceneGraphTranslation::SetAnimationTrack, m_Translator,
                  std::placeholders::_1, std::placeholders::_2);
    CacheAnimationKey =
        std::bind(&ISceneGraphTranslation::CacheAnimationKey, m_Translator,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    LogWarning = std::bind(&ISceneGraphTranslation::LogWarning, m_Translator,
                           std::placeholders::_1, std::placeholders::_2);
}

//==============================================================================
/**
 *	DTOR
 */
ColladaDOMWalker::~ColladaDOMWalker()
{
}

//==============================================================================
/**
 *	Loads a Collada document into the Collada DOM.
 */
bool ColladaDOMWalker::LoadDocument(const std::string &inFilePath)
{
    if (QFileInfo(QString::fromStdString(inFilePath)).exists()) {
        m_DAE.reset(new DAE);
        QFile file(QString::fromStdString(inFilePath));
        file.open(QIODevice::ReadOnly);
        m_ColladaRoot = m_DAE->openFromMemory141(inFilePath,file.readAll().constData());
        file.close();
        m_DAEFilename = inFilePath.c_str();

        // Extract authoring_tool information
        const domAssetRef theAssetRef = m_ColladaRoot->getAsset();
        if (theAssetRef) {
            const domAsset::domContributor_Array theContributorArray =
                theAssetRef->getContributor_array();
            long theContributorCount = (long)theContributorArray.getCount();
            for (long theIndex = 0; theIndex < (long)theContributorCount; ++theIndex) {
                const domAsset::domContributor::domAuthoring_toolRef theAuthoringTool =
                    theContributorArray[theIndex]->getAuthoring_tool();
                if (theAuthoringTool) {
                    // There may be > 1 authoring tool in dae. But currently only take the 1st one
                    // we find
                    SetFColladaAuthoringTool(theAuthoringTool->getValue());
                    break;
                }
            }
        }
        return true;
    }

    return false;
}

//==============================================================================
/**
 *	Starts the import process.
 *	This begins by looking at the active visual_scene node and starting import
 *	from that node.
 */
void ColladaDOMWalker::ProcessScene()
{
    if (m_ColladaRoot != NULL) {
        const domCOLLADA::domSceneRef theScene = m_ColladaRoot->getScene();
        // Retrieve the active visual_scene
        const domInstanceWithExtraRef theInstanceVisualScene = theScene->getInstance_visual_scene();
        const xsAnyURI &theVisualSceneURI = theInstanceVisualScene->getUrl();
        const daeElementRef theVisualSceneRef = theVisualSceneURI.getElement();
        domVisual_scene *theVisualScene = daeSafeCast<domVisual_scene>(theVisualSceneRef);

        // Get the up axis for this COLLADA doc
        // TODO: Collada specs does allow up-axis to vary within the DAE, but I have not seen a DAE
        // that does that.
        //       We might need to support varying up-axis in the future.
        domUpAxisType theAxisType = UPAXISTYPE_Y_UP;
        const domAssetRef theAssetRef = m_ColladaRoot->getAsset();
        if (theAssetRef) {
            const domAsset::domUp_axisRef theUpAxisRef = theAssetRef->getUp_axis();
            if (theUpAxisRef)
                theAxisType = theUpAxisRef->getValue();
        }

        // Disable wrapping the visual scene in it's own group, since m_TopMostParent exists
        // We might encounter DAEs that have multiple <visual_scene>s
        // PushGroup( GetNameOrIDOrSid( theVisualScene ) );

        switch (theAxisType) {
        // TODO: What about X up?
        case UPAXISTYPE_Z_UP: {
            std::vector<INodeTransform *> theTransforms;

            NodeTransform *theRotX = new NodeTransform(ETransformType_Rotation4);
            (*theRotX)[0] = 1.0f;
            (*theRotX)[1] = (*theRotX)[2] = 0.0f;
            (*theRotX)[3] =
                -90.0f; // Apply a -90 degree rotation on the X-axis to 'right' the model
            theTransforms.push_back(theRotX);

            SetTransforms(theTransforms);
            delete theRotX;
        } break;
        default:
            break;
        }

        ProcessVisualScene(theVisualScene);

        // Disable wrapping the visual scene in it's own group, since m_TopMostParent exists
        // PopGroup( );
    }
}

void ColladaDOMWalker::SetFColladaAuthoringTool(const char *inName)
{
    // There may be > 1 authoring tool in dae. But currently only take the 1st one we find

    // Guess the authoring tool type & version
    // This looks very hackish because there is no standard and it is based on the test data that I
    // have
    const std::string theAuthoringToolLowerCase = QByteArray::fromRawData(
                inName, int(strlen(inName))).toLower().toStdString();

    EAuthoringToolType theAuthoringToolType = EAuthoringToolType_Unknown;
    if (theAuthoringToolLowerCase.find("studiocore") != std::string::npos) {
        theAuthoringToolType = EAuthoringToolType_StudioCORE;
    } else if (theAuthoringToolLowerCase.find("cinema4d") != std::string::npos) {
        theAuthoringToolType = EAuthoringToolType_Cinema4D;
    } else if (theAuthoringToolLowerCase.find("sketchup") != std::string::npos) {
        theAuthoringToolType = EAuthoringToolType_SketchUp;
    } else if (theAuthoringToolLowerCase.find("nextgen") != std::string::npos) {
        if (theAuthoringToolLowerCase.find("max") != std::string::npos) {
            // Max NextGen 0.9.5 will export something like this:
            // <authoring_tool>COLLADAMax NextGen;  Version: 0.9.5;  Revision: 447;  Platform:
            // Win32;  Configuration: Release Max2009</authoring_tool>
            theAuthoringToolType = EAuthoringToolType_NextGen_Max;
        } else // if ( theAuthoringToolLowerCase.find( "maya" ) != std::string::npos )
        {
            theAuthoringToolType = EAuthoringToolType_NextGen_Maya;
        }
    } else if (theAuthoringToolLowerCase.find("feeling") != std::string::npos
               || theAuthoringToolLowerCase.find("fcollada") != std::string::npos) {
        if (theAuthoringToolLowerCase.find("max") != std::string::npos) {
            // ColladaMax 3.0.5C will export something like this:
            // <authoring_tool>3dsMax 11 - Feeling ColladaMax v3.05B.</authoring_tool>
            theAuthoringToolType = EAuthoringToolType_FCollada_Max;
        } else // if ( theAuthoringToolLowerCase.find( "maya" ) != std::string::npos )
        {
            // <authoring_tool>Maya 8.5 | ColladaMaya v3.02 | FCollada v3.2</authoring_tool>
            theAuthoringToolType = EAuthoringToolType_FCollada_Maya;
        }
    } else if (theAuthoringToolLowerCase.find("collada maya") != std::string::npos) {
        // Maya NextGen 0.9.5 will export something like this:
        // <authoring_tool>COLLADA Maya2009</authoring_tool>
        theAuthoringToolType = EAuthoringToolType_NextGen_Maya;
    } else if (theAuthoringToolLowerCase.find("colladamaya") != std::string::npos) {
        // ColladaMaya 3.05C will export something like this:
        // <authoring_tool>Maya2008 | ColladaMaya v3.05B</authoring_tool>
        // Other version of ColladaMaya:
        // <authoring_tool>Maya8.5 | ColladaMaya v3.04E</authoring_tool>
        theAuthoringToolType = EAuthoringToolType_FCollada_Maya;
    } else if (theAuthoringToolLowerCase.find("colladamax") != std::string::npos) {
        theAuthoringToolType = EAuthoringToolType_FCollada_Max;
    } else if (theAuthoringToolLowerCase.find("opencollada") != std::string::npos
               && theAuthoringToolLowerCase.find("3ds max") != std::string::npos) {
        theAuthoringToolType = EAuthoringToolType_FCollada_Max;
    }

    long theAuthoringToolVersion = 0;
    if (theAuthoringToolType == EAuthoringToolType_FCollada_Max
        || theAuthoringToolType == EAuthoringToolType_FCollada_Maya)
        theAuthoringToolVersion = GetFColladaVersion(theAuthoringToolLowerCase);

    SetAuthoringTool(theAuthoringToolType, theAuthoringToolVersion);
}

long ColladaDOMWalker::GetFColladaVersion(std::string theAuthoringToolLowerCase)
{
    // Quick & dirty way to get the version
    if (theAuthoringToolLowerCase.find("v3.05") != std::string::npos)
        return 305;
    else if (theAuthoringToolLowerCase.find("v3.04") != std::string::npos)
        return 304;
    else if (theAuthoringToolLowerCase.find("v3.03") != std::string::npos)
        return 303;
    else if (theAuthoringToolLowerCase.find("v3.02") != std::string::npos)
        return 302;
    else if (theAuthoringToolLowerCase.find("v3.01") != std::string::npos)
        return 301;

    return 0;
}

//==============================================================================
/**
 *	Process nodes in the visual_scene.
 */
void ColladaDOMWalker::ProcessVisualScene(domVisual_scene *inVisualScene)
{
    const domNode_Array &theNodes = inVisualScene->getNode_array();
    long theNodeCount = (long)theNodes.getCount();

    for (long theIndex = 0; theIndex < theNodeCount; ++theIndex) {
        ProcessNode(theNodes[theIndex]);
    }
}

//==============================================================================
/**
 *	Process a <node> object.
 *	This ia recursive and may end up creating parent-child node objects.
 */
void ColladaDOMWalker::ProcessNode(const domNodeRef inNode)
{
    switch (inNode->getType()) {
    case NODETYPE_NODE: {
        const domInstance_geometry_Array &theInstanceGeometryArray =
            inNode->getInstance_geometry_array();
        long theGeometryCount = (long)theInstanceGeometryArray.getCount();
        bool thePushModelFlag = false;

        if (theGeometryCount == 1) // Just a single model
        {
            thePushModelFlag = true;
            PushModel(GetNameOrIDOrSid(inNode));
            ProcessTransform(inNode);
            if (!ProcessInstanceGeometry(inNode, theInstanceGeometryArray[0])) {
                // The model doesn't have valid geometry. Pop it then process it as group
                PopModel();
                thePushModelFlag = false;
                PushGroup(GetNameOrIDOrSid(inNode));
                ProcessTransform(inNode);
            }
        } else // Multiple models
        {
            thePushModelFlag = false;
            PushGroup(GetNameOrIDOrSid(inNode));
            ProcessTransform(inNode);

            for (long theIndex = 0; theIndex < theGeometryCount; ++theIndex) {
                const domInstance_geometryRef theInstanceGeometryRef =
                    theInstanceGeometryArray[theIndex];
                PushModel(GetNameOrIDOrSid(theInstanceGeometryRef));
                ProcessInstanceGeometry(inNode, theInstanceGeometryRef);
                PopModel();
            }
        }

        const domNode_Array &theNodes = inNode->getNode_array();
        long theNodesCount = (long)theNodes.getCount();

        for (long theIndex = 0; theIndex < theNodesCount; ++theIndex) {
            ProcessNode(theNodes[theIndex]);
        }

        if (thePushModelFlag)
            PopModel();
        else
            PopGroup();
    } break;
    default: // No support for other types currently
        break;
    }
}

//==============================================================================
/**
 *	Processes transform elements on a <node>.
 */
void ColladaDOMWalker::ProcessTransform(const domNodeRef inNode)
{
    std::vector<INodeTransform *> theTransforms;

    daeTArray<daeSmartRef<daeElement>> theChildren = inNode->getChildren();
    long theChildCount = (long)theChildren.getCount();
    for (long theIndex = 0; theIndex < theChildCount; ++theIndex) {
        daeElement *theChild = theChildren[theIndex];

        if (!ApplyAnimation(theChild)) // if there is animation, do not process transformation
                                       // because animation overwrites transformation
        {
            switch (GetElementType(*theChild)) {
            case COLLADA_TYPE::MATRIX:
                theTransforms.push_back(new NodeTransform(ETransformType_Matrix4x4));
                ProcessMatrix(theChild, *theTransforms.back());
                break;
            case COLLADA_TYPE::ROTATE:
                theTransforms.push_back(new NodeTransform(ETransformType_Rotation4));
                ProcessRotation(theChild, *theTransforms.back());
                break;
            case COLLADA_TYPE::SCALE:
                theTransforms.push_back(new NodeTransform(ETransformType_Scale3));
                ProcessScale(theChild, *theTransforms.back());
                break;
            case COLLADA_TYPE::SKEW:
                // TODO: Skew??
                break;
            case COLLADA_TYPE::TRANSLATE:
                theTransforms.push_back(new NodeTransform(ETransformType_Translate3));
                ProcessTranslation(theChild, *theTransforms.back());
                break;
            default:
                break;
            }
        }
    }

    SetTransforms(theTransforms);

    std::for_each(theTransforms.begin(), theTransforms.end(), NodeTransform::Delete);
}

//==============================================================================
/**
 *	Attempts to locate a daeElement via it's string ID from the collada root.
 */
daeElement *ColladaDOMWalker::FindElementByID(const char *inElementID)
{
    daeDefaultIDRefResolver theIDResolver(*m_DAE);

    daeElement *theElement =
        theIDResolver.resolveElement(inElementID, m_ColladaRoot->getDocument());

    return theElement;
}

//==============================================================================
/**
 *	Processes <instance_geometry> on a <node>
 *	Returns true if the model has valid geometry
 */
bool ColladaDOMWalker::ProcessInstanceGeometry(const domNodeRef inNode,
                                               domInstance_geometryRef inInstanceGeometryRef)
{
    TFaceIndicies theFaceIndicies;

    const xsAnyURI &theGeometryURI = inInstanceGeometryRef->getUrl();
    const daeElementRef theGeometryRef = theGeometryURI.getElement();
    const domGeometry *theGeometry = daeSafeCast<domGeometry>(theGeometryRef);
    ProcessGeometry(theGeometry, theFaceIndicies);

    if (theFaceIndicies.first.size() == 0) {
        MarkInvalid(); // Mark the Model as invalid
        return false; // No valid geometry
    }

    const domBind_materialRef theBindMaterialRef = inInstanceGeometryRef->getBind_material();
    if (theBindMaterialRef) // <bind_material> is present, use it instead
        ProcessBindMaterial(theBindMaterialRef, theFaceIndicies);
    else // Use material defined in <triangles>
        ProcessDefaultMaterial(theFaceIndicies);

    return true;
}

//==============================================================================
/**
 *	Processes <geometry> referenced from an <instance_geometry>
 */
void ColladaDOMWalker::ProcessGeometry(const domGeometry *inGeometry, TFaceIndicies &ioFaceIndicies)
{
    TFloatsList theVertices;
    TFloatsList theNormals;
    TFloatsList theTexCoords;
    TFloatsList theTexTangents;
    TFloatsList theTexBinormals;
    TFloatsList theWeights;
    TFloatsList theBoneIndex;
    TFloatsList theTexCoords2;
    TFloatsList theColors;

    bool theHasNormals = false;
    bool theHasTexCoords = false;
    bool theHasTexCoords2 = false;
    bool theHasTexTangents = false;
    bool theHasTexBinormals = false;
    bool theHasColors = false;

    const domMeshRef theMesh = inGeometry->getMesh();

    // TODO: Support <lines> element?

    // Process <triangles> element
    const domTriangles_Array &theTriangles = theMesh->getTriangles_array();
    long theTriangleArrayCount = (long)theTriangles.getCount();
    for (long theIndex = 0; theIndex < theTriangleArrayCount; ++theIndex) {
        ProcessTriangle(theTriangles[theIndex], ioFaceIndicies, theHasNormals, theHasTexCoords,
                        theHasTexCoords2, theHasTexTangents, theHasTexBinormals, theHasColors);
    }

    // Prepare arrays for population
    long theNumberOfUniqueFacePoints = (long)ioFaceIndicies.first.size();
    if (theNumberOfUniqueFacePoints == 0)
        return;

    theVertices.resize(theNumberOfUniqueFacePoints * 3);
    if (theHasNormals)
        theNormals.resize(theNumberOfUniqueFacePoints * 3);
    if (theHasTexCoords)
        theTexCoords.resize(theNumberOfUniqueFacePoints * 2);
    if (theHasTexCoords2)
        theTexCoords2.resize(theNumberOfUniqueFacePoints * 2);
    if (theHasTexTangents)
        theTexTangents.resize(theNumberOfUniqueFacePoints * 3);
    if (theHasTexBinormals)
        theTexBinormals.resize(theNumberOfUniqueFacePoints * 3);
    if (theHasColors)
        theColors.resize(theNumberOfUniqueFacePoints * 3);

    // Populate vertex, normal and texcoord arrays so that face indicies can reference from them
    TFaceIndicies::first_type::const_iterator theIter = ioFaceIndicies.first.begin();
    TFaceIndicies::first_type::const_iterator theEnd = ioFaceIndicies.first.end();
    for (; theIter != theEnd; ++theIter) {
        long theFaceIndex = theIter->second;
        const TVertexInfoTuple &thePointTuple = theIter->first;

        Push3Floats(theVertices, theFaceIndex, get<0>(thePointTuple));

        if (theHasNormals)
            Push3Floats(theNormals, theFaceIndex, get<1>(thePointTuple));
        if (theHasTexCoords)
            Push2Floats(theTexCoords, theFaceIndex, get<2>(thePointTuple));
        if (theHasTexTangents)
            Push3Floats(theTexTangents, theFaceIndex, get<3>(thePointTuple));
        if (theHasTexBinormals)
            Push3Floats(theTexBinormals, theFaceIndex, get<4>(thePointTuple));
        if (theHasTexCoords2)
            Push2Floats(theTexCoords2, theFaceIndex, get<5>(thePointTuple));
        if (theHasColors)
            Push3Floats(theColors, theFaceIndex, get<6>(thePointTuple));
    }

    // Lump all acquired face indicies from different materials, into a single list
    long theMaterialCount = (long)ioFaceIndicies.second.size();
    TLongsList theEntireFaceIndiciesList;
    for (long theIndex = 0; theIndex < theMaterialCount; ++theIndex) {
        theEntireFaceIndiciesList.insert(theEntireFaceIndiciesList.end(),
                                         ioFaceIndicies.second[theIndex].second.begin(),
                                         ioFaceIndicies.second[theIndex].second.end());
    }

    m_Translator->SetGeometry(theVertices, theNormals, theTexCoords, theTexCoords2, theTexTangents,
                              theTexBinormals, theWeights, theBoneIndex, theColors,
                              theEntireFaceIndiciesList);

    // Pop up warning message if model contains non-triangles geometry
    if (theMesh->getPolylist_array().getCount() || theMesh->getPolygons_array().getCount()
        || theMesh->getLines_array().getCount() || theMesh->getLinestrips_array().getCount()
        || theMesh->getTrifans_array().getCount() || theMesh->getTristrips_array().getCount()) {
        LogWarning(ESceneGraphWarningCode_OnlySupportTriangles, GetNameOrIDOrSid(inGeometry));
    }
}

//==============================================================================
/**
 *	Processes the <triangle> element.
 */
void ColladaDOMWalker::ProcessTriangle(const domTrianglesRef inTrianglesRef,
                                       TFaceIndicies &ioFaceIndicies, bool &outHasNormals,
                                       bool &outHasTexCoords, bool &outHasTexCoords2,
                                       bool &outHasTexTangents, bool &outHasTexBinormals,
                                       bool &outHasColors)
{
    SSourceArrayInfo theVertexArrayInfo;
    SSourceArrayInfo theNormalArrayInfo;
    SSourceArrayInfo theTexCoordArrayInfo;
    SSourceArrayInfo theTexCoord2ArrayInfo;
    SSourceArrayInfo theTexTangentArrayInfo;
    SSourceArrayInfo theTexBinormalArrayInfo;
    SSourceArrayInfo theColorArrayInfo;

    const domInputLocalOffset_Array &theInputOffsets = inTrianglesRef->getInput_array();
    long theInputOffsetsCount = (long)theInputOffsets.getCount();
    long theMaxOffset = 0; // Some <input>s share the same offset, so we want to advance our <p> by
                           // the max offset that we find
    long theVertexArrayOffset = -1;

    // Retrieve the appropriate source array depending on the semantic name (inputs with offsets)
    for (long theIndex = 0; theIndex < theInputOffsetsCount; ++theIndex) {
        const char *theSemantic = theInputOffsets[theIndex]->getSemantic();
        long theCurrentOffset = (long)theInputOffsets[theIndex]->getOffset();
        theMaxOffset =
            Q3DStudio::MAX(theCurrentOffset, theMaxOffset); // Retrieve the max offset amount

        if (::strcmp(theSemantic, "VERTEX") == 0) {
            // VERTEX require one more lookup to find it's POSITION <source>
            theVertexArrayOffset = theCurrentOffset;
        } else if (::strcmp(theSemantic, "NORMAL") == 0) {
            GetDomSourceArrayAndVectorCapacity(theInputOffsets[theIndex], g_XYZIdentifiers,
                                               theNormalArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXCOORD") == 0 && theTexCoordArrayInfo.m_POffset == -1) {
            GetDomSourceArrayAndVectorCapacity(theInputOffsets[theIndex], g_STIdentifiers,
                                               theTexCoordArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXCOORD") == 0
                   && theTexCoord2ArrayInfo.m_POffset == -1) {
            GetDomSourceArrayAndVectorCapacity(theInputOffsets[theIndex], g_STIdentifiers,
                                               theTexCoord2ArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXTANGENT") == 0) {
            GetDomSourceArrayAndVectorCapacity(theInputOffsets[theIndex], g_XYZIdentifiers,
                                               theTexTangentArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXBINORMAL") == 0) {
            GetDomSourceArrayAndVectorCapacity(theInputOffsets[theIndex], g_XYZIdentifiers,
                                               theTexBinormalArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "COLOR") == 0) {
            GetDomSourceArrayAndVectorCapacity(theInputOffsets[theIndex], g_RGBAIdentifiers,
                                               theColorArrayInfo, LogWarning);
        }
    }

    ++theMaxOffset; // Increment offset by 1 which becomes the stride of the <p> list

    // Retrieve inputs without offsets in vertices
    domMesh *theMesh = (domMesh *)theInputOffsets[0]->getParentElement()->getParentElement();
    domVertices *theVertices = theMesh->getVertices();
    domInputLocal_Array &theVerticesInputArray = theVertices->getInput_array();
    long theInputArrayCount = (long)theVerticesInputArray.getCount();
    for (long theInputArrayIndex = 0; theInputArrayIndex < theInputArrayCount;
         ++theInputArrayIndex) {
        const char *theSemantic = theVerticesInputArray[theInputArrayIndex]->getSemantic();
        domSource *theDomSource = daeSafeCast<domSource>(
            theVerticesInputArray[theInputArrayIndex]->getSource().getElement());
        if (::strcmp(theSemantic, "POSITION") == 0) {
            GetSourceArrayInfo(theDomSource, g_XYZIdentifiers, theVertexArrayOffset,
                               theVertexArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "NORMAL") == 0) {
            GetSourceArrayInfo(theDomSource, g_XYZIdentifiers, theVertexArrayOffset,
                               theNormalArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXCOORD") == 0 && theTexCoordArrayInfo.m_POffset == -1) {
            GetSourceArrayInfo(theDomSource, g_STIdentifiers, theVertexArrayOffset,
                               theTexCoordArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXCOORD") == 0
                   && theTexCoord2ArrayInfo.m_POffset == -1) {
            GetSourceArrayInfo(theDomSource, g_STIdentifiers, theVertexArrayOffset,
                               theTexCoord2ArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXTANGENT") == 0) {
            GetSourceArrayInfo(theDomSource, g_XYZIdentifiers, theVertexArrayOffset,
                               theTexTangentArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "TEXBINORMAL") == 0) {
            GetSourceArrayInfo(theDomSource, g_XYZIdentifiers, theVertexArrayOffset,
                               theTexBinormalArrayInfo, LogWarning);
        } else if (::strcmp(theSemantic, "COLOR") == 0) {
            GetSourceArrayInfo(theDomSource, g_XYZIdentifiers, theVertexArrayOffset,
                               theColorArrayInfo, LogWarning);
        }
    }

    outHasNormals |= theNormalArrayInfo.m_POffset > -1;
    outHasTexCoords |= theTexCoordArrayInfo.m_POffset > -1;
    outHasTexCoords2 |= theTexCoord2ArrayInfo.m_POffset > -1;
    outHasTexTangents |= theTexTangentArrayInfo.m_POffset > -1;
    outHasTexBinormals |= theTexBinormalArrayInfo.m_POffset > -1;
    outHasColors |= theColorArrayInfo.m_POffset > -1;

    // check if we need to generate tangents and binormals
    std::vector<SVector3> newTangents;
    std::vector<SVector3> newBinormals;
    if ((theTexTangentArrayInfo.m_POffset == -1) && (theTexBinormalArrayInfo.m_POffset == -1)) {
        GenerateMeshTangents(inTrianglesRef, theVertexArrayInfo, theNormalArrayInfo,
                             theTexCoordArrayInfo, theTexCoord2ArrayInfo, newTangents,
                             newBinormals);
    }

    // Organize face points into face indicies.
    TLongsList theMaterialFaceIndicies;
    theMaterialFaceIndicies.reserve((long)inTrianglesRef->getCount());

    const domPRef thePRef = inTrianglesRef->getP();
    const domListOfUInts &theListOfPInts = thePRef->getValue();
    long thePIntsCount = (long)theListOfPInts.getCount();
    for (long theIndex = 0; theIndex < thePIntsCount; theIndex += theMaxOffset) {
        TVertexInfoTuple theFaceTupleValues;

        GetFaceTupleValue(get<0>(theFaceTupleValues), theVertexArrayInfo,
                          (unsigned long)theListOfPInts[theIndex + theVertexArrayInfo.m_POffset]);

        if (outHasNormals)
            GetFaceTupleValue(
                get<1>(theFaceTupleValues), theNormalArrayInfo,
                (unsigned long)theListOfPInts[theIndex + theNormalArrayInfo.m_POffset]);

        if (outHasTexCoords)
            GetFaceTupleValue(
                get<2>(theFaceTupleValues), theTexCoordArrayInfo,
                (unsigned long)theListOfPInts[theIndex + theTexCoordArrayInfo.m_POffset]);

        // first check if we generated the tangents
        if (newTangents.size() > 0)
            GetFaceTupleValue(
                get<3>(theFaceTupleValues), newTangents,
                (unsigned long)theListOfPInts[theIndex + theVertexArrayInfo.m_POffset]);
        else if (outHasTexTangents)
            GetFaceTupleValue(
                get<3>(theFaceTupleValues), theTexTangentArrayInfo,
                (unsigned long)theListOfPInts[theIndex + theTexTangentArrayInfo.m_POffset]);

        // first check if we generated the binormals
        if (newBinormals.size() > 0)
            GetFaceTupleValue(
                get<4>(theFaceTupleValues), newBinormals,
                (unsigned long)theListOfPInts[theIndex + theVertexArrayInfo.m_POffset]);
        else if (outHasTexBinormals)
            GetFaceTupleValue(
                get<4>(theFaceTupleValues), theTexBinormalArrayInfo,
                (unsigned long)theListOfPInts[theIndex + theTexBinormalArrayInfo.m_POffset]);

        if (outHasTexCoords2)
            GetFaceTupleValue(
                get<5>(theFaceTupleValues), theTexCoord2ArrayInfo,
                (unsigned long)theListOfPInts[theIndex + theTexCoord2ArrayInfo.m_POffset]);

        if (outHasColors)
            GetFaceTupleValue(
                get<6>(theFaceTupleValues), theColorArrayInfo,
                (unsigned long)theListOfPInts[theIndex + theColorArrayInfo.m_POffset]);

        long theFaceIndex = RetrieveFaceIndex(ioFaceIndicies, theFaceTupleValues);

        theMaterialFaceIndicies.push_back(theFaceIndex);
    }

    outHasTexTangents |= newTangents.size() > 0;
    outHasTexBinormals |= newBinormals.size() > 0;

    // Set the face indicies used by this particular material
    const xsNCName theMaterialName =
        inTrianglesRef->getMaterial(); // TODO: Handle the material settings for this face
    if (theMaterialName != nullptr) {
        ioFaceIndicies.second.push_back(std::make_pair(theMaterialName, theMaterialFaceIndicies));
    } else {
        g_StudioApp.GetDialogs()->DisplayKnownErrorDialog(
                    QObject::tr("The mesh files could not be created.\n"
                                "Materials are missing from the imported model."));
    }
}

/**
* @brief computer tangents and binormals from mesh input data like
*		 normals and uv's.
*		 If you are interested in how it works check
*		 "Math for 3D Game programming and CG"
*
* @param[in]	 inTrianglesRef			pointer to triangle data
* @param[in]	 inVertexArrayInfo		vertex array info
* @param[in]	 inNormalArrayInfo		normal array info
* @param[in]	 inTexCoordArrayInfo	tex coord array info
* @param[in/out] outTangents			output vectors of tangents
* @param[in/out] outBinormals			output vectors of binormals
*
* @return none
*/
void ColladaDOMWalker::GenerateMeshTangents(const domTrianglesRef inTrianglesRef,
                                            const SSourceArrayInfo &inVertexArrayInfo,
                                            const SSourceArrayInfo &inNormalArrayInfo,
                                            const SSourceArrayInfo &inTexCoordArrayInfo,
                                            const SSourceArrayInfo &inTexCoordArray2Info,
                                            std::vector<SVector3> &outTangents,
                                            std::vector<SVector3> &outBinormals)
{
    // we need this information
    if (inVertexArrayInfo.m_POffset == -1 || inNormalArrayInfo.m_POffset == -1
        || inTexCoordArrayInfo.m_POffset == -1) {
        return;
    }

    long triCount = (long)inTrianglesRef->getCount();
    if (triCount == 0)
        return;

    // allocate arrays
    long vertexCount = (long)inVertexArrayInfo.m_Array->getCount() / 3;
    SVector3 *tan1 = new SVector3[vertexCount * 2];
    SVector3 *tan2 = tan1 + vertexCount;
    SVector3 *normals = new SVector3[vertexCount];

    // failed to allocate menory
    if (!tan1 || !normals)
        return;

    memset(tan1, 0x00, vertexCount * sizeof(SVector3) * 2);

    const domPRef thePRef = inTrianglesRef->getP();
    const domListOfUInts &theListOfPInts = thePRef->getValue();

    long maxOffset = (inNormalArrayInfo.m_POffset > inVertexArrayInfo.m_POffset)
        ? inNormalArrayInfo.m_POffset
        : inVertexArrayInfo.m_POffset;
    maxOffset =
        (inTexCoordArrayInfo.m_POffset > maxOffset) ? inTexCoordArrayInfo.m_POffset : maxOffset;
    maxOffset =
        (inTexCoordArray2Info.m_POffset > maxOffset) ? inTexCoordArray2Info.m_POffset : maxOffset;
    maxOffset++;

    SVector3 vx1, vx2, vx3;
    SVector3 uv1, uv2, uv3;

    for (long i = 0; i < triCount; i++) {
        // read an entire triangle
        // read vertex 1 data
        long indexCount = i * maxOffset * 3; // we have a vertex, normal and texcoord entry;
        long index1 = (unsigned long)theListOfPInts[indexCount + inVertexArrayInfo.m_POffset];
        GetFaceTupleValue((float *)vx1, inVertexArrayInfo,
                          (unsigned long)theListOfPInts[indexCount + inVertexArrayInfo.m_POffset]);
        GetFaceTupleValue((float *)normals[index1], inNormalArrayInfo,
                          (unsigned long)theListOfPInts[indexCount + inNormalArrayInfo.m_POffset]);
        GetFaceTupleValue(
            (float *)uv1, inTexCoordArrayInfo,
            (unsigned long)theListOfPInts[indexCount + inTexCoordArrayInfo.m_POffset]);

        // read vertex 2 data
        indexCount += maxOffset; // we have a vertex, normal and texcoord entry;
        long index2 = (unsigned long)theListOfPInts[indexCount + inVertexArrayInfo.m_POffset];
        GetFaceTupleValue((float *)vx2, inVertexArrayInfo,
                          (unsigned long)theListOfPInts[indexCount + inVertexArrayInfo.m_POffset]);
        GetFaceTupleValue((float *)normals[index2], inNormalArrayInfo,
                          (unsigned long)theListOfPInts[indexCount + inNormalArrayInfo.m_POffset]);
        GetFaceTupleValue(
            (float *)uv2, inTexCoordArrayInfo,
            (unsigned long)theListOfPInts[indexCount + inTexCoordArrayInfo.m_POffset]);

        // read vertex 3 data
        indexCount += maxOffset; // we have a vertex, normal and texcoord entry;
        long index3 = (unsigned long)theListOfPInts[indexCount + inVertexArrayInfo.m_POffset];
        GetFaceTupleValue((float *)vx3, inVertexArrayInfo,
                          (unsigned long)theListOfPInts[indexCount + inVertexArrayInfo.m_POffset]);
        GetFaceTupleValue((float *)normals[index3], inNormalArrayInfo,
                          (unsigned long)theListOfPInts[indexCount + inNormalArrayInfo.m_POffset]);
        GetFaceTupleValue(
            (float *)uv3, inTexCoordArrayInfo,
            (unsigned long)theListOfPInts[indexCount + inTexCoordArrayInfo.m_POffset]);

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

        tan1[index1][0] += sx;
        tan1[index1][1] += sy;
        tan1[index1][2] += sz;
        tan1[index2][0] += sx;
        tan1[index2][1] += sy;
        tan1[index2][2] += sz;
        tan1[index3][0] += sx;
        tan1[index3][1] += sy;
        tan1[index3][2] += sz;

        tan2[index1][0] += tx;
        tan2[index1][1] += ty;
        tan2[index1][2] += tz;
        tan2[index2][0] += tx;
        tan2[index2][1] += ty;
        tan2[index2][2] += tz;
        tan2[index3][0] += tx;
        tan2[index3][1] += ty;
        tan2[index3][2] += tz;
    }

    outTangents.resize(vertexCount);
    outBinormals.resize(vertexCount);

    // compute actual tangents
    for (int i = 0; i < vertexCount; i++) {
        const QT3DSVec3 n(normals[i][0], normals[i][1], normals[i][2]);
        const qt3ds::QT3DSVec3 t(tan1[i][0], tan1[i][1], tan1[i][2]);
        const qt3ds::QT3DSVec3 t2(tan2[i][0], tan2[i][1], tan2[i][2]);

        // Gram-Schmidt orthogonalize
        qt3ds::QT3DSVec3 temp, t3, bi;
        temp = t - n * n.dot(t);
        t3 = temp;
        t3.normalize();
        outTangents[i][0] = t3.x;
        outTangents[i][1] = t3.y;
        outTangents[i][2] = t3.z;

        // Calculate binormal
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
}

//==============================================================================
/**
 *	Processes the <bind_material> element.
 */
void ColladaDOMWalker::ProcessBindMaterial(const domBind_materialRef inBindMaterialRef,
                                           const TFaceIndicies &inFaceIndicies)
{
    const domBind_material::domTechnique_commonRef theTechiniqueCommon =
        inBindMaterialRef->getTechnique_common();
    const domInstance_material_Array &theInstanceMaterials =
        theTechiniqueCommon->getInstance_material_array();
    long theInstanceMaterialCount = (long)theInstanceMaterials.getCount();

    // Match stored material subsets with the list from <bind_material>
    long theNumberOfMaterials = (long)inFaceIndicies.second.size();
    long theFaceIndexStart = 0;

    for (long theMaterialIndex = 0; theMaterialIndex < theNumberOfMaterials; ++theMaterialIndex) {
        for (long theInstanceMaterialIndex = 0; theInstanceMaterialIndex < theInstanceMaterialCount;
             ++theInstanceMaterialIndex) {
            const domInstance_materialRef theInstanceMaterialRef =
                theInstanceMaterials[theInstanceMaterialIndex];
            if (inFaceIndicies.second[theMaterialIndex].first
                == theInstanceMaterialRef->getSymbol()) {
                std::string theName;
                daeElementRef theMaterialElementRef =
                    theInstanceMaterialRef->getTarget().getElement();

                if (theMaterialElementRef)
                    theName = GetNameOrIDOrSid(theMaterialElementRef);

                PushMaterial(theName.empty() ? theInstanceMaterialRef->getSymbol()
                                             : theName.c_str());
                ProcessInstanceMaterial(theInstanceMaterialRef);
                PopMaterial(theFaceIndexStart,
                            (long)(inFaceIndicies.second[theMaterialIndex].second.size() / 3));
            }
        }

        theFaceIndexStart += (long)(inFaceIndicies.second[theMaterialIndex].second.size() / 3);
    }
}

//==============================================================================
/**
 *	Processes default materials found in <triangles>.
 */
void ColladaDOMWalker::ProcessDefaultMaterial(const TFaceIndicies &inFaceIndicies)
{
    // Collada specs indicates that when <bind_material> is not defined, the shading becomes
    // application defined.
    // But we can try to locate the material within <library_materials> and use that instead if
    // available.
    TFaceIndicies::second_type::const_iterator theIter = inFaceIndicies.second.begin();
    TFaceIndicies::second_type::const_iterator theEnd = inFaceIndicies.second.end();
    long theFaceIndexStart = 0;
    for (; theIter != theEnd; ++theIter) {
        domMaterial *theMaterial =
            daeSafeCast<domMaterial>(FindElementByID(theIter->first.c_str()));

        PushMaterial(theMaterial ? GetNameOrIDOrSid(theMaterial) : "Default");
        if (theMaterial)
            ProcessMaterial(theMaterial);

        PopMaterial(theFaceIndexStart, (long)(theIter->second.size() / 3));
        theFaceIndexStart += (long)(theIter->second.size());
    }
}

//==============================================================================
/**
 *	Processes the <instance_material> element.
 */
void ColladaDOMWalker::ProcessInstanceMaterial(const domInstance_materialRef inInstanceMaterialRef)
{
    // Process material
    const xsAnyURI &theMaterialURI = inInstanceMaterialRef->getTarget();
    const daeElementRef theMaterialRef = theMaterialURI.getElement();
    const domMaterial *theMaterial = daeSafeCast<domMaterial>(theMaterialRef);
    if (theMaterial)
        ProcessMaterial(theMaterial);
}

//==============================================================================
/**
 *	Processes the <material> element referenced by <instance_material>.
 */
void ColladaDOMWalker::ProcessMaterial(const domMaterial *inMaterial)
{
    const domInstance_effectRef theInstanceEffectRef = inMaterial->getInstance_effect();
    ProcessInstanceEffect(theInstanceEffectRef);
}

//==============================================================================
/**
 *	Processes the <instance_effect> element referenced by <material>.
 */
void ColladaDOMWalker::ProcessInstanceEffect(const domInstance_effectRef inInstanceEffectRef)
{
    const xsAnyURI &theEffectURI = inInstanceEffectRef->getUrl();
    const daeElementRef theEffectRef = theEffectURI.getElement();
    const domEffect *theEffect = daeSafeCast<domEffect>(theEffectRef);
    ProcessEffect(theEffect);
}

//==============================================================================
/**
 *	Processes the <effect> element referenced by <instance_effect>.
 */
void ColladaDOMWalker::ProcessEffect(const domEffect *inEffect)
{
    const domFx_profile_abstract_Array &theProfiles = inEffect->getFx_profile_abstract_array();
    long theProfilesCount = (long)theProfiles.getCount();
    for (long theIndex = 0; theIndex < theProfilesCount; ++theIndex) {
        ProcessProfile(theProfiles[theIndex]);
    }
}

//==============================================================================
/**
 *	Processes the profile element referenced by <effect>.
 */
void ColladaDOMWalker::ProcessProfile(const domFx_profile_abstractRef &inProfile)
{
    // Support profile_Common for now
    switch (inProfile->getElementType()) {
    case COLLADA_TYPE::PROFILE_COMMON: {
        const domProfile_COMMON *theProfileCommon =
            daeSmartRef<domProfile_COMMON>::staticCast(inProfile);
        ProcessProfileCommon(theProfileCommon);
    } break;

    default:
        break;
    }
}

//==============================================================================
/**
 *	Processes the <profile_common> element.
 */
void ColladaDOMWalker::ProcessProfileCommon(const domProfile_COMMON *inProfileCommon)
{
    const domProfile_COMMON::domTechniqueRef theTechinqueRef = inProfileCommon->getTechnique();
    ProcessProfileCommonTechnique(theTechinqueRef);
}

//==============================================================================
/**
 * Process texture parameters which is stored under <extra> tag
 * This is used to extract Maya texture placement MAYA extension
 */
void ColladaDOMWalker::ProcessTextureParameters(const domExtraRef inTextureExtraRef,
                                                STextureParameters &outTextureParameters)
{
    if (inTextureExtraRef) {
        const domTechnique_Array &theTechniqueArray = inTextureExtraRef->getTechnique_array();
        long theNoOfTechnique = (long)theTechniqueArray.getCount();
        for (long i = 0; i < theNoOfTechnique; ++i) {
            domTechniqueRef theTechnique = theTechniqueArray[i];
            const char *theProfile = theTechnique->getProfile();
            if (::strcmp(theProfile, "MAYA") == 0) {
                // Extract Maya information
                outTextureParameters.m_Flag = true;
                if (daeElement *theElement = theTechnique->getChild("coverageU")) {
                    outTextureParameters.m_coverageU.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("coverageV")) {
                    outTextureParameters.m_coverageV.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("repeatU")) {
                    outTextureParameters.m_repeatU.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("repeatV")) {
                    outTextureParameters.m_repeatV.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("offsetU")) {
                    outTextureParameters.m_offsetU.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("offsetV")) {
                    outTextureParameters.m_offsetV.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("rotateUV")) {
                    outTextureParameters.m_rotateUV.SetValue(GetFloatFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("wrapU")) {
                    outTextureParameters.m_wrapU.SetValue(GetIntFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("wrapV")) {
                    outTextureParameters.m_wrapV.SetValue(GetIntFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("mirrorU")) {
                    outTextureParameters.m_mirrorU.SetValue(GetIntFromElementChar(theElement));
                }
                if (daeElement *theElement = theTechnique->getChild("mirrorV")) {
                    outTextureParameters.m_mirrorV.SetValue(GetIntFromElementChar(theElement));
                }
            }
        }
    }
}

//==============================================================================
/**
 * Process texture references.
 */
void ColladaDOMWalker::ProcessTexture(
    const domCommon_color_or_texture_type_complexType::domTextureRef inTextureRef, long inMapType)
{
    if (inTextureRef != NULL) {
        // TODO: What about texcoord?
        const xsNCName theTextureParamSid = inTextureRef->getTexture();
        ColladaDOMWalker::TURIList theTextures;
        STextureParameters theTextureParameters;
        ProcessTextureParameters(inTextureRef->getExtra(), theTextureParameters);

        daeElement *theNewParamPtr = NULL;

        // Look for param name in <newparam>s
        if (RecursiveFindElementBySid(theTextureParamSid, theNewParamPtr, inTextureRef,
                                      domProfile_COMMON::domTechnique::ID(), domEffect::ID())) {
            FindTexturesViaNewParam(theNewParamPtr, theTextures);

            TURIList::const_iterator theIter = theTextures.begin();
            TURIList::const_iterator theEnd = theTextures.end();
            for (; theIter != theEnd; ++theIter) {
                PushTexture(theIter->first.c_str(), theIter->second.path().c_str(), inMapType);
                SetTexture(inMapType, theTextureParameters);
                PopTexture();
            }
        }
    }
}

//==============================================================================
/**
 * Process color or texture references.
 */
void ColladaDOMWalker::GetColorOrTextureParamInfo(
    const domCommon_color_or_texture_type_complexType *inColorOrTexturePtr,
    SColorOrTexture &outColor, long inMapType)
{
    if (inColorOrTexturePtr) {
        if (const domCommon_color_or_texture_type_complexType::domColorRef theColorRef =
                inColorOrTexturePtr->getColor()) {
            domFloat4 theDomFloat4 = theColorRef->getValue();
            outColor.SetColor((float)theDomFloat4[0], (float)theDomFloat4[1],
                              (float)theDomFloat4[2], (float)theDomFloat4[3]);
            ApplyAnimation(theColorRef);
        } else if (const domCommon_color_or_texture_type_complexType::domTextureRef theTextureRef =
                       inColorOrTexturePtr->getTexture()) {
            ProcessTexture(theTextureRef, inMapType);
            outColor.SetTexture();
        }
    }
}

//==============================================================================
/**
 * Process <technique_common> element.
 */
void ColladaDOMWalker::ProcessProfileCommonTechnique(
    const domProfile_COMMON::domTechniqueRef inTechniqueRef)
{
    static const float theZeroColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Blinn COMMON
    BeginCommonTechniqueObject(inTechniqueRef, Blinn)
        ProcessColorOrTextureParam(Emission, ETextureMapTypeEmissive);
    ProcessColorOrTextureParam(Ambient, ETextureMapTypeDiffuse);
    ProcessColorOrTextureParam(Diffuse, ETextureMapTypeDiffuse);
    ProcessColorOrTextureParam(Specular, ETextureMapTypeSpecular);
    ProcessFloatParam(Shininess);
    ProcessColorOrTextureParam(Reflective, ETextureMapTypeDiffuse);
    ProcessFloatParam(Reflectivity);
    ProcessTransparentParam(Transparent);
    ProcessFloatParam(Transparency);
    ProcessFloatParam(Index_of_refraction);
    EndCommonTechniqueObject()

        // Constant COMMON
        BeginCommonTechniqueObject(inTechniqueRef, Constant)
            ProcessColorOrTextureParam(Emission, ETextureMapTypeEmissive);
    ProcessColorOrTextureParam(Reflective, ETextureMapTypeDiffuse);
    ProcessFloatParam(Reflectivity);
    ProcessTransparentParam(Transparent);
    ProcessFloatParam(Transparency);
    ProcessFloatParam(Index_of_refraction);
    EndCommonTechniqueObject()

        // Lambert COMMON
        BeginCommonTechniqueObject(inTechniqueRef, Lambert)
            ProcessColorOrTextureParam(Emission, ETextureMapTypeEmissive);
    ProcessColorOrTextureParam(Ambient, ETextureMapTypeDiffuse);
    ProcessColorOrTextureParam(Diffuse, ETextureMapTypeDiffuse);
    ProcessColorOrTextureParam(Reflective, ETextureMapTypeDiffuse);
    ProcessFloatParam(Reflectivity);
    ProcessTransparentParam(Transparent);
    ProcessFloatParam(Transparency);
    ProcessFloatParam(Index_of_refraction);
    EndCommonTechniqueObject()

        // Phong COMMON
        BeginCommonTechniqueObject(inTechniqueRef, Phong)
            ProcessColorOrTextureParam(Emission, ETextureMapTypeEmissive);
    ProcessColorOrTextureParam(Ambient, ETextureMapTypeDiffuse);
    ProcessColorOrTextureParam(Diffuse, ETextureMapTypeDiffuse);
    ProcessColorOrTextureParam(Specular, ETextureMapTypeSpecular);
    ProcessFloatParam(Shininess);
    ProcessColorOrTextureParam(Reflective, ETextureMapTypeDiffuse);
    ProcessFloatParam(Reflectivity);
    ProcessTransparentParam(Transparent);
    ProcessFloatParam(Transparency);
    ProcessFloatParam(Index_of_refraction);
    EndCommonTechniqueObject()
}

//==============================================================================
/**
 * Process <technique_common> <extra> element.
 */
void ColladaDOMWalker::ProcessProfileCommonTechniqueExtra(
    const domProfile_COMMON::domTechniqueRef inTechniqueRef,
    SMaterialExtraParameters &outMaterialExtraParameters)
{
    const domExtra_Array &theExtras = inTechniqueRef->getExtra_array();
    long theNoOfExtras = (long)theExtras.getCount();

    for (long theExtraIndex = 0; theExtraIndex < theNoOfExtras; ++theExtraIndex) {
        const domTechnique_Array &theTechniqueArray =
            theExtras[theExtraIndex]->getTechnique_array();
        long theNoOfTechnique = (long)theTechniqueArray.getCount();
        for (long i = 0; i < theNoOfTechnique; ++i) {
            domTechniqueRef theTechnique = theTechniqueArray[i];
            const char *theProfile = theTechnique->getProfile();
            if (::strcmp(theProfile, "FCOLLADA") == 0) {
                // Extract FCollada information
                outMaterialExtraParameters.m_Flag = true;
                if (daeElement *theElement = theTechnique->getChild("spec_level")) {
                    if (daeElement *theElementValue = theElement->getChild("float")) {
                        outMaterialExtraParameters.m_SpecLevel.SetValue(
                            GetFloatFromElementChar(theElementValue));
                    }
                }
                if (daeElement *theElement = theTechnique->getChild("emission_level")) {
                    if (daeElement *theElementValue = theElement->getChild("float")) {
                        outMaterialExtraParameters.m_EmissionLevel.SetValue(
                            GetFloatFromElementChar(theElementValue));
                    }
                }
                break;
            }
        }
    }
}

//==============================================================================
/**
 *	 Processes any <library_animations> within the Collada file.
 */
void ColladaDOMWalker::ProcessLibraryAnimations()
{
    const domLibrary_animations_Array &theAnimationsArray =
        m_ColladaRoot->getLibrary_animations_array();

    long theLibrariesCount = (long)theAnimationsArray.getCount();

    for (long theIndex = 0; theIndex < theLibrariesCount; ++theIndex) {
        ProcessAnimations(theAnimationsArray[theIndex]);
    }
}

//==============================================================================
/**
 *	 Process animations within a <library_animations>.
 */
void ColladaDOMWalker::ProcessAnimations(const domLibrary_animationsRef inLibraryAnimation)
{
    const domAnimation_Array &theAnimationsArray = inLibraryAnimation->getAnimation_array();
    long theAnimationsCount = (long)theAnimationsArray.getCount();

    for (long theIndex = 0; theIndex < theAnimationsCount; ++theIndex) {
        ProcessAnimation(theAnimationsArray[theIndex]);
    }
}

//==============================================================================
/**
 *	 Processes an <animation< element.
 */
void ColladaDOMWalker::ProcessAnimation(const domAnimationRef inAnimation)
{
    const domChannel_Array &theChannels = inAnimation->getChannel_array();

    long theChannelCount = (long)theChannels.getCount();

    for (long theIndex = 0; theIndex < theChannelCount; ++theIndex) {
        ProcessChannel(theChannels[theIndex]);
    }
}

//==============================================================================
/**
 *	 Processes <channel> elements within <animation>
 */
void ColladaDOMWalker::ProcessChannel(const domChannelRef inChannel)
{
    TStringList theIdentifierList;
    daeElement *theContainerElement = NULL;

    const char *theBaseProperty = GetAnimatedPropertyInfoFromElement(
        inChannel->getTarget(), m_ColladaRoot, theContainerElement, theIdentifierList);

    if (theBaseProperty && !theIdentifierList.empty()) {
        const domURIFragmentType &theSamplerSource = inChannel->getSource();
        const daeElementRef theSamplerRef = theSamplerSource.getElement();
        const domSampler *theDomSampler = daeSafeCast<domSampler>(theSamplerRef);

        ProcessSampler(theDomSampler, theContainerElement, theBaseProperty, theIdentifierList);
    }
}

//==============================================================================
/**
 *	 Processes <sampler> elements refered from <channel>.
 *	 Creates animation tracks and maps them to their target elements.
 *	 This will then be used to determine which elements are animated when processing
 *	 the scene.
 */
void ColladaDOMWalker::ProcessSampler(const domSampler *inSamplerRef,
                                      daeElement *inContainerElement, const char *inBaseProperty,
                                      const TStringList &inIdentifiers)
{
    const domInputLocal_Array &theInputArray = inSamplerRef->getInput_array();

    SSourceArrayInfo theINPUTInfo;
    SSourceArrayInfo theOUTPUTInfo;
    SSourceArrayInfo theIN_TANGENTInfo;
    SSourceArrayInfo theOUT_TANGENTInfo;

    // TODO: We should check the interpolation type
    // SSourceArrayInfo theINTERPOLATIONInfo;

    GetDomSourceArrayInfo(theInputArray, "INPUT", g_TIMEIdentifiers, theINPUTInfo);
    GetDomSourceArrayInfo(theInputArray, "OUTPUT", inIdentifiers, theOUTPUTInfo);
    // GetDomSourceArrayInfo( theInputArray, "INTERPOLATION", inIdentifiers, theINTERPOLATIONInfo );

    // The number of XY animation tangents is equal to the number of animated floats
    TStringList theAnimTangentsIdentifiers;
    for (long theIndex = 0; theIndex < theOUTPUTInfo.m_Stride; ++theIndex)
        theAnimTangentsIdentifiers.insert(theAnimTangentsIdentifiers.begin(),
                                          g_XYIdentifiers.begin(), g_XYIdentifiers.end());

    GetDomSourceArrayInfo(theInputArray, "IN_TANGENT", theAnimTangentsIdentifiers,
                          theIN_TANGENTInfo);
    GetDomSourceArrayInfo(theInputArray, "OUT_TANGENT", theAnimTangentsIdentifiers,
                          theOUT_TANGENTInfo);

    // Process the sampler and convert them to tracks
    for (long theAnimatedSubPropertyIndex = 0;
         theAnimatedSubPropertyIndex < (long)theOUTPUTInfo.m_Offset.size();
         ++theAnimatedSubPropertyIndex) {
        // For each animated sub property, create a track
        CacheAnimationTrack(inContainerElement);

        // Set the base property and sub property to animate
        SetAnimationTrack(inBaseProperty, inIdentifiers[theAnimatedSubPropertyIndex].c_str());

        for (long theKeyframeIndex = 0; theKeyframeIndex < theINPUTInfo.m_Array->getCount();
             ++theKeyframeIndex) {
            // Create a key
            SKeyframeParameters theKeyframeInfo(
                (float)theINPUTInfo.m_Array->getValue()[theKeyframeIndex],
                (float)theOUTPUTInfo.m_Array
                    ->getValue()[theKeyframeIndex * theOUTPUTInfo.m_Stride
                                 + theOUTPUTInfo.m_Offset[theAnimatedSubPropertyIndex]],
                theIN_TANGENTInfo.m_Array != NULL
                    ? (float)theIN_TANGENTInfo.m_Array
                          ->getValue()[theKeyframeIndex * theIN_TANGENTInfo.m_Stride
                                       + theAnimatedSubPropertyIndex * 2]
                    : 0.0f,
                theIN_TANGENTInfo.m_Array != NULL
                    ? (float)theIN_TANGENTInfo.m_Array
                          ->getValue()[theKeyframeIndex * theIN_TANGENTInfo.m_Stride
                                       + theAnimatedSubPropertyIndex * 2 + 1]
                    : 0.0f,
                theOUT_TANGENTInfo.m_Array != NULL
                    ? (float)theOUT_TANGENTInfo.m_Array
                          ->getValue()[theKeyframeIndex * theOUT_TANGENTInfo.m_Stride
                                       + theAnimatedSubPropertyIndex * 2]
                    : 0.0f,
                theOUT_TANGENTInfo.m_Array != NULL
                    ? (float)theOUT_TANGENTInfo.m_Array
                          ->getValue()[theKeyframeIndex * theOUT_TANGENTInfo.m_Stride
                                       + theAnimatedSubPropertyIndex * 2 + 1]
                    : 0.0f);

            // Set information about the key
            CacheAnimationKey(inBaseProperty, inIdentifiers[theAnimatedSubPropertyIndex].c_str(),
                              theKeyframeInfo);
        }
    }
}

//==============================================================================
/**
 * Associates inElement with an index returned from the ColladaTranslation class.
 */
void ColladaDOMWalker::TrackObjectIndex(const daeElement *inElement, long inIndex)
{
    if (inElement)
        m_ElementToIndicies.insert(std::make_pair(inElement, inIndex));
}

//==============================================================================
/**
 *	Retrieved the stored index associated with inElement
 */
void ColladaDOMWalker::GetIndicesFromElement(const daeElement *inElement, TLongsList &outIndicies)
{
    auto theResult = m_ElementToIndicies.equal_range(inElement);

    outIndicies.clear();

    for (TElementToIndicesMap::const_iterator theIter = theResult.first;
         theIter != theResult.second; ++theIter) {
        outIndicies.push_back(theIter->second);
    }
}

//==============================================================================
/**
 *	Checks an element to determine if it's been animated.
 *	If it is, the relavent animation tracks will be retrieved from storage and
 *	attached to the target object.
 */
bool ColladaDOMWalker::ApplyAnimation(const daeElement *inContainerElement)
{
    TLongsList theAnimationTrackIndicies;
    GetIndicesFromElement(inContainerElement, theAnimationTrackIndicies);
    bool theAnimated = false;

    for (TLongsList::const_iterator theIter = theAnimationTrackIndicies.begin();
         theIter != theAnimationTrackIndicies.end(); ++theIter) {
        theAnimated = true;
        ApplyAnimationTrack(*theIter);
    }

    return theAnimated;
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

bool CImportTranslation::ParseColladaFile(const std::string &fileName, Import &import,
                                          ISGTranslationLog &log)
{
    ISceneGraphTranslation &transHelper = ISceneGraphTranslation::CreateTranslation(import, log);
    ScopedHelper __scope(transHelper);
    ColladaDOMWalker theDOMWalker(&transHelper);
    if (theDOMWalker.LoadDocument(fileName)) {
        theDOMWalker.ProcessLibraryAnimations();
        theDOMWalker.ProcessScene();
        return true;
    }
    return false;
}
