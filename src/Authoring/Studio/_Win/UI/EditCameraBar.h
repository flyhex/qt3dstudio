/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
#ifndef INCLUDED_EDIT_CAMERA_BAR
#define INCLUDED_EDIT_CAMERA_BAR 1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//==============================================================================
//	Includes
//==============================================================================
#include "DispatchListeners.h"
#include <vector>

#include <QtWidgets/qtoolbar.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
QT_END_NAMESPACE

//==============================================================================
//	Forwards
//==============================================================================
class CSceneView;

//==============================================================================
/**
 *	@class CEditCameraBar
 */
class CEditCameraBar : public QToolBar, public CEditCameraChangeListener
{
    Q_OBJECT
public:
    CEditCameraBar(QWidget* parent = nullptr); // standard constructor
    virtual ~CEditCameraBar();

    // Implementation
    // Generated message map functions
    void OnCameraChanged();
    void OnCustomizeToolbar();

public:
    void SetupCameras();
    void SetSceneView(CSceneView *inSceneView);
    void Enable(bool inFlag);
    void setCameraIndex(int inIndex);
    void HandleCameraChanged(int inIndex);

    // CEditCameraChangeListener
    void OnEditCameraChanged() override;
    void OnEditCamerasTransformed() override {}
    void OnAuthorZoomChanged() override {}

protected:
    CSceneView *m_SceneView; ///< The scene view object
    long m_ActiveCameraIndex; ///< The index of the active camera in the list

    // UI Controls definition
    QComboBox* m_CameraSelector; ///< Combo box for selecting edit camera
};

#endif
