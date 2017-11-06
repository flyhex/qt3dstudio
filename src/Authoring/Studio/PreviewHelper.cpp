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
#include "Strings.h"

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

#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>

#include "remotedeploymentsender.h"

//=============================================================================
/**
 *	PKC : Yes, we're duplicating functionality found in UICStateApplication
 *  but we want to eliminate as many dependencies on UICState as possible in Studio.
 */

Q3DStudio::CString CPreviewHelper::GetLaunchFile(const Q3DStudio::CString &inUipPath)
{
    Q3DStudio::CFilePath theUipPath(inUipPath);

    Q3DStudio::CString theDir = theUipPath.GetDirectory();
    Q3DStudio::CString theStem = theUipPath.GetFileStem();

    // Check if a corresponding .UIA file actually exists
    theDir.append('/');
    Q3DStudio::CString idealPath = theDir + theStem + Q3DStudio::CString(".uia");

    Q3DStudio::CFilePath theUiaPath(idealPath);

    return (theUiaPath.IsFile()) ? idealPath : inUipPath;
}

//=============================================================================
/**
 *	Callback for previewing a presentation.
 */
void CPreviewHelper::OnPreview()
{
    Q3DStudio::CBuildConfigurations &theConfigurations =
            g_StudioApp.GetCore()->GetBuildConfigurations();
    Q3DStudio::CBuildConfiguration *theBuildConfiguration =
            theConfigurations.GetConfiguration(CStudioPreferences::GetPreviewConfig());
    if (theBuildConfiguration)
        PreviewViaConfig(theBuildConfiguration, EXECMODE_PREVIEW);
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
        PreviewViaConfig(theBuildConfiguration, EXECMODE_DEPLOY, &project);
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
                                      EExecMode inMode, RemoteDeploymentSender *project)
{
    bool theUsingTempFile;
    Qt3DSFile theDocument = GetDocumentFile(theUsingTempFile);
    CCore *theCore = g_StudioApp.GetCore();
    try {
        if (theCore->GetDoc()->IsModified()) {
            theCore->OnSaveDocument(theDocument);
        }

        DoPreviewViaConfig(inSelectedConfig, theDocument.GetAbsolutePath(),
                           inMode, project);
    } catch (...) {
        theCore->GetDispatch()->FireOnProgressEnd();
        g_StudioApp.GetDialogs()->DisplaySaveReadOnlyFailed(theDocument);
    }
}

//=============================================================================
/**
 *	Launch a viewing app to preview a file.
 *	@param	inDocumentFile		File to be previewed
 */
void CPreviewHelper::DoPreviewViaConfig(Q3DStudio::CBuildConfiguration * /*inSelectedConfig*/,
                                        const Q3DStudio::CString &inDocumentFile,
                                        EExecMode inMode,
                                        RemoteDeploymentSender *project)
{
    using namespace Q3DStudio;
    QString theCommandStr;

    if (inMode == EXECMODE_DEPLOY) {
        Q_ASSERT(project);
        project->streamProject(inDocumentFile.GetCharStar());
    } else if (inMode == EXECMODE_PREVIEW
               && CStudioPreferences::GetPreviewProperty("PLATFORM") == "PC") {
        // Quick Preview on PC without going via NANT
        CFilePath theCurrentPath(Qt3DSFile::GetApplicationDirectory().GetAbsolutePath());
        CFilePath theViewerDir(QApplication::applicationDirPath());
        if (!theViewerDir.IsDirectory()) {
            // theMainDir = theCurrentPath.GetDirectory().GetDirectory();
            // theViewerDir = CFilePath::CombineBaseAndRelative( theMainDir, CFilePath(
            // L"Runtime/Build/Bin/Win32" ) );
            theViewerDir = theCurrentPath.GetDirectory(); // Developing directory
        }

        Q3DStudio::CString theDocumentFile = CPreviewHelper::GetLaunchFile(inDocumentFile);
#ifdef Q_OS_WIN
        QString theViewerFile = "Qt3DViewer.exe";
        theCommandStr = theViewerDir.filePath() + "\\" + theViewerFile;
#else
#ifdef Q_OS_MACOS
        QString theViewerFile = "../../../Qt3DViewer.app/Contents/MacOS/Qt3DViewer";
#else
        QString theViewerFile = "Qt3DViewer";
#endif
        theCommandStr = theViewerDir.filePath() + "/" + theViewerFile;
#endif

        QProcess *p = new QProcess;
        auto finished = static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished);
        QObject::connect(p, finished, p, &QObject::deleteLater);
        p->start(theCommandStr, { theDocumentFile.toQString() });

        if (!p->waitForStarted()) {
            QMessageBox::critical(nullptr, QObject::tr("Error Launching Viewer"), QObject::tr("%1 failed with error: %2")
                                  .arg(theViewerFile).arg(p->errorString()));
            delete p;
            return;
        }
    }
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

//=============================================================================
/**
 *	Gets a file to be previewed or conditioned based on the current document.
 *	If the document has not been saved yet (and thus does not have a name), a temp file is used.
 *	If the document has been saved but it's dirty, a temp file based on the current document is
 *used (same path but different extension).
 *	@param	outUsingTempFile	if temp file if used
 *	@return the document file to be previewed or conditioned
 */
Qt3DSFile CPreviewHelper::GetDocumentFile(bool &outUsingTempFile)
{
    Qt3DSFile theDocument = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath();
    Qt3DSFile theResult("");
    theResult = theDocument;
    outUsingTempFile = false;
    return theResult;
}
