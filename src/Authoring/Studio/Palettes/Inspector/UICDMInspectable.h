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
#ifndef INCLUDED_UICDM_INSPECTABLE_H
#define INCLUDED_UICDM_INSPECTABLE_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "InspectableBase.h"
#include "UICDMHandles.h"
#include "StudioApp.h"

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
/**
*	For inspecting data model instances
*/
class CUICDMInspectable : public CInspectableBase
{
protected: // Fields
    UICDM::CUICDMInstanceHandle m_Instance;
    UICDM::CUICDMInstanceHandle m_DualPersonalityInstance;
    CStudioApp &m_App;

public: // Constructor
    CUICDMInspectable(CStudioApp &inApp, CCore *inCore, UICDM::CUICDMInstanceHandle inInstance,
                      UICDM::CUICDMInstanceHandle inDualPersonalityInstance = 0);

public: // CInspectableBase
    Q3DStudio::CString GetName() override;
    long GetGroupCount() override;
    CInspectorGroup *GetGroup(long) override;
    EStudioObjectType GetObjectType() override;
    // virtual std::wstring			GetTypeString( ) const;
    bool IsValid() const override;
    bool IsMaster() override;
    virtual UICDM::TMetaDataPropertyHandleList GetGroupProperties(long inGroupIndex);
    virtual UICDM::CUICDMInstanceHandle GetGroupInstance(long inGroupIndex);

protected:
    virtual Q3DStudio::CString GetGroupName(long inGroupIndex);
};

#endif
