/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

//=============================================================================
// Prefix
//=============================================================================
#ifndef INCLUDED_DIALOGS_H
#define INCLUDED_DIALOGS_H 1

#pragma once

//=============================================================================
// Includes
//=============================================================================
#include "Qt3DSFile.h"
#include "StudioObjectTypes.h"
#include "Qt3DSMessageBox.h"
//#include "MultilineEditDlg.h"
#include "Qt3DSFileTools.h"
#include "CColor.h"
#include <QMessageBox>

//=============================================================================
// Forwards
//=============================================================================
class IDoc;
class CStudioApp;
class CControl;
class CDialogControl;

class CProgressView;

class CDialogs
{
public:
    enum ESavePromptResult {
        CANCEL_OPERATION,
        CONTINUE_NO_SAVE,
        SAVE_FIRST,
    };

    CDialogs(bool inShowGUI = true);
    virtual ~CDialogs();

    void DisplayAssetDeleteFailed();
    void DisplayRefreshResourceFailed(const Q3DStudio::CString &inResourceName,
                                      const Q3DStudio::CString &inDescription);
    QString ConfirmRefreshModelFile(const QString &inOriginalPath);

    // This is not an appropriate place for these, but better
    // in an inappropriate place than duplicated
    static const char *GetDAEFileExtension();
    static const char *GetFbxFileExtension();
    // Null terminated list
    static const char **GetImgFileExtensions();
    static const char *GetImportFileExtension();
    static const char *GetMeshFileExtension();
    static const char *GetLUAFileExtension();
    static const char *GetQmlFileExtension();
    static const char **GetFontFileExtensions();
    static const char **GetEffectFileExtensions();
    static const char **GetMaterialFileExtensions();
    static const char **GetSoundFileExtensions();
    static bool IsImageFileExtension(const char *inExt);
    static bool IsFontFileExtension(const char *inExt);
    static bool IsEffectFileExtension(const char *inExt);
    static bool IsMaterialFileExtension(const char *inExt);
    static bool IsSoundFileExtension(const char *inExt);

    static const wchar_t *GetWideDAEFileExtension();
    static const wchar_t *GetWideFbxFileExtension();
    static const wchar_t *GetWideImportFileExtension();
    static const wchar_t *GetWideMeshFileExtension();
    static const wchar_t *GetWideLUAFileExtension();
    static const wchar_t **GetWideFontFileExtensions();
    static const wchar_t **GetWideImgFileExtensions();
    static const wchar_t **GetWideEffectFileExtensions();
    static const wchar_t **GetWideMaterialFileExtensions();
    static const wchar_t **GetWideSoundFileExtensions();
    static bool IsImageFileExtension(const wchar_t *inExt);
    static bool IsFontFileExtension(const wchar_t *inExt);
    static bool IsEffectFileExtension(const wchar_t *inExt);
    static bool IsMaterialFileExtension(const wchar_t *inExt);
    static bool IsPathFileExtension(const wchar_t *inExt);
    static bool IsPathBufferExtension(const wchar_t *inExt);
    static bool IsSoundFileExtension(const wchar_t *inExt);

    Qt3DSFile GetExportChoice(const Q3DStudio::CString &inExtension,
                             const Q3DStudio::CString &inDefaultName);

    std::pair<Qt3DSFile, bool> GetSaveAsChoice(const Q3DStudio::CString &inDialogTitle = "",
                                              bool inFilenameUntitled = false);
    // Returns pair of file along with a boolean indicating the state of the create
    // new directory checkbox.
    std::pair<Qt3DSFile, bool>
    GetNewDocumentChoice(const Q3DStudio::CString &inInitialDirectory = Q3DStudio::CString());
    Qt3DSFile GetFileOpenChoice(const Q3DStudio::CString &inInitialDirectory = Q3DStudio::CString());

    void DisplayImportFailed(const QUrl &inURL, const QString &inDescription,
                             bool inWarningsOnly);
    void DisplayLoadingPresentationFailed(const Qt3DSFile &inPresentation, long inErrorIDS = -1);
    void DisplaySavingPresentationFailed();
    void DisplaySaveReadOnlyFailed(const Qt3DSFile &inSavedLocation);
    Qt3DSMessageBox::EMessageBoxReturn DisplayMessageBox(const Q3DStudio::CString &inTitle,
                                                        const Q3DStudio::CString &inText,
                                                        Qt3DSMessageBox::EMessageBoxIcon inIcon,
                                                        bool inShowCancel);
    int DisplayChoiceBox(const Q3DStudio::CString &inTitle, const Q3DStudio::CString &inText,
                         int inIcon);
    void DisplayKnownErrorDialog(const Q3DStudio::CString &inErrorText);

    ESavePromptResult PromptForSave();
    bool PromptForKeyframeInterpolation(float &ioEaseIn, float &ioEaseOut);

    bool ConfirmRevert();

    void DisplayProgressScreen(const Q3DStudio::CString &inActionText,
                               const Q3DStudio::CString &inAdditionalText);
    void DestroyProgressScreen();

    bool PromptObjectTimebarColor(CColor &ioColor);
    void DisplayProfilingStatistics();
    /*void DisplayMultilineTextEdit(Q3DStudio::CString &ioText,
                                  CMultilineEditDlg::INotification *inNotifiction = NULL);*/

    void DisplayEnvironmentVariablesError(const Q3DStudio::CString &inErrorMessage);

    void ResetSettings(const Q3DStudio::CString &inCurrentDocPath = "");

    bool DisplayResetKeyframeValuesDlg();
    void DisplayPasteFailed();

    static void DisplayGLVersionError(const Q3DStudio::CString &inGLVersion,
                                      const Q3DStudio::CString &inMinVersion);
    static void DisplayGLVersionWarning(const Q3DStudio::CString &inGLVersion,
                                        const Q3DStudio::CString &inRecommendedVersion);

protected:
    QString CreateAllowedTypesString(long inFileTypeFilter, bool inForImport);
    static void DisplayGLVersionDialog(const Q3DStudio::CString &inGLVersion,
                                       const Q3DStudio::CString &inRecommendedVersion,
                                       bool inError);

    CProgressView *m_ProgressPalette;
    bool m_ShowGUI;

    Q3DStudio::CString m_LastSaveFile; ///< Path to the file was previously saved
};
#endif // INCLUDED_DIALOGS_H
