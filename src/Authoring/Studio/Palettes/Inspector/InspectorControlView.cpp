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

#include "InspectorControlView.h"
#include "Literals.h"
#include "CColor.h"
#include "Qt3DSDMValue.h"
#include "StudioUtils.h"
#include "InspectorControlModel.h"
#include "StudioPreferences.h"
#include "Core.h"
#include "Doc.h"
#include "IDocumentEditor.h"
#include "ImageChooserModel.h"
#include "ImageChooserView.h"
#include "MeshChooserView.h"
#include "TextureChooserView.h"
#include "InspectableBase.h"
#include "StudioApp.h"
#include "ObjectListModel.h"
#include "ObjectBrowserView.h"
#include "IDirectoryWatchingSystem.h"
#include "StandardExtensions.h"
#include "FileChooserView.h"
#include "IObjectReferenceHelper.h"
#include "Qt3DSDMStudioSystem.h"
#include "StudioFullSystem.h"
#include "ClientDataModelBridge.h"
#include "MainFrm.h"
#include "DataInputDlg.h"
#include "Dialogs.h"
#include "ProjectFile.h"
#include "MaterialRefView.h"
#include "BasicObjectsModel.h"
#include "Qt3DSDMSlides.h"
#include "VariantsGroupModel.h"
#include "VariantTagDialog.h"
#include "SlideView.h"
#include "TimelineWidget.h"
#include "SelectedValue.h"
#include "Qt3DSDMInspectable.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMMaterialInspectable.h"
#include "GuideInspectable.h"

#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qlistwidget.h>

InspectorControlView::InspectorControlView(const QSize &preferredSize, QWidget *parent)
    : QQuickWidget(parent),
      TabNavigable(),
      m_variantsGroupModel(new VariantsGroupModel(this)),
      m_inspectorControlModel(new InspectorControlModel(m_variantsGroupModel, this)),
      m_meshChooserView(new MeshChooserView(this)),
      m_preferredSize(preferredSize)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &InspectorControlView::initialize);
    auto dispatch = g_StudioApp.GetCore()->GetDispatch();
    dispatch->AddPresentationChangeListener(this);
    dispatch->AddDataModelListener(this);

    connect(m_meshChooserView, &MeshChooserView::meshSelected, this,
            [this] (int handle, int instance, const QString &name) {
        if (name.startsWith(QLatin1Char('#'))) {
            if (m_inspectorControlModel)
                m_inspectorControlModel->setPropertyValue(instance, handle, name);
        } else {
            setPropertyValueFromFilename(instance, handle, name);
        }
    });
}

static bool isInList(const wchar_t **list, const Q3DStudio::CString &inStr)
{
    for (const wchar_t **item = list; item && *item; ++item) {
        if (inStr.Compare(*item, Q3DStudio::CString::ENDOFSTRING, false))
            return true;
    }
    return false;
}

void InspectorControlView::filterMaterials(std::vector<Q3DStudio::CFilePath> &materials)
{
    static const wchar_t *extensions[] = {
        L"material",
        L"shader",
        nullptr
    };
    for (size_t i = 0; i < m_fileList.size(); ++i) {
        if (isInList(extensions, m_fileList[i].GetExtension()))
            materials.push_back(m_fileList[i]);
    }
}

void InspectorControlView::filterMatDatas(std::vector<Q3DStudio::CFilePath> &matDatas)
{
    static const wchar_t *extensions[] = {
        L"materialdef",
        nullptr
    };
    for (size_t i = 0; i < m_fileList.size(); ++i) {
        if (isInList(extensions, m_fileList[i].GetExtension()))
            matDatas.push_back(m_fileList[i]);
    }
}

void InspectorControlView::OnNewPresentation()
{
    auto core = g_StudioApp.GetCore();
    auto sp = core->GetDoc()->GetStudioSystem()->GetFullSystem()->GetSignalProvider();
    auto assetGraph = core->GetDoc()->GetAssetGraph();

    m_connections.push_back(core->GetDispatch()->ConnectSelectionChange(
                std::bind(&InspectorControlView::OnSelectionSet, this, std::placeholders::_1)));
    m_connections.push_back(g_StudioApp.getDirectoryWatchingSystem().AddDirectory(
                g_StudioApp.GetCore()->getProjectFile().getProjectPath(),
                std::bind(&InspectorControlView::onFilesChanged, this, std::placeholders::_1)));
    m_connections.push_back(sp->ConnectInstancePropertyValue(
                std::bind(&InspectorControlView::onPropertyChanged, this, std::placeholders::_1,
                          std::placeholders::_2)));
    m_connections.push_back(assetGraph->ConnectChildAdded(
                std::bind(&InspectorControlView::onChildAdded, this, std::placeholders::_2)));
    m_connections.push_back(assetGraph->ConnectChildRemoved(
                std::bind(&InspectorControlView::onChildRemoved, this)));
}

void InspectorControlView::OnClosingPresentation()
{
    // Image chooser model needs to be deleted, because otherwise it'll try to update the model for
    // the new presentation before subpresentations are resolved, corrupting the model.
    // The model also has a connection to project file which needs to refreshed if project changes.
    delete m_imageChooserView;
    m_fileList.clear();
    m_connections.clear();
}

void InspectorControlView::onFilesChanged(
        const Q3DStudio::TFileModificationList &inFileModificationList)
{
    static const wchar_t *materialExtensions[] = {
        L"material", L"shader", L"materialdef",
        nullptr
    };
    static const wchar_t *fontExtensions[] = {
        L"ttf", L"otf",
        nullptr
    };

    bool updateFonts = false;
    for (size_t idx = 0, end = inFileModificationList.size(); idx < end; ++idx) {
        const Q3DStudio::SFileModificationRecord &record(inFileModificationList[idx]);
        if (record.m_FileInfo.IsFile()) {
            if (isInList(materialExtensions, record.m_File.GetExtension())) {
                Q3DStudio::CFilePath relativePath(
                            Q3DStudio::CFilePath::GetRelativePathFromBase(
                                g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory(),
                                record.m_File));

                if (record.m_ModificationType == Q3DStudio::FileModificationType::Created)
                    qt3dsdm::binary_sort_insert_unique(m_fileList, relativePath);
                else if (record.m_ModificationType == Q3DStudio::FileModificationType::Destroyed)
                    qt3dsdm::binary_sort_erase(m_fileList, relativePath);
            } else if (isInList(fontExtensions, record.m_File.GetExtension())) {
                if (record.m_ModificationType == Q3DStudio::FileModificationType::Created
                    || record.m_ModificationType == Q3DStudio::FileModificationType::Destroyed) {
                    updateFonts = true;
                }
            } else if (record.m_ModificationType == Q3DStudio::FileModificationType::Modified
                       && record.m_File.toQString()
                       == g_StudioApp.GetCore()->getProjectFile().getProjectFilePath()) {
                g_StudioApp.GetCore()->getProjectFile().loadSubpresentationsAndDatainputs(
                                g_StudioApp.m_subpresentations, g_StudioApp.m_dataInputDialogItems);
                m_inspectorControlModel->refreshRenderables();
            }
        }
    }
    std::vector<Q3DStudio::CFilePath> materials;
    filterMaterials(materials);
    m_inspectorControlModel->setMaterials(materials);

    std::vector<Q3DStudio::CFilePath> matDatas;
    filterMatDatas(matDatas);
    m_inspectorControlModel->setMatDatas(matDatas);

    if (updateFonts) {
        // The fonts list in doc is not necessarily yet updated, so do update async
        QTimer::singleShot(0, this, [this]() {
            m_inspectorControlModel->updateFontValues(nullptr);
        });
    }
}

InspectorControlView::~InspectorControlView()
{
    g_StudioApp.GetCore()->GetDispatch()->RemovePresentationChangeListener(this);
    delete m_dataInputChooserView;
}

QSize InspectorControlView::sizeHint() const
{
    return m_preferredSize;
}

void InspectorControlView::mousePressEvent(QMouseEvent *event)
{
    g_StudioApp.setLastActiveView(this);
    QQuickWidget::mousePressEvent(event);
}

void InspectorControlView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_parentView"), this);
    rootContext()->setContextProperty(QStringLiteral("_inspectorModel"), m_inspectorControlModel);
    rootContext()->setContextProperty(QStringLiteral("_variantsGroupModel"), m_variantsGroupModel);
    rootContext()->setContextProperty(QStringLiteral("_resDir"), StudioUtils::resourceImageUrl());
    rootContext()->setContextProperty(QStringLiteral("_tabOrderHandler"), tabOrderHandler());
    rootContext()->setContextProperty(QStringLiteral("_mouseHelper"), &m_mouseHelper);
    rootContext()->setContextProperty(QStringLiteral("_utils"), &m_qmlUtils);
    m_mouseHelper.setWidget(this);

    qmlRegisterUncreatableType<qt3dsdm::DataModelDataType>(
                "Qt3DStudio", 1, 0, "DataModelDataType",
                QStringLiteral("DataModelDataType is an enum container"));
    qmlRegisterUncreatableType<qt3dsdm::AdditionalMetaDataType>(
                "Qt3DStudio", 1, 0, "AdditionalMetaDataType",
                QStringLiteral("AdditionalMetaDataType is an enum container"));
    engine()->addImportPath(StudioUtils::qmlImportPath());
    setSource(QUrl(QStringLiteral("qrc:/Palettes/Inspector/InspectorControlView.qml")));
}

QAbstractItemModel *InspectorControlView::inspectorControlModel() const
{
    return m_inspectorControlModel;
}

QString InspectorControlView::titleText() const
{
    if (m_inspectableBase) {
        Q3DStudio::CString theName = m_inspectableBase->getName();
        if (theName == L"PathAnchorPoint")
            return tr("Anchor Point");
        else
            return theName.toQString();
    }
    return tr("No Object Selected");
}

bool InspectorControlView::isRefMaterial(int instance) const
{
    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    return bridge->IsReferencedMaterialInstance(instance);
}

QString InspectorControlView::noneString() const
{
    return ChooserModelBase::noneString();
}

bool InspectorControlView::canLinkProperty(int instance, int handle) const
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();

    if (bridge->isInsideMaterialContainer(instance))
        return false;

    if (bridge->IsMaterialBaseInstance(instance)) // all material types are unlinkable
        return false;

    if (handle == bridge->GetSceneAsset().m_Eyeball.m_Property) // eyeball is unlinkable
        return false;

    return doc->GetDocumentReader().CanPropertyBeLinked(instance, handle);
}

bool InspectorControlView::canOpenInInspector(int instance, int handle) const
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::SValue value;
    doc->GetPropertySystem()->GetInstancePropertyValue(instance, handle, value);
    if (!value.empty() && value.getType() == qt3dsdm::DataModelDataType::Long4) {
        qt3dsdm::SLong4 guid = qt3dsdm::get<qt3dsdm::SLong4>(value);
        return guid.Valid();
    }
    return false;
}

void InspectorControlView::openInInspector()
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    qt3dsdm::SValue value;
    doc->GetPropertySystem()->GetInstancePropertyValue(m_contextMenuInstance, m_contextMenuHandle,
                                                       value);
    qt3dsdm::SLong4 guid = qt3dsdm::get<qt3dsdm::SLong4>(value);
    const auto instance = bridge->GetInstanceByGUID(guid);
    doc->SelectDataModelObject(instance);
}

void InspectorControlView::onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                             qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    m_inspectorControlModel->notifyPropertyChanged(inInstance, inProperty);

    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    // titleChanged implies icon change too, but that will only occur if inspectable type changes,
    // which will invalidate the inspectable anyway, so in reality we are only interested in name
    // property here
    if (inProperty == bridge->GetNameProperty() && m_inspectableBase
        && m_inspectableBase->isValid()) {
        Q_EMIT titleChanged();
    }
}

void InspectorControlView::onChildAdded(int inChild)
{
    // Changes to asset graph invalidate the object browser model, so close it if it is open
    if (m_activeBrowser.isActive() && m_activeBrowser.m_browser == m_objectReferenceView)
        m_activeBrowser.clear();

    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    if (bridge->IsCustomMaterialInstance(inChild)) {
        QVector<qt3dsdm::Qt3DSDMInstanceHandle> refMats;
        doc->getSceneReferencedMaterials(doc->GetSceneInstance(), refMats);
        for (auto &refMat : qAsConst(refMats)) {
            if ((int)bridge->getMaterialReference(refMat) == inChild)
                g_StudioApp.GetCore()->GetDispatch()->FireImmediateRefreshInstance(refMat);
        }
    }
}

void InspectorControlView::onChildRemoved()
{
    // Changes to asset graph invalidate the object browser model, so close it if it is open
    if (m_activeBrowser.isActive() && m_activeBrowser.m_browser == m_objectReferenceView)
        m_activeBrowser.clear();
}

QColor InspectorControlView::titleColor(int instance, int handle) const
{
    QColor ret = CStudioPreferences::textColor();
    if (instance != 0) {
        if (g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                .IsPropertyLinked(instance, handle)) {
            ret = CStudioPreferences::masterColor();
        }
    }
    return ret;
}

QString InspectorControlView::titleIcon() const
{
    if (m_inspectableBase)
        return CStudioObjectTypes::GetNormalIconName(m_inspectableBase->getObjectType());
    return {};
}

bool InspectorControlView::isEditable(int handle) const
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    if (doc->GetStudioSystem()->GetSlideSystem()->IsMasterSlide(doc->GetActiveSlide())
            && doc->GetStudioSystem()->GetPropertySystem()->GetName(handle) == L"eyeball") {
        return false;
    }
    return true;
}

void InspectorControlView::OnSelectionSet(Q3DStudio::SSelectedValue selectable)
{
    CInspectableBase *inspectable = createInspectableFromSelectable(selectable);

    if (inspectable && !inspectable->isValid())
        inspectable = nullptr;

    setInspectable(inspectable);
}

CInspectableBase *InspectorControlView::createInspectableFromSelectable(
                                                        Q3DStudio::SSelectedValue selectable)
{
    using namespace Q3DStudio;

    CInspectableBase *inspectableBase = nullptr;
    if (!selectable.empty()) {
        switch (selectable.getType()) {
        case SelectedValueTypes::Slide: {
            // TODO: seems like slides are not directly selectable, this should be removed.
            auto selectableInstance = selectable.getData<SSlideInstanceWrapper>().m_Instance;
            inspectableBase = new Qt3DSDMInspectable(selectableInstance);
        } break;

        case SelectedValueTypes::MultipleInstances:
        case SelectedValueTypes::Instance: {
            CDoc *doc = g_StudioApp.GetCore()->GetDoc();
            // Note: Inspector doesn't support multiple selection
            qt3dsdm::TInstanceHandleList selectedsInstances = selectable.GetSelectedInstances();
            if (selectedsInstances.size() == 1) {
                Qt3DSDMInstanceHandle selectedInstance = selectedsInstances[0];
                if (doc->GetDocumentReader().IsInstance(selectedInstance)) {
                    auto *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
                    qt3dsdm::Qt3DSDMSlideHandle activeSlide = doc->GetActiveSlide();

                    // Scene or Component (when being edited)
                    if (selectedInstance == bridge->GetOwningComponentInstance(activeSlide)) {
                        Qt3DSDMInstanceHandle activeSlideInstance = doc->GetStudioSystem()
                                              ->GetSlideSystem()->GetSlideInstance(activeSlide);
                        inspectableBase = new Qt3DSDMInspectable(selectedInstance,
                                                                 activeSlideInstance);
                    } else if (bridge->IsMaterialBaseInstance(selectedInstance)) {
                        inspectableBase = new Qt3DSDMMaterialInspectable(selectedInstance);
                    } else {
                        inspectableBase = new Qt3DSDMInspectable(selectedInstance);
                    }
                }
            }
        } break;

        case SelectedValueTypes::Guide: {
            qt3dsdm::Qt3DSDMGuideHandle guide = selectable.getData<qt3dsdm::Qt3DSDMGuideHandle>();
            inspectableBase = new GuideInspectable(guide);
        } break;

        default:
            break; // Ignore slide insertion and unknown selectable types
        };
    }

    return inspectableBase;
}

void InspectorControlView::setInspectable(CInspectableBase *inInspectable)
{
    if (m_inspectableBase != inInspectable) {
        m_activeBrowser.clear();
        m_inspectableBase = inInspectable;
        m_inspectorControlModel->setInspectable(inInspectable);

        Q_EMIT titleChanged();

        m_variantsGroupModel->refresh();
    }
}

void InspectorControlView::showContextMenu(int x, int y, int handle, int instance)
{
    m_contextMenuInstance = instance;
    m_contextMenuHandle = handle;

    QMenu theContextMenu;

    auto doc = g_StudioApp.GetCore()->GetDoc();

    if (canOpenInInspector(instance, handle)) {
        auto action = theContextMenu.addAction(tr("Open in Inspector"));
        connect(action, &QAction::triggered, this, &InspectorControlView::openInInspector);
    }

    if (canLinkProperty(instance, handle)) {
        bool isLinked = doc->GetDocumentReader().IsPropertyLinked(instance, handle);
        auto action = theContextMenu.addAction(isLinked ? tr("Unlink Property from Master Slide")
                                                        : tr("Link Property from Master Slide"));
        connect(action, &QAction::triggered, this, &InspectorControlView::toggleMasterLink);
    } else {
        auto action = theContextMenu.addAction(tr("Unable to link from Master Slide"));
        action->setEnabled(false);
    }

    theContextMenu.exec(mapToGlobal({x, y}));
    m_contextMenuInstance = 0;
    m_contextMenuHandle = 0;
}

// context menu for the variants tags
void InspectorControlView::showTagContextMenu(int x, int y, const QString &group,
                                              const QString &tag)
{
    QMenu theContextMenu;

    auto actionRename = theContextMenu.addAction(QObject::tr("Rename Tag"));
    connect(actionRename, &QAction::triggered, this, [&]() {
        VariantTagDialog dlg(VariantTagDialog::RenameTag, group, tag);
        if (dlg.exec() == QDialog::Accepted) {
            g_StudioApp.GetCore()->getProjectFile().renameVariantTag(group, dlg.getNames().first,
                                                                     dlg.getNames().second);
            m_variantsGroupModel->refresh();

            // refresh slide view so the tooltip show the renamed tag immediately, no need to
            // refresh the timeline because each row gets the tags directly from the property which
            // is always up to date.
            g_StudioApp.m_pMainWnd->getSlideView()->refreshVariants();
        }
    });

    auto actionDelete = theContextMenu.addAction(QObject::tr("Delete Tag"));
    connect(actionDelete, &QAction::triggered, this, [&]() {
        g_StudioApp.GetCore()->getProjectFile().deleteVariantTag(group, tag);
        g_StudioApp.m_pMainWnd->getTimelineWidget()->refreshVariants();
        g_StudioApp.m_pMainWnd->getSlideView()->refreshVariants();
        m_variantsGroupModel->refresh();
        if (g_StudioApp.GetCore()->getProjectFile().variantsDef()[group].m_tags.size() == 0)
            g_StudioApp.m_pMainWnd->updateActionFilterEnableState();
    });

    theContextMenu.exec(mapToGlobal({x, y}));
}

// context menu for the variants groups
void InspectorControlView::showGroupContextMenu(int x, int y, const QString &group)
{
    QMenu theContextMenu;

    ProjectFile &projectFile = g_StudioApp.GetCore()->getProjectFile();

    auto actionRename = theContextMenu.addAction(QObject::tr("Rename Group"));
    connect(actionRename, &QAction::triggered, this, [&]() {
        VariantTagDialog dlg(VariantTagDialog::RenameGroup, {}, group);
        if (dlg.exec() == QDialog::Accepted) {
            projectFile.renameVariantGroup(dlg.getNames().first, dlg.getNames().second);
            g_StudioApp.m_pMainWnd->getTimelineWidget()->refreshVariants();
            m_variantsGroupModel->refresh();

            // refresh slide view so the tooltip show the renamed group immediately, no need to
            // refresh the timeline because each row gets the tags directly from the property which
            // is always up to date.
            g_StudioApp.m_pMainWnd->getSlideView()->refreshVariants();
        }
    });

    auto actionColor = theContextMenu.addAction(QObject::tr("Change Group Color"));
    connect(actionColor, &QAction::triggered, this, [&]() {
        const auto variantsDef = projectFile.variantsDef();
        QColor newColor = this->showColorDialog(variantsDef[group].m_color);
        projectFile.changeVariantGroupColor(group, newColor.name());
        // no need to refresh variants in the timeline widget as it references the group color in
        // the project file m_variants, and a redraw is triggered upon color selection dialog close.
        g_StudioApp.m_pMainWnd->getSlideView()->refreshVariants();
        m_variantsGroupModel->refresh();
    });

    auto actionDelete = theContextMenu.addAction(QObject::tr("Delete Group"));
    connect(actionDelete, &QAction::triggered, this, [&]() {
        projectFile.deleteVariantGroup(group);
        g_StudioApp.m_pMainWnd->getTimelineWidget()->refreshVariants();
        g_StudioApp.m_pMainWnd->getSlideView()->refreshVariants();
        g_StudioApp.m_pMainWnd->updateActionFilterEnableState();
        m_variantsGroupModel->refresh();
    });

    theContextMenu.exec(mapToGlobal({x, y}));
}

void InspectorControlView::toggleMasterLink()
{
    Q3DStudio::ScopedDocumentEditor editor(*g_StudioApp.GetCore()->GetDoc(),
                                           QObject::tr("Link Property"), __FILE__, __LINE__);
    bool wasLinked = editor->IsPropertyLinked(m_contextMenuInstance, m_contextMenuHandle);

    if (wasLinked)
        editor->UnlinkProperty(m_contextMenuInstance, m_contextMenuHandle);
    else
        editor->LinkProperty(m_contextMenuInstance, m_contextMenuHandle);
}

void InspectorControlView::setPropertyValueFromFilename(long instance, int handle,
                                                        const QString &name)
{
    if (m_inspectorControlModel) {
        QString value;
        if (name != ChooserModelBase::noneString()) {
            // Relativize the path to the presentation
            const auto doc = g_StudioApp.GetCore()->GetDoc();
            const QDir documentDir(doc->GetDocumentDirectory());
            QString relativeName = documentDir.relativeFilePath(name);
            value = relativeName;
        }
        m_inspectorControlModel->setPropertyValue(instance, handle, value);
    }
}

QObject *InspectorControlView::showImageChooser(int handle, int instance, const QPoint &point)
{
    if (!m_imageChooserView) {
        m_imageChooserView = new ImageChooserView(this);
        connect(m_imageChooserView, &ImageChooserView::imageSelected, this,
                [this] (int handle, int instance, const QString &imageName) {
            // To avoid duplicate undo points when setting image property we can't rely
            // on regular property duplication checks, as images are not directly stored as
            // their paths. Also, there is no check for duplication on renderables.
            if (m_imageChooserView->currentDataModelPath() != imageName) {
                QString renderableId = g_StudioApp.getRenderableId(imageName);
                if (renderableId.isEmpty()) {
                    setPropertyValueFromFilename(instance, handle, imageName);
                } else {
                    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(),
                                                      QObject::tr("Set Property"))
                            ->setInstanceImagePropertyValue(
                                instance, handle, Q3DStudio::CString::fromQString(renderableId));
                    if (m_inspectorControlModel)
                        m_inspectorControlModel->saveIfMaterial(instance);
                }
            }
        });
    }

    m_imageChooserView->setHandle(handle);
    m_imageChooserView->setInstance(instance);

    CDialogs::showWidgetBrowser(this, m_imageChooserView, point);
    m_activeBrowser.setData(m_imageChooserView, handle, instance);

    return m_imageChooserView;
}

QObject *InspectorControlView::showFilesChooser(int handle, int instance, const QPoint &point)
{
    if (!m_fileChooserView) {
        m_fileChooserView = new FileChooserView(this);
        connect(m_fileChooserView, &FileChooserView::fileSelected, this,
                [this] (int handle, int instance, const QString &fileName) {
            setPropertyValueFromFilename(instance, handle, fileName);
        });
    }

    m_fileChooserView->setHandle(handle);
    m_fileChooserView->setInstance(instance);

    CDialogs::showWidgetBrowser(this, m_fileChooserView, point);
    m_activeBrowser.setData(m_fileChooserView, handle, instance);

    return m_fileChooserView;
}

QObject *InspectorControlView::showMeshChooser(int handle, int instance, const QPoint &point)
{
    m_meshChooserView->setHandle(handle);
    m_meshChooserView->setInstance(instance);

    m_activeBrowser.setData(m_meshChooserView, handle, instance);
    int numPrimitives = BasicObjectsModel::BasicMeshesModel().count();
    bool combo = numPrimitives == m_meshChooserView->numMeshes(); // make a combobox size popup
    int comboH = qMin(m_meshChooserView->numMeshes(), 15) // max popup height: 15 items
                 * CStudioPreferences::controlBaseHeight();

    CDialogs::showWidgetBrowser(this, m_meshChooserView, point,
                                CDialogs::WidgetBrowserAlign::ComboBox,
                                combo ? QSize(CStudioPreferences::valueWidth(), comboH) : QSize());

    return m_meshChooserView;
}

QObject *InspectorControlView::showTextureChooser(int handle, int instance, const QPoint &point)
{
    if (!m_textureChooserView) {
        m_textureChooserView = new TextureChooserView(this);
        connect(m_textureChooserView, &TextureChooserView::textureSelected, this,
                [this] (int handle, int instance, const QString &fileName) {
            if (m_textureChooserView->currentDataModelPath() != fileName) {
                QString renderableId = g_StudioApp.getRenderableId(fileName);
                if (renderableId.isEmpty())
                    setPropertyValueFromFilename(instance, handle, fileName);
                else
                    m_inspectorControlModel->setPropertyValue(instance, handle, renderableId);
            }
        });
    }

    m_textureChooserView->setHandle(handle);
    m_textureChooserView->setInstance(instance);

    CDialogs::showWidgetBrowser(this, m_textureChooserView, point);
    m_activeBrowser.setData(m_textureChooserView, handle, instance);

    return m_textureChooserView;
}

QObject *InspectorControlView::showObjectReference(int handle, int instance, const QPoint &point)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    // different base handle than current active root instance means that we have entered/exited
    // component after the reference model had been created, and we need to recreate it
    if (!m_objectReferenceModel
        || (m_objectReferenceModel->baseHandle() != doc->GetActiveRootInstance())) {
        if (m_objectReferenceModel)
            delete m_objectReferenceModel;
        m_objectReferenceModel = new ObjectListModel(g_StudioApp.GetCore(),
                                                     doc->GetActiveRootInstance(), this, true);
    }
    if (!m_objectReferenceView)
        m_objectReferenceView = new ObjectBrowserView(this);
    m_objectReferenceView->setModel(m_objectReferenceModel);

    if (doc->GetStudioSystem()->GetClientDataModelBridge()
            ->GetObjectType(instance) == OBJTYPE_ALIAS) {
        QVector<EStudioObjectType> exclude;
        exclude << OBJTYPE_ALIAS << OBJTYPE_BEHAVIOR << OBJTYPE_CUSTOMMATERIAL
                << OBJTYPE_EFFECT << OBJTYPE_GUIDE << OBJTYPE_IMAGE << OBJTYPE_LAYER
                << OBJTYPE_MATERIAL << OBJTYPE_REFERENCEDMATERIAL << OBJTYPE_SCENE;
        m_objectReferenceModel->excludeObjectTypes(exclude);
    } else {
        m_objectReferenceModel->excludeObjectTypes(QVector<EStudioObjectType>());
    }

    disconnect(m_objectReferenceView, nullptr, nullptr, nullptr);

    IObjectReferenceHelper *objRefHelper = doc->GetDataModelObjectReferenceHelper();
    if (objRefHelper) {
        qt3dsdm::SValue value = m_inspectorControlModel->currentPropertyValue(instance, handle);
        qt3dsdm::Qt3DSDMInstanceHandle refInstance = objRefHelper->Resolve(value, instance);
        m_objectReferenceView->selectAndExpand(refInstance, instance);
    }

    CDialogs::showWidgetBrowser(this, m_objectReferenceView, point);
    m_activeBrowser.setData(m_objectReferenceView, handle, instance);

    connect(m_objectReferenceView, &ObjectBrowserView::selectionChanged,
            this, [this, doc, handle, instance] {
        auto selectedItem = m_objectReferenceView->selectedHandle();
        qt3dsdm::SObjectRefType objRef = doc->GetDataModelObjectReferenceHelper()->GetAssetRefValue(
                    selectedItem, instance,
                    (CRelativePathTools::EPathType)(m_objectReferenceView->pathType()));
        qt3dsdm::SValue value = m_inspectorControlModel->currentPropertyValue(instance, handle);
        if (!(value.getData<qt3dsdm::SObjectRefType>() == objRef)) {
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Property"))
                    ->SetInstancePropertyValue(instance, handle, objRef);
        }
    });

    return m_objectReferenceView;
}

QObject *InspectorControlView::showMaterialReference(int handle, int instance, const QPoint &point)
{
    // create the list widget
    if (!m_matRefListWidget)
        m_matRefListWidget = new MaterialRefView(this);

    disconnect(m_matRefListWidget, &QListWidget::itemClicked, nullptr, nullptr);
    disconnect(m_matRefListWidget, &QListWidget::itemDoubleClicked, nullptr, nullptr);

    const QSize popupSize = m_matRefListWidget->refreshMaterials(instance, handle);
    CDialogs::showWidgetBrowser(this, m_matRefListWidget, point,
                                CDialogs::WidgetBrowserAlign::ComboBox, popupSize);
    m_activeBrowser.setData(m_matRefListWidget, handle, instance);

    connect(m_matRefListWidget, &QListWidget::itemClicked, this,
            [instance, handle](QListWidgetItem *item) {
        auto selectedInstance = item->data(Qt::UserRole).toInt();
        if (selectedInstance > 0) {
            qt3dsdm::SValue value;
            CDoc *doc = g_StudioApp.GetCore()->GetDoc();
            const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
            propertySystem->GetInstancePropertyValue(instance, handle, value);
            auto refInstance = doc->GetDataModelObjectReferenceHelper()->Resolve(value, instance);
            if (selectedInstance != refInstance) {
                auto objRef = doc->GetDataModelObjectReferenceHelper()->GetAssetRefValue(
                            selectedInstance, instance, CRelativePathTools::EPATHTYPE_GUID);
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Property"))
                        ->SetInstancePropertyValue(instance, handle, objRef);
            }
        }
    });
    connect(m_matRefListWidget, &QListWidget::itemDoubleClicked, this, [this]() {
        m_matRefListWidget->hide();
    });

    return m_matRefListWidget;
}

void InspectorControlView::showDataInputChooser(int handle, int instance, const QPoint &point)
{
    if (!m_dataInputChooserView) {
        const QVector<EDataType> acceptedTypes;
        m_dataInputChooserView = new DataInputSelectView(acceptedTypes, this);
        connect(m_dataInputChooserView, &DataInputSelectView::dataInputChanged, this,
                [this](int handle, int instance, const QString &controllerName) {
            bool controlled =
                    controllerName == m_dataInputChooserView->getNoneString() ? false : true;
            m_inspectorControlModel
                ->setPropertyControllerInstance(
                    instance, handle,
                    Q3DStudio::CString::fromQString(controllerName), controlled);
            m_inspectorControlModel->setPropertyControlled(instance, handle);
        });
    }
    const auto propertySystem =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetPropertySystem();
    const qt3dsdm::DataModelDataType::Value dataType
        = propertySystem->GetDataType(handle);
    // only add datainputs with matching type for this property
    QVector<QPair<QString, int>> dataInputList;

   for (auto &it : qAsConst(g_StudioApp.m_dataInputDialogItems))
        dataInputList.append({it->name, it->type});

    m_dataInputChooserView->setMatchingTypes(CDataInputDlg::getAcceptedTypes(dataType));
    m_dataInputChooserView->
            setData(dataInputList,
                    m_inspectorControlModel->currentControllerValue(instance, handle),
                    handle, instance);
    CDialogs::showWidgetBrowser(this, m_dataInputChooserView, point,
                                CDialogs::WidgetBrowserAlign::ToolButton);
    m_activeBrowser.setData(m_dataInputChooserView, handle, instance);
}

QColor InspectorControlView::showColorDialog(const QColor &color, int instance, int handle)
{
    bool showAlpha = false;
    if (instance && handle) {
        auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
                                                     ->GetClientDataModelBridge();
        showAlpha = bridge->getBGColorProperty(instance).GetHandleValue() == handle
                || bridge->getTextColorProperty(instance).GetHandleValue() == handle;
    }

    m_currentColor = color;
    CDialogs *dialogs = g_StudioApp.GetDialogs();
    connect(dialogs, &CDialogs::onColorChanged,
            this, &InspectorControlView::dialogCurrentColorChanged);
    QColor currentColor = dialogs->displayColorDialog(color, showAlpha);
    disconnect(dialogs, &CDialogs::onColorChanged,
               this, &InspectorControlView::dialogCurrentColorChanged);
    return currentColor;
}

bool InspectorControlView::toolTipsEnabled()
{
    return CStudioPreferences::ShouldShowTooltips();
}

// Converts a path that is relative to the current presentation to be relative to
// the current project root
QString InspectorControlView::convertPathToProjectRoot(const QString &presentationPath)
{
    QDir projDir(g_StudioApp.GetCore()->getProjectFile().getProjectPath());
    QFileInfo presentationFile(g_StudioApp.GetCore()->GetDoc()->GetDocumentPath());
    QDir presentationDir(presentationFile.absolutePath());
    QString absPath = presentationDir.absoluteFilePath(presentationPath);

    return projDir.relativeFilePath(absPath);
}

void InspectorControlView::OnBeginDataModelNotifications()
{
}

void InspectorControlView::OnEndDataModelNotifications()
{
    CInspectableBase *inspectable = m_inspectorControlModel->inspectable();
    if (inspectable && !inspectable->isValid())
        OnSelectionSet(Q3DStudio::SSelectedValue());
    m_inspectorControlModel->refresh();

    if (m_activeBrowser.isActive()) {
        // Check if the instance/handle pair still has an active UI control. If not, close browser.
        if (!m_inspectorControlModel->hasInstanceProperty(
                m_activeBrowser.m_instance, m_activeBrowser.m_handle)) {
            m_activeBrowser.clear();
        } else {
            // Update browser selection
            if (m_activeBrowser.m_browser == m_imageChooserView) {
                m_imageChooserView->updateSelection();
            } else if (m_activeBrowser.m_browser == m_fileChooserView) {
                m_fileChooserView->updateSelection();
            } else if (m_activeBrowser.m_browser == m_meshChooserView) {
                m_meshChooserView->updateSelection();
            } else if (m_activeBrowser.m_browser == m_textureChooserView) {
                m_textureChooserView->updateSelection();
            } else if (m_activeBrowser.m_browser == m_objectReferenceView) {
                IObjectReferenceHelper *objRefHelper
                        = g_StudioApp.GetCore()->GetDoc()->GetDataModelObjectReferenceHelper();
                if (objRefHelper) {
                    qt3dsdm::SValue value = m_inspectorControlModel->currentPropertyValue(
                                m_activeBrowser.m_instance, m_activeBrowser.m_handle);
                    qt3dsdm::Qt3DSDMInstanceHandle refInstance
                            = objRefHelper->Resolve(value, m_activeBrowser.m_instance);
                    m_objectReferenceView->selectAndExpand(refInstance, m_activeBrowser.m_instance);
                }
            } else if (m_activeBrowser.m_browser == m_matRefListWidget) {
                m_matRefListWidget->updateSelection();
            } else if (m_activeBrowser.m_browser == m_dataInputChooserView) {
                m_dataInputChooserView->setCurrentController(
                            m_inspectorControlModel->currentControllerValue(
                                m_dataInputChooserView->instance(),
                                m_dataInputChooserView->handle()));
            }
        }
    }
}

void InspectorControlView::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    m_inspectorControlModel->refresh();
}

void InspectorControlView::OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    m_inspectorControlModel->refresh();
}
