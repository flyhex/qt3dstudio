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

//==============================================================================
//								CONSTRUCTION
//==============================================================================

//==============================================================================
/**
 *	Simple constructor - creates a NULL vector.
 */

inline CVector3::CVector3()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
{
}

//==============================================================================
/**
 *	Construction from three coordinates.
 *	@param	inX is the x coordinate
 *	@param	inY is the y coordinate
 *	@param	inZ is the z coordinate
 */

inline CVector3::CVector3(const float inX, const float inY, const float inZ)
    : x(inX)
    , y(inY)
    , z(inZ)
{
}

//==============================================================================
/**
 *	Construction from three coordinates in an array.
 *	@param	inComponents is an array with three coordinates
 */

inline CVector3::CVector3(const float inComponents[])
    : x(inComponents[0])
    , y(inComponents[1])
    , z(inComponents[2])
{
}

//==============================================================================
/**
 *	Copy constructor
 *	@param	inVector is the source vector
 */

inline CVector3::CVector3(const CVector3 &inVector)
    : x(inVector.x)
    , y(inVector.y)
    , z(inVector.z)
{
}

//==============================================================================
//								INITIALIZATION
//==============================================================================

//==============================================================================
/**
 *	Assignment of coordinates.
 *	@param	inX is the x coordinate
 *	@param	inY is the y coordinate
 *	@param	inZ is the z coordinate
 *	@return a reference to this modified vector
 */

inline CVector3 &CVector3::Set(const float inX, const float inY, const float inZ)
{
    x = inX;
    y = inY;
    z = inZ;
    return *this;
}

//==============================================================================
/**
 *	Assignment of coordinates using an array.
 *	@param	inComponents is the coordinate array
 *	@return a reference to this modified vector
 */

inline CVector3 &CVector3::Set(const float inComponents[])
{
    x = inComponents[0];
    y = inComponents[1];
    z = inComponents[2];
    return *this;
}

//==============================================================================
/**
 *	Assignment of coordinates using another vector.
 *	@param	inVector is the vector to be copied.
 *	@return a reference to this modified vector
 */

inline CVector3 &CVector3::Set(const CVector3 &inVector)
{
    x = inVector.x;
    y = inVector.y;
    z = inVector.z;
    return *this;
}

//==============================================================================
//								OPERATORS
//==============================================================================

//==============================================================================
/**
 *	Returns either the x,y or z value of the vector.  Reference is read only.
 *	Fast implementation depends on the fields being packed next to each other.
 *	@param	inIndex is the index of the value.  X=0, Y=1, Z=2
 *	@return the requested coordinate
 */
inline const float &CVector3::operator[](int inIndex) const
{
    return *(&x + inIndex);
}

//==============================================================================
/**
 *	Returns either the x,y or z value of the vector.
 *	Reference is read/write in contrast to the const method above and can
 *	thus be used for modification of values such as theVector[1] = 1.0f
 *	Fast implementation depends on the fields being packed next to each other.
 *	@param	inIndex is the index of the value.  X=0, Y=1, Z=2
 *	@return the value of the requested component
*/
inline float &CVector3::operator[](int inIndex)
{
    return *(&x + inIndex);
}

//==============================================================================
/**
 *	Compare this vector with another. Exact match is not needed but instead
 *	an error of EPSILON is allowed.
 *	@param	inVector is the vector we are compared with
 *	@return	true if the vectors are close to identical
 *	@see	EPSILON
 */
inline bool CVector3::operator==(const CVector3 &inVector) const
{
    return (::abs(x - inVector.x) < EPSILON) && (::abs(y - inVector.y) < EPSILON)
        && (::abs(z - inVector.z) < EPSILON);
}

//==============================================================================
/**
 *	Compare this vector with another. Exact match is not needed but instead
 *	an error of EPSILON is allowed.
 *	@param	inVector is the vector we are compared with
 *	@return	true if the vectors are not even close to identical
 *	@see	EPSILON
 */
inline bool CVector3::operator!=(const CVector3 &inVector) const
{
    return (::abs(x - inVector.x) > EPSILON) || (::abs(y - inVector.y) > EPSILON)
        || (::abs(z - inVector.z) > EPSILON);
}

//==============================================================================
/**
 *	Add this vector with another but do not modify this vector.
 *	@param	inVector is the second vector being added
 *	@return a new vector
 */

inline CVector3 CVector3::operator+(const CVector3 &inVector) const
{
    return CVector3(x + inVector.x, y + inVector.y, z + inVector.z);
}

//==============================================================================
/**
 *	Add two vectors but do not modify this vector.
 *	@param	inVector is vector being subtracted
 *	@return a new vector
 */

inline CVector3 CVector3::operator-(const CVector3 &inVector) const
{
    return CVector3(x - inVector.x, y - inVector.y, z - inVector.z);
}

//==============================================================================
/**
 *	Get a scaled copy of this vector.
 *	@param	inFactor is the factor that scales each coordinate
 *	@return a new scaled vector
 */

inline CVector3 CVector3::operator*(float inFactor) const
{
    return CVector3(x * inFactor, y * inFactor, z * inFactor);
}

//==============================================================================
/**
 *	Perform a dot product.
 *	@param	inVector is the second vector of the dot product
 *	@return the dot product
 *	@see	DotProduct
 */
float CVector3::operator*(const CVector3 &inVector) const
{
    return x * inVector.x + y * inVector.y + z * inVector.z;
}

//==============================================================================
/**
 *	Get a scaled copy of this vector.
 *	@param	inDivisor is the divisor that scales each coordinate
 *	@return	a new scaled vector
 */
inline CVector3 CVector3::operator/(float inDivisor) const
{
    return CVector3(x / inDivisor, y / inDivisor, z / inDivisor);
}

//==============================================================================
/**
 *	Perform a cross product of two vectors.
 *	@param	inVector is the second vector of the cross product
 *	@return	the cross product vector
 *	@see	CrossProduct
 */
CVector3 CVector3::operator%(const CVector3 &inVector) const
{
    return CVector3(y * inVector.z - z * inVector.y, z * inVector.x - x * inVector.z,
                    x * inVector.y - y * inVector.x);
}

//==============================================================================
/**
 *	Invert the vector.
 *	@return	the inverted copy of this vector
 */
inline CVector3 CVector3::operator-() const
{
    return CVector3(-x, -y, -z);
}

//==============================================================================
/**
 *	Simple assignment.
 *	@param	inVector
 *	@return	a reference to this modified vector
 */
inline CVector3 &CVector3::operator=(const CVector3 &inVector)
{
    x = inVector.x;
    y = inVector.y;
    z = inVector.z;
    return *this;
}

//==============================================================================
/**
 *	Increment this vector.
 *	@param	inVector has the coordinates by which this textor will be incremented
 *	@return	a reference to this modified vector
 */
inline CVector3 &CVector3::operator+=(const CVector3 &inVector)
{
    x += inVector.x;
    y += inVector.y;
    z += inVector.z;
    return *this;
}

//==============================================================================
/**
 *	Decrement this vector.
 *	@param	inVector has the coordinates by which this textor will be decremented
 *	@return	a reference to this modified vector
 */
inline CVector3 &CVector3::operator-=(const CVector3 &inVector)
{
    x -= inVector.x;
    y -= inVector.y;
    z -= inVector.z;
    return *this;
}

//==============================================================================
/**
 *	Scale this vector by a factor.
 *	@param	inFactor is the scale factor
 *	@return	a reference to this modified vector
 */
inline CVector3 &CVector3::operator*=(float inFactor)
{
    x *= inFactor;
    y *= inFactor;
    z *= inFactor;
    return *this;
}

//==============================================================================
/**
 *	Scale this vector by a divisor.
 *	@param	inDivisor is the inverted scale factor
 *	@return	a reference to this modified vector
 */
inline CVector3 &CVector3::operator/=(float inDivisor)
{
    x /= inDivisor;
    y /= inDivisor;
    z /= inDivisor;
    return *this;
}

//==============================================================================
/**
 *	Perform a cross product and immedately store the result in this vector.
 *	@param	inVector is the second vector of the cross product
 *	@return	a reference to this modified vector
 *	@see	CrossProduct
 */
CVector3 &CVector3::operator%=(const CVector3 &inVector)
{
    float theX = y * inVector.z - z * inVector.y;
    float theY = z * inVector.x - x * inVector.z;
    float theZ = x * inVector.y - y * inVector.x;

    x = theX;
    y = theY;
    z = theZ;
    return *this;
}

//==============================================================================
//								FUNCTIONS
//==============================================================================

//==============================================================================
/**
 *	Calculates the squared distance between this vector and another.
 *	This is often used in sorting situations where the real distance isn't needed.
 *	@param	inVector the vector to which the distance is being calculated
 *	@return	the squared distance between vectors
 */
inline float CVector3::DistanceSquared(const CVector3 &inVector) const
{
    return (x - inVector.x) * (x - inVector.x) + (y - inVector.y) * (y - inVector.y)
        + (z - inVector.z) * (z - inVector.z);
}

//==============================================================================
/**
 *	Calculates the distance between this vector and another.
 *	@param	inVector the vector to which the distance is being calculated
 *	@return	the distance between vectors
 */
inline float CVector3::Distance(const CVector3 &inVector) const
{
    return ::sqrtf((x - inVector.x) * (x - inVector.x) + (y - inVector.y) * (y - inVector.y)
                   + (z - inVector.z) * (z - inVector.z));
}

//==============================================================================
/**
 *	Calculates the squared length (squared magnitude) of the current vector.
 *	@return	the squared length of this vector
 */
inline float CVector3::LengthSquared() const
{
    return x * x + y * y + z * z;
}

//==============================================================================
/**
 *	Calculates the length (magnitude) of the current vector.
 *	@return	the length of this vector
 */
inline float CVector3::Length() const
{
    return ::sqrt(x * x + y * y + z * z);
}

//==============================================================================
/**
 *	Calculates the dot product of this vector and another.
 *	@param	inVector3 is the other vector
 *	@return	the dot product
 *	@see	operator*
 */
inline float CVector3::DotProduct(const CVector3 &inVector) const
{
    return x * inVector.x + y * inVector.y + z * inVector.z;
}

//==============================================================================
/**
 *	Calculates the cross product of this vector and another.
 *	@param	inVector3 is the other vector
 *	@return the cross product vector
 *	@see	operator%
 *	@see	operator%=
 */
inline CVector3 CVector3::CrossProduct(const CVector3 &inVector)
{
    return CVector3(y * inVector.z - z * inVector.y, z * inVector.x - x * inVector.z,
                    x * inVector.y - y * inVector.x);
}

//==============================================================================
/**
 *	Normalizes the current vector making it a unit vector.
 *	The normalized vector is defined to be: V_norm = V / Magnitude(V).
  *	@return	this normalized vector
*/
inline CVector3 &CVector3::Normalize()
{
    float theLengthSquared = LengthSquared();
    if ((theLengthSquared > SQREPSILON) && ::abs(theLengthSquared - 1.0f) > SQREPSILON) {
        float theInvLength = 1.0f / ::sqrt(theLengthSquared);
        x *= theInvLength;
        y *= theInvLength;
        z *= theInvLength;
    }
    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the minimum between
 *	this vector and another.
 *	@param	inVector3 The vector that whose elements are tested against the
 *				current vector to build the minimum.
 *	@return	this minimized vector
*/
inline CVector3 &CVector3::Minimize(const CVector3 &inVector)
{
    x = MIN<float>(x, inVector.x);
    y = MIN<float>(y, inVector.y);
    z = MIN<float>(z, inVector.z);
    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the maximum between
 *	this vector and another.
 *	@param	inVector3 The vector that whose elements are tested against the
 *				current vector to build the maximum.
 *	@return	this maximized vector
*/
inline CVector3 &CVector3::Maximize(const CVector3 &inVector)
{
    x = MAX<float>(x, inVector.x);
    y = MAX<float>(y, inVector.y);
    z = MAX<float>(z, inVector.z);
    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector by performing a linear interpolation between the
 *	current vector and the given vector.  If inFactor is 0.0 then this vector
 *	remains unchanged.  If inFactor is 1.0 then this vector becomes inDestVector.
 *	If inFactor is between 0.0-1.0 then this vector becomes a vector between
 *	this old vector and inDestVector proportionally interpolated based in inFactor.
 *	If inFactor is less than 0 or more than 1 then this vector is extrapolated.
 *
 *	@param	inDestVector Second vector for the interpolation
 *	@param	inFactor Weight for the interpolation
 *	@return	this interpolated vector
 */
inline CVector3 &CVector3::InterpolateLinear(const CVector3 &inDestVector, float inFactor)
{
    x += inFactor * (inDestVector.x - x);
    y += inFactor * (inDestVector.y - y);
    z += inFactor * (inDestVector.z - z);
    return *this;
}

//==============================================================================
/**
 *	Transforms this vector by the given matrix.
 *
 *	The matrix is row column of the form (Matrix[row][column]):
 *<code>							<p>
 *			| m0	m1	m2	w |		<br>
 *			| m4	m5	m6	w |		<br>
 *			| m8	m9	m10	w |		<br>
 *			| tX	tY	tZ	w |		<br>
 *</code>
 *	@param	inMatrix	transform matrix
 *	@param	inTranslate	false if you only want to rotate/scale the vector
 *	@return	this transformed vector
 */
CVector3 &CVector3::Transform(const CMatrix &inMatrix, bool inTranslate)
{
    float theX = x;
    float theY = y;
    float theZ = z;

    x = theX * inMatrix.m[0][0] + theY * inMatrix.m[1][0] + theZ * inMatrix.m[2][0];
    y = theX * inMatrix.m[0][1] + theY * inMatrix.m[1][1] + theZ * inMatrix.m[2][1];
    z = theX * inMatrix.m[0][2] + theY * inMatrix.m[1][2] + theZ * inMatrix.m[2][2];

    if (inTranslate) {
        x += inMatrix.m[3][0];
        y += inMatrix.m[3][1];
        z += inMatrix.m[3][2];
    }

    return *this;
}
