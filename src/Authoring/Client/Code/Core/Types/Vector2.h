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

#ifndef __VECTOR2_H_
#define __VECTOR2_H_

#include "Qt3DSString.h"

namespace Q3DStudio {
// TODO Remove these soon.
typedef long IMatrix;
typedef long IVector4;

class CVector2
{
    // Field members are public for obvious speed reasons.  Using an unnamed union, which allows
    // two names for one variable.
public:
    float x; ///< X portion of vector
    float y; ///< Y portion of vector

    // Construction
public:
    CVector2();
    CVector2(float inX, float inY);
    CVector2(const CVector2 &inVector);
    ~CVector2() {}

    // Access
public:
    void Set(const CVector2 &inVector);
    void Get(float &outX, float &outY) const;
    void Set(float inX, float inY);
    float GetX() const { return x; }
    float GetY() const { return y; }
    void SetX(float inX) { x = inX; }
    void SetY(float inY) { y = inY; }

    // Operators
public:
    CVector2 &operator=(const CVector2 &inVector);
    bool operator==(const CVector2 &inVector) const;
    bool operator!=(const CVector2 &inVector) const;
    CVector2 operator+(const CVector2 &inVector) const;
    CVector2 operator-(const CVector2 &inVector) const;
    CVector2 operator*(float inScalar) const;
    CVector2 operator/(float inScalar) const;
    CVector2 operator+=(const CVector2 &inVector);
    CVector2 operator-=(const CVector2 &inVector);
    CVector2 operator*=(float inScalar);
    CVector2 operator/=(float inScalar);

    // Functions
public:
    void Normalize();
    void Minimize(const CVector2 &inVector);
    void Maximize(const CVector2 &inVector);
    float DotProduct(const CVector2 &inVector);
    float LengthSquared();
    float Length();
    QString toString();
    void fromString(const Q3DStudio::CString &inStringValue);
};

} // namespace Q3DStudio

#endif //__VECTOR2_H_
