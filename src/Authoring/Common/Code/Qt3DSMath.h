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
#ifndef __QT3DS_MATH_H__
#define __QT3DS_MATH_H__

//==============================================================================
//	Includes
//==============================================================================
#include <math.h>
#include <memory.h>

#ifdef _WIN32
// Generate these functions as inline code, not as function calls:
#pragma intrinsic(sin, cos, sqrt, memcpy, memset, acos, fabs, abs)
#endif

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Template Macros
//==============================================================================
template <class T>
inline const T &MIN(const T &inA, const T &inB)
{
    return inA < inB ? inA : inB;
}
template <class T>
inline const T &MAX(const T &inA, const T &inB)
{
    return inA > inB ? inA : inB;
}
template <class T>
inline const T &MIN3(const T &inA, const T &inB, const T &inC)
{
    return inA < inB ? MIN(inA, inC) : MIN(inB, inC);
}
template <class T>
inline const T &MAX3(const T &inA, const T &inB, const T &inC)
{
    return inA > inB ? MAX(inA, inC) : MAX(inB, inC);
}
template <class T>
inline const T CLAMP(const T &inVal, const T &inMin, const T &inMax)
{
    if (inVal <= inMin)
        return inMin;
    else if (inVal >= inMax)
        return inMax;
    return inVal;
}
template <class T>
inline void MINMAX(const T &inA, const T &inB, T &outMin, T &outMax)
{
    if (inA < inB) {
        outMin = inA;
        outMax = inB;
    } else {
        outMin = inB;
        outMax = inA;
    }
}

//==============================================================================
//	Constants
//==============================================================================
extern const float PI; /// The venerable PI
extern const float EPSILON; /// Maximum allowed error in float comparisons
extern const float SQREPSILON; /// EPSILON squared is often used in distance computations
extern const float
    DTOR; /// Degrees to Radians - multiply with this factor to transform from "D to R"
extern const float
    RTOD; /// Radians to Degrees - multiply with this factor to transform from "R to D"

//==============================================================================
//	Forwards
//==============================================================================
class CVector3;
class CEulerAngles;
class CAxisAngle;
class CQuaternion;
class CMatrix;

#ifdef MATTIAS_OFF
//==============================================================================
/**
 *	@class	CVector3
 *	@brief	Fast and minimal implementation of a 3D vector.
 *
 *	This implementation of a 3D vector along with  all the sibling and helper
 *	classes in the geometry section, try to strike a balance between clean code,
 *	ease-of-use and speed.  Fields are public and most methods are inlined to
 *	give the impression of a very intelligent and capable structure.  Heavier
 *	code such as intersections is encouraged to be added externally instead
 *	of here.
 *
 *	The vector is a member of the geometry classes next to CMatrix, CRotation
 *	and CQuaternion.
 *	@see	CMatrix
 *	@see	CRotation
 *	@see	CQuaternion
 */
class CVector3
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    float x; ///< X component of vector
    float y; ///< Y component of vector
    float z; ///< Z component of vector

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    inline CVector3();
    inline CVector3(const float inX, const float inY, const float inZ);
    inline CVector3(const float inComponents[]);
    inline CVector3(const CVector3 &inVector);

public: // Initialization
    inline CVector3 &Set(const float inX, const float inY, const float inZ);
    inline CVector3 &Set(const float inComponents[]);
    inline CVector3 &Set(const CVector3 &inVector);

public: // Functions
    inline float DistanceSquared(const CVector3 &inVector) const;
    inline float Distance(const CVector3 &inVector) const;
    inline float LengthSquared() const;
    inline float Length() const;
    inline float DotProduct(const CVector3 &inVector) const;
    inline CVector3 CrossProduct(const CVector3 &inVector);
    inline CVector3 &Normalize();
    inline CVector3 &Minimize(const CVector3 &inVector);
    inline CVector3 &Maximize(const CVector3 &inVector);
    inline CVector3 &InterpolateLinear(const CVector3 &inDestVector, float inFactor);
    inline CVector3 &Transform(const CMatrix &inMatrix, bool inTranslate = true);

public: // Operators
    inline bool operator==(const CVector3 &inVector) const;
    inline bool operator!=(const CVector3 &inVector) const;
    inline CVector3 operator+(const CVector3 &inVector) const;
    inline CVector3 operator-(const CVector3 &inVector) const;
    inline CVector3 operator*(float inFactor) const;
    inline float operator*(const CVector3 &inVector) const;
    inline CVector3 operator/(float inDivisor) const;
    inline CVector3 operator%(const CVector3 &inVector) const;
    inline CVector3 &operator=(const CVector3 &inVector);
    inline CVector3 &operator+=(const CVector3 &inVector);
    inline CVector3 &operator-=(const CVector3 &inVector);
    inline CVector3 &operator*=(float inFactor);
    inline CVector3 &operator/=(float inDivisor);
    inline CVector3 &operator%=(const CVector3 &inVector);
    inline CVector3 operator-() const;
    inline float &operator[](int inIndex);
    inline const float &operator[](int inIndex) const;
};

//==============================================================================
/**
 *	@class CEulerAngles
 *	@brief Traditional yaw, pitch, roll representation of rotation.
 *	Quaternion can use this class to access and mutate it's state old-skool.
 *	All angles are in radians.
 *	@see CQuaternion
 */
class CEulerAngles
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    enum EEulerDefs {
        X = 0, ///< Start with X axis
        Y = 1, ///< Start with Y axis
        Z = 2, ///< Start with Z axis

        STATICFRAME = 0, ///< Keep axis globally oriented
        ROTATEFRAME = 1, ///< Move axis with each operation

        NOREPEAT = 0, ///< Use all three axis
        REPEAT = 1, ///< Repeat first axis as last operation

        EVEN = 0, ///< Normal XYZ or shifted (XYZ, YZX or ZXY)
        ODD = 1, ///< Swap two last axis (XZY, YXZ or ZYX)
    };

#define GETEULERORDER(i, p, r, f) ((((((i << 1) + p) << 1) + r) << 1) + f)

    enum EOrder {
        XYZs = GETEULERORDER(X, EVEN, NOREPEAT, STATICFRAME),
        XYXs = GETEULERORDER(X, EVEN, REPEAT, STATICFRAME),
        XZYs = GETEULERORDER(X, ODD, NOREPEAT, STATICFRAME),
        XZXs = GETEULERORDER(X, ODD, REPEAT, STATICFRAME),
        YZXs = GETEULERORDER(Y, EVEN, NOREPEAT, STATICFRAME),
        YZYs = GETEULERORDER(Y, EVEN, REPEAT, STATICFRAME),
        YXZs = GETEULERORDER(Y, ODD, NOREPEAT, STATICFRAME),
        YXYs = GETEULERORDER(Y, ODD, REPEAT, STATICFRAME),
        ZXYs = GETEULERORDER(Z, EVEN, NOREPEAT, STATICFRAME),
        ZXZs = GETEULERORDER(Z, EVEN, REPEAT, STATICFRAME),
        ZYXs = GETEULERORDER(Z, ODD, NOREPEAT, STATICFRAME),
        ZYZs = GETEULERORDER(Z, ODD, REPEAT, STATICFRAME),

        ZYXr = GETEULERORDER(X, EVEN, NOREPEAT, ROTATEFRAME),
        XYXr = GETEULERORDER(X, EVEN, REPEAT, ROTATEFRAME),
        YZXr = GETEULERORDER(X, ODD, NOREPEAT, ROTATEFRAME),
        XZXr = GETEULERORDER(X, ODD, REPEAT, ROTATEFRAME),
        XZYr = GETEULERORDER(Y, EVEN, NOREPEAT, ROTATEFRAME),
        YZYr = GETEULERORDER(Y, EVEN, REPEAT, ROTATEFRAME),
        ZXYr = GETEULERORDER(Y, ODD, NOREPEAT, ROTATEFRAME),
        YXYr = GETEULERORDER(Y, ODD, REPEAT, ROTATEFRAME),
        YXZr = GETEULERORDER(Z, EVEN, NOREPEAT, ROTATEFRAME),
        ZXZr = GETEULERORDER(Z, EVEN, REPEAT, ROTATEFRAME),
        XYZr = GETEULERORDER(Z, ODD, NOREPEAT, ROTATEFRAME),
        ZYZr = GETEULERORDER(Z, ODD, REPEAT, ROTATEFRAME)
    };

    CVector3 m_Angle;
    long m_Order;

    //==============================================================================
    //	Methods
    //==============================================================================
    CEulerAngles(const CVector3 &inAngle, long inOrder = YXZr)
        : m_Angle(inAngle)
        , m_Order(inOrder)
    {
    }
};

//==============================================================================
/**
 *	@class CAxisAngle
 *	@brief LookAt vector with roll component.
 *	Quaternion can use this class to access and mutate it's state.  All angles
 *	are in radians.
 *	@see CQuaternion
 */
class CAxisAngle
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    CVector3 m_Axis; ///< The LookAt vector
    float m_Angle; ///< Roll of LookAt vector in radians

    //==============================================================================
    //	Methods
    //==============================================================================
    CAxisAngle(const CVector3 &inAxis, float inAngle)
        : m_Axis(inAxis)
        , m_Angle(inAngle)
    {
    }
};

//==============================================================================
/**
 *	@class CQuaternion
 *	@brief Smart way to represent rotation in 3-space.
 *	@see CAxisAngle
 *	@see CEulerAngles
*/
class CQuaternion
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    float qx; ///< X portion of quaternion - Not related to x coordinate in any way
    float qy; ///< Y portion of quaternion - Not related to y coordinate in any way
    float qz; ///< Z portion of quaternion - Not related to z coordinate in any way
    float qw; ///< W portion of quaternion

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    inline CQuaternion();
    inline CQuaternion(const float inX, const float inY, const float inZ, const float inW);
    inline CQuaternion(const CQuaternion &inQuat);
    inline CQuaternion(const CMatrix &inMatrix);
    inline CQuaternion(const CAxisAngle &inAxisAngle);
    inline CQuaternion(const CEulerAngles &inAngles);

public: // Initialization
    inline CQuaternion &Set(const float inX, const float inY, const float inZ, const float inW);
    inline CQuaternion &Reset();

public: // Functions
    inline CQuaternion &Normalize();
    inline CQuaternion &Invert();
    inline float DotProduct(const CQuaternion &inQuat) const;
    inline CQuaternion Slerp(const CQuaternion &inQuatA, const CQuaternion &inQuatB,
                             float inFactor);

public: // Converters
    inline CMatrix &ToMatrix(CMatrix &outMatrix) const;
    inline CAxisAngle &ToAxisAngle(CAxisAngle &outAxisAngle) const;
    CEulerAngles ToEulerAngles(long inOrder = CEulerAngles::XYZs) const;

public: // Operators
    inline CQuaternion &operator=(const CQuaternion &inQuat);
    inline CQuaternion &operator=(const CMatrix &inMatrix);
    inline CQuaternion &operator=(const CAxisAngle &inAxisAngle);
    CQuaternion &operator=(const CEulerAngles &inEulerAngles);
    inline bool operator==(const CQuaternion &inQuat) const;
    inline bool operator!=(const CQuaternion &inQuat) const;
    inline CQuaternion operator+(const CQuaternion &inQuat) const;
    inline CQuaternion &operator+=(const CQuaternion &inQuat);
    inline CQuaternion operator*(const CQuaternion &inQuat) const;
    inline CQuaternion operator*(float inFactor) const;
    inline CQuaternion &operator*=(const CQuaternion &inQuat);
    inline CQuaternion &operator*=(float inFactor);

    inline CVector3 operator*(const CVector3 &inVector) const;
};

//==============================================================================
/**
 *	@class CMatrix
 *	@brief A row major matrix for 3D transformation.
 *	Matrix[row][column]
 *<code>
 *
 *		(see code for correct formatting of matrices)
 *	| 0 1 2 3 |  or  | 00 01 02 03 |  or  | Xx Xy Xz w |	<br>
 *	| 4 5 6 7 |      | 10 11 12 13 |      | Yx Yy Yz w |	<br>
 *	| 8 9 A B |      | 20 21 22 23 |      | Zx Zy Zz w |	<br>
 *	| C D E F |      | 30 31 32 33 |      | Tx Ty Tz w |	<br>
 *															</code>
 *	Rotations are concatenated in the the following order: YXZ
 *	All rotations are clockwise when viewed down the positive axis
 *	towards the origin.
 */
class CMatrix
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    float m[4][4]; ///< 16 elements in a 4 x 4

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    inline CMatrix();
    inline CMatrix(const CMatrix &inMatrix);
    inline CMatrix(const float inComponents[]);

public: // Initialization
    inline CMatrix &Zero();
    inline CMatrix &Identity();

    inline CMatrix &Set(const CMatrix &inMatrix);
    inline CMatrix &Set(const float inComponents[]);
    inline CMatrix &Set(const CVector3 &inTranslation, const CEulerAngles &inRotation,
                        const CVector3 &inScale, const CVector3 &inPivot);

    inline CMatrix &SetTranslate(const CVector3 &inTranslate);
    inline CMatrix &SetRotate(const CQuaternion &inRotation);
    inline CMatrix &SetScale(const CVector3 &inScale);
    inline CMatrix &SetFrustum(const bool inOrthographic, const float inNear, const float inFar,
                               const float inLeft, const float inTop, const float inRight,
                               const float inBottom);

public: // Functions
    inline bool IsIdentity() const;
    inline bool IsAffine() const;

    inline CMatrix &Translate(const CVector3 &inTranslate);
    inline CMatrix &Rotate(const CQuaternion &inRotation);
    inline CMatrix &Scale(const CVector3 &inScale);
    inline CMatrix &Transpose();
    inline float Invert();
    inline CMatrix &MultiplyAffine(const CMatrix &inMatrix);

public: // Operators
    inline CMatrix &operator=(const CMatrix &inMatrix);
    inline bool operator==(const CMatrix &inMatrix) const;
    inline bool operator!=(const CMatrix &inMatrix) const;
    inline float *operator[](int inRow);
    inline const float *operator[](int inRow) const;

public: // Static common matrices
    static const float s_Identity[16]; ///< Enabling fast initialization from static identity matrix
};

// Stream operators for easy debugging
// std::ostream &operator<<( std::ostream& inStream, const CVector3& inVector );
// std::ostream &operator<<( std::ostream& inStream, const CQuaternion& inQuat );
// std::ostream &operator<<( std::ostream& inStream, const CMatrix& inMatrix );

// Implementation
#include "Vector3.inl"
#include "Quaternion.inl"
#include "Matrix.inl"

#endif // MATTIAS_OFF

} // namespace Q3DStudio

#endif //__QT3DS_MATH_H_
