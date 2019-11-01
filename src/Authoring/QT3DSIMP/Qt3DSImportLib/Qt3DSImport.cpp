/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "Qt3DSImport.h"
#include "Qt3DSImportImpl.h"
#include "Qt3DSFileTools.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMWStrOps.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "Qt3DSImportComposerTypes.h"
#include "Qt3DSFileToolsSeekableMeshBufIOStream.h"
#include "foundation/StrConvertUTF.h"
#include <QtCore/qurl.h>

using qt3dsdm::IStringTable;

namespace qt3dsdm {
}

using namespace qt3dsimp;

namespace eastl {
template <>
struct hash<AnimationId>
{
    size_t operator()(const AnimationId &id) const
    {
        return hash<TIMPHandle>()(id.m_Instance) ^ hash<TCharPtr>()(id.m_Property)
            ^ hash<QT3DSU32>()(id.m_SubPropIndex);
    }
};

template <>
struct equal_to<AnimationId>
{
    bool operator()(const AnimationId &lhs, const AnimationId &rhs) const
    {
        return lhs.m_Instance == rhs.m_Instance && AreEqual(lhs.m_Property, rhs.m_Property)
            && lhs.m_SubPropIndex == rhs.m_SubPropIndex;
    }
};
}

using eastl::make_pair;
using std::get;
using std::tuple;
using std::shared_ptr;

namespace qt3ds {
namespace foundation {
}
}

using namespace Q3DStudio;

namespace {

template <typename TDataType>
struct AddRemoveImpl
{
    ImportArray<TDataType> m_Existing;
    ImportArray<TDataType> m_Added;
    ImportArray<TDataType> m_Removed;

    operator AddRemoveData<TDataType>() const
    {
        return AddRemoveData<TDataType>(m_Existing, m_Added, m_Removed);
    }
    void clear()
    {
        m_Existing.clear();
        m_Added.clear();
        m_Removed.clear();
    }
};

struct ImportReportImpl
{
    // There is a precondition upon added instances that parents are
    // added before children.
    AddRemoveImpl<InstanceDesc> m_Instances;
    AddRemoveImpl<ParentChildLink> m_Links;
    ImportArray<Pair<InstanceDesc, ComposerObjectTypes::Enum>> m_TypeChanges;
    AddRemoveImpl<Pair<TCharPtr, TCharPtr>> m_Images;
    AddRemoveImpl<Pair<TCharPtr, TCharPtr>> m_Meshes;
    AddRemoveImpl<Pair<TCharPtr, TCharPtr>> m_PathBuffers;
    AddRemoveImpl<Animation> m_Animations;

    operator ImportReport()
    {
        return ImportReport(m_Instances, m_Links, m_Images, m_Meshes, m_PathBuffers, m_Animations);
    }

    void clear()
    {
        m_Instances.clear();
        m_Links.clear();
        m_Images.clear();
        m_Meshes.clear();
        m_PathBuffers.clear();
        m_Animations.clear();
    }
};

struct InstanceDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumInstances(); }
    void GetItems(const Import &src, NVDataRef<InstanceDesc> data) { src.GetInstances(data); }
    // ID's *and* types have to match in order to get valid output
    bool HasItem(const Import &src, const InstanceDesc &data)
    {
        Option<InstanceDesc> result(src.FindInstanceById(data.m_Id));
        return result.hasValue() && result->m_Type == data.m_Type;
    }
};

bool FindChild(Option<InstanceDesc> hdlOpt, TCharPtr inCompareNextSibling,
               ComposerObjectTypes::Enum childType, NVDataRef<InstanceDesc> children)
{
    if (hdlOpt.hasValue() == false)
        return false;
    TCharPtr id = hdlOpt->m_Id;
    QT3DSIMP_FOREACH(idx, children.size())
    {
        TCharPtr nextSibling = idx + 1 < children.size() ? children[idx + 1].m_Id : L"";
        // Because we know that the import objects share a single string table,
        // we know that equal strings will actually be equal via ptr comparison
        if (children[idx].m_Id == id && children[idx].m_Type == childType
            && inCompareNextSibling == nextSibling)
            return true;
    }
    return false;
}

struct ImageDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumImages(); }
    void GetItems(const Import &src, NVDataRef<Pair<TCharPtr, TCharPtr>> data)
    {
        src.GetImages(data);
    }
    bool HasItem(const Import &src, const Pair<TCharPtr, TCharPtr> &data)
    {
        return src.FindImageByRelativePath(data.first).hasValue();
    }
};

struct MeshDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumMeshes(); }
    void GetItems(const Import &src, NVDataRef<Pair<TCharPtr, TCharPtr>> data)
    {
        src.GetMeshes(data);
    }
    bool HasItem(const Import &src, const Pair<TCharPtr, TCharPtr> &data)
    {
        return src.HasMesh(data.first);
    }
};

struct PathBufferDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumPathBuffers(); }
    void GetItems(const Import &src, NVDataRef<Pair<TCharPtr, TCharPtr>> data)
    {
        src.GetPathBuffers(data);
    }
    bool HasItem(const Import &src, const Pair<TCharPtr, TCharPtr> &data)
    {
        return src.FindPathBufferByRelativePath(data.first).hasValue();
    }
};

struct AnimationDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumAnimations(); }
    void GetItems(const Import &src, NVDataRef<Animation> data) { src.GetAnimations(data); }
    bool HasItem(const Import &src, const Animation &data)
    {
        return src.FindAnimation(data.m_InstanceId, data.m_PropertyName, data.m_SubPropertyIndex)
            .hasValue();
    }
};

/* Numbers generated from random.org
-784146867	-929005747	161935609	-555777497	-951253607
44567464	-991929537(used)	-541528542	-52708032	765363476
*/

struct MeshEntry
{
    TCharPtr m_SourceId;
    TCharPtr m_FilePath;
    TCharPtr m_ReferencePath; // file path plus revision
    MeshEntry(TCharPtr inSourceId, CFilePath inPath, QT3DSU32 inRevision,
              qt3dsdm::IStringTable &inStringTable)
        : m_SourceId(inSourceId)
        , m_FilePath(L"")
        , m_ReferencePath(L"")
    {
        m_FilePath = inStringTable.RegisterStr(inPath.toCString());
        inPath.SetIdentifier(QString::number(inRevision));
        m_ReferencePath = inStringTable.RegisterStr(inPath.toCString());
    }

    MeshEntry(TCharPtr inSourceId, TCharPtr inPath, qt3dsdm::IStringTable &inStringTable)
        : m_SourceId(inStringTable.RegisterStr(inSourceId))
        , m_FilePath(inStringTable.RegisterStr(inPath))
        , m_ReferencePath(L"")
    {
    }
};

struct STCharPtrHash
{
#ifdef _WIN32
    size_t operator()(TCharPtr nm) const
    {
        StaticAssert<sizeof(wchar_t) == sizeof(char16_t)>::valid_expression();
        return eastl::hash<const char16_t *>()(reinterpret_cast<const char16_t *>(nm));
    }

#else
    size_t operator()(TCharPtr nm) const
    {
        StaticAssert<sizeof(wchar_t) == sizeof(char32_t)>::valid_expression();
        return eastl::hash<const char32_t *>()(reinterpret_cast<const char32_t *>(nm));
    }
#endif
};

struct STCharPtrEqualTo
{
    bool operator()(TCharPtr lhs, TCharPtr rhs) const { return AreEqual(lhs, rhs); }
};

class ImportImpl : public Import
{
public:
    typedef ImportHashMap<TCharPtr, Instance *, STCharPtrHash, STCharPtrEqualTo> TIdToInstanceMap;
    typedef ImportHashMap<TCharPtr, TCharPtr, STCharPtrHash, STCharPtrEqualTo> TStrToStrMap;
    typedef ImportHashMap<TCharPtr, MeshEntry, STCharPtrHash, STCharPtrEqualTo> TPathToMeshMap;

    TStringTablePtr m_StringTablePtr;
    qt3dsdm::IStringTable &m_StringTable;
    ImportHashSet<TIMPHandle> m_ValidInstances;
    ImportHashSet<TIMPHandle> m_InValidInstances;
    TIdToInstanceMap m_IdToInstMap;
    // Mapping from original ID to new location
    TStrToStrMap m_Images;
    TStrToStrMap m_PathBuffers;
    // Mapping from mesh name to new location
    TPathToMeshMap m_Meshes;
    ImportHashMap<AnimationId, Animation *> m_Animations;

    CFilePath m_DestDirectory;
    CFilePath m_SrcDirectory;
    CFilePath m_SrcFile;
    CFilePath m_FullSrcDirectory;
    CFilePath m_ImageDir;
    CFilePath m_MeshDir;
    CFilePath m_PathBufferDir;
    mutable ImportReportImpl m_ImportReport;
    eastl::string m_ConvertStr;

    virtual ~ImportImpl()
    {
        for (ImportHashSet<TIMPHandle>::iterator iter = m_ValidInstances.begin(),
                                                 end = m_ValidInstances.end();
             iter != end; ++iter)
            delete fromHdl((*iter));
        for (ImportHashMap<AnimationId, Animation *>::iterator iter = m_Animations.begin(),
                                                               end = m_Animations.end();
             iter != end; ++iter)
            free(iter->second);
        m_ValidInstances.clear();
        m_Animations.clear();
        m_InValidInstances.clear();
    }

    ImportImpl(TStringTablePtr strTable, const Q3DStudio::CString &srcFile,
               const Q3DStudio::CString &destDirectory, const wchar_t *imagesDir = L"",
               const wchar_t *meshesDir = L"")
        : m_StringTablePtr(strTable)
        , m_StringTable(*strTable.get())
        , m_DestDirectory(destDirectory)
    {
        if (srcFile.Length()) {
            m_FullSrcDirectory = CFilePath(srcFile).GetDirectory();
            m_SrcFile = CFilePath::GetRelativePathFromBase(destDirectory, srcFile);
            m_SrcDirectory = CFilePath(m_SrcFile).GetDirectory();
        }
        // On load, images and meshes directories will be trivial.
        if (!IsTrivial(imagesDir)) {
            m_ImageDir = imagesDir;
            if (m_ImageDir.isRelative())
                m_ImageDir = CFilePath::CombineBaseAndRelative(destDirectory, CString(imagesDir));
        }
        if (!IsTrivial(meshesDir)) {
            m_MeshDir = meshesDir;
            if (m_MeshDir.isRelative())
                m_MeshDir = CFilePath::CombineBaseAndRelative(destDirectory, CString(meshesDir));
        }
        m_PathBufferDir = CFilePath::CombineBaseAndRelative(destDirectory, CString(L"paths"));
    }

    TCharPtr RegisterStr(TCharPtr str) override { return m_StringTable.RegisterStr(str); }
    QString GetSrcFile() const override { return m_SrcFile.toQString(); }
    QString GetDestDir() const override { return m_DestDirectory.toQString(); }
    QString GetImageDir() const override { return m_ImageDir.toQString(); }
    QString GetMeshDir() const override { return m_MeshDir.toQString(); }
    QString GetPathBufferDir() const override { return m_PathBufferDir.toQString(); }
    void Release() override { delete this; }

    Instance *GetInstance(TIMPHandle inst) const
    {
        if (m_ValidInstances.contains(inst))
            return fromHdl(inst);
        return nullptr;
    }

    Instance *GetInstance(TIMPHandle inst)
    {
        if (m_ValidInstances.contains(inst))
            return fromHdl(inst);
        return nullptr;
    }

    Instance *GetInstance(TCharPtr id)
    {
        ImportHashMap<TCharPtr, Instance *>::const_iterator entry = m_IdToInstMap.find(id);
        if (entry != m_IdToInstMap.end())
            return entry->second;
        return nullptr;
    }

    Instance *GetInstance(TCharPtr id) const
    {
        ImportHashMap<TCharPtr, Instance *>::const_iterator entry = m_IdToInstMap.find(id);
        if (entry != m_IdToInstMap.end())
            return entry->second;
        return nullptr;
    }

    Option<InstanceDesc> GetInstanceByHandle(TIMPHandle inst) const override
    {
        if (m_ValidInstances.contains(inst))
            return *fromHdl(inst);
        return Empty();
    }
    Option<InstanceDesc> FindInstanceById(TCharPtr inst) const override
    {
        Instance *retval = GetInstance(inst);
        if (retval && retval->m_Valid)
            return *retval;
        return Empty();
    }

    Option<InstanceDesc> FindAnyInstanceById(TCharPtr inst) const override
    {
        Instance *retval = GetInstance(inst);
        if (retval)
            return *retval;
        return Empty();
    }

    // Add a mapping from an named id to a handle
    QT3DSU32 GetNumInstances() const override
    {
        return (QT3DSU32)(m_ValidInstances.size() - m_InValidInstances.size());
    }

    void AddInstance(Instance *inInstance, NVDataRef<InstanceDesc> &inAdded, QT3DSU32 &addIdx,
                     QT3DSU32 numItems) const
    {
        if (addIdx < numItems) {
            inAdded[addIdx] = *inInstance;
            ++addIdx;
            QT3DSIMP_FOREACH(idx, (QT3DSU32)inInstance->m_Children.size())
            {
                Instance *child = GetInstance(inInstance->m_Children[idx]);
                AddInstance(child, inAdded, addIdx, numItems);
            }
        }
    }

    QT3DSU32 GetInstances(NVDataRef<InstanceDesc> outDescs) const override
    {
        QT3DS_ASSERT(outDescs.size() >= GetNumInstances());
        QT3DSU32 numItems = qMin(outDescs.size(), GetNumInstances());
        QT3DSU32 idx = 0;

        ImportHashSet<TIMPHandle> &validInstances(
            const_cast<ImportHashSet<TIMPHandle> &>(m_ValidInstances));
        for (ImportHashSet<TIMPHandle>::iterator theIter = validInstances.begin(),
                                                 end = validInstances.end();
             theIter != end; ++theIter) {
            if (m_InValidInstances.contains(*theIter))
                continue;

            Instance *theInstance = GetInstance(*theIter);
            if (theInstance->m_Parent == 0)
                AddInstance(theInstance, outDescs, idx, numItems);
        }

        return numItems;
    }
    QT3DSU32 GetNumProperties(TIMPHandle instance) const override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }
        return (QT3DSU32)inst->m_PropertyValues.size();
    }
    QT3DSU32 GetProperties(TIMPHandle instance, NVDataRef<PropertyValue> outBuffer) const override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }
        QT3DSU32 numItems = qMin((QT3DSU32)outBuffer.size(), (QT3DSU32)inst->m_PropertyValues.size());
        QT3DSU32 idx = 0;
        for (ImportHashMap<ComposerPropertyNames::Enum, SInternValue>::iterator
                 iter = inst->m_PropertyValues.begin(),
                 end = inst->m_PropertyValues.end();
             iter != end && idx < numItems; ++iter, ++idx)
            outBuffer[idx] = PropertyValue(iter->first, iter->second);
        return numItems;
    }
    Option<SValue> GetInstancePropertyValue(TIMPHandle instance,
                                                    ComposerPropertyNames::Enum val) const override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return Empty();
        }
        const ImportHashMap<ComposerPropertyNames::Enum, SInternValue>::iterator entry =
            inst->m_PropertyValues.find(val);
        if (entry != inst->m_PropertyValues.end())
            return entry->second.GetValue();
        return Empty();
    }

    // Returns number of valid child instances
    QT3DSU32 GetNumChildren(TIMPHandle instance) const override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }
        QT3DSU32 count = 0;
        for (QT3DSU32 i = 0; i < inst->m_Children.size(); ++i) {
            Instance *theInstance(GetInstance(inst->m_Children[i]));
            if (theInstance && theInstance->m_Valid)
                count++;
        }

        return count;
    }

    // Returns valid children in the childBuffer parameter.
    QT3DSU32 GetChildren(TIMPHandle instance, NVDataRef<InstanceDesc> childBuffer) const override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }

        QT3DSU32 count = 0;
        for (QT3DSU32 i = 0; i < inst->m_Children.size(); ++i) {
            Instance *theInstance(GetInstance(inst->m_Children[i]));
            if (theInstance && theInstance->m_Valid) {
                childBuffer[count] = FindInstanceById(inst->m_Children[i]);
                count++;
                if (count == childBuffer.size())
                    break;
            }
        }
        return count;
    }

    void MarkInstanceInvalid(TIMPHandle instance) override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return;
        }
        if (m_InValidInstances.insert(instance).second) {
            inst->MarkInvalid();
            QT3DSIMP_FOREACH(idx, (QT3DSU32)inst->m_Children.size())
            {
                Instance *theInstance(GetInstance(inst->m_Children[idx]));
                if (theInstance)
                    MarkInstanceInvalid(toHdl(theInstance));
            }
            std::vector<AnimationId> animationsToErase;
            for (ImportHashMap<AnimationId, Animation *>::iterator iter = m_Animations.begin(),
                                                                   end = m_Animations.end();
                 iter != end; ++iter) {
                if (iter->first.m_Instance == instance)
                    animationsToErase.push_back(iter->first);
            }
            for (size_t idx = 0, end = animationsToErase.size(); idx < end; ++idx)
                m_Animations.erase(animationsToErase[idx]);
        }
    }

    TIMPHandle CreateInstance(TCharPtr name, ComposerObjectTypes::Enum inType) override
    {
        if (IsTrivial(name) == false) {
            Option<InstanceDesc> exists = FindInstanceById(name);
            bool hasValue = exists.hasValue();
            if (hasValue) {
                QT3DS_ASSERT(false);
                return exists->m_Handle;
            }
        }
        name = m_StringTable.RegisterStr(name);
        InstanceDesc newDesc;
        newDesc.m_Id = name;
        newDesc.m_Type = inType;
        Instance *newInst = new Instance(newDesc);
        m_ValidInstances.insert(toHdl(newInst));
        m_IdToInstMap.insert(eastl::make_pair(name, newInst));
        return toHdl(newInst);
    }

    Instance *CopyInstanceHierarchy(Instance &inSource, ImportHashMap<TCharPtr, TCharPtr> &inIdMap)
    {
        wstring uniqueStem(inSource.m_Id);
        wstring instId(uniqueStem);
        int index = 1;
        do {
            wchar_t buf[16];
            WStrOps<QT3DSU32>().ToStr(index, toDataRef(buf, 16));
            ++index;
            instId.assign(uniqueStem);
            instId.append(L"_");
            instId.append(buf);
        } while (FindInstanceById(RegisterStr(instId.c_str())).hasValue());

        Instance *retval =
            GetInstance(CreateInstance(RegisterStr(instId.c_str()), inSource.m_Type));
        inIdMap.insert(eastl::make_pair(RegisterStr(inSource.m_Id), RegisterStr(retval->m_Id)));

        QT3DSIMP_FOREACH(idx, (QT3DSU32)inSource.m_Children.size())
        {
            Instance *oldChild = GetInstance(inSource.m_Children[idx]);
            if (oldChild == nullptr) {
                QT3DS_ASSERT(false);
                return retval;
            }
            Instance *child = CopyInstanceHierarchy(*oldChild, inIdMap);
            AddChild(toHdl(retval), toHdl(child));
        }
        return retval;
    }

    void CopyInstancePropertiesAndAnimation(Instance &inSource,
                                            ImportHashMap<TCharPtr, TCharPtr> &inIdMap)
    {
        const ImportHashMap<TCharPtr, TCharPtr>::iterator theNewItemId =
            inIdMap.find(inSource.m_Id);
        if (theNewItemId == inIdMap.end()) {
            QT3DS_ASSERT(false);
            return;
        }
        Instance *theCopy = GetInstance(theNewItemId->second);
        if (theCopy == nullptr) {
            QT3DS_ASSERT(false);
            return;
        }

        for (ImportHashMap<ComposerPropertyNames::Enum, SInternValue>::iterator
                 iter = inSource.m_PropertyValues.begin(),
                 end = inSource.m_PropertyValues.end();
             iter != end; ++iter) {
            SInternValue current(iter->second);
            if (GetValueType(current.GetValue()) == DataModelDataType::StringRef) {
                const SStringRef &theRef = get<SStringRef>(current.GetValue());
                const ImportHashMap<TCharPtr, TCharPtr>::iterator theNewId =
                    inIdMap.find(theRef.m_Id);
                if (theNewId != inIdMap.end())
                    current = SInternValue::ISwearThisHasAlreadyBeenInternalized(
                        SValue(SStringRef(theNewId->second)));
            }
            theCopy->m_PropertyValues.insert(eastl::make_pair(iter->first, current));
        }
        for (ImportHashMap<AnimationId, Animation *>::iterator iter = m_Animations.begin(),
                                                               end = m_Animations.end();
             iter != end; ++iter) {
            if (iter->first.m_Instance == toHdl(&inSource)) {
                const Animation *theSrcAnimation = iter->second;
                DoAddAnimation(theCopy->m_Id, theSrcAnimation->m_PropertyName,
                               theSrcAnimation->m_SubPropertyIndex, theSrcAnimation->m_Type,
                               theSrcAnimation->m_Keyframes);
            }
        }
        QT3DSIMP_FOREACH(idx, (QT3DSU32)inSource.m_Children.size())
        {
            Instance *oldChild = GetInstance(inSource.m_Children[idx]);
            if (oldChild == nullptr) {
                QT3DS_ASSERT(false);
                return;
            }
            CopyInstancePropertiesAndAnimation(*oldChild, inIdMap);
        }
    }

    TIMPHandle CopyInstance(TIMPHandle inSource) override
    {
        Instance *inst = GetInstance(inSource);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }

        // map from old ids to new ids
        ImportHashMap<TCharPtr, TCharPtr> idMap;
        // copy the hierarchy first
        Instance *retval = CopyInstanceHierarchy(*inst, idMap);

        Instance *parent = fromHdl(inst->m_Parent);
        if (parent != nullptr)
            parent->AddChild(retval, inst);

        // Copy properties and animations
        CopyInstancePropertiesAndAnimation(*inst, idMap);

        return toHdl(retval);
    }

    bool SetInstanceProperties(TIMPHandle instance, NVConstDataRef<PropertyValue> inBuffer) override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return false;
        }
        inst->SetPropertyValues(inBuffer, m_StringTable);
        return true;
    }
    bool DoSetInstancePropertyValue(TIMPHandle instance, ComposerPropertyNames::Enum pname,
                                            const TImportModelValue &val) override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return false;
        }
        inst->SetPropertyValue(pname, SInternValue(val, m_StringTable));
        return true;
    }

    bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            TCharPtr val) override
    {
        return DoSetInstancePropertyValue(inst, pname, std::make_shared<CDataStr>(val));
    }
    bool AddChild(TIMPHandle instance, TIMPHandle childHdl) override
    {
        Instance *inst = GetInstance(instance);
        Instance *child = GetInstance(childHdl);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return false;
        }
        if (child == nullptr) {
            QT3DS_ASSERT(false);
            return false;
        }
        // Hmm, two parents both have same child...
        // not idea.
        if (child->m_Parent != 0) {
            QT3DS_ASSERT(false);
            return false;
        }

        inst->AddChild(child);
        return true;
    }

    static inline ImportErrorCodes::Enum FromFileErrorCode(Q3DStudio::FileErrorCodes::Enum val)
    {
        switch (val) {
        case Q3DStudio::FileErrorCodes::SourceNotReadable:
            return ImportErrorCodes::SourceFileNotReadable;
        case Q3DStudio::FileErrorCodes::DestNotWriteable:
            return ImportErrorCodes::ResourceNotWriteable;
        case Q3DStudio::FileErrorCodes::NoError:
            return ImportErrorCodes::NoError;
        case Q3DStudio::FileErrorCodes::SourceNotExist:
            return ImportErrorCodes::SourceFileDoesNotExist;
        default:
            break;
        }

        QT3DS_ASSERT(false);
        return ImportErrorCodes::ResourceNotWriteable;
    }

    QT3DSU32 GetNumImages() const override { return (QT3DSU32)m_Images.size(); }
    QT3DSU32 GetImages(NVDataRef<Pair<TCharPtr, TCharPtr>> imgPaths) const override
    {
        QT3DSU32 numItems = qMin((QT3DSU32)imgPaths.size(), (QT3DSU32)m_Images.size());
        QT3DS_ASSERT(numItems == m_Images.size());
        QT3DSU32 idx = 0;
        for (ImportHashMap<TCharPtr, TCharPtr>::const_iterator iter = m_Images.begin(),
                                                               end = m_Images.end();
             iter != end && idx < numItems; ++iter, ++idx)
            imgPaths[idx] = Pair<TCharPtr, TCharPtr>(iter->first, iter->second);
        return numItems;
    }

    // Copies the an appropriate location in our import directory
    // Returns the path of the added image.  This may mangle the name slightly
    // In case of a conflict.
    CharPtrOrError AddImage(TCharPtr _imgPath) override
    {
        Option<TCharPtr> added = FindImageByPath(_imgPath);
        if (added.hasValue())
            return *added;

        if (!m_ImageDir.IsDirectory())
            m_ImageDir.CreateDir(true);

        if (!m_ImageDir.IsDirectory()) {
            return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory,
                                   m_ImageDir.toCString());
        }

        Q3DStudio::CString imgPath = CFilePath::GetAbsolutePath(CString(_imgPath));
        Q3DStudio::CString srcImgPath = CFilePath::GetRelativePathFromBase(m_FullSrcDirectory,
                                                                           imgPath);

        // Need to get rid of %20 and such in the path, or we won't find the image
        Q3DStudio::CString cleanImgPath
                = Q3DStudio::CString::fromQString(QUrl::fromPercentEncoding(
                                                      imgPath.toQString().toUtf8()));
        Q3DStudio::CString cleanSrcImgPath
                = Q3DStudio::CString::fromQString(QUrl::fromPercentEncoding(
                                                      srcImgPath.toQString().toUtf8()));

        Q3DStudio::SFileErrorCodeFileNameAndNumBytes copyResult
                = Q3DStudio::SFileTools::FindAndCopyDestFile(m_ImageDir, cleanImgPath);
        // Get the actual return value relative do our destination directory
        Q3DStudio::CString _retval = CFilePath::GetRelativePathFromBase(m_DestDirectory,
                                                                        copyResult.m_DestFilename);
        // Register the string, so we can hand retval back to clients
        TCharPtr retval = m_StringTable.RegisterStr(_retval.c_str());
        m_Images.insert(eastl::make_pair(m_StringTable.RegisterStr(cleanSrcImgPath.c_str()),
                                         retval));

        if (copyResult.m_Error != Q3DStudio::FileErrorCodes::NoError) {
            CharPtrOrError errorValue(ImportErrorData(FromFileErrorCode(copyResult.m_Error),
                                                      RegisterStr(cleanImgPath)));
            errorValue.m_Value = retval;
            return errorValue;
        }
        return retval;
    }
    CharPtrOrError AddOrReplaceImage(TCharPtr _imgPath, Option<TCharPtr> dstPath)
    {
        if (dstPath.hasValue()) {
            if (!m_ImageDir.IsDirectory())
                m_ImageDir.CreateDir(true);
            if (!m_ImageDir.IsDirectory()) {
                return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory,
                                       m_ImageDir.toCString());
            }
            CFilePath fullDestPath = CFilePath::CombineBaseAndRelative(m_DestDirectory,
                                                                       CString(dstPath.getValue()));
            Q3DStudio::SFileErrorCodeAndNumBytes copyResult = Q3DStudio::SFileTools::Copy(
                fullDestPath, Q3DStudio::FileOpenFlags(Q3DStudio::FileOpenFlagValues::Truncate
                                                       | Q3DStudio::FileOpenFlagValues::Open
                                                       | Q3DStudio::FileOpenFlagValues::Create),
                        CString(_imgPath));
            // Regardless of if the copy operation succeeds or not, if the destination exists
            // already then we enter it into our dictionary.
            if (fullDestPath.IsFile()) {
                CFilePath imgPath = CFilePath::GetAbsolutePath(CString(_imgPath));
                CFilePath srcImgPath = CFilePath::GetRelativePathFromBase(m_FullSrcDirectory,
                                                                          imgPath);
                // Need to get rid of %20 and such in the path, or we won't find the image
                Q3DStudio::CString cleanImgPath
                        = Q3DStudio::CString::fromQString(
                            QUrl::fromPercentEncoding(imgPath.toQString().toUtf8()));
                Q3DStudio::CString cleanSrcImgPath
                        = Q3DStudio::CString::fromQString(
                            QUrl::fromPercentEncoding(srcImgPath.toQString().toUtf8()));
                m_Images.insert(eastl::make_pair(m_StringTable.RegisterStr(cleanSrcImgPath),
                                                 m_StringTable.RegisterStr(dstPath.getValue())));
            }

            TCharPtr returnPath = dstPath.getValue();

            if (copyResult.m_Error != Q3DStudio::FileErrorCodes::NoError) {
                QT3DS_ASSERT(false);
                const wchar_t *extraData = nullptr;
                ImportErrorCodes::Enum error = FromFileErrorCode(copyResult.m_Error);
                if (error == ImportErrorCodes::ResourceNotWriteable)
                    extraData = dstPath.getValue();
                else if (error == ImportErrorCodes::SourceFileNotReadable)
                    extraData = _imgPath;
                CharPtrOrError errorRet(ImportErrorData(error, extraData));
                errorRet.m_Value = returnPath;
                return errorRet;
            }
            return returnPath;
        }
        return AddImage(_imgPath);
    }
    Option<TCharPtr> FindImageByPath(TCharPtr _imgPath) const override
    {
        Q3DStudio::CString imgPath = CFilePath::GetAbsolutePath(CString(_imgPath));
        Q3DStudio::CString srcImgPath = CFilePath::GetRelativePathFromBase(m_FullSrcDirectory,
                                                                           imgPath);
        // Need to get rid of %20 and such in the path, or we won't find the image
        Q3DStudio::CString cleanPath
                = Q3DStudio::CString::fromQString(QUrl::fromPercentEncoding(
                                                      srcImgPath.toQString().toUtf8()));
        return FindImageByRelativePath(cleanPath.c_str());
    }

    Option<TCharPtr> FindImageByRelativePath(TCharPtr imgPath) const override
    {
        ImportHashMap<TCharPtr, TCharPtr>::const_iterator entry = m_Images.find(imgPath);
        if (entry != m_Images.end())
            return entry->second;
        return Empty();
    }

    QT3DSU32 GetNumMeshes() const override { return (QT3DSU32)m_Meshes.size(); }
    QT3DSU32 GetMeshes(NVDataRef<Pair<TCharPtr, TCharPtr>> bufferPaths) const override
    {
        QT3DSU32 numItems = qMin((QT3DSU32)m_Meshes.size(), bufferPaths.size());
        QT3DS_ASSERT(numItems == m_Meshes.size());
        QT3DSU32 idx = 0;
        for (ImportHashMap<TCharPtr, MeshEntry>::const_iterator iter = m_Meshes.begin(),
                                                                end = m_Meshes.end();
             iter != end && idx < numItems; ++iter, ++idx)
            bufferPaths[idx] = Pair<TCharPtr, TCharPtr>(iter->first, iter->second.m_FilePath);
        return numItems;
    }
    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    CharPtrOrError AddMesh(const Mesh &meshBuffer, TCharPtr _name) override
    {
        if (IsTrivial(_name)) {
            QT3DS_ASSERT(false);
            return L"";
        }
        Q3DStudio::CString name = CFilePath::MakeSafeFileStem(_name);

        Option<TCharPtr> meshOpt = FindMeshReferencePathByName(name.c_str());
        if (meshOpt.hasValue())
            return *meshOpt;

        if (!m_MeshDir.IsDirectory())
            m_MeshDir.CreateDir(true);
        if (!m_MeshDir.IsDirectory())
            return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_MeshDir.toCString());

        Q3DStudio::TFilePtr handf =
            Q3DStudio::SFileTools::FindUniqueDestFile(m_MeshDir, name, L"mesh");
        Qt3DSFileToolsSeekableMeshBufIOStream output(handf);
        MallocAllocator alloc;
        QT3DSU32 meshId = meshBuffer.SaveMulti(alloc, output);
        CFilePath _retval = CFilePath::GetRelativePathFromBase(m_DestDirectory, handf->m_Path);
        MeshEntry newEntry(m_StringTable.RegisterStr(name.c_str()), _retval, meshId, m_StringTable);
        m_Meshes.insert(eastl::make_pair(newEntry.m_SourceId, newEntry));
        return newEntry.m_ReferencePath;
    }

    CharPtrOrError AddOrReplaceMesh(const Mesh &meshBuffer, TCharPtr name, Option<TCharPtr> srcMesh)
    {
        using namespace Q3DStudio;
        if (srcMesh.hasValue()) {
            if (!m_MeshDir.IsDirectory())
                m_MeshDir.CreateDir(true);
            if (!m_MeshDir.IsDirectory()) {
                return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory,
                                       m_MeshDir.toCString());
            }

            CFilePath meshPath =
                    CFilePath::CombineBaseAndRelative(m_DestDirectory, CString(srcMesh.getValue()));

            meshPath = meshPath.filePath();
            Qt3DSFileToolsSeekableMeshBufIOStream output(
                        SFile::Wrap(SFile::OpenForWrite(meshPath, FileWriteFlags()), meshPath));
            if (output.IsOpen() == false) {
                QT3DS_ASSERT(false);
                return ImportErrorData(ImportErrorCodes::ResourceNotWriteable, srcMesh.getValue());
            }
            MallocAllocator allocator;
            QT3DSU32 meshId = meshBuffer.SaveMulti(allocator, output);

            CFilePath relativePath = CFilePath::GetRelativePathFromBase(m_DestDirectory, meshPath);

            MeshEntry newEntry(m_StringTable.RegisterStr(name), relativePath, meshId,
                               m_StringTable);
            m_Meshes.insert(eastl::make_pair(newEntry.m_SourceId, newEntry));
            return newEntry.m_ReferencePath;
        }
        return AddMesh(meshBuffer, name);
    }

    bool HasMesh(TCharPtr meshName) const override
    {
        return m_Meshes.find(meshName) != m_Meshes.end();
    }

    Option<TCharPtr> FindMeshReferencePathByName(TCharPtr meshName) const override
    {
        ImportHashMap<TCharPtr, MeshEntry>::const_iterator entry = m_Meshes.find(meshName);
        if (entry != m_Meshes.end()) {
            QT3DS_ASSERT(!IsTrivial(entry->second.m_ReferencePath));
            return entry->second.m_ReferencePath;
        }
        return Empty();
    }

    Option<TCharPtr> FindMeshFilePathByName(TCharPtr meshName) const override
    {
        ImportHashMap<TCharPtr, MeshEntry>::const_iterator entry = m_Meshes.find(meshName);
        if (entry != m_Meshes.end()) {
            QT3DS_ASSERT(!IsTrivial(entry->second.m_FilePath));
            return entry->second.m_FilePath;
        }
        return Empty();
    }

    QT3DSU32 GetNumPathBuffers() const override { return (QT3DSU32)m_PathBuffers.size(); }
    QT3DSU32 GetPathBuffers(NVDataRef<Pair<TCharPtr, TCharPtr>> pathBufferPaths) const override
    {
        QT3DSU32 numItems = qMin((QT3DSU32)pathBufferPaths.size(), (QT3DSU32)m_PathBuffers.size());
        QT3DS_ASSERT(numItems == m_PathBuffers.size());
        QT3DSU32 idx = 0;
        for (ImportHashMap<TCharPtr, TCharPtr>::const_iterator iter = m_PathBuffers.begin(),
                                                               end = m_PathBuffers.end();
             iter != end && idx < numItems; ++iter, ++idx)
            pathBufferPaths[idx] = Pair<TCharPtr, TCharPtr>(iter->first, iter->second);
        return numItems;
    }

    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    CharPtrOrError AddPathBuffer(const SPathBuffer &pathBuffer, TCharPtr _name) override
    {
        if (IsTrivial(_name)) {
            QT3DS_ASSERT(false);
            return L"";
        }
        Q3DStudio::CString name = CFilePath::MakeSafeFileStem(_name);

        Option<TCharPtr> pathBufferOpt = FindPathBufferByRelativePath(name.c_str());
        if (pathBufferOpt.hasValue())
            return *pathBufferOpt;

        if (!m_PathBufferDir.IsDirectory())
            m_PathBufferDir.CreateDir(true);
        if (!m_PathBufferDir.IsDirectory())
            return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_PathBufferDir.toCString());

        Q3DStudio::TFilePtr handf =
            Q3DStudio::SFileTools::FindUniqueDestFile(m_PathBufferDir, name, L"path");
        Qt3DSFileToolsSeekableMeshBufIOStream output(handf);
        MallocAllocator alloc;
        pathBuffer.Save(output);
        CFilePath _retval = CFilePath::GetRelativePathFromBase(m_DestDirectory, handf->m_Path);
        const wchar_t *return_Value = m_StringTable.RegisterStr(_retval.toCString());
        m_PathBuffers.insert(
            eastl::make_pair(m_StringTable.RegisterStr(name.c_str()), return_Value));
        return return_Value;
    }

    CharPtrOrError AddOrReplacePathBuffer(const SPathBuffer &pathBuffer, TCharPtr name,
                                          Option<TCharPtr> srcPathBuffer)
    {
        using namespace Q3DStudio;
        if (srcPathBuffer.hasValue()) {
            if (!m_PathBufferDir.IsDirectory())
                m_PathBufferDir.CreateDir(true);
            if (!m_PathBufferDir.IsDirectory())
                return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_PathBufferDir.toCString());

            CFilePath pathBufferPath = CFilePath::CombineBaseAndRelative(
                m_DestDirectory, CString(srcPathBuffer.getValue()));

            pathBufferPath = pathBufferPath.filePath();
            Qt3DSFileToolsSeekableMeshBufIOStream output(SFile::Wrap(
                SFile::OpenForWrite(pathBufferPath, FileOpenFlags(FileOpenFlagValues::Open
                                                                  | FileOpenFlagValues::Create)),
                pathBufferPath));
            if (output.IsOpen() == false) {
                QT3DS_ASSERT(false);
                return ImportErrorData(ImportErrorCodes::ResourceNotWriteable,
                                       srcPathBuffer.getValue());
            }
            MallocAllocator allocator;
            pathBuffer.Save(output);

            CFilePath relativePath =
                CFilePath::GetRelativePathFromBase(m_DestDirectory, pathBufferPath);

            const wchar_t *relPath = m_StringTable.RegisterStr(relativePath.toCString());
            m_PathBuffers.insert(eastl::make_pair(m_StringTable.RegisterStr(name), relPath));
            return relPath;
        }
        return AddPathBuffer(pathBuffer, name);
    }

    Option<TCharPtr> FindPathBufferByPath(TCharPtr _pathBufferPath) const override
    {
        Q3DStudio::CString pathBufferPath = CFilePath::GetAbsolutePath(CString(_pathBufferPath));
        Q3DStudio::CString srcpathBufferPath =
            CFilePath::GetRelativePathFromBase(m_FullSrcDirectory, pathBufferPath);
        return FindPathBufferByRelativePath(srcpathBufferPath.c_str());
    }

    Option<TCharPtr> FindPathBufferByRelativePath(TCharPtr pathBufferPath) const override
    {
        ImportHashMap<TCharPtr, TCharPtr>::const_iterator entry =
            m_PathBuffers.find(pathBufferPath);
        if (entry != m_PathBuffers.end())
            return entry->second;
        return Empty();
    }

    QT3DSU32 GetNumAnimations() const override { return QT3DSU32(m_Animations.size()); }
    QT3DSU32 GetAnimations(NVDataRef<Animation> outBuffers) const override
    {
        QT3DSU32 numItems = qMin(QT3DSU32(m_Animations.size()), outBuffers.size());
        QT3DS_ASSERT(numItems == m_Animations.size());
        QT3DSU32 idx = 0;
        for (ImportHashMap<AnimationId, Animation *>::const_iterator iter = m_Animations.begin(),
                                                                     end = m_Animations.end();
             iter != end && idx < numItems; ++iter, ++idx)
            outBuffers[idx] = *iter->second;
        return numItems;
    }

    // Data is copied into this object, you can release the anim buffer data after this
    void DoAddAnimation(TCharPtr instance, TCharPtr propName, QT3DSU32 subPropIndex,
                                EAnimationType bufType, NVConstDataRef<QT3DSF32> values) override
    {
        propName = m_StringTable.RegisterStr(propName);
        Option<Animation> buffer(FindAnimation(instance, propName, subPropIndex));
        if (buffer.hasValue()) {
            QT3DS_ASSERT(false);
            return;
        }
        // Find Instance by id checks for the validity of the instance, so we should be OK here.
        Option<InstanceDesc> instOpt = FindInstanceById(instance);
        if (instOpt.hasValue() == false)
            return;

        m_Animations.insert(eastl::make_pair(
            AnimationId(instOpt->m_Handle, propName, subPropIndex),
            CreateAnimation(instOpt->m_Id, propName, subPropIndex, bufType, values)));
    }

    Option<Animation> FindAnimation(TCharPtr instId, TCharPtr propName,
                                            QT3DSU32 subPropIndex) const override
    {
        Option<InstanceDesc> instance(FindInstanceById(instId));
        if (instance.hasValue() == false)
            return Empty();
        AnimationId id(instance->m_Handle, propName, subPropIndex);
        ImportHashMap<AnimationId, Animation *>::const_iterator entry(m_Animations.find(id));
        if (entry != m_Animations.end())
            return *entry->second;
        return Empty();
    }

    template <typename TDataType, typename TDiffOp>
    void AddAddedItems(AddRemoveImpl<TDataType> &dtype, TDiffOp op) const
    {
        dtype.m_Added.resize(op.NumItems(*this));
        op.GetItems(*this, dtype.m_Added);
    }

    void FinalizeReport() const
    {
        // Now ensure the links are ordered appropriately so a single pass through each vector
        // is enough.
        std::stable_sort(m_ImportReport.m_Links.m_Existing.begin(),
                         m_ImportReport.m_Links.m_Existing.end());
        std::stable_sort(m_ImportReport.m_Links.m_Added.begin(),
                         m_ImportReport.m_Links.m_Added.end());
        std::stable_sort(m_ImportReport.m_Links.m_Removed.begin(),
                         m_ImportReport.m_Links.m_Removed.end());
    }

    // Add instances in depth first order in order to reduce the number of parent/child reording
    // issues.
    ImportReport CompileReport() const override
    {
        m_ImportReport.clear();
        AddAddedItems(m_ImportReport.m_Instances, InstanceDiffOp());
        AddAddedItems(m_ImportReport.m_Images, ImageDiffOp());
        AddAddedItems(m_ImportReport.m_Meshes, MeshDiffOp());
        AddAddedItems(m_ImportReport.m_Animations, AnimationDiffOp());

        ImportArray<InstanceDesc> childList;
        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Added.size())
        {
            const InstanceDesc &inst(m_ImportReport.m_Instances.m_Added[idx]);
            childList.resize(GetNumChildren(inst.m_Handle));

            GetChildren(inst.m_Handle, childList);
            QT3DSIMP_FOREACH(child, childList.size())
            {
                TCharPtr sibling = child + 1 < childList.size() ? childList[child + 1].m_Id : L"";
                m_ImportReport.m_Links.m_Added.push_back(
                    ParentChildLink(inst.m_Id, childList[child].m_Id, sibling));
            }
        }
        FinalizeReport();
        return m_ImportReport;
    }

    static QT3DSU32 FindHighestRevisionInDocument(IDOMReader &inReader)
    {
        QT3DSU32 retval = 0;
        IDOMReader::Scope readerScope(inReader);
        for (bool success = inReader.MoveToFirstChild(L"Project"); success;
             success = inReader.MoveToNextSibling(L"Project")) {
            QT3DSU32 tempId;
            if (inReader.Att("Revision", tempId))
                retval = qMax(retval, tempId);
        }
        return retval;
    }

    void Serialize(IDOMWriter &writer, ComposerObjectTypes::Enum inType, Animation &inAnimation,
                   MemoryBuffer<RawAllocator> &inTempBuf)
    {
        Instance *theInstance = GetInstance(inAnimation.m_InstanceId);
        if (theInstance == nullptr || theInstance->m_Valid == false)
            return;
        IDOMWriter::Scope __animScope(writer, L"AnimationTrack");
        TCharStr thePropName(inAnimation.m_PropertyName);
        SImportComposerTypes theTypes;
        SImportAsset &theAsset(theTypes.GetImportAssetForType(inType));
        DataModelDataType::Value theType(theAsset.GetPropertyDataType(thePropName.wide_str()));
        size_t arity = getDatatypeAnimatableArity(theType);
        if (arity == 0) {
            QT3DS_ASSERT(false);
            return;
        };
        if (arity > 1) {
            thePropName.append(L".");
            switch (inAnimation.m_SubPropertyIndex) {
            case 0:
                thePropName.append(L"x");
                break;
            case 1:
                thePropName.append(L"y");
                break;
            case 2:
                thePropName.append(L"z");
                break;
            case 3:
                thePropName.append(L"w");
                break;
            }
        }

        writer.Att(L"property", thePropName);
        writer.Att(L"type", inAnimation.m_Type);
        inTempBuf.clear();
        WCharTWriter bufWriter(inTempBuf);
        bufWriter.Write(
            toConstDataRef(inAnimation.m_Keyframes.begin(), inAnimation.m_Keyframes.size()), 20,
            writer.GetTabs() + 1);
        if (inTempBuf.size()) {
            char buffer[] = { 0, 0 };
            inTempBuf.write(buffer, 2);
            writer.Value((const wchar_t *)inTempBuf.begin());
        }
    }
    void SerializeAnimation(IDOMReader &reader, const wchar_t *inInstanceId,
                            MemoryBuffer<RawAllocator> &inTempBuf,
                            MemoryBuffer<RawAllocator> &inAttributeBuffer)
    {
        Animation theAnimation;
        theAnimation.m_InstanceId = inInstanceId;
        TCharStr theName;
        reader.Att(L"property", theName);
        eastl_size_t theDotPos(theName.find(L"."));
        if (theDotPos != wstring::npos) {
            wchar_t theExtension(theName.at(theDotPos + 1));
            theName = theName.substr(0, theDotPos);
            switch (theExtension) {
            default:
                QT3DS_ASSERT(false);
            // fallthrough intentional
            case 'x':
            case 'r':
                theAnimation.m_SubPropertyIndex = 0;
                break;
            case 'y':
            case 'g':
                theAnimation.m_SubPropertyIndex = 1;
                break;
            case 'z':
            case 'b':
                theAnimation.m_SubPropertyIndex = 2;
                break;
            case 'w':
            case 'a':
                theAnimation.m_SubPropertyIndex = 3;
                break;
            }
        }
        theAnimation.m_PropertyName = m_StringTable.RegisterStr(theName.wide_str());
        reader.Att(L"type", theAnimation.m_Type);
        inAttributeBuffer.clear();
        const char8_t *theValue;
        reader.Value(theValue);
        inAttributeBuffer.write(theValue, (QT3DSU32)strlen(theValue) + 1);
        inAttributeBuffer.write((QT3DSU16)0);
        WCharTReader theReader((char8_t *)inAttributeBuffer.begin(), inTempBuf, m_StringTable);
        NVConstDataRef<float> theData;
        theReader.ReadBuffer(theData);
        Animation *newAnim =
            CreateAnimation(theAnimation.m_InstanceId, theAnimation.m_PropertyName,
                            theAnimation.m_SubPropertyIndex, theAnimation.m_Type, theData);
        m_Animations.insert(eastl::make_pair(
            AnimationId(toHdl(GetInstance(theAnimation.m_InstanceId)), theAnimation.m_PropertyName,
                        theAnimation.m_SubPropertyIndex),
            newAnim));
    }
    struct PropertyNameSorter
    {
        bool operator()(ComposerPropertyNames::Enum lhs, ComposerPropertyNames::Enum rhs) const
        {
            if (lhs == rhs)
                return false;
            if (lhs == ComposerPropertyNames::name)
                return true;
            if (rhs == ComposerPropertyNames::name)
                return false;
            return wcscmp(ComposerPropertyNames::Convert(lhs), ComposerPropertyNames::Convert(rhs))
                < 0;
        }
    };

    void Serialize(IDOMWriter &writer, Instance &inInstance, MemoryBuffer<RawAllocator> &inTempBuf)
    {
        if (inInstance.m_Valid == false)
            return;
        IDOMWriter::Scope __instanceScope(writer, ComposerObjectTypes::Convert(inInstance.m_Type));
        writer.Att(L"id", inInstance.m_Id);
        // Write properties, then write animations
        ImportArray<ComposerPropertyNames::Enum> theNames;
        for (ImportHashMap<ComposerPropertyNames::Enum, SInternValue>::iterator
                 iter = inInstance.m_PropertyValues.begin(),
                 end = inInstance.m_PropertyValues.end();
             iter != end; ++iter) {
            theNames.push_back(iter->first);
        }
        std::sort(theNames.begin(), theNames.end(), PropertyNameSorter());

        for (QT3DSU32 nameIdx = 0, nameEnd = (QT3DSU32)theNames.size(); nameIdx < nameEnd; ++nameIdx) {
            ImportHashMap<ComposerPropertyNames::Enum, SInternValue>::const_iterator iter(
                inInstance.m_PropertyValues.find(theNames[nameIdx]));
            inTempBuf.clear();
            WCharTWriter bufWriter(inTempBuf);
            WStrOps<SValue>().ToBuf(iter->second.GetValue(), bufWriter);
            if (inTempBuf.size()) {
                inTempBuf.writeZeros(sizeof(wchar_t));
                writer.Att(ComposerPropertyNames::Convert(iter->first),
                           (const wchar_t *)inTempBuf.begin());
            }
        }
        for (ImportHashMap<AnimationId, Animation *>::const_iterator iter = m_Animations.begin(),
                                                                     end = m_Animations.end();
             iter != end; ++iter) {
            if (iter->first.m_Instance == toHdl(&inInstance))
                Serialize(writer, inInstance.m_Type, *iter->second, inTempBuf);
        }
        for (QT3DSU32 childIdx = 0, childEnd = (QT3DSU32)inInstance.m_Children.size();
             childIdx < childEnd; ++childIdx) {
            Instance *childInstance = GetInstance(inInstance.m_Children[childIdx]);
            if (childInstance)
                Serialize(writer, *childInstance, inTempBuf);
        }
    }
    void Serialize(IDOMReader &reader, Instance &inInstance, MemoryBuffer<RawAllocator> &inTempBuf,
                   MemoryBuffer<RawAllocator> &inAttributeBuffer)
    {
        inInstance.m_Type = ComposerObjectTypes::Convert(reader.GetElementName());
        reader.Att(L"id", inInstance.m_Id);
        SImportComposerTypes theTypes;
        SImportAsset &theAsset(theTypes.GetImportAssetForType(inInstance.m_Type));
        m_ValidInstances.insert(inInstance.m_Handle);
        m_IdToInstMap.insert(eastl::make_pair(inInstance.m_Id, &inInstance));
        for (eastl::pair<const char8_t *, const char8_t *> att = reader.GetNarrowFirstAttribute();
             !IsTrivial(att.first); att = reader.GetNarrowNextAttribute()) {
            if (AreEqual("id", att.first))
                continue;
            DataModelDataType::Value thePropertyType = theAsset.GetPropertyDataType(att.first);
            if (thePropertyType != DataModelDataType::None) {
                if (thePropertyType == DataModelDataType::Long4)
                    thePropertyType = DataModelDataType::StringRef;
                inAttributeBuffer.clear();
                inAttributeBuffer.write(att.second, (QT3DSU32)strlen(att.second) + 1);
                inAttributeBuffer.write((QT3DSU16)0);
                WCharTReader theReader((char8_t *)inAttributeBuffer.begin(), inTempBuf,
                                       m_StringTable);
                SValue theValue = WStrOps<SValue>().BufTo(thePropertyType, theReader);
                inInstance.m_PropertyValues.insert(
                    eastl::make_pair(ComposerPropertyNames::Convert(att.first),
                                     SInternValue(theValue, m_StringTable)));
            }
        }
        {
            IDOMReader::Scope animScope(reader);
            for (bool success = reader.MoveToFirstChild(); success;
                 success = reader.MoveToNextSibling()) {
                if (AreEqual(reader.GetElementName(), L"AnimationTrack")) {
                    SerializeAnimation(reader, inInstance.m_Id, inTempBuf, inAttributeBuffer);
                } else {
                    Instance *newInstance = new Instance();
                    Serialize(reader, *newInstance, inTempBuf, inAttributeBuffer);
                    inInstance.AddChild(newInstance);
                }
            }
        }
    }

    void Serialize(IDOMWriter &inWriter, const wchar_t *inElemName, TStrToStrMap &inHashMap) const
    {
        for (TStrToStrMap::iterator iter = inHashMap.begin(), end = inHashMap.end(); iter != end;
             ++iter) {
            IDOMWriter::Scope __elemScope(inWriter, inElemName);
            inWriter.ChildValue(L"Source", iter->first);
            inWriter.ChildValue(L"Dest", iter->second);
        }
    }

    void Serialize(IDOMReader &inReader, const wchar_t *inElemName, TStrToStrMap &inHashMap)
    {
        IDOMReader::Scope itemScope(inReader);
        for (bool success = inReader.MoveToFirstChild(inElemName); success;
             success = inReader.MoveToNextSibling(inElemName)) {
            const char8_t *source, *dest;
            inReader.ChildValue("Source", source);
            inReader.ChildValue("Dest", dest);
            inHashMap.insert(
                eastl::make_pair(m_StringTable.GetWideStr(source), m_StringTable.GetWideStr(dest)));
        }
    }

    void Serialize(IDOMWriter &inWriter, const wchar_t *inElemName, TPathToMeshMap &inHashMap) const
    {
        for (TPathToMeshMap::iterator iter = inHashMap.begin(), end = inHashMap.end(); iter != end;
             ++iter) {
            IDOMWriter::Scope __elemScope(inWriter, inElemName);
            inWriter.ChildValue(L"Source", iter->first);
            inWriter.ChildValue(L"Dest", iter->second.m_FilePath);
        }
    }

    void Serialize(IDOMReader &inReader, const wchar_t *inElemName, TPathToMeshMap &inHashMap)
    {
        IDOMReader::Scope itemScope(inReader);
        for (bool success = inReader.MoveToFirstChild(inElemName); success;
             success = inReader.MoveToNextSibling(inElemName)) {
            const char8_t *source, *dest;
            inReader.ChildValue("Source", source);
            inReader.ChildValue("Dest", dest);
            MeshEntry theEntry(m_StringTable.GetWideStr(source), m_StringTable.GetWideStr(dest),
                               m_StringTable);
            inHashMap.insert(eastl::make_pair(theEntry.m_SourceId, theEntry));
        }
    }

    QT3DSU32 Save(TCharPtr fname) const override
    {
        using namespace Q3DStudio;
        QT3DSU32 theRevisionId = 1;
        CFilePath fullPath = CFilePath::CombineBaseAndRelative(m_DestDirectory, CString(fname));

        std::shared_ptr<IDOMFactory> factory(IDOMFactory::CreateDOMFactory(m_StringTablePtr));
        std::shared_ptr<IDOMWriter> theWriter;
        SDOMElement *theTopElement = nullptr;
        bool exists = fullPath.Exists();

        {
            if (exists) {
                CFileSeekableIOStream stream(fullPath.toCString(), FileReadFlags());

                // OK, ensure we can open this file in append mode.
                // This is kind of tricky because we need to write the data to the file
                // after we read it.
                if (stream.IsOpen() == false) {
                    QT3DS_ASSERT(false);
                    return 0;
                }
                stream.SetPosition(0, SeekPosition::Begin);

                theTopElement = CDOMSerializer::Read(*factory, stream);

                if (theTopElement == nullptr) {
                    QT3DS_ASSERT(false);
                    return 0;
                }
                eastl::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>>
                    theDomAccess(
                        IDOMWriter::CreateDOMWriter(factory, *theTopElement, m_StringTablePtr));
                theWriter = theDomAccess.first;
                theRevisionId = FindHighestRevisionInDocument(*theDomAccess.second) + 1;
            } else {
                theTopElement = factory->NextElement(L"UIP");
                theWriter =
                    IDOMWriter::CreateDOMWriter(factory, *theTopElement, m_StringTablePtr).first;
                // Should be GetUIP file version
                theWriter->Att(L"Version", Import::GetImportFileVersion());
            }

            QT3DS_ASSERT(theTopElement);
            QT3DS_ASSERT(theWriter.get());

            IDOMWriter &writer(*theWriter);

            IDOMWriter::Scope __docScope(writer, L"Project");
            writer.Att(L"Revision", theRevisionId);

            MemoryBuffer<RawAllocator> tempBuf;
            ImportArray<Instance *> rootList;
            for (ImportHashMap<TCharPtr, Instance *>::const_iterator iter = m_IdToInstMap.begin(),
                                                                     end = m_IdToInstMap.end();
                 iter != end; ++iter) {
                if (iter->second->m_Parent == 0)
                    rootList.push_back(iter->second);
            }
            {
                IDOMWriter::Scope __graphScope(writer, L"Graph");
                for (QT3DSU32 idx = 0, end = (QT3DSU32)rootList.size(); idx < end; ++idx)
                    const_cast<ImportImpl &>(*this).Serialize(writer, *rootList[idx], tempBuf);
            }
            {
                IDOMWriter::Scope __importScope(writer, L"Import");
                CFilePath theDirectory(fullPath.GetDirectory());
                CFilePath theSrcFile = m_SrcFile;
                if (theSrcFile.IsAbsolute())
                    theSrcFile = CFilePath::GetRelativePathFromBase(theDirectory, m_SrcFile);

                CString src = theSrcFile.toCString();
                writer.Att(L"SrcFile", src.c_str());
                writer.Att(L"ImageDir", L"maps");
                writer.Att(L"MeshDir", L"meshes");
                Serialize(writer, L"Image", const_cast<TStrToStrMap &>(m_Images));
                Serialize(writer, L"Mesh", const_cast<TPathToMeshMap &>(m_Meshes));
            }
        }
        {
            CFileSeekableIOStream stream(fullPath.toCString(), FileWriteFlags());
            stream.SetPosition(0, SeekPosition::Begin);
            CDOMSerializer::WriteXMLHeader(stream);
            CDOMSerializer::Write(*theTopElement, stream);
        }

        return theRevisionId;
    }

    bool Load(const Q3DStudio::CString &fname, QT3DSU32 inDocumentId)
    {
        using namespace Q3DStudio;
        std::shared_ptr<IDOMFactory> factory(IDOMFactory::CreateDOMFactory(m_StringTablePtr));
        SDOMElement *topElement = nullptr;
        {
            Qt3DSFileToolsSeekableMeshBufIOStream stream(
                SFile::Wrap(SFile::OpenForRead(fname), fname));
            if (stream.IsOpen() == false) {
                QT3DS_ASSERT(false);
                return false;
            }

            topElement = CDOMSerializer::Read(*factory, stream);
        }

        if (topElement == nullptr) {
            QT3DS_ASSERT(false);
            return false;
        }
        std::shared_ptr<IDOMReader> readerPtr(
            IDOMReader::CreateDOMReader(*topElement, m_StringTablePtr));
        IDOMReader &reader(*readerPtr);

        QT3DSU32 fileVersion = (QT3DSU32)-1;
        reader.Att("Version", fileVersion);

        if (inDocumentId == 0)
            inDocumentId = FindHighestRevisionInDocument(reader);

        bool success = false;
        for (success = reader.MoveToFirstChild(L"Project"); success;
             success = reader.MoveToNextSibling(L"Project")) {
            QT3DSU32 temp;
            if (reader.Att("Revision", temp) && temp == inDocumentId)
                break;
        }
        if (success == false) {
            QT3DS_ASSERT(false);
            return false;
        }

        const wchar_t *srcFile, *imagesDir, *meshDir;

        if (fileVersion > Import::GetImportFileVersion()) {
            QT3DS_ASSERT(false);
            return false;
        }
        MemoryBuffer<RawAllocator> attBuf;
        MemoryBuffer<RawAllocator> tempBuf;
        {
            IDOMReader::Scope __graphScope(reader);
            if (reader.MoveToFirstChild(L"Graph")) {

                for (bool success = reader.MoveToFirstChild(); success;
                     success = reader.MoveToNextSibling()) {
                    Instance *newInstance = new Instance();
                    Serialize(reader, *newInstance, tempBuf, attBuf);
                }
            }
        }
        {
            IDOMReader::Scope __importScope(reader);
            if (reader.MoveToFirstChild(L"Import")) {
                reader.Att(L"SrcFile", srcFile);
                reader.Att(L"ImageDir", imagesDir);
                reader.Att(L"MeshDir", meshDir);
                // We ignore, however, srcpath.
                // because it may be different.
                m_SrcFile = CString(srcFile);
                m_SrcDirectory = m_SrcFile.GetDirectory();
                if (m_SrcDirectory.isRelative()) {
                    m_FullSrcDirectory =
                        CFilePath::CombineBaseAndRelative(m_DestDirectory, m_SrcDirectory);
                } else {
                    m_FullSrcDirectory = m_SrcDirectory;
                }
                m_ImageDir = CFilePath::CombineBaseAndRelative(m_DestDirectory, CString(imagesDir));
                m_MeshDir = CFilePath::CombineBaseAndRelative(m_DestDirectory, CString(meshDir));
                Serialize(reader, L"Image", m_Images);
                Serialize(reader, L"Mesh", m_Meshes);
            }
        }
        return true;
    }
};

class RefreshImpl : public Import
{
    mutable ImportReportImpl m_ImportReport;
    ImportImpl m_Import;
    Import &m_Source;
    mutable MemoryBuffer<RawAllocator> m_TempBuffer;

public:
    RefreshImpl(ImportImpl &src, const Q3DStudio::CString &srcDirectory)
        : m_Import(src.m_StringTablePtr, srcDirectory, src.m_DestDirectory, src.m_ImageDir.toCString(),
                   src.m_MeshDir.toCString())
        , m_Source(src)
    {
    }
    virtual ~RefreshImpl() { m_Source.Release(); }

    // Returns the import interface you should use to import again from the new source.
    Import &GetImportInterface() { return *this; }

    // Implement the import interface...

    TCharPtr RegisterStr(TCharPtr str) override { return m_Import.RegisterStr(str); }
    QString GetSrcFile() const override { return m_Import.GetSrcFile(); }
    QString GetDestDir() const override { return m_Import.GetDestDir(); }
    QString GetImageDir() const override { return m_Import.GetImageDir(); }
    QString GetMeshDir() const override { return m_Import.GetMeshDir(); }
    QString GetPathBufferDir() const override { return m_Import.GetPathBufferDir(); }
    QT3DSU32 Save(TCharPtr fname) const override { return m_Import.Save(fname); }

    // Add a mapping from an named id to a handle
    Option<InstanceDesc> GetInstanceByHandle(TIMPHandle inst) const override
    {
        return m_Import.GetInstanceByHandle(inst);
    }
    Option<InstanceDesc> FindInstanceById(TCharPtr inst) const override
    {
        return m_Import.FindInstanceById(inst);
    }
    Option<InstanceDesc> FindAnyInstanceById(TCharPtr inst) const override
    {
        return m_Import.FindAnyInstanceById(inst);
    }
    QT3DSU32 GetNumInstances() const override { return m_Import.GetNumInstances(); }
    QT3DSU32 GetInstances(NVDataRef<InstanceDesc> outDescs) const override
    {
        return m_Import.GetInstances(outDescs);
    }
    QT3DSU32 GetNumProperties(TIMPHandle instance) const override
    {
        return m_Import.GetNumProperties(instance);
    }
    QT3DSU32 GetProperties(TIMPHandle inst, NVDataRef<PropertyValue> outBuffer) const override
    {
        return m_Import.GetProperties(inst, outBuffer);
    }
    Option<SValue> GetInstancePropertyValue(TIMPHandle inst,
                                                    ComposerPropertyNames::Enum val) const override
    {
        return m_Import.GetInstancePropertyValue(inst, val);
    }
    QT3DSU32 GetNumChildren(TIMPHandle instance) const override
    {
        return m_Import.GetNumChildren(instance);
    }
    QT3DSU32 GetChildren(TIMPHandle instance, NVDataRef<InstanceDesc> childBuffer) const override
    {
        return m_Import.GetChildren(instance, childBuffer);
    }

    // Carry user id's across.
    TIMPHandle CreateInstance(TCharPtr name, ComposerObjectTypes::Enum inType) override
    {
        TIMPHandle retval = m_Import.CreateInstance(name, inType);
        Option<InstanceDesc> srcInst = m_Source.FindInstanceById(name);
        if (srcInst.hasValue()) {
            if (srcInst->m_Type != inType) {
                Instance *inst = fromHdl(retval);
                m_ImportReport.m_TypeChanges.push_back(
                    Pair<InstanceDesc, ComposerObjectTypes::Enum>(*inst, srcInst->m_Type));
            }
        }
        return retval;
    }

    TIMPHandle CopyInstance(TIMPHandle inSource) override { return m_Import.CopyInstance(inSource); }
    bool SetInstanceProperties(TIMPHandle inst, NVConstDataRef<PropertyValue> inBuffer) override
    {
        return m_Import.SetInstanceProperties(inst, inBuffer);
    }
    bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            const TImportModelValue &val) override
    {
        return m_Import.DoSetInstancePropertyValue(inst, pname, val);
    }
    bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            TCharPtr val) override
    {
        return m_Import.DoSetInstancePropertyValue(inst, pname, val);
    }
    bool AddChild(TIMPHandle inst, TIMPHandle child) override
    {
        return m_Import.AddChild(inst, child);
    }
    void MarkInstanceInvalid(TIMPHandle inst) override { m_Import.MarkInstanceInvalid(inst); }

    QT3DSU32 GetNumImages() const override { return m_Import.GetNumImages(); }
    QT3DSU32 GetImages(NVDataRef<Pair<TCharPtr, TCharPtr>> imgPaths) const override
    {
        return m_Import.GetImages(imgPaths);
    }
    // Copies the an appropriate location in our import directory
    // Returns the path of the added image.  This may mangle the name slightly
    // In case of a conflict.
    CharPtrOrError AddImage(TCharPtr _imgPath) override
    {
        Option<TCharPtr> added = m_Source.FindImageByPath(_imgPath);
        return m_Import.AddOrReplaceImage(_imgPath, added);
    }
    Option<TCharPtr> FindImageByPath(TCharPtr imgPath) const override
    {
        return m_Import.FindImageByPath(imgPath);
    }

    Option<TCharPtr> FindImageByRelativePath(TCharPtr imgPath) const override
    {
        return m_Import.FindImageByRelativePath(imgPath);
    }

    QT3DSU32 GetNumMeshes() const override { return m_Import.GetNumMeshes(); }
    QT3DSU32 GetMeshes(NVDataRef<Pair<TCharPtr, TCharPtr>> bufferPaths) const override
    {
        return m_Import.GetMeshes(bufferPaths);
    }
    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    // Mesh name is used to write out a reasonable buffer *and* on refresh to know
    // if a buffer is changed/updated or not
    CharPtrOrError AddMesh(const Mesh &meshBuffer, TCharPtr meshName) override
    {
        Q3DStudio::CString safeName = CFilePath::MakeSafeFileStem(meshName);
        return m_Import.AddOrReplaceMesh(meshBuffer, meshName,
                                         m_Source.FindMeshFilePathByName(safeName.c_str()));
    }

    bool HasMesh(TCharPtr meshName) const override { return m_Import.HasMesh(meshName); }

    Option<TCharPtr> FindMeshReferencePathByName(TCharPtr meshName) const override
    {
        return m_Import.FindMeshReferencePathByName(meshName);
    }

    Option<TCharPtr> FindMeshFilePathByName(TCharPtr meshName) const override
    {
        return m_Import.FindMeshFilePathByName(meshName);
    }

    QT3DSU32 GetNumPathBuffers() const override { return m_Import.GetNumPathBuffers(); }
    QT3DSU32 GetPathBuffers(NVDataRef<Pair<TCharPtr, TCharPtr>> pathBufferPaths) const override
    {
        return m_Import.GetPathBuffers(pathBufferPaths);
    }
    // Copies the an appropriate location in our import directory
    // Returns the path of the added PathBuffer.  This may mangle the name slightly
    // In case of a conflict.
    CharPtrOrError AddPathBuffer(const SPathBuffer &pathBuffer, TCharPtr pathName) override
    {
        Option<TCharPtr> added = m_Source.FindPathBufferByPath(pathName);
        return m_Import.AddOrReplacePathBuffer(pathBuffer, pathName, added);
    }

    Option<TCharPtr> FindPathBufferByPath(TCharPtr pathBufferPath) const override
    {
        return m_Import.FindPathBufferByPath(pathBufferPath);
    }

    Option<TCharPtr> FindPathBufferByRelativePath(TCharPtr pathBufferPath) const override
    {
        return m_Import.FindPathBufferByRelativePath(pathBufferPath);
    }

    QT3DSU32 GetNumAnimations() const override { return m_Import.GetNumAnimations(); }
    QT3DSU32 GetAnimations(NVDataRef<Animation> outBuffers) const override
    {
        return m_Import.GetAnimations(outBuffers);
    }
    // Data is copied into this object, you can release the anim buffer data after this
    void DoAddAnimation(TCharPtr instance, TCharPtr propName, QT3DSU32 subPropIndex,
                                EAnimationType type, NVConstDataRef<QT3DSF32> values) override
    {
        m_Import.DoAddAnimation(instance, propName, subPropIndex, type, values);
    }
    Option<Animation> FindAnimation(TCharPtr instance, TCharPtr propName,
                                            QT3DSU32 subPropIndex) const override
    {
        return m_Import.FindAnimation(instance, propName, subPropIndex);
    }

    template <typename TDataType>
    NVDataRef<TDataType> TempAlloc(QT3DSU32 numItems) const
    {
        m_TempBuffer.reserve(numItems * sizeof(TDataType));
        return toDataRef((TDataType *)m_TempBuffer.begin(), numItems);
    }

    template <typename TDataType, typename TDiffOp>
    void CompileItemReport(AddRemoveImpl<TDataType> &result, TDiffOp op) const
    {
        QT3DSU32 numItems = op.NumItems(m_Import);
        NVDataRef<TDataType> tempData(TempAlloc<TDataType>(numItems));
        op.GetItems(m_Import, tempData);
        QT3DSIMP_FOREACH(idx, numItems)
        {
            const TDataType &newDesc(tempData[idx]);
            if (op.HasItem(m_Source, newDesc))
                result.m_Existing.push_back(newDesc);
            else
                result.m_Added.push_back(newDesc);
        }
        numItems = op.NumItems(m_Source);
        tempData = TempAlloc<TDataType>(numItems);
        op.GetItems(m_Source, tempData);
        QT3DSIMP_FOREACH(idx, numItems)
        {
            const TDataType &oldDesc(tempData[idx]);
            if (op.HasItem(m_Import, oldDesc) == false)
                result.m_Removed.push_back(oldDesc);
        }
    }

    ImportReport CompileReport() const override
    {
        m_ImportReport.clear();
        // Find existint, added, and removed items
        CompileItemReport(m_ImportReport.m_Instances, InstanceDiffOp());
        CompileItemReport(m_ImportReport.m_Images, ImageDiffOp());
        CompileItemReport(m_ImportReport.m_Meshes, MeshDiffOp());
        CompileItemReport(m_ImportReport.m_PathBuffers, PathBufferDiffOp());
        CompileItemReport(m_ImportReport.m_Animations, AnimationDiffOp());

        // OK, prepare parent child links

        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Removed.size())
        {
            const InstanceDesc &oldDesc(m_ImportReport.m_Instances.m_Removed[idx]);
            QT3DSU32 numChildren = m_Source.GetNumChildren(oldDesc.m_Handle);
            NVDataRef<InstanceDesc> tempData(TempAlloc<InstanceDesc>(numChildren));
            m_Source.GetChildren(oldDesc.m_Handle, tempData);
            QT3DSIMP_FOREACH(chld, numChildren)
            m_ImportReport.m_Links.m_Removed.push_back(
                ParentChildLink(oldDesc.m_Id, tempData[chld].m_Id, L""));
        }
        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Added.size())
        {
            const InstanceDesc &newDesc(m_ImportReport.m_Instances.m_Added[idx]);
            QT3DSU32 numChildren = GetNumChildren(newDesc.m_Handle);
            NVDataRef<InstanceDesc> tempData(TempAlloc<InstanceDesc>(numChildren));
            m_Import.GetChildren(newDesc.m_Handle, tempData);
            QT3DSIMP_FOREACH(chldIdx, numChildren)
            {
                // Run through children in reverse order
                // so that we know the sibling pointer is good.
                QT3DSU32 chld = numChildren - chldIdx - 1;
                TCharPtr child = tempData[chld].m_Id;
                // If the chlid existed in the original, then it also needs to be added here.
                // Because that means the a new object was added and an original object was
                // re-attached.
                if (m_Source.FindInstanceById(child).hasValue()) {
                    TCharPtr sibling = chld + 1 < numChildren ? tempData[chld + 1].m_Id : L"";
                    m_ImportReport.m_Links.m_Added.push_back(
                        ParentChildLink(newDesc.m_Id, child, sibling));
                }
            }
        }
        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Existing.size())
        {
            const InstanceDesc &newDesc(m_ImportReport.m_Instances.m_Existing[idx]);
            const InstanceDesc &oldDesc(m_Source.FindInstanceById(newDesc.m_Id));
            QT3DSU32 numNewChildren(GetNumChildren(newDesc.m_Handle));
            QT3DSU32 numOldChildren(m_Source.GetNumChildren(oldDesc.m_Handle));
            NVDataRef<InstanceDesc> tempData(
                TempAlloc<InstanceDesc>(numNewChildren + numOldChildren));
            NVDataRef<InstanceDesc> newChildren(tempData.begin(), numNewChildren);
            NVDataRef<InstanceDesc> oldChildren(tempData.begin() + numNewChildren, numOldChildren);
            m_Import.GetChildren(newDesc.m_Handle, newChildren);
            m_Source.GetChildren(oldDesc.m_Handle, oldChildren);
            QT3DSIMP_FOREACH(childIdx, numNewChildren)
            {
                // Run through the links in reverse order so we know the sibling pointer is good.
                // This is necessary for addChild.
                QT3DSU32 child = numNewChildren - childIdx - 1;
                const InstanceDesc &childDesc = newChildren[child];
                TCharPtr sibling = child + 1 < numNewChildren ? newChildren[child + 1].m_Id : L"";
                ParentChildLink link(newDesc.m_Id, childDesc.m_Id, sibling);
                if (FindChild(m_Source.FindInstanceById(childDesc.m_Id), sibling, childDesc.m_Type,
                              oldChildren))
                    m_ImportReport.m_Links.m_Existing.push_back(link);
                else
                    m_ImportReport.m_Links.m_Added.push_back(link);
            }
            QT3DSIMP_FOREACH(child, oldChildren.size())
            {
                const InstanceDesc &childDesc = oldChildren[child];
                TCharPtr sibling =
                    child + 1 < oldChildren.size() ? oldChildren[child + 1].m_Id : L"";
                if (FindChild(m_Import.FindInstanceById(childDesc.m_Id), sibling, childDesc.m_Type,
                              newChildren)
                    == false)
                    m_ImportReport.m_Links.m_Removed.push_back(
                        ParentChildLink(oldDesc.m_Id, childDesc.m_Id, L""));
            }
        }
        m_Import.FinalizeReport();

        return m_ImportReport;
    }

    void Release() override { delete this; }
};

}

ImportPtrOrError Import::Create(TCharPtr _srcPath, TCharPtr _destDir)
{
    CFilePath srcPath(CFilePath::GetAbsolutePath(_srcPath));
    CFilePath destDir(CFilePath::GetAbsolutePath(_destDir));

    if (!srcPath.IsFile())
        return ImportErrorData(ImportErrorCodes::SourceFileDoesNotExist, srcPath.toCString());

    if (!destDir.CreateDir(true))
        return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, destDir.toCString());

    TStringTablePtr strTable = qt3dsdm::IStringTable::CreateStringTable();
    return new ImportImpl(strTable, srcPath, destDir, L"maps", L"meshes");
}

ImportPtrOrError Import::Load(TCharPtr pathToFile, QT3DSU32 inImportVersion)
{
    CFilePath fullFilePath(CFilePath::GetAbsolutePath(pathToFile));
    CFilePath destDir(fullFilePath.GetDirectory());

    if (fullFilePath.IsFile() == false)
        return ImportErrorData(ImportErrorCodes::SourceFileNotReadable, fullFilePath.toCString());

    TStringTablePtr strTable = qt3dsdm::IStringTable::CreateStringTable();
    ImportImpl *impl = new ImportImpl(strTable, Q3DStudio::CString(), destDir);
    bool success = impl->Load(fullFilePath, inImportVersion);
    QT3DS_ASSERT(success);
    if (success == false) {
        impl->Release();
        return ImportErrorData(ImportErrorCodes::SourceFileNotReadable, fullFilePath.toCString());
    }
    return impl;
}

ImportPtrOrError Import::CreateForRefresh(Import &original, TCharPtr _srcPath)
{
    CFilePath srcPath(CFilePath::GetAbsolutePath(_srcPath));

    if (!srcPath.IsFile()) {
        original.Release();
        return ImportErrorData(ImportErrorCodes::SourceFileDoesNotExist, srcPath.toCString());
    }

    RefreshImpl *refresh(new RefreshImpl(static_cast<ImportImpl &>(original), srcPath));
    return &refresh->GetImportInterface();
}

QT3DSU32 Import::GetHighestImportRevision(TCharPtr pathToFile)
{
    using std::shared_ptr;
    CFilePath fullFilePath(CFilePath::GetAbsolutePath(pathToFile));
    CFileSeekableIOStream stream(fullFilePath.toCString(), FileReadFlags());
    if (stream.IsOpen() == false) {
        QT3DS_ASSERT(false);
        return 0;
    }

    std::shared_ptr<qt3dsdm::IStringTable> theStringTable =
        qt3dsdm::IStringTable::CreateStringTable();
    std::shared_ptr<IDOMFactory> theFactory = IDOMFactory::CreateDOMFactory(theStringTable);
    SDOMElement *theTopElement = CDOMSerializer::Read(*theFactory, stream);

    if (theTopElement == nullptr) {
        QT3DS_ASSERT(false);
        return 0;
    }

    std::shared_ptr<IDOMReader> theReader =
        IDOMReader::CreateDOMReader(*theTopElement, theStringTable);
    return ImportImpl::FindHighestRevisionInDocument(*theReader);
}
