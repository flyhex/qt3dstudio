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
#pragma once
#ifndef QT3DS_IMPORT_H
#define QT3DS_IMPORT_H
#include "Qt3DSImportLibPrecompile.h"
#include "Qt3DSImportMesh.h"
#include "Qt3DSImportPath.h"
#include "Qt3DSImportPerformImport.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSImportComposerTypes.h"

namespace qt3dsimp {
using namespace std;
typedef QT3DSU64 TIMPHandle;

struct InstanceDesc
{
    QString m_Id;
    TIMPHandle m_Parent;
    TIMPHandle m_Handle;
    ComposerObjectTypes::Enum m_Type;
    InstanceDesc()
        : m_Parent(0)
        , m_Handle(0)
        , m_Type(ComposerObjectTypes::Unknown)
    {
    }
};

struct Animation
{
    QString m_InstanceId;
    QString m_PropertyName;
    QT3DSU32 m_SubPropertyIndex;
    EAnimationType m_Type;
    QVector<QT3DSF32> m_Keyframes;
    Animation(const QString &iid, const QString &pname, QT3DSU32 subPropIndex,
              EAnimationType bufType, const QVector<QT3DSF32> &keyframes)
        : m_InstanceId(iid)
        , m_PropertyName(pname)
        , m_SubPropertyIndex(subPropIndex)
        , m_Type(bufType)
        , m_Keyframes(keyframes)
    {
    }
    Animation()
        : m_SubPropertyIndex(0)
        , m_Type(EAnimationTypeNone)
    {
    }
    Animation(const Animation &a)
        : m_InstanceId(a.m_InstanceId)
        , m_PropertyName(a.m_PropertyName)
        , m_SubPropertyIndex(a.m_SubPropertyIndex)
        , m_Type(a.m_Type)
        , m_Keyframes(a.m_Keyframes)
    {
    }
    Animation &operator=(const Animation *o)
    {
        *this = Animation(*o);
        return *this;
    }
};

struct AddRemoveInfo
{
    QT3DSU32 m_Existing; // intersection of new and old
    QT3DSU32 m_NumAdded; // new that doesn't exist in old
    QT3DSU32 m_NumRemoved; // old that doesn't exist in new
    AddRemoveInfo()
        : m_Existing(0)
        , m_NumAdded(0)
        , m_NumRemoved(0)
    {
    }
    AddRemoveInfo(QT3DSU32 exist, QT3DSU32 add, QT3DSU32 remove)
        : m_Existing(exist)
        , m_NumAdded(add)
        , m_NumRemoved(remove)
    {
    }
};

struct ParentChildLink
{
    QString m_Parent;
    QString m_Child;
    QString m_NextSibling;

    ParentChildLink(const QString &p, const QString &c, const QString &ns)
        : m_Parent(p)
        , m_Child(c)
        , m_NextSibling(ns)
    {
    }
    ParentChildLink() {}
    bool operator<(const ParentChildLink &other) const
    {
        return m_Parent < other.m_Parent;
    }
};

template <typename TDataType>
struct AddRemoveData
{
    QVector<TDataType> m_Existing;
    QVector<TDataType> m_Added;
    QVector<TDataType> m_Removed;
    AddRemoveData() {}
    AddRemoveData(const QVector<TDataType> &exist, const QVector<TDataType> &add,
                  const QVector<TDataType> &rem)
        : m_Existing(exist)
        , m_Added(add)
        , m_Removed(rem)
    {
    }
};

// Contains the basic information to perform an import or a refresh.
// Note that during an import or refresh, the user-id information on the
// descriptors may change.  So you can't ever use exactly this information
// on the descriptors.  You have to always query the descriptor.  Specifically,
// Create instance, delete instance, and update instance can change user id
// information.
struct ImportReport
{
    // There is a precondition you can rely on that parents are added
    // before children.  Furthermore, instances that are added won't
    // appear in the below parent child link added section.
    AddRemoveData<InstanceDesc> m_Instances;
    // For children, existing does tell you the intersection of the two
    // sets *but* existing is in the order of the new set.
    // This allows clients to detect child reordering where it may have
    // a substantial effect (such as materials which would then get mapped
    // to a different mesh instance).
    AddRemoveData<ParentChildLink> m_Links;

    AddRemoveData<std::pair<QString, QString>> m_Images;
    AddRemoveData<std::pair<QString, QString>> m_Meshes;
    AddRemoveData<std::pair<QString, QString>> m_PathBuffers;
    AddRemoveData<Animation> m_Animations;

    ImportReport() {}
    ImportReport(AddRemoveData<InstanceDesc> instances, AddRemoveData<ParentChildLink> links,
                 AddRemoveData<std::pair<QString, QString>> imageBuffers,
                 AddRemoveData<std::pair<QString, QString>> meshes,
                 AddRemoveData<std::pair<QString, QString>> pathBuffers,
                 AddRemoveData<Animation> animBuffers)
        : m_Instances(instances)
        , m_Links(links)
        , m_Images(imageBuffers)
        , m_Meshes(meshes)
        , m_PathBuffers(pathBuffers)
        , m_Animations(animBuffers)
    {
    }
};

template <typename TDataType>
struct DatatypeOrError
{
    bool m_Error;
    ImportErrorData m_ErrorData;
    TDataType m_Value;
    DatatypeOrError(const TDataType &value)
        : m_Error(false)
        , m_Value(value)
    {
    }
    DatatypeOrError(const ImportErrorData &error, const TDataType &badValue = TDataType())
        : m_Error(true)
        , m_ErrorData(error)
        , m_Value(badValue)
    {
    }

    TDataType &operator->()
    {
        QT3DS_ASSERT(m_Error == false);
        return m_Value;
    }
    TDataType &operator*()
    {
        QT3DS_ASSERT(m_Error == false);
        return m_Value;
    }
    operator TDataType()
    {
        QT3DS_ASSERT(m_Error == false);
        return m_Value;
    }
};
class Import;
typedef DatatypeOrError<Import *> ImportPtrOrError;
typedef DatatypeOrError<QString> QStringOrError;

template <typename TDataType>
struct SImportConverter
{
    TImportModelValue Convert(const TDataType &inType) { return inType; }
};

template <>
struct SImportConverter<TDataStrPtr>
{
    const wchar_t *Convert(const wchar_t *inValue) { return inValue; }
};
// Long4s are string refs, so they are set by string
template <>
struct SImportConverter<SLong4>
{
    SValue Convert(const wchar_t *inValue) { return SStringRef(inValue); }
};

class Import
{
protected:
    virtual ~Import(){}

public:
    // So users can check if a file is an import file.
    //<UICImportLib version="${fileversion}"
    static QT3DSU32 GetImportFileVersion() { return 1; }

    // Cache this string in the string table and return
    // a representation that will be around until this import object
    // is destroyed.
    virtual QString RegisterStr(TCharPtr data) = 0;
    // Returns the source directory relative to the dest directory
    // or the full path if it is on a different drive
    virtual QString GetSrcFile() const = 0;
    // Returns the full path of the destination directory
    virtual QString GetDestDir() const = 0;
    // Returns the full path of the image directory
    virtual QString GetImageDir() const = 0;
    // Returns the full path of the mesh directory
    virtual QString GetMeshDir() const = 0;
    // Returns the full path to the path buffer directory
    //virtual QString GetPathBufferDir() const = 0;
    virtual void Release() = 0;
    // returns false if fname couldn't be opened for write
    // Fname is appended to the directory this object was created with.
    // This is necessary in order to keep relative paths valid within
    // the import file.  Returns an ID that uniquely identifies this import
    // object within the final document.
    virtual QT3DSU32 Save(const QString &fname) const = 0;

    // Add a mapping from an named id to a handle
    virtual Option<InstanceDesc> GetInstanceByHandle(TIMPHandle inst) const = 0;
    virtual Option<InstanceDesc> FindInstanceById(const QString &inst) const = 0;
    virtual Option<InstanceDesc> FindAnyInstanceById(const QString &inst) const = 0;
    virtual QT3DSU32 GetNumInstances() const = 0;
    virtual QT3DSU32 GetInstances(QVector<InstanceDesc> &outDescs) const = 0;
    virtual QT3DSU32 GetNumProperties(TIMPHandle instance) const = 0;
    virtual QT3DSU32 GetProperties(TIMPHandle inst, QVector<PropertyValue> &outBuffer) const = 0;
    virtual Option<SValue> GetInstancePropertyValue(TIMPHandle inst,
                                                    ComposerPropertyNames::Enum val) const = 0;
    virtual QT3DSU32 GetNumChildren(TIMPHandle instance) const = 0;
    virtual QT3DSU32 GetChildren(TIMPHandle instance, QVector<InstanceDesc> &childBuffer) const = 0;
    // Invalid instances will not be saved out to the import file and should be ignored
    // in the import report.
    virtual void MarkInstanceInvalid(TIMPHandle inst) = 0;

    virtual TIMPHandle CreateInstance(const QString &id, ComposerObjectTypes::Enum inType) = 0;
    // The new instance ends up attached to the same parent
    // and just after inSource in the parent's children lists.
    // This performs a deep copy and fixes up references if they occur in the
    // new hierarchy.
    virtual TIMPHandle CopyInstance(TIMPHandle inSource) = 0;

    // Returns true if inst exists, false otherwise.

    virtual bool SetInstanceProperties(TIMPHandle inst, const QVector<PropertyValue> &inBuffer) = 0;

    template <typename TPropertyType, typename TDataType>
    void SetInstancePropertyValue(TIMPHandle inst,
                                  const SImportPropertyDefinition<TPropertyType> &inProperty,
                                  const TDataType &val)
    {
        DoSetInstancePropertyValue(inst, inProperty.m_Name,
                                   SImportConverter<TPropertyType>().Convert(val));
    }

    template <typename TPropertyType>
    void SetInstancePropertyValue(TIMPHandle inst,
                                  const SImportPropertyDefinition<TPropertyType> &inProperty,
                                  const QString &val)
    {
        DoSetInstancePropertyValue(inst, inProperty.m_Name, val);
    }

    virtual bool AddChild(TIMPHandle inst, TIMPHandle child) = 0;

    virtual QT3DSU32 GetNumImages() const = 0;
    // Returns imgOriginalPath,imgDestPath pair
    virtual QT3DSU32 GetImages(QVector<std::pair<QString, QString>> &imgPaths) const = 0;
    // Copies the an appropriate location in our import directory
    // Returns the path of the added image.  This may mangle the name slightly
    // In case of a conflict.
    // If this function returns L"", there was a problem copying the data
    // and you need to abort the import process immediately.  For a new import,
    // then delete the existing imported images.  For refresh, the user may just
    // be stuck with a half-updated dataset.
    // This path is relative to the current working directory when this object was created
    // or absolute.
    // Returns a relative path, from dest directory, where the image was saved out to.
    virtual QStringOrError AddImage(const QString &imgPath) = 0;
    // Assuming this image path is relative to the current working directory, find the image
    virtual Option<QString> FindImageByPath(const QString &imgPath) const = 0;
    // Assuming this path is relative to the import source document, find the image.
    virtual Option<QString> FindImageByRelativePath(const QString &imgPath) const = 0;

    virtual QT3DSU32 GetNumMeshes() const = 0;
    // Returns meshName,meshDestPath pair
    virtual QT3DSU32 GetMeshes(QVector<std::pair<QString, QString>> &bufferPaths) const = 0;

    // Copies the vertex buffer into the appropriate location, renaming if necessary.
    // Mesh name is used to write out a reasonable buffer *and* on refresh to know
    // if a buffer is changed/updated or not
    // If this function returns L"", there was a problem copying the data
    // and you need to abort the import process immediately. For a new import,
    // then delete the existing imported images.  For refresh, the user may just
    // be stuck with a half-updated dataset.
    // Returns a relative path, from the mesh directory, where the mesh was saved out
    // to.
    virtual QStringOrError AddMesh(const Mesh &meshBuffer, const QString &meshName) = 0;
    virtual bool HasMesh(const QString &meshName) const = 0;
    // Return the mesh path with the current version number
    virtual Option<QString> FindMeshReferencePathByName(const QString &meshName) const = 0;
    virtual Option<QString> FindMeshFilePathByName(const QString &meshName) const = 0;
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    virtual QT3DSU32 GetNumPathBuffers() const = 0;
    // Returns imgOriginalPath,imgDestPath pair
    virtual QT3DSU32 GetPathBuffers(QVector<std::pair<QString, QString>> pathBufferPaths) const = 0;
    virtual QStringOrError AddPathBuffer(const SPathBuffer &pathBuffer, QString pathName) = 0;
    // Assuming this Path path is relative to the current working directory, find the Path
    virtual Option<QString> FindPathBufferByPath(QString pathBufferPath) const = 0;
    // Assuming this path is relative to the import source document, find the Path.
    virtual Option<QString> FindPathBufferByRelativePath(QString pathBufferPath) const = 0;
#endif
    virtual QT3DSU32 GetNumAnimations() const = 0;
    virtual QT3DSU32 GetAnimations(QVector<Animation> &outBuffers) const = 0;
    // Data is copied into this object, you can release the anim buffer data after this
    virtual Option<Animation> FindAnimation(const QString &instance, const QString &propName,
                                            QT3DSU32 subPropIndex) const = 0;
    template <typename TDataType>
    void AddAnimation(const QString &instance,
                      const SImportPropertyDefinition<TDataType> &inProperty,
                      QT3DSU32 subPropIndex, EAnimationType type,const QVector<QT3DSF32> &values)
    {
        std::tuple<bool, size_t> isAnimatableAndArity(
            GetDatatypeAnimatableAndArity(TypeToDataType<TDataType>()));
        if (std::get<0>(isAnimatableAndArity)) {
            if (subPropIndex >= std::get<1>(isAnimatableAndArity)) {
                QT3DS_ASSERT(false);
                subPropIndex = 0;
            }
            DoAddAnimation(instance, ComposerPropertyNames::Convert(inProperty.GetName()),
                           subPropIndex, type, values);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    // Call after the import process is complete in order to get all the new items.
    virtual ImportReport CompileReport() const = 0;

protected:
    // Careful with this.  If the property value contains heap memory
    // Then this may crash if you are coming from a dll.
    virtual void DoAddAnimation(const QString &instance, const QString &propName,
                                QT3DSU32 subPropIndex,
                                EAnimationType type, const QVector<QT3DSF32> &values) = 0;
    virtual bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            const TImportModelValue &val) = 0;
    virtual bool DoSetInstancePropertyValue(TIMPHandle inst, ComposerPropertyNames::Enum pname,
                                            const QString &val) = 0;

public:
    // Create blank import object
    // It is the caller's responsibility to ensure that imgPath and meshBufPath exist and
    // are writable.
    // Of srcDirectoryOrFile is a file, we get the directory of that file using
    // FilePath::GetDirName( srcDirectoryOrFile )
    // We then get its absolute path, get our absolute path, and attempt to create
    // a relative path from dest directory to src directory.
    static ImportPtrOrError Create(const QString &srcFile, const QString &destDirectory);

    // Load an import object from a file.  This can obviously fail
    // Also, an import file can contain multiple documents so you need to identify
    // which document you are talking about.
    // 0 means load the latest document.
    static ImportPtrOrError Load(const QString &pathToFile, QT3DSU32 inDocumentId = 0);

    // Create an import object that will update heavy resources (like images and mesh objects)
    // and it CompileReport will return objects that indicate the differences between the new
    // document and the existing document.
    // You have a chance to refresh from a document that isn't in the same location.
    // We use relative paths throughout the system so that image src paths are relative
    // to the src document directory and such, so we can still import sanely.
    // Dest directory is set to where original's dest directory was set to.
    static ImportPtrOrError CreateForRefresh(Import &original, const QString &srcFile);

    // Return the highest import version in a given document.  Returns zero upon
    // failure, else an integer that is valid for load.
    static QT3DSU32 GetHighestImportRevision(const QString &pathToFile);
};
}

#endif
