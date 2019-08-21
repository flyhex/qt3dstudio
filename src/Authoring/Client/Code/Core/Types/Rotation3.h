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

#ifndef __ROTATION3_H_
#define __ROTATION3_H_

#include "Vector3.h"
#include "Qt3DSObjectCounter.h"

namespace Q3DStudio {

const double QT3DS_DEGREES_TO_RADIANS = (6.2831856f / 360.0f);
const double QT3DS_RADIANS_TO_DEGREES = (360.0f / 6.2831856f);

class CRotation3 : public CVector3
{

    // Construction
public:
    CRotation3();
    CRotation3(const CRotation3 &inRotation);
    CRotation3(const float &inX, const float &inY, const float &inZ);
    virtual ~CRotation3();

    // Access
public:
    virtual bool operator==(const CRotation3 &inOther) const;
    virtual bool operator==(const CVector3 &inOther) const;
    virtual void SetXDegrees(const float &inX);
    virtual void SetYDegrees(const float &inY);
    virtual void SetZDegrees(const float &inZ);

    virtual void SetXRadians(const float &inX);
    virtual void SetYRadians(const float &inY);
    virtual void SetZRadians(const float &inZ);

    virtual float GetXDegrees() const;
    virtual float GetYDegrees() const;
    virtual float GetZDegrees() const;

    virtual float GetXRadians() const;
    virtual float GetYRadians() const;
    virtual float GetZRadians() const;

    virtual void Get(float &outXRotation, float &outYRotation, float &outZRotation) const;
    virtual void Set(const float &inXRotation, const float &inYRotation, const float &inZRotation);
    virtual void GetAngles(float &outXRotation, float &outYRotation, float &outZRotation) const;
    virtual void SetAngles(const float &inXRotation, const float &inYRotation,
                           const float &inZRotation);
    virtual void GetRadians(float &outXRotation, float &outYRotation, float &outZRotation) const;
    virtual void SetRadians(const float &inXRotation, const float &inYRotation,
                            const float &inZRotation);

    virtual void LookAt(const CVector3 &inVector);

    // Operators
public:
    virtual CVector3 &operator=(const CVector3 &inRotation);
};

} // namespace Q3DStudio

#endif //__ROTATION3_H_
