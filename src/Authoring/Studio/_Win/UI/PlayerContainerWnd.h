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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_PLAYER_CONTAINER_WND_H
#define INCLUDED_PLAYER_CONTAINER_WND_H 1

#pragma once

#include <QtWidgets/qscrollarea.h>

class CSceneView;
class CStudioApp;
class CPlayerWnd;
class CWGLRenderContext;

//==============================================================================
//	Class CPlayerContainerWnd
//==============================================================================

class CPlayerContainerWnd : public QScrollArea
{
    Q_OBJECT
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef enum {
        VIEW_EDIT = 0, ///< Edit View
        VIEW_SCENE, ///< Scene View
    } EViewMode;

public:
    CPlayerContainerWnd(CSceneView *inSceneView);
    virtual ~CPlayerContainerWnd();

    void SetPlayerWnd(CPlayerWnd *inPlayerWnd);
    void SetPlayerWndPosition(long &outLeftPresentationEdge, long &outTopPresentionEdge);
    void SetScrollRanges();
    void RecenterClient();
    void OnRulerGuideToggled();

    void SetViewMode(EViewMode inViewMode);
    EViewMode GetViewMode();
    bool IsDeploymentView();

    QRect GetDisplayedClientRect() const { return m_ClientRect; }
    bool IsMouseDown() const { return m_IsMouseDown; }
    bool IsMiddleMouseDown() const { return m_IsMiddleMouseDown; }

    QSize GetEffectivePresentationSize() const;

    qreal fixedDevicePixelRatio() const;

Q_SIGNALS:
    void toolChanged();

protected:
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

    void scrollContentsBy(int, int) override;

private:
    CPlayerContainerWnd() {}
    bool ShouldHideScrollBars();

protected:
    CSceneView *m_SceneView; ///< Pointer to the SceneView for rulers manipulation
    CPlayerWnd *m_PlayerWnd; ///< Pointer to the window control that contains client
    QRect m_ClientRect; ///< The rect where the client is drawn
    bool m_IsMouseDown; ///< true if the mouse (any button) is down
    bool m_IsMiddleMouseDown; ///< true if the middle mouse ( or scroll wheel ) is down
    EViewMode m_ViewMode;
};

#endif // INCLUDED_PLAYER_CONTAINER_WND_H
