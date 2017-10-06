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

#ifndef __VECTOR3_H_
#define __VECTOR3_H_

#include "UICObjectCounter.h"

namespace Q3DStudio {

class CMatrix;
class CRotation3;

class CVector3
{
    // public field members for fast access (use unions to provide two names for one variable)
public:
    float x; ///< X portion of vector
    float y; ///< Y portion of vector
    float z; ///< Z portion of vector

    DEFINE_OBJECT_COUNTER(CVector3)

    // Construction
public:
    CVector3();
    CVector3(float inX, float inY, float inZ);
    CVector3(const CRotation3 &inRotation);
    CVector3(const CVector3 &inVector);
    ~CVector3();
    // Access
public:
    void Get(float &outX, float &outY, float &inZ) const;
    void Set(const float &inX, const float &inY, const float &inZ);
    void Set(const CVector3 &inVector);
    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetZ() const { return z; }
    void SetX(const float &inX) { x = inX; }
    void SetY(const float &inY) { y = inY; }
    void SetZ(const float &inZ) { z = inZ; }

    // Operators
public:
    float operator[](long inIndex) const;
    bool operator==(const CVector3 &inVector) const;
    bool operator!=(const CVector3 &inVector) const;
    CVector3 operator+(const CVector3 &inVector) const;
    CVector3 operator-(const CVector3 &inVector) const;
    CVector3 operator*(float inScalar) const;
    CVector3 operator/(float inScalar) const;
    CVector3 &operator=(const CVector3 &inVector);
    CVector3 &operator+=(const CVector3 &inVector);
    CVector3 &operator-=(const CVector3 &inVector);
    CVector3 &operator*=(float inScalar);
    CVector3 &operator/=(float inScalar);

    // Functions
public:
    float Distance(const CVector3 &inVector) const;
    float DistanceSquared(const CVector3 &inVector) const;
    float DotProduct(const CVector3 &inVector) const;
    float LengthSquared() const;
    float Length() const;
    CVector3 CrossProduct(const CVector3 &inVector) const;
    CVector3 &Normalize();
    CVector3 &Minimize(const CVector3 &inVector);
    CVector3 &Maximize(const CVector3 &inVector);
    CVector3 &InterpolateLinear(const CVector3 &inVector, float inInterpParam);
    CVector3 &Transform(const CMatrix &inMatrix, bool inTranslate = true);

    // Implementation
protected:
    bool CheckBounds(long inIndex) const;
};

} // namespace Q3DStudio

#endif //__VECTOR2_H_
