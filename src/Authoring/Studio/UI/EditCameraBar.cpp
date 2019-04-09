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

#include "EditCameraBar.h"
#include "MainFrm.h"
#include "SceneView.h"
#include "StudioPreferences.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlistview.h>

CEditCameraBar::CEditCameraBar(QWidget *parent) : QToolBar(parent)
{
    initialize();

    connect(m_CameraSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CEditCameraBar::handleCameraChanged);
}

CEditCameraBar::~CEditCameraBar()
{
    delete m_CameraSelector;
}

/**
 *	Setup the list of edit cameras into the camera combo box
 *	@param inCameras	the container that holds the edit cameras
 */
void CEditCameraBar::setupCameras()
{
    m_CameraSelector->clear();
    Q3DStudio::IStudioRenderer &theRenderer = g_StudioApp.getRenderer();
    QStringList theCameraNames;
    theRenderer.GetEditCameraList(theCameraNames);
    m_CameraSelector->addItems(theCameraNames);
    m_CameraSelector->addItem(tr("Scene Camera View"));

    m_CameraSelector->insertSeparator(m_CameraSelector->count() - 1);

    // adding a 1px spacing, else the separator will disappear sometimes (QComboBox bug)
    qobject_cast<QListView *>(m_CameraSelector->view())->setSpacing(1);

    // set initial view
    int viewIndex = CStudioPreferences::GetPreferredStartupView();
      // if not set or invalid index, use the scene camera view (last index)
    if (viewIndex == -1 || viewIndex > m_CameraSelector->count() - 3)
        viewIndex = m_CameraSelector->count() - 1;

    m_CameraSelector->setCurrentIndex(viewIndex);
    handleCameraChanged(viewIndex);

    QString ctrlKey(QStringLiteral("Ctrl+"));
#ifdef Q_OS_MACOS
    ctrlKey = "⌘";
#endif

    m_CameraSelector->setToolTip(tr("Change Camera View (%1<1..9>)").arg(ctrlKey));
}

void CEditCameraBar::setCameraIndex(int inIndex)
{
    m_CameraSelector->setCurrentIndex(inIndex);
    handleCameraChanged(inIndex);
}

/**
 *	Handle the switching of the current edit camera
 *	@param inIndex	the index of the to-be-activated camera in the combo box
 */
void CEditCameraBar::handleCameraChanged(int inIndex)
{
    Q3DStudio::IStudioRenderer &theRenderer = g_StudioApp.getRenderer();

    // last index is scene camera view, renderer requires index -1 for it
    theRenderer.SetEditCamera(inIndex == m_CameraSelector->count() - 1 ? -1 : inIndex);

    if (m_SceneView)
        m_SceneView->onEditCameraChanged();

    // if the current tool is camera rotate and has been switch to 2d camera
    // set the tool to camera pan
    long theToolMode = g_StudioApp.GetToolMode();
    if (!theRenderer.DoesEditCameraSupportRotation(theRenderer.GetEditCamera())
            && theToolMode == STUDIO_TOOLMODE_CAMERA_ROTATE) {
        g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
        m_SceneView->setViewCursor(); // Just set cursor, we don't want to update previous tool
    }

    // Trigger for tool changed. Changing between deployment/edit camera can change the tool
    g_StudioApp.m_pMainWnd->OnUpdateToolChange();
}

/**
 *	Set the current scene view. This scene view is notified when there is a camera
 *	changed.
 *	@param inSceneView	the scene view object
 */
void CEditCameraBar::setSceneView(CSceneView *inSceneView)
{
    m_SceneView = inSceneView;
}

void CEditCameraBar::initialize()
{
    // Create the combo box
    addWidget(m_CameraSelector = new QComboBox);
#if (defined Q_OS_MACOS)
    // There is a "selected" icon in the popup, and automatic scaling does not work for some reason
    m_CameraSelector->setMinimumContentsLength(tr("Scene Camera View").length() + 1);
#else
    m_CameraSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
#endif
    // We need to specify accessibleName and objectName for the combobox, as it's in the toolbar,
    // and we want to use a different style for it.
    const QString objName(QStringLiteral("cameraSelector"));
    m_CameraSelector->setAccessibleName(objName);
    m_CameraSelector->setObjectName(objName);
}
