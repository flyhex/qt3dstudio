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

#include <functional>

#include "InspectorControlModel.h"
#include "Core.h"
#include "Doc.h"
#include "ControlGraphIterators.h"
#include "InspectorGroup.h"
#include "UICDMInspectorGroup.h"
#include "UICDMInspectorRow.h"
#include "UICDMStudioSystem.h"
#include "UICDMInspectable.h"
#include "UICDMDataCore.h"
#include "StudioApp.h"
#include "IDocumentEditor.h"
#include "Control.h"
#include "ControlData.h"
#include "UICDMMetaData.h"
#include "UICDMSignals.h"
#include "CmdDataModelDeanimate.h"
#include "GuideInspectable.h"
#include "UICDMDataTypes.h"
#include "IObjectReferenceHelper.h"
#include "UICDMXML.h"
#include "UICDMStringTable.h"
#include "UICFileTools.h"
#include "UICDMSlideCore.h"
#include "SlideSystem.h"
#include "UICDMMaterialInspectable.h"
#include "ClientDataModelBridge.h"
#include "IDocumentReader.h"
#include "IStudioRenderer.h"

static QStringList renderableItems()
{
    QStringList renderables;
    renderables.push_back(QObject::tr("No renderable item"));
    const CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    Q3DStudio::CString docDir = doc->GetDocumentDirectory();
    Q3DStudio::CFilePath fullDocPath = doc->GetDocumentPath().GetAbsolutePath();
    Q3DStudio::CString docFilename = fullDocPath.GetFileName();
    // First step, find uia file, parse and pull out renderable asset id's but ignoring the
    // current presentation.
    std::vector<Q3DStudio::CFilePath> dirFiles;
    Q3DStudio::CFilePath thePath(docDir);
    thePath.ListFilesAndDirectories(dirFiles);
    for (size_t idx = 0, end = dirFiles.size(); idx < end; ++idx) {
        if (!dirFiles[idx].IsFile())
            continue;

        Q3DStudio::CString ext = dirFiles[idx].GetExtension();
        if (!ext.CompareNoCase("uia"))
            continue;

        qt3dsdm::TStringTablePtr theStringTable
                = qt3dsdm::IStringTable::CreateStringTable();
        std::shared_ptr<qt3dsdm::IDOMFactory> theDomFact =
            qt3dsdm::IDOMFactory::CreateDOMFactory(theStringTable);
        Q3DStudio::CString fullFile = dirFiles[idx];
        qt3ds::foundation::CFileSeekableIOStream theStream(
            fullFile, qt3ds::foundation::FileReadFlags());

        qt3dsdm::SDOMElement *theElem
                = qt3dsdm::CDOMSerializer::Read(*theDomFact, theStream);
        if (theElem) {
            std::shared_ptr<qt3dsdm::IDOMReader> theReader =
                qt3dsdm::IDOMReader::CreateDOMReader(*theElem, theStringTable,
                                                   theDomFact);
            if (theReader->MoveToFirstChild("assets")) {
                for (bool success = theReader->MoveToFirstChild(); success;
                     success = theReader->MoveToNextSibling()) {
                    if (qt3dsdm::AreEqual(theReader->GetElementName(), L"presentation") ||
                        qt3dsdm::AreEqual(theReader->GetElementName(), L"presentation-qml")) {
                        qt3dsdm::TXMLStr src = nullptr;
                        qt3dsdm::TXMLStr id = nullptr;
                        theReader->Att("src", src);
                        theReader->Att("id", id);
                        if (docFilename != src.c_str())
                            renderables.push_back(QString::fromLatin1(id.c_str()));
                    } else if (qt3dsdm::AreEqual(theReader->GetElementName(),
                                        L"renderplugin")) {
                        const wchar_t *id = nullptr;
                        theReader->UnregisteredAtt(L"id", id);
                        renderables.push_back(QString::fromWCharArray(id));
                    }
                }
            }
        }
    }
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
    qt3dsdm::CUICDMSlideHandle slide = slideCore.GetSlideByInstance(instance);
    qt3dsdm::CUICDMSlideHandle master = slideSystem.GetMasterSlide(slide);
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

void InspectorControlModel::setMaterials(std::vector<Q3DStudio::CFilePath> &materials)
{
    m_materials.clear();
    const Q3DStudio::CString base = g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory();

    for (Q3DStudio::CFilePath path : materials) {

        const QString relativePath = path.toQString();
        const Q3DStudio::CFilePath absolutePath
            = Q3DStudio::CFilePath::CombineBaseAndRelative(base, path);

        const QString name = g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                                  .GetCustomMaterialName(absolutePath).toQString();

        m_materials.push_back({name, relativePath});
    }
}

InspectorControlBase* InspectorControlModel::createMaterialItem(CUICDMInspectable *inspectable,
                                                        int groupIndex)
{
    const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    InspectorControlBase *item = new InspectorControlBase;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);

    item->m_title = tr("Material Type");

    CClientDataModelBridge *theBridge = studio->GetClientDataModelBridge();
    EStudioObjectType theType = theBridge->GetObjectType(item->m_instance);
    item->m_dataType = qt3dsdm::DataModelDataType::StringRef;
    item->m_propertyType = qt3dsdm::AdditionalMetaDataType::None;
    item->m_tooltip = tr("Type of material being used or custom material");

    item->m_animatable = false;

    QStringList values;
    values.push_back(tr("Standard Material"));
    values.push_back(tr("Referenced Material"));

    const QString sourcePath = theBridge->GetSourcePath(item->m_instance).toQString();

    switch (theType) {

    case OBJTYPE_MATERIAL:
        item->m_value = tr("Standard Material");
        break;

    case OBJTYPE_REFERENCEDMATERIAL:
        item->m_value = tr("Referenced Material");
        break;
    }

    for (size_t matIdx = 0, end = m_materials.size(); matIdx < end; ++matIdx) {
        values.push_back(m_materials[matIdx].m_name);
        if (m_materials[matIdx].m_relativePath == sourcePath)
            item->m_value = values.last();
    }

    item->m_values = values;

    return item;
}

InspectorControlBase* InspectorControlModel::createItem(CUICDMInspectable *inspectable,
                                                        Q3DStudio::CUICDMInspectorRow *row,
                                                        int groupIndex)
{
    return createItem(inspectable, row->GetMetaDataPropertyInfo(), groupIndex);
}

InspectorControlBase* InspectorControlModel::createItem(CUICDMInspectable *inspectable,
                                                        const qt3dsdm::SMetaDataPropertyInfo &metaProperty,
                                                        int groupIndex)
{
    const auto studio = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    if (metaProperty.m_IsHidden)
        return nullptr;

    InspectorControlBase *item = new InspectorControlBase;
    item->m_property = metaProperty.m_Property;
    item->m_instance = inspectable->GetGroupInstance(groupIndex);

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

    item->m_animatable = metaProperty.m_Animatable &&
            studio->GetAnimationSystem()->IsPropertyAnimatable(item->m_instance,
                                                               metaProperty.m_Property);
    if (item->m_animatable) {
        item->m_animated = studio->GetAnimationSystem()->IsPropertyAnimated(item->m_instance,
                                                                            metaProperty.m_Property);

        // Update the Animate Toggle on undo/redo
        auto signalProvider = studio->GetFullSystemSignalProvider();
        item->m_connections.push_back(signalProvider->ConnectAnimationCreated(
                                          std::bind(&InspectorControlModel::updateAnimateToggleState,
                                                    this, item)));

        item->m_connections.push_back(signalProvider->ConnectAnimationDeleted(
                                          std::bind(&InspectorControlModel::updateAnimateToggleState,
                                                    this, item)));
    }

    // synchronize the value itself
    updatePropertyValue(item);
    return item;
}

qt3dsdm::SValue InspectorControlModel::currentPropertyValue(long instance, int handle)
{
    qt3dsdm::SValue value;
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studioSystem = doc->GetStudioSystem();
    const auto propertySystem = studioSystem->GetPropertySystem();
    propertySystem->GetInstancePropertyValue(instance, handle,  value);

    return  value;
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
    if (m_groupElements.size() != theCount)
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

    if (const auto cdmInspectable = dynamic_cast<CUICDMInspectable *>(inspectable)) {
        int existingIndex = 0;
        if (const auto group = dynamic_cast<const CUICDMInspectorGroup *>(theInspectorGroup)) {
            const auto materialGroup
                    = dynamic_cast<const UICDMMaterialInspectorGroup *>(group);
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
        long theCount = inspectBase->GetGroupCount();
        for (long theIndex = 0; theIndex < theCount; ++theIndex) {
            result.append(computeGroup(inspectBase, theIndex));
        }
    }

    return result;
}

auto InspectorControlModel::computeGroup(CInspectableBase* inspectable,
                                         int theIndex)
    -> GroupInspectorControl
{
    CInspectorGroup* theInspectorGroup = inspectable->GetGroup(theIndex);
    GroupInspectorControl result;
    result.groupTitle = theInspectorGroup->GetName();

    if (const auto cdmInspectable = dynamic_cast<CUICDMInspectable *>(inspectable)) {
        if (const auto group = dynamic_cast<CUICDMInspectorGroup *>(theInspectorGroup)) {
            const auto materialGroup
                    = dynamic_cast<UICDMMaterialInspectorGroup *>(group);
            if (materialGroup && materialGroup->isMaterialGroup()) {
                InspectorControlBase *item = createMaterialItem(cdmInspectable, theIndex);
                if (item) {
                    result.controlElements.push_back(QVariant::fromValue(item));
                }
            }
            for (const auto row : group->GetRows()) {
                InspectorControlBase *item = createItem(cdmInspectable, row, theIndex);
                if (!item)
                    continue;

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
    m_groupElements = computeTree(m_inspectableBase);
    endResetModel();
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
    const auto instance = element->m_instance;
    propertySystem->GetInstancePropertyValue(instance, element->m_property, value);

    const auto metaDataProvider = doc->GetStudioSystem()->GetActionMetaData();
    const auto info = metaDataProvider->GetMetaDataPropertyInfo(
                metaDataProvider->GetMetaDataProperty(instance, element->m_property));
    switch (element->m_dataType) {
    case qt3dsdm::DataModelDataType::String:
        element->m_value = qt3dsdm::get<QString>(value);
        //intentional fall-through
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
                auto bridge = studioSystem->GetClientDataModelBridge();
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
            std::vector<Q3DStudio::CString> fontNames;
            g_StudioApp.GetCore()->GetDoc()->GetProjectFonts(fontNames);
            QStringList possibleValues;
            for (const auto &fontName: fontNames)
                possibleValues.append(fontName.toQString());
            element->m_values = possibleValues;
            element->m_value = qt3dsdm::get<QString>(value);
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Mesh) {
            QString meshValue = qt3dsdm::get<QString>(value);
            Q3DStudio::CFilePath theSelectionItem(Q3DStudio::CString::fromQString(meshValue));
            Q3DStudio::CFilePath theSelectionWithoutId(theSelectionItem.GetPathWithoutIdentifier());
            if (theSelectionWithoutId.size())
                element->m_value = theSelectionWithoutId.GetFileName().toQString();
            else
                element->m_value = theSelectionItem.GetIdentifier().toQString();
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::Texture) {
            QFileInfo fileInfo(qt3dsdm::get<QString>(value));
            element->m_value = fileInfo.fileName();
        } else if (element->m_propertyType == qt3dsdm::AdditionalMetaDataType::PathBuffer) {
            element->m_value = qt3dsdm::get<QString>(value);
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
    Q_EMIT element->valueChanged();
    Q_EMIT element->valuesChanged();
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
        long theCount = m_inspectableBase->GetGroupCount();
        for (long theIndex = 0; theIndex < theCount; ++theIndex) {
            if (isGroupRebuildRequired(m_inspectableBase, theIndex)) {
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
            if (property->m_property.Valid())
                updatePropertyValue(property);
        }
    }
    Q_EMIT dataChanged(index(0), index(rowCount() - 1));
}

void InspectorControlModel::setMaterialTypeValue(long instance, int handle, const QVariant &value)
{
    Q_UNUSED(handle);
    const QString typeValue = value.toString();
    Q3DStudio::CString v;

    if (typeValue == tr("Standard Material")) {
        v = Q3DStudio::CString("Standard Material");
    } else if (typeValue == tr("Referenced Material")) {
        v = Q3DStudio::CString("Referenced Material");
    } else {
        for (size_t matIdx = 0, end = m_materials.size(); matIdx < end; ++matIdx) {
            if (m_materials[matIdx].m_name == typeValue) {
                v = Q3DStudio::CString::fromQString(m_materials[matIdx].m_relativePath);
                break;
            }
        }
    }

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(),
                                      QObject::tr("Set Property"))->SetMaterialType(instance, v);
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

void InspectorControlModel::setPropertyValue(long instance, int handle, const QVariant &value, bool commit)
{
    qt3dsdm::SValue oldValue = currentPropertyValue(instance, handle);
    qt3dsdm::SValue v = value;

    // If this set is a commit for property that was previously changed without
    // committing, we must let the set go through even if the value hasn't changed
    // to finish the transaction.
    if (v == oldValue && !(commit && (m_modifiedProperty.first == instance
                                      && m_modifiedProperty.second == handle))) {
        return;
    }

    if (!commit && m_modifiedProperty.first == 0) {
        m_modifiedProperty.first = instance;
        m_modifiedProperty.second = handle;
    }

    // some properties may initialize OpenGL resources (e.g. loading meshes will
    // initialize vertex buffers), so the renderer's OpenGL context must be current
    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.GetRenderer());
    theRenderer.MakeContextCurrent();

    m_UpdatableEditor.EnsureEditor(L"Set Property", __FILE__, __LINE__)
            .SetInstancePropertyValue(instance, handle, v);

    theRenderer.ReleaseContext();

    m_UpdatableEditor.FireImmediateRefresh(instance);

    if (commit) {
        m_modifiedProperty.first = 0;
        m_modifiedProperty.second = 0;
        m_UpdatableEditor.CommitEditor();
        refreshTree();
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

void InspectorControlModel::onSlideRearranged(const qt3dsdm::CUICDMSlideHandle &inMaster,
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
