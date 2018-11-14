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
#ifndef INCLUDED_QT3DSDM_INSPECTABLE_H
#define INCLUDED_QT3DSDM_INSPECTABLE_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "InspectableBase.h"
#include "Qt3DSDMHandles.h"
#include "StudioApp.h"

//==============================================================================
/**
*	For inspecting data model instances
*/
class Qt3DSDMInspectable : public CInspectableBase
{
protected: // Fields
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    qt3dsdm::Qt3DSDMInstanceHandle m_DualPersonalityInstance;
    CStudioApp &m_App;

public: // Constructor
    Qt3DSDMInspectable(CStudioApp &inApp, CCore *inCore, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                       qt3dsdm::Qt3DSDMInstanceHandle inDualPersonalityInstance = 0);

public: // CInspectableBase
    QString GetName() override;
    long GetGroupCount() override;
    CInspectorGroup *GetGroup(long) override;
    EStudioObjectType GetObjectType() override;
    bool IsValid() const override;
    bool IsMaster() override;
    virtual qt3dsdm::TMetaDataPropertyHandleList GetGroupProperties(long inGroupIndex);
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetGroupInstance(long inGroupIndex);

protected:
    virtual QString GetGroupName(long inGroupIndex);
};

#endif
