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

#ifndef WIDGETCONTROL_H
#define WIDGETCONTROL_H

#include "Control.h"
#include "DropContainer.h"

#include <QtWidgets/qwidget.h>

class CRenderer;
class WidgetControl;

class WidgetControl : public QWidget, public CWinDropContainer
{
    Q_OBJECT
public:
    explicit WidgetControl(CControl *control, QWidget *parent = nullptr);
    virtual ~WidgetControl();
    void setContextMenuShown(bool shown) { m_isContextMenuShown = shown; }

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

public:
    QSize sizeHint() const override;
    CControl *getControl() const;

protected:
    void onDragEnter() override;
    bool OnDragWithin(CDropSource &inSource) override;
    bool OnDragReceive(CDropSource &inSource) override;
    void OnDragLeave() override;
    void OnReflectMouse(CPt &inPoint, Qt::KeyboardModifiers inFlags) override;

private:
    void setControlSize(const QSize &size);

    CControl *m_control;
    bool m_isLeftMouseDown = false;
    bool m_isContextMenuShown = false;
};

#endif // WIDGETCONTROL_H
