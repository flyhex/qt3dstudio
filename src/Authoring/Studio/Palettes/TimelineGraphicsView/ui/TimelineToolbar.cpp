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

#include <QtWidgets/qslider.h>

TimelineToolbar::TimelineToolbar() : QToolBar()
{
    setContentsMargins(0, 0, 0, 0);
    setIconSize(QSize(15, 15));

    // create icons
    static const QIcon iconLayer = QIcon(":/images/Asset-Layer-Normal.png");
    static const QIcon iconDelete = QIcon(":/images/Action-Trash-Normal.png");
    static const QIcon iconFirst = QIcon(":/images/playback_tools_low-00.png");
    static const QIcon iconStop = QIcon(":/images/playback_tools_low-01.png");
    static const QIcon iconPlay = QIcon(":/images/playback_tools_low-02.png");
    static const QIcon iconLast = QIcon(":/images/playback_tools_low-04.png");

    // create actions
    QAction *actionNewLayer    = new QAction(iconLayer, tr("Add New Layer"));
    QAction *actionDeleteLayer = new QAction(iconDelete, tr("Delete Layer"));
             m_actionTime      = new QAction("0:00.000");
    QAction *actionFirst       = new QAction(iconFirst, tr("Go to Timeline Start"));
    QAction *actionStop        = new QAction(iconStop, tr("Stop Playing"));
    QAction *actionPlay        = new QAction(iconPlay, tr("Start Playing"));
    QAction *actionLast        = new QAction(iconLast, tr("Go to Timeline End"));

    m_scaleSlider = new QSlider();
    m_scaleSlider->setOrientation(Qt::Horizontal);
    m_scaleSlider->setMaximumWidth(200);
    m_scaleSlider->setMinimumWidth(100);
    m_scaleSlider->setMinimum(1);
    m_scaleSlider->setMaximum(8);
    m_scaleSlider->setPageStep(.1);
    m_scaleSlider->setValue(2);
    m_scaleSlider->setStyleSheet("background-color:0,0,0,0;");

    m_actionDuration = new QAction("0:20");

    // connections
    connect(actionNewLayer   , &QAction::triggered, this, &TimelineToolbar::newLayerTriggered);
    connect(actionDeleteLayer, &QAction::triggered, this, &TimelineToolbar::deleteLayerTriggered);
    connect(m_actionTime     , &QAction::triggered, this, &TimelineToolbar::gotoTimeTriggered);
    connect(actionFirst      , &QAction::triggered, this, &TimelineToolbar::firstFrameTriggered);
    connect(actionStop       , &QAction::triggered, this, &TimelineToolbar::stopTriggered);
    connect(actionPlay       , &QAction::triggered, this, &TimelineToolbar::playTriggered);
    connect(actionLast       , &QAction::triggered, this, &TimelineToolbar::lastFrameTriggered);
    connect(m_scaleSlider    , &QSlider::valueChanged, this, &TimelineToolbar::timelineScaleChanged);
    connect(m_actionDuration , &QAction::triggered, this, &TimelineToolbar::setDurationTriggered);

    // add actions
    addAction(actionNewLayer);
    addAction(actionDeleteLayer);
    addSpacing(100);
    addAction(m_actionTime);
    addSpacing(10);
    addAction(actionFirst);
    addAction(actionStop);
    addAction(actionPlay);
    addAction(actionLast);
    addSpacing(20);
    addWidget(m_scaleSlider);
    addSeparator();
    addAction(m_actionDuration);
}

// add a spacer widget
void TimelineToolbar::addSpacing(int width)
{
    auto *widget = new QWidget;
    widget->setStyleSheet("background-color:0,0,0,0;"); // make the widget transparent
    widget->setFixedWidth(width);
    addWidget(widget);
}

void TimelineToolbar::setTime(double secsAndmillis)
{
    long totalMillis = secsAndmillis * 1000;
    long mins = totalMillis % 3600000 / 60000;
    long secs = totalMillis % 60000 / 1000;
    long millis = totalMillis % 1000;

    m_actionTime->setText(QString::asprintf("%01d:%02d.%03d", mins, secs, millis));
}
