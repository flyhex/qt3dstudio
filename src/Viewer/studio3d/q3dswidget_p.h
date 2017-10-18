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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtStudio3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef Q3DSWIDGET_P_H
#define Q3DSWIDGET_P_H

#include "q3dswidget.h"
#include "Qt3DSViewerApp.h"

#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class Q3DSViewerSettings;
class Q3DSPresentation;

class Q_STUDIO3D_EXPORT Q3DSWidgetPrivate : public QObject
{
public:
    Q_OBJECT
    Q_DECLARE_PUBLIC(Q3DSWidget)

public:
    explicit Q3DSWidgetPrivate(Q3DSWidget *q);
    virtual ~Q3DSWidgetPrivate();

    void reset();
    void setUpdateInterval(int interval);
    bool initialize();

    Q3DSViewerSettings *settings();
    Q3DSPresentation *presentation();

private Q_SLOTS:
    void shutdown();

private:
    bool initializeRuntime();
    void releaseRuntime();
    void resetUpdateTimer();

    Q3DSWidget *q_ptr;

    Q3DSViewer::Q3DSViewerApp *m_viewerApp;
    QTimer *m_timer;
    int m_updateInterval;
    qreal m_pixelRatio;

    Q3DSViewerSettings *m_settings;
    Q3DSPresentation *m_presentation;
};

QT_END_NAMESPACE

#endif // Q3DSWIDGET_P_H
