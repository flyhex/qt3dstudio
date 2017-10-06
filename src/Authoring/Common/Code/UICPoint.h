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

#ifndef __UICPOINT_H__
#define __UICPOINT_H__

#ifndef __PLATFORMCONVERSION_H__
#include "PlatformConversion.h"
#endif

#include "UICPointBase.h"

namespace Q3DStudio {

//==============================================================================
/**
*		@class	CPoint
*		@brief	This class contains a Point
*/
//==============================================================================
class CPoint : public CPointBase
{
public:
    // Constructors
    CPoint();
    CPoint(int initX, int initY);
    CPoint(const CPoint &initPt);
#ifndef __PLATFORMCONVERSION_H__
    CPoint(const UICPoint &initPt);
    CPoint(const UICPoint *initPt);
#endif

    // Operations
    void Offset(int xOffset, int yOffset);
    void Offset(CPoint &point);
#ifndef __PLATFORMCONVERSION_H__
    void operator=(const UICPoint &srcPt);
#endif
    bool operator==(CPoint &point) const;
    bool operator!=(CPoint &point) const;
    void operator+=(CPoint &point);
    void operator-=(CPoint &point);
    void SetPoint(int X, int Y);

    // Operators returning CPoint values
    CPoint operator-() const;
    CPoint operator+(CPoint &point) const;
};

// CPoint
inline CPoint::CPoint()
{
}
inline CPoint::CPoint(int initX, int initY)
{
    x = initX;
    y = initY;
}
inline CPoint::CPoint(const CPoint &initPt)
{
    *(CPoint *)this = initPt;
}
#ifndef __PLATFORMCONVERSION_H__
inline CPoint::CPoint(const UICPoint &initPt)
{
    CPlatformConversion::UICPointToCPoint(initPt, *this);
}
inline CPoint::CPoint(const UICPoint *initPt)
{
    CPlatformConversion::UICPointToCPoint(*initPt, *this);
}
#endif
inline void CPoint::Offset(int xOffset, int yOffset)
{
    x += xOffset;
    y += yOffset;
}
inline void CPoint::Offset(CPoint &point)
{
    x += point.x;
    y += point.y;
}
#ifndef __PLATFORMCONVERSION_H__
inline void CPoint::operator=(const UICPoint &srcPt)
{
    CPlatformConversion::UICPointToCPoint(srcPt, *this);
}
#endif
inline bool CPoint::operator==(CPoint &point) const
{
    return (x == point.x && y == point.y);
}
inline bool CPoint::operator!=(CPoint &point) const
{
    return (x != point.x || y != point.y);
}
inline void CPoint::operator+=(CPoint &point)
{
    x += point.x;
    y += point.y;
}
inline void CPoint::operator-=(CPoint &point)
{
    x -= point.x;
    y -= point.y;
}
inline void CPoint::SetPoint(int X, int Y)
{
    x = X;
    y = Y;
}
inline CPoint CPoint::operator-() const
{
    return CPoint(-x, -y);
}
inline CPoint CPoint::operator+(CPoint &point) const
{
    return CPoint(x + point.x, y + point.y);
}

} // namespace Q3DStudio

#endif // __UICPOINT_H__
