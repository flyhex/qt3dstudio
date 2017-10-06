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
//	Prefix
//==============================================================================
#ifndef __MATRIX_H_
#define __MATRIX_H_
#include <QtGlobal>

#include "Vector3.h"

//==============================================================================
//	Forwards
//==============================================================================
namespace Q3DStudio {
class CRotation3;
};

typedef qint32 INT32;
typedef float FLOAT;

namespace Q3DStudio {

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *		@class CMatrix: A simple, row major matrix in the form:
 *
 *		| m0	m1		m2		m3 |			or		| m0	m1
 *m2	w |
 *		| m4	m5		m6		m7 |        			| m4	m5
 *m6	w |
 *		| m8	m9		m10		m11 |        			| m8	m9
 *m10	w |
 *		| m12	m13		m14		m15 |        			| tX	tY
 *tZ	w |
 *
 *		Rotations are concatenated in the the following order: YXZ
 *		All rotations are clockwise when viewed down the positive axis towards to origin.
  */
class CMatrix
{
    //==============================================================================
    //	Enumerations
    //==============================================================================
public:
    enum ERotationOrder {
        // maya and studio uses "static"
        XYZ = 0,
        YZX,
        ZXY,
        XZY,
        YXZ,
        ZYX,
        // adding "relative" because 3ds max uses this
        XYZr,
        YZXr,
        ZXYr,
        XZYr,
        YXZr,
        ZYXr
    };

    enum EOrientation { LEFT_HANDED = 0, RIGHT_HANDED };

    typedef float TArray[4][4];

    //==============================================================================
    //	Constants
    //==============================================================================
public:
    static const long s_DefaultOrder = YXZ;

    // Field members
public:
    TArray m; ///< 16 elements in a 4 x 4

    // Construction/Destruction
public:
    CMatrix();
    CMatrix(const CMatrix &inMatrix);
    CMatrix(float in00, float in01, float in02, float in03, float in10, float in11, float in12,
            float in13, float in20, float in21, float in22, float in23, float in30, float in31,
            float in32, float in33);
    virtual ~CMatrix() {}

    // Set functions for MatrixScriptObject
public:
    void Set(const CMatrix &inMatrix);

    // Equality operators
public:
    bool operator==(const CMatrix &inMatrix) const;
    bool operator!=(const CMatrix &inMatrix) const;

    // Assignment operators
public:
    CMatrix &operator=(const CMatrix &inMatrix);
    CMatrix &operator+=(const CMatrix &inMatrix);
    CMatrix &operator-=(const CMatrix &inMatrix);
    CMatrix &operator*=(const CMatrix &inMatrix);

    // Access operators
public:
    float &operator()(const long &inRow, const long &inColumn);
    float operator()(const long &inRow, const long &inColumn) const;
    TArray &GetArray();

    // Binary operators
public:
    CMatrix operator+(const CMatrix &inMatrix) const;
    CMatrix operator-(const CMatrix &inMatrix) const;
    CMatrix operator*(const CMatrix &inMatrix) const;

    // Matrix functions
public:
    CMatrix &Zero();
    CMatrix &Identity();
    CMatrix &Orthographic(const float &inNear, const float &inFar, const float &inWidth,
                          const float &inHeight);
    CMatrix &Frustum(const bool &inOrthographic, const float &inNear, const float &inFar,
                     const float &inLeft, const float &inTop, const float &inRight,
                     const float &inBottom);
    CMatrix &Transpose();
    CMatrix &Translate(const CVector3 &inVector);
    CMatrix &Translate(const float &inX, const float &inY, const float &inZ);
    CMatrix &Rotate(const CRotation3 &inRotation);
    CMatrix &Scale(const CVector3 &inVector);
    float Determinant() const;
    float Invert();
    bool IsIdentity() const;

    CVector3 GetTranslation() const;
    CVector3 GetXComponent() const;
    CVector3 GetYComponent() const;
    CVector3 GetZComponent() const;

    CMatrix &Billboard(const CMatrix &inMatrix, bool inFreeRotateFlag = false);
    CMatrix &CloneRotation(const CMatrix &inMatrix, bool inMirrorFlag = false);

    void SetTranslate(const CVector3 &inTranslate);
    void SetRotate(const CRotation3 &inRotation, long inRotOder, long inOrientation);
    void SetScale(const CVector3 &inScale);

    // Implemntation
protected:
    bool IsAffine() const;
    bool CheckBounds(const long &inRow, const long &inColumn) const;
    CMatrix MakeTranslate(const CVector3 &inVector) const;
    CMatrix MakeRotate(const CRotation3 &inRotation) const;
    CMatrix MakeScale(const CVector3 &inScale) const;
    CMatrix Concatenate(const CMatrix &inMatrix) const;
    float InvertAffine();
    float InvertNonSingular();
    float Determinant2x2(const float &inA, const float &inB, const float &inC,
                         const float &inD) const;
    float Determinant3x3(const float &inA1, const float &inA2, const float &inA3, const float &inB1,
                         const float &inB2, const float &inB3, const float &inC1, const float &inC2,
                         const float &inC3) const;
    void FormatEulerAngles(INT32 inRotationOrder, FLOAT inXAngle, FLOAT inYAngle, FLOAT inZAngle,
                           INT32 &outRotationOrder, FLOAT &outFirstAngle, FLOAT &outSecondAngle,
                           FLOAT &outThirdAngle);
};

} // namespace Q3DStudio

#endif // __MATRIX_H_
