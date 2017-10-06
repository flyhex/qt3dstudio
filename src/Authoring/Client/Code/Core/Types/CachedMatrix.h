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
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Matrix.h"
#include "Vector3.h"
#include "Rotation3.h"

#include <math.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	@class	CCachedMatrix
 *	@brief	Cached matrix combining translation, scale, rotation and pivot.
 *
 *	The trig functions in rotate is killing us so we cache these.
 */
class CCachedMatrix : public CMatrix
{
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    CVector3 m_Translation; ///< Last transformation vector
    CRotation3 m_Rotation; ///< Last rotation angles
    CVector3 m_Scale; ///< Last scale vector
    CVector3 m_Pivot; ///< Last pivot vector

    CMatrix m_TransMatrix; ///< Last transformation matrix
    CMatrix m_RotMatrix; ///< Last rotation matrix
    CMatrix m_ScaleMatrix; ///< Last scale matrix
    CMatrix m_PivotMatrix; ///< Last pivot matrix

    CMatrix m_ScalePivotRotationMatrix; ///< Last scale*pivot*rotation matrix

    /*		float			m_SinX;			///< Sin of last x rotation
                    float			m_SinY;			///< Sin of last y rotation
                    float			m_SinZ;			///< Sin of last z rotation
                    float			m_CosX;			///< Cos of last x rotation
                    float			m_CosY;			///< Cos of last y rotation
                    float			m_CosZ;			///< Cos of last z rotation
       */

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CCachedMatrix();

private:
    CCachedMatrix(const CMatrix &inMatrix);
    CCachedMatrix(float in00, float in01, float in02, float in03, float in10, float in11,
                  float in12, float in13, float in20, float in21, float in22, float in23,
                  float in30, float in31, float in32, float in33);

public: // Access
    void Set(const CVector3 &inTranslation, const CRotation3 &inRotation, const CVector3 &inScale,
             const CVector3 &inPivot, const long inRotOrder = CMatrix::s_DefaultOrder,
             const long inOrientation = CMatrix::LEFT_HANDED, const bool inForce = false);

    inline void SinCos(const float inValue, float &outSin, float &outCos)
    {
        outSin = ::sin(inValue);
        outCos = ::cos(inValue);
    };

    void ConvertLeftToRight()
    {
        m[0][2] = -m[0][2];
        m[1][2] = -m[1][2];
        m[2][0] = -m[2][0];
        m[2][1] = -m[2][1];
        m[2][3] = -m[2][3];
        m[3][2] = -m[3][2];
    }
};
} // namespace Q3DStudio
