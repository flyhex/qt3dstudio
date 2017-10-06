/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QKeyEvent>
#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QMessageBox>
#include <QShortcut>
#include <QWindow>
#include <QVariant>
#include <QGLFormat>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QtStudio3D/private/q3dsviewersettings_p.h>
#include <QtGui/qguiapplication.h>

#include "Qt3DSView.h"
#include "q3dspresentationitem.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_studio3D(0)
    , m_connectionInfo(0)
    , m_remoteProject(0)
{
    ui->setupUi(this);

    ui->actionOpen->setShortcut(QKeySequence::Open);
    QList<QKeySequence> shortcuts;
    shortcuts.push_back(QKeySequence(QKeySequence::Quit));
    shortcuts.push_back(QKeySequence("CTRL+Q"));
    ui->actionQuit->setShortcuts(shortcuts);
    ui->actionReload->setShortcut(QKeySequence::Refresh);

    QStringList strArg = QApplication::arguments();
    if (strArg.size() >= 2) {
        QFileInfo theFilePath(strArg[1]);
        if (theFilePath.exists()) {
            m_openFileDir = theFilePath.path();
            QSettings().setValue("DirectoryOfLastOpen", m_openFileDir);
        }
    }

#ifdef Q_OS_ANDROID
    m_openFileDir = QStringLiteral("/sdcard/qt3dviewer"); // Add default folder for Android
#else
    // Allow drops. Not usable for Android.
    setAcceptDrops(true);
#endif

    addAction(ui->actionFull_Screen);
    addAction(ui->actionShowOnScreenStats);
    addAction(ui->actionBorder);

    // Set import paths so that standalone installation works
    QString extraImportPath1(QStringLiteral("%1/qml"));
    QString extraImportPath2(QStringLiteral("%1/../qml"));
    ui->quickWidget->engine()->addImportPath(
                extraImportPath1.arg(QGuiApplication::applicationDirPath()));
    ui->quickWidget->engine()->addImportPath(
                extraImportPath2.arg(QGuiApplication::applicationDirPath()));

    ui->quickWidget->setSource(QUrl("qrc:/viewer.qml"));

    updateUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_actionBorder_triggered()
{
    Q3DSView *view = viewer();
    if (!view)
        return;

    QColor matte = view->viewerSettings()->matteColor();
    if (matte == QColor(Qt::black) || !matte.isValid())
        view->viewerSettings()->setMatteColor(QColor(50, 50, 50));
    else
        view->viewerSettings()->setMatteColor(Qt::black);

    updateUI();
}

void MainWindow::on_actionFull_Screen_triggered()
{
    if (ui->actionFull_Screen->isChecked()) {
        showFullScreen();
        ui->menuBar->hide();
    } else {
        showNormal();
        ui->menuBar->show();
    }
}

void MainWindow::on_actionShowOnScreenStats_triggered()
{
    Q3DSView *view = viewer();
    if (!view)
        return;

    view->viewerSettings()->setShowRenderStats(!view->viewerSettings()->isShowRenderStats());
    updateUI();
}

void MainWindow::on_actionQuit_triggered()
{
    delete m_studio3D;
    m_studio3D = 0;
    close();
}

void MainWindow::on_actionOpen_triggered()
{
    QSettings settings;
    if (m_openFileDir.size() == 0) {
        m_openFileDir
                = settings.value("DirectoryOfLastOpen", QString("")).toString();
    }

    QString filename = QFileDialog::getOpenFileName(
                this, tr("Open File or Project"), m_openFileDir,
                tr("All supported formats (*.uip *.uia *.uiab);;Studio UI Presentation "
                   "(*.uip);;Application file (*.uia);;Binary Application (*.uiab)"),
                NULL, QFileDialog::DontUseNativeDialog);

    if (filename.size() == 0)
        return;

    QFileInfo theInfo(filename);
    m_openFileDir = theInfo.path();
    settings.setValue("DirectoryOfLastOpen", m_openFileDir);

    loadFile(filename);
}

void MainWindow::on_actionConnect_triggered()
{
    if (m_remoteProject) {
        delete m_remoteProject;
        m_remoteProject = 0;
        delete m_connectionInfo;
        m_connectionInfo = 0;
        if (m_studio3D)
            m_studio3D->setVisible(true);

        updateUI();
        return;
    }

    m_remoteProject = new RemoteProject(this);
    if (!m_remoteProject->startServer()) {
        QString msg = "Unable to connect to remote project.\n";
        QMessageBox::warning(this, "Connection Error", msg, QMessageBox::Close);
        delete m_remoteProject;
        m_remoteProject = 0;
        updateUI();
        return;
    }

    int port = m_remoteProject->serverPort();
    QString message;
    QTextStream stream(&message);
    stream << "Connection Info\n"
           << "Address: " << m_remoteProject->hostAddress().toString() << "\n"
           << "Port: " + QString::number(port);

    QQmlEngine *engine = ui->quickWidget->engine();
    QQuickItem *root = ui->quickWidget->rootObject();

    QByteArray qml = "import QtQuick 2.7\n"
                     "import QtQuick.Controls 2.2\n"
                     "Label {\n"
                     "    color: \"White\"\n"
                     "    horizontalAlignment: Text.AlignHCenter\n"
                     "    verticalAlignment: Text.AlignVCenter\n"
                     "    anchors.fill: parent\n"
                     "    font.pixelSize: 42\n"
                     "}";

    QQmlComponent component(engine);
    component.setData(qml, QUrl());

    if (component.isError()) {
        qCritical() << "error" << component.errors();
        return;
    }

    m_connectionInfo = qobject_cast<QQuickItem *>(component.create());
    m_connectionInfo->setProperty("text", message);

    QQmlEngine::setObjectOwnership(m_connectionInfo, QQmlEngine::CppOwnership);
    m_connectionInfo->setParentItem(root);
    m_connectionInfo->setParent(engine);

    connect(m_remoteProject, &RemoteProject::remoteConnected,
        this, &MainWindow::remoteConnected);

    connect(m_remoteProject, &RemoteProject::remoteDisconnected,
        this, &MainWindow::remoteDisconnected);

    connect(m_remoteProject, &RemoteProject::projectChanged,
        this, &MainWindow::loadRemoteProject);

    updateUI();
}

void MainWindow::on_actionReload_triggered()
{
    if (Q3DSView *view = viewer()) {
        view->reset();
        updateUI();
    }
}

Q3DSView *MainWindow::viewer() const
{
    return dynamic_cast<Q3DSView *>(m_studio3D);
}

void MainWindow::loadFile(const QString &filename)
{
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists())
        return;

    QUrl sourceUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());

    if (m_studio3D) {
        viewer()->presentation()->setSource(sourceUrl);
        return;
    }

    QQmlEngine *engine = ui->quickWidget->engine();
    QQuickItem *root = ui->quickWidget->rootObject();

    QByteArray qml = "import QtStudio3D 1.0\n"
                     "Studio3D {\n"
                     "  id: studio3D\n"
                     "  anchors.fill: parent\n"
                     "  focus: true\n"
                     "}";

    QQmlComponent component(engine);
    component.setData(qml, QUrl());

    if (component.isError()) {
        qDebug() << "error" << component.errors();
        return;
    }

    m_studio3D = qobject_cast<QQuickItem *>(component.create());
    viewer()->presentation()->setSource(sourceUrl);

    QQmlEngine::setObjectOwnership(m_studio3D, QQmlEngine::CppOwnership);
    m_studio3D->setParentItem(root);
    m_studio3D->setParent(engine);

#ifdef Q_OS_ANDROID
    // We have custom mouse event handling in android
    viewer()->setIgnoreEvents(true, false, false);
#endif

    updateUI();
}

void MainWindow::on_actionCenter_triggered()
{
    if (Q3DSView *view = viewer()) {
        view->viewerSettings()->setScaleMode(Q3DSViewerSettings::ScaleModeCenter);
        updateUI();
    }
}

void MainWindow::on_actionScale_To_Fit_triggered()
{
    if (Q3DSView *view = viewer()) {
        view->viewerSettings()->setScaleMode(Q3DSViewerSettings::ScaleModeFit);
        updateUI();
    }
}

void MainWindow::on_actionScale_To_Fill_triggered()
{
    if (Q3DSView *view = viewer()) {
        view->viewerSettings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);
        updateUI();
    }
}

QString MainWindow::convertMimeDataToFilename(const QMimeData *mimeData)
{
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            QString str = url.toLocalFile();
            if (str.isEmpty() == false) {
                if ((QFileInfo(str).suffix() == "uip")
                        || (QFileInfo(str).suffix() == "uia")
                        || (QFileInfo(str).suffix() == "uiab"))
                {
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
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());
    updateUI();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());
    QMainWindow::closeEvent(event);
}

#ifdef Q_OS_ANDROID
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    m_swipeStart = event->pos();

    Q3DSView *view = viewer();
    if (view)
        view->presentation()->mousePressEvent(event);

    event->accept();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q3DSView *view = viewer();
    if (view)
        view->presentation()->mouseReleaseEvent(event);

    event->accept();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // Fake swipe down event, as SwipeGesture doesn't work (unless you use 3 fingers..)
    int swipeLength = height() / 10;
    if (ui->actionFull_Screen->isChecked() && event->pos().y() > m_swipeStart.y() + swipeLength) {
        ui->actionFull_Screen->setChecked(false);
        on_actionFull_Screen_triggered();
    }

    Q3DSView *view = viewer();
    if (view)
        view->presentation()->mouseMoveEvent(event);

    event->accept();
}
#endif

void MainWindow::updateUI()
{
    ui->actionConnect->setChecked(m_remoteProject);

    if (m_remoteProject) {
        if (m_connectionInfo)
            m_connectionInfo->setVisible(!m_remoteProject->isConnected());
        if (m_studio3D)
            m_studio3D->setVisible(m_remoteProject->isConnected());
    }

    Q3DSView *view = viewer();
    if (!view)
        return;

    Q3DSViewerSettings::ScaleMode scaleMode = view->viewerSettings()->scaleMode();
    ui->actionCenter->setChecked(scaleMode == Q3DSViewerSettings::ScaleModeCenter);
    ui->actionScale_To_Fit->setChecked(scaleMode == Q3DSViewerSettings::ScaleModeFit);
    ui->actionScale_To_Fill->setChecked(scaleMode == Q3DSViewerSettings::ScaleModeFill);

    QColor matte = view->viewerSettings()->matteColor();
    ui->actionBorder->setChecked(matte.isValid() && matte != QColor(Qt::black));
    ui->actionShowOnScreenStats->setChecked(view->viewerSettings()->isShowRenderStats());
}

void MainWindow::loadRemoteProject()
{
    Q_ASSERT(m_remoteProject);
    const QString remote = m_remoteProject->fileName();
    loadFile(remote);
    updateUI();
}

void MainWindow::remoteConnected()
{
    updateUI();
}

void MainWindow::remoteDisconnected()
{
    updateUI();
}
