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

#include "Q3DSPlayerWnd.h"
#include "DispatchListeners.h"

#include <QtWidgets/qwidget.h>
#include <QtGui/qcursor.h>

class CHotKeys;

class CSceneView : public QWidget,
                   public CEditCameraChangeListener
{
    Q_OBJECT
protected:
    QScopedPointer<Q3DStudio::Q3DSPlayerWnd> m_playerWnd; // second-level child (grandchild)
    QCursor m_arrowCursor; // A pointer to the current cursor (changes according to mode)
    QCursor m_cursorGroupMove; // The move group cursor
    QCursor m_cursorGroupRotate; // The rotate group cursor
    QCursor m_cursorGroupScale; // The scale group cursor
    QCursor m_cursorItemMove; // The move item cursor
    QCursor m_cursorItemRotate; // The rotate item cursor
    QCursor m_cursorItemScale; // The scale item cursor
    QCursor m_cursorEditCameraPan; // The edit camera pan cursor
    QCursor m_cursorEditCameraRotate; // The edit camera rotate cursor
    QCursor m_cursorEditCameraZoom; // The edit camera zoom cursor

    long m_previousSelectMode; // The previous select mode

public:
    CSceneView(QWidget *parent = nullptr);
    CSceneView(); // used for serialization only!
    virtual ~CSceneView();

    void setViewCursor();
    void recheckSizingMode();

    void setPlayerWndPosition();

    // redirect to/from PlayerContainerWnd
    bool isDeploymentView();
    void setViewMode(Q3DStudio::Q3DSPlayerWnd::EViewMode inViewMode);
    void setToolMode(long inMode);

    void onRulerGuideToggled();

    // CEditCameraChangeListener
    void onEditCameraChanged() override;
    void onEditCamerasTransformed() override {}
    void onAuthorZoomChanged() override;

    QSize sizeHint() const override;

    void onToolGroupSelection();
    void onToolItemSelection();

    Q3DStudio::Q3DSPlayerWnd *getPlayerWnd() const;

Q_SIGNALS:
    void toolChanged();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void onDropReceived();
};

#endif // INCLUDED_SCENE_VIEW_H
