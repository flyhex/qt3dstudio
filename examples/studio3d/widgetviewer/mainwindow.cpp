/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QKeyEvent>
#include <QtGui/QWindow>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtStudio3D/Q3DSPresentation>
#include <QtStudio3D/Q3DSViewerSettings>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->actionOpen->setShortcut(QKeySequence::Open);
    QList<QKeySequence> shortcuts;
    shortcuts.push_back(QKeySequence(QKeySequence::Quit));
    shortcuts.push_back(QKeySequence("CTRL+Q"));
    ui->actionQuit->setShortcuts(shortcuts);
    ui->actionFull_Screen->setShortcut(QKeySequence::FullScreen);
    ui->actionReload->setShortcut(QKeySequence::Refresh);

    QStringList strArg = QApplication::arguments();
    if (strArg.size() >= 2) {
        QFileInfo theFilePath(strArg[1]);
        if (theFilePath.exists()) {
            m_openFileDir = theFilePath.path();
            ui->widget->presentation()->setSource(strArg[1]);
            QSettings().setValue("PreviousOpenFolder", m_openFileDir);
        }
    }

    m_embeddedMode = false;
    m_startInFullscreen = false;
    QStringList::const_iterator constIterator;
    for (constIterator = strArg.constBegin(); constIterator != strArg.constEnd(); ++constIterator) {
        if (!(*constIterator).compare("-embedded")) {
            m_embeddedMode = true;
        } else if (!(*constIterator).compare("-fullscreen")) {
            // When in fullscreen also enable embedded mode
            m_embeddedMode = true;
            m_startInFullscreen = true;
        }
    }

    // Hide menu when we are in embedded mode
    if (m_embeddedMode)
        ui->menuBar->hide();

    QSettings settings;
    ui->actionBorder->setChecked(settings.value("ShowMatte", false).toBool());
    ui->actionFull_Screen->setChecked(settings.value("FullScreen", false).toBool());
    ui->actionShowOnScreenStats->setChecked(settings.value("ShowOnScreenStats", false).toBool());

    // Set runtime states
    ui->widget->settings()->setMatteColor(settings.value("ShowMatte", false).toBool()
                                          ? QColor(50, 50, 50) : Qt::black);
    ui->widget->settings()->setShowRenderStats(settings.value("ShowOnScreenStats", false).toBool());
    if (!m_startInFullscreen)
        m_startInFullscreen = settings.value("FullScreen", false).toBool();

    addAction(ui->actionFull_Screen);
    addAction(ui->actionShowOnScreenStats);
    addAction(ui->actionBorder);
    addAction(ui->actionShadeMode);

    // Disable hotkeys when we are in embedded mode
    if (!m_embeddedMode) {
        addAction(ui->actionReload);
        addAction(ui->actionQuit);
        addAction(ui->actionOpen);
    }

    // Allow drag'n'dropping
    setAcceptDrops(true);

    ui->widget->setUpdateInterval(0);

    connect(ui->widget, &Q3DSWidget::runningChanged, this, &MainWindow::updateUI);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("FullScreen", this->isFullScreen());
    settings.setValue("ShowMatte", ui->widget->settings()->matteColor() != Qt::black);
    settings.setValue("ShowOnScreenStats", ui->widget->settings()->isShowRenderStats());
    // Don't save the window geometry if we are in fullscreen mode.
    // This is invalid geometry and actually breaks our ability to start on a second monitor
    if (!this->isFullScreen()) {
        settings.setValue("WindowGeometry", saveGeometry());
        settings.setValue("WindowState", saveState());
    }
    delete ui;
}

void MainWindow::toggleFullscreenMode(bool inFullscreen)
{
    if (inFullscreen) {
        showFullScreen();
        ui->menuBar->hide();
    } else {
        showNormal();
        if (!m_embeddedMode)
            ui->menuBar->show();
    }
    updateScaleUI();
}

void MainWindow::on_actionFull_Screen_triggered()
{
    bool isFullScreen = this->isFullScreen();
    if (!isFullScreen) {
        QSettings settings;
        settings.setValue("WindowGeometry", saveGeometry());
        settings.setValue("WindowState", saveState());
    }
    toggleFullscreenMode(!isFullScreen);
}

void MainWindow::on_actionBorder_triggered()
{
    if (ui->widget->settings()->matteColor() == Qt::black)
        ui->widget->settings()->setMatteColor(QColor(50, 50, 50));
    else
        ui->widget->settings()->setMatteColor(Qt::black);
}

void MainWindow::on_actionShowOnScreenStats_triggered()
{
    bool showStats = !ui->widget->settings()->isShowRenderStats();
    ui->widget->settings()->setShowRenderStats(showStats);
}

void MainWindow::on_actionShadeMode_triggered()
{
    Q3DSViewerSettings::ShadeMode shadeMode = ui->widget->settings()->shadeMode();
    if (shadeMode == Q3DSViewerSettings::ShadeModeShaded)
        ui->widget->settings()->setShadeMode(Q3DSViewerSettings::ShadeModeShadedWireframe);
    else
        ui->widget->settings()->setShadeMode(Q3DSViewerSettings::ShadeModeShaded);
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionOpen_triggered()
{
    QSettings settings;
    if (m_openFileDir.size() == 0)
        m_openFileDir = settings.value("PreviousOpenFolder", QString("")).toString();

    QString filename = QFileDialog::getOpenFileName(
                this, tr("Open File or Project"), m_openFileDir,
                tr("All supported formats (*.uip *.uia);;Studio UI Presentation "
                   "(*.uip);;Application file (*.uia)"),
                nullptr, QFileDialog::DontUseNativeDialog);

    if (filename.size() == 0)
        return;

    QFileInfo theInfo(filename);
    m_openFileDir = theInfo.path();
    settings.setValue("PreviousOpenFolder", m_openFileDir);

    loadFile(filename);
}

void MainWindow::on_actionReload_triggered()
{
    ui->widget->reset();
    updateScaleUI();
}

void MainWindow::loadFile(const QString &filename)
{
    ui->widget->presentation()->setSource(QUrl::fromLocalFile(filename));
    ui->widget->initialize();
    updateScaleUI();
}

void MainWindow::on_actionCenter_triggered()
{
    ui->widget->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeCenter);
    updateScaleUI();
}

void MainWindow::on_actionScale_To_Fit_triggered()
{
    ui->widget->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFit);
    updateScaleUI();
}

void MainWindow::on_actionScale_To_Fill_triggered()
{
    ui->widget->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);
    updateScaleUI();
}

QString MainWindow::convertMimeDataToFilename(const QMimeData *mimeData)
{
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            QString str = url.toLocalFile();
            if (!str.isEmpty()) {
                if (QFileInfo(str).suffix() == QStringLiteral("uip")
                        || QFileInfo(str).suffix() == QStringLiteral("uia")) {
                    // Allow uip and uia
                    return str;
                }
            }
        }
    }
    return QString();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (convertMimeDataToFilename(event->mimeData()).isEmpty() == false)
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QString fileName = convertMimeDataToFilename(event->mimeData());
    if (fileName.isEmpty() == false)
        loadFile(fileName);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    restoreGeometry(QSettings().value("WindowGeometry").toByteArray());
    restoreState(QSettings().value("WindowState").toByteArray());
    if (m_startInFullscreen || this->isFullScreen()) {
        m_startInFullscreen = false; // only once
        toggleFullscreenMode(true);
    }
    if (ui->widget->isRunning() == false && !ui->widget->presentation()->source().isEmpty()) {
        if (!ui->widget->initialize()) {
            QString msg = QStringLiteral("Unable to initialize OpenGL.\nThis may be because your "
                    "graphic device is not sufficient, or simply because your driver is too old.\n"
                    "\nPlease try upgrading your graphics driver and try again.");
            QMessageBox::critical(NULL, "Fatal Error", msg, QMessageBox::Close);
            QCoreApplication::exit(1);
        }
    }
}

void MainWindow::updateUI()
{
    QString name = ui->widget->presentation()->source().toString();
    setWindowTitle(tr("%1Qt 3D Studio Viewer example").arg(name.size()
                                                           ? name.append(" - ") : name));
    updateScaleUI();
    QWindow *w = windowHandle();
    if (w) {
        if (!QQmlEngine::contextForObject(w)) {
            QQmlEngine *engine = new QQmlEngine(this);
            QQmlContext *context = new QQmlContext(engine, this);
            QQmlEngine::setContextForObject(w, context);
        }
    }
}

void MainWindow::updateScaleUI()
{
    ui->actionCenter->setChecked(false);
    ui->actionScale_To_Fit->setChecked(false);
    ui->actionScale_To_Fill->setChecked(false);
    Q3DSViewerSettings::ScaleMode mode = ui->widget->settings()->scaleMode();
    if (mode == Q3DSViewerSettings::ScaleModeCenter)
        ui->actionCenter->setChecked(true);
    else if (mode == Q3DSViewerSettings::ScaleModeFill)
        ui->actionScale_To_Fill->setChecked(true);
    else
        ui->actionScale_To_Fit->setChecked(true);
}
