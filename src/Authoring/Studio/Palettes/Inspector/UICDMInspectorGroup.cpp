/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#include "UICDMInspectorGroup.h"
#include "UICDMInspectorRow.h"
#include "UICDMInspectable.h"
#include "StudioApp.h"
#include "Core.h"
#include "UICDMMetaData.h"
#include "Doc.h"
#include "UICDMStudioSystem.h"

//==============================================================================
/**
 * Constructor
 */
CUICDMInspectorGroup::CUICDMInspectorGroup(CStudioApp &inApp, const QString &inName,
                                           CUICDMInspectable &inInspectable, long inIndex)
    : CEasyInspectorGroup(inName)
    , m_App(inApp)
    , m_Inspectable(inInspectable)
    , m_Index(inIndex)
{
}

//==============================================================================
/**
 *	clean up
 */
CUICDMInspectorGroup::~CUICDMInspectorGroup()
{
    std::vector<Q3DStudio::CUICDMInspectorRow *>::iterator theIterator =
        m_UICDMInspectorRows.begin();
    for (; theIterator != m_UICDMInspectorRows.end(); ++theIterator)
        delete (*theIterator);
}

//==============================================================================
/**
 * Method to create a new InspectorRowBase.
 */
void CUICDMInspectorGroup::CreateRow(CDoc *inDoc, qt3dsdm::CUICDMMetaDataPropertyHandle inProperty)
{
    Q3DStudio::CUICDMInspectorRow *theUICDMRow =
        new Q3DStudio::CUICDMInspectorRow(inDoc, inProperty);
    m_UICDMInspectorRows.push_back(
        theUICDMRow); // this CUICDMInspectorRow is now owned by this class

}
