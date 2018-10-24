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
#include "Dialogs.h"
#include "FileDropSource.h"
#include "DropTarget.h"
#include "StudioObjectTypes.h"
#include "HotKeys.h"
#include "Doc.h"
#include "StudioApp.h"
#include "Core.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentEditor.h"
#include "Qt3DSFileTools.h"
#include "ImportUtils.h"
#include "ChooseImagePropertyDlg.h"
#include "Dispatch.h"

bool CFileDropSource::s_FileHasValidTarget = false;

bool CFileDropSource::ValidateTarget(CDropTarget *inTarget)
{
    using namespace Q3DStudio;

    EStudioObjectType targetType = (EStudioObjectType)inTarget->GetObjectType();
    if (m_ObjectType & (OBJTYPE_PRESENTATION | OBJTYPE_QML_STREAM)) {
        SetHasValidTarget(targetType & (OBJTYPE_LAYER | OBJTYPE_MATERIAL | OBJTYPE_CUSTOMMATERIAL
                                        | OBJTYPE_REFERENCEDMATERIAL | OBJTYPE_IMAGE));
        return m_HasValidTarget;
    }

    if (m_ObjectType == OBJTYPE_MATERIALDATA) {
        SetHasValidTarget(targetType & (OBJTYPE_MATERIAL | OBJTYPE_CUSTOMMATERIAL
                                        | OBJTYPE_REFERENCEDMATERIAL));
        return m_HasValidTarget;
    }

    // the only thing we want to do from here is check the type.
    bool targetIsValid = CStudioObjectTypes::AcceptableParent((EStudioObjectType)m_ObjectType,
                                                              targetType);

    // allow material and image rows as valid targets for image drops
    if (!targetIsValid && m_FileType == DocumentEditorFileType::Image
        && (targetType & (OBJTYPE_MATERIAL | OBJTYPE_CUSTOMMATERIAL | OBJTYPE_REFERENCEDMATERIAL
                          | OBJTYPE_IMAGE))) {
        targetIsValid = true;
    }

    if (!targetIsValid) {
        SetHasValidTarget(false);
        return false;
    } else {
        if (CHotKeys::IsKeyDown(Qt::AltModifier) && targetType != OBJTYPE_SCENE
            && targetType != OBJTYPE_COMPONENT) {
            qt3dsdm::Qt3DSDMInstanceHandle theTarget = inTarget->GetInstance();
            CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
            IDocumentReader &theReader(theDoc->GetDocumentReader());
            qt3dsdm::Qt3DSDMSlideHandle toSlide = theReader.GetAssociatedSlide(theTarget);

            if (!theReader.IsMasterSlide(toSlide))
                targetIsValid = false;
        }

        SetHasValidTarget(targetIsValid);
        return targetIsValid;
    }
}

CFileDropSource::CFileDropSource(long inFlavor, const QString &filePath)
    : CDropSource(inFlavor)
    , m_FilePath(filePath)
{
    const auto objFileType = Q3DStudio::ImportUtils::GetObjectFileTypeForFile(filePath);
    m_ObjectType = objFileType.m_ObjectType;
    m_FileType = objFileType.m_FileType;
}

void CFileDropSource::SetHasValidTarget(bool inValid)
{
    m_HasValidTarget = inValid;
    CFileDropSource::s_FileHasValidTarget = inValid;
}

bool CFileDropSource::GetHasValidTarget()
{
    return CFileDropSource::s_FileHasValidTarget;
}

bool CFileDropSource::CanMove()
{
    return false;
}

bool CFileDropSource::CanCopy()
{
    return true;
}

CCmd *CFileDropSource::GenerateAssetCommand(qt3dsdm::Qt3DSDMInstanceHandle inTarget,
                                            EDROPDESTINATION inDestType,
                                            qt3dsdm::Qt3DSDMSlideHandle inSlide)
{
    CDoc &theDoc(*g_StudioApp.GetCore()->GetDoc());
    CPt thePoint;
    //  if ( CHotKeys::IsKeyDown( Qt::AltModifier ) )
    //      thePoint = GetCurrentPoint();

    long theStartTime = -1;
    if (CHotKeys::IsKeyDown(Qt::ControlModifier))
        theStartTime = theDoc.GetCurrentViewTime();

    if (QFileInfo(m_FilePath).isFile()) {
        QString theCommandName;
        // TODO: internationalize
        switch (m_FileType) {
        case Q3DStudio::DocumentEditorFileType::DAE:
            theCommandName = QObject::tr("File Drop DAE File");
            break;
        case Q3DStudio::DocumentEditorFileType::Import:
            theCommandName = QObject::tr("File Drop Import File");
            break;
        case Q3DStudio::DocumentEditorFileType::Image:
            theCommandName = QObject::tr("File Drop Image File");
            break;
        case Q3DStudio::DocumentEditorFileType::Behavior:
            theCommandName = QObject::tr("File Drop Behavior File");
            break;
        case Q3DStudio::DocumentEditorFileType::Mesh:
            theCommandName = QObject::tr("File Drop Mesh File");
            break;
        case Q3DStudio::DocumentEditorFileType::Font:
            theCommandName = QObject::tr("File Drop Font File");
            break;
        case Q3DStudio::DocumentEditorFileType::Effect:
            theCommandName = QObject::tr("File Drop Effect File");
            break;
        case Q3DStudio::DocumentEditorFileType::Material:
            theCommandName = QObject::tr("File Drop Material File");
            break;
        case Q3DStudio::DocumentEditorFileType::Path:
            theCommandName = QObject::tr("File Drop Path File");
            break;
        case Q3DStudio::DocumentEditorFileType::Presentation:
            theCommandName = QObject::tr("File Drop Subpresentation File");
            break;
        case Q3DStudio::DocumentEditorFileType::QmlStream:
            theCommandName = QObject::tr("File Drop QML Stream File");
            break;
        case Q3DStudio::DocumentEditorFileType::FBX:
            theCommandName = QObject::tr("File Drop FBX File");
            break;
        case Q3DStudio::DocumentEditorFileType::Sound:
            theCommandName = QObject::tr("File Drop Sound File");
            break;
        case Q3DStudio::DocumentEditorFileType::Project:
            theCommandName = QObject::tr("File Drop Project File");
            break;
        case Q3DStudio::DocumentEditorFileType::MaterialData:
            theCommandName = QObject::tr("File Drop Material Data File");
            break;
        default:
            theCommandName = QObject::tr("File Drop Unknown File");
            break;
        }

        auto &bridge(*theDoc.GetStudioSystem()->GetClientDataModelBridge());
        bool isPres = m_FileType == Q3DStudio::DocumentEditorFileType::Presentation
                      || m_FileType == Q3DStudio::DocumentEditorFileType::QmlStream;
        bool isImage =  m_FileType == Q3DStudio::DocumentEditorFileType::Image
                        && inDestType == EDROPDESTINATION_ON;
        bool isMatData =  m_FileType == Q3DStudio::DocumentEditorFileType::MaterialData;
        EStudioObjectType rowType = bridge.GetObjectType(inTarget);
        if (isPres || isImage) { // set as a texture
            Q3DStudio::CString src; // presentation id or image file name
            if (isPres) {
                QString pathFromRoot = QDir(theDoc.GetCore()->getProjectFile().getProjectPath())
                                            .relativeFilePath(m_FilePath);
                src = Q3DStudio::CString::fromQString(theDoc.GetCore()
                                                ->getProjectFile().getPresentationId(pathFromRoot));
            } else { // Image
                src = Q3DStudio::CString::fromQString(QFileInfo(theDoc.GetDocumentPath()).dir()
                                                      .relativeFilePath(m_FilePath));
            }

            if (rowType == OBJTYPE_LAYER) { // Drop on a Layer
                if (isPres) {
                    auto propHandle = bridge.GetSourcePathProperty();
                    Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                            ->SetInstancePropertyValueAsRenderable(inTarget, propHandle, src);
                } else { // Image
                    ChooseImagePropertyDlg dlg(inTarget);
                    if (dlg.exec() == QDialog::Accepted) {
                        qt3dsdm::Qt3DSDMPropertyHandle propHandle = dlg.getSelectedPropertyHandle();
                        Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                            ->SetInstancePropertyValueAsRenderable(inTarget, propHandle, src);
                    }
                }
            } else if (rowType & (OBJTYPE_MATERIAL | OBJTYPE_CUSTOMMATERIAL
                                  | OBJTYPE_REFERENCEDMATERIAL)) { // Drop on a Material
                // if this is a ref material, update the material it references
                qt3dsdm::Qt3DSDMInstanceHandle refInstance = 0;
                if (rowType == OBJTYPE_REFERENCEDMATERIAL) {
                    auto optValue = theDoc.getSceneEditor()->GetInstancePropertyValue(inTarget,
                                    bridge.GetObjectDefinitions().m_ReferencedMaterial
                                    .m_ReferencedMaterial.m_Property);
                    if (optValue.hasValue()) {
                        refInstance = bridge.GetInstance(theDoc.GetSceneInstance(),
                                                         optValue.getValue());
                    }
                }
                ChooseImagePropertyDlg dlg(refInstance ? refInstance : inTarget, refInstance != 0);
                if (isImage)
                    dlg.setTextureTitle();

                if (dlg.exec() == QDialog::Accepted) {
                    qt3dsdm::Qt3DSDMPropertyHandle propHandle = dlg.getSelectedPropertyHandle();
                    if (dlg.detachMaterial()) {
                        Q3DStudio::ScopedDocumentEditor editor(
                            Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc,
                                                              tr("Set material diffuse map")));
                        editor->BeginAggregateOperation();
                        editor->SetMaterialType(inTarget, "Standard Material");
                        editor->setInstanceImagePropertyValue(inTarget, propHandle, src, isPres);
                        editor->EndAggregateOperation();
                    } else {
                        Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                        ->setInstanceImagePropertyValue(refInstance ? refInstance : inTarget,
                                                        propHandle, src, isPres);
                    }
                }
            } else if (rowType == OBJTYPE_IMAGE) {
                auto propHandle = isPres ? bridge.getSubpresentationProperty()
                                         : bridge.GetSourcePathProperty();
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                       ->SetInstancePropertyValueAsRenderable(inTarget, propHandle, src);
            } else if (rowType == OBJTYPE_SCENE) { // dropping on the scene as a texture
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                       ->addRectFromSource(src, inSlide, thePoint, theStartTime);
            }
        } else if (isMatData) {
            if (rowType == OBJTYPE_REFERENCEDMATERIAL || rowType == OBJTYPE_MATERIAL
                    || rowType == OBJTYPE_CUSTOMMATERIAL) {
                const auto sceneEditor = theDoc.getSceneEditor();
                const Q3DStudio::CString materialName = Q3DStudio::CString::fromQString(
                            QFileInfo(m_FilePath).completeBaseName());
                QString name;
                QMap<QString, QString> values;
                QMap<QString, QMap<QString, QString>> textureValues;
                sceneEditor->getMaterialInfo(m_FilePath, name, values, textureValues);
                const auto material = sceneEditor->getOrCreateMaterial(materialName);
                Q3DStudio::CString docDir = theDoc.GetDocumentDirectory();
                Q3DStudio::CFilePath relPath = Q3DStudio::CFilePath::GetRelativePathFromBase(
                            docDir, Q3DStudio::CString::fromQString(m_FilePath));
                Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                        ->setMaterialProperties(inTarget, name, relPath, values, textureValues);
                theDoc.SelectDataModelObject(inTarget);
            }
        } else {
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(theDoc, theCommandName)
                ->ImportFile(static_cast<Q3DStudio::DocumentEditorFileType::Enum>(m_FileType),
                             Q3DStudio::CString::fromQString(m_FilePath), inTarget, inSlide,
                             CDialogs::GetImportFileExtension(),
                             Q3DStudio::ImportUtils::GetInsertTypeForDropType(inDestType), thePoint,
                             theStartTime);
        }
    }
    return nullptr;
}
