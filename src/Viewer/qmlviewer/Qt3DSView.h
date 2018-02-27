/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#ifndef Q3DS_VIEW_H
#define Q3DS_VIEW_H


#include <QtStudio3D/private/q3dscommandqueue_p.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtQuick/qquickframebufferobject.h>

QT_BEGIN_NAMESPACE

class Q3DSRenderer;
class Q3DSViewerSettings;
class Q3DSPresentationItem;

class Q3DSView : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(Q3DSPresentationItem *presentation READ presentation CONSTANT)
    Q_PROPERTY(Q3DSViewerSettings *viewerSettings READ viewerSettings CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    Q3DSView();
    virtual ~Q3DSView();

    QQuickFramebufferObject::Renderer *createRenderer() const override;

    bool isRunning() const;
    Q3DSPresentationItem *presentation() const;
    Q3DSViewerSettings *viewerSettings() const;
    QString error() const;
    void setError(const QString &error);

    void getCommands(bool emitInitialize, CommandQueue &renderQueue);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void setIgnoreEvents(bool mouse, bool wheel, bool keyboard);

    void componentComplete() override;

Q_SIGNALS:
    void frameUpdate();
    void runningChanged(bool initialized);
    void errorChanged(const QString &error);
    Q_REVISION(1) void presentationReady();

public Q_SLOTS:
    void reset();

protected Q_SLOTS:
    void handleWindowChanged(QQuickWindow *window);
    void handleVisibleChanged();
    void tick();
    void requestResponseHandler(const QString &elementPath, CommandType commandType,
                                void *requestData);

protected:
    Q3DSViewerSettings *m_viewerSettings;
    Q3DSPresentationItem *m_presentation;

    bool m_emitRunningChange;
    bool m_isRunning;
    bool m_ignoreMouseEvents;
    bool m_ignoreWheelEvents;
    bool m_ignoreKeyboardEvents;

    CommandQueue m_pendingCommands;
    qreal m_pixelRatio;
    QString m_error;
};

QT_END_NAMESPACE

#endif // Q3DS_VIEW_H
