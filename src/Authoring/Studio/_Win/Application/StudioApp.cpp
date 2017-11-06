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
#include "stdafx.h"

#ifdef _WIN32
#pragma warning(disable : 4100) // unreferenced formal parameter
#endif
#include "StudioApp.h"
#include "SubPresentationsDlg.h"
#include "Qt3DSStateApplication.h"

#include <QtGui/qsurfaceformat.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qurl.h>
#include <QtGui/qopenglcontext.h>
#include <QtWidgets/qaction.h>

int main(int argc, char *argv[])
{
    // to enable QOpenGLWidget to work on macOS, we must set the default
    // QSurfaceFormat before QApplication is created. Otherwise context-sharing
    // fails and QOpenGLWidget breaks.
    // Creating QOpenGLContext requires QApplication so it needs to be created
    // beforehand then.

    // init runtime static resources
    Q_INIT_RESOURCE(res);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // fortunately, we know which OpenGL version we can use on macOS, so we
    // can simply hard-code it here.
#if defined(Q_OS_MACOS)
    QSurfaceFormat openGL33Format;
    openGL33Format.setRenderableType(QSurfaceFormat::OpenGL);
    openGL33Format.setProfile(QSurfaceFormat::CoreProfile);
    openGL33Format.setMajorVersion(3);
    openGL33Format.setMinorVersion(3);
    openGL33Format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(openGL33Format);

    QApplication guiApp(argc, argv);
#else
    QApplication guiApp(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QScopedPointer<QOpenGLContext> context(new QOpenGLContext());
    context->setFormat(format);
    context->create();
    if (context->isOpenGLES()) {
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setMajorVersion(2);
        format.setMinorVersion(0);
        QSurfaceFormat::setDefaultFormat(format);
    }
#endif

    // Load and apply stylesheet for the application
    QFile styleFile(":/style.qss");
    styleFile.open(QFile::ReadOnly);
    guiApp.setStyleSheet(styleFile.readAll());
    g_StudioApp.InitInstance(argc, argv);
    return g_StudioApp.Run();
}

//==============================================================================
//	Includes
//==============================================================================
#include "Exceptions.h"
#include "IOLibraryException.h"
#include "Strings.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "Views.h"
#include "StringLoader.h"
#include "Doc.h"
#include "Dialogs.h"
#include "Dispatch.h"
#include "StartupDlg.h"
#include "RecentItems.h"
#include "StudioPreferences.h"
#include "MsgRouter.h"
#include "SplashView.h"
#include "Views.h"
#include "Qt3DSFile.h"
#include "Qt3DSFileTools.h"
#include "ITickTock.h"
#include "IStudioRenderer.h"
#include "IDocumentEditor.h"
#include "StudioUtils.h"

#include "IObjectReferenceHelper.h"
#include "ClientDataModelBridge.h"
#include "CommonConstants.h"
#include "IOLibraryException.h"

#include "Qt3DSDMErrors.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <QApplication>
#include <QSettings>

#ifdef KDAB_TEMPORARILY_REMOVED
#include "..\Build\versionnumber.h"
#endif

#include "Qt3DSDESKey.h" // g_DESKey

#include "Core.h"
#include "HotKeys.h"
#include "StudioTutorialWidget.h"
#include "GuideInspectable.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMInspectable.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMMaterialInspectable.h"
#include "Qt3DSDMSceneInspectable.h"
#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMDataCore.h"
#include "IDirectoryWatchingSystem.h"
#include "ITickTock.h"
#include "Qt3DSFileTools.h"
#include "foundation/Qt3DSLogging.h"

#ifdef USE_LICENSE_HANDLER
#include "licensehandler.h"
#endif

CStudioApp g_StudioApp;
long g_ErrorCode = 0;

using namespace Q3DStudio;

#ifndef WIN32
namespace qt3ds
{
void NVAssert(const char *exp, const char *file, int line, bool *igonore)
{
    qFatal("NVAssertion thrown %s(%d): %s", file, line, exp);
}
}
#endif

//=============================================================================
/**
 * Constructor
 */
CStudioApp::CStudioApp()
    : m_Core(NULL)
    , m_SplashPalette(nullptr)
    , m_UnitTestResults(0)
    , m_IsSilent(false)
    , m_Views(NULL)
    , m_ToolMode(STUDIO_TOOLMODE_MOVE)
    , m_ManipulationMode(StudioManipulationModes::Local)
    , m_SelectMode(STUDIO_SELECTMODE_GROUP)
    , m_Dialogs(NULL)
    , m_PlaybackTime(0)
    , m_AuthorZoom(false)
    , m_welcomeShownThisSession(false)
    , m_goStraightToWelcomeFileDialog(false)
    , m_tutorialPage(0)
{
}

//=============================================================================
/**
 * Destructor
 */
CStudioApp::~CStudioApp()
{
    delete m_SplashPalette;
    m_SplashPalette = nullptr;
    delete m_Views;
    m_Views = nullptr;
    delete m_Dialogs;
    m_Dialogs = nullptr;
    delete m_Core;
    m_Core = nullptr;
    // Do not call PerformShutdown from here as the C has already been shutdown (!!)
}

void CStudioApp::PerformShutdown()
{
    m_DirectoryWatcherTicker = std::shared_ptr<qt3dsdm::ISignalConnection>();

    // Dispatch un-registration
    if (m_Core) {
        m_Core->GetDispatch()->RemoveAppStatusListener(this);
        m_Core->GetDispatch()->RemoveCoreAsynchronousEventListener(this);
        qCInfo(qt3ds::TRACE_INFO) << "Studio exiting successfully";
    }

    if (m_Renderer) {
        m_Renderer->Close();
        m_Renderer = std::shared_ptr<Q3DStudio::IStudioRenderer>();
    }

    delete m_SplashPalette;
    m_SplashPalette = nullptr;
    delete m_Views;
    m_Views = nullptr;
    delete m_Dialogs;
    m_Dialogs = nullptr;
    delete m_Core;
    m_Core = nullptr;

    CStringLoader::UnloadStrings();

    // Get rid of the temp files
    Qt3DSFile::ClearCurrentTempCache();

    qApp->exit();
}

//=============================================================================
/**
 * Entry location for the creation of this application.
 * This creates the all the views, then returns if everything
 * was successful.
 */
bool CStudioApp::InitInstance(int argc, char* argv[])
{
    QApplication::setOrganizationName("The Qt Company");
    QApplication::setOrganizationDomain("qt.io");
    QApplication::setApplicationName("Qt 3D Studio");
    QApplication::setApplicationVersion(
                QString::fromWCharArray(CStudioPreferences::GetVersionString().c_str()));

    qCInfo(qt3ds::TRACE_INFO) << "Studio: " << QApplication::applicationFilePath();

    // Load the strings used by the app gui
    // This needs to occur prior to parsing command line arguments
    // since access to the strings are required by some of the cases
    Qt3DSFile theResDir(CString::fromQString(resourcePath() + QStringLiteral("/strings")));
    CStringLoader::LoadStrings(theResDir);
    qCInfo(qt3ds::TRACE_INFO) << "Version: "
                              << CStudioPreferences::GetVersionString().GetCharStar();

    std::vector<wchar_t*> wargv;
    wargv.resize(argc);
    for (int i = 0; i < argc; ++i) {
        QString arg = argv[i];
        wargv[i] = new wchar_t[arg.size() + 1];
        wargv[i][arg.size()] = L'\0';
        arg.toWCharArray(wargv[i]);
    }

    // Parse the command line so we know what's up
    m_CmdLineParser.ParseArguments(argc, &(wargv[0]));

    // Silent mode indicates that Studio will be operated in a muted "no-GUI" mode
    m_IsSilent = m_CmdLineParser.IsSilent();

    // If we're just running unit tests, return before creating windows/MFC controls
    if (m_CmdLineParser.IsRunUnitTests()) {
        {
            RunCmdLineTests(m_CmdLineParser.GetFilename());
            return false; // return false so we bail from loading the app
        }
    }

#ifdef KDAB_TEMPORARILY_REMOVED
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need
    // Change the registry key under which our settings are stored
    Q3DStudio::CString theRegistryKey =
        ::LoadResourceString(IDS_UICOMPOSER_PALETTE_SETTINGS_REGISTRY_KEY);
    SetRegistryKey(theRegistryKey);
#endif

    CFilePath thePreferencesPath = CFilePath::GetUserApplicationDirectory();
    thePreferencesPath = CFilePath::CombineBaseAndRelative(
        thePreferencesPath, CFilePath(L"Qt3DSComposer\\Preferences.setting"));
    CPreferences::SetPreferencesFile(thePreferencesPath);

#ifdef KDAB_TEMPORARILY_REMOVED
    // Set up the path to the help file
    InitHelpSystem();
#endif

    CStudioPreferences::LoadPreferences();

    m_Dialogs = new CDialogs(!m_IsSilent);

    if (!m_IsSilent) {
        // Show the splash screen
        m_SplashPalette = new CSplashView();
        m_SplashPalette->setWindowTitle(::LoadResourceString(IDS_PROJNAME).toQString());
        m_SplashPalette->show();

        m_Views = new CViews(this);
    }

    m_Core = new CCore();
    GetRenderer();
    m_Core->GetDoc()->SetSceneGraph(m_Renderer);

    // Dispatch registration
    m_Core->GetDispatch()->AddAppStatusListener(this);
    m_Core->GetDispatch()->AddCoreAsynchronousEventListener(this);

    return true;
}

//=============================================================================
/**
 *	Exit location for the destruction of this application.
 *	@return 0 on success; -1 on failure
 */
int CStudioApp::ExitInstance()
{
    int theResult = -1;

#ifdef KDAB_TEMPORARILY_REMOVED
    // CWinApp::InitInstance returns 1(TRUE) on success; 0(FALSE) on failure
    if (CWinApp::InitInstance() == TRUE)
        theResult = 0;

    if (m_CmdLineParser.IsRunUnitTests() && 0 == m_UnitTestResults)
        theResult = m_UnitTestResults; // unit tests return 0 on success; 1 on failure

#ifdef _DEBUG
    // If appears that MFC takes over dumping memory leaks on exit
    // this is the only way I've been able to turn it off.

    // Turn off dumping CRT memory leaks
    ::_CrtSetReportMode(_CRT_WARN, 0);
    ::_CrtSetReportMode(_CRT_ERROR, 0);
    ::_CrtSetReportMode(_CRT_ASSERT, 0);
#endif

    // Make sure any GDI resources have been cleaned up
    CResourceCache::GetInstance()->Clear();
    // Called in conjuction with Gdiplus::GdiplusStartup
    Gdiplus::GdiplusShutdown(m_pGdiToken);
#endif

    return theResult;
}

//=============================================================================
/**
 * Command handler to display the about dialog.
 */
void CStudioApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.exec();
}

//=============================================================================
/**
 *	Main application execution loop.
 *	The application's main thread stays in this until the app exits.
 *	@return 0 on success; -1 on failure
 */
int CStudioApp::Run()
{
    int theRetVal = -1;

#ifdef USE_LICENSE_HANDLER
    LicenseHandler lh;
    if (!lh.handleLicense())
        return theRetVal;
#endif

    try {
        CCmdLineParser::EExecutionMode theMode = m_CmdLineParser.PopExecutionMode();
        if (CCmdLineParser::END_OF_CMDS == theMode)
            theMode = CCmdLineParser::NORMAL;

        for (; CCmdLineParser::END_OF_CMDS != theMode;
             theMode = m_CmdLineParser.PopExecutionMode()) {
            // This just switches the execution mode of the app and starts it in the correct state.
            switch (theMode) {
            case CCmdLineParser::TEST_CMD_LINE:
                theRetVal = RunSystemTests(m_CmdLineParser.GetFilename());
                break;
            case CCmdLineParser::OPEN_FILE:
                theRetVal = OpenAndRunApplication(m_CmdLineParser.GetFilename());
                break;
            default:
                theRetVal = BlankRunApplication();
                break;
            }

            // if any operations returned a bad value, stop following operations
            if (-1 == theRetVal)
                break;
        }
        PerformShutdown();
    } catch (Qt3DSExceptionClass &inException) {
        g_ErrorCode = inException.GetErrorCode();
        throw;
    } catch (qt3dsdm::Qt3DSDMError &uicdmError) {
        Q_UNUSED(uicdmError);

#ifdef KDAB_TEMPORARILY_REMOVED
        EXCEPTION_POINTERS *pExPtrs;
        CStackOps::GetExceptionPointers(1, &pExPtrs);
        StudioUnhandledCrashHandler(pExPtrs);
#endif
        exit(1);
    } catch (...) {
        throw;
    }

    return theRetVal;
}

bool CStudioApp::HandleWelcomeRes(int res, bool recursive)
{
    int theReturn = true;
    switch (res) {
    case StudioTutorialWidget::createNewResult: {
        std::pair<Qt3DSFile, bool> theFile = m_Dialogs->
            GetNewDocumentChoice(Q3DStudio::CString("."));
        if (theFile.first.GetPath() != "") {
            m_Core->OnNewDocument(theFile.first, theFile.second);
            theReturn = true;
            m_welcomeShownThisSession = true;
        } else {
            // User Cancels the dialog. Show the welcome screen.
            if (recursive) {
                m_welcomeShownThisSession = false;
                m_goStraightToWelcomeFileDialog = true;
                theReturn = ShowStartupDialog();
            } else {
                theReturn = false;
            }
        }
    } break;

    case StudioTutorialWidget::openSampleResult: {
        // Try three options:
        // - open a specific example directory with .uip file in it
        // - failing that, show the main example root dir
        // - failing all previous, show Qt3DStudio dir
        Q3DStudio::CFilePath filePath;

        filePath = Qt3DSFile::GetApplicationDirectory().GetPath()+
                Q3DStudio::CString("../examples/qmldynamickeyframes/presentation");

        if (!filePath.Exists()) {
            filePath = Qt3DSFile::GetApplicationDirectory().GetPath()+
                    Q3DStudio::CString("../examples");
        }
        if (!filePath.Exists()) {
            filePath =  Qt3DSFile::GetApplicationDirectory().GetPath()+
                    Q3DStudio::CString(".");
        }

        Qt3DSFile theFile = m_Dialogs->GetFileOpenChoice(filePath);

        if (theFile.GetPath() != "") {
            OnLoadDocument(theFile);
            theReturn = true;
            m_welcomeShownThisSession = true;
        } else {
            // User Cancels the dialog. Show the welcome screen.
            if (recursive) {
                m_welcomeShownThisSession = false;
                m_goStraightToWelcomeFileDialog = true;
                theReturn = ShowStartupDialog();
            } else {
                theReturn = false;
            }
        }
    } break;

    default:
        ASSERT(false); // Should not reach this block.
        theReturn = false;
        break;
    }
    return theReturn;
}

//=============================================================================
/**
 * Show startup dialog and perform necessary action such as create new doc or load doc.
 * Return false if user requests to exit
 */
bool CStudioApp::ShowStartupDialog()
{
    int welcomeRes = QDialog::Rejected;
    bool theReturn = true;

    if (!m_welcomeShownThisSession){
        m_welcomeShownThisSession = true;

        bool show = false;
        QSettings settings;

        if (!settings.contains("showWelcomeScreen")) {
            settings.setValue("showWelcomeScreen", 1);
            show = true;
        } else {
            // if we are returning to welcome dialog page after canceling
            // file dialog, do not care about settings but always show
            // welcome
            show = settings.value("showWelcomeScreen").toBool()
                   || m_goStraightToWelcomeFileDialog;
        }

        if (show) {
            StudioTutorialWidget tutorial(m_pMainWnd, m_goStraightToWelcomeFileDialog, true);
            welcomeRes = tutorial.exec();
        }
    }

    // show the usual startup dialog only if user rejected tutorial
    // ( = did not open samples or create new project)
    if (welcomeRes == QDialog::Rejected) {
        CStartupDlg theStartupDlg;

        // Populate recent items
        Q3DStudio::CFilePath theMostRecentDirectory;
        if (m_Views) {
            CRecentItems *theRecentItems = m_Views->GetMainFrame()->GetRecentItems();
            for (long theIndex = 0; theIndex < theRecentItems->GetItemCount(); ++theIndex) {
                if (theIndex == 0) {
                    theMostRecentDirectory =
                        Q3DStudio::CFilePath(theRecentItems->GetItem(0).GetAbsolutePath())
                        .GetDirectory();
                }
                theStartupDlg.AddRecentItem(theRecentItems->GetItem(theIndex));
            }
        }

        theStartupDlg.exec();
        CStartupDlg::EStartupChoice theChoice = theStartupDlg.GetChoice();

        switch (theChoice) {
        case CStartupDlg::EStartupChoice_Exit:
            theReturn = true;
            break;

        case CStartupDlg::EStartupChoice_NewDoc: {
            std::pair<Qt3DSFile, bool> theFile = m_Dialogs->
                GetNewDocumentChoice(theMostRecentDirectory);
            if (theFile.first.GetPath() != "") {
                m_Core->OnNewDocument(theFile.first, theFile.second);
                theReturn = true;
            } else {
                // User Cancels the dialog. Show startup dialog again.
                theReturn = ShowStartupDialog();
            }
        } break;

        case CStartupDlg::EStartupChoice_OpenDoc: {
            Qt3DSFile theFile = m_Dialogs->GetFileOpenChoice(theMostRecentDirectory);
            if (theFile.GetPath() != "") {
                OnLoadDocument(theFile);
                theReturn = true;
            } else {
                // User Cancels the dialog. Show startup dialog again.
                theReturn = ShowStartupDialog();
            }
        } break;

        case CStartupDlg::EStartupChoice_OpenRecent: {
            Qt3DSFile theFile = theStartupDlg.GetRecentDoc();
            if (theFile.GetPath() != "") {
                OnLoadDocument(theFile);
                // throw SlideNotFound( L"");
                theReturn = true;
            } else {
                // User Cancels the dialog. Show startup dialog again.
                theReturn = ShowStartupDialog();
            }
        } break;

        default:
            ASSERT(false); // Should not reach this block.
            theReturn = false;
            break;
        }
    } else { // open sample or create new
        theReturn = HandleWelcomeRes(welcomeRes, true);
    }
    return theReturn;
}

//=============================================================================
/**
 * Start the app.
 */
int CStudioApp::BlankRunApplication()
{
    InitCore();

    if (ShowStartupDialog())
        return RunApplication();
    return -1;
}

//=============================================================================
/**
 *	Run the unit tests specified on the command line then return.
 *	@param		inTestPath	input unit test path
 *	@return		0 on success; 1 on failure
 */
int CStudioApp::RunCmdLineTests(const Q3DStudio::CString &inTestPath)
{
    Q_UNUSED(inTestPath);

    return m_UnitTestResults;
}

//=============================================================================
/**
 *	Run the system level tests specified on the command line then return.
 *	@param inTestPath
 *	@return 0 on success; -1 on failure
 */
int CStudioApp::RunSystemTests(const Q3DStudio::CString &inTestPath)
{
    int theSystemTestsResult = -1;

    Q_UNUSED(inTestPath);

    if (0 == m_UnitTestResults)
        theSystemTestsResult = m_UnitTestResults; // unit tests return 0 on success; 1 on failure

    return theSystemTestsResult;
}

//=============================================================================
/**
 * Open the specified file and run the application.
 * This will load the file then go into the standard app loop.
 * On load with the -silent flag, this would force the application to exit on
 * load failures.
 * @return 0 on success; -1 on failure
 */
int CStudioApp::OpenAndRunApplication(const Q3DStudio::CString &inFilename)
{
    int theSuccess = -1;
    InitCore();
    if (OnLoadDocument(
            inFilename,
            false)) // Load document. Upon failure, don't show startup dialog but exit immediately.
        theSuccess = RunApplication();
    return theSuccess;
}

//=============================================================================
/**
 *	This is the app execution loop, the main thread loops here until the app exits.
 *	@return 0 on success; -1 on failure
 */
int CStudioApp::RunApplication()
{
    return qApp->exec();
}

//=============================================================================
/**
 * Initialize the core and all the views.
 */
void CStudioApp::InitCore()
{
    // Initialize and cache the RenderSelector values for the first time,
    // this way, subsequent attempts to instantiate a RenderSelector would circumvent the need
    // for any extra (unneccesary) creation of render contexts which inadvertently cause exceptions
    // to be thrown.

    m_Core->Initialize();

    if (m_Views) {
        m_Views->CreateViews();
        m_pMainWnd = m_Views->GetMainFrame();
    } else {
        ASSERT(0); // No views? wha?
    }

    // At this point, get rid of the splash screen, otherwise any errors dialog would be hidden
    // behind.
    // Could happen when this is directly activated due to a uip file being dbl-clicked or dragged
    // into the executable.
    if (m_SplashPalette) {
        m_SplashPalette->deleteLater();
        m_SplashPalette = nullptr;
    }

    RegisterGlobalKeyboardShortcuts(m_Core->GetHotKeys(), m_pMainWnd);
    m_Core->GetDispatch()->AddPresentationChangeListener(this);
}

struct SIImportFailedHandler : public Q3DStudio::IImportFailedHandler
{
    CDialogs &m_Dialogs;
    SIImportFailedHandler(CDialogs &dialogs)
        : m_Dialogs(dialogs)
    {
    }
    void DisplayImportFailed(const QString &inDocumentPath,
                             const QString &inDescription, bool inWarningsOnly) override
    {
        m_Dialogs.DisplayImportFailed(QUrl(inDocumentPath), inDescription, inWarningsOnly);
    }
};

struct SIDeletingReferencedObjectHandler : public Q3DStudio::IDeletingReferencedObjectHandler
{
    CDialogs &m_Dialogs;

    SIDeletingReferencedObjectHandler(CDialogs &dialogs)
        : m_Dialogs(dialogs)
    {
    }

    void DisplayMessageBox(const Q3DStudio::CString &inDescription) override
    {
        Q3DStudio::CString theTitle(::LoadResourceString(IDS_DELETE_OBJ_CONFIRM_TITLE));
        Q3DStudio::CString theFormat(::LoadResourceString(IDS_DELETE_OBJ_CONFIRM_MSG));
        Q3DStudio::CString theMessage;
        theMessage.Format(theFormat, static_cast<const wchar_t *>(inDescription));

        m_Dialogs.DisplayMessageBox(theTitle, theMessage, Qt3DSMessageBox::ICON_WARNING, false);
    }
};

void CStudioApp::SetupTimer(long inMessageId, QWidget *inWnd)
{
    m_TickTock = ITickTock::CreateTickTock(inMessageId, inWnd);
    GetDirectoryWatchingSystem();
    m_Core->GetDoc()->SetDirectoryWatchingSystem(m_DirectoryWatchingSystem);
    m_Core->GetDoc()->SetImportFailedHandler(
        std::make_shared<SIImportFailedHandler>(std::ref(*GetDialogs())));
    m_Core->GetDoc()->SetDocMessageBoxHandler(
        std::make_shared<SIDeletingReferencedObjectHandler>(std::ref(*GetDialogs())));
}

ITickTock &CStudioApp::GetTickTock()
{
    if (m_TickTock == nullptr)
        throw std::runtime_error("Uninitialized TickTock");
    return *m_TickTock;
}

Q3DStudio::IStudioRenderer &CStudioApp::GetRenderer()
{
    if (!m_Renderer)
        m_Renderer = Q3DStudio::IStudioRenderer::CreateStudioRenderer();
    return *m_Renderer;
}

void CStudioApp::ClearGuides()
{
    SCOPED_DOCUMENT_EDITOR(*m_Core->GetDoc(), QObject::tr("Clear Guides"))->ClearGuides();
}

void SendAsyncCommand(CDispatch &inDispatch, Q3DStudio::TCallbackFunc inFunc)
{
    inDispatch.FireOnAsynchronousCommand(inFunc);
}

IDirectoryWatchingSystem &CStudioApp::GetDirectoryWatchingSystem()
{
    if (m_DirectoryWatchingSystem == nullptr) {
        Q3DStudio::TCallbackCaller theCaller =
            std::bind(SendAsyncCommand, std::ref(*m_Core->GetDispatch()), std::placeholders::_1);
        m_DirectoryWatchingSystem =
            IDirectoryWatchingSystem::CreateThreadedDirectoryWatchingSystem(theCaller);
    }
    return *m_DirectoryWatchingSystem;
}

CCore *CStudioApp::GetCore()
{
    return m_Core;
}

//=============================================================================
/**
 * Get the view manager for this core to communicate to the views.
 */
CViews *CStudioApp::GetViews()
{
    return m_Views;
}

//=============================================================================
/**
 * Get the dialog manager for this core for displaying dialogs.
 */
CDialogs *CStudioApp::GetDialogs()
{
    return m_Dialogs;
}

long CStudioApp::GetToolMode()
{
    return m_ToolMode;
}

void CStudioApp::SetToolMode(long inToolMode)
{
    if (m_ToolMode != inToolMode) {
        m_ToolMode = inToolMode;
        m_Core->GetDispatch()->FireOnToolbarChange();
    }
}

long CStudioApp::GetSelectMode()
{
    return m_SelectMode;
}

void CStudioApp::SetSelectMode(long inSelectMode)
{
    if (m_SelectMode != inSelectMode) {
        m_SelectMode = inSelectMode;
        m_Core->GetDispatch()->FireOnToolbarChange();
    }
}

StudioManipulationModes::Enum CStudioApp::GetMinpulationMode() const
{
    return m_ManipulationMode;
}
void CStudioApp::SetMinpulationMode(StudioManipulationModes::Enum inManipulationMode)
{
    if (m_ManipulationMode != inManipulationMode) {
        m_ManipulationMode = inManipulationMode;
        m_Core->GetDispatch()->FireOnToolbarChange();
    }
}

//=============================================================================
/**
 * return true if undo is possible
 */
bool CStudioApp::CanUndo()
{
    return m_Core->GetCmdStack()->CanUndo();
}

//=============================================================================
/**
 * return true if redo is possible
 */
bool CStudioApp::CanRedo()
{
    return m_Core->GetCmdStack()->CanRedo();
}

void CStudioApp::OnCopy()
{
    m_Core->GetDoc()->HandleCopy();
}

bool CStudioApp::CanCopy()
{
    return m_Core->GetDoc()->CanCopy();
}

//=============================================================================
/**
 * Get a string describing the type of the copy operation that can be done.
 * Precedence of copying is 1) Actions; 2) Keyframes; 3) Objects
 */
Q3DStudio::CString CStudioApp::GetCopyType()
{
    Q3DStudio::CString theCopyType;

    CDoc *theDoc = m_Core->GetDoc();
    if (theDoc->CanCopyAction())
        theCopyType = ::LoadResourceString(IDS_MENU_COPYPASTE_TYPE_ACTION);
    else if (theDoc->CanCopyKeyframe())
        theCopyType = ::LoadResourceString(IDS_MENU_COPYPASTE_TYPE_KEYFRAMES);
    else
        theCopyType = ::LoadResourceString(IDS_MENU_COPYPASTE_TYPE_OBJECT);

    return theCopyType;
}

//=============================================================================
/**
 * Cuts the selected object or keys
 */
void CStudioApp::OnCut()
{
    m_Core->GetDoc()->HandleCut();
}

bool CStudioApp::CanCut()
{
    return m_Core->GetDoc()->CanCut();
}

//=============================================================================
/**
 * Paste keys from the copied list yo
 */
void CStudioApp::OnPaste()
{
    m_Core->GetDoc()->HandlePaste();
}

bool CStudioApp::CanPaste()
{
    return m_Core->GetDoc()->CanPaste();
}

//=============================================================================
/**
 * Get a string describing the type of the paste operation that can be done.
 * Precedence of paste is 1) Actions; 2) Object ; 3) Keyframes
 */
Q3DStudio::CString CStudioApp::GetPasteType()
{
    Q3DStudio::CString thePasteType;

    CDoc *theDoc = m_Core->GetDoc();
    if (theDoc->CanPasteAction())
        thePasteType = ::LoadResourceString(IDS_MENU_COPYPASTE_TYPE_ACTION);
    else if (theDoc->CanPasteObject())
        thePasteType = ::LoadResourceString(IDS_MENU_COPYPASTE_TYPE_OBJECT);
    else
        thePasteType = ::LoadResourceString(IDS_MENU_COPYPASTE_TYPE_KEYFRAMES);

    return thePasteType;
}

bool CStudioApp::CanChangeTimebarColor()
{
    bool theRetVal = true;
    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = m_Core->GetDoc()->GetSelectedInstance();
    if (!theSelectedInstance.Valid()
        || m_Core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge()->IsSceneInstance(
               theSelectedInstance))
        theRetVal = false;

    return theRetVal;
}

//=============================================================================
/**
 * Sets any changed keyframes on the selected object
 */
void CStudioApp::HandleSetChangedKeys()
{
    m_Core->GetDoc()->SetChangedKeyframes();
}

//=============================================================================
/**
 * Deletes all selected keys
 */
void CStudioApp::DeleteSelectedKeys()
{
    m_Core->GetDoc()->DeleteSelectedKeys();
}

//=============================================================================
/**
 * Handles the duplicate object command
 */
void CStudioApp::HandleDuplicateCommand()
{
    m_Core->GetDoc()->HandleDuplicateCommand();
}

//=============================================================================
/**
 * return true if the selected object is duplicatable
 */
bool CStudioApp::CanDuplicateObject()
{
    // Get the currently selected object
    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = m_Core->GetDoc()->GetSelectedInstance();
    if (!theSelectedInstance.Valid())
        return false;

    // Check if the object can be duplicated
    return m_Core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge()->IsDuplicateable(
        theSelectedInstance);
}

//==============================================================================
/**
 * Toggles the state of autoset keyframes.
 */
void CStudioApp::OnToggleAutosetKeyframes()
{
    SetAutosetKeyframes(!CStudioPreferences::IsAutosetKeyframesOn());

    m_Core->GetDispatch()->FireOnToolbarChange();
}

//==============================================================================
/**
 * Updates the preferences, and AnimationSystem.
 */
void CStudioApp::SetAutosetKeyframes(bool inFlag)
{
    CStudioPreferences::SetAutosetKeyframesOn(inFlag);

    m_Core->GetDoc()->GetStudioSystem()->GetAnimationSystem()->SetAutoKeyframe(inFlag);
}

//==============================================================================
/**
 *	If the presentation is not currently playing, this function will make it
 *	start playing from the current position.  The starting point of the playhead
 *	is saved so that it can be restored later.
 */
void CStudioApp::PlaybackPlay()
{
    CDoc *theDoc = m_Core->GetDoc();
    if (!theDoc->IsPlaying()) {
        m_PlaybackTime = theDoc->GetCurrentViewTime();
        m_PlaybackOriginalSlide = theDoc->GetActiveSlide();
        theDoc->SetPlayMode(PLAYMODE_PLAY);
    }
}

//==============================================================================
/**
 *	If the presentation is currently playing, it is stopped.  The playhead is
 *	left wherever it was stopped at (hence it's not restored).
 */
void CStudioApp::PlaybackStopNoRestore()
{
    m_Core->GetDoc()->SetPlayMode(PLAYMODE_STOP);
}

//==============================================================================
/**
 *	Moves the playhead back to time zero.
 */
void CStudioApp::PlaybackRewind()
{
    CDoc *theDoc = m_Core->GetDoc();
    if (theDoc->IsPlaying()) {
        theDoc->SetPlayMode(PLAYMODE_STOP, 0);
        theDoc->SetPlayMode(PLAYMODE_PLAY);
    } else {
        m_Core->GetDoc()->NotifyTimeChanged(0);
    }
}

//=============================================================================
/**
 * Performs a file revert.
 * This will revert the doc to the last saved version.
 */
void CStudioApp::OnRevert()
{
    if (!m_Core->GetDoc()->IsModified() || m_Dialogs->ConfirmRevert()) {
        Qt3DSFile theCurrentDoc = m_Core->GetDoc()->GetDocumentPath();
        OnLoadDocument(theCurrentDoc);
    }
}

//=============================================================================
/**
 * Check to see if it is possible to perform a revert.
 */
bool CStudioApp::CanRevert()
{
    return m_Core->GetDoc()->IsModified() && m_Core->GetDoc()->GetDocumentPath().GetPath() != "";
}

//==============================================================================
/**
 * Handles the recent list.
 */
void CStudioApp::OnFileOpenRecent(const Qt3DSFile &inDocument)
{
    if (PerformSavePrompt())
        OnLoadDocument(inDocument);
}

//==============================================================================
/**
 * Called when closing the current doc, this prompts the user to save the doc.
 * This will only prompt if the doc is modified, and if the user selects save
 * then this will perform the save operation.
 * @return true if the operation should continue, false if not.
 */
bool CStudioApp::PerformSavePrompt()
{
    if (m_Core->GetDoc()->IsModified()) {
        CDialogs::ESavePromptResult theResult = m_Dialogs->PromptForSave();
        if (theResult == CDialogs::SAVE_FIRST) {
            bool onSaveResult = OnSave();
            if (onSaveResult)
                return true;
        } else if (theResult == CDialogs::CONTINUE_NO_SAVE)
            return true;

        return false;
    }
    return true;
}

//==============================================================================
/**
 *	If the presentation is currently playing, it is stopped.  The playhead is
 *	restored to the position found in m_PlaybackTime.
 */
void CStudioApp::PlaybackStop()
{
    CDoc *theDoc = m_Core->GetDoc();
    // change it back to the original slide first before restoring the original time
    if (m_PlaybackOriginalSlide.Valid()) {
        if (m_PlaybackOriginalSlide != theDoc->GetActiveSlide())
            theDoc->NotifyActiveSlideChanged(m_PlaybackOriginalSlide);
        theDoc->SetPlayMode(PLAYMODE_STOP, m_PlaybackTime);
    }
    // Invalidate the playback original slide so we don't inadvertently trigger this code later.
    m_PlaybackOriginalSlide = 0;
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::AdvanceTime()
{
    long theDeltaTime = CStudioPreferences::GetTimeAdvanceAmount();
    long theTime =
        (m_Core->GetDoc()->GetCurrentViewTime() + theDeltaTime) / theDeltaTime * theDeltaTime;
    m_Core->GetDoc()->NotifyTimeChanged(theTime);
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::ReduceTime()
{
    long theDeltaTime = CStudioPreferences::GetTimeAdvanceAmount();
    long theTime = (m_Core->GetDoc()->GetCurrentViewTime() - 1) / theDeltaTime * theDeltaTime;
    m_Core->GetDoc()->NotifyTimeChanged(theTime);
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::AdvanceUltraBigTime()
{
    long theDeltaTime = CStudioPreferences::GetBigTimeAdvanceAmount();
    long theTime =
        (m_Core->GetDoc()->GetCurrentViewTime() + theDeltaTime) / theDeltaTime * theDeltaTime;
    m_Core->GetDoc()->NotifyTimeChanged(theTime);
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::ReduceUltraBigTime()
{
    long theDeltaTime = CStudioPreferences::GetBigTimeAdvanceAmount();
    long theTime = (m_Core->GetDoc()->GetCurrentViewTime() - 1) / theDeltaTime * theDeltaTime;
    m_Core->GetDoc()->NotifyTimeChanged(theTime);
}

//==============================================================================
/**
 *	If the presentation is currently playing, it is stopped.  Otherwise, the
 *	presetation starts playing from its current position. Called when the user
 *	presses the Enter key.
 */
void CStudioApp::PlaybackToggle()
{
    // If the presentation is playing, stop it and leave the playhead where it is
    if (m_Core->GetDoc()->IsPlaying())
        PlaybackStopNoRestore();
    // Otherwise, the presentation is stopped, so start it playing
    else
        PlaybackPlay();
}

CInspectableBase *CStudioApp::GetInspectableFromSelectable(Q3DStudio::SSelectedValue inSelectable)
{
    CInspectableBase *theInspectableBase = nullptr;
    if (inSelectable.empty() == false) {
        switch (inSelectable.getType()) {
        case Q3DStudio::SelectedValueTypes::Slide:
            theInspectableBase = new Qt3DSDMInspectable(
                *this, m_Core, inSelectable.getData<Q3DStudio::SSlideInstanceWrapper>().m_Instance);
            break;
        case Q3DStudio::SelectedValueTypes::MultipleInstances:
        case Q3DStudio::SelectedValueTypes::Instance: {

            // We need to decide whether to display SlideInspectable or UICDMInspectable
            // We display SlideInspectable if user selects a Scene or Component where the current
            // active slide belongs,
            // for example when user selects the Root in Timeline Palette
            CDoc *theDoc = m_Core->GetDoc();
            qt3dsdm::TInstanceHandleList theSelectedInstances =
                theDoc->GetSelectedValue().GetSelectedInstances();
            qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance;
            if (theSelectedInstances.size() == 1)
                theSelectedInstance = theSelectedInstances[0];

            if (m_Core->GetDoc()->GetDocumentReader().IsInstance(theSelectedInstance)) {
                CClientDataModelBridge *theBridge =
                    theDoc->GetStudioSystem()->GetClientDataModelBridge();
                qt3dsdm::Qt3DSDMSlideHandle theCurrentActiveSlide = theDoc->GetActiveSlide();

                // Slide, scene or component
                if (theSelectedInstance
                    == theBridge->GetOwningComponentInstance(theCurrentActiveSlide)) {
                    Qt3DSDMInstanceHandle theCurrentActiveSlideInstance =
                        theDoc->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(
                            theCurrentActiveSlide);

                    if (theBridge->IsSceneInstance(theSelectedInstance))
                        theInspectableBase = new Qt3DSDMSceneInspectable(
                            *this, m_Core, theSelectedInstance, theCurrentActiveSlideInstance);
                    else if (theBridge->IsComponentInstance(theSelectedInstance))
                        theInspectableBase = new Qt3DSDMInspectable(
                            *this, m_Core, theSelectedInstance, theCurrentActiveSlideInstance);
                }
                if (theInspectableBase == nullptr) {
                    if (theBridge->IsMaterialBaseInstance(theSelectedInstance))
                        theInspectableBase =
                            new Qt3DSDMMaterialInspectable(*this, m_Core, theSelectedInstance);
                    else
                        theInspectableBase =
                            new Qt3DSDMInspectable(*this, m_Core, theSelectedInstance);
                }
            }
        } break;
        case Q3DStudio::SelectedValueTypes::Guide: {
            qt3dsdm::Qt3DSDMGuideHandle theGuide = inSelectable.getData<qt3dsdm::Qt3DSDMGuideHandle>();
            theInspectableBase = CGuideInspectable::CreateInspectable(*m_Core, theGuide);
        } break;
        };
    }

    return theInspectableBase;
}

void CStudioApp::RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler,
                                                 QWidget *actionParent)
{
    m_Core->RegisterGlobalKeyboardShortcuts(inShortcutHandler, actionParent);

    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::Key_Period),
                        CStudioApp::AdvanceTime);
    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::Key_Comma),
                        CStudioApp::ReduceTime);
    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::ShiftModifier | Qt::Key_Period),
                        CStudioApp::AdvanceUltraBigTime);
    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::ShiftModifier | Qt::Key_Comma),
                        CStudioApp::ReduceUltraBigTime);
    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::Key_Return),
                        CStudioApp::PlaybackToggle);

    inShortcutHandler->RegisterKeyUpEvent(
        new CDynHotKeyConsumer<CStudioApp>(this, &CStudioApp::PlaybackStop), 0,
        Qt::Key_Space);
    inShortcutHandler->RegisterKeyDownEvent(
        new CDynHotKeyConsumer<CStudioApp>(this, &CStudioApp::PlaybackPlay), 0,
        Qt::Key_Space);

    if (m_Views)
        m_Views->RegisterGlobalKeyboardShortcuts(inShortcutHandler, actionParent);
}

//=============================================================================
/**
 * Handles the Save command
 * This will save the file, if the file has not been saved before this will
 * do a save as operation.
 * @return true if the file was successfully saved.
 */
bool CStudioApp::OnSave()
{
    Qt3DSFile theCurrentDoc = m_Core->GetDoc()->GetDocumentPath();
    if (!theCurrentDoc.IsFile()) {
        return OnSaveAs();
    } else if (!theCurrentDoc.CanWrite()) {
        m_Dialogs->DisplaySavingPresentationFailed();
        return false;
    } else {
        m_Core->OnSaveDocument(theCurrentDoc);
        return true;
    }
}

//=============================================================================
/**
 * Command handler for the File Save As menu option.
 * This will prompt the user for a location to save the file out to then
 * will perform the save.
 * @return true if the file was successfully saved.
 */
bool CStudioApp::OnSaveAs()
{
    Qt3DSFile theFile = m_Dialogs->GetSaveAsChoice().first;
    if (theFile.GetPath() != "") {
        m_Core->OnSaveDocument(theFile);
        return true;
    }
    return false;
}

//=============================================================================
/**
 * Command handler for the File Save As menu option.
 * This will prompt the user for a location to save the file out to then
 * save a copy, leaving the original file open in the editor.
 * @return true if the file was successfully saved.
 */
bool CStudioApp::OnSaveCopy()
{
    Qt3DSFile theFile = m_Dialogs->GetSaveAsChoice().first;
    if (theFile.GetPath() != "") {
        // Send in a "true" to teh save function to indicate this is a copy
        m_Core->OnSaveDocument(theFile, true);
        return true;
    }
    return false;
}

//=============================================================================
/**
 * Call to load a new document.
 * There should not be a currently active document when this is called.
 * @param inDocument the path to the UIP file to be loaded.
 * @param inShowStartupDialogOnError true to show startup dialog if loading document is error
 * @return true if loading was successful
 */
bool CStudioApp::OnLoadDocument(const Qt3DSFile &inDocument, bool inShowStartupDialogOnError)
{
    m_Core->GetDispatch()->FireOnProgressBegin(CString::fromQString(QObject::tr("Loading ")),
                                               inDocument.GetName());

    bool theLoadResult = false;
    int theLoadErrorParameter = -1;
    Q3DStudio::CString theErrorText;
    try {
        OnLoadDocumentCatcher(inDocument);
        m_Core->GetDispatch()->FireOnOpenDocument(inDocument, true);
        // Loading was successful
        theLoadResult = true;
    } catch (CUnsupportedFileFormatException &) {
        theErrorText = CString::fromQString(QObject::tr("The file could not be opened.  It is either invalid or was made with an "
                                                        "old version of Studio."));
        // We've encountered a file format that is older than the current, OR
        // corrupt files, unsupported file formats and illegal types.
    } catch (CInvalidFileFormatException &) {
        theErrorText = CString::fromQString(QObject::tr("The file could not be opened.  It appears to have been made with a newer "
                                                        "version of Studio."));
        // Cannot support opening newer file format, the UIP or (AP ie client portion)'s version is
        // mismatched.
    } catch (CLoadReferencedFileException &inError) {
        // referenced files (e.g. Data Files) failed to load
        theErrorText.Format(L"%ls failed to load due to invalid referenced file: %ls.",
                            static_cast<const wchar_t *>(inDocument.GetName()),
                            inError.GetFilePath()); // TODO: Localize
        const wchar_t *theDesc = inError.GetDescription();
        if (theDesc && wcslen(theDesc) > 0) {
            // append any description is provided
            theErrorText += L"\n";
            theErrorText += inError.GetDescription();
        }
    } catch (CIOException &) { // provide specific error message if possible
        if (inDocument.Exists() == false)
            theLoadErrorParameter = IDS_ERROR_LOADFILENOTEXIST;
        qCCritical(qt3ds::INTERNAL_ERROR)
                << "Failed to load document, IO error (file may be unreadable or nonexistent)";
    } catch (...) {
        qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to load document, uknown error";
        // We don't know exactly what went wrong during a load, but let studio 'fail gracefully'.
    }

    if (theErrorText.Length()) {
        qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to load document: "
                                          << theErrorText.GetCharStar();
    }

    m_Core->GetDispatch()->FireOnProgressEnd();

    // load fail
    if (!theLoadResult) {
        if (!theErrorText.IsEmpty())
            m_Dialogs->DisplayKnownErrorDialog(theErrorText);
        else
            m_Dialogs->DisplayLoadingPresentationFailed(inDocument, theLoadErrorParameter);

        m_Core->GetDispatch()->FireOnOpenDocument(inDocument, false);

        // Show startup dialog
        if (inShowStartupDialogOnError)
            if (!ShowStartupDialog())
                qApp->quit();
    } else {
        m_Dialogs->ResetSettings(inDocument.GetPath());

        m_subpresentations.clear();
        m_Core->GetDoc()->LoadUIASubpresentations(m_Core->GetDoc()->GetDocumentUIAFile(),
                                                  m_subpresentations);
    }

    m_AuthorZoom = false;

    m_Core->GetDispatch()->FireAuthorZoomChanged();

    return theLoadResult;
}


//=============================================================================
/**
 *
 */
void CStudioApp::SaveUIAFile()
{
    QStringList list;
    for (SubPresentationRecord r : m_subpresentations) {
        list.append(r.m_type);
        list.append(r.m_id);
        list.append(r.m_argsOrSrc);
    }
    Q3DStudio::CFilePath doc(GetCore()->GetDoc()->GetDocumentPath().GetAbsolutePath());
    qt3ds::state::IApplication::EnsureApplicationFile(doc.GetCharStar(), list);
}

//=============================================================================
/**
 * Called by OnLoadDocument, to allow the error reporting to be inserted.
 * Because of the nature of the error reporting, OnLoadDocument has to have
 * a certain structure that limits it (C type variables, no object destructors).
 */
void CStudioApp::OnLoadDocumentCatcher(const Qt3DSFile &inDocument)
{
    {
        CDispatchDataModelNotificationScope __scope(*m_Core->GetDispatch());
        m_Core->GetDoc()->CloseDocument();
        m_Core->GetDoc()->LoadDocument(inDocument);
    }

    // Make sure the client scene is resized properly
    if (m_Views)
        m_Views->RecheckMainframeSizingMode();
}

void CStudioApp::OnFileOpen()
{
    if (PerformSavePrompt()) {
        Qt3DSFile theFile = m_Dialogs->GetFileOpenChoice();
        if (theFile.GetPath() != "")
            OnLoadDocument(theFile);
    }
}
using namespace std;

void CStudioApp::OnFileNew()
{
    if (PerformSavePrompt()) {
        pair<Qt3DSFile, bool> theFile = m_Dialogs->GetNewDocumentChoice();
        if (theFile.first.GetPath() != "")
            m_Core->OnNewDocument(theFile.first, theFile.second);
    }
}

bool CStudioApp::IsAuthorZoom()
{
    return m_AuthorZoom;
}

void CStudioApp::SetAuthorZoom(bool inZoom)
{
    if (m_AuthorZoom != inZoom) {
        m_AuthorZoom = inZoom;
        m_Core->GetDispatch()->FireAuthorZoomChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
// These commands come over the dispatch from inside the core. The core doesn't
// have access to the CMsgRouter at the moment, so this relays the message.
void CStudioApp::OnAsynchronousCommand(CCmd *inCmd)
{
    CMsgRouter::GetInstance()->SendCommand(inCmd, m_Core);
}

void CStudioApp::OnDisplayAppStatus(Q3DStudio::CString &inStatusMsg)
{
    // Do nothing, it was used to show this in the status bar
}

void CStudioApp::OnProgressBegin(const Q3DStudio::CString &inActionText,
                                 const Q3DStudio::CString &inAdditionalText)
{
    m_Dialogs->DisplayProgressScreen(inActionText, inAdditionalText);
}

void CStudioApp::OnProgressEnd()
{
    m_Dialogs->DestroyProgressScreen();
}

void CStudioApp::OnAssetDeleteFail()
{
    m_Dialogs->DisplayAssetDeleteFailed();
}

void CStudioApp::OnPasteFail()
{
    m_Dialogs->DisplayPasteFailed();
}

void CStudioApp::OnBuildconfigurationFileParseFail(const Q3DStudio::CString &inMessage)
{
    m_Dialogs->DisplayMessageBox(::LoadResourceString(IDS_BUILDCONFIGS_ERROR_TITLE), inMessage,
                                 Qt3DSMessageBox::ICON_ERROR, false);
}

void CStudioApp::OnSaveFail(bool inKnownError)
{
    qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to save project: "
                                      << (inKnownError ? "KnownError" : "UnknownError");
    if (inKnownError) {
        m_Dialogs->DisplaySavingPresentationFailed();
    } else {
        m_Dialogs->DisplayKnownErrorDialog(::LoadResourceString(IDS_SAVE_UNKNOWN_ERROR));
    }
}

void CStudioApp::OnProjectVariableFail(const Q3DStudio::CString &inMessage)
{
    m_Dialogs->DisplayEnvironmentVariablesError(inMessage);
}

void CStudioApp::OnErrorFail(const Q3DStudio::CString &inText)
{
    qCCritical(qt3ds::INTERNAL_ERROR) << inText.GetCharStar();
    m_Dialogs->DisplayMessageBox(::LoadResourceString(IDS_PROJNAME), inText,
                                 Qt3DSMessageBox::ICON_ERROR, false);
}

void CStudioApp::OnRefreshResourceFail(const Q3DStudio::CString &inResourceName,
                                       const Q3DStudio::CString &inDescription)
{
    qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to refresh resource: "
                                      << inResourceName.GetCharStar();
    qCCritical(qt3ds::INTERNAL_ERROR) << inDescription.GetCharStar();
    m_Dialogs->DisplayRefreshResourceFailed(inResourceName, inDescription);
}

void CStudioApp::OnNewPresentation()
{
    m_Core->GetDoc()->GetStudioSystem()->GetAnimationSystem()->SetAutoKeyframe(
        CStudioPreferences::IsAutosetKeyframesOn());
    qCInfo(qt3ds::TRACE_INFO) << "New Presentation: "
              << m_Core->GetDoc()->GetDocumentPath().GetAbsolutePath().GetCharStar();
}

void CStudioApp::OnPresentationModifiedExternally()
{
    int theUserChoice = m_Dialogs->DisplayChoiceBox(
        ::LoadResourceString(IDS_TITLE_WARNING),
        ::LoadResourceString(IDS_PRESENTATION_MODIFIED_EXTERNALLY), Qt3DSMessageBox::ICON_WARNING);
    if (theUserChoice == IDYES) {
        Qt3DSFile theCurrentDoc = m_Core->GetDoc()->GetDocumentPath();
        OnLoadDocument(theCurrentDoc);
    }
}
