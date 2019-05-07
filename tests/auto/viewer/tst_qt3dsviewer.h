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

#ifndef TST_QT3DSVIEWER
#define TST_QT3DSVIEWER

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/QQuickView>
#include <QtStudio3D/q3dspresentation.h>
#include <QtStudio3D/q3dsviewersettings.h>

class tst_qt3dsviewer : public QObject
{
    Q_OBJECT
public:
    tst_qt3dsviewer()
    {
    }

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testEmpty();
    void testLoading();
    void testSlides();
    void testFrameUpdates();
    void testSettings();
    void testCreateElement();

private:
    QQuickView m_viewer;
    QObject *m_studio3DItem = nullptr;
    Q3DSPresentation *m_presentation = nullptr;
    Q3DSViewerSettings *m_settings = nullptr;
};

#endif // TST_QT3DSVIEWER
