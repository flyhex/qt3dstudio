/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_PT_H
#define INCLUDED_PT_H 1

#pragma once

#include <QPoint>

class CPt
{
public:
    CPt();
    CPt(long inX, long inY);
    CPt(const CPt *inPt);
    //	CPt( const CPoint& inPt );

    CPt(const QPoint& qpoint);

    void Offset(const CPt inPt);
    void operator+=(const CPt inPt);
    void operator-=(const CPt inPt);
    CPt operator+(const CPt inPt) const;
    CPt operator-(const CPt inPt) const;
    CPt operator-() const;

    bool operator<(const CPt &inPt) const;
    bool operator>(const CPt &inPt) const;
    bool operator<=(const CPt &inPt) const;
    bool operator>=(const CPt &inPt) const;
    bool operator==(const CPt &inPt) const;
    bool operator!=(const CPt &inPt) const;
    operator QPoint() const;
    //	POINT operator POINT( ) const;

    // sweetness- allows one variable to have 2 names, in here because of the integration of CPoint
    // and CSize, plus it's slick and really annoys edwards.
    //	union
    //	{
    long x;
    //		long cx;
    //	};

    //	union
    //	{
    long y;
    //		long cy;
    //	};
};

//=============================================================================
/**
 * Constructor, initializes to 0, 0.
 */
inline CPt::CPt()
    : x(0)
    , y(0)
{
}

//=============================================================================
/**
 * Constructor.
 * @param inX the x value.
 * @param inY the y value.
 */
inline CPt::CPt(long inX, long inY)
    : x(inX)
    , y(inY)
{
}

//=============================================================================
/**
 * Constructor from another point object.
 * Meant mainly to speed up the + and - operators.
 * @param inPt the point to copy to this.
 */
inline CPt::CPt(const CPt *inPt)
    : x(inPt->x)
    , y(inPt->y)
{
}

/*
inline CPt::CPt( const CPoint& inPoint ): x( inPoint.x ), y( inPoint.y )
{
}
*/

//=============================================================================
/**
 * Offset this point by inPt.
 * This just adds the x and y components of inPt to this.
 * @param inPt the amount to offset this by.
 */
inline void CPt::Offset(const CPt inPt)
{
    x += inPt.x;
    y += inPt.y;
}

//=============================================================================
/**
 * Add the values of inPt to this.
 * This adds the x and y components of inPt to the x and y components of this.
 * @param inPt the values to add to this.
 */
inline void CPt::operator+=(const CPt inPt)
{
    x += inPt.x;
    y += inPt.y;
}

//=============================================================================
/**
 * Subtracts the values of inPt from this.
 * This subtracts the x and y components of inPt from the x and y components of
 * this.
 * @param inPt the values to subtract from this.
 */
inline void CPt::operator-=(const CPt inPt)
{
    x -= inPt.x;
    y -= inPt.y;
}

//=============================================================================
/**
 * Add this and the values of inPt together.
 * This adds the x and y components of this and inPt together and returns the
 * result.
 * @param inPt the value to add to this.
 * @return the result.
 */
inline CPt CPt::operator+(const CPt inPt) const
{
    CPt theRetVal(this);
    theRetVal += inPt;
    return theRetVal;
}

//=============================================================================
/**
 * Subtract the values of inPt from these values and return the result.
 * This subtracts the x and y components of inPt from these values and returns
 * the result.
 * @param inPt the value to subtract from this.
 * @return the result.
 */
inline CPt CPt::operator-(const CPt inPt) const
{
    CPt theRetVal(this);
    theRetVal -= inPt;
    return theRetVal;
}

//=============================================================================
/**
 * Comparator to see if one point is less than another.
 * A point is considered to be smaller only if both it's x and y values are less
 * than the others.
 * @param inPt the point to compare this to.
 * @return true if this is less than inPt.
 */
inline bool CPt::operator<(const CPt &inPt) const
{
    return (x < inPt.x && y < inPt.y);
}

//=============================================================================
/**
 * Comparator to see if one point is greater than another.
 * A point is considered to be larger only if both it's x and y values are
 * greater than the others.
 * @param inPt the point to compare this to.
 * @return true if this is less than inPt.
 */
inline bool CPt::operator>(const CPt &inPt) const
{
    return (x > inPt.x && y > inPt.y);
}

//=============================================================================
/**
 * Comparator to see if one point is less than or equal to another.
 * @see operator <.
 */
inline bool CPt::operator<=(const CPt &inPt) const
{
    return (*this < inPt || (inPt.x == x && inPt.y == y));
}

//=============================================================================
/**
 * Comparator to see if one point is greater than or equal to another.
 * @see operator >.
 */
inline bool CPt::operator>=(const CPt &inPt) const
{
    return (*this > inPt || (inPt.x == x && inPt.y == y));
}

//=============================================================================
/**
 * Comparator to see if two points are equal to each other.
 */
inline bool CPt::operator==(const CPt &inPt) const
{
    return (x == inPt.x && y == inPt.y);
}

//=============================================================================
/**
 * Comparator to see if two points are not equal to each other.
 */
inline bool CPt::operator!=(const CPt &inPt) const
{
    return (x != inPt.x || y != inPt.y);
}

inline CPt CPt::operator-() const
{
    return CPt(-x, -y);
}

inline CPt::operator QPoint() const
{
    return QPoint(x, y);
}
#endif // INCLUDED_PT_H
