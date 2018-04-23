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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "EditCameraBar.h"
#include "MainFrm.h"
#include "SceneView.h"
#include "StudioPreferences.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qabstractitemview.h>
#include <QtWidgets/qlistview.h>

//==============================================================================
/**
 *	Constructor
 */
CEditCameraBar::CEditCameraBar(QWidget* parent)
    : QToolBar(parent)
    , m_SceneView(nullptr)
    , m_ActiveCameraIndex(-1)
{
    OnCustomizeToolbar();
    auto activated = static_cast<void(QComboBox::*)(int)>(&QComboBox::activated);
    connect(m_CameraSelector, activated, this, &CEditCameraBar::OnCameraChanged);
}

//==============================================================================
/**
 *	Destructor
 */
CEditCameraBar::~CEditCameraBar()
{
    delete m_CameraSelector;
}

//==============================================================================
/**
 *	Setup the list of edit cameras into the camera combo box
 *	@param inCameras	the container that holds the edit cameras
 */
void CEditCameraBar::SetupCameras()
{
    m_CameraSelector->clear();
    Q3DStudio::IStudioRenderer &theRenderer = g_StudioApp.getRenderer();
    QStringList theCameraNames;
    theRenderer.GetEditCameraList(theCameraNames);
    int idx = 1;
    for (const QString &str : qAsConst(theCameraNames)) {
        m_CameraSelector->addItem(str, QVariant((int)idx));
        idx++;
    }

    m_CameraSelector->addItem(tr("Scene Camera View"));
    m_CameraSelector->setItemData(m_CameraSelector->count() - 1, 0);

    m_CameraSelector->insertSeparator(m_CameraSelector->count() - 1);
    // adding a 1px spacing, else the separator will disappear sometimes (QComboBox bug)
    qobject_cast<QListView*>(m_CameraSelector->view())->setSpacing(1);

    long thePreferredView = CStudioPreferences::GetPreferredStartupView();
    long theNumItems = m_CameraSelector->count();
    if (thePreferredView == -1) {
        // deployment view
        m_CameraSelector->setCurrentIndex(theNumItems - 1);
        HandleCameraChanged(theNumItems - 1); // set to the last one
    } else {
        int theIndex;
        if (thePreferredView < theNumItems - 2)
            theIndex = thePreferredView;
        else // possibly from old content where cameras are removed
            theIndex = 0;
        m_CameraSelector->setCurrentIndex(theIndex);
        HandleCameraChanged(theIndex);
    }
}

//==============================================================================
/**
 *	Callback method when the camera is changed from the camera selection combo box
 */
void CEditCameraBar::OnCameraChanged()
{
    HandleCameraChanged(m_CameraSelector->currentIndex());
}

void CEditCameraBar::setCameraIndex(int inIndex)
{
    m_CameraSelector->setCurrentIndex(inIndex);
    OnCameraChanged();
}

//==============================================================================
/**
 *	Handle the switching of the current edit camera
 *	@param inIndex	the index of the to-be-activated camera in the combo box
 */
void CEditCameraBar::HandleCameraChanged(int inIndex)
{
    Q3DStudio::IStudioRenderer &theRenderer = g_StudioApp.getRenderer();
    QStringList theCameraNames;
    theRenderer.GetEditCameraList(theCameraNames);
    int theNumCameras = theCameraNames.size();
    if (inIndex < theNumCameras) {
        theRenderer.SetEditCamera(inIndex);
        m_ActiveCameraIndex = inIndex;
        if (m_SceneView)
            m_SceneView->setViewMode(CPlayerContainerWnd::VIEW_EDIT);
    } else if (inIndex > theNumCameras) {
        theRenderer.SetEditCamera(-1);
        m_ActiveCameraIndex = inIndex;
        if (m_SceneView)
            m_SceneView->setViewMode(CPlayerContainerWnd::VIEW_SCENE);
    } else {
        m_CameraSelector->setCurrentIndex(m_ActiveCameraIndex);
    }
    if (m_SceneView)
        m_SceneView->onEditCameraChanged();

    CMainFrame *theMainFrame = g_StudioApp.m_pMainWnd;
    ASSERT(theMainFrame != nullptr);

    // if the current tool is camera rotate and has been switch to 2d camera
    // set the tool to camera pan
    if (theMainFrame != nullptr) {
        long theToolMode = g_StudioApp.GetToolMode();
        if (!theRenderer.DoesEditCameraSupportRotation(theRenderer.GetEditCamera())
                && theToolMode == STUDIO_TOOLMODE_CAMERA_ROTATE) {
            g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
            m_SceneView->setViewCursor(); // Just set cursor, we don't want to update previous tool
        }

        // Trigger for tool changed. Changing between deployment/edit camera can change the tool
        theMainFrame->OnUpdateToolChange();
    }
}

//==============================================================================
/**
 *	Set the current scene view. This scene view is notified when there is a camera
 *	changed.
 *	@param inSceneView	the scene view object
 */
void CEditCameraBar::SetSceneView(CSceneView *inSceneView)
{
    m_SceneView = inSceneView;
}

//==============================================================================
/**
 *	Enable/Disable the edit camera selector combo box.
 *	@param inFlag	true to enable the camera selector combo box, false to disable
 */
void CEditCameraBar::Enable(bool inFlag)
{
    m_CameraSelector->setEnabled(inFlag);
}

//==============================================================================
/**
 *	When the active camera is changed, the display string needs to be changed. Hence
 *	find which entry is the one which is modified and update with the new string
 *	@param inCamera	the camera that has been modified
 */
void CEditCameraBar::onEditCameraChanged()
{
    using qt3ds::QT3DSI32;
    QT3DSI32 cameraIndex = g_StudioApp.getRenderer().GetEditCamera();
    long theNumEntry = m_CameraSelector->count();
    for (long theIndex = 0; theIndex < theNumEntry; ++theIndex) {
        if (m_CameraSelector->itemData(theIndex).toInt() == cameraIndex) {
            if (theIndex != m_CameraSelector->currentIndex()) {
                m_CameraSelector->setCurrentIndex(theIndex);
                HandleCameraChanged(theIndex);
            }
            break;
        }
    }
}

//==============================================================================
/**
 *	Callback while creating the toolbar in MainFrm. This allows the toolbar to add
 *	other controls to it. For this toolbar, we want to add a descriptor and a camera
 *	selector dropdown combobox.
 */
//==============================================================================
void CEditCameraBar::OnCustomizeToolbar()
{
    // Create the combo box
    addWidget(m_CameraSelector = new QComboBox);
    // We need to specify accessibleName and objectName for the combobox, as it's in the toolbar,
    // and we want to use a different style for it.
    m_CameraSelector->setAccessibleName(QStringLiteral("cameraSelector"));
    m_CameraSelector->setObjectName(QStringLiteral("cameraSelector"));
    m_CameraSelector->setMinimumWidth(145);
}
