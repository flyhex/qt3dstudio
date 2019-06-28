/****************************************************************************
**
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

#include "InspectorControlModel.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "InspectorGroup.h"
#include "Qt3DSDMInspectorGroup.h"
#include "Qt3DSDMInspectorRow.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMInspectable.h"
#include "Qt3DSDMDataCore.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMSignals.h"
#include "CmdDataModelDeanimate.h"
#include "GuideInspectable.h"
#include "Qt3DSDMDataTypes.h"
#include "IObjectReferenceHelper.h"
#include "SlideSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentReader.h"
#include "IStudioRenderer.h"
#include "Dialogs.h"
#include "Dispatch.h"
#include "VariantsGroupModel.h"
#include "StudioProjectSettings.h"
#include "Literals.h"

#include <QtCore/qfileinfo.h>

static QStringList renderableItems()
{
    QStringList renderables;
    renderables.push_back(QObject::tr("No renderable item"));
    const CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    Q3DStudio::CString docDir = Q3DStudio::CString::fromQString(doc->GetDocumentDirectory());

    for (SubPresentationRecord r : qAsConst(g_StudioApp.m_subpresentations))
        renderables.push_back(r.m_id);

    // second step, find the renderable plugins.
    {
        Q3DStudio::CFilePath pluginDir
                = Q3DStudio::CFilePath::CombineBaseAndRelative(docDir, "plugins");
        if (pluginDir.Exists() && pluginDir.IsDirectory()) {
            std::vector<Q3DStudio::CFilePath> dirFiles;
            pluginDir.ListFilesAndDirectories(dirFiles);
            for (size_t idx = 0, end = dirFiles.size(); idx < end; ++idx) {
                if (dirFiles[idx].IsFile()) {
                    Q3DStudio::CFilePath relPath =
                        Q3DStudio::CFilePath::GetRelativePathFromBase(docDir, dirFiles[idx]);
                    renderables.push_back(relPath.toQString());
                }
            }
        }
    }
    std::sort(renderables.begin() + 1, renderables.end());
    return renderables;
}

static std::pair<bool, bool> getSlideCharacteristics(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                     const qt3dsdm::ISlideCore &slideCore,
                                                     const qt3dsdm::ISlideSystem &slideSystem)
{
    // Get the slide from the instance.
    qt3dsdm::Qt3DSDMSlideHandle slide = slideCore.GetSlideByInstance(instance);
    qt3dsdm::Qt3DSDMSlideHandle master = slideSystem.GetMasterSlide(slide);
    int index = int(slideSystem.GetSlideIndex(slide));
    int count = int(slideSystem.GetSlideCount(master));
    bool hasNextSlide = index > 0 && index < count - 1;
    bool hasPreviousSlide = index > 1;
    return std::make_pair(hasNextSlide, hasPreviousSlide);
}

InspectorControlModel::InspectorControlModel(VariantsGroupModel *variantsModel, QObject *parent)
    : QAbstractListModel(parent)
    , m_UpdatableEditor(*g_StudioApp.GetCore()->GetDoc())
    , m_variantsModel(variantsModel)
{
    m_modifiedProperty.first = 0;
    m_modifiedProperty.second = 0;
}

void InspectorControlModel::setInspectable(CInspectableBase *inInspectable)
{
    // Commit any pending transactions on selection change
    m_UpdatableEditor.CommitEditor();
    m_modifiedProperty.first = 0;
    m_modifiedProperty.second = 0;
    m_previouslyCommittedValue = {};

    if (m_inspectableBase != inInspectable) {
        m_inspectableBase = inInspectable;
        rebuildTree();
    }
}

CInspectableBase *InspectorControlModel::inspectable() const
{
    return m_inspectableBase;
}

qt3dsdm::Qt3DSDMInstanceHandle InspectorControlModel::getReferenceMaterial(
                                                                CInspectableBase *inspectable) const
{
    if (inspectable)
        return getBridge()->getMaterialReference(inspectable->getInstance());

    return 0;
}

void InspectorControlModel::notifyPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                  qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    auto doc = g_StudioApp.GetCore()->GetDoc();
    if (!getBridge()->IsSceneGraphInstance(inInstance))
        return;

    if (inProperty == getBridge()->getVariantsProperty(inInstance)) {
        // variants model is updated upon edit but this is needed to handle undoing
        m_variantsModel->refresh();
        return;
    }

    bool changed = false;
    for (int row = 0; row < m_groupElements.count(); ++row) {
        auto group = m_groupElements[row];
        for (int p = 0; p < group.controlElements.count(); ++p) {
            QVariant& element = group.controlElements[p];
            InspectorControlBase *property = element.value<InspectorControlBase *>();
            qt3dsdm::Qt3DSDMInstanceHandle imageInstance;
            if (property->m_dataType == qt3dsdm::DataModelDataType::Long4
                && property->m_property.Valid()) {
                imageInstance = doc->GetDocumentReader().GetImageInstanceForProperty(
                                                property->m_instance, property->m_property);
            }
            if (property->m_property == inProperty || imageInstance == inInstance) {
                updatePropertyValue(property);
                changed = true;
            }
        }
    }
    if (changed)
        Q_EMIT dataChanged(index(0), index(rowCount() - 1));
}

bool InspectorControlModel::hasInstanceProperty(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                qt3dsdm::Qt3DSDMPropertyHandle handle) const
{
    for (const auto &group : qAsConst(m_groupElements)) {
        for (const auto &element : qAsConst(group.controlElements)) {
            InspectorControlBase *property = element.value<InspectorControlBase *>();
            if (property->m_property == handle) {
                auto refInstance = getBridge()->getMaterialReference(property->m_instance);
                return property->m_instance == instance || refInstance == instance;
            }
        }
    }

    return false;
}

bool InspectorControlModel::isInsideMaterialContainer() const
{
    return isInsideMaterialContainer(m_inspectableBase);
}

bool InspectorControlModel::isInsideMaterialContainer(CInspectableBase *inspectable) const
{
    if (!inspectable || !inspectable->getInstance())
        return false;

    return getBridge()->isInsideMaterialContainer(inspectable->getInstance());
}

bool InspectorControlModel::isDefaultMaterial() const
{
    if (m_inspectableBase)
        return getBridge()->isDefaultMaterial(m_inspectableBase->getInstance());

    return false;
}

bool InspectorControlModel::isAnimatableMaterial() const
{
    return isAnimatableMaterial(m_inspectableBase);
}

bool InspectorControlModel::isAnimatableMaterial(CInspectableBase *inspectable) const
{
    if (!inspectable)
        return false;

    return inspectable->getObjectType() & (OBJTYPE_CUSTOMMATERIAL | OBJTYPE_MATERIAL);
}

bool InspectorControlModel::isBasicMaterial() const
{
    return isBasicMaterial(m_inspectableBase);
}

bool InspectorControlModel::isBasicMaterial(CInspectableBase *inspectable) const
{
    if (!inspectable)
        return false;

    if (inspectable->getObjectType() == OBJTYPE_REFERENCEDMATERIAL) {
        const auto instance = inspectable->getInstance();
        if (!instance.Valid())
            return false;

        const auto refMaterial = getBridge()->getMaterialReference(instance);
        if (refMaterial.Valid() && getBridge()->isInsideMaterialContainer(refMaterial))
            return true;
    }

    return false;
}

bool InspectorControlModel::isMaterial() const
{
    if (m_inspectableBase)
        return m_inspectableBase->getObjectType() & OBJTYPE_IS_MATERIAL;

    return false;
}

void InspectorControlModel::addMaterial()
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMInstanceHandle instance = m_inspectableBase->getInstance();
    if (!instance.Valid())
        return;

    QString path = doc->getSceneEditor()->getMaterialDirectoryPath() + QStringLiteral("/Material");
    QString extension = QStringLiteral(".materialdef");

    auto absPath = path + extension;
    int i = 1;
    while (QFileInfo(absPath).exists()) {
        i++;
        absPath = path + QString::number(i) + extension;
    }

    qt3dsdm::Qt3DSDMInstanceHandle newMaterial;
    {
        newMaterial = Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, {})
                      ->getOrCreateMaterial(absPath, false);
    }
    // Several aspects of the editor are not updated correctly
    // if the data core is changed without a transaction
    // The above scope completes the transaction for creating a new material
    // Next the added undo has to be popped from the stack
    // TODO: Find a way to update the editor fully without a transaction
    g_StudioApp.GetCore()->GetCmdStack()->RemoveLastUndo();

    saveIfMaterial(newMaterial);

    Q3DStudio::ScopedDocumentEditor sceneEditor(
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, tr("Set Material Type")));
    doc->SelectDataModelObject(newMaterial);

    const auto type = getBridge()->GetObjectType(instance);
    if (type == OBJTYPE_REFERENCEDMATERIAL) {
        sceneEditor->setMaterialReferenceByPath(instance, absPath);
        const auto relPath = doc->GetRelativePathToDoc(absPath);
        sceneEditor->setMaterialSourcePath(instance, Q3DStudio::CString::fromQString(relPath));
        sceneEditor->SetName(instance, getBridge()->GetName(newMaterial, true));

        doc->GetStudioSystem()->GetFullSystemSignalSender()->SendInstancePropertyValue(
                                                        instance, getBridge()->GetNameProperty());
    }
}

void InspectorControlModel::duplicateMaterial()
{
    qt3dsdm::Qt3DSDMInstanceHandle instance = m_inspectableBase->getInstance();
    if (!instance.Valid())
        return;

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto type = m_inspectableBase->getObjectType();

    if (type & ~OBJTYPE_IS_MATERIAL)
        return;

    auto material = instance;
    if (type == OBJTYPE_REFERENCEDMATERIAL)
        material = getReferenceMaterial(m_inspectableBase);

    if (material.Valid()) {
        const auto sceneEditor = doc->getSceneEditor();
        auto originalMaterialName = sceneEditor->GetName(material).toQString()
                                    + QStringLiteral(" Copy");
        int slashIndex = originalMaterialName.lastIndexOf(QLatin1Char('/'));
        if (slashIndex != -1)
            originalMaterialName = originalMaterialName.mid(slashIndex + 1);
        auto materialName = originalMaterialName;
        int i = 1;
        auto absPath = sceneEditor->getMaterialFilePath(materialName);
        while (QFileInfo(absPath).exists()) {
            i++;
            materialName = originalMaterialName + QString::number(i);
            absPath = sceneEditor->getMaterialFilePath(materialName);
        }

        qt3dsdm::Qt3DSDMInstanceHandle duplicate;
        {
            Q3DStudio::ScopedDocumentEditor scopedEditor(
                        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, {}));
            duplicate = scopedEditor->getOrCreateMaterial(materialName, false);
            scopedEditor->copyMaterialProperties(material, duplicate);
        }
        // Several aspects of the editor are not updated correctly
        // if the data core is changed without a transaction
        // The above scope completes the transaction for creating a new material
        // Next the added undo has to be popped from the stack
        // TODO: Find a way to update the editor fully without a transaction
        g_StudioApp.GetCore()->GetCmdStack()->RemoveLastUndo();

        saveIfMaterial(duplicate);

        Q3DStudio::ScopedDocumentEditor scopedEditor(
                    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, tr("Set Material Type")));
        doc->SelectDataModelObject(duplicate);

        if (type == OBJTYPE_REFERENCEDMATERIAL) {
            scopedEditor->setMaterialReferenceByPath(instance, absPath);
            const auto relPath = doc->GetRelativePathToDoc(absPath);
            scopedEditor->setMaterialSourcePath(instance, Q3DStudio::CString::fromQString(relPath));
            scopedEditor->SetName(instance, getBridge()->GetName(duplicate, true));
            doc->GetStudioSystem()->GetFullSystemSignalSender()->SendInstancePropertyValue(
                                                        instance, getBridge()->GetNameProperty());
        }
    }
}

void InspectorControlModel::updateMaterialValues(const QStringList &values, int elementIndex,
                                                 bool updatingShaders)
{
    // Find if there are any material items and update the values of those
    int startIndex = 0;
    bool isReferenced = !isAnimatableMaterial() && updatingShaders;
    if (isReferenced && m_groupElements.count() > 0)
        startIndex = m_groupElements.count() - 1; // Update the last group for referenced materials
    for (int row = startIndex; row < m_groupElements.count(); ++row) {
        const CInspectorGroup *inspectorGroup = m_inspectableBase->getGroup(row);
        const auto group = static_cast<const Qt3DSDMInspectorGroup *>(inspectorGroup);
        if (group->isMaterial() || isReferenced) {
            if (m_groupElements[row].controlElements.size()) {
                auto item = m_groupElements[row].controlElements[elementIndex]
                            .value<InspectorControlBase *>();
                item->m_values = values;
                Q_EMIT item->valuesChanged();
                // Changing values resets the selected index, so pretend the value has also changed
                Q_EMIT item->valueChanged();
            }
        }
    }
}

void InspectorControlModel::updateShaderValues()
{
    int index = 0;
    if (isAnimatableMaterial() && !isInsideMaterialContainer())
        index = 1;
    updateMaterialValues(shaderValues(), index, true);
}

void InspectorControlModel::updateMatDataValues()
{
    int index = 0;
    if (!isInsideMaterialContainer())
        index = 1;
    updateMaterialValues(matDataValues(), index);
}

void InspectorControlModel::setMaterials(std::vector<Q3DStudio::CFilePath> &materials)
{
    m_materials.clear();
    for (Q3DStudio::CFilePath &path : materials) {
        const QString absolutePath = g_StudioApp.GetCore()->GetDoc()->GetResolvedPathToDoc(path);
        const QString name = g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                                  .GetCustomMaterialName(absolutePath).toQString();

        m_materials.push_back({name, path.toQString()});
    }

    if (!isDefaultMaterial())
        updateShaderValues();
}

void InspectorControlModel::setMatDatas(const std::vector<Q3DStudio::CFilePath> &matDatas)
{
    m_matDatas.clear();

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    bool isDocModified = doc->isModified();
    const auto sceneEditor = doc->getSceneEditor();
    if (!sceneEditor)
        return;

    bool newMaterialSelected = false;
    for (const Q3DStudio::CFilePath &path : matDatas) {
        bool isNewFile = true;
        for (auto &oldPath : m_cachedMatDatas) {
            if (path.toQString() == oldPath.toQString()) {
                isNewFile = false;
                break;
            }
        }

        const QString relativePath = path.toQString();
        const Q3DStudio::CFilePath absolutePath
            = Q3DStudio::CFilePath::CombineBaseAndRelative(doc->GetDocumentDirectory(), path);

        QString name;
        QMap<QString, QString> values;
        QMap<QString, QMap<QString, QString>> textureValues;
        sceneEditor->getMaterialInfo(
                    absolutePath.toQString(), name, values, textureValues);

        m_matDatas.push_back({name, relativePath, values, textureValues});

        bool needRewrite = false;
        if (values.contains(QStringLiteral("path"))) {
            const QString oldPath = values[QStringLiteral("path")];
            needRewrite = oldPath != absolutePath.toQString();
            if (!QFileInfo(oldPath).exists()) {
                const auto instance = sceneEditor->getMaterial(oldPath);
                if (instance.Valid()) {
                    const QString oldName = sceneEditor->GetName(instance).toQString();
                    const QString newName = sceneEditor
                            ->getMaterialNameFromFilePath(relativePath);
                    const QString actualPath = sceneEditor
                            ->getFilePathFromMaterialName(oldName);
                    if (actualPath == oldPath) {
                        doc->queueMaterialRename(oldName, newName);
                        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, tr("Set Name"))
                                ->setMaterialNameByPath(instance, relativePath);
                    }
                }
            }
        }

        auto material = sceneEditor->getMaterial(relativePath);
        if (isNewFile && !newMaterialSelected && !material.Valid()) {
            {
                material = Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QString())
                        ->getOrCreateMaterial(relativePath, false);
            }
            // Several aspects of the editor are not updated correctly
            // if the data core is changed without a transaction
            // The above scope completes the transaction for creating a new material
            // Next the added undo has to be popped from the stack
            // TODO: Find a way to update the editor fully without a transaction
            g_StudioApp.GetCore()->GetCmdStack()->RemoveLastUndo();
        }

        if (material.Valid())
            sceneEditor->setMaterialValues(relativePath, values, textureValues);

        if (isNewFile && !newMaterialSelected) {
            doc->SelectDataModelObject(material);
            newMaterialSelected = true;
        }

        if (needRewrite && material.Valid())
            sceneEditor->writeMaterialFile(material, name, false, absolutePath.toQString());
    }

    if (isBasicMaterial())
        updateMatDataValues();

    sceneEditor->removeDeletedFromMaterialContainer();
    // Modified flag has to be restored because of the hidden transaction
    doc->SetModifiedFlag(isDocModified);

    m_cachedMatDatas = matDatas;
}

QString InspectorControlModel::getBasicMaterialString() const
{
    return QObject::tr("Basic Material");
}

QString InspectorControlModel::getAnimatableMaterialString() const
{
    return QObject::tr("Animatable Material");
}

QString InspectorControlModel::getReferencedMaterialString() const
{
    return QObject::tr("Referenced Material");
}

QString InspectorControlModel::getStandardMaterialString() const
{
    return QObject::tr("Standard");
}

QString InspectorControlModel::getDefaultMaterialString() const
{
    return QObject::tr("Default");
}

bool InspectorControlModel::isGroupCollapsed(int groupIdx) const
{
    if (m_inspectableBase) {
        auto instance = m_inspectableBase->getInstance();
        if (instance && groupIdx > -1 && groupIdx < m_groupElements.size()
            && m_collapseMap.contains(instance)) {
            return m_collapseMap[instance].contains(groupIdx);
        }
    }

    return false;
}

void InspectorControlModel::updateGroupCollapseState(int groupIdx, bool isCollapsed)
{
    if (m_inspectableBase) {
        auto instance = m_inspectableBase->getInstance();
        if (instance && groupIdx > -1 && groupIdx < m_groupElements.size()) {
            if (isCollapsed)
                m_collapseMap[instance][groupIdx] = true;
            else
                m_collapseMap[instance].remove(groupIdx);
        }
    }
}

void InspectorControlModel::updateFontValues(InspectorControlBase *element) const
{
    // Find if there are any font items and update the values of those
    QVector<InspectorControlBase *> fontElements;
    if (element) {
        fontElements.append(element);
    } else {
        for (int row = 0; row < m_groupElements.count(); ++row) {
            auto group = m_groupElements[row];
            for (int p = 0; p < group.controlElements.count(); ++p) {
                QVariant &element = group.controlElements[p];
                InspectorControlBase *property = element.value<InspectorControlBase *>();
                if (property->m_propertyType == qt3dsdm::AdditionalMetaDataType::Font)
                    fontElements.append(property);
            }
        }
    }

    if (fontElements.size()) {
        std::vector<QString> fontNames;
        g_StudioApp.GetCore()->GetDoc()->GetProjectFonts(fontNames);
        QStringList possibleValues;
        for (const auto &fontName : fontNames)
            possibleValues.append(fontName);
        for (auto fontElement : qAsConst(fontElements)) {
            fontElement->m_values = possibleValues;
            Q_EMIT fontElement->valuesChanged();
            // Changing values resets the selected index, so pretend the value has also changed
            Q_EMIT fontElement->valueChanged();
        }
    }
}

QStringList InspectorControlModel::materialTypeValues() const
{
    QStringList values;
    values.push_back(getBasicMaterialString());
    values.push_back(getAnimatableMaterialString());
    values.push_back(getReferencedMaterialString());
    return values;
}

QStringList InspectorControlModel::shaderValues() const
{
    QStringList values;
    values.push_back(getStandardMaterialString());
    for (size_t matIdx = 0, end = m_materials.size(); matIdx < end; ++matIdx)
        values.push_back(m_materials[matIdx].m_name);
    return values;
}

QStringList InspectorControlModel::matDataValues() const
{
    QStringList values;
    QStringList names;
    const QString defaultMaterialShownName = getDefaultMaterialString();
    values.push_back(defaultMaterialShownName);
    names.push_back(defaultMaterialShownName);
    for (size_t matIdx = 0, end = m_matDatas.size(); matIdx < end; ++matIdx) {
        QString shownName = m_matDatas[matIdx].m_name;
        int slashIndex = shownName.lastIndexOf(QLatin1Char('/'));
        if (slashIndex != -1)
            shownName = shownName.mid(slashIndex + 1);
        if (names.contains(shownName))
            shownName += QLatin1String(" (") + m_matDatas[matIdx].m_relativePath + QLatin1Char(')');
        else
            names.push_back(shownName);
        values.push_back(shownName);
    }
    return values;
}

InspectorControlBase *InspectorControlModel::createMaterialTypeItem(
        Qt3DSDMInspectable *inspectable, int groupIndex)
{
    InspectorControlBase *item = new InspectorControlBase;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);

    item->m_title = tr("Material Type");
    item->m_dataType = qt3dsdm::DataModelDataType::StringRef;
    item->m_propertyType = qt3dsdm::AdditionalMetaDataType::None;
    item->m_tooltip = tr("Type of material being used");
    item->m_animatable = false;
    item->m_values = materialTypeValues();

    switch (inspectable->getObjectType()) {
    case OBJTYPE_MATERIAL:
    case OBJTYPE_CUSTOMMATERIAL:
        item->m_value = getAnimatableMaterialString();
        break;

    case OBJTYPE_REFERENCEDMATERIAL: {
        QString sourcePath = getBridge()->GetSourcePath(item->m_instance);
        item->m_value = sourcePath == getBridge()->getDefaultMaterialName()
                        ? getBasicMaterialString()
                        : getReferencedMaterialString();

        for (auto &mData : m_matDatas) {
            if (QString::compare(mData.m_relativePath, sourcePath, Qt::CaseInsensitive) == 0) {
                item->m_value = getBasicMaterialString();
                break;
            }
        }
    } break;

    default:
        break;
    }

    return item;
}

InspectorControlBase *InspectorControlModel::createShaderItem(
        Qt3DSDMInspectable *inspectable, int groupIndex)
{
    InspectorControlBase *item = new InspectorControlBase;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);

    item->m_title = tr("Shader");
    item->m_dataType = qt3dsdm::DataModelDataType::StringRef;
    item->m_propertyType = qt3dsdm::AdditionalMetaDataType::Renderable;
    item->m_tooltip = tr("Shader being used");
    item->m_animatable = false;

    const QStringList values = shaderValues();
    item->m_values = values;

    QString sourcePath = getBridge()->GetSourcePath(item->m_instance);

    item->m_value = values[0];
    for (int i = 0, end = int(m_materials.size()); i < end; ++i) {
        if (m_materials[i].m_relativePath == sourcePath) {
            item->m_value = values[i + 1]; // + 1 for Standard shader
            break;
        }
    }

    return item;
}

InspectorControlBase *InspectorControlModel::createMatDataItem(
        Qt3DSDMInspectable *inspectable, int groupIndex)
{
    InspectorControlBase *item = new InspectorControlBase;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);

    item->m_title = tr("Source Material");
    item->m_dataType = qt3dsdm::DataModelDataType::StringRef;
    item->m_propertyType = qt3dsdm::AdditionalMetaDataType::ObjectRef;
    item->m_tooltip = tr("Source material definitions used");
    item->m_animatable = false;

    const QStringList values = matDataValues();
    item->m_values = values;

    QString sourcePath = getBridge()->GetSourcePath(item->m_instance);

    item->m_value = getDefaultMaterialString();
    for (int i = 0, end = int(m_matDatas.size()); i < end; ++i) {
        if (QString::compare(m_matDatas[i].m_relativePath, sourcePath, Qt::CaseInsensitive) == 0) {
            item->m_value = values[i + 1]; // + 1 for Default basic material
            break;
        }
    }

    return item;
}

InspectorControlBase* InspectorControlModel::createItem(Qt3DSDMInspectable *inspectable,
                                                        Q3DStudio::Qt3DSDMInspectorRow *row,
                                                        int groupIndex)
{
    return createItem(inspectable, row->GetMetaDataPropertyInfo(), groupIndex);
}

InspectorControlBase* InspectorControlModel::createItem(Qt3DSDMInspectable *inspectable,
                                                        const qt3dsdm::SMetaDataPropertyInfo &metaProperty,
                                                        int groupIndex)
{
    const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    if (metaProperty.m_IsHidden)
        return nullptr;

    Q3DStudio::CString title;
    title.Assign(metaProperty.m_FormalName.c_str());
    if (title.IsEmpty())
        title.Assign(metaProperty.m_Name.c_str());

    // Hide name for basic materials
    if (title == "Name" && isBasicMaterial())
        return nullptr;

    InspectorControlBase *item = new InspectorControlBase;
    item->m_property = metaProperty.m_Property;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);
    item->m_metaProperty = metaProperty;

    item->m_title = title.toQString();

    const auto propertySystem = studio->GetPropertySystem();
    item->m_dataType = propertySystem->GetDataType(metaProperty.m_Property);
    item->m_propertyType = static_cast<qt3dsdm::AdditionalMetaDataType::Value>
            (propertySystem->GetAdditionalMetaDataType(item->m_instance, metaProperty.m_Property));
    item->m_tooltip = Q3DStudio::CString(metaProperty.m_Description.c_str()).toQString();
    // \n is parsed as \\n from the material and effect files. Replace them to fix multi-line
    // tooltips
    item->m_tooltip.replace(QLatin1String("\\n"), QLatin1String("\n"));

    item->m_animatable = metaProperty.m_Animatable &&
            studio->GetAnimationSystem()->IsPropertyAnimatable(item->m_instance,
                                                               metaProperty.m_Property);
    // If a property is animatable, it should be controllable in addition to
    // properties explicitly set as controllable in metadata
    item->m_controllable = item->m_animatable || metaProperty.m_Controllable;

    // disable IBL Override for reference and 'default basic' materials
    if (item->m_title == QLatin1String("IBL Override")
        && getBridge()->GetObjectType(item->m_instance) == OBJTYPE_REFERENCEDMATERIAL
        && (!isBasicMaterial() || getBridge()->isDefaultMaterial(item->m_instance))) {
        item->m_enabled = false;
    }
    auto signalProvider = studio->GetFullSystemSignalProvider();
    if (item->m_animatable) {
        item->m_animated = studio->GetAnimationSystem()->IsPropertyAnimated(item->m_instance,
                                                                            metaProperty.m_Property);

        // Update the Animate Toggle on undo/redo
        item->m_connections.push_back(signalProvider->ConnectAnimationCreated(
                                          std::bind(&InspectorControlModel::updateAnimateToggleState,
                                                    this, item)));

        item->m_connections.push_back(signalProvider->ConnectAnimationDeleted(
                                          std::bind(&InspectorControlModel::updateAnimateToggleState,
                                                    this, item)));
    }

    if (item->m_controllable) {
        // Set the name of current controller
        item->m_controller = currentControllerValue(item->m_instance, item->m_property);
        // Update UI icon state and tooltip
        updateControlledToggleState(item);
        item->m_connections.push_back(signalProvider->ConnectControlledToggled(
            std::bind(&InspectorControlModel::updateControlledToggleState,
                      this, item)));
    }

    // synchronize the value itself
    updatePropertyValue(item);
    return item;
}

qt3dsdm::SValue InspectorControlModel::currentPropertyValue(long instance, int handle) const
{
    auto propertySystem = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetPropertySystem();
    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(instance, handle,  value);

    return value;
}

QString InspectorControlModel::currentControllerValue(long instance, int handle) const
{
    return g_StudioApp.GetCore()->GetDoc()->GetCurrentController(instance, handle);
}

void InspectorControlModel::updateControlledToggleState(InspectorControlBase* inItem) const
{
    if (inItem->m_instance) {
        const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
        // toggle if controlledproperty contains the name of this property
        qt3dsdm::SValue currPropVal = currentPropertyValue(
            inItem->m_instance, studio->GetPropertySystem()->GetAggregateInstancePropertyByName(
                inItem->m_instance, qt3dsdm::TCharStr(L"controlledproperty")));
        Q3DStudio::CString currPropValStr;
        if (!currPropVal.empty())
            currPropValStr = qt3dsdm::get<qt3dsdm::TDataStrPtr>(currPropVal)->GetData();
        // Restore original tool tip from metadata when turning control off
        if (!currPropValStr.size()) {
            inItem->m_controlled = false;
            inItem->m_controller = "";
        } else {
           Q3DStudio::CString propName
               = studio->GetPropertySystem()->GetName(inItem->m_property).c_str();
           // Search specifically for whitespace followed with registered property name.
           // This avoids finding datainput with same name as the property, as datainput
           // name is always prepended with "$"
           long propNamePos = currPropValStr.find(" " + propName);
           if ((propNamePos == currPropValStr.ENDOFSTRING)
               && (propNamePos != 0)) {
               inItem->m_controlled = false;
               inItem->m_controller = "";
           } else {
               inItem->m_controlled = true;
               // controller name is prepended with "$" to differentiate from property
               // with same name. Reverse find specifically for $.
               long posCtrlr = currPropValStr.substr(0, propNamePos).ReverseFind("$");

               // this is the first controller - property pair in controlledproperty
               if (posCtrlr < 0)
                   posCtrlr = 0;

               // remove $ from controller name for showing it in UI
               posCtrlr++;
               const QString ctrlName = currPropValStr.substr(
                   posCtrlr, propNamePos - posCtrlr).toQString();

               inItem->m_controller = ctrlName;
           }
        }

        Q_EMIT inItem->tooltipChanged();
        // Emit signal always to trigger updating of controller name in UI
        // also when user switches from one controller to another
        Q_EMIT inItem->controlledChanged();
    }
}

void InspectorControlModel::updateAnimateToggleState(InspectorControlBase* inItem)
{
    const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    bool animated = studio->GetAnimationSystem()->IsPropertyAnimated(inItem->m_instance,
                                                                     inItem->m_property);
    if (animated != inItem->m_animated) {
        inItem->m_animated = animated;
        Q_EMIT inItem->animatedChanged();
    }
}

bool InspectorControlModel::isTreeRebuildRequired(CInspectableBase* inspectBase)
{
    if (inspectBase != m_inspectableBase || !inspectBase)
        return true;

    long theCount = m_inspectableBase->getGroupCount();
    auto refMaterial = getReferenceMaterial(inspectBase);
    if (refMaterial != m_refMaterial)
        return true;
    long refMaterialGroupCount = 0;
    if (refMaterial.Valid())
        refMaterialGroupCount = 1; // Only the last group of the refMaterial is used

    if (m_groupElements.size() != theCount + refMaterialGroupCount)
        return true;

    for (long theIndex = 0; theIndex < theCount; ++theIndex) {
        const CInspectorGroup *theInspectorGroup = m_inspectableBase->getGroup(theIndex);
        if (m_groupElements.at(theIndex).groupTitle != theInspectorGroup->GetName())
            return true;
    }

    return false;
}

bool InspectorControlModel::isGroupRebuildRequired(CInspectableBase *inspectable,
                                                   int theIndex) const
{
    Q_ASSERT(theIndex < m_groupElements.size());
    const CInspectorGroup *theInspectorGroup = inspectable->getGroup(theIndex);
    const auto existingGroup = m_groupElements.at(theIndex);
    if (existingGroup.groupTitle != theInspectorGroup->GetName())
        return true;

    if (inspectable->getObjectType() != OBJTYPE_GUIDE) {
        const auto dmInspectable = static_cast<Qt3DSDMInspectable *>(inspectable);
        const auto group = static_cast<const Qt3DSDMInspectorGroup *>(theInspectorGroup);
        int existingIndex = 0;
        if (group && group->isMaterial()) {
            auto i = existingGroup.controlElements.at(existingIndex++)
                                                  .value<InspectorControlBase *>();
            if (i->m_instance != dmInspectable->GetGroupInstance(theIndex))
                return true;

            if (!isInsideMaterialContainer())
                existingIndex++; // Add material type dropdown to existing elements
        }

        if ((existingGroup.controlElements.size() - existingIndex) != group->GetRows().size())
            return true;

        for (const auto row : group->GetRows()) {
            auto i = existingGroup.controlElements.at(existingIndex++)
                                                  .value<InspectorControlBase *>();
            if (i->m_instance != dmInspectable->GetGroupInstance(theIndex))
                return true;

            if (i->m_property != row->GetMetaDataPropertyInfo().m_Property)
                return true;
        }
    }

    return false;
}

CClientDataModelBridge *InspectorControlModel::getBridge() const
{
    return g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
}

CInspectableBase *InspectorControlModel::getInspectableFromInstance(
                                                        qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::ISlideSystem *slideSystem = doc->GetStudioSystem()->GetSlideSystem();

    if (doc->GetDocumentReader().IsInstance(inInstance)) {
        qt3dsdm::Qt3DSDMSlideHandle activeSlide = doc->GetActiveSlide();
        qt3dsdm::Qt3DSDMInstanceHandle activeSlideInstance = 0;

        // Slide, scene or component
        if (inInstance == getBridge()->GetOwningComponentInstance(activeSlide))
            activeSlideInstance = slideSystem->GetSlideInstance(activeSlide);

        return new Qt3DSDMInspectable(inInstance, activeSlideInstance);
    }

    return nullptr;
}

auto InspectorControlModel::computeTree(CInspectableBase *inspectableBase)
    -> QVector<GroupInspectorControl>
{
    QVector<GroupInspectorControl> result;

    if (inspectableBase) {
        qt3dsdm::Qt3DSDMInstanceHandle instance = inspectableBase->getInstance();

        if (isDefaultMaterial()) {
            // for default materials skip the first 2 groups (Basic Properties, Lighting) and only
            // show the Material group
            result.append(computeGroup(inspectableBase, 2));
            result[0].groupInfo = tr("\nDefault material cannot be edited. Create new or import "
                                     "material, then apply.");
        } else {
            // disable animation for basic materials, this applies only when editing the basic
            // material from the project palette.
            bool disableAnim = instance.Valid()
                    && getBridge()->isInsideMaterialContainer(instance);
            long groupCount = inspectableBase->getGroupCount();
            for (long idx = 0; idx < groupCount; ++idx)
                result.append(computeGroup(inspectableBase, idx, disableAnim));
        }

        // show original material properties for referenced materials
        auto refMaterial = getReferenceMaterial(inspectableBase);
        if (refMaterial.Valid()) {
            auto refMaterialInspectable = getInspectableFromInstance(refMaterial);
            if (refMaterialInspectable) {
                QString materialSrcPath;
                if (instance.Valid())
                    materialSrcPath = getBridge()->GetSourcePath(instance);

                if (materialSrcPath != getBridge()->getDefaultMaterialName()
                    && getBridge()->GetSourcePath(refMaterial)
                       != getBridge()->getDefaultMaterialName()) {
                    result.append(computeGroup(refMaterialInspectable,
                                               refMaterialInspectable->getGroupCount() - 1,
                                               true, true));
                }
            }
        }
    }

    return result;
}

auto InspectorControlModel::computeGroup(CInspectableBase *inspectable, int index,
                                         bool disableAnimation, bool isReference)
    -> GroupInspectorControl
{
    CInspectorGroup *theInspectorGroup = inspectable->getGroup(index);
    GroupInspectorControl result;
    result.groupTitle = theInspectorGroup->GetName();
    result.groupInfo.clear();

    if (isReference)
        result.groupTitle += tr(" (Reference)");

    if (inspectable->getObjectType() != OBJTYPE_GUIDE) {
        const auto dmInspectable = static_cast<Qt3DSDMInspectable *>(inspectable);
        const auto group = static_cast<Qt3DSDMInspectorGroup *>(theInspectorGroup);

        bool isBasicMat = isBasicMaterial(dmInspectable);
        if (group->isMaterial()) {
            InspectorControlBase *item = nullptr;

            if (!isInsideMaterialContainer(dmInspectable) && !isReference) {
                item = createMaterialTypeItem(dmInspectable, index);
                if (item)
                    result.controlElements.push_back(QVariant::fromValue(item));
            }

            if (isAnimatableMaterial(dmInspectable)) {
                item = createShaderItem(dmInspectable, index);
                if (item)
                    result.controlElements.push_back(QVariant::fromValue(item));
            } else if (isBasicMat) {
                item = createMatDataItem(dmInspectable, index);
                if (item)
                    result.controlElements.push_back(QVariant::fromValue(item));
            }
        }

        for (const auto row : group->GetRows()) {
            InspectorControlBase *item = createItem(dmInspectable, row, index);
            if (!item)
                continue;

            if (disableAnimation)
                item->m_animatable = false;

            if (!isBasicMat || item->m_title != getReferencedMaterialString())
                result.controlElements.push_back(QVariant::fromValue(item));
        }
    } else { // Guide
        const auto guideInspectable = static_cast<GuideInspectable *>(inspectable);
        // Guide properties don't come from metadata as they are not actual objects
        m_guideInspectable = guideInspectable;
        const auto &properties = m_guideInspectable->properties();
        for (int i = 0, count = int(properties.size()); i < count; ++i) {
            auto &prop = properties[i];
            InspectorControlBase *item = new InspectorControlBase;
            item->m_title = prop->GetInspectableFormalName();
            item->m_dataType = prop->GetInspectableType();
            item->m_propertyType = prop->GetInspectableAdditionalType();
            item->m_tooltip = prop->GetInspectableDescription();
            item->m_animatable = false;
            item->m_controllable = false;
            item->m_property = i + 1; // Zero property is considered invalid, so +1
            result.controlElements.push_back(QVariant::fromValue(item));
            updatePropertyValue(item);
        }
    }

    return result;
}

void InspectorControlModel::rebuildTree()
{
    beginResetModel();
    m_guideInspectable = nullptr;
    QVector<QObject *> deleteVector;
    for (int i = 0; i < m_groupElements.count(); ++i) {
        auto group = m_groupElements[i];
        for (int p = 0; p < group.controlElements.count(); ++p)
            deleteVector.append(group.controlElements[p].value<QObject *>());
    }
    m_groupElements = computeTree(m_inspectableBase);
    endResetModel();

    // Clean the old objects after reset is done so that qml will not freak out about null pointers
    for (int i = 0; i < deleteVector.count(); ++i)
        deleteVector[i]->deleteLater();

    m_refMaterial = getReferenceMaterial(m_inspectableBase);
}

int InspectorControlModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_groupElements.count();
}

void InspectorControlModel::updatePropertyValue(InspectorControlBase *element) const
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studioSystem = doc->GetStudioSystem();
    const auto propertySystem = studioSystem->GetPropertySystem();
    qt3dsdm::SValue value;
    auto instance = element->m_instance;
    qt3dsdm::Option<qt3dsdm::SMetaDataPropertyInfo> info;

    // For ref materials update IBL Override from the referenced material. This applies only for
    // basic materials as IBL Override is disabled for referenced (and default basic) materials.
    if (element->m_property
            == getBridge()->GetObjectDefinitions().m_MaterialBase.m_IblProbe.m_Property) {
        int refInstance = getBridge()->getMaterialReference(instance);
        if (refInstance)
            instance = refInstance;
    }

    if (m_guideInspectable) {
        value = m_guideInspectable->properties()
                [handleToGuidePropIndex(element->m_property)]->GetInspectableData();
    } else {
        if (!propertySystem->HandleValid(instance))
            return;
        propertySystem->GetInstancePropertyValue(instance, element->m_property, value);

        if (value.getType() == qt3dsdm::DataModelDataType::None)
            return;

        const auto metaDataProvider = doc->GetStudioSystem()->GetActionMetaData();
        info = metaDataProvider->GetMetaDataPropertyInfo(
                    metaDataProvider->GetMetaDataProperty(instance, element->m_property));
    }

    bool skipEmits = false;
    switch (element->m_dataType) {
    case qt3dsdm::DataModelDataType::String: {
        QString stringValue = qt3dsdm::get<QString>(value);
        if (getBridge()->isInsideMaterialContainer(element->m_instance)) {
            int index = stringValue.lastIndexOf(QLatin1Char('/'));
            if (index != -1)
                stringValue = stringValue.mid(index + 1);
        }

        element->m_value = stringValue;
    }
    Q_FALLTHROUGH(); // fall-through for other String-derived datatypes

    case qt3dsdm::DataModelDataType::StringOrInt:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::StringList) {
            QStringList stringlist;
            if (m_guideInspectable) {
                const auto strings = m_guideInspectable->properties()
                        [handleToGuidePropIndex(element->m_property)]->GetInspectableList();
                for (auto &str : strings)
                    stringlist.append(QString::fromWCharArray(str.wide_str()));
            } else {
                stringlist = qt3dsdm::get<QStringList>(info->m_MetaDataData);
            }
            auto slideSystem = studioSystem->GetSlideSystem();

            if (element->m_title == QLatin1String("Play Mode")) {
                std::pair<bool, bool> slideData(
                    getSlideCharacteristics(element->m_instance, *studioSystem->GetSlideCore(),
                                            *slideSystem));
                bool hasNextSlide(slideData.first);
                bool hasPreviousSlide(slideData.second);
                if (!hasNextSlide && !hasPreviousSlide)
                    stringlist.removeAll("Play Through To...");
            } else if (element->m_title == QLatin1String("Play Through To")) {
                // the code duplication is intentional as we may ask for slide characteristics
                // only if the property refers to slides
                std::pair<bool, bool> slideData(
                    getSlideCharacteristics(element->m_instance, *studioSystem->GetSlideCore(),
                                            *slideSystem));
                bool hasNextSlide(slideData.first);
                bool hasPreviousSlide(slideData.second);
                if (!hasNextSlide)
                    stringlist.removeAll("Next");
                if (!hasPreviousSlide)
                    stringlist.removeAll("Previous");

                auto itemCount = stringlist.count();
                QString listOpt;
                int selectedSlideHandle = 0;
                int selectedIndex = -1;
                qt3dsdm::SStringOrInt stringOrInt = qt3dsdm::get<qt3dsdm::SStringOrInt>(value);
                if (stringOrInt.GetType() == qt3dsdm::SStringOrIntTypes::String)
                    listOpt = QString::fromWCharArray(qt3dsdm::get<qt3dsdm::TDataStrPtr>
                                                      (stringOrInt.m_Value)->GetData());
                else
                    selectedSlideHandle = qt3dsdm::get<long>(stringOrInt.m_Value);

                selectedIndex = stringlist.indexOf(listOpt);
                // Add the slide names (exclude the master slide)
                auto slideHandle = slideSystem->GetSlideByInstance(instance);
                auto masterSlide = slideSystem->GetMasterSlide(slideHandle);
                long slideCount = long(slideSystem->GetSlideCount(masterSlide));
                for (long slideIndex = 1; slideIndex < slideCount; ++slideIndex) {
                    auto currentSlide = slideSystem->GetSlideByIndex(masterSlide, slideIndex);
                    auto currentInstance = slideSystem->GetSlideInstance(currentSlide);

                    QString slideName = getBridge()->GetName(currentInstance).toQString();
                    //hack to add a separator before the item
                    if (slideIndex == 1 && itemCount > 0)
                        slideName += "|separator";
                    stringlist.append(slideName);

                    if (currentSlide.GetHandleValue() == selectedSlideHandle)
                        selectedIndex = slideIndex + itemCount - 1;
                }

                element->m_value = QString(selectedIndex > 0 ? stringlist[selectedIndex]
                                   : stringlist.first()).replace(QLatin1String("|separator"),
                                                                 QString());
            }
            element->m_values = stringlist;
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Import) {
            QStringList stringlist = qt3dsdm::get<QStringList>(info->m_MetaDataData);
            element->m_values = stringlist;
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Renderable) {
            element->m_values = renderableItems();
            if (element->m_value.toString().isEmpty())
                element->m_value = element->m_values.toStringList().at(0);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::MultiLine) {
            element->m_value = qt3dsdm::get<QString>(value);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Font) {
            updateFontValues(element);
            skipEmits = true; // updateFontValues handles emits in correct order
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Mesh) {
            QString meshValue = QFileInfo(qt3dsdm::get<QString>(value)).fileName();
            element->m_value = meshValue.startsWith('#'_L1) ? meshValue.mid(1) : meshValue;
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Texture) {
            QFileInfo fileInfo(qt3dsdm::get<QString>(value));
            element->m_value = fileInfo.fileName();
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::PathBuffer) {
            element->m_value = qt3dsdm::get<QString>(value);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::String) {
            // Basic string already handled, do not warn about that.
            // If we hit any other datatypes then give a warning
        } else {
            qWarning() << "KDAB_TODO: InspectorControlModel::updatePropertyValue: need to implement:"
                       << element->m_dataType << " element->m_propertyType : "
                       << element->m_propertyType;
        }
        break;

    case qt3dsdm::DataModelDataType::StringRef:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::None) {
            element->m_value = qt3dsdm::get<QString>(value);
        }
        break;

    case qt3dsdm::DataModelDataType::Bool:
        element->m_value = qt3dsdm::get<bool>(value);
        break;

    case qt3dsdm::DataModelDataType::Long4:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Image) {
            qt3dsdm::Option<qt3dsdm::SLong4> guid = qt3dsdm::get<qt3dsdm::SLong4>(value);
            qt3dsdm::Qt3DSDMInstanceHandle imageInstance = doc->GetDocumentReader()
                    .GetInstanceForGuid(guid);
            if (imageInstance.Valid()) {
                Q3DStudio::CString path = doc->GetDocumentReader().GetSourcePath(imageInstance);
                Q3DStudio::CFilePath relPath(path);
                element->m_value = QVariant(relPath.GetFileName().toQString());
            } else {
                element->m_value = QVariant(QString());
            }
        } else {
            qWarning() << "KDAB_TODO: InspectorControlModel::updatePropertyValue: need to implement:"
                       << element->m_dataType << " " << element->m_title;
        }
        break;

    case qt3dsdm::DataModelDataType::Long:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Range) {
            element->m_value = qt3dsdm::get<int>(value);
            qt3dsdm::SMetaDataRange ranges;
            if (m_guideInspectable) {
                const auto prop = m_guideInspectable->properties()
                        [handleToGuidePropIndex(element->m_property)];
                ranges.m_min = prop->GetInspectableMin();
                ranges.m_max = prop->GetInspectableMax();
            } else {
                ranges = qt3dsdm::get<qt3dsdm::SMetaDataRange>(info->m_MetaDataData);
            }
            const QList<double> rangesValues{ranges.m_min, ranges.m_max, double(ranges.m_decimals)};
            element->m_values = QVariant::fromValue<QList<double> >(rangesValues);
        }
        else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::ShadowMapResolution) {
            element->m_value = qt3dsdm::get<int>(value);
        } else {
            qWarning() << "KDAB_TODO: InspectorControlModel::updatePropertyValue: need to implement:"
                       << element->m_dataType;
        }
        break;

    case qt3dsdm::DataModelDataType::Float3:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Color) {
            element->m_value = qt3dsdm::get<QColor>(value);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Rotation) {
            const QVector3D theFloat3 = qt3dsdm::get<QVector3D>(value);
            const QList<double> float3Values{theFloat3.x(), theFloat3.y(), theFloat3.z()};
            element->m_values = QVariant::fromValue<QList<double> >(float3Values);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::None) {
            const QVector3D theFloat3 = qt3dsdm::get<QVector3D>(value);
            const QList<double> float3Values{theFloat3.x(), theFloat3.y(), theFloat3.z()};
            element->m_values = QVariant::fromValue<QList<double> >(float3Values);
        }
        break;

    case qt3dsdm::DataModelDataType::Float4:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Color) {
            element->m_value = qt3dsdm::get<QColor>(value);
        }
        break;

    case qt3dsdm::DataModelDataType::Float2:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::None) {
            const QVector2D theFloat2 = qt3dsdm::get<QVector2D>(value);
            const QList<double> float2Values{theFloat2.x(), theFloat2.y()};
            element->m_values = QVariant::fromValue<QList<double> >(float2Values);
        } else {
            qWarning() << "TODO: InspectorControlModel::updatePropertyValue: need to implement:"
                       << element->m_dataType << element->m_propertyType;
        }
        break;

    case qt3dsdm::DataModelDataType::Float:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::None) {
            element->m_value = qt3dsdm::get<float>(value);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Range) {
            element->m_value = qt3dsdm::get<float>(value);
            const qt3dsdm::SMetaDataRange ranges = qt3dsdm::get<qt3dsdm::SMetaDataRange>(info->m_MetaDataData);
            const QList<double> rangesValues{ranges.m_min, ranges.m_max, double(ranges.m_decimals)};
            element->m_values = QVariant::fromValue<QList<double> >(rangesValues);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::FontSize) {
            element->m_value = qt3dsdm::get<float>(value);
        }
        break;

    case qt3dsdm::DataModelDataType::ObjectRef:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::ObjectRef) {
            IObjectReferenceHelper *objRefHelper = doc->GetDataModelObjectReferenceHelper();
            if (objRefHelper) {
                qt3dsdm::Qt3DSDMInstanceHandle refInstance = objRefHelper->Resolve(value, instance);
                QString refName = objRefHelper->LookupObjectFormalName(refInstance).toQString();
                if (getBridge()->IsReferencedMaterialInstance(instance) && !refName.isEmpty()) {
                    // get the material's object name (parent)
                    auto parentInstance = getBridge()->GetParentInstance(refInstance);
                    qt3dsdm::SValue vParent;
                    propertySystem->GetInstancePropertyValue(parentInstance,
                                   getBridge()->GetObjectDefinitions().m_Named.m_NameProp, vParent);
                    QString parentName = qt3dsdm::get<QString>(vParent);
                    refName.append(QLatin1String(" (") + parentName + QLatin1String(")"));
                }
                element->m_value = refName;
            }
        }
        break;

    default:
        qWarning() << "TODO: InspectorControlModel::updatePropertyValue: I've no idea how to handle this datatype"
                   << element->m_dataType;
        break;
    }

    if (!skipEmits) {
        Q_EMIT element->valueChanged();
        Q_EMIT element->valuesChanged();
    }

    // Controlled state must be manually set after undo operations,
    // as only the "controlledproperty" is restored in undo,
    // not the controlled flag nor the tooltip
    if (element->m_controllable)
          updateControlledToggleState(element);
}

void InspectorControlModel::refreshRenderables()
{
    for (int row = 0; row < m_groupElements.count(); ++row) {
        auto group = m_groupElements[row];
        for (int p = 0; p < group.controlElements.count(); ++p) {
            QVariant& element = group.controlElements[p];
            InspectorControlBase *property = element.value<InspectorControlBase *>();
            if (property->m_property.Valid()
                    && property->m_propertyType == qt3dsdm::AdditionalMetaDataType::Renderable) {
                updatePropertyValue(property);
            }
        }
    }
}

void InspectorControlModel::refreshTree()
{
    //check if the structure has changed
    if (isTreeRebuildRequired(m_inspectableBase)) {
        rebuildTree();
    } else {
        // group structure is intact, let's walk to see which rows changed
        QVector<QObject *> deleteVector;
        long theCount = m_inspectableBase->getGroupCount();
        for (long theIndex = 0; theIndex < theCount; ++theIndex) {
            if (isGroupRebuildRequired(m_inspectableBase, theIndex)) {
                auto group = m_groupElements[theIndex];
                for (int p = 0; p < group.controlElements.count(); ++p)
                    deleteVector.append(group.controlElements[p].value<QObject *>());
                m_groupElements[theIndex] = computeGroup(m_inspectableBase, theIndex);
                Q_EMIT dataChanged(index(theIndex), index(theIndex));
            }
        }
    }
}

void InspectorControlModel::refresh()
{
    refreshTree();
    // update values
    for (int row = 0; row < m_groupElements.count(); ++row) {
        auto group = m_groupElements[row];
        for (int p = 0; p < group.controlElements.count(); ++p) {
            QVariant& element = group.controlElements[p];
            InspectorControlBase *property = element.value<InspectorControlBase *>();
            if (property->m_property.Valid()) {
                updatePropertyValue(property);
                updateControlledToggleState(property);
            }
        }
    }
    Q_EMIT dataChanged(index(0), index(rowCount() - 1));
}

void InspectorControlModel::saveIfMaterial(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    if (!instance.Valid())
        return;

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto sceneEditor = doc->getSceneEditor();

    const auto studio = doc->GetStudioSystem();
    EStudioObjectType type = getBridge()->GetObjectType(instance);

    auto material = instance;
    if (type == OBJTYPE_IMAGE)
        material = sceneEditor->GetParent(instance);

    if (!material.Valid())
        return;

    const auto refMaterial = getBridge()->getMaterialReference(material);
    if (refMaterial.Valid())
        material = refMaterial;

    if (!getBridge()->isInsideMaterialContainer(material))
        return;

    type = getBridge()->GetObjectType(material);

    if (type & (OBJTYPE_MATERIAL | OBJTYPE_CUSTOMMATERIAL)) {
        qt3dsdm::SValue value;
        studio->GetPropertySystem()->GetInstancePropertyValue(
            material, getBridge()->GetObjectDefinitions().m_Named.m_NameProp, value);
        qt3dsdm::TDataStrPtr namePtr(qt3dsdm::get<qt3dsdm::TDataStrPtr>(value));
        QString materialName = QString::fromWCharArray(namePtr->GetData(),
                                                       int(namePtr->GetLength()));
        QString sourcePath;
        for (int i = 0; i < m_matDatas.size(); ++i) {
            if (QString::compare(m_matDatas[i].m_name, materialName, Qt::CaseInsensitive) == 0) {
                sourcePath = doc->GetDocumentDirectory() + QLatin1Char('/')
                        + m_matDatas[i].m_relativePath;
            }
        }

        sceneEditor->writeMaterialFile(material, materialName, sourcePath.isEmpty(), sourcePath);
    }
}

void InspectorControlModel::setMaterialTypeValue(long instance, int handle, const QVariant &value)
{
    Q_UNUSED(handle)

    const QString typeValue = value.toString();
    QString v;

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto sceneEditor = doc->getSceneEditor();
    const Q3DStudio::CString oldType = sceneEditor->GetObjectTypeName(instance);
    qt3dsdm::Qt3DSDMInstanceHandle refMaterial;
    if (oldType == "ReferencedMaterial")
        refMaterial = getBridge()->getMaterialReference(instance);

    bool changeMaterialFile = false;
    bool canCopyProperties = false;
    if (typeValue == getAnimatableMaterialString()) {
        v = QStringLiteral("Standard Material");
        if (refMaterial.Valid()) {
            const auto refSourcePath = getBridge()->GetSourcePath(refMaterial);
            for (auto &material : m_materials) {
                if (refSourcePath == material.m_relativePath) {
                    v = material.m_relativePath;
                    break;
                }
            }
        }
        canCopyProperties = true;
    } else if (typeValue == getBasicMaterialString()) {
        v = QStringLiteral("Referenced Material");
        changeMaterialFile = true;
    } else if (typeValue == getReferencedMaterialString()) {
        v = QStringLiteral("Referenced Material");
    }

    Q3DStudio::ScopedDocumentEditor scopedEditor(
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, tr("Set Material Type")));

    scopedEditor->SetMaterialType(instance, v);

    if (refMaterial.Valid() && canCopyProperties) {
        const Q3DStudio::CString newType = sceneEditor->GetObjectTypeName(instance);
        const Q3DStudio::CString refType = sceneEditor->GetObjectTypeName(refMaterial);
        if (refType == newType)
            scopedEditor->copyMaterialProperties(refMaterial, instance);

        if (getBridge()->isInsideMaterialContainer(refMaterial)) {
            const auto name = scopedEditor->GetName(instance);
            if (!name.toQString().endsWith(QLatin1String("_animatable")))
                scopedEditor->SetName(instance, name + "_animatable");
        }
    }

    if (changeMaterialFile) {
        scopedEditor->setMaterialProperties(instance, Q3DStudio::CString::fromQString(
                                            getBridge()->getDefaultMaterialName()), {}, {});

        // Select the original instance again since potentially creating a material selects the
        // created one
        doc->SelectDataModelObject(instance);

        rebuildTree(); // Hack to mimic value changing behavior of the type selector
    }

    saveIfMaterial(instance);
}

void InspectorControlModel::setShaderValue(long instance, int handle, const QVariant &value)
{
    Q_UNUSED(handle)

    const QString typeValue = value.toString();
    QString v = QStringLiteral("Standard Material");
    for (size_t matIdx = 0, end = m_materials.size(); matIdx < end; ++matIdx) {
        if (m_materials[matIdx].m_name == typeValue)
            v = m_materials[matIdx].m_relativePath;
    }

    const auto doc = g_StudioApp.GetCore()->GetDoc();

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Material Type"))
            ->SetMaterialType(instance, v);

    const auto dispatch = g_StudioApp.GetCore()->GetDispatch();
    QVector<qt3dsdm::Qt3DSDMInstanceHandle> refMats;
    doc->getSceneReferencedMaterials(doc->GetSceneInstance(), refMats);
    for (auto &refMat : qAsConst(refMats)) {
        const auto origMat = getBridge()->getMaterialReference(refMat);
        if (origMat.Valid() && long(origMat) == instance)
            dispatch->FireImmediateRefreshInstance(refMat);
    }

    saveIfMaterial(instance);
}

void InspectorControlModel::setMatDataValue(long instance, int handle, const QVariant &value)
{
    Q_UNUSED(handle)

    const QString typeValue = value.toString();
    QString v;
    QString name;
    QString srcPath;
    QMap<QString, QString> values;
    QMap<QString, QMap<QString, QString>> textureValues;

    const auto doc = g_StudioApp.GetCore()->GetDoc();

    bool changeMaterialFile = false;
    if (typeValue == getDefaultMaterialString()) {
        v = QStringLiteral("Referenced Material");
        name = getBridge()->getDefaultMaterialName();
        srcPath = name;
        changeMaterialFile = true;
    } else {
        const auto sceneEditor = doc->getSceneEditor();
        for (size_t matIdx = 0, end = m_matDatas.size(); matIdx < end; ++matIdx) {
            QString shownName = m_matDatas[matIdx].m_name;
            int slashIndex = shownName.lastIndexOf(QLatin1Char('/'));
            if (slashIndex != -1)
                shownName = shownName.mid(slashIndex + 1);
            if (QString::compare(shownName + QLatin1String(" (")
                    + m_matDatas[matIdx].m_relativePath + QLatin1Char(')'),
                                 typeValue, Qt::CaseInsensitive) == 0
                    || QString::compare(shownName, typeValue, Qt::CaseInsensitive) == 0) {
                v = QStringLiteral("Referenced Material");
                changeMaterialFile = true;
                name = m_matDatas[matIdx].m_name;
                srcPath = m_matDatas[matIdx].m_relativePath;
                const auto material = sceneEditor->getMaterial(srcPath);
                if (material.Valid()) {
                    // Get the correct case source path
                    const auto absPath = sceneEditor->getFilePathFromMaterialName(
                                sceneEditor->GetName(material).toQString());
                    const auto relPath = QDir(doc->GetDocumentDirectory())
                            .relativeFilePath(absPath);
                    srcPath = relPath;
                }
                values = m_matDatas[matIdx].m_values;
                textureValues = m_matDatas[matIdx].m_textureValues;
                break;
            }
        }
    }

    if (changeMaterialFile) {
        {
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QString())
                    ->setMaterialValues(srcPath, values, textureValues);
        }
        // Several aspects of the editor are not updated correctly
        // if the data core is changed without a transaction
        // The above scope completes the transaction for creating a new material
        // Next the added undo has to be popped from the stack
        // TODO: Find a way to update the editor fully without a transaction
        g_StudioApp.GetCore()->GetCmdStack()->RemoveLastUndo();
    }

    Q3DStudio::ScopedDocumentEditor scopedEditor(
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, tr("Set Material Type")));
    scopedEditor->SetMaterialType(instance, v);

    if (changeMaterialFile) {
        scopedEditor->setMaterialSourcePath(instance, Q3DStudio::CString::fromQString(srcPath));
        scopedEditor->setMaterialReferenceByPath(instance, srcPath);

        // Select original instance again since potentially
        // creating a material selects the created one
        doc->SelectDataModelObject(instance);

        rebuildTree(); // Hack to mimic value changing behavior of the type selector
    }

    saveIfMaterial(instance);
}

void InspectorControlModel::setRenderableValue(long instance, int handle, const QVariant &value)
{
    qt3dsdm::SValue oldValue = currentPropertyValue(instance, handle);

    QString v = value.toString();
    if (v == QObject::tr("No renderable item"))
        v = QString();

    if (v == qt3dsdm::get<QString>(oldValue))
        return;

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), QObject::tr("Set Property"))
            ->SetInstancePropertyValueAsRenderable(instance, handle,
                                                   Q3DStudio::CString::fromQString(v));
}

void InspectorControlModel::setPropertyValue(long instance, int handle, const QVariant &value,
                                             bool commit)
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto studio = doc->GetStudioSystem();
    // Name property needs special handling
    if (instance && handle == getBridge()->GetNameProperty()) {
        // Ignore preview of name property change
        if (!commit)
            return;

        m_modifiedProperty.first = 0;
        m_modifiedProperty.second = 0;
        m_previouslyCommittedValue = {};

        Q3DStudio::CString currentName = getBridge()->GetName(instance, true);
        Q3DStudio::CString newName = Q3DStudio::CString::fromQString(value.toString());
        if (!newName.IsEmpty()) {
            if (getBridge()->isInsideMaterialContainer(instance)
                && ((newName.Find('/') != Q3DStudio::CString::ENDOFSTRING
                     || newName.Find('#') != Q3DStudio::CString::ENDOFSTRING
                     || newName.Find(':') != Q3DStudio::CString::ENDOFSTRING)
                    || m_suspendMaterialRename)) {
                return;
            }
            qt3dsdm::Qt3DSDMInstanceHandle parentInstance = getBridge()
                                                            ->GetParentInstance(instance);

            if (parentInstance == doc->GetSceneInstance()
                && newName.toQString() == getBridge()->getMaterialContainerName()) {
                QString theTitle = QObject::tr("Rename Object Error");
                QString theString = getBridge()->getMaterialContainerName()
                                    + QObject::tr(" is a reserved name.");
                // Display error message box asynchronously so focus loss won't trigger setting
                // the name again
                g_StudioApp.GetDialogs()->asyncDisplayMessageBox(theTitle, theString,
                                                                 Qt3DSMessageBox::ICON_ERROR);
                return;
            }

            Q3DStudio::CString realNewName = newName;
            if (getBridge()->isInsideMaterialContainer(instance)) {
                Q3DStudio::CString realName = getBridge()->GetName(instance);
                int slashIndex = realName.rfind('/');
                if (slashIndex != Q3DStudio::CString::ENDOFSTRING)
                    realNewName = realName.Left(slashIndex + 1) + newName;
            }

            if (!getBridge()->CheckNameUnique(parentInstance, instance, realNewName)) {
                QString origNewName = newName.toQString();
                realNewName = getBridge()->GetUniqueChildName(parentInstance, instance,
                                                              realNewName);
                newName = realNewName;
                if (getBridge()->isInsideMaterialContainer(instance)) {
                    int slashIndex = newName.rfind('/');
                    if (slashIndex != Q3DStudio::CString::ENDOFSTRING)
                        newName = newName.substr(slashIndex + 1);
                }
                // Display rename message box asynchronously so focus loss won't trigger setting
                // the name again
                g_StudioApp.GetDialogs()->DisplayObjectRenamed(origNewName, newName.toQString(),
                                                               true);
            }

            const auto sceneEditor = doc->getSceneEditor();

            // A materialdef with the same name might exists as a file but not in the container,
            // so an additional check is needed for that case
            if (getBridge()->isInsideMaterialContainer(instance)) {
                int i = 1;
                while (QFileInfo(sceneEditor->getFilePathFromMaterialName(
                                     realNewName.toQString())).exists()) {
                    ++i;
                    realNewName = Q3DStudio::CString::fromQString(
                                realNewName.toQString() + QString::number(i));
                    if (!getBridge()->CheckNameUnique(parentInstance, instance, realNewName)) {
                        realNewName = getBridge()->GetUniqueChildName(parentInstance, instance,
                                                                      realNewName);
                    }
                }
                newName = realNewName;
                int slashIndex = newName.rfind('/');
                if (slashIndex != Q3DStudio::CString::ENDOFSTRING)
                    newName = newName.substr(slashIndex + 1);
            }

            if (newName != currentName) {
                if (getBridge()->isInsideMaterialContainer(instance)) {
                    const auto properOldName = sceneEditor->GetName(instance).toQString();
                    const QString dirPath = doc->GetDocumentDirectory();
                    for (size_t matIdx = 0, end = m_matDatas.size(); matIdx < end; ++matIdx) {
                        if (m_matDatas[matIdx].m_name == properOldName) {
                            QFileInfo fileInfo(dirPath + QLatin1Char('/')
                                               + m_matDatas[matIdx].m_relativePath);
                            const QString newFile = fileInfo.absolutePath()
                                                    + QLatin1Char('/')
                                                    + newName.toQString()
                                                    + QStringLiteral(".materialdef");
                            const auto properNewName
                                    = sceneEditor->getMaterialNameFromFilePath(newFile);
                            newName = Q3DStudio::CString::fromQString(properNewName);
                            doc->queueMaterialRename(properOldName, properNewName);
                        }
                    }
                }
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(
                            *g_StudioApp.GetCore()->GetDoc(),
                            QObject::tr("Set Name"))->SetName(instance, newName, false);
            }
        }
        return;
    }

    qt3dsdm::SValue oldValue = m_guideInspectable
            ? m_guideInspectable->properties()[handleToGuidePropIndex(handle)]->GetInspectableData()
            : currentPropertyValue(instance, handle);
    qt3dsdm::SValue v = value;

    const bool hasPreview = (m_modifiedProperty.first == instance
            && m_modifiedProperty.second == handle);

    // If this set is a commit for property that was previously changed without
    // committing, we must let the set go through even if the value hasn't changed
    // to finish the transaction.
    if (v == oldValue && !(commit && hasPreview))
        return;

    if (!commit && !hasPreview) {
        m_previouslyCommittedValue = oldValue;
        m_modifiedProperty.first = instance;
        m_modifiedProperty.second = handle;
    }

    if (instance) {
        // If the user enters 0.0 to any (x, y, z) values of camera scale,
        // we reset the value back to original, because zero scale factor will crash
        // camera-specific inverse matrix math. (Additionally, scale of zero for a camera
        // is generally not useful anyway.) We could silently discard zero values also deeper in the
        // value setter code, but then the inspector panel value would not be updated as opposed
        // to both rejecting invalid and resetting the original value here.
        EStudioObjectType theType = getBridge()->GetObjectType(instance);

        if (theType == EStudioObjectType::OBJTYPE_CAMERA &&
                studio->GetPropertySystem()->GetName(handle) == Q3DStudio::CString("scale")) {
            const QVector3D theFloat3 = qt3dsdm::get<QVector3D>(v);
            if (theFloat3.x() == 0.0f || theFloat3.y() == 0.0f || theFloat3.z() == 0.0f )
                v = oldValue;
        }

        // some properties may initialize OpenGL resources (e.g. loading meshes will
        // initialize vertex buffers), so the renderer's OpenGL context must be current
        Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
        theRenderer.MakeContextCurrent();
        m_UpdatableEditor.EnsureEditor(QObject::tr("Set Property"), __FILE__, __LINE__)
                .SetInstancePropertyValue(instance, handle, v);

        theRenderer.ReleaseContext();

        m_UpdatableEditor.FireImmediateRefresh(instance);
    } else if (m_guideInspectable) {
        m_guideInspectable->properties()[handleToGuidePropIndex(handle)]->ChangeInspectableData(v);
    }

    if (commit) {
        m_modifiedProperty.first = 0;
        m_modifiedProperty.second = 0;
        if (m_previouslyCommittedValue == v) {
            if (m_guideInspectable)
                m_guideInspectable->Rollback();
            else
                m_UpdatableEditor.RollbackEditor();
        } else {
            if (m_guideInspectable) {
                // If the guide ends up over the matte, destroy it
                QSize presSize = g_StudioApp.GetCore()->GetStudioProjectSettings()
                        ->getPresentationSize();
                bool isInPres = true;
                qt3dsdm::SValue posValue = m_guideInspectable->GetPosition();
                float position = qt3dsdm::get<float>(posValue);
                if (m_guideInspectable->isHorizontal())
                    isInPres = 0.f <= position && float(presSize.height()) >= position;
                else
                    isInPres = 0.f <= position && float(presSize.width()) >= position;
                if (isInPres)
                    m_guideInspectable->Commit();
                else
                    m_guideInspectable->Destroy();
            } else {
                m_UpdatableEditor.CommitEditor();
            }
        }

        m_previouslyCommittedValue = {};
        refreshTree();

        saveIfMaterial(instance);
    }
}

void InspectorControlModel::setSlideSelection(long instance, int handle, int index,
                                              const QStringList &list)
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studioSystem = doc->GetStudioSystem();
    const auto metaDataProvider = doc->GetStudioSystem()->GetActionMetaData();
    const auto info = metaDataProvider->GetMetaDataPropertyInfo(
                metaDataProvider->GetMetaDataProperty(instance, handle));
    QStringList stringlist = qt3dsdm::get<QStringList>(info->m_MetaDataData);

    auto slideSystem = studioSystem->GetSlideSystem();
    std::pair<bool, bool> slideData(
        getSlideCharacteristics(instance, *studioSystem->GetSlideCore(),
                                *slideSystem));
    bool hasNextSlide(slideData.first);
    bool hasPreviousSlide(slideData.second);
    qt3dsdm::SStringOrInt newSelectedData;
    if (!hasNextSlide)
        stringlist.removeAll("Next");
    if (!hasPreviousSlide)
        stringlist.removeAll("Previous");

    auto itemCount = stringlist.count();
    if (index < itemCount) {
        newSelectedData = qt3dsdm::SStringOrInt(std::make_shared<qt3dsdm::CDataStr>
                                              (Q3DStudio::CString::fromQString(list[index]).c_str()));
    } else {
        auto slideHandle = slideSystem->GetSlideByInstance(instance);
        auto masterSlide = slideSystem->GetMasterSlide(slideHandle);
        long slideIndex = index - itemCount + 1;
        auto newSelectedSlide = slideSystem->GetSlideByIndex(masterSlide, slideIndex);
        newSelectedData = qt3dsdm::SStringOrInt((long)newSelectedSlide);
    }

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), QObject::tr("Set Property"))
            ->SetInstancePropertyValue(instance, handle, newSelectedData);
}

// temporarily prevent material renaming when opening the colors dialog (fix for QT3DS-3407)
void InspectorControlModel::suspendMaterialRename(bool flag)
{
    m_suspendMaterialRename = flag;
}

void InspectorControlModel::setPropertyControllerInstance(
    long instance,int property, Q3DStudio::CString controllerInstance, bool controlled)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    IObjectReferenceHelper *objRefHelper = doc->GetDataModelObjectReferenceHelper();

    Q3DStudio::CString instancepath = Q3DStudio::CString(
        objRefHelper->GetObjectReferenceString(doc->GetSceneInstance(),
                                               CRelativePathTools::EPATHTYPE_GUID, instance));
    Q_ASSERT(instancepath.size());

    doc->SetInstancePropertyControlled(instance, instancepath, property,
                                       controllerInstance, controlled);
}

void InspectorControlModel::setPropertyControlled(long instance, int property)
{
    const auto signalSender
        = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystemSignalSender();

    signalSender->SendControlledToggled(instance, property);
}

bool InspectorControlModel::isLayer(long instance) const
{
    return getBridge()->GetObjectType(instance) == EStudioObjectType::OBJTYPE_LAYER;
}

QString InspectorControlModel::renderableId(const QString &filePath) const
{
    return g_StudioApp.getRenderableId(filePath);
}

void InspectorControlModel::setPropertyAnimated(long instance, int handle, bool animated)
{
    CCmd* cmd = nullptr;
    auto doc = g_StudioApp.GetCore()->GetDoc();
    if (animated)
        cmd = new CCmdDataModelAnimate(doc, instance, handle);
    else
        cmd = new CCmdDataModelDeanimate(doc, instance, handle);

    g_StudioApp.GetCore()->ExecuteCommand(cmd);
}

QVariant InspectorControlModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(),index.parent()))
        return {};

    const auto row = index.row();

    switch (role) {
    case GroupValuesRole:
        return m_groupElements.at(row).controlElements;
    case GroupTitleRole:
        return m_groupElements.at(row).groupTitle;
    case GroupInfoRole:
        return m_groupElements.at(row).groupInfo;
    }
    return {};
}

QHash<int, QByteArray> InspectorControlModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(GroupValuesRole, "values");
    names.insert(GroupTitleRole, "title");
    names.insert(GroupInfoRole, "info");
    return names;
}

InspectorControlBase::~InspectorControlBase()
{
}
