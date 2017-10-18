/****************************************************************************
**
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

#include "MouseHelper.h"
#include "MainFrm.h"
#include "StudioApp.h"
#include <QtWidgets/qapplication.h>
#include <QtGui/qcursor.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>
#include <QtWidgets/qmainwindow.h>
#include <QtCore/qtimer.h>

void MouseHelper::startUnboundedDrag()
{
    qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
    m_startPos = QCursor::pos();
    QWindow *window = g_StudioApp.m_pMainWnd->windowHandle();
    if (window) {
        QSize screenSize = window->screen()->size() / window->screen()->devicePixelRatio();
        m_referencePoint = QPoint(screenSize.width() / 2, screenSize.height() / 2);
    } else {
        // Just assume the screen of the app is at least 400x400 if we can't resolve window
        m_referencePoint = QPoint(200, 200);
    }
    QCursor::setPos(m_referencePoint);
}

void MouseHelper::endUnboundedDrag()
{
    QCursor::setPos(m_startPos);

    // If the referencePoint is above a control that sets override cursor, the cursor
    // of that control will flicker briefly when restoring the cursor. Work around that
    // by restoring the cursor asynchronously.
    QTimer::singleShot(0, [](){
        // First change to default cursor to avoid any remaining flicker of cursor
        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();
    });
}

QPoint MouseHelper::delta()
{
    QPoint delta = QCursor::pos() - m_referencePoint;

    // Limit delta to m_referencePoint size, so maximum delta to all directions is the same
    // even with multi-screen virtual desktops
    if (delta.x() > m_referencePoint.x())
        delta.setX(m_referencePoint.x());
    else if (delta.x() < -m_referencePoint.x())
        delta.setX(-m_referencePoint.x());

    if (delta.y() > m_referencePoint.y())
        delta.setY(m_referencePoint.y());
    else if (delta.y() < -m_referencePoint.y())
        delta.setY(-m_referencePoint.y());

    QCursor::setPos(m_referencePoint);
    return delta;
}

