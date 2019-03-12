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
#include "MainFrm.h"
#include "ui_MainFrm.h"

#include "StudioConst.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Bindings/TimelineTranslationManager.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "SceneView.h"
#include "StudioApp.h"
#include "IKeyframesManager.h"
#include "Dialogs.h"
#include "StudioPreferencesPropSheet.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "RecentItems.h"
#include "PaletteManager.h"
#include "Core.h"
#include "ITickTock.h"
#include "IStudioRenderer.h"
#include "DataInputListDlg.h"
#include "StudioTutorialWidget.h"
#include "remotedeploymentsender.h"
#include "InspectorControlView.h"
#include "TimelineWidget.h"
#include "ProjectView.h"
#include "RowTree.h"
#include "WidgetControl.h"
#include "SlideView.h"

#include <QtGui/qevent.h>
#include <QtGui/qdesktopservices.h>
#include <QtWidgets/qdockwidget.h>
#include <QtCore/qsettings.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>
#include <QtCore/qdir.h>
#include <QtCore/qprocess.h>
#include <QtGui/qfontdatabase.h>

// Constants
const long PLAYBACK_TIMER_TIMEOUT = 10; // 10 milliseconds

//==============================================================================
/**
 * Constructor
 */
CMainFrame::CMainFrame()
    : m_ui(new Ui::MainFrame)
    , m_remoteDeploymentSender(new RemoteDeploymentSender(this))
    , m_sceneView(nullptr)
    , m_recentItems(nullptr)
    , m_paletteManager(nullptr)
    , m_updateUITimer(new QTimer)
    , m_playbackTimer(new QTimer)
    , m_propSheet(nullptr)
{
    m_ui->setupUi(this);

    // Register TitilliumWeb as application font in case user doesn't have it already installed
    QFontDatabase::addApplicationFont(QStringLiteral(":/TitilliumWeb-Light.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/TitilliumWeb-Regular.ttf"));

    OnCreate();

    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddFileOpenListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddClientPlayChangeListener(this);
    g_StudioApp.setupTimer(WM_STUDIO_TIMER, this);

    // File Menu
    connect(m_ui->action_New_Project, &QAction::triggered, this, &CMainFrame::OnProjectNew);
    connect(m_ui->action_New_Presentation, &QAction::triggered, this, &CMainFrame::OnFileNew);
    connect(m_ui->action_Open, &QAction::triggered, this, &CMainFrame::OnFileOpen);
    connect(m_ui->action_Save, &QAction::triggered, this, &CMainFrame::OnFileSave);
    connect(m_ui->action_Save_Project_As, &QAction::triggered, this, &CMainFrame::onProjectSaveAs);
    connect(m_ui->action_Duplicate_Presentation, &QAction::triggered,
            this, &CMainFrame::onDuplicatePresentation);
    connect(m_ui->action_Revert, &QAction::triggered, this, &CMainFrame::OnFileRevert);
    connect(m_ui->actionImportAssets, &QAction::triggered, this, &CMainFrame::OnFileImportAssets);
    connect(m_ui->actionData_Inputs, &QAction::triggered, this, &CMainFrame::OnFileDataInputs);
    connect(m_ui->action_Connect_to_Device, &QAction::triggered, this,
            &CMainFrame::OnFileConnectToDevice);
    m_recentItems.reset(new CRecentItems(m_ui->menuRecent_Projects));
    connect(m_recentItems.data(), &CRecentItems::openRecent, this, &CMainFrame::OnFileOpenRecent);
    connect(m_ui->action_Exit, &QAction::triggered, this, &CMainFrame::close);

    // Edit Menu
    connect(m_ui->action_Undo, &QAction::triggered, this, &CMainFrame::OnEditUndo);
    connect(m_ui->action_Redo, &QAction::triggered, this, &CMainFrame::OnEditRedo);
//    connect(m_ui->actionRepeat, &QAction::triggered, this, &CMainFrame::onEditRepeat); // TODO: Implement
    connect(m_ui->action_Cut, &QAction::triggered, this, &CMainFrame::OnEditCut);
    connect(m_ui->action_Copy, &QAction::triggered, this, &CMainFrame::OnEditCopy);
    connect(m_ui->action_Paste, &QAction::triggered, this, &CMainFrame::OnEditPaste);
    connect(m_ui->actionPaste_to_Master_Slide, &QAction::triggered,
            this, &CMainFrame::onEditPasteToMaster);
    connect(m_ui->action_Duplicate_Object, &QAction::triggered, this, &CMainFrame::OnEditDuplicate);
    connect(m_ui->actionDelete, &QAction::triggered, this, &CMainFrame::onEditDelete);
    connect(m_ui->actionGroup, &QAction::triggered, this, &CMainFrame::onEditGroup);
//    connect(m_ui->actionParent, &QAction::triggered, this, &CMainFrame::onEditParent); // TODO: Implement
//    connect(m_ui->actionUnparent, &QAction::triggered, this, &CMainFrame::onEditUnparent); // TODO: Implement
    connect(m_ui->actionStudio_Preferences, &QAction::triggered,
            this, &CMainFrame::OnEditApplicationPreferences);
    connect(m_ui->actionPresentation_Settings, &QAction::triggered,
            this, &CMainFrame::OnEditPresentationPreferences);
    connect(m_ui->menu_Edit, &QMenu::aboutToShow, [this]() {
        // macOS doesn't block menubar while startup dialog is being shown, and that causes a
        // crash on aboutToShow if it's called before everything is set
        if (m_ui->menu_Edit->isEnabled()) {
            QString type = g_StudioApp.getDuplicateType();
            QString label = tr("Duplicate %1").arg(type);
            m_ui->action_Duplicate_Object->setText(label);
            m_ui->action_Duplicate_Object->setEnabled(!type.isEmpty());

            type = g_StudioApp.getDeleteType();
            label = tr("Delete %1").arg(type);
            m_ui->actionDelete->setText(label);
            m_ui->actionDelete->setEnabled(!type.isEmpty());

            if (g_StudioApp.canUngroupSelectedObjects()) {
                m_ui->actionGroup->setText(tr("Ungroup Objects"));
                m_ui->actionGroup->setEnabled(true);
            } else {
                m_ui->actionGroup->setText(tr("Group Objects"));
                m_ui->actionGroup->setEnabled(g_StudioApp.canGroupSelectedObjects());
            }
        }
    });
    connect(m_ui->menu_Edit, &QMenu::aboutToHide, [this]() {
        // Enable potentially disabled items so hotkeys will work
        m_ui->action_Duplicate_Object->setEnabled(true);
        m_ui->actionDelete->setEnabled(true);
        m_ui->actionGroup->setEnabled(true);
    });

    // View Menu
    connect(m_ui->actionReset_layout, &QAction::triggered, this, &CMainFrame::onViewResetLayout);
    connect(m_ui->actionFit_Selected, &QAction::triggered,
            this, &CMainFrame::OnEditCameraZoomExtent);
//    connect(m_ui->actionFit_all, &QAction::triggered, this, &CMainFrame::onViewFitAll); // TODO: Implement
    connect(m_ui->actionToggle_hide_unhide_selected, &QAction::triggered,
            []() { g_StudioApp.toggleEyeball(); });
//    connect(m_ui->actionToggle_hide_unhide_unselected, &QAction::triggered,
//            []() {  }); // TODO: Implement?
    connect(m_ui->actionAction, &QAction::triggered, this, &CMainFrame::OnViewAction);
    connect(m_ui->actionBasic_Objects, &QAction::triggered, this, &CMainFrame::OnViewBasicObjects);
    connect(m_ui->actionInspector, &QAction::triggered, this, &CMainFrame::OnViewInspector);
    connect(m_ui->actionProject, &QAction::triggered, this, &CMainFrame::OnViewProject);
    connect(m_ui->actionSlide, &QAction::triggered, this, &CMainFrame::OnViewSlide);
    connect(m_ui->actionTimeline, &QAction::triggered, this, &CMainFrame::OnViewTimeline);
    connect(m_ui->actionSceneCamera, &QAction::triggered, this, &CMainFrame::onViewSceneCamera);
    connect(m_ui->actionBounding_Boxes, &QAction::triggered,
            this, &CMainFrame::OnViewBoundingBoxes);
    connect(m_ui->actionPivot_Point, &QAction::triggered, this, &CMainFrame::OnViewPivotPoint);
    connect(m_ui->actionWireframe, &QAction::triggered, this, &CMainFrame::OnViewWireframe);
    connect(m_ui->actionTooltips, &QAction::triggered, this, &CMainFrame::OnViewTooltips);
    connect(m_ui->actionCamera_Preview, &QAction::triggered, this, &CMainFrame::OnShowEditPreview);
    connect(m_ui->actionEdit_Lighting, &QAction::triggered, this,
            &CMainFrame::OnEditViewLightingEnabled);
//    connect(m_ui->actionFind, &QAction::triggered, this, &CMainFrame::onViewFind); // TODO: Implement

    // Timeline Menu
    connect(m_ui->actionSet_Changed_Keyframes, &QAction::triggered,
            this, &CMainFrame::OnTimelineSetChangedKeyframe);
    connect(m_ui->actionDelete_Selected_Keyframe_s, &QAction::triggered,
            [](){ g_StudioApp.DeleteSelectedKeys(); });
    connect(m_ui->actionSet_Interpolation, &QAction::triggered,
            this, &CMainFrame::OnTimelineSetInterpolation);
    connect(m_ui->actionChange_Time_Bar_Color, &QAction::triggered,
            this, &CMainFrame::OnTimelineSetTimeBarColor);
    connect(m_ui->actionAutoset_Keyframes, &QAction::triggered,
            this, &CMainFrame::OnToolAutosetkeys);

    // Help Menu
    connect(m_ui->action_Reference_Manual, &QAction::triggered, this, &CMainFrame::OnHelpIndex);
    connect(m_ui->action_Visit_Qt_Web_Site, &QAction::triggered, this, &CMainFrame::OnHelpVisitQt);
    connect(m_ui->action_About_Qt_3D_Studio, &QAction::triggered,
            []() { g_StudioApp.onAppAbout(); });
    connect(m_ui->action_Open_Tutorial, &QAction::triggered, this, &CMainFrame::OnHelpOpenTutorial);

    // Selection toolbar
    connect(m_ui->actionItem_Select_Tool, &QAction::triggered,
            m_sceneView.data(), &CSceneView::onToolItemSelection);
    connect(m_ui->actionGroup_Select_Tool, &QAction::triggered,
            m_sceneView.data(), &CSceneView::onToolGroupSelection);

    // Playback toolbar
    connect(m_ui->actionPreview, &QAction::triggered,
            this, &CMainFrame::OnPlaybackPreviewRuntime2);

    connect(m_ui->actionRemote_Preview, &QAction::triggered,
            this, &CMainFrame::OnPlaybackPreviewRemote);

    // Tool mode toolbar
    connect(m_ui->actionPosition_Tool, &QAction::triggered, this, &CMainFrame::OnToolMove);
    connect(m_ui->actionRotation_Tool, &QAction::triggered, this, &CMainFrame::OnToolRotate);
    connect(m_ui->actionScale_Tool, &QAction::triggered, this, &CMainFrame::OnToolScale);
    connect(m_ui->actionLocal_Global_Manipulators, &QAction::triggered,
            this, &CMainFrame::OnToolGlobalManipulators);

    // Edit Camera toolbar
#if 0 // TODO: Disabled until UX decision is made if these buttons are needed at all or not
    connect(m_ui->actionPan_Tool, &QAction::triggered, this, &CMainFrame::OnEditCameraPan);
    connect(m_ui->actionOrbit_Tool, &QAction::triggered, this, &CMainFrame::OnEditCameraRotate);
    connect(m_ui->actionZoom_Tool, &QAction::triggered, this, &CMainFrame::OnEditCameraZoom);
#endif
    connect(m_ui->actionShading_Mode, &QAction::triggered, this, &CMainFrame::OnEditViewFillMode);
    connect(m_ui->actionRulers_Guides, &QAction::triggered, this, &CMainFrame::OnViewGuidesRulers);
    connect(m_ui->actionClear_Guides, &QAction::triggered, this, &CMainFrame::OnClearGuides);
    connect(m_ui->actionLock_Guides, &QAction::triggered, this, &CMainFrame::OnLockGuides);

    // Others
    connect(m_remoteDeploymentSender.data(), &RemoteDeploymentSender::connectionChanged,
            this, &CMainFrame::OnConnectionChanged);

    // Hide unimplemented menu items
    m_ui->actionRepeat->setVisible(false);
    m_ui->actionParent->setVisible(false);
    m_ui->actionUnparent->setVisible(false);
    m_ui->actionFit_all->setVisible(false);
    m_ui->actionToggle_hide_unhide_unselected->setVisible(false);
    m_ui->actionFind->setVisible(false);

    // TODO: better solution?
    m_updateUITimer->start(500);
    connect(m_updateUITimer.data(), &QTimer::timeout, [&]() {
        if (QApplication::activeWindow() != this)
            return;

        OnUpdateFileSave();
        OnUpdateEditUndo();
        OnUpdateEditRedo();
        OnUpdateEditCopy();
        OnUpdateEditCut();
        OnUpdateToolAutosetkeys();
        OnUpdateEditPaste();
        OnUpdateTimelineDeleteSelectedKeyframes();
        OnUpdateTimelineSetInterpolation();
        OnUpdateTimelineSetTimeBarColor();
        OnUpdateViewBoundingBoxes();
        OnUpdateViewPivotPoint();
        OnUpdateViewWireframe();
        OnUpdateViewTooltips();
        OnUpdateViewTimeline();
        onUpdateViewSceneCamera();
        OnUpdateViewInspector();
        OnUpdateViewAction();
        OnUpdateViewBasicObjects();
        OnUpdateViewProject();
        OnUpdateViewSlide();
        OnUpdateHelpIndex();
        OnUpdatePlaybackPlay();
        OnUpdatePlaybackRewind();
        OnUpdatePlaybackStop();
        OnUpdatePlaybackPreview();
        OnUpdateToolMove();
        OnUpdateToolRotate();
        OnUpdateToolScale();
        OnUpdateToolGlobalManipulators();
        OnUpdateToolGroupSelection();
        OnUpdateToolItemSelection();
        OnUpdateCameraZoomExtentAndAuthorZoom();
        OnUpdateEditCameraPan();
        OnUpdateEditCameraRotate();
        OnUpdateEditCameraZoom();
        OnUpdateEditViewFillMode();
        OnUpdateViewGuidesRulers();
        OnUpdateClearGuides();
        OnUpdateLockGuides();
        OnUpdateCameraPreview();
        OnUpdateEditViewLightingEnabled();
    });

    m_playbackTimer->setInterval(PLAYBACK_TIMER_TIMEOUT);
    connect(m_playbackTimer.data(), &QTimer::timeout, this, &CMainFrame::onPlaybackTimeout);
    qApp->installEventFilter(this);
}

//==============================================================================
/**
 * Destructor
 */
CMainFrame::~CMainFrame()
{
    qApp->removeEventFilter(this);
    m_playbackTimer->stop();
    m_updateUITimer->stop();
}

//==============================================================================
/**
 *	Timer callback
 */
void CMainFrame::onPlaybackTimeout()
{
    // Timer callback that drives playback
    Q_ASSERT(&g_StudioApp);
    g_StudioApp.GetCore()->GetDoc()->ClientStep();
}

//==============================================================================

void CMainFrame::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
}

void CMainFrame::hideEvent(QHideEvent *event)
{
    QMainWindow::hideEvent(event);
}

/**
 * Called when the main frame is actually created.  Sets up tool bars and default
 * views.
 */
void CMainFrame::OnCreate()
{
    m_sceneView.reset(new CSceneView(this));
    connect(m_sceneView.data(), &CSceneView::toolChanged, this, &CMainFrame::OnUpdateToolChange);

    m_sceneView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // tell the edit camera bar about this scene view
    m_ui->m_EditCamerasBar->SetSceneView(m_sceneView.data());

    // Newly launched, the file dialog for open and import should default to more recent
    // opened/imported
    CDialogs *theDialogs = g_StudioApp.GetDialogs();
    // this must NOT be in 'command line' mode
    if (theDialogs) {
        QString theMostRecentOpen;
        if (m_recentItems && m_recentItems->GetItemCount() > 0)
            theMostRecentOpen = m_recentItems->GetItem(0);
        if (theMostRecentOpen.isEmpty()) // default to exe
            theMostRecentOpen = Qt3DSFile::GetApplicationDirectory();

        theDialogs->ResetSettings(theMostRecentOpen);
    }

    // Create the view manager
    m_paletteManager.reset(new CPaletteManager(this));

    // Remove basic toolbar (open, save, undo/redo, etc.)
    // Kept in ui form in case it is going to be added back later on.
    delete m_ui->toolBar;

    // Disable toolbars and menus until we have a presentation
    m_ui->m_ClientToolsBar->setEnabled(false);
    m_ui->m_EditCamerasBar->setEnabled(false);
    m_ui->m_PlaybackToolbar->setEnabled(false);
    m_ui->menu_Edit->setEnabled(false);
    m_ui->menu_Timeline->setEnabled(false);
    m_ui->menu_View->setEnabled(false);
    m_ui->action_Save_Project_As->setEnabled(false);
    m_ui->action_Duplicate_Presentation->setEnabled(false);
    m_ui->action_Connect_to_Device->setEnabled(false);
    m_ui->action_Revert->setEnabled(false);
    m_ui->actionImportAssets->setEnabled(false);
    m_ui->actionRemote_Preview->setEnabled(false);
    m_ui->action_New_Presentation->setEnabled(false);
    m_ui->actionData_Inputs->setEnabled(false);

#if 1 // TODO: Hidden until UX decision is made if these buttons are needed at all or not
    m_ui->actionPan_Tool->setVisible(false);
    m_ui->actionOrbit_Tool->setVisible(false);
    m_ui->actionZoom_Tool->setVisible(false);
#endif

    // Show a message about opening or creating a presentation
    m_sceneView.data()->setVisible(false);
    setCentralWidget(m_ui->infoText);
}

//==============================================================================
/**
 * Called when a new presenation is created.  We have to wait to associate the
 * scene object with the scene view until this point, because the scene object
 * does not exist until this function gets called.
 */
void CMainFrame::OnNewPresentation()
{
    // Make sure scene is visible
    showScene();

    // Associate the scene object with the scene view
    m_ui->m_EditCamerasBar->SetupCameras();
    // Enable dockables, toolbars, and menus
    m_paletteManager->EnablePalettes();
    m_ui->m_ClientToolsBar->setEnabled(true);
    m_ui->m_EditCamerasBar->setEnabled(true);
    m_ui->m_PlaybackToolbar->setEnabled(true);
    m_ui->menu_Edit->setEnabled(true);
    m_ui->menu_Timeline->setEnabled(true);
    m_ui->menu_View->setEnabled(true);
    m_ui->action_Save_Project_As->setEnabled(true);
    m_ui->action_Duplicate_Presentation->setEnabled(true);
    m_ui->action_Connect_to_Device->setEnabled(true);
    m_ui->action_Revert->setEnabled(true);
    m_ui->actionImportAssets->setEnabled(true);
    m_ui->action_New_Presentation->setEnabled(true);
    m_ui->actionData_Inputs->setEnabled(true);

    // Clear data input list and sub-presentation list
    g_StudioApp.m_subpresentations.clear();
    g_StudioApp.m_dataInputDialogItems.clear();
}

//==============================================================================
/**
 * Called when the current presentation is being closed.
 * This will close all the editor windows that are open.
 */
void CMainFrame::OnClosingPresentation()
{
}

//==============================================================================
/**
 * Handles the Timeline | Set Interpolation menu item
 *	This is a temporary method that will display the Set Interpolation dialog.
 */
void CMainFrame::OnTimelineSetInterpolation()
{
    g_StudioApp.GetCore()->GetDoc()->SetKeyframeInterpolation();
}

//==============================================================================
/**
 *	OnEditRedo: calls handleRedoOperation
 */
//==============================================================================
void CMainFrame::OnEditRedo()
{
    g_StudioApp.GetCore()->GetCmdStack()->Redo();
}

//==============================================================================
/**
 *	OnEditUndo: calls HandleUndoOperation
 */
//==============================================================================
void CMainFrame::OnEditUndo()
{
    g_StudioApp.GetCore()->GetCmdStack()->Undo();
}

//==============================================================================
/**
 *	OnUpdateEditUndo: Handler for ID_EDIT_UNDO message
 *
 *	@param	pCmndUI	The UI element that generated the message
 */
void CMainFrame::OnUpdateEditUndo()
{
    const QString undoDescription = QObject::tr("Undo %1\tCtrl+Z").arg(
                g_StudioApp.GetCore()->GetCmdStack()->GetUndoDescription());
    m_ui->action_Undo->setEnabled(g_StudioApp.CanUndo());
    m_ui->action_Undo->setText(undoDescription);
}

//==============================================================================
/**
 *	OnUpdateEditRedo: handles the message ID_EDIT_REDO
 *
 *	@param	pCmndUI	The UI element that generated the message
 */
void CMainFrame::OnUpdateEditRedo()
{
    const QString redoDescription = QObject::tr("Redo %1\tCtrl+Y").arg(
                g_StudioApp.GetCore()->GetCmdStack()->GetRedoDescription());
    m_ui->action_Redo->setEnabled(g_StudioApp.CanRedo());
    m_ui->action_Redo->setText(redoDescription);
}

//==============================================================================
/**
 *	OnEditCopy: Handles the Copy message
 *
 *	Tells the doc to copy the selected keyframes.
 */
void CMainFrame::OnEditCopy()
{
    g_StudioApp.OnCopy();
}

//==============================================================================
/**
 *	OnUpdateEditCopy: Handle the update UI command for the copy button and menu item
 *
 *	If there are keyframes selected, the button is enabled, otherwise, it is
 *	disabled.
 *
 *	@param	pCmndUI	The UI element that generated the message
 */
void CMainFrame::OnUpdateEditCopy()
{
    // TODO: Actions cannot currently be copied/cut/pasted via main edit menu
    // ActionView handles action copy/cut/paste internally
    if (g_StudioApp.CanCopy()) {
        QString theDescription = tr("Copy %1\tCtrl+C").arg(g_StudioApp.GetCopyType());

        m_ui->action_Copy->setText(theDescription);
        m_ui->action_Copy->setEnabled(true);
    } else {
        m_ui->action_Copy->setEnabled(false);
    }
}

//==============================================================================
/**
 *	OnEditCut: Handles the Cut message
 *
 *	Tells the doc to cut the selected keyframes.
 */
void CMainFrame::OnEditCut()
{
    g_StudioApp.OnCut();
}

//==============================================================================
/**
 *	OnUpdateEditCut: Handle the update UI command for the cut button and menu item
 *
 *	If there are keyframes selected, the button is enabled, otherwise, it is
 *	disabled.
 *
 *	@param	pCmndUI	The UI element that generated the message
 */
void CMainFrame::OnUpdateEditCut()
{
    if (g_StudioApp.CanCut()) {
        QString theDescription = tr("Cut %1\tCtrl+X").arg(g_StudioApp.GetCopyType());

        m_ui->action_Cut->setText(theDescription);
        m_ui->action_Cut->setEnabled(true);
    } else {
        m_ui->action_Cut->setEnabled(false);
    }
}

//==============================================================================
/**
 *	OnEditPaste: Handles the Paste command
 *
 *	Tells the doc to paste the copied keyframes at the current playhead time, on
 *	the currently selected object.
 */
void CMainFrame::OnEditPaste()
{
    g_StudioApp.OnPaste();
}

void CMainFrame::onEditPasteToMaster()
{
    g_StudioApp.GetCore()->GetDoc()->HandleMasterPaste();
}

//==============================================================================
/**
 *	OnUpdateEditPaste: Handle the update UI command for the paste button and menu item
 *
 *	If there we can perform a keyframe paste, the button is enabled, otherwise, it is
 *	disabled.
 *
 *	@param	pCmndUI	The UI element that generated the message
 */
void CMainFrame::OnUpdateEditPaste()
{
    if (g_StudioApp.CanPaste()) {
        QString theUndoDescription = tr("Paste %1\tCtrl+V").arg(g_StudioApp.GetPasteType());

        m_ui->action_Paste->setText(theUndoDescription);

        m_ui->action_Paste->setEnabled(true);
        m_ui->actionPaste_to_Master_Slide->setEnabled(true);
    } else {
        m_ui->action_Paste->setEnabled(false);
        m_ui->actionPaste_to_Master_Slide->setEnabled(false);
    }
}

//=============================================================================
/**
 * Called when a tool mode changes from a modifier key
 */
void CMainFrame::OnUpdateToolChange()
{
    long theSelectMode = g_StudioApp.GetSelectMode();
    m_ui->actionGroup_Select_Tool->setChecked(theSelectMode == STUDIO_SELECTMODE_GROUP);
    m_ui->actionItem_Select_Tool->setChecked(theSelectMode == STUDIO_SELECTMODE_ENTITY);

    // See what tool mode we are in and change checks accordingly
    long theToolMode = g_StudioApp.GetToolMode();
    m_ui->actionPosition_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_MOVE);
    m_ui->actionRotation_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_ROTATE);
    m_ui->actionScale_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_SCALE);
    m_ui->actionLocal_Global_Manipulators->setChecked(g_StudioApp.GetManipulationMode()
                                                      == StudioManipulationModes::Global);

#if 0 // TODO: Disabled until UX decision is made if these buttons are needed at all or not
    m_ui->actionPan_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_CAMERA_PAN);
    m_ui->actionOrbit_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_CAMERA_ROTATE);
    m_ui->actionZoom_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_CAMERA_ZOOM);
#endif
}

//==============================================================================
/**
 *	OnTimelineSettimebarcolor: Handles the ID_TIMELINE_SETTIMEBARCOLOR message.
 *
 *	Called when the user clicks on Timeline->Change Time Bar Color.  Changes
 *	the currently selected timebar's color.
 */
void CMainFrame::OnTimelineSetTimeBarColor()
{
    getTimelineWidget()->openBarColorDialog();
}

//==============================================================================
/**
 *	OnUpdateTimelineSetTimeBarColor: Handles the update UI message for the
 *	"Change Time Bar Color" menu item.
 *
 *	If the currently selected object is an item in the timeline and it has a
 *	time bar, this menu item is enabled.  Otherwise, the menu item is disabled.
 *
 *	@param pCmndUI	Pointer to the ui object that generated this update message.
 */
void CMainFrame::OnUpdateTimelineSetTimeBarColor()
{
    m_ui->actionChange_Time_Bar_Color->setEnabled(g_StudioApp.CanChangeTimebarColor());
}

//==============================================================================
/**
 *	OnTimelineSetChangedKeyframe: Handles the ID_TIMELINE_SETCHANGEDKEYFRAME message.
 *
 *	Calls the StudioDoc handler to insert keyframes for animatable properties that
 *	have changed.
 */
void CMainFrame::OnTimelineSetChangedKeyframe()
{
    g_StudioApp.HandleSetChangedKeys();
}

//==============================================================================
/**
 *	OnUpdateTimelineDeleteSelectedKeyframes: Handles the update UI message for
 *	the "Delete Selected Keyframe(s)" message.
 *
 *	If there are currently keyframes selected, the menu item is enabled.  Otherwise,
 *	the menu item is disabled.
 *
 *	@param pCmdUI	The UI element that generated this message
 */
void CMainFrame::OnUpdateTimelineDeleteSelectedKeyframes()
{
    m_ui->actionDelete_Selected_Keyframe_s->setEnabled(getTimelineWidget()->hasSelectedKeyframes());
}

//==============================================================================
/**
 *	OnUpdateTimelineSetInterpolation: Handles the update UI message for
 *	the "Set Interpolation" message.
 *
 *	If there are currently keyframes selected, this menu item is enabled, otherwise
 *	it is disabled.
 *
 *	@param pCmdUI	The UI element that generated this message
 */
void CMainFrame::OnUpdateTimelineSetInterpolation()
{
    m_ui->actionSet_Interpolation->setEnabled(
                g_StudioApp.GetCore()->GetDoc()->GetKeyframesManager()->HasSelectedKeyframes());
}

//==============================================================================
/**
 *	OnEditDuplicate: Handles the ID_EDIT_DUPLICATE message.
 *
 *	Pass through to the doc.
 */
void CMainFrame::OnEditDuplicate()
{
    g_StudioApp.HandleDuplicateCommand();
}

void CMainFrame::onEditDelete()
{
    g_StudioApp.DeleteSelectedObject();
}

void CMainFrame::onEditGroup()
{
    if (!g_StudioApp.ungroupSelectedObjects())
        g_StudioApp.groupSelectedObjects();
}

/**
 * Command handler for the File Open menu and toolbar options.
 * This will save the file, if the file has not been saved before this will
 * do a save as operation.
 */
void CMainFrame::OnFileOpen()
{
    g_StudioApp.OnFileOpen();
}

//=============================================================================
/**
 * Command handler for the File Save menu and toolbar options.
 * This will save the file, if the file has not been saved before this will
 * do a save as operation.
 */
void CMainFrame::OnFileSave()
{
    g_StudioApp.OnSave();
}

void CMainFrame::OnUpdateFileSave()
{
    m_ui->action_Save->setEnabled(g_StudioApp.GetCore()->GetDoc()->IsModified());
}

/**
 * Command handler for the File Save Project As menu option.
 */
void CMainFrame::onProjectSaveAs()
{
    g_StudioApp.onProjectSaveAs();
}

/**
 * Command handler for the File Duplicate Presentation menu option.
 */
void CMainFrame::onDuplicatePresentation()
{
    g_StudioApp.duplicatePresentation();
}

/**
 * Command handler for the New Project menu option.
 * This will also create a new default presentation.
 */
void CMainFrame::OnProjectNew()
{
    g_StudioApp.OnProjectNew();
}

void CMainFrame::OnFileNew()
{
    g_StudioApp.OnFileNew();
}

void CMainFrame::onCtrlNPressed()
{
    static QMap<int, int> key2index {
        {1, 9},         // Scene Camera View
        {2, 0}, {3, 1}, // Perspective & Orthographic
        {4, 2}, {5, 3}, // Top & Bottom
        {6, 4}, {7, 5}, // Left & Right
        {8, 6}, {9, 7}, // Front & Back
    };

    QAction *action = qobject_cast<QAction *>(sender());
    Q_ASSERT(action);
    QKeySequence shortcut = action->shortcut();
    QMapIterator<int, int> i(key2index);
    while (i.hasNext()) {
        i.next();
        QKeySequence keySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + i.key()));
        if (shortcut.matches(keySequence) == QKeySequence::ExactMatch) {
            m_ui->m_EditCamerasBar->setCameraIndex(i.value());
            break;
        }
    }
}

//==============================================================================
/**
 * Overrides the close method to prompt if the document is modified.
 */
void CMainFrame::closeEvent(QCloseEvent *event)
{
    handleGeometryAndState(true);
    QMainWindow::closeEvent(event);

    if (g_StudioApp.GetCore()->GetDoc()->IsModified()) {
        CDialogs::ESavePromptResult theResult = g_StudioApp.GetDialogs()->PromptForSave();
        if (theResult == CDialogs::SAVE_FIRST) {
            // If the save was canceled or failed then do not exit.
            if (!g_StudioApp.OnSave())
                return;
        } else if (theResult == CDialogs::CANCEL_OPERATION) {
            // On cancel ditch out of here and abort exit. Abort! Abort!
            event->ignore();
            return;
        }
    }

    // Tell the app to shutdown, do it here so it does not rely on static destructor.
    QTimer::singleShot(0, &g_StudioApp, &CStudioApp::performShutdown);
}

//==============================================================================
/**
 *	Displays the preferences dialog and can change program settings.
 */
void CMainFrame::OnEditApplicationPreferences()
{
    EditPreferences(PAGE_STUDIOAPPPREFERENCES);
}

//==============================================================================
/**
 *	Displays the preferences dialog and can change program settings.
 */
void CMainFrame::OnEditPresentationPreferences()
{
    EditPreferences(PAGE_STUDIOPROJECTSETTINGS);
}

//==============================================================================
/**
 *  Displays the data input dialog.
 */
void CMainFrame::OnFileDataInputs()
{
    CDataInputListDlg dataInputDlg(&(g_StudioApp.m_dataInputDialogItems));
    dataInputDlg.exec();

    if (dataInputDlg.result() == QDialog::Accepted)
        g_StudioApp.saveDataInputsToProjectFile();
}

//==============================================================================
/**
 *	EditPreferences: Displays the presentation settings property sheet with
 *					 the specified active page.
 *
 *	Used for editing the application and project settings.
 *
 *	@param	inPageIndex		The page index to select when displayed.
 */
void CMainFrame::EditPreferences(short inPageIndex)
{
    // Set the active page based on the inPageIndex
    m_propSheet.reset(new CStudioPreferencesPropSheet(tr("Studio Preferences"), this, inPageIndex));

    // Display the CStudioPreferencesPropSheet
    int thePrefsReturn = m_propSheet->exec();

    m_sceneView->onEditCameraChanged();

    if (thePrefsReturn == PREFS_RESET_DEFAULTS) {
        // Restore default values
        g_StudioApp.SetAutosetKeyframes(true); // Sets the preference as well
        CStudioPreferences::SetBoundingBoxesOn(true);
        CStudioPreferences::SetDisplayPivotPoint(true);
        CStudioPreferences::SetWireframeModeOn(true);
        CStudioPreferences::SetShowTooltips(true);
        CStudioPreferences::SetTimebarDisplayTime(false);
        g_StudioApp.GetCore()->GetDoc()->SetDefaultKeyframeInterpolation(true);
        CStudioPreferences::SetSnapRange(CStudioPreferences::DEFAULT_SNAPRANGE);
        CStudioPreferences::SetDefaultObjectLifetime(CStudioPreferences::DEFAULT_LIFETIME);
        CStudioPreferences::SetAdvancePropertyExpandedFlag(false);
        CStudioPreferences::SetPreviewConfig("");
        CStudioPreferences::SetPreviewProperty("", "");
        CStudioPreferences::SetDontShowGLVersionDialog(false);
        CStudioPreferences::SetDefaultClientSize(CStudioPreferences::DEFAULT_CLIENT_WIDTH,
                                                 CStudioPreferences::DEFAULT_CLIENT_HEIGHT);
        CStudioPreferences::SetTimeAdvanceAmount(CStudioPreferences::DEFAULT_TIME_ADVANCE);
        CStudioPreferences::SetBigTimeAdvanceAmount(CStudioPreferences::DEFAULT_BIG_TIME_ADVANCE);
        CStudioPreferences::SetTimelineSnappingGridActive(true);
        CStudioPreferences::SetTimelineSnappingGridResolution(SNAPGRID_SECONDS);
        CStudioPreferences::SetEditViewFillMode(true);
        CStudioPreferences::SetPreferredStartupView(
                    CStudioPreferences::PREFERREDSTARTUP_DEFAULTINDEX);
        CStudioPreferences::SetAutoSaveDelay(CStudioPreferences::DEFAULT_AUTOSAVE_DELAY);
        CStudioPreferences::SetAutoSavePreference(true);
        CStudioPreferences::setSelectorLineWidth(
                    (float)CStudioPreferences::DEFAULT_SELECTOR_WIDTH / 10.0f);
        CStudioPreferences::setSelectorLineLength(
                    (float)CStudioPreferences::DEFAULT_SELECTOR_LENGTH);

        RecheckSizingMode();

        // Save preferences, to make sure we do not lose them on a possible crash
        QTimer::singleShot(0, [](){ CStudioPreferences::savePreferences(); });
    } else if (thePrefsReturn == PREFS_RESET_LAYOUT) {
        onViewResetLayout();
    } else if (thePrefsReturn == PREFS_SETTINGS_RESTART) {
        QTimer::singleShot(0, this, &CMainFrame::handleRestart);
    } else if (thePrefsReturn != 0) {
        // Save preferences, to make sure we do not lose them on a possible crash
        QTimer::singleShot(0, [](){ CStudioPreferences::savePreferences(); });
    }
}

//==============================================================================
/**
 *	OnToolAutosetkeys: Called when the Autoset Keyframe button is pressed.
 *	Calls the doc to turn off or on the Autoset Keyframe preference.
 */
void CMainFrame::OnToolAutosetkeys()
{
    // Toggle autoset keyframes to the opposite of what it's currently set as
    g_StudioApp.SetAutosetKeyframes(!CStudioPreferences::IsAutosetKeyframesOn());

    // Don't wait for regular update cycle to update the corresponding toolbar/menu checked status
    m_ui->actionAutoset_Keyframes->setChecked(CStudioPreferences::IsAutosetKeyframesOn());
}

//==============================================================================
/**
 *	OnUpdateToolAutosetkeys: Updates the UI associated with this button.
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *	@param pCmdUI Pointer to the button that generated the message.
 */
void CMainFrame::OnUpdateToolAutosetkeys()
{
    // If autoset keyframes is on
    m_ui->actionAutoset_Keyframes->setChecked(CStudioPreferences::IsAutosetKeyframesOn());
}

//==========================================================================
/**
 *	Called when the presentation is being played in Studio.  Updates the play
 *	button on the main frame.
 */
void CMainFrame::OnPlayStart()
{
    // Update the play button since this doesn't always happen automatically
    Q_EMIT playStateChanged(true);

    if (m_playbackFlag == false) {
        m_playbackFlag = true;
        m_playbackTimer->start();
    }
}

//==========================================================================
/**
 *	Called when the presentation stops being played in Studio.  Updates the play
 *	button on the main frame.
 */
void CMainFrame::OnPlayStop()
{
    // Update the play button since this doesn't always happen automatically
    Q_EMIT playStateChanged(false);

    if (m_playbackFlag == true) {
        m_playbackFlag = false;
        m_playbackTimer->stop();
    }
}

/**
 *	Called when the presentation time changes.  Not handled by this class,
 *	but included because the base class requires it to be implemented.
 */
void CMainFrame::OnTimeChanged(long inTime)
{
}

/**
 *	Handles pressing the play button.
 */
void CMainFrame::OnPlaybackPlay()
{
    g_StudioApp.PlaybackPlay();
}

/**
 *	Handles pressing of the stop button.
 */
void CMainFrame::OnPlaybackStop()
{
    g_StudioApp.PlaybackStopNoRestore();
}

/**
 *	Handles pressing the preview button.
 */
void CMainFrame::OnPlaybackPreview(const QString &viewerExeName, bool remote)
{
    if (remote && m_remoteDeploymentSender->isConnected()) {
        g_StudioApp.GetCore()->GetDispatch()->FireOnProgressBegin(
                    QObject::tr("Deploying to remote device..."), {});
        CPreviewHelper::OnDeploy(*m_remoteDeploymentSender);
        g_StudioApp.GetCore()->GetDispatch()->FireOnProgressEnd();
    } else {
        CPreviewHelper::OnPreview(viewerExeName);
    }
}

void CMainFrame::OnPlaybackPreviewRuntime1()
{
    OnPlaybackPreview(QStringLiteral("Qt3DViewer"));
}

void CMainFrame::OnPlaybackPreviewRuntime2()
{
    OnPlaybackPreview(QStringLiteral("q3dsviewer"));
}

void CMainFrame::OnPlaybackPreviewRemote()
{
    OnPlaybackPreview(QStringLiteral("q3dsviewer"), true);
}

//==============================================================================
/**
 *	Handles the update ui message for the preview button.
 *	Adding more UI updating here would just be redundant.
 *	@param inCmdUI Pointer to the UI element that needs updating
 */
void CMainFrame::OnUpdatePlaybackPreview()
{
}

//==============================================================================
/**
 *	Handles the update ui message for the play button.  Does nothing because the
 *	button state is being updated manually in OnPlayStart() and OnPlayStop.
 *	Adding more UI updating here would just be redundant.
 *	@param inCmdUI Pointer to the UI element that needs updating
 */
void CMainFrame::OnUpdatePlaybackPlay()
{
}

//==============================================================================
/**
 *	Handles pressing the rewind button.
 */
void CMainFrame::OnPlaybackRewind()
{
    g_StudioApp.PlaybackRewind();
}

//==============================================================================
/**
 *	Handles the update ui message for the rewind button.  Does nothing because
 *	no additional ui handling is necessary for this button.
 *	@param inCmdUI Pointer to the UI element that needs updating
 */
void CMainFrame::OnUpdatePlaybackRewind()
{
}

//==============================================================================
/**
 *	Handles the update ui message for the stop button.  Doesn't do anything
 *	because no special ui handling is necessary for the stop button.
 *	@param inCmdUI Pointer to the UI element that needs updating
 */
void CMainFrame::OnUpdatePlaybackStop()
{
}

//==============================================================================
/**
 *	Registers all the keys it will need for shortcuts, also telsl children to register theirs
 *  @param inHotKeys the hotkeys to with which to register
 */
void CMainFrame::RegisterGlobalKeyboardShortcuts(CHotKeys *inHotKeys, QWidget *actionParent)
{
    // Default undo shortcut is Ctrl-Y, which is specified in main form. Let's add the common
    // alternate shortcut for redo, CTRL-SHIFT-Z
    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Z),
                        CMainFrame::OnEditRedo);

    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::Key_Q),
                        CMainFrame::toggleSelectMode);

    for (int keyN = Qt::Key_1; keyN <= Qt::Key_9; keyN++) {
        ADD_GLOBAL_SHORTCUT(actionParent,
                            QKeySequence(Qt::CTRL | static_cast<Qt::Key>(keyN)),
                            CMainFrame::onCtrlNPressed);
    }
}

//==============================================================================
/**
 *	OnUpdateToolMove: Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
//==============================================================================
void CMainFrame::OnUpdateToolMove()
{
    long theCurrentToolSettings = g_StudioApp.GetToolMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionPosition_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_MOVE
                                          && m_ui->actionPosition_Tool->isEnabled());
}

//==============================================================================
/**
 *	OnUpdateToolRotate: Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
void CMainFrame::OnUpdateToolRotate()
{
    long theCurrentToolSettings = g_StudioApp.GetToolMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionRotation_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_ROTATE
                                          && m_ui->actionRotation_Tool->isEnabled());
}

//==============================================================================
/**
 *	OnUpdateToolScale: Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
void CMainFrame::OnUpdateToolScale()
{
    long theCurrentToolSettings = g_StudioApp.GetToolMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionScale_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_SCALE
                                       && m_ui->actionScale_Tool->isEnabled());
}

//==============================================================================
/**
 *	OnUpdateToolScale: Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
void CMainFrame::OnUpdateToolGlobalManipulators()
{
    StudioManipulationModes::Enum theMode = g_StudioApp.GetManipulationMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionLocal_Global_Manipulators->setChecked(
                theMode == StudioManipulationModes::Global
                && m_ui->actionLocal_Global_Manipulators->isEnabled());
}

//==============================================================================
/**
 *	OnToolMove: Called when the Move button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CMainFrame::OnToolMove()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
    m_sceneView->setToolMode(STUDIO_TOOLMODE_MOVE);
}

//==============================================================================
/**
 *	OnToolRotate: Called when the Rotate button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CMainFrame::OnToolRotate()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_ROTATE);
    m_sceneView->setToolMode(STUDIO_TOOLMODE_ROTATE);
}

//==============================================================================
/**
 *	OnToolScale: Called when the Scale button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CMainFrame::OnToolScale()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_SCALE);
    m_sceneView->setToolMode(STUDIO_TOOLMODE_SCALE);
}

void CMainFrame::OnToolGlobalManipulators()
{
    if (m_ui->actionLocal_Global_Manipulators->isChecked())
        g_StudioApp.SetManipulationMode(StudioManipulationModes::Global);
    else
        g_StudioApp.SetManipulationMode(StudioManipulationModes::Local);

    g_StudioApp.getRenderer().RequestRender();
}

//==============================================================================
/**
 *	OnUpdateToolGroupSelection: Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
void CMainFrame::OnUpdateToolGroupSelection()
{
    long theCurrentSelectSettings = g_StudioApp.GetSelectMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionGroup_Select_Tool->setChecked(theCurrentSelectSettings == STUDIO_SELECTMODE_GROUP
                                              && m_ui->actionGroup_Select_Tool->isEnabled());
}

//==============================================================================
/**
 *	OnUpdateToolItemSelection: Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	tool mode, and whether or not the button is enabled.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
void CMainFrame::OnUpdateToolItemSelection()
{
    long theCurrentSelectSettings = g_StudioApp.GetSelectMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionItem_Select_Tool->setChecked(theCurrentSelectSettings == STUDIO_SELECTMODE_ENTITY
                                             && m_ui->actionItem_Select_Tool->isEnabled());
}

//==============================================================================
/**
 *	Called when the edit camera Zoom Extent button is pressed.
 *	Inform the current active edit camera to toggle itself.
 */
void CMainFrame::OnEditCameraZoomExtent()
{
    if (g_StudioApp.getRenderer().GetEditCamera() >= 0)
        g_StudioApp.getRenderer().EditCameraZoomToFit();
    else
        g_StudioApp.SetAuthorZoom(!g_StudioApp.IsAuthorZoom());
}

//==============================================================================
/**
 *	Called when the "Zoom Extent" keyboard shortcut is pressed.
 *	Inform the current active edit camera to toggle itself.
 */
//==============================================================================
void CMainFrame::HandleEditCameraZoomExtent()
{
    OnEditCameraZoomExtent();
}

//==============================================================================
/**
 *	Called when the Pan Edit Camera button is pressed.
 *	Inform the current active edit camera on their tool mode.
 */
void CMainFrame::OnEditCameraPan()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
    m_sceneView->setViewCursor(); // Just set cursor, we don't want to update previous tool
}

//==============================================================================
/**
 *	Called when the Rotate Edit Camera button is pressed.
 *	Inform the current active edit camera on their tool mode.
 */
void CMainFrame::OnEditCameraRotate()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
    m_sceneView->setViewCursor(); // Just set cursor, we don't want to update previous tool
}

//==============================================================================
/**
 *	Called when the Zoom Edit Camera button is pressed.
 *	Inform the current active edit camera on their tool mode.
 */
void CMainFrame::OnEditCameraZoom()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ZOOM);
    m_sceneView->setViewCursor(); // Just set cursor, we don't want to update previous tool
}

//==============================================================================
/**
 *	Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	settings of the current edit camera tool.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
//==============================================================================
void CMainFrame::OnUpdateCameraZoomExtentAndAuthorZoom()
{
    if (m_sceneView.data() == GetActiveView() && !m_sceneView->isDeploymentView())
        m_ui->actionFit_Selected->setChecked(false);
    else
        m_ui->actionFit_Selected->setChecked(g_StudioApp.IsAuthorZoom());
}

//==============================================================================
/**
 *	Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	settings of the current edit camera tool.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
//==============================================================================
void CMainFrame::OnUpdateEditCameraPan()
{
    if (m_sceneView.data() == GetActiveView() && !m_sceneView->isDeploymentView()) {
        m_ui->actionPan_Tool->setEnabled(true);

        long theCurrentToolSettings = g_StudioApp.GetToolMode();
        m_ui->actionPan_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_CAMERA_PAN);
    } else {
        m_ui->actionPan_Tool->setEnabled(false);
        m_ui->actionPan_Tool->setChecked(false);
    }
}

//==============================================================================
/**
 *	Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	settings of the current edit camera tool.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
//==============================================================================
void CMainFrame::OnUpdateEditCameraRotate()
{
#if 0 // TODO: Disabled until UX decision is made if these buttons are needed at all or not
    if (m_SceneView == GetActiveView() && !m_SceneView->IsDeploymentView()
            && g_StudioApp.GetRenderer().DoesEditCameraSupportRotation(
                g_StudioApp.GetRenderer().GetEditCamera())) {
        m_ui->actionOrbit_Tool->setEnabled(true);

        long theCurrentToolSettings = g_StudioApp.GetToolMode();
        m_ui->actionOrbit_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_CAMERA_ROTATE);
    } else {
        m_ui->actionOrbit_Tool->setEnabled(false);
        m_ui->actionOrbit_Tool->setChecked(false);
    }
#endif
}

//==============================================================================
/**
 *	Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	settings of the current edit camera tool.
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
//==============================================================================
void CMainFrame::OnUpdateEditCameraZoom()
{
    if (m_sceneView.data() == GetActiveView() && !m_sceneView->isDeploymentView()) {
        m_ui->actionZoom_Tool->setEnabled(true);

        long theCurrentToolSettings = g_StudioApp.GetToolMode();
        m_ui->actionZoom_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_CAMERA_ZOOM);
    } else {
        m_ui->actionZoom_Tool->setEnabled(false);
        m_ui->actionZoom_Tool->setChecked(false);
    }
}

//==============================================================================
/**
 *	Called when the "Render geometry as solid/wireframe in edit view" button is pressed.
 *	Toggle the mode and update the studio preferences, also cache it in EditCameraContainer
 */
//==============================================================================
void CMainFrame::HandleEditViewFillModeKey()
{
    if (m_sceneView.data() == GetActiveView() && !m_sceneView->isDeploymentView()) {
        OnEditViewFillMode();
        bool theEditViewFillMode = g_StudioApp.getRenderer().IsPolygonFillModeEnabled();
        m_ui->actionShading_Mode->setChecked(theEditViewFillMode);
    }
}

//==============================================================================
/**
 *	Called when the "Render geometry as solid/wireframe in edit view" button is pressed.
 *	Toggle the mode and update the studio preferences, also cache it in EditCameraContainer
 */
//==============================================================================
void CMainFrame::OnEditViewFillMode()
{
    bool theEditViewFillMode = !g_StudioApp.getRenderer().IsPolygonFillModeEnabled();
    g_StudioApp.getRenderer().SetPolygonFillModeEnabled(theEditViewFillMode);
}

//==============================================================================
/**
 *	Updates the UI associated with this button.
 *
 *	Checks or unchecks this button on the toolbar, depending on the current
 *	settings of the "Render geometry as solid/wireframe in edit view".
 *
 *	@param pCmdUI Pointer to the button that generated the message.
 */
//==============================================================================
void CMainFrame::OnUpdateEditViewFillMode()
{
    if (m_sceneView.data() == GetActiveView() && !m_sceneView->isDeploymentView()) {
        m_ui->actionShading_Mode->setEnabled(true);
        m_ui->actionShading_Mode->setChecked(g_StudioApp.getRenderer().IsPolygonFillModeEnabled());
    } else {
        m_ui->actionShading_Mode->setEnabled(false);
        m_ui->actionShading_Mode->setChecked(false);
    }
}

void CMainFrame::OnViewGuidesRulers()
{
    g_StudioApp.getRenderer().SetGuidesEnabled(!g_StudioApp.getRenderer().AreGuidesEnabled());
    g_StudioApp.GetCore()->GetDispatch()->FireAuthorZoomChanged();
    m_sceneView->onRulerGuideToggled();
}

void CMainFrame::OnUpdateViewGuidesRulers()
{
    m_ui->actionRulers_Guides->setEnabled(m_sceneView->isDeploymentView());
    m_ui->actionRulers_Guides->setChecked(g_StudioApp.getRenderer().AreGuidesEnabled());
}

void CMainFrame::OnClearGuides()
{
    g_StudioApp.clearGuides();
}

void CMainFrame::OnUpdateClearGuides()
{
    bool enable = g_StudioApp.getRenderer().AreGuidesEnabled()
            && g_StudioApp.getRenderer().AreGuidesEditable() && m_sceneView->isDeploymentView();

    m_ui->actionClear_Guides->setEnabled(enable);
}

void CMainFrame::OnLockGuides()
{
    g_StudioApp.getRenderer().SetGuidesEditable(!g_StudioApp.getRenderer().AreGuidesEditable());
}

void CMainFrame::OnUpdateLockGuides()
{
    bool enable = g_StudioApp.getRenderer().AreGuidesEnabled() && m_sceneView->isDeploymentView();
    m_ui->actionLock_Guides->setEnabled(enable);
    // Set to the inverse of guides editable.
    m_ui->actionLock_Guides->setChecked(!g_StudioApp.getRenderer().AreGuidesEditable());
}

void CMainFrame::OnUpdateCameraPreview()
{
    m_ui->actionCamera_Preview->setChecked(CStudioPreferences::showEditModePreview());
    g_StudioApp.getRenderer().RequestRender();
}

void CMainFrame::OnUpdateEditViewLightingEnabled()
{
    m_ui->actionEdit_Lighting->setChecked(CStudioPreferences::editModeLightingEnabled());
    g_StudioApp.getRenderer().RequestRender();
}

void CMainFrame::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == WM_STUDIO_TIMER)
        g_StudioApp.getTickTock().ProcessMessages();
    QMainWindow::timerEvent(event);
}

void CMainFrame::onViewResetLayout()
{
    // Ask for a restart
    int theChoice = QMessageBox::question(this,
                                          tr("Restart Needed"),
                                          tr("Are you sure that you want to restore Qt 3D Studio "
                                             "layout? \nYour current layout will be lost, and "
                                             "Studio will restart."));

    // If "Yes" is clicked, delete window geometry and window state keys from QSettings
    if (theChoice == QMessageBox::Yes) {
        QSettings settings;
        QString geoKey = QStringLiteral("mainWindowGeometry") + QString::number(STUDIO_VERSION_NUM);
        QString stateKey = QStringLiteral("mainWindowState") + QString::number(STUDIO_VERSION_NUM);
        settings.remove(geoKey);
        settings.remove(stateKey);
        // Prevent saving geometry and state, and exit
        m_resettingLayout = true;
        QTimer::singleShot(0, this, &CMainFrame::handleRestart);
    }
}

void CMainFrame::OnViewAction()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_ACTION);
}

void CMainFrame::OnUpdateViewAction()
{
    m_ui->actionAction->setChecked(
                m_paletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_ACTION));
}

void CMainFrame::OnViewBasicObjects()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_BASICOBJECTS);
}

void CMainFrame::OnUpdateViewBasicObjects()
{
    m_ui->actionBasic_Objects->setChecked(m_paletteManager->IsControlVisible(
                                              CPaletteManager::CONTROLTYPE_BASICOBJECTS));
}

void CMainFrame::OnViewInspector()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_INSPECTOR);
}

void CMainFrame::OnUpdateViewInspector()
{
    m_ui->actionInspector->setChecked(
                m_paletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_INSPECTOR));
}

void CMainFrame::OnViewProject()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_PROJECT);
}

void CMainFrame::OnUpdateViewProject()
{
    m_ui->actionProject->setChecked(
                m_paletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_PROJECT));
}

void CMainFrame::OnViewSlide()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_SLIDE);
}

void CMainFrame::OnUpdateViewSlide()
{
    m_ui->actionSlide->setChecked(
                m_paletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_SLIDE)
                ? TRUE : FALSE);
}

//==============================================================================
/**
 *	Called when the View Inspector Palette menu item is chosen.
 */
void CMainFrame::OnViewTimeline()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_TIMELINE);
}

//==============================================================================
/**
 *	Checks or unchecks the menu item depending on if the view is available or not.
 *	@param pCmdUI Pointer to the UI element that generated the message.
 */
void CMainFrame::OnUpdateViewTimeline()
{
    m_ui->actionTimeline->setChecked(
                m_paletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_TIMELINE));
}

void CMainFrame::onViewSceneCamera()
{
    m_paletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_SCENECAMERA);
    onUpdateViewSceneCamera();
}

void CMainFrame::onUpdateViewSceneCamera()
{
    const bool cameraVisible = m_paletteManager->IsControlVisible(
                CPaletteManager::CONTROLTYPE_SCENECAMERA);
    m_ui->actionSceneCamera->setChecked(cameraVisible);
    g_StudioApp.getRenderer().setFullSizePreview(cameraVisible);
    g_StudioApp.getRenderer().RequestRender();
}

//==============================================================================
/**
 *	Called when the View Inspector Palette menu item is chosen.
 */
void CMainFrame::OnViewBoundingBoxes()
{
    CStudioPreferences::SetBoundingBoxesOn(!CStudioPreferences::IsBoundingBoxesOn());
    g_StudioApp.getRenderer().RequestRender();
}

//==============================================================================
/**
 *	Checks or unchecks the menu item depending on if the view is available or not.
 *	@param pCmdUI Pointer to the UI element that generated the message.
 */
void CMainFrame::OnUpdateViewBoundingBoxes()
{
    m_ui->actionBounding_Boxes->setChecked(CStudioPreferences::IsBoundingBoxesOn());
}

//==============================================================================
/**
 *	Called when the View Pivot Point menu item is chosen.
 */
void CMainFrame::OnViewPivotPoint()
{
    CStudioPreferences::SetDisplayPivotPoint(!CStudioPreferences::ShouldDisplayPivotPoint());
    g_StudioApp.getRenderer().RequestRender();
}

//==============================================================================
/**
 *	Checks or unchecks the menu item depending on if the view is available or not.
 *	@param pCmdUI Pointer to the UI element that generated the message.
 */
void CMainFrame::OnUpdateViewPivotPoint()
{
    m_ui->actionPivot_Point->setChecked(CStudioPreferences::ShouldDisplayPivotPoint());
}

//==============================================================================
/**
 *	Called when the View Wireframe menu item is chosen.
 */
void CMainFrame::OnViewWireframe()
{
    CStudioPreferences::SetWireframeModeOn(!CStudioPreferences::IsWireframeModeOn());

    // Don't wait for regular update cycle to update the corresponding toolbar/menu checked status
    m_ui->actionWireframe->setChecked(CStudioPreferences::IsWireframeModeOn());

    g_StudioApp.getRenderer().RequestRender();
}

//==============================================================================
/**
 *	Checks or unchecks the menu item depending on if the view is available or not.
 *	@param pCmdUI Pointer to the UI element that generated the message.
 */
void CMainFrame::OnUpdateViewWireframe()
{
    m_ui->actionWireframe->setChecked(CStudioPreferences::IsWireframeModeOn());
}

//==============================================================================
/**
 *	Checks or unchecks the menu item depending whether tooltips should be shown
 *	or not.
 *	@param inCmdUI Pointer to the UI element that generated the message.
 */
void CMainFrame::OnUpdateViewTooltips()
{
    m_ui->actionTooltips->setChecked(CStudioPreferences::ShouldShowTooltips());
}

//==============================================================================
/**
 *	Called when the "View->Tooltips" menu item is chosen.  Toggles tooltips on
 *	and off for custom controls.
 */
void CMainFrame::OnViewTooltips()
{
    CStudioPreferences::SetShowTooltips(!CStudioPreferences::ShouldShowTooltips());
}

//==============================================================================
/**
 *	Called when the update message occurs for the Help->Help Topics menu item.
 *	If the help file exists, the menu item is enabled, otherwise it's disabled.
 *	@param inCmdUI UI element that generated the message
 */
void CMainFrame::OnUpdateHelpIndex()
{
    QFile theFile(g_StudioApp.m_helpFilePath);
    m_ui->action_Reference_Manual->setEnabled(theFile.exists());
}

//==============================================================================
/**
 *	Handles the ID_HELP_INDEX command.  Opens the online help for Studio.
 */
void CMainFrame::OnHelpIndex()
{
    QFile theFile(g_StudioApp.m_helpFilePath);
    if (theFile.exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(theFile.fileName()));
}

//==============================================================================
/**
 *	Handles the ID_HELP_VISIT_QT command.  Opens the Qt Web site.
 */
void CMainFrame::OnHelpVisitQt()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.qt.io/3d-studio")));
}

//==============================================================================
/**
 *  Opens the tutorial.
 */
void CMainFrame::OnHelpOpenTutorial()
{
    StudioTutorialWidget tutorial(this);
    int welcomeRes = tutorial.exec();
    g_StudioApp.handleWelcomeRes(welcomeRes, false);
}

//==============================================================================
/**
 * Handle the file revert menu option.
 */
void CMainFrame::OnFileRevert()
{
    g_StudioApp.OnRevert();
}

void CMainFrame::OnFileImportAssets()
{
    m_paletteManager->projectView()->assetImportAction(0);
}

void CMainFrame::OnFileConnectToDevice()
{
    if (m_remoteDeploymentSender->isConnected()) {
        g_StudioApp.GetCore()->GetDispatch()->FireOnProgressBegin(
                            QObject::tr("Disconnecting from remote device..."), {});
        m_remoteDeploymentSender->disconnect();
    } else {
        QPair<QString, int> info = m_remoteDeploymentSender->initConnection();
        if (!info.first.isEmpty()) {
            g_StudioApp.GetCore()->GetDispatch()->FireOnProgressBegin(
                            QObject::tr("Connecting to remote device..."), {});
            m_remoteDeploymentSender->connect(info);
        } else {
            m_ui->action_Connect_to_Device->setChecked(false);
        }
    }
}

//==============================================================================
/**
 * Handles the recent list.
 */
void CMainFrame::OnFileOpenRecent(int nID)
{
    g_StudioApp.OnFileOpenRecent(m_recentItems->GetItem(nID));
}

//==============================================================================
/**
 *	Tells the scene view to recheck its sizing mode and tells client to update
 */
void CMainFrame::RecheckSizingMode()
{
    m_sceneView->recheckSizingMode();
}

//==============================================================================
/**
 * Callback when a Core is opened or fails to open.
 */
void CMainFrame::OnOpenDocument(const QString &inFilename, bool inSucceeded)
{
    if (inSucceeded)
        m_recentItems->AddRecentItem(inFilename);
    else
        m_recentItems->RemoveRecentItem(inFilename);
}

//==============================================================================
/**
 * Callback when a Core is saved or fails to save.
 */
void CMainFrame::OnSaveDocument(const QString &inFilename, bool inSucceeded, bool inSaveCopy)
{
    if (!inSaveCopy)
        OnOpenDocument(inFilename, inSucceeded);
}

//==============================================================================
/**
 * Callback for when a the doc gets a new path
 */
void CMainFrame::OnDocumentPathChanged(const QString &inNewPath)
{
    QFileInfo info(inNewPath);
    QString theTitle = info.fileName();
    if (theTitle.isEmpty())
        theTitle = QObject::tr("Untitled");

    theTitle.append(QStringLiteral(" - ") + QObject::tr("Qt 3D Studio"));

    // TODO: Move this whole pile to the studio app
    setWindowTitle(theTitle);

    if (info.exists())
        m_recentItems->AddRecentItem(inNewPath);
}

void CMainFrame::OnShowSlide()
{
    m_paletteManager->ShowControl(CPaletteManager::CONTROLTYPE_SLIDE);
}

void CMainFrame::OnShowTimeline()
{
    m_paletteManager->ShowControl(CPaletteManager::CONTROLTYPE_TIMELINE);
}

void CMainFrame::OnShowBasic()
{
    m_paletteManager->ShowControl(CPaletteManager::CONTROLTYPE_BASICOBJECTS);
}

void CMainFrame::OnShowProject()
{
    m_paletteManager->ShowControl(CPaletteManager::CONTROLTYPE_PROJECT);
}

void CMainFrame::OnShowAction()
{
    m_paletteManager->ShowControl(CPaletteManager::CONTROLTYPE_ACTION);
}

void CMainFrame::OnShowInspector()
{
    m_paletteManager->ShowControl(CPaletteManager::CONTROLTYPE_INSPECTOR);
}

void CMainFrame::OnShowEditPreview()
{
    bool show = CStudioPreferences::showEditModePreview();
    CStudioPreferences::setShowEditModePreview(!show);
}

void CMainFrame::OnEditViewLightingEnabled()
{
    bool enabled = CStudioPreferences::editModeLightingEnabled();
    CStudioPreferences::setEditModeLightingEnabled(!enabled);
}

void CMainFrame::OnConnectionChanged(bool connected)
{
    g_StudioApp.GetCore()->GetDispatch()->FireOnProgressEnd();
    m_ui->action_Connect_to_Device->setChecked(connected);
    m_ui->actionRemote_Preview->setEnabled(connected);
}

TimelineWidget *CMainFrame::getTimelineWidget() const
{
    WidgetControl *control = static_cast<WidgetControl *>
            (m_paletteManager->GetControl(CPaletteManager::CONTROLTYPE_TIMELINE)->widget());
    return static_cast<TimelineWidget *>(control->getControl());
}

SlideView *CMainFrame::getSlideView() const
{
    return static_cast<SlideView *>(m_paletteManager->GetControl(CPaletteManager::CONTROLTYPE_SLIDE)
                                    ->widget());
}

CRecentItems *CMainFrame::GetRecentItems()
{
    return m_recentItems.data();
}

QWidget *CMainFrame::GetActiveView()
{
    return centralWidget();
}

CPlayerWnd *CMainFrame::GetPlayerWnd() const
{
    return m_sceneView->getPlayerWnd();
}

bool CMainFrame::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ToolTip: {
        if (CStudioPreferences::ShouldShowTooltips())
            event->ignore();
        else
            return true;
        break;
    }
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Tab) {
            if (m_paletteManager->tabNavigateFocusedWidget(true))
                return true;
        } else if (ke->key() == Qt::Key_Backtab) {
            if (m_paletteManager->tabNavigateFocusedWidget(false))
                return true;
        }
        break;
    }
    default:
        break;
    }
    return QMainWindow::eventFilter(obj, event);
}

void CMainFrame::handleGeometryAndState(bool save)
{
    if (m_resettingLayout)
        return;

    QSettings settings;
    QString geoKey = QStringLiteral("mainWindowGeometry") + QString::number(STUDIO_VERSION_NUM);
    QString stateKey = QStringLiteral("mainWindowState") + QString::number(STUDIO_VERSION_NUM);
    if (save) {
        settings.setValue(geoKey, saveGeometry());
        settings.setValue(stateKey, saveState(STUDIO_VERSION_NUM));
    } else {
        // Restoring geometry and state back to back results in errors in state restoration, so
        // let's restore state asynchronously
        restoreGeometry(settings.value(geoKey).toByteArray());
        QTimer::singleShot(0, this, [this, stateKey]() {
            QSettings settings;
            restoreState(settings.value(stateKey).toByteArray(), STUDIO_VERSION_NUM);
        });
    }
}

void CMainFrame::handleRestart()
{
    QStringList presentationFile = QStringList(g_StudioApp.GetCore()->GetDoc()->GetDocumentPath());
    close();
    QProcess::startDetached(qApp->arguments()[0], presentationFile);
}

void CMainFrame::initializeGeometryAndState()
{
    QSettings settings;
    QString stateKey = QStringLiteral("mainWindowState") + QString::number(STUDIO_VERSION_NUM);
    if (!settings.contains(stateKey)) {
        // On first run, save and restore geometry and state. For some reason they are both needed
        // to avoid a bug with palettes resizing to their original size when window is resized or
        // something in a palette is edited.
        handleGeometryAndState(true);
    }
    handleGeometryAndState(false);
}

void CMainFrame::toggleSelectMode()
{
    if (m_ui->actionItem_Select_Tool->isChecked())
        m_sceneView->onToolGroupSelection();
    else
        m_sceneView->onToolItemSelection();
}

void CMainFrame::showScene()
{
    if (!m_sceneView.data()->isVisible()) {
        setCentralWidget(m_sceneView.data());
        m_sceneView.data()->setVisible(true);
    }
}
