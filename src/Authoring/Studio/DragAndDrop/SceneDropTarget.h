/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef __SCENEDROPTARGET_H__
#define __SCENEDROPTARGET_H__

//==============================================================================
//	Includes
//==============================================================================
#include "DropTarget.h"

//==============================================================================
/**
 *	@class
 *	@brief	LEONARD.PONCE needs to enter a brief description here.
 *
 *	LEONARD.PONCE needs to enter a long description here.
 */
class CSceneViewDropTarget : public CDropTarget
{
protected:
    long m_DropTime; ///< The Time to drop any thing.
    long m_DropSourceObjectType;

public:
    CSceneViewDropTarget();
    bool Accept(CDropSource &inSource) override;
    bool Drop(CDropSource &inSource) override;
    qt3dsdm::Qt3DSDMInstanceHandle GetInstance() override;

    bool IsRelative(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    bool IsSelf(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    long GetObjectType() override;
    void SetDropSourceObjectType(long inObjType);
    void SetDropTime(long inDropTime);
    long GetDropTime();
};
#endif // #ifndef __SCENEDROPTARGET_H__
