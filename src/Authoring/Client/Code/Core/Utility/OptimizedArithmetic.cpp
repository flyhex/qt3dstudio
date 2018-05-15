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

#include "Qt3DSCommonPrecompile.h"

#include "OptimizedArithmetic.h"
#include "Matrix.h"
#include "Qt3DSMath.h"

#ifdef WIN32
#include "cpuid.h"
#endif

void SSE_MatrixMultiply(const float *inSource1, const float *inSource2, float *outDest);
void C_MatrixMultiply(const float *inSource1, const float *inSource2, float *outDest);
bool C_IntersectTriangle(bool inUseFrontface, bool inUseBackface, const float *inOrig,
                         const float *inDirection, const float *inTestV0, const float *inTestV1,
                         const float *inTestV2, float *outIntersection, float *outIntersectOffset);
void C_TransformBoundingBox(float *ioMinBox, float *ioMaxBox, const float *inTransform);

// Static initializers of all the available functions.
// These should all default to the most common implementation (C most likely).
FMatrixMultiply COptimizedArithmetic::MatrixMultiply(C_MatrixMultiply);
FIntersectTriangle COptimizedArithmetic::IntersectTriangle(C_IntersectTriangle);
FTransformBoundingBox COptimizedArithmetic::TransformBoundingBox(C_TransformBoundingBox);

// Static initializer which allows auto-initialization.
COptArithInitializer COptimizedArithmetic::m_Initializer;

//==============================================================================
/**
 * Constructor of the auto-initializer, this just initializes COptimizedArithmetic.
 * This should only be called automatically by the static on COptimizedArithmetic.
 */
COptArithInitializer::COptArithInitializer()
{
    COptimizedArithmetic::Initialize();
}

//==============================================================================
/**
 * Private constructor, does nothing. Unnecessary since the class is all static.
 */
COptimizedArithmetic::COptimizedArithmetic()
{
}

COptimizedArithmetic::~COptimizedArithmetic()
{
}

//==============================================================================
/**
 * Initializes all the potential optimized functions and determines which methods
 * to use.
 */
void COptimizedArithmetic::Initialize()
{
    if (HasSSE())
        MatrixMultiply = SSE_MatrixMultiply;
    else
        MatrixMultiply = C_MatrixMultiply;

    IntersectTriangle = C_IntersectTriangle;
    TransformBoundingBox = C_TransformBoundingBox;
}

//==============================================================================
/**
 * Checks to see if MMX is available.
 * This will check if the processor has the feature and the OS supports it.
 * @return true if MMX is supported.
 */
bool COptimizedArithmetic::HasMMX()
{
    bool hasMMX = false;
#ifdef WIN32
    _p_info info;
    _cpuid(&info);

    hasMMX = (info.feature & _CPU_FEATURE_MMX) && (info.checks & _CPU_FEATURE_MMX)
        && (info.os_support & _CPU_FEATURE_MMX);
#endif
    return hasMMX;
}

//==============================================================================
/**
 * Checks to see if SSE is available.
 * This will check if the processor has the feature and the OS supports it.
 * @return true if SSE is supported.
 */
bool COptimizedArithmetic::HasSSE()
{
    bool hasSSE = false;
#if defined(WIN32) && !defined(_AMD64_)
    _p_info info;
    _cpuid(&info);

    hasSSE = (info.feature & _CPU_FEATURE_SSE) && (info.checks & _CPU_FEATURE_SSE)
        && (info.os_support & _CPU_FEATURE_SSE);
#endif
    return hasSSE;
}

//==============================================================================
/**
 * Checks to see if SSE2 is available.
 * This will check if the processor has the feature and the OS supports it.
 * @return true if SSE2 is supported.
 */
bool COptimizedArithmetic::HasSSE2()
{
    bool hasSSE2 = false;
#if defined(WIN32) && !defined(_AMD64_)
    _p_info info;
    _cpuid(&info);

    hasSSE2 = (info.feature & _CPU_FEATURE_SSE2) && (info.checks & _CPU_FEATURE_SSE2)
        && (info.os_support & _CPU_FEATURE_SSE2);
#endif
    return hasSSE2;
}

//==============================================================================
/**
 * Checks to see if 3DNow! is available.
 * This will check if the processor has the feature and the OS supports it.
 * @return true if 3DNow! is supported.
 */
bool COptimizedArithmetic::Has3DNow()
{
    bool has3DNow = false;
#if defined(WIN32) && !defined(_AMD64_)
    _p_info info;
    _cpuid(&info);

    has3DNow = (info.feature & _CPU_FEATURE_3DNOW) && (info.checks & _CPU_FEATURE_3DNOW)
        && (info.os_support & _CPU_FEATURE_3DNOW);
#endif
    return has3DNow;
}

#if defined(WIN32) && !defined(_AMD64_) && !defined(__GNUC__)
//==============================================================================
/**
 * The following code is part of an SSE matrix multiplication algorithm developed
 * by Intel. The original document can be found at:
 * http://cedar.intel.com/media/pdf/appnotes/ap930/24504501.pdf
 * The document is titled: Streaming 'SIMD Extensions- Matrix Multiplication'
 */
//==============================================================================

// Offset for mat[i][j], w is an row width, t == 1 for transposed access.
#define mi(w, t, i, j) 4 * ((i * w + j) * (1 - t) + (j * w + i) * t)

// Load & multiply.
#define flm(k, i, j, m, n, a, b)                                                                   \
    __asm fld dword ptr[edx + mi(m, a, i, k)] __asm fmul dword ptr[ecx + mi(n, b, k, j)]

#define e4(i, j, l, m, n, a, b)                                                                    \
    flm(0, i, j, m, n, a, b) flm(1, i, j, m, n, a, b) flm(2, i, j, m, n, a, b)                     \
        flm(3, i, j, m, n, a, b) __asm faddp st(1),                                                \
        st(0) __asm fxch st(2) __asm faddp st(1), st(0) __asm faddp st(1),                         \
        st(0) __asm fstp dword ptr[eax + mi(l, 0, i, j)]

//==============================================================================
/**
 * Main entry point for the SSE Matrix Multiplication.
 * This is 4x4 matrix multiplication.
 *
 * @param inSource1 the first matrix to be multiplied (must be 16 floats long).
 * @param inSource2 the second matrix to be multiplied (must be 16 floats long).
 * @param outDestination where the result goes, this cannot be either of the sources.
 */
void SSE_MatrixMultiply(const float *inSource1, const float *inSource2, float *outDestination)
{
    __asm mov edx, DWORD PTR inSource1;
    __asm mov ecx, DWORD PTR inSource2;
    __asm mov eax, DWORD PTR outDestination;
    e4(0, 0, 4, 4, 4, 0, 0) e4(0, 1, 4, 4, 4, 0, 0) e4(0, 2, 4, 4, 4, 0, 0) e4(0, 3, 4, 4, 4, 0, 0)
        e4(1, 0, 4, 4, 4, 0, 0) e4(1, 1, 4, 4, 4, 0, 0) e4(1, 2, 4, 4, 4, 0, 0)
            e4(1, 3, 4, 4, 4, 0, 0) e4(2, 0, 4, 4, 4, 0, 0) e4(2, 1, 4, 4, 4, 0, 0)
                e4(2, 2, 4, 4, 4, 0, 0) e4(2, 3, 4, 4, 4, 0, 0) e4(3, 0, 4, 4, 4, 0, 0)
                    e4(3, 1, 4, 4, 4, 0, 0) e4(3, 2, 4, 4, 4, 0, 0) e4(3, 3, 4, 4, 4, 0, 0)
}
#else
// No SSE_MatrixMultiply on non-windows.
void SSE_MatrixMultiply(const float *inSource1, const float *inSource2, float *outDestination)
{
}
#endif

//==============================================================================
/**
 * Standard C MatrixMultiplication, copied from CMatrix::Concatenate.
 * This is 4x4 matrix multiplication.
 *
 * @param inSource1 the first matrix to be multiplied (must be 16 floats long).
 * @param inSource2 the second matrix to be multiplied (must be 16 floats long).
 * @param outDestination where the result goes, this cannot be either of the sources.
 */
void C_MatrixMultiply(const float *inSource1, const float *inSource2, float *outDest)
{
    outDest[0] = inSource1[0] * inSource2[0] + inSource1[1] * inSource2[4]
        + inSource1[2] * inSource2[8] + inSource1[3] * inSource2[12];
    outDest[1] = inSource1[0] * inSource2[1] + inSource1[1] * inSource2[5]
        + inSource1[2] * inSource2[9] + inSource1[3] * inSource2[13];
    outDest[2] = inSource1[0] * inSource2[2] + inSource1[1] * inSource2[6]
        + inSource1[2] * inSource2[10] + inSource1[3] * inSource2[14];
    outDest[3] = inSource1[0] * inSource2[3] + inSource1[1] * inSource2[7]
        + inSource1[2] * inSource2[11] + inSource1[3] * inSource2[15];

    outDest[4] = inSource1[4] * inSource2[0] + inSource1[5] * inSource2[4]
        + inSource1[6] * inSource2[8] + inSource1[7] * inSource2[12];
    outDest[5] = inSource1[4] * inSource2[1] + inSource1[5] * inSource2[5]
        + inSource1[6] * inSource2[9] + inSource1[7] * inSource2[13];
    outDest[6] = inSource1[4] * inSource2[2] + inSource1[5] * inSource2[6]
        + inSource1[6] * inSource2[10] + inSource1[7] * inSource2[14];
    outDest[7] = inSource1[4] * inSource2[3] + inSource1[5] * inSource2[7]
        + inSource1[6] * inSource2[11] + inSource1[7] * inSource2[15];

    outDest[8] = inSource1[8] * inSource2[0] + inSource1[9] * inSource2[4]
        + inSource1[10] * inSource2[8] + inSource1[11] * inSource2[12];
    outDest[9] = inSource1[8] * inSource2[1] + inSource1[9] * inSource2[5]
        + inSource1[10] * inSource2[9] + inSource1[11] * inSource2[13];
    outDest[10] = inSource1[8] * inSource2[2] + inSource1[9] * inSource2[6]
        + inSource1[10] * inSource2[10] + inSource1[11] * inSource2[14];
    outDest[11] = inSource1[8] * inSource2[3] + inSource1[9] * inSource2[7]
        + inSource1[10] * inSource2[11] + inSource1[11] * inSource2[15];

    outDest[12] = inSource1[12] * inSource2[0] + inSource1[13] * inSource2[4]
        + inSource1[14] * inSource2[8] + inSource1[15] * inSource2[12];
    outDest[13] = inSource1[12] * inSource2[1] + inSource1[13] * inSource2[5]
        + inSource1[14] * inSource2[9] + inSource1[15] * inSource2[13];
    outDest[14] = inSource1[12] * inSource2[2] + inSource1[13] * inSource2[6]
        + inSource1[14] * inSource2[10] + inSource1[15] * inSource2[14];
    outDest[15] = inSource1[12] * inSource2[3] + inSource1[13] * inSource2[7]
        + inSource1[14] * inSource2[11] + inSource1[15] * inSource2[15];
}

//==============================================================================
/**
 * Subtract inRHS from inLHS. This subtracts the values of each component of
 * inRHS from inLHS.
 * @param inLHS the vector that is to be subtracted from.
 * @param inRHS the vector that is to be subtracted.
 * @param outVect the result, this can be inLHS or inRHS.
 */
/*
static inline void C_VectorSubtract( const float* inLHS, const float* inRHS, float* outVect )
{
        outVect[0] = inLHS[0] - inRHS[0];
        outVect[1] = inLHS[1] - inRHS[1];
        outVect[2] = inLHS[2] - inRHS[2];
}
/*/
#define C_VectorSubtract(inLHS, inRHS, outVect)                                                    \
    {                                                                                              \
        outVect[0] = inLHS[0] - inRHS[0];                                                          \
        outVect[1] = inLHS[1] - inRHS[1];                                                          \
        outVect[2] = inLHS[2] - inRHS[2];                                                          \
    }

#define C_VectorSum(inLHS, inRHS, outVect)                                                         \
    {                                                                                              \
        outVect[0] = inLHS[0] + inRHS[0];                                                          \
        outVect[1] = inLHS[1] + inRHS[1];                                                          \
        outVect[2] = inLHS[2] + inRHS[2];                                                          \
    }

#define C_VectorCopy(inVect, outVect)                                                              \
    {                                                                                              \
        outVect[0] = outVect[0];                                                                   \
        outVect[1] = outVect[0];                                                                   \
        outVect[2] = outVect[0];                                                                   \
    }

/**/

//==============================================================================
/**
 * Calculates the dot product between the two vectors.
 * @param inVect1 the first vector to calculate the dot product with.
 * @param inVect2 the second vector to calculate the dot product with.
 * @return the dot product of the two vectors.
 */
/*
static inline float C_VectorDotProduct( const float* inVect1, const float* inVect2 )
{
        return inVect1[0] * inVect2[0] + inVect1[1] * inVect2[1] + inVect1[2] * inVect2[2];
}
/*/
#define C_VectorDotProduct(inVect1, inVect2)                                                       \
    (inVect1[0] * inVect2[0] + inVect1[1] * inVect2[1] + inVect1[2] * inVect2[2])
/**/

//==============================================================================
/**
 * Calculates the cross product of the two vectors.
 * @param inVector1 the first vector to calculate the cross product with.
 * @param inVector2
 */
/*
static inline void C_VectorCrossProduct( const float* inVector1, const float* inVector2, float*
outVect )
{
       outVect[0] = inVector1[1] * inVector2[2] - inVector1[2] * inVector2[1];
       outVect[1] = inVector1[2] * inVector2[0] - inVector1[0] * inVector2[2];
       outVect[2] = inVector1[0] * inVector2[1] - inVector1[1] * inVector2[0];
}
/*/
#define C_VectorCrossProduct(inVector1, inVector2, outVect)                                        \
    {                                                                                              \
        outVect[0] = inVector1[1] * inVector2[2] - inVector1[2] * inVector2[1];                    \
        outVect[1] = inVector1[2] * inVector2[0] - inVector1[0] * inVector2[2];                    \
        outVect[2] = inVector1[0] * inVector2[1] - inVector1[1] * inVector2[0];                    \
    }
/**/

//==============================================================================
/**
 * Scales a vector by inScale amount.
 * @param inVector the vector to scale.
 * @param inScale the amount to scale the vector.
 * @param outVect the result, this can be same vector as inVector.
 */
static inline void C_VectorScale(const float *inVector, float inScale, float *outVect)
{
    outVect[0] = inVector[0] * inScale;
    outVect[1] = inVector[1] * inScale;
    outVect[2] = inVector[2] * inScale;
}

//==============================================================================
/**
 * Set the components of one vector to the components of another vector.
 * @param outDest the vector to be set.
 * @param inSource the value to set outDest as.
 */
/*
static inline void C_SetVector( float* outDest, const float* inSource )
{
        outDest[0] = inSource[0];
        outDest[1] = inSource[1];
        outDest[2] = inSource[2];
}
/*/
#define C_SetVector(outDest, inSource)                                                             \
    {                                                                                              \
        outDest[0] = inSource[0];                                                                  \
        outDest[1] = inSource[1];                                                                  \
        outDest[2] = inSource[2];                                                                  \
    }
/**/

//==============================================================================
/**
 * Transform the vector by the given matrix.
 * The martrix is row column of the form (Matrix[row][column]):
 *
 *			| m0	m1	m2	w |
 *			| m4	m5	m6	w |
 *			| m8	m9	m10	w |
 *			| tX	tY	tZ	w |
 *
 * @param ioVector the input data and the return data, this vector will be modified.
 * @param inMatrix the transform matrix.
 * @param inTranslate is false if you only want to rotate the vector
 */
static inline void C_VectorTransform(float *ioVector, const float *inMatrix,
                                     bool inTranslate = true)
{
    float theX = ioVector[0];
    float theY = ioVector[1];
    float theZ = ioVector[2];
    float theW = inTranslate ? 1.0f : 0;

    ioVector[0] =
        theX * inMatrix[0] + theY * inMatrix[4] + theZ * inMatrix[8] + theW * inMatrix[12];
    ioVector[1] =
        theX * inMatrix[1] + theY * inMatrix[5] + theZ * inMatrix[9] + theW * inMatrix[13];
    ioVector[2] =
        theX * inMatrix[2] + theY * inMatrix[6] + theZ * inMatrix[10] + theW * inMatrix[14];
}
/*/
#define C_VectorTransform( ioVector, inMatrix, inTranslate ) {
\
        float theX = ioVector[0];
\
        float theY = ioVector[1];
\
        float theZ = ioVector[2];
\
        float theW = ( inTranslate ? 1.0f : 0 );
\
                                                                                                                                                                                                                                                        \
        ioVector[0] = theX * inMatrix[ 0 ] + theY * inMatrix[ 4 ] + theZ * inMatrix[ 8 ] + theW *
inMatrix[ 12 ];				\
        ioVector[1] = theX * inMatrix[ 1 ] + theY * inMatrix[ 5 ] + theZ * inMatrix[ 9 ] + theW *
inMatrix[ 13 ];				\
        ioVector[2] = theX * inMatrix[ 2 ] + theY * inMatrix[ 6 ] + theZ * inMatrix[ 10 ] + theW *
inMatrix[ 14 ];				\
}
/**/

//==============================================================================
/**
 * Modified ioDestination to contain components that are the maximum between
 * ioDestination and inComparison.
 * @param ioDestination the source of the comparison, also the result of the operation.
 * @param inComparison the vector whose components are to be compared.
 */
/*
static inline void C_VectorMaximize( float* ioDestination, const float* inComparison )
{
        if ( ioDestination[0] < inComparison[0] )
                ioDestination[0] = inComparison[0];
        if ( ioDestination[1] < inComparison[1] )
                ioDestination[1] = inComparison[1];
        if ( ioDestination[2] < inComparison[2] )
                ioDestination[2] = inComparison[2];
}
/*/
#define C_VectorMaximize(ioDestination, inComparison)                                              \
    {                                                                                              \
        if (ioDestination[0] < inComparison[0])                                                    \
            ioDestination[0] = inComparison[0];                                                    \
        if (ioDestination[1] < inComparison[1])                                                    \
            ioDestination[1] = inComparison[1];                                                    \
        if (ioDestination[2] < inComparison[2])                                                    \
            ioDestination[2] = inComparison[2];                                                    \
    }
/**/

//==============================================================================
/**
 * Modified ioDestination to contain components that are the minimum between
 * ioDestination and inComparison.
 * @param ioDestination the source of the comparison, also the result of the operation.
 * @param inComparison the vector whose components are to be compared.
 */
/*
static inline void C_VectorMinimize( float* ioDestination, const float* inComparison )
{
        if ( ioDestination[0] > inComparison[0] )
                ioDestination[0] = inComparison[0];
        if ( ioDestination[1] > inComparison[1] )
                ioDestination[1] = inComparison[1];
        if ( ioDestination[2] > inComparison[2] )
                ioDestination[2] = inComparison[2];
}
/*/
#define C_VectorMinimize(ioDestination, inComparison)                                              \
    {                                                                                              \
        if (ioDestination[0] > inComparison[0])                                                    \
            ioDestination[0] = inComparison[0];                                                    \
        if (ioDestination[1] > inComparison[1])                                                    \
            ioDestination[1] = inComparison[1];                                                    \
        if (ioDestination[2] > inComparison[2])                                                    \
            ioDestination[2] = inComparison[2];                                                    \
    }
/**/

//==============================================================================
/*
 * IntersectTriangle: Check the poly (defined by the 3 verticies) for intersection.
 *
 * Given a ray origin (orig) and direction (dir), and three vertices of
 * of a triangle, this function returns true and the interpolated texture
 * coordinates if the ray intersects the triangle
 *
 * We use the parametric equation of plane relative to trigon T. To solve for the values of the
 *parametric variables, we
 * utilise the calculation of a generalized perp operator on plane which is defined as:
 * Given plane Pi and vector in plane A:
 * Perp( A ) lies in Pi and is perpendicular to A
 *
 * For further details, please refer to the reference links
 *
 * @param inUseBackface	bool				This is a vertex of the poly face.
 * @param inOrig			float[3]		This is the origin of the Pickray (3D
 *space).
 * @param inDirection		float[3]		This is the direction of the Pickray.
 * @param inTestV0			float[3]		This is a vertex of the poly face.
 * @param inTestV1			float[3]		This is a vertex of the poly face.
 * @param inTestV2			float[3]		This is a vertex of the poly face.
 * @param outIntersection	float[3]		This will return the point on the ray where
 *intersection occured.
 * @param outIntersectOffset float			outIntersection = inOrig + theIntersectionOffset
 ** inDirection;
 *
 * @return	true if intersection occured. false otherwise.
 *
 *	References
 *	http://geometryalgorithms.com/Archive/algorithm_0105/algorithm_0105.htm
 *	http://geometryalgorithms.com/Archive/algorithm_0105/algorithm_0105.htm#intersect_RayTriangle()
 *
 *	Didier Badouel, "An Efficient Ray-Polygon Intersection" in Graphics Gems (1990)
 *	Francis Hill, "The Pleasures of 'Perp Dot' Products" in Graphics Gems IV (1994)
 *	[Note: the first critical definition has a typo, and should be: a^ = (-ay, ax).]
 *	Tomas Moller & Eric Haines, Real-Time Rendering, Chapter 10 "Intersection Test Methods"
 *(1999)
 *	Tomas Moller & Ben Trumbore, "Fast, Minimum Storage Ray-Triangle Intersection",  J. Graphics
 *Tools 2(1), 21-28 (1997)
 *	Joseph O'Rourke, Computational Geometry in C (2nd Edition), Section 7.3 "Segment-Triangle
 *Intersection" (1998)
 *	J.P. Snyder and A.H. Barr, "Ray Tracing Complex Models Containing Surface Tessellations",
 *ACM Comp Graphics 21, (1987)
 */
bool C_IntersectTriangle(bool inUseFrontface, bool inUseBackface, const float *inOrig,
                         const float *inDirection, const float *inTestV0, const float *inTestV1,
                         const float *inTestV2, float *outIntersection, float *outIntersectOffset)
{
    // Holds the return value
    // We've split this into 2 data bits: "DoContinue" and "Return"
    // DoContinue  | Return
    //		0			0			= 0
    //		0			1			= 1
    //		1			0			= 2
    //		1			1			= 3
    long theReturn = 2;

    // trigon vectors :
    // triangle edge vectors and plane normal
    float theTEdge1[3];
    C_VectorSubtract(inTestV1, inTestV0, theTEdge1);
    float theTEdge2[3];
    C_VectorSubtract(inTestV2, inTestV0, theTEdge2);
    float theTNormal[3];
    C_VectorCrossProduct(theTEdge1, theTEdge2, theTNormal);

    // Check for backface, give up if we are current checking a backfacing trigon and is not
    // required
    if ((!inUseFrontface && C_VectorDotProduct(theTNormal, inDirection) < -Q3DStudio::SQREPSILON)
        || (!inUseBackface && C_VectorDotProduct(theTNormal, inDirection) > Q3DStudio::SQREPSILON)) {
        // the trigon is facing the wrong way
        // DoContinue = false;
        // Return = false;
        theReturn = 0;
    }

    if (theReturn && !theTNormal[0] && !theTNormal[1] && !theTNormal[2]) {
        // the trigon is not well formed
        theReturn = 0;
    }

    if (theReturn) {
        float theVV0Orig[3]; // vector from V0 to Orig
        C_VectorSubtract(inOrig, inTestV0, theVV0Orig);
        float theDot0 = C_VectorDotProduct(theVV0Orig, theTNormal)
            * -1; // the dot prod of theVVOrig & theTNormal
        float theDot1 =
            C_VectorDotProduct(inDirection, theTNormal); // the dot prod of inDirection & theTNormal
        *outIntersectOffset = theDot0 / theDot1; // something to hold theDot0 / theDot1

        if (theReturn >= 2) // ok to continue
        {
            if (::abs(theDot1) < Q3DStudio::SQREPSILON) {
                // ray is parallel to triangle plane
                if (theDot0 == 0) {
                    // ray lies in triangle plane
                    // return the Orig ( correct me if i m wrong here )
                    C_VectorCopy(inOrig, outIntersection);

                    // DoContinue = false;
                    // Return = true;
                    theReturn = 1;
                } else {
                    // ray disjoint from plane
                    // DoContinue = false;
                    // Return = false;
                    theReturn = 0;
                }
            }
        }

        if (theReturn >= 2) // ok to continue
        {
            // get intersect point of ray with triangle plane
            if (*outIntersectOffset < -Q3DStudio::EPSILON) {
                // ray goes away from triangle => no intersect
                // DoContinue = false;
                // Return = false;
                theReturn = 0;
            }
        }

        if (theReturn >= 2) // ok to continue
        {
            // for a segment, also test if (r > 1.0) => no intersect
            // outIntersection = inOrig + theIntersectionOffset * inDirection;
            C_VectorScale(inDirection, *outIntersectOffset, outIntersection);
            C_VectorSum(inOrig, outIntersection, outIntersection);

            // is outIntersection inside the trigon?
            float theEdge1Edge1 = C_VectorDotProduct(theTEdge1, theTEdge1);
            float theEdge1Edge2 = C_VectorDotProduct(theTEdge1, theTEdge2);
            float theEdge2Edge2 = C_VectorDotProduct(theTEdge2, theTEdge2);
            float theVV0Isect[3];
            C_VectorSubtract(outIntersection, inTestV0, theVV0Isect);
            float theV0IsectEdge2 = C_VectorDotProduct(theVV0Isect, theTEdge2);
            float theV0IsectEdge1 = C_VectorDotProduct(theVV0Isect, theTEdge1);
            float theDiff = theEdge1Edge2 * theEdge1Edge2 - theEdge1Edge1 * theEdge2Edge2;

            // get and test s, t parametric coords
            // for details, refer to link
            float theS, theT;
            // s = (uv * wv - vv * wu) / D;
            theS = (theEdge1Edge2 * theV0IsectEdge2 - theEdge2Edge2 * theV0IsectEdge1) / theDiff;

            if (theS < -Q3DStudio::EPSILON || theS > (1.0f + Q3DStudio::EPSILON)) {
                // theIntersection is outside trigon
                // Return = false;
                // DoContinue = false;
                theReturn = 0;
            }

            // t = (uv * wu - uu * wv) / D;
            theT = (theEdge1Edge2 * theV0IsectEdge1 - theEdge1Edge1 * theV0IsectEdge2) / theDiff;

            if (theT < -Q3DStudio::EPSILON || (theS + theT) > (1.0 + Q3DStudio::EPSILON)) {
                // theIntersection is outside trigon
                // Return = false;
                // DoContinue = false;
                theReturn = 0;
            }

            if (theReturn >= 2) {
                theReturn = 1; // just to make sure our return value is true at this point
            }
        }
    }

    // returns true or false, we are interested in only the "Return" bit
    return 1 == (theReturn & 1);
}

//==============================================================================
/**
 *	Transforms the bounding box using the given matrix.  The resulting box
 *	is guaranteed to keep the min/max corners min/maxed even of the
 *	applied matrix results in a rotation that flips one or more corners.
 *	Note that each corner has to be translated individually and all
 *	eight corners have to be processed to spen the new box.
 *
 *	@param ioMinBox on input the local min, on output the global min.
 *	@param ioMaxBox on input the local max, on output the global max.
 *	@param inTransform is the matrix that will apply the transform
 */
void C_TransformBoundingBox(float *ioMinBox, float *ioMaxBox, const float *inTransform)
{
    float theCorner[3];

    // Store the original min and max box values
    float theOrigMinBox[3];
    theOrigMinBox[0] = ioMinBox[0];
    theOrigMinBox[1] = ioMinBox[1];
    theOrigMinBox[2] = ioMinBox[2];

    float theOrigMaxBox[3];
    theOrigMaxBox[0] = ioMaxBox[0];
    theOrigMaxBox[1] = ioMaxBox[1];
    theOrigMaxBox[2] = ioMaxBox[2];

    // Reset bounding box to be empty which is different than the box having both corners at 0,0,0.
    ioMinBox[0] = FLT_MAX;
    ioMinBox[1] = FLT_MAX;
    ioMinBox[2] = FLT_MAX;
    ioMaxBox[0] = -FLT_MAX;
    ioMaxBox[1] = -FLT_MAX;
    ioMaxBox[2] = -FLT_MAX;

    // Create and test the implicit 8 corners of the box
    C_SetVector(theCorner, theOrigMaxBox);
    // transform the corner by the specified transform
    C_VectorTransform(theCorner, inTransform);
    // check and modify the min box.
    C_VectorMinimize(ioMinBox, theCorner);
    // check and modify the max box.
    C_VectorMaximize(ioMaxBox, theCorner);

    // Repeat for each corner.

    C_SetVector(theCorner, theOrigMinBox);
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);

    C_SetVector(theCorner, theOrigMaxBox);
    theCorner[0] = theOrigMinBox[0];
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);

    C_SetVector(theCorner, theOrigMaxBox);
    theCorner[1] = theOrigMinBox[1];
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);

    C_SetVector(theCorner, theOrigMaxBox);
    theCorner[2] = theOrigMinBox[2];
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);

    C_SetVector(theCorner, theOrigMinBox);
    theCorner[0] = theOrigMaxBox[0];
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);

    C_SetVector(theCorner, theOrigMinBox);
    theCorner[1] = theOrigMaxBox[1];
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);

    C_SetVector(theCorner, theOrigMinBox);
    theCorner[2] = theOrigMaxBox[2];
    C_VectorTransform(theCorner, inTransform);
    C_VectorMinimize(ioMinBox, theCorner);
    C_VectorMaximize(ioMaxBox, theCorner);
}
