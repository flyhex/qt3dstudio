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
#include "Strings.h"
#include "StringLoader.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMSceneInspectable.h"
#include "Core.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"

Qt3DSDMSceneInspectable::Qt3DSDMSceneInspectable(
    CStudioApp &inApp, CCore *inCore, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
    qt3dsdm::Qt3DSDMInstanceHandle inCurrentActiveSlideInstance)
    : Qt3DSDMInspectable(inApp, inCore, inInstance)
    , m_CurrentActiveSlideInstance(inCurrentActiveSlideInstance)
{
}

bool Qt3DSDMSceneInspectable::IsValid() const
{
    return Qt3DSDMInspectable::IsValid()
        && m_Core->GetDoc()->GetStudioSystem()->IsInstance(m_CurrentActiveSlideInstance);
}

long Qt3DSDMSceneInspectable::GetGroupCount()
{
    return 2; // hard-coded to basic and shared
}

//==============================================================================
/**
 *	Return the Resource String ID for the Group Name, given the group index
 */
Q3DStudio::CString Qt3DSDMSceneInspectable::GetGroupName(long inGroupIndex)
{
    return (inGroupIndex == 0) ? ::LoadResourceString(IDS_PROPERTIES_BASIC)
                               : ::LoadResourceString(IDS_PROPERTIES_SHARED);
}

qt3dsdm::Qt3DSDMInstanceHandle Qt3DSDMSceneInspectable::GetGroupInstance(long inGroupIndex)
{
    return (inGroupIndex == 0) ? m_CurrentActiveSlideInstance : m_Instance;
}