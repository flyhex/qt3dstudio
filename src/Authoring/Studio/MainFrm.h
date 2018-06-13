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
#ifndef INCLUDED_MAIN_FRAME
#define INCLUDED_MAIN_FRAME 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "PreviewHelper.h"
#include "DispatchListeners.h"

#include <QtWidgets/qmainwindow.h>
#include <QtCore/qtimer.h>

//==============================================================================
//	Forwards
//==============================================================================
class CHotKeys;
class CPaletteManager;
class CRecentItems;
class CSceneView;
class CStudioApp;
class ITimelineTimebar;
class RemoteDeploymentSender;
class TimelineWidget;
class CStudioPreferencesPropSheet;

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainFrame;
}
QT_END_NAMESPACE

class CPlayerWnd;

class CMainFrame : public QMainWindow,
        public CPresentationChangeListener,
        public CFileOpenListener,
        public CClientPlayChangeListener
{
    Q_OBJECT
public:
    CMainFrame();
    virtual ~CMainFrame();

    void OnNewPresentation() override;
    void OnClosingPresentation() override;

    // CFileOpenListener
    void OnOpenDocument(const Qt3DSFile &inFilename, bool inSucceeded) override;
    void OnSaveDocument(const Qt3DSFile &inFilename, bool inSucceeded, bool inSaveCopy) override;
    void OnDocumentPathChanged(const Qt3DSFile &inNewPath) override;

    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler, QWidget *actionParent);
    void RecheckSizingMode();

    // CClientPlayChangeListener
    void OnPlayStart() override;
    void OnPlayStop() override;
    void OnTimeChanged(long inTime) override;

    CRecentItems *GetRecentItems();

    void OnCreate();

    void onPlaybackTimeout();

    void OnFileOpen();
    void OnFileSave();
    void OnUpdateFileSave();
    void OnFileSaveAs();
    void OnFileSaveCopy();
    void OnProjectNew();
    void OnFileNew();
    void OnFileRevert();
    void OnFileImportAssets();
    void OnFileConnectToDevice();
    void OnFileOpenRecent(int nID);

    void OnEditRedo();
    void OnEditUndo();
    void OnUpdateEditUndo();
    void OnUpdateEditRedo();
    void OnEditCopy();
    void OnUpdateEditCopy();
    void OnEditCut();
    void OnUpdateEditCut();
    void OnEditPaste();
    void onEditPasteToMaster();
    void OnUpdateEditPaste();
    void OnEditDuplicate();
    void onEditDelete();
    void onEditGroup();

    void timerEvent(QTimerEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void OnUpdateTimelineSetTimeBarColor();
    void OnTimelineSetTimeBarColor();
    void OnTimelineSetChangedKeyframe();
    void OnUpdateTimelineDeleteSelectedKeyframes();
    void OnTimelineSetTimeBarText();
    void OnUpdateTimelineSetTimeBarText();
    void OnUpdateTimelineSetInterpolation();
    void OnTimelineSetInterpolation();
    void closeEvent(QCloseEvent *event) override;
    void OnToolAutosetkeys();
    void OnUpdateToolAutosetkeys();
    void OnEditApplicationPreferences();
    void OnEditPresentationPreferences();
    void OnFileDataInputs();
    void OnPlaybackPlay();
    void OnUpdatePlaybackPlay();
    void OnPlaybackRewind();
    void OnUpdatePlaybackRewind();
    void OnPlaybackStop();
    void OnUpdatePlaybackStop();
    void OnPlaybackPreview(const QString &viewerExeName, bool remote = false);
    void OnPlaybackPreviewRuntime1();
    void OnPlaybackPreviewRuntime2();
    void OnPlaybackPreviewRemote();
    void OnUpdatePlaybackPreview();
    void OnUpdateToolMove();
    void OnUpdateToolRotate();
    void OnUpdateToolScale();
    void OnUpdateToolGlobalManipulators();
    void OnToolMove();
    void OnToolRotate();
    void OnToolScale();
    void OnToolGlobalManipulators();
    void OnUpdateToolChange();
    void OnUpdateToolGroupSelection();
    void OnUpdateToolItemSelection();

    void OnViewBoundingBoxes();
    void OnUpdateViewBoundingBoxes();
    void OnViewPivotPoint();
    void OnUpdateViewPivotPoint();
    void OnViewWireframe();
    void OnUpdateViewWireframe();
    void OnViewHelpPalette();
    void OnUpdateViewHelpPalette();
    void OnUpdateViewTooltips();
    void OnViewTooltips();
    void OnUpdateHelpIndex();
    void OnHelpIndex();
    void OnHelpVisitQt();
    void OnHelpOpenTutorial();

    void onViewResetLayout();
    void OnViewAction();
    void OnUpdateViewAction();
    void OnViewBasicObjects();
    void OnUpdateViewBasicObjects();
    void OnViewInspector();
    void OnUpdateViewInspector();
    void OnViewProject();
    void OnUpdateViewProject();
    void OnViewSlide();
    void OnUpdateViewSlide();
    void OnViewTimeline();
    void OnUpdateViewTimeline();

    void OnEditCameraZoomExtent();
    void OnEditCameraPan();
    void OnEditCameraRotate();
    void OnEditCameraZoom();
    void OnUpdateCameraZoomExtentAndAuthorZoom();
    void OnUpdateEditCameraPan();
    void OnUpdateEditCameraRotate();
    void OnUpdateEditCameraZoom();
    void OnEditViewFillMode();
    void OnUpdateEditViewFillMode();

    void OnViewGuidesRulers();
    void OnUpdateViewGuidesRulers();
    void OnClearGuides();
    void OnUpdateClearGuides();
    void OnLockGuides();
    void OnUpdateLockGuides();

    void OnShowSlide();
    void OnShowTimeline();
    void OnShowBasic();
    void OnShowProject();
    void OnShowAction();
    void OnShowInspector();
    void OnShowEditPreview();
    void OnUpdateCameraPreview();

    void OnConnectionChanged(bool);

    void onCtrlNPressed();

    TimelineWidget *getTimelineWidget() const;

    void EditPreferences(short inPageIndex);

    void HandleEditViewFillModeKey();
    void HandleEditCameraZoomExtent();

    QWidget *GetActiveView();
    CPlayerWnd *GetPlayerWnd() const;

    void initializeGeometryAndState();

    void toggleSelectMode();
    void showScene();

Q_SIGNALS:
    void playStateChanged(bool started);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void handleGeometryAndState(bool save);
    void handleRestart();

    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::MainFrame)> m_ui;
    QScopedPointer<RemoteDeploymentSender> m_remoteDeploymentSender;
    QScopedPointer<CSceneView> m_sceneView;
    QScopedPointer<CRecentItems> m_recentItems;
    QScopedPointer<CPaletteManager> m_paletteManager;
    QScopedPointer<QTimer> m_updateUITimer;
    QScopedPointer<QTimer> m_playbackTimer;
    QScopedPointer<CStudioPreferencesPropSheet> m_propSheet;

    bool m_playbackFlag = false;
    bool m_resettingLayout = false;
};

#endif // INCLUDED_MAIN_FRAME
