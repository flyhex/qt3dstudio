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
#include "Vector2.h"
#include <math.h>

namespace Q3DStudio {

//==============================================================================
/**
*	Constructs the CVector2 object, setting its value to (x = 0.0f, y = 0.0f).
*/
CVector2::CVector2()
    : x(0.0)
    , y(0.0)
{
}

//==============================================================================
/**
*	Constructs the CVector2 object, setting its value to the X and Y passed in.
*/
CVector2::CVector2(float inX, float inY)
    : x(inX)
    , y(inY)
{
}

//==============================================================================
/**
*	Constructs a copy of the CVector2 object.
*
*	@param	inVector	The source for the copy operation.
*/
CVector2::CVector2(const CVector2 &inVector)
{
    x = inVector.x;
    y = inVector.y;
}

//==============================================================================
/**
 *	Sets the x,y for the current vector given a vector.
 *
 *	@param inVector The source vector
 */
void CVector2::Set(const CVector2 &inVector)
{
    x = inVector.x;
    y = inVector.y;
}

//==============================================================================
/**
 *	Gets the x and y components for the current vector.
 *
 *	@param outX The x component of the vector.
 *	@param outY The y component of the vector.
 */
void CVector2::Get(float &outX, float &outY) const
{
    outX = x;
    outY = y;
}

//==============================================================================
/**
 *	Sets the x and y components for the current vector.
 *
 *	@param inX The x component of the vector.
 *	@param inY The y component of the vector.
 */
void CVector2::Set(float inX, float inY)
{
    x = inX;
    y = inY;
}

//==============================================================================
/**
 *	Equals operator for the Vector2 object.
 *
 *	@return		The new vector after assignment.
 */
CVector2 &CVector2::operator=(const CVector2 &inVector)
{
    if (&inVector != this) {
        x = inVector.x;
        y = inVector.y;
    }

    return *this;
}

//==============================================================================
/**
 *	Tests to see if two vectors are equal.
 *
 *	@return		true if vectors are equal, otherwise false.
 */
bool CVector2::operator==(const CVector2 &inVector) const
{
    return (x == inVector.x && y == inVector.y);
}

//==============================================================================
/**
 *	Tests to see if two vectors are NOT equal.
 *
 *	@return		true if vectors are NOT equal, otherwise false.
 */
bool CVector2::operator!=(const CVector2 &inVector) const
{
    return (x != inVector.x || y != inVector.y);
}

//==============================================================================
/**
 *	Computes the sum of two vectors.
 *
 *	@param inVector2 The vector to be added to the current vector.
 *
 *	@return		The sum vector.
 */
CVector2 CVector2::operator+(const CVector2 &inVector) const
{
    CVector2 theSum;

    theSum.x = x + inVector.x;
    theSum.y = y + inVector.y;

    return theSum;
}

//==============================================================================
/**
 *	Subtracts the given vector from the current vector.
 *
 *	@param inVector2 The vector to be subtracted from the current vector.
 *
 *	@return		The difference vector.
 */
CVector2 CVector2::operator-(const CVector2 &inVector) const
{
    CVector2 theDiff;

    theDiff.x = x - inVector.x;
    theDiff.y = y - inVector.y;

    return theDiff;
}

//==============================================================================
/**
 *	Multiplies by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *
 *	@return		The scaled vector.
 */
CVector2 CVector2::operator*(float inScalar) const
{
    CVector2 theProduct;

    theProduct.x = inScalar * x;
    theProduct.y = inScalar * y;

    return theProduct;
}

//==============================================================================
/**
 *	Divides by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *
 *	@return		The scaled vector.
 */
CVector2 CVector2::operator/(float inScalar) const
{
    CVector2 theDivide;
    float theInverse = static_cast<float>(1.0 / inScalar);

    theDivide.x = theInverse * x;
    theDivide.y = theInverse * y;

    return theDivide;
}

//==============================================================================
/**
 *	Adds the current vector to this vector.
 *
 *	@param inVector2 The vector to be added to the current vector.
 *
 *	@return		The sum vector.
 */
CVector2 CVector2::operator+=(const CVector2 &inVector)
{
    x += inVector.x;
    y += inVector.y;

    return *this;
}

//==============================================================================
/**
 *	Subtracts the current vector from the this vector.
 *
 *	@param inVector2 The vector to be subtracted from the current vector.
 *
 *	@return		The difference vector.
 */
CVector2 CVector2::operator-=(const CVector2 &inVector)
{
    x -= inVector.x;
    y -= inVector.y;

    return *this;
}

//==============================================================================
/**
 *	Multiplies this vector by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *
 *	@return		The scaled vector.
 */
CVector2 CVector2::operator*=(float inScalar)
{
    x *= inScalar;
    y *= inScalar;

    return *this;
}

//==============================================================================
/**
 *	Divides this vector by the scalar.
 *
 *	@param inScalar The scalar to use on the vector.
 *
 *	@return		The scaled vector.
 */
CVector2 CVector2::operator/=(float inScalar)
{
    float theInverse = static_cast<float>(1.0 / inScalar);

    x *= theInverse;
    y *= theInverse;

    return *this;
}

//==============================================================================
/**
 *	Calculates the length (magnitude) of the current vector.
 *
 *	@param outLength The magnitude of the vector
 *
 *	@return	The length of the vector.
 */
float CVector2::Length()
{
    return static_cast<float>(::sqrt(static_cast<double>(x * x + y * y)));
}

//==============================================================================
/**
 *	Calculates the squared length (squared magnitude) of the current vector.
 *
 *	@param outLengthSquared The squared magnitude of the vector
 */
float CVector2::LengthSquared()
{
    return x * x + y * y;
}

//==============================================================================
/**
 *	Normalizes the current vector making it a unit vector.
 *
 *	The normalized vector is defined to be:
 *
 *	V norm = V / Magnitude(V)
 */
void CVector2::Normalize()
{
    float theVectorLength = Length();

    // TODO change this to epsilon or something.
    if (theVectorLength > 1e-06) {
        float theInvLength = static_cast<float>(1.0 / theVectorLength);
        x *= theInvLength;
        y *= theInvLength;
    }
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the minimum between
 *	the given vector and this vector.
 *
 *	@param inVector The vector that whose elements are tested against the
 *	current vector to build the minimum.
 */
void CVector2::Minimize(const CVector2 &inVector)
{
    if (inVector.x < x) {
        x = inVector.x;
    }
    if (inVector.y < y) {
        y = inVector.y;
    }
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the maximum between
 *	the given vector and this vector.
 *
 *	@param inVector The vector that whose elements are tested against the
 *	current vector to build the maximum.
 */
void CVector2::Maximize(const CVector2 &inVector)
{
    if (inVector.x > x) {
        x = inVector.x;
    }
    if (inVector.y > y) {
        y = inVector.y;
    }
}

//==============================================================================
/**
 *	Calculates the dot product of the this vector and the vector supplied as
 *	an argument.
 *
 *	@param inVector2	The vector that is used to calculate the dot product with
 *	the current vector.
 *
 *	@return The result of the calculation.
 */
float CVector2::DotProduct(const CVector2 &inVector)
{
    return x * inVector.x + y * inVector.y;
}

//==============================================================================
/**
 *	Transforms the current vector into a string
 *
 *	This method takes the current values of the Vector and returns them
 *	in a string in the format X Y Z
 */
Q3DStudio::CString CVector2::toString()
{
    Q3DStudio::CString theStringBuffer;
    theStringBuffer.Format(_LSTR("%.6f %.6f"), x, y);
    return theStringBuffer;
}

//==============================================================================
/**
 *	Takes the values of the string and sets the vector to those	values.
 *
 *	This method takes the values of the string and stores them in this
 *	vector.  The string must be in the format of X Y Z
 */
void CVector2::fromString(const Q3DStudio::CString &inStringValue)
{
    ::sscanf(inStringValue.GetCharStar(), "%f %f", &x, &y);
}

} // namespace Q3DStudio
