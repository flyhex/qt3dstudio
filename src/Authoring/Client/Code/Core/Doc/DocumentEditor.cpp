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
#include "StudioCoreSystem.h"
#include "IDocumentBufferCache.h"
#include "Qt3DSImportMesh.h"
#include "Qt3DSDMSlideGraphCore.h"
#include "IComposerEditorInterface.h"
#include "Qt3DSDMXML.h"
#include "IComposerSerializer.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "Qt3DSDMMetaData.h"

#include "Q3DSInputStreamFactory.h"

#include "foundation/Qt3DSMemoryBuffer.h"
#include "IDirectoryWatchingSystem.h"
#include "Qt3DSDMActionCore.h"
#include "PresentationFile.h"
#include "ActionSystem.h"
#include "StandardExtensions.h"

#include "IDocSceneGraph.h"

#include "foundation/Qt3DSFoundation.h"
#include "Q3DStudioNVFoundation.h"
#include "Qt3DSDMGuides.h"

#include "Qt3DSImportPath.h"
#include "Dialogs.h"
#include "foundation/Qt3DSLogging.h"
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdir.h>
#include <unordered_set>
#include "DataModelObjectReferenceHelper.h"
#include "q3dsruntime2api_p.h"
#include <QtCore/qbuffer.h>
#include "IObjectReferenceHelper.h"
#include "StudioProjectSettings.h"
#include "StudioApp.h"

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

typedef QSharedPointer<Q3DStudio::IInputStreamFactory> TStreamFactoryPtr;

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
    IAnimationCore &m_AnimationCore;
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
    QHash<QString, QString> m_ImportFileToDAEMap;
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
        , m_AnimationCore(*m_StudioSystem.GetFullSystem()->GetAnimationCore())
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
        , m_InputStreamFactory(Q3DStudio::IInputStreamFactory::Create())
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        IDirectoryWatchingSystem *theSystem(m_Doc.GetDirectoryWatchingSystem());
        if (theSystem) {
            m_ProjectDirWatcher = theSystem->AddDirectory(m_Doc.GetCore()->getProjectFile()
                                                          .getProjectPath(),
                                        std::bind(&CDocEditor::OnProjectDirChanged, this,
                                                  std::placeholders::_1));
        }
        // Qml types have been registered by the Q3DSEngine already.
        m_InputStreamFactory->addSearchDirectory(m_Doc.GetCore()->getProjectFile()
                                                 .getProjectPath());
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
                long theViewTime = theSlideInfo.m_ComponentMilliseconds;
                return eyeballVal && theStart <= theViewTime && theEnd > 0 && theEnd >= theViewTime;
            }
        }
        return false;
    }

    TPropertyHandle FindProperty(Qt3DSDMInstanceHandle instance,
                                 const QString &inPropName) const override
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

    QString GetObjectTypeName(TInstanceHandle instance) const override
    {
        if (IsInstance(instance)) {
            Option<QString> theTypeName = m_MetaData.GetTypeForInstance(instance);
            if (theTypeName.hasValue())
                return theTypeName;
        }
        return {};
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
                outPaths.push_back(make_pair(Qt3DSDMSlideHandle(0), theStr->toQString()));
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
                    outPaths.push_back(make_pair(theSlide, theStr->toQString()));
            }
        }
    }

    void GetAllSourcePaths(Qt3DSDMInstanceHandle inInstance,
                           TSlideStringList &outPaths) const override
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        GetAllPaths(inInstance, theDefinitions.m_Asset.m_SourcePath, outPaths);
    }

    void GetPathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap,
                              qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                              bool inIncludeIdentifiers = true) const
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        TInstanceHandleList existing;
        TSlideStringList thePaths;
        m_DataCore.GetInstancesDerivedFrom(existing, theDefinitions.m_Asset.m_Instance);
        outInstanceMap.clear();
        for (size_t idx = 0, end = existing.size(); idx < end; ++idx) {
            Qt3DSDMInstanceHandle theAsset(existing[idx]);
            thePaths.clear();
            GetAllPaths(theAsset, inProperty, thePaths);

            for (size_t pathIdx = 0, pathEnd = thePaths.size(); pathIdx < pathEnd; ++pathIdx) {
                const pair<qt3dsdm::Qt3DSDMSlideHandle, QString> &theSlideStr(
                    thePaths[pathIdx]);
                QString thePath(theSlideStr.second);
                if (inIncludeIdentifiers == false)
                    thePath = CFilePath::stripIdentifier(thePath);
                if (!outInstanceMap.contains(thePath))
                    outInstanceMap.insert(thePath, TSlideInstanceList());
                insert_unique(outInstanceMap[thePath],
                              make_pair(theSlideStr.first, theAsset));
            }
        }
    }

    void GetSourcePathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap,
                                            bool inIncludeIdentifiers = true) const override
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        GetPathToInstanceMap(outInstanceMap, theDefinitions.m_Asset.m_SourcePath,
                             inIncludeIdentifiers);
    }

    void GetImportPathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap) const override
    {
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());
        GetPathToInstanceMap(outInstanceMap, theDefinitions.m_Asset.m_ImportFile, false);
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

    QString GetImportId(TInstanceHandle inInstance) const override
    {
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(
                inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_ImportId, theValue)) {
            TDataStrPtr theStr(get<TDataStrPtr>(theValue));
            if (theStr)
                return theStr->toQString();
        }
        return {};
    }

    QString GetFileId(TInstanceHandle inInstance) const override
    {
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(
                inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_FileId, theValue)) {
            TDataStrPtr theStr(get<TDataStrPtr>(theValue));
            if (theStr)
                return theStr->toQString();
        }
        return {};
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
            } else {
                outChildren.push_back(theChild);
            }
        }
    }

    bool IsInSceneGraph(TInstanceHandle child) const override { return m_AssetGraph.IsExist(child); }

    // If the path has any sub-path children, then yes it is externalizeable.
    bool IsPathExternalizeable(TInstanceHandle path) const override
    {
        for (QT3DSI32 idx = 0, end = m_AssetGraph.GetChildCount(path); idx < end; ++idx) {
            TInstanceHandle theChild = m_AssetGraph.GetChild(path, idx);
            if (GetObjectTypeName(theChild) == QLatin1String("SubPath"))
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
                         const QString &propName, long subIndex) override
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
        return m_AnimationCore.GetAnimation(inSlide, instance, propHdl, subIndex).Valid();
    }

    bool IsAnimationArtistEdited(TSlideHandle inSlide, Qt3DSDMInstanceHandle instance,
                                 const QString &propName, long subIndex) override
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
            m_AnimationCore.GetAnimation(inSlide, instance, propHdl, subIndex);
        if (animHandle.Valid() == false)
            return false;
        return m_AnimationCore.IsArtistEdited(animHandle);
    }

    pair<std::shared_ptr<qt3dsdm::IDOMWriter>, CFilePath>
    DoCopySceneGraphObject(const TInstanceHandleList &inInstances)
    {
        if (inInstances.empty())
            return pair<std::shared_ptr<qt3dsdm::IDOMWriter>, CFilePath>();

        std::shared_ptr<IDOMWriter> theWriter(m_Doc.CreateDOMWriter());
        TInstanceHandleList theInstances = ToGraphOrdering(inInstances);
        m_Doc.CreateSerializer()->SerializeSceneGraphObjects(*theWriter, theInstances,
                                                             GetActiveSlide(inInstances[0]));
        CFilePath theFile = WriteWriterToFile(*theWriter, L"SceneGraph");
        return make_pair(theWriter, theFile);
    }

    // Not exposed through public interface yet
    std::shared_ptr<qt3dsdm::IDOMReader>
    CopySceneGraphObjectsToMemory(const qt3dsdm::TInstanceHandleList &instanceList)
    {
        return DoCopySceneGraphObject(instanceList).first->CreateDOMReader();
    }

    // Exposed through document reader interface
    virtual std::shared_ptr<qt3dsdm::IDOMReader>
    CopySceneGraphObjectToMemory(Qt3DSDMInstanceHandle inInstance) override
    {
        TInstanceHandleList instanceList;
        instanceList.push_back(inInstance);
        return CopySceneGraphObjectsToMemory(instanceList);
    }

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

            CDOMSerializer::Write(*inWriter.GetTopElement(), *theFile->m_OpenFile.data());
        }
        return theFinalPath;
    }

    CFilePath CopySceneGraphObjects(TInstanceHandleList inInstances) override
    {
        if (inInstances.empty())
            return L"";
        bool shouldCopy = true;
        for (size_t idx = 0, end = inInstances.size(); idx < end && shouldCopy; ++idx)
            shouldCopy = IsInstance(inInstances[idx]);

        if (!shouldCopy)
            return L"";

        return DoCopySceneGraphObject(inInstances).second;
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

    QSharedPointer<QFile> openQFileStream(const Q3DStudio::CString &inFile,
                                          FileOpenFlags flags) const
    {
        return openQFileStream(inFile.toQString(), flags);
    }

    QSharedPointer<QFile> openQFileStream(const QString &inFile, FileOpenFlags flags) const
    {
        QFile::OpenMode qflags = QFile::ReadOnly;
        bool open = flags & FileOpenFlagValues::Open;
        bool create = flags & FileOpenFlagValues::Create;
        if (flags & FileOpenFlagValues::Truncate)
            qflags |= QFile::Truncate;
        if (flags & FileOpenFlagValues::Write)
            qflags |= QFile::WriteOnly;
        if (flags & FileOpenFlagValues::Append)
            qflags |= QFile::Append;

        QSharedPointer<QFile> file(new QFile(inFile));
        if (open && !file->exists())
            return {};
        if (create && file->exists())
            return {};
        if (!file->open(qflags))
            return {};

        return file;
    }

    void ParseSourcePathsOutOfEffectFile(const QString &inFile,
                                         std::vector<QString> &outFilePaths) override
    {
        QSharedPointer<QFile> theStream(openQFileStream(inFile, FileReadFlags()));
        if (!theStream.isNull()) {
            std::shared_ptr<IDOMFactory> theFactory =
                IDOMFactory::CreateDOMFactory(m_DataCore.GetStringTablePtr());
            SImportXmlErrorHandler theImportHandler(m_Doc.GetImportFailedHandler(), inFile);
            qt3dsdm::SDOMElement *theElem =
                CDOMSerializer::Read(*theFactory, *theStream, &theImportHandler);

            QFileInfo theFilePath(inFile);
            QString theFileDir(theFilePath.canonicalPath());
            if (theElem) {
                std::shared_ptr<IDOMReader> theReader = IDOMReader::CreateDOMReader(
                    *theElem, m_DataCore.GetStringTablePtr(), theFactory);
                if (theReader->MoveToFirstChild(QStringLiteral("MetaData"))) {
                    for (bool success = theReader->MoveToFirstChild(QStringLiteral("Property"));
                         success;
                         success = theReader->MoveToNextSibling(QStringLiteral("Property"))) {
                        QString type;
                        QString defValue;
                        theReader->Att(QStringLiteral("type"), type);
                        theReader->Att(QStringLiteral("default"), defValue);
                        if (type == QLatin1String("Texture")) {
                           QFileInfo theDefPath(CFilePath::CombineBaseAndRelative(
                                                    theFileDir, defValue));
                            if (theDefPath.isFile())
                                outFilePaths.push_back(defValue);
                        }
                    }
                }
            }
        }
    }

    QString GetCustomMaterialName(const QString &inFullPathToFile) const override
    {
        QString retval;
        QSharedPointer<QFile> theStream(openQFileStream(inFullPathToFile, FileReadFlags()));
        if (!theStream.isNull()) {
            std::shared_ptr<IDOMFactory> theFactory =
                IDOMFactory::CreateDOMFactory(m_DataCore.GetStringTablePtr());
            SImportXmlErrorHandler theImportHandler(m_Doc.GetImportFailedHandler(),
                                                    inFullPathToFile);
            qt3dsdm::SDOMElement *theElem =
                CDOMSerializer::Read(*theFactory, *theStream, &theImportHandler);
            if (theElem) {
                // OK, then this just may be a valid material file.  Get the file stem of the path.
                QFileInfo thePath(inFullPathToFile);
                retval = thePath.baseName();
                std::shared_ptr<IDOMReader> theReader = IDOMReader::CreateDOMReader(
                    *theElem, m_DataCore.GetStringTablePtr(), theFactory);
                QString formalName;
                if (theReader->Att(QStringLiteral("formalName"), formalName)
                        && !formalName.isEmpty()) {
                    retval = formalName;
                }
            }
        }
        return retval;
    }

    void getMaterialInfo(const QString &inFullPathToFile,
                         QString &outName, QMap<QString, QString> &outValues,
                         QMap<QString, QMap<QString, QString>> &outTextureValues) override
    {
        QSharedPointer<QFile> theStream(openQFileStream(inFullPathToFile, FileReadFlags()));
        if (!theStream.isNull()) {
            std::shared_ptr<IDOMFactory> theFactory =
                IDOMFactory::CreateDOMFactory(m_DataCore.GetStringTablePtr());
            SImportXmlErrorHandler theImportHandler(m_Doc.GetImportFailedHandler(),
                                                    inFullPathToFile);
            qt3dsdm::SDOMElement *theElem =
                CDOMSerializer::Read(*theFactory, *theStream, &theImportHandler);
            if (theElem) {
                outName = QFileInfo(inFullPathToFile).completeBaseName();
                std::shared_ptr<IDOMReader> theReader = IDOMReader::CreateDOMReader(
                    *theElem, m_DataCore.GetStringTablePtr(), theFactory);

                for (bool success = theReader->MoveToFirstChild(QStringLiteral("Property"));
                     success;
                     success = theReader->MoveToNextSibling(QStringLiteral("Property"))) {
                    QString name;
                    QString value;
                    theReader->Att(QStringLiteral("name"), name);
                    theReader->Value(value);
                    outValues[name] = value;
                }

                if (theReader->GetElementName() == QLatin1String("Property"))
                    theReader->Leave();

                for (bool texSuccess = theReader->MoveToFirstChild(QStringLiteral("TextureData"));
                     texSuccess;
                     texSuccess = theReader->MoveToNextSibling(QStringLiteral("TextureData"))) {
                    QMap<QString, QString> texValues;
                    QString texName;
                    theReader->Att(QStringLiteral("name"), texName);
                    for (bool success = theReader->MoveToFirstChild(QStringLiteral("Property"));
                         success;
                         success = theReader->MoveToNextSibling(QStringLiteral("Property"))) {
                        QString name;
                        QString value;
                        theReader->Att(QStringLiteral("name"), name);
                        theReader->Value(value);
                        texValues[name] = value;
                    }
                    outTextureValues[texName] = texValues;

                    if (theReader->GetElementName() == QLatin1String("Property"))
                        theReader->Leave();
                }

                outValues[QStringLiteral("name")] = outName;
            }
        }
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
    void Rollback() override { m_Doc.RollbackTransaction(); }
    // Release when finished editing
    void Release() override { m_Doc.CloseTransaction(); }

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
    CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Enum inType, TInstanceHandle inParent,
                             TSlideHandle inSlide, TInstanceHandle inTargetId = TInstanceHandle(),
                             bool setTimeRange = true) override
    {
        Qt3DSDMInstanceHandle retval = IDocumentEditor::CreateSceneGraphInstance(
            ComposerObjectTypes::Convert(inType), inParent, inSlide, m_DataCore, m_SlideSystem,
            m_Bridge.GetObjectDefinitions(), m_AssetGraph, m_MetaData, inTargetId, setTimeRange);
        if (setTimeRange)
            SetTimeRangeToParent(retval);
        return retval;
    }

    TInstanceHandle CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Enum inType,
                                                     TInstanceHandle inParent, TSlideHandle inSlide,
                                                     DocumentEditorInsertType::Enum inInsertType,
                                                     const CPt &inPosition,
                                                     EPrimitiveType inPrimitiveType,
                                                     long inStartTime,
                                                     bool setTimeRange = true) override
    {
        TInstanceHandle retval(CreateSceneGraphInstance(inType, inParent, inSlide,
                                                        TInstanceHandle(), setTimeRange));

        QString theName;
        if (inType == ComposerObjectTypes::Model) {
            QString theSourcePath = m_Doc.GetBufferCache().GetPrimitiveName(inPrimitiveType);
            if (!theSourcePath.isEmpty()) {
                // Trigger material generation.
                SetInstancePropertyValue(retval,
                                         m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath,
                                         std::make_shared<CDataStr>(theSourcePath));
                theName = theSourcePath.right(theSourcePath.length() - 1);
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
                retval, m_Bridge.GetObjectDefinitions().m_SlideOwner.m_Instance))
            m_Bridge.GetOrCreateGraphRoot(retval);
        // if we did not set time range earlier, let's set it now to match parent
        TInstanceHandle handle = FinalizeAddOrDrop(retval, inParent, inInsertType,
                                                   inPosition, !setTimeRange, true, false);
        SetName(retval, theName, true);
        return handle;
    }

    QString GetSourcePath(Qt3DSDMInstanceHandle inInstance)
    {
        Option<SValue> theValue = GetInstancePropertyValue(
            inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath);
        if (theValue.hasValue()) {
            TDataStrPtr theStr(get<TDataStrPtr>(*theValue));
            if (theStr)
                return theStr->toQString();
        }
        return {};
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
            QString theSourcePath(GetSourcePath(instance));
            TInstanceHandleList theParents;
            Qt3DSDMInstanceHandle theInstanceParent;
            m_DataCore.GetInstanceParents(instance, theParents);
            for (size_t idx = 0; idx < theParents.size(); ++idx) {
                Qt3DSDMInstanceHandle theParent(theParents[idx]);
                if (m_DataCore.IsInstanceOrDerivedFrom(theParent, theObjectDefInstance)
                    && theParent != theObjectDefInstance
                    && theSourcePath == GetSourcePath(theParent)) {
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

        int numSubsets = 0;
        for (size_t propIdx = 0, propEnd = theValues.size(); propIdx < propEnd; ++propIdx) {
            TDataStrPtr newValue(get<TDataStrPtr>(theValues[propIdx].second));
            Q3DSRenderMesh *theBuffer = m_Doc.GetBufferCache().GetOrCreateModelBuffer(
                newValue->toQString());
            if (theBuffer)
                numSubsets = qMax(numSubsets, theBuffer->m_subsets.size());
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
            setMaterialReferenceByName(theMaterials.back(), "Default");
            setMaterialSourcePath(theMaterials.back(), "Default");
        }

        // Now go through and if possible ensure that on each slide the name of the material matches
        // the subset name.
        for (size_t propIdx = 0, propEnd = theValues.size(); propIdx < propEnd; ++propIdx) {
            TDataStrPtr newValue(get<TDataStrPtr>(theValues[propIdx].second));
            Q3DSRenderMesh *theBuffer = m_Doc.GetBufferCache().GetOrCreateModelBuffer(
                newValue->toQString());
            if (theBuffer == NULL)
                continue;
            for (long subsetIdx = 0, subsetEnd = theBuffer->m_subsets.size(); subsetIdx < subsetEnd;
                 ++subsetIdx) {
                const QString &theSubsetName = theBuffer->m_subsetNames[subsetIdx];
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
                            && get<TDataStrPtr>(theValue)->toQString() != theSubsetName)
                            m_SlideCore.ForceSetInstancePropertyValue(
                                theSlide, theMaterial, theNameProp,
                                std::make_shared<CDataStr>(theSubsetName));
                    } else if (m_DataCore.GetInstancePropertyValue(theMaterial, theNameProp,
                                                                   theDMValue)
                                   == false
                               || get<TDataStrPtr>(theDMValue)->toQString() != theSubsetName) {
                        m_DataCore.SetInstancePropertyValue(
                            theMaterial, theNameProp,
                            std::make_shared<CDataStr>(theSubsetName));
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
        QString materialName = isStroke ? QStringLiteral("Stroke") : QStringLiteral("Fill");
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
                        if (GetObjectTypeName(child) == QLatin1String("SubPath"))
                            subPathChildren.push_back(child);
                    }
                    for (QT3DSU32 idx = 0, end = subPathChildren.size(); idx < end; ++idx)
                        DeleteInstance(subPathChildren[idx]);
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

            QString docPath(m_Doc.GetDocumentPath());
            QString docDir(QFileInfo(docPath).absolutePath());
            STranslationLog log;
            QString theFullPathToDocument(
                m_Doc.GetResolvedPathToDoc(QFileInfo(newValue->toQString())));
            QFileInfo info(theFullPathToDocument);
            if (newValue && *newValue->GetData() && info.exists()) {
                std::pair<long, long> times = GetTimeRange(instance);
                DoImport(info, theFullPathToDocument,
                         m_AssetGraph.GetParent(instance), instance, theNewSlide, docDir, log,
                         std::bind(CPerformImport::ImportToComposerFromImportFile,
                                   std::placeholders::_1, std::placeholders::_2),
                         DocumentEditorInsertType::Unknown, CPt(), times.first);
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
        if (propName == m_Bridge.GetObjectDefinitions().m_Material.m_SpecularReflection)
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TextureMapping,
                                     std::make_shared<CDataStr>(L"Environmental Mapping"), false);
        else if (propName == m_Bridge.GetObjectDefinitions().m_Layer.m_LightProbe
                 || propName == m_Bridge.GetObjectDefinitions().m_Layer.m_LightProbe2)
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TextureMapping,
                                     std::make_shared<CDataStr>(L"Light Probe"), false);
        else if (propName == m_Bridge.GetObjectDefinitions().m_MaterialBase.m_IblProbe)
            SetInstancePropertyValue(theImageInstance,
                                     m_Bridge.GetObjectDefinitions().m_Image.m_TextureMapping,
                                     std::make_shared<CDataStr>(L"IBL Override"), false);
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
                                                    const QString &inSourcePath) override
    {
        QString thePath = m_Doc.GetResolvedPathToDoc(inSourcePath);
        QFileInfo info(thePath);
        assert(info.isFile());
        if (!info.isFile())
            return 0;
        Qt3DSDMSlideHandle theSlide(GetAssociatedSlide(instance));

        TInstanceHandle theImageInstance =
            CreateImageInstanceForMaterialOrLayer(instance, propName);
        TDataStrPtr thePtrPath(new CDataStr(inSourcePath));
        SetInstancePropertyValue(instance, propName, thePtrPath);

        return theImageInstance;
    }

    virtual TInstanceHandle
    SetInstancePropertyValueAsRenderable(TInstanceHandle instance, TPropertyHandle propName,
                                         const QString &inSourcePath) override
    {
        QString thePath = m_Doc.GetResolvedPathToDoc(inSourcePath);
        QFileInfo info(thePath);
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
                                         std::make_shared<qt3dsdm::CDataStr>(inSourcePath),
                                         true);
                // Setting the parent image property to empty will delete the image child,
                // so we should skip setting the property there
                if (inSourcePath.isEmpty())
                    parentEmptied = true;
            }
            if (!parentEmptied) {
                SetInstancePropertyValue(instance, propName,
                                         std::make_shared<qt3dsdm::CDataStr>(inSourcePath),
                                         true);
            }
        } else if (m_Bridge.IsLayerInstance(instance)
                   && m_Bridge.GetSourcePathProperty() == propName
                   && !inSourcePath.isEmpty()) {
            // Resize the layer to be the size of the presentation

            QSize presSize(g_StudioApp.getRenderableSize(inSourcePath));
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
                        ->GetPresentationSize().x;
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
                        ->GetPresentationSize().y;
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
                                     std::make_shared<qt3dsdm::CDataStr>(inSourcePath),
                                     true);
        } else {
            SetInstancePropertyValue(instance, propName,
                                     std::make_shared<qt3dsdm::CDataStr>(inSourcePath),
                                     true);
        }

        // If this is a render plugin
        if (info.exists() && info.suffix() == QLatin1String("plugin")) {
            Qt3DSDMSlideHandle theSlide(GetAssociatedSlide(instance));
            return LoadRenderPlugin(info.canonicalFilePath(), instance, theSlide,
                                    DocumentEditorInsertType::LastChild, -1);
        }
        return TInstanceHandle();
    }

    /**
     * Sets a subpresentation for an instance's property that accepts a texture (image) as its value
     * If there is no texture exists, a new one is created. Next, the texture subpresentation
     *  property is set to the pId param
     * @param instance  the instance
     * @param prop  the image property
     * @param pId   the presentation Id to set for the texture
     */
    void setInstanceImagePropertyValueAsRenderable(TInstanceHandle instance, TPropertyHandle prop,
                                                   const QString &pId) override
    {
        Qt3DSDMPropertyHandle img = GetImageInstanceForProperty(instance, prop);

        if (!img)
            img = CreateImageInstanceForMaterialOrLayer(instance, prop);

        SetInstancePropertyValueAsRenderable(img, m_Bridge.getSubpresentationProperty(), pId);
    }

    /**
     * Create a scene rect and set its material's texture from the provided subpresentation Id
     *
     * @param pId the presentation Id to set for the texture
     * @param slide the slide to add to
     * @param pos add position in the scene
     * @param startTime add at this start time
     */
    void addRectForSubpresentation(const QString &pId, TSlideHandle slide, const CPt &pos = CPt(),
                                   long startTime = -1) override
    {
        qt3dsdm::Qt3DSDMPropertyHandle activeLayer = m_Doc.GetActiveLayer();
        Qt3DSDMInstanceHandle rectInstance =
                CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Model, activeLayer, slide,
                                         Q3DStudio::DocumentEditorInsertType::LastChild, pos,
                                         PRIMITIVETYPE_RECT, startTime, false);

        // Set the subpresentation for the rect's material's diffuseMap
        const long childCount = m_AssetGraph.GetChildCount(rectInstance);
        for (long i = 0; i < childCount; ++i) {
            Qt3DSDMInstanceHandle matInstance(m_AssetGraph.GetChild(rectInstance, i));

            if (m_Bridge.IsMaterialBaseInstance(matInstance)) {
                if (m_Bridge.IsReferencedMaterialInstance(matInstance))
                    SetMaterialType(matInstance, "Standard Material");

                auto prop = m_Bridge.GetObjectDefinitions().m_Material.m_DiffuseMap1.m_Property;
                setInstanceImagePropertyValueAsRenderable(matInstance, prop, pId);
                break;
            }
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
        QString theName = GetName(instance);
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
        if (inRelativePathToMaterialFile == "Standard Material")
            newMaterial =
                CreateSceneGraphInstance(ComposerObjectTypes::Material, model, theSlide, instance);
        else if (inRelativePathToMaterialFile == "Referenced Material")
            newMaterial = CreateSceneGraphInstance(ComposerObjectTypes::ReferencedMaterial, model,
                                                   theSlide, instance);
        else {
            QString thePath = m_Doc.GetResolvedPathToDoc(QFileInfo(inRelativePathToMaterialFile));
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

    QString getMaterialPath(const QString &materialName) const
    {
        return m_Doc.GetCore()->getProjectFile().getProjectPath()
                + QStringLiteral("/materials/")
                + materialName + QStringLiteral(".matdata");
    }

    QString writeMaterialFile(Qt3DSDMInstanceHandle instance,
                              const QString &materialName,
                              bool createNewFile,
                              const QString &sourcePath = {}) override
    {
        EStudioObjectType type = m_Bridge.GetObjectType(instance);

        if (type == EStudioObjectType::OBJTYPE_MATERIAL
            || type == EStudioObjectType::OBJTYPE_CUSTOMMATERIAL) {
            QString actualSourcePath = sourcePath;
            if (actualSourcePath.isEmpty())
                actualSourcePath = getMaterialPath(materialName);

            QFileInfo fileInfo(actualSourcePath);
            if (!fileInfo.dir().exists())
                fileInfo.dir().mkdir(QStringLiteral("."));

            QFile file(actualSourcePath);
            if ((createNewFile && !file.exists()) || (!createNewFile && file.exists()))
                saveMaterial(instance, file);
            return m_Doc.GetRelativePathToDoc(QFileInfo(actualSourcePath));
        }

        return {};
    }

    void writeProperty(QFile &file, const QString &name, const QString &value, int indent = 1)
    {
        for (int i = 0; i < indent; ++i)
            file.write("\t");
        file.write("<Property name=\"");
        file.write(name.toUtf8().constData());
        file.write("\">");
        file.write(value.toUtf8().constData());
        file.write("</Property>\n");
    }

    void writeProperty(QFile &file, const QString &name, const SValue &value, int indent = 1)
    {
        MemoryBuffer<RawAllocator> tempBuffer;
        WCharTWriter writer(tempBuffer);
        WStrOps<SValue>().ToBuf(value, writer);
        tempBuffer.write(0);

        if (tempBuffer.size()) {
            writeProperty(file, name,
                          QString::fromWCharArray((const wchar_t *)tempBuffer.begin()), indent);
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

    void saveMaterial(Qt3DSDMInstanceHandle instance, QFile &file)
    {
        SValue value;
        file.open(QIODevice::WriteOnly);
        file.write("<MaterialData version=\"1.0\">\n");
        QMap<QString, Qt3DSDMInstanceHandle> textureHandles;
        qt3dsdm::TPropertyHandleList propList;
        m_PropertySystem.GetAggregateInstanceProperties(instance, propList);
        for (auto &prop : propList) {
            const auto name = m_PropertySystem.GetName(prop);

            if (!isSaveableMaterialProperty(name))
                continue;

            if (m_AnimationSystem.IsPropertyAnimated(instance, prop))
                continue;

            m_PropertySystem.GetInstancePropertyValue(instance, prop, value);

            if (!value.empty()) {
                bool valid = true;
                QString path;
                if (value.getType() == DataModelDataType::Long4) {
                    SLong4 guid = get<qt3dsdm::SLong4>(value);
                    if (guid.Valid()) {
                        auto ref = m_Bridge.GetInstanceByGUID(guid);
                        textureHandles[name] = ref;
                        path = m_Bridge.GetSourcePath(ref);
                    } else {
                        valid = false;
                    }
                }

                if (path.isEmpty() && valid)
                    writeProperty(file, name, value);
                else
                    writeProperty(file, name, path);
            }
        }

        writeProperty(file, QStringLiteral("presentation"), m_Doc.GetDocumentPath());

        const QFileInfo fileInfo(file);
        writeProperty(file, QStringLiteral("path"), fileInfo.absoluteFilePath());
        writeProperty(file, QStringLiteral("filename"), fileInfo.completeBaseName());

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
                const auto name = m_PropertySystem.GetName(prop);

                if (!isSaveableMaterialProperty(name))
                    continue;

                if (m_AnimationSystem.IsPropertyAnimated(handle, prop))
                    continue;

                m_PropertySystem.GetInstancePropertyValue(handle, prop, value);
                if (!value.empty())
                    writeProperty(file, name, value, 2);
            }
            file.write("\t</TextureData>\n");
        }

        file.write("</MaterialData>");
    }

    bool isMaterialContainer(TInstanceHandle instance) const override
    {
        return instance == getMaterialContainer();
    }

    bool isInsideMaterialContainer(TInstanceHandle instance) const override
    {
        auto parentInstance = GetParent(instance);
        return isMaterialContainer(parentInstance);
    }

    QString getMaterialContainerName() const
    {
        return QStringLiteral("MaterialContainer");
    }

    QString getMaterialContainerParentPath() const
    {
        return GetName(m_Doc.GetSceneInstance());
    }

    QString getMaterialContainerPath() const
    {
        return getMaterialContainerParentPath() + QStringLiteral(".") + getMaterialContainerName();
    }

    Qt3DSDMInstanceHandle getMaterialContainer() const override
    {
        IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
        Qt3DSDMInstanceHandle instance;
        CRelativePathTools::EPathType type;
        objRefHelper->ResolvePath(m_Doc.GetSceneInstance(),
                                  getMaterialContainerPath(),
                                  type, instance, true);
        return instance;
    }

    Qt3DSDMInstanceHandle getOrCreateMaterialContainer()
    {
        auto instance = getMaterialContainer();
        if (!instance.Valid()) {
            IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
            Qt3DSDMInstanceHandle parent;
            CRelativePathTools::EPathType type;
            objRefHelper->ResolvePath(m_Doc.GetSceneInstance(),
                                      getMaterialContainerParentPath(),
                                      type, parent, true);
            if (!parent.Valid())
                parent = m_Doc.GetSceneInstance();
            Qt3DSDMSlideHandle slide = m_Bridge.GetOrCreateGraphRoot(parent);
            instance = CreateSceneGraphInstance(ComposerObjectTypes::Material, parent,
                                                slide, DocumentEditorInsertType::LastChild,
                                                CPt(), PRIMITIVETYPE_UNKNOWN, -1);
            SetName(instance, getMaterialContainerName());
            SetTimeRange(instance, 0, 0);
        }
        return instance;
    }

    Qt3DSDMInstanceHandle getMaterial(const QString &materialName) override
    {
        IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
        QString name = getMaterialContainerPath() + QStringLiteral(".")
                + QString(materialName).replace(QLatin1String("."), QLatin1String("\\."));
        Qt3DSDMInstanceHandle material;
        CRelativePathTools::EPathType type;
        objRefHelper->ResolvePath(m_Doc.GetSceneInstance(),
                                  name.toUtf8().constData(),
                                  type, material, true);
        return material;
    }

    Qt3DSDMInstanceHandle getOrCreateMaterial(const QString &materialName) override
    {
        auto material = getMaterial(materialName);
        if (!material.Valid()) {
            auto parent = getOrCreateMaterialContainer();
            material = CreateSceneGraphInstance(ComposerObjectTypes::Material, parent,
                                                GetAssociatedSlide(parent),
                                                DocumentEditorInsertType::LastChild,
                                                CPt(), PRIMITIVETYPE_UNKNOWN, -1);
            SetName(material, materialName);
        }
        return material;
    }

    void getMaterialReference(TInstanceHandle instance, TInstanceHandle &reference) const override
    {
        auto optValue = GetInstancePropertyValue(instance,
                                 m_Bridge.GetObjectDefinitions().m_ReferencedMaterial
                                                 .m_ReferencedMaterial.m_Property);
        if (optValue.hasValue()) {
            reference = GetInstanceForObjectRef(m_Doc.GetSceneInstance(),
                                                get<SObjectRefType>(optValue.getValue()));
        }
    }

    void setMaterialProperties(TInstanceHandle instance, const QString &materialName,
                               const QString &materialSourcePath,
                               const QMap<QString, QString> &values,
                               const QMap<QString, QMap<QString, QString>> &textureValues) override
    {
        setMaterialReferenceByName(instance, materialName);
        setMaterialSourcePath(instance, materialSourcePath);
        setMaterialValues(materialName, values, textureValues);
    }

    void setMaterialReference(TInstanceHandle instance, TInstanceHandle reference) override
    {
        IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
        SObjectRefType objRef = objRefHelper->GetAssetRefValue(reference, m_Doc.GetSceneInstance(),
                                                               CRelativePathTools::EPATHTYPE_GUID);
        SetInstancePropertyValue(instance,
                                 m_Bridge.GetObjectDefinitions().m_ReferencedMaterial
                                 .m_ReferencedMaterial.m_Property,
                                 objRef, false);
        SetName(instance, GetName(reference));
    }

    void setMaterialReferenceByName(TInstanceHandle instance,
                                    const QString &materialName) override
    {
        Qt3DSDMInstanceHandle material = getOrCreateMaterial(materialName);
        IObjectReferenceHelper *objRefHelper = m_Doc.GetDataModelObjectReferenceHelper();
        SObjectRefType objRef = objRefHelper->GetAssetRefValue(material, m_Doc.GetSceneInstance(),
                                                               CRelativePathTools::EPATHTYPE_GUID);
        SetInstancePropertyValue(instance,
                                 m_Bridge.GetObjectDefinitions().m_ReferencedMaterial
                                 .m_ReferencedMaterial.m_Property,
                                 objRef, false);
    }

    void setMaterialSourcePath(TInstanceHandle instance,
                               const QString &materialSourcePath) override
    {
        SetInstancePropertyValue(instance, m_Bridge.GetSceneAsset().m_SourcePath,
                                 std::make_shared<CDataStr>(materialSourcePath));
    }

    void setMaterialValues(const QString &materialName,
                           const QMap<QString, QString> &values,
                           const QMap<QString, QMap<QString, QString>> &textureValues) override
    {
        auto instance = getOrCreateMaterial(materialName);
        if (instance.Valid())
            setMaterialValues(instance, values, textureValues);
    }

    struct ChildInstance
    {
        QString name;
        TInstanceHandle handle;
    };

    QVector<ChildInstance> setPropertyValues(TInstanceHandle instance,
                                             const QMap<QString, QString> &values)
    {
        QVector<ChildInstance> childInstances;
        QMapIterator<QString, QString> i(values);
        while (i.hasNext()) {
            i.next();

            if (!isSaveableMaterialProperty(i.key()))
                continue;

            QString propName(i.key());
            QString propString = i.value();
            Qt3DSDMPropertyHandle prop
                    = m_PropertySystem.GetAggregateInstancePropertyByName(
                        instance, propName);

            if (m_AnimationSystem.IsPropertyAnimated(instance, prop))
                continue;

            const auto type = m_PropertySystem.GetDataType(prop);
            switch (type) {
            case DataModelDataType::Long4:
            {
                SetInstancePropertyValue(instance, prop,
                                         std::make_shared<CDataStr>(propString));
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
                SetInstancePropertyValue(instance, prop, i.value().toFloat());
                break;
            }
            case DataModelDataType::Float2:
            {
                QStringList floats = i.value().split(QStringLiteral(" "));
                if (floats.length() == 2) {
                    SFloat2 value(floats[0].toFloat(), floats[1].toFloat());
                    SetInstancePropertyValue(instance, prop, value);
                }
                break;
            }
            case DataModelDataType::Float3:
            {
                QStringList floats = i.value().split(QStringLiteral(" "));
                if (floats.length() == 3) {
                    SFloat3 value(floats[0].toFloat(), floats[1].toFloat(), floats[2].toFloat());
                    SetInstancePropertyValue(instance, prop, value);
                }
                break;
            }
            case DataModelDataType::Bool:
            {
                if (propString == "True")
                    SetInstancePropertyValue(instance, prop, true);
                else if (propString == "False")
                    SetInstancePropertyValue(instance, prop, false);
                break;
            }
            case DataModelDataType::String:
            {
                SetInstancePropertyValue(instance, prop, std::make_shared<CDataStr>(propString));
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
                if (values.contains(QStringLiteral("name")))
                    SetName(instance, values[QStringLiteral("name")]);
            }
        }

        const auto childInstances = setPropertyValues(instance, values);

        for (auto &child : childInstances) {
            if (textureValues.contains(child.name))
                setPropertyValues(child.handle, textureValues[child.name]);
        }
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
        const auto srcSlide = m_SlideSystem.GetApplicableSlide(src);
        const auto dstSlide = m_SlideSystem.GetApplicableSlide(dst);
        const auto name = GetName(dst);
        SValue value;
        qt3dsdm::TPropertyHandleList propList;
        m_PropertySystem.GetAggregateInstanceProperties(src, propList);

        for (auto &prop : propList) {
            const auto name = m_PropertySystem.GetName(prop);

            if (!isSaveableMaterialProperty(name))
                continue;

            TInstanceHandle srcChild;
            m_PropertySystem.GetInstancePropertyValue(src, prop, value);
            if (!value.empty() && value.getType() == DataModelDataType::Long4) {
                SLong4 guid = get<qt3dsdm::SLong4>(value);
                if (guid.Valid()) {
                    srcChild = m_Bridge.GetInstanceByGUID(guid);
                    const auto path = std::make_shared<CDataStr>(m_Bridge.GetSourcePath(srcChild));
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
        m_AnimationCore.CopyAnimations(inSourceSlide, inSourceInstance, inDestSlide,
                                       inDestInstance);
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
        m_AnimationCore.OffsetAnimations(m_Doc.GetActiveSlide(), inInstance, inOffset);

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

    void SetTimebarColor(TInstanceHandle inInstance, const QColor &inColor) override
    {
        m_PropertySystem.SetInstancePropertyValue(
            inInstance, m_Bridge.GetSceneAsset().m_TimebarColor,
            qt3dsdm::SFloat3(inColor.redF(), inColor.greenF(),
                           inColor.blueF()));
    }

    void SetTimebarText(TInstanceHandle inInstance, const Q3DStudio::CString &inComment) override
    {
        m_PropertySystem.SetInstancePropertyValue(inInstance,
                                                  m_Bridge.GetSceneAsset().m_TimebarText,
                                                  qt3dsdm::SStringRef(inComment.c_str()));
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
    void AddKeyframes(Qt3DSDMAnimationHandle animHandle, const float *keyframeValues, long numValues,
                      long inOffsetInSeconds)
    {
        long numFloatsPerKeyframe = sizeof(TKeyframeType) / sizeof(float);
        if (numValues % numFloatsPerKeyframe) {
            QT3DS_ASSERT(false);
        }
        const TKeyframeType *keyframes = reinterpret_cast<const TKeyframeType *>(keyframeValues);
        long numKeyframes = numValues / numFloatsPerKeyframe;
        for (long idx = 0; idx < numKeyframes; ++idx) {
            TKeyframeType theData(keyframes[idx]);
            theData.m_KeyframeSeconds += inOffsetInSeconds;
            m_AnimationCore.InsertKeyframe(animHandle, theData);
        }
    }

    void SetKeyframeTime(TKeyframeHandle inKeyframe, long inTime) override
    {
        float theTimeinSecs = static_cast<float>(inTime) / 1000.f;
        // round off to 4 decimal place to workaround precision issues
        // TODO: fix this, either all talk float OR long. choose one.
        theTimeinSecs = ceilf(theTimeinSecs * 10000.0f) / 10000.0f;
        TKeyframe theData = m_AnimationCore.GetKeyframeData(inKeyframe);
        // Function programming paradigm, returns new value instead of changing
        // current value.
        theData = qt3dsdm::SetKeyframeSeconds(theData, theTimeinSecs);
        m_AnimationCore.SetKeyframeData(inKeyframe, theData);
    }

    void DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation) override
    {
        m_AnimationCore.DeleteAllKeyframes(inAnimation);
    }

    void KeyframeProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                  bool inDoDiffValue) override
    {
        m_AnimationSystem.KeyframeProperty(inInstance, inProperty, inDoDiffValue);
    }

    virtual Qt3DSDMAnimationHandle
    CreateOrSetAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle instance,
                         const QString &propName, long subIndex, EAnimationType animType,
                         const float *keyframeValues, long numValues, bool /*inUserEdited*/) override
    {
        Qt3DSDMPropertyHandle propHdl =
            m_DataCore.GetAggregateInstancePropertyByName(instance, propName);
        if (propHdl.Valid() == false) {
            QT3DS_ASSERT(false);
            return 0;
        }
        if (inSlide.Valid() == false) {
            Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(instance);
            if (theSlide.Valid() == false) {
                assert(0);
                return 0;
            }
            if (m_SlideSystem.IsPropertyLinked(instance, propHdl))
                theSlide = m_SlideSystem.GetMasterSlide(theSlide);
            inSlide = theSlide;
        }

        Qt3DSDMAnimationHandle animHandle =
            m_AnimationCore.GetAnimation(inSlide, instance, propHdl, subIndex);

        if (animHandle.Valid() == true)
            m_AnimationCore.DeleteAnimation(animHandle);

        animHandle =
            m_AnimationCore.CreateAnimation(inSlide, instance, propHdl, subIndex, animType, false);

        long theStartTime = GetTimeRange(instance).first;
        long theTimeOffsetInSeconds = long(theStartTime / 1000.f);

        switch (animType) {
        case EAnimationTypeLinear:
            AddKeyframes<SLinearKeyframe>(animHandle, keyframeValues, numValues,
                                          theTimeOffsetInSeconds);
            break;
        case EAnimationTypeBezier:
            AddKeyframes<SBezierKeyframe>(animHandle, keyframeValues, numValues,
                                          theTimeOffsetInSeconds);
            break;
        case EAnimationTypeEaseInOut:
            AddKeyframes<SEaseInEaseOutKeyframe>(animHandle, keyframeValues, numValues,
                                                 theTimeOffsetInSeconds);
            break;
        default:
            QT3DS_ASSERT(false);
            AddKeyframes<SLinearKeyframe>(animHandle, keyframeValues, numValues,
                                          theTimeOffsetInSeconds);
            break;
        }
        return animHandle;
    }
    bool RemoveAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle instance,
                                 const QString &propName, long subIndex) override
    {
        Qt3DSDMPropertyHandle propHdl =
            m_DataCore.GetAggregateInstancePropertyByName(instance, propName);
        if (propHdl.Valid() == false) {
            QT3DS_ASSERT(false);
            return false;
        }
        Qt3DSDMAnimationHandle animHandle =
            m_AnimationCore.GetAnimation(inSlide, instance, propHdl, subIndex);
        if (animHandle.Valid()) {
            m_AnimationCore.DeleteAnimation(animHandle);
            return true;
        }
        return false;
    }

    void SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited = true) override
    {
        m_AnimationCore.SetIsArtistEdited(inAnimation, inEdited);
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

    QString GetName(Qt3DSDMInstanceHandle inInstance) const override
    {
        Option<SValue> theValue = GetInstancePropertyValue(
            inInstance, m_Bridge.GetObjectDefinitions().m_Named.m_NameProp);
        if (theValue.hasValue()) {
            TDataStrPtr theNamePtr(get<TDataStrPtr>(*theValue));
            if (theNamePtr)
                return theNamePtr->toQString();
        }
        return {};
    }

    QString GetSourcePath(Qt3DSDMInstanceHandle inInstance) const override
    {
        Option<SValue> theValue = GetInstancePropertyValue(
            inInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath);
        if (theValue.hasValue()) {
            TDataStrPtr theNamePtr(get<TDataStrPtr>(*theValue));
            if (theNamePtr)
                return theNamePtr->toQString();
        }
        return {};
    }

    TInstanceHandle GetFirstBaseClass(Qt3DSDMInstanceHandle inInstance) const override
    {
        TInstanceHandleList theList;
        m_DataCore.GetInstanceParents(inInstance, theList);
        if (theList.size())
            return theList[0];
        return 0;
    }

    void SetName(Qt3DSDMInstanceHandle inInstance, const QString &inName,
                         bool inMakeUnique = false) override
    {
        QString theUniqueName = inName;
        if (inMakeUnique)
            theUniqueName = m_Bridge.GetUniqueChildName(GetParent(inInstance), inInstance,
                                                        inName);

        SetInstancePropertyValue(inInstance, m_Bridge.GetNameProperty(),
                                 std::make_shared<CDataStr>(theUniqueName), false);
    }

    TInstanceHandleList DoPasteSceneGraphObject(std::shared_ptr<IDOMReader> inReader,
                                                TInstanceHandle inNewRoot,
                                                bool inGenerateUniqueName,
                                                DocumentEditorInsertType::Enum inInsertType,
                                                const CPt &inPosition,
                                                bool notifyRename = true)
    {
        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        TInstanceHandleList retval = theSerializer->SerializeSceneGraphObject(
            *inReader, m_Doc.GetDocumentDirectory(), inNewRoot, GetActiveSlide(inNewRoot));
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

    TInstanceHandleList PasteSceneGraphObject(const QString &inFilePath,
                                                      TInstanceHandle inNewRoot,
                                                      bool inGenerateUniqueName,
                                                      DocumentEditorInsertType::Enum inInsertType,
                                                      const CPt &inPosition) override
    {
        qt3ds::QT3DSI32 theVersion = 0;
        std::shared_ptr<IDOMReader> theReader = m_Doc.CreateDOMReader(
            inFilePath, theVersion);
        if (!theReader)
            return TInstanceHandleList();
        return DoPasteSceneGraphObject(theReader, inNewRoot, inGenerateUniqueName, inInsertType,
                                       inPosition, false);
    }

    virtual TInstanceHandleList
    PasteSceneGraphObjectMaster(const QString &inFilePath, TInstanceHandle inNewRoot,
                                bool inGenerateUniqueName,
                                DocumentEditorInsertType::Enum inInsertType, const CPt &inPosition) override
    {
        qt3ds::QT3DSI32 theVersion = 0;
        std::shared_ptr<IDOMReader> theReader = m_Doc.CreateDOMReader(
            inFilePath, theVersion);
        if (!theReader)
            return TInstanceHandleList();

        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        TInstanceHandleList retval = theSerializer->SerializeSceneGraphObject(
            *theReader, m_Doc.GetDocumentDirectory(), inNewRoot,
            m_Doc.GetStudioSystem()->GetSlideSystem()->GetMasterSlide(GetActiveSlide(inNewRoot)));
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
        QSet<TInstanceHandle> updateList;
        TInstanceHandle theParent(inDest);
        if (inInsertType == DocumentEditorInsertType::PreviousSibling
            || inInsertType == DocumentEditorInsertType::NextSibling)
            theParent = GetParent(inDest);

        if (m_Bridge.IsComponentInstance(theParent)
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
            QString currName = m_Bridge.GetName(theInstance);
            if (checkUniqueName) {
                if (!m_Bridge.CheckNameUnique(theParent, theInstance, currName)) {
                    QString newName = m_Bridge.GetUniqueChildName(theParent, theInstance, currName);
                    if (notifyRename)
                        m_Doc.getMoveRenameHandler()->displayMessageBox(currName, newName);
                    SetName(theInstance, newName);
                }
            }
            TInstanceHandle oldParentHandle = GetParent(theInstance);
            if (inInsertType == DocumentEditorInsertType::PreviousSibling)
                m_AssetGraph.MoveBefore(theInstance, inDest);
            else if (inInsertType == DocumentEditorInsertType::NextSibling)
                m_AssetGraph.MoveAfter(theInstance, inDest);
            else if (inInsertType == DocumentEditorInsertType::LastChild)
                m_AssetGraph.MoveTo(theInstance, inDest, COpaquePosition::LAST);

            // If moving into a group, update the target group pivot point and position
            TInstanceHandle newParentHandle = GetParent(theInstance);
            if (m_Bridge.IsGroupInstance(newParentHandle))
                updateList.insert(newParentHandle);
            // If moving from a group, update the source group pivot and position
            if (m_Bridge.IsGroupInstance(oldParentHandle))
                updateList.insert(oldParentHandle);
        }

        if (!updateList.empty()) {
            for (auto it : qAsConst(updateList)) {
                TInstanceHandleList childHandles;
                GetChildren(GetActiveSlide(it), it, childHandles);
                updatePivotAndPosition(it, childHandles);
            }
        }
    }

    // Move the pivot point and position to the center of the objects in list
    void updatePivotAndPosition(TInstanceHandle handle, TInstanceHandleList objectList)
    {
        // Calculate the center
        SFloat3 pivotPoint;
        for (auto it : objectList) {
            SFloat3 position = GetPosition(it);
            pivotPoint[0] += position[0];
            pivotPoint[1] += position[1];
            pivotPoint[2] += position[2];
        }
        size_t objectCount = objectList.size();
        pivotPoint[0] /= objectCount;
        pivotPoint[1] /= objectCount;
        pivotPoint[2] /= objectCount;
        // Modify the pivot point of the target
        SetInstancePropertyValue(handle, m_Bridge.GetObjectDefinitions().m_Node.m_Pivot,
                                 pivotPoint, false);
        // Modify the position of the target
        SetPosition(handle, pivotPoint);
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
                QString name = GetName(selected) + QLatin1String("@@to_be_deleted@@");
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
        TInstanceHandleList sortedList(ToGraphOrdering(inInstances));

        // Create a new group next to the topmost item in the graph
        TInstanceHandle sibling = sortedList.front();
        Qt3DSDMSlideHandle slide = GetActiveSlide(sibling);
        TInstanceHandle group = CreateSceneGraphInstance(ComposerObjectTypes::Group, sibling, slide,
                                                         DocumentEditorInsertType::PreviousSibling,
                                                         CPt(), PRIMITIVETYPE_UNKNOWN, -1);
        // Move items into the group
        RearrangeObjects(sortedList, group, DocumentEditorInsertType::LastChild, true);
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

        // Update pivot and position
        updatePivotAndPosition(component, inInstances);

        QString theName = GetName(theInstances[0]);

        // now cut the group
        std::shared_ptr<IDOMReader> theReader(CopySceneGraphObjectsToMemory(theInstances));
        DeleteInstances(theInstances);

        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        Qt3DSDMSlideHandle theComponentSlide(m_Bridge.GetComponentActiveSlide(component));

        // Paste into the master slide of the new component
        TInstanceHandleList insertedHandles = theSerializer->SerializeSceneGraphObject(
                    *theReader,m_Doc.GetDocumentDirectory(), component,
                    m_SlideSystem.GetMasterSlide(theComponentSlide));

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
                    m_SlideSystem.GetMasterSlide(theComponentSlide));

        if (insertedHandles.size()) {
            // Restore the original time range for all objects.
            for (int i = 0; i < theStartEndTimes.size(); i++) {
                if (theStartEndTimes.at(i) != std::make_pair(0L, 0L))
                    SetTimeRange(insertedHandles.at(i), theStartEndTimes.at(i).first,
                                 theStartEndTimes.at(i).second);
            }

            // Update pivot and position
            TInstanceHandleList childHandles;
            GetChildren(m_SlideSystem.GetMasterSlide(theComponentSlide), targetComponent,
                        childHandles);
            updatePivotAndPosition(targetComponent, childHandles);

            // Check for name uniqueness
            if (checkUniqueName) {
                for (auto instance : insertedHandles) {
                    QString currName = m_Bridge.GetName(instance);
                    if (!m_Bridge.CheckNameUnique(targetComponent, instance, currName)) {
                        QString newName = m_Bridge.GetUniqueChildName(targetComponent, instance,
                                                                      currName);
                        if (notifyRename)
                            m_Doc.getMoveRenameHandler()->displayMessageBox(currName, newName);
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
        return DoPasteSceneGraphObject(theReader, inDest, true, inInsertType, CPt(), false);
    }

    Qt3DSDMActionHandle AddAction(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                                         const QString &inEvent, const QString &inHandler) override
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

    Qt3DSDMActionHandle PasteAction(const QString &inFilePath,
                                    Qt3DSDMInstanceHandle inNewRoot) override
    {
        QSharedPointer<QFile> theStream(openQFileStream(inFilePath, FileReadFlags()));
        if (!theStream.isNull()) {
            QT3DS_ASSERT(false);
            return 0;
        }
        std::shared_ptr<IDOMFactory> theFactory(
            IDOMFactory::CreateDOMFactory(m_DataCore.GetStringTablePtr()));
        SDOMElement *theElem = CDOMSerializer::Read(*theFactory, *theStream);
        if (theElem == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }
        std::shared_ptr<IDOMReader> theReader(
            IDOMReader::CreateDOMReader(*theElem, m_DataCore.GetStringTablePtr(), theFactory));
        std::shared_ptr<IComposerSerializer> theSerializer = m_Doc.CreateSerializer();
        return theSerializer->SerializeAction(*theReader, inNewRoot, GetActiveSlide(inNewRoot));
    }

    bool ContainsSlideByName(const QString &inName, Qt3DSDMSlideHandle inMasterSlide)
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

    QString GenerateUniqueSlideName(const QString &inStem, Qt3DSDMSlideHandle inMasterSlide,
                                    int inStartIndex)
    {
        size_t theStartIndex = inStartIndex;
        if (theStartIndex < 0)
            theStartIndex = m_SlideSystem.GetSlideCount(inMasterSlide);

        QString baseName = inStem;
        int nameIdx = (int)theStartIndex;
        QString theNameStr = baseName;
        theNameStr.append(QString::number(nameIdx));
        while (ContainsSlideByName(theNameStr, inMasterSlide)) {
            ++nameIdx;
            theNameStr = baseName;
            theNameStr.append(QString::number(nameIdx));
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
        QString theNewName = GenerateUniqueSlideName(QStringLiteral("Slide"), inMasterSlide,
                                                     inIndex);
        Qt3DSDMSlideHandle theNewSlide = m_SlideSystem.DuplicateSlide(inMasterSlide, inIndex);
        Qt3DSDMInstanceHandle newInstance(m_SlideSystem.GetSlideInstance(theNewSlide));
        m_DataCore.SetInstancePropertyValue(newInstance,
                                            m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                                            std::make_shared<CDataStr>(theNewName));
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

        QString theNewName = GenerateUniqueSlideName(QStringLiteral("Slide"), theMaster,
                                                     theIndex + 1);

        Qt3DSDMSlideHandle theNewSlide = theSerializer->SerializeSlide(
            *theReader, m_Doc.GetDocumentDirectory(), theMaster, theIndex);

        Qt3DSDMInstanceHandle newInstance(m_SlideSystem.GetSlideInstance(theNewSlide));
        m_DataCore.SetInstancePropertyValue(newInstance,
                                            m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                                            std::make_shared<CDataStr>(theNewName));

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

        for (auto &instance : children) {
            const auto name = GetName(instance);
            writeMaterialFile(getOrCreateMaterial(name), name, false);
        }
    }

    void updateMaterialInstances(const QStringList &filenames) override
    {
        const auto parent = getMaterialContainer();
        if (parent.Valid()) {
            TInstanceList children;
            GetChildren(GetAssociatedSlide(parent), parent, children);

            for (auto &instance : children) {
                auto name = GetName(instance);
                if (name != QLatin1String("Default") && !filenames.contains(name))
                    DeleteInstance(instance);
            }
        }
    }

    TInstanceHandle DoImport(
        const QFileInfo &inImportFilePath, const QString &importSrc, Qt3DSDMInstanceHandle inParent,
        Qt3DSDMInstanceHandle inRoot, Qt3DSDMSlideHandle inSlide, const QString &inDocDir,
        STranslationLog &inTranslationLog,
        function<SImportResult(IComposerEditorInterface &, QString)> inImportFunction,
        DocumentEditorInsertType::Enum inInsertType, const CPt &inPosition, long inStartTime)
    {
        QString outputDir(inImportFilePath.absolutePath());
        bool alwaysKeepDirectory = !inImportFilePath.path().isEmpty();
        bool keepDirectory = false;
        Qt3DSDMInstanceHandle theRealParent = inInsertType == DocumentEditorInsertType::LastChild
            ? inParent
            : Qt3DSDMInstanceHandle(m_AssetGraph.GetParent(inParent));
        // We have to pass in the real parent to the editor interface so that object lifetimes can
        // be setup correctly as the import tree is being built.
        std::shared_ptr<IComposerEditorInterface> importToComposer =
            IComposerEditorInterface::CreateEditorInterface(
                    *this, theRealParent, inRoot, inSlide, inDocDir,
                    inImportFilePath.absoluteFilePath(), inStartTime);

        CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());
        QString theDestFile(importToComposer->GetDestImportFile());
        try {
            theDispatch.FireOnProgressBegin(QObject::tr("Importing "), importSrc);
            SImportResult result = inImportFunction(*importToComposer, theDestFile);
            bool forceError = importToComposer->HasError();
            if (!forceError)
                importToComposer->Finalize(result.m_FilePath);
            keepDirectory = alwaysKeepDirectory || forceError == false;
            theDispatch.FireOnProgressEnd();
            IDocumentEditor::DisplayImportErrors(importSrc, result.m_Error,
                                                 m_Doc.GetImportFailedHandler(), inTranslationLog,
                                                 forceError);
            if (!forceError) {
                Qt3DSDMInstanceHandle theImportRoot = importToComposer->GetRoot();
                QString theRelPath(m_Doc.GetRelativePathToDoc(QFileInfo(theDestFile)));
                SValue theSourcePathValue(std::make_shared<CDataStr>(theRelPath));
                Qt3DSDMPropertyHandle theProp(m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath);
                if (inSlide.Valid())
                    m_SlideCore.ForceSetInstancePropertyValue(inSlide, theImportRoot, theProp,
                                                              theSourcePathValue);
                else
                    m_DataCore.SetInstancePropertyValue(theImportRoot, theProp, theSourcePathValue);

                // Do not check for unique name as we set it anyway after getting new handle
                Qt3DSDMInstanceHandle retval =
                    FinalizeAddOrDrop(importToComposer->GetRoot(), inParent, inInsertType,
                                      inPosition, inStartTime == -1, true, false);
                SetName(retval, QFileInfo(theRelPath).baseName(), true);

                updateMaterialFiles();

                return retval;
            }
        } catch (...) {
            theDispatch.FireOnProgressEnd();
            m_Doc.RollbackTransaction(); // Run away!!!
        }
        return 0;
    }

    TInstanceHandle ImportDAE(const QString &inFullPathToDocument,
                              TInstanceHandle inParent, TSlideHandle inSlide,
                              const QString &inImportFileExtension,
                              DocumentEditorInsertType::Enum inDropType,
                              const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        // If we already have an import file that points back to this DAE then we need to
        // not import the DAE but import the import file again.

        QFileInfo importSrc(inFullPathToDocument);
        if (importSrc.exists() == false)
            return 0;
        QString theRelativeDAE = m_Doc.GetRelativePathToDoc(importSrc);

        QString docPath(m_Doc.GetDocumentPath());
        QString docDir(QFileInfo(docPath).canonicalPath());
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        if (docPath.size() == 0) {
            if (theHandler) {
                theHandler->DisplayImportFailed(importSrc.filePath(),
                                                QObject::tr("Qt3DSComposer Document Has No Path"),
                                                false);
            }
            return 0;
        }
        if (!importSrc.isFile()) {
            if (theHandler) {
                theHandler->DisplayImportFailed(importSrc.filePath(),
                                                QObject::tr("Source File Doesn't Exist"), false);
            }
            return 0;
        }

        QString fname = importSrc.baseName();

        QString importsDir = CFilePath::CombineBaseAndRelative(docDir, QStringLiteral("Imports"));
        QFileInfo importInfo(importsDir);
        if (!importInfo.exists())
            CFilePath::CreateDir(importsDir, true);

        QDir outputDir = Q3DStudio::SFileTools::FindUniqueDestDirectory(importsDir, fname);
        const QString outputFileName = fname + QLatin1Char('.') + inImportFileExtension;
        SColladaTranslator translator(importSrc.canonicalFilePath());
        TInstanceHandle retval =
            DoImport(CFilePath::CombineBaseAndRelative(outputDir.canonicalPath(), outputFileName),
                     importSrc.absoluteFilePath(),
                     inParent, 0, inSlide, docDir, translator.m_TranslationLog,
                     std::bind(CPerformImport::ImportToComposer, translator,
                               std::placeholders::_1, std::placeholders::_2), inDropType,
                               inPosition, inStartTime);
        if (retval.Valid()) {
            QString theRelativeImport = m_Doc.GetRelativePathToDoc(outputFileName);
            m_ImportFileToDAEMap.insert(theRelativeImport, theRelativeDAE);
        }

        return retval;
    }

    TInstanceHandle LoadImportFile(const QString &inFullPathToDocument,
                                   TInstanceHandle inParent, TSlideHandle inSlide,
                                   DocumentEditorInsertType::Enum inDropType,
                                   const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        QString docPath(m_Doc.GetDocumentPath());
        QString docDir(m_Doc.GetDocumentDirectory());
        STranslationLog log;
        return DoImport(inFullPathToDocument, inFullPathToDocument, inParent, 0, inSlide,
                        docDir, log, std::bind(CPerformImport::ImportToComposerFromImportFile,
                                       std::placeholders::_1, std::placeholders::_2),
                                       inDropType, inPosition, inStartTime);
    }

    TInstanceHandle AutomapImage(const QString &inFullPathToDocument,
                                 TInstanceHandle inParent, TSlideHandle inSlide,
                                 DocumentEditorInsertType::Enum inDropType,
                                 const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        (void)inStartTime;

        QFileInfo imageSrc(inFullPathToDocument);
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());

        if (!imageSrc.isFile()) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.filePath(),
                                                QObject::tr("Image File Doesn't Exist"), false);
            return 0;
        }
        QString relativePath = m_Doc.GetRelativePathToDoc(imageSrc);
        Q3DSImageTextureData theImageBuffer =
            m_Doc.GetBufferCache().GetOrCreateImageBuffer(relativePath);
        if (!theImageBuffer.m_valid) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.filePath(),
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
        qt3dsdm::Qt3DSDMInstanceHandle theMaterialInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::ReferencedMaterial, theModelInstance,
                                                        inSlide);
        CFilePath theFilePath(inFullPathToDocument);
        // Create the reference material
        auto imageMaterial = getOrCreateMaterial(theFilePath.GetFileStem().toQString());
        // Load the image as the child of the reference material
        qt3dsdm::Qt3DSDMInstanceHandle theImageInstance = SetInstancePropertyValueAsImage(
            imageMaterial, m_Bridge.GetDefaultMaterial().m_DiffuseMap1, relativePath);
        auto sourcePath = writeMaterialFile(imageMaterial,
                                            theFilePath.GetFileStem().toQString(), true);

        setMaterialReferenceByName(theMaterialInstance, theFilePath.GetFileStem().toQString());
        setMaterialSourcePath(theMaterialInstance, sourcePath);
        SetName(theMaterialInstance, theFilePath.GetFileStem().toQString());

        if (inStartTime != -1)
            SetStartTime(theModelInstance, inStartTime);

        float theHeight = theImageBuffer.m_height / 100.0f;
        float theWidth = theImageBuffer.m_width/ 100.0f;
        qt3dsdm::SFloat3 theScale = qt3dsdm::SFloat3(2, 2, 1); // Default, per Danc.
        if (!qIsNull(theHeight) && qIsNull(theWidth))
            theScale = qt3dsdm::SFloat3(theWidth, theHeight, 1);
        m_PropertySystem.SetInstancePropertyValue(theModelInstance, m_Bridge.GetNode().m_Scale,
                                                  theScale);

        SetName(theModelInstance, theFilePath.GetFileStem().toQString(), true);

        // Set the image as the property of the first diffuse map.
        return FinalizeAddOrDrop(theModelInstance, inParent, inDropType, inPosition,
                                 inStartTime == -1);
    }

    TInstanceHandle LoadMesh(const QString &inFullPathToDocument,
                             TInstanceHandle inParent, TSlideHandle inSlide,
                             DocumentEditorInsertType::Enum inDropType,
                             const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        QFileInfo imageSrc(inFullPathToDocument);
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        if (!imageSrc.isFile()) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.filePath(),
                                                QObject::tr("Source File Doesn't Exist"), false);
            return 0;
        }
        QString theRelativePath(m_Doc.GetRelativePathToDoc(imageSrc));
        SModelBufferAndPath theModelBuffer =
            m_Doc.GetBufferCache().GetOrCreateModelBuffer(theRelativePath);
        if (!theModelBuffer.m_modelBuffer) {
            if (theHandler)
                theHandler->DisplayImportFailed(imageSrc.filePath(),
                                                QObject::tr("Could Not Load Model Buffer"), false);
            return 0;
        }
        // Ensure we include the model buffer version in the relative path
        QString relativePath = m_Doc.GetRelativePathToDoc(theModelBuffer.m_filePath);

        qt3dsdm::Qt3DSDMInstanceHandle theModelInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::Model, inParent, inSlide);

        SValue theValue(std::make_shared<qt3dsdm::CDataStr>(CString::fromQString(relativePath)));
        m_PropertySystem.SetInstancePropertyValue(theModelInstance,
                                                  m_Bridge.GetSourcePathProperty(), theValue);

        if (inStartTime != -1)
            SetStartTime(theModelInstance, inStartTime);

        CheckMeshSubsets(theModelInstance, m_Bridge.GetSourcePathProperty());

        SetName(theModelInstance, imageSrc.baseName(), true);

        return FinalizeAddOrDrop(theModelInstance, inParent, inDropType, inPosition,
                                 inStartTime == -1);
    }

    QString LoadScriptFile(const CFilePath &inFile)
    {
        QString retval;

        QQmlEngine engine;
        QString path = inFile.filePath();
        path.replace('\\', '/');
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

            if (inWarnings[idx].m_ExtraInfo.size())
                theMessageStr += QLatin1Char(' ') + inWarnings[idx].m_ExtraInfo;

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
                QString theData;
                QString theOriginalData;
                if (theRef.GetReferenceType() == ObjectReferenceType::Relative) {
                    TDataStrPtr theRefValue = get<TDataStrPtr>(theRef.m_Value);
                    if (theRefValue)
                        theData = theRefValue->toQString();
                }
                theOriginalData = theData;

                if (!theData.contains(QStringLiteral("Scene")) || theData.size() == 0) {
                    Qt3DSDMInstanceHandle currentInstance = inDynamicInstance;
                    // Resolve this absolute reference string and override the default values
                    // in the datacore to be this exact datatype
                    if (!theData.contains(QStringLiteral("Scene"))) {
                        QString theItemName;
                        // Walk through the data and attempt to find each object in the asset graph
                        // ignoring slides or anything else.
                        if (theData.length() > 6)
                            theData = theData.right(theData.length() - 6);
                        else
                            theData.clear();
                        currentInstance = m_Doc.GetSceneInstance();
                        while (theData.size() && currentInstance.Valid()) {
                            int thePos = theData.indexOf(QLatin1Char('.'));
                            if (thePos != -1) {
                                theItemName = theData.left(thePos);
                                theData = theData.right(theData.length() - thePos + 1);
                            } else {
                                theItemName = theData;
                                theData.clear();
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
                                QString theName(GetName(theChild));
                                if (theName == theItemName)
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
        virtual QString LoadInstanceData(const QString &inAbsPath) = 0;

        virtual std::shared_ptr<IDOMReader>
        ParseInstanceDefinition(const QString &inFullPathToDocument,
                                std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                std::shared_ptr<IImportFailedHandler> inHandler,
                                Q3DStudio::IInputStreamFactory &inInputStreamFactory) = 0;
    };

    virtual TInstanceHandle LoadDynamicInstance(const QString &inFullPathToDocument,
                                                TInstanceHandle inParent, TSlideHandle inSlide,
                                                DocumentEditorInsertType::Enum inDropType,
                                                long inStartTime,
                                                ISpecificDynamicInstance &inSpecificInstance,
                                                bool inFinalize)
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        TInstanceHandleList existing;
        m_DataCore.GetInstancesDerivedFrom(existing, inSpecificInstance.GetRootInstance());
        QString theRelativePath(m_Doc.GetRelativePathToDoc(inFullPathToDocument));
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
            if (theRelativePath == GetSourcePath(existing[idx]))
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
                    std::make_shared<CDataStr>(theRelativePath));

                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                    std::make_shared<CDataStr>(QFileInfo(theRelativePath).baseName()));
                std::vector<SMetaDataLoadWarning> theWarnings;
                m_MetaData.LoadInstance(*theReaderPtr, theParentInstance,
                                        QFileInfo(theRelativePath).baseName(), theWarnings);
                QString theLoadError = inSpecificInstance.LoadInstanceData(inFullPathToDocument);
                DisplayLoadWarnings(inFullPathToDocument, theWarnings, theLoadError);
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
        QString LoadInstanceData(const QString &inAbsPath) override
        {
            return m_Editor.LoadScriptFile(inAbsPath);
        }

        virtual std::shared_ptr<IDOMReader>
        ParseInstanceDefinition(const QString &inFullPathToDocument,
                                std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                std::shared_ptr<IImportFailedHandler> inHandler,
                                Q3DStudio::IInputStreamFactory &inInputStreamFactory) override
        {
            return IDocumentEditor::ParseScriptFile(inFullPathToDocument, inStringTable, inHandler,
                                                    inInputStreamFactory);
        }
    };

    TInstanceHandle LoadBehavior(const QString &inFullPathToDocument,
                                 TInstanceHandle inParent, TSlideHandle inSlide,
                                 DocumentEditorInsertType::Enum inDropType,
                                 long inStartTime) override
    {
        TInstanceHandle ret;
        if (inFullPathToDocument.endsWith(QStringLiteral(".qml"))) {
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
        QString LoadInstanceData(const QString &) override
        {
            // We would want to ask the render system to possibly load the dll at this point.
            return QString();
        }

        virtual std::shared_ptr<IDOMReader>
        ParseInstanceDefinition(const QString &inFullPathToDocument,
                                std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                std::shared_ptr<IImportFailedHandler> inHandler,
                                Q3DStudio::IInputStreamFactory &inInputStreamFactory) override
        {
            return IDocumentEditor::ParsePluginFile(inFullPathToDocument, inStringTable, inHandler,
                                                    inInputStreamFactory);
        }
    };

    TInstanceHandle LoadRenderPlugin(const QString &inFullPathToDocument,
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
    }

    TInstanceHandle CreateText(const QString &inFullPathToDocument,
                               TInstanceHandle inParent, TSlideHandle inSlide,
                               DocumentEditorInsertType::Enum inDropType,
                               const CPt &inPosition = CPt(), long inStartTime = -1) override
    {
        (void)inStartTime;

        QFileInfo theFontFile(inFullPathToDocument);
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());

        if (!theFontFile.isFile()) {
            if (theHandler) {
                theHandler->DisplayImportFailed(theFontFile.fileName(),
                                                QObject::tr("Font File Doesn't Exist"), false);
            }
            return 0;
        }

        // Get the font name of the font file
        QString theFontName = m_Doc.GetProjectFontName(theFontFile);
        if (theFontName.size() == 0) {
            if (theHandler) {
                theHandler->DisplayImportFailed(theFontFile.fileName(),
                                                QObject::tr("Unable to load Font File"), false);
            }
            return 0;
        }

        // Create text instance
        qt3dsdm::Qt3DSDMInstanceHandle theTextInstance =
            CreateSceneGraphInstance(ComposerObjectTypes::Text, inParent, inSlide);

        // Set the Font property to the font file
        m_PropertySystem.SetInstancePropertyValue(theTextInstance, m_Bridge.GetText().m_Font,
                                                  std::make_shared<qt3dsdm::CDataStr>(theFontName));

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

    typedef void (IMetaData::*TDynamicObjectLoader)(const QString &inShaderFile,
                                                    Qt3DSDMInstanceHandle inInstance,
                                                    const QString &inName,
                                                    std::vector<SMetaDataLoadWarning> &outWarnings,
                                                    QFile &stream);

    TInstanceHandle LoadDynamicObject(const QString &inFullPathToDocument,
                                      TInstanceHandle inParent, TSlideHandle inSlide,
                                      DocumentEditorInsertType::Enum inDropType, long inStartTime,
                                      TDynamicObjectLoader inLoader,
                                      TInstanceHandle inDerivationParent,
                                      TInstanceHandle inTargetId = TInstanceHandle())
    {
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());

        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        QFileInfo theShaderFile(inFullPathToDocument);
        if (theShaderFile.suffix() == QLatin1String("nvmpe")) {
            // If user drag-drop nvmpe file, we find the corresponding glsl file and use it to load
            // the effect.
            QString shaderFile = inFullPathToDocument;
            QString newShaderFile = shaderFile.left(shaderFile.length() - 5);
            newShaderFile.append(QStringLiteral("glsl"));
            theShaderFile = QFileInfo(newShaderFile);
        }

        TInstanceHandleList existing;
        m_DataCore.GetInstancesDerivedFrom(existing, inDerivationParent);
        QString theRelativePath(m_Doc.GetRelativePathToDoc(theShaderFile));
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
            if (theRelativePath == GetSourcePath(existing[idx]))
                theParentInstance = existing[idx];
        }

        if (theParentInstance.Valid() == false) {
            if (theShaderFile.exists()) {
                theParentInstance = m_DataCore.CreateInstance();
                m_DataCore.DeriveInstance(theParentInstance, inDerivationParent);
                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Asset.m_SourcePath,
                    std::make_shared<CDataStr>(theRelativePath));

                m_DataCore.SetInstancePropertyValue(
                    theParentInstance, m_Bridge.GetObjectDefinitions().m_Named.m_NameProp,
                    std::make_shared<CDataStr>(QFileInfo(theRelativePath).baseName()));

                std::vector<SMetaDataLoadWarning> theWarnings;
                QString shaderFile = theShaderFile.filePath();
                IRefCountedInputStream theStream(
                    m_InputStreamFactory->getStreamForFile(shaderFile));
                (m_MetaData.*inLoader)(theRelativePath,
                                       theParentInstance,
                                       QFileInfo(theRelativePath).baseName(),
                                       theWarnings,
                                       *theStream);
                DisplayLoadWarnings(shaderFile, theWarnings, QString());
            } else {
                if (theHandler) {
                    theHandler->DisplayImportFailed(theShaderFile.filePath(),
                                                    QObject::tr("Unable to load Shader File"),
                                                    false);
                }
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

    TInstanceHandle LoadEffect(const QString &inFullPathToDocument,
                               TInstanceHandle inParent, TSlideHandle inSlide,
                               DocumentEditorInsertType::Enum inDropType, long inStartTime) override
    {
        return LoadDynamicObject(inFullPathToDocument, inParent, inSlide, inDropType, inStartTime,
                                 &IMetaData::LoadEffectInstance,
                                 m_Bridge.GetObjectDefinitions().m_Effect.m_Instance);
    }

    TInstanceHandle LoadCustomMaterial(const QString &inFullPathToDocument,
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
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    static void eatspace(const char8_t *str)
    {
        while (!isTrivial(str) && *str == ' ') {
            ++str;
        }
    }

    void SetUniqueName(TInstanceHandle inItem, const char8_t *inNameBase,
                       eastl::vector<Q3DStudio::CString> &inExistingNames)
    {
        QString theName(inNameBase);
        QT3DSU32 idx = 1;
        while (eastl::find(inExistingNames.begin(), inExistingNames.end(), theName)
               != inExistingNames.end()) {
            char8_t nameBuffer[64];
            sprintf(nameBuffer, "%d", idx);
            ++idx;
            theName.assign(inNameBase);
            theName.append("_");
            theName.append(nameBuffer);
        }
        SetName(inItem, theName, false);
        inExistingNames.push_back(theName);
    }

    virtual TInstanceHandle LoadPathBuffer(const QString &inFullPathToDocument,
                                           TInstanceHandle inParent, TSlideHandle inSlide,
                                           DocumentEditorInsertType::Enum inDropType,
                                           long inStartTime)
    {
        std::shared_ptr<IImportFailedHandler> theHandler(m_Doc.GetImportFailedHandler());
        Q3DStudio::CString relPath = m_Doc.GetRelativePathToDoc(inFullPathToDocument);
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
                                 TDataStrPtr(new CDataStr(relPath.c_str())));
        FinalizeAddOrDrop(retval, inParent, inDropType, CPt(), inStartTime == -1, false);
        return retval;
    }
#endif
    TInstanceHandle ImportFile(DocumentEditorFileType::Enum inFileType,
                               const QString &inFullPathToDocument,
                               TInstanceHandle inParent, TSlideHandle inSlide,
                               const QString &inImportFileExtension,
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
            if (theHandler) {
                theHandler->DisplayImportFailed(inFullPathToDocument,
                            QObject::tr("Unsupported Document Editor Type (at this time!)"), false);
            }
            break;
        }
        }
        return 0;
    }

    void DepthFirstAddImportChildren(TSlideHandle inSlide, TInstanceHandle inInstance,
                                     TIdMultiMap &inMap, std::unordered_set<int> &ioAddedChildren)
    {
        QString theId = GetImportId(inInstance);
        if (!theId.isEmpty() && m_SlideSystem.GetAssociatedSlide(inInstance) == inSlide) {
            TIdMultiMap::iterator theResult =
                inMap.insert(theId, QVector<QPair<TSlideHandle, TInstanceHandle>>());
            insert_unique_qt(*theResult, QPair<TSlideHandle, TInstanceHandle>(inSlide, inInstance));
            ioAddedChildren.insert(inInstance);
        }

        for (long idx = 0, end = m_AssetGraph.GetChildCount(inInstance); idx < end; ++idx) {
            TInstanceHandle theInstance = m_AssetGraph.GetChild(inInstance, idx);
            DepthFirstAddImportChildren(inSlide, theInstance, inMap, ioAddedChildren);
        }
    }

    // Precondition is that our source path to instance map
    // has all of the source-path-to-instance hooks already looked up.
    void DoRefreshImport(const QString &inOldFile, const QString &inNewFile)
    {
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        vector<QString> importFileList;

        // Find which import files use this dae file.
        for (TCharPtrToSlideInstanceMap::iterator theIter = m_SourcePathInstanceMap.begin(),
                                                  end = m_SourcePathInstanceMap.end();
             theIter != end; ++theIter) {
            QFileInfo theSource(theIter.key());
            if (theSource.suffix() == QLatin1String("import")) {
                QFileInfo theFullPath = m_Doc.GetResolvedPathToDoc(theSource);
                if (theFullPath.exists() && theFullPath.isFile()) {
                    if (std::find(importFileList.begin(), importFileList.end(),
                                  theFullPath.filePath())
                        == importFileList.end()) {
                        ImportPtrOrError theImport = Import::Load(theFullPath.absoluteFilePath());
                        if (theImport.m_Value) {
                            QString theSrcFile = CFilePath::CombineBaseAndRelative(
                                theImport.m_Value->GetDestDir(),
                                theImport.m_Value->GetSrcFile());
                            if (theSrcFile.compare(inOldFile, Qt::CaseInsensitive))
                                importFileList.push_back(theFullPath.canonicalFilePath());
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
        //		 communicate the import changes to the item.
        // 4.  Run the refresh process using a composer editor that runs off of our
        //		mappings
        TIdMultiMap theGroupIdMap;
        for (size_t importIdx = 0, end = importFileList.size(); importIdx < end; ++importIdx) {
            theGroupIdMap.clear();
            QString theImportFilePath = importFileList[importIdx];
            QString theImportRelativePath = m_Doc.GetRelativePathToDoc(theImportFilePath);
            TCharPtrToSlideInstanceMap::iterator theIter =
                m_SourcePathInstanceMap.find(theImportRelativePath);
            if (theIter == m_SourcePathInstanceMap.end())
                continue;
            // First pass just build the group id entries.  This avoids us copying hashtables which
            // may
            // be quite expensive
            for (TSlideInstanceList::iterator theSlideInst = theIter->begin(),
                                              theSlideInstEnd = theIter->end();
                 theSlideInst != theSlideInstEnd; ++theSlideInst) {
                TInstanceHandle theRoot = theSlideInst->second;
                TSlideHandle theSlide = theSlideInst->first;

                // For a depth first search of all children of this object *in this slide*,
                // if they have an import id then add them to the map.
                DepthFirstAddImportChildren(theSlide, theRoot, theGroupIdMap, theAddedInstances);
                TIdMultiMap::iterator theGroupId
                        = theGroupIdMap.insert(GetImportId(theRoot),
                                      QVector<QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>>());
                insert_unique_qt(*theGroupId, QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>
                                 (theSlide, theRoot));
                theAddedInstances.insert(theRoot);
            }
            // Since some objects may be completely free standing, we need to go through *all*
            // objects.
            // Unfortunately the first revision of the system didn't put import paths on objects so
            // we need both the above loop *and* to consider every object who's import path matches
            // out import document's relative path.
            theIter = theImportPaths.find(theImportRelativePath);
            TSlideHandleList theAssociatedSlides;
            if (theIter != theImportPaths.end()) {
                vector<pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>> &theInstances = *theIter;
                for (size_t freeInstanceIdx = 0, end = theInstances.size(); freeInstanceIdx < end;
                     ++freeInstanceIdx) {
                    if (theAddedInstances.find(theInstances[freeInstanceIdx].second)
                        != theAddedInstances.end())
                        continue;
                    theAssociatedSlides.clear();
                    Qt3DSDMInstanceHandle theInstance(theInstances[freeInstanceIdx].second);
                    GetAllAssociatedSlides(theInstance, theAssociatedSlides);
                    TIdMultiMap::iterator theInstanceId =
                        theGroupIdMap.insert(GetImportId(theInstance),
                                       QVector<QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>>());
                    for (size_t slideIdx = 0, slideEnd = theAssociatedSlides.size();
                         slideIdx < slideEnd; ++slideIdx) {
                        insert_unique_qt(*theInstanceId,
                                      QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>(
                                             theAssociatedSlides[slideIdx], theInstance));
                    }
                    theAddedInstances.insert(theInstance);
                }
            }

            //
            // OK, we have distinct maps sorted on a per-slide basis for all trees of children
            // of this asset.  We now need to attempt to run the refresh algorithm.
            qt3dsimp::ImportPtrOrError theImportPtr
                    = qt3dsimp::Import::Load(theImportFilePath);
            if (theImportPtr.m_Value == NULL) {
                QT3DS_ASSERT(false);
                continue;
            }

            QFileInfo newInfo(inNewFile);
            if (newInfo.exists() == false) {
                QT3DS_ASSERT(false);
                continue;
            }

            QFileInfo oldInfo(inOldFile);
            // Select correct translator according to file type
            ITranslator *translator = nullptr;
            STranslationLog *translationLog = nullptr;
            QString newExtension(newInfo.suffix());
            QString oldExtension(oldInfo.suffix());
            if (newExtension.compare(CDialogs::GetDAEFileExtension(), Qt::CaseInsensitive) == 0
                && oldExtension.compare(CDialogs::GetDAEFileExtension(), Qt::CaseInsensitive) == 0) {
                SColladaTranslator *colladaTranslator = new SColladaTranslator(inNewFile);
                translationLog = &(colladaTranslator->m_TranslationLog);
                translator = colladaTranslator;
#ifdef QT_3DSTUDIO_FBX
            } else if (newExtension.compare(CDialogs::GetFbxFileExtension(),
                                            Qt::CaseInsensitive) == 0
                && oldExtension.compare(CDialogs::GetFbxFileExtension(),
                                        Qt::CaseInsensitive) == 0) {
                SFbxTranslator *fbxTranslator = new SFbxTranslator(inNewFile);
                translationLog = &(fbxTranslator->m_TranslationLog);
                translator = fbxTranslator;
#endif
            } else {
                STranslationLog emptyLog;
                IDocumentEditor::DisplayImportErrors(inNewFile,
                    ImportErrorCodes::TranslationToImportFailed,
                    m_Doc.GetImportFailedHandler(), emptyLog, true);
                continue;
            }

            std::shared_ptr<IComposerEditor> theComposer(
                IComposerEditorInterface::CreateEditorInterface(*this, theGroupIdMap,
                    m_Doc.GetDocumentDirectory(), theImportFilePath, 0, m_AssetGraph));

            SImportResult theImportResult = CPerformImport::RefreshToComposer(
                *translator, *theComposer, *theImportPtr.m_Value, theImportFilePath);

            IDocumentEditor::DisplayImportErrors(inNewFile, theImportResult.m_Error,
                m_Doc.GetImportFailedHandler(),
                *translationLog, false);
        }
    }

    void RefreshImport(const QString &inOldFile, const QString &inNewFile) override
    {
        CDispatch &theDispatch(*m_Doc.GetCore()->GetDispatch());
        theDispatch.FireOnProgressBegin(
            QObject::tr("Refreshing Import "), inOldFile);
        ScopedBoolean __ignoredDirs(m_IgnoreDirChange);
        try {
            m_SourcePathInstanceMap.clear();
            GetSourcePathToInstanceMap(m_SourcePathInstanceMap, false);
            DoRefreshImport(inOldFile, inNewFile);
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
            GetSourcePathToInstanceMap(m_SourcePathInstanceMap, false);
            for (TCharPtrToSlideInstanceMap::iterator theIter = m_SourcePathInstanceMap.begin(),
                                                      end = m_SourcePathInstanceMap.end();
                 theIter != end; ++theIter) {
                QFileInfo theSource(theIter.key());
                if (theSource.suffix() == QLatin1String("mesh")) {
                    QFileInfo theFullPath(m_Doc.GetResolvedPathToDoc(theSource));
#ifdef KDAB_TEMPORARILY_REMOVED
                    if (!theFullPath.exists() || !theFullPath.isfile()
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
                    QSharedPointer<QFile> output(openQFileStream(theFullPath, FileWriteFlags()));
                    if (!output.data())
                        QT3DS_ALWAYS_ASSERT_MESSAGE(theFullPath.toCString().GetCharStar());
                    MallocAllocator allocator;
                    theMesh->SaveMulti(allocator, *output);

                    delete theMesh;
#endif
                    cleanedSome = true;
                }
            }
        } catch (...) {
        }
        theDispatch.FireOnProgressEnd();

        return cleanedSome;
    }
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    void ExternalizePath(TInstanceHandle path) override
    {
        QString thePathsDirectory(
            CFilePath::CombineBaseAndRelative(m_Doc.GetDocumentDirectory(),
                                              QStringLiteral("paths")));
        CFilePath::CreateDir(thePathsDirectory, true);
        QString theName = GetName(path);
        QString theTargetFileName(CFilePath::CombineBaseAndRelative(thePathsDirectory, theName));
        theTargetFileName.append(QLatin1String(".path"));
        QFileInfo info(theTargetFileName);
        if (info.exists()) {
            QString targetFileStem = theTargetFileName.left(theTargetFileName.length() - 5);
            QString targetFile;
            QT3DSU32 index = 1;
            do {
                targetFile = QStringLiteral("%1_%2%3").arg(targetFileStem)
                        .arg(index).arg(QStringLiteral(".path"));
                ++index;
                info = QFileInfo(targetFile);
            } while (info.exists());
            theTargetFileName = targetFile;
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
                        QT3DSVec2 c1 = IPathManager::GetControlPointFromAngleDistance(
                            prevPos, prevAngle, prevDistance);

                        QT3DSF32 angle = GetTypedInstancePropertyValue<float>(theAnchor, angleProp);
                        QT3DSF32 distance =
                            GetTypedInstancePropertyValue<float>(theAnchor, incomingdistanceProp);
                        QT3DSVec2 c2 = IPathManager::GetControlPointFromAngleDistance(position, angle,
                                                                                   distance);
                        theBuilder->CubicCurveTo(c1, c2, position);
#endif
                    }
                    theLastAnchor = theAnchor;
                }
                if (isClosed)
                    theBuilder->Close();
            }
        }
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
        SPathBuffer theBuffer = theBuilder->GetPathBuffer();
        CFileSeekableIOStream theWriter(theTargetFileName.toCString(), FileWriteFlags());
        theBuffer.Save(theWriter);

        for (QT3DSU32 idx = 0, end = theSubPathChildren.size(); idx < end; ++idx)
            DeleteInstance(theSubPathChildren[idx]);

        CFilePath relativeFileName(
            CFilePath::GetRelativePathFromBase(m_Doc.GetDocumentDirectory(), theTargetFileName));
        SetInstancePropertyValue(path, theDefinitions.m_Asset.m_SourcePath,
                                 TDataStrPtr(new CDataStr(relativeFileName.toCString())));
#endif
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
        CFileSeekableIOStream theReader(thePathToPathFile.toCString(), FileReadFlags());
        if (theReader.IsOpen() == false)
            return;
        qt3dsimp::SPathBuffer *theLoadedBuffer =
            qt3dsimp::SPathBuffer::Load(theReader, *m_Foundation.m_Foundation);
        if (theLoadedBuffer == NULL)
            return;
#endif
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
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
#endif
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
#endif
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
        theLoadedBuffer->Free(m_Foundation.m_Foundation->getAllocator());
#endif
    }
#endif
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
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
#endif
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
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
#endif
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
            for (size_t idx = 0, end = selectedInstances.size(); idx < end; ++idx) {
                qt3dsdm::Qt3DSDMInstanceHandle handle(selectedInstances[idx]);
                if (handle.Valid()) {
                    if (value.empty()) {
                        // First valid handle selects if all are hidden/unhidden
                        propertySystem->GetInstancePropertyValue(handle, property, value);
                        boolValue = !qt3dsdm::get<bool>(value);
                    }
                    propertySystem->SetInstancePropertyValue(handle, property, boolValue);
                }
            }
        }
    }

    void BuildDAEMap(const TFileModificationList &inList)
    {
        for (size_t fileIdx = 0, fileEnd = inList.size(); fileIdx < fileEnd; ++fileIdx) {
            const SFileModificationRecord &theRecord(inList[fileIdx]);
            QString theExtension = theRecord.m_File.suffix();
            bool isImport = theExtension.compare(QLatin1String("import"), Qt::CaseInsensitive) == 0;
            QString theRelativePath(m_Doc.GetRelativePathToDoc(theRecord.m_File));

            if (theRecord.m_ModificationType == FileModificationType::InfoChanged
                || theRecord.m_ModificationType == FileModificationType::Destroyed) {
                if (isImport)
                    m_ImportFileToDAEMap.erase(m_ImportFileToDAEMap.find(theRelativePath));
                continue;
            }
            if (isImport) {
                qt3dsimp::ImportPtrOrError theImportPtr
                            = qt3dsimp::Import::Load(theRecord.m_File.canonicalFilePath());
                if (theImportPtr.m_Value) {
                    QString theDestDir = theImportPtr.m_Value->GetDestDir();
                    QString theSrcFile = theImportPtr.m_Value->GetSrcFile();
                    QString theFullSrcPath
                            = CFilePath::CombineBaseAndRelative(theDestDir, theSrcFile);
                    QString theDAERelativePath = m_Doc.GetRelativePathToDoc(theFullSrcPath);
                    QHash<QString, QString>::iterator theInsertResult
                            = m_ImportFileToDAEMap.insert(theRelativePath, theDAERelativePath);
                    theImportPtr.m_Value->Release();
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
            && m_Doc.GetDocumentPath().endsWith(inList[0].m_File.fileName())
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
        GetSourcePathToInstanceMap(m_SourcePathInstanceMap);
        TInstanceHandleList theParents;
        SComposerObjectDefinitions &theDefinitions(m_Bridge.GetObjectDefinitions());

        for (size_t fileIdx = 0, fileEnd = inList.size(); fileIdx < fileEnd; ++fileIdx) {
            const SFileModificationRecord &theRecord(inList[fileIdx]);

            QString theExtension = theRecord.m_File.suffix();
            bool isImport = theExtension.compare(QLatin1String("import"),
                                                 Qt::CaseInsensitive) == 0;
            QString theRelativePath(m_Doc.GetRelativePathToDoc(theRecord.m_File));

#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
            if ((theExtension.CompareNoCase(L"ttf")
                 || theExtension.CompareNoCase(L"otf")) // should use CDialogs::IsFontFileExtension
                && m_Doc.GetSceneGraph() && m_Doc.GetSceneGraph()->GetTextRenderer()) {
                m_Doc.GetSceneGraph()->GetTextRenderer()->ReloadFonts();
                CFilePath thePath = m_Doc.GetDocumentDirectory();
                CFilePath theFontCache = CFilePath::CombineBaseAndRelative(thePath, L"fontcache");
                theFontCache.DeleteThisDirectory(true);
            }
#endif
            if (theRecord.m_ModificationType == FileModificationType::InfoChanged
                || theRecord.m_ModificationType == FileModificationType::Destroyed) {
                if (isImport)
                    m_ImportFileToDAEMap.erase(m_ImportFileToDAEMap.find(theRelativePath));
                continue;
            }

            QDir modifiedPath = QDir::cleanPath(theRelativePath);
            TCharPtrToSlideInstanceMap::iterator theFind = m_SourcePathInstanceMap.end();
            for (TCharPtrToSlideInstanceMap::iterator it = m_SourcePathInstanceMap.begin();
                 it != m_SourcePathInstanceMap.end(); ++it) {
                QDir sourcePath = QDir::cleanPath(it.key());
                if (sourcePath == modifiedPath) {
                    theFind = it;
                    break;
                }
            }

            if (theFind == m_SourcePathInstanceMap.end())
                continue;

            const TSlideInstanceList theInstances(*theFind);
            if (theRecord.m_ModificationType != FileModificationType::Created) {
                requestRender = true;
                m_Doc.GetBufferCache().InvalidateBuffer(theRelativePath);
            }

            qCInfo(qt3ds::TRACE_INFO) << "Change detected: " << theRelativePath << " "
                      << ModificationTypeToString(theRecord.m_ModificationType);

            if (isImport) {
                qt3dsimp::ImportPtrOrError theImportPtr
                        = qt3dsimp::Import::Load(theRecord.m_File.canonicalFilePath());
                if (theImportPtr.m_Value) {
                    ENSURE_PROGRESS;
                    CFilePath theDestDir = theImportPtr.m_Value->GetDestDir();
                    CFilePath theSrcFile = theImportPtr.m_Value->GetSrcFile();
                    CFilePath theFullSrcPath
                            = CFilePath::CombineBaseAndRelative(theDestDir, theSrcFile);
                    QString theDAERelativePath = m_Doc.GetRelativePathToDoc(theFullSrcPath);
                    QHash<QString, QString>::iterator theInsertResult
                            = m_ImportFileToDAEMap.insert(theRelativePath, theDAERelativePath);
                    theImportPtr.m_Value->Release();
                }
            } else if (theExtension.compare(QLatin1String("qml"), Qt::CaseInsensitive) == 0
                       && theRecord.m_ModificationType != FileModificationType::Created
                       && theInstances.empty() == false) {
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
                        theReaderPtr = ParseScriptFile(theRecord.m_File.canonicalFilePath(),
                                                       m_DataCore.GetStringTablePtr(),
                                                       m_Doc.GetImportFailedHandler(),
                                                       *m_InputStreamFactory);
                        if (!theReaderPtr) {
                            // then effectively no change...
                            QT3DS_ASSERT(false);
                        } else {
                            std::vector<SMetaDataLoadWarning> theWarnings;
                            m_MetaData.LoadInstance(*theReaderPtr, theBehavior,
                                                    theRelativePath,
                                                    theWarnings);
                            CScriptDynamicInstanceLoader inSpecificInstance(*this);
                            QString theLoadError =
                                inSpecificInstance.LoadInstanceData(theRecord.m_File
                                                                    .canonicalFilePath());
                            DisplayLoadWarnings(theRecord.m_File.canonicalFilePath(),
                                                theWarnings, theLoadError);
                        }
                    }
                }
            } else if (theExtension.compare(QLatin1String("effect"), Qt::CaseInsensitive) == 0
                       && theRecord.m_ModificationType != FileModificationType::Created
                       && theInstances.empty() == false) {
                QString theNameStr = GetName(theInstances[0].second);
                std::vector<SMetaDataLoadWarning> theWarnings;
                IRefCountedInputStream theStream(
                    m_InputStreamFactory->getStreamForFile(theRecord.m_File.canonicalFilePath()));
                if (theStream.data()) {
                    m_MetaData.LoadEffectInstance(theRelativePath,
                                                  theInstances[0].second,
                                                  theNameStr,
                                                  theWarnings, *theStream);
                }

                for (size_t i = 0; i < theInstances.size(); ++i) {
                    theDispatch.FireReloadEffectInstance(theInstances[i].second);
                    theDispatch.FireImmediateRefreshInstance(theInstances[i].second);
                }
            }
            // There used to be an extension here for meshes
            // but that causes the product to delete materials in some cases which loses work.
            // so that experiment failed and we will just have to let the users manually updated
            // their
            // meshes through the dropdown if they need them updated.
        }
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
    QString resultDialogStr;
    std::shared_ptr<IImportFailedHandler> theHandler(inHandler);
    if (inImportError == ImportErrorCodes::TranslationToImportFailed || inForceError) {
        isError = true;
        resultDialogStr = "Failed to import file";
    }

    for (size_t idx = 0; idx < inTranslationLog.m_Warnings.size(); ++idx) {
        const std::pair<ESceneGraphWarningCode, QString> &warning(
            inTranslationLog.m_Warnings[idx]);
        QString formatStr = QObject::tr("Unrecognized warning");
        switch (warning.first) {
        case ESceneGraphWarningCode_OnlySupportTriangles:
            formatStr = QObject::tr("Model %1 contains geometric elements other than triangles");
            formatStr = formatStr.arg(warning.second);
            break;
        case ESceneGraphWarningCode_TrianglesDuplicateSemantic:
            formatStr = QObject::tr("Triangle contains duplicate semantics, ex: 1 triangle " \
                                       "has multiple TEXCOORD (multiple UV maps)");
            break;
        case ESceneGraphWarningCode_MissingSourceFile:
            formatStr = QObject::tr("Couldn't find a source image file %1");
            formatStr = formatStr.arg(warning.second);
            break;
        case ESceneGraphWarningCode_LockedDestFile:
            formatStr = QObject::tr("An image or mesh file %ls is not writeable");
            break;
        case ESceneGraphWarningCode_VertexBufferTooLarge:
            formatStr = QObject::tr("A single mesh exceeds the maximum vertex count of 65535");
            break;
        default:
            break;
        }

        if (resultDialogStr.size())
            resultDialogStr += QLatin1Char('\n');
        resultDialogStr += formatStr;
    }
    if (resultDialogStr.size()) {
        if (theHandler)
            theHandler->DisplayImportFailed(inImportSource, resultDialogStr, !isError);
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

void IDocumentEditor::UnlinkAlwaysUnlinkedProperties(Qt3DSDMInstanceHandle inInstance,
                                                     SComposerObjectDefinitions &inDefs,
                                                     ISlideSystem &inSlideSystem)
{
    Qt3DSDMPropertyHandle *theUnlinked(GetAlwaysUnlinkedProperties(inDefs));
    for (Qt3DSDMPropertyHandle *theHandle = theUnlinked; theHandle->Valid(); ++theHandle)
        inSlideSystem.UnlinkProperty(inInstance, *theHandle);
}

Qt3DSDMInstanceHandle IDocumentEditor::CreateSceneGraphInstance(
    const QString &inType, TInstanceHandle inParent, TSlideHandle inSlide,
    qt3dsdm::IDataCore &inDataCore, qt3dsdm::ISlideSystem &inSlideSystem,
    qt3dsdm::SComposerObjectDefinitions &inObjectDefs, Q3DStudio::CGraph &inAssetGraph,
    qt3dsdm::IMetaData &inMetaData, TInstanceHandle inTargetId, bool setTimeRange)
{
    return CreateSceneGraphInstance(inMetaData.GetCanonicalInstanceForType(inType),
                                    inParent, inSlide, inDataCore, inSlideSystem, inObjectDefs,
                                    inAssetGraph, inMetaData, inTargetId);
}

Qt3DSDMInstanceHandle IDocumentEditor::CreateSceneGraphInstance(
    Qt3DSDMInstanceHandle inMaster, TInstanceHandle inParent, TSlideHandle inSlide,
    qt3dsdm::IDataCore &inDataCore, qt3dsdm::ISlideSystem &inSlideSystem,
    qt3dsdm::SComposerObjectDefinitions &inObjectDefs, Q3DStudio::CGraph &inAssetGraph,
    qt3dsdm::IMetaData &inMetaData, TInstanceHandle inTargetId)
{
    Option<QString> theTypeOpt = inMetaData.GetTypeForInstance(inMaster);
    if (theTypeOpt.hasValue() == false)
        return 0;

    SComposerObjectDefinitions &theDefs(inObjectDefs);
    TInstanceHandle retval = inDataCore.CreateInstance(inTargetId);
    TInstanceHandle theDerivationParent(inMaster);
    inDataCore.DeriveInstance(retval, theDerivationParent);

    if (inParent.Valid())
        inAssetGraph.AddChild(inParent, retval);
    else
        inAssetGraph.AddRoot(retval);

    if (inSlide.Valid()) {
        inSlideSystem.AssociateInstanceWithSlide(inSlide, retval);
        UnlinkAlwaysUnlinkedProperties(retval, inObjectDefs, inSlideSystem);
    }

    Q3DStudio::CId theId;
    if (ComposerObjectTypes::Convert(theTypeOpt) == qt3dsdm::ComposerObjectTypes::Scene)
        theId = SCENE_GUID;
    else
        theId.Generate();

    TGUIDPacked thePackedGuid(theId);
    SLong4 theLong4Id(thePackedGuid.Data1, thePackedGuid.Data2, thePackedGuid.Data3,
                      thePackedGuid.Data4);
    inDataCore.SetInstancePropertyValue(retval, theDefs.m_Guided.m_GuidProp, theLong4Id);
    SValue outValue;
    inDataCore.GetInstancePropertyValue(retval, theDefs.m_Guided.m_GuidProp, outValue);
    SLong4 outLong4Id = qt3dsdm::get<qt3dsdm::SLong4>(outValue);
    return retval;
}

struct SScriptParser
{
    static std::shared_ptr<IDOMReader>
    ParseScriptFile(std::shared_ptr<IDOMFactory> factory,
                    std::shared_ptr<qt3dsdm::IStringTable> stringTable,
                    const QString &fullPath, CXmlErrorHandler &errorHandler,
                    Q3DStudio::IInputStreamFactory &streamFactory)
    {
        QByteArray byteArray;
        {
            IRefCountedInputStream stream(
                streamFactory.getStreamForFile(fullPath));
            if (!stream) {
                QT3DS_ASSERT(0);
                return std::shared_ptr<IDOMReader>();
            }

            byteArray = stream->readAll();
        }

        QString code = QString::fromUtf8(byteArray);

        bool skipXml = false;
        auto start = code.indexOf(QLatin1String("/*[["));
        if (start == -1)
            skipXml = true;

        QString tagged(QStringLiteral("<Behavior>\n"));
        if (!skipXml) {
            start += 4;
            auto end = code.indexOf(QLatin1String("]]*/"), start);
            if (end == -1)
                return std::shared_ptr<IDOMReader>();

            QString xml = code.mid(start, end - start).trimmed();

            tagged.append(xml);
        }
        tagged.append(QLatin1String("</Behavior>\n"));
        tagged.replace(QLatin1String("\r\n"), QLatin1String("\n"));

        QByteArray array = tagged.toLatin1();
        QBuffer xmlStream(&array);

        SDOMElement *element(CDOMSerializer::Read(*factory, xmlStream, &errorHandler));
        if (!element)
            return std::shared_ptr<IDOMReader>();

        return IDOMReader::CreateDOMReader(*element, stringTable, factory);
    }
};

std::shared_ptr<IDOMReader>
IDocumentEditor::ParseScriptFile(const QString &inFullPathToDocument,
                                 std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                 std::shared_ptr<IImportFailedHandler> inHandler,
                                 Q3DStudio::IInputStreamFactory &inInputStreamFactory)
{
    std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SImportXmlErrorHandler theXmlErrorHandler(inHandler, inFullPathToDocument);
    std::shared_ptr<IDOMReader> theReaderPtr(
        SScriptParser::ParseScriptFile(theFactory, inStringTable,
                                       inFullPathToDocument,
                                       theXmlErrorHandler, inInputStreamFactory));

    if (!theReaderPtr) {
        QT3DS_ASSERT(false);
        if (inHandler) {
            inHandler->DisplayImportFailed(inFullPathToDocument,
                                           QObject::tr("Failed to parse script data"),
                                           false);
        }
    }
    return theReaderPtr;
}


struct CRenderPluginParser
{

    static bool NavigateToMetadata(std::shared_ptr<qt3dsdm::IDOMReader> inReader)
    {
        return inReader->MoveToFirstChild("metadata");
    }

    static std::shared_ptr<qt3dsdm::IDOMReader>
    ParseFile(std::shared_ptr<qt3dsdm::IDOMFactory> inFactory,
              std::shared_ptr<qt3dsdm::IStringTable> inStringTable, const QString &inFileData,
              qt3dsdm::CXmlErrorHandler &inErrorHandler,
              IInputStreamFactory &inStreamFactory)
    {
        using namespace qt3ds;
        using namespace qt3ds::foundation;

        IRefCountedInputStream theStream(
            inStreamFactory.getStreamForFile(inFileData));
        if (!theStream) {
            QT3DS_ASSERT(0);
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        }

        qt3dsdm::SDOMElement *theElem(
            qt3dsdm::CDOMSerializer::Read(*inFactory, *theStream, &inErrorHandler));

        if (!theElem) {
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        } else
            return qt3dsdm::IDOMReader::CreateDOMReader(*theElem, inStringTable, inFactory);
    }
};

struct CCustomMaterialParser
{

    static bool NavigateToMetadata(std::shared_ptr<qt3dsdm::IDOMReader> inReader)
    {
        return inReader->MoveToFirstChild("MetaData");
    }

    static std::shared_ptr<qt3dsdm::IDOMReader>
    ParseFile(std::shared_ptr<qt3dsdm::IDOMFactory> inFactory,
              std::shared_ptr<qt3dsdm::IStringTable> inStringTable, const char8_t *inFileData,
              qt3dsdm::CXmlErrorHandler &inErrorHandler,
              IInputStreamFactory &inStreamFactory)
    {
        using namespace qt3ds;
        using namespace qt3ds::foundation;

        IRefCountedInputStream theStream(
            inStreamFactory.getStreamForFile(inFileData));
        if (!theStream.data()) {
            QT3DS_ASSERT(0);
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        }

        qt3dsdm::SDOMElement *theElem(
            qt3dsdm::CDOMSerializer::Read(*inFactory, *theStream, &inErrorHandler));

        if (!theElem) {
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        } else
            return qt3dsdm::IDOMReader::CreateDOMReader(*theElem, inStringTable, inFactory);
    }
};


std::shared_ptr<IDOMReader>
IDocumentEditor::ParsePluginFile(const QString &inFullPathToDocument,
                                 std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                 std::shared_ptr<IImportFailedHandler> inHandler,
                                 IInputStreamFactory &inInputStreamFactory)
{
    std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SImportXmlErrorHandler theXmlErrorHandler(inHandler, inFullPathToDocument);

    std::shared_ptr<IDOMReader> theReaderPtr = CRenderPluginParser::ParseFile(
        theFactory, theStringTable, inFullPathToDocument,
        theXmlErrorHandler, inInputStreamFactory);
    if (!theReaderPtr) {
        QT3DS_ASSERT(false);
        if (inHandler) {
            inHandler->DisplayImportFailed(inFullPathToDocument,
                                           QObject::tr("Failed to parse plugin file"), false);
        }
    }
    CRenderPluginParser::NavigateToMetadata(theReaderPtr);
    return theReaderPtr;
}

std::shared_ptr<IDOMReader>
IDocumentEditor::ParseCustomMaterialFile(const QString &inFullPathToDocument,
                                         std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                                         std::shared_ptr<IImportFailedHandler> inHandler,
                                         IInputStreamFactory &inInputStreamFactory)
{
    std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SImportXmlErrorHandler theXmlErrorHandler(inHandler, inFullPathToDocument);

    std::shared_ptr<IDOMReader> theReaderPtr = CRenderPluginParser::ParseFile(
        theFactory, theStringTable, inFullPathToDocument,
        theXmlErrorHandler, inInputStreamFactory);
    if (!theReaderPtr) {
        QT3DS_ASSERT(false);
        if (inHandler) {
            inHandler->DisplayImportFailed(inFullPathToDocument,
                                           QObject::tr("Failed to parse material file"), false);
        }
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
        qCWarning(qt3ds::WARNING) << m_file << "(" << m_line
                                  << "): Document editor committed upon destruction";
        CommitEditor();
    }
}

IDocumentEditor &CUpdateableDocumentEditor::EnsureEditor(const QString &inCommandName,
                                                         const char *inFile, int inLine)
{
    if (!HasEditor()) {
        m_file = inFile;
        m_line = inLine;
    }
    return m_editorIDocDoc.MaybeOpenTransaction(inCommandName, inFile, inLine);
}

bool CUpdateableDocumentEditor::HasEditor() const
{
    return m_editorIDocDoc.IsTransactionOpened() && !m_file.isEmpty();
}

void CUpdateableDocumentEditor::FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle *inInstances,
                                                     long inInstanceCount)
{
    m_editorIDocDoc.GetCore()->GetDispatch()->FireImmediateRefreshInstance(inInstances,
                                                                           inInstanceCount);
}

void CUpdateableDocumentEditor::CommitEditor()
{
    if (HasEditor()) {
        m_editorIDocDoc.CloseTransaction();
        m_file.clear();
    }
}

void CUpdateableDocumentEditor::RollbackEditor()
{
    if (HasEditor()) {
        m_editorIDocDoc.RollbackTransaction();
        m_editorIDocDoc.CloseTransaction();
        m_file.clear();
    }
}

std::shared_ptr<IInternalDocumentEditor> IInternalDocumentEditor::CreateEditor(CDoc &doc)
{
    return std::shared_ptr<IInternalDocumentEditor>(new CDocEditor(doc));
}
