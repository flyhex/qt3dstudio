/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef INCLUDED_STUDIO_APP_H
#define INCLUDED_STUDIO_APP_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "CmdLineParser.h"
#include "StudioObjectTypes.h"
#include "DispatchListeners.h"
#include "Qt3DSDMHandles.h"

#include <QtWidgets/qapplication.h>

namespace Q3DStudio {
class IInternalDirectoryWatchingSystem;
class IDirectoryWatchingSystem;
class ITickTock;
class IStudioRenderer;
struct SSelectedValue;
};

namespace qt3dsdm {
class ISignalConnection;
};

struct StudioManipulationModes
{
    enum Enum {
        Local,
        Global,
    };
};

//==============================================================================
//	Forwards
//==============================================================================
class CCore;
class CDialogs;
class CSplashView;
class CInspectableBase;
class CDirectoryWatchingSystemWrapper;
class CHotKeys;
class CViews;
class CMainFrame;
enum EStudioObjectType;
struct SubPresentationRecord;
class CDataInputDialogItem;

class CStudioApp : public QObject,
        public CCoreAsynchronousEventListener,
        public CAppStatusListener,
        public CFailListener,
        public CPresentationChangeListener // to setup auto set keyframes
{
    Q_OBJECT
public:
    CStudioApp();
    virtual ~CStudioApp();

    // Overrides
public:
    virtual bool InitInstance(int argc, char *argv[]);
    virtual int Run();

    void OnAppAbout();

public:
    void PerformShutdown();
    Q3DStudio::IDirectoryWatchingSystem &GetDirectoryWatchingSystem();
    void SetupTimer(long inMessageId, QWidget *inWnd);
    Q3DStudio::ITickTock &GetTickTock();
    Q3DStudio::IStudioRenderer &GetRenderer();
    void ClearGuides();

protected:
    int RunApplication();
    int BlankRunApplication();
    int RunCmdLineTests(const Q3DStudio::CString &inTestArgs);
    int RunSystemTests(const Q3DStudio::CString &inTestArgs);
    int OpenAndRunApplication(const Q3DStudio::CString &inFilename);
    void InitCore();
    bool ShowStartupDialog();
    bool HandleWelcomeRes(int res, bool recursive);

    CCore *m_Core;
    CSplashView *m_SplashPalette; ///< Startup splash palette
    QString m_OldHelpFilePath; ///< Stores a pointer to the old
    CCmdLineParser m_CmdLineParser; ///< Stores and returns execution modes
    int m_UnitTestResults; ///< 0 on success; 1 on failure
    bool m_IsSilent; ///< true indicates Studio running in silent mode (no GUI)
    CViews *m_Views;
    long m_ToolMode;
    StudioManipulationModes::Enum m_ManipulationMode; ///< Controls what space the tras, rot, and
                                                      /// scale manipulations work in.
    long m_SelectMode;
    CDialogs *m_Dialogs;
    long m_PlaybackTime; ///< Stores the playhead's starting position so that it can be restored
                         ///after playing the presentation for a little while
    qt3dsdm::Qt3DSDMSlideHandle m_PlaybackOriginalSlide; ///< Stores the current slide handle
                                                         /// before playback started.

    std::shared_ptr<Q3DStudio::ITickTock> m_TickTock;
    std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> m_DirectoryWatchingSystem;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_DirectoryWatcherTicker;
    std::shared_ptr<Q3DStudio::IStudioRenderer> m_Renderer;
    bool m_AuthorZoom;

private:
    bool m_welcomeShownThisSession;
    // are we are launching welcome screen again due to
    // user canceling file dialog?
    bool m_goStraightToWelcomeFileDialog;
    int m_tutorialPage;
public:
    CMainFrame* m_pMainWnd;

    CCore *GetCore();
    CViews *GetViews();
    CDialogs *GetDialogs();
    long GetToolMode();
    void SetToolMode(long inToolMode);
    long GetSelectMode();
    void SetSelectMode(long inSelectMode);

    StudioManipulationModes::Enum GetMinpulationMode() const;
    void SetMinpulationMode(StudioManipulationModes::Enum inManipulationMode);

    bool CanUndo();
    bool CanRedo();
    void OnCopy();
    bool CanCopy();
    QString GetCopyType();
    void OnCut();
    bool CanCut();
    void OnPaste();
    bool CanPaste();
    QString GetPasteType();
    void SetSelectedObjectTimebarColor();
    bool CanChangeTimebarColor();
    void HandleSetChangedKeys();
    void DeleteSelectedKeys();
    void HandleDuplicateCommand();
    bool CanDuplicateObject();
    void OnToggleAutosetKeyframes();
    void SetAutosetKeyframes(bool inFlag);
    void PlaybackPlay();
    void PlaybackStopNoRestore();
    void PlaybackRewind();
    void OnRevert();
    bool CanRevert();
    void OnFileOpenRecent(const Qt3DSFile &inDocument);
    bool PerformSavePrompt();
    void PlaybackStop();
    void AdvanceTime();
    void ReduceTime();
    void AdvanceUltraBigTime();
    void ReduceUltraBigTime();
    void PlaybackToggle();
    CInspectableBase *GetInspectableFromSelectable(Q3DStudio::SSelectedValue inSelectable);
    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler, QWidget *actionParent);
    bool OnSave();
    bool OnSaveAs();
    bool OnSaveCopy();
    bool OnLoadDocument(const Qt3DSFile &inDocument, bool inShowStartupDialogOnError = true);
    void OnLoadDocumentCatcher(const Qt3DSFile &inLocation);
    void OnFileOpen();
    void OnFileNew();
    bool IsAuthorZoom();
    void SetAuthorZoom(bool inZoom);

    // CCoreAsynchronousEventListener
    void OnAsynchronousCommand(CCmd *inCmd) override;

    // CAppStatusListener
    void OnDisplayAppStatus(Q3DStudio::CString &inStatusMsg) override;
    void OnProgressBegin(const Q3DStudio::CString &inActionText,
                         const Q3DStudio::CString &inAdditionalText) override;
    void OnProgressEnd() override;

    // CFailListener
    void OnAssetDeleteFail() override;
    void OnPasteFail() override;
    void OnBuildconfigurationFileParseFail(const Q3DStudio::CString &inMessage) override;
    void OnSaveFail(bool inKnownError) override;
    void OnProjectVariableFail(const Q3DStudio::CString &inMessage) override;
    void OnErrorFail(const Q3DStudio::CString &inText) override;
    void OnRefreshResourceFail(const Q3DStudio::CString &inResourceName,
                               const Q3DStudio::CString &inDescription) override;

    // CPresentationChangeListener
    void OnNewPresentation() override;
    void OnPresentationModifiedExternally() override;

    Q3DStudio::CString m_pszHelpFilePath;

    QVector<SubPresentationRecord> m_subpresentations;
    QVector<CDataInputDialogItem *> m_dataInputDialogItems;

    void SaveUIAFile(bool subpresentations = true);
};

extern CStudioApp g_StudioApp;

#endif // INCLUDED_STUDIO_APP_H
