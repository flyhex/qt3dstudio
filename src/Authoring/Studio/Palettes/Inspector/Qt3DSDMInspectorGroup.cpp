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
#include "Qt3DSDMInspectorGroup.h"
#include "Qt3DSDMInspectorRow.h"
#include "Qt3DSDMInspectable.h"
#include "StudioApp.h"
#include "Core.h"
#include "Qt3DSDMMetaData.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"

//==============================================================================
/**
 * Constructor
 */
Qt3DSDMInspectorGroup::Qt3DSDMInspectorGroup(CStudioApp &inApp, const QString &inName,
                                           Qt3DSDMInspectable &inInspectable, long inIndex)
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
Qt3DSDMInspectorGroup::~Qt3DSDMInspectorGroup()
{
    std::vector<Q3DStudio::Qt3DSDMInspectorRow *>::iterator theIterator =
        m_UICDMInspectorRows.begin();
    for (; theIterator != m_UICDMInspectorRows.end(); ++theIterator)
        delete (*theIterator);
}

//==============================================================================
/**
 * Method to create a new InspectorRowBase.
 */
void Qt3DSDMInspectorGroup::CreateRow(CDoc *inDoc, qt3dsdm::Qt3DSDMMetaDataPropertyHandle inProperty)
{
    Q3DStudio::Qt3DSDMInspectorRow *theUICDMRow =
        new Q3DStudio::Qt3DSDMInspectorRow(inDoc, inProperty);
    m_UICDMInspectorRows.push_back(
        theUICDMRow); // this Qt3DSDMInspectorRow is now owned by this class

}
