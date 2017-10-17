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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtQuickWidgets/QQuickWidget>

#include "remotedeploymentreceiver.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QMimeData;
class Q3DSView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(bool generatorMode, QWidget *parent = nullptr);
    ~MainWindow();

    void loadFile(const QString &filename);
    void loadProject(const QByteArray &data);
    void updateProgress(int percent);
    void setGeneratorDetails(const QString &filename);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
#ifdef Q_OS_ANDROID
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#endif

private:
    Q3DSView *viewer() const;
    QString convertMimeDataToFilename(const QMimeData *mimeData);
    void setupGeneratorUI();

public Q_SLOTS:
    void generatorProgress(int totalFrames, int frameCount);
    void generatorFinished(bool success, const QString &details);

private Q_SLOTS:
    void on_actionToggle_Scale_Mode_triggered();
    void on_actionToggle_Shade_Mode_triggered();
    void on_actionBorder_triggered();
    void on_actionFull_Screen_triggered();
    void on_actionShowOnScreenStats_triggered();
    void on_actionQuit_triggered();
    void on_actionOpen_triggered();
    void on_actionConnect_triggered();
    void on_actionReload_triggered();
    void on_actionCenter_triggered();
    void on_actionScale_To_Fit_triggered();
    void on_actionScale_To_Fill_triggered();

    void updateUI(bool statusVisible = false);

    void loadRemoteDeploymentReceiver();
    void remoteConnected();
    void remoteDisconnected();

private:
    Ui::MainWindow *ui;
    Q3DSView *m_studio3D = nullptr;
    QQuickItem *m_connectionInfo = nullptr;
    QString m_openFileDir;
    bool m_embeddedMode;
    QPoint m_swipeStart;
    RemoteDeploymentReceiver *m_remoteDeploymentReceiver = nullptr;
    bool m_generatorMode;
    QQuickItem *m_generatorInfo = nullptr;
};

#endif // MAINWINDOW_H
