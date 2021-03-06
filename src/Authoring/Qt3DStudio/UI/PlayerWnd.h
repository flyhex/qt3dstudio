/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

#ifndef INCLUDED_PLAYERWND_H
#define INCLUDED_PLAYERWND_H 1

#pragma once

#include "DropContainer.h"
#include "PlayerContainerWnd.h"

#include <QtWidgets/qopenglwidget.h>

class CPlayerContainerWnd;
class CStudioApp;
class CMouseCursor;
class CHotkeys;

class CPlayerWnd : public QOpenGLWidget, public CWinDropContainer
{
    Q_OBJECT
public:
    explicit CPlayerWnd(QWidget *parent = nullptr);
    ~CPlayerWnd();

    void setContainerWnd(CPlayerContainerWnd *inSceneView);
    CPlayerContainerWnd *containerWnd() const { return m_containerWnd; }

    QSize sizeHint() const override;

    bool OnDragWithin(CDropSource &inSource) override;
    bool OnDragReceive(CDropSource &inSource) override;
    void OnDragLeave() override {}
    void OnReflectMouse(CPt &, Qt::KeyboardModifiers) override {}

    qreal fixedDevicePixelRatio() const;
    void setToolMode(long toolMode) { m_previousToolMode = toolMode; }

protected:

    CPlayerContainerWnd *m_containerWnd;
    bool m_mouseDown;
    bool m_resumePlayOnMouseRelease = false;
    long m_previousToolMode;

Q_SIGNALS:
    void dropReceived();
    void newFrame();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
};

#endif // INCLUDED_PLAYERWND_H
