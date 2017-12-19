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
#include "PreviewHelper.h"
#include "StudioApp.h"
#include "Dialogs.h"
#include "Dispatch.h"
#include "Doc.h"
#include "StudioPreferences.h"
#include "StudioProjectSettings.h"
#include "StudioProjectVariables.h"
#include "Core.h"
#include "Qt3DSFileTools.h"

#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qmessagebox.h>
#include <QtCore/qprocess.h>

#include "remotedeploymentsender.h"

static const Q3DStudio::CString previewSuffix("_@preview@");
static const Q3DStudio::CString uipSuffix(".uip");
static const Q3DStudio::CString uiaSuffix(".uia");

Q3DStudio::CString CPreviewHelper::GetLaunchFile(const Q3DStudio::CString &inUipPath)
{
    const bool isPreview = inUipPath.Right(previewSuffix.Length() + uipSuffix.Length())
            .Compare(previewSuffix + uipSuffix);

    Q3DStudio::CFilePath theUipPath(inUipPath);

    Q3DStudio::CString theDir = theUipPath.GetDirectory();
    Q3DStudio::CString theStem = theUipPath.GetFileStem();
    if (isPreview)
        theStem = theStem.Left(theStem.Length() - previewSuffix.Length());

    // Check if a corresponding .UIA file actually exists
    theDir.append('/');
    Q3DStudio::CString uiaPath = theDir + theStem + uiaSuffix;

    Q3DStudio::CFilePath theUiaPath(uiaPath);
    if (theUiaPath.IsFile()) {
        if (isPreview) {
            // We need to make a preview .uia that points to the preview .uip
            Q3DStudio::CString previewPath = theDir + theStem + previewSuffix + uiaSuffix;
            QFile previewFile(previewPath.toQString());
            QFile origFile(uiaPath.toQString());
            if (previewFile.open(QIODevice::WriteOnly) && origFile.open(QIODevice::ReadOnly)) {
                QByteArray content = origFile.readAll();
                QString origUip = (theStem + uipSuffix).toQString();
                QString previewUip = (theStem + previewSuffix + uipSuffix).toQString();
                content.replace(origUip, previewUip.toUtf8());
                previewFile.write(content);
                previewFile.flush();
                return previewPath;
            } else {
                qCWarning(qt3ds::WARNING) << "Failed to create preview .uia file:"
                                          << previewPath.toQString();
                return inUipPath;
            }
        }
        return uiaPath;
    } else {
        return inUipPath;
    }
}

//=============================================================================
/**
 *	Callback for previewing a presentation.
 */
void CPreviewHelper::OnPreview(const QString &viewerExeName)
{
    Q3DStudio::CBuildConfigurations &theConfigurations =
            g_StudioApp.GetCore()->GetBuildConfigurations();
    Q3DStudio::CBuildConfiguration *theBuildConfiguration =
            theConfigurations.GetConfiguration(CStudioPreferences::GetPreviewConfig());
    if (theBuildConfiguration)
        PreviewViaConfig(theBuildConfiguration, EXECMODE_PREVIEW, viewerExeName);
}

//=============================================================================
/**
 *	Callback for deploying a presentation.
 */
void CPreviewHelper::OnDeploy(RemoteDeploymentSender &project)
{
    Q3DStudio::CBuildConfigurations &theConfigurations =
            g_StudioApp.GetCore()->GetBuildConfigurations();
    Q3DStudio::CBuildConfiguration *theBuildConfiguration =
            theConfigurations.GetConfiguration(CStudioPreferences::GetPreviewConfig());
    if (theBuildConfiguration) {
        // ItemDataPtr != nullptr ==> Build configurations specified NANT pipeline exporter
        PreviewViaConfig(theBuildConfiguration, EXECMODE_DEPLOY, QString(), &project);
    }
}

//=============================================================================
/**
 *	Previewing a presentation using the build configurations loaded.
 *	This involves 2 steps:
 *	1	Export the presentation using the specified exporter.
 *	2	Viewing the exported content following the command specified in the configuration.
 */
void CPreviewHelper::PreviewViaConfig(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                      EExecMode inMode, const QString &viewerExeName,
                                      RemoteDeploymentSender *project)
{
    bool theUsingTempFile;
    Qt3DSFile theDocument = GetDocumentFile(theUsingTempFile);
    CCore *theCore = g_StudioApp.GetCore();
    try {
        if (theUsingTempFile)
            theCore->OnSaveDocument(theDocument, true);

        DoPreviewViaConfig(inSelectedConfig, theDocument.GetAbsolutePath(),
                           inMode, viewerExeName, project);
    } catch (...) {
        theCore->GetDispatch()->FireOnProgressEnd();
        g_StudioApp.GetDialogs()->DisplaySaveReadOnlyFailed(theDocument);
    }
}

QString CPreviewHelper::getViewerFilePath(const QString &exeName)
{
    using namespace Q3DStudio;
    CFilePath currentPath(Qt3DSFile::GetApplicationDirectory().GetAbsolutePath());
    CFilePath viewerDir(QApplication::applicationDirPath());
    if (!viewerDir.IsDirectory())
        viewerDir = currentPath.GetDirectory(); // Developing directory

    QString viewerFile;
#ifdef Q_OS_WIN
    viewerFile = QStringLiteral("%1.exe").arg(exeName);
#else
#ifdef Q_OS_MACOS
    viewerFile = QStringLiteral("../../../%1.app/Contents/MacOS/%1").arg(exeName);
#else
    viewerFile = QStringLiteral("%1").arg(exeName);
#endif
#endif
    return viewerDir.filePath() + QStringLiteral("/") + viewerFile;
}

void CPreviewHelper::cleanupProcess(QProcess *p, QString *pDocStr)
{
    p->disconnect();
    QString preview = previewSuffix.toQString();
    QString uia = preview + uiaSuffix.toQString();
    QString uip = preview + uipSuffix.toQString();
    if (pDocStr->endsWith(uia) || pDocStr->endsWith(uip)) {
        QFile(*pDocStr).remove();
        if (pDocStr->endsWith(uia)) {
            pDocStr->replace(uia, uip);
            QFile(*pDocStr).remove();
        }
    }
    if (p->state() == QProcess::Running) {
        p->terminate();
        p->waitForFinished(5000); // To avoid warning about deleting a running process
    }
    p->deleteLater();
    delete pDocStr;
}

void CPreviewHelper::DoPreviewViaConfig(Q3DStudio::CBuildConfiguration * /*inSelectedConfig*/,
                                        const Q3DStudio::CString &inDocumentFile,
                                        EExecMode inMode, const QString &viewerExeName,
                                        RemoteDeploymentSender *project)
{
    using namespace Q3DStudio;

    if (inMode == EXECMODE_DEPLOY) {
        Q_ASSERT(project);
        project->streamProject(inDocumentFile.GetCharStar());
    } else if (inMode == EXECMODE_PREVIEW
               && CStudioPreferences::GetPreviewProperty("PLATFORM") == "PC") {
        // Quick Preview on PC without going via NANT
        Q3DStudio::CString theDocumentFile = CPreviewHelper::GetLaunchFile(inDocumentFile);
        QString theCommandStr = getViewerFilePath(viewerExeName);
        QString *pDocStr = new QString(theDocumentFile.toQString());

        QProcess *p = new QProcess;
        QMetaObject::Connection *connection = new QMetaObject::Connection(
                    QObject::connect(qApp, &QApplication::aboutToQuit, [p, pDocStr](){
            // connection object is never destroyed, but it doesn't matter as application is
            // quitting anyway.
            cleanupProcess(p, pDocStr);
        }));
        auto finished
                = static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished);
        QObject::connect(p, finished, [p, pDocStr, connection](){
            // Disconnect the other connection to avoid duplicate cleanup
            QObject::disconnect(*connection);
            delete connection;
            cleanupProcess(p, pDocStr);
        });
        p->start(theCommandStr, { *pDocStr });

        if (!p->waitForStarted()) {
            QMessageBox::critical(nullptr, QObject::tr("Error Launching Viewer"),
                                  QObject::tr("'%1' failed with error: '%2'")
                                  .arg(theCommandStr).arg(p->errorString()));
            delete p;
            return;
        }
    }
}

bool CPreviewHelper::viewerExists(const QString &exeName)
{
    return QFileInfo(getViewerFilePath(exeName)).exists();
}

//=============================================================================
/**
 *	Interpret the string by resolving the variables.
 *	@param	inSourceString		String to be interpreted
 */
Q3DStudio::CString CPreviewHelper::InterpretString(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                                   const Q3DStudio::CString &inDocumentFile,
                                                   const Q3DStudio::CString &inSourceString)
{
    Q3DStudio::CString theReturnString;
    long theStart = 0; // start index of string
    long theBeginIndex = 0; // index of '%'
    long theEndIndex = 0; // index of '%'
    while (Q3DStudio::CString::ENDOFSTRING != theEndIndex) {
        theBeginIndex = inSourceString.Find('%', theStart);
        if (Q3DStudio::CString::ENDOFSTRING != theBeginIndex) {
            theReturnString += inSourceString.Extract(theStart, theBeginIndex - theStart);
            // find the corresponding '%'
            theEndIndex = inSourceString.Find('%', theBeginIndex + 1);
            if (Q3DStudio::CString::ENDOFSTRING != theEndIndex) {
                // first, resolve the variable by the toolbar selection
                Q3DStudio::CString theVariable =
                        inSourceString.Extract(theBeginIndex + 1, theEndIndex - theBeginIndex - 1);
                Q3DStudio::CString theResolvedVariable;
                bool theHasResolved = ResolveVariable(inSelectedConfig, inDocumentFile, theVariable,
                                                      theResolvedVariable);

                if (theHasResolved) {
                    theReturnString += theResolvedVariable;
                } else {
                    theReturnString += "_NULL_";
                }
                theStart = theEndIndex + 1;
            } else
                theReturnString += inSourceString.Extract(theBeginIndex);
        } else {
            theEndIndex = theBeginIndex;
            theReturnString += inSourceString.Extract(theStart);
        }
    }

    return theReturnString;
}

//==============================================================================
/**
 *	Resolves the passed in variable and write out the resolved value if it exists
 *	in the current selected build configuration.
 *	@param inVariable	the environment to be resolved
 *	@param outValue		the string to receive the resolved value
 *	@return true if the variable exists, else false
 */
//==============================================================================
bool CPreviewHelper::ResolveVariable(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                     const Q3DStudio::CString &inDocumentFile,
                                     const Q3DStudio::CString &inVariable,
                                     Q3DStudio::CString &outValue)
{
    Q3DStudio::CString theReturnStr;
    bool theHasResolved = false;

    // Handle special variable
    if (inVariable == "BUILDFILE") {
        if (inSelectedConfig) {
            theReturnStr = inSelectedConfig->GetPath();
            theHasResolved = true;
        }
    } else if (inVariable == "UIPFILE") {
        theReturnStr = inDocumentFile;
        theHasResolved = true;
    }

    if (!theHasResolved) {
        Q3DStudio::CString theValue = CStudioPreferences::GetPreviewProperty(inVariable);
        if (theValue != "") {
            theReturnStr = theValue;
            theHasResolved = true;
        }
    }

    if (theHasResolved)
        outValue = InterpretString(inSelectedConfig, inDocumentFile, theReturnStr);

    return theHasResolved;
}

/**
 *  Gets a file to be previewed based on the current document.
 *  If the document is dirty, a temp file based on the current document is used.
 *  @param outUsingTempFile indicates if temp file is used
 *  @return the document file to be previewed
 */
Qt3DSFile CPreviewHelper::GetDocumentFile(bool &outUsingTempFile)
{
    Qt3DSFile result("");
    if (g_StudioApp.GetCore()->GetDoc()->IsModified()) {
        Qt3DSFile document = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath();
        Q3DStudio::CFilePath absPath = document.GetAbsolutePath();
        Q3DStudio::CString dir = absPath.GetDirectory();
        Q3DStudio::CString stem = absPath.GetFileStem();
        dir.append('/');
        result = dir + stem + previewSuffix + uipSuffix;
        outUsingTempFile = true;
    } else {
        result = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath();
        outUsingTempFile = false;
    }
    return result;
}
