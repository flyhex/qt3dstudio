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
#include "stdafx.h"
#include "MainFrm.h"
#include "ui_MainFrm.h"

#include "StudioConst.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Bindings/TimelineTranslationManager.h"
#include "Bindings/UICDMTimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"
#include "SceneView.h"
#include "Strings.h"
#include "StringLoader.h"
#include "StudioApp.h"
#include "TimelineControl.h"
#include "UICOptions.h"
#include "UICColor.h"

#include "Doc.h"
#include "IKeyframesManager.h"
#include "Dispatch.h"
#include "Dialogs.h"
#include "StudioPreferencesPropSheet.h"
#include "StudioProjectSettings.h"
#include "StudioPreferences.h"
#include "Views.h"
#include "HotKeys.h"
#include "RecentItems.h"
#include "PaletteManager.h"
#include "Core.h"
#include "ITickTock.h"
#include "IStudioRenderer.h"
#include "SubPresentationsDlg.h"
#include "StudioTutorialWidget.h"
#include "remoteproject.h"

#include "InspectorControlView.h"

#include <QtGui/qevent.h>
#include <QtGui/qdesktopservices.h>
#include <QtWidgets/qdockwidget.h>
#include <QtCore/qsettings.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>
#include <QtCore/qdir.h>

// Constants
const long PLAYBACK_TIMER_TIMEOUT = 10; // 10 milliseconds

#ifdef KDAB_TEMPORARILY_REMOVED
ON_MESSAGE(WM_STUDIO_MESSAGE_ROUTER, OnMsgRouterMsg)
#endif

//==============================================================================
/**
 * Constructor
 */
CMainFrame::CMainFrame()
    : m_ui(new Ui::MainFrame)
{
    m_ui->setupUi(this);

    OnCreate();

    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddFileOpenListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddClientPlayChangeListener(this);
    g_StudioApp.SetupTimer(WM_STUDIO_TIMER, this);

    m_remoteProject = new RemoteProject(this);

    // File Menu
    connect(m_ui->action_Open, &QAction::triggered, this, &CMainFrame::OnFileOpen);
    connect(m_ui->action_Save, &QAction::triggered, this, &CMainFrame::OnFileSave);
    connect(m_ui->action_New, &QAction::triggered, this, &CMainFrame::OnFileNew);
    connect(m_ui->actionSave_As, &QAction::triggered, this, &CMainFrame::OnFileSaveAs);
    connect(m_ui->actionSave_a_Copy, &QAction::triggered, this, &CMainFrame::OnFileSaveCopy);
    connect(m_ui->action_Revert, &QAction::triggered, this, &CMainFrame::OnFileRevert);
    connect(m_ui->action_Connect_to_Device, &QAction::triggered, this,
        &CMainFrame::OnFileConnectToDevice);
    connect(m_remoteProject, &RemoteProject::connectionChanged,
        m_ui->action_Connect_to_Device, &QAction::setChecked);
    connect(m_ui->action_Exit, &QAction::triggered, this, &CMainFrame::close);

    m_RecentItems = new CRecentItems(m_ui->menuRecent_Projects, 0);
    connect(m_RecentItems, &CRecentItems::openRecent, this, &CMainFrame::OnFileOpenRecent);
    //ON_COMMAND_RANGE(WM_STUDIO_OPEN_RECENT_MIN, WM_STUDIO_OPEN_RECENT_MAX, OnFileOpenRecent)

    // Edit Menu
    connect(m_ui->action_Undo, &QAction::triggered, this, &CMainFrame::OnEditUndo);
    connect(m_ui->action_Redo, &QAction::triggered, this, &CMainFrame::OnEditRedo);
    connect(m_ui->action_Copy, &QAction::triggered, this, &CMainFrame::OnEditCopy);
    connect(m_ui->action_Cut, &QAction::triggered, this, &CMainFrame::OnEditCut);
    connect(m_ui->actionAutoset_Keyframes, &QAction::triggered, this, &CMainFrame::OnToolAutosetkeys);
    connect(m_ui->action_Paste, &QAction::triggered, this, &CMainFrame::OnEditPaste);
    connect(m_ui->actionStudio_Preferences, &QAction::triggered, this, &CMainFrame::OnEditApplicationPreferences);
    connect(m_ui->actionPresentation_Settings, &QAction::triggered, this, &CMainFrame::OnEditPresentationPreferences);
    connect(m_ui->actionSubpresentations, &QAction::triggered, this,
            &CMainFrame::OnEditSubPresentations);
    connect(m_ui->action_Duplicate_Object, &QAction::triggered, this, &CMainFrame::OnEditDuplicate);

    // Timeline Menu
    connect(m_ui->actionDelete_Selected_Keyframe_s, &QAction::triggered, this, &CMainFrame::OnTimelineDeleteSelectedKeyframes);
    connect(m_ui->actionSet_Interpolation, &QAction::triggered, this, &CMainFrame::OnTimelineSetInterpolation);
    connect(m_ui->actionChange_Time_Bar_Color, &QAction::triggered, this, &CMainFrame::OnTimelineSetTimeBarColor);
    connect(m_ui->actionSet_Changed_Keyframes, &QAction::triggered, this, &CMainFrame::OnTimelineSetChangedKeyframe);

    // View Menu
    connect(m_ui->actionBounding_Boxes, &QAction::triggered, this, &CMainFrame::OnViewBoundingBoxes);
    connect(m_ui->actionPivot_Point, &QAction::triggered, this, &CMainFrame::OnViewPivotPoint);
    connect(m_ui->actionWireframe, &QAction::triggered, this, &CMainFrame::OnViewWireframe);
    connect(m_ui->actionTooltips, &QAction::triggered, this, &CMainFrame::OnViewTooltips);

    // Tools Menu
    connect(m_ui->actionTimeline, &QAction::triggered, this, &CMainFrame::OnViewTimeline);
    connect(m_ui->actionAction, &QAction::triggered, this, &CMainFrame::OnViewAction);
    connect(m_ui->actionBasic_Objects, &QAction::triggered, this, &CMainFrame::OnViewBasicObjects);
    connect(m_ui->actionProject, &QAction::triggered, this, &CMainFrame::OnViewProject);
    connect(m_ui->actionSlide, &QAction::triggered, this, &CMainFrame::OnViewSlide);
    connect(m_ui->actionInspector, &QAction::triggered, this, &CMainFrame::OnViewInspector);

    // Help Menu
    connect(m_ui->action_Reference_Manual, &QAction::triggered, this, &CMainFrame::OnHelpIndex);
    connect(m_ui->action_Visit_Qt_Web_Site, &QAction::triggered, this, &CMainFrame::OnHelpVisitQt);
    connect(m_ui->action_About_Qt_3D_Studio, &QAction::triggered, []() { g_StudioApp.OnAppAbout(); });
    connect(m_ui->action_Open_Tutorial, &QAction::triggered, this, &CMainFrame::OnHelpOpenTutorial);


    connect(m_ui->actionItem_Select_Tool, &QAction::triggered, m_SceneView, &CSceneView::OnToolItemSelection);
    connect(m_ui->actionGroup_Select_Tool, &QAction::triggered, m_SceneView, &CSceneView::OnToolGroupSelection);

    // Playback toolbar
    connect(m_ui->actionPlay, &QAction::triggered, this, &CMainFrame::OnPlaybackPlay);
    connect(m_ui->actionRewind, &QAction::triggered, this, &CMainFrame::OnPlaybackRewind);
    connect(m_ui->actionStop, &QAction::triggered, this, &CMainFrame::OnPlaybackStop);
    connect(m_ui->actionPreview, &QAction::triggered, this, &CMainFrame::OnPlaybackPreview);

    // Tool mode toolbar
    connect(m_ui->actionPosition_Tool, &QAction::triggered, this, &CMainFrame::OnToolMove);
    connect(m_ui->actionRotation_Tool, &QAction::triggered, this, &CMainFrame::OnToolRotate);
    connect(m_ui->actionScale_Tool, &QAction::triggered, this, &CMainFrame::OnToolScale);
    connect(m_ui->actionLocal_Global_Manipulators, &QAction::triggered, this, &CMainFrame::OnToolGlobalManipulators);

    // Edit Camera toolbar
    connect(m_ui->actionFit_Selected, &QAction::triggered, this, &CMainFrame::OnEditCameraZoomExtent);
    connect(m_ui->actionPan_Tool, &QAction::triggered, this, &CMainFrame::OnEditCameraPan);
    connect(m_ui->actionOrbit_Tool, &QAction::triggered, this, &CMainFrame::OnEditCameraRotate);
    connect(m_ui->actionZoom_Tool, &QAction::triggered, this, &CMainFrame::OnEditCameraZoom);
    connect(m_ui->actionShading_Mode, &QAction::triggered, this, &CMainFrame::OnEditViewFillMode);
    connect(m_ui->actionRulers_Guides, &QAction::triggered, this, &CMainFrame::OnViewGuidesRulers);
    connect(m_ui->actionClear_Guides, &QAction::triggered, this, &CMainFrame::OnClearGuides);
    connect(m_ui->actionLock_Guides, &QAction::triggered, this, &CMainFrame::OnLockGuides);

    // TODO: better solution?
    QTimer* updateUITimer = new QTimer;
    updateUITimer->start(500);
    connect(updateUITimer, &QTimer::timeout, [&]() {
        if (QApplication::activeWindow() != this)
            return;

        OnUpdateFileSave();
        OnUpdateEditUndo();
        OnUpdateEditRedo();
        OnUpdateEditCopy();
        OnUpdateEditCut();
        OnUpdateToolAutosetkeys();
        OnUpdateEditPaste();
        OnUpdateEditDuplicate();
        OnUpdateTimelineDeleteSelectedKeyframes();
        OnUpdateTimelineSetInterpolation();
        OnUpdateTimelineSetTimeBarColor();
        OnUpdateViewBoundingBoxes();
        OnUpdateViewPivotPoint();
        OnUpdateViewWireframe();
        OnUpdateViewTooltips();
        OnUpdateViewTimeline();
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
    });

    m_playbackTimer.setInterval(PLAYBACK_TIMER_TIMEOUT);
    connect(&m_playbackTimer, &QTimer::timeout, this, &CMainFrame::onPlaybackTimeout);
    qApp->installEventFilter(this);
}

//==============================================================================
/**
 * Destructor
 */
CMainFrame::~CMainFrame()
{
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

    QSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());
}

/**
 * Called when the main frame is actually created.  Sets up tool bars and default
 * views.
 */
int CMainFrame::OnCreate()
{
    m_SceneView = new CSceneView(&g_StudioApp, this);
    connect(m_SceneView, &CSceneView::toolChanged, this, &CMainFrame::OnUpdateToolChange);

#ifdef INCLUDE_EDIT_CAMERA
    // tell the edit camera bar about this scene view
    m_ui->m_EditCamerasBar->SetSceneView(m_SceneView);
#endif

    // Newly launched, the file dialog for open and import should default to more recent
    // opened/imported
    CDialogs *theDialogs = g_StudioApp.GetDialogs();
    // this must NOT be in 'command line' mode
    if (theDialogs) {
        Q3DStudio::CString theMostRecentOpen;
        if (m_RecentItems && m_RecentItems->GetItemCount() > 0)
            theMostRecentOpen = m_RecentItems->GetItem(0).GetPath();
        if (theMostRecentOpen.IsEmpty()) // default to exe
            theMostRecentOpen = CUICFile::GetApplicationDirectory().GetPath();

        theDialogs->ResetSettings(theMostRecentOpen);
    }
#ifdef KDAB_TEMPORARILY_REMOVED

        // Change the background color for the menu bar
        if (IsMenu(theMenu)) {
            MENUINFO theMenuInfo = { 0 };
            theMenuInfo.cbSize = sizeof(MENUINFO);
            theMenuInfo.fMask = MIM_BACKGROUND;

            CBrush *theNewBrush = new CBrush();
            CColor theBaseColor(CStudioPreferences::GetMenuBarBaseColor());
            theNewBrush->CreateSolidBrush(
                RGB(theBaseColor.GetRed(), theBaseColor.GetGreen(), theBaseColor.GetBlue()));
            theMenuInfo.hbrBack = *theNewBrush; // Brush you want to draw

            SetMenuInfo(theMenu, &theMenuInfo);
        }
   #endif

    // Create the view manager
    m_PaletteManager = new CPaletteManager(this);

    // Remove basic toolbar (open, save, undo/redo, etc.)
    // Kept in ui form in case it is going to be added back later on.
    delete m_ui->toolBar;

    setCentralWidget(m_SceneView);
    return 0;
}

#ifdef KDAB_TEMPORARILY_REMOVED

// If we want things to look good then on the *first* paint we want to actually
// paint over the background.  I don't know why, but if you were to paint
// the entire background on every message you would get some flashing.
void CMainFrame::OnPaint()
{
    static bool needsPaint = true;
    if (needsPaint) {
        needsPaint = false;
        RECT theRect;
        ::GetClientRect(GetSafeHwnd(), &theRect);
        CBrush theBrush;
        ::CColor theColor = CStudioPreferences::GetBaseColor();
        DWORD theColorRef = RGB(theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
        theBrush.CreateSolidBrush(theColor);
        CPaintDC dc(this);
        dc.FillRect(&theRect, &theBrush);
    }
    CFrameWnd::OnPaint();
}
//==============================================================================
void CMainFrame::OnInitMenu(CMenu *inMenu)
{
    ReleaseFocus();
    // Emulate a Control ModifierUp on the Sceneview to "unstick" the Alt-Key/Scale Tool
    m_SceneView->HandleModifierUp(CHotKeys::KEY_MENU, 0, 0);
    CFrameWnd::OnInitMenu(inMenu);
}

::CString CMainFrame::m_WindowClass;

::CString CMainFrame::GetWindowClass()
{
    if (m_WindowClass.GetLength() == 0) {
        ::CColor theColor = CStudioPreferences::GetBaseColor();
        DWORD theRefColor = RGB(theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
        HBRUSH theBrush = ::CreateSolidBrush(theRefColor);
        m_WindowClass = AfxRegisterWndClass(0, 0, theBrush, 0);
    }
    return m_WindowClass;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT &cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
        return FALSE;

    // TODO: Modify the Window class or styles here by modifying
    cs.lpszClass = GetWindowClass();
    return TRUE;
}

BOOL CMainFrame::DestroyWindow()
{
    // Save before we destroy anything
    SaveLayout();

    SAFE_DELETE(m_PaletteManager);

    g_StudioApp.GetCore()->GetDispatch()->RemovePresentationChangeListener(this);
    g_StudioApp.GetCore()->GetDispatch()->RemoveFileOpenListener(this);
    g_StudioApp.GetCore()->GetDispatch()->RemoveClientPlayChangeListener(this);

    SAFE_DELETE(m_RecentItems);

    // In order to help debug why image lists sometimes crash on shutdown
    // This will help identify whcih cone is causing the problem
    m_ClientToolsImageList.DeleteImageList();
    m_ClientToolsImageListHot.DeleteImageList();
    m_ClientToolsImageListDisabled.DeleteImageList();
    m_PlaybackImageList.DeleteImageList();
    m_PlaybackImageListHot.DeleteImageList();
    m_PlaybackImageListDisabled.DeleteImageList();
    m_EditCameraToolsImageList.DeleteImageList();
    m_EditCameraToolsImageListHot.DeleteImageList();
    m_EditCameraToolsImageListDisabled.DeleteImageList();

    return CFrameWnd::DestroyWindow();
}
#endif

//==============================================================================
/**
 * Called when a new presenation is created.  We have to wait to associate the
 * scene object with the scene view until this point, because the scene object
 * does not exist until this function gets called.
 */
void CMainFrame::OnNewPresentation()
{
    // TODO - CN needs to fix this
    // Associate the scene object with the scene view
    m_ui->m_EditCamerasBar->SetupCameras();
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
 *	Store all the information regarding the layout of the main frame and its palettes.
 *	This is used to save the current position, so when the app is restarted the
 *	window will come back to it's previous location.
 *	On one hand, there is the main frame which continues to use the original
 *	CPaletteState scheme.  Now there is also the control bars/palettes that dock.
 *	These all now use the MFC path for serialization to the registry.
 *	This operation used to occur every time there was a change to any window, now
 *	the final state is only saved when the app is closed.
 */
void CMainFrame::SaveLayout()
{
    if (m_PaletteManager)
        m_PaletteManager->Save();

#ifdef KDAB_TEMPORARILY_REMOVED
    // Only save the window position if we're not minimized
    if (!IsIconic()) {
        CPaletteState theState("MainWindow");

        theState.SetMaximized(IsZoomed() == TRUE);
        if (!IsZoomed()) {
            /*
            SDJ: 12/17/03 - From MSDN
            The coordinates used in a WINDOWPLACEMENT structure should be used only by the
            GetWindowPlacement and SetWindowPlacement functions. Passing coordinates
            to functions which expect screen coordinates (such as SetWindowPos) will result
            in the window appearing in the wrong location. For example, if the taskbar is at
            the top of the screen, saving window coordinates using GetWindowPlacement and
            restoring them using SetWindowPos causes the window to appear to "creep" up the screen.

            We were using GetWindowPlacement with MoveWindow (in SetLayout) which was causing
            "creep" when the taskbar was at the top of the screen. Replaced GetWindowPlacement
            with the following GetWindowRect call.
            */

            CRect theRect;
            GetWindowRect(&theRect);

            theState.SetPosition(CPt(theRect.left, theRect.top));
            theState.SetSize(CPt(theRect.right - theRect.left, theRect.bottom - theRect.top));
        }
        theState.SetVisible(IsWindowVisible() ? true : false);

        theState.SaveState();

        // Force a recalc of the layout to ensure that everything is correct with the docking
        RecalcLayout(TRUE);
    }
#endif
}

//==============================================================================
/**
 *	Restore all the information regarding the layout of the main frame and its palettes.
 *	This will make the window be located at the last stored location.
 *	On one hand, there is the main frame which continues to use the original
 *	CPaletteState scheme.  Now there is also the control bars/palettes that dock.
 *	These all now use the MFC path for serialization to the registry.  This happens
 *	on startup.
 */
void CMainFrame::RestoreLayout()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CPaletteState theState("MainWindow");
    theState.LoadState();

    SetLayout(theState);

    if (!m_PaletteManager->Load())
        m_PaletteManager->RestoreDefaults(true);

    // Force a recalc for the layout for the docking palettes to work
    RecalcLayout(TRUE);
#endif
}

//==============================================================================
/**
 * Set the state of this window to the specified state.
 */
void CMainFrame::SetLayout(const CPaletteState &inState)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CPt thePosition = inState.GetPosition();
    CPt theSize = inState.GetSize();
    bool isVisible = inState.IsVisible();

    CRect theWindowRect(thePosition.x, thePosition.y, thePosition.x + theSize.x,
                        thePosition.y + theSize.y);

    // Relocate the window to the new location
    MoveWindow(theWindowRect);

    if (inState.IsMaximized())
        ::AfxGetApp()->m_nCmdShow = SW_SHOWMAXIMIZED;
    ShowWindow(::AfxGetApp()->m_nCmdShow);
    ShowWindow((isVisible ? SW_SHOW : SW_HIDE));
#endif
    // UpdateWindow();
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
    const QString undoDescription = QObject::tr("Undo %1\tCtrl+Z").arg(g_StudioApp.GetCore()->GetCmdStack()->GetUndoDescription());
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
    const QString redoDescription = QObject::tr("Redo %1\tCtrl+Y").arg(g_StudioApp.GetCore()->GetCmdStack()->GetRedoDescription());
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
    if (g_StudioApp.CanCopy()) {
        Q3DStudio::CString theDescription;
        theDescription.Format(::LoadResourceString(IDS_MENU_WIN_COPY_FORMAT),
                              static_cast<const wchar_t *>(g_StudioApp.GetCopyType()));

        m_ui->action_Copy->setText(theDescription.toQString());
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
        Q3DStudio::CString theDescription;
        theDescription.Format(::LoadResourceString(IDS_MENU_WIN_CUT_FORMAT),
                              static_cast<const wchar_t *>(g_StudioApp.GetCopyType()));

        m_ui->action_Cut->setText(theDescription.toQString());
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
        Q3DStudio::CString theUndoDescription;
        theUndoDescription.Format(::LoadResourceString(IDS_MENU_WIN_PASTE_FORMAT),
                                  static_cast<const wchar_t *>(g_StudioApp.GetPasteType()));

        m_ui->action_Paste->setText(theUndoDescription.toQString());

        m_ui->action_Paste->setEnabled(true);
    } else {
        m_ui->action_Paste->setEnabled(false);
    }
}

#ifdef KDAB_TEMPORARILY_REMOVED
//=============================================================================
/**
 * Router for forwarding messages onto the MsgRouter.
 * This is used for posting messages between threads.
 */
LRESULT CMainFrame::OnMsgRouterMsg(WPARAM inWParam, LPARAM inLParam)
{
    CMsgRouter::GetInstance()->HandleMessage(inWParam, inLParam);

    return TRUE;
}
#endif

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
    m_ui->actionLocal_Global_Manipulators->setChecked(g_StudioApp.GetMinpulationMode()
                                                      == StudioManipulationModes::Global);

#ifdef INCLUDE_EDIT_CAMERA
    m_ui->actionPan_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_CAMERA_PAN);
    m_ui->actionOrbit_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_CAMERA_ROTATE);
    m_ui->actionZoom_Tool->setChecked(theToolMode == STUDIO_TOOLMODE_CAMERA_ZOOM);

/*CEditCameraContainer* theEditCameras = g_StudioApp.GetCore()->GetDoc()->GetEditCameraContainer( );
CSEditCamera* theActiveEditCamera = theEditCameras->GetActiveEditCamera( );
bool theIsEditView = !m_SceneView->IsDeploymentView( );
m_EditCamerasBar.GetToolBarCtrl( ).EnableButton( ID_TOOL_EDITCAMERA_ROTATE, ( theActiveEditCamera &&
theActiveEditCamera->Is3D( ) ) );
m_EditCamerasBar.GetToolBarCtrl( ).EnableButton( ID_TOOL_EDITCAMERA_PAN, theIsEditView );
m_EditCamerasBar.GetToolBarCtrl( ).EnableButton( ID_TOOL_EDITCAMERA_ZOOM, theIsEditView );
m_EditCamerasBar.GetToolBarCtrl( ).EnableButton( ID_EDITCAMERA_ZOOMEXTENT, theIsEditView );

m_EditCamerasBar.GetToolBarCtrl( ).EnableButton( ID_TOOL_EDITVIEW_FILLMODE, theIsEditView );
m_EditCamerasBar.GetToolBarCtrl( ).CheckButton( ID_TOOL_EDITVIEW_FILLMODE, theIsEditView &&
theEditCameras->GetEditViewFillMode( ) );
*/
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
    ITimelineTimebar *theTimelineTimebar = GetSelectedTimelineTimebar();
    if (theTimelineTimebar != NULL) {
        CColor theColor = theTimelineTimebar->GetTimebarColor();

        if (g_StudioApp.GetDialogs()->PromptObjectTimebarColor(theColor))
            theTimelineTimebar->SetTimebarColor(theColor);
    }
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
 *	OnTimelineDeleteSelectedKeyframes: Handles the ID_TIMELINE_DELETESELECTEDKEYFRAMES
 *	message.
 *
 *	Deletes all currently selected keyframes.
 */
void CMainFrame::OnTimelineDeleteSelectedKeyframes()
{
    g_StudioApp.DeleteSelectedKeys();
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
    m_ui->actionDelete_Selected_Keyframe_s->setEnabled(g_StudioApp.CanCopy());
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
    m_ui->actionSet_Interpolation->setEnabled(g_StudioApp.GetCore()->GetDoc()->GetKeyframesManager()->HasSelectedKeyframes());
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

//==============================================================================
/**
 *	OnUpdateEditDuplicate: Handles the UPDATE COMMAND UI message for this menu item.
 *
 *	If the currently selected object is not null, and it is not in the library,
 *	then the user can duplicate the object and the menu item is enabled.  Otherwise,
 *	it is disabled.
 */
void CMainFrame::OnUpdateEditDuplicate()
{
    m_ui->action_Duplicate_Object->setEnabled(g_StudioApp.CanDuplicateObject());
}

//=============================================================================
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
//=============================================================================
/**
 * Command handler for the File Save As menu option.
 * This will prompt the user for a location to save the file out to then
 * will perform the save.
 */
void CMainFrame::OnFileSaveAs()
{
    g_StudioApp.OnSaveAs();
}

//=============================================================================
/**
 * Command handler for the File Save a Copy menu option.
 * This will prompt the user for a location to save the file out to then
 * save a copy, leaving the original file open in the editor.
 */
void CMainFrame::OnFileSaveCopy()
{
    g_StudioApp.OnSaveCopy();
}

//=============================================================================
/**
 * Command handler for the New Document menu option.
 * This will create a new default document.
 */
void CMainFrame::OnFileNew()
{
    g_StudioApp.OnFileNew();
}


//==============================================================================
/**
 * Overrides the close method to prompt if the document is modified.
 */
void CMainFrame::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());
    QMainWindow::closeEvent(event);

    if (g_StudioApp.GetCore()->GetDoc()->IsModified()) {
        CDialogs::ESavePromptResult theResult = g_StudioApp.GetDialogs()->PromptForSave();
        if (theResult == CDialogs::SAVE_FIRST) {
            // If the save was canceled or failed then do not exit.
            if (!g_StudioApp.OnSave())
                return;
        }
        // On cancel ditch out of here and abort exit. Abort! Abort!
        else if (theResult == CDialogs::CANCEL_OPERATION) {
            event->ignore();
            return;
        }
    }

    // Tell the app to shutdown, do it here so it does not rely on static destructor.
    g_StudioApp.PerformShutdown();
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
 *  Displays the preferences dialog and can change subpresentation settings.
 */
void CMainFrame::OnEditSubPresentations()
{
    QString dir = g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory().toQString();

    CSubPresentationsDlg dlg(QDir::toNativeSeparators(dir),
                             g_StudioApp.m_subpresentations);
    dlg.exec();
    if (dlg.result() == QDialog::Accepted) {
        g_StudioApp.m_subpresentations = dlg.subpresentations();
        g_StudioApp.SaveUIAFile();
    }
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
    CStudioPreferencesPropSheet thePropSheet(::LoadResourceString(IDS_STUDIOPREFSTITLE).toQString(),
                                             this, inPageIndex);

    // CStudioProjectSettings *theProjectSettings = g_StudioApp.GetCore()->GetStudioProjectSettings();

    // Q3DStudio::CString theAuthorName theAuthorName = theProjectSettings->GetAuthor();
    // Q3DStudio::CString theCompanyName theCompanyName = theProjectSettings->GetCompany();

    // Display the CStudioPreferencesPropSheet
    int thePrefsReturn = thePropSheet.exec();

    // Ugly hack to fix some bug
    m_SceneView->OnEditCameraChanged();

    if (thePrefsReturn == PREFS_RESET_DEFAULTS) {
        // Restore default values
        g_StudioApp.SetAutosetKeyframes(true);
        CStudioPreferences::SetBoundingBoxesOn(true);
        CStudioPreferences::SetTimebarDisplayTime(true);
        g_StudioApp.GetCore()->GetDoc()->SetDefaultKeyframeInterpolation(true);
        CStudioPreferences::SetNudgeAmount(10.0f);
        CStudioPreferences::SetSnapRange(10);
        CStudioPreferences::SetTimelineSnappingGridActive(true);
        CStudioPreferences::SetTimelineSnappingGridResolution(SNAPGRID_SECONDS);
// Edit Cameras
#ifdef INCLUDE_EDIT_CAMERA
        CColor theDefaultBgColor(CStudioPreferences::EDITVIEW_DEFAULTBGCOLOR);
        CStudioPreferences::SetEditViewBackgroundColor(theDefaultBgColor);
        // g_StudioApp.GetCore()->GetDoc()->SetEditViewBackgroundColor( theDefaultBgColor );
        CStudioPreferences::SetPreferredStartupView(
            CStudioPreferences::PREFERREDSTARTUP_DEFAULTINDEX);
#endif

        if (m_PaletteManager)
            m_PaletteManager->RestoreDefaults();

        RecheckSizingMode();

        // Remove keys related to the ObjRef and PropRef Pickers
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("ObjectReferenceWidth");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("ObjectReferenceHeight");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("ObjectReferenceXPos");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("ObjectReferenceYPos");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("PropertyReferenceWidth");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("PropertyReferenceHeight");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("PropertyReferenceXPos");
        CPreferences::GetUserPreferences(::LoadResourceString(IDS_PREFS_REFERENCECONTROLS))
            .RemoveKey("PropertyReferenceYPos");

        // Also clear out the viewer's settings file
        Q3DStudio::CFilePath theFilePath(Q3DStudio::CFilePath::CombineBaseAndRelative(
            Q3DStudio::CFilePath::GetUserApplicationDirectory(),
            "UIComposer\\UICViewerSettings.txt"));
        theFilePath.DeleteThisFile();
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
    m_ui->actionPlay->setChecked(true);

    if (m_PlaybackFlag == false) {
        m_PlaybackFlag = true;

        // g_StudioApp.GetCore()->GetDoc()->SetPlayMode( PLAYMODE_PLAY );

        m_playbackTimer.start();
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
    m_ui->actionPlay->setChecked(false);

    if (m_PlaybackFlag == true) {
        m_PlaybackFlag = false;
        m_playbackTimer.stop();
        // g_StudioApp.GetCore()->GetDoc()->SetPlayMode( PLAYMODE_STOP );
    }
}

//==========================================================================
/**
 *	Called when the presentation time changes.  Not handled by this class,
 *	but included because the base class requires it to be implemented.
 */
void CMainFrame::OnTimeChanged(long inTime)
{
    Q_UNUSED(inTime);
    // Do nothing
}

//==============================================================================
/**
 *	Handles pressing the play button.
 */
void CMainFrame::OnPlaybackPlay()
{
    g_StudioApp.PlaybackPlay();
}

//==============================================================================
/**
 *	Handles pressing of the stop button.
 */
void CMainFrame::OnPlaybackStop()
{
    g_StudioApp.PlaybackStopNoRestore();
}

//==============================================================================
/**
 *	Handles pressing the preview button.
 */
void CMainFrame::OnPlaybackPreview()
{
    if (m_remoteProject->isConnected())
        CPreviewHelper::OnDeploy(*m_remoteProject);
    else
        CPreviewHelper::OnPreview();
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
void CMainFrame::RegisterGlobalKeyboardShortcuts(CHotKeys *inHotKeys)
{
    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnToolMove),
                                0, Qt::Key_W);
    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnToolRotate),
                                0, Qt::Key_R);
    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnToolScale),
                                0, Qt::Key_E);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnViewBoundingBoxes),
        Qt::ControlModifier, Qt::Key_B);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnViewPivotPoint),
        Qt::ControlModifier | Qt::AltModifier, Qt::Key_P);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnEditPresentationPreferences),
        Qt::ControlModifier, Qt::Key_P);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnOpenMostRecentlyUsedDocument),
        Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier, Qt::Key_P);

    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnShowAction),
                                Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_A);
    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnShowBasic),
                                Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_B);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnShowInspector),
        Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_I);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnShowProject),
        Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_P);
    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnShowSlide),
                                Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_D);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnShowTimeline),
        Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_T);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnViewGuidesRulers),
        Qt::ControlModifier, Qt::Key_Semicolon);
    inHotKeys->RegisterKeyEvent(new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnLockGuides),
                                Qt::ControlModifier | Qt::AltModifier, Qt::Key_Semicolon);

#ifdef INCLUDE_EDIT_CAMERA
    inHotKeys->RegisterKeyDownEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::HandleEditViewFillModeKey), 0,
        Qt::Key_F3);
    inHotKeys->RegisterKeyEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::HandleEditCameraZoomExtent), 0, Qt::Key_F);
#endif

    inHotKeys->RegisterKeyDownEvent(
        new CDynHotKeyConsumer<CMainFrame>(this, &CMainFrame::OnPlaybackPreview), 0,
        Qt::Key_F5);

    m_SceneView->RegisterGlobalKeyboardShortcuts(inHotKeys);

    CTimelineControl *theTimelineControl = GetTimelineControl();
    if (theTimelineControl)
        theTimelineControl->RegisterGlobalKeyboardShortcuts(inHotKeys);
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
    m_ui->actionPosition_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_MOVE && m_ui->actionPosition_Tool->isEnabled());
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
    m_ui->actionRotation_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_ROTATE && m_ui->actionRotation_Tool->isEnabled());
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
    m_ui->actionScale_Tool->setChecked(theCurrentToolSettings == STUDIO_TOOLMODE_SCALE && m_ui->actionScale_Tool->isEnabled());
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
    StudioManipulationModes::Enum theMode = g_StudioApp.GetMinpulationMode();

    // If the current tool mode matches this button
    // If the button is currently enabled
    m_ui->actionLocal_Global_Manipulators->setChecked(theMode == StudioManipulationModes::Global && m_ui->actionLocal_Global_Manipulators->isEnabled());
}

//==============================================================================
/**
 *	OnToolMove: Called when the Move button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CMainFrame::OnToolMove()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
    m_SceneView->SetToolMode(STUDIO_TOOLMODE_MOVE);
}

//==============================================================================
/**
 *	OnToolRotate: Called when the Rotate button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CMainFrame::OnToolRotate()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_ROTATE);
    m_SceneView->SetToolMode(STUDIO_TOOLMODE_ROTATE);
}

//==============================================================================
/**
 *	OnToolScale: Called when the Scale button is pressed.
 *	Sets the current tool mode and changes the cursor.
 */
void CMainFrame::OnToolScale()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_SCALE);
    m_SceneView->SetToolMode(STUDIO_TOOLMODE_SCALE);
}

void CMainFrame::OnToolGlobalManipulators()
{
    if (m_ui->actionLocal_Global_Manipulators->isChecked())
        g_StudioApp.SetMinpulationMode(StudioManipulationModes::Global);
    else
        g_StudioApp.SetMinpulationMode(StudioManipulationModes::Local);

    g_StudioApp.GetRenderer().RequestRender();
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
    m_ui->actionGroup_Select_Tool->setChecked(theCurrentSelectSettings == STUDIO_SELECTMODE_GROUP && m_ui->actionGroup_Select_Tool->isEnabled());
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
    m_ui->actionItem_Select_Tool->setChecked(theCurrentSelectSettings == STUDIO_SELECTMODE_ENTITY && m_ui->actionItem_Select_Tool->isEnabled());
}

//==============================================================================
/**
 *	Called when the edit camera Zoom Extent button is pressed.
 *	Inform the current active edit camera to toggle itself.
 */
void CMainFrame::OnEditCameraZoomExtent()
{
    if (g_StudioApp.GetRenderer().GetEditCamera() >= 0)
        g_StudioApp.GetRenderer().EditCameraZoomToFit();
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
    m_SceneView->SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
}

//==============================================================================
/**
 *	Called when the Rotate Edit Camera button is pressed.
 *	Inform the current active edit camera on their tool mode.
 */
void CMainFrame::OnEditCameraRotate()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
    m_SceneView->SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
}

//==============================================================================
/**
 *	Called when the Zoom Edit Camera button is pressed.
 *	Inform the current active edit camera on their tool mode.
 */
void CMainFrame::OnEditCameraZoom()
{
    g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ZOOM);
    m_SceneView->SetToolMode(STUDIO_TOOLMODE_CAMERA_ZOOM);
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
    if (m_SceneView == GetActiveView() && !m_SceneView->IsDeploymentView()) {
        m_ui->actionFit_Selected->setChecked(false);
    } else {
        m_ui->actionFit_Selected->setChecked(g_StudioApp.IsAuthorZoom());
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
void CMainFrame::OnUpdateEditCameraPan()
{
    if (m_SceneView == GetActiveView() && !m_SceneView->IsDeploymentView()) {
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
    if (m_SceneView == GetActiveView() && !m_SceneView->IsDeploymentView()) {
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
    if (m_SceneView == GetActiveView() && !m_SceneView->IsDeploymentView()) {
        OnEditViewFillMode();
        bool theEditViewFillMode = g_StudioApp.GetRenderer().IsEditLightEnabled();
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
    bool theEditViewFillMode = !g_StudioApp.GetRenderer().IsEditLightEnabled();
    g_StudioApp.GetRenderer().SetEnableEditLight(theEditViewFillMode);
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
    if (m_SceneView == GetActiveView() && !m_SceneView->IsDeploymentView()) {
        m_ui->actionShading_Mode->setEnabled(true);
        m_ui->actionShading_Mode->setChecked(g_StudioApp.GetRenderer().IsEditLightEnabled());
    } else {
        m_ui->actionShading_Mode->setEnabled(false);
        m_ui->actionShading_Mode->setChecked(false);
    }
}

void CMainFrame::OnViewGuidesRulers()
{
    g_StudioApp.GetRenderer().SetGuidesEnabled(!g_StudioApp.GetRenderer().AreGuidesEnabled());
    g_StudioApp.GetCore()->GetDispatch()->FireAuthorZoomChanged();
    m_SceneView->OnRulerGuideToggled();
}

void CMainFrame::OnUpdateViewGuidesRulers()
{
    m_ui->actionRulers_Guides->setEnabled(m_SceneView->IsDeploymentView());
    m_ui->actionRulers_Guides->setChecked(g_StudioApp.GetRenderer().AreGuidesEnabled());
}

void CMainFrame::OnClearGuides()
{
    g_StudioApp.ClearGuides();
}

void CMainFrame::OnUpdateClearGuides()
{
    bool enable = g_StudioApp.GetRenderer().AreGuidesEnabled()
        && g_StudioApp.GetRenderer().AreGuidesEditable() && m_SceneView->IsDeploymentView();

    m_ui->actionClear_Guides->setEnabled(enable);
}

void CMainFrame::OnLockGuides()
{
    g_StudioApp.GetRenderer().SetGuidesEditable(!g_StudioApp.GetRenderer().AreGuidesEditable());
}

void CMainFrame::OnUpdateLockGuides()
{
    bool enable = g_StudioApp.GetRenderer().AreGuidesEnabled() && m_SceneView->IsDeploymentView();
    m_ui->actionLock_Guides->setEnabled(enable);
    // Set to the inverse of guides editable.
    m_ui->actionLock_Guides->setChecked(!g_StudioApp.GetRenderer().AreGuidesEditable());
}

void CMainFrame::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == WM_STUDIO_TIMER)
        g_StudioApp.GetTickTock().ProcessMessages();
    QMainWindow::timerEvent(event);
}

void CMainFrame::OnViewAction()
{
    m_PaletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_ACTION);
    SaveLayout();
}

void CMainFrame::OnUpdateViewAction()
{
    m_ui->actionAction->setChecked(
        m_PaletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_ACTION));
}

void CMainFrame::OnViewBasicObjects()
{
    m_PaletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_BASICOBJECTS);
    SaveLayout();
}

void CMainFrame::OnUpdateViewBasicObjects()
{
    m_ui->actionBasic_Objects->setChecked(m_PaletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_BASICOBJECTS));
}

void CMainFrame::OnViewInspector()
{
    m_PaletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_INSPECTOR);
    SaveLayout();
}

void CMainFrame::OnUpdateViewInspector()
{
    m_ui->actionInspector->setChecked(
        m_PaletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_INSPECTOR));
}

void CMainFrame::OnViewProject()
{
    m_PaletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_PROJECT);
    SaveLayout();
}

void CMainFrame::OnUpdateViewProject()
{
    m_ui->actionProject->setChecked(
        m_PaletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_PROJECT));
}

void CMainFrame::OnViewSlide()
{
    m_PaletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_SLIDE);
    SaveLayout();
}

void CMainFrame::OnUpdateViewSlide()
{
    m_ui->actionSlide->setChecked(
        m_PaletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_SLIDE) ? TRUE : FALSE);
}

//==============================================================================
/**
 *	Called when the View Inspector Palette menu item is chosen.
 */
void CMainFrame::OnViewTimeline()
{
    m_PaletteManager->ToggleControl(CPaletteManager::CONTROLTYPE_TIMELINE);
    SaveLayout();
}

//==============================================================================
/**
 *	Checks or unchecks the menu item depending on if the view is available or not.
 *	@param pCmdUI Pointer to the UI element that generated the message.
 */
void CMainFrame::OnUpdateViewTimeline()
{
    m_ui->actionTimeline->setChecked(
        m_PaletteManager->IsControlVisible(CPaletteManager::CONTROLTYPE_TIMELINE));
}

//==============================================================================
/**
 *	Called when the View Inspector Palette menu item is chosen.
 */
void CMainFrame::OnViewBoundingBoxes()
{
    CStudioPreferences::SetBoundingBoxesOn(!CStudioPreferences::IsBoundingBoxesOn());
    g_StudioApp.GetRenderer().RequestRender();
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
    g_StudioApp.GetRenderer().RequestRender();
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
    g_StudioApp.GetRenderer().RequestRender();
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
    CUICFile theFile(g_StudioApp.m_pszHelpFilePath);
    m_ui->action_Reference_Manual->setEnabled(theFile.Exists());
}

//==============================================================================
/**
 *	Handles the ID_HELP_INDEX command.  Opens the online help for Studio.
 */
void CMainFrame::OnHelpIndex()
{
    CUICFile theFile(g_StudioApp.m_pszHelpFilePath);
    if (theFile.Exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(theFile.GetAbsolutePath().toQString()));
}

//==============================================================================
/**
 *	Handles the ID_HELP_VISIT_QT command.  Opens the Qt Web site.
 */
void CMainFrame::OnHelpVisitQt()
{
    Q3DStudio::CString theWebSite(::LoadResourceString(IDS_HELP_VISIT_QT));
    QDesktopServices::openUrl(QUrl(theWebSite.toQString()));
}

//==============================================================================
/**
 *  Opens the tutorial.
 */
void CMainFrame::OnHelpOpenTutorial()
{
    StudioTutorialWidget tutorial(false);
    tutorial.exec();
}

//==============================================================================
/**
 *	OnHelpBehaviorReference: Handles the ID_HELP_BEHAVIORREFERENCE message.
 *	Called when the Behavior Reference menu item is chosen inside the Help menu.
 *	Opens the HTML file to display the Behavior help.
 */
void CMainFrame::OnHelpBehaviorReference()
{
    CUICFile theFile(CUICFile::GetApplicationDirectory(),
                     ::LoadResourceString(IDS_HELP_BEHAVIORREFERENCE));
    if (theFile.Exists()) {
        g_StudioApp.GetCore()->GetDispatch()->FireOnNavigate(
                    Q3DStudio::CString::fromQString(theFile.GetURL().path()), true);
        SaveLayout();
    }
}

//==============================================================================
/**
 * Handle the file revert menu option.
 */
void CMainFrame::OnFileRevert()
{
    g_StudioApp.OnRevert();
}

void CMainFrame::OnFileConnectToDevice()
{
    if (m_remoteProject->isConnected())
        m_remoteProject->disconnect();
    else
        m_remoteProject->connect();
}

//==============================================================================
/**
 * Handles the recent list.
 */
void CMainFrame::OnFileOpenRecent(int nID)
{
    g_StudioApp.OnFileOpenRecent(m_RecentItems->GetItem(nID));
}

//==============================================================================
/**
 *	Tells the scene view to recheck its sizing mode and tells client to update
 */
void CMainFrame::RecheckSizingMode()
{
    m_SceneView->RecheckSizingMode();
}

//==============================================================================
/**
 * Callback when a Core is opened or fails to open.
 */
void CMainFrame::OnOpenDocument(const CUICFile &inFilename, bool inSucceeded)
{
    if (inSucceeded)
        m_RecentItems->AddRecentItem(inFilename);
    else
        m_RecentItems->RemoveRecentItem(inFilename);
}

//==============================================================================
/**
 * Callback when a Core is saved or fails to save.
 */
void CMainFrame::OnSaveDocument(const CUICFile &inFilename, bool inSucceeded, bool inSaveCopy)
{
    if (!inSaveCopy)
        OnOpenDocument(inFilename, inSucceeded);
}

//==============================================================================
/**
 * Callback for when a the doc gets a new path
 */
void CMainFrame::OnDocumentPathChanged(const CUICFile &inNewPath)
{
    QString theTitle = inNewPath.GetName().toQString();
    if (theTitle.isEmpty())
        theTitle = QObject::tr("Untitled");

    theTitle = theTitle + " - " + QObject::tr("Qt 3D Studio");

    // TODO: Move this whole pile to the studio app
    setWindowTitle(theTitle);

    if (inNewPath.Exists())
        m_RecentItems->AddRecentItem(inNewPath);
}

//==============================================================================
/**
 *	Secret hot-key compatibility with Aftereffects muscle memory
 */
void CMainFrame::OnOpenMostRecentlyUsedDocument()
{
    CUICFile theDocument = m_RecentItems->GetItem(0);
    if (theDocument.Exists())
        g_StudioApp.OnLoadDocument(theDocument);
}

void CMainFrame::OnShowSlide()
{
    m_PaletteManager->ShowControl(CPaletteManager::CONTROLTYPE_SLIDE);
}

void CMainFrame::OnShowTimeline()
{
    m_PaletteManager->ShowControl(CPaletteManager::CONTROLTYPE_TIMELINE);
}

void CMainFrame::OnShowBasic()
{
    m_PaletteManager->ShowControl(CPaletteManager::CONTROLTYPE_BASICOBJECTS);
}

void CMainFrame::OnShowProject()
{
    m_PaletteManager->ShowControl(CPaletteManager::CONTROLTYPE_PROJECT);
}

void CMainFrame::OnShowAction()
{
    m_PaletteManager->ShowControl(CPaletteManager::CONTROLTYPE_ACTION);
}

void CMainFrame::OnShowInspector()
{
    m_PaletteManager->ShowControl(CPaletteManager::CONTROLTYPE_INSPECTOR);
}

CTimelineControl *CMainFrame::GetTimelineControl()
{
    return m_PaletteManager->GetTimelineControl();
}

ITimelineTimebar *CMainFrame::GetSelectedTimelineTimebar()
{
    CUICDMTimelineItemBinding *theTimelineItemBinding =
        GetTimelineControl()->GetTranslationManager()->GetSelectedBinding();
    if (theTimelineItemBinding == NULL)
        return NULL;

    return theTimelineItemBinding->GetTimelineItem()->GetTimebar();
}

CRecentItems *CMainFrame::GetRecentItems()
{
    return m_RecentItems;
}

QWidget *CMainFrame::GetActiveView()
{
    return centralWidget();
}

bool CMainFrame::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Tab) {
            if (m_PaletteManager->tabNavigateFocusedWidget(true))
                return true;
        } else if (ke->key() == Qt::Key_Backtab) {
            if (m_PaletteManager->tabNavigateFocusedWidget(false))
                return true;
        }
        break;
    }
    default:
        break;
    }
    return QMainWindow::eventFilter(obj, event);
}
