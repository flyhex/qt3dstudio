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

#ifndef INCLUDED_PLAYER_CONTAINER_WND_H
#define INCLUDED_PLAYER_CONTAINER_WND_H 1

#pragma once

#include <QtWidgets/qscrollarea.h>

class CSceneView;
class CStudioApp;
namespace Q3DStudio {
class Q3DSPlayerWnd;
}
class CWGLRenderContext;

class CPlayerContainerWnd : public QScrollArea
{
    Q_OBJECT
public:
    typedef enum {
        VIEW_EDIT = 0, ///< Edit View
        VIEW_SCENE, ///< Scene View
    } EViewMode;

public:
    CPlayerContainerWnd(CSceneView *inSceneView);
    ~CPlayerContainerWnd() override;

    void SetPlayerWnd(Q3DStudio::Q3DSPlayerWnd *inPlayerWnd);
    void SetPlayerWndPosition(long &outLeftPresentationEdge, long &outTopPresentionEdge);
    void SetScrollRanges();
    void RecenterClient();
    void OnRulerGuideToggled();

    void SetViewMode(EViewMode inViewMode);
    EViewMode GetViewMode();
    bool IsDeploymentView();
    void setToolMode(long inMode);

    QRect GetDisplayedClientRect() const { return m_ClientRect; }

    QSize GetEffectivePresentationSize() const;

    qreal fixedDevicePixelRatio() const;

Q_SIGNALS:
    void toolChanged();

protected:
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void scrollContentsBy(int, int) override;

private:
    CPlayerContainerWnd() {}
    bool ShouldHideScrollBars();

protected:
    CSceneView *m_SceneView; ///< Pointer to the SceneView for rulers manipulation
    Q3DStudio::Q3DSPlayerWnd *m_PlayerWnd; ///< Pointer to the window control that contains client
    QRect m_ClientRect; ///< The rect where the client is drawn
    EViewMode m_ViewMode;
};

#endif // INCLUDED_PLAYER_CONTAINER_WND_H
