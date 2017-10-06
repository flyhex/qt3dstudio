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

#ifndef INCLUDED_RCT_H
#define INCLUDED_RCT_H 1

#pragma once

#include "Pt.h"

#include <QRect>

#ifdef WIN32
#include <Windows.h>
#endif

class CRct
{
public:
    CRct();
    CRct(long inX, long inY, long inWidth, long inHeight);
    CRct(const CPt &inPos, const CPt &inSize);
    CRct(const CPt &inSize);
    CRct(const QRect &inRect);
#ifdef WIN32
    CRct(const RECT &inRect);
    operator RECT() const;
    CRct &operator=(const RECT &inRect);
#endif

    void Offset(const CPt &inOffset);

    bool IsInRect(const CPt &inPoint);

    void And(const CRct &inRect);
    void Or(const CRct &inRect);

    void Normalize();

    static long LesserOfTwo(const long &inA, const long &inB);
    static long GreaterOfTwo(const long &inA, const long &inB);

    operator QRect() const;

    CPt position;
    CPt size;
};

//=============================================================================
/**
 * Constructs a rect of points 0,0,0,0.
 */
inline CRct::CRct()
{
}

//=============================================================================
/**
 * Constructs a rect of the points.
 * @param inX the X position of the top left of the rectangle.
 * @param inY the Y position of the top left of the rectangle.
 * @param inWidth the width of the rectangle.
 * @param inHeight the height of the rectangle.
 */
inline CRct::CRct(long inX, long inY, long inWidth, long inHeight)
    : position(inX, inY)
    , size(inWidth, inHeight)
{
}

//=============================================================================
/**
 * Constructs a rect of the position and size.
 * @param inPos the position of the top left of the rectangle.
 * @param inSize the width and height of the rectangle.
 */
inline CRct::CRct(const CPt &inPos, const CPt &inSize)
    : position(inPos)
    , size(inSize)
{
}

//=============================================================================
/**
 * Constructs a rectangle at 0,0 with size inSize.
 * @param inSize the width and height of the rectangle.
 */
inline CRct::CRct(const CPt &inSize)
    : size(inSize)
{
}

inline CRct::CRct(const QRect &inRect)
    : position(inRect.x(), inRect.y())
    , size(inRect.width(), inRect.height())
{
}

#ifdef WIN32
inline CRct::CRct(const RECT &inRect)
    : position(inRect.left, inRect.top)
    , size(inRect.right - inRect.left, inRect.bottom - inRect.top)
{
}

inline CRct::operator RECT() const
{
    RECT theRect = { position.x, position.y, position.x + size.x, position.y + size.y };
    return theRect;
}

inline CRct &CRct::operator=(const RECT &inRect)
{
    position.x = inRect.left;
    position.y = inRect.top;
    size.x = inRect.right - inRect.left;
    size.y = inRect.bottom - inRect.top;

    return *this;
}
#endif // _WIN32

//=============================================================================
/**
 * Offset the rectangle.
 */
inline void CRct::Offset(const CPt &inOffset)
{
    position += inOffset;
    //	size += inOffset;
}

//=============================================================================
/**
 * @return true if the point is in this rectangle.
 */
inline bool CRct::IsInRect(const CPt &inPoint)
{
    if (inPoint.x >= position.x && inPoint.x < (position.x + size.x) && inPoint.y >= position.y
        && inPoint.y < (position.y + size.y))
        return true;
    return false;
}

//=============================================================================
/**
 * Ease method to return the larger of the two numbers.
 * @param inA the first number to check.
 * @param inB the second number to check.
 * @return the larger of the two numbers.
 */
inline long CRct::GreaterOfTwo(const long &inA, const long &inB)
{
    return (inA > inB) ? inA : inB;
}

//=============================================================================
/**
 * Ease method to return the smaller of the two numbers.
 * @param inA the first number to check.
 * @param inB the second number to check.
 * @return the smaller of the two numbers.
 */
inline long CRct::LesserOfTwo(const long &inA, const long &inB)
{
    return (inA < inB) ? inA : inB;
}

//=============================================================================
/**
 * Combine inRect with this rect, setting this rect to the union of the two.
 * This will modify this rect and make it only the area that exists in both
 * rects.
 * @param inRect the rect to merge with this one.
 */
inline void CRct::And(const CRct &inRect)
{
    long theXPos = GreaterOfTwo(position.x, inRect.position.x);
    long theXSize = LesserOfTwo(position.x + size.x, inRect.position.x + inRect.size.x) - theXPos;
    theXSize = GreaterOfTwo(0, theXSize);

    long theYPos = GreaterOfTwo(position.y, inRect.position.y);
    long theYSize = LesserOfTwo(position.y + size.y, inRect.position.y + inRect.size.y) - theYPos;
    theYSize = GreaterOfTwo(0, theYSize);

    position.x = theXPos;
    position.y = theYPos;
    size.x = theXSize;
    size.y = theYSize;
}

//=============================================================================
/**
 * Combine inRect with this rect, making this the smallest rect containing both.
 * This will modify this rect to make it the smallest possible rect that
 * contains the entire area of both rects. If this rect's size is 0 then it
 * this will be set only to inRect.
 * @param inRect the rect to merge with this.
 */
inline void CRct::Or(const CRct &inRect)
{
    long theXPos;
    long theXSize;
    if (size.x == 0) {
        theXPos = inRect.position.x;
        theXSize = inRect.size.x;
    } else {
        theXPos = LesserOfTwo(position.x, inRect.position.x);
        theXSize = GreaterOfTwo(position.x + size.x, inRect.position.x + inRect.size.x) - theXPos;
    }

    long theYPos;
    long theYSize;
    if (size.y == 0) {
        theYPos = inRect.position.y;
        theYSize = inRect.size.y;
    } else {
        theYPos = LesserOfTwo(position.y, inRect.position.y);
        theYSize = GreaterOfTwo(position.y + size.y, inRect.position.y + inRect.size.y) - theYPos;
    }

    position.x = theXPos;
    position.y = theYPos;
    size.x = theXSize;
    size.y = theYSize;
}

//=============================================================================
/**
 * Normalize this rectangle so that it's size is positive.
 * If the size is negative in either direction then this will modify the
 * position and the size so that the size becomes positive. The position will
 * be modified so this will still cover the same area.
 */
inline void CRct::Normalize()
{
    if (size.x < 0) {
        position.x += size.x;
        size.x = -size.x;
    }
    if (size.y < 0) {
        position.y += size.y;
        size.y = -size.y;
    }
}

inline CRct::operator QRect() const
{
    return QRect(position.x, position.y, size.x, size.y);
}

#endif // INCLUDED_RCT_H
