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

//==============================================================================
//	Includes
//==============================================================================
#include "stdafx.h"

#include <stdio.h>
#include <math.h>

#include "Vector3.h"
#include "Rotation3.h"
#include "Matrix.h"

namespace Q3DStudio {

IMPLEMENT_OBJECT_COUNTER(CVector3)

//==============================================================================
/**
*	Constructs the CVector3 object, setting its value to (x = 0.0f, y = 0.0f).
*/
CVector3::CVector3()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
{
    ADDTO_OBJECT_COUNTER(CVector3)
}

//==============================================================================
/**
*	Constructs the CVector3 object, setting its value to the X, Y, Z passed in.
*/
CVector3::CVector3(float inX, float inY, float inZ)
    : x(inX)
    , y(inY)
    , z(inZ)
{
    ADDTO_OBJECT_COUNTER(CVector3)
}

//==============================================================================
/**
 *	Constructs a copy of the CRotation3 object.
 *
 *	@param	inRotation	The source for the copy operation.
 */
CVector3::CVector3(const CRotation3 &inRotation)
{
    ADDTO_OBJECT_COUNTER(CVector3)
    inRotation.Get(x, y, z);
}

//==============================================================================
/**
 *	Constructs a copy of the CVector3 object.
 *
 *	@param	inVector	The source for the copy operation.
 */
CVector3::CVector3(const CVector3 &inVector)
    : x(inVector.x)
    , y(inVector.y)
    , z(inVector.z)
{
    ADDTO_OBJECT_COUNTER(CVector3)
}

CVector3::~CVector3()
{
    REMOVEFROM_OBJECT_COUNTER(CVector3)
}

//==============================================================================
/**
 *	Equals operator for the Vector3 object.
 *
 *	@return		The new vector after assignment.
 */
CVector3 &CVector3::operator=(const CVector3 &inVector)
{
    if (&inVector != this) {
        x = inVector.x;
        y = inVector.y;
        z = inVector.z;
    }

    return *this;
}

//==============================================================================
/**
 *	Returns either the x,y, or z  position of the vector.
 *
 *	@param		inIndex 	The index into the vector array.
 *
 *	@return		A reference to either the x,y, or z portion of the vector.
 */
float CVector3::operator[](long inIndex) const
{
    // Check boundries
    if (!CheckBounds(inIndex)) {
        // Throw exception
    }

    float theReturn;
    switch (inIndex) {
    case 0:
        theReturn = x;
        break;

    case 1:
        theReturn = y;
        break;

    case 2:
        theReturn = z;
        break;

    default:
        theReturn = 12345;
        // Throw exception
        break;
    }

    return theReturn;
}

//==============================================================================
/**
 *	Tests to see if two vectors are equal.
 *
 *	@return		true if vectors are equal, otherwise false.
 */
bool CVector3::operator==(const CVector3 &inVector) const
{
    return (x == inVector.x && y == inVector.y && z == inVector.z);
}

//==============================================================================
/**
 *	Tests to see if two vectors are NOT equal.
 *
 *	@return		true if vectors are NOT equal, otherwise false.
 */
bool CVector3::operator!=(const CVector3 &inVector) const
{
    return (x != inVector.x || y != inVector.y || z != inVector.z);
}

//==============================================================================
/**
 *	Computes the sum of two vectors.
 *
 *	@param inVector3 The vector to be added to the current vector.
 *	@return		The sum vector.
 */
CVector3 CVector3::operator+(const CVector3 &inVector) const
{
    CVector3 theSum;

    theSum.x = x + inVector.x;
    theSum.y = y + inVector.y;
    theSum.z = z + inVector.z;

    return theSum;
}

//==============================================================================
/**
 *	Subtracts the given vector from the current vector.
 *
 *	@param inVector3 The vector to be subtracted from the current vector.
 *	@return		The difference vector.
 */
CVector3 CVector3::operator-(const CVector3 &inVector) const
{
    CVector3 theDiff;

    theDiff.x = x - inVector.x;
    theDiff.y = y - inVector.y;
    theDiff.z = z - inVector.z;

    return theDiff;
}

//==============================================================================
/**
 *	Multiplies by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *	@return		The scaled vector.
 */
CVector3 CVector3::operator*(float inScalar) const
{
    CVector3 theProduct;

    theProduct.x = inScalar * x;
    theProduct.y = inScalar * y;
    theProduct.z = inScalar * z;

    return theProduct;
}

//==============================================================================
/**
 *	Divides by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *	@return		The scaled vector.
 */
CVector3 CVector3::operator/(float inScalar) const
{
    CVector3 theDivide;
    float theInverse = static_cast<float>(1.0f / inScalar);

    theDivide.x = theInverse * x;
    theDivide.y = theInverse * y;
    theDivide.z = theInverse * z;

    return theDivide;
}

//==============================================================================
/**
 *	Adds the current vector to this vector.
 *
 *	@param inVector3 The vector to be added to the current vector.
 *	@return		The sum vector.
 */
CVector3 &CVector3::operator+=(const CVector3 &inVector)
{
    x += inVector.x;
    y += inVector.y;
    z += inVector.z;

    return *this;
}

//==============================================================================
/**
 *	Subtracts the current vector from the this vector.
 *
 *	@param inVector3 The vector to be subtracted from the current vector.
 *	@return		The difference vector.
 */
CVector3 &CVector3::operator-=(const CVector3 &inVector)
{
    x -= inVector.x;
    y -= inVector.y;
    z -= inVector.z;

    return *this;
}

//==============================================================================
/**
 *	Multiplies this vector by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *	@return		The scaled vector.
 */
CVector3 &CVector3::operator*=(float inScalar)
{
    x *= inScalar;
    y *= inScalar;
    z *= inScalar;

    return *this;
}

//==============================================================================
/**
 *	Divides this vector by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *	@return		The scaled vector.
 */
CVector3 &CVector3::operator/=(float inScalar)
{
    float theInverse = static_cast<float>(1.0f / inScalar);

    x *= theInverse;
    y *= theInverse;
    z *= theInverse;

    return *this;
}

//==============================================================================
/**
 *	Sets the x and y components for the current vector.
 *
 *	@param inX The x component of the vector.
 *	@param inY The y component of the vector.
 *	@param inZ The Z component of the vector.
 */
void CVector3::Set(const float &inX, const float &inY, const float &inZ)
{
    x = inX;
    y = inY;
    z = inZ;
}

//==============================================================================
/**
 *	Sets the x,y and z for the current vector given a vector.
 *
 *	@param inVector The source vector
 */
void CVector3::Set(const CVector3 &inVector)
{
    x = inVector.x;
    y = inVector.y;
    z = inVector.z;
}

//==============================================================================
/**
 *	Gets the x and y components for the current vector.
 *
 *	@param outX The x component of the vector.
 *	@param outY The y component of the vector.
 *	@param outZ The z component of the vector.
 */
void CVector3::Get(float &outX, float &outY, float &outZ) const
{
    outX = x;
    outY = y;
    outZ = z;
}

//==============================================================================
/**
 *	Calculates the length (magnitude) of the current vector.
 *
 *	@param outLength The magnitude of the vector
 *	@return	The length of the vector.
 */
float CVector3::Length() const
{
    return static_cast<float>(::sqrt(static_cast<double>(x * x + y * y + z * z)));
}

//==============================================================================
/**
 *	Calculates the squared length (squared magnitude) of the current vector.
 *
 *	@param outLengthSquared The squared magnitude of the vector
 */
float CVector3::LengthSquared() const
{
    return x * x + y * y + z * z;
}

//==============================================================================
/**
 *	Normalizes the current vector making it a unit vector.
 *
 *	The normalized vector is defined to be:
 *	V norm = V / Magnitude(V)
 */
CVector3 &CVector3::Normalize()
{
    float theVectorLength = Length();

    // TODO change this to epsilon or something.
    if (theVectorLength > 1e-06) {
        float theInvLength = static_cast<float>(1.0 / theVectorLength);
        x *= theInvLength;
        y *= theInvLength;
        z *= theInvLength;
    }

    return *this;
}

//==============================================================================
/**
 *	Calculates the dot product of the this vector and the vector supplied as
 *	an argument.
 *
 *	@param inVector3	The vector that is used to calculate the dot product with
 *	the current vector.
 *
 *	@return The result of the calculation.
 */
float CVector3::DotProduct(const CVector3 &inVector) const
{
    return x * inVector.x + y * inVector.y + z * inVector.z;
}

//==============================================================================
/**
 *	Returns the cross product of this vector and the vector passed in.
 *
 *	@param inVector3 The vector that is used to calculate the cross product
 *				with the current vector.
 *	@return  result cross product vector
 */
CVector3 CVector3::CrossProduct(const CVector3 &inVector) const
{
    CVector3 theCrossProduct;

    theCrossProduct.x = y * inVector.z - z * inVector.y;
    theCrossProduct.y = z * inVector.x - x * inVector.z;
    theCrossProduct.z = x * inVector.y - y * inVector.x;

    return theCrossProduct;
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the minimum between
 *	the given vector and this vector.
 *
 *	@param inVector3 The vector that whose elements are tested against the
 *				current vector to build the minimum.
 */
CVector3 &CVector3::Minimize(const CVector3 &inVector)
{
    if (inVector.x < x)
        x = inVector.x;

    if (inVector.y < y)
        y = inVector.y;

    if (inVector.z < z)
        z = inVector.z;

    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the maximum between
 *	the given vector and this vector.
 *
 *	@param inVector3 The vector that whose elements are tested against the
 *				current vector to build the maximum.
 */
CVector3 &CVector3::Maximize(const CVector3 &inVector)
{
    if (inVector.x > x)
        x = inVector.x;

    if (inVector.y > y)
        y = inVector.y;

    if (inVector.z > z)
        z = inVector.z;

    return *this;
}

//==============================================================================
/**
 *	Validates the index being passed in does not overstep the vertex array bounds.
 *
 *	@param	inIndex	The index into the vector
 *	@return	true if within range, otherwise false
 */
bool CVector3::CheckBounds(long inIndex) const
{
    return (inIndex <= 3 && inIndex >= 0);
}

//==============================================================================
/**
 *	Transforms this vector by the given matrix.
 *
 *	The martrix is row column of the form (Matrix[row][column]):
 *
 *			| m0	m1	m2	w |
 *			| m4	m5	m6	w |
 *			| m8	m9	m10	w |
 *			| tX	tY	tZ	w |
 *
 *	@inMatrix	the transform matrix
 *	@inTranslate is false if you only want to rotate the vector
 *	@return		this matrix
 */
CVector3 &CVector3::Transform(const CMatrix &inMatrix, bool inTranslate)
{
    float theX = x;
    float theY = y;
    float theZ = z;
    float theW = inTranslate ? 1.0f : 0;

    x = theX * inMatrix.m[0][0] + theY * inMatrix.m[1][0] + theZ * inMatrix.m[2][0]
        + theW * inMatrix.m[3][0];
    y = theX * inMatrix.m[0][1] + theY * inMatrix.m[1][1] + theZ * inMatrix.m[2][1]
        + theW * inMatrix.m[3][1];
    z = theX * inMatrix.m[0][2] + theY * inMatrix.m[1][2] + theZ * inMatrix.m[2][2]
        + theW * inMatrix.m[3][2];

    return *this;
}

//==============================================================================
/**
 *	Calculates the distance between this vector and another.
 *
 *	@param	inVector3 the vector to which the distance is being calculated.
 *	@return	distance between vectors
 */
float CVector3::Distance(const CVector3 &inVector) const
{
    float theDx = x - inVector.x;
    float theDy = y - inVector.y;
    float theDz = z - inVector.z;
    return static_cast<float>(sqrt(theDx * theDx + theDy * theDy + theDz * theDz));
}

//==============================================================================
/**
 *	Calculates the squared distance between this vector and another.
 *
 *	@param	inVector the vector to which the distance is being calculated.
 *	@return	squared distance between vectors
 */
float CVector3::DistanceSquared(const CVector3 &inVector) const
{
    float theDx = x - inVector.x;
    float theDy = y - inVector.y;
    float theDz = z - inVector.z;
    return theDx * theDx + theDy * theDy + theDz * theDz;
}

//==============================================================================
/**
 *	Modifies the current vector by performing a linear interpolation between the
 *	current vector and the given vector.
 *
 *	The interpolation is created using the following formula:
 *
 *	Vnew = Vorig + inInterpParam( inVector2 - Vorig )
 *
 *	@param	inVector The vector that acts as the second vector for the interpolation.
 *	@return	this interpolated vector.
 */
CVector3 &CVector3::InterpolateLinear(const CVector3 &inVector, float inInterpParam)
{
    CVector3 theTempVector1(inVector * inInterpParam);
    CVector3 theTempVector2(*this * inInterpParam);

    *this += theTempVector1 - theTempVector2;

    return *this;
}

} // namespace Q3DStudio
