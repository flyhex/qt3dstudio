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
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qwidget.h>
#include <QtGui/qcursor.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>
#include <QtCore/qtimer.h>
#include <QtGui/qevent.h>
#include <QtGui/qbitmap.h>

static void setBlankCursor()
{
    // Qt::BlankCursor gets corrupted in some situations, so use custom bitmap (QTBUG-61678)
    static QBitmap *zeroBitmap = nullptr;
    if (!zeroBitmap) {
        zeroBitmap = new QBitmap(32, 32);
        zeroBitmap->clear();
    }
    QGuiApplication::setOverrideCursor(QCursor(*zeroBitmap, *zeroBitmap));
}

MouseHelper::MouseHelper(QObject *parent)
    : QObject(parent)
    , m_dragState(StateNotDragging)
    , m_maxDelta(50, 50)
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
    setBlankCursor();
    m_startPos = QCursor::pos();

    QWindow *window = g_StudioApp.m_pMainWnd->windowHandle();
    if (window)
        window->installEventFilter(this); // Always install event filter to main window

    if (m_widget) {
        // Use the center of the on-screen portion of the parent widget as reference point.
        // This ensures cursor restores properly, as the cursor stays on the widget.
        m_window = m_widget->window()->windowHandle();
        const QRect screenGeometry = m_window->screen()->geometry();
        const QPoint bottomRight = screenGeometry.bottomRight();
        QSize widgetSize = m_widget->size();
        QPoint widgetPos = m_widget->mapToGlobal(QPoint(0, 0));
        if (widgetPos.x() < 0) {
            widgetSize.setWidth(widgetSize.width() + widgetPos.x());
            widgetPos.setX(0);
        }
        if (widgetPos.y() < 0) {
            widgetSize.setHeight(widgetSize.height() + widgetPos.y());
            widgetPos.setY(0);
        }
        if (widgetPos.x() + widgetSize.width() > bottomRight.x())
            widgetSize.setWidth(bottomRight.x() - widgetPos.x());
        if (widgetPos.y() + widgetSize.height() > bottomRight.y())
            widgetSize.setHeight(bottomRight.y() - widgetPos.y());
        m_maxDelta = QPoint(widgetSize.width() / 2, widgetSize.height() / 2);
        m_referencePoint = widgetPos + m_maxDelta;
    } else {
        // Just assume the screen of the app is at least 400x400 if we don't have widget
        m_referencePoint = QPoint(200, 200);
        m_window = nullptr;
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
        // In macOS we come here after QCursor::setPos has been called from resetCursor(), but
        // before the actual position has changed. This results in hude delta values, which makes
        // mouse drag adjustment on macOS unusable (QT3DS-3763).
        // This also breaks the cursor position resetting on macOS, but that's due to a bug in
        // Qt (QTBUG-33959)
        if (m_previousPoint != m_referencePoint) {
            delta = currentPoint - m_previousPoint;

            // Limit delta to even out the maximum possible change rate regardless of widget position
            if (delta.x() > m_maxDelta.x())
                delta.setX(m_maxDelta.x());
            else if (delta.x() < -m_maxDelta.x())
                delta.setX(-m_maxDelta.x());

            if (delta.y() > m_maxDelta.y())
                delta.setY(m_maxDelta.y());
            else if (delta.y() < -m_maxDelta.y())
                delta.setY(-m_maxDelta.y());

            if (!m_cursorResetTimer.isActive())
                m_cursorResetTimer.start();
        }
        m_previousPoint = currentPoint;
    }

    return delta;
}

void MouseHelper::setWidget(QWidget *widget)
{
     m_widget = widget;
}

void MouseHelper::resetCursor()
{
    switch (m_dragState) {
    case StateDragging:
        if (m_window)
            QCursor::setPos(m_window->screen(), m_referencePoint);
        else
            QCursor::setPos(m_referencePoint);
        m_previousPoint = m_referencePoint;
        break;
    case StateEndingDrag:
        if (m_window)
            QCursor::setPos(m_window->screen(), m_startPos);
        else
            QCursor::setPos(m_startPos);
        m_dragState = StateFinalCursorReset;
        m_cursorResetTimer.start();
        break;
    case StateFinalCursorReset:
        // First change to default cursor to avoid any flicker of cursor
        qApp->changeOverrideCursor(Qt::ArrowCursor);
        qApp->restoreOverrideCursor();
        m_dragState = StateNotDragging;
        break;
    case StateNotDragging:
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
