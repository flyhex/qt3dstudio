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
#ifndef INCLUDED_SCENE_VIEW_H
#define INCLUDED_SCENE_VIEW_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include <QtWidgets/qwidget.h>
#include <QtGui/qcursor.h>

#include "EditorPane.h"
#include "PlayerWnd.h"
#include "PlayerContainerWnd.h"
#include "DispatchListeners.h"

class CStudioApp;
class CHotKeys;

//==============================================================================
//	Class CSceneView
//==============================================================================

//==============================================================================
/**
 *	CSceneView: Scene View.
 *
 *	Provides the view of the scene and displays the 3D Client window.
 */
//==============================================================================
class CSceneView : public QWidget,
                   public CEditorPane,
                   public CSelectedNodePropChangeListener,
                   public CClientPlayChangeListener,
                   public CEditCameraChangeListener
{
    Q_OBJECT
    //==========================================================================
    //	Protected Properties
    //==========================================================================
protected:
    bool m_RegisteredKeyDown = false; ///< True if a key is down
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

    //==========================================================================
    //	Public Methods
    //==========================================================================
public:
    CSceneView(CStudioApp *inStudioApp, QWidget *parent = nullptr);
    CSceneView(); // used for serialization only!
    virtual ~CSceneView();

    bool HandleModifierUp(int inChar, int inRepCnt, Qt::KeyboardModifiers modifiers);
    bool HandleModifierDown(int inChar, int inRepCnt, Qt::KeyboardModifiers modifiers);
    bool PtInClientRect(const QPoint& inPoint);
    void SetViewCursor();
    void SetToolMode(long inMode);
    void RecheckSizingMode();

    void SetRegisteredKey(bool inStatus);
    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler);
    bool GetKeyStatus();

    void SetPlayerWndPosition();

    // redirect to/from PlayerContainerWnd
    bool IsDeploymentView();
    void SetViewMode(CPlayerContainerWnd::EViewMode inViewMode);

    // Tool helper methods
    void RestorePreviousTool();
    void SetToolOnCtrl();
    void SetToolOnAlt();

    void OnRulerGuideToggled();

    // CSelectedNodePropChangeListener
    void OnPropValueChanged(const Q3DStudio::CString &inPropertyName) override;
    void OnMouseDragged(bool inConstrainXAxis, bool inConstrainYAxis) override;

    // CClientPlayChangeListener
    void OnTimeChanged(long inTime) override;

    // CEditCameraChangeListener

    void OnEditCameraChanged() override;
    void OnEditCamerasTransformed() override;
    void OnAuthorZoomChanged() override;

    QSize sizeHint() const override;

    void OnToolGroupSelection();
    void OnToolItemSelection();
public:

    void resizeEvent(QResizeEvent *event) override;

Q_SIGNALS:
    void toolChanged();

protected:
    void RecalcMatte();

    // Generated message map functions
protected:
    void OnSetCursor();
    void showEvent(QShowEvent *event) override;

private:
    void onDropReceived();
};

#endif // INCLUDED_SCENE_VIEW_H
