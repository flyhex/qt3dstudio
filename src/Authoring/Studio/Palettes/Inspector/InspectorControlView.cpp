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
#include "DataInputSelectDlg.h"
#include "MainFrm.h"

#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qdesktopwidget.h>

InspectorControlView::InspectorControlView(const QSize &preferredSize, QWidget *parent)
    : QQuickWidget(parent),
      TabNavigable(),
      m_inspectorControlModel(new InspectorControlModel(this)),
      m_instance(0),
      m_handle(0),
      m_preferredSize(preferredSize)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &InspectorControlView::initialize);
    auto dispatch = g_StudioApp.GetCore()->GetDispatch();
    dispatch->AddPresentationChangeListener(this);
    dispatch->AddDataModelListener(this);

    m_selectionChangedConnection = g_StudioApp.GetCore()->GetDispatch()->ConnectSelectionChange(
                std::bind(&InspectorControlView::OnSelectionSet, this, std::placeholders::_1));
}

const wchar_t **AllSupportedExtensionsList()
{
    static const wchar_t *extensions[] = {
        L"png", L"jpg", L"jpeg", L"dds", L"hdr",
        L"mesh", L"import", L"path",
        L"material",
        nullptr
    };
    return extensions;
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
        nullptr
    };
    for (size_t i = 0; i < m_fileList.size(); ++i) {
        if (isInList(extensions, m_fileList[i].GetExtension()))
            materials.push_back(m_fileList[i]);
    }
}

void InspectorControlView::OnNewPresentation()
{
    m_DirectoryConnection = g_StudioApp.GetDirectoryWatchingSystem().AddDirectory(
                g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory().toQString(),
                std::bind(&InspectorControlView::onFilesChanged, this, std::placeholders::_1));
}

void InspectorControlView::OnClosingPresentation()
{
    m_fileList.clear();
}

void InspectorControlView::OnLoadedSubPresentation()
{
    m_DirectoryConnection = g_StudioApp.GetDirectoryWatchingSystem().AddDirectory(
                g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory().toQString(),
                std::bind(&InspectorControlView::onFilesChanged, this, std::placeholders::_1));
}

void InspectorControlView::OnTimeChanged()
{
    m_inspectorControlModel->refresh();
}

void InspectorControlView::onFilesChanged(
        const Q3DStudio::TFileModificationList &inFileModificationList)
{
    const wchar_t **extensions = AllSupportedExtensionsList();
    for (size_t idx = 0, end = inFileModificationList.size(); idx < end; ++idx) {
        const Q3DStudio::SFileModificationRecord &record(inFileModificationList[idx]);
        if (record.m_FileInfo.IsFile()
                && isInList(extensions, record.m_File.GetExtension())) {
            Q3DStudio::CFilePath relativePath(
                        Q3DStudio::CFilePath::GetRelativePathFromBase(
                            g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory(),
                            record.m_File));

            if (record.m_ModificationType == Q3DStudio::FileModificationType::Created)
                qt3dsdm::binary_sort_insert_unique(m_fileList, relativePath);
            else if (record.m_ModificationType == Q3DStudio::FileModificationType::Destroyed)
                qt3dsdm::binary_sort_erase(m_fileList, relativePath);
        }
        if (record.m_FileInfo.IsFile()
                && record.m_ModificationType == Q3DStudio::FileModificationType::Modified) {
            if (record.m_File.toQString() == g_StudioApp.GetCore()->GetDoc()
                    ->GetDocumentUIAFile(false)) {
                m_inspectorControlModel->refreshRenderables();
            }
        }
    }
    std::vector<Q3DStudio::CFilePath> materials;
    filterMaterials(materials);
    m_inspectorControlModel->setMaterials(materials);
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

void InspectorControlView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_inspectorView"_L1, this);
    rootContext()->setContextProperty("_inspectorModel"_L1, m_inspectorControlModel);
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());
    rootContext()->setContextProperty("_tabOrderHandler"_L1, tabOrderHandler());
    rootContext()->setContextProperty("_mouseHelper"_L1, &m_mouseHelper);

    qmlRegisterUncreatableType<qt3dsdm::DataModelDataType>("Qt3DStudio", 1, 0, "DataModelDataType",
                                                         "DataModelDataType is an enum container");
    qmlRegisterUncreatableType<qt3dsdm::AdditionalMetaDataType>(
                "Qt3DStudio", 1, 0, "AdditionalMetaDataType",
                "AdditionalMetaDataType is an enum container");
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Inspector/InspectorControlView.qml"_L1));
}

QAbstractItemModel *InspectorControlView::inspectorControlModel() const
{
    return m_inspectorControlModel;
}

QString InspectorControlView::titleText() const
{
    if (m_inspectableBase) {
        Q3DStudio::CString theName = m_inspectableBase->GetName();
        if (theName == L"PathAnchorPoint")
            return tr("Anchor Point");
        else
            return theName.toQString();
    }
    return tr("No Object Selected");
}

static EStudioObjectType instanceObjectType(int instance)
{
    auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studio = doc->GetStudioSystem();
    return studio->GetClientDataModelBridge()->GetObjectType(instance);
}

bool InspectorControlView::canLinkProperty(int instance, int handle) const
{
    EStudioObjectType type = instanceObjectType(instance);
    bool canBeLinkedFlag = g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                                .CanPropertyBeLinked(instance, handle);
    if (qt3dsdm::Qt3DSDMPropertyHandle(handle).Valid() == false
            && (type == OBJTYPE_CUSTOMMATERIAL || type == OBJTYPE_MATERIAL
                || type == OBJTYPE_REFERENCEDMATERIAL)) {
        canBeLinkedFlag = false;
    }
    return canBeLinkedFlag;
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
        return CStudioObjectTypes::GetNormalIconName(m_inspectableBase->GetObjectType());
    return {};
}

void InspectorControlView::OnSelectionSet(Q3DStudio::SSelectedValue inSelectable)
{
    updateInspectable(g_StudioApp.GetInspectableFromSelectable(inSelectable));
}

void InspectorControlView::updateInspectable(CInspectableBase *inInspectable)
{
    if (inInspectable != nullptr) {
        if (inInspectable->IsValid() == false)
            inInspectable = nullptr;
    }
    setInspectable(inInspectable);
}

void InspectorControlView::setInspectable(CInspectableBase *inInspectable)
{
    if (m_inspectableBase != inInspectable) {
        m_inspectableBase = inInspectable;
        m_inspectorControlModel->setInspectable(inInspectable);
        Q_EMIT titleChanged();
        auto sp = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystem()->GetSignalProvider();
        m_PropertyChangeConnection = sp->ConnectInstancePropertyValue(
                    std::bind(&InspectorControlView::titleChanged, this));
        m_timeChanged = sp->ConnectComponentSeconds(
                    std::bind(&InspectorControlView::OnTimeChanged, this));
    }
}

void InspectorControlView::showContextMenu(int x, int y, int handle, int instance)
{
    m_instance = instance;
    m_handle = handle;

    QMenu theContextMenu;

    auto doc = g_StudioApp.GetCore()->GetDoc();

    bool canBeLinkedFlag = canLinkProperty(instance, handle);
    if (canBeLinkedFlag) {
        const bool isLinkedFlag = doc->GetDocumentReader().IsPropertyLinked(instance, handle);

        if (isLinkedFlag) {
            auto action = theContextMenu.addAction(QObject::tr("Unlink Property from Master Slide"));
            action->setEnabled(canBeLinkedFlag);
            connect(action, &QAction::triggered, this, &InspectorControlView::toggleMasterLink);
        } else {
            auto action = theContextMenu.addAction(QObject::tr("Link Property from Master Slide"));
            action->setEnabled(canBeLinkedFlag);
            connect(action, &QAction::triggered, this, &InspectorControlView::toggleMasterLink);
        }

    } else {
        auto action = theContextMenu.addAction(QObject::tr("Unable to link from Master Slide"));
        action->setEnabled(false);
    }
    theContextMenu.exec(mapToGlobal({x, y}));
    m_instance = 0;
    m_handle = 0;
}

void InspectorControlView::toggleMasterLink()
{
    Q3DStudio::ScopedDocumentEditor editor(*g_StudioApp.GetCore()->GetDoc(),
                                           L"Link Property", __FILE__, __LINE__);
    bool wasLinked = editor->IsPropertyLinked(m_instance, m_handle);

    if (wasLinked)
        editor->UnlinkProperty(m_instance, m_handle);
    else
        editor->LinkProperty(m_instance, m_handle);
}

void InspectorControlView::setPropertyValueFromFilename(long instance, int handle,
                                                        const QString &name)
{
    if (m_inspectorControlModel) {
        QString value;
        if (name != tr("[None]")) {
            // Relativize the path to the project
            const auto doc = g_StudioApp.GetCore()->GetDoc();
            const QDir documentDir(doc->GetDocumentDirectory().toQString());
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
                [this] (int handle, int instance, const QString &imageName){
            setPropertyValueFromFilename(instance, handle, imageName);
            m_imageChooserView->hide();
        });
    }

    m_imageChooserView->setHandle(handle);
    m_imageChooserView->setInstance(instance);

    showBrowser(m_imageChooserView, point);

    return m_imageChooserView;
}

QObject *InspectorControlView::showFilesChooser(int handle, int instance, const QPoint &point)
{
    if (!m_fileChooserView) {
        m_fileChooserView = new FileChooserView(this);
        connect(m_fileChooserView, &FileChooserView::fileSelected, this,
                [this] (int handle, int instance, const QString &fileName){
            setPropertyValueFromFilename(instance, handle, fileName);
            m_fileChooserView->hide();
        });
    }

    m_fileChooserView->setHandle(handle);
    m_fileChooserView->setInstance(instance);

    showBrowser(m_fileChooserView, point);

    return m_fileChooserView;
}

QObject *InspectorControlView::showMeshChooser(int handle, int instance, const QPoint &point)
{
    if (!m_meshChooserView) {
        m_meshChooserView = new MeshChooserView(this);
        connect(m_meshChooserView, &MeshChooserView::meshSelected, this,
                [this] (int handle, int instance, const QString &name){
            if (name.startsWith(QStringLiteral("#"))) {
                if (m_inspectorControlModel)
                    m_inspectorControlModel->setPropertyValue(instance, handle, name);
            } else {
                setPropertyValueFromFilename(instance, handle, name);
            }
        });
    }

    m_meshChooserView->setHandle(handle);
    m_meshChooserView->setInstance(instance);

    showBrowser(m_meshChooserView, point);

    return m_meshChooserView;
}

QObject *InspectorControlView::showTextureChooser(int handle, int instance, const QPoint &point)
{
    if (!m_textureChooserView) {
        m_textureChooserView = new TextureChooserView(this);
        connect(m_textureChooserView, &TextureChooserView::textureSelected, this,
                [this] (int handle, int instance, const QString &fileName){
            setPropertyValueFromFilename(instance, handle, fileName);
            m_textureChooserView->hide();
        });
    }

    m_textureChooserView->setHandle(handle);
    m_textureChooserView->setInstance(instance);

    showBrowser(m_textureChooserView, point);

    return m_textureChooserView;
}

QObject *InspectorControlView::showObjectReference(int handle, int instance, const QPoint &point)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    if (!m_objectReferenceModel) {
        m_objectReferenceModel
            = new ObjectListModel(g_StudioApp.GetCore(), doc->GetActiveRootInstance(), this);
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
        m_objectReferenceView->selectAndExpand(refInstance);
    }

    showBrowser(m_objectReferenceView, point);

    connect(m_objectReferenceView, &ObjectBrowserView::selectionChanged,
            this, [this, doc, handle, instance] {
        auto selectedItem = m_objectReferenceView->selectedHandle();
        qt3dsdm::SObjectRefType objRef = doc->GetDataModelObjectReferenceHelper()->GetAssetRefValue(
                    selectedItem, handle,
                    (CRelativePathTools::EPathType)(m_objectReferenceView->pathType()));
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Property"))
                ->SetInstancePropertyValue(instance, handle, objRef);
    });

    return m_objectReferenceView;
}

void InspectorControlView::showDataInputChooser(int handle, int instance, const QPoint &point)
{
    if (!m_dataInputChooserView) {
        m_dataInputChooserView = new DataInputSelectDlg(g_StudioApp.m_pMainWnd);
        connect(m_dataInputChooserView, &DataInputSelectDlg::dataInputChanged, this,
                [this, handle, instance](const QString &controllerName) {

            bool controlled = controllerName == tr("[No control]") ? false : true;
            m_inspectorControlModel
                ->setPropertyControllerInstance(
                    instance, handle,
                    Q3DStudio::CString::fromQString(controllerName), controlled);
            m_inspectorControlModel->setPropertyControlled(instance, handle);
            m_inspectorControlModel->setCurrentController(controllerName);
        });
    }

    QStringList dataInputList;
    dataInputList.append(tr("[No control]"));
    for (int i = 0; i < g_StudioApp.m_dataInputDialogItems.size(); i++)
        dataInputList.append(g_StudioApp.m_dataInputDialogItems[i]->name);

    m_dataInputChooserView->setData(dataInputList,
                                    m_inspectorControlModel->getCurrentController());
    m_dataInputChooserView->setWindowModality(Qt::WindowModality::ApplicationModal);
    m_dataInputChooserView->setWindowFlags(Qt::Popup);
    m_dataInputChooserView->showDialog(point);
}

void InspectorControlView::showBrowser(QQuickWidget *browser, const QPoint &point)
{
    QSize popupSize = CStudioPreferences::browserPopupSize();
    browser->resize(popupSize);

    // Make sure the popup doesn't go outside the screen
    QSize screenSize = QApplication::desktop()->availableGeometry(
                QApplication::desktop()->screenNumber(this)).size();
    QPoint newPos = point - QPoint(popupSize.width(), popupSize.height());
    if (newPos.y() < 0)
        newPos.setY(0);
    if (newPos.x() + popupSize.width() > screenSize.width())
        newPos.setX(screenSize.width() - popupSize.width());
    else if (newPos.x() < 0)
        newPos.setX(0);
    browser->move(newPos);

    // Show asynchronously to avoid flashing blank window on first show
    QTimer::singleShot(0, this, [browser] {
        browser->show();
        browser->activateWindow();
        browser->setFocus();
    });
}

void InspectorControlView::OnBeginDataModelNotifications()
{

}

void InspectorControlView::OnEndDataModelNotifications()
{
    CInspectableBase *inspectable = m_inspectorControlModel->inspectable();
    if (inspectable && !inspectable->IsValid())
        OnSelectionSet(Q3DStudio::SSelectedValue());
    m_inspectorControlModel->refresh();
}

void InspectorControlView::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    m_inspectorControlModel->refresh();
}

void InspectorControlView::OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    m_inspectorControlModel->refresh();
}
