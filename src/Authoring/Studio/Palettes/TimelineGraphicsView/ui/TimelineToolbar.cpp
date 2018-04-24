/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "TimelineToolbar.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "Dispatch.h"
#include "DataInputSelectView.h"
#include "DataInputDlg.h"
#include "Qt3DSDMStudioSystem.h"
#include "StudioPreferences.h"
#include "ClientDataModelBridge.h"
#include "IDocumentEditor.h"
#include "DocumentEditorEnumerations.h"

#include <QtWidgets/qslider.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimer.h>

TimelineToolbar::TimelineToolbar() : QToolBar()
{
    setContentsMargins(0, 0, 0, 0);
    setIconSize(QSize(15, 15));

    // create icons
    static const QIcon iconLayer = QIcon(":/images/Asset-Layer-Normal.png");
    static const QIcon iconDelete = QIcon(":/images/Action-Trash-Normal.png");
    static const QIcon iconFirst = QIcon(":/images/playback_tools_low-00.png");
    static const QIcon iconLast = QIcon(":/images/playback_tools_low-04.png");
    static const QIcon iconZoomIn = QIcon(":/images/zoom_in.png");
    static const QIcon iconZoomOut = QIcon(":/images/zoom_out.png");
    m_iconDiActive = QIcon(":/images/Objects-DataInput-Normal.png");
    m_iconDiInactive = QIcon(":/images/Objects-DataInput-Disabled.png");
    m_iconStop = QIcon(":/images/playback_tools_low-01.png");
    m_iconPlay = QIcon(":/images/playback_tools_low-02.png");

    // create actions
    QAction *actionNewLayer = new QAction(iconLayer, tr("Add New Layer"));
    QAction *actionFirst = new QAction(iconFirst, tr("Go to Timeline Start"));
    QAction *actionLast = new QAction(iconLast, tr("Go to Timeline End"));
    m_actionDataInput = new QAction(m_iconDiActive, "");
    m_actionDeleteRow = new QAction(iconDelete, tr("Delete Selected Object"));
    m_actionPlayStop = new QAction();
    m_timeLabel = new TimelineToolbarLabel();
    m_diLabel = new QLabel();
    m_actionZoomIn = new QAction(iconZoomIn, tr("Zoom In"));
    m_actionZoomOut = new QAction(iconZoomOut, tr("Zoom Out"));

    m_scaleSlider = new QSlider();
    m_scaleSlider->setOrientation(Qt::Horizontal);
    m_scaleSlider->setFixedWidth(100);
    m_scaleSlider->setMinimum(1);
    m_scaleSlider->setMaximum(22);
    m_scaleSlider->setPageStep(.1);
    m_scaleSlider->setValue(2);

    m_timeLabel->setText(tr("0:00.000"));
    m_timeLabel->setMinimumWidth(80);

    m_diLabel->setText("");
    m_diLabel->setMinimumWidth(100);
    m_diLabel->setAlignment(Qt::AlignCenter);
    QString styleString = "QLabel { background: transparent; color: "
            + QString(CStudioPreferences::dataInputColor().name()) + "; }";
    m_diLabel->setStyleSheet(styleString);

    updatePlayButtonState(false);

    // connections
    connect(actionNewLayer, &QAction::triggered, this, &TimelineToolbar::newLayerTriggered);
    connect(m_actionDeleteRow, &QAction::triggered, this, &TimelineToolbar::deleteLayerTriggered);
    connect(m_timeLabel, &TimelineToolbarLabel::clicked, this,
            &TimelineToolbar::gotoTimeTriggered);
    connect(actionFirst, &QAction::triggered, this, &TimelineToolbar::firstFrameTriggered);
    connect(m_actionPlayStop, &QAction::triggered, this, &TimelineToolbar::onPlayButtonClicked);
    connect(actionLast, &QAction::triggered, this, &TimelineToolbar::lastFrameTriggered);
    connect(m_scaleSlider, &QSlider::valueChanged, this, &TimelineToolbar::onZoomLevelChanged);
    connect(m_actionZoomIn, &QAction::triggered, this, &TimelineToolbar::onZoomInButtonClicked);
    connect(m_actionZoomOut, &QAction::triggered, this, &TimelineToolbar::onZoomOutButtonClicked);
    connect(m_actionDataInput, &QAction::triggered, this, &TimelineToolbar::onDiButtonClicked);

    // add actions
    addAction(actionNewLayer);
    addAction(m_actionDeleteRow);
    addAction(m_actionDataInput);
    addWidget(m_diLabel);
    addSpacing(20);
    addWidget(m_timeLabel);
    addSpacing(20);
    addAction(actionFirst);
    addAction(m_actionPlayStop);
    addAction(actionLast);
    addSpacing(30);
    addAction(m_actionZoomOut);
    addWidget(m_scaleSlider);
    addAction(m_actionZoomIn);

    // add keyboard shortcuts
    m_actionZoomOut->setShortcut(Qt::Key_Minus);
    m_actionZoomIn->setShortcut(Qt::Key_Plus);

    m_connectSelectionChange = g_StudioApp.GetCore()->GetDispatch()->ConnectSelectionChange(
                std::bind(&TimelineToolbar::onSelectionChange, this, std::placeholders::_1));

    // make datainput indicator listen to selection dialog choice
    m_dataInputSelector = new DataInputSelectView(this);
    g_StudioApp.GetCore()->GetDispatch()->AddDataModelListener(this);
    connect(m_dataInputSelector, &DataInputSelectView::dataInputChanged,
            this, &TimelineToolbar::onDataInputChange);
}

TimelineToolbar::~TimelineToolbar()
{
    delete m_dataInputSelector;
}

void TimelineToolbar::onSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    qt3dsdm::TInstanceHandleList selectedInstances = inNewSelectable.GetSelectedInstances();
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *theClientBridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    bool canDelete = false;
    for (size_t idx = 0, end = selectedInstances.size(); idx < end; ++idx) {
        if (theClientBridge->CanDelete(selectedInstances[idx])) {
            canDelete = true;
            break;
        }
    }

    m_actionDeleteRow->setEnabled(canDelete);

    updateDataInputStatus(false);
}

// add a spacer widget
void TimelineToolbar::addSpacing(int width)
{
    auto *widget = new QWidget;
    widget->setStyleSheet("background-color:0,0,0,0;"); // make the widget transparent
    widget->setFixedWidth(width);
    addWidget(widget);
}

void TimelineToolbar::setTime(long totalMillis)
{
    long mins = totalMillis % 3600000 / 60000;
    long secs = totalMillis % 60000 / 1000;
    long millis = totalMillis % 1000;

    m_timeLabel->setText(QString::asprintf("%01d:%02d.%03d", mins, secs, millis));
}

void TimelineToolbar::updatePlayButtonState(bool started)
{
    if (started) {
        m_actionPlayStop->setIcon(m_iconStop);
        m_actionPlayStop->setText(tr("Stop Playing"));
    } else {
        m_actionPlayStop->setIcon(m_iconPlay);
        m_actionPlayStop->setText(tr("Start Playing"));
    }
}

void TimelineToolbar::onPlayButtonClicked()
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    if (doc->IsPlaying())
        emit stopTriggered();
    else
        emit playTriggered();
}

void TimelineToolbar::onZoomLevelChanged(int scale)
{
    m_actionZoomIn->setEnabled(scale < m_scaleSlider->maximum());
    m_actionZoomOut->setEnabled(scale > m_scaleSlider->minimum());

    emit timelineScaleChanged(scale);
}

void TimelineToolbar::onZoomInButtonClicked()
{
    m_scaleSlider->setValue(m_scaleSlider->value() + 1);
}

void TimelineToolbar::onZoomOutButtonClicked()
{
    m_scaleSlider->setValue(m_scaleSlider->value() - 1);
}

void TimelineToolbar::onDiButtonClicked()
{
    showDataInputChooser(mapToGlobal(pos()));
}

// Update datainput button state according to this timecontext
// control state. If triggered via datamodel change i.e. dispatch message,
// force update from actual property values
void TimelineToolbar::updateDataInputStatus(bool isViaDispatch)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMPropertyHandle ctrldProp;
    qt3dsdm::Qt3DSDMInstanceHandle timeCtxRoot = doc->GetActiveRootInstance();
    CClientDataModelBridge *theClientBridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    // Only check for updates if we have entered new time context or receive dispatch (undo/redo).
    // Actual control changes in this time context are handled in onDataInputChange
    if (m_currTimeCtxRoot != timeCtxRoot || isViaDispatch) {
        if (theClientBridge->GetObjectType(timeCtxRoot) == EStudioObjectType::OBJTYPE_SCENE) {
            ctrldProp = theClientBridge->GetObjectDefinitions().m_Scene.m_ControlledProperty;
        } else if (theClientBridge->GetObjectType(timeCtxRoot) ==
                   EStudioObjectType::OBJTYPE_COMPONENT) {
            ctrldProp = theClientBridge->GetObjectDefinitions().m_Component.m_ControlledProperty;
        } else {
            Q_ASSERT(false);
        }

        qt3dsdm::SValue controlledPropertyVal;
        doc->GetStudioSystem()->GetPropertySystem()->GetInstancePropertyValue(
                    timeCtxRoot, ctrldProp, controlledPropertyVal);
        auto existingCtrl = qt3dsdm::get<QString>(controlledPropertyVal);

        int slideStrPos = existingCtrl.indexOf("@timeline");
        if (slideStrPos != -1) {
            int ctrStrPos = existingCtrl.lastIndexOf("$", slideStrPos - 2);
            m_currController = existingCtrl.mid(ctrStrPos + 1, slideStrPos - ctrStrPos - 2);
        } else {
            m_currController.clear();
        }
        // Toggle if we changed to a controlled time context, or if icon current state
        // differs from the control state of current time context
        if (m_currController.size()) {
            m_actionDataInput->setToolTip(
                tr("Timeline Controller:\n%1").arg(m_currController));
            m_actionDataInput->setIcon(m_iconDiActive);
            updateTimelineTitleColor(true);
        } else {
            // TODO actually delete the entire property instead of setting it as empty string
            m_actionDataInput->setIcon(m_iconDiInactive);
            m_actionDataInput->setToolTip(tr("No control"));
            updateTimelineTitleColor(false);
        }

        m_currTimeCtxRoot = timeCtxRoot;
        m_diLabel->setText(m_currController);
    }
}

void TimelineToolbar::showDataInputChooser(const QPoint &point)
{
    QString currCtr = m_currController.size() ?
        m_currController : m_dataInputSelector->getNoneString();
    QVector<QPair<QString, int>> dataInputList;
    for (int i = 0; i < g_StudioApp.m_dataInputDialogItems.size(); i++) {
        if (g_StudioApp.m_dataInputDialogItems[i]->type == EDataType::DataTypeRangedNumber)
            dataInputList.append(QPair<QString, int>(g_StudioApp.m_dataInputDialogItems[i]->name,
                                                     g_StudioApp.m_dataInputDialogItems[i]->type));
    }
    m_dataInputSelector->setData(dataInputList, currCtr);

    showBrowser(m_dataInputSelector, point);
}

void TimelineToolbar::showBrowser(QQuickWidget *browser, const QPoint &point)
{
    QSize popupSize = CStudioPreferences::browserPopupSize();
    browser->resize(popupSize);

    // Make sure the popup doesn't go outside the screen
    QSize screenSize = QApplication::desktop()->availableGeometry(
                QApplication::desktop()->screenNumber(this)).size();
    QPoint newPos = point - QPoint(popupSize.width(), popupSize.height());
    if (newPos.y() < 0)
        newPos.setY(0);
    else if (newPos.y() - popupSize.height() < -screenSize.height())
        newPos.setY(-screenSize.height() + popupSize.height());

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

void TimelineToolbar::onDataInputChange(int handle, int instance, const QString &dataInputName)
{
    Q_UNUSED(handle)
    Q_UNUSED(instance)

    if (dataInputName == m_currController)
        return;

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    QString fullTimeControlStr;

    if (dataInputName != m_dataInputSelector->getNoneString()) {
        m_actionDataInput->setToolTip(tr("Timeline Controller:\n%1").arg(dataInputName));
        fullTimeControlStr = "$" + dataInputName + " @timeline";
        m_actionDataInput->setIcon(m_iconDiActive);
        m_currController = dataInputName;
        updateTimelineTitleColor(false);
    } else {
        m_actionDataInput->setToolTip(tr("No control"));
        // TODO actually delete the entire property instead of setting it as empty string
        m_actionDataInput->setIcon(m_iconDiInactive);
        m_currController.clear();
        updateTimelineTitleColor(true);
    }

    // To indicate that this presentation timeline is controlled by data input,
    // we set "controlled property" of this time context root (scene or component)
    // to contain the name of controller followed by special indicator "@timeline".
    // Either replace existing timeline control indicator string or append new one
    // but do not touch @slide indicator string as scene can have both
    qt3dsdm::Qt3DSDMPropertyHandle ctrldPropertyHandle;
    qt3dsdm::Qt3DSDMInstanceHandle timeCtxRoot = doc->GetActiveRootInstance();
    // Time context root is either scene or component
    if (bridge->GetObjectType(timeCtxRoot) == EStudioObjectType::OBJTYPE_SCENE)
        ctrldPropertyHandle = bridge->GetObjectDefinitions().m_Scene.m_ControlledProperty;
    else if (bridge->GetObjectType(timeCtxRoot) == EStudioObjectType::OBJTYPE_COMPONENT)
        ctrldPropertyHandle = bridge->GetObjectDefinitions().m_Component.m_ControlledProperty;
    else
        Q_ASSERT(false);

    qt3dsdm::SValue controlledPropertyVal;
    doc->GetStudioSystem()->GetPropertySystem()->GetInstancePropertyValue(
                timeCtxRoot, ctrldPropertyHandle, controlledPropertyVal);

    auto existingCtrl = qt3dsdm::get<QString>(controlledPropertyVal);
    int slideStrPos = existingCtrl.indexOf("@timeline");
    if (slideStrPos != -1) {
        // find the controlling datainput name and build the string to replace
        int ctrStrPos = existingCtrl.lastIndexOf("$", slideStrPos - 2);
        QString prevCtrler = existingCtrl.mid(ctrStrPos, slideStrPos - ctrStrPos);
        existingCtrl.replace(prevCtrler + "@timeline", fullTimeControlStr);
    } else {
        if (!existingCtrl.isEmpty() && m_currController.size())
            existingCtrl.append(" ");
        existingCtrl.append(fullTimeControlStr);
    }

    if (existingCtrl.endsWith(" "))
        existingCtrl.chop(1);

    if (existingCtrl.startsWith(" "))
        existingCtrl.remove(0, 1);

    m_currTimeCtxRoot = timeCtxRoot;
    m_diLabel->setText(m_currController);
    qt3dsdm::SValue fullCtrlPropVal
        = std::make_shared<qt3dsdm::CDataStr>(
            Q3DStudio::CString::fromQString(existingCtrl));
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Timeline control"))
        ->SetInstancePropertyValue(timeCtxRoot, ctrldPropertyHandle, fullCtrlPropVal);
}

void TimelineToolbar::OnBeginDataModelNotifications()
{
}

void TimelineToolbar::OnEndDataModelNotifications()
{
    updateDataInputStatus(true);
}

void TimelineToolbar::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    updateDataInputStatus(true);
}

void TimelineToolbar::OnImmediateRefreshInstanceMultiple(
    qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    updateDataInputStatus(true);
}

// Notify the user about control state change also with timeline dock
// title color change.
void TimelineToolbar::updateTimelineTitleColor(bool controlled)
{
    QString styleString;
    if (controlled) {
        styleString= "QDockWidget { color: "
                + QString(CStudioPreferences::dataInputColor().name()) + "; }";
    } else {
        styleString = "QDockWidget { color: "
                + QString(CStudioPreferences::textColor().name()) + "; }";
    }
    QWidget *timelineDock = parentWidget()->parentWidget();
    timelineDock->setStyleSheet(styleString);
}
