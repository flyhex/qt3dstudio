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

#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

int main(int argc, char *argv[])
{
    // to enable QOpenGLWidget to work on macOS, we must set the default
    // QSurfaceFormat before QApplication is created. Otherwise context-sharing
    // fails and QOpenGLWidget breaks.

    // fortunately, we know which OpenGL version we can use on macOS, so we
    // can simply hard-code it here.
#if defined(Q_OS_MACOS)
    QSurfaceFormat openGL33Format;
    openGL33Format.setRenderableType(QSurfaceFormat::OpenGL);
    openGL33Format.setProfile(QSurfaceFormat::CoreProfile);
    openGL33Format.setMajorVersion(3);
    openGL33Format.setMinorVersion(3);
    openGL33Format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(openGL33Format);
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("The Qt Company");
    QCoreApplication::setOrganizationDomain("qt.io");
    QCoreApplication::setApplicationName("Qt 3D Viewer");

    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("file",
        QCoreApplication::translate("main", "The file to open."));
    parser.process(a);

    const QStringList files = parser.positionalArguments();
    if (files.count() > 1)
        parser.showHelp(-1);

    MainWindow w;
    w.resize(1280, 720);
    w.show();

    if (!files.isEmpty())
        w.loadFile(files.first());

    QFile styleFile(":/resources/style/dark.qss");
    styleFile.open(QFile::ReadOnly);

    QString style(styleFile.readAll());
    a.setStyleSheet(style);

    return a.exec();
}
