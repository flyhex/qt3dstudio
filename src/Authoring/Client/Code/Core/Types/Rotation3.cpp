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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include <math.h>
#include "Rotation3.h"

namespace Q3DStudio {

//==============================================================================
/**
*	Constructs the CRotation object, setting its value to (x = 0.0f, y = 0.0f, z=0.0f).
*/
CRotation3::CRotation3()
{
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

//==============================================================================
/**
 *	Constructs a copy of the CRotation3 object.
 *
 *	@param	inRotation	The source for the copy operation.
 */
CRotation3::CRotation3(const CRotation3 &inRotation)
{
    x = inRotation.x;
    y = inRotation.y;
    z = inRotation.z;
}

//==============================================================================
/**
*	Constructs the CVector3 object, setting its value to the X, Y, Z passed in.
*	Values are in Degrees.
*/
CRotation3::CRotation3(const float &inX, const float &inY, const float &inZ)
{
    SetAngles(inX, inY, inZ);
}

CRotation3::~CRotation3()
{
}

bool CRotation3::operator==(const CRotation3 &inOther) const
{
    return CVector3::operator==(inOther);
}

bool CRotation3::operator==(const CVector3 &inOther) const
{
    return CVector3::operator==(inOther);
}

void CRotation3::SetXDegrees(const float &inX)
{
    x = (float)(inX * QT3DS_DEGREES_TO_RADIANS);
}
void CRotation3::SetYDegrees(const float &inY)
{
    y = (float)(inY * QT3DS_DEGREES_TO_RADIANS);
}
void CRotation3::SetZDegrees(const float &inZ)
{
    z = (float)(inZ * QT3DS_DEGREES_TO_RADIANS);
}

void CRotation3::SetXRadians(const float &inX)
{
    x = inX;
}
void CRotation3::SetYRadians(const float &inY)
{
    y = inY;
}
void CRotation3::SetZRadians(const float &inZ)
{
    z = inZ;
}

float CRotation3::GetXDegrees() const
{
    return (float)(x * QT3DS_RADIANS_TO_DEGREES);
}
float CRotation3::GetYDegrees() const
{
    return (float)(y * QT3DS_RADIANS_TO_DEGREES);
}
float CRotation3::GetZDegrees() const
{
    return (float)(z * QT3DS_RADIANS_TO_DEGREES);
}

float CRotation3::GetXRadians() const
{
    return x;
}
float CRotation3::GetYRadians() const
{
    return y;
}
float CRotation3::GetZRadians() const
{
    return z;
}

//==============================================================================
/**
 *		Sets the rotation vector as angles in degrees, storing them to radians.
 *
 *		@param inXRotation The x component of the vector in degrees.
 *		@param inYRotation The y component of the vector in degrees.
 *		@param inZRotation The z component of the vector in degrees.
 */
void CRotation3::Set(const float &inXRotation, const float &inYRotation, const float &inZRotation)
{
    return SetAngles(inXRotation, inYRotation, inZRotation);
}

//==============================================================================
/**
 *		Gets the rotation vector as angles in degrees, converting them from radians.
 *
 *		@param outXRotation The x component of the vector in degrees.
 *		@param outYRotation The y component of the vector in degrees.
 *		@param outZRotation The z component of the vector in degrees.
 */
void CRotation3::Get(float &outXRotation, float &outYRotation, float &outZRotation) const
{
    GetAngles(outXRotation, outYRotation, outZRotation);
}

//==============================================================================
/**
 *		Sets the rotation vector as angles in degrees, storing them to radians.
 *
 *		@param inXRotation The x component of the vector in degrees.
 *		@param inYRotation The y component of the vector in degrees.
 *		@param inZRotation The z component of the vector in degrees.
 *
 *		@return HRESULT Always returns S_OK.
 */
void CRotation3::SetAngles(const float &inXRotation, const float &inYRotation,
                           const float &inZRotation)
{
    x = (float)(inXRotation * QT3DS_DEGREES_TO_RADIANS);
    y = (float)(inYRotation * QT3DS_DEGREES_TO_RADIANS);
    z = (float)(inZRotation * QT3DS_DEGREES_TO_RADIANS);
}

//==============================================================================
/**
 *		Gets the rotation vector as angles in degrees, converting them from radians.
 *
 *		@param outXRotation The x component of the vector in degrees.
 *		@param outYRotation The y component of the vector in degrees.
 *		@param outZRotation The z component of the vector in degrees.
 *
 *		@return HRESULT Always returns S_OK.
 */
void CRotation3::GetAngles(float &outXRotation, float &outYRotation, float &outZRotation) const
{
    outXRotation = (float)(x * QT3DS_RADIANS_TO_DEGREES);
    outYRotation = (float)(y * QT3DS_RADIANS_TO_DEGREES);
    outZRotation = (float)(z * QT3DS_RADIANS_TO_DEGREES);
}

//==============================================================================
/**
 *		Sets the rotation vector as angles in radians, storing them to radians.
 *
 *		@param inXRotation The x component of the vector in radians.
 *		@param inYRotation The y component of the vector in radians.
 *		@param inZRotation The z component of the vector in radians.
 *
 *		@return HRESULT Always returns S_OK.
 */
void CRotation3::SetRadians(const float &inXRotation, const float &inYRotation,
                            const float &inZRotation)
{
    x = inXRotation;
    y = inYRotation;
    z = inZRotation;
}

//==============================================================================
/**
 *		Gets the rotation vector as angles in radians, from storage as radians.
 *
 *		@param outXRotation The x component of the vector in radians.
 *		@param outYRotation The y component of the vector in radians.
 *		@param outZRotation The z component of the vector in radians.
 *
 *		@return HRESULT Always returns S_OK.
 */
void CRotation3::GetRadians(float &outXRotation, float &outYRotation, float &outZRotation) const
{
    outXRotation = x;
    outYRotation = y;
    outZRotation = z;
}

//==============================================================================
/**
 *	Equals operator for the CVector3 object.
 *
 *	@return		The new rotation vector after assignment.
 */
CVector3 &CRotation3::operator=(const CVector3 &inRotation)
{
    if (&inRotation != this) {
        x = inRotation.x;
        y = inRotation.y;
        z = inRotation.z;
    }

    return *this;
}

//==============================================================================
/**
 *	Sets the rotation to "look at" the specifed vector.
 */
void CRotation3::LookAt(const CVector3 &inVector)
{
    /*
    A function lookAt that takes a vector and does not return anything, it just
    modifies the object. This code is setting the values as degrees, but if you
    need radians then just use thePitch and theYaw.
    */

    double theMag = sqrt(inVector.x * inVector.x + inVector.z * inVector.z);
    double thePitch = -(atan2(static_cast<double>(inVector.y), theMag));
    double theYaw = atan2(inVector.x, inVector.z);

    x = (float)thePitch;
    y = (float)theYaw;
    z = 0;

    //	x = ( thePitch / ( PI ) ) * 180;
    //	y = ( theYaw / ( 2 * PI ) ) * 360;
    //	z = 0;
}

} // namespace Q3DStudio
