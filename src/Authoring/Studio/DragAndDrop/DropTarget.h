/****************************************************************************
**
** Copyright (C) 1999-2003 NVIDIA Corporation.
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

#ifndef INCLUDED_DROPTARGET
#define INCLUDED_DROPTARGET

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "StudioObjectTypes.h"
#include "UICDMHandles.h"

class CDropSource;
class CStudioApp;

class CDropTarget
{
protected:
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    long m_ObjectType;

public:
    CDropTarget();
    virtual ~CDropTarget() {}

    virtual bool Accept(CDropSource &inSource) = 0;
    virtual bool Drop(CDropSource &inSource) = 0;
    virtual long GetObjectType() = 0;

    virtual void SetInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) { m_Instance = inInstance; }
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetInstance() { return m_Instance; }

    virtual bool IsRelative(qt3dsdm::Qt3DSDMInstanceHandle) { return false; }
    virtual bool IsSelf(qt3dsdm::Qt3DSDMInstanceHandle) { return false; }
    virtual bool IsMaster() { return false; }
    virtual bool CanAddToMaster();
};

#endif