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
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"

#include <QtWidgets/qslider.h>

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
    m_iconStop = QIcon(":/images/playback_tools_low-01.png");
    m_iconPlay = QIcon(":/images/playback_tools_low-02.png");

    // create actions
    QAction *actionNewLayer = new QAction(iconLayer, tr("Add New Layer"));
    QAction *actionFirst = new QAction(iconFirst, tr("Go to Timeline Start"));
    QAction *actionLast = new QAction(iconLast, tr("Go to Timeline End"));
    m_actionDeleteRow = new QAction(iconDelete, tr("Delete Selected Object"));
    m_actionPlayStop = new QAction();
    m_timeLabel = new TimelineToolbarLabel();
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

    // add actions
    addAction(actionNewLayer);
    addAction(m_actionDeleteRow);
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
