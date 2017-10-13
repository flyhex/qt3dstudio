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

#include "q3dsimagesequencegeneratorthread_p.h"
#include "q3dssurfaceviewer.h"
#include "q3dspresentation.h"
#include "q3dsviewersettings.h"

#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglframebufferobject.h>

bool Q3DSImageSequenceGeneratorThread::initialize(
        const QString &presentation, qreal start, qreal end, qreal fps, qreal frameInterval,
        int width, int height, const QString &outPath, const QString &outFile)
{
    QFileInfo fileInfo(presentation);
    if (!fileInfo.exists()) {
        qWarning("Generating image sequence failed - File not found");
        return false;
    }

    m_outputFileName = QStringLiteral("%2/%1_%3.png");
    if (outFile.isEmpty()) {
        m_outputFileName = m_outputFileName.arg(fileInfo.baseName())
                .arg(outPath).arg(QStringLiteral("%1"));
    } else {
        m_outputFileName = m_outputFileName.arg(outFile).arg(outPath).arg(QStringLiteral("%1"));
    }

    m_sourceUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);

    m_context = new QOpenGLContext;
    m_context->setFormat(format);
    if (!m_context->create()) {
        qWarning("Generating image sequence failed - Failed to create context");
        return false;
    }

    m_surface = new QOffscreenSurface;
    m_surface->setFormat(m_context->format());
    m_surface->create();

    m_mainThread = QThread::currentThread();
    m_surface->moveToThread(this);
    m_context->moveToThread(this);

    m_start = start;
    m_end = end;
    m_fps = fps;
    m_frameInterval = frameInterval;
    m_width = width;
    m_height = height;

    return true;
}

Q3DSImageSequenceGeneratorThread::~Q3DSImageSequenceGeneratorThread() {
    delete m_context;
    delete m_surface;
}

void Q3DSImageSequenceGeneratorThread::run() {
    if (!m_context->makeCurrent(m_surface)) {
        qWarning("Generating image sequence failed - Couldn't make context current");
        Q_EMIT generationFinished(false);
        cleanup();
        return;
    }

    const QSize size(m_width, m_height);
    QOpenGLFramebufferObject fbo(size, QOpenGLFramebufferObject::CombinedDepthStencil);

    Q3DSSurfaceViewer viewer;
    viewer.presentation()->setSource(m_sourceUrl);
    viewer.settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    viewer.setUpdateInterval(-1);
    viewer.setAutoSize(false);
    viewer.setSize(size);

    if (!viewer.initialize(m_surface, m_context, fbo.handle())) {
        qWarning("Generating image sequence failed - Viewer initialization failed.");
        Q_EMIT generationFinished(false);
        cleanup();
        return;
    }

    if (m_frameInterval <= 0)
        m_frameInterval = 1000.0 / m_fps;

    // Presentations always assume you want to start animating at time zero and set a local
    // offset to global time when they render the first frame. This means we need to always
    // render a frame at global time zero first.
    if (qRound(m_start) != 0) {
        viewer.presentation()->setGlobalAnimationTime(0);
        viewer.update();
    }

    // Add a bit of time to the end time to ensure we don't lose the last frame to rounding errors
    m_end += m_frameInterval / 10000.0;

    int frameCount = 0;
    int totalFrames = qCeil((m_end - m_start) / m_frameInterval);
    for (qreal t = m_start; t <= m_end; t += m_frameInterval) {
        ++frameCount;
        viewer.presentation()->setGlobalAnimationTime(qRound64(t));
        viewer.update();
        if (!fbo.toImage().save(m_outputFileName.arg(frameCount))) {
            qWarning().nospace() << "Generating image sequence failed - Failed to write file: '"
                                 << m_outputFileName.arg(frameCount) << "'";
            Q_EMIT generationFinished(false);
            cleanup();
            return;
        }
        Q_EMIT progress(totalFrames, frameCount);
    }

    Q_EMIT generationFinished(true);
    cleanup();
}

void Q3DSImageSequenceGeneratorThread::cleanup()
{
    m_context->doneCurrent();
    delete m_context;
    m_context = nullptr;
    // Surface needs to be deleted in the thread it was created in
    m_surface->moveToThread(m_mainThread);
    deleteLater();
}
