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

#include <QtWidgets/qapplication.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qfile.h>
#include <QtStudio3D/q3dssurfaceviewer.h>
#include <QtStudio3D/private/q3dsimagesequencegenerator_p.h>

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
                                 QCoreApplication::translate("main", "The presentation file to open."),
                                 QCoreApplication::translate("main", "[file]"));

    parser.addOption({"sequence",
                      QCoreApplication::translate("main",
                      "Generates an image sequence.\n"
                      "The file argument must be specified.\n"""
                      "Specifying any of the seq-* arguments\n"
                      "implies setting this option.")});
    parser.addOption({"seq-start",
                      QCoreApplication::translate("main",
                      "Start time of the sequence in milliseconds.\n"
                      "The default value is 0."),
                      QCoreApplication::translate("main", "ms"), QString::number(0)});
    parser.addOption({"seq-end",
                      QCoreApplication::translate("main",
                      "End time of the sequence in milliseconds.\n"
                      "The default value is 10000."),
                      QCoreApplication::translate("main", "ms"), QString::number(1000)});
    parser.addOption({"seq-fps",
                      QCoreApplication::translate("main",
                      "Frames per second for the sequence.\n"
                      "The default value is 60."),
                      QCoreApplication::translate("main", "fps"), QString::number(60)});
    parser.addOption({"seq-interval",
                      QCoreApplication::translate("main",
                      "Time interval between frames in the sequence\n"
                      "in milliseconds. The seq-fps argument is ignored\n"
                      "if this argument is used."),
                      QCoreApplication::translate("main", "ms"), QString::number(0)});
    parser.addOption({"seq-width",
                      QCoreApplication::translate("main",
                      "Width of the image sequence.\n"
                      "The default value is 1920."),
                      QCoreApplication::translate("main", "pixels"), QString::number(1920)});
    parser.addOption({"seq-height",
                      QCoreApplication::translate("main",
                      "Height of the image sequence.\n"
                      "The default value is 1080."),
                      QCoreApplication::translate("main", "pixels"), QString::number(1080)});
    parser.addOption({"seq-outpath",
                      QCoreApplication::translate("main",
                      "Output path of the image sequence.\n"
                      "The default value is the current directory."),
                      QCoreApplication::translate("main", "path"), QStringLiteral(".")});
    parser.addOption({"seq-outfile",
                      QCoreApplication::translate("main",
                      "Output filename base for the image sequence.\n"
                      "The default value is derived from the presentation filename."),
                      QCoreApplication::translate("main", "file"), QStringLiteral("")});
    parser.process(a);

    const QStringList files = parser.positionalArguments();
    if (files.count() > 1) {
        qWarning() << "Only one presentation file can be given.";
        parser.showHelp(-1);
    }

    bool generateSequence = parser.isSet("sequence") || parser.isSet("seq-start")
            || parser.isSet("seq-end") || parser.isSet("seq-fps")
            || parser.isSet("seq-interval") || parser.isSet("seq-width")
            || parser.isSet("seq-height") || parser.isSet("seq-outpath")
            || parser.isSet("seq-outfile");

    Q3DSImageSequenceGenerator *generator = nullptr;

    MainWindow w(generateSequence);

    if (generateSequence) {
        if (files.count() != 1) {
            qWarning() << "Presentation file is required for generating an image sequence.";
            parser.showHelp(-1);
        }
        generator = new Q3DSImageSequenceGenerator;
        QObject::connect(generator, &Q3DSImageSequenceGenerator::progress,
                         &w, &MainWindow::generatorProgress);
        QObject::connect(generator, &Q3DSImageSequenceGenerator::finished,
                         &w, &MainWindow::generatorFinished);
        w.setGeneratorDetails(files.first());
        generator->generateImageSequence(
                    files.first(),
                    parser.value("seq-start").toDouble(),
                    parser.value("seq-end").toDouble(),
                    parser.value("seq-fps").toDouble(),
                    parser.value("seq-interval").toDouble(),
                    parser.value("seq-width").toInt(),
                    parser.value("seq-height").toInt(),
                    parser.value("seq-outpath"),
                    parser.value("seq-outfile"));
    } else if (!files.isEmpty()) {
        w.loadFile(files.first());
    }

    w.show();
#ifndef Q_OS_ANDROID
    QFile styleFile(":/style.qss");
#else
    // We need a different stylesheet for Android, or the controls are too small, and some of
    // them aren't displayed correctly (see QTBUG-41773)
    QFile styleFile(":/style_android.qss");
#endif
    styleFile.open(QFile::ReadOnly);

    QString style(styleFile.readAll());
    a.setStyleSheet(style);

    return a.exec();
}
