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
#include <QFileInfo>
#include <QtCore/qurl.h>

#include <functional>

#include "InspectorControlModel.h"
#include "Core.h"
#include "Doc.h"
#include "ControlGraphIterators.h"
#include "InspectorGroup.h"
#include "Qt3DSDMInspectorGroup.h"
#include "Qt3DSDMInspectorRow.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMInspectable.h"
#include "Qt3DSDMDataCore.h"
#include "StudioApp.h"
#include "IDocumentEditor.h"
#include "Control.h"
#include "ControlData.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMSignals.h"
#include "CmdDataModelDeanimate.h"
#include "GuideInspectable.h"
#include "Qt3DSDMDataTypes.h"
#include "IObjectReferenceHelper.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSFileTools.h"
#include "Qt3DSDMSlideCore.h"
#include "SlideSystem.h"
#include "Qt3DSDMMaterialInspectable.h"
#include "ClientDataModelBridge.h"
#include "IDocumentReader.h"
#include "IStudioRenderer.h"
#include "foundation/Qt3DSLogging.h"
#include "Dialogs.h"

static QStringList renderableItems()
{
    QStringList renderables;
    renderables.push_back(QObject::tr("No renderable item"));
    const CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    Q3DStudio::CString docDir = doc->GetDocumentDirectory();

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
    int index = (int)slideSystem.GetSlideIndex(slide);
    int count = (int)slideSystem.GetSlideCount(master);
    bool hasNextSlide = index > 0 && index < count - 1;
    bool hasPreviousSlide = index > 1;
    return std::make_pair(hasNextSlide, hasPreviousSlide);
}

InspectorControlModel::InspectorControlModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_UpdatableEditor(*g_StudioApp.GetCore()->GetDoc())
{
    m_modifiedProperty.first = 0;
    m_modifiedProperty.second = 0;
}

void InspectorControlModel::setInspectable(CInspectableBase *inInspectable)
{
    const auto signalProvider
            = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();

    if (m_notifier.get() == nullptr) {
        m_notifier = signalProvider->ConnectInstancePropertyValue(
                    std::bind(&InspectorControlModel::notifyInstancePropertyValue,
                              this, std::placeholders::_1, std::placeholders::_2));
    }
    if (m_slideNotifier.get() == nullptr) {
        m_slideNotifier = signalProvider->ConnectSlideRearranged(
                    std::bind(&InspectorControlModel::onSlideRearranged, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    }

    if (m_inspectableBase != inInspectable) {
        m_inspectableBase = inInspectable;
        rebuildTree();
    }
}

CInspectableBase *InspectorControlModel::inspectable() const
{
    return m_inspectableBase;
}

qt3dsdm::Qt3DSDMInstanceHandle getReferenceMaterial(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    qt3dsdm::Qt3DSDMInstanceHandle refMaterial;
    const auto sceneEditor = g_StudioApp.GetCore()->GetDoc()->getSceneEditor();
    sceneEditor->getMaterialReference(instance, refMaterial);
    return refMaterial;
}

qt3dsdm::Qt3DSDMInstanceHandle getReferenceMaterial(CInspectableBase *inspectBase)
{
    qt3dsdm::Qt3DSDMInstanceHandle refMaterial;
    if (const auto cdmInspectable = dynamic_cast<Qt3DSDMInspectable *>(inspectBase))
        refMaterial = getReferenceMaterial(cdmInspectable->GetGroupInstance(0));
    return refMaterial;
}

CInspectableBase *getReferenceMaterialInspectable(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    if (instance.Valid())
        return g_StudioApp.getInspectableFromInstance(instance);
    return nullptr;
}

CInspectableBase *getReferenceMaterialInspectable(CInspectableBase *inspectBase)
{
    if (const auto cdmInspectable = dynamic_cast<Qt3DSDMInspectable *>(inspectBase)) {
        auto refMaterial = getReferenceMaterial(cdmInspectable->GetGroupInstance(0));
        if (refMaterial.Valid())
            return g_StudioApp.getInspectableFromInstance(refMaterial);
    }
    return nullptr;
}

void InspectorControlModel::notifyInstancePropertyValue(qt3dsdm::Qt3DSDMInstanceHandle inHandle,
                                                        qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    auto doc = g_StudioApp.GetCore()->GetDoc();
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
            if (property->m_property == inProperty || imageInstance == inHandle) {
                updatePropertyValue(property);
                changed = true;
            }
        }
    }
    if (changed)
        Q_EMIT dataChanged(index(0), index(rowCount() - 1));
}

QVariant InspectorControlModel::getPropertyValue(long instance, int handle)
{
    for (int row = 0; row < m_groupElements.count(); ++row) {
        auto group = m_groupElements[row];
        for (int p = 0; p < group.controlElements.count(); ++p) {
            QVariant& element = group.controlElements[p];
            InspectorControlBase *property = element.value<InspectorControlBase *>();
            if (property->m_property == qt3dsdm::CDataModelHandle(handle))
                return property->m_value;
        }
    }
    return {};
}

void InspectorControlModel::updateMaterialValues()
{
    // Find if there are any material items and update the values of those
    for (int row = 0; row < m_groupElements.count(); ++row) {
        const CInspectorGroup *inspectorGroup = m_inspectableBase->GetGroup(row);
        const auto group = dynamic_cast<const Qt3DSDMInspectorGroup *>(inspectorGroup);
        const auto materialGroup = dynamic_cast<const Qt3DSDMMaterialInspectorGroup *>(group);
        if (materialGroup && materialGroup->isMaterialGroup()) {
            if (m_groupElements[row].controlElements.size()) {
                auto item = m_groupElements[row].controlElements[0]
                        .value<InspectorControlBase *>();
                item->m_values = materialValues();
                Q_EMIT item->valuesChanged();
                // Changing values resets the selected index, so pretend the value has also changed
                Q_EMIT item->valueChanged();
            }
        }
    }
}

void InspectorControlModel::setMaterials(std::vector<Q3DStudio::CFilePath> &materials)
{
    m_materials.clear();
    const Q3DStudio::CString base = g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory();

    for (Q3DStudio::CFilePath path : materials) {

        const QString relativePath = path.toQString();
        const Q3DStudio::CFilePath absolutePath
            = Q3DStudio::CFilePath::CombineBaseAndRelative(base, path);

        const QString name = g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                                  .GetCustomMaterialName(
                                        absolutePath.toCString()).toQString();

        m_materials.push_back({name, relativePath});
    }

    updateMaterialValues();
}

void InspectorControlModel::setMatDatas(std::vector<Q3DStudio::CFilePath> &matDatas)
{
    m_matDatas.clear();

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto sceneEditor = doc->getSceneEditor();
    if (!sceneEditor)
        return;

    QStringList filenames;
    for (Q3DStudio::CFilePath path : matDatas) {
        const QString relativePath = path.toQString();
        const Q3DStudio::CFilePath absolutePath
            = Q3DStudio::CFilePath::CombineBaseAndRelative(doc->GetDocumentDirectory(), path);

        QString name;
        QMap<QString, QString> values;
        sceneEditor->getMaterialInfo(
                    absolutePath.toQString(), name, values);

        m_matDatas.push_back({name, relativePath, values});
        filenames.push_back(name);

        bool needRewrite = false;
        if (values.contains(QStringLiteral("presentation"))
                && values.contains(QStringLiteral("path"))
                && values.contains(QStringLiteral("filename"))) {
            if (values[QStringLiteral("presentation")]
                    == doc->GetDocumentPath().GetAbsolutePath().toQString()) {
                if (!QFileInfo(values["path"]).exists()) {
                    const auto instance = sceneEditor->getMaterial(
                                Q3DStudio::CString::fromQString(
                                    values[QStringLiteral("filename")]));
                    if (instance.Valid()) {
                        sceneEditor->SetName(instance, Q3DStudio::CString::fromQString(name));
                        needRewrite = true;
                    }
                }
            }
        }

        auto material = sceneEditor->getOrCreateMaterial(Q3DStudio::CString::fromQString(name));
        sceneEditor->setMaterialValues(name, values);

        if (needRewrite)
            sceneEditor->writeMaterialFile(material, name, false, absolutePath.toQString());
    }

    sceneEditor->updateMaterialInstances(filenames);

    updateMaterialValues();
}

QString getStandardMaterialString()
{
    return QObject::tr("Standard Material");
}

QString getReferencedMaterialString()
{
    return QObject::tr("Referenced Material");
}

QString getDefaultMaterialString()
{
    return QObject::tr("Default");
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
        std::vector<Q3DStudio::CString> fontNames;
        g_StudioApp.GetCore()->GetDoc()->GetProjectFonts(fontNames);
        QStringList possibleValues;
        for (const auto &fontName : fontNames)
            possibleValues.append(fontName.toQString());
        for (auto fontElement : qAsConst(fontElements)) {
            fontElement->m_values = possibleValues;
            Q_EMIT fontElement->valuesChanged();
            // Changing values resets the selected index, so pretend the value has also changed
            Q_EMIT fontElement->valueChanged();
        }
    }
}

QStringList InspectorControlModel::materialValues() const
{
    QStringList values;
    values.push_back(getStandardMaterialString());
    values.push_back(getReferencedMaterialString());

    for (size_t matIdx = 0, end = m_materials.size(); matIdx < end; ++matIdx)
        values.push_back(m_materials[matIdx].m_name);

    values.push_back(getDefaultMaterialString());

    for (size_t matIdx = 0, end = m_matDatas.size(); matIdx < end; ++matIdx)
        values.push_back(m_matDatas[matIdx].m_name);

    return values;
}

InspectorControlBase* InspectorControlModel::createMaterialItem(Qt3DSDMInspectable *inspectable,
                                                        int groupIndex)
{
    const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    auto instance = inspectable->GetGroupInstance(groupIndex);

    CClientDataModelBridge *theBridge = studio->GetClientDataModelBridge();
    EStudioObjectType theType = theBridge->GetObjectType(instance);

    InspectorControlBase *item = new InspectorControlBase;
    item->m_instance = instance;

    item->m_title = tr("Material Type");
    item->m_dataType = qt3dsdm::DataModelDataType::StringRef;
    item->m_propertyType = qt3dsdm::AdditionalMetaDataType::None;
    item->m_tooltip = tr("Type of material being used or custom material");

    item->m_animatable = false;

    const QStringList values = materialValues();
    item->m_values = values;

    QString sourcePath = theBridge->GetSourcePath(item->m_instance).toQString();

    if (!sourcePath.isEmpty() && sourcePath != QLatin1String("Default")) {
        const auto doc = g_StudioApp.GetCore()->GetDoc();
        const auto dirPath = doc->GetDocumentDirectory().toQString();
        QFileInfo fileInfo(dirPath + QStringLiteral("/") + sourcePath);
        if (!fileInfo.exists()) {
            const auto sceneEditor = g_StudioApp.GetCore()->GetDoc()->getSceneEditor();
            const auto refMaterial = getReferenceMaterial(item->m_instance);
            if (refMaterial.Valid()) {
                const auto matName = sceneEditor->GetName(refMaterial);
                QFileInfo newFileInfo(fileInfo.absoluteDir().path() + QStringLiteral("/")
                                      + matName.toQString() + QStringLiteral(".matdata"));
                const QDir docDir(dirPath);
                const auto relPath = docDir.relativeFilePath(newFileInfo.absoluteFilePath());
                sceneEditor->setMaterialSourcePath(item->m_instance,
                                                   Q3DStudio::CString::fromQString(relPath));
                sourcePath = theBridge->GetSourcePath(item->m_instance).toQString();
            }
        }
    }

    switch (theType) {
    case OBJTYPE_MATERIAL:
        item->m_value = getStandardMaterialString();
        break;

    case OBJTYPE_REFERENCEDMATERIAL:
        item->m_value = getReferencedMaterialString();
        if (sourcePath == QLatin1String("Default"))
            item->m_value = getDefaultMaterialString();
        for (int matIdx = 0, end = int(m_matDatas.size()); matIdx < end; ++matIdx) {
            if (m_matDatas[matIdx].m_relativePath == sourcePath)
                item->m_value = values[m_materials.size() + matIdx + 3];
        }
        break;
    }

    for (int matIdx = 0, end = int(m_materials.size()); matIdx < end; ++matIdx) {
        if (m_materials[matIdx].m_relativePath == sourcePath)
            item->m_value = values[matIdx + 2]; // +2 for standard material and referenced material
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

    InspectorControlBase *item = new InspectorControlBase;
    item->m_property = metaProperty.m_Property;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);
    item->m_metaProperty = metaProperty;

    Q3DStudio::CString title;
    title.Assign(metaProperty.m_FormalName.c_str());
    if (title.IsEmpty())
        title.Assign(metaProperty.m_Name.c_str());
    item->m_title = title.toQString();

    const auto propertySystem = studio->GetPropertySystem();
    item->m_dataType = propertySystem->GetDataType(metaProperty.m_Property);
    item->m_propertyType = static_cast<qt3dsdm::AdditionalMetaDataType::Value>
            (propertySystem->GetAdditionalMetaDataType(item->m_instance, metaProperty.m_Property));
    item->m_tooltip = Q3DStudio::CString(metaProperty.m_Description.c_str()).toQString();
    // \n is parsed as \\n from the material and effect files. Replace them to fix multi-line
    // tooltips
    item->m_tooltip.replace(QStringLiteral("\\n"), QStringLiteral("\n"));

    item->m_animatable = metaProperty.m_Animatable &&
            studio->GetAnimationSystem()->IsPropertyAnimatable(item->m_instance,
                                                               metaProperty.m_Property);

    item->m_controllable = metaProperty.m_Controllable;

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
    qt3dsdm::SValue value;
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studioSystem = doc->GetStudioSystem();
    const auto propertySystem = studioSystem->GetPropertySystem();
    propertySystem->GetInstancePropertyValue(instance, handle,  value);

    return  value;
}

QString InspectorControlModel::currentControllerValue(long instance, int handle) const
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studio = doc->GetStudioSystem();

    qt3dsdm::SValue currPropVal = currentPropertyValue(
                instance, studio->GetPropertySystem()->GetAggregateInstancePropertyByName(
                    instance, qt3dsdm::TCharStr(L"controlledproperty")));
    if (!currPropVal.empty()) {
        Q3DStudio::CString currPropValStr
                = qt3dsdm::get<qt3dsdm::TDataStrPtr>(currPropVal)->GetData();

        Q3DStudio::CString propName
                = studio->GetPropertySystem()->GetName(handle).c_str();

        // Datainput controller name is always prepended with "$". Differentiate
        // between datainput and property that has the same name by searching specifically
        // for whitespace followed by property name.
        long propNamePos = currPropValStr.find(" " + propName);
        if ((propNamePos != currPropValStr.ENDOFSTRING) && (propNamePos != 0)) {
            long posCtrlr = currPropValStr.substr(0, propNamePos).ReverseFind("$");

            // adjust pos if this is the first controller - property pair
            // in controlledproperty
            if (posCtrlr < 0)
                posCtrlr = 0;

            // remove $
            posCtrlr++;
            return currPropValStr.substr(posCtrlr, propNamePos - posCtrlr).toQString();
        }
        else
            return {};
    } else {
        return {};
    }
}

void InspectorControlModel::updateControlledToggleState(InspectorControlBase* inItem) const
{
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
        inItem->m_tooltip = Q3DStudio::CString(
            inItem->m_metaProperty.m_Description.c_str()).toQString();
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
           inItem->m_tooltip = Q3DStudio::CString(
               inItem->m_metaProperty.m_Description.c_str()).toQString();
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

           inItem->m_tooltip = tr("Controlling Data Input:\n%1").arg(ctrlName);
           inItem->m_controller = ctrlName;
       }
    }

    Q_EMIT inItem->tooltipChanged();
    // Emit signal always to trigger updating of controller name in UI
    // also when user switches from one controller to another
    Q_EMIT inItem->controlledChanged();
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

bool InspectorControlModel::isTreeRebuildRequired(CInspectableBase* inspectBase) const
{
    if (inspectBase != m_inspectableBase || !inspectBase)
        return true;

    long theCount = m_inspectableBase->GetGroupCount();
    long refMaterialGroupCount = 0;
    auto refMaterial = getReferenceMaterial(inspectBase);
    if (refMaterial.Valid()) {
        auto refMaterialInspectable = getReferenceMaterialInspectable(refMaterial);
        if (refMaterialInspectable)
            refMaterialGroupCount = refMaterialInspectable->GetGroupCount();
    }

    if (m_groupElements.size() != theCount + refMaterialGroupCount)
        return true;

    for (long theIndex = 0; theIndex < theCount; ++theIndex) {
        const CInspectorGroup *theInspectorGroup = m_inspectableBase->GetGroup(theIndex);
        if (m_groupElements.at(theIndex).groupTitle != theInspectorGroup->GetName())
            return true;
    }

    return false;
}

bool InspectorControlModel::isGroupRebuildRequired(CInspectableBase* inspectable, int theIndex) const
{
    Q_ASSERT(theIndex < m_groupElements.size());
    const CInspectorGroup *theInspectorGroup = inspectable->GetGroup(theIndex);
    const auto existingGroup = m_groupElements.at(theIndex);
    if (existingGroup.groupTitle != theInspectorGroup->GetName())
        return true;

    if (const auto cdmInspectable = dynamic_cast<Qt3DSDMInspectable *>(inspectable)) {
        int existingIndex = 0;
        if (const auto group = dynamic_cast<const Qt3DSDMInspectorGroup *>(theInspectorGroup)) {
            const auto materialGroup
                    = dynamic_cast<const Qt3DSDMMaterialInspectorGroup *>(group);
            if (materialGroup && materialGroup->isMaterialGroup()) {
                auto i = existingGroup.controlElements.at(existingIndex++).value<InspectorControlBase*>();
                if (i->m_instance != cdmInspectable->GetGroupInstance(theIndex))
                    return true;
            }

            if ((existingGroup.controlElements.size() - existingIndex) != group->GetRows().size())
                return true;

            for (const auto row : group->GetRows()) {
                auto i = existingGroup.controlElements.at(existingIndex++).value<InspectorControlBase*>();
                if (i->m_instance != cdmInspectable->GetGroupInstance(theIndex))
                    return true;

                if (i->m_property != row->GetMetaDataPropertyInfo().m_Property)
                    return true;
            }
        }
    }

    return false;
}

auto InspectorControlModel::computeTree(CInspectableBase* inspectBase)
    -> QVector<GroupInspectorControl>
{
    QVector<GroupInspectorControl> result;

    if (inspectBase) {
        bool isMaterialFromFile = false;

        const auto sceneEditor = g_StudioApp.GetCore()->GetDoc()->getSceneEditor();

        qt3dsdm::Qt3DSDMInstanceHandle instance;
        if (const auto inspectable = dynamic_cast<Qt3DSDMInspectable *>(inspectBase))
            instance = inspectable->GetGroupInstance(0);

        if (instance.Valid())
            isMaterialFromFile = sceneEditor->isInsideMaterialContainer(instance);

        long theCount = inspectBase->GetGroupCount();
        for (long theIndex = 0; theIndex < theCount; ++theIndex) {
            result.append(computeGroup(inspectBase, theIndex, isMaterialFromFile, false));
        }

        //Show original material properties for referenced materials
        auto refMaterial = getReferenceMaterial(inspectBase);
        if (refMaterial.Valid()) {
            auto refMaterialInspectable = getReferenceMaterialInspectable(refMaterial);
            if (refMaterialInspectable) {
                const auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
                        ->GetClientDataModelBridge();

                QString materialSrcPath;
                if (instance.Valid())
                    materialSrcPath = bridge->GetSourcePath(instance).toQString();

                if (materialSrcPath != QLatin1String("Default")
                        && bridge->GetSourcePath(refMaterial) != "Default") {
                    long theCount = refMaterialInspectable->GetGroupCount();
                    for (long theIndex = theCount - 1; theIndex < theCount; ++theIndex)
                        result.append(computeGroup(refMaterialInspectable, theIndex, true, true));
                }
            }
        }
    }

    return result;
}

auto InspectorControlModel::computeGroup(CInspectableBase* inspectable,
                                         int theIndex, bool disableAnimation, bool isReference)
    -> GroupInspectorControl
{
    CInspectorGroup* theInspectorGroup = inspectable->GetGroup(theIndex);
    GroupInspectorControl result;
    result.groupTitle = theInspectorGroup->GetName();

    if (isReference)
        result.groupTitle += QLatin1String(" (Reference)");

    if (const auto cdmInspectable = dynamic_cast<Qt3DSDMInspectable *>(inspectable)) {
        if (const auto group = dynamic_cast<Qt3DSDMInspectorGroup *>(theInspectorGroup)) {
            const auto materialGroup
                    = dynamic_cast<Qt3DSDMMaterialInspectorGroup *>(group);
            if (!isReference && materialGroup && materialGroup->isMaterialGroup()) {
                InspectorControlBase *item = createMaterialItem(cdmInspectable, theIndex);
                if (item)
                    result.controlElements.push_back(QVariant::fromValue(item));
            }
            for (const auto row : group->GetRows()) {
                InspectorControlBase *item = createItem(cdmInspectable, row, theIndex);
                if (!item)
                    continue;

                if (disableAnimation)
                    item->m_animatable = false;

                result.controlElements.push_back(QVariant::fromValue(item));
            }
        }
    } else if (dynamic_cast<SGuideInspectableImpl *>(inspectable)) {
        //KDAB_FIXME: load row element (How ?)
    }

    return result;
}

void InspectorControlModel::rebuildTree()
{
    beginResetModel();
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
    auto bridge = studioSystem->GetClientDataModelBridge();
    qt3dsdm::SValue value;
    const auto instance = element->m_instance;
    if (!propertySystem->HandleValid(instance))
        return;
    propertySystem->GetInstancePropertyValue(instance, element->m_property, value);

    const auto metaDataProvider = doc->GetStudioSystem()->GetActionMetaData();
    const auto info = metaDataProvider->GetMetaDataPropertyInfo(
                metaDataProvider->GetMetaDataProperty(instance, element->m_property));
    bool skipEmits = false;
    switch (element->m_dataType) {
    case qt3dsdm::DataModelDataType::String:
        element->m_value = qt3dsdm::get<QString>(value);
        // intentional fall-through for other String-derived datatypes
    case qt3dsdm::DataModelDataType::StringOrInt:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::StringList) {
            QStringList stringlist = qt3dsdm::get<QStringList>(info->m_MetaDataData);
            auto slideSystem = studioSystem->GetSlideSystem();

            if (element->m_title == QStringLiteral("Play Mode")) {
                std::pair<bool, bool> slideData(
                    getSlideCharacteristics(element->m_instance, *studioSystem->GetSlideCore(),
                                            *slideSystem));
                bool hasNextSlide(slideData.first);
                bool hasPreviousSlide(slideData.second);
                if (!hasNextSlide && !hasPreviousSlide)
                    stringlist.removeAll("Play Through To...");
            } else if (element->m_title == QStringLiteral("Play Through To")) {
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
                long slideCount = (long)slideSystem->GetSlideCount(masterSlide);
                for (long slideIndex = 1; slideIndex < slideCount; ++slideIndex) {
                    auto currentSlide = slideSystem->GetSlideByIndex(masterSlide, slideIndex);
                    auto currentInstance = slideSystem->GetSlideInstance(currentSlide);

                    QString slideName = bridge->GetName(currentInstance).toQString();
                    //hack to add a separator before the item
                    if (slideIndex == 1 && itemCount > 0)
                        slideName += "|separator";
                    stringlist.append(slideName);

                    if (currentSlide.GetHandleValue() == selectedSlideHandle)
                        selectedIndex = slideIndex + itemCount - 1;
                }

                element->m_value = QString(selectedIndex > 0 ? stringlist[selectedIndex]
                                   : stringlist.first()).replace("|separator", "");
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
            QString meshValue = qt3dsdm::get<QString>(value);
            Q3DStudio::CFilePath theSelectionItem(Q3DStudio::CString::fromQString(meshValue));
            Q3DStudio::CFilePath theSelectionWithoutId(theSelectionItem.filePath());
            QString theSelectionWithoutIdName = theSelectionWithoutId.GetFileName().toQString();
            if (theSelectionWithoutIdName.size())
                element->m_value = theSelectionWithoutIdName;
            else
                element->m_value = theSelectionItem.GetIdentifier().toQString();
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
                element->m_value = QVariant(QString(""));
            }
        } else {
            qWarning() << "KDAB_TODO: InspectorControlModel::updatePropertyValue: need to implement:"
                       << element->m_dataType << " " << element->m_title;
        }
        break;
    case qt3dsdm::DataModelDataType::Long:
        if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Range) {
            element->m_value = qt3dsdm::get<int>(value);
            const qt3dsdm::SMetaDataRange ranges = qt3dsdm::get<qt3dsdm::SMetaDataRange>(info->m_MetaDataData);
            const QList<double> rangesValues{ranges.m_Min, ranges.m_Max};
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
            const QList<double> rangesValues{ranges.m_Min, ranges.m_Max};
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
                element->m_value = objRefHelper->LookupObjectFormalName(refInstance).toQString();
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
            if (property->m_propertyType == qt3dsdm::AdditionalMetaDataType::Renderable)
                updatePropertyValue(property);
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
        long theCount = m_inspectableBase->GetGroupCount();
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
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto sceneEditor = doc->getSceneEditor();

    auto material = instance;
    const auto refMaterial = getReferenceMaterial(instance);
    if (refMaterial.Valid())
        material = refMaterial;

    if (!sceneEditor->isInsideMaterialContainer(material))
        return;

    const auto studio = doc->GetStudioSystem();
    const auto bridge = studio->GetClientDataModelBridge();
    EStudioObjectType type = bridge->GetObjectType(material);

    if (type == EStudioObjectType::OBJTYPE_MATERIAL
        || type == EStudioObjectType::OBJTYPE_CUSTOMMATERIAL) {
        qt3dsdm::SValue value;
        studio->GetPropertySystem()->GetInstancePropertyValue(
            material, bridge->GetObjectDefinitions().m_Named.m_NameProp, value);
        qt3dsdm::TDataStrPtr namePtr(qt3dsdm::get<qt3dsdm::TDataStrPtr>(value));
        QString materialName = QString::fromWCharArray(namePtr->GetData(), namePtr->GetLength());
        QString sourcePath;
        for (int i = 0; i < m_matDatas.size(); ++i) {
            if (m_matDatas[i].m_name == materialName) {
                sourcePath = doc->GetDocumentDirectory().toQString() + QDir::separator()
                        + m_matDatas[i].m_relativePath;
            }
        }

        if (!sourcePath.isEmpty())
            sceneEditor->writeMaterialFile(material, materialName, false, sourcePath);
    }
}

void InspectorControlModel::setMaterialTypeValue(long instance, int handle, const QVariant &value)
{
    Q_UNUSED(handle);
    const QString typeValue = value.toString();
    Q3DStudio::CString v;
    QString name;
    Q3DStudio::CString srcPath;
    QMap<QString, QString> values;

    bool changeMaterialFile = false;
    if (typeValue == getStandardMaterialString()) {
        v = Q3DStudio::CString("Standard Material");
    } else if (typeValue == getReferencedMaterialString()) {
        v = Q3DStudio::CString("Referenced Material");
    } else if (typeValue == getDefaultMaterialString()) {
        v = Q3DStudio::CString("Referenced Material");
        name = QLatin1String("Default");
        srcPath = "Default";
        changeMaterialFile = true;
    } else {
        for (size_t matIdx = 0, end = m_matDatas.size(); matIdx < end; ++matIdx) {
            if (m_matDatas[matIdx].m_name == typeValue) {
                v = Q3DStudio::CString("Referenced Material");
                changeMaterialFile = true;
                name = m_matDatas[matIdx].m_name;
                srcPath = Q3DStudio::CString::fromQString(m_matDatas[matIdx].m_relativePath);
                values = m_matDatas[matIdx].m_values;
                break;
            }
        }
        for (size_t matIdx = 0, end = m_materials.size(); matIdx < end; ++matIdx) {
            if (m_materials[matIdx].m_name == typeValue) {
                v = Q3DStudio::CString::fromQString(m_materials[matIdx].m_relativePath);
                break;
            }
        }
    }

    const auto sceneEditor = g_StudioApp.GetCore()->GetDoc()->getSceneEditor();
    const Q3DStudio::CString oldType = sceneEditor->GetObjectTypeName(instance);

    qt3dsdm::Qt3DSDMInstanceHandle refMaterial;
    if (oldType == "ReferencedMaterial" && typeValue == getStandardMaterialString())
        sceneEditor->getMaterialReference(instance, refMaterial);

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(),
                                      QObject::tr("Set Material Type"))
            ->SetMaterialType(instance, v);

    if (refMaterial.Valid())
        sceneEditor->copyMaterialProperties(refMaterial, instance);

    if (changeMaterialFile) {
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(),
                                          QObject::tr("Set Material Properties"))
                ->setMaterialProperties(instance, name, srcPath, values);

        // Select original instance again since potentially
        // creating a material selects the created one
        const auto doc = g_StudioApp.GetCore()->GetDoc();
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
    const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    const auto bridge = studio->GetClientDataModelBridge();
    // Name property needs special handling
    if (handle == bridge->GetNameProperty()) {
        // Ignore preview of name property change
        if (!commit)
            return;

        m_modifiedProperty.first = 0;
        m_modifiedProperty.second = 0;
        m_previouslyCommittedValue = {};

        Q3DStudio::CString currentName = bridge->GetName(instance);
        Q3DStudio::CString newName = Q3DStudio::CString::fromQString(value.toString());
        if (!newName.IsEmpty()) {
            qt3dsdm::Qt3DSDMInstanceHandle parentInstance = bridge->GetParentInstance(instance);
            if (!bridge->CheckNameUnique(parentInstance, instance, newName)) {
                QString origNewName = newName.toQString();
                newName = bridge->GetUniqueChildName(parentInstance, instance, newName);
                // Display rename message box asynchronously so focus loss won't trigger setting
                // the name again
                g_StudioApp.GetDialogs()->DisplayObjectRenamed(origNewName, newName.toQString(),
                                                               true);
            }
            if (newName != currentName) {
                bool canRename = false;

                const auto sceneEditor = g_StudioApp.GetCore()->GetDoc()->getSceneEditor();
                if (sceneEditor->isInsideMaterialContainer(instance)) {
                    const auto doc = g_StudioApp.GetCore()->GetDoc();
                    const auto dirPath = doc->GetDocumentDirectory().toQString();
                    for (size_t matIdx = 0, end = m_matDatas.size(); matIdx < end; ++matIdx) {
                        if (m_matDatas[matIdx].m_name == currentName.toQString()) {
                            QFileInfo fileInfo(dirPath + QStringLiteral("/")
                                               + m_matDatas[matIdx].m_relativePath);
                            const QString oldFile = fileInfo.absoluteFilePath();
                            const QString newFile = fileInfo.absolutePath()
                                    + QStringLiteral("/")
                                    + newName.toQString()
                                    + QStringLiteral(".matdata");
                            canRename = QFile::rename(oldFile, newFile);
                            break;
                        }
                    }
                } else {
                    canRename = true;
                }

                if (canRename) {
                    Q3DStudio::SCOPED_DOCUMENT_EDITOR(
                                *g_StudioApp.GetCore()->GetDoc(),
                                QObject::tr("Set Name"))->SetName(instance, newName, false);
                }
            }
        }
        return;
    }

    qt3dsdm::SValue oldValue = currentPropertyValue(instance, handle);
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

    // If the user enters 0.0 to any (x, y, z) values of camera scale,
    // we reset the value back to original, because zero scale factor will crash
    // camera-specific inverse matrix math. (Additionally, scale of zero for a camera
    // is generally not useful anyway.) We could silently discard zero values also deeper in the
    // value setter code, but then the inspector panel value would not be updated as opposed
    // to both rejecting invalid and resetting the original value here.
    EStudioObjectType theType = bridge->GetObjectType(instance);
    qt3dsdm::AdditionalMetaDataType::Value additionalType
            = studio->GetPropertySystem()->GetAdditionalMetaDataType(instance, handle);

    if (theType == EStudioObjectType::OBJTYPE_CAMERA &&
            studio->GetPropertySystem()->GetName(handle) == Q3DStudio::CString("scale")) {
        const QVector3D theFloat3 = qt3dsdm::get<QVector3D>(v);
        if (theFloat3.x() == 0.0f || theFloat3.y() == 0.0f || theFloat3.z() == 0.0f )
            v = oldValue;
    } else if ((theType == EStudioObjectType::OBJTYPE_CUSTOMMATERIAL
         || theType == EStudioObjectType::OBJTYPE_EFFECT) &&
            studio->GetPropertySystem()->GetDataType(handle)
                == qt3dsdm::DataModelDataType::String
            && additionalType == qt3dsdm::AdditionalMetaDataType::Texture) {
        // force . at the beginning of the url
        QString x = value.toString();
        QUrl url(x);
        if (url.isRelative() && !x.startsWith("./")) {
            QString s = QString("./%1").arg(url.path());
            v = QVariant::fromValue(s);
        }
    }

    // some properties may initialize OpenGL resources (e.g. loading meshes will
    // initialize vertex buffers), so the renderer's OpenGL context must be current
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
    theRenderer.MakeContextCurrent();

    m_UpdatableEditor.EnsureEditor(L"Set Property", __FILE__, __LINE__)
            .SetInstancePropertyValue(instance, handle, v);

    theRenderer.ReleaseContext();

    m_UpdatableEditor.FireImmediateRefresh(instance);

    if (commit) {
        m_modifiedProperty.first = 0;
        m_modifiedProperty.second = 0;
        if (m_previouslyCommittedValue == v)
            m_UpdatableEditor.RollbackEditor();
        else
            m_UpdatableEditor.CommitEditor();

        m_previouslyCommittedValue = {};
        refreshTree();
    }

    auto refMaterial = getReferenceMaterial(instance);
    if (refMaterial.Valid())
        saveIfMaterial(refMaterial);
    else
        saveIfMaterial(instance);
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
    return g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge()
            ->GetObjectType(instance) == EStudioObjectType::OBJTYPE_LAYER;
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

void InspectorControlModel::onSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster,
                                              int inOldIndex, int inNewIndex)
{
    Q_UNUSED(inMaster);
    Q_UNUSED(inOldIndex);
    Q_UNUSED(inNewIndex);
    rebuildTree();
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
    }
    return {};
}

QHash<int, QByteArray> InspectorControlModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(GroupValuesRole, "values");
    names.insert(GroupTitleRole, "title");
    return names;
}

InspectorControlBase::~InspectorControlBase()
{
}
