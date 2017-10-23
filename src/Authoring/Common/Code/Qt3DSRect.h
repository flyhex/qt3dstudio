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

#ifndef __QT3DS_RECT_H__
#define __QT3DS_RECT_H__

#ifndef __PLATFORMCONVERSION_H__
#include "PlatformConversion.h"
#endif

#include "Qt3DSPoint.h"
#include "Qt3DSRectBase.h"

namespace Q3DStudio {

//==============================================================================
/**
*		@class	CRect
*		@brief	This class contains a Rect
*/
//==============================================================================
class CRect : public CRectBase
{
public:
    // Constructors
    CRect();
    CRect(int l, int t, int r, int b);

    CRect(const CRect &srcRect);
    CRect(const CRect *lpSrcRect);
#ifndef __PLATFORMCONVERSION_H__
    CRect(const Qt3DSRect &inRect);
    CRect(const Qt3DSRect *inRect);
#endif
    CRect(CPoint &topLeft, CPoint &bottomRight);

    // Attributes (in addition to CRect members)
    int Width() const;
    int Height() const;
    CPoint &TopLeft();
    CPoint &BottomRight();
    const CPoint &TopLeft() const;
    const CPoint &BottomRight() const;
    CPoint CenterPoint() const;

    // convert between CRect and CRect*/const CRect* (no need for &)
    operator CRect *();
    operator const CRect *() const;

    bool IsRectEmpty() const;
    bool IsRectNull() const;
    bool PtInRect(CPoint &point) const;

    // Operations
    void SetRect(int x1, int y1, int x2, int y2);
    void SetRect(CPoint &topLeft, CPoint &bottomRight);
    void SetRectEmpty();
    void CopyRect(const CRect *lpSrcRect);
    bool EqualRect(const CRect *lpRect) const;

    void InflateRect(int x, int y);
    void InflateRect(const CRect *lpRect);
    void InflateRect(int l, int t, int r, int b);
    void DeflateRect(int x, int y);
    void DeflateRect(const CRect *lpRect);
    void DeflateRect(int l, int t, int r, int b);

    void OffsetRect(int x, int y);
    void OffsetRect(CPoint &point);
    void NormalizeRect();

    // absolute position of rectangle
    void MoveToY(int y);
    void MoveToX(int x);
    void MoveToXY(int x, int y);
    void MoveToXY(CPoint &point);

    // operations that fill '*this' with result
    bool IntersectRect(const CRect *lpRect1, const CRect *lpRect2);
    bool UnionRect(const CRect *lpRect1, const CRect *lpRect2);
    bool SubtractRect(const CRect *lpRectSrc1, const CRect *lpRectSrc2);

    // Additional Operations
    void operator=(const CRect &srcRect);
#ifndef __PLATFORMCONVERSION_H__
    void operator=(const Qt3DSRect &srcRect);
#endif
    bool operator==(const CRect &rect) const;
    bool operator!=(const CRect &rect) const;
    void operator+=(CPoint &point);
    void operator+=(const CRect *lpRect);
    void operator-=(CPoint &point);
    void operator-=(const CRect *lpRect);
    void operator&=(const CRect &rect);
    void operator|=(const CRect &rect);

    // Operators returning CRect values
    CRect operator+(CPoint &point) const;
    CRect operator-(CPoint &point) const;
    CRect operator+(const CRect *lpRect) const;
    CRect operator-(const CRect *lpRect) const;
    CRect operator&(const CRect &rect2) const;
    CRect operator|(const CRect &rect2) const;
};

// CRect
inline CRect::CRect()
{
}
inline CRect::CRect(int l, int t, int r, int b)
{
    left = l;
    top = t;
    right = r;
    bottom = b;
}
inline CRect::CRect(const CRect &srcRect)
{
    CopyRect(&srcRect);
}
inline CRect::CRect(const CRect *lpSrcRect)
{
    CopyRect(lpSrcRect);
}
#ifndef __PLATFORMCONVERSION_H__
inline CRect::CRect(const Qt3DSRect &inRect)
{
    CPlatformConversion::Qt3DSRectToCRect(inRect, *this);
}
inline CRect::CRect(const Qt3DSRect *inRect)
{
    CPlatformConversion::Qt3DSRectToCRect(*inRect, *this);
}
#endif
inline CRect::CRect(CPoint &topLeft, CPoint &bottomRight)
{
    left = topLeft.x;
    top = topLeft.y;
    right = bottomRight.x;
    bottom = bottomRight.y;
}
inline int CRect::Width() const
{
    return right - left;
}
inline int CRect::Height() const
{
    return bottom - top;
}
inline CPoint &CRect::TopLeft()
{
    return *((CPoint *)this);
}
inline CPoint &CRect::BottomRight()
{
    return *((CPoint *)this + 1);
}
inline const CPoint &CRect::TopLeft() const
{
    return *((CPoint *)this);
}
inline const CPoint &CRect::BottomRight() const
{
    return *((CPoint *)this + 1);
}
inline CPoint CRect::CenterPoint() const
{
    return CPoint((left + right) / 2, (top + bottom) / 2);
}
inline CRect::operator CRect *()
{
    return this;
}
inline CRect::operator const CRect *() const
{
    return this;
}
inline bool CRect::IsRectEmpty() const
{
    return (Width() <= 0 && Height() <= 0);
}
inline bool CRect::IsRectNull() const
{
    return (left == 0 && right == 0 && top == 0 && bottom == 0);
}
inline bool CRect::PtInRect(CPoint &point) const
{
    return (point.x >= left && point.x <= right && point.y >= top && point.y <= bottom);
}
inline void CRect::SetRect(int x1, int y1, int x2, int y2)
{
    left = x1;
    top = y1;
    right = x2;
    bottom = y2;
}
inline void CRect::SetRect(CPoint &topLeft, CPoint &bottomRight)
{
    left = topLeft.x;
    top = topLeft.y;
    right = bottomRight.x;
    bottom = bottomRight.y;
}
inline void CRect::SetRectEmpty()
{
    left = 0;
    top = 0;
    right = 0;
    bottom = 0;
}
inline void CRect::CopyRect(const CRect *lpSrcRect)
{
    left = lpSrcRect->left;
    top = lpSrcRect->top;
    right = lpSrcRect->right;
    bottom = lpSrcRect->bottom;
}
inline bool CRect::EqualRect(const CRect *lpRect) const
{
    return (left == lpRect->left && top == lpRect->top && right == lpRect->right
            && bottom == lpRect->bottom);
}
inline void CRect::InflateRect(int x, int y)
{
    InflateRect(x, y, x, y);
}
inline void CRect::DeflateRect(int x, int y)
{
    InflateRect(-x, -y, -x, -y);
}
inline void CRect::OffsetRect(int x, int y)
{
    left += x;
    top += y;
    right += x;
    bottom += y;
}
inline void CRect::OffsetRect(CPoint &point)
{
    OffsetRect(point.x, point.y);
}
inline void CRect::operator=(const CRect &srcRect)
{
    CopyRect(&srcRect);
}
#ifndef __PLATFORMCONVERSION_H__
inline void CRect::operator=(const Qt3DSRect &srcRect)
{
    CPlatformConversion::Qt3DSRectToCRect(srcRect, *this);
}
#endif
inline bool CRect::operator==(const CRect &rect) const
{
    return EqualRect(&rect);
}
inline bool CRect::operator!=(const CRect &rect) const
{
    return !EqualRect(&rect);
}
inline void CRect::operator+=(CPoint &point)
{
    OffsetRect(point.x, point.y);
}
inline void CRect::operator+=(const CRect *lpRect)
{
    InflateRect(lpRect);
}
inline void CRect::operator-=(CPoint &point)
{
    OffsetRect(-point.x, -point.y);
}
inline void CRect::operator-=(const CRect *lpRect)
{
    DeflateRect(lpRect);
}
inline void CRect::operator&=(const CRect &rect)
{
    IntersectRect(this, &rect);
}
inline void CRect::operator|=(const CRect &rect)
{
    UnionRect(this, &rect);
}
inline CRect CRect::operator+(CPoint &pt) const
{
    CRect rect(*this);
    rect.OffsetRect(pt.x, pt.y);
    return rect;
}
inline CRect CRect::operator-(CPoint &pt) const
{
    CRect rect(*this);
    rect.OffsetRect(-pt.x, -pt.y);
    return rect;
}
inline CRect CRect::operator+(const CRect *lpRect) const
{
    CRect rect(this);
    rect.InflateRect(lpRect);
    return rect;
}
inline CRect CRect::operator-(const CRect *lpRect) const
{
    CRect rect(this);
    rect.DeflateRect(lpRect);
    return rect;
}
inline CRect CRect::operator&(const CRect &rect2) const
{
    CRect rect;
    rect.IntersectRect(this, &rect2);
    return rect;
}
inline CRect CRect::operator|(const CRect &rect2) const
{
    CRect rect;
    rect.UnionRect(this, &rect2);
    return rect;
}

inline void CRect::NormalizeRect()
{
    int nTemp;
    if (left > right) {
        nTemp = left;
        left = right;
        right = nTemp;
    }
    if (top > bottom) {
        nTemp = top;
        top = bottom;
        bottom = nTemp;
    }
}

inline void CRect::MoveToY(int y)
{
    bottom = Height() + y;
    top = y;
}
inline void CRect::MoveToX(int x)
{
    right = Width() + x;
    left = x;
}
inline void CRect::MoveToXY(int x, int y)
{
    MoveToX(x);
    MoveToY(y);
}
inline void CRect::MoveToXY(CPoint &pt)
{
    MoveToX(pt.x);
    MoveToY(pt.y);
}

inline void CRect::InflateRect(const CRect *lpRect)
{
    left -= lpRect->left;
    top -= lpRect->top;
    right += lpRect->right;
    bottom += lpRect->bottom;
}

inline void CRect::InflateRect(int l, int t, int r, int b)
{
    left -= l;
    top -= t;
    right += r;
    bottom += b;
}

inline void CRect::DeflateRect(const CRect *lpRect)
{
    left += lpRect->left;
    top += lpRect->top;
    right -= lpRect->right;
    bottom -= lpRect->bottom;
}

inline void CRect::DeflateRect(int l, int t, int r, int b)
{
    left += l;
    top += t;
    right -= r;
    bottom -= b;
}

} // namespace Q3DStudio

#endif // __QT3DS_RECT_H__
