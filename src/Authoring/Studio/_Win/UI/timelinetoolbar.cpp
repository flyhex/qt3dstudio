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

#include <QtCore/qdatetime.h>

QTimeLineToolbar::QTimeLineToolbar(CMainFrame *mainFrame, QWidget *pParent)
    : QWidget(pParent)
    , m_ui(new QT_PREPEND_NAMESPACE(Ui::TimeLineToolbar))
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
        timeEditDlg.ShowDialog(doc->GetCurrentViewTime(), 0, doc, PLAYHEAD);
    });

    connect(m_ui->deleteObject, &QPushButton::clicked,
            doc, &CDoc::DeleteSelectedObject);

    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    m_Connections.push_back(theDispatch->ConnectSelectionChange(
        std::bind(&QTimeLineToolbar::OnSelectionChange, this, std::placeholders::_1)));

    // TODO: Add datainput button handling
    m_ui->addDataInputButton->setVisible(false);

    // TODO: add layer button handling
    m_ui->addLayerButton->setVisible(false);
}

QTimeLineToolbar::~QTimeLineToolbar()
{
    delete m_ui;
    m_Connections.clear();
}

void QTimeLineToolbar::onTimeChanged(long time)
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

void QTimeLineToolbar::OnSelectionChange(Q3DStudio::SSelectedValue newSelectable)
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
