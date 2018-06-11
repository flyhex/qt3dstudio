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
#include "Qt3DSCommonPrecompile.h"
#include "Frustum.h"
#include "Matrix.h"
#include "BoundingBox.h"

namespace Q3DStudio {

//==============================================================================
//								CONSTRUCTION
//==============================================================================

//==============================================================================
/**
 *	Construct an empty frustum
 */

CFrustum::CFrustum()
{
}

//==============================================================================
/**
 *	Construct a fustum from a view/projection matrix.
 *	http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
 */

void CFrustum::Set(const CMatrix &inProjection)
{
#define M(x, y) inProjection.m[x][y]
    // Left clipping plane
    m_Planes[SIDE_LEFT].m_Normal.x = M(0, 3) + M(0, 0);
    m_Planes[SIDE_LEFT].m_Normal.y = M(1, 3) + M(1, 0);
    m_Planes[SIDE_LEFT].m_Normal.z = M(2, 3) + M(2, 0);
    m_Planes[SIDE_LEFT].m_D = M(3, 3) + M(3, 0);

    // Right clipping plane
    m_Planes[SIDE_RIGHT].m_Normal.x = M(0, 3) - M(0, 0);
    m_Planes[SIDE_RIGHT].m_Normal.y = M(1, 3) - M(1, 0);
    m_Planes[SIDE_RIGHT].m_Normal.z = M(2, 3) - M(2, 0);
    m_Planes[SIDE_RIGHT].m_D = M(3, 3) - M(3, 0);

    // Top clipping plane
    m_Planes[SIDE_TOP].m_Normal.x = M(0, 3) - M(0, 1);
    m_Planes[SIDE_TOP].m_Normal.y = M(1, 3) - M(1, 1);
    m_Planes[SIDE_TOP].m_Normal.z = M(2, 3) - M(2, 1);
    m_Planes[SIDE_TOP].m_D = M(3, 3) - M(3, 1);

    // Bottom clipping plane
    m_Planes[SIDE_BOTTOM].m_Normal.x = M(0, 3) + M(0, 1);
    m_Planes[SIDE_BOTTOM].m_Normal.y = M(1, 3) + M(1, 1);
    m_Planes[SIDE_BOTTOM].m_Normal.z = M(2, 3) + M(2, 1);
    m_Planes[SIDE_BOTTOM].m_D = M(3, 3) + M(3, 1);

    // Near clipping plane
    m_Planes[SIDE_NEAR].m_Normal.x = M(0, 2);
    m_Planes[SIDE_NEAR].m_Normal.y = M(1, 2);
    m_Planes[SIDE_NEAR].m_Normal.z = M(2, 2);
    m_Planes[SIDE_NEAR].m_D = M(3, 2);

    // Far clipping plane
    m_Planes[SIDE_FAR].m_Normal.x = M(0, 3) - M(0, 2);
    m_Planes[SIDE_FAR].m_Normal.y = M(1, 3) - M(1, 2);
    m_Planes[SIDE_FAR].m_Normal.z = M(2, 3) - M(2, 2);
    m_Planes[SIDE_FAR].m_D = M(3, 3) - M(3, 2);

    // Normalize normals the normal norm
    for (long thePlaneIndex = 0; thePlaneIndex < SIDE_COUNT; ++thePlaneIndex)
        m_Planes[thePlaneIndex].Normalize();
}

//==============================================================================
//								ACCESS
//==============================================================================

//==============================================================================
/**
 *	Check the given box against this frustum to see if it's completely inside,
 *	partially inside or completely outside.  Being completely outside is a
 *	criterion for culling.  Being completely inside is also very good to know
 *	since there is no need to futher check containment of children as they
 *	too will all be inside.
 */
CFrustum::EContainment CFrustum::CheckContainment(const CBoundingBox &inBox) const
{
    EContainment theResult = CONTAINMENT_FULL;

    for (long thePlaneIndex = 0; theResult != CONTAINMENT_NONE && thePlaneIndex < SIDE_COUNT;
         ++thePlaneIndex)

        switch (m_Planes[thePlaneIndex].CheckRelation(inBox)) {
        // Return NONE immediately if the box is completely outside one of the planes
        case CPlane::RELATION_BACK:
            theResult = CONTAINMENT_NONE;
            break;

        // Flag as PARTIAL if the box clips any plane
        case CPlane::RELATION_CLIPPED:
            theResult = CONTAINMENT_PARTIAL;
            break;

        // Stay FULL only if all planes report front
        case CPlane::RELATION_FRONT:
        default:
            break;
        }

    return theResult;
}

} // namespace Q3DStudio
