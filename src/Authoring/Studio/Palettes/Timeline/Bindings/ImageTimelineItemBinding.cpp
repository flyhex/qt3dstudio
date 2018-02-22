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
#include "ImageTimelineItemBinding.h"
#include "TimelineTranslationManager.h"
#include "Qt3DSDMHandles.h"
#include "BaseStateRow.h"
#include "Doc.h"
#include "IObjectReferenceHelper.h"
#include "EmptyTimelineTimebar.h"

using namespace qt3dsdm;

CImageTimelineItemBinding::CImageTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                                     Qt3DSDMInstanceHandle inDataHandle)
    : Qt3DSDMTimelineItemBinding(inMgr, inDataHandle)
{
}

CImageTimelineItemBinding::~CImageTimelineItemBinding()
{
}

ITimelineTimebar *CImageTimelineItemBinding::GetTimebar()
{ // No timebars on images
    return new CEmptyTimelineTimebar();
}

Q3DStudio::CString CImageTimelineItemBinding::GetName() const
{
    return m_Name;
}

void CImageTimelineItemBinding::SetName(const Q3DStudio::CString &inName)
{
    m_Name = inName;
}

EStudioObjectType CImageTimelineItemBinding::GetObjectType() const
{
    return OBJTYPE_IMAGE;
}

bool CImageTimelineItemBinding::ShowToggleControls() const
{
    // no toggle controls, by design
    return false;
}

void CImageTimelineItemBinding::Bind(CBaseStateRow *inRow)
{
    Qt3DSDMTimelineItemBinding::Bind(inRow);
    inRow->requestSetNameReadOnly();
}

//=============================================================================
/**
 *	Open the associated item as though it was double-clicked in explorer
 */
bool CImageTimelineItemBinding::OpenAssociatedEditor()
{
    return OpenSourcePathFile();
}
