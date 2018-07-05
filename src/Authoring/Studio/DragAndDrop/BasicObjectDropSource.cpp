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
#include "BasicObjectDropSource.h"
#include "Doc.h"
#include "DropTarget.h"

#include "Dialogs.h"
#include "Dispatch.h"
#include "StudioApp.h"
#include "Core.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMDataCore.h"
#include "IDocumentEditor.h"
#include "ImportUtils.h"
#include "BasicObjectsModel.h"
#include "IDragable.h"
#include "IDocSceneGraph.h"
#include "Qt3DSTextRenderer.h"
#include "HotKeys.h"
#include "StudioUtils.h"

#include <QtCore/qdir.h>

//===============================================================================
/**
 *
 */
CBasicObjectDropSource::CBasicObjectDropSource(long inFlavor, IDragable *inDragable)
    : CDropSource(inFlavor, 0)
    , m_IsIndependent(false)
{
    auto item = dynamic_cast<BasicObjectItem *>(inDragable);
    if (item) {
        m_ObjectType = item->objectType();
        m_PrimitiveType = item->primitiveType();
    }
}

//===============================================================================
/**
 *
 */
bool CBasicObjectDropSource::ValidateTarget(CDropTarget *inTarget)
{
    using namespace Q3DStudio;

    EStudioObjectType targetType = (EStudioObjectType)inTarget->GetObjectType();
    bool theValidTarget = false;

    // the only thing we want to do from here is check the type.
    theValidTarget =
        CStudioObjectTypes::AcceptableParent((EStudioObjectType)m_ObjectType, targetType);

    if (!theValidTarget) {
        SetHasValidTarget(theValidTarget);
        return theValidTarget;
    } else {
        if (CHotKeys::IsKeyDown(Qt::AltModifier) && targetType != OBJTYPE_SCENE
            && targetType != OBJTYPE_COMPONENT) {
            qt3dsdm::Qt3DSDMInstanceHandle theTarget = inTarget->GetInstance();
            CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
            IDocumentReader &theReader(theDoc->GetDocumentReader());
            qt3dsdm::Qt3DSDMSlideHandle toSlide = theReader.GetAssociatedSlide(theTarget);
            ;

            if (!theReader.IsMasterSlide(toSlide))
                theValidTarget = false;
        }

        SetHasValidTarget(theValidTarget);
        return theValidTarget;
    }
}

//===============================================================================
/**
 *
 */
bool CBasicObjectDropSource::CanMove()
{
    return true;
}

//===============================================================================
/**
 *
 */
bool CBasicObjectDropSource::CanCopy()
{
    return true;
}

CCmd *CBasicObjectDropSource::GenerateAssetCommand(qt3dsdm::Qt3DSDMInstanceHandle inTarget,
                                                   EDROPDESTINATION inDestType,
                                                   qt3dsdm::Qt3DSDMSlideHandle inSlide)
{
    using namespace Q3DStudio;
    using qt3dsdm::ComposerObjectTypes;
    using namespace std;

    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    CPt thePoint;
    //	if ( CHotKeys::IsKeyDown( CHotKeys::KEY_MENU ) )
    //		thePoint = GetCurrentPoint();

    long theStartTime = -1;
    if (CHotKeys::IsKeyDown(Qt::ControlModifier))
        theStartTime = theDoc->GetCurrentViewTime();

    DocumentEditorInsertType::Enum theInsertType(ImportUtils::GetInsertTypeForDropType(inDestType));
    ComposerObjectTypes::Enum theComposerType;
    switch (m_ObjectType) {
    case OBJTYPE_SCENE:
        theComposerType = ComposerObjectTypes::Scene;
        break;
    case OBJTYPE_LAYER:
        theComposerType = ComposerObjectTypes::Layer;
        break;
    case OBJTYPE_BEHAVIOR:
        theComposerType = ComposerObjectTypes::Behavior;
        break;
    case OBJTYPE_MATERIAL:
        theComposerType = ComposerObjectTypes::Material;
        break;
    case OBJTYPE_CAMERA:
        theComposerType = ComposerObjectTypes::Camera;
        break;
    case OBJTYPE_LIGHT:
        theComposerType = ComposerObjectTypes::Light;
        break;
    case OBJTYPE_MODEL:
        theComposerType = ComposerObjectTypes::Model;
        break;
    case OBJTYPE_GROUP:
        theComposerType = ComposerObjectTypes::Group;
        break;
    case OBJTYPE_IMAGE:
        theComposerType = ComposerObjectTypes::Image;
        break;
    case OBJTYPE_TEXT:
        theComposerType = ComposerObjectTypes::Text;
        break;
    case OBJTYPE_COMPONENT:
        theComposerType = ComposerObjectTypes::Component;
        break;
    case OBJTYPE_ALIAS:
        theComposerType = ComposerObjectTypes::Alias;
        break;
    case OBJTYPE_PATH:
        theComposerType = ComposerObjectTypes::Path;
        break;
    default:
        QT3DS_ASSERT(false);
        theComposerType = ComposerObjectTypes::Unknown;
        break;
    }
    if (theComposerType != ComposerObjectTypes::Unknown) {
        if (theComposerType == ComposerObjectTypes::Text) {
            // For Text, we need to check if user already has font file inside fonts folder
            CFilePath theFontFile;
            CFilePath theFontDir = CFilePath::CombineBaseAndRelative(
                        g_StudioApp.GetCore()->getProjectFile().getProjectPath(),
                        CFilePath(L"fonts"));
            if (!theFontDir.Exists()) {
                // Create font dir if necessary
                theFontDir.CreateDir(true);
            } else {
                // Recursively find the first font file in font dir
                vector<CFilePath> theFiles;
                theFontDir.RecursivelyFindFilesOfType(nullptr, theFiles, false);
                for (size_t i = 0; i < theFiles.size(); ++i) {
                    if (CDialogs::IsFontFileExtension(theFiles[i].GetExtension())) {
                        // Reuse the font in fonts subdirectory
                        theFontFile = theFiles[i];
                        break;
                    }
                }
            }

            if (theFontFile.filePath().isEmpty()) {
                // If user doesn't have any font file, copy the default font file from Studio's res
                // folder

                CFilePath theResFontFile;

                QDir theResFontDir(resourcePath() + "/Font");
                Q_FOREACH (QFileInfo fontFile, theResFontDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
                    CString ext = CString::fromQString(fontFile.suffix());
                    if (CDialogs::IsFontFileExtension(ext)) {
                        theResFontFile = CString::fromQString(fontFile.absoluteFilePath());
                        theFontFile = CFilePath::CombineBaseAndRelative(theFontDir, CString::fromQString(fontFile.fileName()));
                        break;
                    }
                }

                if (theResFontFile.filePath().isEmpty()) {
                    QT3DS_ASSERT(false);
                    std::shared_ptr<IImportFailedHandler> theHandler(
                        theDoc->GetImportFailedHandler());
                    if (theHandler)
                        theHandler->DisplayImportFailed(
                            theResFontDir.absolutePath(),
                            QObject::tr("Default Font File Doesn't Exist in the Directory"),
                            false);
                    return nullptr;
                }

                // Copy the file to project's fonts folder
                SFileTools::Copy(theFontFile,
                                 Q3DStudio::FileOpenFlags(Q3DStudio::FileOpenFlagValues::Create),
                                 theResFontFile);
                // Force the text renderer to refresh
                if (theDoc->GetSceneGraph() && theDoc->GetSceneGraph()->GetTextRenderer())
                    theDoc->GetSceneGraph()->GetTextRenderer()->ReloadFonts();
            }

            // Lastly, we use the font file to create the Text object. This is similar to drag-drop
            // the font file from Project Palette to Scene.
            SCOPED_DOCUMENT_EDITOR(*theDoc, QObject::tr("Add Text"))
                ->ImportFile(DocumentEditorFileType::Font, theFontFile, inTarget, inSlide,
                             CDialogs::GetImportFileExtension(),
                             Q3DStudio::ImportUtils::GetInsertTypeForDropType(inDestType), thePoint,
                             theStartTime);
        } else {
            SCOPED_DOCUMENT_EDITOR(*theDoc, QObject::tr("Add Instance"))
                ->CreateSceneGraphInstance(theComposerType, inTarget, inSlide, theInsertType,
                                           thePoint, (EPrimitiveType)m_PrimitiveType, theStartTime);
        }
    }
    return nullptr;
}
