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
#include "stdafx.h"
#include "qtAuthoring-config.h"
#include "Strings.h"
#include "StringLoader.h"

#include "Dialogs.h"
#include "StudioApp.h"
#include "Doc.h"

#include "InterpolationDlg.h"

#include "Qt3DSMessageBox.h"
#include "StringTokenizer.h"
#include "Preferences.h"
#include "ProgressView.h"
#include "Views.h"
#include "MasterP.h"
#include "TimeEditDlg.h"
#include "StudioPreferences.h"
#include "ResetKeyframeValuesDlg.h"
#include "GLVersionDlg.h"
#include "Core.h"
#include "Qt3DSMacros.h"
#include "IDocumentEditor.h"
#include "Qt3DSFileTools.h"
#include "ImportUtils.h"

#include "StringLoader.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include <iostream>

//=============================================================================
/**
 *	Constructor
 *	@param	inShowGUI	true if dialogs should be displayed or piped to std:cout instead
 */
CDialogs::CDialogs(bool inShowGUI /*= true*/)
    : m_ProgressPalette(nullptr)
    , m_ShowGUI(inShowGUI)
    , m_LastSaveFile(Q3DStudio::CString("."))
{
}

//=============================================================================
/**
 *	Destructor
 */
CDialogs::~CDialogs()
{
}

//=============================================================================
/**
 *	Displays a multiline text edit.
 *	@param	ioText
 *	@param	inNotification
 */
#if 0
void CDialogs::DisplayMultilineTextEdit(Q3DStudio::CString &ioText,
                                        CMultilineEditDlg::INotification *inNotifiction)
{
    CRct theLocation = CStudioPreferences::GetMultilineTextLocation();

    CMultilineEditDlg theMultilineEditDlg(inNotifiction, theLocation);
    theMultilineEditDlg.SetString(ioText);
    if (theMultilineEditDlg.DoModal() == IDOK) {
        // Set the string
        ioText = theMultilineEditDlg.GetString();

        // Save the window position
        theLocation = theMultilineEditDlg.GetLocation();
        CStudioPreferences::SetMultilineTextLocation(theLocation);
    }
}
#endif

//=============================================================================
/**
 * Displays a dialog asking the user to choose the keyframe interpolation.
 *
 * @param ioEaseIn value to be set as the ease in default - passes back the value chosen by the user
 * @param ioEaseOut value to be set as the ease out default - passes back the value chosen by the
 * user
 * @return true if the user clicked OK on the dialog (indicating that the values should be updated
 * on the track)
 */
bool CDialogs::PromptForKeyframeInterpolation(float &ioEaseIn, float &ioEaseOut)
{
    bool theReturnValue = false;

    CInterpolationDlg theInterpolationDialog;
    theInterpolationDialog.setEaseIn(ioEaseIn);
    theInterpolationDialog.setEaseOut(ioEaseOut);

        // If the user presses the OK button
    if (theInterpolationDialog.exec() ==  QDialog::Accepted) {
        // Retrieve the new interpolation values
        ioEaseIn = theInterpolationDialog.easeIn();
        ioEaseOut = theInterpolationDialog.easeOut();
        theReturnValue = true;
    }

    return theReturnValue;
}

//=============================================================================
/**
 *	Notify the user that the deletion of an asset has failed.
 */
void CDialogs::DisplayAssetDeleteFailed()
{
    Q3DStudio::CString theMessage =
        ::LoadResourceString(IDS_ERROR_CLIENTSAVE); // TODO: Should display the correct string
    Q3DStudio::CString theTitle = ::LoadResourceString(IDS_ERROR_MSGTITLE);

    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theTitle, theMessage, Qt3DSMessageBox::ICON_ERROR, false,
                             qApp->activeWindow());
    else {
        std::cout << theTitle.GetCharStar() << ": " << theMessage.GetCharStar() << std::endl;
    }
}

//=============================================================================
/**
 *	Get the export choice.
 */
Qt3DSFile CDialogs::GetExportChoice(const Q3DStudio::CString &, const Q3DStudio::CString &)
{
    // Need to fix this for windows if we decide to use it
    return Qt3DSFile("", false, false);
}

//==============================================================================
/**
 *	Notify that we are unable to refresh the resource.
 */
void CDialogs::DisplayRefreshResourceFailed(const Q3DStudio::CString &inResourceName,
                                            const Q3DStudio::CString &inDescription)
{
    Q3DStudio::CString theTitle(::LoadResourceString(IDS_ERROR_REFRESHRESOURCETITLE));

    Q3DStudio::CString theText;
    theText.Format(::LoadResourceString(IDS_ERROR_REFRESHRESOURCETEXT),
                   static_cast<const wchar_t *>(inResourceName));

    if (!inDescription.IsEmpty())
        theText += inDescription;

    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theTitle, theText, Qt3DSMessageBox::ICON_WARNING, false,
                             qApp->activeWindow());
    else {
        std::cout << theTitle.GetCharStar() << ": " << theText.GetCharStar() << std::endl << std::endl;
    }
}

//=============================================================================
/**
 *	Notify the user that the loading of the requested resource failed.
 *
 *	Possible failed import messages are:
 *	IDS_ERROR_IMPORTLUARESOURCETEXT				- Sorry - Studio was unable to
 *import the LUA script.\nPlease check the file and try again.\nNote that LUA files must be
 *syntactically correct to be importable.
 *	IDS_ERROR_IMPORTRESOURCETEXT				- Sorry - Studio was unable to
 *import the resource.\nPlease check the above file and try again.
 *	IDS_ERROR_IMPORTUNSUPPORTEDRESOURCETYPETEXT	- Sorry - Studio was unable to import
 *the resource file.\nThis resource file type is not currently supported by Studio.\n
 *
 *  @param inURL				the URL for the asset that was to have been imported
 *	@param inErrorDescription	description for the failure, if any
 *	@param inWarningsOnly		not a failure, just warnings
 */
void CDialogs::DisplayImportFailed(const QUrl &inURL, const QString &inDescription,
                                   bool inWarningsOnly)
{
    // Notify the user we couldn't load the resource.
    Q3DStudio::CString theTitle, theText, theMsgText;

    theTitle = ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETITLE);
    // specify if its an error or warning
    theTitle = (!inWarningsOnly) ? ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETITLE_ERROR)
                                 : ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETITLE_WARNING);

    // Determine the asset type
    EStudioObjectType theAssetType =
        Q3DStudio::ImportUtils::GetObjectFileTypeForFile(inURL.path(), false)
            .m_ObjectType;

    bool theIsStudioObject = theAssetType != OBJTYPE_UNKNOWN;

    // Is this a LUA file, but perhaps incorrectly formatted?
    if (theAssetType == OBJTYPE_BEHAVIOR) {
        // Load the message about the LUA format
        if (inWarningsOnly)
            theText = ::LoadResourceString(IDS_WARNING_IMPORTLUARESOURCETEXT);
        else
            theText = ::LoadResourceString(IDS_ERROR_IMPORTLUARESOURCETEXT);
        if (!inDescription.isEmpty()) {
            theText += L"\n";
            theText += Q3DStudio::CString::fromQString(inDescription);
        }
    } else if (theAssetType != OBJTYPE_UNKNOWN || theIsStudioObject) {
        // Valid registered file type, but invalid file

        bool theNoDescription = inDescription.isEmpty();
        // Load default text stating that the import resource failed.
        // descriptions if present are presented as "reasons" for failure.
        if (!inWarningsOnly || theNoDescription) {
            theText.Format(::LoadResourceString(IDS_ERROR_IMPORTRESOURCETEXT), theNoDescription
                               ? L"."
                               : ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETEXT_REASON).c_str());
            theText += "\n";
        }
        if (!theNoDescription)
            theText += Q3DStudio::CString::fromQString(inDescription);
        else
            theText += ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETEXT_CHECKFILE);
    } else {
        // Display the warning messsage if we have one
        // instead of a meaningless message. This provides more feed back
        if (!inDescription.isEmpty())
            theText += Q3DStudio::CString::fromQString(inDescription);
        else
            theText = ::LoadResourceString(
                IDS_ERROR_IMPORTUNSUPPORTEDRESOURCETYPETEXT); // Load default text stating that the
                                                              // import resource failed.
    }

    Q3DStudio::CString theFormatString(
        !inWarningsOnly ? ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETEXT_FAILED)
                        : ::LoadResourceString(IDS_ERROR_IMPORTRESOURCETEXT_COMPLETEWITHWARNING));
    theFormatString += _LSTR("\n%ls\n\n");
    std::wstring wstr = inURL.path().toStdWString();
    theMsgText.Format(theFormatString, static_cast<const wchar_t *>(wstr.c_str()));
    theMsgText += theText;

    // Display the failed import resource message.
    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theTitle, theMsgText, Qt3DSMessageBox::ICON_WARNING, false,
                             qApp->activeWindow());
    else {
        std::cout << theTitle.GetCharStar() << ": " << theMsgText.GetCharStar() << std::endl;
    }
}

namespace {

inline Q3DStudio::CString CreateExtensionsList(const char **extList)
{
    Q3DStudio::CString retval;
    for (const char **ext = extList; *ext != nullptr; ++ext) {
        if (retval.Length())
            retval += " ";
        retval += Q3DStudio::CString("*.") + *ext;
    }
    return retval;
}

struct SAllowedTypesEntry
{
    Q3DStudio::DocumentEditorFileType::Enum m_FileType;
    long m_ResourceStringId; // Model Files, Image Files, etc
    const char **m_FileExtensions;
};
const char *imgExts[] = {
    "png", "jpg", "jpeg", "dds", "bmp", "gif", "hdr", nullptr,
};
const wchar_t *wideImgExts[] = {
    L"png", L"jpg", L"jpeg", L"dds", L"bmp", L"gif", L"hdr", nullptr,
};
const char *modelExts[] = {
    CDialogs::GetDAEFileExtension(),
#ifdef QT_3DSTUDIO_FBX
    CDialogs::GetFbxFileExtension(),
#endif
    // TODO CDialogs::GetImportFileExtension(),
    // TODO CDialogs::GetMeshFileExtension(),
    nullptr,
};
const char *meshExts[] = {
    CDialogs::GetMeshFileExtension(), nullptr,
};
const char *importExts[] = {
    CDialogs::GetImportFileExtension(), nullptr,
};
const char *behaviorExts[] = {
    CDialogs::GetQmlFileExtension(), nullptr,
};
const char *fontExts[] = {
    "ttf", "otf", nullptr,
};
const wchar_t *wideFontExts[] = {
    L"ttf", L"otf", nullptr,
};
const char *effectExts[] = {
    "effect", nullptr,
};
const wchar_t *wideEffectExts[] = {
    L"effect", nullptr,
};
const char *materialExts[] = {
    "material", nullptr,
};
const wchar_t *wideMaterialExts[] = {
    L"material", nullptr,
};
const char *soundExts[] = {
    "wav", nullptr,
};
const wchar_t *wideSoundExts[] = {
    L"wav", nullptr,
};

// List of file types allowed during import
SAllowedTypesEntry g_AllowedImportTypes[] = {
    { Q3DStudio::DocumentEditorFileType::DAE, IDS_LIBRARYIMPORT_MODEL, modelExts },
#ifdef QT_3DSTUDIO_FBX
    { Q3DStudio::DocumentEditorFileType::FBX, IDS_LIBRARYIMPORT_MODEL, modelExts },
#endif
};
size_t g_NumAllowedImportTypes = sizeof(g_AllowedImportTypes) / sizeof(*g_AllowedImportTypes);

// List of file types allowed for file references
SAllowedTypesEntry g_AllowedFileReferencesTypes[] = {
    { Q3DStudio::DocumentEditorFileType::Image, IDS_LIBRARYIMPORT_IMAGE, imgExts },
    { Q3DStudio::DocumentEditorFileType::Behavior, IDS_LIBRARYIMPORT_BEHAVIOR, behaviorExts },
    { Q3DStudio::DocumentEditorFileType::Mesh, IDS_LIBRARYIMPORT_MESH, meshExts },
    { Q3DStudio::DocumentEditorFileType::Import, IDS_LIBRARYIMPORT_IMPORT, importExts },
    { Q3DStudio::DocumentEditorFileType::Effect, IDS_LIBRARYIMPORT_EFFECT, effectExts },
};
size_t g_NumAllowedFileReferencesTypes =
    sizeof(g_AllowedFileReferencesTypes) / sizeof(*g_AllowedFileReferencesTypes);
}

QString CDialogs::ConfirmRefreshModelFile(const QString &inFile)
{
    // this produces an extension string which contains all allowed formats specified in
    // g_AllowedImportTypes
    // currently DAE and FBX
    QString theFileFilter =
        CreateAllowedTypesString(Q3DStudio::DocumentEditorFileType::DAE, true);


    return QFileDialog::getOpenFileName(qApp->activeWindow(), QObject::tr("Open"),
                                        inFile, theFileFilter, nullptr,
                                        QFileDialog::DontUseNativeDialog);
}

//==============================================================================
/**
 *	Notify the user that the presentation we tried to load has failed.
 *	@param	inPresentation	The AKFile that we failed to load.
 */
void CDialogs::DisplayLoadingPresentationFailed(const Qt3DSFile &inPresentation,
                                                long inErrorIDS /*= -1 */)
{
    Q_UNUSED(inPresentation);

    if (inErrorIDS == -1) // if unspecified, default to the 'generic' string
        inErrorIDS = IDS_ERROR_LOADPRESENTATION;
    Q3DStudio::CString theErrorMessage;
    theErrorMessage.Format(::LoadResourceString(inErrorIDS),
                           static_cast<const wchar_t *>(inPresentation.GetName()));
    Q3DStudio::CString theErrorTitle(::LoadResourceString(IDS_ERROR_LOADPRESENTATION_TITLE));

    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theErrorTitle, theErrorMessage, Qt3DSMessageBox::ICON_WARNING, false,
                             qApp->activeWindow());
    else {
        std::cout << theErrorTitle.GetCharStar() << ": " << theErrorMessage.GetCharStar() << std::endl;
    }
}

//==============================================================================
/**
 *	Notify the user that the presentation we tried to save has failed.
 *
 *	@param	inSavedLocation	The AKFile that we failed to save.
 */
void CDialogs::DisplaySavingPresentationFailed()
{
    Q3DStudio::CString theErrorMessage = ::LoadResourceString(IDS_ERROR_EXPORTPRESENTATION);
    Q3DStudio::CString theErrorTitle = ::LoadResourceString(IDS_PROJNAME);

    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theErrorTitle, theErrorMessage, Qt3DSMessageBox::ICON_WARNING, false,
                             qApp->activeWindow());
    else {
        std::cout << theErrorTitle.GetCharStar() << ": " << theErrorMessage.GetCharStar() << std::endl;
    }
}

//==============================================================================
/**
 *	Display a message box to indicate failure to overwrite a read-only file
 *
 *	@param	inSavedLocation
 *			the file location to be saved
 *
 *	@return	void
 */
void CDialogs::DisplaySaveReadOnlyFailed(const Qt3DSFile &inSavedLocation)
{
    Q3DStudio::CString theMsg = ::LoadResourceString(IDS_SAVE_READONLY_WARNING);
    Q3DStudio::CString theTitle = ::LoadResourceString(IDS_PROJNAME);

    Q3DStudio::CString theFormattedText;
    theFormattedText.Format(theMsg, static_cast<const wchar_t *>(inSavedLocation.GetName()));

    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theTitle, theFormattedText, Qt3DSMessageBox::ICON_WARNING, false,
                             qApp->activeWindow());
    else {
        std::cout << theTitle.GetCharStar() << ": " << theFormattedText.GetCharStar() << std::endl;
    }
}

//==============================================================================
/**
 *	Displays a Qt3DSMessageBox using the specified parameters.  The message box
 *	is modal to the main frame.  This provides an easy way to place modal dialogs
 * 	to the user, without requiring your class to know about the main frame or
 *	window refs.
 *	@param inTitle Title of the message box (not used on Mac)
 *	@param inText Text of the message
 *	@param inIcon Icon to be displayed next to the text
 *	@param inShowCancel true to show a Cancel button, false only show an OK button
 *	@return Indication of which button was pressed to dismiss the dialog
 */
Qt3DSMessageBox::EMessageBoxReturn
CDialogs::DisplayMessageBox(const Q3DStudio::CString &inTitle, const Q3DStudio::CString &inText,
                            Qt3DSMessageBox::EMessageBoxIcon inIcon, bool inShowCancel)
{
    Qt3DSMessageBox::EMessageBoxReturn theUserChoice;

    if (m_ShowGUI) {
        theUserChoice =
            Qt3DSMessageBox::Show(inTitle, inText, inIcon, inShowCancel, qApp->activeWindow());
    } else {
        std::cout << inTitle.GetCharStar() << ": " << inText.GetCharStar() << std::endl;
        theUserChoice = Qt3DSMessageBox::MSGBX_OK;
    }

    return theUserChoice;
}

int CDialogs::DisplayChoiceBox(const Q3DStudio::CString &inTitle, const Q3DStudio::CString &inText,
                               int inIcon)
{
    if (m_ShowGUI) {
        QMessageBox box;
        box.setWindowTitle(inTitle.toQString());
        box.setText(inText.toQString());
        switch (inIcon) {
        case Qt3DSMessageBox::ICON_WARNING:
            box.setIcon(QMessageBox::Warning);
            break;
        case Qt3DSMessageBox::ICON_ERROR:
            box.setIcon(QMessageBox::Critical);
            break;
        case Qt3DSMessageBox::ICON_INFO:
            box.setIcon(QMessageBox::Information);
            break;
        default:
            break;
        }
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        switch (box.exec()) {
        case QMessageBox::Yes:
            return IDYES;
        case QMessageBox::No:
            return IDNO;
        default:
            Q_UNREACHABLE();
        }
    } else {
        std::cout << inTitle.GetCharStar() << ": " << inText << std::endl;
        return IDYES;
    }
}

const char *CDialogs::GetDAEFileExtension()
{
    return "dae";
}
const char *CDialogs::GetFbxFileExtension()
{
    return "fbx";
}
// Null terminated list
const char **CDialogs::GetImgFileExtensions()
{
    return imgExts;
}
const char *CDialogs::GetImportFileExtension()
{
    return "import";
}
const char *CDialogs::GetMeshFileExtension()
{
    return "mesh";
}
const char *CDialogs::GetQmlFileExtension()
{
    return "qml";
}
const char **CDialogs::GetFontFileExtensions()
{
    return fontExts;
}
const char **CDialogs::GetEffectFileExtensions()
{
    return effectExts;
}
const char **CDialogs::GetMaterialFileExtensions()
{
    return materialExts;
}
const char **CDialogs::GetSoundFileExtensions()
{
    return soundExts;
}

bool IsFileExtension(const char *inExt, const char **inExts)
{
    if (inExt == nullptr)
        return false;
    for (const char **ext = inExts; *ext != nullptr; ++ext)
        if (QString::compare(inExt, *ext, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}
bool CDialogs::IsImageFileExtension(const char *inExt)
{
    return IsFileExtension(inExt, imgExts);
}
bool CDialogs::IsFontFileExtension(const char *inExt)
{
    return IsFileExtension(inExt, fontExts);
}
bool CDialogs::IsEffectFileExtension(const char *inExt)
{
    return IsFileExtension(inExt, effectExts);
}
bool CDialogs::IsMaterialFileExtension(const char *inExt)
{
    return IsFileExtension(inExt, materialExts);
}
bool CDialogs::IsSoundFileExtension(const char *inExt)
{
    return IsFileExtension(inExt, soundExts);
}

const wchar_t **CDialogs::GetWideImgFileExtensions()
{
    return wideImgExts;
}
const wchar_t *CDialogs::GetWideDAEFileExtension()
{
    return L"dae";
}
const wchar_t *CDialogs::GetWideFbxFileExtension()
{
    return L"fbx";
}
const wchar_t *CDialogs::GetWideImportFileExtension()
{
    return L"import";
}
const wchar_t *CDialogs::GetWideMeshFileExtension()
{
    return L"mesh";
}
const wchar_t *CDialogs::GetWideLUAFileExtension()
{
    return L"lua";
}
const wchar_t **CDialogs::GetWideFontFileExtensions()
{
    return wideFontExts;
}
const wchar_t **CDialogs::GetWideEffectFileExtensions()
{
    return wideEffectExts;
}
const wchar_t **CDialogs::GetWideMaterialFileExtensions()
{
    return wideMaterialExts;
}

bool IsFileExtension(const wchar_t *inExt, const wchar_t **inExts)
{
    if (inExt == nullptr)
        return false;
    for (const wchar_t **ext = inExts; *ext != nullptr; ++ext) {
        if (QString::compare(QString::fromWCharArray(inExt),
                             QString::fromWCharArray(*ext), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}
bool CDialogs::IsImageFileExtension(const wchar_t *inExt)
{
    return IsFileExtension(inExt, wideImgExts);
}
bool CDialogs::IsFontFileExtension(const wchar_t *inExt)
{
    return IsFileExtension(inExt, wideFontExts);
}
bool CDialogs::IsEffectFileExtension(const wchar_t *inExt)
{
    return IsFileExtension(inExt, wideEffectExts);
}

bool CDialogs::IsMaterialFileExtension(const wchar_t *inExt)
{
    return IsFileExtension(inExt, wideMaterialExts);
}

bool CDialogs::IsPathFileExtension(const wchar_t *inExt)
{
    return QString::compare(QString::fromWCharArray(inExt), "svg", Qt::CaseInsensitive) == 0;
}

bool CDialogs::IsPathBufferExtension(const wchar_t *inExt)
{
    return QString::compare(QString::fromWCharArray(inExt), "path", Qt::CaseInsensitive) == 0;
}

bool CDialogs::IsSoundFileExtension(const wchar_t *inExt)
{
    return IsFileExtension(inExt, wideSoundExts);
}

//==============================================================================
/**
 *	CreateAllowedTypesString: Creates the string used to determine allowable types for import or
 *for filereferences
 *	@return the string that dynamicly created with the extensions supported.
 */
QString CDialogs::CreateAllowedTypesString(long inFileTypeFilter, bool inForImport)
{
    QString theReturnString;
    size_t theCount = inForImport ? g_NumAllowedImportTypes : g_NumAllowedFileReferencesTypes;
    for (size_t idx = 0; idx < theCount; ++idx) {
        const SAllowedTypesEntry &entry =
            inForImport ? g_AllowedImportTypes[idx] : g_AllowedFileReferencesTypes[idx];
        if (inFileTypeFilter == Q3DStudio::DocumentEditorFileType::Unknown
            || inFileTypeFilter == entry.m_FileType) {
            QString theTypeString(::LoadResourceString(entry.m_ResourceStringId).toQString());
            QString theExtensions(CreateExtensionsList(entry.m_FileExtensions).toQString());
            theReturnString += theTypeString + " (" + theExtensions + ");;";
        }
    }
    return theReturnString;
}

//==============================================================================
/**
 *	Display a error dialog box with the given text string that describes the error.
 */
void CDialogs::DisplayKnownErrorDialog(const Q3DStudio::CString &inErrorText)
{
    if (inErrorText.Length() > 0) // make sure this is valid
    {
        Q3DStudio::CString theTitle(::LoadResourceString(IDS_PROJNAME));
        if (m_ShowGUI)
            Qt3DSMessageBox::Show(theTitle, inErrorText, Qt3DSMessageBox::ICON_ERROR, false,
                                 qApp->activeWindow());
        else
            std::cout << inErrorText.GetCharStar() << ": " << inErrorText.GetCharStar() << std::endl;
    }
}

//==============================================================================
/**
 *	Prompt the user to save the document before losing their changes.
 *	This is used when closing, loading or newing up a document when the current
 *	one has modifications.
 *	@return	the user's choice.
 */
CDialogs::ESavePromptResult CDialogs::PromptForSave()
{
    Q3DStudio::CString theDocTitle;

    Qt3DSFile theCurrentDoc = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath();
    if (theCurrentDoc.IsFile())
        theDocTitle = theCurrentDoc.GetName();
    else // if the current doc has not been saved then use the default title.
        theDocTitle = ::LoadResourceString(IDS_UNTITLED_DOCUMENT_TITLE);

    Q3DStudio::CString thePrompt = ::LoadResourceString(IDS_PROMPT_FOR_SAVE);
    thePrompt.Format(thePrompt, static_cast<const wchar_t *>(theDocTitle));

    int theChoice = QMessageBox::warning(nullptr, ::LoadResourceString(IDS_PROJNAME).toQString(),
                         thePrompt.toQString(), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    ESavePromptResult theResult = CANCEL_OPERATION;

    switch (theChoice) {
    case QMessageBox::Yes:
        theResult = SAVE_FIRST;
        break;
    case QMessageBox::No:
        theResult = CONTINUE_NO_SAVE;
        break;
    case QMessageBox::Cancel:
        theResult = CANCEL_OPERATION;
        break;
    default:
        break;
    }

    return theResult;
}

//==============================================================================
/**
 *	Prompt the user for a file to save to from the SaveAs menu option.
 *	@return	an invalid file if the user cancels the save dialog.
 */
std::pair<Qt3DSFile, bool> CDialogs::GetSaveAsChoice(const Q3DStudio::CString &inDialogTitle,
                                                    bool inFilenameUntitled)
{
    Qt3DSFile theFile("");
    Q3DStudio::CString theFileExt;
    Q3DStudio::CString theImportFilter;

    Q3DStudio::CString theFilename = g_StudioApp.GetCore()->GetDoc()->GetDocumentPath().GetAbsolutePath();

    if (theFilename == "" || inFilenameUntitled)
        theFilename = ::LoadResourceString(IDS_UNTITLED_DOCUMENT_TITLE);

    theFileExt = ::LoadResourceString(IDS_FILE_EXT_UIP);
    theImportFilter = ::LoadResourceString(IDS_FILE_DESC_UIP);

    theImportFilter += " (*" + theFileExt + ")|*" + theFileExt + "|";

    QFileDialog theFileDlg;
    theFileDlg.setOption(QFileDialog::DontConfirmOverwrite);
    const QFileInfo fi(m_LastSaveFile.toQString());
    theFileDlg.setDirectory((fi.path() == QStringLiteral("."))
                            ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            : fi.path());
    theFileDlg.setAcceptMode(QFileDialog::AcceptSave);
    theFileDlg.setDefaultSuffix(theFileExt.toQString());
    theFileDlg.setOption(QFileDialog::DontUseNativeDialog, true);
    if (inDialogTitle != "")
        theFileDlg.setWindowTitle(inDialogTitle.toQString());

    bool theCreateDir = false;
    bool theShowDialog = true;

    while (theShowDialog && theFileDlg.exec()) {
        theShowDialog = false;
        theFile = Qt3DSFile(Q3DStudio::CString::fromQString(theFileDlg.selectedFiles().front()));

        m_LastSaveFile = theFile.GetAbsolutePath();
        // customising a dialog box will force us to use non-native.
        // defaulting this for now, until we can agree a better workflow for
        // creating new projects
        theCreateDir = true;

        if (theCreateDir) {
            // If user checks "Create directory for project"
            // we need to check manually if the file in the directory already exists (because the
            // default file dialog can't do that for you)
            // If the file exists, show warning message if user wants to overwrite the file
            // This is to fix bug #6315: Create new project in folder with same name will overwrite
            // existing file without warning
            Q3DStudio::CFilePath theFinalDir;
            Q3DStudio::CFilePath theFinalDoc;
            g_StudioApp.GetCore()->GetCreateDirectoryFileName(theFile, theFinalDir, theFinalDoc);

            // Update last save file to final doc
            m_LastSaveFile = theFinalDoc;
            if (theFinalDoc.Exists()) {
                const QString theTitle(QObject::tr("Confirm Save As"));
                const QString filePath(theFinalDir.GetFileName().toQString() + QDir::separator() + theFinalDoc.GetFileName().toQString());
                const QString theString = QObject::tr("%1 already exists.\nDo you want to replace it?").arg(filePath);

                auto result = QMessageBox::question(nullptr, theTitle, theString);
                if (result != QMessageBox::Yes) {
                    // Reset the file and show the file dialog again
                    theFile = Qt3DSFile("");
                    theShowDialog = true;
                    continue;
                }
            } // of over-writing case
        }
    }

    return std::make_pair(theFile, theCreateDir);
}

//==============================================================================
/**
 *	Prompt the user for a file to create.
 *	@return	an invalid file if the user cancels the save dialog.
 */
std::pair<Qt3DSFile, bool>
CDialogs::GetNewDocumentChoice(const Q3DStudio::CString &inInitialDirectory)
{
    if (inInitialDirectory.size())
        m_LastSaveFile = inInitialDirectory;
    return GetSaveAsChoice(::LoadResourceString(IDS_CREATE_NEW_DOCUMENT_TITLE), true);
}

//==============================================================================
/**
 * Prompt the user for a file to open.
 * This will return an invalid file if the user cancels the save dialog.
 */
Qt3DSFile CDialogs::GetFileOpenChoice(const Q3DStudio::CString &inInitialDirectory)
{
    Qt3DSFile theFile("");
    Q3DStudio::CString theFileExt;
    Q3DStudio::CString theImportFilter;

    theFileExt = ::LoadResourceString(IDS_FILE_EXT_UIP);
    theImportFilter = ::LoadResourceString(IDS_FILE_DESC_UIP);

    theImportFilter += " (*" + theFileExt + ")";

    QFileDialog theFileDlg(qApp->activeWindow(), QString(),
                           (inInitialDirectory == Q3DStudio::CString("."))
                           ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                           : inInitialDirectory.toQString(),
                           theImportFilter.toQString());
    theFileDlg.setAcceptMode(QFileDialog::AcceptOpen);
    theFileDlg.setOption(QFileDialog::DontUseNativeDialog, true);

    if (theFileDlg.exec() == QDialog::Accepted) {
        QFileInfo fi(theFileDlg.selectedFiles().first());
        Q3DStudio::CString thePath = Q3DStudio::CString::fromQString(fi.absolutePath());
        Q3DStudio::CString theName = Q3DStudio::CString::fromQString(fi.fileName());
        theFile = Qt3DSFile(thePath, theName);

        m_LastSaveFile = theFile.GetAbsolutePath();
    }

    return theFile;
}

//==============================================================================
/**
 *	Prompt the user to make sure they want to revert the current project.
 *	@return true if they do want to continue with the revert.
 */
bool CDialogs::ConfirmRevert()
{
    bool theConfirmation = false;
    Q3DStudio::CString thePrompt = ::LoadResourceString(IDS_PROMPT_FOR_REVERT);
    Q3DStudio::CString theTitle = ::LoadResourceString(IDS_PROJNAME);

    Qt3DSMessageBox::EMessageBoxReturn theChoice;

    if (m_ShowGUI)
        theChoice = Qt3DSMessageBox::Show(theTitle, thePrompt, Qt3DSMessageBox::ICON_WARNING, true,
                                         qApp->activeWindow());
    else {
        std::cout << theTitle.GetCharStar() << ": " << thePrompt.GetCharStar() << std::endl;
        theChoice = Qt3DSMessageBox::MSGBX_OK;
    }

    // user decided to go ahead and delete all unused resources
    if (theChoice == Qt3DSMessageBox::MSGBX_OK)
        theConfirmation = true;

    return theConfirmation;
}

//==============================================================================
/**
 * Displays a progress screen, if there is not one aleady being shown.  The
 * progress screen doesn't get dismissed until you call
 * CDialogs::DestroyProgressScreen().
 * @param inActionText text to be displayed as the action
 * @param inAdditionalText additional text, for example a file name
 */
void CDialogs::DisplayProgressScreen(const Q3DStudio::CString &inActionText,
                                     const Q3DStudio::CString &inAdditionalText)
{
    if (m_ShowGUI && !m_ProgressPalette) {
        m_ProgressPalette = new CProgressView(qApp->activeWindow());
        m_ProgressPalette->SetActionText(inActionText);
        m_ProgressPalette->SetAdditionalText(inAdditionalText);
        m_ProgressPalette->show();
        qApp->processEvents();
    }
}

//==============================================================================
/**
 * If a loading screen is currently being shown, this function destroys it.  You
 * can show the loading screen again with another call to
 * CDialogs::DisplayLoadingScreen().
 */
void CDialogs::DestroyProgressScreen()
{
    if (m_ShowGUI && m_ProgressPalette) {
        delete m_ProgressPalette;
        m_ProgressPalette = nullptr;
    }
}

//==============================================================================
/**
 *	Display statistics from profiling.
 */
void CDialogs::DisplayProfilingStatistics()
{
    Q3DStudio::CString theStatistics = "<Not Enabled>";
#ifdef PERFORM_PROFILE
    CMasterProf *theMasterP = CMasterProf::GetInstance();

    for (long theIndex = 0; theIndex < theMasterP->GetProfilerCount(); ++theIndex) {
        CMethProf *theProf = theMasterP->GetProfiler(theIndex);
        if (theProf->GetCount() > 0) {
            theStatistics += theProf->GetDescription();
            theStatistics += "\n";
        }
    }
#endif // #ifdef PERFORM_PROFILE
    if (m_ShowGUI)
        Qt3DSMessageBox::Show("Profiling Statistics", theStatistics, Qt3DSMessageBox::ICON_INFO);
    else {
        std::cout << "Profiling Statistics"
                  << ": " << theStatistics.GetCharStar() << std::endl;
    }
}

//==============================================================================
/**
 *	Inform the user that the environment variables entered does not match the format
 *	expected, listing down all those settings that are wrong.
 *	@param	inErrorMessage the listing of all those errors.
 */
void CDialogs::DisplayEnvironmentVariablesError(const Q3DStudio::CString &inErrorMessage)
{
    Q3DStudio::CString theTitle = ::LoadResourceString(IDS_ERROR_PROJECT_VARIABLES_TITLE);
    Q3DStudio::CString theMessage = ::LoadResourceString(IDS_ERROR_PROJECT_VARIABLES_MSG);
    theMessage += inErrorMessage;
    theMessage += ::LoadResourceString(IDS_PROJECT_VARIABLES_FORMAT);
    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theTitle, theMessage, Qt3DSMessageBox::ICON_ERROR, false,
                             qApp->activeWindow());
    else {
        std::cout << theTitle.GetCharStar() << ": " << theMessage.GetCharStar() << std::endl;
    }
}

//==============================================================================
/**
 *	Reset settings.
 *	Typically inCurrentDocPath is only set when Studio is first launched.
 *	@param inCurrentDocPath	the current document path, if any. Application directory if
 *there is none.
 */
void CDialogs::ResetSettings(const Q3DStudio::CString &inCurrentDocPath)
{
    // Initialize the default dir/paths to the current document path if specified, otherwise leave
    // everything as it is.
    if (!inCurrentDocPath.IsEmpty()) {
        m_LastSaveFile = inCurrentDocPath;
    }
}

bool CDialogs::DisplayResetKeyframeValuesDlg()
{
    CResetKeyframeValuesDlg theDialog;
    return theDialog.exec() == QDialog::Accepted;
}

//==============================================================================
/**
 *	User trying to do a pathological paste, such as pasting a component copied from a different
 *instance
 *	of Studio into an instance of the same component that already exists in the current instance
 *of Studio, and
 *	is potentially replaced and deleted.
 */
void CDialogs::DisplayPasteFailed()
{
    Q3DStudio::CString theTitle(::LoadResourceString(IDS_ERROR_PATHOLOGICAL_PASTE_TITLE));
    Q3DStudio::CString theMessage(::LoadResourceString(IDS_ERROR_PATHOLOGICAL_PASTE_MESSAGE));

    if (m_ShowGUI)
        Qt3DSMessageBox::Show(theTitle, theMessage, Qt3DSMessageBox::ICON_ERROR, false,
                             qApp->activeWindow());
    else
        std::cout << theTitle.GetCharStar() << ": " << theMessage.GetCharStar() << std::endl;
}

//==============================================================================
/**
 *	Video card OpenGL version is too low to be supported.
 */
void CDialogs::DisplayGLVersionError(const Q3DStudio::CString &inGLVersion,
                                     const Q3DStudio::CString &inMinVersion)
{
    DisplayGLVersionDialog(inGLVersion, inMinVersion, true);
}

//==============================================================================
/**
 *	Video card OpenGL version is outdated, but could be usable.
 */
void CDialogs::DisplayGLVersionWarning(const Q3DStudio::CString &inGLVersion,
                                       const Q3DStudio::CString &inRecommendedVersion)
{
    DisplayGLVersionDialog(inGLVersion, inRecommendedVersion, false);
}

//==============================================================================
/**
 *	Display the error dialog or warning dialog that OpenGL version is lower than what is
 *expected
 */
void CDialogs::DisplayGLVersionDialog(const Q3DStudio::CString &inGLVersion,
                                      const Q3DStudio::CString &inRecommendedVersion, bool inError)
{
    long theTitleResourceID;
    long theMessageResourceID;

    if (inError) {
        theTitleResourceID = IDS_ERROR_MSGTITLE;
        theMessageResourceID = IDS_GL_VERSION_ERROR;
    } else {
        theTitleResourceID = IDS_WARNING_MSGTITLE;
        theMessageResourceID = IDS_GL_VERSION_WARNING;
    }

    Q3DStudio::CString theTitle(LoadResourceString(theTitleResourceID));
    Q3DStudio::CString theMessage(LoadResourceString(theMessageResourceID));
    Q3DStudio::CString theFormattedMessage;
    theFormattedMessage.Format(theMessage, static_cast<const wchar_t *>(inGLVersion),
                               static_cast<const wchar_t *>(inRecommendedVersion));

    CGLVersionDlg theGLVersionDlg;
    theGLVersionDlg.Initialize(theTitle, theFormattedMessage, inError);
    theGLVersionDlg.exec();

    if (theGLVersionDlg.GetDontShowAgain())
        CStudioPreferences::SetDontShowGLVersionDialog(true);
}
