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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "SlideTimelineItemBinding.h"
#include "BaseStateRow.h"

// Data model specific
#include "Doc.h"
#include "CmdGeneric.h"
#include "EmptyTimelineTimebar.h"

#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "ClientDataModelBridge.h"

using namespace qt3dsdm;

CSlideTimelineItemBinding::CSlideTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                                     Qt3DSDMInstanceHandle inDataHandle)
    : CUICDMTimelineItemBinding(inMgr)
{
    qt3dsdm::Qt3DSDMSlideHandle theSlideHandle =
        m_StudioSystem->GetSlideSystem()->GetSlideByInstance(inDataHandle);

    // Get the owning component of m_SlideHandle.
    // This should return CAsset OBJTYPE_SCENE or OBJTYPE_COMPONENT.
    qt3dsdm::Qt3DSDMInstanceHandle theInstance =
        m_StudioSystem->GetClientDataModelBridge()->GetOwningComponentInstance(theSlideHandle);
    SetInstanceHandle(theInstance);

    // Listen to change on Asset name
    IStudioFullSystemSignalProvider *theEngine = m_StudioSystem->GetFullSystemSignalProvider();
    std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> theSetter(
        std::bind(&CSlideTimelineItemBinding::OnPropertyChanged, this, std::placeholders::_2));
    m_Connection = theEngine->ConnectInstancePropertyValue(
        std::bind(qt3dsdm::MaybackCallbackInstancePropertyValue<std::function<void(
                        Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>>,
                    std::placeholders::_1, std::placeholders::_2, theInstance,
                    m_StudioSystem->GetClientDataModelBridge()->GetNameProperty(), theSetter));
}

ITimelineTimebar *CSlideTimelineItemBinding::GetTimebar()
{ // No timebars on slides
    return new CEmptyTimelineTimebar();
}

void CSlideTimelineItemBinding::SetName(const Q3DStudio::CString & /*inName*/)
{
    // Do nothing because name is read only
}

void CSlideTimelineItemBinding::Bind(CBaseStateRow *inRow)
{
    CUICDMTimelineItemBinding::Bind(inRow);
    GetRow()->SetNameReadOnly(true);
}

bool CSlideTimelineItemBinding::IsValidTransaction(EUserTransaction inTransaction)
{
    qt3dsdm::Qt3DSDMInstanceHandle theInstance = GetInstance();
    switch (inTransaction) {
    // Disable the following context menus
    case EUserTransaction_Rename:
    case EUserTransaction_MakeComponent:
    case EUserTransaction_EditComponent:
        return false;
    }

    return CUICDMTimelineItemBinding::IsValidTransaction(inTransaction);
}
