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

#include "Qt3DSCommonPrecompile.h"
#include "qtAuthoring-config.h"
#include "ProjectDropTarget.h"
#include "DropSource.h"
#include "ExplorerFileDropSource.h"
#include "IDragable.h"
#include "StudioApp.h"
#include "Core.h"
#include "Dialogs.h"
#include "Doc.h"
#include "Qt3DSFile.h"
#include "Qt3DSFileTools.h"
#include "Qt3DSImportPerformImport.h"
#include "Qt3DSImportTranslation.h"
#include "IDocumentEditor.h"

//===============================================================================
/**
 * 	Constructor
 */
CProjectDropTarget::CProjectDropTarget(const Q3DStudio::CFilePath &inTargetDir)
    : m_TargetDir(inTargetDir)
{
}

//===============================================================================
/**
 * 	This get called on every DragWithin.
 *	Note: the source will validate the target instead of the otherway around.
 *	This is because the DropSource knows how to get information from itself without
 *	creating an asset. This is mainly for DropSources that have a lazy creation idiom.
 *  like files.
 *	@param the DropSource in question.
 *	@return true if the DropSource likes the DropTarget.
 */
bool CProjectDropTarget::Accept(CDropSource &inSource)
{
    return inSource.ValidateTarget(this);
}

//===============================================================================
/**
 *	This is where is actually happens.
 *	Note: At this point everything should be verified, and setup in the dropsource.
 *	The only thing left to do is to get the Assets and move/copy or connect them.
 *  @param inSource the Object in question.
 *	@return true if the drop was successful .
 */
bool CProjectDropTarget::Drop(CDropSource &inSource)
{
    using namespace Q3DStudio;
    using namespace qt3dsimp;
    // Drag and Drop - From Explorer window to Project Palette
    // For all valid Project File Types:
    // - This performs a file copy from the source Explorer location to the selected Project Palette
    // Folder
    // - The destination copy must NOT be read-only even if the source is read-only
    // For DAE, it will import the file.
    if (inSource.GetFlavor() == QT3DS_FLAVOR_FILE) {
        // Create target directory if it doesn't exist
        if (!m_TargetDir.Exists())
            m_TargetDir.CreateDir(true);
        // also make sure that target directory is a directory, not a file
        else if (m_TargetDir.IsFile())
            m_TargetDir = m_TargetDir.GetDirectory();

        // Sanity check
        Q_ASSERT(m_TargetDir.IsDirectory());

        // Get the Source file to be copied
        CExplorerFileDropSource *theFileDropSource =
            static_cast<CExplorerFileDropSource *>(&inSource);
        CFilePath theSourceFile(theFileDropSource->GetFile().GetAbsolutePath());

        if (theSourceFile.IsFile() && m_TargetDir.IsDirectory()) {
            // Get the file extension
            Q3DStudio::CString theExtension(theSourceFile.GetExtension());

            Q3DStudio::CString theFileStem = theSourceFile.GetFileStem();
            Q3DStudio::CString outputFileName(theFileStem + L"."
                                              + CDialogs::GetImportFileExtension());

            if (theExtension.Compare(CDialogs::GetWideDAEFileExtension(),
                                     Q3DStudio::CString::ENDOFSTRING, false)) {
                SColladaTranslator theTranslator(theSourceFile.toQString());

                CFilePath theOutputDir =
                    SFileTools::FindUniqueDestDirectory(m_TargetDir, theFileStem);
                CFilePath theFullOutputFile(
                    CFilePath::CombineBaseAndRelative(theOutputDir, outputFileName));
                SImportResult theImportResult =
                    CPerformImport::TranslateToImportFile(theTranslator, theFullOutputFile);
                bool forceError = theFullOutputFile.IsFile() == false;
                IDocumentEditor::DisplayImportErrors(
                    theSourceFile.toQString(), theImportResult.m_Error,
                    g_StudioApp.GetCore()->GetDoc()->GetImportFailedHandler(),
                    theTranslator.m_TranslationLog, forceError);
#ifdef QT_3DSTUDIO_FBX
            } else if (theExtension.Compare(CDialogs::GetWideFbxFileExtension(),
                                            Q3DStudio::CString::ENDOFSTRING, false)) {
                SFbxTranslator theTranslator(theSourceFile.toQString());

                CFilePath theOutputDir =
                    SFileTools::FindUniqueDestDirectory(m_TargetDir, theFileStem);
                CFilePath theFullOutputFile(
                    CFilePath::CombineBaseAndRelative(theOutputDir, outputFileName));
                SImportResult theImportResult =
                    CPerformImport::TranslateToImportFile(theTranslator, theFullOutputFile);
                bool forceError = theFullOutputFile.IsFile() == false;
                IDocumentEditor::DisplayImportErrors(
                    theSourceFile.toQString(), theImportResult.m_Error,
                    g_StudioApp.GetCore()->GetDoc()->GetImportFailedHandler(),
                    theTranslator.m_TranslationLog, forceError);
#endif
            } else {
                // Copy the file to target directory
                // FindAndCopyDestFile will make sure the file name is unique and make sure it is
                // not read only.
                SFileErrorCodeFileNameAndNumBytes theCopyResult = SFileTools::FindAndCopyDestFile(
                    m_TargetDir, CFilePath::GetAbsolutePath(theSourceFile));

                // Sanity check
                Q_ASSERT(theCopyResult.m_Error == Q3DStudio::FileErrorCodes::NoError);
                Q_ASSERT(theCopyResult.m_DestFilename.Exists());

                // For effect and custom material files, automatically copy related resources
                if (CDialogs::IsEffectFileExtension(theExtension)
                    || CDialogs::IsMaterialFileExtension(theExtension)) {
                    std::shared_ptr<IImportFailedHandler> theHandler(
                        g_StudioApp.GetCore()->GetDoc()->GetImportFailedHandler());
                    CFilePath theShaderFile(theSourceFile);

                    if (theShaderFile.GetExtension() != "effect"
                        && theShaderFile.GetExtension() != "material") {
                        Q_ASSERT(false); // what file is this?
                    } else {
                    }

                    std::vector<Q3DStudio::CString> theEffectFileSourcePaths;
                    g_StudioApp.GetCore()
                        ->GetDoc()
                        ->GetDocumentReader()
                        .ParseSourcePathsOutOfEffectFile(
                            Q3DStudio::CFilePath::GetAbsolutePath(theSourceFile),
                            theEffectFileSourcePaths);

                    CFilePath theFileDir(
                        Q3DStudio::CFilePath::GetAbsolutePath(theSourceFile).GetDirectory());
                    CFilePath theDocumentDir(
                        g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory());
                    for (size_t idx = 0; idx < theEffectFileSourcePaths.size(); ++idx) {
                        CFilePath theSourcePath = CFilePath::CombineBaseAndRelative(
                            theFileDir, theEffectFileSourcePaths[idx]);
                        CFilePath theResultPath = CFilePath::CombineBaseAndRelative(
                            theDocumentDir, theEffectFileSourcePaths[idx]);
                        CFilePath theResultDir(theResultPath.GetDirectory());
                        theResultDir.CreateDir(true);
                        // If the file already exists, these file flags will ensure it won't get
                        // overwritten.
                        SFileTools::Copy(theResultPath,
                                         qt3ds::foundation::FileOpenFlags(
                                             qt3ds::foundation::FileOpenFlagValues::Create
                                             | qt3ds::foundation::FileOpenFlagValues::Write),
                                         theSourcePath);
                    }
                }
            }
        }
    }

    // we are always successful
    return true;
}

//===============================================================================
/**
 *	This will get the objec ttype from the Asset.
 *	Note: The asset can change all of the time, so i always ask the asset for its type.
 *	@return the Studio object type.
 */
long CProjectDropTarget::GetObjectType()
{
    return QT3DS_FLAVOR_FILE;
}
