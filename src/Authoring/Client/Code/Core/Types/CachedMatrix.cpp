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
#include "stdafx.h"
#include "CachedMatrix.h"
#include "OptimizedArithmetic.h"
#include <math.h>

namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor.
 */
CCachedMatrix::CCachedMatrix() /*:
	m_Translation( 0,0,0 ),
	m_Rotation( 0,0,0 ),
	m_Scale( 1,1,1 ),
	m_Pivot( 0,0,0 ),
	m_SinX( ::sin( 0.0f ) ),
	m_SinY( ::sin( 0.0f ) ),
	m_SinZ( ::sin( 0.0f ) ),
	m_CosX( ::cos( 0.0f ) ),
	m_CosY( ::cos( 0.0f ) ),
	m_CosZ( ::cos( 0.0f ) )*/
{
    Set(CVector3(0, 0, 0), CRotation3(0, 0, 0), CVector3(1, 1, 1), CVector3(0, 0, 0), true);
}

//==============================================================================
/**
 *	Private constructor - disallow this.
 */
CCachedMatrix::CCachedMatrix(const CMatrix &inMatrix)
    : CMatrix(inMatrix)
{
}

//==============================================================================
/**
 *	Private constructor - disallow this.
 */
CCachedMatrix::CCachedMatrix(float in00, float in01, float in02, float in03, float in10, float in11,
                             float in12, float in13, float in20, float in21, float in22, float in23,
                             float in30, float in31, float in32, float in33)
    : CMatrix(in00, in01, in02, in03, in10, in11, in12, in13, in20, in21, in22, in23, in30, in31,
              in32, in33)
{
}

//==============================================================================
/**
 *	Heavily optimized assignment method - and lazy too!
 *	This has the same result as creating four full 4x4 matrices and multiplying
 *	them together.  It's also cached meaning it won't recompute a matrix if
 *	the components are the same.  The whole method can be replaced with:
 *
 *	*this = m_ScaleMatrix * m_PivotMatrix * m_RotMatrix * m_TransMatrix;
 *
 *	@param	inTranslation	translation coordinates
 *	@param	inRotation		euler angle rotations
 *	@param	inScale			scaling dimensions
 *	@param	inPivot			pivot offset vector
 *	@return reference to this modified matrix
 */
void CCachedMatrix::Set(const CVector3 &inTranslation, const CRotation3 &inRotation,
                        const CVector3 &inScale, const CVector3 &inPivot, const long inRotOrder,
                        const long inOrientation, const bool inForce /*= false*/)
{
    bool theTranslationSame = !inForce && inTranslation.x == m_Translation.x
        && inTranslation.y == m_Translation.y && inTranslation.z == m_Translation.z;
    bool theRotationSame = !inForce && inRotation.x == m_Rotation.x && inRotation.y == m_Rotation.y
        && inRotation.z == m_Rotation.z;
    bool theScaleSame =
        !inForce && inScale.x == m_Scale.x && inScale.y == m_Scale.y && inScale.z == m_Scale.z;
    bool thePivotSame =
        !inForce && inPivot.x == m_Pivot.x && inPivot.y == m_Pivot.y && inPivot.z == m_Pivot.z;

    // Bail out if not change.  Manual comparison to use fast exact match and not overloaded
    // operator with epsilon tolerance
    if (theTranslationSame && theRotationSame && theScaleSame && thePivotSame)
        return;

    // Recreate the base matrices if needed
    if (!theTranslationSame) {
        m_TransMatrix.SetTranslate(inTranslation);
        m_Translation = inTranslation;
    }

    if (!theRotationSame) {
        m_RotMatrix.SetRotate(inRotation, inRotOrder, inOrientation);
        m_Rotation = inRotation;
    }

    if (!theScaleSame) {
        m_ScaleMatrix.SetScale(inScale);
        m_Scale = inScale;
    }

    if (!thePivotSame || !theScaleSame) {
        Q3DStudio::CVector3 theScaledPivot;
        theScaledPivot.x = -inPivot.x * inScale.x;
        theScaledPivot.y = -inPivot.y * inScale.y;
        theScaledPivot.z = -inPivot.z * inScale.z;
        m_PivotMatrix.SetTranslate(theScaledPivot);
        m_Pivot = inPivot;
    }

    // Update the transform.
    //	COptimizedArithmetic::MatrixMultiply( m_PivotMatrix.m[0], m_RotMatrix.m[0], m[0] );
    //	COptimizedArithmetic::MatrixMultiply( m[0], m_TransMatrix.m[0], m_PivotMatrix.m[0] );		//
    //put into the pivot as a temporary location
    //	COptimizedArithmetic::MatrixMultiply( m_ScaleMatrix.m[0], m_PivotMatrix.m[0], m[0] );

    if (!theRotationSame || !thePivotSame || !theScaleSame) {
        COptimizedArithmetic::MatrixMultiply(m_ScaleMatrix.m[0], m_PivotMatrix.m[0], m[0]);
        COptimizedArithmetic::MatrixMultiply(m[0], m_RotMatrix.m[0],
                                             m_ScalePivotRotationMatrix.m[0]);
    }

    if (!theTranslationSame || !theRotationSame || !thePivotSame || !theScaleSame)
        COptimizedArithmetic::MatrixMultiply(m_ScalePivotRotationMatrix.m[0], m_TransMatrix.m[0],
                                             m[0]);

    //	*this = m_ScaleMatrix * m_PivotMatrix * m_RotMatrix * m_TransMatrix;	// Distributed
    //original - plain speak
}

/*
// Generate these functions as inline code, not as function calls:
//#pragma intrinsic ( sin, cos, memcpy, memset )

// Recalculate X trig?
        if ( m_Rotation.x != inRotation.x )
        {
                m_Rotation.x = inRotation.x;
                m_SinX = ::sin( inRotation.x );
                m_CosX = ::cos( inRotation.x );
        }

        // Recalculate Y trig?
        if ( m_Rotation.y != inRotation.y )
        {
                m_Rotation.y = inRotation.y;
                m_SinY = ::sin( inRotation.y );
                m_CosY = ::cos( inRotation.y );
        }

        // Recalculate Z trig?
        if ( m_Rotation.z != inRotation.z )
        {
                m_Rotation.z = inRotation.z;
                m_SinZ = ::sin( inRotation.z );
                m_CosZ = ::cos( inRotation.z );
        }

        // Precalulate the scaled rotation 3x3 matrix and populate the rotation section of our 4x4
matrix
        float	r1 =	inScale.x * (( m_CosZ * m_CosY ) + ( m_SinX * m_SinY * m_SinZ ));
m[0][0] = r1;
        float	r4 =    inScale.x * ( m_CosX * m_SinZ );
m[0][1] = r4;
        float	r7 =  - inScale.x * (( m_SinY * m_CosZ ) + ( m_SinZ * m_CosY * m_SinX ));
m[0][2] = r7;
                                                                                                                                                                                m[0][3] = 0;

        float	r2 =  - inScale.y * (( m_SinZ * m_CosY ) + ( m_SinX * m_SinY * m_CosZ ));
m[1][0] = r2;
        float	r5 =	inScale.y * ( m_CosX * m_CosZ );
m[1][1] = r5;
        float	r8 =	inScale.y * (( m_SinY * m_SinZ ) + ( m_SinX * m_CosY * m_CosZ ));
m[1][2] = r8;
                                                                                                                                                                                m[1][3] = 0;

        float	r3 =	inScale.z * ( m_SinY * m_CosX );
m[2][0] = r3;
        float	r6 =  - inScale.z * ( m_SinX );
m[2][1] = r6;
        float	r9 =	inScale.z * ( m_CosX * m_CosY );
m[2][2] = r9;
                                                                                                                                                                                m[2][3] = 0;

        // Reuse the scaled rotation matrix in the translation/pivot section of out 4x4 matrix
        m[3][0] = inTranslation.x	- r1 * inPivot.x - r2 * inPivot.y - r3 * inPivot.z;
        m[3][1] = inTranslation.y	- r4 * inPivot.x - r5 * inPivot.y - r6 * inPivot.z;
        m[3][2] = inTranslation.z	- r7 * inPivot.x - r8 * inPivot.y - r9 * inPivot.z;
        m[3][3] = 1.0f;

        m_Translation = inTranslation;
        m_Rotation = inRotation;
        m_Scale = inScale;
        m_Pivot = inPivot;
*/

}; // namespace Q3DStudio
