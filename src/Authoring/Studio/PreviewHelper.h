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
#ifndef INCLUDED_PREVIEW_HELPER
#define INCLUDED_PREVIEW_HELPER 1

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSFile.h"
#include "Qt3DSString.h"
#include "remotedeploymentsender.h"

//==============================================================================
//	Forwards
//==============================================================================
class CHotKeys;

namespace Q3DStudio {
class CBuildConfiguration;
}
QT_FORWARD_DECLARE_CLASS(QProcess)

//==============================================================================
/**
 *	@class CPreviewHelper
 */
class CPreviewHelper
{
public:
    enum EExecMode {
        EXECMODE_PREVIEW, ///< Preview
        EXECMODE_DEPLOY, ///< Deploy
    };

public:
    static void OnPreview(const QString &viewerExeName);
    static void OnDeploy(RemoteDeploymentSender &project);
    static void PreviewViaConfig(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                 EExecMode inMode, const QString &viewerExeName,
                                 RemoteDeploymentSender *project = 0);
    static void DoPreviewViaConfig(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                   const Q3DStudio::CString &inDocumentFile,
                                   EExecMode inMode, const QString &viewerExeName,
                                   RemoteDeploymentSender *project = 0);
    static bool viewerExists(const QString &exeName);

protected:
    static Q3DStudio::CString InterpretString(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                              const Q3DStudio::CString &inDocumentFile,
                                              const Q3DStudio::CString &inSourceString);
    static bool ResolveVariable(Q3DStudio::CBuildConfiguration *inSelectedConfig,
                                const Q3DStudio::CString &inDocumentFile,
                                const Q3DStudio::CString &inVariable, Q3DStudio::CString &outValue);
    static Qt3DSFile GetDocumentFile(bool &outUsingTempFile);
    static Q3DStudio::CString GetLaunchFile(const Q3DStudio::CString &inDocFile);
    static QString getViewerFilePath(const QString &exeName);
    static void cleanupProcess(QProcess *p, QString *pDocStr);
};

#endif
