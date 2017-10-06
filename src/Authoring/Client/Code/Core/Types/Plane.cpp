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
#include "stdafx.h"
#include "Plane.h"
#include "Matrix.h"
#include "BoundingBox.h"
#include "Vector3.h"

namespace Q3DStudio {

//==============================================================================
//								CONSTRUCTION
//==============================================================================

//==============================================================================
/**
 *	Construct a default plane.  MF: Hmmm should the normal be zero?
 */

CPlane::CPlane()
    : m_D(0)
{
}

//==============================================================================
/**
 *	Construct a given plane.
 */

CPlane::CPlane(const CVector3 &inNormal, const float &inDistance)
    : m_Normal(inNormal)
    , m_D(inDistance)
{
}

//==============================================================================
//								OPERATORS
//==============================================================================

//==============================================================================
/**
 *	Normalize the representation of the plane
 */
void CPlane::Normalize()
{
    float theInvLength = 1.0f / m_Normal.Length();

    m_Normal *= theInvLength;
    m_D *= theInvLength;
}

//==============================================================================
/**
 *	See if the given box is inside, outside or straddling this plane.
 */
CPlane::ERelation CPlane::CheckRelation(const CBoundingBox &inBox) const
{
    CVector3 theNearPoint(inBox.MaxPoint());
    CVector3 theFarPoint(inBox.MinPoint());

    // Find the corners that most likely to straddle this plane
    // by finding the one furthest in the direction of the
    // normals and the other in the furthest opposite direction.
    if (m_Normal.x > 0) {
        theNearPoint.x = inBox.MinPoint().x;
        theFarPoint.x = inBox.MaxPoint().x;
    }
    if (m_Normal.y > 0) {
        theNearPoint.y = inBox.MinPoint().y;
        theFarPoint.y = inBox.MaxPoint().y;
    }
    if (m_Normal.z > 0) {
        theNearPoint.z = inBox.MinPoint().z;
        theFarPoint.z = inBox.MaxPoint().z;
    }

    // If the near point is in front then all points are in front...
    if (m_Normal.DotProduct(theNearPoint) + m_D > 0)
        return CPlane::RELATION_FRONT;

    // If the near point is in back but the far point is in front, then the box clips...
    if (m_Normal.DotProduct(theFarPoint) + m_D > 0)
        return CPlane::RELATION_CLIPPED;

    // Else, all points are behind.
    return CPlane::RELATION_BACK;
}

}; // namespace Q3DStudio
