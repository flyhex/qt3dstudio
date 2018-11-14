/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#include "GroupTimelineItemBinding.h"
#include "TimelineTranslationManager.h"
#include "StudioApp.h"
#include "Core.h"
#include "Dialogs.h"

// Data model specific
#include "Doc.h"
#include "CmdGeneric.h"

#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSFileTools.h"

using namespace qt3dsdm;

CGroupTimelineItemBinding::CGroupTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                                     Qt3DSDMInstanceHandle inDataHandle)
    : Qt3DSDMTimelineItemBinding(inMgr, inDataHandle)
{
}

//=============================================================================
/**
 * Ideally we like to be able to edit the component in a different editor ( we've been hoping for
 * that feature ) BUT we don't have that,
 * and it has always been we 'dive' into the component within Studio.
 */
bool CGroupTimelineItemBinding::OpenAssociatedEditor()
{
    if (GetObjectType() == OBJTYPE_COMPONENT) {
        ISlideSystem *theSlideSystem = m_StudioSystem->GetSlideSystem();

        qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
        Q3DStudio::CId theId = m_StudioSystem->GetClientDataModelBridge()->GetGUID(theInstance);
        qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
            theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theId));

        if (theMasterSlide.Valid()) {
            Qt3DSDMSlideHandle theActiveSlide = theSlideSystem->GetActiveSlide(theMasterSlide);

            CCmd *theCmd = new CCmdGeneric<CDoc, Qt3DSDMSlideHandle>(
                m_TransMgr->GetDoc(), &CDoc::NotifyActiveSlideChanged,
                &CDoc::NotifyActiveSlideChanged, theActiveSlide, NULL, "");
            theCmd->SetUndoable(false);
            theCmd->SetModifiedFlag(false);
            m_TransMgr->GetDoc()->GetCore()->ExecuteCommand(theCmd, false);
        }
        return true;
    }
    return false;
}

bool CGroupTimelineItemBinding::IsImported() const
{

    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    qt3dsdm::IPropertySystem *thePropertySystem =
        m_TransMgr->GetDoc()->GetStudioSystem()->GetPropertySystem();
    qt3dsdm::SValue theValue;
    if (thePropertySystem->GetInstancePropertyValue(theInstance, m_TransMgr->GetDoc()
                                                                     ->GetStudioSystem()
                                                                     ->GetClientDataModelBridge()
                                                                     ->GetSourcePathProperty(),
                                                    theValue)) {
        qt3dsdm::TDataStrPtr theSrcPath(qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue));
        QFileInfo theFilePath(theSrcPath->toQString());
        if (theFilePath.suffix() == CDialogs::GetImportFileExtension())
            return true;
    }
    // If it is, check to be sure that
    // we can get to the import file.
    // If we can, then we are imported.
    return false;
}
