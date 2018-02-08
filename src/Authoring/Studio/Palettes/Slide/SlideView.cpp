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

#include "SlideView.h"
#include "CColor.h"
#include "Core.h"
#include "Dispatch.h"
#include "Doc.h"
#include "Literals.h"
#include "StudioPreferences.h"
#include "SlideModel.h"
#include "StudioApp.h"
#include "StudioUtils.h"
#include "SlideContextMenu.h"
#include "DataInputSelectDlg.h"
#include "DataInputDlg.h"
#include "IDocumentEditor.h"

#include "ClientDataModelBridge.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

SlideView::SlideView(QWidget *parent) : QQuickWidget(parent)
  , m_MasterSlideModel(new SlideModel(1, this))
  , m_SlidesModel(new SlideModel(0, this))
  , m_ActiveRoot(0)
{
    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_CurrentModel = m_SlidesModel;
    QTimer::singleShot(0, this, &SlideView::initialize);
}

SlideView::~SlideView()
{
    clearSlideList();
    g_StudioApp.GetCore()->GetDispatch()->RemovePresentationChangeListener(this);
    delete m_dataInputSelector;
}

bool SlideView::showMasterSlide() const
{
    return m_CurrentModel == m_MasterSlideModel;
}

void SlideView::setShowMasterSlide(bool show)
{
    const bool currentIsMaster = m_CurrentModel == m_MasterSlideModel;
    if (show == currentIsMaster)
        return;

    if (show)
        m_CurrentModel = m_MasterSlideModel;
    else
        m_CurrentModel = m_SlidesModel;

    // We need to get the first slide in the correct master mode
    CDoc *theDoc = GetDoc();
    qt3dsdm::Qt3DSDMInstanceHandle theRoot = theDoc->GetActiveRootInstance();
    CClientDataModelBridge *theBridge = GetBridge();
    qt3dsdm::Qt3DSDMSlideHandle theNewActiveSlide =
        theBridge->GetOrCreateGraphRoot(theRoot); // this will return the master slide
    qt3dsdm::ISlideSystem *theSlideSystem = theDoc->GetStudioSystem()->GetSlideSystem();
    if (m_CurrentModel != m_MasterSlideModel) {
        const auto theFind = m_MasterSlideReturnPointers.find(theNewActiveSlide);
        size_t theSlideIndex = 1;
        size_t theNumSlides = theSlideSystem->GetSlideCount(theNewActiveSlide);
        if (theFind != m_MasterSlideReturnPointers.end() && theFind->second < theNumSlides)
            theSlideIndex = theFind->second;

        theNewActiveSlide = theSlideSystem->GetSlideByIndex(
            theNewActiveSlide, theSlideIndex); // activate the first slide
    } else {
        int theIndex = theSlideSystem->GetActiveSlideIndex(theNewActiveSlide);
        m_MasterSlideReturnPointers[theNewActiveSlide] = theIndex;
    }

    // We have forced a mode change, and so we need to set the current active TC
    // to be in the correct mode so our slide palette will show the correct information
    if (theNewActiveSlide.Valid()) {
        theDoc->NotifyActiveSlideChanged(theNewActiveSlide);
    }

    Q_EMIT showMasterSlideChanged();
    Q_EMIT currentModelChanged();
}

void SlideView::showControllerDialog(const QPoint &point)
{
    QStringList dataInputList;
    dataInputList.append(tr("[No control]"));
    for (int i = 0; i < g_StudioApp.m_dataInputDialogItems.size(); i++) {
        if (g_StudioApp.m_dataInputDialogItems[i]->type == EDataType::DataTypeString)
            dataInputList.append(g_StudioApp.m_dataInputDialogItems[i]->name);
    }
    QString currCtr = m_currentController.size() ?
        m_currentController : tr("[No control]");
    m_dataInputSelector->setData(dataInputList, currCtr);
    m_dataInputSelector->showDialog(point);

    return;
}

QSize SlideView::sizeHint() const
{
    return {150, 500};
}

void SlideView::deselectAll()
{
    g_StudioApp.GetCore()->GetDoc()->DeselectAllItems();
}

void SlideView::addNewSlide(int row)
{
    m_SlidesModel->addNewSlide(row);
}

void SlideView::removeSlide(int row)
{
    m_SlidesModel->removeSlide(row);
}

void SlideView::duplicateSlide(int row)
{
    m_SlidesModel->duplicateRow(row);
}

void SlideView::startSlideRearrange(int row)
{
    m_SlidesModel->startRearrange(row);
}

void SlideView::moveSlide(int from, int to)
{
    m_SlidesModel->move(from, to);
}

void SlideView::finishSlideRearrange(bool commit)
{
    m_SlidesModel->finishRearrange(commit);
}

void SlideView::showContextMenu(int x, int y, int row)
{
    SlideContextMenu contextMenu(this, row, m_SlidesModel->rowCount(),
                                 m_CurrentModel == m_MasterSlideModel);
    contextMenu.exec(mapToGlobal({x, y}));
}

void SlideView::OnNewPresentation()
{
    // Register callbacks
    qt3dsdm::IStudioFullSystemSignalProvider *theSignalProvider =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();
    m_MasterSlideReturnPointers.clear();

    m_Connections.push_back(theSignalProvider->ConnectActiveSlide(
        std::bind(&SlideView::OnActiveSlide, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));

    // Needed for undo/redo functionality to work properly
    m_Connections.push_back(theSignalProvider->ConnectSlideCreated(
        std::bind(&SlideView::OnNewSlide, this, std::placeholders::_1)));
    m_Connections.push_back(theSignalProvider->ConnectSlideDeleted(
        std::bind(&SlideView::OnDeleteSlide, this, std::placeholders::_1)));
    m_Connections.push_back(theSignalProvider->ConnectSlideRearranged(
        std::bind(&SlideView::OnSlideRearranged, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));

    // Set up listener for the name changes to slide
    m_Connections.push_back(theSignalProvider->ConnectInstancePropertyValue(
                std::bind(&SlideModel::refreshSlideLabel, m_SlidesModel,
                          std::placeholders::_1, std::placeholders::_2)));
    // Set up listener for undo/redo changes in order to update
    // slide datainput control
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    theDispatch->AddDataModelListener(this);
    updateDataInputStatus(false);
}

void SlideView::OnClosingPresentation()
{
    m_Connections.clear();
    clearSlideList();
}

void SlideView::OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                              const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    // Don't use inIndex because inIndex might have been changed due to deletion
    Q_UNUSED(inIndex);
    Q_UNUSED(inMaster);

    qt3dsdm::ISlideSystem &theSlideSystem(*GetDoc()->GetStudioSystem()->GetSlideSystem());
    setShowMasterSlide(theSlideSystem.GetSlideIndex(inSlide) == 0);
    setActiveSlide(inSlide);
}

void SlideView::OnNewSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    m_SlidesModel->onNewSlide(inSlide);
}

void SlideView::OnDeleteSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    m_SlidesModel->onDeleteSlide(inSlide);
}

void SlideView::OnSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inOldIndex,
                                  int inNewIndex)
{
    m_SlidesModel->onSlideRearranged(inMaster, inOldIndex, inNewIndex);
}

void SlideView::onDataInputChange(const QString &dataInputName)
{
    if (dataInputName == m_currentController ||
        (dataInputName == tr("[No Control]") && !m_currentController.size())) {
        return;
    }

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    qt3dsdm::Qt3DSDMInstanceHandle slideRoot = doc->GetSceneInstance();
    QString fullSlideControlStr;

    if (dataInputName != tr("[No control]")) {
        fullSlideControlStr = dataInputName + " @slide";
        m_controlled = true;
        m_currentController = dataInputName;
        m_toolTip = tr("Slide Controller:\n") + m_currentController;
    } else {
        m_controlled = false;
        m_currentController.clear();
        m_toolTip = tr("No controller");
    }

    qt3dsdm::Qt3DSDMPropertyHandle ctrldProp
            = bridge->GetObjectDefinitions().m_Scene.m_ControlledProperty;
    qt3dsdm::Option<qt3dsdm::SValue> controlledPropertyVal
        = Q3DStudio::SCOPED_DOCUMENT_EDITOR(
            *doc,
            QObject::tr("Get DataInput control"))->GetInstancePropertyValue(slideRoot,
                                                                            ctrldProp);

    // To indicate that slide transitions are controlled by data input,
    // we set "controlled property" of this scene to contain the name of
    // controller followed by special indicator "@slide".
    // If we have existing slide control in this root element, replace it.
    // Otherwise just append slide control string to controlledproperty
    // (it might already contain timeline control information)
    auto existingCtrl = qt3dsdm::get<QString>(controlledPropertyVal.getValue());
    if (existingCtrl.contains("@slide")) {
        int slideStrPos = existingCtrl.indexOf("@slide");
        // find the controlling datainput name and build the string to replace
        int ctrStrPos = existingCtrl.lastIndexOf(" ", slideStrPos - 2);
        QString prevCtrler = existingCtrl.mid(ctrStrPos + 1, slideStrPos - ctrStrPos - 1);
        existingCtrl.replace(prevCtrler + "@slide", fullSlideControlStr);
    } else {
        (!existingCtrl.isEmpty() && m_controlled) ? existingCtrl.append(" ") : 0;
        existingCtrl.append(fullSlideControlStr);
    }

    if (existingCtrl.endsWith(" "))
        existingCtrl.chop(1);

    if (existingCtrl.startsWith(" "))
        existingCtrl.remove(0, 1);

    qt3dsdm::SValue fullCtrlPropVal
            = std::make_shared<qt3dsdm::CDataStr>(
                Q3DStudio::CString::fromQString(existingCtrl));

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Slide control"))
        ->SetInstancePropertyValue(slideRoot, ctrldProp, fullCtrlPropVal);

    Q_EMIT controlledChanged();
}

// Set the state of slide control based on scene
// controlledproperty
void SlideView::updateDataInputStatus(bool isViaDispatch)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    qt3dsdm::Qt3DSDMInstanceHandle slideRoot = doc->GetSceneInstance();
    qt3dsdm::Qt3DSDMPropertyHandle ctrldProp
        = bridge->GetObjectDefinitions().m_Scene.m_ControlledProperty;
    qt3dsdm::Option<qt3dsdm::SValue> controlledPropertyVal
        = Q3DStudio::SCOPED_DOCUMENT_EDITOR(
            *doc,
            QObject::tr("Get DataInput control"))->GetInstancePropertyValue(slideRoot,
                                                                            ctrldProp);

    auto existingCtrl = qt3dsdm::get<QString>(controlledPropertyVal.getValue());
    if (existingCtrl.contains("@slide")) {
        int slideStrPos = existingCtrl.indexOf("@slide");
        int ctrStrPos = existingCtrl.lastIndexOf(" ", slideStrPos - 2);
        m_currentController = existingCtrl.mid(ctrStrPos + 1, slideStrPos - ctrStrPos - 2);
        m_toolTip = tr("Slide Controller:\n") + m_currentController;
        m_controlled = true;
    } else {
        m_currentController.clear();
        m_toolTip = tr("No controller");
        m_controlled = false;
    }

    // update UI
    Q_EMIT controlledChanged();
}
void SlideView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_slideView"_L1, this);
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());

    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Slide/SlideView.qml"_L1));

    m_dataInputSelector = new DataInputSelectDlg(parentWidget());
    connect(m_dataInputSelector, &DataInputSelectDlg::dataInputChanged,
            this, &SlideView::onDataInputChange);
    m_dataInputSelector->hide();
    m_dataInputSelector->setWindowTitle(tr("Select slide controller"));
}

void SlideView::clearSlideList()
{
    m_ActiveRoot = 0;
    m_SlidesModel->clear();
}

void SlideView::setActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inActiveSlideHandle)
{
    // Make sure we are in the correct master mode based on the inActiveSlideHandle
    // If we changed mode, then we need to force a rebuild
    bool theRebuildFlag = isMaster(inActiveSlideHandle) && (m_CurrentModel != m_MasterSlideModel);

    // Check to see if the incoming slide is a sibling of the current active slide
    // If it is, then we may be able to update without rebuilding everything
    if (!theRebuildFlag
        && m_ActiveRoot == GetBridge()->GetOwningComponentInstance(inActiveSlideHandle)) {
        // If this is a new active slide, but the same root parent
        if (m_ActiveSlideHandle != inActiveSlideHandle) {
                m_ActiveSlideHandle = inActiveSlideHandle;
        }
    } else {
        // We have a new parent or a new slide that makes us rebuild the entire list
        rebuildSlideList(inActiveSlideHandle);
    }
}

void SlideView::rebuildSlideList(const qt3dsdm::Qt3DSDMSlideHandle &inActiveSlideHandle)
{
    // Clear out the existing slides
    clearSlideList();

    // Add new slide controls as required
    if (inActiveSlideHandle.Valid()) {
        m_ActiveSlideHandle = inActiveSlideHandle;
        m_ActiveRoot = GetBridge()->GetOwningComponentInstance(inActiveSlideHandle);

        // Get the Master Slide handle and the slide count
        qt3dsdm::ISlideSystem *theSlideSystem = GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
            theSlideSystem->GetMasterSlide(inActiveSlideHandle);

        // update handle for master slide
        qt3dsdm::Qt3DSDMSlideHandle theMasterSlideHandle =
                theSlideSystem->GetSlideByIndex(theMasterSlide, 0);
        m_MasterSlideModel->setData(m_MasterSlideModel->index(0, 0),
                                    QVariant::fromValue(theMasterSlideHandle),
                                    SlideModel::HandleRole);

        long theSlideCount = (long)theSlideSystem->GetSlideCount(theMasterSlide);

        // Iterate through, creating the new slide controls
        m_SlidesModel->clear();
        m_SlidesModel->insertRows(0, theSlideCount - 1, {});
        int row = 0;
        for (long theSlideIndex = 1; theSlideIndex < theSlideCount; ++theSlideIndex) {
            qt3dsdm::Qt3DSDMSlideHandle theSlideHandle =
                    theSlideSystem->GetSlideByIndex(theMasterSlide, theSlideIndex);
            auto index = m_SlidesModel->index(row, 0);
            m_SlidesModel->setData(index,
                                   QVariant::fromValue(theSlideHandle),
                                   SlideModel::HandleRole);
            const auto instanceHandle =
                    GetDoc()->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(theSlideHandle);
            m_SlidesModel->setData(index,
                                   GetBridge()->GetName(instanceHandle).toQString(),
                                   SlideModel::NameRole);
            // This slide is the active slide
            if (theSlideHandle == m_ActiveSlideHandle) {
                m_SlidesModel->setData(index, true, SlideModel::SelectedRole);
            }
            row++;
        }
    }
}

CDoc *SlideView::GetDoc()
{
    return g_StudioApp.GetCore()->GetDoc();
}

CClientDataModelBridge *SlideView::GetBridge()
{
    return GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
}

qt3dsdm::ISlideSystem *SlideView::GetSlideSystem()
{
    return GetDoc()->GetStudioSystem()->GetSlideSystem();
}

long SlideView::GetSlideIndex(const qt3dsdm::Qt3DSDMSlideHandle &inSlideHandle)
{
    return GetSlideSystem()->GetSlideIndex(inSlideHandle);
}

bool SlideView::isMaster(const qt3dsdm::Qt3DSDMSlideHandle &inSlideHandle)
{
    return (0 == GetSlideIndex(inSlideHandle));
}

void SlideView::OnBeginDataModelNotifications()
{
}

void SlideView::OnEndDataModelNotifications()
{
    updateDataInputStatus(true);
}

void SlideView::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    updateDataInputStatus(true);
}

void SlideView::OnImmediateRefreshInstanceMultiple(
    qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    updateDataInputStatus(true);
}


