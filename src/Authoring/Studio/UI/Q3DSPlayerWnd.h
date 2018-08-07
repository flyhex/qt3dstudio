/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef Q3DS_PLAYER_WND
#define Q3DS_PLAYER_WND

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "DropContainer.h"
#include "PlayerContainerWnd.h"

#include <QtWidgets/qopenglwidget.h>

class CPlayerContainerWnd;
class CStudioApp;
class CMouseCursor;
class CHotkeys;

namespace Q3DStudio {

class Q3DSPlayerWnd : public QOpenGLWidget, public CWinDropContainer
{
    Q_OBJECT
public:
    explicit Q3DSPlayerWnd(QWidget *parent = nullptr);
    ~Q3DSPlayerWnd() override;

    void setContainerWnd(CPlayerContainerWnd *inSceneView);

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

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
};

}

#endif // Q3DS_PLAYER_WND
