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
#include "Plane.h"

namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CMatrix;
class CBoundingBox;

//==============================================================================
/**
 *	@class CFrustum
 *	@brief Geometrical primitive aiding the culling of visible objects.
 */
class CFrustum
{
    //==============================================================================
    //	Enumerations
    //==============================================================================
public:
    enum { SIDE_TOP = 0, SIDE_BOTTOM, SIDE_LEFT, SIDE_RIGHT, SIDE_FAR, SIDE_NEAR, SIDE_COUNT };

    enum EContainment {
        CONTAINMENT_FULL, ///< Volume is completely inside the frustum
        CONTAINMENT_PARTIAL, ///< Vaolume is partially inside the frustum
        CONTAINMENT_NONE ///< No part of the volume is inside the frustum
    };

    //==============================================================================
    //	Fields
    //==============================================================================
private:
    CPlane m_Planes[SIDE_COUNT];

    //==============================================================================
    //	Methods
    //==============================================================================

public: // Construction
    CFrustum();
    void Set(const CMatrix &inProjection);

public: // Access
    EContainment CheckContainment(const CBoundingBox &inBox) const;
};
} // namespace Q3DStudio
