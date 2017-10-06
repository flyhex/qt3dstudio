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

#include "Matrix.h"
#include "OptimizedArithmetic.h"
#include "Vector3.h"
#include "Rotation3.h"
#include <cmath>

#include "EulerAngles.h"

namespace Q3DStudio {

const float IDENTITY_MATRIX[4][4] = {
    { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 }
};

//==============================================================================
/**
*	Constructs the CMatrix object, setting it to the identitiy
*/
CMatrix::CMatrix()
{
    // Initialize the fields.
    ::memcpy(&m, &IDENTITY_MATRIX, sizeof(m));
}

//==============================================================================
/**
 *		Constructs and initializes the object given a source matrix.
 */
CMatrix::CMatrix(const CMatrix &inMatrix)
{
    ::memcpy(&m, &inMatrix.m, sizeof(m));
}

//==============================================================================
/**
 *		Constructs and initializes the object given all elements of the matrix.
 */
CMatrix::CMatrix(float in00, float in01, float in02, float in03, float in10, float in11, float in12,
                 float in13, float in20, float in21, float in22, float in23, float in30, float in31,
                 float in32, float in33)
{
    m[0][0] = in00;
    m[0][1] = in01;
    m[0][2] = in02;
    m[0][3] = in03;

    m[1][0] = in10;
    m[1][1] = in11;
    m[1][2] = in12;
    m[1][3] = in13;

    m[2][0] = in20;
    m[2][1] = in21;
    m[2][2] = in22;
    m[2][3] = in23;

    m[3][0] = in30;
    m[3][1] = in31;
    m[3][2] = in32;
    m[3][3] = in33;
}

//==============================================================================
/**
 *		Set: Sets the values of this matrix with the given matrix.
 *
 *		@param inMatrix
 */
void CMatrix::Set(const CMatrix &inMatrix)
{
    ::memcpy(&m, &inMatrix.m, sizeof(m));
}

//==============================================================================
/**
 *		Provides (row, column) access to the matrix.
 *
 *		@param 	inRow		The row index into the matrix
 *		@param	inColumn	The column index into the array
 *
 *		@return	a reference to the element
 */
float &CMatrix::operator()(const long &inRow, const long &inColumn)
{
    // Check bounds of input
    if (!CheckBounds(inRow, inColumn)) {
        // throw out_of_range exception
    }

    return m[inRow][inColumn];
}

//==============================================================================
/**
 *		Provides direct access to array elements.
 *		@return	a reference to the matrix 4x4
 */
CMatrix::TArray &CMatrix::GetArray()
{
    return m;
}

//==============================================================================
/**
 *		Provides (row, column) access to the matrix.
 *
 *		@param 	inRow		The row index into the matrix
 *		@param	inColumn	The column index into the array
 *
 *		@return	a copy of the element
 */
float CMatrix::operator()(const long &inRow, const long &inColumn) const
{
    // Check bounds of input
    if (!CheckBounds(inRow, inColumn)) {
        // throw out_of_range exception
    }

    return m[inRow][inColumn];
}

//==============================================================================
/**
 *		Adds this matrix to the matrix being passed in and returns the sum.
 *
 *		This method does not modify this matrix.
 *
 *		@param	inMatrix	The matrix to add to this matrix
 *
 *		@return	The sum of the two matrices.
 */
CMatrix CMatrix::operator+(const CMatrix &inMatrix) const
{
    CMatrix theSum;

    for (long theRow = 0; theRow < 4; ++theRow) {
        for (long theColumn = 0; theColumn < 4; ++theColumn) {
            theSum.m[theRow][theColumn] = m[theRow][theColumn] + inMatrix.m[theRow][theColumn];
        }
    }

    return theSum;
}

//==============================================================================
/**
 *		Subtracts the matrix passed in from this matrix and returns the difference.
 *
 *		This method does not modify this matrix.
 *
 *		@param	inMatrix	The matrix to subtract from this matrix
 *
 *		@return	The sum of the two matrices.
 */
CMatrix CMatrix::operator-(const CMatrix &inMatrix) const
{
    CMatrix theDiff;

    for (long theRow = 0; theRow < 4; ++theRow) {
        for (long theColumn = 0; theColumn < 4; ++theColumn) {
            theDiff.m[theRow][theColumn] = m[theRow][theColumn] - inMatrix.m[theRow][theColumn];
        }
    }

    return theDiff;
}

//==============================================================================
/**
 *		Multiiply this by the one passed in and return the new matrix.
 *
 *		This method does not modify this matrix.
 *
 *		@param	inMatrix	The matrix to multiply to this matrix
 *
 *		@return	The product of the two matrices.
 */
CMatrix CMatrix::operator*(const CMatrix &inMatrix) const
{
    CMatrix theCat;
    COptimizedArithmetic::MatrixMultiply(m[0], inMatrix.m[0], theCat.m[0]);

    return theCat;
}

//==============================================================================
/**
 *		Checks to see if this matrix is equal to the matrix being passed in.
 *
 *		@param	inMatrix	The matrix to to test for equality with this matrix.
 *
 *		@return	true if the matrices are equal, otherwise false.
 */
bool CMatrix::operator==(const CMatrix &inMatrix) const
{
    bool theEqualFlag = true;

    for (long theRow = 0; theEqualFlag && theRow < 4; ++theRow) {
        for (long theColumn = 0; theEqualFlag && theColumn < 4; ++theColumn) {
            if (inMatrix.m[theRow][theColumn] != m[theRow][theColumn]) {
                theEqualFlag = false;
            }
        }
    }

    return theEqualFlag;
}

//==============================================================================
/**
 *		Checks to see if this matrix is NOT equal to the matrix being passed in.
 *
 *		@param	inMatrix	The matrix to to test for inequality with this matrix.
 *
 *		@return	true if the matrices are NOT equal, otherwise false.
 */
bool CMatrix::operator!=(const CMatrix &inMatrix) const
{
    bool theNotEqualFlag = false;

    for (long theRow = 0; !theNotEqualFlag && theRow < 4; ++theRow) {
        for (long theColumn = 0; !theNotEqualFlag && theColumn < 4; ++theColumn) {
            if (inMatrix.m[theRow][theColumn] != m[theRow][theColumn]) {
                theNotEqualFlag = true;
            }
        }
    }

    return theNotEqualFlag;
}

//==============================================================================
/**
 *		operator =: Assign the value of this vector.
 *
 *		Long Description.
 *
 *		@param CMatrix&	inMatrix
 */
CMatrix &CMatrix::operator=(const CMatrix &inMatrix)
{
    // Check for self assignment
    if (this != &inMatrix) {
        ::memcpy(&m, &inMatrix.m, sizeof(m));
    }

    return *this;
}

//==============================================================================
/**
 *		Add a matrix to this matrix.
 *
 *		This method does modify this matrix.
 *
 *		@param	inMatrix	The matrix to add to this.
 *
 *		@return	this modified matrix.
 */
CMatrix &CMatrix::operator+=(const CMatrix &inMatrix)
{
    for (long theRow = 0; theRow < 4; ++theRow) {
        for (long theColumn = 0; theColumn < 4; ++theColumn) {
            m[theRow][theColumn] += inMatrix.m[theRow][theColumn];
        }
    }

    return *this;
}

//==============================================================================
/**
 *		Subtract a matrix from this matrix.
 *
 *		This method does modify this matrix.
 *
 *		@param	inMatrix	The matrix to subtract from this.
 *
 *		@return	this modified matrix.
 */
CMatrix &CMatrix::operator-=(const CMatrix &inMatrix)
{
    for (long theRow = 0; theRow < 4; ++theRow) {
        for (long theColumn = 0; theColumn < 4; ++theColumn) {
            m[theRow][theColumn] -= inMatrix.m[theRow][theColumn];
        }
    }

    return *this;
}

//==============================================================================
/**
 *		Multiply a matrix to this matrix.
 *
 *		This method does modify this matrix.
 *
 *		@param	inMatrix	The matrix to multiply to this.
 *
 *		@return	this modified matrix.
 */
CMatrix &CMatrix::operator*=(const CMatrix &inMatrix)
{
    CMatrix theCat;
    COptimizedArithmetic::MatrixMultiply(m[0], inMatrix.m[0], theCat.m[0]);

    *this = theCat;

    return *this;
}

//==============================================================================
/**
 *		Sets this matrix to the given rotation and translation vectors and returns this.
 *
 *		@param	inVector	The vector to translate the matrix by.
 *
 *		@return	This matrix after the translation.
 */
/*CMatrix& CMatrix::Set( const CVector3& inVector )
{
        CMatrix	theTransMatrix ( MakeTranslate( inVector ) );

        // Modify this matrix
        this *= theTransMatrix;

        return *this;
}*/

//==============================================================================
/**
 *		Translates this matrix by the given vector and returns this.
 *
 *		@param	inVector	The vector to translate the matrix by.
 *
 *		@return	This matrix after the translation.
 */
CMatrix &CMatrix::Translate(const CVector3 &inVector)
{
    CMatrix theTransMatrix(MakeTranslate(inVector));

    // Modify this matrix
    *this *= theTransMatrix;

    return *this;
}

//==============================================================================
/**
 *		Rotates this matrix by the given rotation vector and returns this.
 *
 *		@param	inRotation		The vector to rotate the matrix by.
 *
 *		@return	This matrix after the rotation.
 */
CMatrix &CMatrix::Rotate(const CRotation3 &inRotation)
{
    CMatrix theRotateMatrix(MakeRotate(inRotation));

    // Modify this matrix
    *this *= theRotateMatrix;

    return *this;
}

//==============================================================================
/**
 *		Scales this matrix by the given vector and returns this.
 *
 *		@param	inScale		The vector to scale the matrix by.
 *
 *		@return	This matrix after the rotation.
 */
CMatrix &CMatrix::Scale(const CVector3 &inScale)
{
    CMatrix theScaleMatrix(MakeScale(inScale));

    // Modify this matrix
    *this *= theScaleMatrix;

    return *this;
}

//==============================================================================
/**
 *		Returns the translation vector implicit in this matrix.
 *
 *		@return	The vector representing the translation portion of the matrix.
 */
CVector3 CMatrix::GetTranslation() const
{
    CVector3 theLocation(m[3][0], m[3][1], m[3][2]);

    return theLocation;
}

//==============================================================================
/**
 *		Returns the x component(axis) from the matrix.
 *
 *		@return	The x axis vector.
 */
CVector3 CMatrix::GetXComponent() const
{
    CVector3 theXComponent(m[0][0], m[0][1], m[0][2]);

    return theXComponent;
}

//==============================================================================
/**
 *		Returns the y component(axis) from the matrix.
 *
 *		@return	The y axis vector.
 */
CVector3 CMatrix::GetYComponent() const
{
    CVector3 theYComponent(m[1][0], m[1][1], m[1][2]);

    return theYComponent;
}

//==============================================================================
/**
 *		Returns the z component(axis) from the matrix.
 *
 *		@return	The z axis vector.
 */
CVector3 CMatrix::GetZComponent() const
{
    CVector3 theZComponent(m[2][0], m[2][1], m[2][2]);

    return theZComponent;
}

//==============================================================================
/**
 *		Rotate this matrix to face the specified matrix.
 *
 *		@param	inMatrix	The maxtirx we are to face.
 *		@return	This matrix after the rotation.
 */
CMatrix &CMatrix::Billboard(const CMatrix &inMatrix, bool inFreeRotateFlag /*= false*/)
{
    CVector3 theStartPos = GetTranslation();
    CVector3 theTargetPos = inMatrix.GetTranslation();
    CVector3 theUp = inMatrix.GetYComponent();
    CVector3 theDistance(theStartPos - theTargetPos);

    // Create the initial Z axis
    CVector3 theZ(theStartPos - theTargetPos);
    theZ.Normalize();

    // Create the X and Y
    CVector3 theY(theUp);
    CVector3 theX(theY.CrossProduct(theZ));

    // Billboard: Finalize the info and lock the Y axis (like a tree)
    if (!inFreeRotateFlag) {
        theZ = theX.CrossProduct(theY);
        theX.Normalize();
        theY.Normalize();
        theZ.Normalize();
    }

    // LookAt: Finalize the info with no locked axis (free rotate)
    else {
        theY = theZ.CrossProduct(theX);
        theX.Normalize();
        theY.Normalize();
    }

    // Copy it into the matrix
    m[0][0] = theX.x;
    m[0][1] = theX.y;
    m[0][2] = theX.z;

    m[1][0] = theY.x;
    m[1][1] = theY.y;
    m[1][2] = theY.z;

    m[2][0] = theZ.x;
    m[2][1] = theZ.y;
    m[2][2] = theZ.z;

    return *this;
}

//==============================================================================
/**
 *		Rotate this matrix to align with the rotation of the specified matrix.
 *
 *		@param	inMatrix		The maxtrix we are cloning.
 *		@param	inMirrorFlag	Flag indicating that we should flip the z and face the
 *opposite direction.
 *		@return	This matrix after the rotation.
 */
CMatrix &CMatrix::CloneRotation(const CMatrix &inMatrix, bool inMirrorFlag /*= false*/)
{
    // Create the axes
    CVector3 theZ(inMatrix.GetZComponent());
    if (inMirrorFlag)
        theZ *= -1;
    CVector3 theY(inMatrix.GetYComponent());
    CVector3 theX(theY.CrossProduct(theZ));

    // Normalize
    theX.Normalize();
    theY.Normalize();
    theZ.Normalize();

    // Copy it into the matrix
    m[0][0] = theX.x;
    m[0][1] = theX.y;
    m[0][2] = theX.z;

    m[1][0] = theY.x;
    m[1][1] = theY.y;
    m[1][2] = theY.z;

    m[2][0] = theZ.x;
    m[2][1] = theZ.y;
    m[2][2] = theZ.z;

    return *this;
}

//==============================================================================
/**
 *		Translates this matrix by the given x, y, and z and returns this.
 *
 *		@param	inX	X portion of vector to translate by
 *		@param	inY	Y portion of vector to translate by
 *		@param	inZ	Z portion of vector to translate by
 *
 *		@return	This matrix after the translation.
 */
CMatrix &CMatrix::Translate(const float &inX, const float &inY, const float &inZ)
{
    CVector3 theVector(inX, inY, inZ);

    return Translate(theVector);
}

//==============================================================================
/**
 *		Initializes this matrix as a Translation Matrix given a translation vector.
 *		NOTE: These were created to avoid temporary intermediate matrices.
 *		@param	inTranslate	the Vector that contains the Translation
 */
void CMatrix::SetTranslate(const CVector3 &inTranslate)
{
    Identity();

    // Set tranlate portion of matrix.
    m[3][0] = inTranslate.x;
    m[3][1] = inTranslate.y;
    m[3][2] = inTranslate.z;
}

//==============================================================================
/**
 *		Initializes this matrix as a Rotation Matrix given a rotation vector.
 *		NOTE: These were created to avoid temporary intermediate matrices.
 *		@param	inRotation	the Vector that contains the Rotation
 *		@param	inRotOrder	the rotation order (see CMatrix::ERotationOrder)
 *		@param	inOrientation	the handedness (see CMatrix::EOrientation)
 */
void CMatrix::SetRotate(const CRotation3 &inRotation, long inRotOrder, long inOrientation)
{
    if (inOrientation == CMatrix::RIGHT_HANDED) {
        // Convert to left-handed

        EulerAngleConverter::EulerAngles theEulerAngles;
        INT32 theRotationOrder;
        FormatEulerAngles(inRotOrder, -inRotation.x, -inRotation.y, -inRotation.z, theRotationOrder,
                          theEulerAngles.x, theEulerAngles.y, theEulerAngles.z);
        theEulerAngles.w = static_cast<FLOAT>(theRotationOrder);

        EulerAngleConverter::CEulerAngleConverter theConverter;
        theConverter.Eul_ToHMatrix(theEulerAngles, m);

        m[0][2] *= -1;
        m[1][2] *= -1;
        m[2][0] *= -1;
        m[2][1] *= -1;

        // Set tranlate portion of matrix.
        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;
    } else {

        // The original code
        // Left-handed hard-coded YXZ rotation.

        // Precalculate the sins and cosins.
        float theSinX = ::sin(inRotation.x);
        float theCosX = ::cos(inRotation.x);
        float theSinY = ::sin(inRotation.y);
        float theCosY = ::cos(inRotation.y);
        float theSinZ = ::sin(inRotation.z);
        float theCosZ = ::cos(inRotation.z);

        // Fill out the matrix.
        m[0][0] = (theCosZ * theCosY) + (theSinX * theSinY * theSinZ);
        m[0][1] = -theCosX * -theSinZ;
        m[0][2] = -(theSinY * theCosZ) + (theSinZ * theCosY * theSinX);

        m[1][0] = -(theSinZ * theCosY) + (theSinX * theSinY * theCosZ);
        m[1][1] = theCosX * theCosZ;
        m[1][2] = (theSinY * theSinZ) + (theSinX * theCosY * theCosZ);

        m[2][0] = theSinY * theCosX;
        m[2][1] = -theSinX;
        m[2][2] = theCosX * theCosY;

        // Set tranlate portion of matrix.
        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;

    } // Left-Handed
}

//==============================================================================
/**
*	Re-arrange euler angles information to a format useable by EulerAngleConverter
*	@param inRotationOrder		the rotation order enum from Studio
*	@param inXAngle				the x-axis rotation from Studio
*	@param inYAngle				the y-axis rotation from Studio
*	@param inZAngle				the z-axis rotation from Studio
*	@param outRotationOrder		the rotation order useable by EulerAngleConverter
*	@param outFirstAngle		the first euler angle used in EulerAngleConverter
*	@param outSecondAngle		the second euler angle used in EulerAngleConverter
*	@param outThirdAngle		the third euler angle used in EulerAngleConverter
*/
void CMatrix::FormatEulerAngles(INT32 inRotationOrder, FLOAT inXAngle, FLOAT inYAngle,
                                FLOAT inZAngle, INT32 &outRotationOrder, FLOAT &outFirstAngle,
                                FLOAT &outSecondAngle, FLOAT &outThirdAngle)
{
    if (inRotationOrder == XYZ) {
        outRotationOrder = EulOrdXYZs;
        outFirstAngle = inXAngle;
        outSecondAngle = inYAngle;
        outThirdAngle = inZAngle;
    } else if (inRotationOrder == YZX) {
        outRotationOrder = EulOrdYZXs;
        outFirstAngle = inYAngle;
        outSecondAngle = inZAngle;
        outThirdAngle = inXAngle;
    } else if (inRotationOrder == ZXY) {
        outRotationOrder = EulOrdZXYs;
        outFirstAngle = inZAngle;
        outSecondAngle = inXAngle;
        outThirdAngle = inYAngle;
    } else if (inRotationOrder == XZY) {
        outRotationOrder = EulOrdXZYs;
        outFirstAngle = inXAngle;
        outSecondAngle = inZAngle;
        outThirdAngle = inYAngle;
    } else if (inRotationOrder == YXZ) {
        outRotationOrder = EulOrdYXZs;
        outFirstAngle = inYAngle;
        outSecondAngle = inXAngle;
        outThirdAngle = inZAngle;
    } else if (inRotationOrder == ZYX) {
        outRotationOrder = EulOrdZYXs;
        outFirstAngle = inZAngle;
        outSecondAngle = inYAngle;
        outThirdAngle = inXAngle;
    }
    //
    else if (inRotationOrder == XYZr) {
        outRotationOrder = EulOrdXYZr;
        outFirstAngle = inXAngle;
        outSecondAngle = inYAngle;
        outThirdAngle = inZAngle;
    } else if (inRotationOrder == YZXr) {
        outRotationOrder = EulOrdYZXr;
        outFirstAngle = inYAngle;
        outSecondAngle = inZAngle;
        outThirdAngle = inXAngle;
    } else if (inRotationOrder == ZXYr) {
        outRotationOrder = EulOrdZXYr;
        outFirstAngle = inZAngle;
        outSecondAngle = inXAngle;
        outThirdAngle = inYAngle;
    } else if (inRotationOrder == XZYr) {
        outRotationOrder = EulOrdXZYr;
        outFirstAngle = inXAngle;
        outSecondAngle = inZAngle;
        outThirdAngle = inYAngle;
    } else if (inRotationOrder == YXZr) {
        outRotationOrder = EulOrdYXZr;
        outFirstAngle = inYAngle;
        outSecondAngle = inXAngle;
        outThirdAngle = inZAngle;
    } else if (inRotationOrder == ZYXr) {
        outRotationOrder = EulOrdZYXr;
        outFirstAngle = inZAngle;
        outSecondAngle = inYAngle;
        outThirdAngle = inXAngle;
    } else {
        // assert( false );
    }
}

//==============================================================================
/**
 *		Initializes this matrix as a Scale Matrix given the Scale Vector.
 *		NOTE: These were created to avoid temporary intermediate matrices.
 *		@param	inScale.
 */
void CMatrix::SetScale(const CVector3 &inScale)
{
    Identity();
    // Create a scaled identity matrix.
    m[0][0] = inScale.x;
    m[1][1] = inScale.y;
    m[2][2] = inScale.z;
}

//==============================================================================
/**
 *		Set this matrix to the all zeros.
 *
 *		@return		this matrix as a zero matrix.
 */
CMatrix &CMatrix::Zero()
{
    m[0][0] = 0;
    m[0][1] = 0;
    m[0][2] = 0;
    m[0][3] = 0;

    m[1][0] = 0;
    m[1][1] = 0;
    m[1][2] = 0;
    m[1][3] = 0;

    m[2][0] = 0;
    m[2][1] = 0;
    m[2][2] = 0;
    m[2][3] = 0;

    m[3][0] = 0;
    m[3][1] = 0;
    m[3][2] = 0;
    m[3][3] = 0;

    return *this;
}

//==============================================================================
/**
 *		Set this matrix to the Identity.
 *
 *		@return		this matrix as an identity matrix
 */
CMatrix &CMatrix::Identity()
{
    ::memcpy(&m, &IDENTITY_MATRIX, sizeof(m));

    return *this;
}

//==============================================================================
/**
 *	Sets the current matrix to describe a perspective matrix that produces
 *	a perspective projection. This is a DX style 0..1 clipping prjection matrix.
 *
 *	A = (r+l)/(r-l)
 *	B = (t+b)/(t-b)
 *	C = f/(f-n)
 *	D = -fn/(f-n)
 *
 *	2n/(r-l)	0			0		0
 *	0			2n/(t-b)	0		0
 *	A			B			C		1
 *	0			0			D		0
 */
CMatrix &CMatrix::Frustum(const bool &inOrthographic, const float &inNear, const float &inFar,
                          const float &inLeft, const float &inTop, const float &inRight,
                          const float &inBottom)
{
    float theWidth = static_cast<float>(inRight - inLeft);
    float theHeight = static_cast<float>(inBottom - inTop);
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
            m[3][2] = (-inFar * theClipRecip) + 1; //-(inFar+inNear) * theClipRecip;
            m[3][3] = 1.0;
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
/**
 *		Flip the matrix along its diagonal.
 *
 *		@return		this matrix, transposed.
 */
CMatrix &CMatrix::Transpose()
{
    CMatrix theHoldMatrix(*this);

    // Loop through and flip all the data on the matrix
    for (long theRow = 0; theRow < 4; ++theRow) {
        for (long theColumn = 0; theColumn < 4; ++theColumn) {
            // flip elements along the diagonal by reading from the hold matrix
            m[theRow][theColumn] = theHoldMatrix.m[theColumn][theRow];
        }
    }

    return *this;
}

//==============================================================================
/**
 *		Validates the the row and column being passed in does not overstep the
 *		matrix array bounds.
 *
 *		@param	inRow		The row index into the array
 *		@param	inColumn	The column index into the array
 *
 *		@return	true if the row/column is within range, otherwise false
 */
bool CMatrix::CheckBounds(const long &inRow, const long &inColumn) const
{
    return ((inRow < 4 && inRow >= 0) && (inColumn < 4 && inColumn >= 0));
}

//==============================================================================
/**
 *		Creates a translation matrix from the incoming vector.
 *
 *		@param	inVector	The vector from which the translation matrix is created
 *from.
 *
 *		@return	The translation matrix.
 */
CMatrix CMatrix::MakeTranslate(const CVector3 &inVector) const
{
    CMatrix theTranslation;

    // Set tranlate portion of matrix.
    theTranslation.m[3][0] = inVector.x;
    theTranslation.m[3][1] = inVector.y;
    theTranslation.m[3][2] = inVector.z;

    return theTranslation;
}

//==============================================================================
/**
 *		Creates a rotation matrix from the incoming vector.
 *
 *		@param	inRotation		The rotation vector from which the rotation matrix
 *is created from.
 *
 *		@return	The rotation matrix.
 */
CMatrix CMatrix::MakeRotate(const CRotation3 &inRotation) const
{
    CMatrix theRotation;

    // Precalculate the sins and cosins.
    float theSinX = (float)sin(inRotation.x);
    float theSinY = (float)sin(inRotation.y);
    float theSinZ = (float)sin(inRotation.z);
    float theCosX = (float)cos(inRotation.x);
    float theCosY = (float)cos(inRotation.y);
    float theCosZ = (float)cos(inRotation.z);

    // Fill out the matrix.
    theRotation.m[0][0] = (theCosZ * theCosY) + (theSinX * theSinY * theSinZ);
    theRotation.m[0][1] = -theCosX * -theSinZ;
    theRotation.m[0][2] = -(theSinY * theCosZ) + (theSinZ * theCosY * theSinX);

    theRotation.m[1][0] = -(theSinZ * theCosY) + (theSinX * theSinY * theCosZ);
    theRotation.m[1][1] = theCosX * theCosZ;
    theRotation.m[1][2] = (theSinY * theSinZ) + (theSinX * theCosY * theCosZ);

    theRotation.m[2][0] = theSinY * theCosX;
    theRotation.m[2][1] = -theSinX;
    theRotation.m[2][2] = theCosX * theCosY;

    return theRotation;
}

//==============================================================================
/**
 *		Creates a scale matrix from the incoming vector.
 *
 *		@param	inVector	The vector from which the scale matrix is created from.
 *
 *		@return	The scale matrix.
 */
CMatrix CMatrix::MakeScale(const CVector3 &inScale) const
{
    CMatrix theScale;

    // Create a scaled identity matrix.
    theScale.m[0][0] = inScale.x;
    theScale.m[1][1] = inScale.y;
    theScale.m[2][2] = inScale.z;

    return theScale;
}

//==============================================================================
/**
 *		Performs a matrix concatenation ( mulitplication ) of this matrix by the one passed
 *in.
 */
CMatrix CMatrix::Concatenate(const CMatrix &inMatrix) const
{
    CMatrix theCat;

    COptimizedArithmetic::MatrixMultiply(m[0], inMatrix.m[0], theCat.m[0]);
    /*
            theCat.m[ 0 ][ 0 ] = m[ 0 ][ 0 ] * inMatrix.m[ 0 ][ 0 ] + m[ 0 ][ 1 ] * inMatrix.m[ 1 ][
       0 ] + m[ 0 ][ 2 ] * inMatrix.m[ 2 ][ 0 ] + m[ 0 ][ 3 ] * inMatrix.m[ 3 ][ 0 ];
            theCat.m[ 0 ][ 1 ] = m[ 0 ][ 0 ] * inMatrix.m[ 0 ][ 1 ] + m[ 0 ][ 1 ] * inMatrix.m[ 1 ][
       1 ] + m[ 0 ][ 2 ] * inMatrix.m[ 2 ][ 1 ] + m[ 0 ][ 3 ] * inMatrix.m[ 3 ][ 1 ];
            theCat.m[ 0 ][ 2 ] = m[ 0 ][ 0 ] * inMatrix.m[ 0 ][ 2 ] + m[ 0 ][ 1 ] * inMatrix.m[ 1 ][
       2 ] + m[ 0 ][ 2 ] * inMatrix.m[ 2 ][ 2 ] + m[ 0 ][ 3 ] * inMatrix.m[ 3 ][ 2 ];
            theCat.m[ 0 ][ 3 ] = m[ 0 ][ 0 ] * inMatrix.m[ 0 ][ 3 ] + m[ 0 ][ 1 ] * inMatrix.m[ 1 ][
       3 ] + m[ 0 ][ 2 ] * inMatrix.m[ 2 ][ 3 ] + m[ 0 ][ 3 ] * inMatrix.m[ 3 ][ 3 ];

            theCat.m[ 1 ][ 0 ] = m[ 1 ][ 0 ] * inMatrix.m[ 0 ][ 0 ] + m[ 1 ][ 1 ] * inMatrix.m[ 1 ][
       0 ] + m[ 1 ][ 2 ] * inMatrix.m[ 2 ][ 0 ] + m[ 1 ][ 3 ] * inMatrix.m[ 3 ][ 0 ];
            theCat.m[ 1 ][ 1 ] = m[ 1 ][ 0 ] * inMatrix.m[ 0 ][ 1 ] + m[ 1 ][ 1 ] * inMatrix.m[ 1 ][
       1 ] + m[ 1 ][ 2 ] * inMatrix.m[ 2 ][ 1 ] + m[ 1 ][ 3 ] * inMatrix.m[ 3 ][ 1 ];
            theCat.m[ 1 ][ 2 ] = m[ 1 ][ 0 ] * inMatrix.m[ 0 ][ 2 ] + m[ 1 ][ 1 ] * inMatrix.m[ 1 ][
       2 ] + m[ 1 ][ 2 ] * inMatrix.m[ 2 ][ 2 ] + m[ 1 ][ 3 ] * inMatrix.m[ 3 ][ 2 ];
            theCat.m[ 1 ][ 3 ] = m[ 1 ][ 0 ] * inMatrix.m[ 0 ][ 3 ] + m[ 1 ][ 1 ] * inMatrix.m[ 1 ][
       3 ] + m[ 1 ][ 2 ] * inMatrix.m[ 2 ][ 3 ] + m[ 1 ][ 3 ] * inMatrix.m[ 3 ][ 3 ];

            theCat.m[ 2 ][ 0 ] = m[ 2 ][ 0 ] * inMatrix.m[ 0 ][ 0 ] + m[ 2 ][ 1 ] * inMatrix.m[ 1 ][
       0 ] + m[ 2 ][ 2 ] * inMatrix.m[ 2 ][ 0 ] + m[ 2 ][ 3 ] * inMatrix.m[ 3 ][ 0 ];
            theCat.m[ 2 ][ 1 ] = m[ 2 ][ 0 ] * inMatrix.m[ 0 ][ 1 ] + m[ 2 ][ 1 ] * inMatrix.m[ 1 ][
       1 ] + m[ 2 ][ 2 ] * inMatrix.m[ 2 ][ 1 ] + m[ 2 ][ 3 ] * inMatrix.m[ 3 ][ 1 ];
            theCat.m[ 2 ][ 2 ] = m[ 2 ][ 0 ] * inMatrix.m[ 0 ][ 2 ] + m[ 2 ][ 1 ] * inMatrix.m[ 1 ][
       2 ] + m[ 2 ][ 2 ] * inMatrix.m[ 2 ][ 2 ] + m[ 2 ][ 3 ] * inMatrix.m[ 3 ][ 2 ];
            theCat.m[ 2 ][ 3 ] = m[ 2 ][ 0 ] * inMatrix.m[ 0 ][ 3 ] + m[ 2 ][ 1 ] * inMatrix.m[ 1 ][
       3 ] + m[ 2 ][ 2 ] * inMatrix.m[ 2 ][ 3 ] + m[ 2 ][ 3 ] * inMatrix.m[ 3 ][ 3 ];

            theCat.m[ 3 ][ 0 ] = m[ 3 ][ 0 ] * inMatrix.m[ 0 ][ 0 ] + m[ 3 ][ 1 ] * inMatrix.m[ 1 ][
       0 ] + m[ 3 ][ 2 ] * inMatrix.m[ 2 ][ 0 ] + m[ 3 ][ 3 ] * inMatrix.m[ 3 ][ 0 ];
            theCat.m[ 3 ][ 1 ] = m[ 3 ][ 0 ] * inMatrix.m[ 0 ][ 1 ] + m[ 3 ][ 1 ] * inMatrix.m[ 1 ][
       1 ] + m[ 3 ][ 2 ] * inMatrix.m[ 2 ][ 1 ] + m[ 3 ][ 3 ] * inMatrix.m[ 3 ][ 1 ];
            theCat.m[ 3 ][ 2 ] = m[ 3 ][ 0 ] * inMatrix.m[ 0 ][ 2 ] + m[ 3 ][ 1 ] * inMatrix.m[ 1 ][
       2 ] + m[ 3 ][ 2 ] * inMatrix.m[ 2 ][ 2 ] + m[ 3 ][ 3 ] * inMatrix.m[ 3 ][ 2 ];
            theCat.m[ 3 ][ 3 ] = m[ 3 ][ 0 ] * inMatrix.m[ 0 ][ 3 ] + m[ 3 ][ 1 ] * inMatrix.m[ 1 ][
       3 ] + m[ 3 ][ 2 ] * inMatrix.m[ 2 ][ 3 ] + m[ 3 ][ 3 ] * inMatrix.m[ 3 ][ 3 ];
    */
    return theCat;
}

//==============================================================================
/**
 *	Inverts the matrix.
 *
 *	Attempts to invert this matrix.  If the matrix is singular, the inversion
 *	can not be done.
 *
 *	@return		true if the matrix was inverted, otherwise false if the matrix
 *				is singular.
 */
float CMatrix::Invert()
{
    float theDeterminant(0.0f);

    if (IsAffine()) {
        theDeterminant = InvertAffine();
    } else {
        theDeterminant = InvertNonSingular();
    }

    return theDeterminant;
}

//==============================================================================
/**
 *	Checks to see if this matrix is affine.
 *
 *														   T
 *	An affine matrix has the last column being [ 0 0 0 1 ]
 *
 *	@return		true if the matrix is affine, otherwise false.
 */
bool CMatrix::IsAffine() const
{
    return ((m[0][3] == 0.0 && m[1][3] == 0.0 && m[2][3] == 0.0 && m[3][3] == 1.0) ? true : false);
}

//==============================================================================
/**
 * Computes the inverse of a 3D affine matrix; i.e. a matrix with a dimen-
 * sionality of 4 where the right column has the entries (0, 0, 0, 1).
 *
 * This procedure treats the 4 by 4 matrix as a block matrix and
 * calculates the inverse of one submatrix for a significant perform-
 * ance improvement over a general procedure that can invert any non-
 * singular matrix:
 *       --      --          --         --
 *       |           | -1    |   -1      |
 *       | A       0 |       |  A      0 |
 *  -1   |           |       |           |
 * M  =  |           |   =   |     -1    |
 *       | C       1 |       | -C A    1 |
 *       |           |       |           |
 *       --         --       --         --
 *
 * where M is a 4 by 4 matrix,
 * A is the 3 by 3 upper left submatrix of M,
 * C is the 1 by 3 lower left submatrix of M.
 *
 * @return determinant of matrix
 *
 */
float CMatrix::InvertAffine()
{
    const float PRECISIONLIMIT(1.0e-07f);
    float theDeterminant(0.0f);
    float thePositiveDet(0.0f);
    float theNegativeDet(0.0f);
    float theTempDet(0.0f);
    CMatrix theInverse;

    /*
    * Calculate the determinant of submatrix A and determine if the
    * the matrix is singular as limited by the float precision.
    */
    theTempDet = m[0][0] * m[1][1] * m[2][2];
    if (theTempDet >= 0.0)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = m[0][1] * m[1][2] * m[2][0];
    if (theTempDet >= 0.0)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = m[0][2] * m[1][0] * m[2][1];
    if (theTempDet >= 0.0)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m[0][2] * m[1][1] * m[2][0];
    if (theTempDet >= 0.0)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m[0][1] * m[1][0] * m[2][2];
    if (theTempDet >= 0.0)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m[0][0] * m[1][2] * m[2][1];
    if (theTempDet >= 0.0)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theDeterminant = thePositiveDet + theNegativeDet;
    float theTrueDeterminate = theDeterminant; // Grab the old determinate from the otha sheeiit.

    // Is the submatrix A nonsingular? (i.e. there is an inverse?)
    if ((theDeterminant != 0.0)
        || (::abs(theDeterminant / (thePositiveDet - theNegativeDet)) >= PRECISIONLIMIT)) {
        // Calculate inverse(A) = adj(A) / det(A)
        theDeterminant = 1.0f / theDeterminant;
        theInverse.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * theDeterminant;
        theInverse.m[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * theDeterminant;
        theInverse.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * theDeterminant;
        theInverse.m[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * theDeterminant;
        theInverse.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * theDeterminant;
        theInverse.m[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * theDeterminant;
        theInverse.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * theDeterminant;
        theInverse.m[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * theDeterminant;
        theInverse.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * theDeterminant;

        // Calculate -C * inverse(A)
        theInverse.m[3][0] = -(m[3][0] * theInverse.m[0][0] + m[3][1] * theInverse.m[1][0]
                               + m[3][2] * theInverse.m[2][0]);
        theInverse.m[3][1] = -(m[3][0] * theInverse.m[0][1] + m[3][1] * theInverse.m[1][1]
                               + m[3][2] * theInverse.m[2][1]);
        theInverse.m[3][2] = -(m[3][0] * theInverse.m[0][2] + m[3][1] * theInverse.m[1][2]
                               + m[3][2] * theInverse.m[2][2]);

        // Fill in last column
        theInverse.m[0][3] = theInverse.m[1][3] = theInverse.m[2][3] = 0.0;
        theInverse.m[3][3] = 1.0;
        ;

        // Assing ourselves to the inverse
        *this = theInverse;
    }

    return theTrueDeterminate;
}

//==============================================================================
/**
 *	Inverts a nonsingular matrix.
 *
 *	@return	Determinant of matrix.
 */
float CMatrix::InvertNonSingular()
{
    return 0.0f;
}

//==============================================================================
/**
 *	Calculate the determinant of this 4x4 matrix.
 *
 *	@return		the determinant of this matrix.
 */
float CMatrix::Determinant() const
{
    return m[0][0] * Determinant3x3(m[1][1], m[2][1], m[3][1], m[1][2], m[2][2], m[3][2], m[1][3],
                                    m[2][3], m[3][3])
        - m[0][1] * Determinant3x3(m[1][0], m[2][0], m[3][0], m[1][2], m[2][2], m[3][2], m[1][3],
                                   m[2][3], m[3][3])
        + m[0][2] * Determinant3x3(m[1][0], m[2][0], m[3][0], m[1][1], m[2][1], m[3][1], m[1][3],
                                   m[2][3], m[3][3])
        - m[0][3] * Determinant3x3(m[1][0], m[2][0], m[3][0], m[1][1], m[2][1], m[3][1], m[1][2],
                                   m[2][2], m[3][2]);
}

//==============================================================================
/**
 *	Calculate the determinant of a 3x3 matrix in the form
 *
 * | inA1, inB1, inC1 |
 * | inA2, inB2, inC2 |
 * | inA3, inB3, inC3 |
 *
 *	@return		the derminant
 */
float CMatrix::Determinant3x3(const float &inA1, const float &inA2, const float &inA3,
                              const float &inB1, const float &inB2, const float &inB3,
                              const float &inC1, const float &inC2, const float &inC3) const
{
    return inA1 * Determinant2x2(inB2, inB3, inC2, inC3)
        - inB1 * Determinant2x2(inA2, inA3, inC2, inC3)
        + inC1 * Determinant2x2(inA2, inA3, inB2, inB3);
}

//==============================================================================
/**
 *	Calculate the determinant of a 2x2 matrix.
 *
 *	@return		the determinant of the matrix.
 */
float CMatrix::Determinant2x2(const float &inA, const float &inB, const float &inC,
                              const float &inD) const
{
    return inA * inD - inB * inC;
}

//==============================================================================
/**
 *	Checks to see if this matrix is the identity matrix
 *
 *	@return		true if this is the identity, otherwise false
 */
bool CMatrix::IsIdentity() const
{
    // Create and identity matrix
    CMatrix theIdentity;

    return (theIdentity == *this ? true : false);
}

} // namespace Q3DStudio
