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

#ifndef INCLUDED_EDIT_CAMERA_BAR
#define INCLUDED_EDIT_CAMERA_BAR

#include <QtWidgets/qtoolbar.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class CSceneView;

class CEditCameraBar : public QToolBar
{
    Q_OBJECT

public:
    CEditCameraBar(QWidget *parent = nullptr);
    virtual ~CEditCameraBar() override;

    void setupCameras();
    void setSceneView(CSceneView *inSceneView);
    void setCameraIndex(int inIndex);

private:
    void initialize();
    void handleCameraChanged(int inIndex);

    CSceneView *m_SceneView = nullptr;
    QComboBox *m_CameraSelector = nullptr;
};

#endif
