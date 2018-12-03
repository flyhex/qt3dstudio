/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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
#include "ImportUtils.h"
#include "Dialogs.h"
#include "Qt3DSFileTools.h"
#include "StudioApp.h"

namespace Q3DStudio {

SObjectFileType ImportUtils::GetObjectFileTypeForFile(const QString &filePath,
                                                      bool inCheckFileExists /*= true*/)
{
    QFileInfo info(filePath);
    if (inCheckFileExists && !info.isFile())
        return SObjectFileType(OBJTYPE_UNKNOWN, DocumentEditorFileType::Unknown);

    QString ext(info.suffix()); // file extension

    if (ext.compare(CDialogs::GetImportFileExtension(), Qt::CaseInsensitive) == 0) {
        return SObjectFileType(OBJTYPE_GROUP, DocumentEditorFileType::Import);
    } else if (ext.compare(CDialogs::GetMeshFileExtension(), Qt::CaseInsensitive) == 0) {
        return SObjectFileType(OBJTYPE_MODEL, DocumentEditorFileType::Mesh);
    } else if (CDialogs::IsImageFileExtension(ext)) {
        // Drag-drop image to scene will auto-map to Rectangle.
        return SObjectFileType(OBJTYPE_MODEL, OBJTYPE_IMAGE, DocumentEditorFileType::Image);
    } else if (ext.compare(CDialogs::GetQmlFileExtension(), Qt::CaseInsensitive) == 0) {
        return g_StudioApp.isQmlStream(filePath)
                ? SObjectFileType(OBJTYPE_QML_STREAM, DocumentEditorFileType::QmlStream)
                : SObjectFileType(OBJTYPE_BEHAVIOR, DocumentEditorFileType::Behavior);
    } else if (ext.compare(CDialogs::GetMaterialDataFileExtension(), Qt::CaseInsensitive) == 0) {
        return SObjectFileType(OBJTYPE_MATERIALDATA, DocumentEditorFileType::MaterialData);
    } else if (CDialogs::IsFontFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_TEXT, DocumentEditorFileType::Font);
    } else if (CDialogs::IsEffectFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_EFFECT, DocumentEditorFileType::Effect);
    } else if (CDialogs::IsMaterialFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_CUSTOMMATERIAL, DocumentEditorFileType::Material);
    } else if (CDialogs::IsPathFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_PATH, DocumentEditorFileType::Path);
    } else if (CDialogs::IsPathBufferExtension(ext)) {
        return SObjectFileType(OBJTYPE_PATH, DocumentEditorFileType::Path);
    } else if (CDialogs::IsSoundFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_SOUND, DocumentEditorFileType::Sound);
    } else if (CDialogs::isPresentationFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_PRESENTATION, DocumentEditorFileType::Presentation);
    } else if (CDialogs::isProjectFileExtension(ext)) {
        return SObjectFileType(OBJTYPE_PROJECT, DocumentEditorFileType::Project);
    }

    return SObjectFileType(OBJTYPE_UNKNOWN, DocumentEditorFileType::Unknown);
}

DocumentEditorInsertType::Enum ImportUtils::GetInsertTypeForDropType(EDROPDESTINATION inDestination)
{
    switch (inDestination) {
    case EDROPDESTINATION_ON:
        return DocumentEditorInsertType::LastChild;
    case EDROPDESTINATION_ABOVE:
        return DocumentEditorInsertType::PreviousSibling;
    case EDROPDESTINATION_BELOW:
        return DocumentEditorInsertType::NextSibling;
    }
    assert(0);
    return DocumentEditorInsertType::LastChild;
}
}
