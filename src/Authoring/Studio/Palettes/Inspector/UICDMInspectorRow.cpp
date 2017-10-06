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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "UICDMInspectorRow.h"
#include "UICDMMetaData.h"
#include "Doc.h"
#include "StudioApp.h"
#include "UICDMSlides.h"
#include "UICDMStudioSystem.h"
#include "UICDMAnimation.h"
#include "UICDMSignals.h"
#include "CmdDataModelDeanimate.h"
#include "UICDMDataCore.h"
#include "Core.h"
#include "ClientDataModelBridge.h"
#include "IDocumentEditor.h"

//==============================================================================
//	Namespace
//==============================================================================
using namespace UICDM;
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
CUICDMInspectorRow::CUICDMInspectorRow(CDoc *inDoc,
                                       CUICDMMetaDataPropertyHandle inProperty)
    : m_MetaProperty(inProperty)
{
    IMetaData *theMetaData = inDoc->GetStudioSystem()->GetActionMetaData();
    SMetaDataPropertyInfo theInfo(theMetaData->GetMetaDataPropertyInfo(inProperty));
    m_MetaDataPropertyInfo = theInfo;
}

//==============================================================================
/**
 *	Destructor
 */
CUICDMInspectorRow::~CUICDMInspectorRow()
{
}

} // namespace Q3DStudio
