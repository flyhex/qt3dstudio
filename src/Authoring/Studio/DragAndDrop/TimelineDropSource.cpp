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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "TimelineDropSource.h"
#include "FileDropSource.h"
#include "Dispatch.h"
#include "DropTarget.h"
#include "StudioObjectTypes.h"
#include "HotKeys.h"
#include "Core.h"
#include "Doc.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "UICDMSlides.h"
#include "Bindings/UICDMTimelineItemBinding.h"
#include "IDocumentEditor.h"
#include "ImportUtils.h"

#pragma warning(disable : 4100)

using namespace Q3DStudio;

//===============================================================================
/**
 *
 */
CTimeLineDropSource::CTimeLineDropSource(long inFlavor, IDragable *inDraggable)
    : CDropSource(inFlavor, sizeof(inDraggable))
{
    m_Copy = true;

    m_Instances = g_StudioApp.GetCore()->GetDoc()->GetSelectedValue().GetSelectedInstances();
    if (m_Instances.size())
        m_ObjectType = g_StudioApp.GetCore()
                           ->GetDoc()
                           ->GetStudioSystem()
                           ->GetClientDataModelBridge()
                           ->GetObjectType(m_Instances[0]);
    else
        m_ObjectType = OBJTYPE_UNKNOWN;
}

//===============================================================================
/**
 *
 */
bool CTimeLineDropSource::CanMove()
{
    return !m_Copy;
}

//===============================================================================
/**
 *
 */
bool CTimeLineDropSource::CanCopy()
{
    bool theReturn = false;
    // This is here because some Assets can not be copied ( scene, material )
    theReturn = m_Copy && g_StudioApp.GetCore()->GetDoc()->CanCopyObject(m_Instances);

    return theReturn;
}

//===============================================================================
/**
 *
 */
void CTimeLineDropSource::InterpretKeyFlags(long inModifyerKeys)
{
    m_Copy = ((inModifyerKeys & CHotKeys::MODIFIER_CONTROL) != 0);
}

//===============================================================================
/**
 *
 */
bool CTimeLineDropSource::ValidateTarget(CDropTarget *inTarget)
{
    // the only thing we want to do from here is check the type.
    bool theValidTarget = CStudioObjectTypes::AcceptableParent(
        (EStudioObjectType)GetObjectType(), (EStudioObjectType)inTarget->GetObjectType());

    for (size_t idx = 0, end = m_Instances.size(); idx < end && theValidTarget; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theHandle(m_Instances[idx]);

        if (theValidTarget && theHandle.Valid()) {
            theValidTarget &= (!inTarget->IsSelf(theHandle) && !inTarget->IsRelative(theHandle));
            qt3dsdm::ISlideSystem *theSlideSystem =
                g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetSlideSystem();
            qt3dsdm::CUICDMSlideHandle theSlide = theSlideSystem->GetAssociatedSlide(theHandle);
            bool theIsMaster = theSlideSystem->IsMasterSlide(theSlide);

            theValidTarget &= !(theIsMaster && !inTarget->IsMaster());
        }
    }

    SetHasValidTarget(theValidTarget);

    return theValidTarget;
}

using namespace qt3dsdm;
using namespace Q3DStudio;

inline void Rearrange(CDoc &inDoc, const qt3dsdm::TInstanceHandleList &inInstances,
                      Qt3DSDMInstanceHandle inTarget, DocumentEditorInsertType::Enum inInsertType)
{
    SCOPED_DOCUMENT_EDITOR(inDoc, QObject::tr("Rearrange Object"))
        ->RearrangeObjects(inInstances, inTarget, inInsertType);
}

CCmd *CTimeLineDropSource::GenerateAssetCommand(qt3dsdm::Qt3DSDMInstanceHandle inTarget,
                                                EDROPDESTINATION inDestType,
                                                qt3dsdm::CUICDMSlideHandle inSlide)
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    CClientDataModelBridge *theBridge = theDoc->GetStudioSystem()->GetClientDataModelBridge();

    if (CanCopy()) {
        SCOPED_DOCUMENT_EDITOR(*theDoc, QObject::tr("Duplicate Object"))
            ->DuplicateInstances(m_Instances, inTarget,
                                 ImportUtils::GetInsertTypeForDropType(inDestType));
    } else {
        // We can't do the rearrange inline because it deletes a timeline item.
        // So we will effectively postmessage and do it out of line.
        theDoc->GetCore()->GetDispatch()->FireOnAsynchronousCommand(
            std::bind(Rearrange, std::ref(*theDoc), m_Instances, inTarget,
                        ImportUtils::GetInsertTypeForDropType(inDestType)));
    }

    return nullptr;
}
