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
#include <QtWidgets/qdesktopwidget.h>
#include <QtGui/qpainter.h>

StudioTutorialWidget::StudioTutorialWidget(QWidget *parent, bool goToFileDialog,
                                           bool showProjectButtons) :
    QDialog(parent, Qt::MSWindowsFixedSizeDialogHint),
    m_ui(new Ui::StudioTutorialWidget),
    m_welcomeImages(0),
    m_imgIter(0),
    m_palette(0),
    m_displayScale(1.0),
    m_showProjectButtons(showProjectButtons)
{
    m_ui->setupUi(this);

    connect(m_ui->studioTutorialBack, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleBack);
    connect(m_ui->studioTutorialForward, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleFwd);
    connect(m_ui->pageIndicator, &StudioTutorialPageIndicator::indexChanged, this,
            &StudioTutorialWidget::handleIndexChange);
    connect(m_ui->studioTutorialShowAgain, &QCheckBox::stateChanged, this,
            &StudioTutorialWidget::handleDoNotShowAgainChange);
    connect(m_ui->studioTutorialNew, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleCreateNew);
    connect(m_ui->studioTutorialOpen, &QPushButton::clicked, this,
            &StudioTutorialWidget::handleOpenSample);

    OnInitDialog(goToFileDialog);

    if (m_showProjectButtons)
        m_ui->studioTutorialNew->setText(tr("Create New"));
    else
        m_ui->studioTutorialNew->setText(tr("OK"));
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
    m_imgIterPrev = m_imgIter;
    m_pageOutPixmap = getScaledPic(m_imgIterPrev);
    m_pageInPixmap = getScaledPic(m_imgIter);
    m_backgroundPixmap = QPixmap(":/images/Tutorial/background.png");

    // based on background PNG, get the scale that we need to fit welcome
    // screen and buttons comfortably on display
    m_displayScale = getDisplayScalingForImage(m_backgroundPixmap);
    QSize backgroundSize = m_backgroundPixmap.size();
    QRect screenRect = QApplication::desktop()->availableGeometry(
                QApplication::desktop()->screenNumber(this));
    QSize windowSize = screenRect.size();

    m_ui->verticalWidget->setFixedSize(backgroundSize);

    move(screenRect.x() + (windowSize.width() - backgroundSize.width()) / 2,
         screenRect.y() + (windowSize.height() - backgroundSize.height()) / 2);

    // do we go straight to last page with file dialog buttons?
    int initPage = goToFileDialog ? m_welcomeImages->size() - 1 : 0;
    m_imgIter = m_welcomeImages->begin() + initPage;
    updateButtons();

    QSettings settings;
    m_ui->studioTutorialShowAgain->setChecked(!settings.value("showWelcomeScreen").toBool());

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void StudioTutorialWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    if (!m_palette) {
        m_palette = new QPalette;
        m_palette->setBrush(QPalette::Window, m_backgroundPixmap);
        setPalette(*m_palette);
        resize(m_backgroundPixmap.size());
        setFixedSize(size());
    }

    // Make tutorial images to scale to full background width, vertically centered,
    // while keeping correct aspect ratio
    qreal aspectRatio = (qreal) m_pageInPixmap.height() / m_pageInPixmap.width();
    int rectHeight = size().width() * aspectRatio;
    QRect rect(0, (size().height() / 2) - (rectHeight / 2),
               size().width(), rectHeight);

    qreal pageOutOpacity = 1.0 - m_pageInOpacity;

    if (pageOutOpacity > 0.0) {
        painter.setOpacity(pageOutOpacity);
        painter.drawPixmap(rect, m_pageOutPixmap);
    }
    if (m_pageInOpacity > 0.0) {
        painter.setOpacity(m_pageInOpacity);
        painter.drawPixmap(rect, m_pageInPixmap);
    }

    if (m_pageInOpacity < 1.0) {
        // Page switching animation still going on
        qreal opacityAnimationStep = qreal(m_opacityTime.restart()) / 300.0;
        m_pageInOpacity += opacityAnimationStep;
        m_pageInOpacity = qMin(m_pageInOpacity, 1.0);
        update();
    }
}

void StudioTutorialWidget::animateInOut()
{
    m_pageOutPixmap = getScaledPic(m_imgIterPrev);
    m_pageInPixmap = getScaledPic(m_imgIter);
    m_pageInOpacity = 0.0;
    m_opacityTime.start();
    update();
}

void StudioTutorialWidget::handleFwd()
{
    if (*m_imgIter != m_welcomeImages->last()) {
        m_imgIterPrev = m_imgIter;
        m_imgIter++;
        updateButtons();
        animateInOut();
    }
}

void StudioTutorialWidget::handleBack()
{
    if (*m_imgIter != m_welcomeImages->first()) {
        m_imgIterPrev = m_imgIter;
        m_imgIter--;
        updateButtons();
        animateInOut();
    }
}

void StudioTutorialWidget::handleIndexChange(int index)
{
    index = qBound(0, index, m_welcomeImages->size() - 1);
    if (index != page()) {
        m_imgIterPrev = m_imgIter;
        m_imgIter = m_welcomeImages->begin() + index;
        updateButtons();
        animateInOut();
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
    QString imagePath = QStringLiteral(":/images/Tutorial/screens/1x");

    // Use @2x images for hiDPI displays
    if (devicePixelRatio() > 1.0)
        imagePath = QStringLiteral(":/images/Tutorial/screens/2x");

    QDirIterator *it = new QDirIterator(imagePath, QDirIterator::NoIteratorFlags);

    while (it->hasNext())
        m_welcomeImages->append(it->next());

    m_ui->pageIndicator->setCount(m_welcomeImages->size());
}

int StudioTutorialWidget::page() const
{
    int i = 0;
    QList<QString>::iterator iter = m_welcomeImages->begin();
    while (iter != m_imgIter) { ++i; iter++; }
    return i;
}

QPixmap StudioTutorialWidget::getScaledPic(const QList<QString>::iterator &iter)
{
    QPixmap picOrig = QPixmap(*iter);
    QPixmap pic = picOrig;
    if (m_displayScale < 1.0) {
        // Limit to the maximum size of @2x images
        pic = picOrig.scaledToHeight(qMin(1200.0, m_displayScale * picOrig.height()),
                                     Qt::SmoothTransformation);
    }

    pic.setDevicePixelRatio(devicePixelRatio());
    return pic;
}

qreal StudioTutorialWidget::getDisplayScalingForImage(const QPixmap &picOrig)
{
    // Note that high DPI scaling has an effect on the display
    // resolution returned by QApplication::desktop()->availableGeometry().
    // DPI scaling factor is integer and taken from the primary screen.
    // When running studio on secondary monitor with different DPI,
    // or running it on primary with non-integer scaling, we might
    // get different dialog size than intended.
    QSize displaySize = QApplication::desktop()->availableGeometry(
                QApplication::desktop()->screenNumber(this)).size();

    // Scale down if images do not fit on screen, otherwise use
    // 1:1 PNGs to avoid scaling artifacts. Scale to 90% of the display size if scaling is needed.
    if (picOrig.height() > displaySize.height() || picOrig.width() > displaySize.width()) {
        QSize picScaledSize = picOrig.size();
        picScaledSize.scale(displaySize * 0.9, Qt::KeepAspectRatio);
        m_displayScale = qMin((qreal)picScaledSize.height() / (qreal)picOrig.height(),
                              (qreal)picScaledSize.width() / (qreal)picOrig.width());
        return m_displayScale;
    } else {
        return 1.0;
    }
}

void StudioTutorialWidget::updateButtons()
{
    bool isFirst = (*m_imgIter == m_welcomeImages->first());
    bool isLast = (*m_imgIter == m_welcomeImages->last());

    m_ui->studioTutorialBack->setVisible(!isFirst);
    m_ui->studioTutorialForward->setVisible(!isLast);
    m_ui->studioTutorialOpen->setVisible(isLast && m_showProjectButtons);
    m_ui->studioTutorialNew->setVisible(isLast);
    m_ui->studioTutorialShowAgain->setVisible(isFirst);
    m_ui->checkBoxLabel->setVisible(isFirst);

    m_ui->pageIndicator->setCurrentIndex(page());
}
