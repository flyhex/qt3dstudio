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

#ifndef INCLUDED_SCENE_VIEW_H
#define INCLUDED_SCENE_VIEW_H 1

#pragma once

#include "PlayerWnd.h"
#include "PlayerContainerWnd.h"
#include "DispatchListeners.h"

#include <QtWidgets/qwidget.h>
#include <QtGui/qcursor.h>

class CStudioApp;
class CHotKeys;

class CSceneView : public QWidget,
                   public CEditCameraChangeListener
{
    Q_OBJECT
protected:
    CPlayerContainerWnd *m_PlayerContainerWnd = nullptr; ///< first-level child
    CPlayerWnd *m_PlayerWnd = nullptr; ///< second-level child (grandchild)
    QCursor m_ArrowCursor; ///< A pointer to the current cursor (changes according to mode)
    QCursor m_CursorGroupMove; ///< The move group cursor
    QCursor m_CursorGroupRotate; ///< The rotate group cursor
    QCursor m_CursorGroupScale; ///< The scale group cursor
    QCursor m_CursorItemMove; ///< The move item cursor
    QCursor m_CursorItemRotate; ///< The rotate item cursor
    QCursor m_CursorItemScale; ///< The scale item cursor
    QCursor m_CursorEditCameraPan; ///< The edit camera pan cursor
    QCursor m_CursorEditCameraRotate; ///< The edit camera rotate cursor
    QCursor m_CursorEditCameraZoom; ///< The edit camera zoom cursor

    long m_PreviousToolMode; ///< The previous tool mode (used when toggling with hotkeys to switch
                             ///back to previous mode on release)
    long m_PreviousSelectMode; ///< The previous select mode

public:
    CSceneView(CStudioApp *inStudioApp, QWidget *parent = nullptr);
    CSceneView(); // used for serialization only!
    virtual ~CSceneView();

    bool HandleModifierUp(int inChar, int inRepCnt, Qt::KeyboardModifiers modifiers);
    bool HandleModifierDown(int inChar, int inRepCnt, Qt::KeyboardModifiers modifiers);
    void SetViewCursor();
    void SetToolMode(long inMode);
    void RecheckSizingMode();

    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler);

    void SetPlayerWndPosition();

    // redirect to/from PlayerContainerWnd
    bool IsDeploymentView();
    void SetViewMode(CPlayerContainerWnd::EViewMode inViewMode);

    // Tool helper methods
    void RestorePreviousTool();
    void SetToolOnCtrl();
    void SetToolOnAlt();

    void OnRulerGuideToggled();

    // CEditCameraChangeListener
    void OnEditCameraChanged() override;
    void OnEditCamerasTransformed() override {}
    void OnAuthorZoomChanged() override;

    QSize sizeHint() const override;

    void OnToolGroupSelection();
    void OnToolItemSelection();

    CPlayerWnd *GetPlayerWnd() const;

Q_SIGNALS:
    void toolChanged();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void RecalcMatte();
    void onDropReceived();
};

#endif // INCLUDED_SCENE_VIEW_H
