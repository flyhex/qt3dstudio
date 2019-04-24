/****************************************************************************
**
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

#ifndef Q3DSSURFACEVIEWER_H
#define Q3DSSURFACEVIEWER_H

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qsize.h>
#include <QtCore/qurl.h>
#include <QtGui/qimage.h>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class Q3DSSurfaceViewerPrivate;
class QSurface;
class QOpenGLContext;
class Q3DSViewerSettings;
class Q3DSPresentation;

class Q_STUDIO3D_EXPORT Q3DSSurfaceViewer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DSSurfaceViewer)
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(bool autoSize READ autoSize WRITE setAutoSize NOTIFY autoSizeChanged)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval NOTIFY updateIntervalChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString presentationId READ presentationId WRITE setPresentationId NOTIFY presentationIdChanged)

public:
    explicit Q3DSSurfaceViewer(QObject *parent = nullptr);
    ~Q3DSSurfaceViewer();

    bool initialize(QSurface *surface, QOpenGLContext *context, GLuint fboId = 0);

    QImage grab(const QRect &rect = QRect());

    // Property accessors
    QSize size() const;
    bool autoSize() const;
    int updateInterval() const;
    bool isRunning() const;
    QString presentationId() const;

    int fboId() const;
    QSurface *surface() const;
    QOpenGLContext *context() const;

    Q3DSViewerSettings *settings() const;
    Q3DSPresentation *presentation() const;

public Q_SLOTS:
    void setSize(const QSize &size);
    void setAutoSize(bool autoSize);
    void setUpdateInterval(int interval);
    void update();
    void shutdown();
    void reset();
    void setPresentationId(const QString &id);

Q_SIGNALS:
    void sizeChanged(const QSize &size);
    void autoSizeChanged(bool autoSize);
    void updateIntervalChanged(bool autoUpdate);
    void runningChanged(bool initialized);
    void frameUpdated();
    void presentationIdChanged(const QString &id);

private:
    Q_DISABLE_COPY(Q3DSSurfaceViewer)
    Q3DSSurfaceViewerPrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // Q3DSSURFACEVIEWER_H
