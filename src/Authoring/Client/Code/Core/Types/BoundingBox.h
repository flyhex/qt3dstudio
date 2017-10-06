/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#include "Matrix.h"
#include "Vector3.h"

namespace Q3DStudio {

class CBoundingBox
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    CVector3 m_MinCorner; ///< smallest x,y,z corner
    CVector3 m_MaxCorner; ///< biggest x,y,z corner
    bool m_Orthographic; ///< FIX:should be moved outside of the class?
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CBoundingBox();
    CBoundingBox(const CVector3 &inCorner1, const CVector3 &inCorner2);
    CBoundingBox(const CBoundingBox &inBox);

public: // Access
    bool IsEmpty() const;
    const CVector3 &MaxPoint() const;
    const CVector3 &MinPoint() const;
    void GetCorners(CVector3 outCorners[8]) const;

public: // Operations
    void Clear();
    CBoundingBox &Expand(const CBoundingBox &inBox);
    CBoundingBox &Expand(const CVector3 &inVertex);
    CBoundingBox &Transform(const CMatrix &inTransform);

public: // Orthographic methods
    bool GetOrthographic() const;
    void SetOrthographic(const bool &inOrthographic = true);
};
} // namespace Q3DStudio
