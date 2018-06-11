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
#include <string.h>
#include "EulerAngles.h"

namespace EulerAngleConverter {

//==============================================================================
//	Ctor and Dtor
//==============================================================================
//==============================================================================
/**
*		Ctor
*/
CEulerAngleConverter::CEulerAngleConverter()
{
    m_OrderInfoBuffer[0] = '\0';
}

//==============================================================================
/**
*		Dtor
*/
CEulerAngleConverter::~CEulerAngleConverter()
{
}

//==============================================================================
//	Interfaces
//==============================================================================
//==============================================================================
/**
*		Constructs a Euler angle & holds it in a EulerAngles struct
*
*		@param ai, aj, ah = x rot, y rot, z rot ( radians )
*		@param order = the order this angle is in namely XYZ( static ), etc.
*					   use the EulOrd**** macros to generate values
*					   0 to 23 is valid
*/
EulerAngles CEulerAngleConverter::Eul_(float ai, float aj, float ah, int order)
{
    EulerAngles ea;
    ea.x = ai;
    ea.y = aj;
    ea.z = ah;
    ea.w = (float)order;
    return (ea);
}

//==============================================================================
/**
*		Construct quaternion from Euler angles (in radians).
*
*		@param ea = incoming angle( radians )
*/
Quat CEulerAngleConverter::Eul_ToQuat(EulerAngles ea)
{
    Quat qu;
    double a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;
    EulGetOrd((unsigned int)ea.w, i, j, k, h, n, s, f);
    if (f == EulFrmR) {
        float t = ea.x;
        ea.x = ea.z;
        ea.z = t;
    }
    if (n == EulParOdd)
        ea.y = -ea.y;
    ti = ea.x * 0.5;
    tj = ea.y * 0.5;
    th = ea.z * 0.5;
    ci = cos(ti);
    cj = cos(tj);
    ch = cos(th);
    si = sin(ti);
    sj = sin(tj);
    sh = sin(th);
    cc = ci * ch;
    cs = ci * sh;
    sc = si * ch;
    ss = si * sh;
    if (s == EulRepYes) {
        a[i] = cj * (cs + sc); /* Could speed up with */
        a[j] = sj * (cc + ss); /* trig identities. */
        a[k] = sj * (cs - sc);
        qu.w = (float)(cj * (cc - ss));
    } else {
        a[i] = cj * sc - sj * cs;
        a[j] = cj * ss + sj * cc;
        a[k] = cj * cs - sj * sc;
        qu.w = (float)(cj * cc + sj * ss);
    }
    if (n == EulParOdd)
        a[j] = -a[j];
    qu.x = (float)a[X];
    qu.y = (float)a[Y];
    qu.z = (float)a[Z];
    return (qu);
}

//==============================================================================
/**
*		Construct matrix from Euler angles (in radians).
*
*		@param ea = incoming angle
*		@param M = outgoing matrix
*/
void CEulerAngleConverter::Eul_ToHMatrix(EulerAngles ea, HMatrix M)
{
    double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;
    EulGetOrd((unsigned int)ea.w, i, j, k, h, n, s, f);
    if (f == EulFrmR) {
        float t = ea.x;
        ea.x = ea.z;
        ea.z = t;
    }
    if (n == EulParOdd) {
        ea.x = -ea.x;
        ea.y = -ea.y;
        ea.z = -ea.z;
    }
    ti = ea.x;
    tj = ea.y;
    th = ea.z;
    ci = cos(ti);
    cj = cos(tj);
    ch = cos(th);
    si = sin(ti);
    sj = sin(tj);
    sh = sin(th);
    cc = ci * ch;
    cs = ci * sh;
    sc = si * ch;
    ss = si * sh;
    if (s == EulRepYes) {
        M[i][i] = (float)cj;
        M[i][j] = (float)(sj * si);
        M[i][k] = (float)(sj * ci);
        M[j][i] = (float)(sj * sh);
        M[j][j] = (float)(-cj * ss + cc);
        M[j][k] = (float)(-cj * cs - sc);
        M[k][i] = (float)(-sj * ch);
        M[k][j] = (float)(cj * sc + cs);
        M[k][k] = (float)(cj * cc - ss);
    } else {
        M[i][i] = (float)(cj * ch);
        M[i][j] = (float)(sj * sc - cs);
        M[i][k] = (float)(sj * cc + ss);
        M[j][i] = (float)(cj * sh);
        M[j][j] = (float)(sj * ss + cc);
        M[j][k] = (float)(sj * cs - sc);
        M[k][i] = (float)(-sj);
        M[k][j] = (float)(cj * si);
        M[k][k] = (float)(cj * ci);
    }
    M[W][X] = M[W][Y] = M[W][Z] = M[X][W] = M[Y][W] = M[Z][W] = 0.0;
    M[W][W] = 1.0;
}

//==============================================================================
/**
*		Convert matrix to Euler angles (in radians).
*
*		@param M = incoming matrix
*		@param order = 0-23, use EulOrd**** to generate this value
*		@return a set of angles in radians!!!!
*/
EulerAngles CEulerAngleConverter::Eul_FromHMatrix(HMatrix M, int order)
{
    EulerAngles ea;
    int i, j, k, h, n, s, f;
    EulGetOrd(order, i, j, k, h, n, s, f);
    if (s == EulRepYes) {
        double sy = sqrt(M[i][j] * M[i][j] + M[i][k] * M[i][k]);
        if (sy > 16 * FLT_EPSILON) {
            ea.x = (float)(atan2((double)M[i][j], (double)M[i][k]));
            ea.y = (float)(atan2((double)sy, (double)M[i][i]));
            ea.z = (float)(atan2((double)M[j][i], -(double)M[k][i]));
        } else {
            ea.x = (float)(atan2(-(double)M[j][k], (double)M[j][j]));
            ea.y = (float)(atan2((double)sy, (double)M[i][i]));
            ea.z = 0;
        }
    } else {
        double cy = sqrt(M[i][i] * M[i][i] + M[j][i] * M[j][i]);
        if (cy > 16 * FLT_EPSILON) {
            ea.x = (float)(atan2((double)M[k][j], (double)M[k][k]));
            ea.y = (float)(atan2(-(double)M[k][i], (double)cy));
            ea.z = (float)(atan2((double)M[j][i], (double)M[i][i]));
        } else {
            ea.x = (float)(atan2(-(double)M[j][k], (double)M[j][j]));
            ea.y = (float)(atan2(-(double)M[k][i], (double)cy));
            ea.z = 0;
        }
    }
    if (n == EulParOdd) {
        ea.x = -ea.x;
        ea.y = -ea.y;
        ea.z = -ea.z;
    }
    if (f == EulFrmR) {
        float t = ea.x;
        ea.x = ea.z;
        ea.z = t;
    }
    ea.w = (float)order;
    return (ea);
}

//==============================================================================
/**
*		Convert quaternion to Euler angles (in radians).
*
*		@param q = incoming quaternion
*		@param order = 0-23, use EulOrd**** to generate this value
*		@return the generated angles ( radians )
*/
EulerAngles CEulerAngleConverter::Eul_FromQuat(Quat q, int order)
{
    HMatrix M;
    double Nq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
    double xs = q.x * s, ys = q.y * s, zs = q.z * s;
    double wx = q.w * xs, wy = q.w * ys, wz = q.w * zs;
    double xx = q.x * xs, xy = q.x * ys, xz = q.x * zs;
    double yy = q.y * ys, yz = q.y * zs, zz = q.z * zs;
    M[X][X] = (float)(1.0 - (yy + zz));
    M[X][Y] = (float)(xy - wz);
    M[X][Z] = (float)(xz + wy);
    M[Y][X] = (float)(xy + wz);
    M[Y][Y] = (float)(1.0 - (xx + zz));
    M[Y][Z] = (float)(yz - wx);
    M[Z][X] = (float)(xz - wy);
    M[Z][Y] = (float)(yz + wx);
    M[Z][Z] = (float)(1.0 - (xx + yy));
    M[W][X] = M[W][Y] = M[W][Z] = M[X][W] = M[Y][W] = M[Z][W] = 0.0;
    M[W][W] = 1.0;
    return (Eul_FromHMatrix(M, order));
}

//==============================================================================
/**
*		Dump the Order information
*
*		@param outStr = the holding string
*/
const char *CEulerAngleConverter::DumpOrderInfo()
{
    long theCount = 0;
    long theOrder[24];
    char theOrderStr[24][16];

    ::strcpy(theOrderStr[theCount++], "EulOrdXYZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdXYXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXYXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXZr");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXZr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXYZr");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYZr");

    theCount = 0;
    theOrder[theCount++] = EulOrdXYZs;
    theOrder[theCount++] = EulOrdXYXs;
    theOrder[theCount++] = EulOrdXZYs;
    theOrder[theCount++] = EulOrdXZXs;
    theOrder[theCount++] = EulOrdYZXs;
    theOrder[theCount++] = EulOrdYZYs;
    theOrder[theCount++] = EulOrdYXZs;
    theOrder[theCount++] = EulOrdYXYs;
    theOrder[theCount++] = EulOrdZXYs;
    theOrder[theCount++] = EulOrdZXZs;
    theOrder[theCount++] = EulOrdZYXs;
    theOrder[theCount++] = EulOrdZYZs;

    theOrder[theCount++] = EulOrdZYXr;
    theOrder[theCount++] = EulOrdXYXr;
    theOrder[theCount++] = EulOrdYZXr;
    theOrder[theCount++] = EulOrdXZXr;
    theOrder[theCount++] = EulOrdXZYr;
    theOrder[theCount++] = EulOrdYZYr;
    theOrder[theCount++] = EulOrdZXYr;
    theOrder[theCount++] = EulOrdYXYr;
    theOrder[theCount++] = EulOrdYXZr;
    theOrder[theCount++] = EulOrdZXZr;
    theOrder[theCount++] = EulOrdXYZr;
    theOrder[theCount++] = EulOrdZYZr;

    char theSubBuf[256];
    ::sprintf(m_OrderInfoBuffer, "");
    for (long theIndex = 0; theIndex < 24; ++theIndex) {
        ::sprintf(theSubBuf, " %16s - %ld\n ", theOrderStr[theIndex], theOrder[theIndex]);
        ::strcat(m_OrderInfoBuffer, theSubBuf);
    }

    return m_OrderInfoBuffer;
}

//==============================================================================
/**
*		Converts a quaternion to a 4x4 matrix
*		http://skal.planet-d.net/matrixfaq.html
*
*		@param inQuat == the quaternion
*		@param outMatrix == the matrix generated
*/
void ConvertQuatToMatrix(double inQuat[4], double outMatrix[16])
{
    const long theQuatX = 0;
    const long theQuatY = 1;
    const long theQuatZ = 2;
    const long theQuatW = 3;

    const double theXX = inQuat[theQuatX] * inQuat[theQuatX];
    const double theXY = inQuat[theQuatX] * inQuat[theQuatY];
    const double theXZ = inQuat[theQuatX] * inQuat[theQuatZ];
    const double theXW = inQuat[theQuatX] * inQuat[theQuatW];
    const double theYY = inQuat[theQuatY] * inQuat[theQuatY];
    const double theYZ = inQuat[theQuatY] * inQuat[theQuatZ];
    const double theYW = inQuat[theQuatY] * inQuat[theQuatW];
    const double theZZ = inQuat[theQuatZ] * inQuat[theQuatZ];
    const double theZW = inQuat[theQuatZ] * inQuat[theQuatW];

    outMatrix[0] = 1 - 2 * (theYY + theZZ);
    outMatrix[1] = 2 * (theXY + theZW);
    outMatrix[2] = 2 * (theXZ - theYW); // followed the "written" formula, not the c-peudo-code
    outMatrix[4] = 2 * (theXY - theZW); // followed the "written" formula, not the c-peudo-code
    outMatrix[5] = 1 - 2 * (theXX + theZZ);
    outMatrix[6] = 2 * (theYZ + theXW); // followed the "written" formula, not the c-peudo-code
    outMatrix[8] = 2 * (theXZ + theYW); // followed the "written" formula, not the c-peudo-code
    outMatrix[9] = 2 * (theYZ - theXW); // followed the "written" formula, not the c-peudo-code
    outMatrix[10] = 1 - 2 * (theXX + theYY);

    outMatrix[3] = outMatrix[7] = outMatrix[11] = outMatrix[12] = outMatrix[13] = outMatrix[14] = 0;
    outMatrix[15] = 1;

    TransposeMatrix(outMatrix); // this is for LH multi, need this to b RH multi
}

//==============================================================================
/**
*		Transpose the give matrix
*		@param ioMatrix == the matrix to transpose
*/
void TransposeMatrix(double ioMatrix[16])
{
    // ioMatrix = 0  1  2  3
    //            4  5  6  7
    //            8  9 10 11
    //           12 13 14 15
    //
    // swapping 1 with 4, 2 wisth 8, 6 with 9, 3 with 12, 7 with 13, 11 with 14
    const long thePairs[][2] = { { 1, 4 },  { 2, 8 },   { 6, 9 },  { 3, 12 },
                                 { 7, 13 }, { 11, 14 }, { -1, -1 } };

    double theTemp;
    for (long theIndex = 0; thePairs[theIndex][0] != -1; ++theIndex) {
        theTemp = ioMatrix[thePairs[theIndex][0]];
        ioMatrix[thePairs[theIndex][0]] = ioMatrix[thePairs[theIndex][1]];
        ioMatrix[thePairs[theIndex][1]] = theTemp;
    }
}

//==============================================================================
/**
*		retireve the rotation axis info from the code
*		@param inCode
*		@param inOrder
*		@return the rotation axis
*/
long GetRotationAxis(long inCode, long inOrder)
{
    // zero out the others...
    long theFlag = 0xf << (inOrder * 4);
    inCode &= theFlag;
    return inCode >> (inOrder * 4);
}

//==============================================================================
/**
*		set the rotation axis info into the code
*		@param inAxis
*		@param inOrder
*		@param outCode
*		@return outCode
*/
long SetRotationAxis(long inAxis, long inOrder, long &outCode)
{
    // zero out existing axis...
    long theFlag = 0xf << (inOrder * 4);
    outCode &= ~theFlag;
    // and set new axis in its place
    outCode |= inAxis << (inOrder * 4);
    return outCode;
}

//==============================================================================
/**
*		Convert matrix to studio friendly euler angle
*		@param inNegScls == if the -ve scaling is present in the axes
*		@param inMatrixDouble16 == the matrix to convert
*		@param outEulerDouble3 == the resultant euler angle
*		@param outMatrixDouble16 == the buffer holding the final rotation matrix
*		@return TRUE ? success : otherwise
*/
bool ConvertMatrixToStudioEuler(bool inNegScls[3], double *inMatrixDouble16, double *outEulerDouble3,
double *outMatrixDouble16)
{
    double theMatrixDouble16[16];
    for (long theIdx = 0; theIdx < 16; ++theIdx)
        theMatrixDouble16[theIdx] = inMatrixDouble16[theIdx];

    double *theAxis[3];
    theAxis[0] = theMatrixDouble16;
    theAxis[1] = theMatrixDouble16 + 4;
    theAxis[2] = theMatrixDouble16 + 8;

    for (long theAxisIdx = 0; theAxisIdx < 3; ++theAxisIdx)
        for (long theAxisUnit = 0; theAxisUnit < 3; ++theAxisUnit) // scale the axis
            theAxis[theAxisIdx][theAxisUnit] =
                    theAxis[theAxisIdx][theAxisUnit] * (inNegScls[theAxisUnit] ? -1 : 1);

    bool theResult =
            ConvertMatrixToStudioEuler(theMatrixDouble16, outEulerDouble3, outMatrixDouble16);

    // This condition can be proven by drawing a truth table
    if ((inNegScls[0] && !inNegScls[1] && !inNegScls[2])
            || (!inNegScls[0] && inNegScls[1] && !inNegScls[2])
            || (!inNegScls[0] && !inNegScls[1] && inNegScls[2])
            || (inNegScls[0] && inNegScls[1] && inNegScls[2])) {
        // Odd count, inv polarity
        // This requiremnt is required based on empirical experiment...
        // u can always construct a few models and try the transformation if
        // u are skeptical of this wierd swizzle
        outEulerDouble3[0] *= -1;
        outEulerDouble3[2] *= -1;
    }

    return theResult;
}

//==============================================================================
/**
*		Convert matrix to studio friendly euler angle
*		@param inMatrixDouble16 == the matrix to convert
*		@param outEulerDouble3 == the resultant euler angle
*		@param outMatrixDouble16 == the buffer holding the final rotation matrix
*		@return TRUE ? success : otherwise
*/
bool ConvertMatrixToStudioEuler(double *inMatrixDouble16, double *outEulerDouble3,
                                double *outMatrixDouble16)
{
    bool theResult = false;

    double *theXAxis = outMatrixDouble16;
    double *theYAxis = outMatrixDouble16 + 4;
    double *theZAxis = outMatrixDouble16 + 8;
    memcpy(outMatrixDouble16, inMatrixDouble16, sizeof(double) * 16);

    // use zaxis to find angle to reset y-axis rotation
    double theYAngle;
    if (theZAxis[2] != 0) {
        theYAngle = atan(theZAxis[0] / theZAxis[2]);
        // atan only from -90 to 90...
        if (theZAxis[2] < 0)
            theYAngle -= M_PI;
        // just to keep it -180 - 180, so it's easier to see
        if (theYAngle < -M_PI)
            theYAngle += 2 * M_PI;
    } else {
        // ccw is +ve
        if (theZAxis[0] > 0)
            theYAngle = M_PI * 0.5;
        else
            theYAngle = -M_PI * 0.5;
    }
    RotateAxes(0, theYAngle, 0, theXAxis, theYAxis, theZAxis);

    // use zaxis again to find angle to reset x-axis rotation
    double theXAngle;
    if (theZAxis[2] != 0) {
        theXAngle = atan(theZAxis[1] / theZAxis[2]);
        // atan only from -90 to 90...
        if (theZAxis[2] < 0)
            theXAngle -= M_PI;
        // just to keep it -180 - 180, so it's easier to see
        if (theXAngle < -M_PI)
            theXAngle += 2 * M_PI;
    } else {
        // ccw is +ve
        if (theZAxis[1] > 0)
            theXAngle = M_PI * 0.5;
        else
            theXAngle = -M_PI * 0.5;
    }
    RotateAxes(theXAngle, 0, 0, theXAxis, theYAxis, theZAxis);

    // use xaxis to find angle to reset z-axis rotation
    double theZAngle;
    if (theXAxis[0] != 0) {
        theZAngle = atan(theXAxis[1] / theXAxis[0]);
        // atan only from -90 to 90...
        if (theXAxis[0] < 0)
            theZAngle -= M_PI;
        // just to keep it -180 - 180, so it's easier to see
        if (theZAngle < -M_PI)
            theZAngle += 2 * M_PI;
    } else {
        // ccw is +ve
        if (theXAxis[1] > 0)
            theZAngle = M_PI * 0.5;
        else
            theZAngle = -M_PI * 0.5;
    }
    RotateAxes(0, 0, theZAngle, theXAxis, theYAxis, theZAxis);

    outEulerDouble3[0] = theXAngle * EulerAngleConverter::TO_DEGREE_MULTIPLIER;
    // Studio's y rotation is opposite
    outEulerDouble3[1] = -theYAngle * EulerAngleConverter::TO_DEGREE_MULTIPLIER;
    outEulerDouble3[2] = theZAngle * EulerAngleConverter::TO_DEGREE_MULTIPLIER;

    theResult = true;
    return theResult;
}

//==============================================================================
/**
*		rotate inPoint abt X for inRadian
*		@param inRadian == the rad of angle to rotate
*		@param inPoint == the point to rotate
*		@param outPoint == the resultant point
*/
void RotateX(double inRadian, double *inPoint, double *outPoint)
{
    double theMatrix[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    theMatrix[4] = cos(inRadian);
    theMatrix[5] = sin(inRadian);
    theMatrix[7] = -sin(inRadian);
    theMatrix[8] = cos(inRadian);

    double x = inPoint[0];
    double y = inPoint[1];
    double z = inPoint[2];
    outPoint[0] = x * theMatrix[0] + y * theMatrix[3] + z * theMatrix[6];
    outPoint[1] = x * theMatrix[1] + y * theMatrix[4] + z * theMatrix[7];
    outPoint[2] = x * theMatrix[2] + y * theMatrix[5] + z * theMatrix[8];
}

//==============================================================================
/**
*		rotate inPoint abt Y for inRadian
*		@param inRadian == the rad of angle to rotate
*		@param inPoint == the point to rotate
*		@param outPoint == the resultant point
*/
void RotateY(double inRadian, double *inPoint, double *outPoint)
{
    double theMatrix[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    theMatrix[0] = cos(inRadian);
    theMatrix[2] = sin(inRadian);
    theMatrix[6] = -sin(inRadian);
    theMatrix[8] = cos(inRadian);

    double x = inPoint[0];
    double y = inPoint[1];
    double z = inPoint[2];
    outPoint[0] = x * theMatrix[0] + y * theMatrix[3] + z * theMatrix[6];
    outPoint[1] = x * theMatrix[1] + y * theMatrix[4] + z * theMatrix[7];
    outPoint[2] = x * theMatrix[2] + y * theMatrix[5] + z * theMatrix[8];
}

//==============================================================================
/**
*		rotate inPoint abt Z for inRadian
*		@param inRadian == the rad of angle to rotate
*		@param inPoint == the point to rotate
*		@param outPoint == the resultant point
*/
void RotateZ(double inRadian, double *inPoint, double *outPoint)
{
    double theMatrix[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    theMatrix[0] = cos(inRadian);
    theMatrix[1] = -sin(inRadian);
    theMatrix[3] = sin(inRadian);
    theMatrix[4] = cos(inRadian);

    double x = inPoint[0];
    double y = inPoint[1];
    double z = inPoint[2];
    outPoint[0] = x * theMatrix[0] + y * theMatrix[3] + z * theMatrix[6];
    outPoint[1] = x * theMatrix[1] + y * theMatrix[4] + z * theMatrix[7];
    outPoint[2] = x * theMatrix[2] + y * theMatrix[5] + z * theMatrix[8];
}

//==============================================================================
/**
*		rotate 3 "affine axis" vectors abt inXRadian in the global space X axis
*		inYRadian in the global space Y axis, inZRadian in the global space Z axis
*		@param inXRadian == the angle to rotate in the global x
*		@param inYRadian == the angle to rotate in the global y
*		@param inZRadian == the angle to rotate in the globla z
*		@param outXAxis == the affine X axis
*		@param outYAxis == the affine Y axis
*		@param outZAxis == the affine Z axis
*/
void RotateAxes(double inXRadian, double inYRadian, double inZRadian, double *outXAxis,
                double *outYAxis, double *outZAxis)
{
    RotateX(inXRadian, outXAxis, outXAxis);
    RotateX(inXRadian, outYAxis, outYAxis);
    RotateX(inXRadian, outZAxis, outZAxis);
    //
    RotateY(inYRadian, outXAxis, outXAxis);
    RotateY(inYRadian, outYAxis, outYAxis);
    RotateY(inYRadian, outZAxis, outZAxis);
    //
    RotateZ(inZRadian, outXAxis, outXAxis);
    RotateZ(inZRadian, outYAxis, outYAxis);
    RotateZ(inZRadian, outZAxis, outZAxis);
}

//==============================================================================
/**
*	Adds a static rotation on one axis.
*
*	@param	inRotOrder
*	@param	inRotSeq
*	@param	inRots
*	@param	outXAxis
*	@param	outYAxis
*	@param	outZAxis
*/
void DoAxes(long inRotOrder, long inRotSeq, double *inRots, double *outXAxis, double *outYAxis,
            double *outZAxis)
{
    long theAxis = EulerAngleConverter::GetRotationAxis(inRotOrder, inRotSeq);

    if (theAxis == EulerAngleConverter::ROTATE_X) {
        EulerAngleConverter::RotateAxes(inRots[0], 0, 0, outXAxis, outYAxis, outZAxis);
    } else if (theAxis == EulerAngleConverter::ROTATE_Y) {
        EulerAngleConverter::RotateAxes(0, -inRots[1], 0, outXAxis, outYAxis, outZAxis);
    } else if (theAxis == EulerAngleConverter::ROTATE_Z) {
        EulerAngleConverter::RotateAxes(0, 0, -inRots[2], outXAxis, outYAxis, outZAxis);
    }
}

//==============================================================================
/**
*	Builds a rotation matrix based on static euler rotation and rotation order.
*
*	This is used by StaticEulerToStudioEuler.
*
*	@param	inStaticEuler		the static euler rotation
*	@param	inRotateOrder		the order of rotation
*	@param	outMatrix			the resultant rotation matrix
*/
void BuildMatrix(double *inStaticEuler, long *inRotateOrder, double *outMatrix)
{
    long theRotOrder = 0;
    SetRotationAxis(inRotateOrder[0], ROTATE_FIRST, theRotOrder);
    SetRotationAxis(inRotateOrder[1], ROTATE_SECOND, theRotOrder);
    SetRotationAxis(inRotateOrder[2], ROTATE_THIRD, theRotOrder);

    // set to identity
    for (long theIndex = 0; theIndex < 16; ++theIndex)
        if (theIndex / 4 == theIndex % 4)
            outMatrix[theIndex] = 1;
        else
            outMatrix[theIndex] = 0;

    double *theXAxis = outMatrix;
    double *theYAxis = outMatrix + 4;
    double *theZAxis = outMatrix + 8;
    DoAxes(theRotOrder, ROTATE_FIRST, inStaticEuler, theXAxis, theYAxis, theZAxis);
    DoAxes(theRotOrder, ROTATE_SECOND, inStaticEuler, theXAxis, theYAxis, theZAxis);
    DoAxes(theRotOrder, ROTATE_THIRD, inStaticEuler, theXAxis, theYAxis, theZAxis);
}

//==============================================================================
/**
*	Matrix multiple two double arrays
*
*	This is used by StaticEulerToStudioEuler.
*
*	@param	inM1			first matrix
*	@param	inM2			second matrix
*	@param	outM3			resultant matrix
*/
void MatrixMultiply(double inM1[4][4], double inM2[4][4], double outM3[4][4])
{
    outM3[0][0] = inM1[0][0] * inM2[0][0] + inM1[1][0] * inM2[0][1] + inM1[2][0] * inM2[0][2]
            + inM1[3][0] * inM2[0][3];
    outM3[0][1] = inM1[0][1] * inM2[0][0] + inM1[1][1] * inM2[0][1] + inM1[2][1] * inM2[0][2]
            + inM1[3][1] * inM2[0][3];
    outM3[0][2] = inM1[0][2] * inM2[0][0] + inM1[1][2] * inM2[0][1] + inM1[2][2] * inM2[0][2]
            + inM1[3][2] * inM2[0][3];
    outM3[0][3] = inM1[0][3] * inM2[0][0] + inM1[1][3] * inM2[0][1] + inM1[2][3] * inM2[0][2]
            + inM1[3][3] * inM2[0][3];
    outM3[1][0] = inM1[0][0] * inM2[1][0] + inM1[1][0] * inM2[1][1] + inM1[2][0] * inM2[1][2]
            + inM1[3][0] * inM2[1][3];
    outM3[1][1] = inM1[0][1] * inM2[1][0] + inM1[1][1] * inM2[1][1] + inM1[2][1] * inM2[1][2]
            + inM1[3][1] * inM2[1][3];
    outM3[1][2] = inM1[0][2] * inM2[1][0] + inM1[1][2] * inM2[1][1] + inM1[2][2] * inM2[1][2]
            + inM1[3][2] * inM2[1][3];
    outM3[1][3] = inM1[0][3] * inM2[1][0] + inM1[1][3] * inM2[1][1] + inM1[2][3] * inM2[1][2]
            + inM1[3][3] * inM2[1][3];
    outM3[2][0] = inM1[0][0] * inM2[2][0] + inM1[1][0] * inM2[2][1] + inM1[2][0] * inM2[2][2]
            + inM1[3][0] * inM2[2][3];
    outM3[2][1] = inM1[0][1] * inM2[2][0] + inM1[1][1] * inM2[2][1] + inM1[2][1] * inM2[2][2]
            + inM1[3][1] * inM2[2][3];
    outM3[2][2] = inM1[0][2] * inM2[2][0] + inM1[1][2] * inM2[2][1] + inM1[2][2] * inM2[2][2]
            + inM1[3][2] * inM2[2][3];
    outM3[2][3] = inM1[0][3] * inM2[2][0] + inM1[1][3] * inM2[2][1] + inM1[2][3] * inM2[2][2]
            + inM1[3][3] * inM2[2][3];
    outM3[3][0] = inM1[0][0] * inM2[3][0] + inM1[1][0] * inM2[3][1] + inM1[2][0] * inM2[3][2]
            + inM1[3][0] * inM2[3][3];
    outM3[3][1] = inM1[0][1] * inM2[3][0] + inM1[1][1] * inM2[3][1] + inM1[2][1] * inM2[3][2]
            + inM1[3][1] * inM2[3][3];
    outM3[3][2] = inM1[0][2] * inM2[3][0] + inM1[1][2] * inM2[3][1] + inM1[2][2] * inM2[3][2]
            + inM1[3][2] * inM2[3][3];
    outM3[3][3] = inM1[0][3] * inM2[3][0] + inM1[1][3] * inM2[3][1] + inM1[2][3] * inM2[3][2]
            + inM1[3][3] * inM2[3][3];
}

//==============================================================================
/**
*	Retrieves a quaternion given an axis angle
*
*	This is used by StaticEulerToStudioEuler.
*
*	@param	inXAxis				XAxis orientation
*	@param	inYAxis				YAxix orientation
*	@param	inZAxis				ZAxis orientation
*	@param	inAngleInDegrees	angle in degrees
*	@param	outDouble4			double[4] containing the quaternion
*/
void GetQuatFromAxisAngle(float inXAxis, float inYAxis, float inZAxis, double inAngleInDegrees,
                          double *outDouble4)
{
    TORAD(inAngleInDegrees);
    double theHalfAngle = 0.5f * inAngleInDegrees;
    double theSin = ::sin(theHalfAngle);
    double qx, qy, qz, qw;
    qx = theSin * inXAxis;
    qy = theSin * inYAxis;
    qz = theSin * inZAxis;
    qw = ::cos(theHalfAngle);

    // Normalize
    const float SQREPSILON = 0.000001f;
    double theNorm = qx * qx + qy * qy + qz * qz + qw * qw;
    if (theNorm < SQREPSILON) {
        qx = qy = qz = 0;
        qw = 1.0f;
    } else if (::abs(theNorm - 1.0f) > SQREPSILON) {
        double theScale = 1.0f / ::sqrt(theNorm);
        qx *= theScale;
        qy *= theScale;
        qz *= theScale;
        qw *= theScale;
    }
    outDouble4[0] = qx;
    outDouble4[1] = qy;
    outDouble4[2] = qz;
    outDouble4[3] = qw;
}

//==============================================================================
/**
*	Converts from static euler rotation to Studio's euler rotation.
*
*	This is an unoptimized version. But it works.
*
*	@param	inStaticEuler		the static euler rotation, in degrees
*	@param	inRotateOrder		the order of rotation
*	@param	outStaticEuler		the resultant Studio's relative euler rotation, in degrees
*/

/* Previous implementation
void StaticEulerToStudioEuler( double* inStaticEuler, long* inRotateOrder, double* outStudioEuler )
{
        double theStaticEuler[3];
        theStaticEuler[0] = TORAD( inStaticEuler[0] );
        theStaticEuler[1] = TORAD( inStaticEuler[1] );
        theStaticEuler[2] = TORAD( inStaticEuler[2] );

        long theRotateOrder[ 3 ] = { ROTATE_X, ROTATE_Y, ROTATE_Z };
        if ( inRotateOrder != NULL )
        {
                ::memcpy( theRotateOrder, inRotateOrder, 3 * sizeof( long ) );
        }

        double the4x4[16];
        double theNULL4x4[16];
        BuildMatrix( theStaticEuler, theRotateOrder, the4x4 );
        ConvertMatrixToStudioEuler( the4x4, outStudioEuler, theNULL4x4 );
}
*/
void StaticEulerToStudioEuler(double *inStaticEuler, long *inRotateOrder, double *outStudioEuler)
{
    double theTempMatr[4][4];
    double theQuat[3][4];
    double theMatr1[4][4];
    double theMatr2[4][4];

    for (long theIndex = 0; theIndex < 3; ++theIndex) {
        switch (inRotateOrder[theIndex]) {
        case ROTATE_X:
            GetQuatFromAxisAngle(1.0f, 0.0f, 0.0f, inStaticEuler[0], theQuat[theIndex]);
            break;
        case ROTATE_Y:
            GetQuatFromAxisAngle(0.0f, 1.0f, 0.0f, inStaticEuler[1], theQuat[theIndex]);
            break;
        case ROTATE_Z:
            GetQuatFromAxisAngle(0.0f, 0.0f, 1.0f, inStaticEuler[2], theQuat[theIndex]);
            break;
        }
    }

    EulerAngleConverter::ConvertQuatToMatrix(theQuat[0], reinterpret_cast<double *>(theMatr1));
    EulerAngleConverter::ConvertQuatToMatrix(theQuat[1], reinterpret_cast<double *>(theMatr2));
    MatrixMultiply(theMatr1, theMatr2, theTempMatr);
    EulerAngleConverter::ConvertQuatToMatrix(theQuat[2], reinterpret_cast<double *>(theMatr1));
    MatrixMultiply(theTempMatr, theMatr1, theMatr2);

    ConvertMatrixToStudioEuler(reinterpret_cast<double *>(theMatr2), outStudioEuler,
                               reinterpret_cast<double *>(theMatr1));
}

} // end of namespace EulerAngleConverter
