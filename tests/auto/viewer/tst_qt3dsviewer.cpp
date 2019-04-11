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
    QVERIFY(m_studio3DItem);
    m_presentation = static_cast<Q3DSPresentation *>(m_studio3DItem->children().at(0));
    QVERIFY(m_presentation);
    m_settings = static_cast<Q3DSViewerSettings *>(m_studio3DItem->children().at(1));
    QVERIFY(m_settings);
}

void tst_qt3dsviewer::cleanup()
{
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

QTEST_MAIN(tst_qt3dsviewer)
