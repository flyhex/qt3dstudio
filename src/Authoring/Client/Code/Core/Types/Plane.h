/****************************************************************************
**
** Copyright (C) 1999-2004 NVIDIA Corporation.
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
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Vector3.h"

namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CBoundingBox;

//==============================================================================
/**
 *	@class CPlane
 *	@brief Geometrical primitive - a boundless plane.
 */
class CPlane
{
    //==============================================================================
    //	Enumerations
    //==============================================================================
public:
    enum ERelation { RELATION_FRONT, RELATION_BACK, RELATION_CLIPPED };

    //==============================================================================
    //	Fields
    //==============================================================================
public:
    CVector3 m_Normal; ///< Normal vector of the plane
    float m_D; ///< Distance to plane from 0,0,0

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CPlane();
    CPlane(const CVector3 &inNormal, const float &inDistance);

public: // Access
    void Normalize();
    ERelation CheckRelation(const CBoundingBox &inBox) const;
};
} // namespace Q3DStudio
