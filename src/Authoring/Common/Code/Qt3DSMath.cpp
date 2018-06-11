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
#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSMath.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const float PI = 3.1415926535897932384626433832795f;
const float EPSILON = 0.0001f;
const float SQREPSILON = 0.000001f;
const float DTOR = PI / 180.0f;
const float RTOD = 180.0f / PI;

// EngineeringTask (MF 09/29/2004) AKMath source disabled until header is working

#ifdef MATTIAS_OFF /////////////////////////////////////////////@#$%^&*(#$%^&*()#$%^&*()#$%&^*()_

const float CMatrix::s_Identity[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

//==============================================================================
//							EULER ANGLES
//==============================================================================
static inline unsigned int GetSafe(unsigned int uiOrder)
{
    static unsigned int aiSafe[] = { 0, 1, 2, 0 };
    return (aiSafe[uiOrder & 0x3]);
}

static inline unsigned int GetNext(unsigned int uiOrder)
{
    static unsigned int aiNext[] = { 1, 2, 0, 1 };
    return (aiNext[uiOrder & 0x3]);
}

static inline void GetOrderData(unsigned int uiOrder, int &i, int &j, int &k, int &h, int &n,
                                int &s, int &f)
{
    f = uiOrder & 0x1;
    uiOrder >>= 1;
    s = uiOrder & 0x1;
    uiOrder >>= 1;
    n = uiOrder & 0x1;
    uiOrder >>= 1;
    i = GetSafe(uiOrder & 0x3);
    j = GetNext(i + n);
    k = GetNext(i + 1 - n);
    h = s ? k : i;
}

CQuaternion &CQuaternion::operator=(const CEulerAngles &inEulerAngles)
{
    // From Ken Shoemake in "Graphics Gems IV", Academic Press, 1994
    float fAngle[3] = { inEulerAngles.m_Angle.x, inEulerAngles.m_Angle.y, inEulerAngles.m_Angle.z };
    float a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;

    GetOrderData(inEulerAngles.m_Order, i, j, k, h, n, s, f);

    if (f == CEulerAngles::ROTATEFRAME) {
        float t = fAngle[0];
        fAngle[0] = fAngle[2];
        fAngle[2] = t;
    }

    if (n == CEulerAngles::ODD) {
        fAngle[1] = -fAngle[1];
    }

    ti = fAngle[0] * 0.5f;
    tj = fAngle[1] * 0.5f;
    th = fAngle[2] * 0.5f;

    ci = ::cos(ti);
    cj = ::cos(tj);
    ch = ::cos(th);

    si = ::sin(ti);
    sj = ::sin(tj);
    sh = ::sin(th);

    cc = ci * ch;
    cs = ci * sh;
    sc = si * ch;
    ss = si * sh;

    if (s == CEulerAngles::REPEAT) {
        a[i] = cj * (cs + sc);
        a[j] = sj * (cc + ss);
        a[k] = sj * (cs - sc);
        qw = cj * (cc - ss);
    } else {
        a[i] = cj * sc - sj * cs;
        a[j] = cj * ss + sj * cc;
        a[k] = cj * cs - sj * sc;
        qw = cj * cc + sj * ss;
    }

    if (n == CEulerAngles::ODD)
        a[j] = -a[j];

    qx = a[CEulerAngles::X];
    qy = a[CEulerAngles::Y];
    qz = a[CEulerAngles::Z];

    return Normalize();
}

CEulerAngles CQuaternion::ToEulerAngles(long inOrder) const
{
    // From Ken Shoemake in "Graphics Gems IV", Academic Press, 1994
    CMatrix kM;
    int i, j, k, h, n, s, f;
    CVector3 kAngles;

    kM.SetRotate(*this);
    GetOrderData(inOrder, i, j, k, h, n, s, f);

    if (s == CEulerAngles::REPEAT) {
        float sy = ::sqrtf(kM[i][j] * kM[i][j] + kM[i][k] * kM[i][k]);

        if (sy > EPSILON) {
            kAngles.x = ::atan2(kM[i][j], kM[i][k]);
            kAngles.y = ::atan2(sy, kM[i][i]);
            kAngles.z = ::atan2(kM[j][i], -kM[k][i]);
        } else {
            kAngles.x = ::atan2(-kM[j][k], kM[j][j]);
            kAngles.y = ::atan2(sy, kM[i][i]);
            kAngles.z = 0.0f;
        }
    } else {
        float cy = ::sqrt(kM[i][i] * kM[i][i] + kM[j][i] * kM[j][i]);

        if (cy > EPSILON) {
            kAngles.x = ::atan2f(kM[k][j], kM[k][k]);
            kAngles.y = ::atan2f(-kM[k][i], cy);
            kAngles.z = ::atan2f(kM[j][i], kM[i][i]);
        } else {
            kAngles.x = ::atan2f(-kM[j][k], kM[j][j]);
            kAngles.y = ::atan2f(-kM[k][i], cy);
            kAngles.z = 0.0f;
        }
    }

    if (n == CEulerAngles::ODD) {
        kAngles.x = -kAngles.x;
        kAngles.y = -kAngles.y;
        kAngles.z = -kAngles.z;
    }
    if (f == CEulerAngles::ROTATEFRAME) {
        float t = kAngles.x;
        kAngles.x = kAngles.z;
        kAngles.z = t;
    }

    return CEulerAngles(kAngles, inOrder);
}

//==============================================================================
// Stream operators for debugging
//==============================================================================
/*std::ostream &operator << ( std::ostream& inStream, const CVector3& inVector )
{
        inStream << "V( " << inVector.x << ", " << inVector.y << ", " << inVector.z << " )";
        return inStream;
}

std::ostream &operator << ( std::ostream& inStream, const CQuaternion& inQuat )
{
        inStream << "Q( " << inQuat.qx << ", " << inQuat.qy << ", " << inQuat.qz << ", " <<
inQuat.qw << " )";
        return inStream;
}

std::ostream &operator << ( std::ostream& inStream, const CMatrix& inMatrix )
{
        inStream << "M( ( " << inMatrix.m[0][0] << ", " << inMatrix.m[0][1] << ", " <<
inMatrix.m[0][2] << ", " << inMatrix.m[0][3] << " ),"
                 <<  " ( " << inMatrix.m[1][0] << ", " << inMatrix.m[1][1] << ", " <<
inMatrix.m[1][2] << ", " << inMatrix.m[1][3] << " ),"
                 <<  " ( " << inMatrix.m[2][0] << ", " << inMatrix.m[2][1] << ", " <<
inMatrix.m[2][2] << ", " << inMatrix.m[2][3] << " ),"
                 <<  " ( " << inMatrix.m[3][0] << ", " << inMatrix.m[3][1] << ", " <<
inMatrix.m[3][2] << ", " << inMatrix.m[3][3] << " ) )";
        return inStream;
}*/

#endif

}; // namespace Q3DStudio
