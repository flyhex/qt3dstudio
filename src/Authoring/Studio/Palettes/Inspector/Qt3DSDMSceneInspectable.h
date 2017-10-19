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
#ifndef INCLUDED_UICDM_SCENE_INSPECTABLE_H
#define INCLUDED_UICDM_SCENE_INSPECTABLE_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMInspectable.h"

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
/**
*	For inspecting scene data model instances
*/
class Qt3DSDMSceneInspectable : public Qt3DSDMInspectable
{
public:
    Qt3DSDMSceneInspectable(CStudioApp &inApp, CCore *inCore, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMInstanceHandle inCurrentActiveSlideInstance);

    bool IsValid() const override;
    // Qt3DSDMInspectable
    long GetGroupCount() override;

protected:
    inline Q3DStudio::CString GetGroupName(long inGroupIndex) override;
    inline qt3dsdm::Qt3DSDMInstanceHandle GetGroupInstance(long inGroupIndex) override;

    qt3dsdm::Qt3DSDMInstanceHandle m_CurrentActiveSlideInstance;
};

#endif
