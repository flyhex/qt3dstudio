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

#ifndef INCLUDED_STUDIO_APP_H
#define INCLUDED_STUDIO_APP_H 1

#pragma once

#include "StudioObjectTypes.h"
#include "Qt3DSImportComposerTypes.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "DispatchListeners.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSFileTools.h"
#include <QtWidgets/qapplication.h>

namespace Q3DStudio {
class IInternalDirectoryWatchingSystem;
class IDirectoryWatchingSystem;
class ITickTock;
class IStudioRenderer;
struct SSelectedValue;
}

namespace qt3dsdm {
class ISignalConnection;
}

struct StudioManipulationModes
{
    enum Enum {
        Local,
        Global,
    };
};

class CCore;
class CDialogs;
class CInspectableBase;
class CDirectoryWatchingSystemWrapper;
class CHotKeys;
class CViews;
class CMainFrame;
enum EStudioObjectType;
struct SubPresentationRecord;
class CDataInputDialogItem;

QT_FORWARD_DECLARE_CLASS(QCommandLineParser)

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

    virtual bool initInstance(const QCommandLineParser &parser);
    virtual bool run(const QCommandLineParser &parser);

    void onAppAbout();

    Q3DStudio::IDirectoryWatchingSystem &getDirectoryWatchingSystem();
    void setupTimer(long inMessageId, QWidget *inWnd);
    Q3DStudio::ITickTock &getTickTock();
    Q3DStudio::IStudioRenderer &getRenderer();
    void clearGuides();
#if (defined Q_OS_MACOS)
    void openApplication(const QString &inFilename);
#endif

public Q_SLOTS:
    void handleMessageReceived(const QString &message, QObject *socket);
    void performShutdown();

protected:
    bool runApplication();
    bool blankRunApplication();
    bool openAndRunApplication(const QString &inFilename);
    bool createAndRunApplication(const QString &filename, const QString &folder = QString(),
                                 bool isNewProject = true);
    void initCore();
    bool showStartupDialog();
    bool handleWelcomeRes(int res, bool recursive);
    QString resolvePresentationFile(const QString &inFile);

    CCore *m_core;
    bool m_isSilent; // true indicates Studio running in silent mode (no GUI)
    CViews *m_views;
    long m_toolMode;
    StudioManipulationModes::Enum m_manipulationMode; // Controls what space the tras, rot, and
                                                      // scale manipulations work in.
    long m_selectMode;
    CDialogs *m_dialogs;
    long m_playbackTime; // Stores the playhead's starting position so that it can be restored
                         //after playing the presentation for a little while
    qt3dsdm::Qt3DSDMSlideHandle m_playbackOriginalSlide; // Stores the current slide handle
                                                         // before playback started.

    std::shared_ptr<Q3DStudio::ITickTock> m_tickTock;
    std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> m_directoryWatchingSystem;
    std::shared_ptr<Q3DStudio::IStudioRenderer> m_renderer;
    bool m_authorZoom;

private:
    void playbackPreviewStart();
    void playbackPreviewEnd();

    bool m_welcomeShownThisSession;
    // are we are launching welcome screen again due to user canceling file dialog?
    bool m_goStraightToWelcomeFileDialog;
    bool m_playbackPreviewOn = false;
    bool m_isOnProgress = false;
    int m_tutorialPage;
    QTimer *m_autosaveTimer;
#if (defined Q_OS_MACOS)
    bool m_fileOpenEvent = false;
#endif

public:
    CMainFrame* m_pMainWnd;
    QWidget *m_lastActiveView = nullptr;

    CCore *GetCore();
    CViews *GetViews();
    CDialogs *GetDialogs();
    long GetToolMode();
    void SetToolMode(long inToolMode);
    long GetSelectMode();
    void SetSelectMode(long inSelectMode);

    StudioManipulationModes::Enum GetManipulationMode() const;
    void SetManipulationMode(StudioManipulationModes::Enum inManipulationMode);

    bool CanUndo();
    bool CanRedo();
    void OnCopy();
    bool CanCopy();
    QString GetCopyType();
    QString getDuplicateType() const;
    QString getDeleteType() const;
    bool canGroupSelectedObjects() const;
    bool canUngroupSelectedObjects() const;
    bool groupSelectedObjects() const;
    bool ungroupSelectedObjects() const;
    void OnCut();
    bool CanCut();
    void OnPaste();
    bool CanPaste();
    QString GetPasteType();
    void SetSelectedObjectTimebarColor();
    bool CanChangeTimebarColor();
    void HandleSetChangedKeys();
    void DeleteSelectedKeys();
    void DeleteSelectedObject();
    void HandleDuplicateCommand();
    void OnToggleAutosetKeyframes();
    void SetAutosetKeyframes(bool inFlag);
    void PlaybackPlay();
    void PlaybackStopNoRestore();
    void PlaybackRewind();
    bool IsPlaying();
    void OnRevert();
    bool CanRevert();
    void OnFileOpenRecent(const Qt3DSFile &inDocument);
    bool PerformSavePrompt();
    void PlaybackStop();
    bool isPlaybackPreviewOn() const;
    void AdvanceTime();
    void ReduceTime();
    void AdvanceUltraBigTime();
    void ReduceUltraBigTime();
    void PlaybackToggle();
    CInspectableBase *GetInspectableFromSelectable(Q3DStudio::SSelectedValue inSelectable);
    CInspectableBase *getInspectableFromInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler, QWidget *actionParent);
    bool OnSave(bool autosave = false);
    bool OnSaveAs();
    bool OnSaveCopy();
    bool OnLoadDocument(const Qt3DSFile &inDocument, bool inShowStartupDialogOnError = true);
    void OnLoadDocumentCatcher(const Qt3DSFile &inLocation);
    void OnFileOpen();
    QString OnProjectNew();
    QString OnFileNew();
    bool IsAuthorZoom() const;
    bool isOnProgress() const;
    void SetAuthorZoom(bool inZoom);
    void SetAutosaveEnabled(bool enabled);
    void SetAutosaveInterval(int interval);
    void toggleEyeball();
    void showPresentationIdUniqueWarning();
    void showInvalidFilenameWarning();
    void checkDeletedDatainputs();
    void saveDataInputsToProjectFile();
    void verifyDatainputBindings();

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
    void OnUndefinedDatainputsFail(
            const QMultiMap<QString,
                            QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                  qt3dsdm::Qt3DSDMPropertyHandle>> *map) override;

    // CPresentationChangeListener
    void OnNewPresentation() override;
    void OnPresentationModifiedExternally() override;

    Q3DStudio::CString m_pszHelpFilePath;

    QVector<SubPresentationRecord> m_subpresentations;
    QMap<QString, CDataInputDialogItem *> m_dataInputDialogItems;

    QString getRenderableId(const QString &filePath) const;
    QString getRenderableAbsolutePath(const QString &renderableId) const;
    QSize getRenderableSize(const QString &renderableId);

    QString getMostRecentDirectory() const;
    QString getMostRecentProjectParentDir() const;

    void setLastActiveView(QWidget *widget) { m_lastActiveView = widget; }
    QWidget *lastActiveView() const { return m_lastActiveView; }
};

extern CStudioApp g_StudioApp;

#endif // INCLUDED_STUDIO_APP_H
