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
#ifndef MOUSEHELPER_H
#define MOUSEHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qtimer.h>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QWindow)

class MouseHelper : public QObject
{
    Q_OBJECT

public:
    explicit MouseHelper(QObject *parent = nullptr);
    ~MouseHelper() override {}

    Q_INVOKABLE void startUnboundedDrag();
    Q_INVOKABLE void endUnboundedDrag();
    Q_INVOKABLE QPoint delta();

    void setWidget(QWidget *widget);

private Q_SLOTS:
    void resetCursor();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QPoint m_startPos;
    QPoint m_referencePoint;
    QPoint m_previousPoint;
    QTimer m_cursorResetTimer;

    enum DragState {
        StateNotDragging,
        StateDragging,
        StateEndingDrag,
        StateFinalCursorReset
    };
    DragState m_dragState;
    QWidget *m_widget = nullptr; // Not owned
    QWindow *m_window = nullptr; // Not owned
    QPoint m_maxDelta;
};

#endif // MOUSEHELPER_H
