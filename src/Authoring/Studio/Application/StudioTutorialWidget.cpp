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

StudioTutorialWidget::StudioTutorialWidget(bool goToFileDialog) :
    QDialog(nullptr, Qt::MSWindowsFixedSizeDialogHint | Qt::FramelessWindowHint),
    m_ui(new Ui::StudioTutorialWidget),
    m_welcomeImages(0),
    m_imgIter(0),
    m_palette(0),
    m_displayScale(1.0)
{
    m_ui->setupUi(this);

    connect(m_ui->studioTutorialBack, &QPushButton::released, this,
            &StudioTutorialWidget::handleBack);
    connect(m_ui->studioTutorialForward, &QPushButton::released, this,
            &StudioTutorialWidget::handleFwd);
    connect(m_ui->studioTutorialShowAgain, &QCheckBox::stateChanged, this,
            &StudioTutorialWidget::handleDoNotShowAgainChange);
    connect(m_ui->studioTutorialNew, &QPushButton::released, this,
            &StudioTutorialWidget::handleCreateNew);
    connect(m_ui->studioTutorialOpen, &QPushButton::released, this,
            &StudioTutorialWidget::handleOpenSample);

    OnInitDialog(goToFileDialog);
}

StudioTutorialWidget::~StudioTutorialWidget()
{
    delete m_ui;
    delete m_welcomeImages;
    delete m_palette;
}

void StudioTutorialWidget::OnInitDialog(bool goToFileDialog)
{
    m_welcomeImages = new QList<QString>();

    // populate welcome screen images
    getImageList();
    m_imgIter = m_welcomeImages->begin();

    // do we go straight to last page with file dialog buttons?
    int page = goToFileDialog ? m_welcomeImages->size() - 1 : 0;
    // based on first PNG, get the scale that we need to fit welcome
    // screen and buttons comfortably on display
    m_displayScale = getDisplayScalingForImage(m_imgIter);
    m_ui->verticalWidget->setMaximumSize(m_displayScale * size());

    if (!m_welcomeImages->isEmpty()) {
        for (int i = 0; i < page && m_imgIter != m_welcomeImages->end(); ++i)
            m_imgIter++;

        m_ui->studioTutorialShowAgain->setVisible(false);
        if (*m_imgIter == m_welcomeImages->last() || m_imgIter == m_welcomeImages->end()) {
            if (m_imgIter == m_welcomeImages->end())
                m_imgIter--;
            m_ui->studioTutorialForward->setVisible(false);
            m_ui->studioTutorialOpen->setVisible(true);
            m_ui->studioTutorialNew->setVisible(true);
        } else {
            if (m_imgIter == m_welcomeImages->begin()) {
                m_ui->studioTutorialBack->setVisible(false);
                m_ui->studioTutorialShowAgain->setVisible(true);
            }
            m_ui->studioTutorialOpen->setVisible(false);
            m_ui->studioTutorialNew->setVisible(false);
        }
    }

    QSettings settings;
    m_ui->studioTutorialShowAgain->setChecked(!settings.value("showWelcomeScreen").toBool());

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void StudioTutorialWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_palette)
        return;

    m_palette = new QPalette;
    QPixmap pic = getScaledPic(m_imgIter);
    m_palette->setBrush(QPalette::Window, pic);
    setPalette(*m_palette);

    // assume all welcome screen images are sized the same
    resize(pic.size());
    setFixedSize(size());

    // If the dialog was originally larger than the screen, it will be placed into the
    // top-left corner. Adjust its position after resizing.
    if (m_displayScale < 1.0) {
        QSize windowSize = GetAvailableDisplaySize(getWidgetScreen(this));
        QSize welcomeSize = size();
        move((windowSize.width() - welcomeSize.width()) / 2,
             (windowSize.height() - welcomeSize.height()) / 2);
    }
}

void StudioTutorialWidget::handleFwd()
{
    if (*m_imgIter != m_welcomeImages->last()) {
        QPixmap pic = getNextScaledPic();
        m_palette->setBrush(QPalette::Window, pic);
        setPalette(*m_palette);

        m_ui->studioTutorialBack->setVisible(true);
        m_ui->studioTutorialShowAgain->setVisible(false);
    }

    if (*m_imgIter == m_welcomeImages->last()) {
        m_ui->studioTutorialForward->setVisible(false);
        m_ui->studioTutorialOpen->setVisible(true);
        m_ui->studioTutorialNew->setVisible(true);
    }
}

void StudioTutorialWidget::handleBack()
{
    if (*m_imgIter != m_welcomeImages->first()) {
        QPixmap pic = getPrevScaledPic();
        m_palette->setBrush(QPalette::Window, pic);
        setPalette(*m_palette);

        m_ui->studioTutorialForward->setVisible(true);
        m_ui->studioTutorialShowAgain->setVisible(false);

        m_ui->studioTutorialOpen->setVisible(false);
        m_ui->studioTutorialNew->setVisible(false);
    }

    if (*m_imgIter == m_welcomeImages->first()) {
        m_ui->studioTutorialBack->setVisible(false);
        m_ui->studioTutorialShowAgain->setVisible(true);
    }
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

void StudioTutorialWidget::getImageList()
{
    QDirIterator *it = new QDirIterator(":/images/Tutorial/screens/",
                                        QDirIterator::NoIteratorFlags);

    while (it->hasNext())
        m_welcomeImages->append(it->next());
}

int StudioTutorialWidget::page() const
{
    int i = 0;
    QList<QString>::iterator iter = m_welcomeImages->begin();
    while (iter != m_imgIter) { ++i; iter++; }
    return i;
}

QPixmap StudioTutorialWidget::getNextScaledPic()
{
    return getScaledPic(++m_imgIter);
}

QPixmap StudioTutorialWidget::getPrevScaledPic()
{
    return getScaledPic(--m_imgIter);
}

QPixmap StudioTutorialWidget::getScaledPic(QList<QString>::iterator iter)
{
    QPixmap picOrig = QPixmap(*iter);
    QPixmap pic = picOrig.scaledToHeight(m_displayScale * picOrig.height(),
                                         Qt::SmoothTransformation);

    pic.setDevicePixelRatio(devicePixelRatio());
    return pic;
}

qreal StudioTutorialWidget::getDisplayScalingForImage(QList<QString>::iterator iter)
{
    QPixmap picOrig = QPixmap(*iter);

    // Set the splash screen to 80% of display size.
    // Note that high DPI scaling has an effect on the display
    // resolution returned by GetAvailableDisplaySize().
    // DPI scaling factor is integer and taken from the primary screen.
    // When running studio on secondary monitor with different DPI,
    // or running it on primary with non-integer scaling, we might
    // get different dialog size than intended.
    QSize displaySize = GetAvailableDisplaySize(getWidgetScreen(this)) * 0.8;

    // scale down if images do not fit on screen, otherwise use
    // 1:1 PNGs to avoid scaling artifacts
    if (picOrig.height() > displaySize.height() ||
            picOrig.width() > displaySize.width()) {
        QSize picScaledSize = picOrig.size();
        picScaledSize.scale(displaySize, Qt::KeepAspectRatio);
        m_displayScale = qMin((qreal)picScaledSize.height() / (qreal)picOrig.height(),
                              (qreal)picScaledSize.width() / (qreal)picOrig.width());
        return m_displayScale;
    } else {
        return 1.0;
    }
}
