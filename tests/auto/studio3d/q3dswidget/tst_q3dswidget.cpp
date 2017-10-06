/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include <QtTest/QtTest>
#include <QtStudio3D/q3dswidget.h>
#include <QtStudio3D/q3dsviewersettings.h>
#include <QtStudio3D/q3dspresentation.h>
#include <QtStudio3D/q3dssceneelement.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qimage.h>
#include <QtGui/qscreen.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qmainwindow.h>
#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include "../shared/shared_presentations.h"

class tst_Q3DSWidget : public QObject
{
    Q_OBJECT
public:
    tst_Q3DSWidget();
    ~tst_Q3DSWidget() {}

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testBasics();
    void testSourceChange();
    void testUpdateInterval();
    void testMultiple();
    void testReset();
    void testSettings();
    void testPresentation();
    void testPresentationActivation();
    void testScene();
    void testMouseInput();

private:
    QMainWindow *createWindow(const QSize &size);

    // Created viewers are returned via *&viewer parameter rather than return value, so that we can
    // use QCOMPARE and QVERIFY inside these functions (they require void return value).
    void createViewer(Q3DSWidget *&viewer, QMainWindow *window, const QUrl &url,
                      int updateInterval);
    void createWindowAndViewer(Q3DSWidget *&viewer, const QUrl &url,
                               const QSize &size = QSize(), int updateInterval = 0);
    void checkPixel(Q3DSWidget *viewer, const QColor &color,
                    const QPoint &pixel = QPoint(50, 50));

    QMainWindow *m_window;
    Q3DSWidget *m_viewer;
};

tst_Q3DSWidget::tst_Q3DSWidget()
    : m_window(nullptr)
    , m_viewer(nullptr)
{
}

//#define DUMP_LOGFILE  // Uncomment log Qt 3D Studio internal messages to log.txt file
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    switch (type) {
    //    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg: {
#ifdef DUMP_LOGFILE
        QFile file("log.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream stream(&file);
            stream << msg << "\n";
        }
        file.close();
#endif
    } break; // swallow
    case QtFatalMsg:
        QFAIL(msg.toLocal8Bit().constData());
    }
}

void tst_Q3DSWidget::initTestCase()
{
    qInstallMessageHandler(messageOutput);
#ifdef DUMP_LOGFILE
    QFile file("log.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream stream(&file);
        stream << "Log file: " << QTime::currentTime().toString() << "\n";
    }
    file.close();
#endif
}

void tst_Q3DSWidget::init()
{
}

void tst_Q3DSWidget::cleanup()
{
    if (m_window)
        m_window->close();

    delete m_viewer;
    m_viewer = nullptr;

    delete m_window;
    m_window = nullptr;
}

QMainWindow *tst_Q3DSWidget::createWindow(const QSize &size)
{
    QMainWindow *window = new QMainWindow();

    window->resize(size);

    return window;
}

void tst_Q3DSWidget::createViewer(Q3DSWidget *&viewer, QMainWindow *window, const QUrl &url,
                                  int updateInterval)
{
    viewer = new Q3DSWidget();
    QSignalSpy spy(viewer, &Q3DSWidget::runningChanged);

    window->setCentralWidget(viewer);

    viewer->presentation()->setSource(url);
    QCOMPARE(viewer->presentation()->source(), url);

    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0);

    viewer->setUpdateInterval(updateInterval);

    QVERIFY(viewer->initialize());

    QCOMPARE(spy.count(), 1);
    QVERIFY(viewer->isRunning());
}

void tst_Q3DSWidget::createWindowAndViewer(Q3DSWidget *&viewer, const QUrl &url,
                                           const QSize &size, int updateInterval)
{
    QSize actualSize = size;
    if (actualSize.isEmpty())
        actualSize = QSize(300, 200);

    m_window = createWindow(actualSize);
    m_window->show();

    createViewer(viewer, m_window, url, updateInterval);

    QGuiApplication::processEvents();
}

void tst_Q3DSWidget::checkPixel(Q3DSWidget *viewer, const QColor &color,
                                const QPoint &pixel)
{
    // Grab operation is potentially costly, so retry only every second instead of using
    // QTRY_COMPARE which would try it every 50ms. We also want to wait first as it takes some time
    // for the presentation to be displayed.
    QColor grabColor;
    for (int i = 0; i < 20; i++) {
        QTest::qWait(1000);
        QImage image = viewer->grabFramebuffer();
        grabColor = QColor(image.pixel(pixel));
        if (grabColor == color)
            break;
    }
    QCOMPARE(grabColor, color);
}

void tst_Q3DSWidget::testBasics()
{
    createWindowAndViewer(m_viewer, RED);

    QSignalSpy spy(m_viewer, &Q3DSWidget::runningChanged);
    QVERIFY(spy.isValid());

    checkPixel(m_viewer, Qt::red);

    m_viewer->shutdown();

    QCOMPARE(spy.count(), 1);
    QVERIFY(!m_viewer->isRunning());
}

void tst_Q3DSWidget::testSourceChange()
{
    createWindowAndViewer(m_viewer, RED);

    QSignalSpy spy(m_viewer->presentation(), &Q3DSPresentation::sourceChanged);
    QVERIFY(spy.isValid());
    QVERIFY(m_viewer->presentation()->source() == RED);

    checkPixel(m_viewer, Qt::red);

    // Different source
    m_viewer->presentation()->setSource(BLUE);
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_viewer->presentation()->source() == BLUE);

    checkPixel(m_viewer, Qt::blue);

    // Reset same source
    m_viewer->presentation()->setSource(BLUE);
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_viewer->presentation()->source() == BLUE);

    checkPixel(m_viewer, Qt::blue);

    // Different source again
    m_viewer->presentation()->setSource(RED);
    QCOMPARE(spy.count(), 2);
    QVERIFY(m_viewer->presentation()->source() == RED);

    checkPixel(m_viewer, Qt::red);
}

void tst_Q3DSWidget::testUpdateInterval()
{
    createWindowAndViewer(m_viewer, ANIMATION);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spy(m_viewer, &Q3DSWidget::updateIntervalChanged);
    QVERIFY(spy.isValid());
    QVERIFY(m_viewer->updateInterval() == 0);

    checkPixel(m_viewer, Qt::black);
    {
        // Grab two images two seconds apart to verify animation is happening
        QImage image1 = m_viewer->grabFramebuffer();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grabFramebuffer();
        QVERIFY(image1 != image2);

    }
    {
        m_viewer->setUpdateInterval(100000);
        QVERIFY(m_viewer->updateInterval() == 100000);
        QCOMPARE(spy.count(), 1);
        // Can't test if animation actually stopped, as grabbing the viewer forces update on it
    }
    {
        m_viewer->setUpdateInterval(20);
        QCOMPARE(spy.count(), 2);
        QVERIFY(m_viewer->updateInterval() == 20);

        // Non-zero interval short enough to see animation
        QImage image1 = m_viewer->grabFramebuffer();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grabFramebuffer();
        QVERIFY(image1 != image2);
    }
    {
        m_viewer->setUpdateInterval(-1);
        QCOMPARE(spy.count(), 3);
        QVERIFY(m_viewer->updateInterval() == -1);
        // Can't test if animation actually stopped, as grabbing the viewer forces update on it
    }
}

void tst_Q3DSWidget::testMultiple()
{
    int viewerCount = 3;

    QVector<QMainWindow *> windows;
    QVector<Q3DSWidget *> viewers;

    windows.resize(viewerCount);
    viewers.resize(viewerCount);

    QSize size(200, 150);
    QUrl url;
    for (int i = 0; i < viewerCount; i++) {
        windows[i] = createWindow(size);
        if (i % 2)
            url = RED;
        else
            url = BLUE;
        windows[i]->move(10 + i * 50, 10 + i * 50);
        windows[i]->show();
        createViewer(viewers[i], windows[i], url, 0);
        QGuiApplication::processEvents();
    }

    for (int i = 0; i < viewerCount; i++) {
        if (i % 2)
            checkPixel(viewers[i], Qt::red);
        else
            checkPixel(viewers[i], Qt::blue);
    }

    for (QMainWindow *w : windows) {
        w->close();
        delete w;
    }
    windows.clear();
}

void tst_Q3DSWidget::testReset()
{
    createWindowAndViewer(m_viewer, RED);

    checkPixel(m_viewer, Qt::red);

    // TODO: change color of the cube to blue (not currently possible, API missing)
    // checkPixel(m_viewer, Qt::blue);

    m_viewer->reset();

    checkPixel(m_viewer, Qt::red);
}

void tst_Q3DSWidget::testSettings()
{
    int width = 500;
    int height = 500;

    createWindowAndViewer(m_viewer, SETTINGS, QSize(width, height));

    Q3DSViewerSettings *s = m_viewer->settings();

    QSignalSpy spy1(s, &Q3DSViewerSettings::matteColorChanged);
    QSignalSpy spy2(s, &Q3DSViewerSettings::showRenderStatsChanged);
    QSignalSpy spy3(s, &Q3DSViewerSettings::shadeModeChanged);
    QSignalSpy spy4(s, &Q3DSViewerSettings::scaleModeChanged);
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());
    QVERIFY(spy3.isValid());
    QVERIFY(spy4.isValid());

    // Check defaults
    QCOMPARE(s->matteColor(), QColor(Qt::black));
    QCOMPARE(s->isShowRenderStats(), false);
    QCOMPARE(s->shadeMode(), Q3DSViewerSettings::ShadeModeShaded);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeCenter);

    // Matte
    checkPixel(m_viewer, Qt::black);

    s->setMatteColor(Qt::cyan);
    QCOMPARE(s->matteColor(), QColor(Qt::cyan));

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 0);
    QCOMPARE(spy3.count(), 0);
    QCOMPARE(spy4.count(), 0);

    checkPixel(m_viewer, Qt::cyan);

    // Render stats
    QImage image1 = m_viewer->grabFramebuffer();

    s->setShowRenderStats(true);
    QCOMPARE(s->isShowRenderStats(), true);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 0);
    QCOMPARE(spy4.count(), 0);

    QImage image2 = m_viewer->grabFramebuffer();
    QVERIFY(image1 != image2);

    // ShadeMode
    image1 = m_viewer->grabFramebuffer();

    s->setShadeMode(Q3DSViewerSettings::ShadeModeShadedWireframe);
    QCOMPARE(s->shadeMode(), Q3DSViewerSettings::ShadeModeShadedWireframe);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(spy4.count(), 0);

    image2 = m_viewer->grabFramebuffer();
    QVERIFY(image1 != image2);

    // Restore shade mode so following tests are not affected by wireframes
    s->setShadeMode(Q3DSViewerSettings::ShadeModeShaded);

    // ScaleMode
    checkPixel(m_viewer, Qt::cyan);
    checkPixel(m_viewer, Qt::cyan, QPoint(50, height / 2));
    s->setScaleMode(Q3DSViewerSettings::ScaleModeFit);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFit);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 1);

    checkPixel(m_viewer, Qt::cyan);
    checkPixel(m_viewer, Qt::red, QPoint(50, height / 2));

    s->setScaleMode(Q3DSViewerSettings::ScaleModeFill);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFill);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);

    checkPixel(m_viewer, Qt::blue);
    checkPixel(m_viewer, Qt::red, QPoint(50, height / 2));

    // Saving & loading settings
    s->save(QStringLiteral("testViewer"), QStringLiteral("The Qt Company"),
            QStringLiteral("tst_q3dsurfaceviewer"));

    image1 = m_viewer->grabFramebuffer();

    s->setMatteColor(Qt::yellow);
    s->setShowRenderStats(false);
    s->setShadeMode(Q3DSViewerSettings::ShadeModeShadedWireframe);
    s->setScaleMode(Q3DSViewerSettings::ScaleModeFit);

    QCOMPARE(s->matteColor(), QColor(Qt::yellow));
    QCOMPARE(s->isShowRenderStats(), false);
    QCOMPARE(s->shadeMode(), Q3DSViewerSettings::ShadeModeShadedWireframe);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFit);

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 3);
    QCOMPARE(spy4.count(), 3);

    image2 = m_viewer->grabFramebuffer();

    s->load(QStringLiteral("testViewer"), QStringLiteral("The Qt Company"),
            QStringLiteral("tst_q3dsurfaceviewer"));

    QCOMPARE(s->matteColor(), QColor(Qt::cyan));
    QCOMPARE(s->isShowRenderStats(), true);
    QCOMPARE(s->shadeMode(), Q3DSViewerSettings::ShadeModeShaded);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFill);

    QCOMPARE(spy1.count(), 3);
    QCOMPARE(spy2.count(), 3);
    QCOMPARE(spy3.count(), 4);
    QCOMPARE(spy4.count(), 4);

    QImage image3 = m_viewer->grabFramebuffer();
    QVERIFY(image1 != image2);
    QVERIFY(image3 != image2);
    QVERIFY(image1 == image3);

    // Clean up the settings so they don't pollute the system (and we have clean slate next time)
    QSettings(QStringLiteral("The Qt Company"), QStringLiteral("tst_q3dwidget")).clear();
}

void tst_Q3DSWidget::testPresentation()
{
    createWindowAndViewer(m_viewer, MULTISLIDE);

    m_viewer->setUpdateBehavior(QOpenGLWidget::PartialUpdate);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QList<QVariant> args;
    Q3DSPresentation *p = m_viewer->presentation();
    QSignalSpy spy1(p, &Q3DSPresentation::slideEntered);
    QSignalSpy spy2(p, &Q3DSPresentation::slideExited);
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());

    // There are three different "scenes":
    // The main Scene, three slides: S1, S2, S3
    // Two components on Scene.Layer:
    //   Scene.Layer.Component1, two slides: C1S1, C1S2
    //   Scene.Layer.Component2, two slides: C2S1, C2S2
    // The component slides also get enter when parent slide is entered, but they do not get
    // the corresponding exit if parent slide is exited.
    QString path = QStringLiteral("Scene");
    QString pathC1 = QStringLiteral("Scene.Layer.Component1");
    QString pathC2 = QStringLiteral("Scene.Layer.Component2");
    QPoint mainPoint(m_viewer->size().width() * 2 / 8, m_viewer->size().height() / 2);
    QPoint bgPoint(m_viewer->size().width() * 2 / 8, m_viewer->size().height() / 32);
    QPoint c1Point(m_viewer->size().width() * 5 / 8, m_viewer->size().height() / 2);
    QPoint c2Point(m_viewer->size().width() * 7 / 8, m_viewer->size().height() / 2);

    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    QCOMPARE(spy1.count(), 3);
    QCOMPARE(spy2.count(), 0);

    // String Attribute
    QImage image1 = m_viewer->grabFramebuffer();
    p->setAttribute(QStringLiteral("Scene.Layer.Text"),
                    QStringLiteral("textstring"), QStringLiteral("Test!"));
    QImage image2 = m_viewer->grabFramebuffer();
    QTRY_VERIFY(image1 != image2);

    // Float Attribute
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.r"), 0.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.g"), 1.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.b"), 1.0);

    checkPixel(m_viewer, Qt::cyan, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.r"), 1.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.g"), 0.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.b"), 0.0);

    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    // Bool Attribute
    checkPixel(m_viewer, Qt::yellow, bgPoint);
    p->setAttribute(QStringLiteral("Scene"), QStringLiteral("bgcolorenable"), false);
    checkPixel(m_viewer, Qt::black, bgPoint);

    // Slide changes
    p->goToSlide(path, 2);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::blue, c1Point);
    checkPixel(m_viewer, Qt::blue, c2Point);

    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 1);
    args = spy1.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S2"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S1"));

    // Time change
    p->goToTime(path, 7);
    checkPixel(m_viewer, Qt::black, mainPoint);
    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 1);

    // More complex slide changes
    // Changing slide that is not visible should not trigger enter signals
    // The slides should still change, though, and become visible later when we switch back to S1
    p->goToSlide(pathC1, QStringLiteral("C1S2"));
    p->goToSlide(pathC2, QStringLiteral("C2S2"));
    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 1);

    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::cyan, c1Point);
    checkPixel(m_viewer, Qt::magenta, c2Point);
    QCOMPARE(spy1.count(), 7);
    QCOMPARE(spy2.count(), 2);

    p->goToSlide(pathC1, QStringLiteral("C1S1"));
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::magenta, c2Point);
    QCOMPARE(spy1.count(), 8);
    QCOMPARE(spy2.count(), 3);

    args = spy1.last();
    QCOMPARE(args.at(0).toString(), pathC1);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C1S1"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), pathC1);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C1S2"));

    p->goToSlide(pathC2, QStringLiteral("C2S1"));
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 9);
    QCOMPARE(spy2.count(), 4);

    args = spy1.last();
    QCOMPARE(args.at(0).toString(), pathC2);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C2S1"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), pathC2);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C2S2"));

    p->goToSlide(path, true, true);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::blue, c1Point);
    checkPixel(m_viewer, Qt::blue, c2Point);
    QCOMPARE(spy1.count(), 10);
    QCOMPARE(spy2.count(), 5);

    args = spy1.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S2"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S1"));

    p->goToSlide(path, false, true);
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 13);
    QCOMPARE(spy2.count(), 6);

    // No wrap, should not change
    p->goToSlide(path, false, false);
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 13);
    QCOMPARE(spy2.count(), 6);

    // Should wrap
    p->goToSlide(path, false, true);
    checkPixel(m_viewer, Qt::green, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::green, c2Point);
    QCOMPARE(spy1.count(), 14);
    QCOMPARE(spy2.count(), 7);

    // No wrap, should not change
    p->goToSlide(path, true, false);
    checkPixel(m_viewer, Qt::green, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::green, c2Point);
    QCOMPARE(spy1.count(), 14);
    QCOMPARE(spy2.count(), 7);

    // Should wrap
    p->goToSlide(path, true, true);
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 17);
    QCOMPARE(spy2.count(), 8);
}

void tst_Q3DSWidget::testPresentationActivation()
{
    createWindowAndViewer(m_viewer, ANIMATION);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    {
        // Grab two images two seconds apart to verify animation is happening
        QImage image1 = m_viewer->grabFramebuffer();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grabFramebuffer();
        QVERIFY(image1 != image2);
    }

    m_viewer->presentation()->setPresentationActive(QStringLiteral("animation"), false);

    {
        // Grab two images two seconds apart to verify animation has stopped
        QImage image1 = m_viewer->grabFramebuffer();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grabFramebuffer();
        QVERIFY(image1 == image2);
    }

    m_viewer->presentation()->setPresentationActive(QStringLiteral("animation"), true);

    {
        // Grab two images two seconds apart to verify animation is happening
        QImage image1 = m_viewer->grabFramebuffer();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grabFramebuffer();
        QVERIFY(image1 != image2);
    }
}

void tst_Q3DSWidget::testScene()
{
    createWindowAndViewer(m_viewer, MULTISLIDE);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QString path = QStringLiteral("Scene");
    QString pathC1 = QStringLiteral("Scene.Layer.Component1");
    QString pathC2 = QStringLiteral("Scene.Layer.Component2");

    Q3DSPresentation *p = m_viewer->presentation();
    Q3DSSceneElement *scene = new Q3DSSceneElement(path);
    Q3DSSceneElement *sceneC1 = new Q3DSSceneElement(pathC1);
    Q3DSSceneElement *sceneC2 = new Q3DSSceneElement(pathC2);
    QSignalSpy spy1(scene, &Q3DSSceneElement::currentSlideIndexChanged);
    QSignalSpy spy2(scene, &Q3DSSceneElement::previousSlideIndexChanged);
    QSignalSpy spy3(scene, &Q3DSSceneElement::currentSlideNameChanged);
    QSignalSpy spy4(scene, &Q3DSSceneElement::previousSlideNameChanged);
    QSignalSpy spy5(scene, &Q3DSSceneElement::elementPathChanged);
    QSignalSpy spy6(sceneC1, &Q3DSSceneElement::currentSlideIndexChanged);
    QSignalSpy spy7(sceneC1, &Q3DSSceneElement::previousSlideIndexChanged);
    QSignalSpy spy8(sceneC1, &Q3DSSceneElement::currentSlideNameChanged);
    QSignalSpy spy9(sceneC1, &Q3DSSceneElement::previousSlideNameChanged);
    QSignalSpy spy10(sceneC1, &Q3DSSceneElement::elementPathChanged);
    QSignalSpy spy11(sceneC2, &Q3DSSceneElement::currentSlideIndexChanged);
    QSignalSpy spy12(sceneC2, &Q3DSSceneElement::previousSlideIndexChanged);
    QSignalSpy spy13(sceneC2, &Q3DSSceneElement::currentSlideNameChanged);
    QSignalSpy spy14(sceneC2, &Q3DSSceneElement::previousSlideNameChanged);
    QSignalSpy spy15(sceneC2, &Q3DSSceneElement::elementPathChanged);
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());
    QVERIFY(spy3.isValid());
    QVERIFY(spy4.isValid());
    QVERIFY(spy5.isValid());
    QVERIFY(spy6.isValid());
    QVERIFY(spy7.isValid());
    QVERIFY(spy8.isValid());
    QVERIFY(spy9.isValid());
    QVERIFY(spy10.isValid());
    QVERIFY(spy11.isValid());
    QVERIFY(spy12.isValid());
    QVERIFY(spy13.isValid());
    QVERIFY(spy14.isValid());
    QVERIFY(spy15.isValid());

    // Defaults
    QCOMPARE(scene->currentSlideIndex(), 0);
    QCOMPARE(scene->previousSlideIndex(), 0);
    QCOMPARE(scene->currentSlideName(), QStringLiteral(""));
    QCOMPARE(scene->previousSlideName(), QStringLiteral(""));
    QCOMPARE(scene->elementPath(), path);

    checkPixel(m_viewer, Qt::red);

    // Ensure the first slide enter events come before we register so they don't muddle the waters
    QGuiApplication::processEvents();

    p->registerElement(scene);
    p->registerElement(sceneC1);
    p->registerElement(sceneC2);

    QCOMPARE(scene->currentSlideIndex(), 1);
    QCOMPARE(scene->previousSlideIndex(), 0);
    QCOMPARE(scene->currentSlideName(), QStringLiteral("S1"));
    QCOMPARE(scene->previousSlideName(), QStringLiteral(""));
    QCOMPARE(scene->elementPath(), path);

    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(spy4.count(), 1);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 0);
    QCOMPARE(spy7.count(), 0);
    QCOMPARE(spy8.count(), 0);
    QCOMPARE(spy9.count(), 0);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 0);
    QCOMPARE(spy12.count(), 0);
    QCOMPARE(spy13.count(), 0);
    QCOMPARE(spy14.count(), 0);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 0);
    // Getting previous slide change without getting current slide change seems illogical here,
    // but that's how the internal viewer logic for previous slide works. It makes sense when
    // you consider the fact that we always get enter events for child slides when parent slide
    // is entered.
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 0);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 0);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 0);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(pathC1, QStringLiteral("C1S2"));
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 0);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 0);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(pathC2, QStringLiteral("C2S2"));
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 1);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 1);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    // Subscenes revert to original slides when parent is re-entered
    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 3);
    QCOMPARE(spy2.count(), 3);
    QCOMPARE(spy3.count(), 3);
    QCOMPARE(spy4.count(), 3);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 1);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 1);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 4);
    QCOMPARE(spy3.count(), 4);
    QCOMPARE(spy4.count(), 4);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 2);
    QCOMPARE(spy7.count(), 2);
    QCOMPARE(spy8.count(), 2);
    QCOMPARE(spy9.count(), 2);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 2);
    QCOMPARE(spy12.count(), 2);
    QCOMPARE(spy13.count(), 2);
    QCOMPARE(spy14.count(), 2);
    QCOMPARE(spy15.count(), 0);

    p->unregisterElement(scene);
    p->unregisterElement(sceneC1);
    p->unregisterElement(sceneC2);

    // No more signals after unregistering
    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 4);
    QCOMPARE(spy3.count(), 4);
    QCOMPARE(spy4.count(), 4);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 2);
    QCOMPARE(spy7.count(), 2);
    QCOMPARE(spy8.count(), 2);
    QCOMPARE(spy9.count(), 2);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 2);
    QCOMPARE(spy12.count(), 2);
    QCOMPARE(spy13.count(), 2);
    QCOMPARE(spy14.count(), 2);
    QCOMPARE(spy15.count(), 0);

    // Reregister
    p->registerElement(scene);
    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 5);
    QCOMPARE(spy2.count(), 5);
    QCOMPARE(spy3.count(), 5);
    QCOMPARE(spy4.count(), 5);
    QCOMPARE(spy5.count(), 0);

    QCOMPARE(scene->currentSlideIndex(), 1);
    QCOMPARE(scene->previousSlideIndex(), 2);
    QCOMPARE(scene->currentSlideName(), QStringLiteral("S1"));
    QCOMPARE(scene->previousSlideName(), QStringLiteral("S2"));
    QCOMPARE(scene->elementPath(), path);

    // Change elementpath
    scene->setElementPath(pathC1);
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 5);
    QCOMPARE(spy2.count(), 5);
    QCOMPARE(spy3.count(), 5);
    QCOMPARE(spy4.count(), 5);
    QCOMPARE(spy5.count(), 1);

    QCOMPARE(scene->currentSlideIndex(), 1);
    QCOMPARE(scene->previousSlideIndex(), 1);
    // Having current and previous slides the same seems weird, but that's how the slide
    // logic works internally.
    QCOMPARE(scene->currentSlideName(), QStringLiteral("C1S1"));
    QCOMPARE(scene->previousSlideName(), QStringLiteral("C1S1"));
    QCOMPARE(scene->elementPath(), pathC1);

    p->goToSlide(pathC1, QStringLiteral("C1S2"));
    checkPixel(m_viewer, Qt::red);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 6);
    QCOMPARE(spy2.count(), 5);
    QCOMPARE(spy3.count(), 6);
    QCOMPARE(spy4.count(), 5);
    QCOMPARE(spy5.count(), 1);

    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);
    QGuiApplication::processEvents();

    QCOMPARE(spy1.count(), 6);
    QCOMPARE(spy2.count(), 5);
    QCOMPARE(spy3.count(), 6);
    QCOMPARE(spy4.count(), 5);
    QCOMPARE(spy5.count(), 1);
}

void tst_Q3DSWidget::testMouseInput()
{
    createWindowAndViewer(m_viewer, MOUSE);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QPoint point1(m_viewer->size().width() * 1 / 4, m_viewer->size().height() / 2);
    QPoint point2(m_viewer->size().width() * 3 / 4, m_viewer->size().height() / 2);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::red, point2);

    QMouseEvent e1(QEvent::MouseButtonPress, point1, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mousePressEvent(&e1);

    checkPixel(m_viewer, Qt::green, point1);
    checkPixel(m_viewer, Qt::red, point2);

    QMouseEvent e2(QEvent::MouseButtonRelease, point1, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mouseReleaseEvent(&e2);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::red, point2);

    QMouseEvent e3(QEvent::MouseButtonPress, point2, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mousePressEvent(&e3);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::blue, point2);

    QMouseEvent e4(QEvent::MouseButtonRelease, point2, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mouseReleaseEvent(&e4);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::red, point2);

    // Note: No way yet to hook mouse move into anything in the presentation
}

QTEST_MAIN(tst_Q3DSWidget)

#include "tst_q3dswidget.moc"
