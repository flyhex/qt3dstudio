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
#include "foundation/StrConvertUTF.h"

using qt3dsdm::IStringTable;

using namespace qt3dsimp;

namespace qt3dsimp {

bool operator<(const AnimationId &a, const AnimationId &o)
{
    if (a.m_Instance < o.m_Instance)
        return true;
    if (a.m_Property < o.m_Property)
        return true;
    if (a.m_SubPropIndex < o.m_SubPropIndex)
        return true;
    return false;
}

uint qHash(const AnimationId &aid, uint seed)
{
    QtPrivate::QHashCombine hash;
    seed = hash(aid.m_Instance, seed);
    seed = qHash(aid.m_Property, seed);
    seed = hash(aid.m_SubPropIndex, seed);
    return seed;
}

bool operator==(const AnimationId &a, const AnimationId &b)
{
    return a.m_Instance == b.m_Instance
            && a.m_Property == b.m_Property
            && a.m_SubPropIndex == b.m_SubPropIndex;
}

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
    QVector<TDataType> m_Existing;
    QVector<TDataType> m_Added;
    QVector<TDataType> m_Removed;

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
    QVector<std::pair<InstanceDesc, ComposerObjectTypes::Enum>> m_TypeChanges;
    AddRemoveImpl<std::pair<QString, QString>> m_Images;
    AddRemoveImpl<std::pair<QString, QString>> m_Meshes;
    AddRemoveImpl<std::pair<QString, QString>> m_PathBuffers;
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
    void GetItems(const Import &src, QVector<InstanceDesc> &data) { src.GetInstances(data); }
    // ID's *and* types have to match in order to get valid output
    bool HasItem(const Import &src, const InstanceDesc &data)
    {
        Option<InstanceDesc> result(src.FindInstanceById(data.m_Id));
        return result.hasValue() && result->m_Type == data.m_Type;
    }
};

bool FindChild(Option<InstanceDesc> hdlOpt, const QString &inCompareNextSibling,
               ComposerObjectTypes::Enum childType, const QVector<InstanceDesc> &children)
{
    if (hdlOpt.hasValue() == false)
        return false;
    QString id = hdlOpt->m_Id;
    QT3DSIMP_FOREACH(idx, children.size())
    {
        QString nextSibling = ((idx + 1) < children.size()) ? children[idx + 1].m_Id : QString();
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
    void GetItems(const Import &src, QVector<std::pair<QString, QString>> &data)
    {
        src.GetImages(data);
    }
    bool HasItem(const Import &src, const std::pair<QString, QString> &data)
    {
        return src.FindImageByRelativePath(data.first).hasValue();
    }
};

struct MeshDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumMeshes(); }
    void GetItems(const Import &src, QVector<std::pair<QString, QString>> &data)
    {
        src.GetMeshes(data);
    }
    bool HasItem(const Import &src, const std::pair<QString, QString> &data)
    {
        return src.HasMesh(data.first);
    }
};
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
struct PathBufferDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumPathBuffers(); }
    void GetItems(const Import &src, QVector<std::pair<QString, QString>> &data)
    {
        src.GetPathBuffers(data);
    }
    bool HasItem(const Import &src, const std::pair<QString, QString> &data)
    {
        return src.FindPathBufferByRelativePath(data.first).hasValue();
    }
};
#endif
struct AnimationDiffOp
{
    QT3DSU32 NumItems(const Import &src) { return src.GetNumAnimations(); }
    void GetItems(const Import &src, QVector<Animation> &data) { src.GetAnimations(data); }
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
    QString m_SourceId;
    QString m_FilePath;
    QString m_ReferencePath; // file path plus revision
    QT3DSU32 m_revision;
    MeshEntry(const QString &inSourceId, const QString &inPath, QT3DSU32 inRevision)
        : m_SourceId(inSourceId)
        , m_FilePath(inPath)
        , m_ReferencePath(inPath)
        , m_revision(inRevision)
    {
        m_ReferencePath.append(QString::number(m_revision));
    }

    MeshEntry(const QString &inSourceId, const QString &inPath)
        : m_SourceId(inSourceId)
        , m_FilePath(inPath)
        , m_revision(0)
    {
    }
};

class ImportImpl : public Import
{
public:
    typedef QHash<QString, Instance *> TIdToInstanceMap;
    typedef QHash<QString, QString> TStrToStrMap;
    typedef QHash<QString, MeshEntry> TPathToMeshMap;

    TStringTablePtr m_StringTablePtr;
    qt3dsdm::IStringTable &m_StringTable;
    QSet<TIMPHandle> m_ValidInstances;
    QSet<TIMPHandle> m_InValidInstances;
    TIdToInstanceMap m_IdToInstMap;
    // Mapping from original ID to new location
    TStrToStrMap m_Images;
    TStrToStrMap m_PathBuffers;
    // Mapping from mesh name to new location
    TPathToMeshMap m_Meshes;
    QHash<AnimationId, Animation *> m_Animations;

    QString m_DestDirectory;
    QString m_SrcDirectory;
    QString m_SrcFile;
    QString m_FullSrcDirectory;
    QString m_ImageDir;
    QString m_MeshDir;
    QString m_PathBufferDir;
    mutable ImportReportImpl m_ImportReport;

    ~ImportImpl() override
    {
        for (auto instance : m_ValidInstances)
            delete fromHdl(instance);
        for (auto animation : m_Animations)
            delete animation;
        m_ValidInstances.clear();
        m_Animations.clear();
        m_InValidInstances.clear();
    }

    ImportImpl(TStringTablePtr strTable, const QString &srcFile,
               const QString &destDirectory, const QString &imagesDir = {},
               const QString &meshesDir = {})
        : m_StringTablePtr(strTable)
        , m_StringTable(*strTable.get())
        , m_DestDirectory(destDirectory)
    {
        if (!srcFile.isEmpty()) {
            QFileInfo info(srcFile);
            m_FullSrcDirectory = info.canonicalPath();
            m_SrcFile = CFilePath::GetRelativePathFromBase(destDirectory, srcFile);
            m_SrcDirectory = QFileInfo(m_SrcFile).canonicalPath();
        }
        // On load, images and meshes directories will be trivial.
        if (!imagesDir.isEmpty()) {
            m_ImageDir = imagesDir;
            if (QFileInfo(m_ImageDir).isRelative())
                m_ImageDir = CFilePath::CombineBaseAndRelative(destDirectory, imagesDir);
        }
        if (!meshesDir.isEmpty()) {
            m_MeshDir = meshesDir;
            if (QFileInfo(m_MeshDir).isRelative())
                m_MeshDir = CFilePath::CombineBaseAndRelative(destDirectory, meshesDir);
        }
        m_PathBufferDir = CFilePath::CombineBaseAndRelative(destDirectory, QStringLiteral("paths"));
    }

    QString RegisterStr(TCharPtr str) override { return QString::fromWCharArray(str); }
    QString GetSrcFile() const override { return m_SrcFile; }
    QString GetDestDir() const override { return m_DestDirectory; }
    QString GetImageDir() const override { return m_ImageDir; }
    QString GetMeshDir() const override { return m_MeshDir; }
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    QString GetPathBufferDir() const override { return m_PathBufferDir; }
#endif
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

    Instance *GetInstance(const QString &id)
    {
        QHash<QString, Instance *>::const_iterator entry
                = m_IdToInstMap.find(id);
        if (entry != m_IdToInstMap.end())
            return *entry;
        return nullptr;
    }

    Instance *GetInstance(const QString &id) const
    {
        QHash<QString, Instance *>::const_iterator entry
                = m_IdToInstMap.find(id);
        if (entry != m_IdToInstMap.end())
            return *entry;
        return nullptr;
    }

    Option<InstanceDesc> GetInstanceByHandle(TIMPHandle inst) const override
    {
        if (m_ValidInstances.contains(inst))
            return *fromHdl(inst);
        return Empty();
    }
    Option<InstanceDesc> FindInstanceById(const QString &inst) const override
    {
        Instance *retval = GetInstance(inst);
        if (retval && retval->m_Valid)
            return *retval;
        return Empty();
    }

    Option<InstanceDesc> FindAnyInstanceById(const QString &inst) const override
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

    void AddInstance(Instance *inInstance, QVector<InstanceDesc> &inAdded, QT3DSU32 &addIdx,
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

    QT3DSU32 GetInstances(QVector<InstanceDesc> &outDescs) const override
    {
        QT3DS_ASSERT(outDescs.size() >= GetNumInstances());
        QT3DSU32 numItems = qMin(QT3DSU32(outDescs.size()), GetNumInstances());
        QT3DSU32 idx = 0;

        for (auto instance : qAsConst(m_ValidInstances)) {
            if (m_InValidInstances.contains(instance))
                continue;

            Instance *theInstance = GetInstance(instance);
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
    QT3DSU32 GetProperties(TIMPHandle instance, QVector<PropertyValue> &outBuffer) const override
    {
        Instance *inst = GetInstance(instance);
        if (inst == nullptr) {
            QT3DS_ASSERT(false);
            return 0;
        }
        QT3DSU32 numItems = qMin(QT3DSU32(outBuffer.size()),
                                 QT3DSU32(inst->m_PropertyValues.size()));
        QT3DSU32 idx = 0;
        for (auto iter = inst->m_PropertyValues.begin(), end = inst->m_PropertyValues.end();
             iter != end && idx < numItems; ++iter, ++idx) {
            outBuffer[idx] = PropertyValue(iter.key(), iter.value());
        }
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
        const QHash<ComposerPropertyNames::Enum, SInternValue>::iterator entry =
            inst->m_PropertyValues.find(val);
        if (entry != inst->m_PropertyValues.end())
            return entry->GetValue();
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
    QT3DSU32 GetChildren(TIMPHandle instance, QVector<InstanceDesc> &childBuffer) const override
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
        if (*m_InValidInstances.insert(instance)) {
            inst->MarkInvalid();
            QT3DSIMP_FOREACH(idx, (QT3DSU32)inst->m_Children.size())
            {
                Instance *theInstance(GetInstance(inst->m_Children[idx]));
                if (theInstance)
                    MarkInstanceInvalid(toHdl(theInstance));
            }
            std::vector<AnimationId> animationsToErase;
            for (auto iter = m_Animations.begin(), end = m_Animations.end(); iter != end; ++iter) {
                if (iter.key().m_Instance == instance)
                    animationsToErase.push_back(iter.key());
            }
            for (auto animation : qAsConst(animationsToErase))
                m_Animations.remove(animation);
        }
    }

    TIMPHandle CreateInstance(const QString &name, ComposerObjectTypes::Enum inType) override
    {
        if (name.isEmpty() == false) {
            Option<InstanceDesc> exists = FindInstanceById(name);
            bool hasValue = exists.hasValue();
            if (hasValue) {
                QT3DS_ASSERT(false);
                return exists->m_Handle;
            }
        }
        InstanceDesc newDesc;
        newDesc.m_Id = name;
        newDesc.m_Type = inType;
        Instance *newInst = new Instance(newDesc);
        m_ValidInstances.insert(toHdl(newInst));
        m_IdToInstMap.insert(name, newInst);
        return toHdl(newInst);
    }

    Instance *CopyInstanceHierarchy(Instance &inSource, QHash<QString, QString> &inIdMap)
    {
        QString uniqueStem(inSource.m_Id);
        QString instId(uniqueStem);
        int index = 1;
        do {
            instId = QStringLiteral("%1_%2").arg(uniqueStem).arg(index);
            ++index;
        } while (FindInstanceById(instId).hasValue());

        Instance *retval = GetInstance(CreateInstance(instId, inSource.m_Type));
        inIdMap.insert(inSource.m_Id, retval->m_Id);

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
                                            QHash<QString, QString> &inIdMap)
    {
        const QHash<QString, QString>::iterator theNewItemId = inIdMap.find(inSource.m_Id);
        if (theNewItemId == inIdMap.end()) {
            QT3DS_ASSERT(false);
            return;
        }
        Instance *theCopy = GetInstance(*theNewItemId);
        if (theCopy == nullptr) {
            QT3DS_ASSERT(false);
            return;
        }

        for (auto iter = inSource.m_PropertyValues.begin(), end = inSource.m_PropertyValues.end();
             iter != end; ++iter) {
            SInternValue current(*iter);
            if (GetValueType(current.GetValue()) == DataModelDataType::StringRef) {
                const SStringRef &theRef = get<SStringRef>(current.GetValue());
                const QHash<QString, QString>::iterator theNewId
                        = inIdMap.find(QString::fromWCharArray(theRef.m_Id));
                if (theNewId != inIdMap.end()) {
                    current = SInternValue::ISwearThisHasAlreadyBeenInternalized(
                        SValue(QVariant::fromValue(*theNewId)));
                }
            }
            theCopy->m_PropertyValues.insert(iter.key(), current);
        }
        for (auto iter = m_Animations.begin(), end = m_Animations.end();  iter != end; ++iter) {
            if (iter.key().m_Instance == toHdl(&inSource)) {
                const Animation *theSrcAnimation = *iter;
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
        QHash<QString, QString> idMap;
        // copy the hierarchy first
        Instance *retval = CopyInstanceHierarchy(*inst, idMap);

        Instance *parent = fromHdl(inst->m_Parent);
        if (parent != nullptr)
            parent->AddChild(retval, inst);

        // Copy properties and animations
        CopyInstancePropertiesAndAnimation(*inst, idMap);

        return toHdl(retval);
    }

    bool SetInstanceProperties(TIMPHandle instance, const QVector<PropertyValue> &inBuffer) override
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
                                    const QString &val) override
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
    QT3DSU32 GetImages(QVector<std::pair<QString, QString>> &imgPaths) const override
    {
        QT3DSU32 numItems = qMin((QT3DSU32)imgPaths.size(), (QT3DSU32)m_Images.size());
        QT3DS_ASSERT(numItems == m_Images.size());
        QT3DSU32 idx = 0;
        for (auto iter = m_Images.begin(), end = m_Images.end();
             iter != end && idx < numItems; ++iter, ++idx)
            imgPaths[idx] = std::pair<QString, QString>(iter.key(), iter.value());
        return numItems;
    }

    bool ensureDirectoryExists(const QString &directory)
    {
        QDir dir(directory);
        dir.mkpath(QStringLiteral("."));
        return dir.exists();
    }
    // Copies the an appropriate location in our import directory
    // Returns the path of the added image.  This may mangle the name slightly
    // In case of a conflict.
    QStringOrError AddImage(const QString &_imgPath) override
    {
        Option<QString> added = FindImageByPath(_imgPath);
        if (added.hasValue())
            return *added;

        if (!ensureDirectoryExists(m_ImageDir))
            return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_ImageDir);

        QString imgPath = QFileInfo(_imgPath).absoluteFilePath();
        QString srcImgPath =
            CFilePath::GetRelativePathFromBase(m_FullSrcDirectory, imgPath);

        QString destFile;
        bool copyResult = Q3DStudio::SFileTools::FindAndCopyDestFile(QDir(m_ImageDir), imgPath,
                                                                     destFile);
        // Get the actual return value relative do our destination directory
        QString retval = CFilePath::GetRelativePathFromBase(m_DestDirectory, destFile);

        m_Images.insert(srcImgPath, retval);

        if (!copyResult) {
            QStringOrError errorValue(
                ImportErrorData(ImportErrorCodes::ResourceNotWriteable, imgPath));
            errorValue.m_Value = retval;
            return errorValue;
        }
        return retval;
    }
    QStringOrError AddOrReplaceImage(const QString &_imgPath, Option<QString> dstPath)
    {
        if (dstPath.hasValue()) {
            if (!ensureDirectoryExists(m_ImageDir))
                return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_ImageDir);

            CFilePath fullDestPath =
                CFilePath::CombineBaseAndRelative(m_DestDirectory, dstPath.getValue());
            Q3DStudio::SFileErrorCodeAndNumBytes copyResult = Q3DStudio::SFileTools::Copy(
                fullDestPath, Q3DStudio::FileOpenFlags(Q3DStudio::FileOpenFlagValues::Truncate
                                                       | Q3DStudio::FileOpenFlagValues::Open
                                                       | Q3DStudio::FileOpenFlagValues::Create),
                _imgPath);
            // Regardless of if the copy operation succeeds or not, if the destination exists
            // already
            // Then we enter it into our dictionary
            if (fullDestPath.IsFile()) {
                QString imgPath = QFileInfo(_imgPath).absoluteFilePath();
                QString srcImgPath =
                    CFilePath::GetRelativePathFromBase(m_FullSrcDirectory, imgPath);
                m_Images.insert(srcImgPath, dstPath.getValue());
            }

            QString returnPath = dstPath.getValue();

            if (copyResult.m_Error != Q3DStudio::FileErrorCodes::NoError) {
                QT3DS_ASSERT(false);
                QString extraData;
                ImportErrorCodes::Enum error = FromFileErrorCode(copyResult.m_Error);
                if (error == ImportErrorCodes::ResourceNotWriteable)
                    extraData = dstPath.getValue();
                else if (error == ImportErrorCodes::SourceFileNotReadable)
                    extraData = _imgPath;
                QStringOrError errorRet(ImportErrorData(error, extraData));
                errorRet.m_Value = returnPath;
                return errorRet;
            }
            return returnPath;
        }
        return AddImage(_imgPath);
    }
    Option<QString> FindImageByPath(const QString &_imgPath) const override
    {
        const QString imgPath(QFileInfo(_imgPath).absoluteFilePath());
        const QString srcImgPath(CFilePath::GetRelativePathFromBase(m_FullSrcDirectory, imgPath));
        return FindImageByRelativePath(srcImgPath);
    }

    Option<QString> FindImageByRelativePath(const QString &imgPath) const override
    {
        QHash<QString, QString>::const_iterator entry = m_Images.find(imgPath);
        if (entry != m_Images.end())
            return *entry;
        return Empty();
    }

    QT3DSU32 GetNumMeshes() const override { return (QT3DSU32)m_Meshes.size(); }
    QT3DSU32 GetMeshes(QVector<std::pair<QString, QString>> &bufferPaths) const override
    {
        QT3DSU32 numItems = qMin(m_Meshes.size(), bufferPaths.size());
        QT3DS_ASSERT(numItems == m_Meshes.size());
        QT3DSU32 idx = 0;
        for (auto iter = m_Meshes.begin(), end = m_Meshes.end();
             iter != end && idx < numItems; ++iter, ++idx) {
            bufferPaths[idx] = std::make_pair(iter.key(), iter->m_FilePath);
        }
        return numItems;
    }

    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    QStringOrError AddMesh(const Mesh &meshBuffer, const QString &_name) override
    {
        if (_name.isNull()) {
            QT3DS_ASSERT(false);
            return QString();
        }
        QString name = CFilePath::MakeSafeFileStem(_name);

        Option<QString> meshOpt = FindMeshReferencePathByName(name);
        if (meshOpt.hasValue())
            return *meshOpt;

        if (!ensureDirectoryExists(m_MeshDir))
            return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_MeshDir);

        const QString handf = Q3DStudio::SFileTools::FindUniqueDestFile(m_MeshDir, name,
                                                                        QStringLiteral("mesh"));
        QFile output(handf);
        output.open(QFile::WriteOnly | QFile::NewOnly);
        MallocAllocator alloc;
        QT3DSU32 meshId = meshBuffer.SaveMulti(alloc, output);
        QString _retval = CFilePath::GetRelativePathFromBase(m_DestDirectory, handf);
        MeshEntry newEntry(name, _retval, meshId);
        m_Meshes.insert(newEntry.m_SourceId, newEntry);
        return newEntry.m_ReferencePath;
    }

    QStringOrError AddOrReplaceMesh(const Mesh &meshBuffer, const QString &name,
                                    Option<QString> srcMesh)
    {
        using namespace Q3DStudio;
        if (srcMesh.hasValue()) {
            if (!ensureDirectoryExists(m_MeshDir))
                return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_MeshDir);

            const QString meshPath
                    = CFilePath::CombineBaseAndRelative(m_DestDirectory, srcMesh.getValue());

            QFile output(meshPath);
            output.open(QFile::WriteOnly | QFile::Truncate);
            if (output.isOpen() == false) {
                QT3DS_ASSERT(false);
                return ImportErrorData(ImportErrorCodes::ResourceNotWriteable, srcMesh.getValue());
            }

            MallocAllocator allocator;
            QT3DSU32 meshId = meshBuffer.SaveMulti(allocator, output);

            QString relativePath = CFilePath::GetRelativePathFromBase(m_DestDirectory, meshPath);

            MeshEntry newEntry(name, relativePath, meshId);
            m_Meshes.insert(newEntry.m_SourceId, newEntry);
            return newEntry.m_ReferencePath;
        }
        return AddMesh(meshBuffer, name);
    }

    bool HasMesh(const QString & meshName) const override
    {
        return m_Meshes.find(meshName) != m_Meshes.end();
    }

    Option<QString> FindMeshReferencePathByName(const QString &meshName) const override
    {
        QHash<QString, MeshEntry>::const_iterator entry = m_Meshes.find(meshName);
        if (entry != m_Meshes.end()) {
            QT3DS_ASSERT(!entry->m_ReferencePath.isEmpty());
            return entry->m_ReferencePath;
        }
        return Empty();
    }

    Option<QString> FindMeshFilePathByName(const QString &meshName) const override
    {
        QHash<QString, MeshEntry>::const_iterator entry = m_Meshes.find(meshName);
        if (entry != m_Meshes.end()) {
            QT3DS_ASSERT(!entry->m_FilePath.isEmpty());
            return entry->m_FilePath;
        }
        return Empty();
    }
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    QT3DSU32 GetNumPathBuffers() const override { return (QT3DSU32)m_PathBuffers.size(); }
    QT3DSU32 GetPathBuffers(QVector<std::pair<QString, QString>> &pathBufferPaths) const override
    {
        QT3DSU32 numItems = qMin((QT3DSU32)pathBufferPaths.size(), (QT3DSU32)m_PathBuffers.size());
        QT3DS_ASSERT(numItems == m_PathBuffers.size());
        QT3DSU32 idx = 0;
        for (ImportHashMap<QString, QString>::const_iterator iter = m_PathBuffers.begin(),
                                                               end = m_PathBuffers.end();
             iter != end && idx < numItems; ++iter, ++idx)
            pathBufferPaths[idx] = std::make_pair(iter->first, iter->second);
        return numItems;
    }

    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    QStringOrError AddPathBuffer(const SPathBuffer &pathBuffer, const QString &_name) override
    {
        if (IsTrivial(_name)) {
            QT3DS_ASSERT(false);
            return L"";
        }
        Q3DStudio::CString name = CFilePath::MakeSafeFileStem(_name);

        Option<QString> pathBufferOpt = FindPathBufferByRelativePath(name.c_str());
        if (pathBufferOpt.hasValue())
            return *pathBufferOpt;

        if (!m_PathBufferDir.IsDirectory())
            m_PathBufferDir.CreateDir(true);
        if (!m_PathBufferDir.IsDirectory())
            return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, m_PathBufferDir.toCString());
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
        Q3DStudio::TFilePtr handf =
            Q3DStudio::SFileTools::FindUniqueDestFile(m_PathBufferDir, name, L"path");
        Qt3DSFileToolsSeekableMeshBufIOStream output(handf);
        MallocAllocator alloc;
        pathBuffer.Save(output);
        CFilePath _retval = CFilePath::GetRelativePathFromBase(m_DestDirectory, handf->m_Path);
        const wchar_t *return_Value = m_StringTable.RegisterStr(_retval.toCString());
        m_PathBuffers.insert(
            std::make_pair(m_StringTable.RegisterStr(name.c_str()), return_Value));
#endif
        return L"";
    }

    QStringOrError AddOrReplacePathBuffer(const SPathBuffer &pathBuffer, const QString &name,
                                          Option<QString> srcPathBuffer)
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
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
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
#endif
        }
        return AddPathBuffer(pathBuffer, name);
    }

    Option<QString> FindPathBufferByPath(const QString &_pathBufferPath) const override
    {
        Q3DStudio::CString pathBufferPath = CFilePath::GetAbsolutePath(CString(_pathBufferPath));
        Q3DStudio::CString srcpathBufferPath =
            CFilePath::GetRelativePathFromBase(m_FullSrcDirectory, pathBufferPath);
        return FindPathBufferByRelativePath(srcpathBufferPath.c_str());
    }

    Option<QString> FindPathBufferByRelativePath(const QString &pathBufferPath) const override
    {
        ImportHashMap<QString, QString>::const_iterator entry =
            m_PathBuffers.find(pathBufferPath);
        if (entry != m_PathBuffers.end())
            return entry->second;
        return Empty();
    }
#endif
    QT3DSU32 GetNumAnimations() const override { return QT3DSU32(m_Animations.size()); }
    QT3DSU32 GetAnimations(QVector<Animation> &outBuffers) const override
    {
        QT3DSU32 numItems = qMin(m_Animations.size(), outBuffers.size());
        Q_ASSERT(numItems == m_Animations.size());
        std::copy(m_Animations.begin(), m_Animations.begin() + numItems, outBuffers.begin());
        return numItems;
    }

    // Data is copied into this object, you can release the anim buffer data after this
    void DoAddAnimation(const QString &instance, const QString &propName, QT3DSU32 subPropIndex,
                        EAnimationType bufType, const QVector<QT3DSF32> &values) override
    {
        Option<Animation> buffer(FindAnimation(instance, propName, subPropIndex));
        if (buffer.hasValue()) {
            QT3DS_ASSERT(false);
            return;
        }
        // Find Instance by id checks for the validity of the instance, so we should be OK here.
        Option<InstanceDesc> instOpt = FindInstanceById(instance);
        if (instOpt.hasValue() == false)
            return;

        m_Animations.insert(
            AnimationId(instOpt->m_Handle, propName, subPropIndex),
            CreateAnimation(instOpt->m_Id, propName, subPropIndex, bufType, values));
    }

    Option<Animation> FindAnimation(const QString &instId, const QString &propName,
                                            QT3DSU32 subPropIndex) const override
    {
        Option<InstanceDesc> instance(FindInstanceById(instId));
        if (instance.hasValue() == false)
            return Empty();
        AnimationId id(instance->m_Handle, propName, subPropIndex);
        QHash<AnimationId, Animation *>::const_iterator entry(m_Animations.find(id));
        if (entry != m_Animations.end())
            return **entry;
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

        QVector<InstanceDesc> childList;
        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Added.size())
        {
            const InstanceDesc &inst(m_ImportReport.m_Instances.m_Added[idx]);
            childList.resize(GetNumChildren(inst.m_Handle));

            GetChildren(inst.m_Handle, childList);
            QT3DSIMP_FOREACH(child, childList.size())
            {
                QString sibling = child + 1 < childList.size()
                        ? childList[child + 1].m_Id : QString();
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
        QString thePropName(inAnimation.m_PropertyName);
        SImportComposerTypes theTypes;
        SImportAsset &theAsset(theTypes.GetImportAssetForType(inType));
        DataModelDataType::Value theType(theAsset.GetPropertyDataType(thePropName));
        std::tuple<bool, size_t> animAndArity = GetDatatypeAnimatableAndArity(theType);
        if (std::get<0>(animAndArity) == false) {
            QT3DS_ASSERT(false);
            return;
        };
        if (std::get<1>(animAndArity) > 1) {
            thePropName.append(QLatin1Char('.'));
            switch (inAnimation.m_SubPropertyIndex) {
            case 0:
                thePropName.append(QLatin1Char('x'));
                break;
            case 1:
                thePropName.append(QLatin1Char('y'));
                break;
            case 2:
                thePropName.append(QLatin1Char('z'));
                break;
            case 3:
                thePropName.append(QLatin1Char('w'));
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
    void SerializeAnimation(IDOMReader &reader, const QString &inInstanceId,
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
        theAnimation.m_PropertyName = QString::fromWCharArray(theName.wide_str());
        reader.Att(L"type", theAnimation.m_Type);
        inAttributeBuffer.clear();
        const char8_t *theValue;
        reader.Value(theValue);
        inAttributeBuffer.write(theValue, (QT3DSU32)strlen(theValue) + 1);
        inAttributeBuffer.write((QT3DSU16)0);
        WCharTReader theReader((char8_t *)inAttributeBuffer.begin(), inTempBuf, m_StringTable);
        QVector<float> theData;
        theReader.ReadBuffer(theData);
        Animation *newAnim =
            CreateAnimation(theAnimation.m_InstanceId, theAnimation.m_PropertyName,
                            theAnimation.m_SubPropertyIndex, theAnimation.m_Type, theData);
        m_Animations.insert(
            AnimationId(toHdl(GetInstance(theAnimation.m_InstanceId)), theAnimation.m_PropertyName,
                        theAnimation.m_SubPropertyIndex),
            newAnim);
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
            return ComposerPropertyNames::Convert(lhs).compare(ComposerPropertyNames::Convert(rhs))
                    < 0;
        }
    };

    void Serialize(IDOMWriter &writer, Instance &inInstance, MemoryBuffer<RawAllocator> &inTempBuf)
    {
        if (inInstance.m_Valid == false)
            return;
        IDOMWriter::Scope __instanceScope(writer, ComposerObjectTypes::Convert(inInstance.m_Type));
        QString id;
        writer.Att(QStringLiteral("id"), inInstance.m_Id);
        // Write properties, then write animations
        QVector<ComposerPropertyNames::Enum> theNames;
        for (QHash<ComposerPropertyNames::Enum, SInternValue>::iterator
                 iter = inInstance.m_PropertyValues.begin(),
                 end = inInstance.m_PropertyValues.end();
             iter != end; ++iter) {
            theNames.push_back(iter.key());
        }
        std::sort(theNames.begin(), theNames.end(), PropertyNameSorter());

        for (QT3DSU32 nameIdx = 0, nameEnd = (QT3DSU32)theNames.size();
             nameIdx < nameEnd; ++nameIdx) {
            QHash<ComposerPropertyNames::Enum, SInternValue>::const_iterator iter(
                inInstance.m_PropertyValues.find(theNames[nameIdx]));
            inTempBuf.clear();
            WCharTWriter bufWriter(inTempBuf);
            WStrOps<SValue>().ToBuf(iter->GetValue(), bufWriter);
            if (inTempBuf.size()) {
                inTempBuf.writeZeros(sizeof(wchar_t));
                writer.Att(ComposerPropertyNames::Convert(iter.key()),
                           QString::fromWCharArray((const wchar_t *)inTempBuf.begin()));
            }
        }
        for (auto iter = m_Animations.begin(), end = m_Animations.end(); iter != end; ++iter) {
            if (iter.key().m_Instance == toHdl(&inInstance))
                Serialize(writer, inInstance.m_Type, **iter, inTempBuf);
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
        m_IdToInstMap.insert(inInstance.m_Id, &inInstance);
        for (std::pair<QString, QString> att = reader.GetFirstAttribute();
             !IsTrivial(att.first); att = reader.GetNextAttribute()) {
            if (QLatin1String("id") == att.first)
                continue;
            DataModelDataType::Value thePropertyType = theAsset.GetPropertyDataType(att.first);
            if (thePropertyType != DataModelDataType::None) {
                if (thePropertyType == DataModelDataType::Long4)
                    thePropertyType = DataModelDataType::StringRef;
                inAttributeBuffer.clear();
                inAttributeBuffer.write(qPrintable(att.second),
                                        (QT3DSU32)strlen(qPrintable(att.second)) + 1);
                inAttributeBuffer.write((QT3DSU16)0);
                WCharTReader theReader((char8_t *)inAttributeBuffer.begin(), inTempBuf,
                                       m_StringTable);
                SValue theValue = WStrOps<SValue>().BufTo(thePropertyType, theReader);
                inInstance.m_PropertyValues.insert(
                    ComposerPropertyNames::Convert(att.first),
                                     SInternValue(theValue, m_StringTable));
            }
        }
        {
            IDOMReader::Scope animScope(reader);
            for (bool success = reader.MoveToFirstChild(); success;
                 success = reader.MoveToNextSibling()) {
                if (reader.GetElementName() == QLatin1String("AnimationTrack")) {
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
            inWriter.ChildValue(QStringLiteral("Source"), iter.key());
            inWriter.ChildValue(QStringLiteral("Dest"), *iter);
        }
    }

    void Serialize(IDOMReader &inReader, const wchar_t *inElemName, TStrToStrMap &inHashMap)
    {
        IDOMReader::Scope itemScope(inReader);
        for (bool success = inReader.MoveToFirstChild(inElemName); success;
             success = inReader.MoveToNextSibling(inElemName)) {
            QString source;
            QString dest;
            inReader.ChildValue(QStringLiteral("Source"), source);
            inReader.ChildValue(QStringLiteral("Dest"), dest);
            inHashMap.insert(source, dest);
        }
    }

    void Serialize(IDOMWriter &inWriter, const wchar_t *inElemName, TPathToMeshMap &inHashMap) const
    {
        for (TPathToMeshMap::iterator iter = inHashMap.begin(), end = inHashMap.end(); iter != end;
             ++iter) {
            IDOMWriter::Scope __elemScope(inWriter, inElemName);
            inWriter.ChildValue(QStringLiteral("Source"), iter.key());
            inWriter.ChildValue(QStringLiteral("Dest"), iter->m_FilePath);
        }
    }

    void Serialize(IDOMReader &inReader, const wchar_t *inElemName, TPathToMeshMap &inHashMap)
    {
        IDOMReader::Scope itemScope(inReader);
        for (bool success = inReader.MoveToFirstChild(inElemName); success;
             success = inReader.MoveToNextSibling(inElemName)) {
            QString source;
            QString dest;
            inReader.ChildValue(QStringLiteral("Source"), source);
            inReader.ChildValue(QStringLiteral("Dest"), dest);
            MeshEntry theEntry(source, dest);
            inHashMap.insert(theEntry.m_SourceId, theEntry);
        }
    }

    QT3DSU32 Save(const QString &fname) const override
    {
        using namespace Q3DStudio;
        QT3DSU32 theRevisionId = 1;
        QString fullPath = Q3DStudio::CFilePath::CombineBaseAndRelative(m_DestDirectory, fname);

        std::shared_ptr<IDOMFactory> factory(IDOMFactory::CreateDOMFactory(m_StringTablePtr));
        std::shared_ptr<IDOMWriter> theWriter;
        SDOMElement *theTopElement = nullptr;
        bool exists = QFileInfo(fullPath).exists();

        {
            if (exists) {
                QFile stream(fullPath);

                // OK, ensure we can open this file in append mode.
                // This is kind of tricky because we need to write the data to the file
                // after we read it.
                if (stream.open(QFile::ReadWrite | QFile::Append) == false) {
                    QT3DS_ASSERT(false);
                    return 0;
                }
                stream.seek(0);

                theTopElement = CDOMSerializer::Read(*factory, stream);

                if (theTopElement == nullptr) {
                    QT3DS_ASSERT(false);
                    return 0;
                }
                std::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>>
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
            QVector<Instance *> rootList;
            for (QHash<QString, Instance *>::const_iterator iter = m_IdToInstMap.begin(),
                                                                     end = m_IdToInstMap.end();
                 iter != end; ++iter) {
                if ((*iter)->m_Parent == 0)
                    rootList.push_back(*iter);
            }
            {
                IDOMWriter::Scope __graphScope(writer, L"Graph");
                for (QT3DSU32 idx = 0, end = (QT3DSU32)rootList.size(); idx < end; ++idx)
                    const_cast<ImportImpl &>(*this).Serialize(writer, *rootList[idx], tempBuf);
            }
            {
                IDOMWriter::Scope __importScope(writer, L"Import");
                QString theSrcFile = m_SrcFile;
                if (QFileInfo(theSrcFile).isAbsolute())
                    theSrcFile = CFilePath::GetRelativePathFromBase(fullPath, m_SrcFile);

                writer.Att(L"SrcFile", theSrcFile);
                writer.Att(L"ImageDir", L"Images");
                writer.Att(L"MeshDir", L"Meshes");
                Serialize(writer, L"Image", const_cast<TStrToStrMap &>(m_Images));
                Serialize(writer, L"Mesh", const_cast<TPathToMeshMap &>(m_Meshes));
            }
        }
        {
            QFile stream(fullPath);
            stream.open(QFile::ReadWrite | QFile::Append);
            stream.seek(0);
            CDOMSerializer::WriteXMLHeader(stream);
            CDOMSerializer::Write(*theTopElement, stream);
        }

        return theRevisionId;
    }

    bool Load(const QString &fname, QT3DSU32 inDocumentId)
    {
        using namespace Q3DStudio;
        std::shared_ptr<IDOMFactory> factory(IDOMFactory::CreateDOMFactory(m_StringTablePtr));
        SDOMElement *topElement = nullptr;
        {
            QFile stream(fname);
            if (stream.open(QFile::ReadOnly) == false) {
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

        QString srcFile;
        QString imagesDir;
        QString meshDir;

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
                m_SrcFile = srcFile;
                m_SrcDirectory = QFileInfo(m_SrcFile).absolutePath();
                m_FullSrcDirectory =
                    CFilePath::CombineBaseAndRelative(m_DestDirectory, m_SrcDirectory);
                m_ImageDir = CFilePath::CombineBaseAndRelative(m_DestDirectory,
                                                               imagesDir);
                m_MeshDir = CFilePath::CombineBaseAndRelative(m_DestDirectory,
                                                              meshDir);
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
    RefreshImpl(ImportImpl &src, const QString &srcDirectory)
        : m_Import(src.m_StringTablePtr, srcDirectory, src.m_DestDirectory, src.m_ImageDir,
                   src.m_MeshDir)
        , m_Source(src)
    {
    }
    virtual ~RefreshImpl() { m_Source.Release(); }

    // Returns the import interface you should use to import again from the new source.
    Import &GetImportInterface() { return *this; }

    // Implement the import interface...

    QString RegisterStr(TCharPtr str) override { return m_Import.RegisterStr(str); }
    QString GetSrcFile() const override { return m_Import.GetSrcFile(); }
    QString GetDestDir() const override { return m_Import.GetDestDir(); }
    QString GetImageDir() const override { return m_Import.GetImageDir(); }
    QString GetMeshDir() const override { return m_Import.GetMeshDir(); }
    //QString GetPathBufferDir() const override { return m_Import.GetPathBufferDir(); }
    QT3DSU32 Save(const QString &fname) const override { return m_Import.Save(fname); }

    // Add a mapping from an named id to a handle
    Option<InstanceDesc> GetInstanceByHandle(TIMPHandle inst) const override
    {
        return m_Import.GetInstanceByHandle(inst);
    }
    Option<InstanceDesc> FindInstanceById(const QString &inst) const override
    {
        return m_Import.FindInstanceById(inst);
    }
    Option<InstanceDesc> FindAnyInstanceById(const QString &inst) const override
    {
        return m_Import.FindAnyInstanceById(inst);
    }
    QT3DSU32 GetNumInstances() const override { return m_Import.GetNumInstances(); }
    QT3DSU32 GetInstances(QVector<InstanceDesc> &outDescs) const override
    {
        return m_Import.GetInstances(outDescs);
    }
    QT3DSU32 GetNumProperties(TIMPHandle instance) const override
    {
        return m_Import.GetNumProperties(instance);
    }
    QT3DSU32 GetProperties(TIMPHandle inst, QVector<PropertyValue> &outBuffer) const override
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
    QT3DSU32 GetChildren(TIMPHandle instance, QVector<InstanceDesc> &childBuffer) const override
    {
        return m_Import.GetChildren(instance, childBuffer);
    }

    // Carry user id's across.
    TIMPHandle CreateInstance(const QString &name, ComposerObjectTypes::Enum inType) override
    {
        TIMPHandle retval = m_Import.CreateInstance(name, inType);
        Option<InstanceDesc> srcInst = m_Source.FindInstanceById(name);
        if (srcInst.hasValue()) {
            if (srcInst->m_Type != inType) {
                Instance *inst = fromHdl(retval);
                m_ImportReport.m_TypeChanges.push_back(
                    std::pair<InstanceDesc, ComposerObjectTypes::Enum>(*inst, srcInst->m_Type));
            }
        }
        return retval;
    }

    TIMPHandle CopyInstance(TIMPHandle inSource) override { return m_Import.CopyInstance(inSource); }
    bool SetInstanceProperties(TIMPHandle inst, const QVector<PropertyValue> &inBuffer) override
    {
        return m_Import.SetInstanceProperties(inst, inBuffer);
    }
    bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            const TImportModelValue &val) override
    {
        return m_Import.DoSetInstancePropertyValue(inst, pname, val);
    }
    bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            const QString &val) override
    {
        return m_Import.DoSetInstancePropertyValue(inst, pname, val);
    }
    bool AddChild(TIMPHandle inst, TIMPHandle child) override
    {
        return m_Import.AddChild(inst, child);
    }
    void MarkInstanceInvalid(TIMPHandle inst) override { m_Import.MarkInstanceInvalid(inst); }

    QT3DSU32 GetNumImages() const override { return m_Import.GetNumImages(); }
    QT3DSU32 GetImages(QVector<std::pair<QString, QString>> &imgPaths) const override
    {
        return m_Import.GetImages(imgPaths);
    }
    // Copies the an appropriate location in our import directory
    // Returns the path of the added image.  This may mangle the name slightly
    // In case of a conflict.
    QStringOrError AddImage(const QString &_imgPath) override
    {
        Option<QString> added = m_Source.FindImageByPath(_imgPath);
        return m_Import.AddOrReplaceImage(_imgPath, added);
    }
    Option<QString> FindImageByPath(const QString &imgPath) const override
    {
        return m_Import.FindImageByPath(imgPath);
    }

    Option<QString> FindImageByRelativePath(const QString &imgPath) const override
    {
        return m_Import.FindImageByRelativePath(imgPath);
    }

    QT3DSU32 GetNumMeshes() const override { return m_Import.GetNumMeshes(); }
    QT3DSU32 GetMeshes(QVector<std::pair<QString, QString>> &bufferPaths) const override
    {
        return m_Import.GetMeshes(bufferPaths);
    }
    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    // Mesh name is used to write out a reasonable buffer *and* on refresh to know
    // if a buffer is changed/updated or not
    QStringOrError AddMesh(const Mesh &meshBuffer, const QString &meshName) override
    {
        const QString &safeName = CFilePath::MakeSafeFileStem(meshName);
        return m_Import.AddOrReplaceMesh(meshBuffer, meshName,
                                         m_Source.FindMeshFilePathByName(safeName));
    }

    bool HasMesh(const QString & meshName) const override { return m_Import.HasMesh(meshName); }

    Option<QString> FindMeshReferencePathByName(const QString &meshName) const override
    {
        return m_Import.FindMeshReferencePathByName(meshName);
    }

    Option<QString> FindMeshFilePathByName(const QString &meshName) const override
    {
        return m_Import.FindMeshFilePathByName(meshName);
    }
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    QT3DSU32 GetNumPathBuffers() const override { return m_Import.GetNumPathBuffers(); }
    QT3DSU32 GetPathBuffers(QVector<std::pair<QString, QString>> pathBufferPaths) const override
    {
        return m_Import.GetPathBuffers(pathBufferPaths);
    }
    // Copies the an appropriate location in our import directory
    // Returns the path of the added PathBuffer.  This may mangle the name slightly
    // In case of a conflict.
    QStringOrError AddPathBuffer(const SPathBuffer &pathBuffer, QString pathName) override
    {
        Option<QString> added = m_Source.FindPathBufferByPath(pathName);
        return m_Import.AddOrReplacePathBuffer(pathBuffer, pathName, added);
    }

    Option<QString> FindPathBufferByPath(QString pathBufferPath) const override
    {
        return m_Import.FindPathBufferByPath(pathBufferPath);
    }

    Option<QString> FindPathBufferByRelativePath(QString pathBufferPath) const override
    {
        return m_Import.FindPathBufferByRelativePath(pathBufferPath);
    }
#endif
    QT3DSU32 GetNumAnimations() const override { return m_Import.GetNumAnimations(); }
    QT3DSU32 GetAnimations(QVector<Animation> &outBuffers) const override
    {
        return m_Import.GetAnimations(outBuffers);
    }
    // Data is copied into this object, you can release the anim buffer data after this
    void DoAddAnimation(const QString &instance, const QString &propName, QT3DSU32 subPropIndex,
                                EAnimationType type, const QVector<QT3DSF32> &values) override
    {
        m_Import.DoAddAnimation(instance, propName, subPropIndex, type, values);
    }
    Option<Animation> FindAnimation(const QString &instance, const QString &propName,
                                            QT3DSU32 subPropIndex) const override
    {
        return m_Import.FindAnimation(instance, propName, subPropIndex);
    }

    template <typename TDataType, typename TDiffOp>
    void CompileItemReport(AddRemoveImpl<TDataType> &result, TDiffOp op) const
    {
        QT3DSU32 numItems = op.NumItems(m_Import);
        QVector<TDataType> tempData(numItems);
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
        tempData.resize(numItems);
        op.GetItems(m_Source, tempData);
        QT3DSIMP_FOREACH(idx, numItems)
        {
            const TDataType &oldDesc(tempData[idx]);
            if (op.HasItem(m_Import, oldDesc) == false)
                result.m_Removed.push_back(oldDesc);
        }
    }

    QVector<InstanceDesc> toVector(const InstanceDesc *data, QT3DSU32 count)
    {
        QVector<InstanceDesc> ret(count);
        for (int i = 0; i < count; ++i)
            ret[i] = data[i];
        return ret;
    }

    ImportReport CompileReport() const override
    {
        m_ImportReport.clear();
        // Find existint, added, and removed items
        CompileItemReport(m_ImportReport.m_Instances, InstanceDiffOp());
        CompileItemReport(m_ImportReport.m_Images, ImageDiffOp());
        CompileItemReport(m_ImportReport.m_Meshes, MeshDiffOp());
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
        CompileItemReport(m_ImportReport.m_PathBuffers, PathBufferDiffOp());
#endif
        CompileItemReport(m_ImportReport.m_Animations, AnimationDiffOp());

        // OK, prepare parent child links

        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Removed.size())
        {
            const InstanceDesc &oldDesc(m_ImportReport.m_Instances.m_Removed[idx]);
            QT3DSU32 numChildren = m_Source.GetNumChildren(oldDesc.m_Handle);
            QVector<InstanceDesc> tempData(numChildren);
            m_Source.GetChildren(oldDesc.m_Handle, tempData);
            QT3DSIMP_FOREACH(chld, numChildren)
            m_ImportReport.m_Links.m_Removed.push_back(
                ParentChildLink(oldDesc.m_Id, tempData[chld].m_Id, QString()));
        }
        QT3DSIMP_FOREACH(idx, m_ImportReport.m_Instances.m_Added.size())
        {
            const InstanceDesc &newDesc(m_ImportReport.m_Instances.m_Added[idx]);
            QT3DSU32 numChildren = GetNumChildren(newDesc.m_Handle);
            QVector<InstanceDesc> tempData(numChildren);
            m_Import.GetChildren(newDesc.m_Handle, tempData);
            QT3DSIMP_FOREACH(chldIdx, numChildren)
            {
                // Run through children in reverse order
                // so that we know the sibling pointer is good.
                QT3DSU32 chld = numChildren - chldIdx - 1;
                QString child = tempData[chld].m_Id;
                // If the chlid existed in the original, then it also needs to be added here.
                // Because that means the a new object was added and an original object was
                // re-attached.
                if (m_Source.FindInstanceById(child).hasValue()) {
                    QString sibling = chld + 1 < numChildren ? tempData[chld + 1].m_Id : QString();
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
            QVector<InstanceDesc> newChildren(numNewChildren);
            QVector<InstanceDesc> oldChildren(numOldChildren);
            m_Import.GetChildren(newDesc.m_Handle, newChildren);
            m_Source.GetChildren(oldDesc.m_Handle, oldChildren);
            QT3DSIMP_FOREACH(childIdx, numNewChildren)
            {
                // Run through the links in reverse order so we know the sibling pointer is good.
                // This is necessary for addChild.
                QT3DSU32 child = numNewChildren - childIdx - 1;
                const InstanceDesc &childDesc = newChildren[child];
                QString sibling = child + 1 < numNewChildren
                        ? newChildren[child + 1].m_Id : QString();
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
                QString sibling =
                    child + 1 < oldChildren.size() ? oldChildren[child + 1].m_Id : QString();
                if (FindChild(m_Import.FindInstanceById(childDesc.m_Id), sibling, childDesc.m_Type,
                              newChildren)
                    == false)
                    m_ImportReport.m_Links.m_Removed.push_back(
                        ParentChildLink(oldDesc.m_Id, childDesc.m_Id, QString()));
            }
        }
        m_Import.FinalizeReport();

        return m_ImportReport;
    }

    void Release() override { delete this; }
};

}

ImportPtrOrError Import::Create(const QString &_srcPath, const QString &_destDir)
{
    QString srcPath(QFileInfo(_srcPath).absoluteFilePath());
    QString destDir(QFileInfo(_destDir).absoluteFilePath());

    if (!QFileInfo(_srcPath).isFile())
        return ImportErrorData(ImportErrorCodes::SourceFileDoesNotExist, srcPath);

    if (!CFilePath::CreateDir(destDir, true))
        return ImportErrorData(ImportErrorCodes::UnableToCreateDirectory, destDir);

    TStringTablePtr strTable = qt3dsdm::IStringTable::CreateStringTable();
    return new ImportImpl(strTable, srcPath, destDir, QStringLiteral("maps"),
                          QStringLiteral("meshes"));
}

ImportPtrOrError Import::Load(const QString &pathToFile, QT3DSU32 inImportVersion)
{
    QFileInfo info(pathToFile);
    QString fullFilePath(info.absoluteFilePath());
    QString destDir(info.canonicalPath());

    if (info.isFile() == false)
        return ImportErrorData(ImportErrorCodes::SourceFileNotReadable, fullFilePath);

    TStringTablePtr strTable = qt3dsdm::IStringTable::CreateStringTable();
    ImportImpl *impl = new ImportImpl(strTable, QString(), destDir);
    bool success = impl->Load(fullFilePath, inImportVersion);
    QT3DS_ASSERT(success);
    if (success == false) {
        impl->Release();
        return ImportErrorData(ImportErrorCodes::SourceFileNotReadable, fullFilePath);
    }
    return impl;
}

ImportPtrOrError Import::CreateForRefresh(Import &original, const QString &_srcPath)
{
    QFileInfo srcPath(_srcPath);

    if (!srcPath.isFile()) {
        original.Release();
        return ImportErrorData(ImportErrorCodes::SourceFileDoesNotExist, _srcPath);
    }

    RefreshImpl *refresh(new RefreshImpl(static_cast<ImportImpl &>(original), _srcPath));
    return &refresh->GetImportInterface();
}

QT3DSU32 Import::GetHighestImportRevision(const QString &pathToFile)
{
    using std::shared_ptr;
    CFilePath fullFilePath(CFilePath::GetAbsolutePath(pathToFile));
    QFile stream(fullFilePath.toQString());
    if (stream.open(QFile::ReadOnly) == false) {
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
