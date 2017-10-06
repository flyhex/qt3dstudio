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
 *	Empty constructor.	These default values represent the identity quaternion.
 */

CQuaternion::CQuaternion()
    : qx(0.0f)
    , qy(0.0f)
    , qz(0.0f)
    , qw(1.0f)
{
}

//==============================================================================
/**
 *	Simple constructor using arguments to initialize the quaternion.
 *	@param	inX first component
 *	@param	inY second component
 *	@param	inZ third component
 *	@param	inW fourth component
 */

CQuaternion::CQuaternion(const float inX, const float inY, const float inZ, const float inW)
    : qx(inX)
    , qy(inY)
    , qz(inZ)
    , qw(inW)
{
}

//==============================================================================
/**
 *	Copy constructor.
 *	@param	inQuat the source quaternion
 */

CQuaternion::CQuaternion(const CQuaternion &inQuat)
    : qx(inQuat.qx)
    , qy(inQuat.qy)
    , qz(inQuat.qz)
    , qw(inQuat.qw)
{
}

//==============================================================================
/**
 *	Conversion constructor from a matrix.  Expensive - caveat emptor.
 *	@param	inMatrix a matrix with a rotation component to be extracted.
 */

CQuaternion::CQuaternion(const CMatrix &inMatrix)
{
    *this = inMatrix;
}

//==============================================================================
/**
 *	Conversion constructor from an axis with an angle
 *	@param	inAxisAngle the axis and angle the quaternion will represent.
 */

CQuaternion::CQuaternion(const CAxisAngle &inAxisAngle)
{
    *this = inAxisAngle;
}

//==============================================================================
/**
 *	Conversion constructor from a set of euler angles.
 *	@param	inAngles the angles the quaternion will represent.
 */

CQuaternion::CQuaternion(const CEulerAngles &inAngles)
{
    *this = inAngles;
}

//==============================================================================
//								INITIALIZATION
//==============================================================================

/**
 *	Explicit assignment method.
 *	@param	inX first component
 *	@param	inY second component
 *	@param	inZ third component
 *	@param	inW fourth component
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::Set(const float inX, const float inY, const float inZ, const float inW)

{
    qx = inX;
    qy = inY;
    qz = inZ;
    qw = inW;
    return *this;
}

//==============================================================================
/**
 *	Resets this quaternion to the identity quaternion.
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::Reset()

{
    qx = 0;
    qy = 0;
    qz = 0;
    qw = 1.0f;
    return *this;
}

//==============================================================================
//								FUNCTIONS
//==============================================================================

//==============================================================================
/**
 *	Normalizes this quaternion.  Most quaternion operations require it to
 *	be normalized.
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::Normalize()

{

    float theNorm = qx * qx + qy * qy + qz * qz + qw * qw;

    if (theNorm < SQREPSILON) {
        qx = 0;
        qy = 0;
        qz = 0;
        qw = 1.0f;
    } else if (::abs(theNorm - 1.0f) > SQREPSILON) {
        float theScale = 1.0f / ::sqrt(theNorm);

        qx *= theScale;
        qy *= theScale;
        qz *= theScale;
        qw *= theScale;
    }

    return *this;
}

//==============================================================================
/**
 *	Inverts this quaternion.
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::Invert()

{

    qx = -qx;
    qy = -qy;
    qz = -qz;
    qw = -qw;
    return *this;
}

//==============================================================================
/**
 *	Gets the dotproduct between this and another quaternion.
 *	@return the dotproduct
 */

float CQuaternion::DotProduct(const CQuaternion &inQuat) const

{

    return qx * inQuat.qx + qy * inQuat.qy + qz * inQuat.qz + qw * inQuat.qw;
}

//==============================================================================
/**
 *	Spherical Linear Interpolation.  Rotate smoothly between two angles
 *	without the gimbal locks that plague Euler angle representations.
 *
 *	TODO: MF - Code needs serious unit testing before release.
 *
 *	@param	inQuatA	rotation we are interpolating from
 *	@param	inQuatB	rotation we are interpolating to
 *	@param	inFactor interpolation amount: 1.0 turns this into inQuatB
 *	@return a reference to this modified quaternion
 */

CQuaternion CQuaternion::Slerp(const CQuaternion &inQuatA, const CQuaternion &inQuatB,
                               float inFactor)

{

    float theFlip = 1.0f;

    float theCos = inQuatA.DotProduct(inQuatB); // This is cos of angle between A and B

    if (theCos < 0)

    {

        theCos = -theCos;

        theFlip = -theFlip;
    }

    float theAlpha = inFactor * theFlip;

    float theBeta = 1.0f - inFactor;

    // Plain linear interpolation only possible when the angle is very small, sin(x) ~= x

    if ((1.0f - theCos) > FLT_EPSILON * 100.0f)

    {

        // Normal interpolation needed

        float theTheta = ::acos(theCos);

        float theSin = ::sin(theTheta);

        theAlpha = ::sin(theAlpha * theTheta) / theSin;

        theBeta = ::sin(theBeta * theTheta) / theSin;
    }

    return CQuaternion(inQuatA.qx * theBeta + inQuatB.qx * theAlpha,

                       inQuatA.qy * theBeta + inQuatB.qy * theAlpha,

                       inQuatA.qz * theBeta + inQuatB.qz * theAlpha,

                       inQuatA.qw * theBeta + inQuatB.qw * theAlpha);
}

//==============================================================================
//								CONVERTERS
//==============================================================================

//==============================================================================
/**
 *	Sets the rotation portion of the given matrix to equivalent of this quaternion.
 *	Be aware that only the uppe 3x3 portion is affected so be sure that the given
 *	matrix is an identity matrix unless you intend otherwise.
 *	@param	outMatrix the matrix written to
 *	@return the modified matrix given as argument
 */

CMatrix &CQuaternion::ToMatrix(CMatrix &outMatrix) const

{

    float fTX = 2.0f * qx;
    float fTY = 2.0f * qy;
    float fTZ = 2.0f * qz;
    float fTWX = fTX * qw;
    float fTWY = fTY * qw;
    float fTWZ = fTZ * qw;
    float fTXX = fTX * qx;
    float fTXY = fTY * qx;
    float fTXZ = fTZ * qx;
    float fTYY = fTY * qy;
    float fTYZ = fTZ * qy;
    float fTZZ = fTZ * qz;

    float *pfMat = &outMatrix.m[0][0];

    pfMat[0] = 1.0f - (fTYY + fTZZ);
    pfMat[1] = fTXY - fTWZ;
    pfMat[2] = fTXZ + fTWY;

    pfMat[4] = fTXY + fTWZ;
    pfMat[5] = 1.0f - (fTXX + fTZZ);
    pfMat[6] = fTYZ - fTWX;

    pfMat[8] = fTXZ - fTWY;
    pfMat[9] = fTYZ + fTWX;
    pfMat[10] = 1.0f - (fTXX + fTYY);

    return outMatrix;
}

//==============================================================================
/**
 *	Sets the given axis-angle to equivalent to this quaternion.
 *	@param	outAxisAngle the axis-angle written to
 *	@return the modified axis-angle given as argument
 */

CAxisAngle &CQuaternion::ToAxisAngle(CAxisAngle &outAxisAngle) const

{

    float theSqrLength = qx * qx + qy * qy + qz * qz;

    if (theSqrLength > 0) {
        float theInvLength = 1.0f / ::log(theSqrLength);

        outAxisAngle.m_Angle = 2.0f * ::acos(qw);
        outAxisAngle.m_Axis.x = qx * theInvLength;
        outAxisAngle.m_Axis.y = qy * theInvLength;
        outAxisAngle.m_Axis.z = qz * theInvLength;
    } else {
        outAxisAngle.m_Angle = 0;
        outAxisAngle.m_Axis.x = 1.0f;
        outAxisAngle.m_Axis.y = 0;
        outAxisAngle.m_Axis.z = 0;
    }

    return outAxisAngle;
}

//==============================================================================
//								OPERATORS
//==============================================================================

//==============================================================================
/**
 *	Simple assignment operator
 *	@param	inQuat the quaternion being copied
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::operator=(const CQuaternion &inQuat)

{
    qx = inQuat.qx;
    qy = inQuat.qy;
    qz = inQuat.qz;
    qw = inQuat.qw;

    return *this;
}

//==============================================================================
/**
 *	Assign from matrix - Extract rotation information from the given matrix.
 *	TODO: MF - Need to clean up this code after unit tests are in place.
 *	@param	inMatrix the matrix containing rotation data
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::operator=(const CMatrix &inMatrix)

{
    CMatrix rkMatrix(inMatrix);
    // Inspired by Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course
    // article "Quaternion Calculus and Fast Animation".

    float fTrace = rkMatrix[0][0] + rkMatrix[1][1] + rkMatrix[2][2];
    float fRoot;

    if (fTrace > 0.0f) {
        fRoot = ::sqrt(fTrace + 1.0f);

        qw = 0.5f * fRoot;

        fRoot = 0.5f / fRoot;

        qx = (rkMatrix[2][1] - rkMatrix[1][2]) * fRoot;
        qy = (rkMatrix[0][2] - rkMatrix[2][0]) * fRoot;
        qz = (rkMatrix[1][0] - rkMatrix[0][1]) * fRoot;
    } else {
        int iNext[3] = { 1, 2, 0 };

        int i = 0;
        if (rkMatrix[1][1] > rkMatrix[0][0])
            i = 1;

        if (rkMatrix[2][2] > rkMatrix[i][i])
            i = 2;

        int j = iNext[i];
        int k = iNext[j];

        fRoot = sqrtf(rkMatrix[i][i] - rkMatrix[j][j] - rkMatrix[k][k] + 1.0f);

        float *apfQuat[3] = { &qx, &qy, &qz };

        *(apfQuat[i]) = 0.5f * fRoot;

        fRoot = 0.5f / fRoot;

        qw = (rkMatrix[k][j] - rkMatrix[j][k]) * fRoot;

        *(apfQuat[j]) = (rkMatrix[j][i] + rkMatrix[i][j]) * fRoot;
        *(apfQuat[k]) = (rkMatrix[k][i] + rkMatrix[i][k]) * fRoot;
    }

    return Normalize();
}

//==============================================================================
/**
 *	Assign from matrix - Extract rotation information from the given axis-angle.
 *	@param	inAxisAngle the corresponding axis-angle
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::operator=(const CAxisAngle &inAxisAngle)

{

    float theHalfAngle = 0.5f * inAxisAngle.m_Angle;
    float theSin = ::sin(theHalfAngle);

    qx = theSin * inAxisAngle.m_Axis.x;
    qy = theSin * inAxisAngle.m_Axis.y;
    qz = theSin * inAxisAngle.m_Axis.z;
    qw = ::cos(theHalfAngle);

    return Normalize();
}

//==============================================================================
/**
 *	Comparison operator.
 *	@param	inQuat quaternion being copmpared to
 *	@return true if the quaternions are rougly the same
 */

bool CQuaternion::operator==(const CQuaternion &inQuat) const

{

    return (::abs(qx - inQuat.qx) < EPSILON) && (::abs(qy - inQuat.qy) < EPSILON)
        && (::abs(qz - inQuat.qz) < EPSILON) && (::abs(qw - inQuat.qw) < EPSILON);
}

//==============================================================================
/**
 *	Inequality operator.
 *	@para	inQuat quaternion being compared to
 *	@return true if the quaternions are not even rougly the same
 */

bool CQuaternion::operator!=(const CQuaternion &inQuat) const

{

    return (::abs(qx - inQuat.qx) >= EPSILON) || (::abs(qy - inQuat.qy) >= EPSILON)
        || (::abs(qz - inQuat.qz) >= EPSILON) || (::abs(qw - inQuat.qw) >= EPSILON);
}

//==============================================================================
/**
 *	Adds this quaternion to another but does not modify this quaternion.
 *	@param	inQuat the other quaternion
 *	@return a new summed quaternion
 */

CQuaternion CQuaternion::operator+(const CQuaternion &inQuat) const

{

    return CQuaternion(qx + inQuat.qx, qy + inQuat.qy, qz + inQuat.qz, qw + inQuat.qw);
}

//==============================================================================
/**
 *	Adds another quaternion to this.
 *	@param	inQuat the quaternion being added
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::operator+=(const CQuaternion &inQuat)

{

    qx += inQuat.qx;
    qy += inQuat.qy;
    qz += inQuat.qz;
    qw += inQuat.qw;
    return *this;
}

//==============================================================================
/**
 *	Concatenates another quaternion with this without affecting this quaternion.
 *	This has the same effect as if you would multiply two rotation matrixes
 *	with each other.  Alas, this operation is not commutative.
 *	@param	inQuat the quaternion being concatenated.
 *	@return the new concatenated quaternion
 */

CQuaternion CQuaternion::operator*(const CQuaternion &inQuat) const

{

    CVector3 theVecOne(qx, qy, qz);
    CVector3 theVecTwo(inQuat.qx, inQuat.qy, inQuat.qz);
    CVector3 theVecResult = (theVecOne % theVecTwo) + theVecTwo * qw + theVecOne * inQuat.qw;

    return CQuaternion(theVecResult.x, theVecResult.y, theVecResult.z,
                       qw * inQuat.qw - theVecOne * theVecTwo);
}

//==============================================================================
/**
 *	Get a scaled copy of this quaternion.
 *	@param	inFactor is the scaling factor
 *	@return a new scaled quaternion
 */

CQuaternion CQuaternion::operator*(float inFactor) const

{

    return CQuaternion(qx * inFactor, qy * inFactor, qz * inFactor, qw * inFactor);
}

//==============================================================================
/**
 *	Concatenates this quaternion with another quaternion.
 *	This has the same effect as if you would multiply two rotation matrixes
 *	with each other.  Alas, this operation is not commutative.
 *	@param	inQuat the quaternion being concatenated
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::operator*=(const CQuaternion &inQuat)

{

    CVector3 theVecOne(qx, qy, qz);
    CVector3 theVecTwo(inQuat.qx, inQuat.qy, inQuat.qz);
    CVector3 theVecResult = (theVecOne % theVecTwo) + theVecTwo * qw + theVecOne * inQuat.qw;

    return Set(theVecResult.x, theVecResult.y, theVecResult.z,
               qw * inQuat.qw - theVecOne * theVecTwo);
}

//==============================================================================
/**
 *	Scales this quaternion
 *	@param	inFactor is the scaling factor
 *	@return a reference to this modified quaternion
 */

CQuaternion &CQuaternion::operator*=(float inScalar)

{

    qx *= inScalar;
    qy *= inScalar;
    qz *= inScalar;
    qw *= inScalar;

    return *this;
}

//==============================================================================
/**
 *	Applies this rotation to a vector.
 *	@param inVector is the vector to be rotated.
 *	@return a new rotated vector
 */

CVector3 CQuaternion::operator*(const CVector3 &inVector) const

{
    CVector3 kVec0(qx, qy, qz);
    CVector3 kVec1(kVec0 % inVector);

    kVec1 += inVector * qw;

    CVector3 kVec2 = kVec1 % kVec0;

    kVec0 *= inVector * kVec0;
    kVec0 += kVec1 * qw;

    return CVector3(kVec0 - kVec2);
}
