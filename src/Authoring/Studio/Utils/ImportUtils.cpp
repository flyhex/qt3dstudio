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

#include "stdafx.h"
#include "ImportUtils.h"
#include "Dialogs.h"
#include "UICFileTools.h"

namespace Q3DStudio {

SObjectFileType ImportUtils::GetObjectFileTypeForFile(const CFilePath &inFile,
                                                      bool inCheckFileExists /*= true*/)
{
    if (inCheckFileExists && inFile.IsFile() == false)
        return SObjectFileType(OBJTYPE_UNKNOWN, DocumentEditorFileType::Unknown);

    Q3DStudio::CString theExtension(inFile.GetExtension());
    theExtension.ToLower();

    if (theExtension.Compare(CDialogs::GetImportFileExtension(), Q3DStudio::CString::ENDOFSTRING,
                             false))
        return SObjectFileType(OBJTYPE_GROUP, DocumentEditorFileType::Import);
    else if (theExtension.Compare(CDialogs::GetMeshFileExtension(), Q3DStudio::CString::ENDOFSTRING,
                                  false))
        return SObjectFileType(OBJTYPE_MODEL, DocumentEditorFileType::Mesh);
    else if (CDialogs::IsImageFileExtension(theExtension))
        return SObjectFileType(
            OBJTYPE_MODEL, OBJTYPE_IMAGE,
            DocumentEditorFileType::Image); // Drag-drop image to scene will auto-map to Rectangle.
    else if (theExtension.Compare(CDialogs::GetLUAFileExtension(),
                                  Q3DStudio::CString::ENDOFSTRING, false)
             || theExtension.Compare(CDialogs::GetQmlFileExtension(),
                                     Q3DStudio::CString::ENDOFSTRING, false))
        return SObjectFileType(OBJTYPE_BEHAVIOR, DocumentEditorFileType::Behavior);
    else if (CDialogs::IsFontFileExtension(theExtension))
        return SObjectFileType(OBJTYPE_TEXT, DocumentEditorFileType::Font);
    else if (CDialogs::IsEffectFileExtension(theExtension))
        return SObjectFileType(OBJTYPE_EFFECT, DocumentEditorFileType::Effect);
    else if (CDialogs::IsMaterialFileExtension(theExtension))
        return SObjectFileType(OBJTYPE_CUSTOMMATERIAL, DocumentEditorFileType::Material);
    else if (CDialogs::IsPathFileExtension(theExtension))
        return SObjectFileType(OBJTYPE_PATH, DocumentEditorFileType::Path);
    else if (CDialogs::IsPathBufferExtension(theExtension))
        return SObjectFileType(OBJTYPE_PATH, DocumentEditorFileType::Path);
    else if (CDialogs::IsSoundFileExtension(theExtension))
        return SObjectFileType(OBJTYPE_SOUND, DocumentEditorFileType::Sound);

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
