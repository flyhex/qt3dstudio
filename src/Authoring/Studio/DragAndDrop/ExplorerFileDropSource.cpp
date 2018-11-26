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
#include "ExplorerFileDropSource.h"
#include "Dialogs.h"
#include "DropTarget.h"
#include "StudioObjectTypes.h"
#include "IDragable.h"
#include "Qt3DSFileTools.h"
#include "ImportUtils.h"

bool CExplorerFileDropSource::s_FileHasValidTarget = false;

//===============================================================================
/**
 *
 */
bool CExplorerFileDropSource::ValidateTarget(CDropTarget *inTarget)
{
    // Check the type is valid and if target can accept
    bool theValidTarget =
        ((m_ObjectType != OBJTYPE_UNKNOWN) && (inTarget->GetObjectType() == QT3DS_FLAVOR_FILE));
    SetHasValidTarget(theValidTarget);

    return theValidTarget;
}

//===============================================================================
/**
 *
 */
CExplorerFileDropSource::CExplorerFileDropSource(long inFlavor, void *inData, unsigned long inSize)
    : CDropSource(inFlavor, inSize)
    , m_File("")
{
    // Pull out all of the SDropItemData and build a file.
    m_File = *(Qt3DSFile *)inData;
    QFileInfo thePath(m_File.GetAbsolutePath());
    m_ObjectType = Q3DStudio::ImportUtils::GetObjectFileTypeForFile(
                thePath.absoluteFilePath()).m_IconType;
    // Fix because DAE files are the *only* thing you can drop onto the project
    if (thePath.suffix().compare(CDialogs::GetDAEFileExtension(), Qt::CaseInsensitive) == 0) {
        m_ObjectType = OBJTYPE_GROUP;
    }
#ifdef QT_3DSTUDIO_FBX
    else if (thePath.suffix().compare(CDialogs::GetFbxFileExtension(), Qt::CaseInsensitive) == 0) {
        m_ObjectType = OBJTYPE_GROUP;
    }
#endif
}

//===============================================================================
/**
 *
 */
void CExplorerFileDropSource::SetHasValidTarget(bool inValid)
{
    m_HasValidTarget = inValid;
    CExplorerFileDropSource::s_FileHasValidTarget = inValid;
}

//===============================================================================
/**
 *
 */
bool CExplorerFileDropSource::GetHasValidTarget()
{
    return CExplorerFileDropSource::s_FileHasValidTarget;
}

//===============================================================================
/**
 *
 */
bool CExplorerFileDropSource::CanMove()
{
    return false;
}

//===============================================================================
/**
 *
 */
bool CExplorerFileDropSource::CanCopy()
{
    return true;
}
