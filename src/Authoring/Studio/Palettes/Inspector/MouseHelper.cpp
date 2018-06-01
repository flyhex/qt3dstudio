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
#include <QtGui/qevent.h>

MouseHelper::MouseHelper(QObject *parent)
    : QObject(parent)
    , m_dragState(StateNotDragging)
{
    // All cursor position modifications are done asynchronously, so we don't get position
    // changes in middle of mouse event handling.
    m_cursorResetTimer.setInterval(0);
    m_cursorResetTimer.setSingleShot(true);
    connect(&m_cursorResetTimer, &QTimer::timeout, this, &MouseHelper::resetCursor);
}

void MouseHelper::startUnboundedDrag()
{
    m_dragState = StateDragging;
    qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
    m_startPos = QCursor::pos();

    QWindow *window = g_StudioApp.m_pMainWnd->windowHandle();
    if (window) {
        window->installEventFilter(this);
        QSize screenSize = window->screen()->size() / window->screen()->devicePixelRatio();
        m_referencePoint = QPoint(screenSize.width() / 2, screenSize.height() / 2);
    } else {
        // Just assume the screen of the app is at least 400x400 if we can't resolve window
        m_referencePoint = QPoint(200, 200);
    }
    m_previousPoint = m_startPos;

    m_cursorResetTimer.start();
}

void MouseHelper::endUnboundedDrag()
{
    QWindow *window = g_StudioApp.m_pMainWnd->windowHandle();
    if (window)
        window->removeEventFilter(this);
    m_dragState = StateEndingDrag;
    m_cursorResetTimer.start();
}

QPoint MouseHelper::delta()
{
    QPoint delta(0, 0);
    if (m_dragState == StateDragging) {
        QPoint currentPoint = QCursor::pos();
        delta = currentPoint - m_previousPoint;
        m_previousPoint = currentPoint;

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

        if (!m_cursorResetTimer.isActive())
            m_cursorResetTimer.start();
    }
    return delta;
}

void MouseHelper::resetCursor()
{
    switch (m_dragState) {
    case StateDragging:
        QCursor::setPos(m_referencePoint);
        m_previousPoint = m_referencePoint;
        break;
    case StateEndingDrag:
        QCursor::setPos(m_startPos);
        // First change to default cursor to avoid any flicker of cursor
        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();
        break;
    case StateNotDragging:
    default:
        break;
    }
}

bool MouseHelper::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)

    // Eat all mouse button events that are not for left button and all key events
    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
            return false;
        else
            return true;
    }
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        return true;
    default:
        break;
    }
    return false;
}

