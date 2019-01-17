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
#include <QtWidgets/qmessagebox.h>
#include "Qt3DSCommonPrecompile.h"
#include "IComposerSerializer.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMSlideCore.h"
#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMActionCore.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMWStrOps.h"
#include "SlideSystem.h"
#include "ActionSystem.h"
#include "Graph.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "StandardExtensions.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "GUIDUtilities.h"
#include "IDocumentEditor.h"
#include "Q3DStudioNVFoundation.h"
#include "IDocSceneGraph.h"
#include "Q3DSInputStreamFactory.h"

#include "Qt3DSDMGuides.h"
#include "foundation/Qt3DSLogging.h"
#include <unordered_map>
#include <unordered_set>

using namespace qt3dsdm;
using namespace std;
using namespace Q3DStudio;
using namespace qt3ds;
using namespace qt3ds::foundation;

QT_BEGIN_NAMESPACE

uint qHash(const qt3dsdm::SLong4 &t, uint seed)
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, t.m_Longs[0]);
    seed = hash(seed, t.m_Longs[1]);
    seed = hash(seed, t.m_Longs[2]);
    seed = hash(seed, t.m_Longs[3]);
    return seed;
}

QT_END_NAMESPACE

namespace std {
template <>
struct hash<SLong4>
{
    size_t operator()(const SLong4 &inValue) const
    {
        hash<unsigned long> hasher;
        return hasher(inValue.m_Longs[0] ^ inValue.m_Longs[1] ^ inValue.m_Longs[2]
                ^ inValue.m_Longs[3]);
    }
};
}

namespace qt3dsdm {

template <>
struct WStrOps<GuideDirections::Enum>
{
    QT3DSU32 ToStr(GuideDirections::Enum value, NVDataRef<char8_t> buffer)
    {
        const char8_t *directionName = NULL;
        switch (value) {
        case GuideDirections::Horizontal:
            directionName = "Horizontal";
            break;
        case GuideDirections::Vertical:
            directionName = "Vertical";
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
        if (directionName != NULL)
            return _snprintf(buffer.begin(), buffer.size(), "%s", directionName);
        else {
            QT3DS_ASSERT(false);
            return 0;
        }
    }
    bool StrTo(const char8_t *buffer, GuideDirections::Enum &item)
    {
        if (AreEqual("Horizontal", buffer)) {
            item = GuideDirections::Horizontal;
            return true;
        }
        if (AreEqual("Vertical", buffer)) {
            item = GuideDirections::Vertical;
            return true;
        }
        return false;
    }
};
}

namespace {

typedef QSharedPointer<Q3DStudio::IInputStreamFactory> TStreamFactoryPtr;

using std::hash;

template <typename TOperator>
static void HandleKeyframe(SLinearKeyframe &inKeyframe, TOperator &inOperator)
{
    inOperator(inKeyframe.m_KeyframeSeconds);
    inOperator(inKeyframe.m_KeyframeSeconds);
}

template <typename TOperator>
static void HandleKeyframe(SBezierKeyframe &inKeyframe, TOperator &inOperator)
{
    inOperator(inKeyframe.m_KeyframeSeconds);
    inOperator(inKeyframe.m_KeyframeValue);
    inOperator(inKeyframe.m_InTangentTime);
    inOperator(inKeyframe.m_InTangentValue);
    inOperator(inKeyframe.m_OutTangentTime);
    inOperator(inKeyframe.m_OutTangentValue);
}

template <typename TOperator>
static void HandleKeyframe(SEaseInEaseOutKeyframe &inKeyframe, TOperator &inOperator)
{
    inOperator(inKeyframe.m_KeyframeSeconds);
    inOperator(inKeyframe.m_KeyframeValue);
    inOperator(inKeyframe.m_EaseIn);
    inOperator(inKeyframe.m_EaseOut);
}

template <typename TItemType>
struct SVectorWriteOperator
{
    vector<TItemType> &m_Vector;
    SVectorWriteOperator(vector<TItemType> &vec)
        : m_Vector(vec)
    {
    }
    void operator()(const TItemType &inValue) { m_Vector.push_back(inValue); }
};

template <typename TItemType>
struct SMemReadOperator
{
    const TItemType *m_Ptr;
    const TItemType *m_End;
    SMemReadOperator(const TItemType *s, const TItemType *e)
        : m_Ptr(s)
        , m_End(e)
    {
    }

    bool IsDone() { return m_Ptr >= m_End; }

    void operator()(TItemType &outValue)
    {
        if (m_Ptr < m_End) {
            outValue = *m_Ptr;
            ++m_Ptr;
        } else {
            QT3DS_ASSERT(false);
        }
    }
};

template <typename TKeyframeType>
static void WriteKeyframes(TKeyframeHandleList &inKeyframes, IAnimationCore &inCore,
                           vector<float> &outValues)
{
    SVectorWriteOperator<float> theOperator(outValues);
    for (size_t idx = 0, end = inKeyframes.size(); idx < end; ++idx) {
        TKeyframe theKeyframeVariant(inCore.GetKeyframeData(inKeyframes[idx]));
        TKeyframeType theData(get<TKeyframeType>(theKeyframeVariant));
        HandleKeyframe(theData, theOperator);
    }
}

template <typename TKeyframeType>
static void ReadKeyframes(Qt3DSDMAnimationHandle inAnimation, IAnimationCore &inCore,
                          const float *inStart, const float *inEnd)
{
    SMemReadOperator<float> theOperator(inStart, inEnd);
    while (theOperator.IsDone() == false) {
        TKeyframeType theData;
        HandleKeyframe(theData, theOperator);
        inCore.InsertKeyframe(inAnimation, theData);
    }
}

struct SPropertyMatcher
{
    Qt3DSDMPropertyHandle rhs;
    SPropertyMatcher(Qt3DSDMPropertyHandle _rhs)
        : rhs(_rhs)
    {
    }
    bool operator()(const pair<Qt3DSDMPropertyHandle, SValue> &lhs) const
    {
        return lhs.first == rhs;
    }
};

struct SPropertyMatches
{
    Qt3DSDMPropertyHandle m_Prop;
    SPropertyMatches(Qt3DSDMPropertyHandle inProp)
        : m_Prop(inProp)
    {
    }
    bool operator()(const pair<Qt3DSDMPropertyHandle, SValue> &inValue)
    {
        return inValue.first == m_Prop;
    }
};

struct SAnimationMatcher
{
    IAnimationCore &m_Core;
    Qt3DSDMAnimationHandle rhs;
    SAnimationMatcher(IAnimationCore &impl, Qt3DSDMAnimationHandle _rhs)
        : m_Core(impl)
        , rhs(_rhs)
    {
    }
    bool operator()(Qt3DSDMAnimationHandle lhs) const
    {
        SAnimationInfo lhsInfo(m_Core.GetAnimationInfo(lhs));
        SAnimationInfo rhsInfo(m_Core.GetAnimationInfo(rhs));
        QT3DS_ASSERT(lhsInfo.m_Instance == rhsInfo.m_Instance);
        return lhsInfo.m_Property == rhsInfo.m_Property && lhsInfo.m_Index == rhsInfo.m_Index;
    }
};

struct SAttributeNameSorter
{
    IDataCore &m_Core;
    SAttributeNameSorter(IDataCore &inImpl)
        : m_Core(inImpl)
    {
    }

    bool operator()(const pair<Qt3DSDMPropertyHandle, SValue> &lhsPair,
                    const pair<Qt3DSDMPropertyHandle, SValue> &rhsPair)
    {
        QString lhs(m_Core.GetProperty(lhsPair.first).m_Name);
        QString rhs(m_Core.GetProperty(rhsPair.first).m_Name);
        if (lhs == rhs)
            return false;
        if (lhs == QLatin1String("name"))
            return true;
        if (rhs == QLatin1String("name"))
            return false;
        return lhs.compare(rhs) < 0;
    }
};

// Erase all properties that *are* on this instance from the property vector.
struct SMetaDataPropertyEraser
{
    Qt3DSDMInstanceHandle m_Instance;
    SComposerObjectDefinitions &m_MetaData;
    SMetaDataPropertyEraser(Qt3DSDMInstanceHandle inst, SComposerObjectDefinitions &meta)
        : m_Instance(inst)
        , m_MetaData(meta)
    {
    }

    bool operator()(const pair<Qt3DSDMPropertyHandle, SValue> &inProperty)
    {
        bool required = inProperty.first == m_MetaData.m_Named.m_NameProp
                || inProperty.first == m_MetaData.m_Asset.m_SourcePath;
        return !required;
    }
};
using std::unordered_set;
using std::unordered_map;
using std::tuple;

// Algorithm to write is to run through the graph, starting at the root instances
// and write out the instances as we come to them.
struct SComposerSerializerImpl : public IComposerSerializer
{
    typedef QSet<Qt3DSDMInstanceHandle> TInstanceSet;
    typedef QSet<Qt3DSDMSlideHandle> TSlideSet;
    typedef QSet<Qt3DSDMActionHandle> TActionSet;
    typedef vector<Qt3DSDMInstanceHandle> TInstanceList;
    typedef QHash<int, QString> THandleToIdMap;
    typedef QHash<QString, int> TIdToHandleMap;
    typedef QHash<SLong4, int> TGUIDToHandleMap;
    typedef QHash<int, SLong4> THandleToGUIDMap;
    typedef QHash<Qt3DSDMInstanceHandle, int> TInstanceIntMap;
    typedef QHash<Qt3DSDMInstanceHandle, QString> TInstanceToSiblingMap;

    IDataCore &m_DataCore;
    IMetaData &m_MetaData;
    ISlideCore &m_SlideCore;
    IAnimationCore &m_AnimationCore;
    IActionCore &m_ActionCore;
    ISlideSystem &m_SlideSystem;
    ISlideGraphCore &m_SlideGraphCore;
    IActionSystem &m_ActionSystem;
    CGraph &m_AssetGraph;
    IGuideSystem &m_GuideSystem;
    SComposerObjectDefinitions &m_ObjectDefinitions;
    qt3dsdm::IStringTable &m_StringTable;
    std::shared_ptr<Q3DStudio::IImportFailedHandler> m_ImportFailedHandler;

    // The instances we have discovered when we are writing
    THandleToIdMap m_HandleToIdMap;
    TIdToHandleMap m_IdToHandleMap;

    TGUIDToHandleMap m_GUIDToHandleMap;
    THandleToGUIDMap m_HandleToGUIDMap;

    Qt3DSDMSlideHandle m_ActiveSlide;
    Qt3DSDMSlideHandle m_ActiveSlideParent;

    THandleToIdMap m_ActionToIdMap;
    TIdToHandleMap m_IdToActionMap;

    THandleToIdMap m_SlideToIdMap;
    TIdToHandleMap m_IdToSlideMap;

    TInstanceSet m_InstanceSet;
    TSlideSet m_SlideSet;
    TInstanceSet m_ExternalReferences;
    TActionSet m_ExternalActions;
    TSlideSet m_ExternalSlides;

    TInstanceSet m_MasterObjectsSet;
    TIdToHandleMap m_SourcePathToMasterInstances;

    TInstanceIntMap m_InstanceToGraphDepthMap;

    // These are cleared just before use
    MemoryBuffer<RawAllocator> m_TempBuffer;
    MemoryBuffer<RawAllocator> m_ValueBuffer;

    Q3DStudio::Foundation::SStudioFoundation m_Foundation;
    TStreamFactoryPtr m_InputStreamFactory;

    // we want to preserve file ids on save but never on a paste operation.
    bool m_PreserveFileIds;

    TInstanceToSiblingMap m_NewInstancesToSiblings;

    Option<int> m_UIPVersion;

    QString idFromRef(const QString &ref) const
    {
        if (ref.startsWith(QLatin1Char('#')))
            return ref.right(ref.length() - 1);
        return ref;
    }

    SComposerSerializerImpl(IDataCore &inDataCore, IMetaData &inMetaData, ISlideCore &inSlideCore,
                            IAnimationCore &inAnimationCore, IActionCore &inActionCore,
                            CGraph &inAssetGraph, ISlideSystem &inSlideSystem,
                            IActionSystem &inActionSystem, ISlideGraphCore &inSlideGraphCore,
                            SComposerObjectDefinitions &inObjectDefinitions,
                            std::shared_ptr<Q3DStudio::IImportFailedHandler> inFailedHandler,
                            IGuideSystem &inGuideSystem)
        : m_DataCore(inDataCore)
        , m_MetaData(inMetaData)
        , m_SlideCore(inSlideCore)
        , m_AnimationCore(inAnimationCore)
        , m_ActionCore(inActionCore)
        , m_SlideSystem(inSlideSystem)
        , m_SlideGraphCore(inSlideGraphCore)
        , m_ActionSystem(inActionSystem)
        , m_AssetGraph(inAssetGraph)
        , m_GuideSystem(inGuideSystem)
        , m_ObjectDefinitions(inObjectDefinitions)
        , m_StringTable(inDataCore.GetStringTable())
        , m_ImportFailedHandler(inFailedHandler)
        , m_Foundation(Q3DStudio::Foundation::SStudioFoundation::Create())
        , m_InputStreamFactory(Q3DStudio::IInputStreamFactory::Create())
        , m_PreserveFileIds(true)
    {
    }

    void reset()
    {
        m_HandleToIdMap.clear();
        m_IdToHandleMap.clear();

        m_GUIDToHandleMap.clear();
        m_HandleToGUIDMap.clear();

        m_ActiveSlide = 0;
        m_ActiveSlideParent = 0;

        m_ActionToIdMap.clear();
        m_IdToActionMap.clear();

        m_SlideToIdMap.clear();
        m_IdToSlideMap.clear();

        m_InstanceSet.clear();
        m_SlideSet.clear();
        m_ExternalReferences.clear();
        m_ExternalActions.clear();
        m_ExternalSlides.clear();

        m_MasterObjectsSet.clear();
        m_SourcePathToMasterInstances.clear();

        m_InstanceToGraphDepthMap.clear();

        m_NewInstancesToSiblings.clear();

        m_UIPVersion = Option<int>();
    }

    QString AddId(const QString &inId, Qt3DSDMInstanceHandle inHandle)
    {
        QString theIdStr(inId);
        m_IdToHandleMap.insert(theIdStr, inHandle);
        m_HandleToIdMap.insert(inHandle, theIdStr);
        if (m_PreserveFileIds) {
            m_DataCore.SetInstancePropertyValue(inHandle, m_ObjectDefinitions.m_Asset.m_FileId,
                                                std::make_shared<CDataStr>(inId));
        }
        return theIdStr;
    }

    QString SetId(const QString &inId, Qt3DSDMInstanceHandle inHandle)
    {
        QString theIdStr = inId;
        *m_IdToHandleMap.insert(theIdStr, inHandle) = inHandle;
        *m_HandleToIdMap.insert(inHandle, theIdStr) = theIdStr;
        if (m_PreserveFileIds) {
            m_DataCore.SetInstancePropertyValue(inHandle, m_ObjectDefinitions.m_Asset.m_FileId,
                                                std::make_shared<CDataStr>(inId));
        }
        return theIdStr;
    }

    QString AddActionId(const QString &inId, Qt3DSDMActionHandle inHandle)
    {
        QString theIdStr(inId);
        m_IdToActionMap.insert(theIdStr, inHandle);
        m_ActionToIdMap.insert(inHandle, theIdStr);
        return theIdStr;
    }

    QString AddSlideId(const QString &inId, Qt3DSDMSlideHandle inHandle)
    {
        QString theIdStr(inId);
        m_IdToSlideMap.insert(theIdStr, inHandle);
        m_SlideToIdMap.insert(inHandle, theIdStr);
        return theIdStr;
    }

    Qt3DSDMInstanceHandle GetInstanceById(const QString &inId)
    {
        if (IsTrivial(inId))
            return 0;

        TIdToHandleMap::iterator find = m_IdToHandleMap.find(idFromRef(inId));
        if (find != m_IdToHandleMap.end())
            return *find;
        return 0;
    }

    Qt3DSDMSlideHandle GetSlideById(const QString &inId)
    {
        if (IsTrivial(inId))
            return 0;

        TIdToHandleMap::iterator find = m_IdToSlideMap.find(idFromRef(inId));
        if (find != m_IdToSlideMap.end())
            return *find;
        return 0;
    }

    Qt3DSDMActionHandle GetActionById(const QString &inId)
    {
        if (IsTrivial(inId))
            return 0;

        TIdToHandleMap::iterator find = m_IdToActionMap.find(idFromRef(inId));
        if (find != m_IdToActionMap.end())
            return *find;
        return 0;
    }

    void AddGuid(SLong4 inId, int inHandle)
    {
        m_GUIDToHandleMap.insert(inId, inHandle);
        m_HandleToGUIDMap.insert(inHandle, inId);
    }

    SLong4 GetInstanceGuid(Qt3DSDMInstanceHandle inInstance)
    {
        THandleToGUIDMap::iterator find = m_HandleToGUIDMap.find(inInstance);
        if (find != m_HandleToGUIDMap.end())
            return *find;
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(inInstance, m_ObjectDefinitions.m_Guided.m_GuidProp,
                                                theValue)) {
            SLong4 theGuid = qt3dsdm::get<SLong4>(theValue);
            AddGuid(theGuid, inInstance);
            return theGuid;
        }
        return SLong4();
    }

    QString GetInstanceName(Qt3DSDMInstanceHandle inInstance)
    {
        Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
        SValue theValue;
        if (theAssociatedSlide.Valid()
                && m_SlideCore.GetSpecificInstancePropertyValue(theAssociatedSlide, inInstance,
                                                                m_ObjectDefinitions.m_Named.m_NameProp,
                                                                theValue)) {
            TDataStrPtr theName(get<TDataStrPtr>(theValue));
            QString retval = theName->toQString();
            if (!retval.isEmpty())
                return retval;
        }
        SValue theDCValue;
        if (m_DataCore.GetInstancePropertyValue(inInstance, m_ObjectDefinitions.m_Named.m_NameProp,
                                                theDCValue)) {
            TDataStrPtr theName(get<TDataStrPtr>(theDCValue));
            QString retval = theName->toQString();
            if (!retval.isEmpty())
                return retval;
        }
        Option<QString> theTypeStr = m_MetaData.GetTypeForInstance(inInstance);
        if (theTypeStr.hasValue())
            return theTypeStr;
        QT3DS_ASSERT(false);
        return {};
    }

    QString GetInstanceName(Qt3DSDMInstanceHandle inInstance, Qt3DSDMSlideHandle inSlide)
    {
        SValue theValue;
        if (m_SlideCore.GetSpecificInstancePropertyValue(
                    inSlide, inInstance, m_ObjectDefinitions.m_Named.m_NameProp, theValue)) {
            TDataStrPtr theName(get<TDataStrPtr>(theValue));
            QString retval = theName->toQString();
            if (!retval.isEmpty())
                return retval;
        }
        return GetInstanceName(inInstance);
    }

    QString GetId(const QString &inIdStem)
    {
        // Create an ID for this instance
        QString theTypeStr(inIdStem);
        QString theTypeStem(theTypeStr);
        QStringList split = theTypeStr.split(QLatin1Char('_'));
        // remove number from the end
        if (split.size() > 1) {
            bool ok = false;
            split.last().toLong(&ok);
            if (ok)
                theTypeStr = theTypeStr.left(theTypeStr.length() - split.last().length() - 1);
        }

        uint idIdx = 1;

        while (m_IdToActionMap.keys().contains(theTypeStr)
               || m_IdToHandleMap.keys().contains(theTypeStr)
               || m_IdToSlideMap.keys().contains(theTypeStr)) {
            QString id = QString("_%1").arg(idIdx, 3, 10, QChar('0'));
            theTypeStr = theTypeStem + id;
            ++idIdx;
        }
        return theTypeStr;
    }

    QString GetInstanceId(Qt3DSDMInstanceHandle inInstance)
    {
        QT3DS_ASSERT(inInstance.Valid());
        THandleToIdMap::iterator theFind(m_HandleToIdMap.find(inInstance));
        if (theFind != m_HandleToIdMap.end())
            return *theFind;

        QString theName(GetInstanceName(inInstance));
        Option<QString> theType = m_MetaData.GetTypeForInstance(inInstance);
        if (theType.hasValue() == false) {
            QT3DS_ASSERT(false);
            return {};
        }

        // for most instances we just want a simple id based on the object name.
        // for images, however, we want do to something else.
        QString theNewId;
        if (m_DataCore.IsInstanceOrDerivedFrom(inInstance,
                                               m_ObjectDefinitions.m_Image.m_Instance)) {
            Qt3DSDMInstanceHandle theMaterial = m_AssetGraph.GetParent(inInstance);
            if (theMaterial.Valid()) {
                QString theIdStr(GetInstanceId(theMaterial));
                SLong4 theGuid = GetInstanceGuid(inInstance);
                theIdStr += QLatin1Char('_');
                TPropertyHandleList theProperties;
                m_DataCore.GetAggregateInstanceProperties(theMaterial, theProperties);
                Qt3DSDMPropertyHandle theProperty;
                for (size_t propIdx = 0, propEnd = theProperties.size();
                     propIdx < propEnd && theProperty.Valid() == false; ++propIdx) {
                    SValue theValue;
                    const Qt3DSDMPropertyDefinition &theDefinition(
                                m_DataCore.GetProperty(theProperties[propIdx]));
                    if (theDefinition.m_Type == DataModelDataType::Long4) {
                        SValue theDCValue;
                        if (m_DataCore.GetInstancePropertyValue(theMaterial, theProperties[propIdx],
                                                                theDCValue)) {
                            SLong4 thePropGuid = get<SLong4>(theDCValue);
                            if (thePropGuid == theGuid)
                                theProperty = theProperties[propIdx];
                        }
                        Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
                        if (theProperty.Valid() == false && theSlide.Valid()
                                && m_SlideCore.GetSpecificInstancePropertyValue(
                                    theSlide, theMaterial, theProperties[propIdx], theValue)) {
                            SLong4 thePropGuid = get<SLong4>(theValue);
                            if (thePropGuid == theGuid)
                                theProperty = theProperties[propIdx];
                        }
                    }
                }
                if (theProperty.Valid()) {
                    theIdStr.append(m_DataCore.GetProperty(theProperty).m_Name);
                    theNewId = GetId(theIdStr);
                }
            }
        }

        if (IsTrivial(theNewId))
            theNewId = GetId(theName);
        return AddId(theNewId, inInstance);
    }

    QString GetActionId(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                         Qt3DSDMInstanceHandle inInstance)
    {
        QT3DS_ASSERT(inAction.Valid());
        THandleToIdMap::iterator theFind(m_ActionToIdMap.find(inAction));
        if (theFind != m_ActionToIdMap.end())
            return *theFind;

        QString theActionName(GetInstanceName(inInstance, inSlide));
        theActionName.append(QStringLiteral("-Action"));

        QString theNewId = GetId(theActionName);
        return AddActionId(theNewId, inAction);
    }

    // If this function is called with an invalid instance and we don't already have an id
    // then we assume we have an external reference and lookup the instance via the component id.
    QString GetSlideId(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance)
    {
        QT3DS_ASSERT(inSlide.Valid());
        THandleToIdMap::iterator theFind(m_SlideToIdMap.find(inSlide));
        if (theFind != m_SlideToIdMap.end())
            return *theFind;

        if (inInstance.Valid() == false) {
            m_ExternalSlides.insert(inSlide);
            Qt3DSDMSlideHandle theMaster = m_SlideSystem.GetMasterSlide(inSlide);
            Qt3DSDMInstanceHandle theSlideData = m_SlideCore.GetSlideInstance(theMaster);
            SValue theValue;
            m_DataCore.GetInstancePropertyValue(
                        theSlideData, m_ObjectDefinitions.m_Slide.m_ComponentId, theValue);
            SLong4 theComponent = get<SLong4>(theValue);
            inInstance = FindInstanceByGUID(theComponent);
            QT3DS_ASSERT(inInstance.Valid());
        }

        const QString theSlideName = GetInstanceName(inInstance) + QLatin1Char('-')
                    + GetSlideName(inSlide);
        const QString theNewId = GetId(theSlideName);
        return AddSlideId(theNewId, inSlide);
    }

    SLong4 GetGuid(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty)
    {
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(inInstance, inProperty, theValue)) {
            SLong4 theLongValue(get<SLong4>(theValue));
            if (theLongValue.m_Longs[0] || theLongValue.m_Longs[1] || theLongValue.m_Longs[2]
                    || theLongValue.m_Longs[3])
                return theLongValue;
        }
        Qt3DSDMSlideHandle theSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
        SValue theSlideValue;
        if (theSlide.Valid()
                && m_SlideCore.GetInstancePropertyValue(theSlide, inInstance, inProperty,
                                                        theSlideValue)) {
            SLong4 theLongValue(get<SLong4>(theSlideValue));
            return theLongValue;
        }
        return SLong4();
    }

    void GetAllInstanceGuids()
    {
        TInstanceHandleList theInstances;
        m_DataCore.GetInstancesDerivedFrom(theInstances, m_ObjectDefinitions.m_Guided.m_Instance);
        // sort the instances so parents appear before children.  This stabilizes the ids used
        // somewhat.
        std::sort(theInstances.begin(), theInstances.end(), std::less<int>());
        for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
            Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
            SLong4 theGuid = GetGuid(theInstance, m_ObjectDefinitions.m_Guided.m_GuidProp);
            if (theGuid.Valid()) {
                AddGuid(theGuid, theInstance);
            }
            SValue theInstanceIdValue;
            if (m_DataCore.GetInstancePropertyValue(
                        theInstance, m_ObjectDefinitions.m_Asset.m_FileId, theInstanceIdValue)) {
                QString theName = qt3dsdm::get<TDataStrPtr>(theInstanceIdValue)->toQString();
                if (!theName.isEmpty()) {
                    const QString theId = GetId(theName);
                    AddId(theId, theInstance);
                }
            }
        }
    }

    void BuildSourcePathMasterObjectMap()
    {
        TInstanceHandleList theInstances;
        TInstanceHandleList theParents;
        // We need to check the source path.
        m_DataCore.GetInstancesDerivedFrom(theInstances, m_ObjectDefinitions.m_Asset.m_Instance);
        for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
            Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
            // Find master objects by finding all asset instances that derive from non-canonical
            // instances.
            // these non-canonical parents *must* be master objects.
            theParents.clear();
            m_DataCore.GetInstanceParents(theInstance, theParents);
            if (theParents.size() != 1)
                continue;
            // If the parent is canonical then continue;
            if (m_MetaData.GetTypeForCanonicalInstance(theParents[0]).hasValue())
                continue;
            // Finally, this must mean the object is canonical.
            Qt3DSDMInstanceHandle theMaster(theParents[0]);
            SValue theValue;
            if (m_DataCore.GetInstancePropertyValue(
                        theMaster, m_ObjectDefinitions.m_Asset.m_SourcePath, theValue)) {
                TDataStrPtr theStr(get<TDataStrPtr>(theValue));
                QString thePath(theStr->toQString());
                if (!IsTrivial(thePath))
                    m_SourcePathToMasterInstances.insert(thePath, theMaster);
            }
        }
    }

    Qt3DSDMInstanceHandle FindInstanceByGUID(SLong4 theGuid)
    {
        if (m_GUIDToHandleMap.size() == 0)
            GetAllInstanceGuids();
        if (theGuid.Valid() == false)
            return 0;

        TGUIDToHandleMap::iterator theIter(m_GUIDToHandleMap.find(theGuid));
        if (theIter != m_GUIDToHandleMap.end())
            return *theIter;
        return 0;
    }

    const QString WriteDataModelValue(const SValue &_inValue, QString &theValueStr)
    {
        DataModelDataType::Value theValueType(GetValueType(_inValue));
        SValue theValue(_inValue);
        // Transform the value into something we can deal with.
        if (theValueType == DataModelDataType::ObjectRef) {
            const SObjectRefType &theRef(get<SObjectRefType>(theValue));
            switch (theRef.GetReferenceType()) {
            case ObjectReferenceType::Absolute:
                theValue = get<SLong4>(theRef.m_Value);
                break;
            case ObjectReferenceType::Relative:
                theValue = get<TDataStrPtr>(theRef.m_Value);
                break;
            default:
                theValue = SLong4();
                break;
            }
        } else if (theValueType == DataModelDataType::StringOrInt) {
            const SStringOrInt &theData(get<SStringOrInt>(theValue));
            if (theData.GetType() == SStringOrIntTypes::Int) {
                Qt3DSDMSlideHandle theHandle(get<long>(theData.m_Value));
                const QString theSlideId(QLatin1Char('#') + GetSlideId(theHandle, 0));
                theValue = std::make_shared<CDataStr>(theSlideId);
            } else
                theValue = get<TDataStrPtr>(theData.m_Value);
        }
        // Resolve SLong4 properties;
        if (GetValueType(theValue) == DataModelDataType::Long4) {
            SLong4 theDataValue = get<SLong4>(theValue);
            Qt3DSDMInstanceHandle theInstance = FindInstanceByGUID(theDataValue);
            if (theInstance.Valid() == false) {
                theValueStr.clear();
            } else {
                theValueStr = QLatin1Char('#') + GetInstanceId(theInstance);
                if (m_InstanceSet.find(theInstance) == m_InstanceSet.end())
                    m_ExternalReferences.insert(theInstance);
            }
        } else {
            m_TempBuffer.clear();
            WCharTWriter theWriter(m_TempBuffer);
            WStrOps<SValue>().ToBuf(theValue, theWriter);

            if (GetValueType(theValue) == DataModelDataType::String || m_TempBuffer.size()) {
                char buffer[] = { 0, 0, 0, 0 };
                m_TempBuffer.write(buffer, 4);
                theValueStr = QString::fromWCharArray((const wchar_t *)m_TempBuffer.begin());
            }
        }
        return theValueStr;
    }

    SValue ParseValue(DataModelDataType::Value inType, const QString &inValue)
    {
        if (IsTrivial(inValue)) {
            SValue retval;
            SetDefault(inType, retval);
            return retval;
        }

        if (inType == DataModelDataType::ObjectRef)
            return ParseObjectRef(inValue);

        if (inType == DataModelDataType::StringOrInt) {
            if (inValue.startsWith(QLatin1Char('#'))) {
                Qt3DSDMSlideHandle theSlide = GetSlideById(idFromRef(inValue));
                QT3DS_ASSERT(theSlide.Valid());
                return SStringOrInt((long)theSlide.GetHandleValue());
            } else
                return SStringOrInt(std::make_shared<CDataStr>(inValue));
        }

        if (inType == DataModelDataType::String)
            return std::make_shared<CDataStr>(inValue);

        DataModelDataType::Value theParseType(inType);
        if (inType == DataModelDataType::Long4)
            theParseType = DataModelDataType::StringRef;

        QByteArray data(inValue.toLatin1());
        m_ValueBuffer.clear();
        m_ValueBuffer.write(data.data(), data.size() + 1);
        m_TempBuffer.clear();

        WCharTReader theReader((char8_t *)m_ValueBuffer.begin(), m_TempBuffer, m_StringTable);
        SValue retval = WStrOps<SValue>().BufTo(theParseType, theReader);

        if (inType == DataModelDataType::Long4) {
            SLong4 theFinalValue;
            SStringRef theRef(get<SStringRef>(retval));
            Qt3DSDMInstanceHandle theRefInstance(
                        GetInstanceById(QString::fromWCharArray(theRef.m_Id)));
            if (theRefInstance.Valid()) {
                THandleToGUIDMap::iterator theGuidFind = m_HandleToGUIDMap.find(theRefInstance);
                if (theGuidFind != m_HandleToGUIDMap.end())
                    theFinalValue = *theGuidFind;
            }
            return theFinalValue;
        }
        return retval;
    }
    const QString WriteObjectRef(const SObjectRefType &inValue, QString &ioValueStr)
    {
        return WriteDataModelValue(SValue(inValue), ioValueStr);
    }

    SObjectRefType ParseObjectRef(TCharPtr inValue)
    {
        // One of two things, either an ID reference *or* pure string.
        if (IsTrivial(inValue))
            return SObjectRefType(SLong4());
        if (inValue[0] == '#') {
            // absolute reference.
            Qt3DSDMInstanceHandle theInstance
                    = GetInstanceById(QString::fromWCharArray(inValue + 1));
            return theInstance.Valid() ? GetInstanceGuid(theInstance) : 0;
        } else {
            return std::make_shared<CDataStr>(inValue);
        }
    }

    SObjectRefType ParseObjectRef(const QString &inValue)
    {
        // One of two things, either an ID reference *or* pure string.
        if (IsTrivial(inValue))
            return SObjectRefType(SLong4());
        if (inValue.startsWith(QLatin1Char('#'))) {
            // absolute reference.
            Qt3DSDMInstanceHandle theInstance = GetInstanceById(idFromRef(inValue));
            return theInstance.Valid() ? GetInstanceGuid(theInstance) : 0;
        } else {
            return std::make_shared<CDataStr>(inValue);
        }
    }

    void SerializePropertyList(qt3dsdm::IDOMWriter &inWriter, TPropertyHandleValuePairList &inList)
    {
        sort(inList.begin(), inList.end(), SAttributeNameSorter(m_DataCore));

        QString theValueStr;

        for (size_t idx = 0, end = inList.size(); idx < end; ++idx) {
            const pair<Qt3DSDMPropertyHandle, SValue> &theValue(inList[idx]);
            QString theName(m_DataCore.GetProperty(theValue.first).m_Name);
            WriteDataModelValue(theValue.second, theValueStr);
            if (GetValueType(theValue.second) == DataModelDataType::String || theValueStr.size())
                inWriter.Att(theName, theValueStr);
        }
    }

    struct SAnimationHandleLessThan
    {
        IDataCore &m_DataCore;
        IAnimationCore &m_AnimationCore;
        SAnimationHandleLessThan(IDataCore &inDC, IAnimationCore &inAc)
            : m_DataCore(inDC)
            , m_AnimationCore(inAc)
        {
        }

        inline bool operator()(Qt3DSDMAnimationHandle lhs, Qt3DSDMAnimationHandle rhs) const
        {
            SAnimationInfo lhsInfo(m_AnimationCore.GetAnimationInfo(lhs));
            SAnimationInfo rhsInfo(m_AnimationCore.GetAnimationInfo(rhs));
            const Qt3DSDMPropertyDefinition &theLhsDef(m_DataCore.GetProperty(lhsInfo.m_Property));
            const Qt3DSDMPropertyDefinition &theRhsDef(m_DataCore.GetProperty(rhsInfo.m_Property));
            int theComparison = theLhsDef.m_Name.compare(theRhsDef.m_Name);
            if (theComparison == 0)
                return lhsInfo.m_Index < rhsInfo.m_Index;
            return theComparison < 0;
        }
    };

    void SerializeAnimations(IDOMWriter &inWriter, TAnimationHandleList &inAnimations)
    {
        TKeyframeHandleList theKeyframes;
        vector<float> theValues;
        std::stable_sort(inAnimations.begin(), inAnimations.end(),
                         SAnimationHandleLessThan(m_DataCore, m_AnimationCore));
        for (size_t idx = 0, end = inAnimations.size(); idx < end; ++idx) {
            theKeyframes.clear();
            theValues.clear();

            IDOMWriter::Scope __animScope(inWriter, L"AnimationTrack");

            SAnimationInfo theInfo(m_AnimationCore.GetAnimationInfo(inAnimations[idx]));
            Qt3DSDMMetaDataPropertyHandle theMetaDataProperty =
                    m_MetaData.GetMetaDataProperty(theInfo.m_Instance, theInfo.m_Property);
            if (theMetaDataProperty.Valid() == false)
                continue;

            SMetaDataPropertyInfo thePropertyInfo(
                        m_MetaData.GetMetaDataPropertyInfo(theMetaDataProperty));
            QString theName = thePropertyInfo.m_Name;
            size_t theArity = get<1>(GetDatatypeAnimatableAndArity(thePropertyInfo.GetDataType()));
            if (theArity > 1) {
                theName.append(QLatin1Char('.'));
                switch (theInfo.m_Index) {
                case 0:
                    theName.append(QLatin1Char('x'));
                    break;
                case 1:
                    theName.append(QLatin1Char('y'));
                    break;
                case 2:
                    theName.append(QLatin1Char('z'));
                    break;
                case 3:
                    theName.append(QLatin1Char('w'));
                    break;
                }
            }
            const QString name(theName);
            inWriter.Att(QStringLiteral("property"), name);
            inWriter.Att(QStringLiteral("type"), theInfo.m_AnimationType);

            if (theInfo.m_DynamicFirstKeyframe)
                inWriter.Att("dynamic", true);

            if (!theInfo.m_ArtistEdited)
                inWriter.Att("artistedited", theInfo.m_ArtistEdited);

            m_AnimationCore.GetKeyframes(inAnimations[idx], theKeyframes);

            switch (theInfo.m_AnimationType) {
            case EAnimationTypeLinear:
                WriteKeyframes<SLinearKeyframe>(theKeyframes, m_AnimationCore, theValues);
                break;
            case EAnimationTypeBezier:
                WriteKeyframes<SBezierKeyframe>(theKeyframes, m_AnimationCore, theValues);
                break;
            case EAnimationTypeEaseInOut:
                WriteKeyframes<SEaseInEaseOutKeyframe>(theKeyframes, m_AnimationCore, theValues);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
            if (theValues.size()) {
                m_TempBuffer.clear();
                WCharTWriter theWriter(m_TempBuffer);
                theWriter.Write(toConstDataRef(&theValues[0], (QT3DSU32)theValues.size()), 30,
                        inWriter.GetTabs() + 1);
                wchar_t buffer = L'\0';
                theWriter.Write(buffer);
                inWriter.Value((const wchar_t *)m_TempBuffer.begin());
            }
        }
    }

    void ParseAnimation(IDOMReader &inReader, Qt3DSDMSlideHandle inSlide,
                        Qt3DSDMInstanceHandle inInstance)
    {
        IDOMReader::Scope __animScope(inReader);
        EAnimationType theAnimationType;
        TCharStr thePropertyName;
        bool firstKeyframeDynamic = false;

        inReader.Att(L"property", thePropertyName);
        inReader.Att(L"type", theAnimationType);
        inReader.Att("dynamic", firstKeyframeDynamic);
        bool artistEdited;
        if (!inReader.Att("artistedited", artistEdited))
            artistEdited = true;

        uint32_t subIndex = 0;
        uint32_t theSeparator = thePropertyName.rfind('.');
        if (theSeparator != TCharStr::npos) {
            wchar_t theEndItem = thePropertyName[theSeparator + 1];
            thePropertyName = thePropertyName.substr(0, theSeparator);
            switch (theEndItem) {
            // sub index is already 0
            case 'x':
            case 'r':
                break;
            case 'y':
            case 'g':
                subIndex = 1;
                break;
            case 'z':
            case 'b':
                subIndex = 2;
                break;
            case 'w':
            case 'a':
                subIndex = 3;
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
        Qt3DSDMMetaDataPropertyHandle theProperty =
                m_MetaData.GetMetaDataProperty(inInstance,
                                               QString::fromWCharArray(thePropertyName.wide_str()));
        if (theProperty.Valid() == false) {
            QT3DS_ASSERT(false);
            return;
        }
        SMetaDataPropertyInfo theInfo(m_MetaData.GetMetaDataPropertyInfo(theProperty));
        std::tuple<bool, size_t> theAnimAndArity = GetDatatypeAnimatableAndArity(theInfo.GetDataType());
        if (std::get<0>(theAnimAndArity) == false) {
            QT3DS_ASSERT(false);
            return;
        }
        if (std::get<1>(theAnimAndArity) <= subIndex) {
            QT3DS_ASSERT(false);
            return;
        }

        // When duplicating a slide the system will copy values from the master slide to the child
        // slide.
        // Then it will expect the file saved out to apply itself to the child slide.  If the source
        // slide
        // has an animation in this slot then it is conceivable the animation here already exists,
        // and we really
        // just need to apply the properties and keyframes we read from the file to this animation.
        // This hack fix should only apply to instances where we have an active slide (i.e.
        // duplicate slide).
        if (m_ActiveSlide == inSlide) {
            // Simple way of overriding all animation info:  Delete the object and re-create.
            Qt3DSDMAnimationHandle animHandle =
                    m_AnimationCore.GetAnimation(inSlide, inInstance, theInfo.m_Property, subIndex);
            if (animHandle.Valid() == true)
                m_AnimationCore.DeleteAnimation(animHandle);
        }
        Qt3DSDMAnimationHandle theAnimation =
                m_AnimationCore.CreateAnimation(inSlide, inInstance, theInfo.m_Property, subIndex,
                                                theAnimationType, firstKeyframeDynamic);
        m_ValueBuffer.clear();
        m_TempBuffer.clear();
        const char8_t *theElemValue;
        inReader.Value(theElemValue);
        if (!IsTrivial(theElemValue)) {
            // Create temporary we can destructively parse
            // Create a destructible value
            m_ValueBuffer.clear();
            m_ValueBuffer.write(theElemValue, (QT3DSU32)strlen(theElemValue) + 1);
            // Clear the destination buffer
            m_TempBuffer.clear();
            NVConstDataRef<float> theFloatValues;
            WCharTReader theReader((char8_t *)m_ValueBuffer.begin(), m_TempBuffer, m_StringTable);
            theReader.ReadBuffer(theFloatValues);
            const float *theStart = theFloatValues.begin(), *theEnd = theFloatValues.end();
            switch (theAnimationType) {
            case EAnimationTypeLinear:
                ReadKeyframes<SLinearKeyframe>(theAnimation, m_AnimationCore, theStart, theEnd);
                break;
            case EAnimationTypeBezier:
                ReadKeyframes<SBezierKeyframe>(theAnimation, m_AnimationCore, theStart, theEnd);
                break;
            case EAnimationTypeEaseInOut:
                ReadKeyframes<SEaseInEaseOutKeyframe>(theAnimation, m_AnimationCore, theStart,
                                                      theEnd);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
        m_AnimationCore.SetIsArtistEdited(theAnimation, artistEdited);
    }

    void SerializeAction(IDOMWriter &inWriter, Qt3DSDMSlideHandle inSlide,
                         Qt3DSDMInstanceHandle inInstance, Qt3DSDMActionHandle inAction)
    {
        QString valueStr;
        IDOMWriter::Scope __actionScope(inWriter, L"Action");

        Qt3DSDMActionHandle theAction(inAction);
        SActionInfo theInfo(m_ActionCore.GetActionInfo(theAction));
        inWriter.Att(L"id", GetActionId(theAction, inSlide, inInstance));
        bool eyeball = m_ActionSystem.GetActionEyeballValue(inSlide, theAction);
        inWriter.Att("eyeball", eyeball);

        WriteObjectRef(theInfo.m_TriggerObject, valueStr);
        inWriter.Att(L"triggerObject", valueStr);

        if (theInfo.m_Event.size())
            inWriter.Att(L"event", theInfo.m_Event);

        WriteObjectRef(theInfo.m_TargetObject, valueStr);
        inWriter.Att(L"targetObject", valueStr);

        if (theInfo.m_Handler.size()) {
            inWriter.Att(L"handler", theInfo.m_Handler);

            for (size_t handlerArgIdx = 0, handlerArgEnd = theInfo.m_HandlerArgs.size();
                 handlerArgIdx < handlerArgEnd; ++handlerArgIdx) {
                SHandlerArgumentInfo theArgInfo(
                            m_ActionCore.GetHandlerArgumentInfo(theInfo.m_HandlerArgs[handlerArgIdx]));
                IDOMWriter::Scope argScope(inWriter, L"HandlerArgument");
                inWriter.Att(L"name", theArgInfo.m_Name);

                DataModelDataType::Value theArgType(GetValueType(theArgInfo.m_Value));
                SValue theArgValue(theArgInfo.m_Value);
                if (theArgInfo.m_ArgType == HandlerArgumentType::Event) {
                    theArgType = DataModelDataType::String;
                    auto theEventHandle = get<qt3ds::QT3DSI32>(theArgInfo.m_Value);
                    theArgValue = SValue(std::make_shared<CDataStr>(
                                             m_MetaData.GetEventInfo(theEventHandle)->m_Name));
                }

                if (theArgType != DataModelDataType::Float)
                    inWriter.Att(L"type", theArgType);
                if (theArgInfo.m_ArgType != HandlerArgumentType::None)
                    inWriter.Att(L"argtype", theArgInfo.m_ArgType);

                SValue theDefault;
                SetDefault(theArgType, theDefault);
                if (!Equals(theArgInfo.m_Value, theDefault)) {
                    WriteDataModelValue(theArgValue, valueStr);
                    inWriter.Att(L"value", valueStr);
                }
            }
        }
    }

    void SerializeActions(IDOMWriter &inWriter, Qt3DSDMSlideHandle inSlide,
                          Qt3DSDMInstanceHandle inInstance, TActionHandleList &inActions)
    {
        // We sort the actions just by action handle.  This keeps the file in the same order because
        // actions are created in file order and their handle values increment monotonically.
        std::stable_sort(inActions.begin(), inActions.end(), std::less<int>());
        for (size_t idx = 0, end = inActions.size(); idx < end; ++idx) {
            SerializeAction(inWriter, inSlide, inInstance, inActions[idx]);
        }
    }

    Qt3DSDMInstanceHandle ResolveObjectRef(Qt3DSDMInstanceHandle inInstance,
                                          const SObjectRefType &inRef)
    {
        if (inRef.GetReferenceType() == ObjectReferenceType::Absolute) {
            SLong4 theGuid = get<SLong4>(inRef.m_Value);
            if (theGuid.Valid())
                return FindInstanceByGUID(theGuid);
            return 0;
        }
        TDataStrPtr theStrPtr(get<TDataStrPtr>(inRef.m_Value));
        if (theStrPtr == NULL)
            return 0;
        QString theParseStr(theStrPtr->toQString());

        // Get rid of this or this. since it's referrng to inInstance
        QString theThisStr(QStringLiteral("this"));
        if (theParseStr.left(theThisStr.size()) == theThisStr) {
            theParseStr = theParseStr.left(theParseStr.length() - theThisStr.size());
            if (theParseStr[0] == QLatin1Char('.'))
                theParseStr = theParseStr.right(theParseStr.length() - 1);
        }

        QString theParentStr(QStringLiteral("parent."));
        Qt3DSDMInstanceHandle theSourceInstance(inInstance);
        while (theParseStr.indexOf(theParentStr) != -1) {
            Qt3DSDMInstanceHandle theParentInstance = m_AssetGraph.GetParent(inInstance);
            // this check is here since scene has no parent, see Bug#6532
            if (theParentInstance.Valid())
                theSourceInstance = theParentInstance;
            else
                return Qt3DSDMInstanceHandle(0);
            theParseStr = theParseStr.right(theParseStr.length() - theParentStr.size());
        }
        while (theParseStr.size()) {
            int periodPos = theParseStr.indexOf(QChar('.'));
            QString theNameStr = theParseStr;
            if (periodPos != -1) {
                theNameStr = theParseStr.left(periodPos);
                theParseStr = theParseStr.right(theParseStr.length() - periodPos + 1);
            } else {
                theParseStr.clear();
            }
            Qt3DSDMInstanceHandle theParent(theSourceInstance);
            bool theFound = false;
            for (long childIdx = 0, childEnd = m_AssetGraph.GetChildCount(theSourceInstance);
                 childIdx < childEnd && !theFound; ++childIdx) {
                Qt3DSDMInstanceHandle theChild = m_AssetGraph.GetChild(theSourceInstance, childIdx);
                if (theNameStr == GetInstanceName(theChild)) {
                    theSourceInstance = theChild;
                    theFound = true;
                }
            }
            if (!theFound)
                return Qt3DSDMInstanceHandle(0);
        }
        return theSourceInstance;
    }

    Qt3DSDMActionHandle ParseAction(IDOMReader &inReader, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inInstance)
    {
        IDOMReader::Scope __actionScope(inReader);
        Qt3DSDMActionHandle theAction;
        QString theActionId;
        bool isRef = false;
        if (inReader.Att(L"id", theActionId)) {
            theAction = m_ActionSystem.CreateAction(inSlide, inInstance, SLong4());
            AddActionId(theActionId, theAction);
        } else if (inReader.Att(L"ref", theActionId)) {
            theAction = GetActionById(theActionId + 1);
            isRef = true;
        }
        bool eyeball = false;
        inReader.Att("eyeball", eyeball);
        m_ActionSystem.SetActionEyeballValue(inSlide, theAction, eyeball);
        // Referenced actions can only set eyeball property on various slides, they can't
        // set anything else, so we can return safely here.
        if (isRef)
            return theAction;

        QString tempStr;
        if (inReader.Att(L"triggerObject", tempStr))
            m_ActionCore.SetTriggerObject(theAction, ParseObjectRef(tempStr));
        if (inReader.Att(L"event", tempStr))
            m_ActionCore.SetEvent(theAction, tempStr);
        SObjectRefType theTargetRef;
        if (inReader.Att(L"targetObject", tempStr)) {
            theTargetRef = ParseObjectRef(tempStr);
            m_ActionCore.SetTargetObject(theAction, theTargetRef);
        }

        Qt3DSDMHandlerHandle theHandler;
        if (inReader.Att(L"handler", tempStr)) {
            m_ActionCore.SetHandler(theAction, tempStr);

            for (bool success = inReader.MoveToFirstChild(); success;
                 success = inReader.MoveToNextSibling()) {
                QString theName;
                inReader.Att(L"name", theName);
                DataModelDataType::Value theDataType(DataModelDataType::Float);
                inReader.Att(L"type", theDataType);
                HandlerArgumentType::Value theArgType(HandlerArgumentType::None);
                inReader.Att(L"argtype", theArgType);

                inReader.Att(L"value", tempStr);
                SValue theValue = ParseValue(theDataType, tempStr);
                if (theArgType == HandlerArgumentType::Event) {
                    TDataStrPtr theStr;
                    if (GetValueType(theValue) == DataModelDataType::String)
                        theStr = get<TDataStrPtr>(theValue);
                    if (theStr) {
                        Qt3DSDMInstanceHandle theTargetInstance =
                                ResolveObjectRef(inInstance, theTargetRef);
                        Qt3DSDMEventHandle theEvent = 0;
                        if (theTargetInstance.Valid())
                            theEvent = m_MetaData.FindEvent(theTargetInstance, theStr->toQString());
                        theValue = SValue((qt3ds::QT3DSI32) theEvent);
                    }
                }

                Qt3DSDMHandlerArgHandle theArgHandle = m_ActionCore.AddHandlerArgument(
                            theAction, theName, theArgType, theDataType);
                m_ActionCore.SetHandlerArgumentValue(theArgHandle, theValue);
            }
        }
        return theAction;
    }

    void GetInstanceSlideInformation(Qt3DSDMInstanceHandle inInstance, Qt3DSDMSlideHandle inSlide,
                                     Qt3DSDMSlideHandle inSlideParent,
                                     TPropertyHandleValuePairList &ioValues,
                                     TAnimationHandleList &ioAnimations,
                                     TActionHandleList &ioActions)
    {
        m_SlideCore.GetSpecificInstancePropertyValues(inSlide, inInstance, ioValues);
        m_AnimationCore.GetSpecificInstanceAnimations(inSlide, inInstance, ioAnimations);
        m_ActionCore.GetActions(inSlide, inInstance, ioActions);

        // During Partial serialization  we pull parent properties and such up onto the active
        // slide during write process, overriding where necessary
        if (inSlideParent.Valid()) {
            TPropertyHandleValuePairList theParentValues;
            TAnimationHandleList theParentAnimations;
            TActionHandleList theParentActions;

            m_SlideCore.GetSpecificInstancePropertyValues(inSlideParent, inInstance,
                                                          theParentValues);
            m_AnimationCore.GetSpecificInstanceAnimations(inSlideParent, inInstance,
                                                          theParentAnimations);
            m_ActionCore.GetActions(inSlideParent, inInstance, theParentActions);

            // Ensure the current slide values override the parent slide values.
            for (size_t idx = 0, end = theParentValues.size(); idx < end; ++idx)
                insert_unique_if(ioValues, theParentValues[idx],
                                 SPropertyMatcher(theParentValues[idx].first));

            for (size_t idx = 0, end = theParentAnimations.size(); idx < end; ++idx)
                insert_unique_if(ioAnimations, theParentAnimations[idx],
                                 SAnimationMatcher(m_AnimationCore, theParentAnimations[idx]));

            // And we just blindly copy over all the actions because actions can't override each
            // other.
            ioActions.insert(ioActions.end(), theParentActions.begin(), theParentActions.end());
        }

        SanitizeHandleValuePairList(inInstance, ioValues);
        Qt3DSDMPropertyHandle *theUnlinked(
                    IDocumentEditor::GetAlwaysUnlinkedProperties(m_ObjectDefinitions));
        // Note that we are explicitly re-querying the iovalues size.
        for (size_t idx = 0; idx < ioValues.size(); ++idx) {
            Qt3DSDMPropertyHandle theProperty(ioValues[idx].first);
            for (Qt3DSDMPropertyHandle *theHandle = theUnlinked; theHandle->Valid(); ++theHandle) {
                if (theProperty == *theHandle
                        && Equals(ioValues[idx].second.toOldSkool(),
                                  m_MetaData.GetDefaultValue(inInstance, theProperty).toOldSkool())) {
                    ioValues.erase(ioValues.begin() + idx);
                    --idx;
                }
            }
        }
    }

    QString GetSlideName(Qt3DSDMSlideHandle inSlide)
    {
        if (inSlide.Valid() == false)
            return {};
        Qt3DSDMInstanceHandle theSlideInstance(m_SlideCore.GetSlideInstance(inSlide));
        SValue theValue;
        if (m_DataCore.GetInstancePropertyValue(theSlideInstance,
                                                m_ObjectDefinitions.m_Named.m_NameProp, theValue)) {
            TDataStrPtr theName = get<TDataStrPtr>(theValue);
            if (theName)
                return theName->toQString();
        }
        return {};
    }

    Qt3DSDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance)
    {
        if (inInstance.Valid() == false)
            return 0;
        Qt3DSDMSlideHandle theAssociatedSlide = m_SlideSystem.GetAssociatedSlide(inInstance);
        if (theAssociatedSlide == m_ActiveSlideParent)
            return m_ActiveSlide;
        return theAssociatedSlide;
    }
    // Properties that should never be written out to a file.
    struct SInvalidProperytEqual
    {
        SComposerObjectDefinitions &m_ObjectDefinitions;
        bool m_IsImageOrMaterial;
        SInvalidProperytEqual(SComposerObjectDefinitions &inDefs, bool inIsImageOrMaterial)
            : m_ObjectDefinitions(inDefs)
            , m_IsImageOrMaterial(inIsImageOrMaterial)
        {
        }
        bool IsGeneralInvalidProperty(Qt3DSDMPropertyHandle inProperty) const
        {
            return inProperty == m_ObjectDefinitions.m_Guided.m_GuidProp
                    || inProperty == m_ObjectDefinitions.m_Asset.m_FileId;
        }
        bool IsImageOrMaterialInvalidProperty(Qt3DSDMPropertyHandle inProperty) const
        {
            return inProperty == m_ObjectDefinitions.m_Asset.m_StartTime
                    || inProperty == m_ObjectDefinitions.m_Asset.m_EndTime;
        }

        bool operator()(const TPropertyHandleValuePair &inProperty) const
        {
            return IsGeneralInvalidProperty(inProperty.first)
                    || (m_IsImageOrMaterial && IsImageOrMaterialInvalidProperty(inProperty.first));
        }
    };

    void SanitizeHandleValuePairList(Qt3DSDMInstanceHandle inInstance,
                                     TPropertyHandleValuePairList &outList)
    {
        bool isImageOrMaterial =
                m_DataCore.IsInstanceOrDerivedFrom(inInstance, m_ObjectDefinitions.m_Image.m_Instance)
                || m_DataCore.IsInstanceOrDerivedFrom(inInstance,
                                                      m_ObjectDefinitions.m_Material.m_Instance);
        erase_if(outList, SInvalidProperytEqual(m_ObjectDefinitions, isImageOrMaterial));
    }

    void GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inInstance,
                                           TPropertyHandleValuePairList &outList)
    {
        m_DataCore.GetSpecificInstancePropertyValues(inInstance, outList);
        SanitizeHandleValuePairList(inInstance, outList);
    }
    void GetSlidePropertyValues(Qt3DSDMSlideHandle inSlide, TPropertyHandleValuePairList &outList)
    {
        Qt3DSDMInstanceHandle theSlideInstance(m_SlideCore.GetSlideInstance(inSlide));
        if (theSlideInstance.Valid() == false)
            return;

        m_DataCore.GetSpecificInstancePropertyValues(theSlideInstance, outList);
        erase_if(outList, SPropertyMatches(m_ObjectDefinitions.m_Slide.m_ComponentId));
        erase_if(outList, SPropertyMatches(m_ObjectDefinitions.m_Named.m_NameProp));
        erase_if(outList, SPropertyMatches(m_ObjectDefinitions.m_Typed.m_TypeProp));
    }

    Option<pair<Qt3DSDMPropertyHandle, SValue>> ParseValue(Qt3DSDMInstanceHandle inInstance,
                                                           const QString &inPropName,
                                                           const QString &inValue)
    {
        Qt3DSDMPropertyHandle theProperty =
                m_DataCore.GetAggregateInstancePropertyByName(inInstance, inPropName);
        if (theProperty.Valid() == false) {
            return Empty();
        }
        DataModelDataType::Value theType = m_DataCore.GetProperty(theProperty).m_Type;
        return make_pair(theProperty, ParseValue(theType, inValue));
    }

    void ParseInstanceProperties(IDOMReader &inReader, Qt3DSDMInstanceHandle inInstance,
                                 vector<pair<QString, QString>> &outExtraAttributes,
                                 TPropertyHandleValuePairList &outProperties)
    {
        bool hasNoLifetime =
                m_DataCore.IsInstanceOrDerivedFrom(inInstance,
                                                   m_ObjectDefinitions.m_Image.m_Instance)
                || m_DataCore.IsInstanceOrDerivedFrom(inInstance,
                                                      m_ObjectDefinitions.m_Material.m_Instance);

        for (std::pair<QString, QString> theAtt = inReader.GetFirstAttribute();
             !IsTrivial(theAtt.first); theAtt = inReader.GetNextAttribute()) {
            Option<pair<Qt3DSDMPropertyHandle, SValue>> theValue =
                    ParseValue(inInstance, theAtt.first, theAtt.second);
            bool ignoreProperty = theValue.hasValue() == false
                    || (hasNoLifetime
                        && (theValue->first == m_ObjectDefinitions.m_Asset.m_StartTime.m_Property
                            || theValue->first
                            == m_ObjectDefinitions.m_Asset.m_EndTime.m_Property));
            if (ignoreProperty)
                outExtraAttributes.push_back(std::make_pair(theAtt.first, theAtt.second));
            else
                outProperties.push_back(std::make_pair(theValue->first, theValue->second));
        }
    }

    void ParseAndSetInstanceProperties(IDOMReader &inReader, Qt3DSDMSlideHandle inSlide,
                                       Qt3DSDMInstanceHandle inInstance,
                                       vector<pair<QString, QString>> &outExtraAttributes,
                                       TPropertyHandleValuePairList &ioProperties)
    {
        outExtraAttributes.clear();
        ioProperties.clear();

        ParseInstanceProperties(inReader, inInstance, outExtraAttributes, ioProperties);

        // Fix for QT3DS-2581: if an image has mapping mode "Light Probe" without
        // explicitly set U tiling mode, force horizontal tiling to "Tiled" to
        // obey legacy behavior where light probe U tiling was hard-wired to tiled. This
        // is to preserve behavior on old presentations that relied on hardcoding.
        bool hasMappingAsProbe = false;
        bool hasTilingH = false;
        for (size_t idx = 0, end = ioProperties.size(); idx < end; ++idx) {
            if (ioProperties[idx].first == m_ObjectDefinitions.m_Image.m_TilingU)
                hasTilingH = true;
            if (ioProperties[idx].first == m_ObjectDefinitions.m_Image.m_TextureMapping
                && ioProperties[idx].second.toQVariant() == QVariant("Light Probe")) {
                hasMappingAsProbe = true;
            }
        }

        if (!hasTilingH && hasMappingAsProbe) {
            qCDebug(qt3ds::TRACE_INFO)
                    << "Setting light probe horizontal tiling default to 'Tiled'.";
            qt3dsdm::SValue theValue(qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(L"Tiled")));
            ioProperties.push_back(std::make_pair(
                                       m_ObjectDefinitions.m_Image.m_TilingU, theValue));
        }

        if (inSlide.Valid()) {
            for (size_t idx = 0, end = ioProperties.size(); idx < end; ++idx)
                m_SlideCore.ForceSetInstancePropertyValue(inSlide, inInstance,
                                                          ioProperties[idx].first,
                                                          ioProperties[idx].second.toOldSkool());
        } else {
            for (size_t idx = 0, end = ioProperties.size(); idx < end; ++idx)
                m_DataCore.SetInstancePropertyValue(inInstance, ioProperties[idx].first,
                                                    ioProperties[idx].second);
        }
    }

    pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> CreateSlide()
    {
        Qt3DSDMInstanceHandle slideInstance = m_DataCore.CreateInstance();
        m_DataCore.DeriveInstance(slideInstance, m_ObjectDefinitions.m_Slide.m_Instance);
        Qt3DSDMSlideHandle masterSlide = m_SlideCore.CreateSlide(slideInstance);
        return make_pair(masterSlide, slideInstance);
    }

    // Monster function that writes out all instance information.  There are a lot of rules
    // observed here, mainly so we don't write out more than we have to.
    // An instance is in a default scope (the scope of its parent) unless it has a "slide" property.
    // Its properties within that scope are written as attributes and any animations or actions
    // added as direct children are in the default scope.
    // A slide owner system may declare a master slide child in which case the master slide now
    // because the default scope.  This element is also necessary to unambiguously state the order
    // of all slide children.
    // The element may then declare slides as children where sub-slide-specific information goes.
    // Finally, the element's scene graph children will be declared.
    bool Serialize(IDOMWriter &inWriter, Qt3DSDMInstanceHandle inInstance, bool inWriteParentRefs)
    {
        if (m_DataCore.IsInstanceOrDerivedFrom(inInstance, m_ObjectDefinitions.m_Asset.m_Instance)
                == false
                && m_DataCore.IsInstanceOrDerivedFrom(inInstance,
                                                      m_ObjectDefinitions.m_PathAnchorPoint.m_Instance)
                == false)
            return false;

        Option<QString> theType = m_MetaData.GetTypeForInstance(inInstance);
        if (theType.hasValue() == false) {
            QT3DS_ASSERT(false);
            return false;
        }

        QString theMasterRef;

        TInstanceHandleList theParents;
        m_DataCore.GetInstanceParents(inInstance, theParents);

        // For non-canonical parents (meaning parents that are setup at runtime
        // during normal project operations
        // we need to save them out into the master classes section
        QT3DS_ASSERT(theParents.size() == 1);
        if (!theParents.empty()) {
            Option<QString> theCanonicalType =
                    m_MetaData.GetTypeForCanonicalInstance(theParents[0]);
            // Meaning this isn't a canonical instance
            if (theCanonicalType.hasValue() == false) {
                Qt3DSDMInstanceHandle theMaster(theParents[0]);
                m_MasterObjectsSet.insert(theParents[0]);
                theMasterRef = "#";
                theMasterRef.append(GetInstanceId(theMaster));
            }
        }

        IDOMWriter::Scope __instanceScope(inWriter, theType);
        inWriter.Att(L"id", GetInstanceId(inInstance));

        m_InstanceSet.insert(inInstance);

        Qt3DSDMSlideHandle theAssociatedSlide = GetAssociatedSlide(inInstance);

        if (theAssociatedSlide.Valid() && !m_SlideSet.contains(theAssociatedSlide))
            m_SlideSet.insert(theAssociatedSlide);

        bool isSlideOwner = m_DataCore.IsInstanceOrDerivedFrom(
                    inInstance, m_ObjectDefinitions.m_SlideOwner.m_Instance);
        if (isSlideOwner) {
            // Ensure we mark all of those slides.
            SLong4 theGuid = GetGuid(inInstance, m_ObjectDefinitions.m_Guided.m_GuidProp);
            Qt3DSDMSlideHandle theMasterSlide(m_SlideSystem.GetMasterSlideByComponentGuid(theGuid));
            if (theMasterSlide.Valid()) {
                TSlideHandleList theChildSlides;
                m_SlideCore.GetChildSlides(theMasterSlide, theChildSlides);
                if (!m_SlideSet.contains(theMasterSlide))
                    m_SlideSet.insert(theMasterSlide);
                for (size_t slideIdx = 0, slideEnd = theChildSlides.size(); slideIdx < slideEnd;
                     ++slideIdx) {
                    if (!m_SlideSet.contains(theChildSlides[slideIdx]))
                        m_SlideSet.insert(theChildSlides[slideIdx]);
                }
            }
        }
        /////////////////////////////////////////////////////////////////////////
        /// Write out object attributes
        ////////////////////////////////////////////////////////////////////////

        // Write out the properties for the active slide.
        TPropertyHandleValuePairList theValues;
        GetSpecificInstancePropertyValues(inInstance, theValues);
        SerializePropertyList(inWriter, theValues);

        if (theMasterRef.size())
            inWriter.Att(QStringLiteral("class"), theMasterRef);

        if (inWriteParentRefs) {
            Qt3DSDMInstanceHandle theParent(m_AssetGraph.GetParent(inInstance));
            if (theParent.Valid()) {
                QString theParentRef("#");
                theParentRef.append(GetInstanceId(theParent));
                m_ExternalReferences.insert(theParent);
                inWriter.Att(QStringLiteral("graphparent"), theParentRef);
            }

            Qt3DSDMInstanceHandle theSibling = m_AssetGraph.GetSibling(inInstance, true);
            if (theSibling.Valid()) {
                QString theSiblingRef("#");
                theSiblingRef.append(GetInstanceId(theSibling));
                m_ExternalReferences.insert(theSibling);
                inWriter.Att(QStringLiteral("graphsibling"), theSiblingRef);
            }
        }

        ///////////////////////////////////////////////////////////////
        // Write out the graph children of this instance.
        CGraphIterator theChildren;

        // We only duplicate things you can see, so if you are on a master slide
        // we only duplicate master children.
        TSlideHandleList theActiveSlideChildren;
        if (m_ActiveSlide.Valid())
            m_SlideCore.GetChildSlides(m_ActiveSlide, theActiveSlideChildren);

        m_AssetGraph.GetChildren(theChildren, inInstance.GetHandleValue());
        for (; theChildren.IsDone() == false; ++theChildren) {
            bool isNonmasterActiveSlideChild = false;
            if (theActiveSlideChildren.size()) {
                // If we a copying from a master slide, we don't want nonmaster
                // only children of this object.
                Qt3DSDMInstanceHandle theChildHandle = theChildren.GetCurrent();
                Qt3DSDMSlideHandle theChildAssociatedSlide = GetAssociatedSlide(theChildHandle);
                isNonmasterActiveSlideChild =
                        std::find(theActiveSlideChildren.begin(), theActiveSlideChildren.end(),
                                  theChildAssociatedSlide)
                        != theActiveSlideChildren.end();
            }
            if (!isNonmasterActiveSlideChild)
                Serialize(inWriter, theChildren.GetCurrent(), false);
        }

        return true;
    }

    // Pass 1 creates all the instances and registeres
    qt3dsdm::TInstanceHandleList
    CreateAndRegisterInstances(IDOMReader &inReader, bool inReturnInstances,
                               Qt3DSDMInstanceHandle inParent = Qt3DSDMInstanceHandle())
    {
        IDOMReader::Scope __childScope(inReader);
        TInstanceHandleList theNewInstances;
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {

            qt3dsdm::Qt3DSDMInstanceHandle theNewInstance;
            QString theParentRef;
            if (inReader.Att(L"graphparent", theParentRef))
                inParent = GetInstanceById(idFromRef(theParentRef));
            QString theSiblingRef;
            bool theSilbingExists = false;
            if (inReader.Att(L"graphsibling", theSiblingRef))
                theSilbingExists = true;

            QString theId;
            inReader.Att(L"id", theId);
            QString theMasterRef;
            if (inReader.Att(L"class", theMasterRef)
                    && GetInstanceById(idFromRef(theMasterRef)).Valid()) {
                theNewInstance = IDocumentEditor::CreateSceneGraphInstance(
                            GetInstanceById(idFromRef(theMasterRef)), inParent, 0, m_DataCore,
                            m_SlideSystem, m_ObjectDefinitions, m_AssetGraph, m_MetaData);
            } else {
                QString theType(inReader.GetElementName());
                theNewInstance = IDocumentEditor::CreateSceneGraphInstance(
                            theType, inParent, 0, m_DataCore, m_SlideSystem, m_ObjectDefinitions,
                            m_AssetGraph, m_MetaData);
            }

            if (theSilbingExists)
                m_NewInstancesToSiblings.insert(theNewInstance, idFromRef(theSiblingRef));

            SLong4 theGuid = GetGuid(theNewInstance, m_ObjectDefinitions.m_Guided.m_GuidProp);
            if (m_PreserveFileIds)
                m_DataCore.SetInstancePropertyValue(theNewInstance,
                                                    m_ObjectDefinitions.m_Asset.m_FileId,
                                                    std::make_shared<CDataStr>(theId));
            SetId(theId, theNewInstance);
            AddGuid(theGuid, theNewInstance);

            CreateAndRegisterInstances(inReader, false, theNewInstance);
            if (inReturnInstances)
                theNewInstances.push_back(theNewInstance);
        }
        return theNewInstances;
    }

    void _MoveNewInstanceToItsPlaceRecursive(Qt3DSDMInstanceHandle inNewInstance)
    {
        TInstanceToSiblingMap::iterator itr = m_NewInstancesToSiblings.find(inNewInstance);
        if (itr != m_NewInstancesToSiblings.end()) {
            Qt3DSDMInstanceHandle theSibling = GetInstanceById(*itr);
            if (theSibling.Valid()) {
                _MoveNewInstanceToItsPlaceRecursive(theSibling);

                m_AssetGraph.MoveBefore(inNewInstance, theSibling);
            }

            m_NewInstancesToSiblings.erase(itr);
        }
    }

    void ReadInstanceProperties(IDOMReader &inReader)
    {
        IDOMReader::Scope __childScope(inReader);
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            QString theId;
            inReader.Att(QStringLiteral("id"), theId);

            Qt3DSDMInstanceHandle theNewInstance = GetInstanceById(theId);
            if (theNewInstance.Valid() == false) {
                QT3DS_ASSERT(false);
                continue;
            }

            TPropertyHandleValuePairList theValues;
            vector<pair<QString, QString>> theExtraAtts;
            ParseAndSetInstanceProperties(inReader, 0, theNewInstance, theExtraAtts, theValues);
            ReadInstanceProperties(inReader);

            // Make the instance in the right place if it's a new instance
            _MoveNewInstanceToItsPlaceRecursive(theNewInstance);
        }
    }

    void WriteSlideMasterOverrides(IDOMWriter &inWriter, Qt3DSDMInstanceHandle inInstance,
                                   Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent)
    {
        TPropertyHandleValuePairList theValues;
        TAnimationHandleList theAnimations;
        TActionHandleList theMasterActions;
        TActionHandleList theActions;

        // Sometimes this gets called for action instances.  In that case we should just ignore the
        // call.
        if (m_DataCore.IsInstanceOrDerivedFrom(inInstance, m_ObjectDefinitions.m_Asset.m_Instance)
                == false)
            return;

        if (inParent.Valid() == false) {
            QT3DS_ASSERT(false);
            return;
        }

        m_ActionCore.GetActions(inParent, inInstance, theMasterActions);
        GetInstanceSlideInformation(inInstance, inSlide, 0, theValues, theAnimations, theActions);

        vector<pair<size_t, bool>> theEyeballChanges;
        for (size_t idx = 0, end = theMasterActions.size(); idx < end; ++idx) {
            bool masterEyeball =
                    m_ActionSystem.GetActionEyeballValue(inParent, theMasterActions[idx]);
            bool childEyeball =
                    m_ActionSystem.GetActionEyeballValue(inSlide, theMasterActions[idx]);
            if (masterEyeball != childEyeball)
                theEyeballChanges.push_back(make_pair(idx, childEyeball));
        }
        if (theValues.size() || theAnimations.size() || theActions.size()
                || theEyeballChanges.size()) {
            // When we copy slides we can get into here.  During normal serialization this can't
            // happen.
            if (inParent == m_ActiveSlideParent)
                m_ExternalReferences.insert(inInstance);

            IDOMWriter::Scope __instanceScope(inWriter, L"Set");
            const QString theRef(QLatin1Char('#') + GetInstanceId(inInstance));
            inWriter.Att(QStringLiteral("ref"), theRef);
            SerializePropertyList(inWriter, theValues);
            SerializeAnimations(inWriter, theAnimations);
            SerializeActions(inWriter, inSlide, inInstance, theActions);
            for (size_t idx = 0, end = theEyeballChanges.size(); idx < end; ++idx) {
                Qt3DSDMActionHandle theAction(theMasterActions[theEyeballChanges[idx].first]);
                IDOMWriter::Scope __actionScope(inWriter, L"Action");
                bool hadAction = m_ActionToIdMap.find(theAction) != m_ActionToIdMap.end();
                const QString theRef(QLatin1Char('#') + GetActionId(theAction, inParent,
                                                                    inInstance));
                inWriter.Att(QStringLiteral("ref"), theRef);
                inWriter.Att(QStringLiteral("eyeball"), theEyeballChanges[idx].second);
                if (!hadAction)
                    m_ExternalActions.insert(theAction);
            }
        }
    }

    void WriteSlideInstance(IDOMWriter &inWriter, Qt3DSDMInstanceHandle inInstance,
                            Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent)
    {
        TPropertyHandleValuePairList theValues;
        TAnimationHandleList theAnimations;
        TActionHandleList theActions;

        // Sometimes this gets called for action instances.  In that case we should just ignore the
        // call.
        if (m_DataCore.IsInstanceOrDerivedFrom(inInstance, m_ObjectDefinitions.m_Asset.m_Instance)
                == false)
            return;

        GetInstanceSlideInformation(inInstance, inSlide, inParent, theValues, theAnimations,
                                    theActions);
        // When copying slides, we don't automatically add all the instances like we do when we
        // cut/paste
        QString commandName = QStringLiteral("Set");
        if (inSlide == m_ActiveSlide || inSlide == m_SlideSystem.GetAssociatedSlide(inInstance))
            commandName = QStringLiteral("Add");

        if (commandName == QStringLiteral("Add") || theValues.size() || theAnimations.size()
                || theActions.size()) {
            IDOMWriter::Scope __instanceScope(inWriter, commandName);
            const QString theRef(QLatin1Char('#') + GetInstanceId(inInstance));
            inWriter.Att(QStringLiteral("ref"), theRef);
            SerializePropertyList(inWriter, theValues);
            SerializeAnimations(inWriter, theAnimations);
            SerializeActions(inWriter, inSlide, inInstance, theActions);
        }
    }

    struct SInstanceNotInSet
    {
        const TInstanceSet &m_Set;
        SInstanceNotInSet(const TInstanceSet &s)
            : m_Set(s)
        {
        }

        bool operator()(Qt3DSDMInstanceHandle inInstance)
        {
            return m_Set.find(inInstance) == m_Set.end();
        }
    };

    // Get the set of instances associated with a slide but filtered by our
    // m_InstanceSet, meaning ignore instances that aren't scene graph instances
    // and that haven't been written out.
    void GetFilteredAssociatedInstances(Qt3DSDMSlideHandle inSlide,
                                        TInstanceHandleList &outInstances)
    {
        m_SlideSystem.GetAssociatedInstances(inSlide, outInstances);
        erase_if(outInstances, SInstanceNotInSet(m_InstanceSet));
    }

    Qt3DSDMInstanceHandle GetSlideComponent(Qt3DSDMSlideHandle inSlide)
    {
        Qt3DSDMSlideHandle theMasterSlide = m_SlideSystem.GetMasterSlide(inSlide);
        Qt3DSDMInstanceHandle theMasterInstance(m_SlideCore.GetSlideInstance(theMasterSlide));
        SLong4 theComponentGuid =
                GetGuid(theMasterInstance, m_ObjectDefinitions.m_Slide.m_ComponentId);
        Qt3DSDMInstanceHandle theComponent = FindInstanceByGUID(theComponentGuid);
        return theComponent;
    }

    struct SInstanceOrderSorter
    {
        const TInstanceIntMap &m_DepthMap;
        SInstanceOrderSorter(const TInstanceIntMap &inMap)
            : m_DepthMap(inMap)
        {
        }
        int GetInstanceOrder(Qt3DSDMInstanceHandle inHdl) const
        {
            TInstanceIntMap::const_iterator theIter = m_DepthMap.find(inHdl);
            if (theIter != m_DepthMap.end())
                return *theIter;
            return QT3DS_MAX_I32;
        }
        bool operator()(Qt3DSDMInstanceHandle lhs, Qt3DSDMInstanceHandle rhs) const
        {
            return GetInstanceOrder(lhs) < GetInstanceOrder(rhs);
        }
        bool operator()(const std::pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> &lhs,
                        const std::pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> &rhs) const
        {
            return GetInstanceOrder(lhs.second) < GetInstanceOrder(rhs.second);
        }
    };

    void SortSlideInstanceList(TInstanceHandleList &inInstances) const
    {
        std::stable_sort(inInstances.begin(), inInstances.end(),
                         SInstanceOrderSorter(m_InstanceToGraphDepthMap));
    }

    void SerializeSlides(IDOMWriter &inWriter, bool inSlideCopy = false)
    {
        if (m_ActiveSlide.Valid()) {
            m_SlideSet.remove(m_ActiveSlide);
            m_SlideSet.remove(m_ActiveSlideParent);

            IDOMWriter::Scope __writeScope(inWriter, L"ActiveState");

            TInstanceHandleList theInstances;
            if (inSlideCopy == false) {
                GetFilteredAssociatedInstances(m_ActiveSlide, theInstances);
                SortSlideInstanceList(theInstances);
                for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
                    Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
                    WriteSlideInstance(inWriter, theInstance, m_ActiveSlide, m_ActiveSlideParent);
                }
            } else {
                inWriter.Att(L"name", GetSlideName(m_ActiveSlide));
                TPropertyHandleValuePairList theValues;
                GetSlidePropertyValues(m_ActiveSlide, theValues);
                SerializePropertyList(inWriter, theValues);

                Qt3DSDMInstanceHandle theComponent = GetSlideComponent(m_ActiveSlide);

                TInstanceHandleList theParentInstances;
                m_SlideSystem.GetAssociatedInstances(m_ActiveSlide, theInstances);
                theInstances.push_back(theComponent);
                theParentInstances.push_back(theComponent);
                if (m_ActiveSlideParent.Valid())
                    m_SlideSystem.GetAssociatedInstances(m_ActiveSlideParent, theParentInstances);

                SortSlideInstanceList(theInstances);
                for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
                    Qt3DSDMInstanceHandle theInstance(theInstances[idx]);

                    // If this is an instance associated with master slide
                    if (find(theParentInstances.begin(), theParentInstances.end(), theInstance)
                            != theParentInstances.end())
                        WriteSlideMasterOverrides(inWriter, theInstance, m_ActiveSlide,
                                                  m_ActiveSlideParent);
                    else
                        WriteSlideInstance(inWriter, theInstance, m_ActiveSlide, 0);
                }
            }
        } // m_ActiveSlide.valid;
        std::vector<std::pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>> theSerializationSlides;
        theSerializationSlides.reserve(m_SlideSet.size());
        TSlideSet theMasterSlideSet;
        for (TSlideSet::iterator theIter = m_SlideSet.begin(), end = m_SlideSet.end();
             theIter != end; ++theIter) {
            Qt3DSDMSlideHandle theSlide = *theIter;
            Qt3DSDMSlideHandle theMasterSlide = m_SlideCore.GetParentSlide(theSlide);
            if (theMasterSlide.Valid() && *theMasterSlideSet.insert(theMasterSlide)) {
                Qt3DSDMInstanceHandle theSlideOwner(GetSlideComponent(theMasterSlide));
                if (theSlideOwner.Valid())
                    theSerializationSlides.push_back(std::make_pair(theMasterSlide, theSlideOwner));
            }
        }
        // Sort the slides by the depth index of their component.
        std::stable_sort(theSerializationSlides.begin(), theSerializationSlides.end(),
                         SInstanceOrderSorter(m_InstanceToGraphDepthMap));
        for (size_t slideSetIdx = 0, slideSetEnd = theSerializationSlides.size();
             slideSetIdx < slideSetEnd; ++slideSetIdx) {
            Qt3DSDMSlideHandle theMasterSlide = theSerializationSlides[slideSetIdx].first;
            Qt3DSDMInstanceHandle theComponent = theSerializationSlides[slideSetIdx].second;
            IDOMWriter::Scope __masterScope(inWriter, L"State");

            inWriter.Att(QStringLiteral("name"), GetSlideName(theMasterSlide));
            const QString theComponentRef(QLatin1Char('#') + GetInstanceId(theComponent));
            inWriter.Att(QStringLiteral("component"), theComponentRef);

            TPropertyHandleValuePairList theValues;
            GetSlidePropertyValues(theMasterSlide, theValues);
            SerializePropertyList(inWriter, theValues);

            TInstanceHandleList theInstances;
            GetFilteredAssociatedInstances(theMasterSlide, theInstances);

            theInstances.push_back(theComponent);
            SortSlideInstanceList(theInstances);
            for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
                Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
                if (m_InstanceSet.find(theInstance) == m_InstanceSet.end())
                    continue;
                WriteSlideInstance(inWriter, theInstance, theMasterSlide, 0);
            }
            TSlideHandleList theChildren;
            m_SlideCore.GetChildSlides(theMasterSlide, theChildren);
            // Generate ids for all of the slides before we attempt to write them out
            // else a reference to a further slide may generate an external slide reference
            // which wouldn't be our intention.
            for (size_t idx = 0, end = theChildren.size(); idx < end; ++idx) {
                Qt3DSDMSlideHandle theChildSlide(theChildren[idx]);
                GetSlideId(theChildSlide, theComponent);
            }
            for (size_t idx = 0, end = theChildren.size(); idx < end; ++idx) {
                Qt3DSDMSlideHandle theChildSlide(theChildren[idx]);
                m_SlideSet.remove(theChildSlide);
                IDOMWriter::Scope __childSlideScope(inWriter, L"State");
                inWriter.Att(L"id", GetSlideId(theChildSlide, theComponent));
                inWriter.Att(L"name", GetSlideName(theChildSlide));
                theValues.clear();
                GetSlidePropertyValues(theChildSlide, theValues);
                SerializePropertyList(inWriter, theValues);
                for (size_t masterIdx = 0, masterEnd = theInstances.size(); masterIdx < masterEnd;
                     ++masterIdx)
                    WriteSlideMasterOverrides(inWriter, theInstances[masterIdx], theChildSlide,
                                              theMasterSlide);

                TInstanceHandleList theChildInstances;
                GetFilteredAssociatedInstances(theChildSlide, theChildInstances);
                SortSlideInstanceList(theChildInstances);
                for (size_t instIdx = 0, instEnd = theChildInstances.size(); instIdx < instEnd;
                     ++instIdx) {
                    // Don't write out the master instances again, we did that with a special
                    // function just
                    // for them above.
                    if (find(theInstances.begin(), theInstances.end(), theChildInstances[instIdx])
                            != theInstances.end())
                        continue;

                    WriteSlideInstance(inWriter, theChildInstances[instIdx], theChildSlide, 0);
                }
            }
        }
    }

    // Structure to record the information needed to parse an action so we
    // can delay action parsing to allow relative path resolution which
    // allows correct event name resolution
    // Probably a better fix would be to delay the event name resolution
    // until the event handle is actually needed but that is too risky to
    // attempt right now.
    struct SActionParseRecord
    {
        void *m_ReaderScope;
        Qt3DSDMSlideHandle m_Slide;
        Qt3DSDMInstanceHandle m_Instance;
        SActionParseRecord()
            : m_ReaderScope(NULL)
        {
        }
        SActionParseRecord(void *inReaderScope, Qt3DSDMSlideHandle inSlide,
                           Qt3DSDMInstanceHandle inInstance)
            : m_ReaderScope(inReaderScope)
            , m_Slide(inSlide)
            , m_Instance(inInstance)
        {
        }
    };

    void ParseSlide(IDOMReader &inReader, Qt3DSDMSlideHandle inSlide, SLong4 inComponentId,
                    vector<SActionParseRecord> &inUnparsedActions)
    {
        IDOMReader::Scope __stateScope(inReader);
        TPropertyHandleValuePairList theValues;
        vector<pair<QString, QString>> theExtraAtts;
        ParseAndSetInstanceProperties(inReader, 0, m_SlideCore.GetSlideInstance(inSlide),
                                      theExtraAtts, theValues);
        // Slides require a two-pass parsing system because slides can refer to each other via id.
        {
            IDOMReader::Scope __stateChildrenScope(inReader);
            // Preparse the slide first to create all child slides in the graph.
            for (bool success = inReader.MoveToFirstChild(L"State"); success;
                 success = inReader.MoveToNextSibling(L"State")) {
                // On the first pass, just create ids for the slides.  This is because we can have
                // references to other slides
                // via id *in* the slide data.
                {
                    QString theId;
                    inReader.Att(QStringLiteral("Id"), theId);
                    pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> theChildSlideInstPair =
                            CreateSlide();
                    m_SlideCore.DeriveSlide(theChildSlideInstPair.first, inSlide);
                    m_DataCore.SetInstancePropertyValue(theChildSlideInstPair.second,
                                                        m_ObjectDefinitions.m_Slide.m_ComponentId,
                                                        inComponentId);
                    if (!IsTrivial(theId))
                        AddSlideId(theId, theChildSlideInstPair.first);
                }
            }
        }
        // Some older files don't have slide ids so we need to be able to parse those files
        // correctly.  If the file doesn't have a slide id then we effectively use the
        // slide index.
        TSlideHandleList theChildren;
        m_SlideCore.GetChildSlides(inSlide, theChildren);
        TSlideHandleList::iterator theCurrentChild(theChildren.begin());
        TSlideHandleList::iterator theLastChild(theChildren.end());

        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            if (inReader.GetElementName() == QLatin1String("Set")
                    || inReader.GetElementName() == QLatin1String("Add")) {
                QString theRef;
                inReader.Att(QStringLiteral("ref"), theRef);
                Qt3DSDMInstanceHandle theInstance = GetInstanceById(idFromRef(theRef));
                if (theInstance.Valid() == false) {
                    QT3DS_ASSERT(false);
                    continue;
                }
                // Sometimes we will read the component's properties *inside* the component.
                // this happens if you attach an action inside of a component.
                // In this case we don't want to also associate the component with the slide where
                // the
                // action exists.
                bool IsAlreadyAssociated = m_SlideSystem.GetAssociatedSlide(theInstance).Valid();
                bool IsGuidDifferent = !(
                            GetGuid(theInstance, m_ObjectDefinitions.m_Guided.m_GuidProp) == inComponentId);
                if (!IsAlreadyAssociated && IsGuidDifferent) {
                    m_SlideSystem.AssociateInstanceWithSlide(inSlide, theInstance);
                    IDocumentEditor::UnlinkAlwaysUnlinkedProperties(
                                theInstance, m_ObjectDefinitions, m_SlideSystem);
                }

                ParseAndSetInstanceProperties(inReader, inSlide, theInstance, theExtraAtts,
                                              theValues);
                // Reset the reader pointer after delving into the children
                IDOMReader::Scope __instanceScope(inReader);
                for (bool childSuccess = inReader.MoveToFirstChild(); childSuccess;
                     childSuccess = inReader.MoveToNextSibling()) {
                    if (inReader.GetElementName() == QLatin1String("AnimationTrack"))
                        ParseAnimation(inReader, inSlide, theInstance);
                    else if (inReader.GetElementName() == QLatin1String("Action"))
                        inUnparsedActions.push_back(
                                    SActionParseRecord(inReader.GetScope(), inSlide, theInstance));
                }
            } else if (inReader.GetElementName() == QLatin1String("State")) {
                QString theId;
                inReader.Att(L"Id", theId);
                Qt3DSDMSlideHandle theSlide;
                if (!IsTrivial(theId))
                    theSlide = GetSlideById(theId);
                else
                    theSlide = *theCurrentChild;

                QT3DS_ASSERT(theSlide.Valid());
                QT3DS_ASSERT(theSlide = *theCurrentChild);
                ++theCurrentChild;

                if (theSlide.Valid())
                    ParseSlide(inReader, theSlide, inComponentId, inUnparsedActions);
            } else {
                QT3DS_ASSERT(false);
            }
        }
    }

    void ReadSlides(IDOMReader &inReader)
    {
        vector<SActionParseRecord> theUnparsedActions;
        IDOMReader::Scope __logicScope(inReader);
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            if (inReader.GetElementName() == QLatin1String("ActiveState")) {
                if (m_ActiveSlide.Valid() == false) {
                    QT3DS_ASSERT(false);
                    continue;
                }

                Q3DStudio::CId theSceneId(SCENE_GUID);
                TGUIDPacked thePackedGuid(theSceneId);
                SLong4 theSceneGuid(thePackedGuid.Data1, thePackedGuid.Data2, thePackedGuid.Data3,
                                    thePackedGuid.Data4);
                ParseSlide(inReader, m_ActiveSlide, theSceneGuid, theUnparsedActions);
            } else if (inReader.GetElementName() == QLatin1String("State")) {
                QString componentRef;
                inReader.Att(L"component", componentRef);
                Qt3DSDMInstanceHandle component = GetInstanceById(idFromRef(componentRef));
                if (component.Valid() == false) {
                    QT3DS_ASSERT(false);
                    continue;
                }

                // Ensure that the component doesn't already have a slide graph associated with it.
                TSlideGraphHandleList theGraphs;
                m_SlideGraphCore.GetSlideGraphs(theGraphs);
                SLong4 theComponentGuid =
                        GetGuid(component, m_ObjectDefinitions.m_Guided.m_GuidProp);
                bool foundDuplicateGraph = false;
                for (size_t idx = 0, end = theGraphs.size();
                     idx < end && foundDuplicateGraph == false; ++idx) {
                    Qt3DSDMSlideHandle theMaster = m_SlideGraphCore.GetGraphRoot(theGraphs[idx]);
                    Qt3DSDMInstanceHandle theSlideInstance = m_SlideCore.GetSlideInstance(theMaster);
                    SValue theValue;
                    if (m_DataCore.GetInstancePropertyValue(
                                theSlideInstance, m_ObjectDefinitions.m_Slide.m_ComponentId,
                                theValue)) {
                        SLong4 theLocalGuid = qt3dsdm::get<SLong4>(theValue);
                        if (theLocalGuid == theComponentGuid) {
                            const char8_t *theErrorRef;
                            inReader.Att("component", theErrorRef);
                            qCCritical(qt3ds::INTERNAL_ERROR) << "Duplicate master slide found: "
                                                              << theErrorRef;
                            foundDuplicateGraph = true;
                        }
                    }
                }
                if (foundDuplicateGraph == false) {
                    pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> theSlideInstPair = CreateSlide();
                    m_SlideGraphCore.CreateSlideGraph(theSlideInstPair.first);
                    m_DataCore.SetInstancePropertyValue(theSlideInstPair.second,
                                                        m_ObjectDefinitions.m_Slide.m_ComponentId,
                                                        theComponentGuid);
                    ParseSlide(inReader, theSlideInstPair.first, theComponentGuid,
                               theUnparsedActions);
                    m_SlideSystem.SetActiveSlide(theSlideInstPair.first, 1);
                }
            }
        }

        for (size_t actionIdx = 0, actionEnd = theUnparsedActions.size(); actionIdx < actionEnd;
             ++actionIdx) {
            SActionParseRecord &theRecord(theUnparsedActions[actionIdx]);
            inReader.SetScope(theRecord.m_ReaderScope);

            ParseAction(inReader, theRecord.m_Slide, theRecord.m_Instance);
        }
    }

    void WriteExternalReferences(IDOMWriter &inWriter)
    {
        // First filter the external references via the instance set
        for (TInstanceSet::iterator theIter = m_InstanceSet.begin(), end = m_InstanceSet.end();
             theIter != end; ++theIter) {
            m_ExternalReferences.remove(*theIter);
        }

        if (m_ExternalReferences.empty() && m_ExternalActions.empty() && m_ExternalSlides.empty())
            return;

        IDOMWriter::Scope __refScope(inWriter, L"ExternalReferences");
        for (TInstanceSet::iterator theIter = m_ExternalReferences.begin(),
             theEnd = m_ExternalReferences.end();
             theIter != theEnd; ++theIter) {
            IDOMWriter::Scope __refScope(inWriter, L"Reference");
            const QString theId(QLatin1Char('#') + GetInstanceId(*theIter));
            inWriter.Att(L"ref", theId);
            SLong4 theGuid = GetInstanceGuid(*theIter);
            inWriter.Att(L"guid", theGuid.toQString());
        }
        for (TActionSet::iterator theIter = m_ExternalActions.begin(),
             theEnd = m_ExternalActions.end();
             theIter != theEnd; ++theIter) {
            Qt3DSDMActionHandle theAction(*theIter);
            IDOMWriter::Scope __refScope(inWriter, L"ActionReference");
            const QString theId(QLatin1Char('#') + GetActionId(theAction, 0, 0));
            inWriter.Att(L"ref", theId);
            inWriter.Att(L"handle", (int)theAction);
        }
        for (TSlideSet::iterator theIter = m_ExternalSlides.begin(),
             theEnd = m_ExternalSlides.end();
             theIter != theEnd; ++theIter) {
            Qt3DSDMSlideHandle theSlide(*theIter);
            IDOMWriter::Scope __refScope(inWriter, L"SlideReference");
            const QString theId(QLatin1Char('#') + GetSlideId(theSlide, 0));
            inWriter.Att(L"ref", theId);
            inWriter.Att(L"handle", (int)theSlide);
        }
    }

    struct SMasterObjectSorter
    {
        IDataCore &m_DataCore;
        Qt3DSDMPropertyHandle m_SourcePathProperty;
        SMasterObjectSorter(IDataCore &inDataCore, Qt3DSDMPropertyHandle inSourcePathProperty)
            : m_DataCore(inDataCore)
            , m_SourcePathProperty(inSourcePathProperty)
        {
        }
        bool operator()(Qt3DSDMInstanceHandle lhs, Qt3DSDMInstanceHandle rhs) const
        {
            SValue lhsValue, rhsValue;
            m_DataCore.GetInstancePropertyValue(lhs, m_SourcePathProperty, lhsValue);
            m_DataCore.GetInstancePropertyValue(rhs, m_SourcePathProperty, rhsValue);
            QString lhsStr = qt3dsdm::get<TDataStrPtr>(lhsValue)->toQString();
            QString rhsStr = qt3dsdm::get<TDataStrPtr>(rhsValue)->toQString();
            return QString::compare(lhsStr, rhsStr) < 0;
        }
    };

    void AddGraphInstanceToOrderMap(Qt3DSDMInstanceHandle inInstance)
    {
        m_InstanceToGraphDepthMap.insert(inInstance, (int)m_InstanceToGraphDepthMap.size());
    }

    void BuildGraphOrderMap(const Qt3DSDMInstanceHandle *inTopInstances, QT3DSU32 inCount)
    {
        /* Code for breadth first ordering
        TInstanceHandleList theNextInstancesList;
        TInstanceHandleList theCurrentInstancesList;
        m_AssetGraph.GetDepthFirst(
        for ( QT3DSU32 idx =0; idx < inCount; ++idx )
                theNextInstancesList.push_back( inTopInstances[idx] );

        do
        {
                std::swap( theCurrentInstancesList, theNextInstancesList );
                theNextInstancesList.clear();
                for ( size_t idx = 0, end = theCurrentInstancesList.size(); idx < end; ++idx )
                {
                        Qt3DSDMInstanceHandle theCurrentInstance( theCurrentInstancesList[idx] );
                        AddGraphInstanceToOrderMap( theCurrentInstance );
                        for ( int childIdx = 0, childEnd = m_AssetGraph.GetChildCount(
        theCurrentInstance ); childIdx < childEnd; ++childIdx )
                                theNextInstancesList.push_back( m_AssetGraph.GetChild(
        theCurrentInstance, childIdx ) );
                }
        }while( theNextInstancesList.size() );
        */
        CGraphIterator theIter;
        m_AssetGraph.GetDepthFirst(theIter, m_AssetGraph.GetRoot(0));
        for (; theIter.IsDone() == false; ++theIter) {
            AddGraphInstanceToOrderMap(theIter.GetCurrent());
        }
    }

    void DoSerializeScene(IDOMWriter &inWriter, const Qt3DSDMInstanceHandle *inTopInstances,
                          QT3DSU32 inCount, bool inWriteParentRefs = false)
    {
        GetAllInstanceGuids();
        {
            IDOMWriter::Scope graphScope(inWriter, L"Graph");
            for (QT3DSU32 idx = 0; idx < inCount; ++idx)
                Serialize(inWriter, inTopInstances[idx], inWriteParentRefs);
        }
        if (m_MasterObjectsSet.size()) {
            {
                IDOMWriter::Scope masterScope(inWriter, L"Classes");
                TInstanceList theMasterObjectsList;
                theMasterObjectsList.insert(theMasterObjectsList.end(), m_MasterObjectsSet.begin(),
                                            m_MasterObjectsSet.end());
                std::stable_sort(
                            theMasterObjectsList.begin(), theMasterObjectsList.end(),
                            SMasterObjectSorter(m_DataCore, m_ObjectDefinitions.m_Asset.m_SourcePath));
                for (TInstanceList::iterator theIter = theMasterObjectsList.begin(),
                     end = theMasterObjectsList.end();
                     theIter != end; ++theIter) {
                    Qt3DSDMInstanceHandle theMaster(*theIter);
                    Option<QString> theType = m_MetaData.GetTypeForInstance(theMaster);
                    if (theType.hasValue() == false) {
                        QT3DS_ASSERT(false);
                        continue;
                    }
                    IDOMWriter::Scope instScope(inWriter, theType);
                    inWriter.Att(L"id", GetInstanceId(theMaster));
                    // Write out all the properties that are on the instance but are not on *this*
                    // instance in the meta data.
                    TPropertyHandleValuePairList theProperties;
                    GetSpecificInstancePropertyValues(theMaster, theProperties);
                    erase_if(theProperties,
                             SMetaDataPropertyEraser(theMaster, m_ObjectDefinitions));
                    SerializePropertyList(inWriter, theProperties);
                }
            }
            inWriter.MoveBefore(L"Classes", L"Graph");
        }
        {
            BuildGraphOrderMap(inTopInstances, inCount);
            IDOMWriter::Scope stateScope(inWriter, L"Logic");
            SerializeSlides(inWriter, inWriteParentRefs);
        }

        WriteExternalReferences(inWriter);
    }

    void ReadExternalReferences(IDOMReader &inReader)
    {
        IDOMReader::Scope __externalReferencesScope(inReader);
        if (inReader.MoveToFirstChild(L"ExternalReferences")) {
            GetAllInstanceGuids();
            QString theRef;
            QString theGuidBuf;
            for (bool success = inReader.MoveToFirstChild(); success;
                 success = inReader.MoveToNextSibling()) {
                if (inReader.GetElementName() == QLatin1String("Reference")) {
                    inReader.Att(L"ref", theRef);
                    inReader.Att(L"guid", theGuidBuf);
                    SLong4 theGuid;
                    QStringList split(theGuidBuf.split(QLatin1Char(' ')));
                    theGuid.m_Longs[0] = split[0].toLong();
                    theGuid.m_Longs[1] = split[1].toLong();
                    theGuid.m_Longs[2] = split[2].toLong();
                    theGuid.m_Longs[3] = split[3].toLong();
                    Qt3DSDMInstanceHandle theInstance = FindInstanceByGUID(theGuid);
                    if (theInstance.Valid())
                        AddId(idFromRef(theRef), theInstance);
                } else if (inReader.GetElementName() == QLatin1String("ActionReference")) {
                    inReader.Att(L"ref", theRef);
                    inReader.Att(L"handle", theGuidBuf);
                    long theHandleValue(theGuidBuf.toLong());
                    Qt3DSDMActionHandle theAction(theHandleValue);
                    QT3DS_ASSERT(m_ActionCore.HandleValid(theAction));
                    AddActionId(idFromRef(theRef), theAction);
                } else if (inReader.GetElementName() == QLatin1String("SlideReference")) {
                    inReader.Att(L"ref", theRef);
                    inReader.Att(L"handle", theGuidBuf);
                    long theHandleValue(theGuidBuf.toLong());
                    Qt3DSDMSlideHandle theSlide(theHandleValue);
                    QT3DS_ASSERT(m_SlideCore.IsSlide(theSlide));
                    AddSlideId(idFromRef(theRef), theSlide);
                }
            }
        }
    }

    Qt3DSDMInstanceHandle loadSourcePathFile(
            IDOMReader &inReader, const QString &inDocumentDirectory, const QString &theSourcePath)
    {
        Qt3DSDMInstanceHandle theMaster;
        QString theFullPath(CFilePath::CombineBaseAndRelative(inDocumentDirectory, theSourcePath));
        QString theType(inReader.GetElementName());
        Qt3DSDMInstanceHandle theCanonicalType(m_MetaData.GetCanonicalInstanceForType(theType));

        if (theCanonicalType.Valid() == false) {
            QT3DS_ASSERT(false);
            return theMaster;
        }

        theMaster = m_DataCore.CreateInstance();
        m_DataCore.DeriveInstance(theMaster, theCanonicalType);
        m_SourcePathToMasterInstances.insert(theSourcePath, theMaster);
        QFileInfo info(theFullPath);
        if (!info.exists()) {
            QT3DS_ASSERT(false);
            return theMaster;
        }
        const QString suffix(info.suffix());
        if (suffix.compare(QLatin1String("qml"), Qt::CaseInsensitive) == 0) {
            std::shared_ptr<IDOMReader> theScriptPtr
                    = IDocumentEditor::ParseScriptFile(theFullPath, m_DataCore.GetStringTablePtr(),
                                                       m_ImportFailedHandler,
                                                       *m_InputStreamFactory);
            if (theScriptPtr) {
                std::vector<SMetaDataLoadWarning> warnings;
                m_MetaData.LoadInstance(*theScriptPtr, theMaster, info.baseName(), warnings);
            }
        } else if (suffix.compare(QLatin1String("glsl"), Qt::CaseInsensitive) == 0
                   || suffix.compare(QLatin1String("effect"), Qt::CaseInsensitive) == 0) {
            std::vector<SMetaDataLoadWarning> warnings;
            Q3DStudio::IRefCountedInputStream theStream
                    = m_InputStreamFactory->getStreamForFile(theFullPath);
            if (theStream) {
                m_MetaData.LoadEffectInstance(theSourcePath, theMaster, info.baseName(),
                                              warnings, *theStream);
                IDocumentEditor::fixDefaultTexturePaths(theMaster);
            }
        } else if (suffix.compare(QLatin1String("plugin"), Qt::CaseInsensitive) == 0) {
            std::shared_ptr<IDOMReader> thePluginPtr
                    = IDocumentEditor::ParsePluginFile(theFullPath, m_DataCore.GetStringTablePtr(),
                                                       m_ImportFailedHandler,
                                                       *m_InputStreamFactory);
            if (thePluginPtr) {
                std::vector<SMetaDataLoadWarning> warnings;
                m_MetaData.LoadInstance(*thePluginPtr, theMaster, info.baseName(), warnings);
            }
        } else if (suffix.compare(QLatin1String("material"), Qt::CaseInsensitive) == 0
                   || suffix.compare(QLatin1String("shader"), Qt::CaseInsensitive) == 0) {
            std::vector<SMetaDataLoadWarning> warnings;
            IRefCountedInputStream theStream = m_InputStreamFactory->getStreamForFile(theFullPath);
            if (theStream) {
                m_MetaData.LoadMaterialInstance(theSourcePath, theMaster, info.baseName(),
                                                warnings, *theStream);
                IDocumentEditor::fixDefaultTexturePaths(theMaster);
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return theMaster;
    }

    qt3dsdm::TInstanceHandleList DoSerializeScene(IDOMReader &inReader,
                                                  const QString &inDocumentDirectory,
                                                  Qt3DSDMInstanceHandle inNewRoot)
    {
        QStringList unknownList;
        // Attempt to work correctly whether we are pointing to the project or not.
        IDOMReader::Scope __outerScope(inReader);
        if (inReader.GetElementName() == QLatin1String("UIP"))
            inReader.MoveToFirstChild(QStringLiteral("Project"));

        {
            ReadExternalReferences(inReader);
            IDOMReader::Scope __masterScope(inReader);
            if (inReader.MoveToFirstChild(QStringLiteral("Classes"))) {
                BuildSourcePathMasterObjectMap();
                for (bool success = inReader.MoveToFirstChild(); success;
                     success = inReader.MoveToNextSibling()) {
                    QString theSourcePath;
                    // Ignore master objects that already exist in the project.
                    if (inReader.Att(QStringLiteral("sourcepath"), theSourcePath) == false) {
                        QT3DS_ASSERT(false);
                        continue;
                    }
                    Qt3DSDMInstanceHandle theMaster;
                    TIdToHandleMap::iterator theFind(m_SourcePathToMasterInstances
                                                     .find(theSourcePath));
                    if (theFind != m_SourcePathToMasterInstances.end()) {
                        theMaster = *theFind;
                    } else {
                        TPropertyHandleValuePairList theValues;
                        vector<pair<QString, QString>> theExtraAtts;
                        theMaster = loadSourcePathFile(inReader, inDocumentDirectory,
                                                       theSourcePath);
                        ParseAndSetInstanceProperties(inReader, 0, theMaster, theExtraAtts,
                                                      theValues);
                    }
                    QString theId;
                    inReader.Att(QStringLiteral("id"), theId);
                    AddId(theId, theMaster);
                }
            }
            if (!unknownList.isEmpty()) {
                QString unknownAssets
                        = QObject::tr("The presentation may not behave as expected.\n"
                                      "The following assets were not recognized:\n");

                for (auto asset : qAsConst(unknownList)) {
                    unknownAssets.append(QLatin1String("\n"));
                    unknownAssets.append(asset);
                }
                QMessageBox::warning(nullptr, QObject::tr("Unknown Assets"), unknownAssets);
            }
        }

        TInstanceHandleList retval;
        {
            IDOMReader::Scope __graphScope(inReader);
            if (!inReader.MoveToFirstChild(L"Graph")) {
                QT3DS_ASSERT(false);
                return retval;
            }
            {
                // Parse the graph in two passes.  First pass just creates the base instances
                // and sets up their GUID's
                // Second pass does the actual parsing of their properties.  This allows us to
                // reference
                // things through the graph in an easier fasion.
                // pass 1
                retval = CreateAndRegisterInstances(inReader, true, inNewRoot);
                // pass 2
                ReadInstanceProperties(inReader);
            }
        }
        {
            IDOMReader::Scope __logicScope(inReader);
            if (!inReader.MoveToFirstChild(L"Logic")) {
                QT3DS_ASSERT(false);
                return TInstanceHandleList();
            }
            ReadSlides(inReader);
        }

        return retval;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////
    // Public API
    /////////////////////////////////////////////////////////////////////////////////////////////
    struct ScopedPreserveFileIds
    {
        bool &m_PreserveFileIds;
        ScopedPreserveFileIds(bool &inPreserveVariable)
            : m_PreserveFileIds(inPreserveVariable)
        {
            m_PreserveFileIds = inPreserveVariable;
            // The act of preserving file ids means that we have to write information
            // into DataModel.  Nothing expects this at this time and this information really shouldn't
            // be transmitted into the universe.  Transmitting the changes could cause the rendering
            // system to render the scene a bunch.
            m_PreserveFileIds = true;
            SetDataModelSignalsEnabled(false);
        }
        ~ScopedPreserveFileIds()
        {
            m_PreserveFileIds = false;
            SetDataModelSignalsEnabled(true);
        }
    };

    void SerializeScene(IDOMWriter &inWriter) override
    {
        reset();
        ScopedPreserveFileIds __preserveFileIds(m_PreserveFileIds);

        vector<Qt3DSDMInstanceHandle> theGraphRoots;
        CGraphIterator theIterator;
        m_AssetGraph.GetRoots(theIterator);
        for (; theIterator.IsDone() == false; ++theIterator)
            theGraphRoots.push_back(theIterator.GetCurrent());

        if (theGraphRoots.empty())
            return;
        QT3DS_ASSERT(theGraphRoots.size() == 1);
        DoSerializeScene(inWriter, &theGraphRoots[0], 1);
        TGuideHandleList theGuides = m_GuideSystem.GetAllGuides();
        // sort the guides by handle value to keep the file as stable as possible.
        std::sort(theGuides.begin(), theGuides.end());
        {
            IDOMWriter::Scope fileScope(inWriter, "Guides");
            if (!m_GuideSystem.AreGuidesEditable())
                inWriter.Att("locked", true);

            for (size_t idx = 0, end = theGuides.size(); idx < end; ++idx) {
                qt3dsdm::SGuideInfo theGuideInfo = m_GuideSystem.GetGuideInfo(theGuides[idx]);
                IDOMWriter::Scope guideScope(inWriter, "Guide");
                inWriter.Att("direction", theGuideInfo.m_Direction);
                inWriter.Att("position", theGuideInfo.m_Position);
                inWriter.Att(L"width", theGuideInfo.m_Width);
            }
        }
    }

    void SerializeScene(IDOMReader &inReader, const QString &inDocumentDirectory,
                                int inUIPVersion) override
    {
        reset();
        ScopedPreserveFileIds __preserveFileIds(m_PreserveFileIds);
        m_UIPVersion = inUIPVersion;
        DoSerializeScene(inReader, inDocumentDirectory, 0);
        if (inReader.MoveToFirstChild(L"Guides")) {
            bool isLocked;
            if (inReader.Att("locked", isLocked))
                m_GuideSystem.SetGuidesEditable(!isLocked);
            for (bool success = inReader.MoveToFirstChild(); success;
                 success = inReader.MoveToNextSibling()) {
                qt3dsdm::SGuideInfo theInfo;
                inReader.Att("direction", theInfo.m_Direction);
                inReader.Att("position", theInfo.m_Position);
                inReader.Att("width", theInfo.m_Width);
                m_GuideSystem.SetGuideInfo(m_GuideSystem.CreateGuide(), theInfo);
            }
        }
    }

    // Read a partial serialization into this slide, attaching the instance as the last child of the
    // new root.
    virtual qt3dsdm::TInstanceHandleList
    SerializeSceneGraphObject(IDOMReader &inReader, const QString &inDocumentDirectory,
                              Qt3DSDMInstanceHandle inNewRoot, Qt3DSDMSlideHandle inActiveSlide) override
    {
        reset();
        m_ActiveSlide = inActiveSlide;
        m_PreserveFileIds = false;
        return DoSerializeScene(inReader, inDocumentDirectory, inNewRoot);
    }

    // Write this instance and its children (and possibly its slides) to a writer.
    // Equivalent to the older partial serialization system
    void SerializeSceneGraphObjects(IDOMWriter &inWriter,
                                            const TInstanceHandleList &inInstances,
                                            Qt3DSDMSlideHandle inActiveSlide) override
    {
        if (inInstances.empty())
            return;
        reset();
        QT3DS_ASSERT(inActiveSlide.Valid());
        m_ActiveSlide = inActiveSlide;
        m_PreserveFileIds = false;
        // It is fine if the parent is invalid
        m_ActiveSlideParent = m_SlideCore.GetParentSlide(m_ActiveSlide);
        DoSerializeScene(inWriter, inInstances.data(), (QT3DSU32)inInstances.size());
    }

    // Save and load just a single action
    void SerializeAction(qt3dsdm::IDOMWriter &inWriter, Qt3DSDMSlideHandle inSlide,
                                 Qt3DSDMActionHandle inAction) override
    {
        reset();
        m_PreserveFileIds = false;
        const SActionInfo &theInfo(m_ActionCore.GetActionInfo(inAction));
        SerializeAction(inWriter, inSlide, theInfo.m_Owner, inAction);
        WriteExternalReferences(inWriter);
    }

    // Load a new action onto this root object
    qt3dsdm::Qt3DSDMActionHandle SerializeAction(qt3dsdm::IDOMReader &inReader,
                                                      qt3dsdm::Qt3DSDMInstanceHandle inNewRoot,
                                                      qt3dsdm::Qt3DSDMSlideHandle inSlide) override
    {
        reset();
        m_PreserveFileIds = false;
        ReadExternalReferences(inReader);
        if (inReader.MoveToFirstChild(L"Action"))
            return ParseAction(inReader, inSlide, inNewRoot);
        return 0;
    }

    struct SInstanceInVector
    {
        const TInstanceHandleList &m_Vec;
        SInstanceInVector(const TInstanceHandleList &v)
            : m_Vec(v)
        {
        }
        bool operator()(Qt3DSDMInstanceHandle hdl)
        {
            return find(m_Vec.begin(), m_Vec.end(), hdl) != m_Vec.end();
        }
    };

    struct SParentInSlide
    {
        Qt3DSDMSlideHandle m_Slide;
        ISlideSystem &m_SlideSystem;
        CGraph &m_AssetGraph;

        SParentInSlide(Qt3DSDMSlideHandle inSlide, ISlideSystem &inSystem, CGraph &inAssetGraph)
            : m_Slide(inSlide)
            , m_SlideSystem(inSystem)
            , m_AssetGraph(inAssetGraph)
        {
        }

        bool operator()(Qt3DSDMInstanceHandle inInstance)
        {
            Qt3DSDMInstanceHandle theParent = m_AssetGraph.GetParent(inInstance);
            if (theParent.Valid() && m_SlideSystem.GetAssociatedSlide(theParent) == m_Slide)
                return true;
            return false;
        }
    };

    void SerializeSlide(qt3dsdm::IDOMWriter &inWriter, qt3dsdm::Qt3DSDMSlideHandle inSlide) override
    {
        reset();
        m_PreserveFileIds = false;
        m_ActiveSlide = inSlide;
        // It is fine if the parent is invalid
        m_ActiveSlideParent = m_SlideCore.GetParentSlide(m_ActiveSlide);
        TInstanceHandleList theSlideInstances;
        m_SlideSystem.GetAssociatedInstances(inSlide, theSlideInstances);
        if (m_ActiveSlideParent.Valid()) {
            TInstanceHandleList theParentInstances;
            m_SlideSystem.GetAssociatedInstances(m_ActiveSlideParent, theParentInstances);
            erase_if(theSlideInstances, SInstanceInVector(theParentInstances));
            erase_if(theSlideInstances, SParentInSlide(inSlide, m_SlideSystem, m_AssetGraph));
        }
        Qt3DSDMInstanceHandle *theInstancePtr = NULL;
        if (theSlideInstances.size()) {
            BuildGraphOrderMap(theInstancePtr, (QT3DSU32)theSlideInstances.size());
            SortSlideInstanceList(theSlideInstances);
            theInstancePtr = &theSlideInstances[0];
        }
        DoSerializeScene(inWriter, theInstancePtr, (QT3DSU32)theSlideInstances.size(), true);
    }

    Qt3DSDMSlideHandle SerializeSlide(qt3dsdm::IDOMReader &inReader,
                                      const QString &inDocumentDirectory,
                                      qt3dsdm::Qt3DSDMSlideHandle inMaster, int newIndex) override
    {
        reset();
        m_PreserveFileIds = false;
        m_ActiveSlideParent = inMaster;
        Qt3DSDMSlideHandle retval = m_SlideSystem.DuplicateSlide(inMaster, newIndex + 1);
        m_ActiveSlide = retval;
        Qt3DSDMInstanceHandle theSlideInstance = m_SlideCore.GetSlideInstance(retval);
        Qt3DSDMInstanceHandle theMasterInstance(m_SlideSystem.GetSlideInstance(inMaster));
        SLong4 theComponentGuid =
                GetGuid(theMasterInstance, m_ObjectDefinitions.m_Slide.m_ComponentId);
        m_DataCore.SetInstancePropertyValue(
                    theSlideInstance, m_ObjectDefinitions.m_Slide.m_ComponentId, theComponentGuid);
        DoSerializeScene(inReader, inDocumentDirectory, 0);
        return retval;
    }
};
}

std::shared_ptr<IComposerSerializer> IComposerSerializer::CreateGraphSlideSerializer(
        IDataCore &inDataCore, IMetaData &inMetaData, ISlideCore &inSlideCore,
        IAnimationCore &inAnimationCore, IActionCore &inActionCore, CGraph &inAssetGraph,
        ISlideSystem &inSlideSystem, IActionSystem &inActionSystem, ISlideGraphCore &inSlideGraphCore,
        SComposerObjectDefinitions &inObjectDefinitions,
        std::shared_ptr<Q3DStudio::IImportFailedHandler> inFailedHandler, IGuideSystem &inGuideSystem)
{
    return std::shared_ptr<SComposerSerializerImpl>(new SComposerSerializerImpl(
                                                          inDataCore, inMetaData, inSlideCore, inAnimationCore, inActionCore, inAssetGraph,
                                                          inSlideSystem, inActionSystem, inSlideGraphCore, inObjectDefinitions, inFailedHandler,
                                                          inGuideSystem));
}
