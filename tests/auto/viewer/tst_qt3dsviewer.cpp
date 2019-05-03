/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "tst_qt3dsviewer.h"

#include <QtQuick/QQuickItem>
#include <QtGui/QSurfaceFormat>
#include <QtStudio3D/Q3DSPresentation>
#include <QtStudio3D/Q3DSElement>
#include <QtStudio3D/Q3DSViewerSettings>
#include <QtCore/QRandomGenerator>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>

void messageOutput(QtMsgType type, const QMessageLogContext &context,
    const QString &msg)
{
    Q_UNUSED(context);
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        break; // swallow
    case QtFatalMsg:
        QFAIL(msg.toLocal8Bit().constData());
    }
}

void tst_qt3dsviewer::initTestCase()
{
    qInstallMessageHandler(messageOutput);
    m_viewer.setTitle(QStringLiteral("tst_qt3dsviewer"));
}

void tst_qt3dsviewer::cleanupTestCase()
{
    QCOMPARE(m_studio3DItem->property("error").toString(), QString());
}

void tst_qt3dsviewer::init()
{
#if defined(Q_OS_ANDROID)
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    m_viewer.setSource(QUrl("qrc:/tst_qt3dsviewer.qml"));
    m_studio3DItem = m_viewer.rootObject();
    m_presentation = nullptr;
    m_settings = nullptr;

    QVERIFY(m_studio3DItem);

    const auto children = m_studio3DItem->children();
    for (auto &child : children) {
        if (!m_presentation)
            m_presentation = qobject_cast<Q3DSPresentation *>(child);
        if (!m_settings)
            m_settings = qobject_cast<Q3DSViewerSettings *>(child);
    }

    QVERIFY(m_presentation);
    QVERIFY(m_settings);
}

void tst_qt3dsviewer::cleanup()
{
    deleteCreatedElements(0);
    m_viewer.hide();
}

void tst_qt3dsviewer::testEmpty()
{
    m_presentation->setProperty("source", QUrl());
    m_viewer.show();
    QTest::qWait(1000);
    QCOMPARE(m_studio3DItem->property("running").toBool(), false);
}

void tst_qt3dsviewer::testLoading()
{
    QCOMPARE(m_studio3DItem->property("running").toBool(), false);
    m_viewer.show();
    QTest::qWait(1000);
    QCOMPARE(m_studio3DItem->property("running").toBool(), true);
}


void tst_qt3dsviewer::testSlides()
{
    QSignalSpy spyEntered(m_presentation,
                          SIGNAL(slideEntered(const QString &, unsigned int, const QString &)));
    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    QCOMPARE(spyEntered.count(), 0);
    QCOMPARE(spyExited.count(), 0);

    m_viewer.show();
    QTest::qWait(1000);

    QCOMPARE(spyEntered.count(), 1);
    QCOMPARE(spyExited.count(), 0);

    QVERIFY(spyExited.wait(12000));
    QCOMPARE(spyEntered.count(), 2);
    QCOMPARE(spyExited.count(), 1);
}

void tst_qt3dsviewer::testFrameUpdates()
{
    QSignalSpy spyFrames(m_studio3DItem, SIGNAL(frameUpdate()));
    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    m_viewer.show();
    QVERIFY(spyExited.wait(12000));
    QVERIFY(spyFrames.count() > 590); // Should be 60 with fudge for startup
}

void tst_qt3dsviewer::testSettings()
{
    m_viewer.show();
    m_settings->setMatteColor(QColor("#0000ff"));
    QVERIFY(m_settings->matteColor() == QColor("#0000ff"));

    // Save and change matte color
    m_settings->save("", "tst_qt3dsviewer", "tst_qt3dsviewer");
    m_settings->setMatteColor(QColor("#00ff00"));
    QVERIFY(m_settings->matteColor() == QColor("#00ff00"));
    // Load and previous matte color should be back
    m_settings->load("", "tst_qt3dsviewer", "tst_qt3dsviewer");
    QVERIFY(m_settings->matteColor() == QColor("#0000ff"));
}

void tst_qt3dsviewer::testCreateElement()
{
    // Currently this method is very bare bones on actual testing due to lack of programmatic
    // feedback on dynamic object creation.
    // It can be used to visually check if items are getting created.
    m_viewer.show();

    m_settings->setShowRenderStats(true);
    m_settings->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));

    int animValue = 0;

    QHash<QString, QVariant> data;
    data.insert(QStringLiteral("name"), QStringLiteral("New Cylinder"));
    data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
    data.insert(QStringLiteral("material"), QString());
    data.insert(QStringLiteral("starttime"), 0);
    data.insert(QStringLiteral("endtime"), 4500);
    data.insert(QStringLiteral("position"),
                QVariant::fromValue<QVector3D>(QVector3D(200, 300, 200)));
    data.insert(QStringLiteral("opacity"), 20.0);

    createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);

    // Elements can be registered before they are created
    Q3DSElement newCylinder(m_presentation, QStringLiteral("Scene.Layer.New Cylinder"));
    Q3DSElement newCylinder2(m_presentation,
                             QStringLiteral("Scene.Layer.New Cylinder.New Cylinder 2"));
    Q3DSElement newSphere(m_presentation, QStringLiteral("Scene.Layer.Cube2.New Sphere"));

    QTimer animationTimer;
    animationTimer.setInterval(10);
    int animDir = 1;
    QObject::connect(&animationTimer, &QTimer::timeout, [&]() {
        if (qAbs(animValue) > 100)
            animDir = -animDir;
        animValue += animDir;
        newCylinder.setAttribute(QStringLiteral("rotation.x"), animValue * 4);
        newCylinder2.setAttribute(QStringLiteral("position.y"), animValue * 3);
        newSphere.setAttribute(QStringLiteral("position.x"), 50 + animValue * 2);
    });

    // Create objects to slides 1 & 2 while slide 1 is executing
    QTimer::singleShot(1000, [&]() {
        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Cylinder 2"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Red"));
        data.insert(QStringLiteral("starttime"), 500);
        data.insert(QStringLiteral("endtime"), 5000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(50, animValue, 50)));

        createElement(QStringLiteral("Scene.Layer.New Cylinder"),
                      QStringLiteral("Slide1"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Sphere"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 1000);
        data.insert(QStringLiteral("endtime"), 4000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(animValue, 75, 0)));

        createElement(QStringLiteral("Scene.Layer.Cube2"), QStringLiteral("Slide2"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("Sphere To Delete"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Red"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 10000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, -100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide2"), data);

        animationTimer.start();
    });

    // Switch to slide 2
    QVERIFY(spyExited.wait(20000));

    // Remove dynamically added object
    QTimer::singleShot(3000, [&]() {
        m_presentation->deleteElement(QStringLiteral("Scene.Layer.Sphere To Delete"));
        // Don't remove the deleted element from createdElements to test removing already deleted
        // element later when everything is cleaned up.
    });

    // Create objects to slides 1 and 2 while slide 2 is executing
    QTimer::singleShot(2000, [&]() {
        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Cylinder 3"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 3000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, -100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Sphere 2"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 5000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, 100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide2"), data);
    });

    // Switch to slide 1
    QVERIFY(spyExited.wait(20000));

    QTimer createTimer;
    createTimer.setInterval(0);
    static int elemCounter = 0;
    QRandomGenerator rnd;

    auto createElementsConnection = QObject::connect(&createTimer, &QTimer::timeout, [&]() {
        // Create a bunch of elements to slide 2 in small batches to avoid slowdown
        for (int i = 0; i < 5; ++i) {
            ++elemCounter;
            data.clear();
            QString elementName = QStringLiteral("MassElement_%1").arg(elemCounter);
            data.insert(QStringLiteral("name"), elementName);
            data.insert(QStringLiteral("sourcepath"),
                        elemCounter % 2 ? QStringLiteral("#Cube") : QStringLiteral("#Cone"));
            data.insert(QStringLiteral("material"),
                        elemCounter % 2 ? QStringLiteral("Basic Green")
                                        : QStringLiteral("Basic Red"));
            data.insert(QStringLiteral("position"),
                        QVariant::fromValue<QVector3D>(QVector3D(rnd.bounded(-600, 600),
                                                                 rnd.bounded(-600, 600),
                                                                 rnd.bounded(800, 1200))));

            createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide2"), data);
        }
        if (elemCounter >= 1000) {
            qDebug() << "Extra elements created:" << elemCounter;
            createTimer.stop();
        }
    });
    qDebug() << "Start creating extra elements";
    createTimer.start();

    // Switch to slide 2
    QVERIFY(spyExited.wait(20000));
    QObject::disconnect(createElementsConnection);

    QTest::qWait(500);
    deleteCreatedElements(1);

    // Switch to slide 1
    QVERIFY(spyExited.wait(20000));
    QTest::qWait(1000);
}

void tst_qt3dsviewer::testCreateMaterial()
{
    m_viewer.show();

    m_settings->setShowRenderStats(true);
    m_settings->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    QSignalSpy spyMatCreated(m_presentation, SIGNAL(materialCreated(const QString &)));

    // Create material via .materialdef file in resources
    m_presentation->createMaterial(
                QStringLiteral("Scene"),
                QStringLiteral(
                    ":/scenes/simple_cube_animation/materials/Basic Blue.materialdef"));
    m_presentation->createMaterial(
                QStringLiteral("Scene"),
                QStringLiteral(
                    ":/scenes/simple_cube_animation/materials/Basic Texture.materialdef"));

    // Create material directly from materialdef content
    auto loadMatDefFile = [&](const QString &fileName) -> QString {
        QFile matDefFile(fileName);
        if (!matDefFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};

        QTextStream in(&matDefFile);
        return in.readAll();
    };
    QString matDef = loadMatDefFile(
                QStringLiteral(":/scenes/simple_cube_animation/materials/Copper.materialdef"));
    QVERIFY(!matDef.isEmpty());

    m_presentation->createMaterial(QStringLiteral("Scene"), matDef);

    QObject::connect(m_presentation, &Q3DSPresentation::materialCreated,
                     [this](const QString &name) {
        QHash<QString, QVariant> data;

        if (name == QStringLiteral("materials/Basic Blue")) {
            data.insert(QStringLiteral("name"), QStringLiteral("Blue Cylinder"));
            data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
            data.insert(QStringLiteral("material"), name);
            data.insert(QStringLiteral("position"),
                        QVariant::fromValue<QVector3D>(QVector3D(200, 300, 200)));
            createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
        }

        if (name == QStringLiteral("materials/Basic Texture")) {
            data.insert(QStringLiteral("name"), QStringLiteral("Textured Cone"));
            data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cone"));
            data.insert(QStringLiteral("material"), name);
            data.insert(QStringLiteral("position"),
                        QVariant::fromValue<QVector3D>(QVector3D(-200, -300, 200)));
            createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
        }

        if (name == QStringLiteral("materials/Copper")) {
            data.insert(QStringLiteral("name"), QStringLiteral("Copper Sphere"));
            data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
            data.insert(QStringLiteral("material"), name);
            data.insert(QStringLiteral("position"),
                        QVariant::fromValue<QVector3D>(QVector3D(-200, 300, 200)));
            createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
        }

        if (name == QStringLiteral("materials/Just Yellow")) {
            QHash<QString, QVariant> data;
            data.insert(QStringLiteral("name"), QStringLiteral("Yellow Cube"));
            data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cube"));
            data.insert(QStringLiteral("material"), name);
            data.insert(QStringLiteral("position"),
                        QVariant::fromValue<QVector3D>(QVector3D(200, -300, 200)));
            createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
        }
    });

    // Create material after start
    QTimer::singleShot(1000, [&]() {
        QString md = loadMatDefFile(QStringLiteral(
                        ":/scenes/simple_cube_animation/materials/Basic Blue.materialdef"));
        // Modify the diffuse color and material name so that we can be sure it is a new one
        md.replace(QStringLiteral("Basic Blue"), QStringLiteral("Just Yellow"));
        md.replace(QRegularExpression(QStringLiteral("\"diffuse\">.*<")),
                   QStringLiteral("\"diffuse\">1 1 0 1<"));
        m_presentation->createMaterial(QStringLiteral("Scene"), md);
    });

    QVERIFY(spyExited.wait(20000));
    QCOMPARE(spyMatCreated.count(), 4);
    QTest::qWait(200); // Extra wait to verify slide change visually
}

void tst_qt3dsviewer::deleteCreatedElements(int interval)
{
    QTimer deleteTimer;
    deleteTimer.setInterval(interval);
    int elemCounter = m_createdElements.size() - 1;
    QObject::connect(&deleteTimer, &QTimer::timeout, [&]() {
        // Delete all elements we created previously
        if (elemCounter >= 0) {
            m_presentation->deleteElement(m_createdElements[elemCounter]);
            --elemCounter;
        } else {
            qDebug() << "Extra elements deleted";
            deleteTimer.stop();
        }
    });
    qDebug() << "Start deleting extra elements";
    deleteTimer.start();

    while (deleteTimer.isActive())
        QTest::qWait(20);

    m_createdElements.clear();
}

void tst_qt3dsviewer::createElement(const QString &parentElementPath, const QString &slideName,
                                    const QHash<QString, QVariant> &properties)
{
    m_createdElements << parentElementPath + QLatin1Char('.')
                         + properties[QStringLiteral("name")].toString();
    m_presentation->createElement(parentElementPath, slideName, properties);
}

QTEST_MAIN(tst_qt3dsviewer)
