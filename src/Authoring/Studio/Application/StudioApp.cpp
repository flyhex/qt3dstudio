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
#include "Qt3DSStateApplication.h"
#include "PlayerWnd.h"
#include "DataInputDlg.h"
#include "qtsingleapplication.h"
#include "qtlocalpeer.h"
#include "TimelineWidget.h"

#include <QtGui/qsurfaceformat.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qurl.h>
#include <QtGui/qopenglcontext.h>
#include <QtWidgets/qaction.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qcommandlineparser.h>

const QString activePresentationQuery = QStringLiteral("activePresentation:");

int main(int argc, char *argv[])
{
    // Note: This will prevent localization from working on Linux, but it will fix QT3DS-1473
    // TODO: To be removed once the new parser is in use
#if defined(Q_OS_LINUX)
    qputenv("LC_ALL", "C");
#endif

    // init runtime static resources
    Q_INIT_RESOURCE(res);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    SharedTools::QtSingleApplication guiApp(QStringLiteral("Qt3DStudio"), argc, argv);

#if defined(Q_OS_MACOS)
    QSurfaceFormat openGL33Format;
    openGL33Format.setRenderableType(QSurfaceFormat::OpenGL);
    openGL33Format.setProfile(QSurfaceFormat::CoreProfile);
    openGL33Format.setMajorVersion(3);
    openGL33Format.setMinorVersion(3);
    openGL33Format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(openGL33Format);
#else
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

    // Parse the command line so we know what's up
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("file",
                                 QObject::tr("The presentation file."),
                                 QObject::tr("[file]"));
    parser.addPositionalArgument("folder",
                                 QObject::tr("The folder in which to create the new\n"
                                             "presentation."),
                                 QObject::tr("[folder]"));
    parser.addOption({"create",
                      QObject::tr("Creates a new presentation.\n"
                      "The file argument must be specified.\n"
                      "The folder argument is optional. In\n"
                      "case it is omitted, the new presentation\n"
                      "is created in the executable folder.")});
    parser.addOption({"silent",
                      QObject::tr("Allows creating a project silently.\n"
                      "Only has effect with create.")});
    parser.process(guiApp);

    const QStringList files = parser.positionalArguments();
    if (files.count() > 1 && !parser.isSet("create")) {
        qWarning() << "Only one presentation file can be given.";
        parser.showHelp(-1);
    } else if (files.count() > 2 && parser.isSet("create")) {
        qWarning() << "Only one presentation file and a target folder can be given.";
        parser.showHelp(-1);
    } else if (files.count() == 0 && parser.isSet("create")) {
        qWarning() << "A presentation file is required.";
        parser.showHelp(-1);
    }

    QObject::connect(&guiApp, &SharedTools::QtSingleApplication::messageReceived,
                     &g_StudioApp, &CStudioApp::handleMessageReceived);

#if (defined Q_OS_MACOS)
    QObject::connect(&guiApp, &SharedTools::QtSingleApplication::fileOpenRequest,
                     &g_StudioApp, &CStudioApp::openApplication);
#endif

    // Load and apply stylesheet for the application
    QFile styleFile(":/style.qss");
    styleFile.open(QFile::ReadOnly);
    guiApp.setStyleSheet(styleFile.readAll());
    g_StudioApp.initInstance(parser);
    return g_StudioApp.run(parser);
}

//==============================================================================
//	Includes
//==============================================================================
#include "Exceptions.h"
#include "IOLibraryException.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "Views.h"
#include "Doc.h"
#include "Dialogs.h"
#include "Dispatch.h"
#include "StartupDlg.h"
#include "RecentItems.h"
#include "StudioPreferences.h"
#include "MsgRouter.h"
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

#include <QtWidgets/qapplication.h>
#include <QtCore/qsettings.h>

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

CStudioApp g_StudioApp;

using namespace Q3DStudio;

namespace qt3ds
{
void Qt3DSAssert(const char *exp, const char *file, int line, bool *ignore)
{
    Q_UNUSED(ignore)
    g_StudioApp.GetDialogs()->DestroyProgressScreen();
    g_StudioApp.GetDialogs()->DisplayKnownErrorDialog(exp);
    qFatal("Assertion thrown %s(%d): %s", file, line, exp);
}
}

//=============================================================================
/**
 * Constructor
 */
CStudioApp::CStudioApp()
    : m_core(nullptr)
    , m_isSilent(false)
    , m_views(nullptr)
    , m_toolMode(STUDIO_TOOLMODE_MOVE)
    , m_manipulationMode(StudioManipulationModes::Local)
    , m_selectMode(STUDIO_SELECTMODE_GROUP)
    , m_dialogs(nullptr)
    , m_playbackTime(0)
    , m_authorZoom(false)
    , m_welcomeShownThisSession(false)
    , m_goStraightToWelcomeFileDialog(false)
    , m_tutorialPage(0)
    , m_autosaveTimer(new QTimer(this))
{
    connect(m_autosaveTimer, &QTimer::timeout, this, [=](){ OnSave(true); });
}

//=============================================================================
/**
 * Destructor
 */
CStudioApp::~CStudioApp()
{
    delete m_views;
    m_views = nullptr;
    delete m_dialogs;
    m_dialogs = nullptr;
    delete m_core;
    m_core = nullptr;
}

void CStudioApp::performShutdown()
{
    // Dispatch un-registration
    if (m_core) {
        m_core->GetDispatch()->RemoveAppStatusListener(this);
        m_core->GetDispatch()->RemoveCoreAsynchronousEventListener(this);
        qCInfo(qt3ds::TRACE_INFO) << "Studio exiting successfully";
    }

    if (m_renderer) {
        if (m_views->getMainFrame())
            m_views->getMainFrame()->GetPlayerWnd()->makeCurrent();
        m_renderer->Close();
        m_renderer = std::shared_ptr<Q3DStudio::IStudioRenderer>();
        if (m_views->getMainFrame())
            m_views->getMainFrame()->GetPlayerWnd()->doneCurrent();
    }

    delete m_views;
    m_views = nullptr;
    delete m_dialogs;
    m_dialogs = nullptr;
    delete m_core;
    m_core = nullptr;

    // Get rid of the temp files
    Qt3DSFile::ClearCurrentTempCache();

    CMsgRouter::GetInstance()->blockMessages();

    qApp->exit();
}

//=============================================================================
/**
 * Entry location for the creation of this application.
 * This creates the all the views, then returns if everything
 * was successful.
 */
bool CStudioApp::initInstance(const QCommandLineParser &parser)
{
    QApplication::setOrganizationName("The Qt Company");
    QApplication::setOrganizationDomain("qt.io");
    QApplication::setApplicationName("Qt 3D Studio");
    QApplication::setApplicationVersion(
                QString::fromWCharArray(CStudioPreferences::GetVersionString().c_str()));

    qCInfo(qt3ds::TRACE_INFO) << "Studio: " << QApplication::applicationFilePath();
    qCInfo(qt3ds::TRACE_INFO) << "Version: "
                              << CStudioPreferences::GetVersionString().GetCharStar();

    // Silent is ignored for everything but create
    m_isSilent = parser.isSet("silent") && parser.isSet("create");

    CFilePath thePreferencesPath = CFilePath::GetUserApplicationDirectory();
    thePreferencesPath = CFilePath::CombineBaseAndRelative(
                thePreferencesPath, CFilePath(L"Qt3DSComposer\\Preferences.setting"));
    CPreferences::SetPreferencesFile(thePreferencesPath);

    // Initialize help file path
    m_pszHelpFilePath = Qt3DSFile::GetApplicationDirectory().GetPath() +
            Q3DStudio::CString("/../doc/qt3dstudio/qt3dstudio-index.html");

    CStudioPreferences::LoadPreferences();

    m_dialogs = new CDialogs(!m_isSilent);

    m_views = new CViews(this);

    m_core = new CCore();
    getRenderer();
    m_core->GetDoc()->SetSceneGraph(m_renderer);

    // Dispatch registration
    m_core->GetDispatch()->AddAppStatusListener(this);
    m_core->GetDispatch()->AddCoreAsynchronousEventListener(this);
    m_core->GetDispatch()->AddFailListener(this);

    // Initialize autosave
    m_autosaveTimer->setInterval(CStudioPreferences::GetAutoSaveDelay() * 1000);
    if (CStudioPreferences::GetAutoSavePreference())
        m_autosaveTimer->start();

    return true;
}

//=============================================================================
/**
 * Command handler to display the about dialog.
 */
void CStudioApp::onAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.exec();
}

//=============================================================================
/**
 * Main application execution loop.
 * The application's main thread stays in this until the app exits.
 * @return true on success; false on failure
 */
bool CStudioApp::run(const QCommandLineParser &parser)
{
    bool theRetVal = false;
    try {
        if (parser.isSet("create")) {
            // Create, requires file and folder
            if (parser.positionalArguments().count() > 1) {
                theRetVal = createAndRunApplication(parser.positionalArguments().at(0),
                                                    parser.positionalArguments().at(1));
            } else {
                theRetVal = createAndRunApplication(parser.positionalArguments().at(0));
            }
        } else if (parser.positionalArguments().count() > 0) {
            // Start given file
            theRetVal = openAndRunApplication(parser.positionalArguments().at(0));
        } else {
            // No arguments, normal start
            theRetVal = blankRunApplication();
        }

        if (!theRetVal)
            qWarning("Problem starting application");

        performShutdown();
    } catch (qt3dsdm::Qt3DSDMError &uicdmError) {
        Q_UNUSED(uicdmError);
        exit(1);
    } catch (...) {
        throw;
    }

    return theRetVal;
}

bool CStudioApp::handleWelcomeRes(int res, bool recursive)
{
    bool theReturn = true;
    switch (res) {
    case StudioTutorialWidget::createNewResult: {
        Qt3DSFile theFile = m_dialogs->GetNewDocumentChoice(Q3DStudio::CString("."));
        if (theFile.GetPath() != "") {
            if (!m_core->OnNewDocument(theFile, true)) {
                // Invalid filename, show a message box and the startup dialog
                showInvalidFilenameWarning();
                theReturn = showStartupDialog();
            } else {
                theReturn = true;
                m_welcomeShownThisSession = true;
            }
        } else {
            // User Cancels the dialog. Show the welcome screen.
            if (recursive) {
                m_welcomeShownThisSession = false;
                m_goStraightToWelcomeFileDialog = true;
                theReturn = showStartupDialog();
            } else {
                theReturn = false;
            }
        }
    } break;

    case StudioTutorialWidget::openSampleResult: {
        // Try three options:
        // - open a specific example .uip
        // - failing that, show the main example root dir
        // - failing all previous, show default Documents dir
        Q3DStudio::CFilePath filePath;
        Qt3DSFile theFile = Qt3DSFile(".");

#ifndef Q_OS_MACOS
        filePath = Qt3DSFile::GetApplicationDirectory().GetPath() +
                Q3DStudio::CString("/../examples/studio3d/SampleProject");

        if (!filePath.Exists()) {
            filePath = Qt3DSFile::GetApplicationDirectory().GetPath() +
                    Q3DStudio::CString("/../examples/studio3d");
#else
        filePath = Qt3DSFile::GetApplicationDirectory().GetPath() +
                Q3DStudio::CString("/../../../../examples/studio3d/SampleProject");

        if (!filePath.Exists()) {
            filePath = Qt3DSFile::GetApplicationDirectory().GetPath() +
                    Q3DStudio::CString("/../../../../examples/studio3d");
#endif
            if (!filePath.Exists())
                filePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            theFile = m_dialogs->GetFileOpenChoice(filePath);
        } else {
            theFile = Qt3DSFile(filePath, Q3DStudio::CString("SampleProject.uip"));
        }

        if (theFile.GetPath() != "") {
            OnLoadDocument(theFile);
            theReturn = true;
            m_welcomeShownThisSession = true;
        } else {
            // User Cancels the dialog. Show the welcome screen.
            if (recursive) {
                m_welcomeShownThisSession = false;
                m_goStraightToWelcomeFileDialog = true;
                theReturn = showStartupDialog();
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

QString CStudioApp::resolvePresentationFile(const QString &inFile)
{
    // If opening an .uia file, parse the main presentation file from it and load that instead
    QString outFile = inFile;
    QFileInfo inFileInfo(inFile);
    if (inFileInfo.suffix().compare(QStringLiteral("uia"), Qt::CaseInsensitive) == 0) {
        QString initialPresentation;
        QString uiaPath = inFileInfo.absoluteFilePath();
        if (inFileInfo.exists())
            m_core->GetDoc()->LoadUIAInitialPresentationFilename(uiaPath, initialPresentation);

        if (!initialPresentation.isEmpty()) {
            QFileInfo uipFile(initialPresentation);
            if (uipFile.isAbsolute()) {
                outFile = initialPresentation;
            } else {
                uiaPath = inFileInfo.path();
                outFile = uiaPath + QStringLiteral("/") + initialPresentation;
            }
        }
    }
    return outFile;
}

//=============================================================================
/**
 * Show startup dialog and perform necessary action such as create new doc or load doc.
 * Return false if user requests to exit
 */
bool CStudioApp::showStartupDialog()
{
#if (defined Q_OS_MACOS)
    if (m_fileOpenEvent)
        return true;
#endif

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
        CStartupDlg theStartupDlg(m_pMainWnd);

        // Populate recent items
        if (m_views) {
            CRecentItems *theRecentItems = m_views->getMainFrame()->GetRecentItems();
            for (long theIndex = 0; theIndex < theRecentItems->GetItemCount(); ++theIndex)
                theStartupDlg.AddRecentItem(theRecentItems->GetItem(theIndex));
        }

        theStartupDlg.exec();
        CStartupDlg::EStartupChoice theChoice = theStartupDlg.GetChoice();

        switch (theChoice) {
        case CStartupDlg::EStartupChoice_Exit:
            theReturn = true;
            break;

        case CStartupDlg::EStartupChoice_NewDoc: {
            Qt3DSFile theFile = m_dialogs->GetNewDocumentChoice(getMostRecentDirectory());
            if (theFile.GetPath() != "") {
                 if (!m_core->OnNewDocument(theFile, true)) {
                     // Invalid filename, show a message box and the dialog again
                     showInvalidFilenameWarning();
                     theReturn = showStartupDialog();
                 }
            } else {
                // User Cancels the dialog. Show startup dialog again.
                theReturn = showStartupDialog();
            }
        } break;

        case CStartupDlg::EStartupChoice_OpenDoc: {
            Qt3DSFile theFile = m_dialogs->GetFileOpenChoice(getMostRecentDirectory());
            if (theFile.GetPath() != "") {
                OnLoadDocument(theFile);
                theReturn = true;
            } else {
                // User Cancels the dialog. Show startup dialog again.
                theReturn = showStartupDialog();
            }
        } break;

        case CStartupDlg::EStartupChoice_OpenRecent: {
            Qt3DSFile theFile = theStartupDlg.GetRecentDoc();
            if (theFile.GetPath() != "") {
                OnLoadDocument(theFile);
                theReturn = true;
            } else {
                // User Cancels the dialog. Show startup dialog again.
                theReturn = showStartupDialog();
            }
        } break;

        default:
            QT3DS_ASSERT(false); // Should not reach this block.
            theReturn = false;
            break;
        }
    } else { // open sample or create new
        theReturn = handleWelcomeRes(welcomeRes, true);
    }
    return theReturn;
}

#if (defined Q_OS_MACOS)
void CStudioApp::openApplication(const QString &inFilename)
{
    m_fileOpenEvent = true;
    QString loadFile = resolvePresentationFile(inFilename);
    OnLoadDocument(CString::fromQString(loadFile), true);
}
#endif

//=============================================================================
/**
 * Start the app.
 */
bool CStudioApp::blankRunApplication()
{
    initCore();
    // Event loop must be running before we launch startup dialog, or possible error message boxes
    // will cause a silent crash.
#if (defined Q_OS_MACOS)
    // Give a bit of time for Finder file open, in case that's how we were started
    QTimer::singleShot(250, this, &CStudioApp::showStartupDialog);
#else
    QTimer::singleShot(0, this, &CStudioApp::showStartupDialog);
#endif
    return runApplication();
}

//=============================================================================
/**
 * Open the specified file and run the application.
 * This will load the file then go into the standard app loop.
 * On load with the -silent flag, this would force the application to exit on
 * load failures.
 * @return true on success; false on failure
 */
bool CStudioApp::openAndRunApplication(const QString &inFilename)
{
    // Need to resolve the actual file we want to load already to be able to check for it
    QString loadFile = resolvePresentationFile(inFilename);

    // First check if the desired presentation is already open on another instance
    SharedTools::QtSingleApplication *app =
            static_cast<SharedTools::QtSingleApplication *>(QCoreApplication::instance());
    const auto pids = app->runningInstances();
    for (const auto pid : pids) {
        app->setBlock(true);
        QString query = activePresentationQuery + loadFile;
        if (app->sendMessage(query, true, 5000, pid))
            return true;
    }

    bool theSuccess = false;
    initCore();
    // Load document. Upon failure, don't show startup dialog but exit immediately.
    if (OnLoadDocument(CString::fromQString(loadFile), false))
        theSuccess = runApplication();
    return theSuccess;
}

bool CStudioApp::createAndRunApplication(const QString &filename, const QString &folder)
{
    bool theSuccess = false;
    initCore();
    // Append .uip if it is not included in the filename
    QString actualFilename = filename;
    if (!actualFilename.endsWith(QStringLiteral(".uip")))
        actualFilename.append(QStringLiteral(".uip"));
    // Create presentation
    Qt3DSFile theFile = Qt3DSFile(CString::fromQString(folder),
                                  CString::fromQString(actualFilename));
    if (theFile.GetPath() != "") {
        theSuccess = m_core->OnNewDocument(theFile, true);
        if (!theSuccess)
            return false;

        theSuccess = m_isSilent || runApplication();
    }
    return theSuccess;
}

//=============================================================================
/**
 * This is the app execution loop, the main thread loops here until the app exits.
 * @return true on success; false on failure
 */
bool CStudioApp::runApplication()
{
    m_pMainWnd->initializeGeometryAndState();
    return qApp->exec() == 0;
}

//=============================================================================
/**
 * Initialize the core and all the views.
 */
void CStudioApp::initCore()
{
    // Initialize and cache the RenderSelector values for the first time,
    // this way, subsequent attempts to instantiate a RenderSelector would circumvent the need
    // for any extra (unneccesary) creation of render contexts which inadvertently cause exceptions
    // to be thrown.

    m_core->Initialize();

    if (m_views) {
        m_views->createViews(m_isSilent);
        m_pMainWnd = m_views->getMainFrame();
    }

    RegisterGlobalKeyboardShortcuts(m_core->GetHotKeys(), m_pMainWnd);
    m_core->GetDispatch()->AddPresentationChangeListener(this);
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
        QString theTitle = QObject::tr("Warning");
        QString theMessage = QObject::tr("The following objects have action(s) that reference this "
                                         "object and/or its descendants:\n%1\nPlease fix the "
                                         "action(s) accordingly.").arg(inDescription.toQString());

        m_Dialogs.DisplayMessageBox(theTitle, theMessage, Qt3DSMessageBox::ICON_WARNING, false);
    }
};

struct SIMoveRenameHandler : public Q3DStudio::IMoveRenameHandler
{
    CDialogs &m_dialogs;

    SIMoveRenameHandler(CDialogs &dialogs)
        : m_dialogs(dialogs)
    {
    }

    void displayMessageBox(const Q3DStudio::CString &origName,
                           const Q3DStudio::CString &newName) override
    {
        QString theTitle = QObject::tr("Warning");
        QString theMessage = QObject::tr("Object %1 was renamed to %2 because "
                                         "original name was duplicated "
                                         "under its parent.")
                                            .arg(origName.toQString()).arg(newName.toQString());
        m_dialogs.DisplayMessageBox(theTitle, theMessage, Qt3DSMessageBox::ICON_WARNING, false);
    }
};

void CStudioApp::setupTimer(long inMessageId, QWidget *inWnd)
{
    m_tickTock = ITickTock::CreateTickTock(inMessageId, inWnd);
    getDirectoryWatchingSystem();
    m_core->GetDoc()->SetDirectoryWatchingSystem(m_directoryWatchingSystem);
    m_core->GetDoc()->SetImportFailedHandler(
                std::make_shared<SIImportFailedHandler>(std::ref(*GetDialogs())));
    m_core->GetDoc()->SetDocMessageBoxHandler(
                std::make_shared<SIDeletingReferencedObjectHandler>(std::ref(*GetDialogs())));
    m_core->GetDoc()->setMoveRenameHandler(
                std::make_shared<SIMoveRenameHandler>(std::ref(*GetDialogs())));
}

ITickTock &CStudioApp::getTickTock()
{
    if (m_tickTock == nullptr)
        throw std::runtime_error("Uninitialized TickTock");
    return *m_tickTock;
}

Q3DStudio::IStudioRenderer &CStudioApp::getRenderer()
{
    if (!m_renderer)
        m_renderer = Q3DStudio::IStudioRenderer::CreateStudioRenderer();
    return *m_renderer;
}

void CStudioApp::clearGuides()
{
    SCOPED_DOCUMENT_EDITOR(*m_core->GetDoc(), QObject::tr("Clear Guides"))->ClearGuides();
}

void CStudioApp::handleMessageReceived(const QString &message, QObject *socket)
{
    if (message.startsWith(activePresentationQuery)) {
        QLocalSocket *lsocket = qobject_cast<QLocalSocket *>(socket);
        if (lsocket) {
            // Another studio instance wants to know if specified presentation is open on this one
            QString checkPath(message.mid(activePresentationQuery.size()));
            QFileInfo checkFile(checkPath);
            QString docPath;
            if (m_core)
                docPath = m_core->GetDoc()->GetDocumentPath().GetAbsolutePath().toQString();
            QFileInfo openFile(docPath);
            if (!checkPath.isEmpty() && checkFile == openFile) {
                lsocket->write(SharedTools::QtLocalPeer::acceptReply(),
                               SharedTools::QtLocalPeer::acceptReply().size());
                // Since we accept active presentation query, it means the querying instance will
                // shut down and this instance must be made active window.
                if (m_pMainWnd) {
                    m_pMainWnd->setWindowState(m_pMainWnd->windowState() & ~Qt::WindowMinimized);
                    m_pMainWnd->raise();
                    m_pMainWnd->activateWindow();
                }
            } else {
                lsocket->write(SharedTools::QtLocalPeer::denyReply(),
                               SharedTools::QtLocalPeer::denyReply().size());
            }
            lsocket->waitForBytesWritten(1000);
        }
    }
    if (socket)
        delete socket;
}

void SendAsyncCommand(CDispatch &inDispatch, Q3DStudio::TCallbackFunc inFunc)
{
    inDispatch.FireOnAsynchronousCommand(inFunc);
}

IDirectoryWatchingSystem &CStudioApp::getDirectoryWatchingSystem()
{
    if (m_directoryWatchingSystem == nullptr) {
        Q3DStudio::TCallbackCaller theCaller =
                std::bind(SendAsyncCommand, std::ref(*m_core->GetDispatch()),
                          std::placeholders::_1);
        m_directoryWatchingSystem =
                IDirectoryWatchingSystem::CreateThreadedDirectoryWatchingSystem(theCaller);
    }
    return *m_directoryWatchingSystem;
}

CCore *CStudioApp::GetCore()
{
    return m_core;
}

//=============================================================================
/**
 * Get the view manager for this core to communicate to the views.
 */
CViews *CStudioApp::GetViews()
{
    return m_views;
}

//=============================================================================
/**
 * Get the dialog manager for this core for displaying dialogs.
 */
CDialogs *CStudioApp::GetDialogs()
{
    return m_dialogs;
}

long CStudioApp::GetToolMode()
{
    return m_toolMode;
}

void CStudioApp::SetToolMode(long inToolMode)
{
    if (m_toolMode != inToolMode) {
        m_toolMode = inToolMode;
        m_core->GetDispatch()->FireOnToolbarChange();
    }
}

long CStudioApp::GetSelectMode()
{
    return m_selectMode;
}

void CStudioApp::SetSelectMode(long inSelectMode)
{
    if (m_selectMode != inSelectMode) {
        m_selectMode = inSelectMode;
        m_core->GetDispatch()->FireOnToolbarChange();
    }
}

StudioManipulationModes::Enum CStudioApp::GetManipulationMode() const
{
    return m_manipulationMode;
}
void CStudioApp::SetManipulationMode(StudioManipulationModes::Enum inManipulationMode)
{
    if (m_manipulationMode != inManipulationMode) {
        m_manipulationMode = inManipulationMode;
        m_core->GetDispatch()->FireOnToolbarChange();
    }
}

//=============================================================================
/**
 * return true if undo is possible
 */
bool CStudioApp::CanUndo()
{
    return m_core->GetCmdStack()->CanUndo()
            && !m_views->getMainFrame()->getTimelineWidget()->dndActive();
}

//=============================================================================
/**
 * return true if redo is possible
 */
bool CStudioApp::CanRedo()
{
    return m_core->GetCmdStack()->CanRedo();
}

void CStudioApp::OnCopy()
{
    m_core->GetDoc()->HandleCopy();
}

bool CStudioApp::CanCopy()
{
    return m_core->GetDoc()->CanCopy();
}

//=============================================================================
/**
 * Get a string describing the type of the copy operation that can be done.
 * Precedence of copying is 1) Actions; 2) Keyframes; 3) Objects
 */
QString CStudioApp::GetCopyType()
{
    QString theCopyType;

    CDoc *theDoc = m_core->GetDoc();
    if (theDoc->CanCopyAction())
        theCopyType = tr("Action");
    else if (theDoc->CanCopyKeyframe())
        theCopyType = tr("Keyframes");
    else
        theCopyType = tr("Object");

    return theCopyType;
}

//=============================================================================
/**
 * Cuts the selected object or keys
 */
void CStudioApp::OnCut()
{
    m_core->GetDoc()->HandleCut();
}

bool CStudioApp::CanCut()
{
    return m_core->GetDoc()->CanCut();
}

//=============================================================================
/**
 * Paste keys from the copied list yo
 */
void CStudioApp::OnPaste()
{
    m_core->GetDoc()->HandlePaste();
}

bool CStudioApp::CanPaste()
{
    return m_core->GetDoc()->CanPaste();
}

//=============================================================================
/**
 * Get a string describing the type of the paste operation that can be done.
 * Precedence of paste is 1) Actions; 2) Object ; 3) Keyframes
 */
QString CStudioApp::GetPasteType()
{
    QString thePasteType;

    CDoc *theDoc = m_core->GetDoc();
    if (theDoc->CanPasteAction())
        thePasteType = tr("Action");
    else if (theDoc->CanPasteObject())
        thePasteType = tr("Object");
    else
        thePasteType = tr("Keyframes");

    return thePasteType;
}

bool CStudioApp::CanChangeTimebarColor()
{
    bool theRetVal = true;
    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = m_core->GetDoc()->GetSelectedInstance();
    if (!theSelectedInstance.Valid()
            || m_core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge()->IsSceneInstance(
                theSelectedInstance)) {
        theRetVal = false;
    }

    return theRetVal;
}

//=============================================================================
/**
 * Sets any changed keyframes on the selected object
 */
void CStudioApp::HandleSetChangedKeys()
{
    m_core->GetDoc()->SetChangedKeyframes();
}

//=============================================================================
/**
 * Deletes all selected keys
 */
void CStudioApp::DeleteSelectedKeys()
{
    m_core->GetDoc()->DeleteSelectedKeys();
}

//=============================================================================
/**
 * Deletes selected object or keyframes
 */
void CStudioApp::DeleteSelectedObject(bool slide)
{
    m_core->GetDoc()->DeleteSelectedItems(slide);
}

//=============================================================================
/**
 * Handles the duplicate object command
 */
void CStudioApp::HandleDuplicateCommand(bool slide)
{
    m_core->GetDoc()->HandleDuplicateCommand(slide);
}

//=============================================================================
/**
 * return true if the selected object is duplicatable
 */
bool CStudioApp::CanDuplicateObject()
{
    // Get the currently selected object
    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = m_core->GetDoc()->GetSelectedInstance();
    if (!theSelectedInstance.Valid())
        return false;

    // Check if the object can be duplicated
    return m_core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge()->IsDuplicateable(
                theSelectedInstance);
}

//==============================================================================
/**
 * Toggles the state of autoset keyframes.
 */
void CStudioApp::OnToggleAutosetKeyframes()
{
    SetAutosetKeyframes(!CStudioPreferences::IsAutosetKeyframesOn());

    m_core->GetDispatch()->FireOnToolbarChange();
}

//==============================================================================
/**
 * Updates the preferences, and AnimationSystem.
 */
void CStudioApp::SetAutosetKeyframes(bool inFlag)
{
    CStudioPreferences::SetAutosetKeyframesOn(inFlag);

    m_core->GetDoc()->GetStudioSystem()->GetAnimationSystem()->SetAutoKeyframe(inFlag);
}

//==============================================================================
/**
 *	If the presentation is not currently playing, this function will make it
 *	start playing from the current position.  The starting point of the playhead
 *	is saved so that it can be restored later.
 */
void CStudioApp::PlaybackPlay()
{
    CDoc *theDoc = m_core->GetDoc();
    if (!theDoc->IsPlaying()) {
        m_playbackTime = theDoc->GetCurrentViewTime();
        m_playbackOriginalSlide = theDoc->GetActiveSlide();
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
    m_core->GetDoc()->SetPlayMode(PLAYMODE_STOP);
}

//==============================================================================
/**
 *	Moves the playhead back to time zero.
 */
void CStudioApp::PlaybackRewind()
{
    CDoc *theDoc = m_core->GetDoc();
    if (theDoc->IsPlaying()) {
        theDoc->SetPlayMode(PLAYMODE_STOP, 0);
        theDoc->SetPlayMode(PLAYMODE_PLAY);
    } else {
        m_core->GetDoc()->NotifyTimeChanged(0);
    }
}

bool CStudioApp::IsPlaying()
{
    return m_core->GetDoc()->IsPlaying();
}

//=============================================================================
/**
 * Performs a file revert.
 * This will revert the doc to the last saved version.
 */
void CStudioApp::OnRevert()
{
    if (!m_core->GetDoc()->IsModified() || m_dialogs->ConfirmRevert()) {
        Qt3DSFile theCurrentDoc = m_core->GetDoc()->GetDocumentPath();
        OnLoadDocument(theCurrentDoc);
    }
}

//=============================================================================
/**
 * Check to see if it is possible to perform a revert.
 */
bool CStudioApp::CanRevert()
{
    return m_core->GetDoc()->IsModified() && m_core->GetDoc()->GetDocumentPath().GetPath() != "";
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
    if (m_core->GetDoc()->IsModified()) {
        CDialogs::ESavePromptResult theResult = m_dialogs->PromptForSave();
        if (theResult == CDialogs::SAVE_FIRST) {
            bool onSaveResult = OnSave();
            if (onSaveResult)
                return true;
        } else if (theResult == CDialogs::CONTINUE_NO_SAVE) {
            return true;
        }

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
    CDoc *theDoc = m_core->GetDoc();
    // change it back to the original slide first before restoring the original time
    if (m_playbackOriginalSlide.Valid()) {
        if (m_playbackOriginalSlide != theDoc->GetActiveSlide())
            theDoc->NotifyActiveSlideChanged(m_playbackOriginalSlide);
        theDoc->SetPlayMode(PLAYMODE_STOP, m_playbackTime);
    }
    // Invalidate the playback original slide so we don't inadvertently trigger this code later.
    m_playbackOriginalSlide = 0;
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::AdvanceTime()
{
    long theDeltaTime = CStudioPreferences::GetTimeAdvanceAmount();
    long theTime =
            (m_core->GetDoc()->GetCurrentViewTime() + theDeltaTime) / theDeltaTime * theDeltaTime;
    m_core->GetDoc()->NotifyTimeChanged(theTime);
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::ReduceTime()
{
    long theDeltaTime = CStudioPreferences::GetTimeAdvanceAmount();
    long theTime = (m_core->GetDoc()->GetCurrentViewTime() - 1) / theDeltaTime * theDeltaTime;
    m_core->GetDoc()->NotifyTimeChanged(theTime);
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::AdvanceUltraBigTime()
{
    long theDeltaTime = CStudioPreferences::GetBigTimeAdvanceAmount();
    long theTime =
            (m_core->GetDoc()->GetCurrentViewTime() + theDeltaTime) / theDeltaTime * theDeltaTime;
    m_core->GetDoc()->NotifyTimeChanged(theTime);
}

//=============================================================================
/**
 * Used for track wheel to do smooth tracking on mac, just scrolls the playhead.
 */
void CStudioApp::ReduceUltraBigTime()
{
    long theDeltaTime = CStudioPreferences::GetBigTimeAdvanceAmount();
    long theTime = (m_core->GetDoc()->GetCurrentViewTime() - 1) / theDeltaTime * theDeltaTime;
    m_core->GetDoc()->NotifyTimeChanged(theTime);
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
    if (m_core->GetDoc()->IsPlaying())
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
                        *this, m_core,
                        inSelectable.getData<Q3DStudio::SSlideInstanceWrapper>().m_Instance);
            break;
        case Q3DStudio::SelectedValueTypes::MultipleInstances:
        case Q3DStudio::SelectedValueTypes::Instance: {

            // We need to decide whether to display SlideInspectable or UICDMInspectable
            // We display SlideInspectable if user selects a Scene or Component where the current
            // active slide belongs,
            // for example when user selects the Root in Timeline Palette
            CDoc *theDoc = m_core->GetDoc();
            qt3dsdm::TInstanceHandleList theSelectedInstances =
                    theDoc->GetSelectedValue().GetSelectedInstances();
            qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance;
            if (theSelectedInstances.size() == 1)
                theSelectedInstance = theSelectedInstances[0];

            if (m_core->GetDoc()->GetDocumentReader().IsInstance(theSelectedInstance)) {
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
                                    *this, m_core, theSelectedInstance,
                                    theCurrentActiveSlideInstance);
                    else if (theBridge->IsComponentInstance(theSelectedInstance))
                        theInspectableBase = new Qt3DSDMInspectable(
                                    *this, m_core, theSelectedInstance,
                                    theCurrentActiveSlideInstance);
                }
                if (theInspectableBase == nullptr) {
                    if (theBridge->IsMaterialBaseInstance(theSelectedInstance))
                        theInspectableBase =
                                new Qt3DSDMMaterialInspectable(*this, m_core, theSelectedInstance);
                    else
                        theInspectableBase =
                                new Qt3DSDMInspectable(*this, m_core, theSelectedInstance);
                }
            }
        } break;
        case Q3DStudio::SelectedValueTypes::Guide: {
            qt3dsdm::Qt3DSDMGuideHandle theGuide
                    = inSelectable.getData<qt3dsdm::Qt3DSDMGuideHandle>();
            theInspectableBase = CGuideInspectable::CreateInspectable(*m_core, theGuide);
        } break;
        };
    }

    return theInspectableBase;
}

void CStudioApp::RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler,
                                                 QWidget *actionParent)
{
    m_core->RegisterGlobalKeyboardShortcuts(inShortcutHandler, actionParent);

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
                new CDynHotKeyConsumer<CStudioApp>(this, &CStudioApp::playbackPreviewEnd), 0,
                Qt::Key_Space);
    inShortcutHandler->RegisterKeyDownEvent(
                new CDynHotKeyConsumer<CStudioApp>(this, &CStudioApp::playbackPreviewStart), 0,
                Qt::Key_Space);

    if (m_views)
        m_views->registerGlobalKeyboardShortcuts(inShortcutHandler, actionParent);
}

void CStudioApp::playbackPreviewStart()
{
    if (!m_playbackPreviewOn) {
        m_playbackPreviewOn = true;
        m_core->GetDoc()->setPlayBackPreviewState(true);
        PlaybackPlay();
    }
}

void CStudioApp::playbackPreviewEnd()
{
    m_core->GetDoc()->setPlayBackPreviewState(false);
    m_playbackPreviewOn = false;
    PlaybackStop();
}

bool CStudioApp::isPlaybackPreviewOn() const
{
    return m_playbackPreviewOn;
}

//=============================================================================
/**
 * Handles the Save command
 * This will save the file, if the file has not been saved before this will
 * do a save as operation.
 * @param autosave set true if triggering an autosave.
 * @return true if the file was successfully saved.
 */
bool CStudioApp::OnSave(bool autosave)
{
    Qt3DSFile theCurrentDoc = m_core->GetDoc()->GetDocumentPath();
    if (!theCurrentDoc.IsFile()) {
        if (autosave)
            return false;
        else
            return OnSaveAs();
    } else if (!theCurrentDoc.CanWrite()) {
        m_dialogs->DisplaySavingPresentationFailed();
        return false;
    } else {
        // Compose autosave filename (insert _autosave before extension)
        QString autosaveFile = theCurrentDoc.GetPath().toQString();
        int insertionPoint = autosaveFile.lastIndexOf(QStringLiteral(".uip"));
        autosaveFile.insert(insertionPoint, QStringLiteral("_autosave"));

        if (autosave) {
            // Set the copy flag to avoid changing actual document name & history
            m_core->OnSaveDocument(Qt3DSFile(CString::fromQString(autosaveFile)), true);
        } else {
            m_core->OnSaveDocument(theCurrentDoc);
            // Delete previous autosave file
            QFile::remove(autosaveFile);
        }

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
    Qt3DSFile theFile = m_dialogs->GetSaveAsChoice();
    if (theFile.GetPath() != "") {
        m_core->OnSaveDocument(theFile);
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
    Qt3DSFile theFile = m_dialogs->GetSaveAsChoice();
    if (theFile.GetPath() != "") {
        // Send in a "true" to the save function to indicate this is a copy
        m_core->OnSaveDocument(theFile, true);
        return true;
    }
    return false;
}

void CStudioApp::SetAutosaveEnabled(bool enabled)
{
    if (enabled)
        m_autosaveTimer->start();
    else
        m_autosaveTimer->stop();
}

void CStudioApp::SetAutosaveInterval(int interval)
{
    m_autosaveTimer->setInterval(interval * 1000);
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
    bool theLoadResult = false;
    QString theLoadErrorParameter;
    QString theErrorText;
    QString loadFile = resolvePresentationFile(inDocument.GetPath().toQString());
    Qt3DSFile loadDocument(CString::fromQString(loadFile));

    m_core->GetDispatch()->FireOnProgressBegin(CString::fromQString(QObject::tr("Loading ")),
                                               loadDocument.GetName());

    // Make sure scene is visible
    if (m_views)
        m_views->getMainFrame()->showScene();

    try {
        OnLoadDocumentCatcher(loadDocument);
        m_core->GetDispatch()->FireOnOpenDocument(loadDocument, true);
        // Loading was successful
        theLoadResult = true;
    } catch (CUnsupportedFileFormatException &) {
        theErrorText = tr("The file could not be opened. It is either invalid or was made with an "
                          "old version of Studio.");
        // We've encountered a file format that is older than the current, OR
        // corrupt files, unsupported file formats and illegal types.
    } catch (CInvalidFileFormatException &) {
        theErrorText = tr("The file could not be opened. The file format is invalid.");
    } catch (CLoadReferencedFileException &inError) {
        // referenced files (e.g. Data Files) failed to load
        theErrorText = tr("%1 failed to load due to invalid referenced file: %2.").arg(
                    loadDocument.GetName().toQString(),
                    Q3DStudio::CString(inError.GetFilePath()).toQString());
        const wchar_t *theDesc = inError.GetDescription();
        if (theDesc && wcslen(theDesc) > 0) {
            // append any description is provided
            theErrorText += QStringLiteral("\n")
                    + Q3DStudio::CString(inError.GetDescription()).toQString();
        }
    } catch (CIOException &) { // provide specific error message if possible
        if (loadDocument.Exists() == false)
            theLoadErrorParameter = tr(" does not exist.");
        qCCritical(qt3ds::INTERNAL_ERROR)
                << "Failed to load document, IO error (file may be unreadable or nonexistent)";
    } catch (...) {
        qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to load document, unknown error";
        // We don't know exactly what went wrong during a load, but let studio 'fail gracefully'.
    }

    if (!theErrorText.isEmpty()) {
        qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to load document: "
                                          << theErrorText;
    }

    m_core->GetDispatch()->FireOnProgressEnd();

    // load fail
    if (!theLoadResult) {
#if (defined Q_OS_MACOS)
        m_fileOpenEvent = false;
#endif
        if (!theErrorText.isEmpty())
            m_dialogs->DisplayKnownErrorDialog(theErrorText);
        else
            m_dialogs->DisplayLoadingPresentationFailed(loadDocument, theLoadErrorParameter);

        m_core->GetDispatch()->FireOnOpenDocument(loadDocument, false);

        // Show startup dialog
        if (inShowStartupDialogOnError) {
            if (!showStartupDialog())
                qApp->quit();
        }
    } else {
        m_dialogs->ResetSettings(loadDocument.GetPath());

        m_subpresentations.clear();
        m_core->GetDoc()->LoadUIASubpresentations(m_core->GetDoc()->GetDocumentUIAFile(true),
                                                  m_subpresentations);

        m_dataInputDialogItems.clear();
        m_core->GetDoc()->LoadUIADataInputs(m_core->GetDoc()->GetDocumentUIAFile(true),
                                            m_dataInputDialogItems);
    }

    m_authorZoom = false;

    m_core->GetDispatch()->FireAuthorZoomChanged();

    checkDeletedDatainputs();

    return theLoadResult;
}

//=============================================================================
/**
 *
 */
void CStudioApp::SaveUIAFile(bool subpresentations)
{
    QStringList list;
    if (subpresentations) {
        for (SubPresentationRecord r : m_subpresentations) {
            list.append(r.m_type);
            list.append(r.m_id);
            list.append(r.m_argsOrSrc);
        }
    } else {
        for (CDataInputDialogItem *item : m_dataInputDialogItems) {
            list.append(item->name);
            if (item->type == EDataType::DataTypeRangedNumber)
                list.append(QStringLiteral("Ranged Number"));
            else if (item->type == EDataType::DataTypeString)
                list.append(QStringLiteral("String"));
            else if (item->type == EDataType::DataTypeFloat)
                list.append(QStringLiteral("Float"));
            else if (item->type == EDataType::DataTypeBoolean)
                list.append(QStringLiteral("Boolean"));
            else if (item->type == EDataType::DataTypeVector3)
                list.append(QStringLiteral("Vector3"));
            else if (item->type == EDataType::DataTypeVector2)
                list.append(QStringLiteral("Vector2"));
#ifdef DATAINPUT_EVALUATOR_ENABLED
            else if (item->type == EDataType::DataTypeEvaluator)
                list.append(QStringLiteral("Evaluator"));
#endif
            else if (item->type == EDataType::DataTypeVariant)
                list.append(QStringLiteral("Variant"));

            // Write min and max regardless of type, as we will get a mess if number of parameters
            // varies between different types
            list.append(QString::number(item->minValue));
            list.append(QString::number(item->maxValue));
            // Write evaluator expression
            list.append(QString(item->valueString));
        }
    }
    Q3DStudio::CFilePath doc(GetCore()->GetDoc()->GetDocumentPath().GetAbsolutePath());
    QByteArray docBA = doc.toQString().toLatin1();
    qt3ds::state::IApplication::EnsureApplicationFile(docBA.constData(), list, subpresentations);
}

CFilePath CStudioApp::getMostRecentDirectory() const
{
    CFilePath mostRecentDirectory = Q3DStudio::CFilePath(".");
    if (m_views) {
        CRecentItems *recentItems = m_views->getMainFrame()->GetRecentItems();
        if (recentItems->GetItemCount() > 0) {
            mostRecentDirectory = CFilePath(recentItems->GetItem(0).GetAbsolutePath())
                    .GetDirectory();
        }
    }
    return mostRecentDirectory;
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
        CDispatchDataModelNotificationScope __scope(*m_core->GetDispatch());
        m_core->GetDoc()->CloseDocument();
        m_core->GetDoc()->LoadDocument(inDocument);
    }

    // Make sure the client scene is resized properly
    if (m_views)
        m_views->recheckMainframeSizingMode();
}

void CStudioApp::OnFileOpen()
{
    if (PerformSavePrompt()) {
        Qt3DSFile theFile = m_dialogs->GetFileOpenChoice(getMostRecentDirectory());
        if (theFile.GetPath() != "")
            OnLoadDocument(theFile);
    }
}

QString CStudioApp::OnFileNew(bool createFolder)
{
    if (PerformSavePrompt()) {
        Qt3DSFile theFile = m_dialogs->GetNewDocumentChoice(Q3DStudio::CString(""), createFolder);
        if (theFile.GetPath() != "") {
            if (!m_core->OnNewDocument(theFile, createFolder))
                showInvalidFilenameWarning();
        } else {
            return theFile.GetName().toQString();
        }
    }
    return QString();
}

bool CStudioApp::IsAuthorZoom()
{
    return m_authorZoom;
}

void CStudioApp::SetAuthorZoom(bool inZoom)
{
    if (m_authorZoom != inZoom) {
        m_authorZoom = inZoom;
        m_core->GetDispatch()->FireAuthorZoomChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
// These commands come over the dispatch from inside the core. The core doesn't
// have access to the CMsgRouter at the moment, so this relays the message.
void CStudioApp::OnAsynchronousCommand(CCmd *inCmd)
{
    CMsgRouter::GetInstance()->SendCommand(inCmd, m_core);
}

void CStudioApp::OnDisplayAppStatus(Q3DStudio::CString &inStatusMsg)
{
    // Do nothing, it was used to show this in the status bar
}

void CStudioApp::OnProgressBegin(const Q3DStudio::CString &inActionText,
                                 const Q3DStudio::CString &inAdditionalText)
{
    m_dialogs->DisplayProgressScreen(inActionText, inAdditionalText);
}

void CStudioApp::OnProgressEnd()
{
    m_dialogs->DestroyProgressScreen();
}

void CStudioApp::OnAssetDeleteFail()
{
    m_dialogs->DisplayAssetDeleteFailed();
}

void CStudioApp::OnPasteFail()
{
    m_dialogs->DisplayPasteFailed();
}

void CStudioApp::OnBuildconfigurationFileParseFail(const Q3DStudio::CString &inMessage)
{
    m_dialogs->DisplayMessageBox(tr("Build Configurations Error"), inMessage.toQString(),
                                 Qt3DSMessageBox::ICON_ERROR, false);
}

void CStudioApp::OnSaveFail(bool inKnownError)
{
    qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to save project: "
                                      << (inKnownError ? "KnownError" : "UnknownError");
    if (inKnownError)
        m_dialogs->DisplaySavingPresentationFailed();
    else
        m_dialogs->DisplayKnownErrorDialog(tr("Unknown error encountered while saving."));
}

void CStudioApp::OnProjectVariableFail(const Q3DStudio::CString &inMessage)
{
    m_dialogs->DisplayEnvironmentVariablesError(inMessage);
}

void CStudioApp::OnErrorFail(const Q3DStudio::CString &inText)
{
    qCCritical(qt3ds::INTERNAL_ERROR) << inText.GetCharStar();
    m_dialogs->DisplayMessageBox(tr("Qt 3D Studio"), inText.toQString(),
                                 Qt3DSMessageBox::ICON_ERROR, false);
}

void CStudioApp::OnRefreshResourceFail(const Q3DStudio::CString &inResourceName,
                                       const Q3DStudio::CString &inDescription)
{
    qCCritical(qt3ds::INTERNAL_ERROR) << "Failed to refresh resource: "
                                      << inResourceName.GetCharStar();
    qCCritical(qt3ds::INTERNAL_ERROR) << inDescription.GetCharStar();
    m_dialogs->DisplayRefreshResourceFailed(inResourceName, inDescription);
}

void CStudioApp::OnNewPresentation()
{
    m_core->GetDoc()->GetStudioSystem()->GetAnimationSystem()->SetAutoKeyframe(
                CStudioPreferences::IsAutosetKeyframesOn());
    qCInfo(qt3ds::TRACE_INFO) << "New Presentation: "
                              << m_core->GetDoc()->GetDocumentPath().GetAbsolutePath().GetCharStar();
}

void CStudioApp::OnPresentationModifiedExternally()
{
    int theUserChoice = m_dialogs->DisplayChoiceBox(
                tr("Warning!"),
                tr("This project has changed on disk. Do you want to reload it?"),
                Qt3DSMessageBox::ICON_WARNING);
    if (theUserChoice == IDYES) {
        Qt3DSFile theCurrentDoc = m_core->GetDoc()->GetDocumentPath();
        OnLoadDocument(theCurrentDoc);
    }
}

void CStudioApp::OnUndefinedDatainputsFail(
        const QMultiMap<QString, QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                       qt3dsdm::Qt3DSDMPropertyHandle>> *map)
{
    bool res = m_dialogs->DisplayUndefinedDatainputDlg(map);

    // Delete invalid datainput bindings if user prompted so
    if (res)
        m_core->GetDoc()->RemoveDatainputBindings(map);
}

void CStudioApp::toggleEyeball()
{
    CDoc *theDoc = m_core->GetDoc();
    qt3dsdm::IPropertySystem *propertySystem = theDoc->GetStudioSystem()->GetPropertySystem();
    qt3dsdm::TInstanceHandleList selectedInstances
            = theDoc->GetSelectedValue().GetSelectedInstances();

    if (selectedInstances.size() > 0) {
        Q3DStudio::ScopedDocumentEditor editor(*theDoc,
                                               L"Visibility Toggle",
                                               __FILE__, __LINE__);
        bool boolValue = false;
        SValue value;
        for (size_t idx = 0, end = selectedInstances.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle handle(selectedInstances[idx]);

            if (handle.Valid()) {
                qt3dsdm::Qt3DSDMPropertyHandle property
                        = theDoc->GetStudioSystem()->GetClientDataModelBridge()
                        ->GetSceneAsset().m_Eyeball;
                if (value.empty()) {
                    // First valid handle selects if all are hidden/unhidden
                    propertySystem->GetInstancePropertyValue(handle, property, value);
                    boolValue = !qt3dsdm::get<bool>(value);
                }
                editor->SetInstancePropertyValue(handle, property, boolValue);
            }
        }
    }
}

void CStudioApp::showInvalidFilenameWarning()
{
    m_dialogs->DisplayMessageBox(tr("Invalid filename"),
                                 tr("The filename given was invalid."),
                                 Qt3DSMessageBox::ICON_WARNING, false);
}

void CStudioApp::checkDeletedDatainputs()
{
    QMultiMap<QString, QPair<qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::Qt3DSDMPropertyHandle>> *map;
    map = new QMultiMap<QString, QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                       qt3dsdm::Qt3DSDMPropertyHandle>>;
    m_core->GetDoc()->UpdateDatainputMap(m_core->GetDoc()->GetActiveRootInstance(), map);

    if (!map->empty())
        m_core->GetDispatch()->FireOnUndefinedDatainputsFail(map);
}
