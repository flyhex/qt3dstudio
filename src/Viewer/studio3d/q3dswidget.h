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

#ifndef Q3DSWIDGET_H
#define Q3DSWIDGET_H

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtWidgets/QOpenGLWidget>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

class Q3DSWidgetPrivate;
class Q3DSViewerSettings;
class Q3DSPresentation;

class Q_STUDIO3D_EXPORT Q3DSWidget : public QOpenGLWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DSWidget)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval NOTIFY updateIntervalChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

public:
    explicit Q3DSWidget(QWidget *parent = nullptr);
    virtual ~Q3DSWidget();

    bool initialize();

    // Property accessors
    int updateInterval() const;
    bool isRunning() const;

    Q3DSViewerSettings *settings() const;
    Q3DSPresentation *presentation() const;

public Q_SLOTS:
    void setUpdateInterval(int interval);
    void shutdown();
    void reset();

Q_SIGNALS:
    void updateIntervalChanged(bool autoUpdate);
    void runningChanged(bool initialized);

protected:
    // Qt event handling
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *e) override;

    // Qt overrides
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    Q_DISABLE_COPY(Q3DSWidget)
    Q3DSWidgetPrivate *d_ptr;
};
QT_END_NAMESPACE
#endif // Q3DSWIDGET_H
