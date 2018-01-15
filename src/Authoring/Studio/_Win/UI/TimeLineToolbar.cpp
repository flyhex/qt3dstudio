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
#include "Doc.h"
#include "Core.h"
#include "Dispatch.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "StudioObjectTypes.h"
#include "IDocumentEditor.h"
#include "DocumentEditorEnumerations.h"

#include <QtCore/qdatetime.h>

TimeLineToolbar::TimeLineToolbar(CMainFrame *mainFrame, const QSize &preferredSize,
                                 QWidget *pParent)
    : QWidget(pParent)
    , m_ui(new QT_PREPEND_NAMESPACE(Ui::TimeLineToolbar))
    , m_preferredSize(preferredSize)
{
    m_ui->setupUi(this);

    connect(m_ui->playButton, &QToolButton::clicked,
            mainFrame, &CMainFrame::OnPlaybackPlay);
    connect(m_ui->rewindButton, &QToolButton::clicked,
            mainFrame, &CMainFrame::OnPlaybackRewind);
    connect(m_ui->stopButton, &QToolButton::clicked,
            mainFrame, &CMainFrame::OnPlaybackStop);
    connect(mainFrame, &CMainFrame::playStateChanged,
            [this](bool state){
        m_ui->playButton->setEnabled(!state);
        m_ui->playButton->setChecked(state);
        m_ui->stopButton->setEnabled(state);
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

    // TODO: Add datainput button handling
    m_ui->addDataInputButton->setVisible(false);
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
