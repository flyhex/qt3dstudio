/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "qtAuthoring-config.h"
#include "IDocumentEditor.h"
#include "Doc.h"
#include "Qt3DSFileTools.h"
#include "StudioFullSystem.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "StudioCoreSystem.h"
#include "StudioFullSystem.h"
#include "CmdDataModel.h"
#include "Qt3DSDMStudioSystem.h"
#include "SlideSystem.h"
#include "Qt3DSDMAnimation.h"
#include "ClientDataModelBridge.h"
#include "Cmd.h"
#include "Core.h"
#include "Dispatch.h"
#include "Qt3DSImportPerformImport.h"
#include "Qt3DSImportTranslation.h"
#include "Qt3DSImport.h"
#include "Qt3DSFileTools.h"
#include "StudioFullSystem.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "StudioCoreSystem.h"
#include "IDocumentBufferCache.h"
#include "Qt3DSImportMesh.h"
#include "Qt3DSDMSlideGraphCore.h"
#include "IComposerEditorInterface.h"
#include "Qt3DSDMXML.h"
#include "foundation/IOStreams.h"
#include "IComposerSerializer.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "Qt3DSDMMetaData.h"
#include "DocumentResourceManagerScriptParser.h"
#include "DocumentResourceManagerRenderPluginParser.h"
#include "DocumentResourceManagerCustomMaterialParser.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "IDirectoryWatchingSystem.h"
#include "Qt3DSDMActionCore.h"
#include "PresentationFile.h"
#include "ActionSystem.h"
#include "StandardExtensions.h"
#include "Qt3DSRenderMesh.h"
#include "Qt3DSRenderImage.h"
#include "IDocSceneGraph.h"
#include "Qt3DSTextRenderer.h"
#include "foundation/Qt3DSFoundation.h"
#include "Q3DStudioNVFoundation.h"
#include "Qt3DSDMGuides.h"
#include "Qt3DSRenderPathManager.h"
#include "Qt3DSImportPath.h"
#include "Dialogs.h"
#include "foundation/Qt3DSLogging.h"
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdir.h>
#include <unordered_set>
#include "runtime/q3dsqmlbehavior.h"
#include "runtime/q3dsmaterialdefinitionparser.h"
#include "Qt3DSFileToolsSeekableMeshBufIOStream.h"
#include "IObjectReferenceHelper.h"
#include "StudioProjectSettings.h"
#include "StudioApp.h"
#include "StudioUtils.h"
#include "Qt3DSDMHandles.h"
#include "IStudioRenderer.h"

namespace {

using namespace Q3DStudio;
using namespace qt3dsdm;
using namespace qt3dsimp;
using namespace Q3DStudio::ComposerImport;
using namespace qt3ds;
using namespace qt3ds::foundation;
using std::unordered_map;

inline SFloat2 ToDataModel(QT3DSVec2 inData)
{
    return SFloat2(inData.x, inData.y);
}

inline QT3DSVec2 ToFnd(SFloat2 value)
{
    return QT3DSVec2(value.m_Floats[0], value.m_Floats[1]);
}

struct ScopedBoolean
{
    bool &m_Value;
    ScopedBoolean(bool &val)
        : m_Value(val)
    {
        m_Value = !m_Value;
    }
    ~ScopedBoolean() { m_Value = !m_Value; }
};

typedef qt3ds::foundation::NVScopedRefCounted<qt3ds::render::IInputStreamFactory> TStreamFactoryPtr;

struct SImportXmlErrorHandler : public CXmlErrorHandler
{
    std::shared_ptr<IImportFailedHandler> m_handler;
    QString m_fullPathToDocument;
    SImportXmlErrorHandler(std::shared_ptr<IImportFailedHandler> hdl,
                           const QString &inFullPathToDocument)
        : m_handler(hdl)
        , m_fullPathToDocument(inFullPathToDocument)
    {
    }
    void OnXmlError(const QString &errorName, int line, int) override
    {
        if (m_handler) {
            const QString str = QObject::tr("Failed to parse XML data.\nLine %1: %2\n")
                    .arg(line).arg(errorName);
            m_handler->DisplayImportFailed(m_fullPathToDocument, str, false);
        }
    }
};

class CDocEditor : public Q3DStudio::IInternalDocumentEditor
{
    CDoc &m_Doc;
    Q3DStudio::CGraph &m_AssetGraph;
    CStudioSystem &m_StudioSystem;
    IDataCore &m_DataCore;
    ISlideSystem &m_SlideSystem;
    ISlideCore &m_SlideCore;
    ISlideGraphCore &m_SlideGraphCore;
    IAnimationCore &m_animCore;
    CClientDataModelBridge &m_Bridge;
    IPropertySystem &m_PropertySystem;
    IMetaData &m_MetaData;
    IActionSystem &m_ActionSystem;
    IActionCore &m_ActionCore;
    IStudioAnimationSystem &m_AnimationSystem;
    IGuideSystem &m_GuideSystem;
    // Items should be added to every slide the parent object exists in.
    std::shared_ptr<ISignalConnection> m_ProjectDirWatcher;
    bool m_IgnoreDirChange;
    TCharPtrToSlideInstanceMap m_SourcePathInstanceMap;
    unordered_map<TCharPtr, TCharPtr> m_ImportFileToDAEMap;
    qt3dsdm::IStringTable &m_StringTable;
    Q3DStudio::Foundation::SStudioFoundation m_Foundation;
    TStreamFactoryPtr m_InputStreamFactory;
    std::unordered_map<long, QT3DSU32> m_GraphOrderMap;

public:
    CDocEditor(CDoc &inDoc)
        : m_Doc(inDoc)
        , m_AssetGraph(*m_Doc.GetAssetGraph())
        , m_StudioSystem(*m_Doc.GetStudioSystem())
        , m_DataCore(*m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetDataCore())
        , m_SlideSystem(*m_StudioSystem.GetFullSystem()->GetSlideSystem())
        , m_SlideCore(*m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetSlideCore())
        , m_SlideGraphCore(*m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetSlideGraphCore())
        , m_animCore(*m_StudioSystem.GetFullSystem()->GetAnimationCore())
        , m_Bridge(*m_StudioSystem.GetClientDataModelBridge())
        , m_PropertySystem(*m_StudioSystem.GetPropertySystem())
        , m_MetaData(*m_StudioSystem.GetActionMetaData())
        , m_ActionSystem(*m_StudioSystem.GetActionSystem())
        , m_ActionCore(*m_StudioSystem.GetFullSystem()->GetActionCore())
        , m_AnimationSystem(*m_StudioSystem.GetAnimationSystem())
        , m_GuideSystem(*m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetGuideSystem())
        , m_IgnoreDirChange(false)
        , m_StringTable(m_DataCore.GetStringTable())
        , m_Foundation(Q3DStudio::Foundation::SStudioFoundation::Create())
        , m_InputStreamFactory(qt3ds::render::IInputStreamFactory::Create(*m_Foundation.m_Foundation))
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        IDirectoryWatchingSystem *theSystem(m_Doc.GetDirectoryWatchingSystem());
        if (theSystem) {
            m_ProjectDirWatcher = theSystem->AddDirectory(m_Doc.GetCore()->getProjectFile()
                                                          .getProjectPath(),
                                        std::bind(&CDocEditor::OnProjectDirChanged, this,
                                                  std::placeholders::_1));
        }

        qmlRegisterType<Q3DSQmlBehavior>("QtStudio3D.Behavior", 1, 0, "Behavior");
        qmlRegisterType<Q3DSQmlBehavior, 1>("QtStudio3D.Behavior", 1, 1, "Behavior");
    }
    virtual ~CDocEditor()
    {
    }
    ///////////////////////////////////////////////////////////////////
    // IDocumentReader
    //////////////////////////////////////////////////////////////////

    bool IsInstance(Qt3DSDMInstanceHandle instance) const override
    {
        return m_DataCore.IsInstance(instance);
    }

    bool IsCurrentlyActive(TInstanceHandle inInstance) const override
    {
        SValue startTime, endTime, eyeball;
        IPropertySystem &thePropertySystem(m_PropertySystem);
        ISlideSystem &theSlideSystem(m_SlideSystem);
        if (IsInstance(inInstance)
            && thePropertySystem.GetInstancePropertyValue(
                   inInstance, m_Bridge.GetSceneAsset().m_StartTime, startTime)
            && thePropertySystem.GetInstancePropertyValue(
                   inInstance, m_Bridge.GetSceneAsset().m_EndTime, endTime)
            && thePropertySystem.GetInstancePropertyValue(
                   inInstance, m_Bridge.GetSceneAsset().m_Eyeball, eyeball)) {
            bool eyeballVal = qt3dsdm::get<bool>(eyeball);
            long theStart = qt3dsdm::get<qt3ds::QT3DSI32>(startTime);
            long theEnd = qt3dsdm::get<qt3ds::QT3DSI32>(endTime);
            Qt3DSDMInstanceHandle theInstance(inInstance);
            SInstanceSlideInformation theSlideInfo(
                theSlideSystem.GetInstanceSlideInformation(theInstance));
            Qt3DSDMSlideHandle theAssociatedSlide = theSlideInfo.m_AssociatedSlide;
            Qt3DSDMSlideHandle theMaster = theSlideInfo.m_MasterSlide;
            Qt3DSDMSlideHandle theActiveSlide = theSlideInfo.m_ActiveSlide;
            if (theAssociatedSlide == theMaster || theAssociatedSlide == theActiveSlide) {
                long theViewTime = theSlideInfo.m_ComponentTime;
                return eyeballVal && theStart <= theViewTime && theEnd > 0 && theEnd >= theViewTime;
            }
        }
        return false;
    }

    TPropertyHandle FindProperty(Qt3DSDMInstanceHandle instance,
                                         const wchar_t *inPropName) const override
    {
        return m_DataCore.GetAggregateInstancePropertyByName(instance, inPropName);
    }

    Option<SValue> GetRawInstancePropertyValue(TInstanceHandle instance,
                                                       TPropertyHandle inProperty) const override
    {
        SValue theValue;
        if (m_PropertySystem.GetInstancePropertyValue(instance, inProperty, theValue))
            return theValue.toOldSkool();
        return Empty();
    }

    Option<SValue> GetInstancePropertyValue(TInstanceHandle instance,
                                                    TPropertyHandle inProperty) const override
    {
        AdditionalMetaDataType::Value thePropertyMetaData =
            m_PropertySystem.GetAdditionalMetaDataType(instance, inProperty);
        if (thePropertyMetaData == AdditionalMetaDataType::Image) {
            TInstanceHandle theImageInstance = GetImageInstanceForProperty(instance, inProperty);
            if (theImageInstance)
                return GetRawInstancePropertyValue(theImageInstance,
                                                   m_Bridge.GetSourcePathProperty());
        } else {
            return GetRawInstancePropertyValue(instance, inProperty);
        }
        return Empty();
    }

    TInstanceHandle GetImageInstanceForProperty(TInstanceHandle instance,
                                                        TPropertyHandle inProperty) const override
    {
        qt3dsdm::Qt3DSDMSlideHandle theAssociatedSlide(m_SlideSystem.GetAssociatedSlide(instance));
        SValue theGuid;
        if (m_SlideCore.GetSpecificInstancePropertyValue(theAssociatedSlide, instance, inProperty,
                                                         theGuid)
            || m_DataCore.GetInstancePropertyValue(instance, inProperty, theGuid)) {
            return m_Bridge.GetInstanceByGUID(get<SLong4>(theGuid));
        }
        return TInstanceHandle();
    }

    Option<SValue> GetSpecificInstancePropertyValue(TSlideHandle inSlide,
                                                            TInstanceHandle instance,
                                                            TPropertyHandle inProperty) const override
    {
        SValue theValue;
        SValue theTempValue;
        if (inSlide.Valid()) {
            if (m_SlideCore.GetSpecificInstancePropertyValue(inSlide, instance, inProperty,
                                                             theValue))
                return theValue;
        } else if (m_DataCore.GetInstancePropertyValue(instance, inProperty, theTempValue))
            return theTempValue.toOldSkool();
        return Empty();
    }

    Q3DStudio::CString GetObjectTypeName(TInstanceHandle instance) const override
    {
        if (IsInstance(instance)) {
            Option<TCharStr> theTypeName = m_MetaData.GetTypeForInstance(instance);
            if (theTypeName.hasValue())
                return CString(theTypeName->wide_str());
        }
        return Q3DStudio::CString();
    }

    // Get every property value associated with this instance, from the data core up.  The
    // associated slide will be NULL for the
    // data core.
    void GetAllPropertyValues(TInstanceHandle instance, TPropertyHandle inProperty,
                                      TSlideValuePairList &outValues) const override
    {
        TSlideHandle theSlide(GetAssociatedSlide(instance));
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(instance, inProperty, theValue))
            outValues.push_back(make_pair(0, theValue.toOldSkool()));

        if (theSlide.Valid()) {
            SValue theSlideValue;
            if (m_SlideCore.GetSpecificInstancePropertyValue(theSlide, instance, inProperty,
                                                             theSlideValue))
                outValues.push_back(make_pair(theSlide, theSlideValue));

            TSlideHandleList theChildren;
            m_SlideCore.GetChildSlides(theSlide, theChildren);
            for (size_t idx = 0, end = theChildren.size(); idx < end; ++idx) {
                if (m_SlideCore.GetSpecificInstancePropertyValue(theChildren[idx], instance,
                                                                 inProperty, theSlideValue))
                    outValues.push_back(make_pair(theChildren[idx], theSlideValue));
            }
        }
    }

    TSlideHandle GetAssociatedSlide(TInstanceHandle inInstance) const override
    {
        TSlideHandle retval = m_SlideSystem.GetAssociatedSlide(inInstance);
        return retval;
    }

    bool IsMasterSlide(TSlideHandle inSlide) const override
    {
        return m_SlideSystem.IsMasterSlide(inSlide);
    }

    TInstanceHandle GetAssociatedComponent(TInstanceHandle inInstance) const override
    {
        return GetComponentForSlide(GetAssociatedSlide(inInstance));
    }

    TSlideHandle GetActiveSlide(TInstanceHandle /*inInstance*/) const override
    {
        return m_Doc.GetActiveSlide();
    }

    TSlideHandle GetComponentActiveSlide(TInstanceHandle inInstance) const override
    {
        return m_Bridge.GetComponentActiveSlide(inInstance);
    }

    TInstanceHandle GetComponentForSlide(TSlideHandle inSlide) const override
    {
        return m_Bridge.GetOwningComponentInstance(inSlide);
    }

    void GetAllAssociatedSlides(TInstanceHandle inInstance, TSlideList &outList) const override
    {
        TSlideHandle retval = m_SlideSystem.GetAssociatedSlide(inInstance);
        if (retval.Valid()) {
            m_SlideCore.GetChildSlides(retval, outList);
            outList.insert(outList.begin(), retval);
        }
    }

    void GetAllPaths(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                     TSlideStringList &outPaths) const
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        if (!m_DataCore.IsInstanceOrDerivedFrom(inInstance, theDefinitions.m_Asset.m_Instance)) {
            QT3DS_ASSERT(false);
            return;
        }

        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(inInstance, inProperty, theValue)) {
            TDataStrPtr theStr(get<TDataStrPtr>(theValue));
            if (theStr && theStr->GetLength())
                outPaths.push_back(make_pair(Qt3DSDMSlideHandle(0), CString(theStr->GetData())));
        }

        TSlideHandleList theSlides;
        GetAllAssociatedSlides(inInstance, theSlides);

        SValue theSlideValue;
        for (size_t idx = 0, end = theSlides.size(); idx < end; ++idx) {
            Qt3DSDMSlideHandle theSlide(theSlides[idx]);
            if (m_SlideCore.GetSpecificInstancePropertyValue(theSlide, inInstance, inProperty,
                                                             theSlideValue)) {
                TDataStrPtr theStr(get<TDataStrPtr>(theSlideValue));
                if (theStr && theStr->GetLength())
                    outPaths.push_back(make_pair(theSlide, CString(theStr->GetData())));
            }
        }
    }

    void GetAllSourcePaths(Qt3DSDMInstanceHandle inInstance, TSlideStringList &outPaths) const override
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        GetAllPaths(inInstance, theDefinitions.m_Asset.m_SourcePath, outPaths);
    }

    void GetPathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap,
                              qt3dsdm::Qt3DSDMPropertyHandle property,
                              bool skipMaterialContainers = true,
                              bool includeIdentifiers = true) const
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        TInstanceHandleList existing;
        TSlideStringList thePaths;
        m_DataCore.GetInstancesDerivedFrom(existing, theDefinitions.m_Asset.m_Instance);
        outInstanceMap.clear();
        for (size_t idx = 0, end = existing.size(); idx < end; ++idx) {
            Qt3DSDMInstanceHandle theAsset(existing[idx]);

            if (skipMaterialContainers && m_Bridge.isInsideMaterialContainer(theAsset))
                continue;

            thePaths.clear();
            GetAllPaths(theAsset, property, thePaths);

            for (size_t pathIdx = 0, pathEnd = thePaths.size(); pathIdx < pathEnd; ++pathIdx) {
                const pair<qt3dsdm::Qt3DSDMSlideHandle, Q3DStudio::CString> &theSlideStr(
                    thePaths[pathIdx]);
                CFilePath thePath(theSlideStr.second);
                if (!includeIdentifiers)
                    thePath = thePath.filePath();
                const wchar_t *theString = m_DataCore.GetStringTable().RegisterStr(
                    thePath.toCString());
                pair<TCharPtrToSlideInstanceMap::iterator, bool> theInsertResult(
                    outInstanceMap.insert(make_pair(theString, TSlideInstanceList())));
                insert_unique(theInsertResult.first->second,
                              make_pair(theSlideStr.first, theAsset));
            }
        }
    }

    void GetSourcePathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap,
                                    bool skipMaterialContainers = true,
                                    bool includeIdentifiers = true) const override
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        GetPathToInstanceMap(outInstanceMap, theDefinitions.m_Asset.m_SourcePath,
                             skipMaterialContainers, includeIdentifiers);
    }

    void GetImportPathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap) const override
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        GetPathToInstanceMap(outInstanceMap, theDefinitions.m_Asset.m_ImportFile, true, false);
    }

    bool CanPropertyBeLinked(TInstanceHandle inInstance, TPropertyHandle inProperty) const override
    {
        if (inProperty == m_Bridge.GetAlias().m_ReferencedNode.m_Property)
            return false;
        return m_SlideSystem.CanPropertyBeLinked(inInstance, inProperty);
    }

    // Return true if a property is linked (exists only on the associated slide && the slide is a
    // master slide).
    bool IsPropertyLinked(TInstanceHandle inInstance, TPropertyHandle inProperty) const override
    {
        if (IsInstance(inInstance)) {
            Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
            if (theAssociatedSlide.Valid() && m_SlideSystem.IsMasterSlide(theAssociatedSlide)) {
                if (inProperty.Valid()) {
                    AdditionalMetaDataType::Value thePropertyMetaData =
                        m_PropertySystem.GetAdditionalMetaDataType(inInstance, inProperty);
                    if (thePropertyMetaData == AdditionalMetaDataType::Image) {
                        qt3dsdm::Qt3DSDMInstanceHandle theInstance =
                            GetImageInstanceForProperty(inInstance, inProperty);
                        if (theInstance)
                            return IsPropertyLinked(theInstance, m_Bridge.GetSourcePathProperty());
                        return true; // No image means the property is linked.
                    }
                }
                return m_SlideSystem.IsPropertyLinked(inInstance, inProperty);
            }
        }
        return false;
    }

    TSlideHandle GetSlideForProperty(TInstanceHandle inInstance,
                                             TPropertyHandle inProperty) const override
    {
        TSlideHandle associatedSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
        if (associatedSlide.Valid()) {
            TSlideHandle theMaster = m_SlideCore.GetParentSlide(associatedSlide);
            bool isMaster = true;
            if (theMaster.Valid() == false || associatedSlide != theMaster)
                isMaster = false;
            if (isMaster && m_SlideSystem.IsPropertyLinked(inInstance, inProperty))
                return theMaster;
            return GetActiveSlide(inInstance);
        }
        return 0;
    }

    bool IsImported(TInstanceHandle instance) const override
    {
        SValue theValue;
        if (IsInstance(instance)
            && m_PropertySystem.GetInstancePropertyValue(instance, m_Bridge.GetImportId(),
                                                         theValue)) {
            return get<TDataStrPtr>(theValue)->GetLength() > 0;
        }
        return false;
    }

    CString GetImportId(TInstanceHandle inInstance) const override
    {
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(
                inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_ImportId, theValue)) {
            TDataStrPtr theStr(get<TDataStrPtr>(theValue));
            if (theStr)
                return m_StringTable.RegisterStr(theStr->GetData());
        }
        return m_StringTable.RegisterStr(L"");
    }

    CString GetFileId(TInstanceHandle inInstance) const override
    {
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(
                inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_FileId, theValue)) {
            TDataStrPtr theStr(get<TDataStrPtr>(theValue));
            if (theStr)
                return m_StringTable.RegisterStr(theStr->GetData());
        }
        return m_StringTable.RegisterStr(L"");
    }

    std::pair<long, long> GetTimeRange(TInstanceHandle instance) const override
    {
        SValue theStart, theEnd;
        bool result = m_PropertySystem.GetInstancePropertyValue(
                          instance, m_Bridge.GetSceneAsset().m_StartTime, theStart)
            && m_PropertySystem.GetInstancePropertyValue(
                   instance, m_Bridge.GetSceneAsset().m_EndTime, theEnd);
        if (result) {
            return std::make_pair(static_cast<long>(get<qt3ds::QT3DSI32>(theStart)),
                                  static_cast<long>(get<qt3ds::QT3DSI32>(theEnd)));
        }
        assert(0);
        return std::make_pair(0L, 0L);
    }

    std::pair<long, long> GetTimeRangeInSlide(Qt3DSDMSlideHandle inSlide,
                                              TInstanceHandle instance) const override
    {
        SValue theStart, theEnd;
        bool result = m_SlideCore.GetSpecificInstancePropertyValue(
                          inSlide, instance, m_Bridge.GetSceneAsset().m_StartTime, theStart)
            && m_SlideCore.GetSpecificInstancePropertyValue(
                   inSlide, instance, m_Bridge.GetSceneAsset().m_EndTime, theEnd);
        if (result) {
            return std::make_pair(static_cast<long>(get<qt3ds::QT3DSI32>(theStart)),
                                  static_cast<long>(get<qt3ds::QT3DSI32>(theEnd)));
        }
        assert(0);
        return std::make_pair((long)0, (long)0);
    }

    qt3dsdm::SLong4 GetGuidForInstance(Qt3DSDMInstanceHandle instance) const override
    {
        if (IsInstance(instance)) {
            Q3DStudio::CId theId(m_Bridge.GetGUID(instance));
            TGUIDPacked thePackedGUID(theId);
            return qt3dsdm::SLong4(thePackedGUID.Data1, thePackedGUID.Data2, thePackedGUID.Data3,
                                 thePackedGUID.Data4);
        }
        return qt3dsdm::SLong4();
    }
    TInstanceHandle GetInstanceForGuid(const qt3dsdm::SLong4 &inGuid) const override
    {
        return m_Bridge.GetInstanceByGUID(inGuid);
    }
    TInstanceHandle GetInstanceForObjectRef(TInstanceHandle inRoot,
                                                    const qt3dsdm::SObjectRefType &inReference) const override
    {
        return m_Bridge.GetInstance(inRoot, inReference);
    }
    Qt3DSDMInstanceHandle GetParent(Qt3DSDMInstanceHandle child) const override
    {
        return m_AssetGraph.GetParent(child);
    }

    // Get all the children if this instance in this slide.  If the slide is invalid,
    // the get all the children of this parent in all slides.
    void GetChildren(TSlideHandle inSlide, TInstanceHandle inParent,
                             TInstanceList &outChildren) const override
    {
        for (long theChildIdx = 0, theChildCount = m_AssetGraph.GetChildCount(inParent);
             theChildIdx < theChildCount; ++theChildIdx) {
            TInstanceHandle theChild(m_AssetGraph.GetChild(inParent, theChildIdx));
            if (inSlide.Valid()) {
                if (m_SlideSystem.GetAssociatedSlide(theChild) == inSlide)
                    outChildren.push_back(theChild);
            } else
                outChildren.push_back(theChild);
        }
    }

    bool IsInSceneGraph(TInstanceHandle child) const override { return m_AssetGraph.IsExist(child); }

    // If the path has any sub-path children, then yes it is externalizeable.
    bool IsPathExternalizeable(TInstanceHandle path) const override
    {
        for (QT3DSI32 idx = 0, end = m_AssetGraph.GetChildCount(path); idx < end; ++idx) {
            TInstanceHandle theChild = m_AssetGraph.GetChild(path, idx);
            if (GetObjectTypeName(theChild) == L"SubPath")
                return true;
        }
        return false;
    }

    bool IsPathInternalizeable(TInstanceHandle path) const override
    {
        Option<TDataStrPtr> theStr =
            const_cast<CDocEditor *>(this)->GetTypedInstancePropertyValue<TDataStrPtr>(
                path, m_Bridge.GetSourcePathProperty());
        if (theStr.hasValue() && (*theStr) && (*theStr)->GetLength())
            return true;
        return false;
    }

    bool AnimationExists(TSlideHandle inSlide, TInstanceHandle instance,
                                 const wchar_t *propName, long subIndex) override
    {
        Qt3DSDMPropertyHandle propHdl =
            m_DataCore.GetAggregateInstancePropertyByName(instance, propName);
        if (propHdl.Valid() == false) {
            QT3DS_ASSERT(false);
            return false;
        }
        if (inSlide.Valid() == false) {
            Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(instance);
            if (theSlide.Valid() == false) {
                assert(0);
                return false;
            }
            if (m_SlideSystem.IsPropertyLinked(instance, propHdl))
                theSlide = m_SlideSystem.GetMasterSlide(theSlide);
            inSlide = theSlide;
        }
        return m_animCore.GetAnimation(inSlide, instance, propHdl, subIndex).Valid();
    }

    bool IsAnimationArtistEdited(TSlideHandle inSlide, Qt3DSDMInstanceHandle instance,
                                         const wchar_t *propName, long subIndex) override
    {
        Qt3DSDMPropertyHandle propHdl =
            m_DataCore.GetAggregateInstancePropertyByName(instance, propName);
        if (propHdl.Valid() == false) {
            QT3DS_ASSERT(false);
            return false;
        }
        if (inSlide.Valid() == false) {
            Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(instance);
            if (theSlide.Valid() == false) {
                assert(0);
                return false;
            }
            if (m_SlideSystem.IsPropertyLinked(instance, propHdl))
                theSlide = m_SlideSystem.GetMasterSlide(theSlide);
            inSlide = theSlide;
        }

        Qt3DSDMAnimationHandle animHandle =
            m_animCore.GetAnimation(inSlide, instance, propHdl, subIndex);
        if (animHandle.Valid() == false)
            return false;
        return m_animCore.IsArtistEdited(animHandle);
    }

    pair<std::shared_ptr<qt3dsdm::IDOMWriter>, CFilePath>
    DoCopySceneGraphObject(const TInstanceHandleList &inInstances, bool preserveFileIds)
    {
        if (inInstances.empty())
            return pair<std::shared_ptr<qt3dsdm::IDOMWriter>, CFilePath>();

        std::shared_ptr<IDOMWriter> theWriter(m_Doc.CreateDOMWriter());
        TInstanceHandleList theInstances = ToGraphOrdering(inInstances);
        m_Doc.CreateSerializer()->SerializeSceneGraphObjects(*theWriter, theInstances,
                                                             GetActiveSlide(inInstances[0]),
                                                             preserveFileIds);
        CFilePath theFile = WriteWriterToFile(*theWriter, L"SceneGraph");
        return make_pair(theWriter, theFile);
    }

    // Not exposed through public interface yet
    std::shared_ptr<qt3dsdm::IDOMReader>
    CopySceneGraphObjectsToMemory(const qt3dsdm::TInstanceHandleList &instanceList)
    {
        return DoCopySceneGraphObject(instanceList, false).first->CreateDOMReader();
    }

    // Exposed through document reader interface
    virtual std::shared_ptr<qt3dsdm::IDOMReader>
    CopySceneGraphObjectToMemory(Qt3DSDMInstanceHandle inInstance) override
    {
        TInstanceHandleList instanceList;
        instanceList.push_back(inInstance);
        return CopySceneGraphObjectsToMemory(instanceList);
    }

    struct SFilePtrOutStream : public IOutStream
    {
        TFilePtr m_File;
        SFilePtrOutStream(TFilePtr f)
            : m_File(f)
        {
        }

        bool Write(NVConstDataRef<QT3DSU8> data) override
        {
            return m_File->Write(data.begin(), data.size()) == data.size();
        }
    };

    CFilePath WriteWriterToFile(IDOMWriter &inWriter, const CString &inStem)
    {
        CFilePath theTempFileDir = CFilePath::CombineBaseAndRelative(
            CFilePath::GetUserApplicationDirectory(), CFilePath(L"Qt3DStudio/temp_files"));
        theTempFileDir.CreateDir(true);
        CFilePath theFinalPath;
        {
            TFilePtr theFile = SFileTools::FindUniqueDestFile(theTempFileDir, inStem, L"uip", true);

            theFinalPath = theFile->m_Path;

            Qt3DSFile::AddTempFile(theFile->m_Path);

            SFilePtrOutStream theFileStream(theFile);

            CDOMSerializer::Write(*inWriter.GetTopElement(), theFileStream);
        }
        return theFinalPath;
    }

    CFilePath CopySceneGraphObjects(TInstanceHandleList inInstances, bool preserveFileIds) override
    {
        if (inInstances.empty())
            return L"";
        bool shouldCopy = true;
        for (size_t idx = 0, end = inInstances.size(); idx < end && shouldCopy; ++idx)
            shouldCopy = IsInstance(inInstances[idx]);

        if (!shouldCopy)
            return L"";

        return DoCopySceneGraphObject(inInstances, preserveFileIds).second;
    }

    CFilePath CopyAction(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide) override
    {
        std::shared_ptr<IComposerSerializer> theSerializer(m_Doc.CreateSerializer());
        std::shared_ptr<qt3dsdm::IDOMWriter> theWriter(
            IDOMWriter::CreateDOMWriter(L"UIPActionFragment", m_DataCore.GetStringTablePtr())
                .first);
        theSerializer->SerializeAction(*theWriter, inSlide, inAction);
        return WriteWriterToFile(*theWriter, L"Action");
    }

    std::shared_ptr<qt3dsdm::IDOMReader> CopySlide(Qt3DSDMSlideHandle inSlide) override
    {
        if (m_SlideSystem.IsMasterSlide(inSlide)) {
            QT3DS_ASSERT(false);
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        }
        std::shared_ptr<IComposerSerializer> theSerializer(m_Doc.CreateSerializer());
        std::shared_ptr<qt3dsdm::IDOMWriter> theWriter(
            IDOMWriter::CreateDOMWriter(L"UIPSlideFragment", m_DataCore.GetStringTablePtr()).first);
        theSerializer->SerializeSlide(*theWriter, inSlide);
#ifdef _DEBUG
        WriteWriterToFile(*theWriter, L"Slide");
#endif
        return theWriter->CreateDOMReader();
    }

    qt3ds::NVFoundationBase &GetFoundation() override { return *m_Foundation.m_Foundation; }

    /**
        Parses an effect, material (they use the same syntax) or material definition file
        for dependent assets and returns them in outPathMap parameter.

        @param inFile The file to parse. Can be absolute or relative to current directory.
        @param projectPath The absolute path of the project root the inFile belongs to, if any.
                           Can be left empty for files that are not in any project.
        @param recurseSourceMaterial If true, parsing .materialdef files will recursively also
                                     parse the shader .material file.
        @param outPathMap A map to return the parsed assets.
                          The key is the destination path parsed from the file. It is assumed to be
                          relative to the project root rather than the asset file itself.
                          The value is absolute source path of the asset.
                          The map is only inserted into in this function, never cleared.
                          Assets that are referred by inFile but don't actually exist are not added
                          to this map.
        @param outPropertySet A set to return material/effect texture properties
    */
    void ParseSourcePathsOutOfEffectFile(const QString &inFile,
                                         const QString &projectPath,
                                         bool recurseSourceMaterial,
                                         QHash<QString, QString> &outPathMap,
                                         QSet<QString> &outPropertySet) override
    {
        QDomDocument domDocMat;
        QDir projDir(projectPath);
        QDir fileDir(QFileInfo(inFile).dir());
        if (StudioUtils::readFileToDomDocument(inFile, domDocMat)) {
            QVector<QDomNodeList> nodeLists;
            QVector<bool> isMatDefs;
            // Read properties from custom materials and effects
            nodeLists.append(domDocMat.documentElement()
                    .firstChildElement(QStringLiteral("MetaData")).childNodes());
            isMatDefs.append(false);
            // Read properties from material definitions
            nodeLists.append(domDocMat.firstChildElement(
                                 QStringLiteral("MaterialData")).childNodes());
            isMatDefs.append(true);
            // Read properties from each texture from the material definition
            QDomNodeList textureDataElems = domDocMat.documentElement()
                    .elementsByTagName(QStringLiteral("TextureData"));
            for (int i = 0, c = textureDataElems.count(); i < c; ++i) {
                nodeLists.append(textureDataElems.at(i).childNodes());
                isMatDefs.append(true);
            }

            for (int j = 0; j < nodeLists.count(); ++j) {
                for (int i = 0, c = nodeLists[j].count(); i < c; ++i) {
                    auto elem = nodeLists[j].at(i).toElement();
                    QString path;
                    if (isMatDefs[j]) {
                        if (elem.attribute(QStringLiteral("name")) == QLatin1String("sourcepath")
                            || elem.attribute(QStringLiteral("type")) == QLatin1String("Texture")) {
                            path = elem.text();
                        }
                    } else if (elem.attribute(QStringLiteral("type")) == QLatin1String("Texture")) {
                        path = elem.attribute(QStringLiteral("default"));
                        outPropertySet.insert(elem.attribute(QStringLiteral("name")));
                    }

                    if (!path.isEmpty() && !outPathMap.contains(path)) {
                        QString absAssetPath;
                        if (projectPath.isEmpty()) {
                            // Importing from library, assume relative path to file itself
                            absAssetPath = QDir::cleanPath(fileDir.absoluteFilePath(path));
                        } else {
                            // When importing from project, all paths are relative to project
                            absAssetPath = QDir::cleanPath(projDir.absoluteFilePath(path));
                        }
                        if (recurseSourceMaterial
                                && (absAssetPath.endsWith(QLatin1String(".material"))
                                    || absAssetPath.endsWith(QLatin1String(".shader")))
                                && !outPathMap.contains(path)) {
                            ParseSourcePathsOutOfEffectFile(absAssetPath, projectPath,
                                                            false, outPathMap, outPropertySet);
                        }
                        outPathMap.insert(path, absAssetPath);
                    }
                }
            }
        } else {
            qWarning() << __FUNCTION__ << "Couldn't open file:" << inFile;
        }
    }

    Q3DStudio::CString GetCustomMaterialName(const QString &inFullPathToFile) const override
    {
        Q3DStudio::CString retval;
        qt3ds::foundation::CFileSeekableIOStream theStream(inFullPathToFile,
                                                           qt3ds::foundation::FileReadFlags());
        if (theStream.IsOpen()) {
            std::shared_ptr<IDOMFactory> theFactory =
                IDOMFactory::CreateDOMFactory(m_DataCore.GetStringTablePtr());
            SImportXmlErrorHandler theImportHandler(m_Doc.GetImportFailedHandler(),
                                                    inFullPathToFile);
            qt3dsdm::SDOMElement *theElem =
                CDOMSerializer::Read(*theFactory, theStream, &theImportHandler);
            if (theElem) {
                // OK, then this just may be a valid material file.  Get the file stem of the path.
                Q3DStudio::CFilePath thePath(inFullPathToFile);
                retval = thePath.GetFileStem();
                std::shared_ptr<IDOMReader> theReader = IDOMReader::CreateDOMReader(
                    *theElem, m_DataCore.GetStringTablePtr(), theFactory);
                const char8_t *attValue;
                if (theReader->UnregisteredAtt("formalName", attValue) && !isTrivial(attValue)) {
                    retval.assign(attValue);
                }
            }
        }
        return retval;
    }

    void getMaterialInfo(const QString &inAbsoluteFilePath,
                         QString &outName, QMap<QString, QString> &outValues,
                         QMap<QString, QMap<QString, QString>> &outTextureValues) override
    {
        Q3DStudio::Q3DSMaterialDefinitionParser::getMaterialInfo(
                    inAbsoluteFilePath, g_StudioApp.GetCore()->getProjectFile().getProjectPath(),
                    m_Doc.GetDocumentDirectory(), outName, outValues, outTextureValues);

        // Fix the outName to follow the file name (in case it has changed)
        outName = getMaterialNameFromFilePath(inAbsoluteFilePath);
        outValues[QStringLiteral("name")] = outName;
    }

    ///////////////////////////////////////////////////////////////////
    // IDocumentEditor
    //////////////////////////////////////////////////////////////////

    void BeginAggregateOperation() override
    {
        m_StudioSystem.GetFullSystem()->BeginAggregateOperation();
    }
    void EndAggregateOperation() override
    {
        m_StudioSystem.GetFullSystem()->EndAggregateOperation();
    }
    void Rollback() override { m_Doc.rollbackTransaction(); }
    // Release when finished editing
    void Release() override { m_Doc.closeTransaction(); }

    bool FilterForNotInSlideAndNotInstance(Q3DStudio::TIdentifier inInstance,
                                           Qt3DSDMSlideHandle inSlide,
                                           Qt3DSDMInstanceHandle inTargetInstance)
    {
        Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
        Qt3DSDMSlideHandle theParentSlide = m_SlideSystem.GetMasterSlide(theAssociatedSlide);
        if (inTargetInstance == Qt3DSDMInstanceHandle(inInstance)
            || (theAssociatedSlide != inSlide && theAssociatedSlide != theParentSlide))
            return true; // The object is *not* in present in this slide or is the target instance
        // the object *is* present in this slide.
        return false;
    }

    void SetTimeRangeToParent(Qt3DSDMInstanceHandle inInstance)
    {
        Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
        if (theAssociatedSlide.Valid() == false)
            return;

        TSlideHandleList theChildSlides;
        m_SlideCore.GetChildSlides(theAssociatedSlide, theChildSlides);
        theChildSlides.insert(theChildSlides.begin(), theAssociatedSlide);
        Qt3DSDMInstanceHandle theParent = m_AssetGraph.GetParent(inInstance);

        Qt3DSDMPropertyHandle theStartProp = m_Bridge.GetObjectDefinitions().m_Asset.m_StartTime;
        Qt3DSDMPropertyHandle theEndProp = m_Bridge.GetObjectDefinitions().m_Asset.m_EndTime;
        bool isParentSlideOwner =
            m_Bridge.GetObjectDefinitions().IsA(theParent, ComposerObjectTypes::SlideOwner);

        for (size_t slideIdx = 0, slideEnd = theChildSlides.size(); slideIdx < slideEnd;
             ++slideIdx) {
            Qt3DSDMSlideHandle theChildSlide(theChildSlides[slideIdx]);
            pair<long, long> destTimeRange(0, 0);
            if (isParentSlideOwner) {
                // Get the previous item in the current slide.
                CGraphIterator theIterator;
                theIterator +=
                    Q3DStudio::TFilter(std::bind(&CDocEditor::FilterForNotInSlideAndNotInstance,
                                                 this, std::placeholders::_1, theChildSlide,
                                                 inInstance));
                m_AssetGraph.GetChildren(theIterator, theParent);
                Qt3DSDMInstanceHandle thePreviousItem;
                if (theIterator.IsDone())
                    continue;

                // Perform max/min of sibing times.
                for (; theIterator.IsDone() == false; ++theIterator) {
                    pair<long, long> theSiblingTime =
                        GetTimeRangeInSlide(theChildSlide, theIterator.GetCurrent());
                    destTimeRange.first = min(destTimeRange.first, theSiblingTime.first);
                    destTimeRange.second = max(destTimeRange.second, theSiblingTime.second);
                }
            } else {
                destTimeRange = GetTimeRangeInSlide(theChildSlide, theParent);
            }
            // ensure the time range is sane.
            destTimeRange.first = min(destTimeRange.first, destTimeRange.second);
            SetTimeRangeInSlide(theChildSlide, inInstance, destTimeRange.first,
                                destTimeRange.second);
        }
    }

    virtual Qt3DSDMInstanceHandle
    CreateSceneGraphInstance(ComposerObjectTypes::Enum inType, TInstanceHandle inParent,
                             TSlideHandle inSlide, TInstanceHandle inTargetId = TInstanceHandle(),
                             bool setTimeRange = true, bool selectCreatedInstance = true) override
    {
        Qt3DSDMInstanceHandle retval = IDocumentEditor::CreateSceneGraphInstance(
                    ComposerObjectTypes::Convert(inType), inParent, inSlide, m_DataCore,
                    m_SlideSystem, m_Bridge.GetObjectDefinitions(), m_AssetGraph, m_MetaData,
                    inTargetId, setTimeRange, selectCreatedInstance);
        if (setTimeRange)
            SetTimeRangeToParent(retval);
        return retval;
    }

    TInstanceHandle CreateSceneGraphInstance(ComposerObjectTypes::Enum inType,
                                             TInstanceHandle inParent, TSlideHandle inSlide,
                                             DocumentEditorInsertType::Enum inInsertType,
                                             const CPt &inPosition, EPrimitiveType inPrimitiveType,
                                             long inStartTime, bool setTimeRange = true,
                                             bool selectCreatedInstance = true) override
    {
        TInstanceHandle retval(CreateSceneGraphInstance(inType, inParent, inSlide,
                                                        TInstanceHandle(), setTimeRange));
        Q3DStudio::CString theName;
        if (inType == ComposerObjectTypes::Model) {
            const wchar_t *theSourcePath = m_Doc.GetBufferCache().GetPrimitiveName(inPrimitiveType);
            if (!IsTrivial(theSourcePath)) {
                // Trigger material generation.
                SetInstancePropertyValue(retval,
                                         m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath,
                                         std::make_shared<CDataStr>(theSourcePath));

                theName = Q3DStudio::CString(theSourcePath + 1);
            } else {
                theName = GetName(retval);
            }
        } else {
            theName = ComposerObjectTypes::Convert(inType);
            // TODO: This should work (QT3DS-2278). The line above is a quick fix in case
            // the actual reason for this to have stopped working is not found in time for 2.1
            // release.
            //theName = GetName(retval);
        }
        if (setTimeRange)
            SetTimeRangeToParent(retval);

        if (inType == ComposerObjectTypes::Layer) {
            CreateSceneGraphInstance(ComposerObjectTypes::Camera, retval, inSlide);
            CreateSceneGraphInstance(ComposerObjectTypes::Light, retval, inSlide);
        }

        if (inStartTime != -1)
            SetStartTime(retval, inStartTime);

        if (m_DataCore.IsInstanceOrDerivedFrom(
                retval, m_Bridge.GetObjectDefinitions().m_SlideOwner.m_Instance)) {
            m_Bridge.GetOrCreateGraphRoot(retval);
        }

        // Only one camera is active by default. If we already have one active, set the new camera
        // to inactive.
        if (inType == ComposerObjectTypes::Camera) {
            auto cameraLayer = m_Bridge.GetResidingLayer(retval);
            if (!m_Doc.getActiveCamera(cameraLayer).Valid()) {
                m_Doc.setActiveCamera(cameraLayer, retval);
            } else {
                SetInstancePropertyValue(
                            retval, m_Bridge.GetSceneAsset().m_Eyeball.m_Property, false);
            }
        }

        // if we did not set time range earlier, let's set it now to match parent
        TInstanceHandle handle = FinalizeAddOrDrop(retval, inParent, inInsertType, inPosition,
                                                   !setTimeRange, selectCreatedInstance, false);
        SetName(retval, theName, true);

        return handle;
    }

    TCharPtr GetSourcePath(Qt3DSDMInstanceHandle inInstance)
    {
        Option<SValue> theValue = GetInstancePropertyValue(
            inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath);
        if (theValue.hasValue()) {
            TDataStrPtr theStr(get<TDataStrPtr>(*theValue));
            if (theStr)
                return theStr->GetData();
        }
        return L"";
    }

    void DoDeleteInstance(Qt3DSDMInstanceHandle instance)
    {
        // For delete, the metadata needs to participate in the undo/redo system.
        m_MetaData.SetConsumer(m_StudioSystem.GetFullSystem()->GetConsumer());
        TInstanceHandleList theDeleteDependentInstances;
        if (instance == m_Doc.GetSceneInstance()) {
            // Something is really really wrong here. Scene should never be deleted.
            QT3DS_ASSERT(false);
            return;
        }
        if (m_Bridge.IsMaterialInstance(instance)) {
            // Go through all slides this material is involved in and
            // its root properties and eliminate any image references
            // found
            std::vector<Q3DStudio::CId> imageIdList;
            m_Doc.IterateImageInstances(instance, &imageIdList);
            for (size_t idx = 0, end = imageIdList.size(); idx < end; ++idx) {
                qt3dsdm::Qt3DSDMInstanceHandle theInstance =
                    m_Bridge.GetInstanceByGUID(imageIdList[idx]);
                if (IsInstance(theInstance))
                    m_DataCore.DeleteInstance(theInstance);
            }
        }
        if (m_Bridge.IsMaterialBaseInstance(instance)
            && !m_Bridge.IsCustomMaterialInstance(instance)) {
            // Find all material instances that may reference this instance.
            // Ensure they are marked as dirty at this point but do not change their reference
            // target
            // because a material type operation may have happened.
            TInstanceHandleList derivedInstances;
            m_DataCore.GetInstancesDerivedFrom(
                derivedInstances,
                this->m_Bridge.GetObjectDefinitions().m_ReferencedMaterial.m_Instance);
            for (size_t idx = 0, end = derivedInstances.size(); idx < end; ++idx) {
                TInstanceHandle theInstance = derivedInstances[idx];
                TPropertyHandle theProperty =
                    this->m_Bridge.GetObjectDefinitions()
                        .m_ReferencedMaterial.m_ReferencedMaterial.m_Property;
                // Find all instances of this reference type.
                std::vector<std::pair<TSlideHandle, qt3dsdm::SObjectRefType>> slideValues;
                SValue theValue;
                if (m_DataCore.GetInstancePropertyValue(theInstance, theProperty, theValue)) {
                    slideValues.push_back(
                        std::make_pair(TSlideHandle(), theValue.getData<qt3dsdm::SObjectRefType>()));
                }

                TSlideHandleList theSlides;
                GetAllAssociatedSlides(theInstance, theSlides);

                SValue theSlideValue;
                for (size_t idx = 0, end = theSlides.size(); idx < end; ++idx) {
                    Qt3DSDMSlideHandle theSlide(theSlides[idx]);
                    if (m_SlideCore.GetSpecificInstancePropertyValue(theSlide, theInstance,
                                                                     theProperty, theSlideValue))
                        slideValues.push_back(std::make_pair(
                            TSlideHandle(), theSlideValue.getData<qt3dsdm::SObjectRefType>()));
                }

                for (size_t valueIdx = 0, valueEnd = slideValues.size(); valueIdx < valueEnd;
                     ++valueIdx) {
                    std::pair<TSlideHandle, qt3dsdm::SObjectRefType> &theEntry(slideValues[valueIdx]);
                    TInstanceHandle theResolvedInstance =
                        GetInstanceForObjectRef(theInstance, theEntry.second);
                    if (theResolvedInstance == instance) {
                        if (theEntry.first.Valid())
                            m_SlideCore.SetInstancePropertyValue(theEntry.first, theInstance,
                                                                 theProperty, theEntry.second);
                        else
                            m_DataCore.SetInstancePropertyValue(theInstance, theProperty,
                                                                theEntry.second);
                    }
                }
            }
        } else if (m_Bridge.IsImageInstance(instance)) {
            // Unassign the image property from material
            Qt3DSDMInstanceHandle theParent;
            Qt3DSDMPropertyHandle theProperty;

            if (!m_Bridge.GetMaterialFromImageInstance(instance, theParent, theProperty))
                m_Bridge.GetLayerFromImageProbeInstance(instance, theParent, theProperty);
            if (theParent.Valid())
                m_PropertySystem.SetInstancePropertyValue(theParent, theProperty, SLong4());
        } else if (m_Bridge.IsBehaviorInstance(instance) || m_Bridge.IsEffectInstance(instance)
                   || m_Bridge.IsCustomMaterialInstance(instance)) {
            // Check if this is the last instance that uses the same sourcepath property
            // If yes, delete the parent as well
            Qt3DSDMInstanceHandle theObjectDefInstance;
            if (m_Bridge.IsBehaviorInstance(instance))
                theObjectDefInstance = m_Bridge.GetObjectDefinitions().m_Behavior.m_Instance;
            else if (m_Bridge.IsEffectInstance(instance))
                theObjectDefInstance = m_Bridge.GetObjectDefinitions().m_Effect.m_Instance;
            else if (m_Bridge.IsCustomMaterialInstance(instance))
                theObjectDefInstance = m_Bridge.GetObjectDefinitions().m_CustomMaterial.m_Instance;
            else
                QT3DS_ASSERT(false);

            // First, we need to get the parent instance that has the same sourcepath property
            CFilePath theSourcePath(GetSourcePath(instance));
            TInstanceHandleList theParents;
            Qt3DSDMInstanceHandle theInstanceParent;
            m_DataCore.GetInstanceParents(instance, theParents);
            for (size_t idx = 0; idx < theParents.size(); ++idx) {
                Qt3DSDMInstanceHandle theParent(theParents[idx]);
                if (m_DataCore.IsInstanceOrDerivedFrom(theParent, theObjectDefInstance)
                    && theParent != theObjectDefInstance
                    && theSourcePath.toCString() == GetSourcePath(theParent)) {
                    theInstanceParent = theParent;
                    break;
                }
            }

            // Now that we got the parent, we check how many children the parent has
            TInstanceHandleList theInstanceChildren;
            m_DataCore.GetInstancesDerivedFrom(
                theInstanceChildren,
                theInstanceParent); // this will return theInstanceParent as well
            if (theInstanceChildren.size() == 2) {
                // if there are only 2 children: theInstanceParent and instance
                // delete theInstanceParent as well
                QT3DS_ASSERT((theInstanceChildren[0] == theInstanceParent
                           && theInstanceChildren[1] == instance)
                          || (theInstanceChildren[1] == theInstanceParent
                              && theInstanceChildren[0] == instance));
                theDeleteDependentInstances.push_back(theInstanceParent);
            }
        }

        // Note that the instance and its parents are still valid.
        // we delete from the bottom of the asset graph upwards.
        m_DataCore.DeleteInstance(instance);

        if (m_AssetGraph.IsExist(instance))
            m_AssetGraph.RemoveNode(instance);

        for (size_t idx = 0; idx < theDeleteDependentInstances.size(); ++idx) {
            QT3DS_ASSERT(!m_AssetGraph.IsExist(theDeleteDependentInstances[idx]));
            m_DataCore.DeleteInstance(theDeleteDependentInstances[idx]);
        }
    }

    void RecursiveDeleteInstanceInSceneGraph(Qt3DSDMInstanceHandle instance)
    {
        while (m_AssetGraph.GetChildCount(instance))
            RecursiveDeleteInstanceInSceneGraph(m_AssetGraph.GetChild(instance, 0));
        DoDeleteInstance(instance);
    }

    void DeleteInstances(const qt3dsdm::TInstanceHandleList &instances) override
    {
        for (size_t idx = 0, end = instances.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(instances[idx]);
            if (theInstance == m_Doc.GetSceneInstance()) {
                // Something is really really wrong here. Scene should never be deleted.
                QT3DS_ASSERT(false);
                return;
            }
            if (m_AssetGraph.IsExist(theInstance)) {
                RecursiveDeleteInstanceInSceneGraph(theInstance);
            } else if (IsInstance(theInstance)) {
                // When deleting multiple instances that have a parent-descendant
                // relationship, it is possible that an instance not in asset graph
                // has already been recursively deleted in this loop.
                // We cannot do blind delete for out-of-graph items without checking
                // if they exist.
                DoDeleteInstance(theInstance);
            }
        }
    }

    void SetSpecificInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle instance,
                                                  TPropertyHandle propName, const SValue &value) override
    {
        if (inSlide.Valid() == false)
            m_DataCore.SetInstancePropertyValue(instance, propName, value);
        else
            m_SlideCore.ForceSetInstancePropertyValue(inSlide, instance, propName, value);

        IInstancePropertyCoreSignalSender *theSender =
            dynamic_cast<CStudioPropertySystem &>(m_PropertySystem).GetPropertyCoreSignalSender();
        theSender->SignalInstancePropertyValue(instance, propName, value);
    }

    void CheckMeshSubsets(TInstanceHandle instance, TPropertyHandle propName,
                          Option<pair<Qt3DSDMSlideHandle, SValue>> inValue = Empty())
    {
        // Simply ensure we have enough materials for all the subsets.
        TSlideValuePairList theValues;
        GetAllPropertyValues(instance, propName, theValues);
        if (inValue.hasValue()) {
            size_t idx = 0;
            for (size_t end = theValues.size(); idx < end; ++idx) {
                if (theValues[idx].first == inValue->first) {
                    theValues[idx].second = inValue->second;
                    break;
                }
            }
            if (idx == theValues.size())
                theValues.push_back(*inValue);
        }

        QT3DSU32 numSubsets = 0;
        for (size_t propIdx = 0, propEnd = theValues.size(); propIdx < propEnd; ++propIdx) {
            TDataStrPtr newValue(get<TDataStrPtr>(theValues[propIdx].second));
            SRenderMesh *theBuffer = m_Doc.GetBufferCache().GetOrCreateModelBuffer(
                Q3DStudio::CFilePath(newValue->GetData()));
            if (theBuffer)
                numSubsets = qMax(numSubsets, (QT3DSU32)theBuffer->m_Subsets.size());
        }

        TInstanceHandleList theMaterials;
        // Child count is required in the loop on purpose.
        for (long child = 0; child < m_AssetGraph.GetChildCount(instance); ++child) {
            Qt3DSDMInstanceHandle theMaterial(m_AssetGraph.GetChild(instance, child));
            if (m_Bridge.IsMaterialBaseInstance(theMaterial)) {
                if (theMaterials.size() < numSubsets)
                    theMaterials.push_back(theMaterial);
                else {
                    // One less material
                    DeleteInstance(theMaterial);
                    --child;
                }
            }
        }

        QT3DSU32 numMaterials = (QT3DSU32)theMaterials.size();
        // Note that I create the materials in the associated slide of the instance,
        // not the active slide at this time.  This is because materials
        // need to be with the asset at all times and aren't attached via slides
        // but are assumed to be there.
        for (; numMaterials < numSubsets; ++numMaterials) {
            theMaterials.push_back(
                        CreateSceneGraphInstance(ComposerObjectTypes::ReferencedMaterial, instance,
                                                 GetAssociatedSlide(instance)));
            setMaterialReferenceByPath(theMaterials.back(), m_Bridge.getDefaultMaterialName());
            setMaterialSourcePath(theMaterials.back(),
                                  CString::fromQString(m_Bridge.getDefaultMaterialName()));
        }

        // Now go through and if possible ensure that on each slide the name of the material matches
        // the subset name.
        for (size_t propIdx = 0, propEnd = theValues.size(); propIdx < propEnd; ++propIdx) {
            TDataStrPtr newValue(get<TDataStrPtr>(theValues[propIdx].second));
            SRenderMesh *theBuffer = m_Doc.GetBufferCache().GetOrCreateModelBuffer(
                Q3DStudio::CFilePath(newValue->GetData()));
            if (theBuffer == NULL)
                continue;
            for (long subsetIdx = 0, subsetEnd = theBuffer->m_Subsets.size(); subsetIdx < subsetEnd;
                 ++subsetIdx) {
#ifdef KDAB_TEMPORARILY_REMOVED
                StaticAssert<sizeof(wchar_t) == sizeof(char16_t)>::valid_expression();
#endif
                const wstring &theSubsetName =
                    Q3DStudio::CString(theBuffer->m_Subsets[subsetIdx].m_Name.c_str()).c_str();
                if (theSubsetName.size()) {
                    Qt3DSDMInstanceHandle theMaterial(theMaterials[subsetIdx]);
                    SValue theValue;
                    Qt3DSDMSlideHandle theSlide(theValues[propIdx].first);
                    Qt3DSDMPropertyHandle theNameProp(
                        m_Bridge.GetObjectDefinitions().m_Named.m_NameProp);
                    SValue theDMValue;
                    if (theSlide.Valid()) {
                        if (m_SlideCore.GetSpecificInstancePropertyValue(theSlide, theMaterial,
                                                                         theNameProp, theValue)
                                == true
                            && AreEqual(get<TDataStrPtr>(theValue)->GetData(),
                                        theSubsetName.c_str())
                                == false)
                            m_SlideCore.ForceSetInstancePropertyValue(
                                theSlide, theMaterial, theNameProp,
                                std::make_shared<CDataStr>(theSubsetName.c_str()));
                    } else if (m_DataCore.GetInstancePropertyValue(theMaterial, theNameProp,
                                                                   theDMValue)
                                   == false
                               || AreEqual(get<TDataStrPtr>(theDMValue)->GetData(),
                                           theSubsetName.c_str())
                                   == false) {
                        m_DataCore.SetInstancePropertyValue(
                            theMaterial, theNameProp,
                            std::make_shared<CDataStr>(theSubsetName.c_str()));
                    }
                }
            }
        }
    }

    struct PathMaterialSlots
    {
        enum Enum {
            Stroke = 1,
            Fill = 1 << 1,
            FillAndStroke = Stroke | Fill,
        };
    };

    static PathMaterialSlots::Enum GetPathMaterialSlots(const wchar_t *inPathType,
                                                        const wchar_t *inPaintStyle)
    {
        if (AreEqual(inPathType, L"Geometry"))
            return PathMaterialSlots::Stroke;
        if (AreEqual(inPaintStyle, L"Filled and Stroked"))
            return PathMaterialSlots::FillAndStroke;
        if (AreEqual(inPaintStyle, L"Filled"))
            return PathMaterialSlots::Fill;
        return PathMaterialSlots::Stroke;
    }

    eastl::pair<TInstanceHandle, Q3DStudio::DocumentEditorInsertType::Enum>
    GetInsertTypeForFirstChild(TInstanceHandle instance)
    {
        if (m_AssetGraph.GetChildCount(instance))
            return eastl::make_pair(TInstanceHandle(m_AssetGraph.GetChild(instance, 0)),
                                    Q3DStudio::DocumentEditorInsertType::PreviousSibling);
        return eastl::make_pair(instance, Q3DStudio::DocumentEditorInsertType::LastChild);
    }

    void CreatePathMaterial(TInstanceHandle instance, bool isStroke, bool hasStroke)
    {
        const wchar_t *materialName = isStroke ? L"Stroke" : L"Fill";
        TInstanceHandle firstChild;
        if (m_AssetGraph.GetChildCount(instance))
            firstChild = m_AssetGraph.GetChild(instance, 0);
        TInstanceHandle theMaterial = CreateSceneGraphInstance(
            ComposerObjectTypes::Material, instance, GetAssociatedSlide(instance));
        if (firstChild.Valid()) {
            if (isStroke)
                m_AssetGraph.MoveBefore(theMaterial, firstChild);
            else {
                if (!hasStroke)
                    m_AssetGraph.MoveBefore(theMaterial, firstChild);
                else
                    m_AssetGraph.MoveAfter(theMaterial, firstChild);
            }
        }
        SetName(theMaterial, materialName);
    }

    // Normal way in to the system.
    void SetInstancePropertyValue(TInstanceHandle instance, TPropertyHandle propName,
                                          const SValue &value, bool inAutoDelete = true) override
    {
        IPropertySystem &thePropertySystem(m_PropertySystem);
        AdditionalMetaDataType::Value theProperytMetaData =
            thePropertySystem.GetAdditionalMetaDataType(instance, propName);
        TSlideHandle theNewSlide(GetSlideForProperty(instance, propName));
        if (theProperytMetaData == AdditionalMetaDataType::Image) {
            TDataStrPtr theImageSourcePath = get<TDataStrPtr>(value);
            bool hasValue = theImageSourcePath && theImageSourcePath->GetLength() > 0;
            qt3dsdm::Qt3DSDMInstanceHandle theImageInstance =
                GetImageInstanceForProperty(instance, propName);
            if (hasValue) {
                if (theImageInstance.Valid() == false)
                    theImageInstance = CreateImageInstanceForMaterialOrLayer(instance, propName);

                if (theImageInstance) {
                    SetInstancePropertyValue(theImageInstance, m_Bridge.GetSourcePathProperty(),
                                             value, inAutoDelete);
                    // Clear subpresentation value
                    SetInstancePropertyValue(theImageInstance,
                                             m_Bridge.GetSceneImage().m_SubPresentation,
                                             std::make_shared<CDataStr>(Q3DStudio::CString()),
                                             inAutoDelete);
                }

            } else {
                if (theImageInstance.Valid()) {
                    TSlideHandle theInstanceSlide = GetAssociatedSlide(instance);
                    if (m_SlideSystem.IsMasterSlide(theInstanceSlide)) {
                        if (IsPropertyLinked(instance, propName) && inAutoDelete) {
                            DeleteImageInstanceFromMaterialOrLayer(instance, propName);
                        } else {
                            SetInstancePropertyValue(theImageInstance,
                                                     m_Bridge.GetSourcePathProperty(), value,
                                                     inAutoDelete);
                            // Clear subpresentation value
                            SetInstancePropertyValue(theImageInstance,
                                                     m_Bridge.GetSceneImage().m_SubPresentation,
                                                     std::make_shared<CDataStr>(Q3DStudio::CString()),
                                                     inAutoDelete);
                        }
                    } else {
                        DeleteImageInstanceFromMaterialOrLayer(instance, propName);
                    }
                }
            }
        } else if (theProperytMetaData == AdditionalMetaDataType::Mesh) {
            CheckMeshSubsets(instance, propName, make_pair(theNewSlide, value));
            thePropertySystem.SetInstancePropertyValue(instance, propName, value);
        } else if (theProperytMetaData == AdditionalMetaDataType::PathBuffer) {
            if (inAutoDelete) {
                TDataStrPtr newValue(get<TDataStrPtr>(value));
                if (newValue->GetLength()) {
                    eastl::vector<TInstanceHandle> subPathChildren;
                    for (QT3DSI32 idx = 0, end = m_AssetGraph.GetChildCount(instance); idx < end;
                         ++idx) {
                        TInstanceHandle child = m_AssetGraph.GetChild(instance, idx);
                        if (GetObjectTypeName(child) == L"SubPath")
                            subPathChildren.push_back(child);
                    }
                    for (QT3DSU32 idx = 0, end = subPathChildren.size(); idx < end; ++idx) {
                        DeleteInstance(subPathChildren[idx]);
                    }
                }
            }
            thePropertySystem.SetInstancePropertyValue(instance, propName, value);
        } else if (theProperytMetaData == AdditionalMetaDataType::Import && inAutoDelete) {
            TInstanceList childList;
            GetChildren(theNewSlide, instance, childList);
            for (size_t idx = 0, end = childList.size(); idx < end; ++idx) {
                if (IsImported(childList[idx]))
                    DeleteInstance(childList[idx]);
            }
            // Run import operation with no handler for errors is the best I can do right now.

            TDataStrPtr newValue(get<TDataStrPtr>(value));

            CFilePath docPath(m_Doc.GetDocumentPath());
            CFilePath docDir(docPath.GetDirectory());
            STranslationLog log;
            CFilePath theFullPathToDocument(
                m_Doc.GetResolvedPathToDoc(CFilePath(newValue->GetData())));
            if (newValue && *newValue->GetData() && theFullPathToDocument.Exists()) {
                std::pair<long, long> times = GetTimeRange(instance);
                DoImport(theFullPathToDocument, theFullPathToDocument,
                         m_AssetGraph.GetParent(instance), instance, theNewSlide, docDir, log,
                         std::bind(CPerformImport::ImportToComposerFromImportFile,
                                   std::placeholders::_1, std::placeholders::_2),
                         DocumentEditorInsertType::Unknown, CPt(), times.first, false);
            }
            thePropertySystem.SetInstancePropertyValue(instance, propName, value);
        } else if (propName == m_Bridge.GetObjectDefinitions().m_Path.m_PathType
                   || propName == m_Bridge.GetObjectDefinitions().m_Path.m_PaintStyle) {
            TDataStrPtr oldPathType = GetTypedInstancePropertyValue<TDataStrPtr>(
                instance, m_Bridge.GetObjectDefinitions().m_Path.m_PathType);
            TDataStrPtr oldPaintStyle = GetTypedInstancePropertyValue<TDataStrPtr>(
                instance, m_Bridge.GetObjectDefinitions().m_Path.m_PaintStyle);
            TDataStrPtr newPathType;
            TDataStrPtr newPaintStyle;
            if (propName == m_Bridge.GetObjectDefinitions().m_Path.m_PathType) {
                newPaintStyle = oldPaintStyle;
                newPathType = get<TDataStrPtr>(value);
            } else {
                newPathType = oldPathType;
                newPaintStyle = get<TDataStrPtr>(value);
            }
            PathMaterialSlots::Enum oldMaterialSlot =
                GetPathMaterialSlots(oldPathType->GetData(), oldPaintStyle->GetData());
            PathMaterialSlots::Enum newMaterialSlot =
                GetPathMaterialSlots(newPathType->GetData(), newPaintStyle->GetData());
            if (oldMaterialSlot != newMaterialSlot) {
                bool hasStroke = (((int)oldMaterialSlot) & PathMaterialSlots::Stroke) > 0;
                bool hasFill = (((int)oldMaterialSlot) & PathMaterialSlots::Fill) > 0;
                bool needsStroke = (((int)newMaterialSlot) & PathMaterialSlots::Stroke) > 0;
                bool needsFill = (((int)newMaterialSlot) & PathMaterialSlots::Fill) > 0;
                // first, remove any materials that should not be there.
                qt3dsdm::Qt3DSDMInstanceHandle firstMaterial;
                qt3dsdm::Qt3DSDMInstanceHandle secondMaterial;
                for (int idx = 0, end = m_AssetGraph.GetChildCount(instance); idx < end; ++idx) {
                    TInstanceHandle childAsset = m_AssetGraph.GetChild(instance, idx);
                    if (m_Bridge.IsMaterialInstance(childAsset)) {
                        if (firstMaterial.Valid())
                            secondMaterial = childAsset;
                        else
                            firstMaterial = childAsset;
                    }
                }
                if (hasStroke && !needsStroke) {
                    if (firstMaterial.Valid())
                        DeleteInstance(firstMaterial);
                }
                if (hasFill && !needsFill) {
                    if (hasStroke) {
                        if (secondMaterial.Valid())
                            DeleteInstance(secondMaterial);
                    } else if (firstMaterial.Valid())
                        DeleteInstance(firstMaterial);
                }

                if (needsStroke && !hasStroke) {
                    CreatePathMaterial(instance, true, false);
                }
                if (needsFill && !hasFill) {
                    CreatePathMaterial(instance, false, needsStroke);
                }
            }
            // Now set the property for reals
            thePropertySystem.SetInstancePropertyValue(instance, propName, value);
        } else if (propName == m_Bridge.GetSceneAsset().m_Eyeball
                   && m_Bridge.IsCameraInstance(instance)) {
            auto cameraLayer = m_Bridge.GetResidingLayer(instance);
            auto activeCamera = m_Doc.getActiveCamera(cameraLayer);
            if ((instance != activeCamera) && get<bool>(value)) {
                // Only one camera per layer should be active. Set the previous one to inactive if
                // we are activating another one.
                if (activeCamera.Valid())
                    thePropertySystem.SetInstancePropertyValue(activeCamera, propName, false);
                m_Doc.setActiveCamera(cameraLayer, instance);
            } else if (!get<bool>(value) && activeCamera == instance) {
                m_Doc.setActiveCamera(cameraLayer, {}); // Inactivating current active camera.
            }
            thePropertySystem.SetInstancePropertyValue(instance, propName, value);
        } else {
            if (propName != m_Bridge.GetAlias().m_ReferencedNode.m_Property) {
                thePropertySystem.SetInstancePropertyValue(instance, propName, value);
            } else {
                // Alias properties are set in the scene graph, not in the slides.
                // This makes the runtime expansion easier and stops problems such as
                // someone unlinking the alias
                // node reference and setting it to different values on different slides.
                m_DataCore.SetInstancePropertyValue(instance, propName, value);
            }
        }
    }

    TInstanceHandle CreateImageInstanceForMaterialOrLayer(TInstanceHandle instance,
                                                          TPropertyHandle propName)
    {
        // Check to make sure there isn't one already assigned here.
        {
            qt3dsdm::Qt3DSDMInstanceHandle theImageInstance =
                GetImageInstanceForProperty(instance, propName);
            if (theImageInstance.Valid())
                return theImageInstance;
        }

        Qt3DSDMSlideHandle theSlide(GetAssociatedSlide(instance));
        TInstanceHandle theImageInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::Image, instance, theSlide);
        const Q3DStudio::TGUIDPacked thePackedGuid(m_Bridge.GetGUID(theImageInstance));
        qt3dsdm::SLong4 theImageGuid(thePackedGuid.Data1, thePackedGuid.Data2, thePackedGuid.Data3,
                                     thePackedGuid.Data4);
        m_SlideCore.ForceSetInstancePropertyValue(theSlide, instance, propName, theImageGuid);
        if (propName == m_Bridge.GetObjectDefinitions().m_Material.m_SpecularReflection) {
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TextureMapping,
                                     std::make_shared<CDataStr>(L"Environmental Mapping"), false);
        } else if (propName == m_Bridge.GetObjectDefinitions().m_Layer.m_LightProbe
                   || propName == m_Bridge.GetObjectDefinitions().m_Layer.m_LightProbe2) {
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TextureMapping,
                                     std::make_shared<CDataStr>(L"Light Probe"), false);
            // Preserve legacy behavior where image based lighting used always tiling for
            // horizontal direction
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TilingU,
                                     std::make_shared<CDataStr>(L"Tiled"), false);
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TilingV,
                                     std::make_shared<CDataStr>(L"No Tiling"), false);
        } else if (propName == m_Bridge.GetObjectDefinitions().m_MaterialBase.m_IblProbe) {
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TextureMapping,
                                     std::make_shared<CDataStr>(L"Light Probe"), false);
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TilingU,
                                     std::make_shared<CDataStr>(L"Tiled"), false);
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TilingV,
                                     std::make_shared<CDataStr>(L"No Tiling"), false);
        }
        return theImageInstance;
    }

    void DeleteImageInstanceFromMaterialOrLayer(TInstanceHandle instance, TPropertyHandle propName)
    {
        Qt3DSDMSlideHandle theAssociatedSlide(GetAssociatedSlide(instance));
        qt3dsdm::Qt3DSDMInstanceHandle theImageInstance =
            GetImageInstanceForProperty(instance, propName);
        if (theImageInstance.Valid()) {
            DeleteInstance(theImageInstance);
            m_SlideCore.SetInstancePropertyValue(theAssociatedSlide, instance, propName, SLong4());
        }
    }

    TInstanceHandle SetInstancePropertyValueAsImage(TInstanceHandle instance,
                                                    TPropertyHandle propName,
                                                    const Q3DStudio::CString &inSourcePath) override
    {
        CFilePath thePath = m_Doc.GetResolvedPathToDoc(inSourcePath);
        assert(thePath.IsFile());
        if (!thePath.IsFile())
            return 0;
        Qt3DSDMSlideHandle theSlide(GetAssociatedSlide(instance));

        TInstanceHandle theImageInstance =
            CreateImageInstanceForMaterialOrLayer(instance, propName);
        TDataStrPtr thePtrPath(new CDataStr(inSourcePath, inSourcePath.size()));
        SetInstancePropertyValue(instance, propName, thePtrPath);

        return theImageInstance;
    }

    virtual TInstanceHandle
    SetInstancePropertyValueAsRenderable(TInstanceHandle instance, TPropertyHandle propName,
                                         const Q3DStudio::CString &inSourcePath) override
    {
        CFilePath thePath = m_Doc.GetResolvedPathToDoc(inSourcePath);
        // Delete any existing renderable object children.
        vector<TInstanceHandle> childrenToDelete;
        for (long idx = 0, end = m_AssetGraph.GetChildCount(instance); idx < end; ++idx) {
            TInstanceHandle existingChild = m_AssetGraph.GetChild(instance, idx);
            if (m_Bridge.IsRenderPluginInstance(existingChild))
                childrenToDelete.push_back(existingChild);
        }
        for (size_t childIdx = 0, childEnd = childrenToDelete.size(); childIdx < childEnd;
             ++childIdx)
            DeleteInstance(childrenToDelete[childIdx]);

        // If this is an image instance, set the inSourcePath also as the value of corresponding
        // image property in the parent
        if (m_Bridge.IsImageInstance(instance)) {
            Qt3DSDMInstanceHandle parent;
            Qt3DSDMPropertyHandle imageProperty;
            if (!m_Bridge.GetMaterialFromImageInstance(instance, parent, imageProperty))
                m_Bridge.GetLayerFromImageProbeInstance(instance, parent, imageProperty);
            bool parentEmptied = false;
            if (parent.Valid()) {
                SetInstancePropertyValue(parent, imageProperty,
                                         std::make_shared<qt3dsdm::CDataStr>(inSourcePath.c_str()),
                                         true);
                // Setting the parent image property to empty will delete the image child,
                // so we should skip setting the property there
                if (inSourcePath.IsEmpty())
                    parentEmptied = true;
            }
            if (!parentEmptied) {
                SetInstancePropertyValue(instance, propName,
                                         std::make_shared<qt3dsdm::CDataStr>(inSourcePath.c_str()),
                                         true);
            }
        } else if (m_Bridge.IsLayerInstance(instance)
                   && m_Bridge.GetSourcePathProperty() == propName
                   && !inSourcePath.IsEmpty()) {
            // Resize the layer to be the size of the presentation
            QSize presSize(g_StudioApp.getRenderableSize(inSourcePath.toQString()));
            auto &layer = m_Bridge.GetLayer();

            // Determine if width and height properties are visible
            auto isPropertyVisible = [this, &instance](TPropertyHandle propHandle) {
                IMetaData &metaData = *m_Doc.GetStudioSystem()->GetActionMetaData();
                Qt3DSDMMetaDataPropertyHandle metaHandle
                        = metaData.GetMetaDataProperty(instance, propHandle);
                qt3ds::foundation::NVConstDataRef<SPropertyFilterInfo> filters(
                            metaData.GetMetaDataPropertyFilters(metaHandle));
                if (filters.size()) {
                    qt3dsdm::IPropertySystem &propertySystem(
                                *m_Doc.GetStudioSystem()->GetPropertySystem());
                    for (QT3DSU32 propIdx = 0, propEnd = filters.size(); propIdx < propEnd;
                         ++propIdx) {
                        const SPropertyFilterInfo &filter(filters[propIdx]);
                        SValue value;
                        propertySystem.GetInstancePropertyValue(
                                    instance, filter.m_FilterProperty, value);
                        if (value == filter.m_Value)
                            return true;
                    }
                }
                return false;
            };
            bool widthVisible = isPropertyVisible(layer.m_Width);
            bool heightVisible = isPropertyVisible(layer.m_Height);

            // If width is visible, adjust that. Otherwise adjust right in relation to left.
            SValue pixelValue = std::make_shared<CDataStr>(L"pixels");
            SValue percentValue = std::make_shared<CDataStr>(L"percent");
            if (widthVisible) {
                SetInstancePropertyValue(instance, layer.m_WidthUnits, pixelValue, true);
                SetInstancePropertyValue(instance, layer.m_Width, float(presSize.width()), true);
            } else {
                long curWidth = m_Doc.GetCore()->GetStudioProjectSettings()
                        ->getPresentationSize().width();
                Option<SValue> leftVal = GetInstancePropertyValue(instance, layer.m_Left);
                Option<SValue> leftUnitsVal = GetInstancePropertyValue(instance, layer.m_LeftUnits);
                float left = qt3dsdm::get<float>(leftVal.getValue());
                if (Equals(leftUnitsVal, percentValue))
                    left = (curWidth * left) / 100;
                float right = curWidth - (left + float(presSize.width()));
                SetInstancePropertyValue(instance, layer.m_RightUnits, pixelValue, true);
                SetInstancePropertyValue(instance, layer.m_Right, right, true);
            }
            // If height is visible, adjust that. Otherwise adjust bottom in relation to top.
            if (heightVisible) {
                SetInstancePropertyValue(instance, layer.m_HeightUnits, pixelValue, true);
                SetInstancePropertyValue(instance, layer.m_Height, float(presSize.height()), true);
            } else {
                long curHeight = m_Doc.GetCore()->GetStudioProjectSettings()
                        ->getPresentationSize().height();
                Option<SValue> topVal = GetInstancePropertyValue(instance, layer.m_Top);
                Option<SValue> topUnitsVal = GetInstancePropertyValue(instance, layer.m_TopUnits);
                float top = qt3dsdm::get<float>(topVal.getValue());
                if (Equals(topUnitsVal, percentValue))
                    top = (curHeight * top) / 100;
                float bottom = curHeight - (top + float(presSize.height()));
                SetInstancePropertyValue(instance, layer.m_BottomUnits, pixelValue, true);
                SetInstancePropertyValue(instance, layer.m_Bottom, bottom, true);
            }
            SetInstancePropertyValue(instance, propName,
                                     std::make_shared<qt3dsdm::CDataStr>(inSourcePath.c_str()),
                                     true);
        } else {
            SetInstancePropertyValue(instance, propName,
                                     std::make_shared<qt3dsdm::CDataStr>(inSourcePath.c_str()),
                                     true);
        }

        // If this is a render plugin
        if (thePath.Exists() && thePath.GetExtension().CompareNoCase("plugin")) {
            Qt3DSDMSlideHandle theSlide(GetAssociatedSlide(instance));
            return LoadRenderPlugin(thePath, instance, theSlide,
                                    DocumentEditorInsertType::LastChild, -1);
        }
        return TInstanceHandle();
    }

    /**
     * Sets an instance's image-type property from a renderable or image. If no texture exists, a
     * new one is created. Next, the texture property from 'prop' param is set to the 'src' param.
     *
     * @param instance the instance
     * @param prop the instance image property
     * @param src the presentation Id or image file name to set for the texture
     */
    void setInstanceImagePropertyValue(TInstanceHandle instance, TPropertyHandle prop,
                                       const CString &src, bool isSubp = true) override
    {
        Qt3DSDMPropertyHandle img = GetImageInstanceForProperty(instance, prop);

        if (!img)
            img = CreateImageInstanceForMaterialOrLayer(instance, prop);

        SetInstancePropertyValueAsRenderable(img, isSubp ? m_Bridge.getSubpresentationProperty()
                                                         : m_Bridge.GetSourcePathProperty(), src);
    }

    /**
     * Create a rect under the active layer and set its material's diffuse map from the provided
     * source.
     *
     * @param src The presentation Id or image file name to set for the texture
     * @param slide The slide to add to
     * @param isSubPres If true, the src parameter is a subpresentation Id
     * @param pos Add position in the scene
     * @param startTime Add at this start time
     */
    void addRectFromSource(const CString &src, TSlideHandle slide, bool isSubPres,
                           const CPt &pos = {}, long startTime = -1) override
    {
        qt3dsdm::Qt3DSDMPropertyHandle activeLayer = m_Doc.GetActiveLayer();
        const auto absSrc = QFileInfo(m_Doc.GetDocumentPath()).dir()
                .absoluteFilePath(src.toQString());
        if (isSubPres) {
            qt3dsdm::Qt3DSDMInstanceHandle rectInstance =
                CreateSceneGraphInstance(ComposerObjectTypes::Model, activeLayer, slide);
            m_PropertySystem.SetInstancePropertyValue(
                rectInstance, m_Bridge.GetSourcePathProperty(),
                std::make_shared<qt3dsdm::CDataStr>(
                    m_Doc.GetBufferCache().GetPrimitiveName(PRIMITIVETYPE_RECT)));

            createRefMaterialFromImageOrPresentation(rectInstance, slide, src, true);
            SetName(rectInstance, src, true);

            FinalizeAddOrDrop(rectInstance, activeLayer,
                              Q3DStudio::DocumentEditorInsertType::LastChild,
                              pos, startTime == -1);
        } else {
            AutomapImage(CString::fromQString(absSrc), activeLayer, slide,
                         Q3DStudio::DocumentEditorInsertType::LastChild, pos, startTime);
        }
    }

    void SetMaterialType(TInstanceHandle instance,
                         const QString &inRelativePathToMaterialFile) override
    {
        if (m_Bridge.GetSourcePath(instance) == inRelativePathToMaterialFile)
            return;

        TInstanceHandle model = m_AssetGraph.GetParent(instance);
        TInstanceHandle newMaterial;
        TSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(model);
        // Keep material names the same so that if you change the material type
        // any relative path links will still work.
        // Next bug is harder (keep id's the same).
        Q3DStudio::CString fileId = GetFileId(instance);
        Q3DStudio::CString theName = GetName(instance);
        SLong4 theGuid = m_Bridge.GetInstanceGUID(instance);
        TInstanceHandle nextChild = m_AssetGraph.GetSibling(instance, true);
        // Now get all the actions on the material and re-add them.

        TActionHandleList theActions;
        m_ActionCore.GetActions(instance, theActions);
        std::vector<SActionInfo> theActionData;
        std::vector<std::vector<SHandlerArgumentInfo>> theActionDataArgs;
        for (size_t actionIdx = 0, actionEnd = theActions.size(); actionIdx < actionEnd;
             ++actionIdx) {
            theActionData.push_back(m_ActionCore.GetActionInfo(theActions[actionIdx]));
            theActionDataArgs.push_back(std::vector<SHandlerArgumentInfo>());
            std::vector<SHandlerArgumentInfo> &theInfoList(theActionDataArgs.back());
            for (size_t argIdx = 0, argEnd = theActionData.back().m_HandlerArgs.size();
                 argIdx < argEnd; ++argIdx)
                theInfoList.push_back(m_ActionCore.GetHandlerArgumentInfo(
                    theActionData.back().m_HandlerArgs[argIdx]));
        }

        // save lightmap values since we want to pass on the current lightmaps settings to the new
        // material
        Option<SValue> theLightmapIndirectValue = GetInstancePropertyValue(
            instance, m_Bridge.GetObjectDefinitions().m_Lightmaps.m_LightmapIndirect);
        Option<SValue> theLightmapRadiosityValue = GetInstancePropertyValue(
            instance, m_Bridge.GetObjectDefinitions().m_Lightmaps.m_LightmapRadiosity);
        Option<SValue> theLightmapShadowValue = GetInstancePropertyValue(
            instance, m_Bridge.GetObjectDefinitions().m_Lightmaps.m_LightmapShadow);

        DeleteInstance(instance);
        if (inRelativePathToMaterialFile == QLatin1String("Standard Material")) {
            newMaterial = CreateSceneGraphInstance(ComposerObjectTypes::Material, model, theSlide,
                                                   instance);
        } else if (inRelativePathToMaterialFile == QLatin1String("Referenced Material")) {
            newMaterial = CreateSceneGraphInstance(ComposerObjectTypes::ReferencedMaterial, model,
                                                   theSlide, instance);
        } else {
            CFilePath thePath = m_Doc.GetResolvedPathToDoc(inRelativePathToMaterialFile);
            newMaterial = LoadCustomMaterial(thePath, model, theSlide,
                                             DocumentEditorInsertType::LastChild, 0, instance);
        }

        if (newMaterial.Valid() && nextChild.Valid())
            m_AssetGraph.MoveBefore(newMaterial, nextChild);

        // restore current lightmap settings for new material
        if (theLightmapIndirectValue.hasValue())
            SetInstancePropertyValue(newMaterial,
                                     m_Bridge.GetObjectDefinitions().m_Lightmaps.m_LightmapIndirect,
                                     theLightmapIndirectValue, false);
        if (theLightmapRadiosityValue.hasValue())
            SetInstancePropertyValue(
                newMaterial, m_Bridge.GetObjectDefinitions().m_Lightmaps.m_LightmapRadiosity,
                theLightmapRadiosityValue, false);
        if (theLightmapShadowValue.hasValue())
            SetInstancePropertyValue(newMaterial,
                                     m_Bridge.GetObjectDefinitions().m_Lightmaps.m_LightmapShadow,
                                     theLightmapShadowValue, false);

        m_DataCore.SetInstancePropertyValue(newMaterial,
                                            m_Bridge.GetObjectDefinitions().m_Asset.m_FileId,
                                            std::make_shared<CDataStr>(fileId.c_str()));
        SetName(newMaterial, theName, false);
        m_Bridge.SetInstanceGUID(newMaterial, theGuid);
        // Copy all actions from old material instance to new material instance
        for (size_t actionIdx = 0, actionEnd = theActionData.size(); actionIdx < actionEnd;
             ++actionIdx) {
            const SActionInfo &theSourceInfo(theActionData[actionIdx]);

            Qt3DSDMActionHandle theNewAction = AddAction(
                theSourceInfo.m_Slide, newMaterial, theSourceInfo.m_Event, theSourceInfo.m_Handler);
            m_ActionCore.SetTriggerObject(theNewAction, theSourceInfo.m_TriggerObject);
            m_ActionCore.SetTargetObject(theNewAction, theSourceInfo.m_TargetObject);
            std::vector<SHandlerArgumentInfo> &theInfoList(theActionDataArgs[actionIdx]);
            for (size_t argIdx = 0, argEnd = theInfoList.size(); argIdx < argEnd; ++argIdx) {
                const SHandlerArgumentInfo &theArgData(theInfoList[argIdx]);
                Qt3DSDMHandlerArgHandle theParamHandle = m_ActionCore.AddHandlerArgument(
                    theNewAction, theArgData.m_Name, theArgData.m_ArgType, theArgData.m_ValueType);
                m_ActionCore.SetHandlerArgumentValue(theParamHandle, theArgData.m_Value);
            }
        }
        m_Doc.SelectDataModelObject(newMaterial);
    }

    QString getMaterialDirectoryPath() const override
    {
        return m_Doc.GetCore()->getProjectFile().getProjectPath() + QStringLiteral("/materials/");
    }

    QString getMaterialFilePath(const QString &materialName) const override
    {
        QString actualMaterialName = materialName;
        int slashIndex = actualMaterialName.lastIndexOf(QLatin1Char('/'));
        if (slashIndex != -1)
            actualMaterialName = actualMaterialName.mid(slashIndex + 1);
        return getMaterialDirectoryPath() + actualMaterialName + QStringLiteral(".materialdef");
    }

    void writeMaterialFile(Qt3DSDMInstanceHandle instance, bool createNewFile) override
    {
        const auto materialName = CFilePath::MakeSafeFileStem(GetName(instance)).toQString();
        writeMaterialFile(instance, materialName, createNewFile,
                          getFilePathFromMaterialName(materialName));
    }

    Q3DStudio::CString writeMaterialFile(Qt3DSDMInstanceHandle instance,
                                         const QString &materialName,
                                         bool createNewFile,
                                         const QString &sourcePath = {}) override
    {
        if (materialName == getMaterialNameFromFilePath(m_Bridge.getDefaultMaterialName()))
            return "";

        EStudioObjectType type = m_Bridge.GetObjectType(instance);

        if (type == EStudioObjectType::OBJTYPE_MATERIAL
            || type == EStudioObjectType::OBJTYPE_CUSTOMMATERIAL) {
            QString actualSourcePath = sourcePath;
            if (actualSourcePath.isEmpty())
                actualSourcePath = getMaterialFilePath(materialName);

            QFileInfo fileInfo(actualSourcePath);
            if (!fileInfo.dir().exists())
                fileInfo.dir().mkpath(QStringLiteral("."));

            QFile file(actualSourcePath);
            if ((createNewFile && !file.exists()) || (!createNewFile && file.exists()))
                saveMaterial(instance, file);
            return Q3DStudio::CString::fromQString(m_Doc.GetRelativePathToDoc(actualSourcePath));
        }

        return "";
    }

    void writeProperty(QFile &file, const QString &name, const QString &value,
                       int indent = 1, bool isTexture = false, bool useCData = true)
    {
        for (int i = 0; i < indent; ++i)
            file.write("\t");
        file.write("<Property name=\"");
        file.write(name.toUtf8().constData());
        if (isTexture)
            file.write("\" type=\"Texture");
        file.write("\">");
        if (useCData) {
            QString cDataValue = QStringLiteral("<![CDATA[") + value + QStringLiteral("]]>");
            file.write(cDataValue.toUtf8().constData());
        } else {
            file.write(value.toUtf8().constData());
        }
        file.write("</Property>\n");
    }

    void writeProperty(QFile &file, const QString &name, const SValue &value, int indent = 1,
                       bool isTexture = false)
    {
        MemoryBuffer<RawAllocator> tempBuffer;
        WCharTWriter writer(tempBuffer);
        WStrOps<SValue>().ToBuf(value, writer);
        tempBuffer.write(0);

        if (tempBuffer.size()) {
            bool useCData = name == QLatin1String("name");
            writeProperty(file, name,
                          QString::fromWCharArray((const wchar_t *)tempBuffer.begin()),
                          indent, isTexture, useCData);
        }
    }

    bool isSaveableMaterialProperty(const QString& name) {
        return name != QLatin1String("starttime")
                && name != QLatin1String("endtime")
                && name != QLatin1String("controlledproperty")
                && name != QLatin1String("eyeball")
                && name != QLatin1String("shy")
                && name != QLatin1String("locked")
                && name != QLatin1String("id")
                && name != QLatin1String("fileid")
                && name != QLatin1String("timebarcolor")
                && name != QLatin1String("timebartext");
    }

    void saveIfMaterial(Qt3DSDMInstanceHandle instance) override
    {
        Qt3DSDMInstanceHandle material;
        if (m_Bridge.isInsideMaterialContainer(instance)) {
            const auto type = m_Bridge.GetObjectType(instance);
            if (type == OBJTYPE_MATERIAL || type == OBJTYPE_CUSTOMMATERIAL) {
                material = instance;
            } else {
                const auto parent = m_Bridge.GetParentInstance(instance);
                const auto parentType = m_Bridge.GetObjectType(parent);
                if (parentType == OBJTYPE_MATERIAL || parentType == OBJTYPE_CUSTOMMATERIAL)
                    material = parent;
            }
        }

        if (material.Valid())
            writeMaterialFile(material, false);
    }

    void saveMaterial(Qt3DSDMInstanceHandle instance, QFile &file)
    {
        SValue value;
        file.open(QIODevice::WriteOnly);
        file.write("<MaterialData version=\"1.0\">\n");
        QMap<QString, Qt3DSDMInstanceHandle> textureHandles;
        qt3dsdm::TPropertyHandleList propList;

        const QDir docDir = m_Doc.GetDocumentDirectory();
        const QDir projDir = g_StudioApp.GetCore()->getProjectFile().getProjectPath();
        auto sourcePathProp = m_Bridge.GetSourcePathProperty();
        // Importing interprets "./" prefix to mean project dir relative
        const QString projPrefix = QStringLiteral("./");

        m_PropertySystem.GetAggregateInstanceProperties(instance, propList);
        for (auto &prop : propList) {
            const auto name = QString::fromWCharArray(m_PropertySystem.GetName(prop).wide_str());

            if (!isSaveableMaterialProperty(name))
                continue;

            if (m_AnimationSystem.IsPropertyAnimated(instance, prop))
                continue;

            m_PropertySystem.GetInstancePropertyValue(instance, prop, value);

            if (!value.empty()) {
                bool valid = true;
                bool isTexture = false;
                bool isPath = true;
                QString strValue;
                if (value.getType() == DataModelDataType::Long4) {
                    SLong4 guid = get<qt3dsdm::SLong4>(value);
                    if (guid.Valid()) {
                        auto ref = m_Bridge.GetInstanceByGUID(guid);
                        textureHandles[name] = ref;
                        strValue = m_Bridge.GetSourcePath(ref);
                        if (strValue.isEmpty()) {
                            strValue = m_Bridge.getSubpresentation(ref).toQString();
                            isPath = false;
                        }
                    } else {
                        valid = false;
                    }
                } else {
                    qt3dsdm::AdditionalMetaDataType::Value additionalMetaDataType
                        = m_PropertySystem.GetAdditionalMetaDataType(instance, prop);
                    if (additionalMetaDataType == AdditionalMetaDataType::Texture) {
                        isTexture = true;
                        TDataStrPtr strPtr = get<TDataStrPtr>(value);
                        strValue = QString::fromWCharArray(strPtr->GetData());
                    } else if (sourcePathProp == prop) {
                        TDataStrPtr strPtr = get<TDataStrPtr>(value);
                        strValue = QString::fromWCharArray(strPtr->GetData());
                    }
                }

                if (strValue.isEmpty() && valid) {
                    writeProperty(file, name, value);
                } else if (!strValue.isEmpty()) {
                    // Save paths relative to the project instead of the presentation.
                    // This makes it possible to use same material from multiple presentations
                    // that are not all in the same folder.
                    if (isPath) {
                        strValue = projPrefix
                                + projDir.relativeFilePath(docDir.absoluteFilePath(strValue));
                    }
                    writeProperty(file, name, strValue, 1, isTexture);
                }
            }
        }

        const QFileInfo fileInfo(file);
        writeProperty(file, QStringLiteral("path"), fileInfo.absoluteFilePath());

        QMapIterator<QString, Qt3DSDMInstanceHandle> i(textureHandles);
        while (i.hasNext()) {
            i.next();
            const auto &texName = i.key();
            const auto &handle = i.value();
            file.write(QByteArrayLiteral("\t<TextureData name=\"")
                       + texName.toUtf8() + QByteArrayLiteral("\">\n"));
            propList.clear();
            m_PropertySystem.GetAggregateInstanceProperties(handle, propList);
            for (auto &prop : propList) {
                const auto name = QString::fromWCharArray(
                            m_PropertySystem.GetName(prop).wide_str());

                if (!isSaveableMaterialProperty(name))
                    continue;

                if (m_AnimationSystem.IsPropertyAnimated(handle, prop))
                    continue;

                m_PropertySystem.GetInstancePropertyValue(handle, prop, value);
                if (!value.empty()) {
                    if (sourcePathProp == prop) {
                        TDataStrPtr strPtr = get<TDataStrPtr>(value);
                        QString strValue = QString::fromWCharArray(strPtr->GetData());
                        strValue = projPrefix
                                + projDir.relativeFilePath(docDir.absoluteFilePath(strValue));
                        writeProperty(file, name, strValue, 2);
                    } else {
                        writeProperty(file, name, value, 2);
                    }
                }
            }
            file.write("\t</TextureData>\n");
        }

        file.write("</MaterialData>");
    }

    Qt3DSDMInstanceHandle getOrCreateMaterialContainer()
    {
        auto instance = m_Bridge.getMaterialContainer();
        if (!instance.Valid()) {
            IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
            Qt3DSDMInstanceHandle parent;
            CRelativePathTools::EPathType type;
            objRefHelper->ResolvePath(m_Doc.GetSceneInstance(),
                                      CString::fromQString(
                                          m_Bridge.getMaterialContainerParentPath()),
                                      type, parent, true);
            if (!parent.Valid())
                parent = m_Doc.GetSceneInstance();
            Qt3DSDMSlideHandle slide = m_Bridge.GetOrCreateGraphRoot(parent);
            instance = CreateSceneGraphInstance(ComposerObjectTypes::Material, parent,
                                                slide, DocumentEditorInsertType::LastChild,
                                                CPt(), PRIMITIVETYPE_UNKNOWN, -1, true, false);
            SetName(instance, CString::fromQString(m_Bridge.getMaterialContainerName()));
            m_SlideCore.forceSetInstancePropertyValueOnAllSlides(
                        instance, m_Bridge.GetSceneAsset().m_EndTime, 0);
        }
        return instance;
    }

    QString getFilePathFromMaterialName(const QString &name) override
    {
        return QDir(m_Doc.GetCore()->getProjectFile().getProjectPath())
                .absoluteFilePath(name + QStringLiteral(".materialdef"));
    }

    QString getMaterialNameFromFilePath(const QString &path) override
    {
        QString materialName;
        QString dirPath;
        if (path.contains(QLatin1String(".materialdef"))) {
            QDir dir(path);
            if (dir.isAbsolute())
                dirPath = m_Doc.GetRelativePathToDoc(path);
            else
                dirPath = dir.path();

            QFileInfo fi = QFileInfo(dirPath);
            materialName = fi.completeBaseName();
            dirPath = fi.path();
            dirPath.remove(QLatin1String("../"));
            if (dirPath.startsWith(QLatin1String("..")))
                dirPath = dirPath.mid(2);
        } else {
            if (!materialName.startsWith(QLatin1String("materials/")))
                dirPath = QLatin1String("materials");
            materialName = path;
        }
        if (dirPath.size() == 0)
            return materialName;
        return dirPath + QLatin1Char('/') + materialName;
    }

    Qt3DSDMInstanceHandle getMaterial(const QString &path) override
    {
        IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
        QString name = m_Bridge.getMaterialContainerPath() + QStringLiteral(".")
                + getMaterialNameFromFilePath(path)
                .replace(QLatin1Char('.'), QLatin1String("\\."));
        Qt3DSDMInstanceHandle material;
        CRelativePathTools::EPathType type;
        objRefHelper->ResolvePath(m_Doc.GetSceneInstance(),
                                  Q3DStudio::CString::fromQString(name),
                                  type, material, true);
        return material;
    }

    Qt3DSDMInstanceHandle getOrCreateMaterial(const QString &path,
                                              bool selectCreatedInstance = true) override
    {
        auto material = getMaterial(path);
        if (!material.Valid()) {
            auto parent = getOrCreateMaterialContainer();
            material = CreateSceneGraphInstance(ComposerObjectTypes::Material, parent,
                                                GetAssociatedSlide(parent),
                                                DocumentEditorInsertType::LastChild,
                                                CPt(), PRIMITIVETYPE_UNKNOWN, -1, true,
                                                selectCreatedInstance);
            SetName(material, Q3DStudio::CString::fromQString(getMaterialNameFromFilePath(path)));
        }
        return material;
    }

    void setMaterialProperties(TInstanceHandle instance,
                               const Q3DStudio::CString &materialSourcePath,
                               const QMap<QString, QString> &values,
                               const QMap<QString, QMap<QString, QString>> &textureValues) override
    {
        SetMaterialType(instance, QStringLiteral("Referenced Material"));
        setMaterialSourcePath(instance, materialSourcePath);
        setMaterialValues(materialSourcePath.toQString(), values, textureValues);
        setMaterialReferenceByPath(instance, materialSourcePath.toQString());
    }

    void setMaterialReferenceByPath(TInstanceHandle instance, const QString &path) override
    {
        Qt3DSDMInstanceHandle material = getOrCreateMaterial(path);
        IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
        SObjectRefType objRef = objRefHelper->GetAssetRefValue(material, m_Doc.GetSceneInstance(),
                                                               CRelativePathTools::EPATHTYPE_GUID);
        SetInstancePropertyValue(instance,
                                 m_Bridge.GetObjectDefinitions().m_ReferencedMaterial
                                 .m_ReferencedMaterial.m_Property,
                                 objRef, false);
        setReferencedMaterialNameByPath(instance, path);
    }


    void setReferencedMaterialNameByPath(TInstanceHandle instance, const QString &path)
    {
        auto name = getMaterialNameFromFilePath(path);
        int slashIndex = name.lastIndexOf(QLatin1Char('/'));
        if (slashIndex != -1)
            name = name.mid(slashIndex + 1);
        SetName(instance, Q3DStudio::CString::fromQString(name));
    }

    void setMaterialNameByPath(TInstanceHandle instance, const QString &path) override
    {
        SetName(instance, Q3DStudio::CString::fromQString(getMaterialNameFromFilePath(path)));
    }

    void setMaterialSourcePath(TInstanceHandle instance,
                               const Q3DStudio::CString &materialSourcePath) override
    {
        SetInstancePropertyValue(instance, m_Bridge.GetSceneAsset().m_SourcePath,
                                 std::make_shared<CDataStr>(materialSourcePath));
    }

    void setMaterialValues(const QString &path,
                           const QMap<QString, QString> &values,
                           const QMap<QString, QMap<QString, QString>> &textureValues) override
    {
        auto instance = getOrCreateMaterial(path, false);
        if (instance.Valid())
            setMaterialValues(instance, values, textureValues);
    }

    struct ChildInstance
    {
        QString name;
        TInstanceHandle handle;
    };

    void setInstanceValueIfChanged(TInstanceHandle instance, TPropertyHandle prop, SValue value)
    {
        SValue oldValue;
        m_PropertySystem.GetInstancePropertyValue(instance, prop, oldValue);
        if (oldValue != value)
            SetInstancePropertyValue(instance, prop, value);
    }

    QVector<ChildInstance> setPropertyValues(TInstanceHandle instance,
                                             const QMap<QString, QString> &values)
    {
        QVector<ChildInstance> childInstances;
        QMapIterator<QString, QString> i(values);
        while (i.hasNext()) {
            i.next();

            if (!isSaveableMaterialProperty(i.key()))
                continue;

            TCharStr propName(i.key().toStdWString().c_str());
            Q3DStudio::CString propString = Q3DStudio::CString::fromQString(i.value());
            Qt3DSDMPropertyHandle prop
                    = m_PropertySystem.GetAggregateInstancePropertyByName(
                        instance, propName);

            if (m_AnimationSystem.IsPropertyAnimated(instance, prop))
                continue;

            const auto type = m_PropertySystem.GetDataType(prop);
            switch (type) {
            case DataModelDataType::Long4:
            {
                setInstanceValueIfChanged(instance, prop, std::make_shared<CDataStr>(propString));
                SValue value;
                m_PropertySystem.GetInstancePropertyValue(instance, prop, value);
                if (!value.empty()) {
                    if (value.getType() == DataModelDataType::Long4) {
                        SLong4 guid = get<qt3dsdm::SLong4>(value);
                        auto childInstance = m_Bridge.GetInstanceByGUID(guid);
                        if (childInstance.Valid())
                            childInstances.push_back({i.key(), childInstance});
                    }
                }
                break;
            }
            case DataModelDataType::Float:
            {
                setInstanceValueIfChanged(instance, prop, i.value().toFloat());
                break;
            }
            case DataModelDataType::Float2:
            {
                QStringList floats = i.value().split(QLatin1Char(' '));
                if (floats.length() == 2) {
                    SFloat2 value(floats[0].toFloat(), floats[1].toFloat());
                    setInstanceValueIfChanged(instance, prop, value);
                }
                break;
            }
            case DataModelDataType::Float3:
            {
                QStringList floats = i.value().split(QLatin1Char(' '));
                if (floats.length() == 3) {
                    SFloat3 value(floats[0].toFloat(), floats[1].toFloat(), floats[2].toFloat());
                    setInstanceValueIfChanged(instance, prop, value);
                }
                break;
            }
            case DataModelDataType::Float4:
            {
                QStringList floats = i.value().split(QLatin1Char(' '));
                if (floats.length() == 4) {
                    SFloat4 value(floats[0].toFloat(), floats[1].toFloat(), floats[2].toFloat(),
                                  floats[3].toFloat());
                    setInstanceValueIfChanged(instance, prop, value);
                }
                break;
            }
            case DataModelDataType::Bool:
            {
                if (propString == "True")
                    setInstanceValueIfChanged(instance, prop, true);
                else if (propString == "False")
                    setInstanceValueIfChanged(instance, prop, false);
                break;
            }
            case DataModelDataType::String:
            {
                setInstanceValueIfChanged(instance, prop, std::make_shared<CDataStr>(propString));
                break;
            }
            default:
                break;
            }
        }
        return childInstances;
    }

    void setMaterialValues(TInstanceHandle instance, const QMap<QString, QString> &values,
                           const QMap<QString, QMap<QString, QString>> &textureValues) override
    {
        if (values.contains(QStringLiteral("type"))) {
            if (values[QStringLiteral("type")] == QLatin1String("CustomMaterial")
                    && values.contains(QStringLiteral("sourcepath"))) {
                SetMaterialType(instance, values[QStringLiteral("sourcepath")]);
                if (values.contains(QStringLiteral("name"))) {
                    SetName(instance, Q3DStudio::CString::fromQString(
                                values[QStringLiteral("name")]));
                }
            }
        }

        const auto childInstances = setPropertyValues(instance, values);

        for (auto &child : childInstances) {
            if (textureValues.contains(child.name))
                setPropertyValues(child.handle, textureValues[child.name]);
        }

        m_Doc.GetCore()->GetDispatch()->FireImmediateRefreshInstance(instance);
    }

    void SetSlideName(TInstanceHandle inSlideInstance, TPropertyHandle propName,
                              const wchar_t *inOldName, const wchar_t *inNewName) override
    {
        SValue theOldValue = std::make_shared<CDataStr>(inOldName);
        SValue theNewValue = std::make_shared<CDataStr>(inNewName);

        // Update the slide name property value
        IPropertySystem &thePropertySystem(m_PropertySystem);
        thePropertySystem.SetInstancePropertyValue(inSlideInstance, propName, theNewValue);

        // Find all actions that point to the old slide name, and change it to new name
        // First, we need to get the owning component instance, for example inSlideInstance is owned
        // by Scene
        Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetSlideByInstance(inSlideInstance);
        if (theSlide.Valid() == false) {
            assert(0);
            return;
        }
        Qt3DSDMInstanceHandle theComponentInstance = m_Bridge.GetOwningComponentInstance(theSlide);
        if (theComponentInstance.Valid() == false) {
            assert(0);
            return;
        }

        // Next, get list of all actions
        TActionHandleList theActions;
        m_ActionCore.GetActions(theActions);
        for (TActionHandleList::iterator theIter = theActions.begin(); theIter != theActions.end();
             ++theIter) {
            // Check if the action target object is the owning component instance, for example if
            // the target object is the Scene
            SActionInfo theActionInfo = m_ActionCore.GetActionInfo(*theIter);
            Qt3DSDMInstanceHandle theTargetInstance =
                m_Bridge.GetInstance(theActionInfo.m_Owner, theActionInfo.m_TargetObject);
            if (theTargetInstance == theComponentInstance) {
                Qt3DSDMHandlerHandle theHandler = m_Bridge.ResolveHandler(theActionInfo);
                if (theHandler.Valid()) {
                    for (THandlerArgHandleList::const_iterator theArgHandle =
                             theActionInfo.m_HandlerArgs.begin();
                         theArgHandle != theActionInfo.m_HandlerArgs.end(); ++theArgHandle) {
                        // and check if handler is Slide type (for example "Go to Slide") that
                        // points to the old slide
                        const SHandlerArgumentInfo &theArgumentInfo =
                            m_ActionCore.GetHandlerArgumentInfo(*theArgHandle);
                        Option<SMetaDataHandlerArgumentInfo> theArgMetaData(
                            m_MetaData.FindHandlerArgumentByName(theHandler,
                                                                 theArgumentInfo.m_Name));
                        if (theArgMetaData.hasValue()
                            && theArgMetaData->m_ArgType == HandlerArgumentType::Slide) {
                            SValue theHandlerValue;
                            m_ActionCore.GetHandlerArgumentValue(*theArgHandle, theHandlerValue);
                            if (Equals(theHandlerValue, theOldValue))
                                // Update action handler argument to point to new slide name
                                m_ActionCore.SetHandlerArgumentValue(*theArgHandle, theNewValue);
                        }
                    }
                }
            }
        }
    }

    void copyMaterialProperties(Qt3DSDMInstanceHandle src, Qt3DSDMInstanceHandle dst) override
    {
        const EStudioObjectType matType = m_Bridge.GetObjectType(src);
        QString materialTypeString;
        if (matType == OBJTYPE_CUSTOMMATERIAL)
            materialTypeString = m_Bridge.GetSourcePath(src);
        else if (matType == OBJTYPE_MATERIAL)
            materialTypeString = QStringLiteral("Standard Material");
        else
            return;
        SetMaterialType(dst, materialTypeString);

        const auto srcSlide = m_SlideSystem.GetApplicableSlide(src);
        const auto dstSlide = m_SlideSystem.GetApplicableSlide(dst);
        const auto name = GetName(dst);
        SValue value;
        qt3dsdm::TPropertyHandleList propList;
        m_PropertySystem.GetAggregateInstanceProperties(src, propList);

        for (auto &prop : propList) {
            const auto name = QString::fromWCharArray(m_PropertySystem.GetName(prop).wide_str());

            if (!isSaveableMaterialProperty(name))
                continue;

            TInstanceHandle srcChild;
            m_PropertySystem.GetInstancePropertyValue(src, prop, value);
            if (!value.empty() && value.getType() == DataModelDataType::Long4) {
                SLong4 guid = get<qt3dsdm::SLong4>(value);
                if (guid.Valid()) {
                    srcChild = m_Bridge.GetInstanceByGUID(guid);
                    const auto path = std::make_shared<CDataStr>(CString::fromQString(
                                                                 m_Bridge.GetSourcePath(srcChild)));
                    SetInstancePropertyValue(dst, prop, path);
                }
            } else {
                m_PropertySystem.SetInstancePropertyValue(dst, prop, value);
            }

            TInstanceHandle dstChild;
            m_PropertySystem.GetInstancePropertyValue(dst, prop, value);
            if (!value.empty() && value.getType() == DataModelDataType::Long4) {
                SLong4 guid = get<qt3dsdm::SLong4>(value);
                if (guid.Valid())
                    dstChild = m_Bridge.GetInstanceByGUID(guid);
            }

            if (srcChild.Valid() && dstChild.Valid())
                CopyProperties(srcSlide, srcChild, dstSlide, dstChild);
        }

        SetName(dst, name);
    }

    void CopyProperties(TSlideHandle inSourceSlide, TInstanceHandle inSourceInstance,
                        TSlideHandle inDestSlide, TInstanceHandle inDestInstance)
    {
        m_SlideCore.CopyProperties(inSourceSlide, inSourceInstance, inDestSlide, inDestInstance);
        m_animCore.CopyAnimations(inSourceSlide, inSourceInstance, inDestSlide, inDestInstance);
    }

    void UnlinkProperty(TInstanceHandle instance, TPropertyHandle propName) override
    {
        IPropertySystem &thePropertySystem(m_PropertySystem);
        AdditionalMetaDataType::Value thePropertyMetaData =
            thePropertySystem.GetAdditionalMetaDataType(instance, propName);
        Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(instance);
        SValue theValue;
        if (thePropertyMetaData == AdditionalMetaDataType::Image) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance;
            if (m_SlideCore.GetSpecificInstancePropertyValue(theAssociatedSlide, instance, propName,
                                                             theValue)) {
                SLong4 theGuid(get<SLong4>(theValue));
                theInstance = m_Bridge.GetInstanceByGUID(theGuid);
            }
            if (theInstance.Valid() == false)
                theInstance = CreateImageInstanceForMaterialOrLayer(instance, propName);

            if (theInstance)
                UnlinkProperty(theInstance, m_Bridge.GetSourcePathProperty());
        } else {
            // Note that we get the value *before* unlinking.
            m_SlideSystem.UnlinkProperty(instance, propName);
            // WE ignore mesh and import properties because for mesh properties, regardless of link
            // or unlink status, the materials need to stay in the associated slide of the model.
            // for imported hierarchies, the operation of recursively going down the tree and
            // manually
            // setting up a new import hierarchy is too tedious; the artist can just re-import
            // the data.
        }
    }
    void LinkProperty(TInstanceHandle instance, TPropertyHandle propName) override
    {
        IPropertySystem &thePropertySystem(m_PropertySystem);
        AdditionalMetaDataType::Value thePropertyMetaData =
            thePropertySystem.GetAdditionalMetaDataType(instance, propName);
        Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(instance);
        SValue theValue;
        if (thePropertyMetaData == AdditionalMetaDataType::Image
            && m_SlideCore.GetSpecificInstancePropertyValue(theAssociatedSlide, instance, propName,
                                                            theValue)) {
            SLong4 theGuid(get<SLong4>(theValue));
            qt3dsdm::Qt3DSDMInstanceHandle theInstance = m_Bridge.GetInstanceByGUID(theGuid);
            if (theInstance) {
                LinkProperty(theInstance, m_Bridge.GetSourcePathProperty());
                // If the instance has no source path property, then we get rid of it automatically.
                m_SlideCore.GetSpecificInstancePropertyValue(
                    theAssociatedSlide, theInstance, m_Bridge.GetSourcePathProperty(), theValue);
                qt3dsdm::TDataStrPtr theSourcePath(get<TDataStrPtr>(theValue));
                if (!theSourcePath || theSourcePath->GetLength() == 0) {
                    DeleteImageInstanceFromMaterialOrLayer(instance, propName);
                }
            }
        } else {
            if (thePropertyMetaData == AdditionalMetaDataType::Import) {
                TSlideHandleList theChildren;
                m_SlideCore.GetChildSlides(theAssociatedSlide, theChildren);
                for (size_t idx = 0, end = theChildren.size(); idx < end; ++idx) {
                    Qt3DSDMSlideHandle theChildSlide(theChildren[idx]);
                    for (long childIdx = 0; childIdx < m_AssetGraph.GetChildCount(instance);
                         ++childIdx) {
                        TInstanceHandle theChild(m_AssetGraph.GetChild(instance, childIdx));
                        if (GetAssociatedSlide(theChild) == theChildSlide && IsImported(theChild)) {
                            DeleteInstance(theChild);
                            --childIdx;
                        }
                    }
                }
            }
            m_SlideSystem.LinkProperty(instance, propName);
        }
    }

    void SetTimeRange(TInstanceHandle inInstance, long inStart, long inEnd) override
    {
        SetStartTime(inInstance, inStart);
        SetEndTime(inInstance, inEnd);
    }

    void SetTimeRangeInSlide(TSlideHandle inSlide, TInstanceHandle inInstance, long inStart,
                                     long inEnd) override
    {
        m_SlideCore.ForceSetInstancePropertyValue(inSlide, inInstance,
                                                  m_Bridge.GetSceneAsset().m_StartTime,
                                                  static_cast<qt3ds::QT3DSI32>(inStart));
        m_SlideCore.ForceSetInstancePropertyValue(inSlide, inInstance,
                                                  m_Bridge.GetSceneAsset().m_EndTime,
                                                  static_cast<qt3ds::QT3DSI32>(inEnd));
    }

    void SetStartTime(TInstanceHandle inInstance, long inStart) override
    {
        m_PropertySystem.SetInstancePropertyValue(inInstance, m_Bridge.GetSceneAsset().m_StartTime,
                                                  static_cast<qt3ds::QT3DSI32>(inStart));
    }

    void SetEndTime(TInstanceHandle inInstance, long inEnd) override
    {
        m_PropertySystem.SetInstancePropertyValue(inInstance, m_Bridge.GetSceneAsset().m_EndTime,
                                                  static_cast<qt3ds::QT3DSI32>(inEnd));
    }

    bool IsAssetNotInActiveSlide(Q3DStudio::TIdentifier inIdentifier)
    {
        Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(inIdentifier);
        Qt3DSDMSlideHandle theActiveSlide = m_Doc.GetActiveSlide();
        // return true to filter the object, apparently.
        bool isInCurrentSlide =
            theSlide == theActiveSlide || theSlide == m_SlideSystem.GetMasterSlide(theActiveSlide);
        return !isInCurrentSlide;
    }
    // The original implementation of this function is absolutely not correct
    // in all cases.
    void GetAssetChildrenInActiveSlide(Qt3DSDMInstanceHandle inInstance, CGraphIterator &outIterator)
    {
        outIterator +=
            Q3DStudio::TFilter(std::bind(&CDocEditor::IsAssetNotInActiveSlide, this,
                                         std::placeholders::_1));
        m_AssetGraph.GetChildren(outIterator, inInstance);
    }

    void ResizeTimeRange(TInstanceHandle inInstance, long inTime, bool inSetStart) override
    {
        // Get the current time range
        std::pair<long, long> theTimeRange = GetTimeRange(inInstance);

        // Change the start time or end time
        if (inSetStart) {
            // Never let the start time get less than 0
            if (inTime < 0)
                inTime = 0;
            // Never let the start time get more than the end time
            else if (inTime > theTimeRange.second)
                inTime = theTimeRange.second;
            // Set start time
            SetStartTime(inInstance, inTime);
        } else {
            // Never let the end time get less than the start time
            if (inTime < theTimeRange.first)
                inTime = theTimeRange.first;
            // Set end time
            SetEndTime(inInstance, inTime);
        }

        // Iterate children and see if we need to resize children time as well
        CGraphIterator theChildren;
        GetAssetChildrenInActiveSlide(inInstance, theChildren);
        for (; !theChildren.IsDone(); ++theChildren) {
            TInstanceHandle theChild = theChildren.GetCurrent();
            // Do not adjust locked children
            SValue locked;
            m_PropertySystem.GetInstancePropertyValue(
                        theChild, m_Bridge.GetSceneAsset().m_Locked, locked);
            if (!qt3dsdm::get<bool>(locked)) {
                std::pair<long, long> theChildTimeRange = GetTimeRange(theChild);
                if (inSetStart) {
                    // If we are resizing start time, if child's start time == parent's child time
                    // then we need to resize child as well
                    if (theChildTimeRange.first == theTimeRange.first)
                        ResizeTimeRange(theChild, inTime, inSetStart);
                } else {
                    // If we are resizing end time, if child's end time == parent's end time
                    // then we need to resize child as well
                    if (theChildTimeRange.second == theTimeRange.second)
                        ResizeTimeRange(theChild, inTime, inSetStart);
                }
            }
        }
    }

    void OffsetTimeRange(TInstanceHandle inInstance, long inOffset) override
    {
        // Get the current time range
        std::pair<long, long> theTimeRange = GetTimeRange(inInstance);

        // Do not allow the object to go into negative time
        if (inOffset < 0 && (-inOffset) > theTimeRange.first) {
            inOffset = -theTimeRange.first;
        }
        SetTimeRange(inInstance, theTimeRange.first + inOffset, theTimeRange.second + inOffset);
        // Offset all the keyframes linked to animations of this instance by this offset.
        m_animCore.OffsetAnimations(m_Doc.GetActiveSlide(), inInstance, inOffset);

        // Offset children time as well
        CGraphIterator theChildren;
        GetAssetChildrenInActiveSlide(inInstance, theChildren);
        for (; !theChildren.IsDone(); ++theChildren) {
            TInstanceHandle theChild = theChildren.GetCurrent();
            // Do not adjust locked children
            SValue locked;
            m_PropertySystem.GetInstancePropertyValue(
                        theChild, m_Bridge.GetSceneAsset().m_Locked, locked);
            if (!qt3dsdm::get<bool>(locked))
                OffsetTimeRange(theChild, inOffset);
        }
    }

    void TruncateTimeRange(TInstanceHandle inInstance, bool inSetStart, long inTime) override
    {
        if (m_Bridge.IsMaterialInstance(inInstance) || m_Bridge.IsImageInstance(inInstance))
            return; // bail!

        // Set the time range if the instance is not the current Component or Scene
        if (inInstance != m_Doc.GetActiveRootInstance()) {
            std::pair<long, long> theRange(GetTimeRange(inInstance));
            if (inSetStart)
                theRange.first = inTime;
            else
                theRange.second = inTime;

            // Ensure the time range is valid before going further.
            if (theRange.first <= theRange.second)
                SetTimeRange(inInstance, theRange.first, theRange.second);
        }

        CGraphIterator theChildren;
        GetAssetChildrenInActiveSlide(inInstance, theChildren);
        for (; !theChildren.IsDone(); ++theChildren) {
            TInstanceHandle theChild = theChildren.GetCurrent();
            // Do not adjust locked children
            SValue locked;
            m_PropertySystem.GetInstancePropertyValue(
                        theChild, m_Bridge.GetSceneAsset().m_Locked, locked);
            if (!qt3dsdm::get<bool>(locked))
                TruncateTimeRange(theChild, inSetStart, inTime);
        }
    }

    void SetTimebarColor(TInstanceHandle inInstance, ::CColor inColor) override
    {
        m_PropertySystem.SetInstancePropertyValue(
            inInstance, m_Bridge.GetSceneAsset().m_TimebarColor,
            qt3dsdm::SFloat4(inColor.GetRed() / 255.0f, inColor.GetGreen() / 255.0f,
                             inColor.GetBlue() / 255.0f, 1.0f));
    }

    void SetTimebarText(TInstanceHandle inInstance, const QString &inComment) override
    {
        m_PropertySystem.SetInstancePropertyValue(inInstance,
                                                  m_Bridge.GetSceneAsset().m_TimebarText,
                                                  QVariant(inComment));
    }

    void AddChild(Qt3DSDMInstanceHandle parent, Qt3DSDMInstanceHandle child,
                          TInstanceHandle inNextSibling) override
    {
        TInstanceHandle currentParent = m_AssetGraph.GetParent(child);
        if (currentParent.Valid() == false)
            m_AssetGraph.AddChild(parent, child);
        if (inNextSibling.Valid())
            m_AssetGraph.MoveBefore(child, inNextSibling);
        else
            m_AssetGraph.MoveTo(child, parent, COpaquePosition::LAST);
    }
    void RemoveChild(Qt3DSDMInstanceHandle parent, Qt3DSDMInstanceHandle child) override
    {
        if (m_AssetGraph.GetParent(child) == parent) {
            m_AssetGraph.RemoveChild(child, false);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    template <typename TKeyframeType>
    void AddKeyframes(Qt3DSDMAnimationHandle animHandle, const float *keyframeValues,
                      long numValues, long timeOffset)
    {
        long numFloatsPerKeyframe = sizeof(TKeyframeType) / sizeof(float);
        if (numValues % numFloatsPerKeyframe) {
            QT3DS_ASSERT(false);
        }
        const TKeyframeType *keyframes = reinterpret_cast<const TKeyframeType *>(keyframeValues);
        long numKeyframes = numValues / numFloatsPerKeyframe;
        for (long idx = 0; idx < numKeyframes; ++idx) {
            TKeyframeType theData(keyframes[idx]);
            theData.m_time += timeOffset;
            m_animCore.InsertKeyframe(animHandle, theData);
        }
    }

    void SetKeyframeTime(TKeyframeHandle kfHandle, long time) override
    {
        TKeyframe kfData = m_animCore.GetKeyframeData(kfHandle);

        // offset control points for bezier keyframes
        offsetBezier(kfData, time - getKeyframeTime(kfData));

        // Functional programming paradigm, returns new value instead of changing current value.
        kfData = qt3dsdm::setKeyframeTime(kfData, time);
        m_animCore.SetKeyframeData(kfHandle, kfData);

        // For bezier keyframes check that control points of the moved keyframe and the keyframes
        // before and after it don't go beyond its adjacent keyframes times.
        Qt3DSDMAnimationHandle anim = m_animCore.GetAnimationForKeyframe(kfHandle);
        EAnimationType animType = m_animCore.GetAnimationInfo(anim).m_AnimationType;
        if (animType == EAnimationTypeBezier) {
            TKeyframeHandleList keyframes;
            m_animCore.GetKeyframes(anim, keyframes);
            for (size_t i = 0; i < keyframes.size(); ++i) {
                if (keyframes[i] == kfHandle) {
                    SBezierKeyframe kfCurr = get<SBezierKeyframe>(m_animCore.GetKeyframeData(
                                                                      kfHandle));
                    if (i > 0) { // check overlap with prev keyframe
                        SBezierKeyframe kfPrev = get<SBezierKeyframe>(m_animCore.GetKeyframeData(
                                                                          keyframes[i - 1]));
                        if (kfPrev.m_OutTangentTime > kfCurr.m_time)
                            kfPrev.m_OutTangentTime = kfCurr.m_time;

                        if (kfCurr.m_InTangentTime < kfPrev.m_time)
                            kfCurr.m_InTangentTime = kfPrev.m_time;

                        m_animCore.SetKeyframeData(keyframes[i - 1], kfPrev);

                        // Mahmoud_TODO: trim the value proportinally also so that the control
                        // point lines maintains the same slope after the time is trimmed
                    }

                    if (i < keyframes.size() - 1) { // check overlap with next keyframe
                        SBezierKeyframe kfNext = get<SBezierKeyframe>(m_animCore.GetKeyframeData(
                                                                          keyframes[i + 1]));
                        if (kfNext.m_InTangentTime < kfCurr.m_time)
                            kfNext.m_InTangentTime = kfCurr.m_time;

                        if (kfCurr.m_OutTangentTime > kfNext.m_time)
                            kfCurr.m_OutTangentTime = kfNext.m_time;

                        m_animCore.SetKeyframeData(keyframes[i + 1], kfNext);
                    }
                    m_animCore.SetKeyframeData(kfHandle, kfCurr);
                    break;
                }
            }
        }
    }

    void setBezierKeyframeValue(TKeyframeHandle kfHandle, const TKeyframe &kfData) override
    {
        m_animCore.SetKeyframeData(kfHandle, kfData);
    }

    void DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation) override
    {
        m_animCore.DeleteAllKeyframes(inAnimation);
    }

    void KeyframeProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                  bool inDoDiffValue) override
    {
        m_AnimationSystem.KeyframeProperty(inInstance, inProperty, inDoDiffValue);
    }

    virtual Qt3DSDMAnimationHandle
    CreateOrSetAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle instance,
                         const wchar_t *propName, long subIndex, EAnimationType animType,
                         const float *keyframeValues, long numValues, bool /*inUserEdited*/) override
    {
        Qt3DSDMPropertyHandle property = m_DataCore.GetAggregateInstancePropertyByName(instance,
                                                                                       propName);
        if (!property.Valid()) {
            QT3DS_ASSERT(false);
            return 0;
        }
        if (!inSlide.Valid()) {
            Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(instance);
            if (!theSlide.Valid()) {
                assert(0);
                return 0;
            }
            if (m_SlideSystem.IsPropertyLinked(instance, property))
                theSlide = m_SlideSystem.GetMasterSlide(theSlide);
            inSlide = theSlide;
        }

        Qt3DSDMAnimationHandle animHandle =
            m_animCore.GetAnimation(inSlide, instance, property, subIndex);

        if (animHandle.Valid())
            m_animCore.DeleteAnimation(animHandle);

        animHandle =
            m_animCore.CreateAnimation(inSlide, instance, property, subIndex, animType, false);

        long theStartTime = GetTimeRange(instance).first;

        switch (animType) {
        case EAnimationTypeLinear:
            AddKeyframes<SLinearKeyframe>(animHandle, keyframeValues, numValues, theStartTime);
            break;
        case EAnimationTypeBezier:
            AddKeyframes<SBezierKeyframe>(animHandle, keyframeValues, numValues, theStartTime);
            break;
        case EAnimationTypeEaseInOut:
            AddKeyframes<SEaseInEaseOutKeyframe>(animHandle, keyframeValues, numValues,
                                                 theStartTime);
            break;
        default:
            QT3DS_ASSERT(false);
            AddKeyframes<SLinearKeyframe>(animHandle, keyframeValues, numValues, theStartTime);
            break;
        }
        return animHandle;
    }
    bool RemoveAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle instance,
                                 const wchar_t *propName, long subIndex) override
    {
        Qt3DSDMPropertyHandle propHdl =
            m_DataCore.GetAggregateInstancePropertyByName(instance, propName);
        if (propHdl.Valid() == false) {
            QT3DS_ASSERT(false);
            return false;
        }
        Qt3DSDMAnimationHandle animHandle =
            m_animCore.GetAnimation(inSlide, instance, propHdl, subIndex);
        if (animHandle.Valid()) {
            m_animCore.DeleteAnimation(animHandle);
            return true;
        }
        return false;
    }

    void SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited = true) override
    {
        m_animCore.SetIsArtistEdited(inAnimation, inEdited);
    }

    qt3dsdm::Qt3DSDMInstanceHandle
    FinalizeAddOrDrop(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::Qt3DSDMInstanceHandle inParent,
                      DocumentEditorInsertType::Enum inInsertType, const CPt &inPosition,
                      bool inSetTimeRangeToParent, bool inSelectInstanceWhenFinished = true,
                      bool checkUniqueName = true, bool notifyRename = true)
    {
        if (inPosition.x != 0 && inPosition.y != 0) {
            Q3DStudio::IDocSceneGraph *theGraph(m_Doc.GetSceneGraph());
            QT3DSVec3 thePos(0, 0, 0);
            if (theGraph) {
                thePos = theGraph->GetIntendedPosition(inInstance, inPosition);
                SetPosition(inInstance, SFloat3(thePos.x, thePos.y, thePos.z));
            } else {
                QT3DS_ASSERT(false);
            }
        }
        RearrangeObject(inInstance, inParent, inInsertType, checkUniqueName, notifyRename);
        if (inSetTimeRangeToParent)
            SetTimeRangeToParent(inInstance);
        if (inSelectInstanceWhenFinished)
            m_Doc.SelectDataModelObject(inInstance);
        return inInstance;
    }

    CString GetName(Qt3DSDMInstanceHandle inInstance) const override
    {
        Option<SValue> theValue = GetInstancePropertyValue(
            inInstance, m_Bridge.GetObjectDefinitions().m_Named.m_NameProp);
        if (theValue.hasValue()) {
            TDataStrPtr theNamePtr(get<TDataStrPtr>(*theValue));
            if (theNamePtr)
                return theNamePtr->GetData();
        }
        return L"";
    }

    CString GetSourcePath(Qt3DSDMInstanceHandle inInstance) const override
    {
        Option<SValue> theValue = GetInstancePropertyValue(
            inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath);
        if (theValue.hasValue()) {
            TDataStrPtr theNamePtr(get<TDataStrPtr>(*theValue));
            if (theNamePtr)
                return theNamePtr->GetData();
        }
        return L"";
    }

    TInstanceHandle GetFirstBaseClass(Qt3DSDMInstanceHandle inInstance) const override
    {
        TInstanceHandleList theList;
        m_DataCore.GetInstanceParents(inInstance, theList);
        if (theList.size())
            return theList[0];
        return 0;
    }

    void SetName(Qt3DSDMInstanceHandle inInstance, const CString &inName,
                         bool inMakeUnique = false) override
    {
        CString theUniqueName = inName;
        if (inMakeUnique)
            theUniqueName = m_Bridge.GetUniqueChildName(GetParent(inInstance), inInstance, inName);

        SetInstancePropertyValue(inInstance, m_Bridge.GetNameProperty(),
                                 std::make_shared<CDataStr>(theUniqueName.c_str()), false);
    }

    TInstanceHandleList DoPasteSceneGraphObject(std::shared_ptr<IDOMReader> inReader,
                                                TInstanceHandle inNewRoot,
                                                bool inGenerateUniqueName,
                                                DocumentEditorInsertType::Enum inInsertType,
                                                const CPt &inPosition,
                                                bool preserveFileIds,
                                                bool notifyRename = true)
    {
        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        TInstanceHandleList retval = theSerializer->SerializeSceneGraphObject(
            *inReader, m_Doc.GetDocumentDirectory(), inNewRoot, GetActiveSlide(inNewRoot),
            preserveFileIds);
        for (size_t idx = 0, end = retval.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(retval[idx]);
            if (inInsertType == DocumentEditorInsertType::NextSibling)
                theInstance = retval[end - idx - 1];

            FinalizeAddOrDrop(theInstance, inNewRoot, inInsertType, inPosition, false, true, true,
                              notifyRename);

            SetName(theInstance, GetName(theInstance), inGenerateUniqueName);
        }

        return retval;
    }

    TInstanceHandleList PasteSceneGraphObject(const CFilePath &inFilePath,
                                              TInstanceHandle inNewRoot,
                                              bool inGenerateUniqueName,
                                              DocumentEditorInsertType::Enum inInsertType,
                                              const CPt &inPosition,
                                              bool preserveFileIds) override
    {
        qt3ds::QT3DSI32 theVersion = 0;
        std::shared_ptr<IDOMReader> theReader = m_Doc.CreateDOMReader(
            inFilePath.toCString(), theVersion);
        if (!theReader)
            return TInstanceHandleList();
        return DoPasteSceneGraphObject(theReader, inNewRoot, inGenerateUniqueName, inInsertType,
                                       inPosition, preserveFileIds, false);
    }

    virtual TInstanceHandleList
    PasteSceneGraphObjectMaster(const CFilePath &inFilePath, TInstanceHandle inNewRoot,
                                bool inGenerateUniqueName,
                                DocumentEditorInsertType::Enum inInsertType, const CPt &inPosition,
                                bool preserveFileIds) override
    {
        qt3ds::QT3DSI32 theVersion = 0;
        std::shared_ptr<IDOMReader> theReader = m_Doc.CreateDOMReader(
            inFilePath.toCString(), theVersion);
        if (!theReader)
            return TInstanceHandleList();

        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        TInstanceHandleList retval = theSerializer->SerializeSceneGraphObject(
            *theReader, m_Doc.GetDocumentDirectory(), inNewRoot,
            m_Doc.GetStudioSystem()->GetSlideSystem()->GetMasterSlide(GetActiveSlide(inNewRoot)),
            preserveFileIds);
        for (size_t idx = 0, end = retval.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(retval[idx]);
            if (inInsertType == DocumentEditorInsertType::NextSibling)
                theInstance = retval[end - idx - 1];

            FinalizeAddOrDrop(theInstance, inNewRoot, inInsertType, inPosition, false, true,
                              true, false);

            SetName(theInstance, GetName(theInstance), inGenerateUniqueName);
        }

        return retval;
    }

    SFloat3 GetPosition(Qt3DSDMInstanceHandle inInstance)
    {
        Option<SValue> theValue =
            GetInstancePropertyValue(inInstance, m_Bridge.GetObjectDefinitions().m_Node.m_Position);
        if (theValue.hasValue())
            return get<SFloat3>(*theValue);
        return SFloat3();
    }

    void SetPosition(Qt3DSDMInstanceHandle inInstance, const SFloat3 &inPos)
    {
        SetInstancePropertyValue(inInstance, m_Bridge.GetObjectDefinitions().m_Node.m_Position,
                                 inPos, false);
    }

    QT3DSU32 BuildGraphOrderItem(qt3dsdm::Qt3DSDMInstanceHandle inInstance, QT3DSU32 inCurrentIndex)
    {
        m_GraphOrderMap.insert(std::make_pair(inInstance.GetHandleValue(), inCurrentIndex));
        ++inCurrentIndex;
        for (long childIdx = 0, childEnd = m_AssetGraph.GetChildCount(inInstance);
             childIdx < childEnd; ++childIdx) {
            inCurrentIndex =
                BuildGraphOrderItem(m_AssetGraph.GetChild(inInstance, childIdx), inCurrentIndex);
        }
        return inCurrentIndex;
    }

    QT3DSU32 GetInstanceGraphOrder(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        if (m_GraphOrderMap.size() == 0) {
            BuildGraphOrderItem(m_AssetGraph.GetRoot(0), 0);
        }
        std::unordered_map<long, QT3DSU32>::iterator iter = m_GraphOrderMap.find(inInstance);
        if (iter != m_GraphOrderMap.end())
            return iter->second;
        return QT3DS_MAX_U32;
    }

    bool GraphOrderLessThan(qt3dsdm::Qt3DSDMInstanceHandle lhs, qt3dsdm::Qt3DSDMInstanceHandle rhs)
    {
        return GetInstanceGraphOrder(lhs) < GetInstanceGraphOrder(rhs);
    }

    qt3dsdm::TInstanceHandleList ToGraphOrdering(const qt3dsdm::TInstanceHandleList &inInstances)
    {
        qt3dsdm::TInstanceHandleList sortableList(inInstances);
        m_GraphOrderMap.clear();
        std::sort(sortableList.begin(), sortableList.end(),
                  std::bind(&CDocEditor::GraphOrderLessThan, this, std::placeholders::_1,
                            std::placeholders::_2));
        return sortableList;
    }

    void RearrangeObjects(const qt3dsdm::TInstanceHandleList &inInstances,
                          TInstanceHandle inDest,
                          DocumentEditorInsertType::Enum inInsertType,
                          bool checkUniqueName, bool notifyRename = true) override
    {
        qt3dsdm::TInstanceHandleList sortableList(ToGraphOrdering(inInstances));
        TInstanceHandle theParent(inDest);
        if (inInsertType == DocumentEditorInsertType::PreviousSibling
            || inInsertType == DocumentEditorInsertType::NextSibling)
            theParent = GetParent(inDest);

        // Moving object into non-root component
        if (m_Bridge.IsComponentInstance(theParent)
                && !m_Bridge.IsActiveComponent(theParent)
                && moveIntoComponent(inInstances, theParent, checkUniqueName, notifyRename)) {
            return;
        }

        for (size_t idx = 0, end = sortableList.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(sortableList[idx]);
            // If the insert type is next sibling, we have to reverse the list
            // in order to respect the ordering.
            if (inInsertType == DocumentEditorInsertType::NextSibling)
                theInstance = sortableList[end - idx - 1];
            // Rename if the new parent already has object with a same name
            CString currName = m_Bridge.GetName(theInstance);
            if (checkUniqueName) {
                if (!m_Bridge.CheckNameUnique(theParent, theInstance, currName)) {
                    CString newName = m_Bridge.GetUniqueChildName(theParent, theInstance,
                                                                  currName);
                    if (notifyRename) {
                        m_Doc.getMoveRenameHandler()->displayMessageBox(currName.toQString(),
                                                                        newName.toQString());
                    }
                    SetName(theInstance, newName);
                }
            }
            if (inInsertType == DocumentEditorInsertType::PreviousSibling)
                m_AssetGraph.MoveBefore(theInstance, inDest);
            else if (inInsertType == DocumentEditorInsertType::NextSibling)
                m_AssetGraph.MoveAfter(theInstance, inDest);
            else if (inInsertType == DocumentEditorInsertType::LastChild)
                m_AssetGraph.MoveTo(theInstance, inDest, COpaquePosition::LAST);
        }
    }

    // Move all children out of a given parent instances and delete the instances.
    // Typically the parent instances are groups as the function name implies.
    void ungroupObjects(const TInstanceHandleList &inInstances) override
    {
        for (size_t idx = 0, end = inInstances.size(); idx < end; ++idx) {
            TInstanceHandle selected = inInstances[idx];
            if (selected.Valid()) {
                TInstanceHandleList childHandles;
                CGraphIterator children;
                GetAssetChildrenInActiveSlide(selected, children);
                for (; !children.IsDone(); ++children) {
                    TInstanceHandle child = children.GetCurrent();
                    childHandles.push_back(child);
                }

                // Rename the selected and to-be deleted instance so that it is less likely to cause
                // name clash when its children are moved to the same level
                CString name = GetName(selected);
                name.append("@@to_be_deleted@@");
                SetName(selected, name);

                // Move group's children directly below the group item
                RearrangeObjects(childHandles, selected, DocumentEditorInsertType::NextSibling,
                                 true);

                // Delete the group
                DeleteInstance(selected);

                // Select ungrouped instances
                for (size_t i = 0, end = childHandles.size(); i < end; ++i) {
                    if (i == 0 && idx == 0)
                        m_Doc.SelectDataModelObject(childHandles[i]);
                    else
                        m_Doc.ToggleDataModelObjectToSelection(childHandles[i]);
                }
            }
        }
    }

    // Creates a new group object and moves the specified objects as its children
    void groupObjects(const TInstanceHandleList &inInstances) override
    {
        if (inInstances.size() > 0) {
            TInstanceHandleList sortedList(ToGraphOrdering(inInstances));
            // Create a new group next to the topmost item in the graph
            TInstanceHandle sibling = sortedList.front();
            Qt3DSDMSlideHandle slide = GetActiveSlide(sibling);
            if (m_Bridge.IsMaster(sibling))
                slide = m_SlideSystem.GetMasterSlide(slide);
            TInstanceHandle group = CreateSceneGraphInstance(
                        ComposerObjectTypes::Group, sibling, slide,
                        DocumentEditorInsertType::PreviousSibling, CPt(),
                        PRIMITIVETYPE_UNKNOWN, -1);
            // Move items into the group
            RearrangeObjects(sortedList, group, DocumentEditorInsertType::LastChild, true);
        }
    }

    Qt3DSDMInstanceHandle MakeComponent(const qt3dsdm::TInstanceHandleList &inInstances) override
    {
        if (inInstances.empty())
            return Qt3DSDMInstanceHandle();

        qt3dsdm::TInstanceHandleList theInstances = ToGraphOrdering(inInstances);

        // Get the original start/end times
        QList<std::pair<long, long>> theStartEndTimes;

        for (auto instance : qAsConst(theInstances))
            theStartEndTimes.append(GetTimeRange(instance));

        // Do this in reverse order.
        // first add new component.
        Qt3DSDMSlideHandle theSlide = GetAssociatedSlide(theInstances[0]);

        TInstanceHandle component = CreateSceneGraphInstance(
            ComposerObjectTypes::Component, theInstances[0], theSlide,
            DocumentEditorInsertType::NextSibling, CPt(), PRIMITIVETYPE_UNKNOWN, 0);

        CString theName = GetName(theInstances[0]);

        // now cut the group
        std::shared_ptr<IDOMReader> theReader(CopySceneGraphObjectsToMemory(theInstances));
        DeleteInstances(theInstances);

        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        Qt3DSDMSlideHandle theComponentSlide(m_Bridge.GetComponentActiveSlide(component));

        // Paste into the master slide of the new component
        TInstanceHandleList insertedHandles = theSerializer->SerializeSceneGraphObject(
                    *theReader,m_Doc.GetDocumentDirectory(), component,
                    m_SlideSystem.GetMasterSlide(theComponentSlide), true);

        // Restore the original time range for all objects.
        if (insertedHandles.size()) {
            for (int i = 0; i < theStartEndTimes.size(); i++) {
                if (theStartEndTimes.at(i) != std::make_pair(0L, 0L)) {
                    SetTimeRange(insertedHandles.at(i), theStartEndTimes.at(i).first,
                                 theStartEndTimes.at(i).second);
                }
            }
        }

        SetName(component, theName);

        m_Doc.SelectDataModelObject(component);
        return component;
    }

    void makeAnimatable(const qt3dsdm::TInstanceHandleList &instances) override
    {
        for (auto &instance : instances) {
            const Q3DStudio::CString oldType = GetObjectTypeName(instance);
            if (oldType == "ReferencedMaterial") {
                Qt3DSDMInstanceHandle refMaterial = m_Bridge.getMaterialReference(instance);

                if (refMaterial.Valid())
                    copyMaterialProperties(refMaterial, instance);
                else
                    SetMaterialType(instance, QStringLiteral("Standard Material"));

                const auto name = GetName(instance);
                if (!name.toQString().endsWith(QLatin1String("_animatable")))
                    SetName(instance, name + "_animatable");
            }
        }
    }

    // Moves specified instances into target component by a simulated cut and paste.
    // This is only necessary when moving objects from outside the component into the component.
    // Returns true if move was done. Returns false if instances are already in target component,
    // which means a regular rearrange can be done.
    bool moveIntoComponent(const qt3dsdm::TInstanceHandleList &inInstances,
                           const Qt3DSDMInstanceHandle targetComponent, bool checkUniqueName,
                           bool notifyRename)
    {
        if (inInstances.empty())
            return false;

        Qt3DSDMInstanceHandle rootInstance = GetParent(inInstances[0]);
        while (rootInstance.Valid() && !m_Bridge.IsComponentInstance(rootInstance))
            rootInstance = GetParent(rootInstance);

        if (rootInstance == targetComponent)
            return false;

        const qt3dsdm::TInstanceHandleList theInstances = ToGraphOrdering(inInstances);
        QList<std::pair<long, long>> theStartEndTimes;

        for (auto instance : theInstances)
            theStartEndTimes.append(GetTimeRange(instance));

        // Now cut the group from the scene.
        std::shared_ptr<IDOMReader> theReader(CopySceneGraphObjectsToMemory(theInstances));

        DeleteInstances(theInstances);

        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        Qt3DSDMSlideHandle theComponentSlide(m_Bridge.GetComponentActiveSlide(targetComponent));

        // Paste into the master slide of the new component.
        TInstanceHandleList insertedHandles =
                theSerializer->SerializeSceneGraphObject(
                    *theReader, m_Doc.GetDocumentDirectory(),
                    targetComponent,
                    m_SlideSystem.GetMasterSlide(theComponentSlide), true);

        if (insertedHandles.size()) {
            // Restore the original time range for all objects.
            for (int i = 0; i < theStartEndTimes.size(); i++) {
                if (theStartEndTimes.at(i) != std::make_pair(0L, 0L))
                    SetTimeRange(insertedHandles.at(i), theStartEndTimes.at(i).first,
                                 theStartEndTimes.at(i).second);
            }

            // Check for name uniqueness
            if (checkUniqueName) {
                for (auto instance : insertedHandles) {
                    CString currName = m_Bridge.GetName(instance);
                    if (!m_Bridge.CheckNameUnique(targetComponent, instance, currName)) {
                        CString newName = m_Bridge.GetUniqueChildName(
                                    targetComponent, instance, currName);
                        if (notifyRename) {
                            m_Doc.getMoveRenameHandler()->displayMessageBox(currName.toQString(),
                                                                            newName.toQString());
                        }
                        SetName(instance, newName);
                    }
                }
            }
        }
        return true;
    }

    void DuplicateInstances(const qt3dsdm::TInstanceHandleList &inInstances) override
    {

        TInstanceHandleList theInstances = ToGraphOrdering(inInstances);
        if (theInstances.empty())
            return;
        DuplicateInstances(theInstances, theInstances.back(),
                           DocumentEditorInsertType::NextSibling);
    }

    TInstanceHandleList DuplicateInstances(const qt3dsdm::TInstanceHandleList &inInstances,
                                                   TInstanceHandle inDest,
                                                   DocumentEditorInsertType::Enum inInsertType) override
    {
        qt3dsdm::TInstanceHandleList theInstances(ToGraphOrdering(inInstances));
        std::shared_ptr<IDOMReader> theReader(CopySceneGraphObjectsToMemory(theInstances));
        return DoPasteSceneGraphObject(theReader, inDest, true, inInsertType, CPt(), false, false);
    }

    Qt3DSDMActionHandle AddAction(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                                         const wstring &inEvent, const wstring &inHandler) override
    {
        Q3DStudio::CId theGuid = m_Bridge.GetGUID(inOwner);
        Q3DStudio::TGUIDPacked thePacked(theGuid);
        SLong4 theInitialTriggerTarget(thePacked.Data1, thePacked.Data2, thePacked.Data3,
                                       thePacked.Data4);
        Qt3DSDMActionHandle theAction =
            m_ActionSystem.CreateAction(inSlide, inOwner, theInitialTriggerTarget);
        m_ActionCore.SetEvent(theAction, inEvent);
        m_ActionCore.SetHandler(theAction, inHandler);
        m_Bridge.ResetHandlerArguments(theAction, inHandler);
        return theAction;
    }

    void DeleteAction(Qt3DSDMActionHandle inAction) override
    {
        m_ActionSystem.DeleteAction(inAction);
    }

    Qt3DSDMActionHandle PasteAction(const CFilePath &inFilePath,
                                           Qt3DSDMInstanceHandle inNewRoot) override
    {
        CFileSeekableIOStream theStream(inFilePath.toCString(), FileReadFlags());
        if (theStream.IsOpen() == false) {
            QT3DS_ASSERT(false);
            return 0;
        }
        std::shared_ptr<IDOMFactory> theFactory(
            IDOMFactory::CreateDOMFactory(m_DataCore.GetStringTablePtr()));
        SDOMElement *theElem = CDOMSerializer::Read(*theFactory, theStream);
        if (theElem == NULL) {
            QT3DS_ASSERT(false);
            return 0;
        }
        std::shared_ptr<IDOMReader> theReader(
            IDOMReader::CreateDOMReader(*theElem, m_DataCore.GetStringTablePtr(), theFactory));
        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        return theSerializer->SerializeAction(*theReader, inNewRoot, GetActiveSlide(inNewRoot));
    }

    bool ContainsSlideByName(const CString &inName, Qt3DSDMSlideHandle inMasterSlide)
    {
        size_t existingCount = m_SlideSystem.GetSlideCount(inMasterSlide);
        for (size_t idx = 0; idx < existingCount; ++idx) {
            Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetSlideByIndex(inMasterSlide, idx);
            Qt3DSDMInstanceHandle theInstance(m_SlideSystem.GetSlideInstance(theSlide));
            if (GetName(theInstance) == inName)
                return true;
        }
        return false;
    }

    CString GenerateUniqueSlideName(const CString &inStem, Qt3DSDMSlideHandle inMasterSlide,
                                    int inStartIndex)
    {
        size_t theStartIndex = inStartIndex;
        if (theStartIndex < 0)
            theStartIndex = m_SlideSystem.GetSlideCount(inMasterSlide);

        CString baseName = inStem;
        int nameIdx = (int)theStartIndex;
        wchar_t nameBuf[16];
        WStrOps<int>().ToStr(nameIdx, toDataRef(nameBuf, 16));
        CString theNameStr = baseName;
        theNameStr.append(nameBuf);
        while (ContainsSlideByName(theNameStr, inMasterSlide)) {
            ++nameIdx;
            WStrOps<int>().ToStr(nameIdx, toDataRef(nameBuf, 16));
            theNameStr = baseName;
            theNameStr.append(nameBuf);
        }
        return theNameStr;
    }

    void CheckSlideGroupPlayThroughTo(Qt3DSDMSlideHandle inSlide)
    {
        Qt3DSDMSlideHandle theMaster(m_SlideSystem.GetMasterSlide(inSlide));
        size_t slideCount(m_SlideSystem.GetSlideCount(theMaster));
        for (size_t idx = 1; idx < slideCount; ++idx) {
            bool hasPrevious = idx > 1;
            bool hasNext = idx < slideCount - 1;
            Qt3DSDMSlideHandle theCurrentSlide = m_SlideSystem.GetSlideByIndex(theMaster, idx);
            Qt3DSDMInstanceHandle theSlideInstance = m_SlideCore.GetSlideInstance(theCurrentSlide);
            SValue theValue;
            Qt3DSDMPropertyHandle theProp = m_Bridge.GetObjectDefinitions().m_Slide.m_PlaythroughTo;
            m_DataCore.GetInstancePropertyValue(theSlideInstance, theProp, theValue);
            SStringOrInt theData(get<SStringOrInt>(theValue));
            if (theData.GetType() == SStringOrIntTypes::Int) {
                Qt3DSDMSlideHandle theSlide((int)get<long>(theData.m_Value));
                if (m_SlideCore.IsSlide(theSlide) == false) {
                    theData = SStringOrInt(std::make_shared<CDataStr>(L"Next"));
                    m_DataCore.SetInstancePropertyValue(theSlideInstance, theProp, theData);
                }
            }
            // Note that we explicitly run this next section to take care of the situation
            // where the target playthroughto slide was deleted and now we have to deal with it.
            if ((hasNext || hasPrevious) && theData.GetType() == SStringOrIntTypes::String) {
                TDataStrPtr theStrPtr = get<TDataStrPtr>(theData.m_Value);
                if (hasNext == false && AreEqual(L"Next", theStrPtr->GetData()))
                    m_DataCore.SetInstancePropertyValue(
                        theSlideInstance, theProp,
                        SStringOrInt(std::make_shared<CDataStr>(L"Previous")));
                else if (hasPrevious == false && AreEqual(L"Previous", theStrPtr->GetData()))
                    m_DataCore.SetInstancePropertyValue(
                        theSlideInstance, theProp,
                        SStringOrInt(std::make_shared<CDataStr>(L"Next")));
            }
            if (slideCount == 2) {
                theProp = m_Bridge.GetObjectDefinitions().m_Slide.m_PlayMode;
                m_DataCore.GetInstancePropertyValue(theSlideInstance, theProp, theValue);
                TDataStrPtr theStrPtr = get<TDataStrPtr>(theValue);
                if (AreEqual(theStrPtr->GetData(), L"Play Through To..."))
                    m_DataCore.SetInstancePropertyValue(theSlideInstance, theProp,
                                                        std::make_shared<CDataStr>(L"Looping"));
            }
        }
    }

    Qt3DSDMSlideHandle AddSlide(Qt3DSDMSlideHandle inMasterSlide, int inIndex = -1) override
    {
        CString theNewName = GenerateUniqueSlideName(L"Slide", inMasterSlide, inIndex);
        Qt3DSDMSlideHandle theNewSlide = m_SlideSystem.DuplicateSlide(inMasterSlide, inIndex);
        Qt3DSDMInstanceHandle newInstance(m_SlideSystem.GetSlideInstance(theNewSlide));
        m_DataCore.SetInstancePropertyValue(newInstance,
                                            m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                                            std::make_shared<CDataStr>(theNewName.c_str()));
        m_Doc.SetActiveSlideWithTransaction(theNewSlide);
        int newSlideIndex = m_SlideSystem.GetSlideIndex(theNewSlide);
        m_SlideSystem.SetActiveSlide(inMasterSlide, newSlideIndex);
        m_Doc.NotifyActiveSlideChanged(theNewSlide, true);
        CheckSlideGroupPlayThroughTo(theNewSlide);
        Qt3DSDMInstanceHandle theInstance = m_Doc.GetSelectedInstance();
        if (theInstance.Valid() && GetAssociatedSlide(theInstance) != inMasterSlide)
            m_Doc.SelectDataModelObject(0);
        return theNewSlide;
    }

    // Only valid if the master slide has more than one slide.
    void DeleteSlide(Qt3DSDMSlideHandle inSlide) override
    {
        TInstanceHandleList theInstances;
        m_SlideSystem.GetAssociatedInstances(inSlide, theInstances);
        for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
            // Action instances are also associated with slides but they need to be deleted
            // by DataModel when the action itself is deleted rather than by us right here.
            TInstanceHandle theInstance(theInstances[idx]);
            if (m_SlideSystem.GetAssociatedSlide(theInstance) == inSlide && IsInstance(theInstance)
                && m_DataCore.IsInstanceOrDerivedFrom(
                       theInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_Instance)) {
                DeleteInstance(theInstance);
            }
        }

        Qt3DSDMSlideHandle theMaster = m_SlideCore.GetParentSlide(inSlide);
        size_t theCount = m_SlideSystem.GetSlideCount(theMaster);
        if (theCount < 2) {
            QT3DS_ASSERT(false);
            return;
        }
        TInstanceHandleList theSlideInstances;
        m_SlideCore.DeleteSlide(inSlide, theSlideInstances);
        m_DataCore.DeleteInstance(theSlideInstances[0]);
        CheckSlideGroupPlayThroughTo(theMaster);
    }

    void RearrangeSlide(Qt3DSDMSlideHandle inSlide, int inNewIndex) override
    {
        Qt3DSDMSlideHandle theMaster = m_SlideSystem.GetMasterSlide(inSlide);
        int theOldIndex = m_SlideSystem.GetSlideIndex(inSlide);
        m_SlideSystem.RearrangeSlide(theMaster, theOldIndex, inNewIndex);
        CheckSlideGroupPlayThroughTo(theMaster);
    }

    Qt3DSDMSlideHandle DuplicateSlide(Qt3DSDMSlideHandle inSlide) override
    {
        std::shared_ptr<IDOMReader> theReader(CopySlide(inSlide));
        if (!theReader)
            return 0;

        Qt3DSDMSlideHandle theMaster = m_SlideSystem.GetMasterSlide(inSlide);
        int theIndex = m_SlideSystem.GetSlideIndex(inSlide);
        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();

        CString theNewName = GenerateUniqueSlideName(L"Slide", theMaster, theIndex + 1);

        Qt3DSDMSlideHandle theNewSlide = theSerializer->SerializeSlide(
            *theReader, m_Doc.GetDocumentDirectory(), theMaster, theIndex);

        Qt3DSDMInstanceHandle newInstance(m_SlideSystem.GetSlideInstance(theNewSlide));
        m_DataCore.SetInstancePropertyValue(newInstance,
                                            m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                                            std::make_shared<CDataStr>(theNewName.c_str()));

        // Ensure the active slide change gets recorded in the transaction system so that
        // undo will place us back at the old slide before things start reading from the object
        // model.
        int newSlideIndex = m_SlideSystem.GetSlideIndex(theNewSlide);
        m_SlideSystem.SetActiveSlide(theMaster, newSlideIndex);
        m_Doc.SetActiveSlideWithTransaction(theNewSlide);

        m_Doc.NotifyActiveSlideChanged(theNewSlide, true);
        CheckSlideGroupPlayThroughTo(theNewSlide);
        return theNewSlide;
    }

    Qt3DSDMGuideHandle CreateGuide(const qt3dsdm::SGuideInfo &inInfo) override
    {
        Qt3DSDMGuideHandle retval = m_GuideSystem.CreateGuide();
        m_GuideSystem.SetGuideInfo(retval, inInfo);
        return retval;
    }

    void UpdateGuide(Qt3DSDMGuideHandle hdl, const qt3dsdm::SGuideInfo &inInfo) override
    {
        m_GuideSystem.SetGuideInfo(hdl, inInfo);
    }

    void DeleteGuide(Qt3DSDMGuideHandle hdl) override { m_GuideSystem.DeleteGuide(hdl); }

    void ClearGuides() override
    {
        qt3dsdm::TGuideHandleList theGuides(GetGuides());
        for (size_t idx = 0, end = theGuides.size(); idx < end; ++idx)
            DeleteGuide(theGuides[idx]);
        m_Doc.GetSceneGraph()->RequestRender();
    }

    qt3dsdm::TGuideHandleList GetGuides() const override { return m_GuideSystem.GetAllGuides(); }

    qt3dsdm::SGuideInfo GetGuideInfo(qt3dsdm::Qt3DSDMGuideHandle inGuide) const override
    {
        return m_GuideSystem.GetGuideInfo(inGuide);
    }

    bool IsGuideValid(qt3dsdm::Qt3DSDMGuideHandle inGuide) const override
    {
        return m_GuideSystem.IsGuideValid(inGuide);
    }

    bool AreGuidesEditable() const override { return m_GuideSystem.AreGuidesEditable(); }

    void SetGuidesEditable(bool val) override
    {
        m_GuideSystem.SetGuidesEditable(val);
        if (m_Doc.GetSelectedValue().getType() == Q3DStudio::SelectedValueTypes::Guide
            && val == false)
            m_Doc.NotifySelectionChanged();
    }

    void updateMaterialFiles()
    {
        auto parent = getOrCreateMaterialContainer();
        TInstanceList children;
        GetChildren(GetAssociatedSlide(parent), parent, children);
        const QString defaultMaterialName
                = getMaterialNameFromFilePath(m_Bridge.getDefaultMaterialName());

        for (auto &instance : children) {
            const auto name = GetName(instance).toQString();
            if (name == defaultMaterialName)
                continue;
            const QString path = getFilePathFromMaterialName(name);
            writeMaterialFile(getOrCreateMaterial(path), name, false, path);
        }
    }

    void updateMaterialInstances(const QStringList &filenames) override
    {
        const auto parent = m_Bridge.getMaterialContainer();
        if (parent.Valid()) {
            TInstanceList children;
            GetChildren(GetAssociatedSlide(parent), parent, children);

            for (auto &instance : children) {
                auto name = GetName(instance).toQString();
                if (name != getMaterialNameFromFilePath(m_Bridge.getDefaultMaterialName())
                        && !filenames.contains(name)) {
                    DeleteInstance(instance);
                }
            }
        }
    }

    void removeUnusedFromMaterialContainer() override
    {
        QVector<Qt3DSDMInstanceHandle> usedMats;
        m_Doc.getUsedSharedMaterials(usedMats);

        const auto parent = m_Bridge.getMaterialContainer();
        if (parent.Valid()) {
            TInstanceList children;
            GetChildren(GetAssociatedSlide(parent), parent, children);

            unsigned int removedChildrenCount = 0;
            for (auto &instance : children) {
                if (!usedMats.contains(instance)) {
                    DeleteInstance(instance);
                    removedChildrenCount++;
                }
            }

            if (removedChildrenCount == children.size())
                DeleteInstance(parent);
        }
    }

    void removeDeletedFromMaterialContainer() override
    {
        const auto parent = m_Bridge.getMaterialContainer();
        if (parent.Valid()) {
            TInstanceList children;
            GetChildren(GetAssociatedSlide(parent), parent, children);

            unsigned int removedChildrenCount = 0;
            for (auto &instance : children) {
                const auto name = GetName(instance).toQString();
                if (name != getMaterialNameFromFilePath(m_Bridge.getDefaultMaterialName())
                        && !QFileInfo(getFilePathFromMaterialName(name)).exists()) {
                    DeleteInstance(instance);
                    removedChildrenCount++;
                }
            }

            if (removedChildrenCount == children.size())
                DeleteInstance(parent);
        }
    }

    TInstanceHandle DoImport(
        CFilePath inImportFilePath, Q3DStudio::CString importSrc, Qt3DSDMInstanceHandle inParent,
        Qt3DSDMInstanceHandle inRoot, Qt3DSDMSlideHandle inSlide, Q3DStudio::CString inDocDir,
        STranslationLog &inTranslationLog,
        function<SImportResult(IComposerEditorInterface &, Q3DStudio::CString)> inImportFunction,
        DocumentEditorInsertType::Enum inInsertType, const CPt &inPosition, long inStartTime,
        bool selectAfterImport)
    {
        CFilePath outputDir(inImportFilePath.GetDirectory());
        bool alwaysKeepDirectory = outputDir.Exists();
        bool keepDirectory = false;
        Qt3DSDMInstanceHandle theRealParent = inInsertType == DocumentEditorInsertType::LastChild
            ? inParent
            : Qt3DSDMInstanceHandle(m_AssetGraph.GetParent(inParent));
        // We have to pass in the real parent to the editor interface so that object lifetimes can
        // be setup correctly as the import tree is being built.
        std::shared_ptr<IComposerEditorInterface> importToComposer =
            IComposerEditorInterface::CreateEditorInterface(*this, theRealParent, inRoot, inSlide,
                                                            inDocDir, inImportFilePath, inStartTime,
                                                            m_StringTable);

        CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());
        CFilePath theDestFile(importToComposer->GetDestImportFile());
        try {
            theDispatch.FireOnProgressBegin(QObject::tr("Importing "),
                                            QFileInfo(importSrc.toQString()).fileName());
            SImportResult result = inImportFunction(*importToComposer, theDestFile);
            bool forceError = importToComposer->HasError();
            if (!forceError)
                importToComposer->Finalize(result.m_FilePath);
            keepDirectory = alwaysKeepDirectory || forceError == false;
            theDispatch.FireOnProgressEnd();
            IDocumentEditor::DisplayImportErrors(importSrc.toQString(), result.m_Error,
                                                 m_Doc.GetImportFailedHandler(), inTranslationLog,
                                                 forceError);
            if (!forceError) {
                Qt3DSDMInstanceHandle theImportRoot = importToComposer->GetRoot();
                CFilePath theRelPath(m_Doc.GetRelativePathToDoc(theDestFile));
                SValue theSourcePathValue(std::make_shared<CDataStr>(theRelPath.toCString()));
                Qt3DSDMPropertyHandle theProp(m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath);
                if (inSlide.Valid())
                    m_SlideCore.ForceSetInstancePropertyValue(inSlide, theImportRoot, theProp,
                                                              theSourcePathValue);
                else
                    m_DataCore.SetInstancePropertyValue(theImportRoot, theProp, theSourcePathValue);

                // Do not check for unique name as we set it anyway after getting new handle
                Qt3DSDMInstanceHandle retval =
                    FinalizeAddOrDrop(importToComposer->GetRoot(), inParent, inInsertType,
                                      inPosition, inStartTime == -1, selectAfterImport, false);
                SetName(retval, theRelPath.GetFileStem(), true);

                updateMaterialFiles();

                return retval;
            }
        } catch (...) {
            theDispatch.FireOnProgressEnd();
            m_Doc.rollbackTransaction(); // Run away!!!
        }
        return 0;
    }

    TInstanceHandle ImportDAE(const Q3DStudio::CString &inFullPathToDocument,
                                      TInstanceHandle inParent, TSlideHandle inSlide,
                                      const Q3DStudio::CString &inImportFileExtension,
                                      DocumentEditorInsertType::Enum inDropType,
                                      const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        // If we already have an import file that points back to this DAE then we need to
        // not import the DAE but import the import file again.

        CFilePath importSrc = CFilePath(inFullPathToDocument);
        if (importSrc.Exists() == false)
            return 0;
        CFilePath theRelativeDAE = m_Doc.GetRelativePathToDoc(importSrc);

        CFilePath docPath(m_Doc.GetDocumentPath());
        CFilePath docDir(docPath.GetDirectory());
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        if (docPath.size() == 0) {
            if (theHandler)
                theHandler->DisplayImportFailed(importSrc.toQString(),
                                                QObject::tr("Qt3DStudio document has no path"),
                                                false);
            return 0;
        }
        if (!importSrc.IsFile()) {
            if (theHandler)
                theHandler->DisplayImportFailed(importSrc.toQString(),
                                                QObject::tr("Source File Doesn't Exist"), false);
            return 0;
        }

        Q3DStudio::CString fname = importSrc.GetFileStem();

        CFilePath importsDir = CFilePath::CombineBaseAndRelative(docDir, CFilePath(L"Imports"));
        if (importsDir.Exists() == false)
            importsDir.CreateDir(true);

        CFilePath outputDir = Q3DStudio::SFileTools::FindUniqueDestDirectory(importsDir, fname);
        Q3DStudio::CString outputFileName(fname + L"." + inImportFileExtension);
        SColladaTranslator translator(importSrc.toQString());
        TInstanceHandle retval =
            DoImport(CFilePath::CombineBaseAndRelative(outputDir, outputFileName), importSrc,
                     inParent, 0, inSlide, docDir, translator.m_TranslationLog,
                     std::bind(CPerformImport::ImportToComposer, translator,
                               std::placeholders::_1, std::placeholders::_2), inDropType,
                               inPosition, inStartTime, true);
        if (retval.Valid()) {
            CFilePath theRelativeImport = m_Doc.GetRelativePathToDoc(outputFileName);
            m_ImportFileToDAEMap.insert(
                make_pair(m_StringTable.RegisterStr(theRelativeImport.toCString()),
                          m_StringTable.RegisterStr(theRelativeDAE.toCString())));
        }

        return retval;
    }

    TInstanceHandle LoadImportFile(const Q3DStudio::CString &inFullPathToDocument,
                                           TInstanceHandle inParent, TSlideHandle inSlide,
                                           DocumentEditorInsertType::Enum inDropType,
                                           const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        CFilePath docPath(m_Doc.GetDocumentPath());
        CFilePath docDir(docPath.GetDirectory());
        STranslationLog log;
        return DoImport(inFullPathToDocument, inFullPathToDocument, inParent, 0, inSlide, docDir,
                        log, std::bind(CPerformImport::ImportToComposerFromImportFile,
                                       std::placeholders::_1, std::placeholders::_2),
                                       inDropType, inPosition, inStartTime, true);
    }

    QString findUniqueMaterialName(const QString &name, const QString &importPath)
    {
        auto materialName = name;
        QString potentialPath = getFilePathFromMaterialName(
                    getMaterialNameFromFilePath(materialName));
        int i = 1;
        const auto originalMaterialName = materialName;
        const QString importFile = QStringLiteral("importfile");
        while (QFileInfo(potentialPath).exists()) {
            i++;
            QString name;
            QMap<QString, QString> values;
            QMap<QString, QMap<QString, QString>> textureValues;
            getMaterialInfo(potentialPath, name, values, textureValues);
            if (values.contains(importFile) && values[importFile] == importPath) {
                const auto material = getOrCreateMaterial(materialName);
                setMaterialValues(material, values, textureValues);
                break;
            }
            materialName = originalMaterialName + QString::number(i);
            potentialPath = getFilePathFromMaterialName(
                        getMaterialNameFromFilePath(materialName));
        }
        return materialName;
    }

    TInstanceHandle createRefMaterialFromImageOrPresentation(
            TInstanceHandle parent, TSlideHandle slide, const CString &absSrc, bool isSubp)
    {
        qt3dsdm::Qt3DSDMInstanceHandle refInstance
                = CreateSceneGraphInstance(ComposerObjectTypes::ReferencedMaterial, parent, slide);
        TInstanceHandle imageMaterial;
        CString relPath;
        QString materialName;
        if (isSubp) {
            relPath = absSrc;
            materialName = findUniqueMaterialName(absSrc.toQString(), relPath.toQString());
            imageMaterial = getOrCreateMaterial(materialName);
            const auto prop = m_Bridge.GetObjectDefinitions().m_Material.m_DiffuseMap1.m_Property;
            setInstanceImagePropertyValue(imageMaterial, prop, absSrc, true);
        } else {
            CFilePath absPath(absSrc);
            relPath = CString::fromQString(m_Doc.GetRelativePathToDoc(absPath));
            materialName = findUniqueMaterialName(absPath.GetFileStem().toQString(),
                                                  relPath.toQString());
            imageMaterial = getOrCreateMaterial(materialName);
            SetInstancePropertyValueAsImage(
                imageMaterial, m_Bridge.GetDefaultMaterial().m_DiffuseMap1, relPath);
        }
        IDocumentEditor::SetSpecificInstancePropertyValue(0, imageMaterial, L"importfile",
                                                          std::make_shared<CDataStr>(relPath));
        auto sourcePath = writeMaterialFile(imageMaterial, materialName, true);

        setMaterialReferenceByPath(refInstance, materialName);
        setMaterialSourcePath(refInstance, sourcePath);
        SetName(refInstance, CString::fromQString(materialName));
        return refInstance;
    }

    TInstanceHandle AutomapImage(const Q3DStudio::CString &inFullPathToDocument,
                                         TInstanceHandle inParent, TSlideHandle inSlide,
                                         DocumentEditorInsertType::Enum inDropType,
                                         const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        (void)inStartTime;

        CFilePath imageSrc(inFullPathToDocument);
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());

        if (!imageSrc.IsFile()) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.toQString(),
                                                QObject::tr("Image File Doesn't Exist"), false);
            return 0;
        }
        CFilePath relativePath = m_Doc.GetRelativePathToDoc(imageSrc);
        SImageTextureData theImageBuffer = m_Doc.GetBufferCache()
                                           .GetOrCreateImageBuffer(relativePath);
        if (!theImageBuffer.m_Texture) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.toQString(),
                                                QObject::tr("Can't Load Image File"), false);
            return 0;
        }

        // Automap the image to a rectangle

        qt3dsdm::Qt3DSDMInstanceHandle theModelInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::Model, inParent, inSlide);
        m_PropertySystem.SetInstancePropertyValue(
            theModelInstance, m_Bridge.GetSourcePathProperty(),
            std::make_shared<qt3dsdm::CDataStr>(
                m_Doc.GetBufferCache().GetPrimitiveName(PRIMITIVETYPE_RECT)));
        // Create the object material
        createRefMaterialFromImageOrPresentation(theModelInstance, inSlide,
                                                 inFullPathToDocument, false);

        if (inStartTime != -1)
            SetStartTime(theModelInstance, inStartTime);

        STextureDetails theDetails = theImageBuffer.m_Texture->GetTextureDetails();
        float theHeight = theDetails.m_Height / 100.0f;
        float theWidth = theDetails.m_Width / 100.0f;
        qt3dsdm::SFloat3 theScale = qt3dsdm::SFloat3(2, 2, 1); // Default, per Danc.
        if (theHeight != 0 && theWidth != 0)
            theScale = qt3dsdm::SFloat3(theWidth, theHeight, 1);
        m_PropertySystem.SetInstancePropertyValue(theModelInstance, m_Bridge.GetNode().m_Scale,
                                                  theScale);

        CFilePath theFilePath(inFullPathToDocument);
        SetName(theModelInstance, theFilePath.GetFileStem(), true);

        // Set the image as the property of the first diffuse map.
        return FinalizeAddOrDrop(theModelInstance, inParent, inDropType, inPosition,
                                 inStartTime == -1);
    }

    TInstanceHandle LoadMesh(const Q3DStudio::CString &inFullPathToDocument,
                                     TInstanceHandle inParent, TSlideHandle inSlide,
                                     DocumentEditorInsertType::Enum inDropType,
                                     const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        CFilePath imageSrc(inFullPathToDocument);
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        if (!imageSrc.IsFile()) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.toQString(),
                                                QObject::tr("Source File Doesn't Exist"), false);
            return 0;
        }
        Q3DStudio::CString theRelativePath = Q3DStudio::CString::fromQString(
                                             m_Doc.GetRelativePathToDoc(inFullPathToDocument));
        SModelBufferAndPath theModelBuffer =
            m_Doc.GetBufferCache().GetOrCreateModelBuffer(theRelativePath);
        if (!theModelBuffer.m_ModelBuffer) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.toQString(),
                                                QObject::tr("Could Not Load Model Buffer"), false);
            return 0;
        }
        // Ensure we include the model buffer version in the relative path
        theRelativePath = Q3DStudio::CString::fromQString(
                          m_Doc.GetRelativePathToDoc(theModelBuffer.m_FilePath));

        qt3dsdm::Qt3DSDMInstanceHandle theModelInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::Model, inParent, inSlide);

        SValue theValue(std::make_shared<qt3dsdm::CDataStr>(theRelativePath));
        m_PropertySystem.SetInstancePropertyValue(theModelInstance,
                                                  m_Bridge.GetSourcePathProperty(), theValue);

        if (inStartTime != -1)
            SetStartTime(theModelInstance, inStartTime);

        CheckMeshSubsets(theModelInstance, m_Bridge.GetSourcePathProperty());

        SetName(theModelInstance, imageSrc.GetFileStem(), true);

        return FinalizeAddOrDrop(theModelInstance, inParent, inDropType, inPosition,
                                 inStartTime == -1);
    }

    static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
    {
        Q_UNUSED(ud)
        Q_UNUSED(osize)

        if (nsize == 0) {
            free(ptr);
            return nullptr;
        } else
            return realloc(ptr, nsize);
    }

    QString LoadScriptFile(const CFilePath &inFile)
    {
        QString retval;

        QQmlEngine engine;
        QString path = inFile.filePath();
        path.replace(QLatin1Char('\\'), QLatin1Char('/'));
        QQmlComponent component(&engine, QUrl::fromLocalFile(path),
                                QQmlComponent::CompilationMode::PreferSynchronous);
        if (component.status() == QQmlComponent::Error)
            retval = component.errorString().toUtf8().data();

        return retval;
    }

    void DisplayLoadWarnings(const QString &inSrcPath,
                             std::vector<SMetaDataLoadWarning> &inWarnings,
                             const QString &inLoadError)
    {
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        if ((inWarnings.empty() && inLoadError.size() == 0) || !theHandler)
            return;

        QString resultDialogStr;

        for (size_t idx = 0, end = inWarnings.size(); idx < end; ++idx) {
            QString theTypeStr;
            QString theMessageStr;

            switch (inWarnings[idx].m_Type) {
            case MetaDataLoadWarningType::InvalidProperty:
                theTypeStr = QObject::tr("Invalid Property");
                break;
            case MetaDataLoadWarningType::InvalidEvent:
                theTypeStr = QObject::tr("Invalid Event");
                break;
            case MetaDataLoadWarningType::InvalidHandler:
                theTypeStr = QObject::tr("Invalid Handler");
                break;
            default:
                QT3DS_ALWAYS_ASSERT_MESSAGE("Unknown load warning type")
                break;
            }

            switch (inWarnings[idx].m_Message) {
            case MetaDataLoadWarningMessage::GeneralError:
                theMessageStr = QObject::tr("General Error");
                break;
            case MetaDataLoadWarningMessage::MissingName:
                theMessageStr = QObject::tr("Missing Name");
                break;
            case MetaDataLoadWarningMessage::InvalidDefault:
                theMessageStr = QObject::tr("Invalid Default");
                break;
            default:
                QT3DS_ALWAYS_ASSERT_MESSAGE("Unknown load warning message")
                break;
            }

            if (inWarnings[idx].m_ExtraInfo.size()) {
                theMessageStr.append(" ");
                theMessageStr.append(QString::fromStdWString(inWarnings[idx].m_ExtraInfo.wide_str()));
            }

            const QString theBuffer = QStringLiteral("%1: %2\n").arg(theTypeStr).arg(theMessageStr);

            resultDialogStr.append(theBuffer);
        }
        if (inLoadError.size()) {
            resultDialogStr.append(QObject::tr("\nError parsing script file: "));
            resultDialogStr.append(inLoadError);
        }
        if (resultDialogStr.size())
            theHandler->DisplayImportFailed(inSrcPath, resultDialogStr, true);
    }

    // Apply meta data to a new dynamic instance.  This sets up the default properties to
    // be what the meta data specifies.
    void ApplyDynamicMetaData(Qt3DSDMInstanceHandle inDynamicInstance,
                              Qt3DSDMInstanceHandle inDynamic)
    {
        std::vector<SMetaDataLoadWarning> theWarnings;
        // For all of the object std::ref properties, check if they have an absolute path
        // reference (path starts with "Scene".  If they do, then attempt to resolve the reference.
        vector<Qt3DSDMMetaDataPropertyHandle> theProperties;
        m_MetaData.GetSpecificMetaDataProperties(inDynamic, theProperties);
        for (size_t propIdx = 0, propEnd = theProperties.size(); propIdx < propEnd; ++propIdx) {
            SMetaDataPropertyInfo theInfo(
                m_MetaData.GetMetaDataPropertyInfo(theProperties[propIdx]));
            if (theInfo.m_CompleteType == CompleteMetaDataType::ObjectRef
                && GetValueType(theInfo.m_DefaultValue) == DataModelDataType::ObjectRef) {
                SObjectRefType theRef(get<SObjectRefType>(theInfo.m_DefaultValue));
                wstring theData;
                wstring theOriginalData;
                if (theRef.GetReferenceType() == ObjectReferenceType::Relative) {
                    TDataStrPtr theRefValue = get<TDataStrPtr>(theRef.m_Value);
                    if (theRefValue) {
                        theData.assign(theRefValue->GetData());
                    }
                }
                theOriginalData = theData;

                if (theData.find(L"Scene") == 0 || theData.size() == 0) {
                    Qt3DSDMInstanceHandle currentInstance = inDynamicInstance;
                    // Resolve this absolute reference string and override the default values
                    // in the datacore to be this exact datatype
                    if (theData.find(L"Scene") == 0) {
                        wstring theItemName;
                        // Walk through the data and attempt to find each object in the asset graph
                        // ignoring slides or anything else.
                        if (theData.size() > 6)
                            theData = theData.substr(6);
                        else
                            theData = L"";
                        currentInstance = m_Doc.GetSceneInstance();
                        while (theData.size() && currentInstance.Valid()) {
                            wstring::size_type thePos = theData.find(L".");
                            if (thePos != wstring::npos) {
                                theItemName = theData.substr(0, thePos);
                                theData = theData.substr(thePos + 1);
                            } else {
                                theItemName = theData;
                                theData = L"";
                            }
                            // Attempt to find the item in the asset graph.
                            long theChildCount = m_AssetGraph.GetChildCount(currentInstance);
                            Qt3DSDMInstanceHandle lastInstance = currentInstance;
                            currentInstance = 0;
                            for (long childIdx = 0;
                                 childIdx < theChildCount && currentInstance.Valid() == false;
                                 ++childIdx) {
                                Qt3DSDMInstanceHandle theChild =
                                    m_AssetGraph.GetChild(lastInstance, childIdx);
                                CString theName(GetName(theChild));
                                if (theName.Compare(theItemName.c_str()))
                                    currentInstance = theChild;
                            }
                        }
                    }

                    if (currentInstance.Valid()) {
                        CId theId(m_Bridge.GetGUID(currentInstance));

                        TGUIDPacked thePackedGuid(theId);
                        qt3dsdm::SLong4 theGuid(thePackedGuid.Data1, thePackedGuid.Data2,
                                              thePackedGuid.Data3, thePackedGuid.Data4);
                        theRef.m_Value = theGuid;
                        // Override the default value with a valid instance.
                        m_DataCore.SetInstancePropertyValue(inDynamic, theInfo.m_Property, theRef);
                    }
                }
            }
        }
    }

    class ISpecificDynamicInstance
    {
    public:
        virtual ~ISpecificDynamicInstance() {}

        virtual Qt3DSDMInstanceHandle GetRootInstance() = 0;
        // returns an error if there was one.  Empty string means no error.
        virtual QString LoadInstanceData(const CFilePath &inAbsPath) = 0;

        virtual std::shared_ptr<IDOMReader>
        ParseInstanceDefinition(const CFilePath &inFullPathToDocument,
                                std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                std::shared_ptr<IImportFailedHandler> inHandler,
                                qt3ds::render::IInputStreamFactory &inInputStreamFactory) = 0;
    };

    virtual TInstanceHandle LoadDynamicInstance(const Q3DStudio::CString &inFullPathToDocument,
                                                TInstanceHandle inParent, TSlideHandle inSlide,
                                                DocumentEditorInsertType::Enum inDropType,
                                                long inStartTime,
                                                ISpecificDynamicInstance &inSpecificInstance,
                                                bool inFinalize)
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        TInstanceHandleList existing;
        m_DataCore.GetInstancesDerivedFrom(existing, inSpecificInstance.GetRootInstance());
        CFilePath theRelativePath(m_Doc.GetRelativePathToDoc(inFullPathToDocument));
        TInstanceHandleList theParents;
        Qt3DSDMInstanceHandle theParentInstance;
        for (size_t idx = 0, end = existing.size(); idx < end && theParentInstance.Valid() == false;
             ++idx) {
            Qt3DSDMInstanceHandle theBehavior(existing[idx]);
            theParents.clear();
            m_DataCore.GetInstanceParents(theBehavior, theParents);
            if (theParents.empty() || theParents[0] != inSpecificInstance.GetRootInstance())
                continue;
            // Ensure this object is *directly* derived from behavior, not indirectly.
            if (theRelativePath.toCString() == GetSourcePath(existing[idx]))
                theParentInstance = existing[idx];
        }

        if (theParentInstance.Valid() == false) {
            std::shared_ptr<IDOMReader> theReaderPtr(inSpecificInstance.ParseInstanceDefinition(
                inFullPathToDocument, m_DataCore.GetStringTablePtr(),
                m_Doc.GetImportFailedHandler(), *m_InputStreamFactory));
            if (theReaderPtr) {
                theParentInstance = m_DataCore.CreateInstance();
                m_DataCore.DeriveInstance(theParentInstance, inSpecificInstance.GetRootInstance());
                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath,
                    std::make_shared<CDataStr>(theRelativePath.toCString()));

                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                    std::make_shared<CDataStr>(theRelativePath.GetFileStem().c_str()));
                std::vector<SMetaDataLoadWarning> theWarnings;
                m_MetaData.LoadInstance(*theReaderPtr, theParentInstance,
                                        theRelativePath.GetFileStem().c_str(), theWarnings);
                QString theLoadError = inSpecificInstance.LoadInstanceData(inFullPathToDocument);
                DisplayLoadWarnings(inFullPathToDocument.toQString(), theWarnings, theLoadError);
            }
        }
        if (theParentInstance.Valid()) {
            TInstanceHandle retval(IDocumentEditor::CreateSceneGraphInstance(
                theParentInstance, inParent, inSlide, m_DataCore, m_SlideSystem,
                m_Bridge.GetObjectDefinitions(), m_AssetGraph, m_MetaData));

            ApplyDynamicMetaData(retval, theParentInstance);
            if (inStartTime != -1)
                SetStartTime(retval, inStartTime);

            // Set unique name
            SetName(retval, GetName(retval), true);

            if (inFinalize)
                return FinalizeAddOrDrop(retval, inParent, inDropType, CPt(), inStartTime == -1);
            return retval;
        }
        return 0;
    }

    struct CScriptDynamicInstanceLoader : public ISpecificDynamicInstance
    {
        CDocEditor &m_Editor;
        CScriptDynamicInstanceLoader(CDocEditor &ed)
            : m_Editor(ed)
        {
        }

        Qt3DSDMInstanceHandle GetRootInstance() override
        {
            return m_Editor.m_Bridge.GetObjectDefinitions().m_Behavior.m_Instance;
        }
        // returns an error if there was one.  Empty string means no error.
        QString LoadInstanceData(const CFilePath &inAbsPath) override
        {
            return m_Editor.LoadScriptFile(inAbsPath);
        }

        virtual std::shared_ptr<IDOMReader>
        ParseInstanceDefinition(const CFilePath &inFullPathToDocument,
                                std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                std::shared_ptr<IImportFailedHandler> inHandler,
                                qt3ds::render::IInputStreamFactory &inInputStreamFactory) override
        {
            return IDocumentEditor::ParseScriptFile(inFullPathToDocument, inStringTable, inHandler,
                                                    inInputStreamFactory);
        }
    };

    TInstanceHandle LoadBehavior(const Q3DStudio::CString &inFullPathToDocument,
                                         TInstanceHandle inParent, TSlideHandle inSlide,
                                         DocumentEditorInsertType::Enum inDropType,
                                         long inStartTime) override
    {
        TInstanceHandle ret;
        if (inFullPathToDocument.Find(".qml") != Q3DStudio::CString::ENDOFSTRING) {
            CScriptDynamicInstanceLoader loader(*this);
            ret = LoadDynamicInstance(inFullPathToDocument, inParent, inSlide,
                                       inDropType, inStartTime, loader, true);
        }
        return ret;
    }

    struct CRenderPluginDynamicInstanceLoader : public ISpecificDynamicInstance
    {
        CDocEditor &m_Editor;
        CRenderPluginDynamicInstanceLoader(CDocEditor &ed)
            : m_Editor(ed)
        {
        }

        Qt3DSDMInstanceHandle GetRootInstance() override
        {
            return m_Editor.m_Bridge.GetObjectDefinitions().m_RenderPlugin.m_Instance;
        }
        // returns an error if there was one.  Empty string means no error.
        QString LoadInstanceData(const CFilePath &) override
        {
            // We would want to ask the render system to possibly load the dll at this point.
            return QString();
        }

        virtual std::shared_ptr<IDOMReader>
        ParseInstanceDefinition(const CFilePath &inFullPathToDocument,
                                std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                std::shared_ptr<IImportFailedHandler> inHandler,
                                qt3ds::render::IInputStreamFactory &inInputStreamFactory) override
        {
            return IDocumentEditor::ParsePluginFile(inFullPathToDocument, inStringTable, inHandler,
                                                    inInputStreamFactory);
        }
    };

    TInstanceHandle LoadRenderPlugin(const Q3DStudio::CString &inFullPathToDocument,
                                             TInstanceHandle inParent, TSlideHandle inSlide,
                                             DocumentEditorInsertType::Enum inDropType,
                                             long inStartTime) override
    {
        CRenderPluginDynamicInstanceLoader loader(*this);
        TInstanceHandle retval = LoadDynamicInstance(inFullPathToDocument, inParent, inSlide,
                                                     inDropType, inStartTime, loader, false);
        // Insert at the beginning.
        if (m_AssetGraph.GetChildCount(inParent) > 1)
            RearrangeObject(retval, m_AssetGraph.GetChild(inParent, 0),
                            DocumentEditorInsertType::PreviousSibling);
        return retval;
    };

    TInstanceHandle CreateText(const Q3DStudio::CString &inFullPathToDocument,
                               TInstanceHandle inParent, TSlideHandle inSlide,
                               DocumentEditorInsertType::Enum inDropType,
                               const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        (void)inStartTime;

        CFilePath theFontFile(inFullPathToDocument);
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());

        if (!theFontFile.IsFile()) {
            if (theHandler)
                theHandler->DisplayImportFailed(theFontFile.toQString(),
                                                QObject::tr("Font File Doesn't Exist"), false);
            return 0;
        }

        // Get the font name of the font file
        QString theFontName = m_Doc.GetProjectFontName(theFontFile);
        if (theFontName.isEmpty()) {
            if (theHandler)
                theHandler->DisplayImportFailed(theFontFile.toQString(),
                                                QObject::tr("Unable to load Font File"), false);
            return 0;
        }

        // Create text instance. Don't set timerange, matching to other
        // objects creation in CBasicObjectDropSource::GenerateAssetCommand()
        qt3dsdm::Qt3DSDMInstanceHandle theTextInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::Text, inParent, inSlide,
                                     TInstanceHandle(), false);

        // Set the Font property to the font file
        m_PropertySystem.SetInstancePropertyValue(theTextInstance, m_Bridge.GetText().m_Font,
                                                  std::make_shared<qt3dsdm::CDataStr>(
                                                  Q3DStudio::CString::fromQString(theFontName)));

        if (inStartTime != -1)
            SetStartTime(theTextInstance, inStartTime);

        // Set the name afterwards, do not do uniqueness check here
        auto handle = FinalizeAddOrDrop(theTextInstance, inParent, inDropType, inPosition,
                                        inStartTime == -1, true, false);
        SetName(handle, ComposerObjectTypes::Convert(ComposerObjectTypes::Text), true);
        // TODO: This should work (QT3DS-2278). The line above is a quick fix in case the actual
        // reason for this to have stopped working is not found in time for 2.1 release.
        //SetName(handle, GetName(handle), true);

        return handle;
    }

    typedef void (IMetaData::*TDynamicObjectLoader)(const char *inShaderFile,
                                                    Qt3DSDMInstanceHandle inInstance,
                                                    const TCharStr &inName,
                                                    std::vector<SMetaDataLoadWarning> &outWarnings,
                                                    qt3ds::foundation::IInStream &stream);

    TInstanceHandle LoadDynamicObject(const Q3DStudio::CString &inFullPathToDocument,
                                      TInstanceHandle inParent, TSlideHandle inSlide,
                                      DocumentEditorInsertType::Enum inDropType, long inStartTime,
                                      TDynamicObjectLoader inLoader,
                                      TInstanceHandle inDerivationParent,
                                      TInstanceHandle inTargetId = TInstanceHandle())
    {
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());

        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        CFilePath theShaderFile(inFullPathToDocument);
        if (theShaderFile.GetExtension() == "nvmpe") {
            // If user drag-drop nvmpe file, we find the corresponding glsl file and use it to load
            // the effect.
            CString shaderFile = theShaderFile.toCString();
            CString newShaderFile = shaderFile.substr(0, shaderFile.Length() - 5);
            newShaderFile.append("glsl");
            theShaderFile = CFilePath(newShaderFile);
        }

        TInstanceHandleList existing;
        m_DataCore.GetInstancesDerivedFrom(existing, inDerivationParent);
        CFilePath theRelativePath(m_Doc.GetRelativePathToDoc(theShaderFile));
        TInstanceHandleList theParents;
        Qt3DSDMInstanceHandle theParentInstance;
        for (size_t idx = 0, end = existing.size(); idx < end && theParentInstance.Valid() == false;
             ++idx) {
            Qt3DSDMInstanceHandle theEffect(existing[idx]);
            theParents.clear();
            m_DataCore.GetInstanceParents(theEffect, theParents);
            if (theParents.empty() || theParents[0] != inDerivationParent)
                continue;
            // Ensure this object is *directly* derived from Effect, not indirectly.
            if (theRelativePath.toCString() == GetSourcePath(existing[idx]))
                theParentInstance = existing[idx];
        }

        if (theParentInstance.Valid() == false) {
            if (theShaderFile.Exists()) {
                theParentInstance = m_DataCore.CreateInstance();
                m_DataCore.DeriveInstance(theParentInstance, inDerivationParent);
                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath,
                    std::make_shared<CDataStr>(theRelativePath.toCString()));

                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                    std::make_shared<CDataStr>(theRelativePath.GetFileStem().c_str()));

                std::vector<SMetaDataLoadWarning> theWarnings;
                QString shaderFile = theShaderFile.toQString();
                NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream(
                    m_InputStreamFactory->GetStreamForFile(shaderFile));
                (m_MetaData.*inLoader)(m_StringTable.GetNarrowStr(theRelativePath.toCString()),
                                       theParentInstance,
                                       theRelativePath.GetFileStem().c_str(),
                                       theWarnings,
                                       *theStream);
                IDocumentEditor::fixDefaultTexturePaths(theParentInstance);
                DisplayLoadWarnings(shaderFile, theWarnings, QString());
            } else {
                if (theHandler)
                    theHandler->DisplayImportFailed(theShaderFile.toQString(),
                                                    QObject::tr("Unable to load Shader File"),
                                                    false);
                return 0;
            }
        }

        TInstanceHandle retval(IDocumentEditor::CreateSceneGraphInstance(
            theParentInstance, inParent, inSlide, m_DataCore, m_SlideSystem,
            m_Bridge.GetObjectDefinitions(), m_AssetGraph, m_MetaData, inTargetId));

        if (inStartTime != -1)
            SetStartTime(retval, inStartTime);

        // Set unique name
        SetName(retval, GetName(retval), true);

        return FinalizeAddOrDrop(retval, inParent, inDropType, CPt(), inStartTime == -1);
    }

    TInstanceHandle LoadEffect(const Q3DStudio::CString &inFullPathToDocument,
                                       TInstanceHandle inParent, TSlideHandle inSlide,
                                       DocumentEditorInsertType::Enum inDropType, long inStartTime) override
    {
        return LoadDynamicObject(inFullPathToDocument, inParent, inSlide, inDropType, inStartTime,
                                 &IMetaData::LoadEffectInstance,
                                 m_Bridge.GetObjectDefinitions().m_Effect.m_Instance);
    }

    TInstanceHandle LoadCustomMaterial(const Q3DStudio::CString &inFullPathToDocument,
                                               TInstanceHandle inParent, TSlideHandle inSlide,
                                               DocumentEditorInsertType::Enum inDropType,
                                               long inStartTime,
                                               TInstanceHandle inTargetId = TInstanceHandle()) override
    {
        return LoadDynamicObject(inFullPathToDocument, inParent, inSlide, inDropType, inStartTime,
                                 &IMetaData::LoadMaterialInstance,
                                 m_Bridge.GetObjectDefinitions().m_CustomMaterial.m_Instance,
                                 inTargetId);
    }

    static void eatspace(const char8_t *str)
    {
        while (!isTrivial(str) && *str == ' ') {
            ++str;
        }
    }

    virtual TInstanceHandle LoadPathBuffer(const Q3DStudio::CString &inFullPathToDocument,
                                           TInstanceHandle inParent, TSlideHandle inSlide,
                                           DocumentEditorInsertType::Enum inDropType,
                                           long inStartTime)
    {
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        QString relPath = m_Doc.GetRelativePathToDoc(inFullPathToDocument);
        TInstanceHandle retval =
            CreateSceneGraphInstance(ComposerObjectTypes::Path, inParent, inSlide);
        Q3DStudio::CFilePath theFilePath(relPath);

        SetName(retval, theFilePath.GetFileStem().GetCharStar(), true);
        CreateSceneGraphInstance(ComposerObjectTypes::Material, retval, inSlide);
        {
            TInstanceHandle strokeMaterial = m_AssetGraph.GetChild(retval, 0);
            SetName(strokeMaterial, L"Stroke");
        }

        qt3dsdm::ISlideCore &theSlideCore(
            *m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetTransactionlessSlideCore());
        theSlideCore.ForceSetInstancePropertyValue(
            inSlide, retval, m_Bridge.GetObjectDefinitions().m_Path.m_PathType,
            TDataStrPtr(new CDataStr(L"Painted")));
        SetInstancePropertyValue(retval, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath,
                                 TDataStrPtr(new CDataStr(CString::fromQString(relPath))));
        FinalizeAddOrDrop(retval, inParent, inDropType, CPt(), inStartTime == -1, false);
        return retval;
    }

    TInstanceHandle ImportFile(DocumentEditorFileType::Enum inFileType,
                                       const Q3DStudio::CString &inFullPathToDocument,
                                       TInstanceHandle inParent, TSlideHandle inSlide,
                                       const Q3DStudio::CString &inImportFileExtension,
                                       DocumentEditorInsertType::Enum inDropType,
                                       const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        switch (inFileType) {
        case DocumentEditorFileType::DAE:
            return ImportDAE(inFullPathToDocument, inParent, inSlide, inImportFileExtension,
                             inDropType, inPosition, inStartTime);
        case DocumentEditorFileType::Image:
            return AutomapImage(inFullPathToDocument, inParent, inSlide, inDropType, inPosition,
                                inStartTime);
        case DocumentEditorFileType::Mesh:
            return LoadMesh(inFullPathToDocument, inParent, inSlide, inDropType, inPosition,
                            inStartTime);
        case DocumentEditorFileType::Import:
            return LoadImportFile(inFullPathToDocument, inParent, inSlide, inDropType, inPosition,
                                  inStartTime);
        case DocumentEditorFileType::Behavior:
            return LoadBehavior(inFullPathToDocument, inParent, inSlide, inDropType, inStartTime);
        case DocumentEditorFileType::Font:
            return CreateText(inFullPathToDocument, inParent, inSlide, inDropType, inPosition,
                              inStartTime);
        case DocumentEditorFileType::Effect:
            return LoadEffect(inFullPathToDocument, inParent, inSlide, inDropType, inStartTime);
        case DocumentEditorFileType::Material:
            return LoadCustomMaterial(inFullPathToDocument, inParent, inSlide, inDropType,
                                      inStartTime);
        default: {
            if (theHandler)
                theHandler->DisplayImportFailed(inFullPathToDocument.toQString(),
                                                QObject::tr("Unsupported Document Editor Type (at this time!)"),
                                                false);
            break;
        }
        }
        return 0;
    }

    void DepthFirstAddImportChildren(TSlideHandle inSlide, TInstanceHandle inInstance,
                                     TIdMultiMap &inMap, std::unordered_set<int> &ioAddedChildren)
    {
        TCharPtr theId = m_StringTable.RegisterStr(GetImportId(inInstance).c_str());
        if (!IsTrivial(theId) && m_SlideSystem.GetAssociatedSlide(inInstance) == inSlide) {
            pair<TIdMultiMap::iterator, bool> theResult =
                inMap.insert(make_pair(theId, vector<pair<TSlideHandle, TInstanceHandle>>()));
            insert_unique(theResult.first->second, make_pair(inSlide, inInstance));
            ioAddedChildren.insert(inInstance);
        }

        for (long idx = 0, end = m_AssetGraph.GetChildCount(inInstance); idx < end; ++idx) {
            TInstanceHandle theInstance = m_AssetGraph.GetChild(inInstance, idx);
            DepthFirstAddImportChildren(inSlide, theInstance, inMap, ioAddedChildren);
        }
    }

    // Precondition is that our source path to instance map
    // has all of the source-path-to-instance hooks already looked up.
    void DoRefreshImport(const CFilePath &inOldFile, const CFilePath &inNewFile,
                         const CFilePath &importFilePath)
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        vector<CFilePath> importFileList;
        if (importFilePath.Exists() && importFilePath.IsFile())
            importFileList.push_back(importFilePath.filePath());

        // Find which import files use this dae file.
        for (TCharPtrToSlideInstanceMap::iterator theIter = m_SourcePathInstanceMap.begin(),
                                                  end = m_SourcePathInstanceMap.end();
             theIter != end; ++theIter) {
            CFilePath theSource(theIter->first);
            if (theSource.GetExtension().Compare(L"import", CString::ENDOFSTRING, false)) {
                CFilePath theFullPath = m_Doc.GetResolvedPathToDoc(theSource);
                if (theFullPath.Exists() && theFullPath.IsFile()) {
                    if (std::find(importFileList.begin(), importFileList.end(),
                                  theFullPath.filePath())
                        == importFileList.end()) {
                        ImportPtrOrError theImport = Import::Load(theFullPath.toCString());
                        if (theImport.m_Value) {
                            CFilePath theSrcFile = CFilePath::CombineBaseAndRelative(
                                CFilePath(theImport.m_Value->GetDestDir()),
                                CFilePath(theImport.m_Value->GetSrcFile()));
                            if (theSrcFile.toCString().Compare(
                                inOldFile.toCString(), false))
                                importFileList.push_back(theFullPath.filePath());
                            theImport.m_Value->Release();
                        }
                    }
                }
            }
        }
        TCharPtrToSlideInstanceMap theImportPaths;
        GetImportPathToInstanceMap(theImportPaths);
        std::unordered_set<int> theAddedInstances;

        // OK, for each import file
        // 1.  Find each group in the system using that import file as its source path.
        // 2.  for each group we find, build a map of import id->item that we will use to
        //     communicate the import changes to the item.
        // 4.  Run the refresh process using a composer editor that runs off of our mappings
        TIdMultiMap theGroupIdMap;
        for (size_t importIdx = 0, end = importFileList.size(); importIdx < end; ++importIdx) {
            theGroupIdMap.clear();
            CFilePath theImportFilePath = importFileList[importIdx];
            CFilePath theImportRelativePath = m_Doc.GetRelativePathToDoc(theImportFilePath);
            TCharPtrToSlideInstanceMap::iterator theIter =
                m_SourcePathInstanceMap.find(m_StringTable.RegisterStr(theImportRelativePath
                                                                       .toCString()));
            if (theIter != m_SourcePathInstanceMap.end()) {
                // First pass just build the group id entries. This avoids us copying hashtables
                // which may be quite expensive
                for (TSlideInstanceList::iterator theSlideInst = theIter->second.begin(),
                                                  theSlideInstEnd = theIter->second.end();
                     theSlideInst != theSlideInstEnd; ++theSlideInst) {
                    TInstanceHandle theRoot = theSlideInst->second;
                    TSlideHandle theSlide = theSlideInst->first;

                    // For a depth first search of all children of this object *in this slide*,
                    // if they have an import id then add them to the map.
                    DepthFirstAddImportChildren(theSlide, theRoot, theGroupIdMap,
                                                theAddedInstances);
                    TIdMultiMap::iterator theGroupId
                        = theGroupIdMap.insert({m_StringTable.GetWideStr(GetImportId(theRoot)),
                                                {}}).first;
                    insert_unique(theGroupId->second, make_pair(theSlide, theRoot));
                    theAddedInstances.insert(theRoot);
                }
                // Since some objects may be completely free standing, we need to go through *all*
                // objects.
                // Unfortunately the first revision of the system didn't put import paths on objects
                // so we need both the above loop *and* to consider every object who's import path
                // matches out import document's relative path.
                theIter = theImportPaths.find(m_StringTable.RegisterStr(theImportRelativePath
                                                                        .toCString()));
                TSlideHandleList theAssociatedSlides;
                if (theIter != theImportPaths.end()) {
                    vector<pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>> &theInstances
                        = theIter->second;
                    for (size_t i = 0, end = theInstances.size(); i < end; ++i) {
                        if (theAddedInstances.find(theInstances[i].second)
                                != theAddedInstances.end()) {
                            continue;
                        }
                        theAssociatedSlides.clear();
                        Qt3DSDMInstanceHandle theInstance(theInstances[i].second);
                        GetAllAssociatedSlides(theInstance, theAssociatedSlides);
                        TIdMultiMap::iterator theInstanceId
                             = theGroupIdMap
                                .insert({m_StringTable.GetWideStr(GetImportId(theInstance)),
                                         {}}).first;
                        for (size_t slideIdx = 0, slideEnd = theAssociatedSlides.size();
                             slideIdx < slideEnd; ++slideIdx) {
                            insert_unique(theInstanceId->second,
                                          make_pair(theAssociatedSlides[slideIdx], theInstance));
                        }
                        theAddedInstances.insert(theInstance);
                    }
                }
            }

            // OK, we have distinct maps sorted on a per-slide basis for all trees of children
            // of this asset.  We now need to attempt to run the refresh algorithm.

            qt3dsimp::ImportPtrOrError theImportPtr
                    = qt3dsimp::Import::Load(theImportFilePath.toCString());
            if (!theImportPtr.m_Value) {
                QT3DS_ASSERT(false);
                continue;
            }

            if (!inNewFile.Exists()) {
                QT3DS_ASSERT(false);
                continue;
            }

            // Select correct translator according to file type
            ITranslator *translator = nullptr;
            STranslationLog *translationLog = nullptr;
            Q3DStudio::CString newExtension(inNewFile.GetExtension());
            Q3DStudio::CString oldExtension(inOldFile.GetExtension());
            if (newExtension.Compare(CDialogs::GetWideDAEFileExtension(),
                Q3DStudio::CString::ENDOFSTRING, false)
                && oldExtension.Compare(CDialogs::GetWideDAEFileExtension(),
                    Q3DStudio::CString::ENDOFSTRING, false)) {
                SColladaTranslator *colladaTranslator
                        = new SColladaTranslator(inNewFile.toQString());
                translationLog = &(colladaTranslator->m_TranslationLog);
                translator = colladaTranslator;
#ifdef QT_3DSTUDIO_FBX
            } else if (newExtension.Compare(CDialogs::GetWideFbxFileExtension(),
                Q3DStudio::CString::ENDOFSTRING, false)
                && oldExtension.Compare(CDialogs::GetWideFbxFileExtension(),
                    Q3DStudio::CString::ENDOFSTRING, false)) {
                SFbxTranslator *fbxTranslator = new SFbxTranslator(inNewFile.toQString());
                translationLog = &(fbxTranslator->m_TranslationLog);
                translator = fbxTranslator;
#endif
            } else {
                STranslationLog emptyLog;
                IDocumentEditor::DisplayImportErrors(inNewFile.toQString(),
                    ImportErrorCodes::TranslationToImportFailed,
                    m_Doc.GetImportFailedHandler(), emptyLog, true);
                continue;
            }

            std::shared_ptr<IComposerEditor> theComposer(
                IComposerEditorInterface::CreateEditorInterface(
                    *this, theGroupIdMap, m_Doc.GetDocumentDirectory(), theImportFilePath, 0,
                    m_StringTable, m_AssetGraph));

            SImportResult theImportResult = CPerformImport::RefreshToComposer(
                *translator, *theComposer, *theImportPtr.m_Value, theImportFilePath);

            IDocumentEditor::DisplayImportErrors(inNewFile.toQString(), theImportResult.m_Error,
                m_Doc.GetImportFailedHandler(),
                *translationLog, false);
        }

        updateMaterialFiles();
    }

    void RefreshImport(const CFilePath &inOldFile, const CFilePath &inNewFile,
                       const CFilePath &importFilePath) override
    {
        CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());

        theDispatch.FireOnProgressBegin(
            QObject::tr("Refreshing Import "), QFileInfo(inNewFile.toQString()).fileName());
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        try {
            m_SourcePathInstanceMap.clear();
            GetSourcePathToInstanceMap(m_SourcePathInstanceMap, true, false);
            DoRefreshImport(inOldFile, inNewFile, importFilePath);
        } catch (...) {
        }

        theDispatch.FireOnProgressEnd();
    }

    bool CleanUpMeshes() override
    {
        CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());
        theDispatch.FireOnProgressBegin(
                    QObject::tr("Old UIP version"), QObject::tr("Cleaning up meshes"));
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        bool cleanedSome = false;
        try {
            vector<CFilePath> importFileList;
            m_SourcePathInstanceMap.clear();
            GetSourcePathToInstanceMap(m_SourcePathInstanceMap, true, false);
            for (TCharPtrToSlideInstanceMap::iterator theIter = m_SourcePathInstanceMap.begin(),
                                                      end = m_SourcePathInstanceMap.end();
                 theIter != end; ++theIter) {
                CFilePath theSource(theIter->first);
                if (theSource.GetExtension().Compare(L"mesh", CString::ENDOFSTRING, false)) {
                    CFilePath theFullPath = m_Doc.GetResolvedPathToDoc(theSource);

                    if (!theFullPath.Exists() || !theFullPath.isFile()
                            || Mesh::GetHighestMultiVersion(theFullPath.toCString().GetCharStar())
                            == 1) {
                        continue;
                    }

                    Mesh *theMesh = Mesh::LoadMulti(
                                theFullPath.toCString().GetCharStar(),
                                Mesh::GetHighestMultiVersion(
                                    theFullPath.toCString().GetCharStar()));

                    if (!theMesh)
                        continue;

                    // Import file still has revisions, so we need to use SaveMulti for saving
                    // the mesh file with correct revision number.
                    // Once import file revisioning has been removed (QT3DS-1815), this can be
                    // replaced with theMesh->Save(theFullPath.toCString().GetCharStar());
                    // It also requires ripping the revisions out from the *.import files
                    Qt3DSFileToolsSeekableMeshBufIOStream output(
                                SFile::Wrap(SFile::OpenForWrite(theFullPath, FileWriteFlags()),
                                            theFullPath));
                    if (!output.IsOpen())
                        QT3DS_ALWAYS_ASSERT_MESSAGE(theFullPath.toCString().GetCharStar());
                    MallocAllocator allocator;
                    theMesh->SaveMulti(allocator, output);

                    delete theMesh;

                    cleanedSome = true;
                }
            }
        } catch (...) {
        }
        theDispatch.FireOnProgressEnd();

        return cleanedSome;
    }

    void ExternalizePath(TInstanceHandle path) override
    {
        CFilePath thePathsDirectory(
            CFilePath::CombineBaseAndRelative(m_Doc.GetDocumentDirectory(), L"paths"));
        thePathsDirectory.CreateDir(true);
        Q3DStudio::CString theName = GetName(path);
        CFilePath theTargetFileName(CFilePath::CombineBaseAndRelative(thePathsDirectory, theName));
        theTargetFileName.setFile(theTargetFileName.filePath() + ".path");
        if (theTargetFileName.Exists()) {
            CString targetFile = theTargetFileName.toCString();
            CFilePath tempPath(targetFile.substr(0, targetFile.size() - 5));
            QT3DSU32 index = 1;
            do {
                wchar_t buffer[64];
                swprintf(buffer, 64, L"%d", index);
                tempPath.setFile(
                    tempPath.filePath() + "_" + QString::fromWCharArray(buffer));
                ++index;
            } while (tempPath.Exists());
            theTargetFileName = tempPath;
            theTargetFileName.setFile(theTargetFileName.filePath() + ".path");
        }
        NVScopedRefCounted<IPathBufferBuilder> theBuilder(
            IPathBufferBuilder::CreateBuilder(*this->m_Foundation.m_Foundation));

        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        TPropertyHandle positionProp(theDefinitions.m_PathAnchorPoint.m_Position.m_Property);
        TPropertyHandle angleProp(theDefinitions.m_PathAnchorPoint.m_IncomingAngle.m_Property);
        TPropertyHandle incomingdistanceProp(
            theDefinitions.m_PathAnchorPoint.m_IncomingDistance.m_Property);
        TPropertyHandle outgoingdistanceProp(
            theDefinitions.m_PathAnchorPoint.m_OutgoingDistance.m_Property);
        TPropertyHandle closedProp(theDefinitions.m_SubPath.m_Closed.m_Property);

        eastl::vector<TInstanceHandle> theSubPathChildren;

        for (QT3DSI32 pathChildIdx = 0, pathChildEnd = m_AssetGraph.GetChildCount(path);
             pathChildIdx < pathChildEnd; ++pathChildIdx) {
            TInstanceHandle pathChild(m_AssetGraph.GetChild(path, pathChildIdx));
            if (GetObjectTypeName(pathChild) == L"SubPath") {
                theSubPathChildren.push_back(pathChild);
                bool isClosed = GetTypedInstancePropertyValue<bool>(pathChild, closedProp);
                TInstanceHandle theLastAnchor;
                for (QT3DSI32 subPathChildIdx = 0,
                           subPathChildEnd = m_AssetGraph.GetChildCount(pathChild);
                     subPathChildIdx < subPathChildEnd; ++subPathChildIdx) {
                    TInstanceHandle theAnchor(m_AssetGraph.GetChild(pathChild, subPathChildIdx));
                    QT3DSVec2 position =
                        ToFnd(GetTypedInstancePropertyValue<SFloat2>(theAnchor, positionProp));
                    if (subPathChildIdx == 0)
                        theBuilder->MoveTo(position);
                    else {
                        QT3DSVec2 prevPos = ToFnd(
                            GetTypedInstancePropertyValue<SFloat2>(theLastAnchor, positionProp));
                        QT3DSF32 prevAngle =
                            GetTypedInstancePropertyValue<float>(theLastAnchor, angleProp) + 180.0f;
                        QT3DSF32 prevDistance = GetTypedInstancePropertyValue<float>(
                            theLastAnchor, outgoingdistanceProp);
                        QT3DSVec2 c1 = IPathManager::GetControlPointFromAngleDistance(
                            prevPos, prevAngle, prevDistance);

                        QT3DSF32 angle = GetTypedInstancePropertyValue<float>(theAnchor, angleProp);
                        QT3DSF32 distance =
                            GetTypedInstancePropertyValue<float>(theAnchor, incomingdistanceProp);
                        QT3DSVec2 c2 = IPathManager::GetControlPointFromAngleDistance(position, angle,
                                                                                   distance);
                        theBuilder->CubicCurveTo(c1, c2, position);
                    }
                    theLastAnchor = theAnchor;
                }
                if (isClosed)
                    theBuilder->Close();
            }
        }
        SPathBuffer theBuffer = theBuilder->GetPathBuffer();
        CFileSeekableIOStream theWriter(theTargetFileName.toCString(), FileWriteFlags());
        theBuffer.Save(theWriter);

        for (QT3DSU32 idx = 0, end = theSubPathChildren.size(); idx < end; ++idx)
            DeleteInstance(theSubPathChildren[idx]);

        CFilePath relativeFileName(
            CFilePath::GetRelativePathFromBase(m_Doc.GetDocumentDirectory(), theTargetFileName));
        SetInstancePropertyValue(path, theDefinitions.m_Asset.m_SourcePath,
                                 TDataStrPtr(new CDataStr(relativeFileName.toCString())));
    }
    static SFloat2 NextDataItem(NVConstDataRef<QT3DSF32> inData, QT3DSU32 &inDataIdx)
    {
        SFloat2 retval(inData[inDataIdx], inData[inDataIdx + 1]);
        inDataIdx += 2;
        return retval;
    }

    static QT3DSF32 ToMinimalAngle(QT3DSF32 inAngle)
    {
        while (inAngle > 360.0f)
            inAngle -= 360.0f;
        while (inAngle < 0.0f)
            inAngle += 360.0f;
        return inAngle;
    }

    void InternalizePath(TInstanceHandle path) override
    {
        Option<TDataStrPtr> thePathOpt =
            GetTypedInstancePropertyValue<TDataStrPtr>(path, m_Bridge.GetSourcePathProperty());
        if (thePathOpt.hasValue() == false || !(*thePathOpt))
            return;
        CFilePath thePathToPathFile = CFilePath::CombineBaseAndRelative(
            m_Doc.GetDocumentDirectory(), (*thePathOpt)->GetData());
        CFileSeekableIOStream theReader(thePathToPathFile.toCString(), FileReadFlags());
        if (theReader.IsOpen() == false)
            return;
        qt3dsimp::SPathBuffer *theLoadedBuffer =
            qt3dsimp::SPathBuffer::Load(theReader, *m_Foundation.m_Foundation);
        if (theLoadedBuffer == NULL)
            return;

        SetInstancePropertyValue(path, m_Bridge.GetSourcePathProperty(),
                                 TDataStrPtr(new CDataStr()), false);

        // Get rid of any existing sub path children.  There shouldn't be any but who knows.
        eastl::vector<TInstanceHandle> theSubPathChildren;

        for (QT3DSI32 pathChildIdx = 0, pathChildEnd = m_AssetGraph.GetChildCount(path);
             pathChildIdx < pathChildEnd; ++pathChildIdx) {
            TInstanceHandle pathChild(m_AssetGraph.GetChild(path, pathChildIdx));
            if (GetObjectTypeName(pathChild) == L"SubPath")
                theSubPathChildren.push_back(pathChild);
        }

        for (QT3DSU32 idx = 0, end = theSubPathChildren.size(); idx < end; ++idx)
            DeleteInstance(theSubPathChildren[idx]);

        QT3DSU32 dataIdx = 0;

        TInstanceHandle theCurrentSubPath;
        TInstanceHandle theCurrentAnchorPoint;
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        TPropertyHandle positionProp(theDefinitions.m_PathAnchorPoint.m_Position.m_Property);
        TPropertyHandle angleProp(theDefinitions.m_PathAnchorPoint.m_IncomingAngle.m_Property);
        TPropertyHandle incomingdistanceProp(
            theDefinitions.m_PathAnchorPoint.m_IncomingDistance.m_Property);
        TPropertyHandle outgoingdistanceProp(
            theDefinitions.m_PathAnchorPoint.m_OutgoingDistance.m_Property);
        TPropertyHandle closedProp(theDefinitions.m_SubPath.m_Closed.m_Property);
        qt3dsdm::ISlideCore &theSlideCore(
            *m_StudioSystem.GetFullSystem()->GetCoreSystem()->GetTransactionlessSlideCore());
        TSlideHandle theCurrentSlide = GetAssociatedSlide(path);
        QT3DSU32 subPathIndex = 0;
        wchar_t theNameBuffer[256];
        QT3DSVec2 theCurrentPosition;

        for (QT3DSU32 idx = 0, end = theLoadedBuffer->m_Commands.size(); idx < end; ++idx) {
            switch (theLoadedBuffer->m_Commands[idx]) {
            case qt3dsimp::PathCommand::MoveTo:
                theCurrentSubPath = CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::SubPath,
                                                             path, theCurrentSlide);
                if (subPathIndex)
                    swprintf(theNameBuffer, 256, L"SubPath_%d", subPathIndex);
                else
                    swprintf(theNameBuffer, 256, L"SubPath");
                SetName(theCurrentSubPath, theNameBuffer);
                theCurrentAnchorPoint =
                    CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::PathAnchorPoint,
                                             theCurrentSubPath, theCurrentSlide);
                SetName(theCurrentAnchorPoint, L"PathAnchorPoint");
                theCurrentPosition = ToFnd(NextDataItem(theLoadedBuffer->m_Data, dataIdx));
                theSlideCore.ForceSetInstancePropertyValue(theCurrentSlide, theCurrentAnchorPoint,
                                                           positionProp,
                                                           ToDataModel(theCurrentPosition));
                ++subPathIndex;
                break;
            case qt3dsimp::PathCommand::CubicCurveTo: {
                QT3DSVec2 c1 = ToFnd(NextDataItem(theLoadedBuffer->m_Data, dataIdx));
                QT3DSVec2 c2 = ToFnd(NextDataItem(theLoadedBuffer->m_Data, dataIdx));
                QT3DSVec2 p2 = ToFnd(NextDataItem(theLoadedBuffer->m_Data, dataIdx));
                QT3DSVec2 outgoing =
                    IPathManager::GetAngleDistanceFromControlPoint(theCurrentPosition, c1);
                outgoing.x += 180.0f;
                outgoing.x = ToMinimalAngle(outgoing.x);
                QT3DSVec2 incoming = IPathManager::GetAngleDistanceFromControlPoint(p2, c2);
                incoming.x = ToMinimalAngle(incoming.x);
                theSlideCore.ForceSetInstancePropertyValue(theCurrentSlide, theCurrentAnchorPoint,
                                                           outgoingdistanceProp, outgoing.y);
                if (fabs(outgoing.y) > .01f) // if the control point is not on the anchor point.
                    theSlideCore.ForceSetInstancePropertyValue(
                        theCurrentSlide, theCurrentAnchorPoint, angleProp, outgoing.x);
                theCurrentPosition = p2;
                theCurrentAnchorPoint =
                    CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::PathAnchorPoint,
                                             theCurrentSubPath, theCurrentSlide);
                SetName(theCurrentAnchorPoint, L"PathAnchorPoint");
                theSlideCore.ForceSetInstancePropertyValue(theCurrentSlide, theCurrentAnchorPoint,
                                                           positionProp,
                                                           ToDataModel(theCurrentPosition));
                theSlideCore.ForceSetInstancePropertyValue(theCurrentSlide, theCurrentAnchorPoint,
                                                           incomingdistanceProp, incoming.y);
                if (fabs(incoming.y) > .01f)
                    theSlideCore.ForceSetInstancePropertyValue(
                        theCurrentSlide, theCurrentAnchorPoint, angleProp, incoming.x);
            } break;
            case qt3dsimp::PathCommand::Close:
                theSlideCore.ForceSetInstancePropertyValue(theCurrentSlide, theCurrentSubPath,
                                                           closedProp, true);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
        theLoadedBuffer->Free(m_Foundation.m_Foundation->getAllocator());
    }

    void ReplaceTextFontNameWithTextFileStem(qt3ds::render::ITextRenderer &inRenderer) override
    {
        TInstanceHandleList theTextInstances;
        m_DataCore.GetInstancesDerivedFrom(theTextInstances, m_Bridge.GetText().m_Instance);
        TSlideHandleList theChildSlides;
        for (size_t idx = 0, end = theTextInstances.size(); idx < end; ++idx) {
            SValue theValue;
            qt3dsdm::Qt3DSDMInstanceHandle theTextHandle(theTextInstances[idx]);
            if (m_DataCore.GetInstancePropertyValue(theTextHandle, m_Bridge.GetText().m_Font,
                                                    theValue)) {
                qt3dsdm::TDataStrPtr theDataStr(qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue));
                if (theDataStr && theDataStr->GetLength()) {

                    Option<CRegisteredString> theNewValueOpt = inRenderer.GetFontNameForFont(
                        m_StringTable.GetNarrowStr(theDataStr->GetData()));
                    if (theNewValueOpt.hasValue()) {
                        CRegisteredString theNewValue(*theNewValueOpt);
                        const wchar_t *theWideValue = m_StringTable.GetWideStr(theNewValue);
                        if (wcscmp(theWideValue, theDataStr->GetData()) != 0)
                            m_DataCore.SetInstancePropertyValue(
                                theTextHandle, m_Bridge.GetText().m_Font,
                                std::make_shared<CDataStr>(theWideValue));
                    }
                }
            }
            qt3dsdm::Qt3DSDMSlideHandle theAssociatedSlide = GetAssociatedSlide(theTextHandle);
            if (theAssociatedSlide.Valid()) {
                theChildSlides.clear();
                m_SlideCore.GetChildSlides(theAssociatedSlide, theChildSlides);
                theChildSlides.insert(theChildSlides.begin(), theAssociatedSlide);
                for (size_t theSlideIdx = 0, theSlideEnd = theChildSlides.size();
                     theSlideIdx < theSlideEnd; ++theSlideIdx) {
                    SValue theSlideValue;
                    if (m_SlideCore.GetSpecificInstancePropertyValue(
                            theChildSlides[theSlideIdx], theTextHandle, m_Bridge.GetText().m_Font,
                            theSlideValue)) {
                        qt3dsdm::TDataStrPtr theDataStr(
                            qt3dsdm::get<qt3dsdm::TDataStrPtr>(theSlideValue));
                        if (theDataStr && theDataStr->GetLength()) {
                            Option<CRegisteredString> theNewValueOpt =
                                inRenderer.GetFontNameForFont(
                                    m_StringTable.GetNarrowStr(theDataStr->GetData()));
                            if (theNewValueOpt.hasValue()) {
                                CRegisteredString theNewValue(*theNewValueOpt);
                                const wchar_t *theWideValue = m_StringTable.GetWideStr(theNewValue);
                                m_SlideCore.ForceSetInstancePropertyValue(
                                    theChildSlides[theSlideIdx], theTextHandle,
                                    m_Bridge.GetText().m_Font,
                                    std::make_shared<CDataStr>(theWideValue));
                            }
                        }
                    }
                }
            }
        }
    }

    void toggleBoolPropertyOnSelected(TPropertyHandle property) override
    {
        qt3dsdm::IPropertySystem *propertySystem = m_Doc.GetStudioSystem()->GetPropertySystem();
        qt3dsdm::TInstanceHandleList selectedInstances
                = m_Doc.GetSelectedValue().GetSelectedInstances();

        if (selectedInstances.size() > 0) {
            bool boolValue = false;
            SValue value;
            qt3dsdm::Qt3DSDMInstanceHandle firstFoundCamera;
            qt3dsdm::Qt3DSDMInstanceHandle foundCameraLayer;
            for (size_t idx = 0, end = selectedInstances.size(); idx < end; ++idx) {
                qt3dsdm::Qt3DSDMInstanceHandle handle(selectedInstances[idx]);
                if (handle.Valid()) {
                    if (value.empty()) {
                        // First valid handle selects if all are hidden/unhidden
                        propertySystem->GetInstancePropertyValue(handle, property, value);
                        boolValue = !qt3dsdm::get<bool>(value);
                    }
                    // First found camera is the one that ends up being activated if there are
                    // several in selection, per-layer. Skip the rest if setting eyeball to true.
                    // It is ok to deactivate (hide) all cameras, though.
                    if (m_DataCore.IsInstanceOrDerivedFrom(
                                handle, m_Bridge.GetObjectDefinitions().m_Camera.m_Instance)) {
                        auto currCameraLayer = m_Bridge.GetResidingLayer(handle);
                        if ((!firstFoundCamera.Valid() || boolValue)
                                && !(foundCameraLayer == currCameraLayer)) {
                            firstFoundCamera = handle;
                            foundCameraLayer = currCameraLayer;
                        } else if (boolValue) {
                            continue;
                        }
                    }
                    SetInstancePropertyValue(handle, property, boolValue);
                }
            }
        }
    }

    void BuildDAEMap(const TFileModificationList &inList)
    {
        for (size_t fileIdx = 0, fileEnd = inList.size(); fileIdx < fileEnd; ++fileIdx) {
            const SFileModificationRecord &theRecord(inList[fileIdx]);
            CString theExtension = theRecord.m_File.GetExtension();
            bool isImport = theExtension.Compare(L"import", CString::ENDOFSTRING, false);
            CFilePath theRelativePath(m_Doc.GetRelativePathToDoc(theRecord.m_File));

            if (theRecord.m_ModificationType == FileModificationType::InfoChanged
                || theRecord.m_ModificationType == FileModificationType::Destroyed) {
                if (isImport)
                    m_ImportFileToDAEMap.erase(theRelativePath.toCString());
                continue;
            }
            if (isImport) {
                qt3dsimp::ImportPtrOrError theImportPtr = qt3dsimp::Import::Load(theRecord.m_File.toCString());
                if (theImportPtr.m_Value) {
                    CFilePath theDestDir = theImportPtr.m_Value->GetDestDir();
                    CFilePath theSrcFile = theImportPtr.m_Value->GetSrcFile();
                    CFilePath theFullSrcPath;
                    if (theSrcFile.isRelative())
                        theFullSrcPath = CFilePath::CombineBaseAndRelative(theDestDir, theSrcFile);
                    else
                        theFullSrcPath = theSrcFile;
                    TCharPtr theDAERelativePath =
                        m_StringTable.RegisterStr(CString::fromQString(
                                                  m_Doc.GetRelativePathToDoc(theFullSrcPath)));
                    pair<unordered_map<TCharPtr, TCharPtr>::iterator, bool> theInsertResult =
                        m_ImportFileToDAEMap.insert(
                            make_pair(m_StringTable.RegisterStr(theRelativePath.toCString()),
                                      theDAERelativePath));
                    theImportPtr.m_Value->Release();
                    if (!theInsertResult.second)
                        theInsertResult.first->second = theDAERelativePath;
                }
            }
        }
    }

    static const char *ModificationTypeToString(FileModificationType::Enum inType)
    {
        switch (inType) {
        case FileModificationType::Created:
            return "Created";
        case FileModificationType::Destroyed:
            return "Destroyed";
        case FileModificationType::InfoChanged:
            return "InfoChanged";
        case FileModificationType::Modified:
            return "Modified";
        case FileModificationType::NoChange:
            return "NoChange";
        default:
            return "Unknown";
        }
    }

    void OnProjectDirChanged(const TFileModificationList &inList)
    {
        if (m_IgnoreDirChange == true) {
            BuildDAEMap(inList);
            return;
        }
        CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());
        bool hasProgressFired = false;
        bool hasDispatchNotificationScope = false;
        bool requestRender = false;

        if (inList.size() == 1
            && m_Doc.GetDocumentPath().endsWith(inList[0].m_File.GetFileName().toQString())
            && inList[0].m_ModificationType == FileModificationType::Modified) {
            if (!m_Doc.GetCore()->HasJustSaved()) {
                CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());
                theDispatch.FireOnPresentationModifiedExternally();
                return;
            }
            m_Doc.GetCore()->SetJustSaved(false);
        }

#define ENSURE_PROGRESS                                                                            \
    if (!hasProgressFired) {                                                                       \
        theDispatch.FireOnProgressBegin(QObject::tr("Updating project"), {});                      \
        hasProgressFired = true;                                                                   \
    }

        m_SourcePathInstanceMap.clear();
        GetSourcePathToInstanceMap(m_SourcePathInstanceMap, false);
        TInstanceHandleList theParents;
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());

        QSet<QString> imageLoadSet;

        g_StudioApp.getRenderer().MakeContextCurrent();
        for (size_t fileIdx = 0, fileEnd = inList.size(); fileIdx < fileEnd; ++fileIdx) {
            const SFileModificationRecord &theRecord(inList[fileIdx]);

            QString theExtension = theRecord.m_File.GetExtension().toQString().toLower();
            bool isImport = theExtension == QLatin1String("import");
            CFilePath theRelativePath(m_Doc.GetRelativePathToDoc(theRecord.m_File));
            const wchar_t *theString(
                m_DataCore.GetStringTable().RegisterStr(theRelativePath.toCString()));

            if (CDialogs::fontExtensions().contains(theExtension)
                && m_Doc.GetSceneGraph() && m_Doc.GetSceneGraph()->GetTextRenderer()) {
                m_Doc.GetSceneGraph()->GetTextRenderer()->ReloadFonts();
                if (m_Doc.GetSceneGraph()->GetDistanceFieldRenderer())
                    m_Doc.GetSceneGraph()->GetDistanceFieldRenderer()->ReloadFonts();
                CFilePath thePath = m_Doc.GetDocumentDirectory();
                CFilePath theFontCache = CFilePath::CombineBaseAndRelative(thePath, L"fontcache");
                theFontCache.DeleteThisDirectory(true);
            }

            if (theRecord.m_ModificationType == FileModificationType::InfoChanged
                || theRecord.m_ModificationType == FileModificationType::Destroyed) {
                if (isImport)
                    m_ImportFileToDAEMap.erase(theRelativePath.toCString());
                continue;
            }

            QDir modifiedPath = QDir::cleanPath(QString::fromWCharArray(theString));
            TCharPtrToSlideInstanceMap::iterator theFind = m_SourcePathInstanceMap.end();
            for (TCharPtrToSlideInstanceMap::iterator it = m_SourcePathInstanceMap.begin();
                 it != m_SourcePathInstanceMap.end(); ++it) {
                QDir sourcePath = QDir::cleanPath(QString::fromWCharArray(it->first));
                if (sourcePath == modifiedPath) {
                    theFind = it;
                    break;
                }
            }

            if (theFind == m_SourcePathInstanceMap.end())
                continue;

            const TSlideInstanceList theInstances(theFind->second);
            if (theRecord.m_ModificationType != FileModificationType::Created) {
                requestRender = true;
                m_Doc.GetBufferCache().InvalidateBuffer(theRelativePath);
            }

            qCInfo(qt3ds::TRACE_INFO) << "Change detected: " << theRelativePath.toQString() << " "
                      << ModificationTypeToString(theRecord.m_ModificationType);

            if (isImport) {
                qt3dsimp::ImportPtrOrError theImportPtr = qt3dsimp::Import::Load(theRecord.m_File.toCString());
                if (theImportPtr.m_Value) {
                    ENSURE_PROGRESS;
                    CFilePath theDestDir = theImportPtr.m_Value->GetDestDir();
                    CFilePath theSrcFile = theImportPtr.m_Value->GetSrcFile();
                    CFilePath theFullSrcPath =
                        CFilePath::CombineBaseAndRelative(theDestDir, theSrcFile);
                    TCharPtr theDAERelativePath =
                        m_StringTable.RegisterStr(CString::fromQString(
                                                  m_Doc.GetRelativePathToDoc(theFullSrcPath)));
                    pair<unordered_map<TCharPtr, TCharPtr>::iterator, bool> theInsertResult =
                        m_ImportFileToDAEMap.insert(
                            make_pair(m_StringTable.RegisterStr(theRelativePath.toCString()),
                                      theDAERelativePath));
                    theImportPtr.m_Value->Release();
                    if (theInsertResult.second == false)
                        theInsertResult.first->second = theDAERelativePath;
                }
            } else if (CDialogs::behaviorExtensions().contains(theExtension)
                       && theRecord.m_ModificationType != FileModificationType::Created
                       && !theInstances.empty()) {
                // First, refresh the parent behavior.
                if (!hasDispatchNotificationScope) {
                    theDispatch.FireBeginDataModelNotifications();
                    hasDispatchNotificationScope = true;
                }

                for (size_t instIdx = 0, instEnd = theInstances.size(); instIdx < instEnd;
                     ++instIdx) {
                    ENSURE_PROGRESS;
                    Qt3DSDMInstanceHandle theBehavior = theInstances[instIdx].second;
                    theParents.clear();
                    m_DataCore.GetInstanceParents(theBehavior, theParents);

                    if (theParents.empty()
                        || theParents[0] != theDefinitions.m_Behavior.m_Instance) {
                        // This indicates we are dealing with a scene instance.
                        // In this case we want to clear any intermediate cached
                        // values from the instance itself so that new defaults
                        // in the file will show through to the UI.
                        m_DataCore.RemoveCachedValues(theBehavior);
                    } else {
                        std::shared_ptr<IDOMReader> theReaderPtr;
                        theReaderPtr = ParseScriptFile(theRecord.m_File,
                                                       m_DataCore.GetStringTablePtr(),
                                                       m_Doc.GetImportFailedHandler(),
                                                       *m_InputStreamFactory);
                        if (!theReaderPtr) {
                            // then effectively no change...
                            QT3DS_ASSERT(false);
                        } else {
                            std::vector<SMetaDataLoadWarning> theWarnings;
                            m_MetaData.LoadInstance(*theReaderPtr, theBehavior,
                                                    theRelativePath.GetFileStem().c_str(),
                                                    theWarnings);
                            CScriptDynamicInstanceLoader inSpecificInstance(*this);
                            QString theLoadError =
                                inSpecificInstance.LoadInstanceData(theRecord.m_File);
                            DisplayLoadWarnings(theRecord.m_File.toQString(),
                                                theWarnings, theLoadError);
                        }
                    }
                }
            } else if (CDialogs::effectExtensions().contains(theExtension)
                       && theRecord.m_ModificationType != FileModificationType::Created
                       && !theInstances.empty()) {
                std::vector<SMetaDataLoadWarning> theWarnings;
                NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream(
                    m_InputStreamFactory->GetStreamForFile(theRecord.m_File.toQString()));
                if (theStream) {
                    m_MetaData.LoadEffectInstance(m_StringTable.GetNarrowStr(
                                                      theRelativePath.toCString()),
                                                  theInstances[0].second,
                                                  theRelativePath.GetFileStem().c_str(),
                                                  theWarnings, *theStream);
                    IDocumentEditor::fixDefaultTexturePaths(theInstances[0].second);
                }

                QList<qt3dsdm::Qt3DSDMInstanceHandle> insts;
                for (size_t i = 0; i < theInstances.size(); ++i) {
                    theDispatch.FireReloadEffectInstance(theInstances[i].second);
                    insts.append(theInstances[i].second);
                }
                theDispatch.FireImmediateRefreshInstance(&insts[0], theInstances.size());

            } else if (CDialogs::shaderExtensions().contains(theExtension)
                       && theRecord.m_ModificationType != FileModificationType::Created
                       && !theInstances.empty()) {
                std::vector<SMetaDataLoadWarning> theWarnings;
                NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream(
                    m_InputStreamFactory->GetStreamForFile(theRecord.m_File.toQString()));
                if (theStream) {
                    m_MetaData.LoadMaterialInstance(m_StringTable.GetNarrowStr(
                                                        theRelativePath.toCString()),
                                                    theInstances[0].second,
                                                    theRelativePath.GetFileStem().c_str(),
                                                    theWarnings,
                                                    *theStream);
                    IDocumentEditor::fixDefaultTexturePaths(theInstances[0].second);
                }

                for (size_t i = 0; i < theInstances.size(); ++i)
                    theDispatch.fireReloadMaterialInstance(theInstances[i].second);
            } else if (CDialogs::mapExtensions().contains(theExtension)
                       && theRecord.m_ModificationType != FileModificationType::Created
                       && !theInstances.empty()) {
                imageLoadSet.insert(theRecord.m_File.toQString());
            }
            // There used to be an extension here for meshes
            // but that causes the product to delete materials in some cases which loses work.
            // so that experiment failed and we will just have to let the users manually update
            // their meshes through the dropdown if they need them updated.
        }


        if (!imageLoadSet.isEmpty()) {
            m_Doc.GetBufferCache().reloadImageSet(
                        imageLoadSet,
                        m_Doc.GetCore()->GetStudioProjectSettings()->getFlipCompressedTextures());
        }

        g_StudioApp.getRenderer().ReleaseContext();

        if (hasProgressFired)
            theDispatch.FireOnProgressEnd();
        if (requestRender && m_Doc.GetSceneGraph())
            m_Doc.GetSceneGraph()->RequestRender();
        if (hasDispatchNotificationScope)
            theDispatch.FireEndDataModelNotifications();
    }
};
}

void IDocumentEditor::DisplayImportErrors(const QString &inImportSource,
                                          ImportErrorCodes::Enum inImportError,
                                          std::shared_ptr<IImportFailedHandler> inHandler,
                                          STranslationLog &inTranslationLog, bool inForceError)
{
    bool isError = false;
    Q3DStudio::CString resultDialogStr;
    std::shared_ptr<IImportFailedHandler> theHandler(inHandler);
    if (inImportError == ImportErrorCodes::TranslationToImportFailed || inForceError) {
        isError = true;
        resultDialogStr = "Failed to import file";
    }

    for (size_t idx = 0; idx < inTranslationLog.m_Warnings.size(); ++idx) {
        const std::pair<ESceneGraphWarningCode, Q3DStudio::CString> &warning(
            inTranslationLog.m_Warnings[idx]);
        const wchar_t *formatStr = L"Unrecognized warning: \"%ls\"";
        switch (warning.first) {
        case ESceneGraphWarningCode_OnlySupportTriangles:
            formatStr = L"Model \"%ls\" contains geometric elements other than triangles";
            break;
        case ESceneGraphWarningCode_TrianglesDuplicateSemantic:
            formatStr = L"Triangle contains duplicate semantics, ex: 1 triangle has multiple "
                        L"TEXCOORD (multiple UV maps)";
            break;
        case ESceneGraphWarningCode_MissingSourceFile:
            formatStr = L"Couldn't find a source image file \"%ls\"";
            break;
        case ESceneGraphWarningCode_LockedDestFile:
            formatStr = L"An image or mesh file \"%ls\" is not writeable";
            break;
        case ESceneGraphWarningCode_VertexBufferTooLarge:
            formatStr = L"A single mesh exceeds the maximum vertex count of 65535";
            break;
        case ESceneGraphWarningCode_MissingMaterial:
            formatStr = L"Materials are missing from mesh \"%ls\", it was not created";
            break;
        case ESceneGraphWarningCode_UnsupportedLight:
            formatStr = L"Light type for \"%ls\" is not supported, it was converted to Directional";
            break;
        case ESceneGraphWarningCode_Rotations:
            formatStr = L"Rotation issue: %ls";
            break;
        case ESceneGraphWarningCode_UnknownMaterial:
            formatStr = L"Unknown material type(s) encountered. Some material properties may be "
                        "incorrect. Make sure your FBX export settings are correct.";
            break;
        default:
            break;
        }

        wchar_t buf[1024] = { 0 };
        swprintf(buf, 1024, formatStr, warning.second.c_str());
        if (resultDialogStr.size())
            resultDialogStr.append('\n');
        resultDialogStr.append(buf);
    }
    if (resultDialogStr.size()) {
        if (theHandler)
            theHandler->DisplayImportFailed(inImportSource, resultDialogStr.toQString(), !isError);
    }
}

Qt3DSDMPropertyHandle *
IDocumentEditor::GetAlwaysUnlinkedProperties(qt3dsdm::SComposerObjectDefinitions &inDefs)
{
    SComposerObjectDefinitions &theDefs(inDefs);
    static Qt3DSDMPropertyHandle theProperties[5];
    theProperties[0] = theDefs.m_Asset.m_StartTime;
    theProperties[1] = theDefs.m_Asset.m_EndTime;
    theProperties[2] = theDefs.m_Asset.m_Eyeball;
    theProperties[3] = theDefs.m_Asset.m_Shy;
    theProperties[4] = Qt3DSDMPropertyHandle();
    return theProperties;
}

// Fixes the default texture paths loaded from material and effect to be presentation relative
void IDocumentEditor::fixDefaultTexturePaths(Qt3DSDMInstanceHandle instance)
{
    const auto core = g_StudioApp.GetCore();
    const QDir docDir(core->GetDoc()->GetDocumentDirectory());
    const QDir projDir = core->getProjectFile().getProjectPath();
    const auto propertySystem = core->GetDoc()->GetStudioSystem()->GetPropertySystem();
    qt3dsdm::TPropertyHandleList propList;
    SValue value;

    propertySystem->GetAggregateInstanceProperties(instance, propList);
    for (auto &prop : propList) {
        qt3dsdm::AdditionalMetaDataType::Value additionalMetaDataType
                = propertySystem->GetAdditionalMetaDataType(instance, prop);
        if (additionalMetaDataType == AdditionalMetaDataType::Texture) {
            propertySystem->GetInstancePropertyValue(instance, prop, value);
            TDataStrPtr strPtr = get<TDataStrPtr>(value);
            const QString strValue = QString::fromWCharArray(strPtr->GetData());
            const QString docRelative = docDir.relativeFilePath(strValue);
            if (!QFileInfo(docDir.filePath(docRelative)).exists()) {
                // Convert path to presentation relative
                const QVariant newVarValue = QVariant::fromValue(
                            docDir.relativeFilePath(projDir.absoluteFilePath(strValue)));
                const SValue newValue = newVarValue;
                propertySystem->SetInstancePropertyValue(instance, prop, newValue);
            }
        }
    }
}

void IDocumentEditor::UnlinkAlwaysUnlinkedProperties(Qt3DSDMInstanceHandle inInstance,
                                                     SComposerObjectDefinitions &inDefs,
                                                     ISlideSystem &inSlideSystem)
{
    Qt3DSDMPropertyHandle *theUnlinked(GetAlwaysUnlinkedProperties(inDefs));
    for (Qt3DSDMPropertyHandle *theHandle = theUnlinked; theHandle->Valid(); ++theHandle)
        inSlideSystem.UnlinkProperty(inInstance, *theHandle);
}

// static
Qt3DSDMInstanceHandle IDocumentEditor::CreateSceneGraphInstance(
    const wchar_t *inType, TInstanceHandle inParent, TSlideHandle inSlide,
    qt3dsdm::IDataCore &inDataCore, qt3dsdm::ISlideSystem &inSlideSystem,
    qt3dsdm::SComposerObjectDefinitions &inObjectDefs, Q3DStudio::CGraph &inAssetGraph,
    qt3dsdm::IMetaData &inMetaData, TInstanceHandle inTargetId, bool setTimeRange,
    bool selectCreatedInstance)
{
    return CreateSceneGraphInstance(inMetaData.GetCanonicalInstanceForType(inType), inParent,
                                    inSlide, inDataCore, inSlideSystem, inObjectDefs, inAssetGraph,
                                    inMetaData, inTargetId, selectCreatedInstance);
}

// static
Qt3DSDMInstanceHandle IDocumentEditor::CreateSceneGraphInstance(
    Qt3DSDMInstanceHandle inMaster, TInstanceHandle inParent, TSlideHandle inSlide,
    qt3dsdm::IDataCore &inDataCore, qt3dsdm::ISlideSystem &inSlideSystem,
    qt3dsdm::SComposerObjectDefinitions &inObjectDefs, Q3DStudio::CGraph &inAssetGraph,
    qt3dsdm::IMetaData &inMetaData, TInstanceHandle inTargetId, bool selectCreatedInstance)
{
    Option<TCharStr> theTypeOpt = inMetaData.GetTypeForInstance(inMaster);
    if (theTypeOpt.hasValue() == false)
        return 0;

    SComposerObjectDefinitions &theDefs(inObjectDefs);
    TInstanceHandle retval = inDataCore.CreateInstance(inTargetId);
    TInstanceHandle theDerivationParent(inMaster);
    inDataCore.DeriveInstance(retval, theDerivationParent);

    // Clear file id derived from the parent
    // This happens for custom shaders and causes id conflicts (QT3DS-4018)
    inDataCore.SetInstancePropertyValue(retval, inObjectDefs.m_Asset.m_FileId,
                                        std::make_shared<CDataStr>(L""));

    if (inParent.Valid())
        inAssetGraph.AddChild(inParent, retval);
    else
        inAssetGraph.AddRoot(retval);

    if (inSlide.Valid()) {
        inSlideSystem.AssociateInstanceWithSlide(inSlide, retval);
        UnlinkAlwaysUnlinkedProperties(retval, inObjectDefs, inSlideSystem);
    }

    Q3DStudio::CId theId;
    if (ComposerObjectTypes::Convert(theTypeOpt->wide_str()) == qt3dsdm::ComposerObjectTypes::Scene)
        theId = SCENE_GUID;
    else
        theId.Generate();

    TGUIDPacked thePackedGuid(theId);
    SLong4 theLong4Id(thePackedGuid.Data1, thePackedGuid.Data2, thePackedGuid.Data3,
                      thePackedGuid.Data4);
    inDataCore.SetInstancePropertyValue(retval, theDefs.m_Guided.m_GuidProp, theLong4Id);
    return retval;
}

std::shared_ptr<IDOMReader>
IDocumentEditor::ParseScriptFile(const CFilePath &inFullPathToDocument,
                                 std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                 std::shared_ptr<IImportFailedHandler> inHandler,
                                 qt3ds::render::IInputStreamFactory &inInputStreamFactory)
{
    using namespace ScriptParser;
    std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SImportXmlErrorHandler theXmlErrorHandler(inHandler, inFullPathToDocument.toQString());
    std::shared_ptr<IDOMReader> theReaderPtr(
        SScriptParser::ParseScriptFile(theFactory, inStringTable,
                                       inFullPathToDocument.toQString(),
                                       theXmlErrorHandler, inInputStreamFactory));

    if (!theReaderPtr) {
        QT3DS_ASSERT(false);
        if (inHandler) {
            inHandler->DisplayImportFailed(inFullPathToDocument.toQString(),
                                           QObject::tr("Failed to parse script data"),
                                           false);
        }
    }
    return theReaderPtr;
}

std::shared_ptr<IDOMReader>
IDocumentEditor::ParsePluginFile(const Q3DStudio::CFilePath &inFullPathToDocument,
                                 std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                 std::shared_ptr<IImportFailedHandler> inHandler,
                                 qt3ds::render::IInputStreamFactory &inInputStreamFactory)
{
    std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SImportXmlErrorHandler theXmlErrorHandler(inHandler, inFullPathToDocument.toQString());

    std::shared_ptr<IDOMReader> theReaderPtr = CRenderPluginParser::ParseFile(
        theFactory, theStringTable, theStringTable->GetNarrowStr(inFullPathToDocument.toCString()),
        theXmlErrorHandler, inInputStreamFactory);
    if (!theReaderPtr) {
        QT3DS_ASSERT(false);
        if (inHandler)
            inHandler->DisplayImportFailed(inFullPathToDocument.toQString(),
                                           QObject::tr("Failed to parse plugin file"),
                                           false);
    }
    CRenderPluginParser::NavigateToMetadata(theReaderPtr);
    return theReaderPtr;
}

std::shared_ptr<IDOMReader>
IDocumentEditor::ParseCustomMaterialFile(const Q3DStudio::CFilePath &inFullPathToDocument,
                                         std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                         std::shared_ptr<IImportFailedHandler> inHandler,
                                         qt3ds::render::IInputStreamFactory &inInputStreamFactory)
{
    std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SImportXmlErrorHandler theXmlErrorHandler(inHandler, inFullPathToDocument.toQString());

    std::shared_ptr<IDOMReader> theReaderPtr = CRenderPluginParser::ParseFile(
        theFactory, theStringTable, theStringTable->GetNarrowStr(inFullPathToDocument.toCString()),
        theXmlErrorHandler, inInputStreamFactory);
    if (!theReaderPtr) {
        QT3DS_ASSERT(false);
        if (inHandler)
            inHandler->DisplayImportFailed(inFullPathToDocument.toQString(),
                                           QObject::tr("Failed to parse material file"),
                                           false);
    }
    CCustomMaterialParser::NavigateToMetadata(theReaderPtr);
    return theReaderPtr;
}

ScopedDocumentEditor::ScopedDocumentEditor(IDoc &inDoc, const QString &inCommandName,
                                           const char *inFile, int inLine)
    : m_Editor(inDoc.OpenTransaction(inCommandName, inFile, inLine))
{
}

CUpdateableDocumentEditor::~CUpdateableDocumentEditor()
{
    if (HasEditor()) {
        qCWarning(qt3ds::WARNING) << m_File << "(" << m_Line
                                  << "): Document editor committed upon destruction";
        CommitEditor();
    }
}

IDocumentEditor &CUpdateableDocumentEditor::EnsureEditor(const QString &inCommandName,
                                                         const char *inFile, int inLine)
{
    if (!HasEditor()) {
        m_File = inFile;
        m_Line = inLine;
    }
    return m_EditorIDocDoc.maybeOpenTransaction(inCommandName, inFile, inLine);
}

bool CUpdateableDocumentEditor::HasEditor() const
{
    return m_EditorIDocDoc.isTransactionOpened() && m_File != NULL;
}

void CUpdateableDocumentEditor::FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle *inInstances,
                                                     long inInstanceCount)
{
    m_EditorIDocDoc.GetCore()->GetDispatch()->FireImmediateRefreshInstance(inInstances,
                                                                           inInstanceCount);
}

void CUpdateableDocumentEditor::CommitEditor()
{
    if (HasEditor()) {
        m_EditorIDocDoc.closeTransaction();
        m_File = NULL;
    }
}

void CUpdateableDocumentEditor::RollbackEditor()
{
    if (HasEditor()) {
        m_EditorIDocDoc.rollbackTransaction();
        m_EditorIDocDoc.closeTransaction();
        m_File = NULL;
    }
}

std::shared_ptr<IInternalDocumentEditor> IInternalDocumentEditor::CreateEditor(CDoc &doc)
{
    return std::make_shared<CDocEditor>(doc);
}
