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

#include "TimeLineToolbar.h"
#include "ui_TimeLineToolbar.h"
#include "StudioApp.h"
#include "MainFrm.h"
#include "TimeEditDlg.h"
#include "DataInputSelectDlg.h"
#include "DataInputDlg.h"
#include "Doc.h"
#include "Core.h"
#include "Dispatch.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "StudioObjectTypes.h"
#include "IDocumentEditor.h"
#include "DocumentEditorEnumerations.h"
#include "StudioPreferences.h"

#include <QtCore/qdatetime.h>

TimeLineToolbar::TimeLineToolbar(CMainFrame *mainFrame, const QSize &preferredSize,
                                 QWidget *pParent)
    : QWidget(pParent)
    , m_ui(new QT_PREPEND_NAMESPACE(Ui::TimeLineToolbar))
    , m_preferredSize(preferredSize)
    , m_mainFrame(mainFrame)
    , m_currController(QString(tr("[No control]")))
    , m_DataInputSelector(nullptr)
{
    m_ui->setupUi(this);

    connect(m_ui->playButton, &QToolButton::clicked,
            this, &TimeLineToolbar::onPlayButtonClicked);
    connect(m_ui->rewindButton, &QToolButton::clicked,
            mainFrame, &CMainFrame::OnPlaybackRewind);
    connect(mainFrame, &CMainFrame::playStateChanged,
            [this](bool started) {
        if (started)
            m_ui->playButton->setIcon(QIcon(":/images/playback_tools_low-01.png"));
        else
            m_ui->playButton->setIcon(QIcon(":/images/playback_tools_low-02.png"));
    });

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    connect(m_ui->timeButton, &QPushButton::clicked,
            [this, doc](){
        CTimeEditDlg timeEditDlg;
        timeEditDlg.showDialog(doc->GetCurrentViewTime(), doc, PLAYHEAD);
    });

    connect(m_ui->deleteObject, &QPushButton::clicked,
            doc, &CDoc::DeleteSelectedObject);

    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    m_Connections.push_back(theDispatch->ConnectSelectionChange(
        std::bind(&TimeLineToolbar::OnSelectionChange, this, std::placeholders::_1)));

    connect(m_ui->addLayerButton, &QPushButton::clicked,
            this, &TimeLineToolbar::onAddLayerClicked);
    // Set as parent to mainframe to allow positioning in the main window
    m_DataInputSelector = new DataInputSelectDlg(mainFrame);
    m_DataInputSelector->hide();
    m_ui->addDataInputButton->setVisible(false);

    connect(m_ui->addDataInputButton, &QPushButton::clicked,
            this, &TimeLineToolbar::onAddDataInputClicked);

    theDispatch->AddDataModelListener(this);
    connect(m_DataInputSelector, &DataInputSelectDlg::dataInputChanged,
            this, &TimeLineToolbar::onDataInputChange);
}

TimeLineToolbar::~TimeLineToolbar()
{
    delete m_ui;
    m_Connections.clear();
}

QSize TimeLineToolbar::sizeHint() const
{
    return m_preferredSize;
}

void TimeLineToolbar::onTimeChanged(long time)
{
    QString formattedTime;
    if (time < 1000 * 60 * 60) {
        formattedTime = QTime::fromMSecsSinceStartOfDay(time).toString(
                    QStringLiteral("m:ss.zzz"));
    } else {
        formattedTime = QTime::fromMSecsSinceStartOfDay(time).toString(
                    QStringLiteral("H:mm:ss.zzz"));
    }
    m_ui->timeButton->setText(formattedTime);

    m_ui->rewindButton->setEnabled(time != 0);
}

void TimeLineToolbar::OnSelectionChange(Q3DStudio::SSelectedValue newSelectable)
{
    qt3dsdm::TInstanceHandleList selectedInstances = newSelectable.GetSelectedInstances();
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *theClientBridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    bool canDelete = false;
    for (size_t idx = 0, end = selectedInstances.size(); idx < end; ++idx) {
        if (theClientBridge->CanDelete(selectedInstances[idx])) {
            canDelete = true;
            break;
        }
    }
    m_ui->deleteObject->setEnabled(canDelete);

    updateDataInputStatus(false);
}

void TimeLineToolbar::onAddDataInputClicked()
{
    showDataInputChooser();
}

// Update datainput button state according to this timecontext
// control state. If triggered via datamodel change i.e. dispatch message,
// force update from actual property values
void TimeLineToolbar::updateDataInputStatus(bool isViaDispatch)
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

        qt3dsdm::Option<qt3dsdm::SValue> controlledPropertyVal
            = Q3DStudio::SCOPED_DOCUMENT_EDITOR(
                *doc,
                QObject::tr("Get Timeline control"))->GetInstancePropertyValue(timeCtxRoot,
                                                                               ctrldProp);
        auto dataInputName = qt3dsdm::get<QString>(controlledPropertyVal.getValue());

        // Toggle if we changed to a controlled time context, or if icon current state
        // differs from the control state of current time context
        if (dataInputName.size()) {
            m_ui->addDataInputButton->setToolTip(
                tr("Timeline controller: %1").arg(dataInputName.left(dataInputName.indexOf(" "))));
            m_ui->addDataInputButton->setIcon(QIcon(":/images/Objects-DataInput-Normal.png"));
            m_currController = dataInputName.left(dataInputName.indexOf(" "));
        } else {
            m_ui->addDataInputButton->setToolTip(QString());
            // TODO actually delete the entire property instead of setting it as empty string
            m_ui->addDataInputButton->setIcon(QIcon(":/images/Objects-DataInput-Disabled.png"));
            m_currController = QString(tr("[No control]"));
        }

        m_currTimeCtxRoot = timeCtxRoot;
    }
}

void TimeLineToolbar::showDataInputChooser()
{
    QStringList dataInputList;
    dataInputList.append(tr("[No control]"));
    for (int i = 0; i < g_StudioApp.m_dataInputDialogItems.size(); i++) {
        if (g_StudioApp.m_dataInputDialogItems[i]->type == EDataType::DataTypeRangedNumber)
            dataInputList.append(g_StudioApp.m_dataInputDialogItems[i]->name);
    }

    m_DataInputSelector->setData(dataInputList, m_currController);
    m_DataInputSelector->setWindowModality(Qt::WindowModality::ApplicationModal);

    m_DataInputSelector->showDialog();
    m_DataInputSelector->setGeometry(10, m_mainFrame->height()/2, 300, 200);
    return;
}

void TimeLineToolbar::onDataInputChange(const QString &dataInputName)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    Q3DStudio::CString fullTimeControlStr;

    if (dataInputName != tr("[No control]")) {
        m_ui->addDataInputButton->setToolTip(tr("Timeline controller: %1").arg(dataInputName));
        fullTimeControlStr = (Q3DStudio::CString::fromQString(dataInputName + " @timeline"));
        m_ui->addDataInputButton->setIcon(QIcon(":/images/Objects-DataInput-Normal.png"));
    } else {
        m_ui->addDataInputButton->setToolTip(QString());
        // TODO actually delete the entire property instead of setting it as empty string
        fullTimeControlStr = Q3DStudio::CString();
        m_ui->addDataInputButton->setIcon(QIcon(":/images/Objects-DataInput-Disabled.png"));
    }

    qt3dsdm::SValue fullTimeCtrlVal = std::make_shared<qt3dsdm::CDataStr>(fullTimeControlStr);

    // To indicate that this presentation timeline is controlled by data input,
    // we set "controlled property" of this time context root (scene or component)
    // to contain the name of controller followed by special indicator "@timeline".
    qt3dsdm::Qt3DSDMPropertyHandle ctrldPropertyHandle;
    qt3dsdm::Qt3DSDMInstanceHandle timeCtxRoot = doc->GetActiveRootInstance();
    // Time context root is either scene or component
    if (bridge->GetObjectType(timeCtxRoot) == EStudioObjectType::OBJTYPE_SCENE)
        ctrldPropertyHandle = bridge->GetObjectDefinitions().m_Scene.m_ControlledProperty;
    else if (bridge->GetObjectType(timeCtxRoot) == EStudioObjectType::OBJTYPE_COMPONENT)
        ctrldPropertyHandle = bridge->GetObjectDefinitions().m_Component.m_ControlledProperty;
    else
        Q_ASSERT(false);

    m_currTimeCtxRoot = timeCtxRoot;
    m_currController = dataInputName;

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Set Timeline control"))
        ->SetInstancePropertyValue(timeCtxRoot, ctrldPropertyHandle, fullTimeCtrlVal);
    return;
}

void TimeLineToolbar::onAddLayerClicked()
{
    using namespace Q3DStudio;

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();

    // If active instance is component, just bail as we can't add layers to components
    qt3dsdm::Qt3DSDMInstanceHandle rootInstance = doc->GetActiveRootInstance();
    if (bridge->GetObjectType(rootInstance) == OBJTYPE_COMPONENT)
        return;

    qt3dsdm::Qt3DSDMSlideHandle slide = doc->GetActiveSlide();
    qt3dsdm::Qt3DSDMInstanceHandle layer = doc->GetActiveLayer();

    SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Add Layer"))
        ->CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Layer, layer, slide,
                                   DocumentEditorInsertType::PreviousSibling,
                                   CPt(), PRIMITIVETYPE_UNKNOWN, -1);
}

void TimeLineToolbar::onPlayButtonClicked()
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    if (doc->IsPlaying())
        m_mainFrame->OnPlaybackStop();
    else
        m_mainFrame->OnPlaybackPlay();
}

void TimeLineToolbar::OnBeginDataModelNotifications()
{
}

void TimeLineToolbar::OnEndDataModelNotifications()
{
    updateDataInputStatus(true);
}

void TimeLineToolbar::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    updateDataInputStatus(true);
}

void TimeLineToolbar::OnImmediateRefreshInstanceMultiple(
    qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    updateDataInputStatus(true);
}
