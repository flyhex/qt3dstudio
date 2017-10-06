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

#include <QtStudio3D/Q3DSSurfaceViewer>
#include <QtStudio3D/Q3DSViewerSettings>
#include <QtStudio3D/Q3DSPresentation>
#include <QtStudio3D/Q3DSSceneElement>
#include <QtStudio3D/Q3DSElement>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtCore/QUrl>
#include <QtCore/QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    QWindow window;

    QSize size(1200, 800);
    window.resize(size);
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.setTitle(QStringLiteral("Qt 3D Studio surface viewer example"));
    window.create();

    QOpenGLContext context;
    context.setFormat(window.format());
    context.create();

    Q3DSSurfaceViewer viewer;
    viewer.presentation()->setSource(QUrl(QStringLiteral("qrc:/presentation/circling_cube.uip")));
    viewer.setUpdateInterval(0);
    viewer.settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);
    viewer.settings()->setShowRenderStats(true);

    Q3DSSceneElement sceneElement(viewer.presentation(), QStringLiteral("Scene"));
    Q3DSElement counterElement(viewer.presentation(), QStringLiteral("Scene.Layer.Loopcounter"));

    viewer.initialize(&window, &context);

    window.show();

    int n = 0;
    QString loopCounter = QStringLiteral("Loop %1");
    QObject::connect(&sceneElement, &Q3DSSceneElement::currentSlideIndexChanged, [&]() {
        if (sceneElement.currentSlideIndex() == 1)
            n++;
        counterElement.setAttribute(QStringLiteral("textstring"), loopCounter.arg(n));
    });

    return app.exec();
}
