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
 *	Empty constructor.  Initializes this matrix to the identity matrix.
 */

CMatrix::CMatrix()
{
    ::memcpy(m, s_Identity, sizeof(m));
}

//==============================================================================
/**
 *	Copy constructor.
 *	@param	inMatrix is the source matrix to copy
 */

CMatrix::CMatrix(const CMatrix &inMatrix)
{
    ::memcpy(m, inMatrix.m, sizeof(m));
}

//==============================================================================
/**
 *	Assignment constructor.  Initializes the matrix from an array.
 *	@param	inComponents is an array of 16 values
 */

CMatrix::CMatrix(const float inComponents[])

{

    ::memcpy(m, inComponents, sizeof(m));
}

//==============================================================================
//								INITIALIZATION
//==============================================================================

//==============================================================================
/**
 *	Sets this matrix to a NULL matrix with all values at zero.
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Zero()
{
    ::memset(m, 0, sizeof(m));
    return *this;
}

//==============================================================================
/**
 *	Sets the matrix to the identity matrix.
 *	@return reference to this matrix
 */

CMatrix &CMatrix::Identity()
{
    ::memcpy(m, s_Identity, sizeof(m));
    return *this;
}

//==============================================================================
/**
 *	Copies the elements from another matrix.
 *	@param inMatrix is the source matrix
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Set(const CMatrix &inMatrix)
{
    ::memcpy(m, inMatrix.m, sizeof(m));
    return *this;
}

//==============================================================================
/**
 *	Copies the given components to this matrix.
 *	@param inComponents is an array of 16 components
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Set(const float inComponents[])

{
    ::memcpy(m, inComponents, sizeof(m));
    return *this;
}

//==============================================================================
/**
 *	Heavily optimized assignment method.  This has the same result as creating
 *	four full 4x4 matrices and multiplying them together.
 *	@param	inTranslation	translation coordinates
 *	@param	inRotation		euler angle rotations
 *	@param	inScale			scaling dimensions
 *	@param	inPivot			pivot offset vector
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Set(const CVector3 &inTranslation, const CEulerAngles &inRotation,
                      const CVector3 &inScale, const CVector3 &inPivot)
{
    // TODO: MF - Since we are bypassing quaternions we have to assert that these angles are Studio
    // angles
    // assert( CEulerAngles::YXZr == inRotation.m_Order );

    // Precalculate the sins and cosins.
    float theSinX = ::sin(inRotation.m_Angle.x);
    float theSinY = ::sin(inRotation.m_Angle.y);
    float theSinZ = ::sin(inRotation.m_Angle.z);
    float theCosX = ::cos(inRotation.m_Angle.x);
    float theCosY = ::cos(inRotation.m_Angle.y);
    float theCosZ = ::cos(inRotation.m_Angle.z);

    // Precalulate the scaled rotation 3x3 matrix and populate the rotation section of our 4x4
    // matrix
    float r1 = inScale.x * ((theCosZ * theCosY) + (theSinX * theSinY * theSinZ));
    m[0][0] = r1;
    float r4 = inScale.x * (theCosX * theSinZ);
    m[0][1] = r4;
    float r7 = -inScale.x * ((theSinY * theCosZ) + (theSinZ * theCosY * theSinX));
    m[0][2] = r7;
    m[0][3] = 0;

    float r2 = -inScale.y * ((theSinZ * theCosY) + (theSinX * theSinY * theCosZ));
    m[1][0] = r2;
    float r5 = inScale.y * (theCosX * theCosZ);
    m[1][1] = r5;
    float r8 = inScale.y * ((theSinY * theSinZ) + (theSinX * theCosY * theCosZ));
    m[1][2] = r8;
    m[1][3] = 0;

    float r3 = inScale.z * (theSinY * theCosX);
    m[2][0] = r3;
    float r6 = -inScale.z * (theSinX);
    m[2][1] = r6;
    float r9 = inScale.z * (theCosX * theCosY);
    m[2][2] = r9;
    m[2][3] = 0;

    // Reuse the scaled rotation matrix in the translation/pivot section of out 4x4 matrix
    m[3][0] = inTranslation.x - r1 * inPivot.x - r2 * inPivot.y - r3 * inPivot.z;
    m[3][1] = inTranslation.y - r4 * inPivot.x - r5 * inPivot.y - r6 * inPivot.z;
    m[3][2] = inTranslation.z - r7 * inPivot.x - r8 * inPivot.y - r9 * inPivot.z;
    m[3][3] = 1.0f;

    return *this;
}

//==============================================================================
/**
 *	Sets the translation portion of the matrix without affecting the rest.
 *	If you want a pure translation matrix you must make sure to start with
 *	the identity matrix.
 *	@param	inTranslate	a translation vector
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::SetTranslate(const CVector3 &inTranslate)
{
    m[3][0] = inTranslate.x;

    m[3][1] = inTranslate.y;

    m[3][2] = inTranslate.z;
    return *this;
}

//==============================================================================
/**
 *	Sets the rotation portion of the matrix without affecting the rest.
 *	FYI: The rotate and scale portions overlap.  If you want a pure scale matrix
 *	you must make sure to start with the identity matrix.
 *	@param	inRotation a quaternion describing the rotaion
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::SetRotate(const CQuaternion &inRotation)
{
    inRotation.ToMatrix(*this);
    return *this;

    /*	// Precalculate the sin and cos.

            float		theSinX	= (float) sin( inRotation.x );

            float		theSinY	= (float) sin( inRotation.y );

            float		theSinZ	= (float) sin( inRotation.z );

            float		theCosX	= (float) cos( inRotation.x );

            float		theCosY	= (float) cos( inRotation.y );

            float		theCosZ	= (float) cos( inRotation.z );



            // Fill out the matrix.

            m[0][0] = ( theCosZ * theCosY ) + ( theSinX * theSinY * theSinZ );

            m[0][1] = -theCosX * -theSinZ;

            m[0][2] = - ( theSinY * theCosZ ) + ( theSinZ * theCosY * theSinX );



            m[1][0] = - ( theSinZ * theCosY ) + ( theSinX * theSinY * theCosZ );

            m[1][1] = theCosX * theCosZ;

            m[1][2] = ( theSinY * theSinZ ) + ( theSinX * theCosY * theCosZ );



            m[2][0] = theSinY * theCosX;

            m[2][1] = - theSinX;

            m[2][2] = theCosX * theCosY;



            // Set tranlate portion of matrix.

            m[ 3 ][ 0 ] = 0.0f;

            m[ 3 ][ 1 ] = 0.0f;

            m[ 3 ][ 2 ] = 0.0f;

            m[ 3 ][ 3 ] = 1.0f;*/
}

//==============================================================================
/**
 *	Sets the scale portion of the matrix without affecting the rest.
 *	FYI: The rotate and scale portions overlap.  If you want a pure scale matrix
 *	you must make sure to start with the identity matrix.
 *	@param	inTranslate	a scale vector
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::SetScale(const CVector3 &inScale)
{
    m[0][0] = inScale.x;

    m[1][1] = inScale.y;

    m[2][2] = inScale.z;

    return *this;
}

//==============================================================================
/**
 *	Sets the current matrix to describe a perspective matrix that produces
 *	a perspective projection. This is a DX style 0..1 clipping projection matrix.
 *
 *	n=near, f=far, l=left, r=right, t=top, b=bottom		<br>
 *	A = (r+l)/(r-l)										<br>
 *	B = (t+b)/(t-b)										<br>
 *	C = f/(f-n) <br>
 *	D = -fn/(f-n)										<p>
 *
 *	2n/(r-l)	0			0		0
 *	0			2n/(t-b)	0		0
 *	A			B			C		1
 *	0			0			D		0
 *
 *	@param inOrthographic true if the matrix should be orthographic, false for
 *	@return reference to this modified matrix
 */
CMatrix &CMatrix::SetFrustum(const bool inOrthographic, const float inNear, const float inFar,
                             const float inLeft, const float inTop, const float inRight,
                             const float inBottom)
{
    float theWidth = inRight - inLeft;
    float theHeight = inBottom - inTop;
    float theClipDist = inFar - inNear;

    if (theWidth > 0 && theHeight > 0 && theClipDist > 0) {
        float theWidthRecip = 1.0f / theWidth;
        float theHeightRecip = 1.0f / theHeight;
        float theClipRecip = 1.0f / theClipDist;

        Identity();
        if (inOrthographic) {
            m[0][0] = 2.0f * theWidthRecip;
            m[1][1] = 2.0f * theHeightRecip;
            m[2][2] = 1.0f * theClipRecip;

            m[3][0] = -(inRight + inLeft) * theWidthRecip;
            m[3][1] = -(inTop + inBottom) * theHeightRecip;
            m[3][2] = (-inFar * theClipRecip) + 1.0f; //-(inFar+inNear) * theClipRecip;
            m[3][3] = 1.0f;
        } else {
            m[0][0] = 2.0f * inNear * theWidthRecip;
            m[1][1] = 2.0f * inNear * theHeightRecip;
            m[2][0] = -(inRight + inLeft) * theWidthRecip;
            m[2][1] = -(inBottom + inTop) * theHeightRecip;
            m[2][2] = inFar * theClipRecip; // C
            m[2][3] = 1.0f;
            m[3][2] = (-inFar * inNear) * theClipRecip; // D
            m[3][3] = 0.0f;
        }
    }

    return *this;
}

//==============================================================================
//								FUNCTIONS
//==============================================================================

//==============================================================================
/**
 *	Check this matrix against the identity matrix.
 *	@return true if this matrix is equivalent to the identity matrix
 */

bool CMatrix::IsIdentity() const
{
    return *this == s_Identity;
}

//==============================================================================
/**
 *	Checks to see if this matrix is affine.
 *	An affine matrix has the last row being [ 0 0 0 1 ]
 *	@return true if the matrix is affine
 */
bool CMatrix::IsAffine() const
{
    return m[0][3] == 0 && m[1][3] == 0 && m[2][3] == 0 && m[3][3] == 1.0f;
}

//==============================================================================
/**
 *	Appends the matrix to include a translation.  Equivalent to post-multiplying
 *	this matrix with a transformation matrix derived from the given vector.
 *	@param inVector is the transformation vector applied
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Translate(const CVector3 &inTranslation)
{
    m[3][0] += m[0][0] * inTranslation.x + m[1][0] * inTranslation.y + m[2][0] * inTranslation.z;
    m[3][1] += m[0][1] * inTranslation.x + m[1][1] * inTranslation.y + m[2][1] * inTranslation.z;
    m[3][2] += m[0][2] * inTranslation.x + m[1][2] * inTranslation.y + m[2][2] * inTranslation.z;
    return *this;
}

//==============================================================================
/**
 *	Appends the matrix to include a rotation.  Equivalent to post-multiplying
 *	this matrix with a rotation matrix derived from the given quaternion.
 *	@param inRotation is the rotation quaternion applied
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Rotate(const CQuaternion &inRotation)
{
    CMatrix theRotation;
    return MultiplyAffine(inRotation.ToMatrix(theRotation));
}

//==============================================================================
/**
 *	Appends the matrix to include scaling.  Equivalent to post-multiplying
 *	this matrix with a scale matrix derived from the given vector.
 *	@param inVector is the scale vector applied
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Scale(const CVector3 &inScale)
{
    m[0][0] *= inScale.x;
    m[0][1] *= inScale.x;
    m[0][2] *= inScale.x;

    m[1][0] *= inScale.y;
    m[1][1] *= inScale.y;
    m[1][2] *= inScale.y;

    m[2][0] *= inScale.z;
    m[2][1] *= inScale.z;
    m[2][2] *= inScale.z;
    return *this;
}

//==============================================================================
/**
 *	Flips the matrix elements around the identity diagonal.
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::Transpose()

{
    float theSwap = m[1][0];
    m[1][0] = m[0][1];
    m[0][1] = theSwap;

    theSwap = m[2][0];
    m[2][0] = m[0][2];
    m[0][2] = theSwap;

    theSwap = m[3][0];
    m[3][0] = m[0][3];
    m[0][3] = theSwap;

    theSwap = m[2][1];
    m[2][1] = m[1][2];
    m[1][2] = theSwap;

    theSwap = m[3][1];
    m[3][1] = m[1][3];
    m[1][3] = theSwap;

    theSwap = m[3][2];
    m[3][2] = m[2][3];
    m[2][3] = theSwap;

    return *this;
}

//==============================================================================
/**
 * Computes the inverse of a 3D affine matrix; i.e. a matrix with a dimen-
 * sionality of 4 where the bottom row has the entries (0, 0, 0, 1).
 *
 * This procedure treats the 4 by 4 matrix as a block matrix and
 * calculates the inverse of one submatrix for a significant performance
 * improvement over a general procedure that can invert any non-singular matrix:
 * (see code for correct formatting of matrix)
 *
 *       |           | -1    |   -1       -1 |	<br>
 *       | A       C |       |  A    -C A    |	<br>
 *  -1   |           |       |               |	<br>
 * M  =  |           |   =   |               |	<br>
 *       | 0       1 |       |  0       1    |	<br>
 *       |           |       |               |	<br>
 *
 * where M is a 4 by 4 matrix,
 * A is the 3 by 3 upper left submatrix of M,
 * C is the 3 by 1 upper right submatrix of M.
 *
 * @return the determinant of matrix
 */
float CMatrix::Invert()
{
    const float PRECISIONLIMIT(1.0e-07f);
    float thePositiveDet(0.0f);
    float theNegativeDet(0.0f);
    float theTempDet;

    // Calculate the determinant of submatrix A and determine if the
    // the matrix is singular as limited by the float precision.
    theTempDet = m[0][0] * m[1][1] * m[2][2];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = m[0][1] * m[1][2] * m[2][0];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = m[0][2] * m[1][0] * m[2][1];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m[0][2] * m[1][1] * m[2][0];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m[0][1] * m[1][0] * m[2][2];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m[0][0] * m[1][2] * m[2][1];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    // Is the submatrix A nonsingular? (i.e. there is an inverse?)
    float theDeterminant = thePositiveDet + theNegativeDet;
    if (theDeterminant != 0
        || ::abs(theDeterminant / (thePositiveDet - theNegativeDet)) >= PRECISIONLIMIT) {
        CMatrix theInverse;
        float theInvDeterminant = 1.0f / theDeterminant;

        // Calculate inverse(A) = adj(A) / det(A)
        theInverse.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * theInvDeterminant;
        theInverse.m[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * theInvDeterminant;
        theInverse.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * theInvDeterminant;
        theInverse.m[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * theInvDeterminant;
        theInverse.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * theInvDeterminant;
        theInverse.m[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * theInvDeterminant;
        theInverse.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * theInvDeterminant;
        theInverse.m[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * theInvDeterminant;
        theInverse.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * theInvDeterminant;

        // Calculate -C * inverse(A)
        theInverse.m[3][0] = -(m[3][0] * theInverse.m[0][0] + m[3][1] * theInverse.m[1][0]
                               + m[3][2] * theInverse.m[2][0]);
        theInverse.m[3][1] = -(m[3][0] * theInverse.m[0][1] + m[3][1] * theInverse.m[1][1]
                               + m[3][2] * theInverse.m[2][1]);
        theInverse.m[3][2] = -(m[3][0] * theInverse.m[0][2] + m[3][1] * theInverse.m[1][2]
                               + m[3][2] * theInverse.m[2][2]);

        // Assing ourselves to the inverse
        *this = theInverse;
    }

    return theDeterminant;
}

//==============================================================================
/**
 *	Fast multiplication of two affine 4x4 matrices.  No affine pre-check.
 *  TODO: MF - Convert to SSE Assembly Code
 *	@return reference to this modified matrix
 */
CMatrix &CMatrix::MultiplyAffine(const CMatrix &inMatrix)
{
    float theMult[4][4];

    theMult[0][0] =
        m[0][0] * inMatrix.m[0][0] + m[0][1] * inMatrix.m[1][0] + m[0][2] * inMatrix.m[2][0];
    theMult[0][1] =
        m[0][0] * inMatrix.m[0][1] + m[0][1] * inMatrix.m[1][1] + m[0][2] * inMatrix.m[2][1];
    theMult[0][2] =
        m[0][0] * inMatrix.m[0][2] + m[0][1] * inMatrix.m[1][2] + m[0][2] * inMatrix.m[2][2];
    theMult[0][3] = 0;

    theMult[1][0] =
        m[1][0] * inMatrix.m[0][0] + m[1][1] * inMatrix.m[1][0] + m[1][2] * inMatrix.m[2][0];
    theMult[1][1] =
        m[1][0] * inMatrix.m[0][1] + m[1][1] * inMatrix.m[1][1] + m[1][2] * inMatrix.m[2][1];
    theMult[1][2] =
        m[1][0] * inMatrix.m[0][2] + m[1][1] * inMatrix.m[1][2] + m[1][2] * inMatrix.m[2][2];
    theMult[1][3] = 0;

    theMult[2][0] =
        m[2][0] * inMatrix.m[0][0] + m[2][1] * inMatrix.m[1][0] + m[2][2] * inMatrix.m[2][0];
    theMult[2][1] =
        m[2][0] * inMatrix.m[0][1] + m[2][1] * inMatrix.m[1][1] + m[2][2] * inMatrix.m[2][1];
    theMult[2][2] =
        m[2][0] * inMatrix.m[0][2] + m[2][1] * inMatrix.m[1][2] + m[2][2] * inMatrix.m[2][2];
    theMult[2][3] = 0;

    theMult[3][0] = m[3][0] * inMatrix.m[0][0] + m[3][1] * inMatrix.m[1][0]
        + m[3][2] * inMatrix.m[2][0] + inMatrix.m[3][0];
    theMult[3][1] = m[3][0] * inMatrix.m[0][1] + m[3][1] * inMatrix.m[1][1]
        + m[3][2] * inMatrix.m[2][1] + inMatrix.m[3][1];
    theMult[3][2] = m[3][0] * inMatrix.m[0][2] + m[3][1] * inMatrix.m[1][2]
        + m[3][2] * inMatrix.m[2][2] + inMatrix.m[3][2];
    theMult[3][3] = 1.0f;

    Set(&theMult[0][0]);
    return *this;
}

//==============================================================================
//								OPERATORS
//==============================================================================

//==============================================================================
/**
 *	Simple assignment operator.
 *	@param inIndex
 *	@return reference to this modified matrix
 */

CMatrix &CMatrix::operator=(const CMatrix &inMatrix)
{
    ::memcpy(m, inMatrix.m, sizeof(m));
    return *this;
}

//==============================================================================
/**
 *	Compares this matrix's elements with another matrix's and returns true
 *	if the matrices are equivalent.
 *	@param	inMatrix is the matrix we are comparing against
 *	@return true if all elements are within EPSILON of each other.
 */

bool CMatrix::operator==(const CMatrix &inMatrix) const
{
    for (int iRow = 0; iRow < 4; ++iRow)
        for (int iCol = 0; iCol < 4; ++iCol)
            if (::abs(m[iRow][iCol] - inMatrix.m[iRow][iCol]) > EPSILON)
                return false;

    return true;
}

//==============================================================================
/**
 *	Compares this matrix's elements with another matrix's and returns true
 *	if the matrices are not equivalent.
 *	@param	inMatrix is the matrix we are comparing against
 *	@return true if one or more elements are more than EPSILON from each other
 */

bool CMatrix::operator!=(const CMatrix &inMatrix) const
{
    for (int iRow = 0; iRow < 4; ++iRow)
        for (int iCol = 0; iCol < 4; ++iCol)
            if (::abs(m[iRow][iCol] - inMatrix.m[iRow][iCol]) <= EPSILON)
                return false;

    return true;
}

//==============================================================================
/**
 *	Allows direct access to the row.  This allows fast direct access in the
 *	form of CMatrix[row][column].
 *	@param inRow is the row index, starting at zero
 *	@return a pointer to the first element
 */

float *CMatrix::operator[](int inRow)
{
    return &m[inRow][0];
}

//==============================================================================
/**
 *	Allows const direct access to the row.  This allows fast direct access in the
 *	form of CMatrix[row][column] from other const code.
 *	@param inRow is the row index, starting at zero
 *	@return a const pointer to the first element
 */

const float *CMatrix::operator[](int inRow) const
{
    return &m[inRow][0];
}
