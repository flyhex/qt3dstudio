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
//	Description
//==============================================================================
// EulerAngles.h - Support for 24 angle schemes
// by Ken Shoemake, shoemake@graphics.cis.upenn.edu
// in "Graphics Gems IV", Academic Press, 1994
// http://vered.rose.utoronto.ca/people/david_dir/GEMS/GEMS.html
// Order type constants, constructors, extractors
//		 There are 24 possible conventions, designated by:
//			o EulAxI = axis used initially
//			o EulPar = parity of axis permutation
//			o EulRep = repetition of initial axis as last
//			o EulFrm = frame from which axes are taken
//		Axes I,J,K will be a permutation of X,Y,Z.
//		Axis H will be either I or K, depending on EulRep.
//		Frame S takes axes from initial static frame.
//		If ord = (AxI=X, Par=Even, Rep=No, Frm=S), then
//		{a,b,c,ord} means Rz(c)Ry(b)Rx(a), where Rz(c)v
//		rotates v around Z by c radians.
//

//==============================================================================
//	Prefixes
//==============================================================================
#ifndef _EULERANGLES_H_
#define _EULERANGLES_H_
#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include "QuatTypes.h"

#define EulFrmS 0
#define EulFrmR 1
#define EulFrm(ord) ((unsigned)(ord)&1)
#define EulRepNo 0
#define EulRepYes 1
#define EulRep(ord) (((unsigned)(ord) >> 1) & 1)
#define EulParEven 0
#define EulParOdd 1
#define EulPar(ord) (((unsigned)(ord) >> 2) & 1)
#define EulSafe "\000\001\002\000"
#define EulNext "\001\002\000\001"
#define EulAxI(ord) ((int)(EulSafe[(((unsigned)(ord) >> 3) & 3)]))
#define EulAxJ(ord) ((int)(EulNext[EulAxI(ord) + (EulPar(ord) == EulParOdd)]))
#define EulAxK(ord) ((int)(EulNext[EulAxI(ord) + (EulPar(ord) != EulParOdd)]))
#define EulAxH(ord) ((EulRep(ord) == EulRepNo) ? EulAxK(ord) : EulAxI(ord))

// EulGetOrd unpacks all useful information about order simultaneously.
#define EulGetOrd(ord, i, j, k, h, n, s, f)                                                        \
{                                                                                              \
    unsigned o = ord;                                                                          \
    f = o & 1;                                                                                 \
    o = o >> 1;                                                                                \
    s = o & 1;                                                                                 \
    o = o >> 1;                                                                                \
    n = o & 1;                                                                                 \
    o = o >> 1;                                                                                \
    i = EulSafe[o & 3];                                                                        \
    j = EulNext[i + n];                                                                        \
    k = EulNext[i + 1 - n];                                                                    \
    h = s ? k : i;                                                                             \
    }

// EulOrd creates an order value between 0 and 23 from 4-tuple choices.
#define EulOrd(i, p, r, f) (((((((i) << 1) + (p)) << 1) + (r)) << 1) + (f))

// Static axes
// X = 0, Y = 1, Z = 2 ref QuatPart
#define EulOrdXYZs EulOrd(0, EulParEven, EulRepNo, EulFrmS)
#define EulOrdXYXs EulOrd(0, EulParEven, EulRepYes, EulFrmS)
#define EulOrdXZYs EulOrd(0, EulParOdd, EulRepNo, EulFrmS)
#define EulOrdXZXs EulOrd(0, EulParOdd, EulRepYes, EulFrmS)
#define EulOrdYZXs EulOrd(1, EulParEven, EulRepNo, EulFrmS)
#define EulOrdYZYs EulOrd(1, EulParEven, EulRepYes, EulFrmS)
#define EulOrdYXZs EulOrd(1, EulParOdd, EulRepNo, EulFrmS)
#define EulOrdYXYs EulOrd(1, EulParOdd, EulRepYes, EulFrmS)
#define EulOrdZXYs EulOrd(2, EulParEven, EulRepNo, EulFrmS)
#define EulOrdZXZs EulOrd(2, EulParEven, EulRepYes, EulFrmS)
#define EulOrdZYXs EulOrd(2, EulParOdd, EulRepNo, EulFrmS)
#define EulOrdZYZs EulOrd(2, EulParOdd, EulRepYes, EulFrmS)

// Rotating axes
#define EulOrdZYXr EulOrd(0, EulParEven, EulRepNo, EulFrmR)
#define EulOrdXYXr EulOrd(0, EulParEven, EulRepYes, EulFrmR)
#define EulOrdYZXr EulOrd(0, EulParOdd, EulRepNo, EulFrmR)
#define EulOrdXZXr EulOrd(0, EulParOdd, EulRepYes, EulFrmR)
#define EulOrdXZYr EulOrd(1, EulParEven, EulRepNo, EulFrmR)
#define EulOrdYZYr EulOrd(1, EulParEven, EulRepYes, EulFrmR)
#define EulOrdZXYr EulOrd(1, EulParOdd, EulRepNo, EulFrmR)
#define EulOrdYXYr EulOrd(1, EulParOdd, EulRepYes, EulFrmR)
#define EulOrdYXZr EulOrd(2, EulParEven, EulRepNo, EulFrmR)
#define EulOrdZXZr EulOrd(2, EulParEven, EulRepYes, EulFrmR)
#define EulOrdXYZr EulOrd(2, EulParOdd, EulRepNo, EulFrmR)
#define EulOrdZYZr EulOrd(2, EulParOdd, EulRepYes, EulFrmR)

#ifndef M_PI
#define M_PI 3.1415926535898
#endif

#define TODEG(x) x = (float)(x * 180 / M_PI);
#define TORAD(x) x = (float)(x / 180 * M_PI);

namespace EulerAngleConverter {
const double TO_DEGREE_MULTIPLIER = 180 / M_PI;
const double TO_RADIAN_MULTIPLIER = M_PI / 180;

// encode rotate order & axes
const long ROTATE_X = 1;
const long ROTATE_Y = 2;
const long ROTATE_Z = 3;
const long ROTATE_FIRST = 0;
const long ROTATE_SECOND = 1;
const long ROTATE_THIRD = 2;
long GetRotationAxis(long inCode, long inOrder);
long SetRotationAxis(long inAxis, long inOrder, long &outCode);

class CEulerAngleConverter
{
private:
    char m_OrderInfoBuffer[1024];

public:
    CEulerAngleConverter();
    virtual ~CEulerAngleConverter();

public:
    EulerAngles Eul_(float ai, float aj, float ah, int order);
    Quat Eul_ToQuat(EulerAngles ea);
    void Eul_ToHMatrix(EulerAngles ea, HMatrix M);
    EulerAngles Eul_FromHMatrix(HMatrix M, int order);
    EulerAngles Eul_FromQuat(Quat q, int order);

    // Debug Stuff
    const char *DumpOrderInfo();
};

void ConvertQuatToMatrix(double inQuat[4], double outMatrix[16]);
void TransposeMatrix(double ioMatrix[16]);
long GetRotationAxis(long inCode, long inOrder);
long SetRotationAxis(long inAxis, long inOrder, long &outCode);

bool ConvertMatrixToStudioEuler(bool inNegScls[3], double *inMatrixDouble16,
                                double *outEulerDouble3, double *outMatrixDouble16);
bool ConvertMatrixToStudioEuler(double *inMatrixDouble16, double *outEulerDouble3,
                                double *outMatrixDouble16);
void RotateX(double inRadian, double *inPoint, double *outPoint);
void RotateY(double inRadian, double *inPoint, double *outPoint);
void RotateZ(double inRadian, double *inPoint, double *outPoint);
void RotateAxes(double inXRadian, double inYRadian, double inZRadian, double *inXAxis,
                double *inYAxis, double *inZAxis);
void DoAxes(long inRotOrder, long inRotSeq, double *inRots, double *inXAxis, double *inYAxis,
            double *inZAxis);

void StaticEulerToStudioEuler(double *inStaticEuler, long *inRotateOrder, double *outStudioEuler);
}

#endif
