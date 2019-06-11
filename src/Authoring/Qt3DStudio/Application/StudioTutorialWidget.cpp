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
#include "StudioTutorialWidget.h"
#include "ui_StudioTutorialWidget.h"
#include "StudioUtils.h"
#include "StudioApp.h"
#include "StudioPreferences.h"
#include <QtWidgets/qdesktopwidget.h>
#include <QtGui/qpainter.h>
#include <QtGui/qbrush.h>
#include <QtCore/qtimer.h>
#include <QtGui/qdesktopservices.h>
#include <QtCore/qurl.h>

StudioTutorialWidget::StudioTutorialWidget(QWidget *parent) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    m_ui(new Ui::StudioTutorialWidget)
{
    m_ui->setupUi(this);

    connect(m_ui->studioTutorialShowAgain, &QCheckBox::stateChanged, this,
            &StudioTutorialWidget::handleDoNotShowAgainChange);
    connect(m_ui->studioTutorialNew, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleCreateNew);
    connect(m_ui->studioTutorialOpen, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleOpenSample);
    connect(m_ui->studioTutorialQuickStart, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleQuickStartGuide);

    QTimer::singleShot(0, this, &StudioTutorialWidget::OnInitDialog);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

StudioTutorialWidget::~StudioTutorialWidget()
{
    delete m_ui;
    delete m_backgroundPalette;
}

void StudioTutorialWidget::OnInitDialog()
{
    QSize backgroundSize = size();
    QRect screenRect = QApplication::desktop()->availableGeometry(
                QApplication::desktop()->screenNumber(this));
    QSize windowSize = screenRect.size();

    move(screenRect.x() + (windowSize.width() - backgroundSize.width()) / 2,
         screenRect.y() + (windowSize.height() - backgroundSize.height()) / 2);

    setFixedSize(backgroundSize);

    QSettings settings;
    m_ui->studioTutorialShowAgain->setChecked(!settings.value("showWelcomeScreen").toBool());
    m_backgroundPalette = new QPalette(palette());
    QPixmap background(size());
    QPainter backgroundPainter(&background);
    QLinearGradient gradient = CStudioPreferences::welcomeBackgroundGradient();
    gradient.setFinalStop(background.width(), 0.0);
    backgroundPainter.fillRect(background.rect(), gradient);
    QPixmap laptop(":/images/welcomedialog/laptop.png");
    backgroundPainter.drawPixmap(0, 100, laptop.width(), laptop.height(), laptop);
    backgroundPainter.end();
    m_backgroundPalette->setBrush(backgroundRole(), QBrush(background));
    setAutoFillBackground(true);
    setPalette(*m_backgroundPalette);
}


void StudioTutorialWidget::handleDoNotShowAgainChange(int state)
{
    QSettings settings;
    const bool show = !(state == Qt::Checked);
    settings.setValue("showWelcomeScreen", show);
}

void StudioTutorialWidget::handleOpenSample()
{
    this->done(StudioTutorialWidget::openSampleResult);
}

void StudioTutorialWidget::handleCreateNew()
{
    this->done(StudioTutorialWidget::createNewResult);
}

void StudioTutorialWidget::handleQuickStartGuide()
{
    QFile theFile(g_StudioApp.m_gettingStartedFilePath);
    if (theFile.exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(theFile.fileName()));
}
