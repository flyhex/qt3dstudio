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
#include "IComposerEditorInterface.h"
#include "Qt3DSImport.h"
#include "Qt3DSImportTranslation.h"
#include "Graph.h"
#include "StandardExtensions.h"
#include "EASTL/hash_map.h"

using namespace Q3DStudio::ComposerImport;
using namespace qt3ds::foundation;

namespace {

// base class between performing refresh and performing
// imports
struct SComposerImportBase
{
    IDocumentEditor &m_Editor;
    QString m_DocumentPath; // Root directory where the studio file sits.
    QString m_DestImportDir; // Directory where we are saving the import file
    QString m_DestImportFile;
    QString m_Relativeimportfile;
    qt3ds::QT3DSI32 m_StartTime;
    SComposerImportBase(IDocumentEditor &inEditor, const QString &docPath,
                        const QString &inFullPathToImportFile, long inStartTime)
        : m_Editor(inEditor)
        , m_DocumentPath(docPath)
        , m_DestImportDir(QFileInfo(inFullPathToImportFile).absolutePath())
        , m_DestImportFile(inFullPathToImportFile)
        , m_Relativeimportfile(Q3DStudio::CFilePath::GetRelativePathFromBase(
                                   m_DocumentPath, inFullPathToImportFile))
        , m_StartTime(inStartTime)
    {
    }
};

struct SComposerImportInterface : public SComposerImportBase, public IComposerEditorInterface
{
    typedef QHash<TImportId, Qt3DSDMInstanceHandle> TImportInstanceMap;
    Qt3DSDMInstanceHandle m_Parent;
    Qt3DSDMInstanceHandle m_Root;
    Qt3DSDMSlideHandle m_Slide;
    qt3ds::QT3DSI32 m_StartTime;
    Import *m_ImportObj;
    TImportInstanceMap m_ImportToInstanceMap;
    TImportInstanceMap m_MaterialToInstanceMap;
    QVector<Qt3DSDMInstanceHandle> m_createdMaterials;

    // When we are refreshing, the root assets is the group we are refreshing.
    SComposerImportInterface(Q3DStudio::IDocumentEditor &editor, qt3dsdm::CDataModelHandle parent,
                             qt3dsdm::CDataModelHandle root, qt3dsdm::Qt3DSDMSlideHandle slide,
                             const QString &docPath, const QString &inFullPathToImportFile,
                             long inStartTime)
        : SComposerImportBase(editor, docPath, inFullPathToImportFile, inStartTime)
        , m_Parent(parent)
        , m_Root(root)
        , m_Slide(slide)
        , m_StartTime(inStartTime)
        , m_ImportObj(NULL)
    {
        m_Editor.BeginAggregateOperation();
    }

    // Fires the 'do' notifications
    ~SComposerImportInterface() { m_Editor.EndAggregateOperation(); }

    // IComposerEditorInterface

    bool HasError() override { return m_Root.Valid() == false; }

    QString GetRelativeimportfile() const { return {}; }

    void Finalize(const QString &inFilePath) override
    {
        m_Editor.SetSpecificInstancePropertyValue(
            m_Slide, m_Root, QStringLiteral("sourcepath"),
            std::make_shared<CDataStr>(m_Relativeimportfile));
        m_Editor.SetSpecificInstancePropertyValue(
            m_Slide, m_Root, QStringLiteral("importfile"),
            std::make_shared<CDataStr>(m_Relativeimportfile));
    }

    Qt3DSDMInstanceHandle FindInstance(TImportId inImportHdl) override
    {
        TImportInstanceMap::const_iterator entry(m_ImportToInstanceMap.find(inImportHdl));
        if (entry != m_ImportToInstanceMap.end())
            return *entry;
        return 0;
    }

    Qt3DSDMInstanceHandle findMaterial(TImportId inImportHdl)
    {
        TImportInstanceMap::const_iterator entry(m_MaterialToInstanceMap.find(inImportHdl));
        if (entry != m_MaterialToInstanceMap.end())
            return *entry;
        return 0;
    }

    void AddInstanceMap(Qt3DSDMInstanceHandle instanceHandle, TImportId inImportId) override
    {
        Q_ASSERT(!inImportId.isEmpty());
        bool success = m_ImportToInstanceMap.insert(inImportId, instanceHandle).key() == inImportId;
        Q_ASSERT(success);
    }

    void addMaterialMap(Qt3DSDMInstanceHandle instanceHandle, TImportId inImportId)
    {
        Q_ASSERT(!inImportId.isEmpty());
        bool success = m_MaterialToInstanceMap.insert(inImportId, instanceHandle).key()
                            == inImportId;
        Q_ASSERT(success);
    }

    Qt3DSDMInstanceHandle GetRoot() const override { return m_Root; }

    const QString GetDestImportFile() const override { return m_DestImportFile; }

    // IComposerEditor
    // Object is stack created for now
    void Release() override {}

    void BeginImport(Import &importObj) override { m_ImportObj = &importObj; }

    void RemoveChild(TImportId parent, TImportId child) override
    {
        Qt3DSDMInstanceHandle childHandle = FindInstance(child);
        Qt3DSDMInstanceHandle parentHandle = FindInstance(parent);
        if (childHandle.Valid() && parentHandle.Valid()) {
            Qt3DSDMInstanceHandle parentId = m_Editor.GetParent(childHandle);
            // If the child was moved, we don't remove it.  Only on the case where
            // it has it's original parent.
            if (parentId == parentHandle)
                m_Editor.RemoveChild(parentHandle, childHandle);
        }
    }

    void RemoveInstance(TImportId inInstance) override
    {
        Qt3DSDMInstanceHandle instance = FindInstance(inInstance);
        if (instance.Valid())
            m_Editor.DeleteInstance(instance);
    }

    Qt3DSDMInstanceHandle CreateInstance(ComposerObjectTypes::Enum type,
                                         Qt3DSDMInstanceHandle parent,
                                         TImportId inImportId, bool useParentSlide = false)
    {
        if (parent.Valid() == false) {
            assert(0);
            return 0;
        }

        // Map the type to the object type.
        auto slide = m_Slide;
        if (useParentSlide)
            slide = m_Editor.GetAssociatedSlide(parent);
        Qt3DSDMInstanceHandle retval = m_Editor.CreateSceneGraphInstance(type, parent, slide);
        m_Editor.SetSpecificInstancePropertyValue(0, retval, "importid",
                                                  std::make_shared<CDataStr>(inImportId));
        m_Editor.SetSpecificInstancePropertyValue(
            slide, retval, QStringLiteral("importfile"),
            std::make_shared<CDataStr>(m_Relativeimportfile));
        AddInstanceMap(retval, inImportId);
        return retval;
    }

    /**
     *	Note that instance properties that point to files (sourcepath generally) point to files
     *	relative to the import file.  You need to do combineBaseAndRelative with those files
     *	and the a new getRelativeFromBase with the final file in order to transfer data
     *	successfully.  The time to do this sort of thing is upon create or update instance.
     */
    void CreateRootInstance(TImportId inImportId, ComposerObjectTypes::Enum type) override
    {
        Qt3DSDMInstanceHandle retval = m_Root;
        if (m_Root.Valid() == false) {
            retval = CreateInstance(type, m_Parent, inImportId);
            m_Root = retval;
            if (m_StartTime >= 0)
                m_Editor.SetStartTime(retval, m_StartTime);
        } else
            AddInstanceMap(m_Root, inImportId);

        QT3DS_ASSERT(m_Root.Valid());
    }

    void CreateInstance(TImportId inImportId, ComposerObjectTypes::Enum type,
                                TImportId inParent) override
    {
        Qt3DSDMInstanceHandle theParent(FindInstance(inParent));
        bool useParentSlide = false;
        if (!theParent.Valid()) {
            theParent = findMaterial(inParent);
            if (theParent.Valid())
                useParentSlide = true;
        }
        if (theParent.Valid())
            CreateInstance(type, theParent, inImportId, useParentSlide);
    }

    void createMaterial(const InstanceDesc &desc, TImportId inParent) override
    {
        QString materialName = desc.m_Id;
        Option<SValue> name = m_ImportObj->GetInstancePropertyValue(desc.m_Handle,
                                                                    ComposerPropertyNames::name);
        if (name.hasValue())
            materialName = qt3dsdm::get<TDataStrPtr>(*name)->toQString();

        QString filepath = m_Editor.getMaterialFilePath(materialName);
        int i = 1;
        const QString originalMaterialName = materialName;
        const QString importFile = QStringLiteral("importfile");
        while (QFileInfo(filepath).exists()) {
            i++;
            QString name;
            QMap<QString, QString> values;
            QMap<QString, QMap<QString, QString>> textureValues;
            m_Editor.getMaterialInfo(filepath, name, values, textureValues);
            if (values.contains(importFile) && values[importFile] == m_Relativeimportfile) {
                const auto material = m_Editor.getOrCreateMaterial(materialName);
                if (!m_createdMaterials.contains(material))
                    m_Editor.setMaterialValues(material, values, textureValues);
                break;
            }
            materialName = originalMaterialName + QString::number(i);
            filepath = m_Editor.getMaterialFilePath(materialName);
        }

        bool isNewMaterial = !m_Editor.getMaterial(materialName).Valid();
        const auto material = m_Editor.getOrCreateMaterial(materialName);
        if (!m_createdMaterials.contains(material) && isNewMaterial) {
            m_Editor.SetSpecificInstancePropertyValue(0, material, QStringLiteral("importid"),
                                                      std::make_shared<CDataStr>(desc.m_Id));
            m_Editor.SetSpecificInstancePropertyValue(
            0, material, QStringLiteral("importfile"),
            std::make_shared<CDataStr>(m_Relativeimportfile));
            addMaterialMap(material, desc.m_Id);
            m_createdMaterials.append(material);
        }

        const auto sourcePath = m_Editor.writeMaterialFile(material,
                                                           materialName,
                                                           true);

        Qt3DSDMInstanceHandle parent(FindInstance(inParent));
        auto instance = m_Editor.CreateSceneGraphInstance(ComposerObjectTypes::ReferencedMaterial,
                                                          parent, m_Slide);
        m_Editor.setMaterialReferenceByPath(instance, materialName);
        m_Editor.SetName(instance, materialName);
        m_Editor.setMaterialSourcePath(instance, sourcePath);
    }

    void UpdateInstanceProperties(TImportId inInstance, const PropertyValue *propertBuffer,
                                  QT3DSU32 propertyBufferSize) override
    {
        Qt3DSDMInstanceHandle hdl(FindInstance(inInstance));
        if (hdl.Valid() == false) {
            hdl = findMaterial(inInstance);
            if (hdl.Valid() == false)
                return;
        }

        if (m_Editor.IsInstance(hdl) == false)
            return;

        for (QT3DSU32 idx = 0; idx < propertyBufferSize; ++idx) {
            const PropertyValue &value(propertBuffer[idx]);
            SValue theValue(value.m_Value);

            DataModelDataType::Value theType = GetValueType(theValue);
            if (value.m_Name == ComposerPropertyNames::sourcepath) {
                // re-work the path to be relative to where the main document
                // is saved instead of where the import result is saved
                TDataStrPtr value = qt3dsdm::get<TDataStrPtr>(theValue);
                if (value->GetLength()) {
                    QString valueStr(value->toQString());
                    QString fullPath =
                        Q3DStudio::CFilePath::CombineBaseAndRelative(m_DestImportDir, valueStr);
                    QString relativePath =
                        Q3DStudio::CFilePath::GetRelativePathFromBase(m_DocumentPath, fullPath);
                    theValue = std::make_shared<CDataStr>(relativePath);
                }
            } else if (theType == DataModelDataType::StringRef) {
                SStringRef theRef = get<SStringRef>(theValue);
                SLong4 theGuid;
                Qt3DSDMInstanceHandle target = FindInstance(QString::fromWCharArray(theRef.m_Id));
                if (target.Valid())
                    theGuid = m_Editor.GetGuidForInstance(target);
                theValue = theGuid;
            }

            // Note that we explicitly set the property values on the instance,
            // not on any given slide.
            m_Editor.SetSpecificInstancePropertyValue(
                0, hdl, ComposerPropertyNames::Convert(value.m_Name), theValue);
        }
    }
    void AddChild(TImportId parent, TImportId child, TImportId inSibling) override
    {
        if (findMaterial(child).Valid())
            return;
        Qt3DSDMInstanceHandle theParent(FindInstance(parent));
        if (!theParent.Valid())
            theParent = findMaterial(parent);
        Qt3DSDMInstanceHandle theChild(FindInstance(child));
        Qt3DSDMInstanceHandle theSibling(FindInstance(inSibling));

        if (theParent.Valid() && theChild.Valid())
            m_Editor.AddChild(theParent, theChild, theSibling);
    }

    void RemoveAnimation(TImportId inInstance, const QString &propName, long propSubIndex) override
    {
        Qt3DSDMInstanceHandle hdl(FindInstance(inInstance));
        if (hdl.Valid())
            m_Editor.RemoveAnimation(m_Slide, hdl, propName, propSubIndex);
    }
    void UpdateAnimation(TImportId inInstance, const QString &propName, long propSubIndex,
                         EAnimationType animType, const float *animData, QT3DSU32 numFloats) override
    {
        Qt3DSDMInstanceHandle hdl(FindInstance(inInstance));
        if (hdl.Valid()) {
            if (m_Editor.IsAnimationArtistEdited(m_Slide, hdl, propName, propSubIndex) == false) {
                Qt3DSDMAnimationHandle anim = m_Editor.CreateOrSetAnimation(
                    m_Slide, hdl, propName, propSubIndex, animType, animData, numFloats, false);
                m_Editor.SetIsArtistEdited(anim, false);
            }
        }
    }
    void AddAnimation(TImportId inInstance, const QString &propName, long propSubIndex,
                      EAnimationType animType, const float *animData, QT3DSU32 numFloats) override
    {
        UpdateAnimation(inInstance, propName, propSubIndex, animType, animData, numFloats);
    }

    void EndImport() override {}
};

struct SComposerRefreshInterface : public SComposerImportBase, public IComposerEditor
{
    TIdMultiMap &m_IdToSlideInstances;
    bool m_HasError;
    CGraph &m_AssetGraph;
    Import *m_importObj;
    QVector<Qt3DSDMInstanceHandle> m_createdMaterials;

    struct SSlideInstanceIdMapIterator
    {
        const QVector<QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>> *m_CurrentItems;
        size_t m_CurrentTreeIdx;
        size_t m_CurrentTreeEnd;
        QString m_Id;

        SSlideInstanceIdMapIterator(TImportId inImportId, TIdMultiMap &inItems)
            : m_CurrentItems(nullptr)
            , m_CurrentTreeIdx(0)
            , m_CurrentTreeEnd(0)
            , m_Id(inImportId)
        {
            FindNextValidList(inItems);
        }
        void FindNextValidList(TIdMultiMap &inItems)
        {
            m_CurrentTreeIdx = 0;
            m_CurrentTreeEnd = 0;
            m_CurrentItems = nullptr;
            TIdMultiMap::const_iterator theFind = inItems.find(m_Id);
            if (theFind != inItems.end()) {
                m_CurrentItems = &(*theFind);
                m_CurrentTreeIdx = 0;
                m_CurrentTreeEnd = theFind->size();
            }
        }
        bool IsDone() const
        {
            if (m_CurrentTreeIdx >= m_CurrentTreeEnd)
                return true;
            return false;
        }
        void Next()
        {
            if (m_CurrentTreeIdx < m_CurrentTreeEnd) {
                ++m_CurrentTreeIdx;
            }
        }
        Qt3DSDMSlideHandle GetCurrentSlide() { return (*m_CurrentItems)[m_CurrentTreeIdx].first; }

        Qt3DSDMInstanceHandle GetCurrentInstance()
        {
            return (*m_CurrentItems)[m_CurrentTreeIdx].second;
        }
    };

    SComposerRefreshInterface(Q3DStudio::IDocumentEditor &editor, TIdMultiMap &inIdToInstanceMap,
                              const QString &docPath, const QString &inDestimportfile,
                              long inStartTime, CGraph &inAssetGraph)
        : SComposerImportBase(editor, docPath, inDestimportfile, inStartTime)
        , m_IdToSlideInstances(inIdToInstanceMap)
        , m_HasError(false)
        , m_AssetGraph(inAssetGraph)
        , m_importObj(nullptr)
    {
    }

    void Release() override {}
    void BeginImport(Import &importObj) override { m_importObj = &importObj; }

    void RemoveChild(TImportId inParentId, TImportId inChildId) override
    {
        for (SSlideInstanceIdMapIterator theIterator(inParentId, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next()) {
            Qt3DSDMInstanceHandle theParent = theIterator.GetCurrentInstance();
            for (long idx = 0; idx < m_AssetGraph.GetChildCount(theParent); ++idx) {
                Qt3DSDMInstanceHandle theChild = m_AssetGraph.GetChild(theParent, idx);
                QString theImportId = m_Editor.GetImportId(theChild);
                if (m_Editor.GetAssociatedSlide(theChild) == theIterator.GetCurrentSlide()
                    && theImportId == inChildId) {
                    m_Editor.RemoveChild(theParent, theChild);
                    --idx;
                }
            }
        }
    }

    void RemoveInstance(TImportId inParentId) override
    {
        SSlideInstanceIdMapIterator theIterator(inParentId, m_IdToSlideInstances);
        if (!theIterator.IsDone()) {
            for (size_t parentIdx = 0, parentEnd = theIterator.m_CurrentTreeEnd;
                 parentIdx < parentEnd; ++parentIdx) {
                if (m_Editor.IsInstance(theIterator.GetCurrentInstance()))
                    m_Editor.DeleteInstance(theIterator.GetCurrentInstance());
            }
            m_IdToSlideInstances.erase(m_IdToSlideInstances.find(theIterator.m_Id));
        }
    }
    /**
     *	Note that instance properties that point to files (sourcepath generally) point to files
     *	relative to the import file.  You need to do combineBaseAndRelative with those files
     *	and the a new getRelativeFromBase with the final file in order to transfer data
     *	successfully.  The time to do this sort of thing is upon create or update instance.
     */
    void CreateRootInstance(TImportId /*inImportId*/, ComposerObjectTypes::Enum /*type*/) override {}
    // inParent may be null (or an invalid handle) if the instance doesn't have a parent (images)
    void CreateInstance(TImportId inImportId, ComposerObjectTypes::Enum type,
                                TImportId inParent) override
    {
        const QString &theInsertId(inImportId);
        TIdMultiMap::iterator theInserter(m_IdToSlideInstances.insert(
            theInsertId, QVector<QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>>()));

        for (SSlideInstanceIdMapIterator theIterator(inParent, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next()) {
            Qt3DSDMInstanceHandle theParent = theIterator.GetCurrentInstance();
            Qt3DSDMInstanceHandle newInstance =
                m_Editor.CreateSceneGraphInstance(type, theParent, theIterator.GetCurrentSlide());
            if (m_StartTime >= 0)
                m_Editor.SetSpecificInstancePropertyValue(0, newInstance,
                                                          QStringLiteral("starttime"), m_StartTime);
            m_Editor.SetSpecificInstancePropertyValue(0, newInstance, QStringLiteral("importid"),
                                                      std::make_shared<CDataStr>(inImportId));
            m_Editor.SetSpecificInstancePropertyValue(
                0, newInstance, QStringLiteral("importfile"),
                std::make_shared<CDataStr>(m_Relativeimportfile));
            insert_unique_qt(*theInserter,
                          QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>(theIterator.GetCurrentSlide(), newInstance));
        }
    }

    void createMaterial(const InstanceDesc &desc, TImportId inParent) override
    {
        QString materialName = desc.m_Id;
        Option<SValue> name = m_importObj->GetInstancePropertyValue(desc.m_Handle,
                                                                    ComposerPropertyNames::name);
        if (name.hasValue())
            materialName = qt3dsdm::get<QString>(*name);

        // Get a unique material name
        // Reuse a material name if previously imported from the same source
        QString filepath = m_Editor.getMaterialFilePath(materialName);
        int i = 1;
        const QString originalMaterialName = materialName;
        const QString importFile = QStringLiteral("importfile");
        while (QFileInfo(filepath).exists()) {
            i++;
            QString name;
            QMap<QString, QString> values;
            QMap<QString, QMap<QString, QString>> textureValues;
            m_Editor.getMaterialInfo(filepath, name, values, textureValues);
            if (values.contains(importFile) && values[importFile] == m_Relativeimportfile) {
                break;
            }
            materialName = originalMaterialName + QString::number(i);
            filepath = m_Editor.getMaterialFilePath(materialName);
        }

        TIdMultiMap::iterator matInserter(m_IdToSlideInstances.insert(
            desc.m_Id, QVector<QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>>()));

        // If previous material exists, remove its children
        auto material = m_Editor.getMaterial(materialName);
        if (material.Valid() && !m_createdMaterials.contains(material)) {
            std::vector<Qt3DSDMInstanceHandle> children;
            m_Editor.GetChildren(0, material, children);
            for (auto &child : children)
                m_Editor.RemoveChild(material, child);
        } else {
            material = m_Editor.getOrCreateMaterial(materialName);
        }

        if (!m_createdMaterials.contains(material)) {
            m_Editor.SetSpecificInstancePropertyValue(0, material, QStringLiteral("importid"),
                                                      SValue(QVariant(desc.m_Id)));
            m_Editor.SetSpecificInstancePropertyValue(
                        0, material, QStringLiteral("importfile"),
                        SValue(QVariant(m_Relativeimportfile)));
            // Insert material in the material container to the map
            // so that its properties are updated
            insert_unique_qt(*matInserter,
                          QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>(
                                 m_Editor.GetAssociatedSlide(material), material));
            m_createdMaterials.append(material);
        }

        const auto sourcePath = m_Editor.writeMaterialFile(material,
                                                           materialName,
                                                           true);

        // Actual material inside material container has been created
        // Next create the referenced material located inside the model
        QString refName = desc.m_Id;
        refName += QLatin1String("_ref");
        TIdMultiMap::iterator refInserter(m_IdToSlideInstances.insert(
            refName, QVector<QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>>()));

        for (SSlideInstanceIdMapIterator theIterator(inParent, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next()) {
            Qt3DSDMInstanceHandle parent = theIterator.GetCurrentInstance();
            std::vector<Qt3DSDMInstanceHandle> children;
            m_Editor.GetChildren(0, parent, children);
            Qt3DSDMInstanceHandle oldVersion;
            for (auto &child : children) {
                if (m_Editor.GetSourcePath(child) == sourcePath
                        && m_Editor.GetObjectTypeName(child) == "ReferencedMaterial") {
                    oldVersion = child;
                    break;
                }
            }
            Qt3DSDMInstanceHandle instance;
            // If an old referenced material with the same name exists, the new one is added
            // as a sibling so that the order remains the same. The old one is then deleted.
            if (oldVersion.Valid()) {
                instance = m_Editor.CreateSceneGraphInstance(
                            ComposerObjectTypes::ReferencedMaterial, oldVersion,
                            theIterator.GetCurrentSlide(), DocumentEditorInsertType::NextSibling,
                            CPt(), PRIMITIVETYPE_UNKNOWN, 0);
                 m_Editor.DeleteInstance(oldVersion);
            } else {
                instance = m_Editor.CreateSceneGraphInstance(
                            ComposerObjectTypes::ReferencedMaterial, parent,
                            theIterator.GetCurrentSlide(), DocumentEditorInsertType::LastChild,
                            CPt(), PRIMITIVETYPE_UNKNOWN, 0);
            }
            m_Editor.setMaterialReferenceByPath(instance, materialName);
            m_Editor.SetName(instance, materialName);
            m_Editor.setMaterialSourcePath(instance, sourcePath);
            // Insert the referenced material to the map
            // so that the child structure remains the same
            insert_unique_qt(*refInserter,
                             QPair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>(
                                 theIterator.GetCurrentSlide(), instance));
        }
    }

    // We guarantee that all instances will be created before their properties are updated thus you
    // can resolve references during this updateInstanceProperties call if necessary.
    void UpdateInstanceProperties(TImportId inInstance, const PropertyValue *propertBuffer,
                                  QT3DSU32 propertyBufferSize) override
    {
        for (SSlideInstanceIdMapIterator theIterator(inInstance, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next()) {
            Qt3DSDMInstanceHandle hdl = theIterator.GetCurrentInstance();
            for (QT3DSU32 idx = 0; idx < propertyBufferSize; ++idx) {
                const PropertyValue &value(propertBuffer[idx]);
                SValue theValue(value.m_Value);

                DataModelDataType::Value theType = GetValueType(theValue);
                if (value.m_Name == ComposerPropertyNames::sourcepath) {
                    // re-work the path to be relative to where the main document
                    // is saved instead of where the import result is saved
                    TDataStrPtr value = qt3dsdm::get<TDataStrPtr>(theValue);
                    if (value->GetLength()) {
                        QString valueStr(value->toQString());
                        QString fullPath =
                            Q3DStudio::CFilePath::CombineBaseAndRelative(m_DestImportDir, valueStr);
                        QString relativePath =
                            Q3DStudio::CFilePath::GetRelativePathFromBase(m_DocumentPath, fullPath);
                        theValue = std::make_shared<CDataStr>(relativePath);
                    }
                } else if (theType == DataModelDataType::StringRef) {
                    SStringRef theRef = get<SStringRef>(theValue);
                    SLong4 theGuid;
                    // We are going to cheat here and look for a child of the current instance
                    // in the current slide who has the same import id;
                    for (long childIdx = 0, childCount = m_AssetGraph.GetChildCount(hdl);
                         childIdx < childCount; ++childIdx) {
                        Qt3DSDMInstanceHandle target = m_AssetGraph.GetChild(hdl, childIdx);
                        if (m_Editor.GetAssociatedSlide(hdl) == theIterator.GetCurrentSlide()
                            && m_Editor.GetImportId(target) == QString::fromWCharArray(theRef.m_Id)) {
                            theGuid = m_Editor.GetGuidForInstance(target);
                            theValue = theGuid;
                        }
                    }
                }
                // Note that we explicitly set the property values on the instance,
                // not on any given slide.
                m_Editor.SetSpecificInstancePropertyValue(
                    0, hdl, ComposerPropertyNames::Convert(value.m_Name), theValue);
                if (value.m_Name != ComposerPropertyNames::name
                        && m_createdMaterials.contains(hdl)) {
                   m_Editor.SetSpecificInstancePropertyValue(
                       m_Editor.GetAssociatedSlide(hdl), hdl,
                               ComposerPropertyNames::Convert(value.m_Name), theValue);
                }
            }
        }
    }
    // This is called even for new instances where we told you the parent because
    // they may be out of order so if the child already has this parent relationship you need
    // to check the order and ensure that is also (somewhat) correct.
    void AddChild(TImportId parent, TImportId child, TImportId nextSiblingId) override
    {
        TIdMultiMap::iterator theParentList =
            m_IdToSlideInstances.find(parent);
        TIdMultiMap::iterator theChildList =
            m_IdToSlideInstances.find(child);
        if (theParentList == m_IdToSlideInstances.end()
            || theChildList == m_IdToSlideInstances.end())
            return;
        size_t numItems = qMin(theParentList->size(), theChildList->size());
        for (size_t idx = 0; idx < numItems; ++idx) {
            Qt3DSDMSlideHandle theParentSlide = (*theParentList)[idx].first;
            Qt3DSDMInstanceHandle theParent((*theParentList)[idx].second);
            Qt3DSDMInstanceHandle theChild((*theChildList)[idx].second);
            if (m_createdMaterials.contains(theChild))
                continue;
            Qt3DSDMInstanceHandle nextSibling;
            if (!nextSiblingId.isEmpty()) {
                for (long childIdx = 0, childCount = m_AssetGraph.GetChildCount(theParent);
                     childIdx < childCount; ++childIdx) {
                    Qt3DSDMInstanceHandle theSibling = m_AssetGraph.GetChild(theParent, childIdx);
                    if (m_Editor.GetAssociatedSlide(theSibling) == theParentSlide
                        && m_Editor.GetImportId(theSibling) == nextSiblingId) {
                        nextSibling = theSibling;
                        break;
                    }
                }
            }
            if (nextSibling.Valid())
                m_AssetGraph.MoveBefore(theChild, nextSibling);
            else
                m_AssetGraph.MoveTo(theChild, theParent, COpaquePosition::LAST);
        }
    }

    void RemoveAnimation(TImportId inInstance, const QString &propName, long propSubIndex) override
    {
        for (SSlideInstanceIdMapIterator theIterator(inInstance, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next())
            m_Editor.RemoveAnimation(theIterator.GetCurrentSlide(),
                                     theIterator.GetCurrentInstance(), propName, propSubIndex);
    }
    void UpdateAnimation(TImportId inInstance, const QString &propName, long propSubIndex,
                         EAnimationType animType, const float *animData, QT3DSU32 numFloats) override
    {
        for (SSlideInstanceIdMapIterator theIterator(inInstance, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next()) {
            if (m_Editor.AnimationExists(theIterator.GetCurrentSlide(),
                                         theIterator.GetCurrentInstance(), propName, propSubIndex)
                && m_Editor.IsAnimationArtistEdited(theIterator.GetCurrentSlide(),
                                                    theIterator.GetCurrentInstance(), propName,
                                                    propSubIndex)
                    == false) {
                Qt3DSDMAnimationHandle anim = m_Editor.CreateOrSetAnimation(
                    theIterator.GetCurrentSlide(), theIterator.GetCurrentInstance(), propName,
                    propSubIndex, animType, animData, numFloats, false);
                m_Editor.SetIsArtistEdited(anim, false);
            }
        }
    }

    void AddAnimation(TImportId inInstance, const QString &propName, long propSubIndex,
                      EAnimationType animType, const float *animData, QT3DSU32 numFloats) override
    {
        for (SSlideInstanceIdMapIterator theIterator(inInstance, m_IdToSlideInstances);
             theIterator.IsDone() == false; theIterator.Next()) {
            if (!m_Editor.AnimationExists(theIterator.GetCurrentSlide(),
                                          theIterator.GetCurrentInstance(), propName,
                                          propSubIndex)) {
                Qt3DSDMAnimationHandle anim = m_Editor.CreateOrSetAnimation(
                    theIterator.GetCurrentSlide(), theIterator.GetCurrentInstance(), propName,
                    propSubIndex, animType, animData, numFloats, false);
                m_Editor.SetIsArtistEdited(anim, false);
            }
        }
    }

    void EndImport() override {}
};
}

std::shared_ptr<IComposerEditorInterface> IComposerEditorInterface::CreateEditorInterface(
    Q3DStudio::IDocumentEditor &editor, qt3dsdm::CDataModelHandle parent,
    qt3dsdm::CDataModelHandle root, qt3dsdm::Qt3DSDMSlideHandle slide,
    const QString &docPath, const QString &destimportfile,
    long inStartTime)
{
    return std::make_shared<SComposerImportInterface>(std::ref(editor), parent, root, slide,
                                                        docPath, destimportfile, inStartTime);
}

// The refresh interface is setup to refresh multiple trees automatically
std::shared_ptr<IComposerEditor> IComposerEditorInterface::CreateEditorInterface(
        Q3DStudio::IDocumentEditor &editor, TIdMultiMap &inRoots, const QString &docPath,
        const QString &destimportfile, long inStartTime, CGraph &inAssetGraph)
{
    return std::make_shared<SComposerRefreshInterface>(
        std::ref(editor), std::ref(inRoots), docPath, destimportfile, inStartTime,
        std::ref(inAssetGraph));
}
